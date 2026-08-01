#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "RateLimiter.hpp"
#include "Router.hpp"
#include "Server.hpp"
#include "ThreadPool.hpp"

#include <csignal>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

Server* g_server = nullptr;

void onSigInt(int) {
    if (g_server != nullptr) {
        g_server->stop();
    }
}

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

    constexpr size_t THREAD_POOL_SIZE = 8;
    constexpr int RATE_LIMIT_MAX = 10000000;
    constexpr int RATE_LIMIT_WINDOW_SEC = 60;

    Logger logger;
    ThreadPool pool(THREAD_POOL_SIZE);
    RateLimiter limiter(RATE_LIMIT_MAX, RATE_LIMIT_WINDOW_SEC);
    Server server(8080, pool, limiter, router, logger);

    g_server = &server;
    std::signal(SIGINT, onSigInt);

    server.run();  // returns after stop(); ThreadPool destructor joins workers
    g_server = nullptr;
    std::cout << "Shutdown complete." << std::endl;
    return 0;
}
