#include "http_server.h"

#include "http_parser.h"
#include "logger.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr std::size_t BUFFER_SIZE = 4096;
constexpr std::size_t MAX_REQUEST_SIZE = 1024 * 1024; // 1 MiB

std::size_t parse_content_length(const std::string& headers) {
    const std::string key = "Content-Length:";
    const auto pos = headers.find(key);
    if (pos == std::string::npos) return 0;

    const auto line_end = headers.find("\r\n", pos);
    const std::string value = headers.substr(pos + key.size(), line_end - (pos + key.size()));

    try {
        return static_cast<std::size_t>(std::stoul(value));
    } catch (...) {
        return 0;
    }
}
}

HttpServer::HttpServer(std::uint16_t port, std::size_t worker_count, std::string static_root)
    : port_(port), thread_pool_(worker_count), static_root_(std::move(static_root)) {}

HttpServer::~HttpServer() {
    stop();
}

Router& HttpServer::router() {
    return router_;
}

void HttpServer::start() {
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));
    }

    int opt = 1;
    if (::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        ::close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("setsockopt() failed");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        const std::string error = std::strerror(errno);
        ::close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("bind() failed: " + error);
    }

    if (::listen(server_fd_, SOMAXCONN) < 0) {
        const std::string error = std::strerror(errno);
        ::close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("listen() failed: " + error);
    }

    running_ = true;
    Logger::instance().log("Server listening on 0.0.0.0:" + std::to_string(port_));

    while (running_) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);

        const int client_fd = ::accept(
            server_fd_,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_len
        );

        if (client_fd < 0) {
            if (!running_) break;
            if (errno == EINTR) continue;
            Logger::instance().log("accept() failed: " + std::string(std::strerror(errno)));
            continue;
        }

        char ip_buffer[INET_ADDRSTRLEN]{};
        ::inet_ntop(AF_INET, &client_address.sin_addr, ip_buffer, sizeof(ip_buffer));
        std::string client_ip = ip_buffer;

        try {
            thread_pool_.enqueue([this, client_fd, client_ip = std::move(client_ip)] {
                handle_client(client_fd, client_ip);
            });
        } catch (...) {
            ::close(client_fd);
        }
    }
}

void HttpServer::stop() {
    const bool was_running = running_.exchange(false);

    if (server_fd_ >= 0) {
        ::shutdown(server_fd_, SHUT_RDWR);
        ::close(server_fd_);
        server_fd_ = -1;
    }

    if (was_running) {
        Logger::instance().log("Server stopping...");
    }

    thread_pool_.stop();
}

bool HttpServer::read_request(int client_fd, std::string& raw_request) {
    raw_request.clear();
    char buffer[BUFFER_SIZE];
    std::size_t expected_total = 0;

    while (raw_request.size() < MAX_REQUEST_SIZE) {
        const ssize_t bytes_read = ::recv(client_fd, buffer, sizeof(buffer), 0);

        if (bytes_read == 0) return false;
        if (bytes_read < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        raw_request.append(buffer, static_cast<std::size_t>(bytes_read));

        const std::size_t header_end = raw_request.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            if (expected_total == 0) {
                const std::size_t content_length = parse_content_length(raw_request.substr(0, header_end));
                expected_total = header_end + 4 + content_length;
            }

            if (raw_request.size() >= expected_total) {
                return true;
            }
        }
    }

    return false;
}

bool HttpServer::send_all(int client_fd, const std::string& data) {
    std::size_t total_sent = 0;

    while (total_sent < data.size()) {
        const ssize_t sent = ::send(
            client_fd,
            data.data() + total_sent,
            data.size() - total_sent,
            MSG_NOSIGNAL
        );

        if (sent < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        total_sent += static_cast<std::size_t>(sent);
    }

    return true;
}

void HttpServer::handle_client(int client_fd, std::string client_ip) {
    std::string raw_request;
    HttpResponse response;
    HttpRequest request;

    if (!read_request(client_fd, raw_request)) {
        ::close(client_fd);
        return;
    }

    if (!HttpParser::parse(raw_request, request)) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.headers["Content-Type"] = "application/json";
        response.body = R"({"error":"invalid HTTP request"})";
    } else {
        response = router_.route(request);

        if (response.status_code == 404 && request.method == "GET" && request.path.rfind("/static/", 0) == 0) {
            response = serve_static_file(request);
        }
    }

    send_all(client_fd, response.serialize());

    if (!request.method.empty()) {
        Logger::instance().log(
            client_ip + " \"" + request.method + " " + request.path + "\" " +
            std::to_string(response.status_code)
        );
    }

    ::shutdown(client_fd, SHUT_RDWR);
    ::close(client_fd);
}

HttpResponse HttpServer::serve_static_file(const HttpRequest& request) const {
    HttpResponse response;

    std::string relative_path = request.path.substr(std::string("/static/").size());
    if (relative_path.empty() || relative_path.find("..") != std::string::npos) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.headers["Content-Type"] = "text/plain";
        response.body = "Invalid path\n";
        return response;
    }

    const std::string full_path = static_root_ + "/" + relative_path;
    std::ifstream file(full_path, std::ios::binary);

    if (!file) {
        response.status_code = 404;
        response.status_text = "Not Found";
        response.headers["Content-Type"] = "text/plain";
        response.body = "File not found\n";
        return response;
    }

    std::ostringstream contents;
    contents << file.rdbuf();

    response.status_code = 200;
    response.status_text = "OK";
    response.headers["Content-Type"] = content_type_for_path(relative_path);
    response.body = contents.str();
    return response;
}

std::string HttpServer::content_type_for_path(const std::string& path) {
    const auto dot = path.rfind('.');
    if (dot == std::string::npos) return "application/octet-stream";

    const std::string ext = path.substr(dot + 1);
    if (ext == "html" || ext == "htm") return "text/html; charset=utf-8";
    if (ext == "css") return "text/css; charset=utf-8";
    if (ext == "js") return "application/javascript; charset=utf-8";
    if (ext == "json") return "application/json";
    if (ext == "txt") return "text/plain; charset=utf-8";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "svg") return "image/svg+xml";
    return "application/octet-stream";
}
