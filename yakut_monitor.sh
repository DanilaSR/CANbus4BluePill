#!/bin/bash
# Heartbeat monitor for the STM32F103 CAN node via yakut.
#
# Usage:
#   ./yakut_monitor.sh                 # monitor forever, human-readable stream
#   ./yakut_monitor.sh 60              # 60 seconds
#   ./yakut_monitor.sh 60 log.jsonl    # 60 s + save raw yakut JSON to a file
#
# Exit code: 0 = clean stream, 2 = hang (GAP) detected by hb-analyze.py.
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=yakut-env.sh
source "$HERE/yakut-env.sh"

DUR="${1:-0}"
LOG="${2:-}"

CMD=(yakut subscribe uavcan.node.Heartbeat.1.0 +M)
[ "$DUR" -gt 0 ] && CMD=(timeout --signal=INT "$DUR" "${CMD[@]}")

run() {
    if [ -n "$LOG" ]; then
        "${CMD[@]}" | tee "$LOG" | python3 "$HERE/hb-analyze.py"
    else
        "${CMD[@]}" | python3 "$HERE/hb-analyze.py"
    fi
}

echo "# monitoring uavcan.node.Heartbeat.1.0 (subject 7509) via ${YAKUT_TRANSPORT%%,*} ..." >&2
run
rc=$?
# `timeout` returns 124 when the time budget expires; normalize to 0 (clean stop).
[ "$DUR" -gt 0 ] && [ "$rc" -eq 124 ] && rc=0
exit "$rc"
