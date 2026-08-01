#!/usr/bin/env bash
# starts the engine and run a strategy in one command
#
# Usage:
#   ./scripts/run.sh <strategy.py>
#   ./scripts/run.sh --build <strategy.py>  # rebuilds engine first
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD=false
STRATEGY=""

while [[ $# -gt 0 ]]; do
  case $1 in
    --build) BUILD=true; shift ;;
    -h|--help)
      sed -n '2,8p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    -*) echo "Unknown option: $1" >&2; exit 1 ;;
    *)  STRATEGY="$1"; shift ;;
  esac
done

if [[ -z "$STRATEGY" ]]; then
  echo "error: strategy file required" >&2
  echo "usage: $0 [--build] <strategy.py>" >&2
  exit 1
fi

ENGINE_BIN="$REPO_ROOT/build/engine-cpp/src/trading_engine"

if [[ "$BUILD" == true ]]; then
  echo "Building engine..."
  cmake --build "$REPO_ROOT/build" -j
fi

if [[ ! -x "$ENGINE_BIN" ]]; then
  echo "error: engine not found at $ENGINE_BIN" >&2
  echo "Run with --build, or: cmake -S . -B build && cmake --build build -j" >&2
  exit 1
fi

ENGINE_PID=""

cleanup() {
  if [[ -n "${ENGINE_PID:-}" ]] && kill -0 "$ENGINE_PID" 2>/dev/null; then
    echo "Stopping engine (pid=$ENGINE_PID)…"
    kill -INT "$ENGINE_PID" 2>/dev/null || true
    wait "$ENGINE_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT

echo "Starting engine…"
"$ENGINE_BIN" &
ENGINE_PID=$!

# Wait for gRPC port to be ready with a 10 seconds timeout limit
python3 - <<'EOF'
import grpc, sys
ch = grpc.insecure_channel("localhost:50051")
try:
    grpc.channel_ready_future(ch).result(timeout=10)
except Exception:
    print("error: engine did not become ready within 10s", file=sys.stderr)
    sys.exit(1)
finally:
    ch.close()
EOF

echo "Engine ready; Running $STRATEGY"
python3 "$STRATEGY"
