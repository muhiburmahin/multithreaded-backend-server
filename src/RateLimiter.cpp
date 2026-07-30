#include "RateLimiter.hpp"

RateLimiter::RateLimiter(int maxRequests, int windowSeconds)
    : maxRequests(maxRequests), windowSeconds(windowSeconds) {}

bool RateLimiter::allow(const std::string& clientIp) {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mapMutex);

    auto& bucket = buckets[clientIp];
    if (bucket.windowStart.time_since_epoch().count() == 0 ||
        now - bucket.windowStart > std::chrono::seconds(windowSeconds)) {
        bucket.count = 0;
        bucket.windowStart = now;
    }

    ++bucket.count;
    return bucket.count <= maxRequests;
}
