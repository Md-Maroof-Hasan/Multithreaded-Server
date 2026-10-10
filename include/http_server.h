#pragma once

#include "router.h"
#include "thread_pool.h"
#include <atomic>
#include <cstdint>
#include <string>

class HttpServer {
public:
    HttpServer(std::uint16_t port, std::size_t worker_count, std::string static_root = "public");
    ~HttpServer();

    Router& router();
    void start();
    void stop();

private:
    void handle_client(int client_fd, std::string client_ip);
    bool read_request(int client_fd, std::string& raw_request);
    bool send_all(int client_fd, const std::string& data);
    HttpResponse serve_static_file(const HttpRequest& request) const;
    static std::string content_type_for_path(const std::string& path);

    std::uint16_t port_;
    int server_fd_ = -1;
    std::atomic<bool> running_{false};
    ThreadPool thread_pool_;
    Router router_;
    std::string static_root_;
};
