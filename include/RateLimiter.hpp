#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

class RateLimiter {
public:
    RateLimiter(int maxRequests, int windowSeconds);
    bool allow(const std::string& clientIp);

private:
    struct Bucket {
        int count;
        std::chrono::steady_clock::time_point windowStart;
    };
    std::unordered_map<std::string, Bucket> buckets;
    std::mutex mapMutex;
    int maxRequests;
    int windowSeconds;
};
