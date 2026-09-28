# Source this file to use yakut against the STM32F103 CAN node:
#   source ./yakut-env.sh
#   yakut subscribe uavcan.node.Heartbeat.1.0 +M
#
# Transport auto-detection:
#   - can0 present (created by ./can0.sh up, see Wireshark setup): socketcan.
#     While can0 is up, slcand owns the adapter's tty, so the slcan transport
#     below would fail -- socketcan also lets yakut and Wireshark listen
#     to the bus at the same time.
#   - otherwise: WeAct USB2CAN directly (SLCAN over the auto-detected ttyACM) at
#     500 kbit/s via python-can, no root required.
# DSDL sources are compiled by pycyphal into ~/.pycyphal on first use (~7 s),
# afterwards it is cached.
export PATH="$HOME/.local/bin:$PATH"
export CYPHAL_PATH="${CYPHAL_PATH:-$HOME/.cyphal/dsdl}"

if ip link show can0 >/dev/null 2>&1; then
    export YAKUT_TRANSPORT='CAN(can.media.pythoncan.PythonCANMedia("socketcan:can0",500000),None)'
else
    # The adapter's ttyACM number changes between boots/plugs; find it by the
    # USB product name (fallback: first /dev/ttyACM*).
    _ACM=
    for _d in /dev/ttyACM*; do
        [ -e "$_d" ] || continue
        _m=$(udevadm info -q property -n "$_d" 2>/dev/null | sed -n 's/^ID_MODEL=//p')
        case "$_m" in *USB2CAN*) _ACM="$_d"; break ;; esac
    done
    [ -n "$_ACM" ] || _ACM=/dev/ttyACM0
    export YAKUT_TRANSPORT="CAN(can.media.pythoncan.PythonCANMedia(\"slcan:${_ACM}\",500000),None)"
fi
