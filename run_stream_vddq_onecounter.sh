#!/bin/bash
set -e

# ======================================================
# STREAM Benchmark Power Measurement (VDDQ rail only)
# ======================================================

# DRAM rail (VDDQ) power sensor path
POWER_PATH="/sys/bus/i2c/drivers/ina3221/1-0041/hwmon/hwmon4/curr2_input"

# Log file path
LOG_FILE="/home/izengin/Desktop/current_reader/vddq_power_log.csv"

# STREAM executable directory
STREAM_DIR="/home/izengin/Desktop/STREAM"

# Configurations
SAMPLES=30          # total number of samples
INTERVAL=1          # seconds between samples
THREADS=8           # number of CPU threads for STREAM

echo "timestamp,current_mA" > "$LOG_FILE"

echo "Starting STREAM and VDDQ current measurement..."
(
    for ((i=0; i<$SAMPLES; i++)); do
        if [[ -f "$POWER_PATH" ]]; then
            CURR=$(cat "$POWER_PATH")       # in mA
            TS=$(date +%s.%N)
            echo "$TS,$CURR" >> "$LOG_FILE"
        fi
        sleep $INTERVAL
    done
    echo "Measurement completed and saved to $LOG_FILE"
) &
MEASURE_PID=$!

# Run STREAM benchmark in parallel
echo "Running STREAM benchmark (OMP_NUM_THREADS=$THREADS)..."
cd "$STREAM_DIR"
OMP_NUM_THREADS=$THREADS ./stream.100M > /dev/null 2>&1

# Stop measurement loop
kill $MEASURE_PID 2>/dev/null || true
wait $MEASURE_PID 2>/dev/null || true

# Compute average current (mA) and approximate power (mW)
AVG_CURR=$(awk -F',' 'NR>1 {sum+=$2; n++} END {if(n>0) printf "%.2f", sum/n}' "$LOG_FILE")
VDDQ_VOLTAGE=1.1  # DRAM rail voltage ~1.1V
AVG_POWER=$(awk -v i="$AVG_CURR" -v v="$VDDQ_VOLTAGE" 'BEGIN {printf "%.3f", i*v/1000}')  # W

echo "Average DRAM current: $AVG_CURR mA"
echo "Estimated DRAM power consumption: $AVG_POWER W"
echo "Log saved: $LOG_FILE"
