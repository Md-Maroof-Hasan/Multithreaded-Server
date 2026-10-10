#!/usr/bin/env bash
set -euo pipefail

URL="${1:-http://127.0.0.1:8080/hello}"
CONNECTIONS="${2:-100}"
DURATION="${3:-10s}"

if command -v wrk >/dev/null 2>&1; then
  wrk -t4 -c"$CONNECTIONS" -d"$DURATION" "$URL"
elif command -v ab >/dev/null 2>&1; then
  ab -n 10000 -c "$CONNECTIONS" "$URL"
else
  echo "Install 'wrk' or ApacheBench ('ab') to run a benchmark."
  exit 1
fi
