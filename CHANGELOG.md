# Changelog

All notable changes to this project are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses [Semantic Versioning](https://semver.org/).

## [v0.2.0] - 2026-10-31

Adds support for the **Elecrow CrowPanel 1.46" HMI ESP32 Rotary Display** (360x360 IPS screen, 8-LED ring) alongside the 1.28" model in a single auto-detecting firmware image, plus Linux tooling and CI release packaging.

### Added
- **CrowPanel 1.46" (360x360) support**: `Panel_JD9855` display driver (`P146B001-IPS-CTP-V2`), `GPIO17` 5V RGB LED rail enable, 8-LED NeoPixel ring sizing, and scaled 360x360 LVGL dial/typography in `ui.cpp`.
- **Automatic board detection**: detects 1.28" (240x240, 5 LEDs) vs 1.46" (360x360, 8 LEDs) at boot via `GPIO17` pull-down sensing, with compile-time override via `CROWPANEL_SIZE` (`0`, `128`, or `146`) in `config.h`.
- **Serial boot diagnostic**: logs detected board model, resolution, LED count, and CST816 touch status when serial connects.
- **Linux support in `upload.sh`**: locates `~/.local/bin/arduino-cli`, `~/.arduino15`, and `esptool.py`, and handles `--before no_reset` when clearing the force-download boot flag.
- **GitHub Actions CI & release packaging**: builds firmware on push/PR and packages a merged `esp32s3` flash image with SHA-256 checksum on `v*` tags.
- **Brand assets**: project logo, README banner, and social preview image.

## [v0.1.0] - 2026-10-23

First release of Input Knob, firmware for the Elecrow CrowPanel 1.28" ESP32-S3 rotary display.

### Added
- **Volume mode**: turn for volume up/down, click to mute, double-click to play/pause.
- **Scroll mode**: turn for the scroll wheel, click for middle click, double-click to switch vertical/horizontal; swipe up/down on the screen scrolls a page.
- **Macro mode**: pick a shortcut by name and click to send it, with Mac (Cmd) and PC (Ctrl/Win) tables switched by double-click and remembered across power cycles.
- **Trackpad mode**: drag on the touch screen to move the cursor (with acceleration), tap to left-click, touch and hold to right-click, turn the knob to scroll.
- **Touch gestures** in the other modes: tap = knob click, swipe left/right = change mode, swipe up/down = page scroll (Scroll) or next/previous shortcut (Macro).
- **USB HID** (keyboard + mouse + media keys), needing no host drivers.
- **Bluetooth LE HID**, advertised as "Input Knob", with automatic switching: USB when a computer has enumerated the knob, otherwise Bluetooth after 3 seconds.
- **LED ring**: a glow in the mode colour, a needle that follows the knob, flashes for mute, macros and clicks, orange breathing while pairing, and a red flash when the connection drops.
- **Round LVGL UI**: connection status, mode, last action, gesture hints and a dial ring that follows the knob.
- **`upload.sh`**: builds, uploads, then clears the ESP32-S3 force-download flag so the knob starts the new firmware without a replug.
- **Documentation**: README, architecture, purpose, how-to and brochure, plus NotebookLM sources (explainer script, annotated snippets and rendered images).

### Fixed
- The "USB not connected" status was always shown, because the core starts USB before `setup()` and the start event was missed. The firmware now queries the mount state directly.
- After a host sleep/wake the knob could stay unmounted while the host still listed it. A non-blocking watchdog now drops off the bus and reconnects (not yet verified against a real sleep cycle).

### Known limitations
- Untested: recovery after Mac sleep/wake, Windows, iOS/Android, and the PC shortcut table on a PC.
- One Bluetooth host at a time; Bluetooth is only used when the knob isn't on a computer's USB port.
- The USB device name shows as `ESP32S3_DEV`.
- Uploading from the Arduino IDE's Upload button (rather than `upload.sh`) leaves the screen dark until the knob is replugged.

[v0.2.0]: https://github.com/bsommers/input-knob/compare/v0.1.0...v0.2.0
[v0.1.0]: https://github.com/bsommers/input-knob/releases/tag/v0.1.0
