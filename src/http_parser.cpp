#include "http_parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
std::string trim(std::string value) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}
}

bool HttpParser::parse(const std::string& raw_request, HttpRequest& request) {
    const std::size_t header_end = raw_request.find("\r\n\r\n");
    if (header_end == std::string::npos) return false;

    const std::string header_block = raw_request.substr(0, header_end);
    request.body = raw_request.substr(header_end + 4);

    std::istringstream stream(header_block);
    std::string request_line;
    if (!std::getline(stream, request_line)) return false;

    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::istringstream request_line_stream(request_line);
    if (!(request_line_stream >> request.method >> request.path >> request.version)) {
        return false;
    }

    request.headers.clear();
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        request.headers[std::move(key)] = std::move(value);
    }

    return true;
}
