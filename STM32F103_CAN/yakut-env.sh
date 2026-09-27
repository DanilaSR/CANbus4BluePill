# Source this file to use yakut against the STM32F103 CAN node:
#   source ./yakut-env.sh
#   yakut subscribe uavcan.node.Heartbeat.1.0 +M
#
# Transport auto-detection:
#   - can0 present (created by ./can0.sh up, see Wireshark setup): socketcan.
#     While can0 is up, slcand owns /dev/ttyACM0, so the slcan transport
#     below would fail -- socketcan also lets yakut and Wireshark listen
#     to the bus at the same time.
#   - otherwise: WeAct USB2CAN directly (SLCAN over /dev/ttyACM0) at
#     500 kbit/s via python-can, no root required.
# DSDL sources are compiled by pycyphal into ~/.pycyphal on first use (~7 s),
# afterwards it is cached.
export PATH="$HOME/.local/bin:$PATH"
export CYPHAL_PATH="${CYPHAL_PATH:-$HOME/.cyphal/dsdl}"

if ip link show can0 >/dev/null 2>&1; then
    export YAKUT_TRANSPORT='CAN(can.media.pythoncan.PythonCANMedia("socketcan:can0",500000),None)'
else
    export YAKUT_TRANSPORT='CAN(can.media.pythoncan.PythonCANMedia("slcan:/dev/ttyACM0",500000),None)'
fi
