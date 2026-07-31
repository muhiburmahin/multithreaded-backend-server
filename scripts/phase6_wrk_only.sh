#!/bin/bash
set -euo pipefail
export PATH="/usr/bin:/bin:/usr/sbin:/sbin:$PATH"
PROJ='/mnt/e/Multithread Backend Server'
TOOLS="$PROJ/.tools"
BUILD="$PROJ/build"
TESTS="$PROJ/tests"
mkdir -p "$TOOLS" "$TESTS"

# unzip (user-local, no sudo)
if [ ! -x "$TOOLS/unzip-root/usr/bin/unzip" ]; then
  cd "$TOOLS"
  apt-get download unzip
  dpkg-deb -x unzip_*.deb "$TOOLS/unzip-root"
fi
export PATH="$TOOLS/unzip-root/usr/bin:$PATH"

# wrk
if [ ! -x "$TOOLS/wrk/wrk" ]; then
  cd "$TOOLS"
  rm -rf wrk wrk-src
  git clone --depth 1 https://github.com/wg/wrk.git wrk-src
  cd wrk-src
  make -j4
  mkdir -p "$TOOLS/wrk"
  cp wrk "$TOOLS/wrk/wrk"
fi
echo "wrk ok: $TOOLS/wrk/wrk"

# ensure server with pool=8 built
pkill -9 -x server 2>/dev/null || true
sleep 1
cd "$BUILD"
./server >/tmp/server_wrk.log 2>&1 &
sleep 1
"$TOOLS/wrk/wrk" -t4 -c100 -d15s http://127.0.0.1:8080/ > "$TESTS/results_wrk.txt"
pkill -9 -x server 2>/dev/null || true
echo "Saved results_wrk.txt"
cat "$TESTS/results_wrk.txt"
