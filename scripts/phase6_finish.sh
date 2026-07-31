#!/bin/bash
set -euo pipefail
PROJ='/mnt/e/Multithread Backend Server'
BUILD="$PROJ/build"
TESTS="$PROJ/tests"
SRC="$PROJ/src/main.cpp"
WRK_BIN=/tmp/wrk/wrk
export PATH="/tmp/unzip-root/usr/bin:$PATH"
mkdir -p "$TESTS"
pkill -9 -x server 2>/dev/null || true
sleep 1

run_ab() {
  local pool="$1"
  local out="$2"
  sed -i "s/constexpr size_t THREAD_POOL_SIZE = [0-9]*;/constexpr size_t THREAD_POOL_SIZE = ${pool};/" "$SRC"
  cd "$BUILD"
  cmake .. -DENABLE_TSAN=OFF >/dev/null
  cmake --build . >/dev/null
  pkill -9 -x server 2>/dev/null || true
  sleep 1
  ./server >/tmp/server_phase6.log 2>&1 &
  sleep 1
  curl -s -o /dev/null http://127.0.0.1:8080/ || { echo fail; cat /tmp/server_phase6.log; exit 1; }
  ab -n 1000 -c 20 "http://127.0.0.1:8080/" > "$out"
  pkill -9 -x server 2>/dev/null || true
  sleep 1
  echo "Saved $out"
}

# pool4 may already exist; re-run both for clean pair
run_ab 4 "$TESTS/results_pool4.txt"
run_ab 8 "$TESTS/results_pool8.txt"

cd "$BUILD"
./server >/tmp/server_phase6.log 2>&1 &
sleep 1
"$WRK_BIN" -t4 -c100 -d15s http://127.0.0.1:8080/ > "$TESTS/results_wrk.txt"
pkill -9 -x server 2>/dev/null || true
echo "Saved $TESTS/results_wrk.txt"
echo DONE
grep -E 'Requests per second|Time per request:|Failed requests|Requests/sec|Latency|Socket errors' \
  "$TESTS/results_pool4.txt" "$TESTS/results_pool8.txt" "$TESTS/results_wrk.txt"
