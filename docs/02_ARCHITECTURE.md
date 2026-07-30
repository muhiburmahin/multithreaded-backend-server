# 02 — System Design & Architecture

Target: **Linux only**. POSIX sockets, no Windows portability.

## 1. Component diagram

```
Main Thread: socket()-bind()-listen() → accept() loop
    → push Task (client fd) into Task Queue (mutex)
    → condition_variable::notify_one()
Worker-1..N: wait → pop task → rate-limit → parse HTTP → respond → close(fd)
```

N = `std::thread::hardware_concurrency()` by default.

## 2. Linux socket lifecycle

`Server::run()`: `socket` → `SO_REUSEADDR` → `bind` → `listen(128)` → `accept` loop → `pool.enqueue(handleConnection)`.

Gotchas:
1. `signal(SIGPIPE, SIG_IGN)` at startup — else client disconnect kills process.
2. Always `close(fd)` on every path; raise `ulimit -n` for load tests if needed.

## 3. STL ↔ Linux

- `std::thread` → `pthread_create` (link `-pthread` / `Threads::Threads`)
- `std::mutex` / `condition_variable` → pthread mutex/cond
- `hardware_concurrency()` → CPU count from `/proc` / cgroup

## 4. Components

| Component | Responsibility | Thread |
|---|---|---|
| Listener | accept loop, enqueue fd | Main |
| Task Queue | queue + mutex + cv | Shared |
| Thread Pool | N workers, `workerLoop()` | Workers |
| Rate Limiter | per-IP counter + mutex | Workers |
| HTTP Layer | parse / build response | Worker (no shared state) |
| Router | method+path → handler | Worker (read-only routes) |
| Logger | thread-safe log + mutex | Workers |

## 5. Class interfaces

```cpp
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);
    size_t activeThreads() const;
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};
    std::atomic<size_t> busyCount{0};
    void workerLoop();
};

class Server {
public:
    Server(int port, ThreadPool& pool, RateLimiter& limiter);
    void run();
    void stop();
private:
    int port, listenFd;
    std::atomic<bool> running{true};
    ThreadPool& pool;
    RateLimiter& limiter;
    void handleConnection(int clientFd, const std::string& ip);
};

class RateLimiter {
public:
    RateLimiter(int maxRequests, int windowSeconds);
    bool allow(const std::string& clientIp);
private:
    struct Bucket { int count; std::chrono::steady_clock::time_point windowStart; };
    std::unordered_map<std::string, Bucket> buckets;
    std::mutex mapMutex;
    int maxRequests, windowSeconds;
};

struct HttpRequest {
    std::string method, path, version, body;
    static HttpRequest parse(const std::string& raw);
};

class HttpResponse {
public:
    static std::string ok(const std::string& body, const std::string& contentType = "text/plain");
    static std::string json(const std::string& jsonBody);
    static std::string tooManyRequests();
    static std::string notFound();
    static std::string badRequest();
};

class Router {
public:
    using Handler = std::function<std::string(const HttpRequest&)>;
    void add(const std::string& method, const std::string& path, Handler handler);
    std::string dispatch(const HttpRequest& req) const;
private:
    std::unordered_map<std::string, Handler> routes; // key = "METHOD /path"
};
```

Endpoints registered in `main.cpp` at startup: `GET /`, `GET /health`, `GET /time`, `POST /echo`.

## 6. Request lifecycle (worker)

1. Wait on cv → pop task
2. `limiter.allow(ip)` → else 429 + close
3. `recv` → parse → `router.dispatch`
4. `logger.log(...)`
5. `send` + `close`

## 7. Race rules

- One mutex per shared object (queue, rate map, logger)
- Prefer `lock_guard` / `unique_lock`
- Never hold lock across `send`/`recv`/`accept`
- Verify with TSAN: `-fsanitize=thread`

## 8. Rate limiting — Fixed Window Counter

Reset bucket when window elapsed; increment count; allow if `count <= MAX`.
