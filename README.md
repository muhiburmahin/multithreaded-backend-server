# Multithreaded Backend Server

C++17 Linux HTTP server for an Operating Systems course project.  
Uses a fixed **Thread Pool**, mutex-protected shared state, a small REST API, per-IP rate limiting, thread-safe logging, and graceful SIGINT shutdown.

Full design: [`docs/02_ARCHITECTURE.md`](docs/02_ARCHITECTURE.md)  
Requirements: [`docs/01_REQUIREMENTS.md`](docs/01_REQUIREMENTS.md)  
Phased plan: [`docs/03_ROADMAP.md`](docs/03_ROADMAP.md)

## Architecture (summary)

```
Main thread: socket → bind → listen → accept loop
    → enqueue(clientFd) into ThreadPool task queue (mutex + condition_variable)
Worker threads: rate-limit check → recv/parse HTTP → Router → log → send → close
```

| Component | Role |
|---|---|
| `Server` | POSIX listen/accept; hands connections to the pool |
| `ThreadPool` | N pre-spawned workers + task queue |
| `RateLimiter` | Fixed-window per-IP counter |
| `HttpRequest` / `HttpResponse` | Minimal HTTP/1.1 parse & response builders |
| `Router` | `(method, path) → handler` |
| `Logger` | Thread-safe request log line |

Details and diagrams: [`docs/02_ARCHITECTURE.md`](docs/02_ARCHITECTURE.md).

## Build (Linux / WSL)

```bash
sudo apt update
sudo apt install -y build-essential cmake apache2-utils
cd "/path/to/Multithread Backend Server"
mkdir -p build && cd build
cmake ..
cmake --build .
```

Optional ThreadSanitizer build:

```bash
cmake .. -DENABLE_TSAN=ON && cmake --build .
```

## Run

```bash
cd build
./server
# Listening on port 8080
# Ctrl+C (or kill -INT <pid>) for graceful shutdown
```

## Sample API checks

```bash
curl -v http://127.0.0.1:8080/
curl -v http://127.0.0.1:8080/health
curl -v http://127.0.0.1:8080/time
curl -v -X POST -d "hello" http://127.0.0.1:8080/echo
curl -v http://127.0.0.1:8080/xyz          # 404
```

Expected:

| Method | Path | Response |
|---|---|---|
| GET | `/` | plain text welcome |
| GET | `/health` | `{"status":"ok"}` |
| GET | `/time` | `{"time":"...Z"}` |
| POST | `/echo` | `{"echo":"hello"}` |
| any other | | `404 Not Found` |

## Load-test results

Raw outputs: [`tests/results_pool4.txt`](tests/results_pool4.txt), [`tests/results_pool8.txt`](tests/results_pool8.txt), [`tests/results_wrk.txt`](tests/results_wrk.txt).

### `ab -n 1000 -c 20` (pool size comparison)

| Thread pool size | Req/sec | Latency (mean) | Failed |
|---:|---:|---:|---:|
| 4 | 9713.36 | 2.059 ms | 0 |
| 8 | 9660.06 | 2.070 ms | 0 |

At concurrency 20 this workload is already saturated; increasing the pool from 4→8 does not meaningfully change throughput.

### `wrk -t4 -c100 -d15s` (pool = 8)

| Metric | Value |
|---|---:|
| Requests/sec | 20165.62 |
| Latency (avg) | 2.91 ms |
| Total requests | 304391 |

## Requirements checklist (R1–R10)

| # | Status | Where in code |
|---|---|---|
| **R1** Concurrent sockets | Done | `Server::run()` in `src/Server.cpp` — non-blocking accept loop; each `clientFd` is enqueued so new accepts are not stalled behind request handling |
| **R2** Mutex / no races | Done | `ThreadPool::queueMutex` (`src/ThreadPool.cpp`), `RateLimiter::mapMutex` (`src/RateLimiter.cpp`), `Logger::logMutex` (`src/Logger.cpp`); verified with `-DENABLE_TSAN=ON` |
| **R3** Basic HTTP | Done | `HttpResponse::ok` / `json` / … in `src/HttpResponse.cpp`; served from `Server::handleConnection` |
| **R4** Dynamic Thread Pool | Done | `ThreadPool` ctor / `enqueue` / `workerLoop` in `src/ThreadPool.cpp`; wired in `Server::run()` via `pool.enqueue(...)` |
| **R5** Load testing | Done | Recorded in `tests/results_pool4.txt`, `tests/results_pool8.txt`, `tests/results_wrk.txt` (table above) |
| **R6** Rate limiting | Done | `RateLimiter::allow` in `src/RateLimiter.cpp`; called first in `Server::handleConnection`; overflow → `HttpResponse::tooManyRequests()` (429) |
| **R7** Basic API | Done | Routes registered in `main()` (`src/main.cpp`); dispatch via `Router::dispatch` in `src/Router.cpp` |
| **R8** Thread-safe logging | Done | `Logger::log` in `src/Logger.cpp`; called from `Server::handleConnection` with IP, method, path, status |
| **R9** Graceful shutdown | Done | `onSigInt` → `Server::stop()` in `src/main.cpp` / `src/Server.cpp` (closes `listenFd`); `ThreadPool::~ThreadPool` joins workers |
| **R10** Error handling | Done | Syscall checks in `Server::run()`; bad/malformed requests → `HttpResponse::badRequest()` / `Router::dispatch` → `notFound()` |

## Project layout

```
include/     ThreadPool, Server, RateLimiter, Http*, Router, Logger
src/         implementations + main.cpp
docs/        requirements, architecture, roadmap
tests/       ab/wrk result captures
```

## Team

Group 8 — Context Switchers · Project: Multithreaded Backend Server
