# STM32F103_CentralProcessor -- central processor (spacecraft OBC)

Cyphal (UAVCAN v1.0) node for the STM32F103C8T6 Blue Pill, derived from the
`STM32F103_CAN` template. It is the seventh project of the fleet: the central
processor analog (OBC) that **subscribes to every unit on the bus** and streams
a human-readable report to USART1 (PA9 TX, 115200 8N1). `build.sh`,
`Makefile`, `can0.sh` and `yakut-env.sh` work exactly as in the template; the
Makefile defaults `NODE_ID` to 50 so `make getinfo`/`make tshark` target it
directly.

- **Node ID:** 50 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)
- **Console:** USART1 PA9/PA10, 115200 8N1, text lines only (no protocol)
- **DSDL:** standard types only (Flywheel's `spacecraft.flywheel.Status.1.0`
  header is copied into `DSDLtypes/`); no new DSDL, no `nnvg` step

## Subscribed subjects

| Subject | Type | Unit (node) | Rate |
|---|---|---|---|
| 7509 | `uavcan.node.Heartbeat.1.0` | all (51–56) | 1 Hz |
| 7510 / 7515 | `uavcan.si.sample.angle.Scalar.1.0` | solar α / β (51) | 1 Hz |
| 7511 / 7512 | `uavcan.si.sample.angular_velocity.Vector3.1.0` / `...acceleration.Vector3.1.0` | IMU (52) | 10 Hz |
| 7513 | `reg.udral.physics.kinematics.geodetic.Point.0.1` | GNSS (55) | 1 Hz |
| 7514 | `uavcan.si.sample.temperature.Scalar.1.0` | temperature (56) | 1 Hz |
| 7516 | `spacecraft.flywheel.Status.1.0` | flywheel (53) | 1 Hz + on change |

Coil (54) is heartbeat-only: it appears in the report with its heartbeat age
and nothing else. Liveness thresholds are 5 s for both heartbeats and data
(`HB_TIMEOUT_US` / `DATA_TIMEOUT_US` in `main.c`).

## Console output

On boot a banner, then two kinds of lines (timestamp `[s.mmm]` prefix on each):

- **`EVENT ...` lines** the moment something changes: a unit coming online or
  offline, a health transition of a unit, a subject going live/stale, and the
  flywheel's state / self-test / over-temperature / speed-limit changes.
- **1 Hz status block:** a fleet header
  `=== units online n/6, health X, uptime T s, rx N frames ===`
  followed by one line per expected unit:
  heartbeat age, then its latest values, e.g.

```
[1234.500]51 solar   hb   0.3s | alpha 12.345deg beta -3.210deg
[1234.500]52 imu     hb   0.4s | w 0.010 0.020 0.005 rad/s a 0.000 -0.010 9.810 m/s2
[1234.500]53 flywheel hb   0.2s | SPINNING 1000.0/1000.0rpm T 45.3C mom 0.052 sts idle
[1234.500]54 coil    hb   0.7s | (heartbeat only)
[1234.500]55 gnss    hb   0.5s | lat 45.965000deg lon 63.038500deg alt 60.0m
[1234.500]56 temp    hb   0.6s | 294.15K (21.0C)
```

Values not yet seen print `--` with a `[no data]` / `[data stale]` suffix.
Wire units are converted for humans: solar angles rad → deg, temperature K →
K (°C), GNSS lat/lon printed as degrees with 6 decimals.

## Own heartbeat: fleet health ladder

Node 50 publishes `Heartbeat.1.0` at 1 Hz whose health mirrors the fleet
(live units = heartbeat heard within 5 s):

| Online | Health | `vendor_specific_status_code` |
|---|---|---|
| 6 (all) | `NOMINAL` | 0 |
| 3–5 | `ADVISORY` | number of offline units |
| 1–2 | `CAUTION` | number of offline units |
| 0 | `WARNING` | 6 |

`mode` stays `OPERATIONAL` — the node itself is always fine; health describes
the *spacecraft*, not the OBC. So a watcher can follow fleet health with
nothing but the node's own heartbeat:

```sh
source ./yakut-env.sh && yakut sub uavcan.node.Heartbeat.1.0 +M
# {"uptime":...,"health":{"value":3},"mode":{"value":0},"vendor_specific_status_code":6}
```

## Verification on hardware (no UART adapter)

There is no USB-TTL adapter on the bench, so the UART text itself is verified
by a host-side unit test of the formatting helpers (`fmt_f`/`fmt_deg`:
19 cases — signs, rounding carry, 6-decimal lat/lon — all pass) and the
aggregation is verified **indirectly over CAN**: the health ladder and the
offline counter change exactly as units appear/disappear on the bus.

```sh
# 1 unit alive -> CAUTION / vss 5 (0 units -> WARNING / vss 6 at boot):
(export CYPHAL_NODE_ID=56; source ./yakut-env.sh; \
 yakut pub -T 1 7514:uavcan.si.sample.temperature.Scalar.1.0 '{kelvin: 294.15}') &
source ./yakut-env.sh && yakut sub uavcan.node.Heartbeat.1.0 +M
# node 50: health 3 (WARNING) vss 6 -> health 2 (CAUTION) vss 5
```

**One SLCAN serial port = one process.** Two `yakut` processes opening the
adapter directly both fail with `multiple access on port` — this is why the
full-fleet simulation needs `can0` (socketcan via `slcand`, which lets any
number of listeners/publishers share the bus):

```sh
sudo make can0-up        # or: sudo ./can0.sh up
# then one yakut process per simulated unit -- see "Fleet simulation" below
```

## Fleet simulation (yakut pub)

`yakut pub` with a configured local node publishes heartbeats automatically,
so each simulated unit is a single background process; `CYPHAL_NODE_ID` must
be exported **before** `source ./yakut-env.sh`:

```sh
cd STM32F103_CentralProcessor
. ./yakut-env.sh          # can0 -> socketcan transport (auto-detected)

(CYPHAL_NODE_ID=51; . ./yakut-env.sh; exec yakut pub -T 1 \
    7510:uavcan.si.sample.angle.Scalar.1.0 '{radian: 0.215}' \
    7515:uavcan.si.sample.angle.Scalar.1.0 '{radian: -0.056}') &
(CYPHAL_NODE_ID=52; . ./yakut-env.sh; exec yakut pub -T 0.1 \
    7511:uavcan.si.sample.angular_velocity.Vector3.1.0 '{radian_per_second: [0.01, 0.02, 0.005]}' \
    7512:uavcan.si.sample.acceleration.Vector3.1.0 '{meter_per_second_per_second: [0.0, -0.01, 9.81]}') &
(CYPHAL_NODE_ID=53; . ./yakut-env.sh; exec yakut pub -T 1 \
    7516:spacecraft.flywheel.Status.1.0 \
    '{speed_rpm: 1000.0, target_rpm: 1000.0, temperature: 45.3, momentum: 0.052,
      state: 1, overtemp_count: 0, target_reached: true, at_max_speed: false,
      selftest_state: 0, selftest_error_rpm: 0.0}') &
(CYPHAL_NODE_ID=54; . ./yakut-env.sh; exec yakut sub uavcan.node.Heartbeat.1.0) &
(CYPHAL_NODE_ID=55; . ./yakut-env.sh; exec yakut pub -T 1 \
    7513:reg.udral.physics.kinematics.geodetic.Point.0.1 \
    '{latitude: 45.965, longitude: 63.0385, altitude: {meter: 60.0}}') &
(CYPHAL_NODE_ID=56; . ./yakut-env.sh; exec yakut pub -T 1 \
    7514:uavcan.si.sample.temperature.Scalar.1.0 '{kelvin: 294.15}') &
```

Expected: node 50 walks `WARNING/6 → CAUTION/5 → ... → NOMINAL/0` as the
processes start, and each `EVENT ... online/offline` transition when one is
killed/restarted. Node 54's `yakut sub` publishes only heartbeats (it
subscribes to itself trivially and prints nothing else).

`yakut-env.sh` exports `CYPHAL_ALLOW_UNREGULATED_FIXED_PORT_ID=1` (needed for
subject 7516, `spacecraft.flywheel.Status.1.0`), and its transport auto-picks
`socketcan:can0` when `can0` is up, else the adapter's SLCAN device
(auto-detected by USB product name — the ttyACM number moves between boots).

## Implementation notes

- **Concurrency model (same as the other units):** libcanard delivers complete
  transfers inside the CAN RX0 IRQ; the callbacks only deserialize into
  **seqlocked slots** and update shadows. The main loop masks
  `USB_LP_CAN1_RX0_IRQn` (`canard_lock`/`unlock`) around its own
  `canard_publish_13b`/`canard_poll` calls, because libcanard is explicitly
  not thread-safe; frames arriving meanwhile wait in the RX FIFO (the masked
  section is microseconds). Printing is blocking UART TX; RX keeps running.
- **TX frame order** uses the Flywheel fix: `canard_tx_cb()` accepts a frame
  only when all three TX mailboxes are free (`< 3U free` → reject): at most
  one frame in flight, so the bxCAN mailboxes can never reorder frames of one
  multi-frame transfer (the v1 CRC check would then silently drop it).
  Relevant here for the 5-frame `Status` transfer received from unit 53.
- **No `%f` in printf:** newlib-nano's float formatting is broken/absent, so
  all numbers go through the integer-scaling `fmt_f`/`fmt_deg` helpers with
  explicit sign handling (host-tested, see above).
- **Heartbeat table** has 8 slots (6 expected units + 2 spare, oldest
  recycled), so a stray node ID cannot evict a real unit.

## Build & flash

```sh
./build.sh                 # or: make build
make flash                  # ST-Link (SWD_FREQ=100 kHz by default)
sudo make can0-up           # slcand + socketcan (any number of yakut/tshark)
make tshark FILTER=         # live decode on the console
source ./yakut-env.sh && yakut sub uavcan.node.Heartbeat.1.0 +M
```

`CYPHAL_NODE_ID` must be exported **before** `source ./yakut-env.sh` if the
process needs a registered local node (publishing); a plain subscriber works
anonymously as-is.
