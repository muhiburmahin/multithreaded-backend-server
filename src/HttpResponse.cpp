#include "HttpResponse.hpp"

std::string HttpResponse::ok(const std::string& /*body*/,
                             const std::string& /*contentType*/) {
    return {};
}

std::string HttpResponse::json(const std::string& /*jsonBody*/) { return {}; }

std::string HttpResponse::tooManyRequests() { return {}; }

std::string HttpResponse::notFound() { return {}; }

std::string HttpResponse::badRequest() { return {}; }
