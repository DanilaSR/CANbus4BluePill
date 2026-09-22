# Source this file to use yakut against the STM32F103 CAN node:
#   source ./yakut-env.sh
#   yakut subscribe uavcan.node.Heartbeat.1.0 +M
#
# Transport: WeAct USB2CAN (SLCAN over /dev/ttyACM0) at 500 kbit/s via
# python-can, no root required. DSDL sources are compiled by pycyphal into
# ~/.pycyphal on first use (~7 s), afterwards it is cached.
export PATH="$HOME/.local/bin:$PATH"
export CYPHAL_PATH="${CYPHAL_PATH:-$HOME/.cyphal/dsdl}"
export YAKUT_TRANSPORT='CAN(can.media.pythoncan.PythonCANMedia("slcan:/dev/ttyACM0",500000),None)'
