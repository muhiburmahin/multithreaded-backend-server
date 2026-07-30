#include "HttpRequest.hpp"

#include <cctype>
#include <sstream>
#include <string>

namespace {

std::string toLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

}  // namespace

HttpRequest HttpRequest::parse(const std::string& raw) {
    HttpRequest req;
    if (raw.empty()) {
        return req;
    }

    const std::size_t headerEnd = raw.find("\r\n\r\n");
    const std::string headerPart =
        (headerEnd == std::string::npos) ? raw : raw.substr(0, headerEnd);

    std::istringstream stream(headerPart);
    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        return req;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    {
        std::istringstream line(requestLine);
        line >> req.method >> req.path >> req.version;
    }

    std::size_t contentLength = 0;
    std::string headerLine;
    while (std::getline(stream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        if (headerLine.empty()) {
            continue;
        }

        const auto colon = headerLine.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string name = toLower(headerLine.substr(0, colon));
        std::string value = headerLine.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }
        if (name == "content-length") {
            try {
                contentLength = static_cast<std::size_t>(std::stoul(value));
            } catch (...) {
                contentLength = 0;
            }
        }
    }

    if (headerEnd == std::string::npos || contentLength == 0) {
        return req;
    }

    const std::size_t bodyStart = headerEnd + 4;
    if (bodyStart >= raw.size()) {
        return req;
    }
    const std::size_t available = raw.size() - bodyStart;
    req.body = raw.substr(bodyStart, std::min(contentLength, available));
    return req;
}
