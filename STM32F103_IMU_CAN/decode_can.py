#!/usr/bin/env python3
"""Decode a candump capture as Cyphal v1.0 CAN frames.

Usage: decode_can.py <candump-file> [subject_id ...]

Decodes the 29-bit ID (priority / subject / source node), reassembles
multi-frame transfers, and prints the payload of the subjects you name.
"""
import sys
import struct
from collections import defaultdict

NAMES = {
    7509: "uavcan.node.Heartbeat.1.0",
    7511: "si.sample.angular_velocity.Vector3.1.0",
    7512: "si.sample.acceleration.Vector3.1.0",
}


def decode_id(cid):
    """Cyphal v1.0 29-bit CAN ID -> (priority, subject_id, source_node_id)."""
    return (cid >> 26) & 0x7, (cid >> 8) & 0x1FFF, cid & 0x7F


def v1_check(cid):
    """Return (ok, reason) for the v1.0 discriminator bits."""
    if (cid >> 24) & 0x1:
        return False, "anonymous bit set"
    if (cid >> 21) & 0x3 != 0b11:
        return False, "bits[22:21] != 0b11 (not a v1.0 subject frame)"
    return True, "v1.0"


def parse(path):
    """Yield (cid, data_bytes) in capture order."""
    with open(path) as fh:
        for line in fh:
            parts = line.split()
            if len(parts) < 4 or not parts[1].startswith(("1", "0")):
                continue
            try:
                cid = int(parts[1], 16)
                data = bytes(int(b, 16) for b in parts[3:])
            except ValueError:
                continue
            yield cid, data


def main():
    path = sys.argv[1]
    wanted = {int(a) for a in sys.argv[2:]} if len(sys.argv) > 2 else set()

    frames = list(parse(path))
    if not frames:
        print("no frames parsed from", path)
        return 1

    print(f"parsed {len(frames)} frames from {path}\n")

    # --- ID-level summary ---
    stats = defaultdict(lambda: [0, 0])  # (prio, subject, node) -> [count, bytes]
    bad = defaultdict(int)
    for cid, data in frames:
        prio, subj, node = decode_id(cid)
        ok, _ = v1_check(cid)
        if not ok:
            bad[cid] += 1
        s = stats[(prio, subj, node)]
        s[0] += 1
        s[1] += len(data)

    print(f"{'prio':>4} {'subject':>8} {'node':>5} {'frames':>7} {'bytes':>7}  type")
    for (prio, subj, node), (n, b) in sorted(stats.items(), key=lambda kv: (kv[0][1], kv[0][2])):
        name = NAMES.get(subj, "")
        print(f"{prio:>4} {subj:>8} {node:>5} {n:>7} {b:>7}  {name}")

    if bad:
        print(f"\n{len(bad)} non-v1.0 frame id(s) present:")
        for cid, n in sorted(bad.items()):
            print(f"  0x{cid:08X} x{n}: {v1_check(cid)[1]}")
    print()

    # --- Reassemble transfers per (subject, node) ---
    # Reorder frames by capture order but group the tail-byte markers.
    tail = 0xAA  # last frame
    cont = 0x00  # non-last frame of a multi-frame transfer
    single = 0x80  # single-frame transfer

    groups = defaultdict(list)
    for cid, data in frames:
        _, subj, node = decode_id(cid)
        groups[(subj, node)].append((cid, data))

    for (subj, node), items in sorted(groups.items()):
        if wanted and subj not in wanted:
            continue
        name = NAMES.get(subj, "")
        if not name:
            continue
        print(f"=== subject {subj} ({name}) from node {node} ===")
        # Walk the stream, joining frames until a tail marker shows up.
        pending = bytearray()
        shown = 0
        for cid, data in items:
            if not data:
                continue
            marker = data[-1]
            pending += data[:-1]
            if marker in (tail, single):
                payload = bytes(pending)
                pending = bytearray()
                if shown < 3:
                    print(f"  payload {len(payload)}B: {payload.hex(' ')}")
                    show_payload(subj, payload)
                shown += 1
        if shown:
            print(f"  ... {shown} complete transfer(s)\n")
        else:
            print("  no complete transfer assembled\n")


def show_payload(subj, p):
    if subj == 7509 and len(p) == 7:
        uptime, health, mode, vendor = struct.unpack("<IBBH", p)
        h = {0: "OK/NOMINAL", 1: "WARNING", 2: "ERROR", 3: "CRITICAL"}.get(health, "?")
        m = {0: "OPERATIONAL", 1: "INITIALIZATION", 2: "MAINTENANCE", 3: "SOFTWARE_UPDATE"}.get(mode, "?")
        print(f"    uptime={uptime}s health={health}({h}) mode={mode}({m}) vendor_specific={vendor}")
    elif subj in (7511, 7512) and len(p) == 12:
        x, y, z = struct.unpack("<fff", p)
        unit = "rad/s" if subj == 7511 else "m/s^2"
        print(f"    x={x:+.5f} y={y:+.5f} z={z:+.5f} {unit}")
    else:
        print(f"    (no built-in decoder for {len(p)}B payload)")


if __name__ == "__main__":
    sys.exit(main())
