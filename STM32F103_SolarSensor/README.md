# STM32F103_SolarSensor -- Solar sensor (spacecraft unit 1/6)

Cyphal (UAVCAN v1) node for the STM32F103C8T6 Blue Pill, derived from the
`STM32F103_CAN` template. `build.sh`, `Makefile`, `can0.sh` and `yakut-env.sh`
work exactly as in the template; the Makefile defaults `NODE_ID` to this unit's
own ID so `make getinfo`/`make tshark` target it directly.

| Type | Subject | Rate |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | 7509 | 1 Hz (priority `fast`) |
| `uavcan.si.sample.angle.Scalar.1.0` (α) | 7510 | 1 Hz (priority `nominal`) |
| `uavcan.si.sample.angle.Scalar.1.0` (β) | 7515 | 1 Hz (priority `nominal`) |

- **Node ID:** 51 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)

- The solar sensor returns two sun aspect angles, **alpha** (subject 7510)
  and **beta** (subject 7515), both `uavcan.si.sample.angle.Scalar.1.0`
  (`radian`, float32), serialized back-to-back from the same 1 Hz sample.
  The standard DSDL has no `angle.Vector2`, hence two Scalar subjects --
  subscribe to both to get the pair.
- `radian` is a synthetic placeholder: a sawtooth drift within ±60° (a
  typical coarse sun sensor field of view), alpha and beta on different
  periods. No solar sensor is wired to this board; replace the marked block
  in `Core/Src/main.c` with a real photodiode/ADC readout.
- The sample `timestamp` stays 0: this demo has no network time sync.

All types are standard Cyphal DSDL: the C headers ship with the shared
`DSDLtypes/` (repository root, symlinked into this project), and
yakut/pycyphal resolve the sources from `~/.cyphal/dsdl`
without extra setup.

## Build & flash

```sh
./build.sh                 # or: make build
make flash                  # ST-Link (SWD_FREQ=100 kHz by default)
make wireshark              # slcand + Wireshark with the Cyphal dissector
make tshark FILTER=         # live decode on the console
source ./yakut-env.sh && yakut sub 7510:uavcan.si.sample.angle.Scalar.1.0 7515:uavcan.si.sample.angle.Scalar.1.0 +M
```
