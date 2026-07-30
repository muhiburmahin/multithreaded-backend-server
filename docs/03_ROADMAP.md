# 03 — Roadmap

Do not start the next phase until the current phase test passes.

## Phase 0 — Environment + skeleton

CMake (C++17, Threads::Threads, ENABLE_TSAN), folder/file skeleton, empty `main()` builds.

## Phase 1 — Basic TCP socket server

Single-threaded `Server::run()`, hardcoded reply. Pass: `curl`/`telnet` gets bytes.

## Phase 2 — HTTP + Router

Parse (with body), responses, four endpoints + 404. Pass: five `curl` checks.

## Phase 3 — Thread Pool

`ThreadPool` + enqueue `handleConnection`. Pass: `ab -n 200 -c 20`, Failed=0.

## Phase 4 — TSAN

Build with `-DENABLE_TSAN=ON`, load test. Pass: no data-race warnings.

## Phase 5 — Rate limiting

`RateLimiter` in `handleConnection`. Pass: burst shows 200 then 429.

## Phase 6 — Load testing

Record `ab` for 2 pool sizes + `wrk` into `tests/results_*.txt`.

## Phase 7 — Logging + graceful shutdown

`Logger` + SIGINT → `server.stop()`, join workers.

## Phase 8 — Docs + submission

README, load-test table, checklist vs `01_REQUIREMENTS.md`.

## Phase 9 — Stretch (optional)

Static files `GET /files/<name>` from `public/`, no path traversal.
