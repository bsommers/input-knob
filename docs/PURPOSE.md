# Purpose

## Problem Statement

Adjusting volume, scrolling a long page or firing a common shortcut means reaching for the keyboard, hunting for a function key or opening a menu. Dedicated USB volume knobs and macro pads exist, but most have no screen, fixed functions, and often need a vendor app. The Elecrow CrowPanel 1.28" rotary display has the right hardware for a better one (a detented knob, a push button, a round colour screen and an ESP32-S3 with native USB and Bluetooth), but it ships with demo firmware rather than an input device.

---

## Solution

Firmware that makes the CrowPanel enumerate as a standard HID keyboard + mouse + media-key device, with four modes (Volume, Scroll, Macro, Trackpad) you cycle through with a long press or a swipe. Because it uses only standard HID reports, macOS, Windows, Linux, iOS, iPadOS and Android all accept it without drivers. The screen shows what the knob will do next and how it's connected.

---

## Goals

1. **Works with no host software** — standard HID only; nothing to install on the computer or phone.
2. **Predictable controls** — the same gestures in every mode (turn / click / double-click / long press), with the screen always saying what each one does.
3. **Connects itself** — USB when on a computer, Bluetooth otherwise, with no setting to flip.
4. **Easy to customise in code** — shortcuts are a table; pins and timings are in one header each.
5. **Reproducible builds** — pinned library versions and board settings committed in `sketch.yaml`.

## Non-Goals

- **Not** a configurable macro-pad platform with a host app or on-device editor. Editing `macros.h` and reflashing is the configuration story, which keeps the firmware small.
- **Not** absolute volume control. HID can only send volume up/down; the knob doesn't know or show the host's actual volume.
- **Not** multi-host Bluetooth switching. One bonded host is active at a time.
- **Not** battery-powered. The board has no battery; Bluetooth use assumes a charger or power bank.

---

## Users and Stakeholders

| Audience | How they interact | What they need from this |
|---|---|---|
| Makers who own the CrowPanel 1.28" | Flash the firmware, use the knob daily | Clear install steps, working defaults, a way to change shortcuts |
| Tinkerers extending it | Edit modes, add shortcuts, port to other hosts | Small, readable modules and a clear place for each change |
| The author | Daily driver on a Mac | Reliable volume control, Bluetooth for other devices |

---

## Success Criteria

- [x] Volume, mute and play/pause work over USB on macOS
- [x] Pairs and works over Bluetooth with a Mac
- [x] Uploads start the new firmware without a manual replug (`upload.sh`)
- [ ] Recovers from Mac sleep/wake without a replug (implemented, untested)
- [ ] Mac/PC shortcut switch verified on Windows
- [ ] Verified on iOS / Android over Bluetooth
- [x] Touch trackpad (move, left/right click) and swipe gestures working
- [x] LED ring: mode glow, turn needle, action flashes

---

## Context and History

Built in October 2026 on Elecrow's own examples (pin map, LovyanGFX panel settings and the TinyUSB mouse lesson). Milestones, in commit order:

1. USB HID firmware with three modes and an LVGL round UI.
2. A fix for a false "USB not connected" status (the core starts USB before `setup()`), plus a reconnect watchdog after the knob was found stuck unmounted following a Mac sleep/wake.
3. Bluetooth LE HID with automatic switching, and runtime Mac/PC shortcut tables.
4. `upload.sh`, which clears the ESP32-S3 force-download flag so the knob boots after every upload.
5. Touch panel (Trackpad mode, tap and swipe gestures) and the LED ring.

---

## Relationship to Other Systems

```
  ╔══════════════════════╗    flash (USB serial)    ┌──────────────────┐
  ║ Arduino IDE /        ║ ───────────────────────► │                  │
  ║ arduino-cli+esptool  ║                          │   Input Knob     │
  ╚══════════════════════╝                          │   (ESP32-S3)     │
                                                    │                  │
  ╔══════════════════════╗   HID reports (USB)      │                  │
  ║ Computer             ║ ◄─────────────────────── │                  │
  ╚══════════════════════╝                          │                  │
  ╔══════════════════════╗   HID over GATT (BLE)    │                  │
  ║ Computer/phone/tablet║ ◄─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │                  │
  ╚══════════════════════╝                          └──────────────────┘
```

The host's OS is the only consumer: it maps consumer-control usages to its volume and media keys, the mouse report to scrolling, and keyboard reports to shortcuts. There is no protocol beyond standard HID.

---

## Trade-offs Made

- **Arduino + LVGL over ESP-IDF**: Chose the Arduino core because Elecrow's examples and libraries target it. Cost: less control over USB start-up (the device name stays `ESP32S3_DEV`) and an old bundled esptool that needed `upload.sh`.
- **Bluedroid BLE (built into the core) over NimBLE**: No extra library to install. Cost: about 0.7 MB more flash; still well under the 3 MB partition.
- **USB always wins over Bluetooth**: No duplicate pairing when plugged into a computer. Cost: you can't use the knob over Bluetooth while it's on a computer's USB port.
- **Fixed shortcut tables over runtime configuration**: A small firmware with no host app. Cost: changing shortcuts means a rebuild.
- **Volume as relative steps**: Works on every OS. Cost: the screen shows "Vol +", not the actual level.
