#!/bin/bash
# run_experiments.sh
# Runs all policy x trace x size combinations and writes results to one CSV.
#
# Usage:
#   cd <project_root>
#   bash scripts/run_experiments.sh
#
# Output:
#   results/benchmark.csv

set -e

BINARY="./cache_audit"
TRACES_DIR="traces/synthetic"
OUTPUT="results/benchmark.csv"

POLICIES=("fifo" "lru" "lfu" "arc" "belady")
TRACES=("loop" "scan" "skewed" "hot_cold")
SIZES=(10 20 40)

# --- Pre-flight ---
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found at $BINARY"
    echo "Build first: g++ -std=c++17 -O2 -I src src/main.cpp src/engine/trace_loader.cpp -o cache_audit"
    exit 1
fi

mkdir -p results
rm -f "$OUTPUT"

total=$(( ${#POLICIES[@]} * ${#TRACES[@]} * ${#SIZES[@]} ))
count=0

echo "Running $total experiments -> $OUTPUT"
echo ""

# --- Main loop ---
for trace in "${TRACES[@]}"; do
    trace_file="$TRACES_DIR/$trace.txt"
    if [ ! -f "$trace_file" ]; then
        echo "WARNING: $trace_file not found, skipping"
        continue
    fi

    for policy in "${POLICIES[@]}"; do
        for size in "${SIZES[@]}"; do
            count=$(( count + 1 ))

            hit_rate=$( $BINARY "$trace_file" "$policy" "$size" --out "$OUTPUT" 2>/dev/null \
                        | grep "Hit Rate" | awk '{print $3}' )

            printf "[%2d/%d]  %-8s | %-9s | size=%-3d | hit_rate=%s\n" \
                   "$count" "$total" "$policy" "$trace" "$size" "$hit_rate"
        done
    done
    echo ""
done

echo "Done. Results: $OUTPUT"
