#include "http_server.h"
#include "logger.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
HttpServer* g_server = nullptr;

void handle_signal(int) {
    if (g_server) {
        g_server->stop();
    }
}
}

int main(int argc, char* argv[]) {
    std::uint16_t port = 8080;
    std::size_t workers = std::thread::hardware_concurrency();
    if (workers == 0) workers = 4;

    if (argc >= 2) {
        port = static_cast<std::uint16_t>(std::stoi(argv[1]));
    }
    if (argc >= 3) {
        workers = static_cast<std::size_t>(std::stoul(argv[2]));
    }

    try {
        HttpServer server(port, workers, "public");
        g_server = &server;

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        server.router().add_route("GET", "/", [](const HttpRequest&) {
            HttpResponse response;
            response.headers["Content-Type"] = "text/html; charset=utf-8";
            response.body =
                "<!doctype html><html><head><title>C++ HTTP Server</title></head>"
                "<body><h1>Multithreaded C++ HTTP Server</h1>"
                "<p>Try <code>/hello</code>, <code>/health</code>, "
                "<code>POST /echo</code>, or <code>/static/index.html</code>.</p>"
                "</body></html>";
            return response;
        });

        server.router().add_route("GET", "/hello", [](const HttpRequest&) {
            HttpResponse response;
            response.headers["Content-Type"] = "application/json";
            response.body = R"({"message":"Hello from C++!"})";
            return response;
        });

        server.router().add_route("GET", "/health", [](const HttpRequest&) {
            HttpResponse response;
            response.headers["Content-Type"] = "application/json";
            response.body = R"({"status":"ok"})";
            return response;
        });

        server.router().add_route("POST", "/echo", [](const HttpRequest& request) {
            HttpResponse response;
            response.headers["Content-Type"] = "text/plain; charset=utf-8";
            response.body = request.body;
            return response;
        });

        Logger::instance().log("Worker threads: " + std::to_string(workers));
        server.start();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
