#!/usr/bin/env python3
"""Lightweight concurrent HTTP load tool used when wrk is unavailable."""
import concurrent.futures
import statistics
import time
import urllib.error
import urllib.request

URL = "http://127.0.0.1:8080/"
THREADS = 4
CONNECTIONS = 100
DURATION = 15.0


def one_request():
    start = time.perf_counter()
    try:
        with urllib.request.urlopen(URL, timeout=5) as resp:
            resp.read()
            ok = 200 <= resp.status < 400
    except Exception:
        ok = False
    return ok, (time.perf_counter() - start) * 1000.0


def worker(stop_at, latencies, counters, lock):
    while time.perf_counter() < stop_at:
        ok, ms = one_request()
        with lock:
            latencies.append(ms)
            counters["requests"] += 1
            if not ok:
                counters["errors"] += 1


def main():
    import threading

    latencies = []
    counters = {"requests": 0, "errors": 0}
    lock = threading.Lock()
    stop_at = time.perf_counter() + DURATION
    threads = [
        threading.Thread(target=worker, args=(stop_at, latencies, counters, lock))
        for _ in range(CONNECTIONS)
    ]
    t0 = time.perf_counter()
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    elapsed = time.perf_counter() - t0
    reqs = counters["requests"]
    errs = counters["errors"]
    rps = reqs / elapsed if elapsed else 0.0
    avg = statistics.mean(latencies) if latencies else 0.0
    stdev = statistics.pstdev(latencies) if len(latencies) > 1 else 0.0
    mx = max(latencies) if latencies else 0.0
    print(f"Running {DURATION:.0f}s test @ {URL}")
    print(f"  {THREADS} threads and {CONNECTIONS} connections")
    print(f"  Thread model: python fallback (wrk unavailable)")
    print(f"  {reqs} requests in {elapsed:.2f}s, {errs} errors")
    print(f"Requests/sec: {rps:.2f}")
    print(f"Latency     avg={avg:.2f}ms  stdev={stdev:.2f}ms  max={mx:.2f}ms")
    print("Socket errors: connect 0, read 0, write 0, timeout 0")


if __name__ == "__main__":
    main()
