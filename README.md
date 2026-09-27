# CAN bus for STM32 Blue Pill

This is usage of wide-known libcanard library for STM32 Blue Pill. The aim is connecting several sensors between each other.

## Spacecraft units

Six node projects derived from the `STM32F103_CAN` template. Each is
self-contained (own `build.sh`, `Makefile`, `can0.sh`, `yakut-env.sh`,
`README.md`); the tools work exactly as in the template. All six publish
`uavcan.node.Heartbeat.1.0` (subject 7509) at 1 Hz; the sensor units and the
flywheel additionally publish their own DSDL type:

| Unit | Project | Node ID | Data subject | DSDL type | Rate |
|---|---|---|---|---|---|
| Solar sensor | `STM32F103_SolarSensor` | 51 | 7510 (α), 7515 (β) | `uavcan.si.sample.angle.Scalar.1.0` | 1 Hz |
| IMU | `STM32F103_IMU_CAN` | 52 | 7511, 7512 | `uavcan.si.sample.angular_velocity.Vector3.1.0`, `uavcan.si.sample.acceleration.Vector3.1.0` | 10 Hz |
| Flywheel | `STM32F103_Flywheel` | 53 | 7516 | `spacecraft.flywheel.Status.1.0` + services 100–102 (`SetSpeed`, `CreateTorque`, `StartSelfTest`) | 1 Hz + on change |
| Coil | `STM32F103_Coil` | 54 | — | (heartbeat only) | — |
| GNSS | `STM32F103_GNSS` | 55 | 7513 | `reg.udral.physics.kinematics.geodetic.Point.0.1` | 1 Hz |
| Temperature sensor | `STM32F103_TemperatureSensor` | 56 | 7514 | `uavcan.si.sample.temperature.Scalar.1.0` | 1 Hz |

Notes:

- Standard Cyphal types only: the C headers already ship with the template's
  `DSDLtypes/`, and yakut/pycyphal resolve them from `~/.cyphal/dsdl` — no
  extra DSDL tooling needed. Subscribe with an explicit subject-ID, e.g.
  `yakut sub 7514:uavcan.si.sample.temperature.Scalar.1.0 +M`.
  **Exception:** `STM32F103_Flywheel` defines its own package
  `spacecraft.flywheel.*` (sources in `~/.cyphal/dsdl/spacecraft/flywheel/`,
  generated C in its `DSDLtypes/`); its fixed port-IDs (100/101/102 services,
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
