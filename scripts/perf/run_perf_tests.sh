#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/../.." && pwd)
BUILD_DIR=${ROOT_DIR}/build
BIN=${BUILD_DIR}/bin/perf_benchmark
RESULTS_DIR=${ROOT_DIR}/perf_results

mkdir -p "$RESULTS_DIR"

# Default parameter sets
ACTOR_COUNTS=(1 10 100 500)
MESSAGE_COUNTS=(100 1000 10000)
SLEEP_MS=0
SAMPLE_INTERVAL_MS=200
REPEATS=3

# Allow override of repeats via third arg or env
if [ "$#" -ge 3 ]; then
  REPEATS="$3"
fi

# Allow overrides via env or args
if [ "$#" -ge 1 ]; then
  IFS=',' read -ra ACTOR_COUNTS <<< "$1"
fi
if [ "$#" -ge 2 ]; then
  IFS=',' read -ra MESSAGE_COUNTS <<< "$2"
fi

# Build the project (Release) if binary not present
if [ ! -x "$BIN" ]; then
  echo "Building project..."
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DCMAKE_SKIP_RPATH=ON
  cmake --build "$BUILD_DIR" -j --target perf_benchmark
fi

echo "Starting performance runs, results -> $RESULTS_DIR"

SUMMARY_CSV="$RESULTS_DIR/summary.csv"
echo "actors,messages,run,exit_code,total_expected,received,elapsed_s,max_cpu_pct,max_rss_kb" > "$SUMMARY_CSV"

run_id=0
for a in "${ACTOR_COUNTS[@]}"; do
  for m in "${MESSAGE_COUNTS[@]}"; do
    for r in $(seq 1 "$REPEATS"); do
      run_id=$((run_id+1))
      out_dir="$RESULTS_DIR/run_${run_id}_a${a}_m${m}_r${r}"
      mkdir -p "$out_dir"
      stats_csv="$out_dir/stats.csv"
      echo "timestamp_ms,cpu_pct,rss_kb" > "$stats_csv"

      echo "Run $run_id: actors=$a messages=$m repeat=$r"
      start_ts=$(date +%s%3N)

      # Start the benchmark (unset LD_LIBRARY_PATH to prefer system libs)
      env -u LD_LIBRARY_PATH "$BIN" --actors "$a" --messages "$m" --sleep-ms "$SLEEP_MS" > "$out_dir/stdout.log" 2> "$out_dir/stderr.log" &
      pid=$!

      # If pidstat available, run it to capture more accurate CPU/RSS samples
      if command -v pidstat >/dev/null 2>&1; then
        pidstat -u -r -p "$pid" 0.$(printf "%03d" "$SAMPLE_INTERVAL_MS") > "$out_dir/pidstat.log" 2>&1 &
        pidstat_pid=$!
      else
        pidstat_pid=0
      fi

      # Sample while process is running (fallback using ps)
      while kill -0 "$pid" 2>/dev/null; do
        ts=$(date +%s%3N)
        read -r cpu rss <<< $(ps -p "$pid" -o %cpu=,rss= || echo "0 0")
        cpu=${cpu:-0}
        rss=${rss:-0}
        echo "$ts,$cpu,$rss" >> "$stats_csv"
        sleep 0.$(printf "%03d" "$SAMPLE_INTERVAL_MS")
      done

      # stop pidstat if running
      if [ "$pidstat_pid" -ne 0 ]; then
        kill "$pidstat_pid" 2>/dev/null || true
        wait "$pidstat_pid" 2>/dev/null || true
      fi

      wait_status=0
      wait $pid || wait_status=$?
      end_ts=$(date +%s%3N)
      elapsed_ms=$((end_ts - start_ts))

      # Parse stdout for totals
      total_expected=$((a * m))
      received=$(grep -oP "received=\K[0-9]+" "$out_dir/stdout.log" || true)
      received=${received:-0}

      # Compute max CPU and max RSS from stats (use pidstat if present)
      if [ -f "$out_dir/pidstat.log" ]; then
        # pidstat output: ignore header lines, find %CPU and RSS columns
        max_cpu=$(awk '/^[0-9]/{if($8>m)m=$8}END{print m+0}' "$out_dir/pidstat.log" || echo 0)
        max_rss=$(awk '/^[0-9]/{if($9>m)m=$9}END{print m+0}' "$out_dir/pidstat.log" || echo 0)
      elif [ -f "$stats_csv" ]; then
        max_cpu=$(awk -F, 'NR>1{if($2>m)m=$2}END{print m+0}' "$stats_csv")
        max_rss=$(awk -F, 'NR>1{if($3>m)m=$3}END{print m+0}' "$stats_csv")
      else
        max_cpu=0
        max_rss=0
      fi

      elapsed_s=$(awk "BEGIN{printf \"%.3f\",${elapsed_ms}/1000}")
      echo "${a},${m},${run_id},${wait_status},${total_expected},${received},${elapsed_s},${max_cpu},${max_rss}" >> "$SUMMARY_CSV"

      echo "Run $run_id finished: exit=$wait_status elapsed=${elapsed_s}s max_cpu=${max_cpu}% max_rss=${max_rss}KB"
    done
  done
done

echo "All runs complete. Summary: $SUMMARY_CSV"
