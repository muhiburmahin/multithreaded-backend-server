#include "RateLimiter.hpp"

RateLimiter::RateLimiter(int maxRequests, int windowSeconds)
    : maxRequests(maxRequests), windowSeconds(windowSeconds) {}

bool RateLimiter::allow(const std::string& /*clientIp*/) { return true; }
