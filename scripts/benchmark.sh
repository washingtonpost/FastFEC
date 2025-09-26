#!/usr/bin/env sh
set -eu

# Benchmark FastFEC on a given .fec input
# Usage: scripts/benchmark.sh <path/to/file.fec> [output_dir]

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN="$ROOT_DIR/zig-out/bin/fastfec"
INPUT=${1:-}
OUTDIR_INPUT=${2:-"$ROOT_DIR/output"}

if [ -z "$INPUT" ]; then
  echo "Usage: $0 <path/to/file.fec> [output_dir]" >&2
  exit 1
fi

# Resolve absolute paths
INPUT_ABS=$(python3 - "$INPUT" <<'PY'
import os,sys
p=sys.argv[1]
print(os.path.abspath(p))
PY
)
OUTDIR=$(python3 - "$OUTDIR_INPUT" <<'PY'
import os,sys
p=sys.argv[1]
print(os.path.abspath(p))
PY
)

if [ ! -x "$BIN" ]; then
  echo "Building FastFEC..." >&2
  (cd "$ROOT_DIR" && zig build >/dev/null)
fi

if [ ! -f "$INPUT_ABS" ]; then
  echo "Input not found: $INPUT_ABS" >&2
  exit 1
fi

mkdir -p "$OUTDIR" >/dev/null 2>&1 || true

echo "Input:  $INPUT_ABS"
echo "Output: $OUTDIR"
BYTES=$(wc -c < "$INPUT_ABS" | awk '{print $1}')
echo "Size:   $BYTES bytes"

start_ns=$(python3 - <<'PY'
import time; print(int(time.time_ns()))
PY
)

# Run benchmark (do not exit on failure; capture rc)
set +e
CMD="$BIN -s -x \"$INPUT_ABS\" \"$OUTDIR\""
echo "Command: $BIN -s -x $INPUT_ABS $OUTDIR"
sh -c "$CMD" >/dev/null 2>"$OUTDIR/.bench.stderr"
rc=$?
set -e

end_ns=$(python3 - <<'PY'
import time; print(int(time.time_ns()))
PY
)

dur_ns=$((end_ns - start_ns))
# Avoid division by zero
if [ "$dur_ns" -le 0 ]; then dur_ns=1; fi

echo "Exit:   $rc"

echo "Time:   $((dur_ns/1000000)) ms"

if [ "$rc" -eq 0 ]; then
  # Throughput in MB/s (1 MB = 1,000,000 bytes)
  mbps=$(python3 - <<PY
bytes_ = $BYTES
ns = $dur_ns
mbps = (bytes_ / 1_000_000) / (ns / 1_000_000_000)
print(f"{mbps:.2f}")
PY
  )
  echo "Speed:  ${mbps} MB/s"
else
  echo "Speed:  (skipped due to non-zero exit)"
  echo "--- stderr (with -s -x) ---"
  sed -n '1,40p' "$OUTDIR/.bench.stderr" || true
  echo "--- retry without -s (still -x) to show messages ---"
  set +e
  "$BIN" -x "$INPUT_ABS" "$OUTDIR" 2>&1 | sed -n '1,60p'
  set -e
fi

# macOS memory stats (best-effort; only if success to avoid noise)
if [ "$rc" -eq 0 ] && command -v /usr/bin/time >/dev/null 2>&1; then
  echo "\nDetailed (one-off) stats via /usr/bin/time:"
  (/usr/bin/time -l "$BIN" -s -x "$INPUT_ABS" "$OUTDIR" >/dev/null) 2>&1 | sed -n 's/^\s*//;p' | head -n 10
fi
