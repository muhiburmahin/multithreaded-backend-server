#include "Router.hpp"

void Router::add(const std::string& /*method*/, const std::string& /*path*/,
                 Handler /*handler*/) {}

std::string Router::dispatch(const HttpRequest& /*req*/) const { return {}; }
