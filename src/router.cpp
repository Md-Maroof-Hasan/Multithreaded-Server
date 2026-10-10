#include "router.h"

#include <utility>

std::string Router::make_key(const std::string& method, const std::string& path) {
    return method + " " + path;
}

void Router::add_route(const std::string& method, const std::string& path, Handler handler) {
    routes_[make_key(method, path)] = std::move(handler);
}

HttpResponse Router::route(const HttpRequest& request) const {
    const auto it = routes_.find(make_key(request.method, request.path));
    if (it != routes_.end()) {
        return it->second(request);
    }

    HttpResponse response;
    response.status_code = 404;
    response.status_text = "Not Found";
    response.headers["Content-Type"] = "application/json";
    response.body = R"({"error":"route not found"})";
    return response;
}
