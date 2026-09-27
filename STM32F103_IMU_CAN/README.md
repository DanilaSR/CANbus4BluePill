# STM32F103_IMU_CAN -- IMU (spacecraft unit 2/6)

Cyphal (UAVCAN v1) node for the STM32F103C8T6 Blue Pill, derived from the
`STM32F103_CAN` template. `build.sh`, `Makefile`, `can0.sh` and `yakut-env.sh`
work exactly as in the template; the Makefile defaults `NODE_ID` to this unit's
own ID so `make getinfo`/`make tshark` target it directly.

| Type | Subject | Rate |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | 7509 | 1 Hz (priority `fast`) |
| `uavcan.si.sample.angular_velocity.Vector3.1.0` | 7511 | 10 Hz (priority `nominal`) |
| `uavcan.si.sample.acceleration.Vector3.1.0` | 7512 | 10 Hz (priority `nominal`) |

- **Node ID:** 52 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)

- Named `IMU_CAN` because the repo already contains `STM32F103_IMU`
  (standalone MPU-9250 + OLED project).
- Gyro/accel traces are drifting placeholders -- no IMU chip is wired to
  this board; replace the marked lines in `Core/Src/main.c` with real
  MPU-9250/ICM-20602 readouts. Sample timestamps stay 0 (no time sync).

All types are standard Cyphal DSDL: the C headers ship with the template's
`DSDLtypes/`, and yakut/pycyphal resolve the sources from `~/.cyphal/dsdl`
without extra setup.

## Build & flash

```sh
./build.sh                 # or: make build
make flash                  # ST-Link (SWD_FREQ=100 kHz by default)
make wireshark              # slcand + Wireshark with the Cyphal dissector
make tshark FILTER=         # live decode on the console
source ./yakut-env.sh && yakut sub uavcan.si.sample.angular_velocity.Vector3.1.0 +M
```
