#!/usr/bin/env bash
# Build and upload the firmware, then start it.
#
# Uploading over the knob's own USB port reboots it with the ESP32-S3
# "force download boot" flag set (RTC_CNTL_OPTION1_REG bit 0). esptool 4.6,
# shipped with ESP32 core 3.0.x, resets without clearing it, so the knob
# stays in download mode with a dark screen until it is power-cycled. This
# script clears the flag after the upload and resets the chip.
#
# Usage: ./upload.sh [port]   (default: the first Espressif USB device found)
set -euo pipefail
cd "$(dirname "$0")"

IDE_CLI="/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"
IDE_CONFIG="$HOME/.arduinoIDE/arduino-cli.yaml"
if command -v arduino-cli >/dev/null; then
  CLI=(arduino-cli)
else
  CLI=("$IDE_CLI")
fi
[ -f "$IDE_CONFIG" ] && CLI+=(--config-file "$IDE_CONFIG")

ESPTOOL=$(ls -d "$HOME"/Library/Arduino15/packages/esp32/tools/esptool_py/*/esptool | sort -V | tail -1)
RTC_CNTL_OPTION1_REG=0x6000812C
FORCE_DOWNLOAD_BOOT=0x1

espressif_port() {
  "${CLI[@]}" board list --format json | python3 -I -c '
import json, sys
for p in json.load(sys.stdin).get("detected_ports", []):
    props = p.get("port", {}).get("properties", {})
    if props.get("vid", "").lower() == "0x303a":
        print(p["port"]["address"]); break'
}

port=${1:-$(espressif_port)}
[ -n "$port" ] || { echo "No Espressif USB device found. Is the knob plugged in?" >&2; exit 1; }

echo "Uploading via $port"
"${CLI[@]}" compile --upload -p "$port" .

# The board re-enumerates as the ROM's USB serial device after the upload.
sleep 1
port=$(espressif_port)
[ -n "$port" ] || { echo "Knob not found after upload; unplug and replug it." >&2; exit 1; }

echo "Clearing force-download flag and starting firmware via $port"
"$ESPTOOL" --chip esp32s3 --port "$port" --after hard_reset \
  write_mem "$RTC_CNTL_OPTION1_REG" 0 "$FORCE_DOWNLOAD_BOOT"
