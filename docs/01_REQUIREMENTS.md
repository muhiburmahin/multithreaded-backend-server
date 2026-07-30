# 01 — Requirements

## Functional requirements

| # | Requirement | Definition of done |
|---|---|---|
| R1 | Accept concurrent client connections using sockets | Server accepts N simultaneous TCP connections without blocking new ones behind existing ones |
| R2 | Implement threading using Mutexes to avoid race conditions | Every shared mutable object is protected by a `std::mutex`; TSAN-clean under load |
| R3 | Serve basic HTTP responses | Returns a valid HTTP/1.1 status line + headers + body for a GET request, verifiable with `curl -v` |
| R4 | Implement a dynamic Thread Pool | Fixed-size pool of pre-spawned worker threads consuming from a task queue — NOT one thread per connection |
| R5 | Implement basic load testing | `ab` or `wrk` results (req/sec, latency, failures) recorded for at least 2 different thread-pool sizes |
| R6 | Implement rate limiting | Per-IP request cap; exceeding it returns `429 Too Many Requests`, verifiable with a burst of requests from one IP |
| R7 | Serve a basic API | `GET /`, `GET /health`, `GET /time`, `POST /echo` all return correct responses, verifiable with `curl` |
| R8 | Thread-safe logging | Every request logged (IP, method, path, status) without garbled/interleaved output under concurrent load |
| R9 | Graceful shutdown | `Ctrl+C` (SIGINT) closes the listener, joins all worker threads, no leaked file descriptors |
| R10 | Error handling | Every syscall return value checked; malformed requests get a clean 400-style response instead of crashing the server |

## API specification (R7)

| Method | Path | Behavior |
|---|---|---|
| GET | `/` | Returns a simple welcome message (`text/plain` or JSON) |
| GET | `/health` | Returns `{"status":"ok"}` (JSON) |
| GET | `/time` | Returns current server time, e.g. `{"time":"2026-07-30T21:33:00Z"}` (JSON) |
| POST | `/echo` | Returns the request body back (`{"echo":"<body>"}`) |
| any other path | Returns `404 Not Found` |

Note: Thread Pool (R4) satisfies both MVP threading and Advanced Thread Pool — no separate thread-per-client version.

## Non-functional / constraints

- Language: C++17
- OS/target: Linux (POSIX sockets)
- Threading: `std::thread`, `std::mutex`, `std::condition_variable` (no raw `pthread_*`)
- Build: CMake
- No DB, no TLS, no keep-alive required

## Out of scope

- HTTPS/TLS, config files, horizontal scaling, keep-alive

## Stretch goals

- Static file serving (`GET /files/<name>` from `public/`)
- In-memory request counter at `/health`
