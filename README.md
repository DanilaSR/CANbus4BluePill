# CAN bus for STM32 Blue Pill

This is usage of wide-known libcanard library for STM32 Blue Pill. The aim is connecting several sensors between each other.

## Building everything (root Makefile)

A single Makefile in the repository root drives all eight projects (each
project keeps its own Makefile — the root one only dispatches, so per-project
flags like `FILTER=`, `SN=`, `GETINFO_TIMEOUT=` still pass through):

```sh
make                 # build all eight projects (default target)
make list            # alias -> directory -> node-ID
make build-flywheel  # build one project
make flash-central   # build + flash one project via ST-Link
make getinfo-gnss    # yakut GetInfo on that project's node
make clean           # remove every build/ directory
make can0-up         # attach the WeAct adapter as can0 (sudo)
make can0-down       # release it (sudo)
make can0-status     # can0 + adapter state, no sudo
make wireshark       # live capture (tshark for headless; FILTER='...')
make help            # list all targets
```

Aliases: `template` `solar` `imu` `flywheel` `coil` `gnss` `temp` `central`;
a full directory name works too (`make build-STM32F103_GNSS`).

## Spacecraft units

Six node projects derived from the `STM32F103_CAN` template, plus the central
processor (below). Each project keeps its own `build.sh`, `Makefile`,
`yakut-env.sh` and `README.md`; everything byte-identical across projects
(`can0.sh`, `Drivers/`, `lib/`, `DSDLtypes/`, `decode_heartbeat.py`,
`hb-analyze.py`, `yakut_monitor.sh`) is stored once in the repository root and
symlinked into each project (`build/` and the like are ignored by the single
root `.gitignore`), so every per-project command works exactly as before. All
six publish `uavcan.node.Heartbeat.1.0` (subject 7509) at 1 Hz; the sensor
units and the flywheel additionally publish their own DSDL type:

| Unit | Project | Node ID | Data subject | DSDL type | Rate |
|---|---|---|---|---|---|
| Solar sensor | `STM32F103_SolarSensor` | 51 | 7510 (α), 7515 (β) | `uavcan.si.sample.angle.Scalar.1.0` | 1 Hz |
| IMU | `STM32F103_IMU_CAN` | 52 | 7511, 7512 | `uavcan.si.sample.angular_velocity.Vector3.1.0`, `uavcan.si.sample.acceleration.Vector3.1.0` | 10 Hz |
| Flywheel | `STM32F103_Flywheel` | 53 | 7516 | `spacecraft.flywheel.Status.1.0` + services 100–102 (`SetSpeed`, `CreateTorque`, `StartSelfTest`) | 1 Hz + on change |
| Coil | `STM32F103_Coil` | 54 | — | (heartbeat only) | — |
| GNSS | `STM32F103_GNSS` | 55 | 7513 | `reg.udral.physics.kinematics.geodetic.Point.0.1` | 1 Hz |
| Temperature sensor | `STM32F103_TemperatureSensor` | 56 | 7514 | `uavcan.si.sample.temperature.Scalar.1.0` | 1 Hz |

All node projects, including the central processor and the `STM32F103_CAN`
template, also serve `uavcan.node.GetInfo.1.0` (service 430) from the shared
`lib/node_getinfo.c`: each reports its own name, hardware/software version and
the 96-bit STM32 unique device ID, so `make getinfo-<name>` gets an answer.

Notes:

- Standard Cyphal types only: the C headers already ship with the shared
  `DSDLtypes/` in the repository root (symlinked into each project), and
  yakut/pycyphal resolve them from `~/.cyphal/dsdl` — no
  extra DSDL tooling needed. Subscribe with an explicit subject-ID, e.g.
  `yakut sub 7514:uavcan.si.sample.temperature.Scalar.1.0 +M`.
  **Exception:** `STM32F103_Flywheel` defines its own package
  `spacecraft.flywheel.*` (sources in `~/.cyphal/dsdl/spacecraft/flywheel/`,
  generated C in the shared `DSDLtypes/`); its fixed port-IDs (100/101/102 services,
  7516 message) are not in the public registry, so both sides allow them via
  `CYPHAL_ALLOW_UNREGULATED_FIXED_PORT_ID=1` (already set in that unit's
  `yakut-env.sh`), and editing the DSDL requires regenerating the headers with
  `nnvg` — see `STM32F103_Flywheel/README.md`.
- The sensor payloads are drifting placeholders — no sensor ICs are wired to
  the boards; replace the marked lines in each `Core/Src/main.c` with real
  driver readouts. Sample timestamps stay 0 (no network time sync yet).
  **Exceptions** (both merged from standalone projects that are now removed):
  - `STM32F103_TemperatureSensor` merged the real STLM75 (I²C) driver from
    `STM32DISC_TEMP` — it publishes real readings when a sensor is wired to
    PB6/PB7, and heartbeat-only (no subject 7514) until then. It also keeps
    `STM32DISC_TEMP`'s Flipper Zero ASCII bridge on USART2 (PA2/PA3).
  - `STM32F103_IMU_CAN` merged the real MPU-9250 + SSD1306 + CSV-stream
    firmware from `STM32F103_IMU` — real gyro/accel (converted to SI units)
    when a sensor is on I²C1 PB6/PB7, heartbeat-only (no 7511/7512) until
    then; the OLED mirror, the USART1 CSV line and `visualize.py` came along.
- Node 55 was the template's original ID; the template is superseded by these
  units (a single BluePill runs one unit at a time), so there is no collision.
- `STM32F103_TemperatureSensor` (node 56) was build-flashed-verified on
  hardware: heartbeat + subject 7514 decode cleanly in yakut and Wireshark;
  after the STLM75 merge the heartbeat-only no-sensor mode was re-verified.
- `STM32F103_IMU_CAN` was named to avoid a clash with the pre-existing
  `STM32F103_IMU` (standalone MPU-9250 + OLED project); that project has
  since been merged into this unit and removed.
- `STM32F103_IMU_CAN` (node 52) was build-flashed-verified on hardware:
  heartbeat decodes in yakut and Wireshark; with no MPU-9250 wired the
  heartbeat-only mode was verified (no subjects 7511/7512 on the bus).
- `STM32F103_Flywheel` (node 53) was build-flashed-verified on hardware: all
  three services answer in yakut, `Status` (subject 7516) decodes in yakut and
  Wireshark (5-frame multi-frame transfer), spin-up/self-test/over-temperature
  behavior observed end-to-end, and the heartbeat health ladder reacts to the
  self-test verdict and the over-temperature fault. See
  `STM32F103_Flywheel/README.md`.

## Central processor (node 50)

`STM32F103_CentralProcessor` is the seventh project: the spacecraft central
processor analog (OBC). It subscribes to **all** of the above — heartbeats on
7509 plus subjects 7510–7516 — and streams a human-readable report to USART1
(PA9 TX, 115200 8N1): `EVENT ...` lines on every change (unit online/offline,
health transition, subject live/stale, flywheel state/self-test/over-temp)
and a 1 Hz status block (fleet header + one line per unit with heartbeat age
and its latest values). Its own heartbeat mirrors fleet health:
`NOMINAL` = all six units heard within 5 s, `ADVISORY` = 3–5, `CAUTION` = 1–2,
`WARNING` = 0, with the number of offline units in
`vendor_specific_status_code`.

Verified over CAN (no USB-TTL adapter on the bench — the UART text is covered
by a host-side unit test of the formatting helpers, the aggregation by the
health ladder reacting live to simulated units): with the full six-process
fleet on `can0`, node 50 held `NOMINAL`/`vendor_specific_status_code 0` for
30+ minutes (330 consecutive 1 Hz heartbeats with zero gaps and monotonic
uptime), dropping to `ADVISORY`/`vss 1` within 6 s of killing one simulated
unit and to `CAUTION`/`vss 4` after killing four, and returning to
`NOMINAL`/`vss 0` on restart — `mode` stayed `OPERATIONAL` throughout.
Note that the SLCAN serial port serves **one process only** — the full
six-process fleet simulation requires `sudo make can0-up` (socketcan), see
`STM32F103_CentralProcessor/README.md`.

The first hardware run also uncovered (and fixed) a `payload.origin` leak:
the RX callbacks of node 50 and the Flywheel must release the multi-frame
payload per the canard.h contract, otherwise the heap grows into the ISR
stack and the node hangs inside the CAN RX interrupt. See
`STM32F103_CentralProcessor/README.md` ("Implementation notes") for the full
failure analysis.
