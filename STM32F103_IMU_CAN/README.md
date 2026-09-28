# STM32F103_IMU_CAN -- IMU (spacecraft unit 2/6)

Cyphal (UAVCAN v1) node for the STM32F103C8T6 Blue Pill: the merged
successor of `STM32F103_IMU` (MPU-9250 + SSD1306 + CSV stream, now
removed), derived from the `STM32F103_CAN` template. `build.sh`, `Makefile`,
`can0.sh` and `yakut-env.sh` work exactly as in the template; the Makefile
defaults `NODE_ID` to this unit's own ID so `make getinfo`/`make tshark`
target it directly.

| Type | Subject | Rate |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | 7509 | 1 Hz (priority `fast`) |
| `uavcan.si.sample.angular_velocity.Vector3.1.0` | 7511 | 10 Hz (priority `nominal`) |
| `uavcan.si.sample.acceleration.Vector3.1.0` | 7512 | 10 Hz (priority `nominal`) |

- **Node ID:** 52 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)
- One I²C1 bus (PB6/PB7, 100 kHz) carries both the **MPU-9250/6500**
  (0x68) and the **0.91" SSD1306 OLED** (0x3C); USART1 (PA9 TX, 115200)
  streams CSV for `visualize.py`.
- Every 100 ms the same sample feeds three sinks:
  Cyphal subjects 7511/7512 in SI units (rad/s, m/s², converted from the
  driver's deg/s and g), the CSV line `ax,ay,az,gx,gy,gz,temp` in the
  driver's original units (g, deg/s, degC) so `visualize.py` works
  unchanged, and the OLED mirror (accel / gyro / temp / tick).
- **No sensor wired:** `MPU9250_Init()` fails its WHO_AM_I probe at boot,
  and the unit heartbeats without publishing 7511/7512 or emitting any
  CSV/OLED traffic — wire the module to PB6/PB7 (external pull-ups on the
  module) and power-cycle to go live. An absent OLED is harmless: its
  writes simply NACK. Sample timestamps stay 0 (no network time sync).
- `visualize.py` (host side) reads the CSV stream and plots it live with
  matplotlib: `python3 visualize.py [port]`.

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
source ./yakut-env.sh && yakut sub 7511:uavcan.si.sample.angular_velocity.Vector3.1.0 +M
```
