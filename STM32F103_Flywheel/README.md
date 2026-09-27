# STM32F103_Flywheel -- Flywheel (spacecraft unit 3/6)

Cyphal (UAVCAN v1) node for the STM32F103C8T6 Blue Pill, derived from the
`STM32F103_CAN` template. `build.sh`, `Makefile`, `can0.sh` and `yakut-env.sh`
work exactly as in the template; the Makefile defaults `NODE_ID` to this unit's
own ID so `make getinfo`/`make tshark` target it directly.

| Type | Subject | Rate |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | 7509 | 1 Hz (priority `fast`) |

- **Node ID:** 53 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)

- Actuator unit: heartbeat only, exactly as the template. When a
  reaction-wheel driver is added it will consume command transfers here.

All types are standard Cyphal DSDL: the C headers ship with the template's
`DSDLtypes/`, and yakut/pycyphal resolve the sources from `~/.cyphal/dsdl`
without extra setup.

## Build & flash

```sh
./build.sh                 # or: make build
make flash                  # ST-Link (SWD_FREQ=100 kHz by default)
make wireshark              # slcand + Wireshark with the Cyphal dissector
make tshark FILTER=         # live decode on the console
source ./yakut-env.sh && yakut sub uavcan.node.Heartbeat.1.0 +M
```
