#include "http_response.h"

#include <sstream>

std::string HttpResponse::serialize() const {
    std::ostringstream out;
    out << "HTTP/1.1 " << status_code << ' ' << status_text << "\r\n";

    bool has_content_length = false;
    bool has_connection = false;

    for (const auto& [key, value] : headers) {
        if (key == "Content-Length") has_content_length = true;
        if (key == "Connection") has_connection = true;
        out << key << ": " << value << "\r\n";
    }

    if (!has_content_length) {
        out << "Content-Length: " << body.size() << "\r\n";
    }

    if (!has_connection) {
        out << "Connection: close\r\n";
    }

    out << "\r\n";
    out << body;
    return out.str();
}
