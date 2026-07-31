#!/bin/bash
set -euo pipefail

PROJ='/mnt/e/Multithread Backend Server'
BUILD="$PROJ/build"
TESTS="$PROJ/tests"
SRC="$PROJ/src/main.cpp"
mkdir -p "$TESTS"

# Prefer system wrk; else build from source using a user-local unzip (no sudo)
export PATH="/tmp/unzip-root/usr/bin:$PATH"
if [ ! -x /tmp/unzip-root/usr/bin/unzip ]; then
  cd /tmp
  apt-get download unzip
  dpkg-deb -x unzip_*.deb /tmp/unzip-root
fi

WRK_BIN="$(command -v wrk || true)"
if [ -z "$WRK_BIN" ]; then
  if [ ! -x /tmp/wrk/wrk ]; then
    cd /tmp
    rm -rf wrk
    git clone --depth 1 https://github.com/wg/wrk.git
    cd wrk
    make -j4
  fi
  WRK_BIN=/tmp/wrk/wrk
fi
echo "Using wrk: $WRK_BIN"
command -v ab >/dev/null

pkill -x server 2>/dev/null || true
sleep 1

run_ab() {
  local pool="$1"
  local out="$2"
  sed -i "s/constexpr size_t THREAD_POOL_SIZE = [0-9]*;/constexpr size_t THREAD_POOL_SIZE = ${pool};/" "$SRC"
  cd "$BUILD"
  cmake .. -DENABLE_TSAN=OFF
  cmake --build .
  pkill -x server 2>/dev/null || true
  sleep 1
  ./server >/tmp/server_phase6.log 2>&1 &
  local spid=$!
  sleep 1
  code=$(curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8080/ || true)
  if [ "$code" != "200" ]; then
    echo "Server failed to start (HTTP $code)" >&2
    cat /tmp/server_phase6.log >&2 || true
    exit 1
  fi
  ab -n 1000 -c 20 "http://127.0.0.1:8080/" > "$out"
  kill -INT "$spid" 2>/dev/null || pkill -x server 2>/dev/null || true
  wait "$spid" 2>/dev/null || true
  sleep 1
  echo "Saved $out"
}

run_ab 4 "$TESTS/results_pool4.txt"
run_ab 8 "$TESTS/results_pool8.txt"

cd "$BUILD"
pkill -x server 2>/dev/null || true
sleep 1
./server >/tmp/server_phase6.log 2>&1 &
spid=$!
sleep 1
"$WRK_BIN" -t4 -c100 -d15s http://127.0.0.1:8080/ > "$TESTS/results_wrk.txt"
kill -INT "$spid" 2>/dev/null || pkill -x server 2>/dev/null || true
wait "$spid" 2>/dev/null || true
echo "Saved $TESTS/results_wrk.txt"

echo "DONE"
grep -E 'Requests per second|Time per request:|Failed requests|Requests/sec|Latency|Socket errors' \
  "$TESTS/results_pool4.txt" "$TESTS/results_pool8.txt" "$TESTS/results_wrk.txt" || true
