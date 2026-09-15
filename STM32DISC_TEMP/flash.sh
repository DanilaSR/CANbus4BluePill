#!/usr/bin/env bash
#-------------------------------------------------------------------------------
# Flash STM32L100RCT6 via ST-Link using STM32CubeProgrammer CLI.
#
# Usage:
#   ./flash.sh            # flash firmware.hex (SWD, verify, reset & run)
#   ./flash.sh <file.hex> # flash a specific file
#-------------------------------------------------------------------------------
set -euo pipefail

PROG="STM32_Programmer_CLI"
HEX="${1:-build/firmware.hex}"

if ! command -v "$PROG" >/dev/null 2>&1; then
  echo "error: $PROG not on PATH." >&2
  echo "Add the STM32CubeProgrammer bin dir, or run with the full path:" >&2
  echo "  /home/danila/st/stm32cubeclt_1.18.0/STM32CubeProgrammer/bin/$PROG" >&2
  exit 1
fi

if [[ ! -f "$HEX" ]]; then
  echo "error: $HEX not found. Run 'make' first." >&2
  exit 1
fi

echo "Flashing $HEX ..."
"$PROG" -c port=SWD -w "$HEX" -v -rst
echo "Done."
