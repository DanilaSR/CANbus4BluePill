#!/usr/bin/env python3
"""Analyze `yakut subscribe uavcan.node.Heartbeat.1.0 +M` output (JSON lines).

Detects and classifies stream anomalies of the Blue Pill heartbeat publisher:
  GAP   - arrival gap > GAP_THRESHOLD s while uptime resumes at prev+1:
          the node kept running but stopped publishing (uwTick-style hang,
          Bug B candidate: firmware stalled, no reset).
  RESET - after a gap the uptime restarted from a small value: the node
          rebooted (watchdog / power / software reset).
  TID   - unexpected transfer-ID discontinuity.

Usage:
  source ./yakut-env.sh
  yakut subscribe uavcan.node.Heartbeat.1.0 +M | ./hb-analyze.py
  ./yakut_monitor.sh 60 log.jsonl        # wrapper with duration + raw log
"""
import json
import sys
import time

HB_NOMINAL_PERIOD = 1.0      # [s] heartbeat publication period
GAP_THRESHOLD = 2.5          # [s] arrival gap beyond this is suspicious
SUBJECT_KEY = "7509"


def main() -> int:
    prev_arrival_wall = None   # host wall clock of previous heartbeat
    prev_up = None
    prev_tid = None
    n_hb = n_gap = n_reset = 0
    t_start = time.time()

    for line in sys.stdin:
        line = line.strip()
        if not line or not line.startswith("{"):
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            continue
        hb = obj.get(SUBJECT_KEY) or next(iter(obj.values()), None)
        if not isinstance(hb, dict) or "uptime" not in hb:
            continue

        meta = hb.get("_meta_", {})
        up = int(hb["uptime"])
        node = meta.get("source_node_id", "?")
        tid = meta.get("transfer_id")
        # Prefer yakut's host-wall-clock metadata: identical behavior for live
        # pipes and offline log replays (with time.time() a fast replay shows
        # every frame at +0.0s, which is meaningless).
        ts = meta.get("ts_system")
        arrival = float(ts) if isinstance(ts, (int, float)) else time.time()
        n_hb += 1

        if prev_arrival_wall is None:
            t_start = arrival
            first = f"stream start: node={node} uptime={up}s tid={tid}"
            print(first, flush=True)
        else:
            delta = arrival - prev_arrival_wall
            if delta > GAP_THRESHOLD:
                if prev_up is not None and up <= prev_up + 1:
                    kind, what = "GAP", "hang (no reset, uptime continued)"
                    n_gap += 1
                else:
                    kind, what = "RESET", "node rebooted"
                    n_reset += 1
                print(
                    f"!!! {kind}: {delta:.2f}s without heartbeat -> {what}; "
                    f"uptime {prev_up} -> {up}, tid {prev_tid} -> {tid}",
                    flush=True,
                )
            elif up > prev_up + int(delta) + 1:
                # uptime jumped forward faster than wall time: impossible normally
                print(f"!!! anomaly: uptime jumped {prev_up} -> {up} in {delta:.2f}s",
                      flush=True)
            elif up < prev_up:
                # Quick reboot that produced no long gap (e.g. debugger NRST that
                # reboots the node cleanly within a heartbeat period).
                n_reset += 1
                print(
                    f"!!! RESET: uptime rolled back {prev_up} -> {up} "
                    f"(tid {prev_tid} -> {tid})",
                    flush=True,
                )
            if (
                prev_tid is not None
                and up == prev_up
                and delta <= GAP_THRESHOLD
                and tid is not None
                and tid != (prev_tid + 1) % 32
            ):
                n_gap += 1
                print(f"!!! TID discontinuity: {prev_tid} -> {tid} (uptime {up})", flush=True)

        print(
            f"[+{arrival - t_start:7.1f}s] HB node={node} uptime={up:6d}s "
            f"tid={tid} health={hb.get('health', {}).get('value')} "
            f"mode={hb.get('mode', {}).get('value')}",
            flush=True,
        )
        prev_arrival_wall = arrival
        prev_up = up
        prev_tid = tid

    # EOF summary (stream duration = last frame arrival - first frame arrival).
    total = max(0.0, arrival - t_start)
    expected = int(total / HB_NOMINAL_PERIOD)
    loss = max(0, expected - n_hb)
    print(
        f"\n== summary: {n_hb} heartbeats in {total:.1f}s "
        f"(expected ~{expected}, missing {loss}), "
        f"gaps(hangs)={n_gap} resets={n_reset} =="
        if n_reset == 0 or n_gap == 0 else
        f"\n== summary: {n_hb} heartbeats in {total:.1f}s "
        f"gaps(hangs)={n_gap} resets={n_reset} ==",
        flush=True,
    )
    return 0 if n_gap == 0 else 2  # exit 2 = hang detected


if __name__ == "__main__":
    sys.exit(main())
