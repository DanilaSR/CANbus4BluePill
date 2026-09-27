# STM32F103_Flywheel -- Flywheel (spacecraft unit 3/6)

Cyphal (UAVCAN v1.0) node for the STM32F103C8T6 Blue Pill, derived from the
`STM32F103_CAN` template. It runs a full reaction-wheel actuator API on top of
a firmware plant model (rigid-body dynamics + viscous drag + thermal model +
over-temperature protection). `build.sh`, `Makefile`, `can0.sh` and
`yakut-env.sh` work exactly as in the template; the Makefile defaults
`NODE_ID` to this unit's own ID so `make getinfo`/`make tshark` target it
directly.

- **Node ID:** 53 (fixed, see `CANARD_NODE_ID` in `Core/Src/main.c`)
- **DSDL package:** `spacecraft.flywheel.*`, sources in
  `~/.cyphal/dsdl/spacecraft/flywheel/` (outside the repo; see
  [DSDL & code generation](#dsdl--code-generation))

| Type | Port | Rate / trigger |
|---|---|---|
| `uavcan.node.Heartbeat.1.0` | subject 7509 | 1 Hz (priority `fast`) |
| `spacecraft.flywheel.SetSpeed.1.0` | service 100 | per request (server) |
| `spacecraft.flywheel.CreateTorque.1.0` | service 101 | per request (server) |
| `spacecraft.flywheel.StartSelfTest.1.0` | service 102 | per request (server) |
| `spacecraft.flywheel.Status.1.0` | subject 7516 | 1 Hz + immediately on state change |

## Services

All three services answer **immediately** — never when the maneuver finishes;
the supervisor tracks progress by subscribing to `Status`.

```sh
CYPHAL_NODE_ID=42 source ./yakut-env.sh   # node-ID must be set BEFORE sourcing
yakut call 53 spacecraft.flywheel.SetSpeed.1.0 '{"target_rpm":1000}'
# {"outcome":0,"current_rpm":0.0,"target_reached":false,"eta_seconds":13}
```

- **SetSpeed (100)** — spin to `target_rpm` (clamped to ±6000 rpm).
  `outcome`: `ACCEPTED`, `AT_MAX_SPEED`, `REJECTED_FAULT` (over-temp latched),
  `REJECTED_BUSY` (self-test running). `eta_seconds` is an estimate only;
  actual progress is read from `Status.target_reached`/`speed_rpm`.
- **CreateTorque (101)** — open-loop torque impulse: torque clamped to
  ±5 mN·m, duration clamped to 120 s. `expected_delta_rpm` gives the ideal
  Δrpm (= torque·duration/J). An accelerating torque is cut at the 6000 rpm
  speed limit (`Status.at_max_speed` reports it); a decelerating one is not.
- **StartSelfTest (102)** — sine-tracking functional test of the actuator
  chain; the verdict arrives in `Status.selftest_state`
  (`RUNNING → PASSED/FAILED`) together with `selftest_error_rpm`.
  Requested amplitude/frequency/duration are clamped (≤600 rpm, ≤0.2 Hz,
  ≤60 s), and the frequency is clamped further to keep the profile
  *trackable* — inside both the actuator authority (peak sine acceleration
  below half of τ_max/J) and the speed-loop bandwidth, so the test cannot
  fail for lack of hardware. The response echoes the values actually used.
  The speed loop is suspended for the duration (the sine is driven around
  the speed captured at start) and the previous speed target resumes
  afterwards — running a self-test never dumps the stored momentum.

## Status (7516)

Published at 1 Hz and immediately after every state change, so the supervisor
never has to poll: subscribing answers "is the wheel alive" even at zero speed
(the heartbeat answers "is the node alive" independently).

Fields: `speed_rpm`, `target_rpm`, `temperature`, `momentum` (L = J·ω),
`state` (`IDLE`/`SPINNING`/`FAULT_OVERTEMP`/`SELFTEST`), `overtemp_count`,
`target_reached`, `at_max_speed`, `selftest_state`, `selftest_error_rpm`,
`reserved` (zero, pads the extent to 28 bytes).

```sh
yakut sub spacecraft.flywheel.Status.1.0 +M
# {"speed_rpm":999.9,"target_rpm":1000.0,"temperature":32.19,"momentum":0.0523,
#  "state":1,"overtemp_count":0,"target_reached":true,"at_max_speed":false,
#  "selftest_state":0,"selftest_error_rpm":0.0}
```

## Fault & health ladder

The thermal model trips a latched fault at 80 °C and recovers below 70 °C.
While latched: drive cut, all three services answer `REJECTED_FAULT`, the
trip counter increments and is mirrored into the heartbeat
(`vendor_specific_status_code`). After recovery the wheel **coasts** until the
next explicit command — the speed loop does not re-engage by itself, because
braking a still-hot, still-fast wheel would feed heat straight back over the
threshold and flap the fault forever.

Heartbeat health: `NOMINAL` → `ADVISORY` (≥1 trip) → `CAUTION` (≥2) →
`WARNING` (fault latched, ≥3 trips, or last self-test FAILED); `mode` stays
`OPERATIONAL` (the node itself is fine).

## Plant model

| Constant | Value | Meaning |
|---|---|---|
| `WHEEL_J` | 5·10⁻⁴ kg·m² | moment of inertia |
| `WHEEL_TAU_MAX` | 5·10⁻³ N·m | max drive torque |
| `WHEEL_MAX_RPM` | 6000 rpm | speed limit |
| `WHEEL_KP` | 5·10⁻³ N·m per rad/s | P speed loop (bandwidth 10 rad/s) |
| `WHEEL_B_DRAG` | 4·10⁻⁷ N·m·s | viscous drag |
| thermal | 0.5·(τ/τmax)²·G(ω) + 0.01·(ω/ωmax)² − 0.0015·(T−25) | °C/s, G = 1+4·max(0,1−rpm/300)² (zero-speed/locked-rotor heating) |
| trip / recover | 80 / 70 °C | fault latch with hysteresis |

## Implementation notes

- **Request handling is IRQ-safe:** the RX callback only queues (SPSC ring,
  `on_request()`), the main loop deserializes and responds
  (`process_requests()`); libcanard is never re-entered from the ISR.
- **TX frame order matters.** libcanard hands out the frames of a transfer
  strictly in order, but bxCAN arbitrates its three TX mailboxes by CAN ID
  and, for equal IDs, by mailbox number — a frame loaded into mailbox 0 while
  an older frame of the same transfer waits in mailbox 2 is transmitted
  *ahead* of it. The receiver then reassembles garbage and the v1 CRC
  residue check silently drops the whole transfer (this is why a 5-frame
  `Status` used to be invisible to `yakut sub` while its frames were clearly
  on the wire). `canard_tx_cb()` therefore accepts a frame only when all
  three mailboxes are empty: at most one frame in flight, nothing to
  reorder, and NART mode guarantees every mailbox frees itself after one
  attempt. libcanard retries the same frame on the next `canard_poll()`.
- **Self-test health coupling:** a FAILED self-test drives heartbeat health
  to `WARNING` (candidate for switching the spacecraft to the backup wheel).

## DSDL & code generation

The four DSDL sources live outside the repo in
`~/.cyphal/dsdl/spacecraft/flywheel/` (`100.SetSpeed.1.0.dsdl`,
`101.CreateTorque.1.0.dsdl`, `102.StartSelfTest.1.0.dsdl`,
`7516.Status.1.0.dsdl`; the numeric prefixes are their fixed port-IDs).
They carry fixed port-IDs that are not in the public registry, so both sides
need them allowed:

- Python side (yakut/pycyphal): `CYPHAL_ALLOW_UNREGULATED_FIXED_PORT_ID=1`,
  already exported by `yakut-env.sh`.
- C side: regenerate the headers in `DSDLtypes/` after any DSDL edit with
  `nnvg` (ships with yakut):

```sh
/home/danila/.local/share/pipx/venvs/yakut/bin/nnvg ~/.cyphal/dsdl/spacecraft \
    -O DSDLtypes -l c --target-endianness little \
    --enable-serialization-asserts --generate-support never \
    --allow-unregulated-fixed-port-id
```

## Build & flash

```sh
./build.sh                 # or: make build
make flash                  # ST-Link (SWD_FREQ=100 kHz by default)
make wireshark              # slcand + Wireshark with the Cyphal dissector
make tshark FILTER=         # live decode on the console
source ./yakut-env.sh && yakut sub uavcan.node.Heartbeat.1.0 +M
```

`CYPHAL_NODE_ID` must be exported **before** `source ./yakut-env.sh` —
`yakut call` needs a registered local node-ID, otherwise the transport comes
up in anonymous mode and refuses service calls.
