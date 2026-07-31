#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RateLimiter.hpp"
#include "Router.hpp"
#include "Server.hpp"
#include "ThreadPool.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

namespace {

std::string currentIsoTime() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

}  // namespace

int main() {
    Router router;
    router.add("GET", "/", [](const HttpRequest&) {
        return HttpResponse::ok("Multithreaded Backend Server is running.");
    });
    router.add("GET", "/health", [](const HttpRequest&) {
        return HttpResponse::json(R"({"status":"ok"})");
    });
    router.add("GET", "/time", [](const HttpRequest&) {
        return HttpResponse::json("{\"time\":\"" + currentIsoTime() + "\"}");
    });
    router.add("POST", "/echo", [](const HttpRequest& req) {
        return HttpResponse::json("{\"echo\":\"" + escapeJson(req.body) + "\"}");
    });

    // Change THREAD_POOL_SIZE between load-test runs (Phase 6: 4 then 8).
    constexpr size_t THREAD_POOL_SIZE = 8;
    // High for load testing; set to 10 for Phase 5 rate-limit demo.
    constexpr int RATE_LIMIT_MAX = 10000000;
    constexpr int RATE_LIMIT_WINDOW_SEC = 60;

    ThreadPool pool(THREAD_POOL_SIZE);
    RateLimiter limiter(RATE_LIMIT_MAX, RATE_LIMIT_WINDOW_SEC);
    Server server(8080, pool, limiter, router);
    server.run();
    return 0;
}
