#include "Router.hpp"

#include "HttpResponse.hpp"

void Router::add(const std::string& method, const std::string& path, Handler handler) {
    routes[method + " " + path] = std::move(handler);
}

std::string Router::dispatch(const HttpRequest& req) const {
    if (req.method.empty() || req.path.empty()) {
        return HttpResponse::badRequest();
    }

    const auto it = routes.find(req.method + " " + req.path);
    if (it == routes.end()) {
        return HttpResponse::notFound();
    }
    return it->second(req);
}
