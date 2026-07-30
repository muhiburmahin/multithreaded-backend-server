#include "HttpResponse.hpp"

#include <string>

namespace {

std::string build(int statusCode, const std::string& reason, const std::string& body,
                  const std::string& contentType) {
    std::string out;
    out.reserve(128 + body.size());
    out += "HTTP/1.1 ";
    out += std::to_string(statusCode);
    out += " ";
    out += reason;
    out += "\r\n";
    out += "Content-Type: ";
    out += contentType;
    out += "\r\n";
    out += "Content-Length: ";
    out += std::to_string(body.size());
    out += "\r\n";
    out += "Connection: close\r\n";
    out += "\r\n";
    out += body;
    return out;
}

}  // namespace

std::string HttpResponse::ok(const std::string& body, const std::string& contentType) {
    return build(200, "OK", body, contentType);
}

std::string HttpResponse::json(const std::string& jsonBody) {
    return build(200, "OK", jsonBody, "application/json");
}

std::string HttpResponse::tooManyRequests() {
    return build(429, "Too Many Requests", "Too Many Requests", "text/plain");
}

std::string HttpResponse::notFound() {
    return build(404, "Not Found", "Not Found", "text/plain");
}

std::string HttpResponse::badRequest() {
    return build(400, "Bad Request", "Bad Request", "text/plain");
}
