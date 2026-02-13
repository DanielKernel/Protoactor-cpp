#!/usr/bin/env bash
# Generate line coverage summary for key sources (unit tests only).
# Requires: build with -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug, then run: ctest -L unit
# Usage: from project root, run: ./scripts/coverage_report.sh [build_dir]
# Default build_dir = build_cov

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build_cov}"
cd "$ROOT"

echo "=== Key source line coverage (unit tests) ==="
TOTAL_LINES=0
COV_LINES=0
for base in thread_pool pid config queue dispatcher platform messages pidset; do
  objdir="$BUILD_DIR/CMakeFiles/protoactor-cpp.dir/src/actor"
  if [ -f "$objdir/${base}.cpp.gcno" ] && [ -f "$objdir/${base}.cpp.gcda" ]; then
    out=$(gcov -n -o "$objdir" --gcno="$objdir/${base}.cpp.gcno" --gcda="$objdir/${base}.cpp.gcda" "src/actor/${base}.cpp" 2>/dev/null | grep -A1 "protoactor-cpp/src/actor/${base}.cpp" | grep "Lines executed" || true)
    if [ -n "$out" ]; then
      pct=$(echo "$out" | sed -n 's/.*Lines executed:\([0-9.]*\)% of \([0-9]*\).*/\1 \2/p')
      echo "  src/actor/${base}.cpp: $out"
      lines=$(echo "$pct" | cut -d' ' -f2)
      pct_num=$(echo "$pct" | cut -d' ' -f1)
      if [ -n "$lines" ] && [ -n "$pct_num" ]; then
        TOTAL_LINES=$((TOTAL_LINES + lines))
        COV_LINES=$((COV_LINES + (${lines} * ${pct_num%.*} / 100)))
      fi
    fi
  fi
done
if [ -f "$BUILD_DIR/CMakeFiles/protoactor-cpp.dir/src/queue/priority_queue.cpp.gcno" ]; then
  objdir="$BUILD_DIR/CMakeFiles/protoactor-cpp.dir/src/queue"
  out=$(gcov -n -o "$objdir" --gcno="$objdir/priority_queue.cpp.gcno" --gcda="$objdir/priority_queue.cpp.gcda" "src/queue/priority_queue.cpp" 2>/dev/null | grep -A1 "protoactor-cpp/src/queue/priority_queue.cpp" | grep "Lines executed" || true)
  if [ -n "$out" ]; then
    echo "  src/queue/priority_queue.cpp: $out"
  fi
fi
echo ""
echo "Target: key code line coverage >= 60% (see tests/README.md)"
