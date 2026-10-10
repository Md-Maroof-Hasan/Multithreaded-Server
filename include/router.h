#pragma once

#include "http_request.h"
#include "http_response.h"
#include <functional>
#include <string>
#include <unordered_map>

class Router {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    void add_route(const std::string& method, const std::string& path, Handler handler);
    HttpResponse route(const HttpRequest& request) const;

private:
    std::unordered_map<std::string, Handler> routes_;
    static std::string make_key(const std::string& method, const std::string& path);
};
