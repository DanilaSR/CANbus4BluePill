# STM32F103_SolarSensor -- Solar sensor (spacecraft unit 1/6)

Cyphal (UAVCAN v1) node for the STM32F103C8T6 Blue Pill, derived from the
`STM32F103_CAN` template. `build.sh`, `Makefile`, `can0.sh` and `yakut-env.sh`
work exactly as in the template; the Makefile defaults `NODE_ID` to this unit's
own ID so `make getinfo`/`make tshark` target it directly.

| Type | Subject | Rate |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | 7509 | 1 Hz (priority `fast`) |
| `uavcan.si.sample.luminance.Scalar.1.0` | 7510 | 1 Hz (priority `nominal`) |

- **Node ID:** 51 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)

- `candela_per_square_meter` is a drifting placeholder (500..549 cd/m²) --
  no light sensor is wired to this board; replace the marked line in
  `Core/Src/main.c` with a real photodiode/TSL2591 readout.
- The sample `timestamp` stays 0: this demo has no network time sync.

All types are standard Cyphal DSDL: the C headers ship with the template's
`DSDLtypes/`, and yakut/pycyphal resolve the sources from `~/.cyphal/dsdl`
without extra setup.

## Build & flash

```sh
./build.sh                 # or: make build
make flash                  # ST-Link (SWD_FREQ=100 kHz by default)
make wireshark              # slcand + Wireshark with the Cyphal dissector
make tshark FILTER=         # live decode on the console
source ./yakut-env.sh && yakut sub uavcan.si.sample.luminance.Scalar.1.0 +M
```
