#pragma once

#include "http_request.h"
#include <string>

class HttpParser {
public:
    static bool parse(const std::string& raw_request, HttpRequest& request);
};
