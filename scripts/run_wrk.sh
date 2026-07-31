#!/bin/bash
set -euo pipefail
export PATH="/usr/bin:/bin:$PATH"
PROJ='/mnt/e/Multithread Backend Server'
BUILD="$PROJ/build"
TESTS="$PROJ/tests"
WRK="$HOME/tools/wrk"

pkill -9 -x server 2>/dev/null || true
sleep 1
cd "$BUILD"
./server >/dev/null 2>&1 &
sleep 1
"$WRK" -t4 -c100 -d15s http://127.0.0.1:8080/ | tee "$TESTS/results_wrk.txt"
pkill -9 -x server 2>/dev/null || true
echo DONE
