#pragma once

#include <string>

class HttpResponse {
public:
    static std::string ok(const std::string& body,
                          const std::string& contentType = "text/plain");
    static std::string json(const std::string& jsonBody);
    static std::string tooManyRequests();
    static std::string notFound();
    static std::string badRequest();
};
