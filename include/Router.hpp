#pragma once

#include "HttpRequest.hpp"

#include <functional>
#include <string>
#include <unordered_map>

class Router {
public:
    using Handler = std::function<std::string(const HttpRequest&)>;

    void add(const std::string& method, const std::string& path, Handler handler);
    std::string dispatch(const HttpRequest& req) const;

private:
    std::unordered_map<std::string, Handler> routes;
};
