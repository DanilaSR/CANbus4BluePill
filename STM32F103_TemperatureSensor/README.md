# STM32F103_TemperatureSensor -- Temperature sensor (spacecraft unit 6/6)

Cyphal (UAVCAN v1) node for the STM32F103C8T6 Blue Pill, derived from the
`STM32F103_CAN` template. `build.sh`, `Makefile`, `can0.sh` and `yakut-env.sh`
work exactly as in the template; the Makefile defaults `NODE_ID` to this unit's
own ID so `make getinfo`/`make tshark` target it directly.

| Type | Subject | Rate |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | 7509 | 1 Hz (priority `fast`) |
| `uavcan.si.sample.temperature.Scalar.1.0` | 7514 | 1 Hz (priority `nominal`) |

- **Node ID:** 56 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)

- `kelvin` is a placeholder drifting around 290 K -- no temperature
  sensor is wired to this board; replace the marked line in
  `Core/Src/main.c` with a real TMP102/DS18B20 readout.
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
source ./yakut-env.sh && yakut sub uavcan.si.sample.temperature.Scalar.1.0 +M
```
