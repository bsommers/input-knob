# Input Knob

Volume / scroll / macro controller for the **Elecrow CrowPanel 1.28" ESP32-S3 Rotary Display** (240x240 round GC9A01, rotary encoder with push button). Shows up as a standard keyboard + mouse + media-key device over **USB or Bluetooth**, so no driver is needed.

## USB or Bluetooth

The knob picks automatically:

- **Plugged into a computer:** uses USB. Bluetooth stays quiet.
- **On a charger or power bank:** after 3 s without a USB host it advertises over Bluetooth as **Input Knob**. Pair it from the computer's, phone's or tablet's Bluetooth settings; it reconnects on its own afterwards. To move it to another device, disconnect or forget it on the current one.

The top of the screen shows the link: **USB**, **Bluetooth**, **Bluetooth pairing...** or **No connection**.

## Controls

| Mode (long press to cycle) | Turn | Click | Double click |
|---|---|---|---|
| **Volume** (blue) | volume up / down | mute | play / pause |
| **Scroll** (green) | scroll wheel | middle click | toggle vertical / horizontal |
| **Macro** (orange) | choose a shortcut | run it | switch Mac / PC shortcuts (remembered) |

Default macros: Copy, Paste, Undo, Redo, Screenshot, Mission Control / Task View, Lock, in Mac (Cmd) and PC (Ctrl / Win) versions. Edit them in `macros.h` using the key codes in `keys.h`.

## Setup (Arduino IDE)

1. **ESP32 core:** Boards Manager → *esp32 by Espressif* 3.x (tested to compile with 3.0.7; Elecrow's examples target 3.3.8).
2. **Libraries:** these exact versions, from Elecrow's repo [`example/Arduino/libraries`](https://github.com/Elecrow-RD/CrowPanel-1.28inch-HMI-ESP32-Rotary-Display-240-240-IPS-Round-Touch-Knob-Screen/tree/master/example/Arduino/libraries):
   - `lvgl` 9.1.0 and `LovyanGFX` 1.2.26 → copy into `~/Documents/Arduino/libraries/`
   - `lv_conf.h` → copy to `~/Documents/Arduino/libraries/lv_conf.h` (next to the `lvgl` folder)
3. **Tools menu:**
   | Setting | Value |
   |---|---|
   | Board | ESP32S3 Dev Module |
   | USB Mode | **USB-OTG (TinyUSB)** (required for HID; the build fails with a clear error otherwise) |
   | USB CDC On Boot | Enabled |
   | PSRAM | OPI PSRAM |
   | Flash Size | 16MB (128Mb) |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
   | Upload Mode | USB-OTG CDC (TinyUSB) |
4. Open `input-knob.ino` and upload. If the upload can't connect, hold **BOOT** while plugging in USB, upload, then unplug and replug. The board can also stay in download mode (dark screen) after a normal upload; unplug and replug to start it.

Command line equivalent:

```sh
arduino-cli compile --upload -p <port> \
  --fqbn "esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,UploadMode=cdc,PSRAM=opi,FlashSize=16M,PartitionScheme=huge_app" .
```

## Configuration

`config.h`: `ENCODER_REVERSED`, button timings, scroll speed and backlight brightness.

## Code layout

| File | Purpose |
|---|---|
| `input-knob.ino` | modes and what each input does |
| `board.h` | GPIO map (from Elecrow's examples and schematic) |
| `knob.*` | encoder (interrupt driven) and button click / double / long press |
| `hid.*` | input actions, routed to USB or Bluetooth |
| `usb_hid.*` | USB HID (TinyUSB) and the reconnect-after-host-sleep watchdog |
| `ble_hid.*` | Bluetooth LE HID (HID over GATT) |
| `keys.h` | raw HID key, modifier and media codes shared by both transports |
| `display.*` | LovyanGFX panel + LVGL display setup |
| `ui.*` | the round LVGL screen |
| `macros.h` | shortcut table |

Not used yet: the touch panel (CST816D), the 5-LED WS2812 ring on GPIO48.
