#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

for pid_file in node*.pid; do
    [[ -f "$pid_file" ]] || continue
    pid=$(cat "$pid_file")
    if kill -0 "$pid" 2>/dev/null; then
        kill -TERM "$pid"
        echo "Stopped node process $pid."
    fi
    rm -f "$pid_file"
done
