#include "RateLimiter.hpp"
#include "Server.hpp"
#include "ThreadPool.hpp"

int main() {
    ThreadPool pool(1);
    RateLimiter limiter(1000, 60);
    Server server(8080, pool, limiter);
    server.run();
    return 0;
}
