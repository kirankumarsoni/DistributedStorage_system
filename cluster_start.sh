#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

mkdir -p logs
node_count=$(awk -F= '$1=="nodes" {gsub(/[[:space:]]/, "", $2); print $2}' config.conf)

for ((node_id=0; node_id<node_count; node_id++)); do
    if [[ -f "node${node_id}.pid" ]] && kill -0 "$(cat "node${node_id}.pid")" 2>/dev/null; then
        echo "Node ${node_id} is already running."
        continue
    fi

    ./build/node_server "$node_id" > "logs/node${node_id}.log" 2>&1 &
    echo $! > "node${node_id}.pid"
    echo "Started node ${node_id}; log: logs/node${node_id}.log"
done
