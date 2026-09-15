#!/usr/bin/env python3
"""Decode UAVCAN (Cyphal) heartbeats from slcan frames on the Arduino serial line.

Frame format (as emitted by sketch_slcan.ino):
    T<8x CAN-ID><hex DLC><2x per byte payload><CR>

CAN ID layout (canard_publish_16b, UAVCAN v1.1 16-bit subject):
    bits [28:26] priority
    bit    [25]  SSP (service) flag
    bit    [24]  reserved 0
    bits  [23:8] subject_id
    bit     [7]  reserved 1 (discriminator)
    bits   [6:0] source_node_id

Heartbeat.1.0 payload (7 bytes + 1 CANard tail byte):
    bytes [0:4]  uptime_sec   (uint32 little-endian)
    byte    [4]  health.value
    byte    [5]  mode.value
    byte    [6]  vendor_specific_status_code
    byte    [7]  CANard tail byte: bit7 SOT, bit6 EOT, bit5 TX toggle, bits[4:0] transfer_id

Usage:
    cat /dev/ttyUSB2 | python3 decode_heartbeat.py
    python3 decode_heartbeat.py /dev/ttyUSB2
    python3 decode_heartbeat.py < log.txt
"""

import sys
import time

SUBJECT_HEARTBEAT = 7509

HEALTH = {0: "NOMINAL", 1: "ADVISORY", 2: "CAUTION", 3: "WARNING"}
MODE = {0: "OPERATIONAL", 1: "INITIALIZATION", 2: "MAINTENANCE", 3: "SOFTWARE_UPDATE",
        4: "DEVELOPMENT", 255: "UNKNOWN"}


def hex_to_u32_le(b: bytes) -> int:
    return int.from_bytes(b, byteorder="little")


def fmt_uptime(s: int) -> str:
    return f"{s // 3600:02d}:{s % 3600 // 60:02d}:{s % 60:02d}"


def decode_frame(line: str) -> bytes:
    """Return raw payload bytes for a valid slcan receive frame, else None."""
    if not line or line[0] not in "tTrR":
        return None
    if line[0] in "tT":  # data frame
        if line[0] == "T":  # extended 29-bit
            ident = int(line[1:9], 16)
            rest = line[9:]
        else:  # standard 11-bit
            ident = int(line[1:4], 16)
            rest = line[4:]
        dlc = int(rest[0], 16)
        data_hex = rest[1:]
        if len(data_hex) != dlc * 2:
            return None
        return (ident, bytes.fromhex(data_hex))
    return None  # ignore RTR / error frames


def print_heartbeat(ident: int, data: bytes) -> None:
    prio = (ident >> 26) & 0x7
    subject = (ident >> 8) & 0xFFFF
    node = ident & 0x7F

    if subject != SUBJECT_HEARTBEAT or len(data) < 8:
        print(f"[other] prio={prio} subject={subject} node={node} "
              f"dlc={len(data)} data={data.hex(' ').upper()}")
        return

    uptime = hex_to_u32_le(data[:4])
    health = data[4]
    mode = data[5]
    vendor = data[6]
    tail = data[7]
    tid = tail & 0x1F

    print(f"heartbeat node={node} subject={subject} uptime={uptime}s "
          f"({fmt_uptime(uptime)}) health={HEALTH.get(health, health)} "
          f"mode={MODE.get(mode, mode)} vendor={vendor} tid={tid}")


def line_reader(stream):
    buf = b""
    while True:
        chunk = stream.read(1)
        if not chunk:
            if buf:
                yield buf
            return
        buf += chunk
        if chunk in (b"\r", b"\n"):
            yield buf
            buf = b""


def main() -> int:
    src = sys.argv[1] if len(sys.argv) > 1 else None
    stream = None
    close = False
    if src is None:
        stream = sys.stdin.buffer
    elif src == "/dev/stdin":
        stream = sys.stdin.buffer
    else:
        stream = open(src, "rb", buffering=0)
        close = True

    n_frame = 0
    n_hb = 0
    try:
        for raw in line_reader(stream):
            line = raw.decode(errors="replace").strip()
            decoded = decode_frame(line)
            if decoded is None:
                continue
            ident, data = decoded
            n_frame += 1
            subject = (ident >> 8) & 0xFFFF
            if subject == SUBJECT_HEARTBEAT:
                n_hb += 1
                print_heartbeat(ident, data)
    except KeyboardInterrupt:
        pass
    finally:
        if close:
            stream.close()

    print(f"\n-- decoded {n_frame} frames, {n_hb} heartbeats --", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())