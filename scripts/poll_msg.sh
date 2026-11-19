#!/bin/bash
while true; do
    python3 /data/dev/capsurf/scripts/watch_task.py --journal /root/.agy/comm/cp-bun-capsurf.jsonl
    echo "[POLLER] Capsurf Poller exited, restarting in 5s..."
    sleep 5
done
