# STM32F103_TemperatureSensor -- temperature sensor (spacecraft unit 6/6)

Cyphal (UAVCAN v1) node for the STM32F103C8T6 Blue Pill, **merged from the
`STM32F103_CAN` template and `STM32DISC_TEMP`** (which was removed): the unit
reads an **STLM75** digital temperature sensor over I2C1 and reports it both
as a Cyphal sample on the CAN bus and as ASCII over UART to a Flipper Zero.
`build.sh`, `Makefile`, `can0.sh` and `yakut-env.sh` work exactly as in the
template; the Makefile defaults `NODE_ID` to 56 so `make getinfo`/`make tshark`
target this unit directly.

| Type | Subject | Rate |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | 7509 | 1 Hz (priority `fast`) |
| `uavcan.si.sample.temperature.Scalar.1.0` | 7514 | 1 Hz (priority `nominal`) |

- **Node ID:** 56 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)

## Sensor: STLM75 over I2C1

- Pins: PB6 = SCL, PB7 = SDA, 100 kHz; **external pull-ups are required**
  (usually present on the sensor breakout board).
- The 7-bit address (0x48..0x4F, A2:A0 strapping) is scanned **once at
  boot** and announced on the UART (`ADDR:0xNN\n`).
- `kelvin = degC + 273.15`, read and published every
  `TEMP_REPORT_INTERVAL_MS` (1 s, `Core/Inc/main.h`).
- **No sensor wired → heartbeat only:** subject 7514 stays silent until a
  sensor is attached and the board is power-cycled (boot reports `ERR\n`).
- If a sensor found at boot stops responding, the I2C HAL error code goes to
  the UART (`ERROR:0x...`) and nothing is published on the bus.
- The sample `timestamp` is 0: this project has no network time sync yet.

## Flipper Zero bridge (USART2, 115200 8N1)

The ASCII protocol from `STM32DISC_TEMP` is preserved on PA2 (TX) / PA3 (RX):

```text
TEMP:23.5\n      unit -> Flipper, every second (same read as subject 7514)
PING\n           Flipper -> unit; answered with PONG\n
READ\n           Flipper -> unit; answered with TEMP:...\n on the next tick
ADDR:0x48\n      boot: sensor address detected
ERR\n            boot: no sensor found on the bus
ERROR:0x00000004 I2C HAL error diagnostic (sensor lost mid-run)
```

## Build & flash

```sh
./build.sh                 # or: make build
make flash                  # ST-Link (SWD_FREQ=100 kHz by default)
make wireshark              # slcand + Wireshark with the Cyphal dissector
make tshark FILTER=         # live decode on the console
source ./yakut-env.sh && yakut sub 7514:uavcan.si.sample.temperature.Scalar.1.0 +M
```

All types are standard Cyphal DSDL: the C headers ship with the shared
`DSDLtypes/` (repository root, symlinked into this project), and
yakut/pycyphal resolve the sources from `~/.cyphal/dsdl`
without extra setup. Subject 7514 has no fixed port-ID, hence the explicit
`7514:` prefix in the subscription.
