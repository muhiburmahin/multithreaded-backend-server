#include "Server.hpp"

Server::Server(int port, ThreadPool& pool, RateLimiter& limiter)
    : port(port), pool(pool), limiter(limiter) {}

void Server::run() {}

void Server::stop() {}

void Server::handleConnection(int /*clientFd*/, const std::string& /*ip*/) {}
