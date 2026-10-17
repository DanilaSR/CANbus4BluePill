#!/bin/bash
# Attach the WeAct USB2CAN adapter (SLCAN firmware, 500 kbit/s) to the kernel
# as CAN interface "can0" via slcand. The adapter is found by its USB product
# name (USB2CANV1_ComPort); the ttyACM number changes between boots/plugs, so
# it is NOT hardcoded.
#
#   sudo ./can0.sh up      # create + bring up can0 (needs root)
#   sudo ./can0.sh down    # tear down can0 and release the serial port
#       ./can0.sh status   # show current state (no root needed)
#
# While can0 is up, everything can listen to the bus AT THE SAME TIME:
# Wireshark/tshark (capture on can0), yakut (transport "socketcan:can0",
# see yakut-env.sh auto-detection), candump, etc.
#
# NOTE: slcand owns the adapter's tty while can0 is up, so transports that
# open the serial port directly (yakut "slcan:/dev/ttyACMx") must NOT run in
# parallel -- use the socketcan transport instead.
set -u

IFACE=can0
SPEED_CODE=3          # slcan speed code: S6 = 500000 bit/s
                      # (S0=10k S1=20k S2=50k S3=100k S4=125k S5=250k S6=500k S7=800k S8=1M)

die() { echo "ERROR: $*" >&2; exit 1; }

need_root() {
    [ "$(id -u)" -eq 0 ] || die "run as root: sudo $0 $*"
}

# Locate the USB2CAN adapter: match the udev product name, fall back to the
# only ttyACM node present (or /dev/ttyACM0 as a last resort).
find_dev() {
    local d model
    for d in /dev/ttyACM*; do
        [ -e "$d" ] || continue
        model=$(udevadm info -q property -n "$d" 2>/dev/null | sed -n 's/^ID_MODEL=//p')
        case "$model" in
            *USB2CAN*) echo "$d"; return 0 ;;
        esac
    done
    # Single ttyACM candidate (udevadm may be unavailable): take it.
    set -- /dev/ttyACM*
    if [ -e "$1" ] && [ $# -eq 1 ]; then
        echo "$1"; return 0
    fi
    return 1
}

cmd_up() {
    need_root
    if ip link show "$IFACE" >/dev/null 2>&1; then
        echo "$IFACE already exists:"
        ip -details link show "$IFACE" | sed -n '1,3p'
        exit 0
    fi
    # A slcand without its netdev is stale (e.g. left over from before the
    # adapter was re-plugged and got a new ttyACM number) -- it only holds a
    # dead device node, drop it so it cannot race the fresh instance.
    local stale
    stale=$(pgrep -f "^slcand .* ${IFACE}\$" || true)
    if [ -n "$stale" ]; then
        echo "killing stale slcand: $stale"
        kill $stale 2>/dev/null || true
        sleep 0.5
    fi
    DEV=$(find_dev) || die "USB2CAN adapter not found (adapter unplugged?)"
    echo "adapter: $DEV"
    if fuser "$DEV" >/dev/null 2>&1; then
        echo "WARNING: $DEV is busy (yakut/decode_heartbeat.py running?);" >&2
        echo "         slcand will likely fail to open it." >&2
    fi

    # slcand daemonizes and writes its pidfile to /var/run.
    slcand -o -c -f -s"$SPEED_CODE" -S115200 "$DEV" "$IFACE" \
        || die "slcand failed to start"

    # slcan interfaces carry their bit rate from the slcand -s code,
    # so a plain "up" is enough (bitrate= in netlink is not supported here).
    ip link set "$IFACE" up || die "failed to bring $IFACE up"

    echo "== $IFACE is up:"
    ip -details link show "$IFACE"
    echo
    echo "Listeners now possible in parallel:"
    echo "  tshark -i $IFACE -f ''          # Wireshark CLI (Cyphal Lua dissector)"
    echo "  wireshark -k -i $IFACE          # GUI live capture"
    echo "  source ./yakut-env.sh && yakut subscribe uavcan.node.Heartbeat.1.0 +M"
    echo "  candump $IFACE"
}

cmd_down() {
    need_root
    DEV=$(find_dev 2>/dev/null || true)
    if ip link show "$IFACE" >/dev/null 2>&1; then
        ip link set "$IFACE" down 2>/dev/null || true
    fi
    # slcand writes no pidfile; locate it by cmdline (anchored, so this
    # script's own "$0 down" argument cannot match).
    local pids
    pids=$(pgrep -f "^slcand .* ${IFACE}\$" || true)
    if [ -n "$pids" ]; then
        kill $pids
    fi
    # slcand removes the netdev when it exits; give it a moment.
    for _ in 1 2 3 4 5 6 7 8; do
        ip link show "$IFACE" >/dev/null 2>&1 || break
        sleep 0.4
    done
    if ip link show "$IFACE" >/dev/null 2>&1; then
        ip link delete "$IFACE" 2>/dev/null || true
    fi
    if ip link show "$IFACE" >/dev/null 2>&1; then
        die "$IFACE still present"
    fi
    echo "can0 down; $DEV released"
}

cmd_status() {
    DEV=$(find_dev 2>/dev/null || true)
    if ip link show "$IFACE" >/dev/null 2>&1; then
        ip -details link show "$IFACE"
        echo "-- slcand: $(pgrep -af "^slcand .* ${IFACE}\$" || echo none)"
    else
        echo "can0: down"
    fi
    if [ -n "$DEV" ]; then
        echo "-- $DEV present; users:"
        fuser -v "$DEV" 2>&1 || echo "   (none visible to uid $(id -u); slcand runs as root)"
    else
        echo "-- USB2CAN adapter not found"
    fi
}

case "${1:-}" in
    up)     cmd_up ;;
    down)   cmd_down ;;
    status) cmd_status ;;
    *)      echo "usage: sudo $0 up|down, $0 status" >&2; exit 1 ;;
esac
