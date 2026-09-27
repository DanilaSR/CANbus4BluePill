# STM32F103_GNSS -- GNSS (spacecraft unit 5/6)

Cyphal (UAVCAN v1) node for the STM32F103C8T6 Blue Pill, derived from the
`STM32F103_CAN` template. `build.sh`, `Makefile`, `can0.sh` and `yakut-env.sh`
work exactly as in the template; the Makefile defaults `NODE_ID` to this unit's
own ID so `make getinfo`/`make tshark` target it directly.

| Type | Subject | Rate |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | 7509 | 1 Hz (priority `fast`) |
| `reg.udral.physics.kinematics.geodetic.Point.0.1` | 7513 | 1 Hz (priority `nominal`) |

- **Node ID:** 55 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)

- Node ID 55 is the ID the `STM32F103_CAN` template used; with a single
  BluePill on the bench only one firmware runs at a time, so there is no
  collision (the template is superseded by these six units).
- The fix (lat/lon/altitude) is a drifting placeholder around 45.965 N,
  63.0385 E -- no GNSS receiver is wired; replace the marked lines with a
  real u-blox M8/M10 solution. Timestamp stays 0 (no time sync).

All types are standard Cyphal DSDL: the C headers ship with the template's
`DSDLtypes/`, and yakut/pycyphal resolve the sources from `~/.cyphal/dsdl`
without extra setup.

## Build & flash

```sh
./build.sh                 # or: make build
make flash                  # ST-Link (SWD_FREQ=100 kHz by default)
make wireshark              # slcand + Wireshark with the Cyphal dissector
make tshark FILTER=         # live decode on the console
source ./yakut-env.sh && yakut sub 7513:reg.udral.physics.kinematics.geodetic.Point.0.1 +M
```
