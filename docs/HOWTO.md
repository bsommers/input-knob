# How-To Guide

## Contents

- [Set up the build environment](#set-up-the-build-environment)
- [Flash the knob](#flash-the-knob)
- [Use it over Bluetooth](#use-it-over-bluetooth)
- [Use it with Windows or Linux](#use-it-with-windows-or-linux)
- [Add or change a shortcut](#add-or-change-a-shortcut)
- [Add a mode](#add-a-mode)
- [Tune the feel](#tune-the-feel)
- [Debug common problems](#debug-common-problems)
- [Restore Elecrow's factory firmware](#restore-elecrows-factory-firmware)

---

## Set up the build environment

**Prerequisites**: Arduino IDE 2.x (or `arduino-cli`), the **esp32 by Espressif** board package 3.x (3.0.7 is tested), Python 3 (used by `upload.sh`), and a USB-C *data* cable.

```bash
# Board package (skip if already installed via the IDE's Boards Manager)
arduino-cli core install esp32:esp32

# Libraries: use the exact versions Elecrow ships, and their lv_conf.h
git clone --depth 1 https://github.com/Elecrow-RD/CrowPanel-1.28inch-HMI-ESP32-Rotary-Display-240-240-IPS-Round-Touch-Knob-Screen.git /tmp/elecrow
mkdir -p ~/Documents/Arduino/libraries
cp -R /tmp/elecrow/example/Arduino/libraries/{lvgl,LovyanGFX,Adafruit_NeoPixel,lv_conf.h} ~/Documents/Arduino/libraries/

# Get the firmware and check it compiles (board settings come from sketch.yaml)
git clone https://github.com/bsommers/input-knob.git && cd input-knob
arduino-cli compile .
# Expected: "Sketch uses ~1343xxx bytes (42%) of program storage space."
```

`lv_conf.h` must sit directly in `libraries/`, next to the `lvgl` folder; that's where LVGL looks for it.

---

## Flash the knob

```bash
./upload.sh            # finds the knob by its Espressif USB vendor ID
./upload.sh /dev/cu.usbmodem2141301   # or name the port explicitly
```

The script compiles, uploads, then clears the chip's force-download flag and resets it, so the new firmware starts without a replug.

**From the Arduino IDE instead**: set the Tools menu as in the [README](../README.md#configuration) and click Upload. The screen will probably stay dark afterwards; unplug and replug the knob.

**First flash over the factory firmware, or the upload can't connect**: unplug, hold **BOOT**, plug in, release, then run `./upload.sh` again. The factory firmware doesn't respond to the USB "reboot to bootloader" request.

---

## Use it over Bluetooth

1. Power the knob from a USB charger or power bank (not a computer's data port).
2. After about 3 s the top label reads **Bluetooth pairing...** (orange).
3. Pair **Input Knob** from the host:
   - **macOS**: System Settings → Bluetooth → Input Knob → Connect. Close the Keyboard Setup Assistant if it appears.
   - **Windows**: Settings → Bluetooth & devices → Add device → Bluetooth → Input Knob.
   - **iOS / iPadOS / Android**: Settings → Bluetooth → Input Knob.
4. The label turns **Bluetooth** (blue). The knob reconnects on its own next time.

To move it to another host, disconnect or forget it on the current one; it starts advertising again.

---

## Use it with Windows or Linux

Volume and scroll work as-is. For shortcuts, switch to PC tables: long-press until **Macro (Mac)** shows, then **double-click**. The title becomes **Macro (PC)**, and the choice is saved across power cycles.

| Macro | Mac | PC |
|---|---|---|
| Copy / Paste / Undo | Cmd+C / V / Z | Ctrl+C / V / Z |
| Redo | Cmd+Shift+Z | Ctrl+Y |
| Screenshot | Cmd+Shift+4 | Win+Shift+S |
| Mission Ctl / Task View | Ctrl+Up | Win+Tab |
| Lock | Ctrl+Cmd+Q | Win+L |

---

## Add or change a shortcut

Shortcuts are one modifier set plus one key, using raw HID codes.

**1. Add any key code you need to `keys.h`** (values from the USB HID Usage Tables, keyboard page 0x07):

```cpp
namespace key {
// ...
constexpr uint8_t T = 0x17;
}  // namespace key
```

**2. Add the entry to *both* tables in `macros.h`, at the same position:**

```cpp
static const Macro MAC_MACROS[] = {
    // ...
    {"New Tab", mod::GUI, key::T},
};

static const Macro PC_MACROS[] = {
    // ...
    {"New Tab", mod::CTRL, key::T},
};
```

**3. Flash:** `./upload.sh`. If the tables differ in length, the build fails with `MAC_MACROS and PC_MACROS must have the same length`.

Expected result: in Macro mode, turning the knob reaches **New Tab**, and a click opens a browser tab.

---

## Add a mode

Example: a **Media** mode where turning skips tracks.

**1. Add the HID usages to `keys.h`:**

```cpp
namespace media {
// ...
constexpr uint16_t NEXT_TRACK = 0x00B5;
constexpr uint16_t PREVIOUS_TRACK = 0x00B6;
}  // namespace media
```

**2. Expose an action in `hid.h` / `hid.cpp`** (it routes automatically):

```cpp
// hid.h
void track(int steps);  // positive = next

// hid.cpp
void track(int steps) {
  const uint16_t usage = steps > 0 ? media::NEXT_TRACK : media::PREVIOUS_TRACK;
  for (int i = abs(steps); i > 0; --i) consumer(usage);
}
```

**3. Wire it into `input-knob.ino`:** add `Media` before `Count` in `enum class Mode`, then add `case Mode::Media:` branches to `showMode()`, `onTurn()` and `onButton()`:

```cpp
// showMode()
case Mode::Media:
  uiSetMode("Media", "click play | 2x play", 0x9B59B6);
  uiSetValue("Turn to skip");
  break;

// onTurn()
case Mode::Media:
  hid::track(detents);
  uiSetValue(detents > 0 ? "Next" : "Previous");
  break;

// onButton()
case Mode::Media:
  hid::playPause();
  uiSetValue("Play/Pause");
  break;
```

Expected result: long press cycles Volume → Scroll → Macro → **Media**.

---

## Tune the feel

Edit `config.h` and reflash:

```cpp
constexpr bool ENCODER_REVERSED = true;        // clockwise turns volume down? flip it
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 250;  // snappier single clicks
constexpr uint32_t BUTTON_LONG_PRESS_MS = 800;    // fewer accidental mode changes
constexpr int8_t SCROLL_LINES_PER_DETENT = 3;     // faster scrolling
constexpr uint8_t BACKLIGHT_PERCENT = 30;         // dimmer screen
```

A single click is reported only after the double-click window closes, so lowering `BUTTON_DOUBLE_CLICK_MS` makes mute feel quicker.

---

## Debug common problems

### Build fails: `Select Tools > USB Mode > USB-OTG (TinyUSB)`

**Cause**: Built with *Hardware CDC and JTAG* USB mode, which can't expose HID (`#error` in `usb_hid.cpp`).
**Fix**: `arduino-cli compile .` uses `sketch.yaml`, which sets it. In the IDE, set **Tools → USB Mode → USB-OTG (TinyUSB)**.

### Build fails: `lv_conf.h` not found, or LVGL errors

**Cause**: `lv_conf.h` isn't next to the `lvgl` folder, or a different LVGL version is installed.
**Fix**:
```bash
ls ~/Documents/Arduino/libraries/        # expect: LovyanGFX  lv_conf.h  lvgl
grep '^version' ~/Documents/Arduino/libraries/lvgl/library.properties   # expect 9.1.0
```

### Screen stays dark after an upload

**Cause**: The board rebooted into ROM download mode (it shows up as "USB JTAG/serial debug unit") because the force-download flag survived the reset.
**Fix**: Use `./upload.sh`, which clears it. Otherwise unplug and replug.

### Upload: `Failed to connect to ESP32-S3: No serial data received`

**Cause**: The running firmware isn't answering the reboot-to-bootloader request (factory firmware, or USB stuck).
**Fix**: Hold **BOOT** while plugging in, then upload. Afterwards `upload.sh` starts the firmware, or replug.

### Screen says **No connection** while plugged into a computer

**Cause**: USB isn't enumerated: a charge-only cable, a hub problem, or the host slept and the connection dropped.
**Fix**: Try a known data cable directly into the computer. After a sleep/wake, the watchdog re-attaches within about 3 s; if it doesn't, replug and [open an issue](https://github.com/bsommers/input-knob/issues). Check the log for failed sends:
```bash
arduino-cli monitor -p /dev/cu.usbmodem<id> -c baudrate=115200
# "[usb] consumer 0x0E9 press failed" means the host isn't accepting reports
```

### Screen shows actions ("Vol +", "Mute") but the host ignores them

**Cause**: The action was dropped because no transport is live (check the top label), or the host's output device can't change volume (some HDMI/DisplayPort audio).
**Fix**: Confirm the label is **USB** or **Bluetooth**. On macOS, check the volume can be changed with the keyboard's own volume keys.

### Bluetooth never shows **Bluetooth pairing...**

**Cause**: The knob is on a computer's USB port, so USB took over and Bluetooth advertising is off.
**Fix**: Power it from a charger or power bank.

### Trackpad cursor moves the wrong way, or too fast or slow

**Cause**: Touch axes or speed don't suit your setup.
**Fix**: Adjust `TRACKPAD_GAIN` and `TRACKPAD_ACCELERATION` in `config.h`. For reversed axes, negate `touch.dx` / `touch.dy` in `onTrackpadTouch()` in `input-knob.ino`.

### Right click (touch and hold) acts as a drag

**Cause**: The finger moved more than `TOUCH_TAP_SLOP_PX` (12 px) during the hold.
**Fix**: Raise `TOUCH_TAP_SLOP_PX` to about 20 in `config.h`.

### Touch does nothing

**Cause**: The CST816D didn't answer at start-up. The serial log shows `[touch] CST816D not responding`.
**Fix**: Power-cycle the knob. If it persists, check that `board.h` still has touch SDA 6, SCL 7, RST 13, INT 5.

### LED needle walks against the knob

**Fix**: Set `LED_RING_REVERSED = true` in `config.h`.

### Clockwise turns the volume down

**Fix**: Set `ENCODER_REVERSED = true` in `config.h` and reflash.

---

## Restore Elecrow's factory firmware

Elecrow publishes it in their repo under `factory_firmware/1.28 firmware/`. Hold **BOOT** while plugging in, then:

```bash
ESPTOOL=$(ls -d ~/Library/Arduino15/packages/esp32/tools/esptool_py/*/esptool | tail -1)
"$ESPTOOL" --chip esp32s3 --port /dev/cu.usbmodem<id> write_flash 0x0 \
  "/tmp/elecrow/factory_firmware/1.28 firmware/RotaryScreen_1_28.ino.merged.bin"
```

Replug when it finishes.
