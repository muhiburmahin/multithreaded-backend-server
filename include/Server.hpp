#pragma once

#include "RateLimiter.hpp"
#include "Router.hpp"
#include "ThreadPool.hpp"

#include <atomic>
#include <string>

class Server {
public:
    Server(int port, ThreadPool& pool, RateLimiter& limiter, Router& router);
    void run();
    void stop();

private:
    int port;
    int listenFd{-1};
    std::atomic<bool> running{true};
    ThreadPool& pool;
    RateLimiter& limiter;
    Router& router;
    void handleConnection(int clientFd, const std::string& ip);
};
