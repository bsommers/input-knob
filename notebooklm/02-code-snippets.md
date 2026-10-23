# Input Knob: annotated code snippets

Excerpts from the Input Knob firmware (github.com/bsommers/input-knob), copied verbatim with line numbers. Each has a matching image in `images/code-*.png`.

## 1. The hardware pin map

`board.h`, lines 7–32 · image: `images/code-01-pin-map.png`

Every GPIO the firmware touches, taken from Elecrow's examples and schematic. GPIO1 and GPIO2 must be driven high or the LCD has no power. The encoder is on 45/42 and its push switch on 41 (active low).

```cpp
namespace pins {
constexpr int POWER_LCD = 1;   // LCD 3V3 rail enable, active high
constexpr int POWER_EXT = 2;   // UART/I2C header 5V rail enable, active high
constexpr int POWER_LED = 40;  // red power LED, active low
constexpr int BACKLIGHT = 46;  // PWM

constexpr int LCD_SCLK = 10;
constexpr int LCD_MOSI = 11;
constexpr int LCD_DC = 3;
constexpr int LCD_CS = 9;
constexpr int LCD_RST = 14;

constexpr int TOUCH_SDA = 6;
constexpr int TOUCH_SCL = 7;
constexpr int TOUCH_RST = 13;
constexpr int TOUCH_INT = 5;

constexpr int ENC_A = 45;
constexpr int ENC_B = 42;
constexpr int ENC_SW = 41;  // active low, needs pull-up

constexpr int RGB_LEDS = 48;  // 5x WS2812B ring
}  // namespace pins

constexpr int SCREEN_WIDTH = 240;
constexpr int SCREEN_HEIGHT = 240;
```

## 2. Encoder: quadrature decoding in an interrupt

`knob.cpp`, lines 9–48 · image: `images/code-02-encoder-isr.png`

Each change on either encoder pin runs a tiny interrupt handler. It combines the previous and current A/B levels into a 4-bit index and looks up +1, -1 or 0 in a 16-entry table, which rejects contact bounce. Four quarter-steps make one detent, the click you feel. Because it's interrupt-driven, fast turns aren't lost while the screen redraws.

```cpp
static const int8_t TRANSITIONS[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0,
};
static constexpr int QUARTER_STEPS_PER_DETENT = 4;

static portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;
static volatile int32_t quarterSteps = 0;
static volatile uint8_t encoderState = 0;

static uint8_t readEncoder() {
  return (digitalRead(pins::ENC_A) << 1) | digitalRead(pins::ENC_B);
}

static void IRAM_ATTR onEncoderEdge() {
  const uint8_t current = readEncoder();
  portENTER_CRITICAL_ISR(&encoderMux);
  quarterSteps += TRANSITIONS[(encoderState << 2) | current];
  encoderState = current;
  portEXIT_CRITICAL_ISR(&encoderMux);
}

void knobBegin() {
  pinMode(pins::ENC_A, INPUT);
  pinMode(pins::ENC_B, INPUT);
  pinMode(pins::ENC_SW, INPUT_PULLUP);
  encoderState = readEncoder();
  attachInterrupt(digitalPinToInterrupt(pins::ENC_A), onEncoderEdge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pins::ENC_B), onEncoderEdge, CHANGE);
}

int knobTakeDetents() {
  portENTER_CRITICAL(&encoderMux);
  const int detents = quarterSteps / QUARTER_STEPS_PER_DETENT;
  quarterSteps -= detents * QUARTER_STEPS_PER_DETENT;
  portEXIT_CRITICAL(&encoderMux);
  return ENCODER_REVERSED ? -detents : detents;
}
```

## 3. Button: click, double-click and long press

`knob.cpp`, lines 50–92 · image: `images/code-03-button-gestures.png`

The button is polled with a 25 ms debounce. A long press (600 ms) fires while you're still holding, so mode changes feel immediate. A single click is only reported after the 300 ms double-click window closes; that small delay is the price of having double-click at all.

```cpp
ButtonEvent buttonPoll() {
  static bool lastRaw = false;
  static bool pressed = false;
  static uint32_t lastChangeMs = 0;
  static uint32_t pressedAtMs = 0;
  static uint32_t releasedAtMs = 0;
  static bool longFired = false;
  static uint8_t pendingClicks = 0;

  const uint32_t now = millis();
  const bool raw = digitalRead(pins::ENC_SW) == LOW;
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChangeMs = now;
  }

  if (raw != pressed && now - lastChangeMs >= BUTTON_DEBOUNCE_MS) {
    pressed = raw;
    if (pressed) {
      pressedAtMs = now;
      longFired = false;
    } else if (!longFired) {
      releasedAtMs = now;
      if (++pendingClicks >= 2) {
        pendingClicks = 0;
        return ButtonEvent::DoubleClick;
      }
    }
  }

  if (pressed && !longFired && now - pressedAtMs >= BUTTON_LONG_PRESS_MS) {
    longFired = true;
    pendingClicks = 0;
    return ButtonEvent::LongPress;
  }

  if (!pressed && pendingClicks == 1 && now - releasedAtMs >= BUTTON_DOUBLE_CLICK_MS) {
    pendingClicks = 0;
    return ButtonEvent::Click;
  }

  return ButtonEvent::None;
}
```

## 4. What a turn does in each mode

`input-knob.ino`, lines 50–73 · image: `images/code-04-mode-actions.png`

The whole personality of the knob: one switch on the current mode. Volume sends media keys, Scroll sends mouse-wheel steps (clockwise scrolls down), Macro moves through the shortcut list. The dial ring and labels update on every turn.

```cpp
static void onTurn(int detents) {
  uiTurn(detents);
  switch (mode) {
    case Mode::Volume:
      hid::volume(detents);
      uiSetValue(detents > 0 ? "Vol +" : "Vol -");
      break;
    case Mode::Scroll: {
      const int8_t amount = static_cast<int8_t>(constrain(detents * SCROLL_LINES_PER_DETENT, -127, 127));
      if (horizontalScroll) {
        hid::scroll(0, amount);
      } else {
        hid::scroll(-amount, 0);  // clockwise scrolls down
      }
      break;
    }
    case Mode::Macro:
      selectedMacro = ((selectedMacro + detents) % MACRO_COUNT + MACRO_COUNT) % MACRO_COUNT;
      uiSetValue(currentMacro().name);
      break;
    default:
      break;
  }
}
```

## 5. Routing every action to USB or Bluetooth

`hid.cpp`, lines 13–50 · image: `images/code-05-router.png`

Modes never know how the knob is connected. Every action goes through this router: USB if a computer has enumerated the knob, otherwise Bluetooth if a device is paired, otherwise nothing. service() turns Bluetooth advertising on after 3 seconds without USB, and off as soon as USB is back.

```cpp
static bool useUsb() { return usbhid::mounted(); }
static bool useBluetooth() { return !useUsb() && blehid::connected(); }

static void consumer(uint16_t usage) {
  if (useUsb()) {
    usbhid::consumer(usage);
  } else if (useBluetooth()) {
    blehid::consumer(usage);
  }
}

namespace hid {

void begin() {
  usbhid::begin();
  blehid::begin();
}

void service() {
  static uint32_t usbLostSinceMs = 0;

  usbhid::service();
  if (usbhid::mounted()) {
    usbLostSinceMs = 0;
    blehid::setAdvertising(false);
    return;
  }
  const uint32_t now = millis();
  if (usbLostSinceMs == 0) usbLostSinceMs = now;
  blehid::setAdvertising(now - usbLostSinceMs >= BLUETOOTH_AFTER_USB_LOST_MS);
}

Link link() {
  if (useUsb()) return Link::Usb;
  if (blehid::connected()) return Link::Bluetooth;
  if (blehid::advertising()) return Link::Pairing;
  return Link::None;
}
```

## 6. Recovering USB after the computer sleeps

`usb_hid.cpp`, lines 36–65 · image: `images/code-06-usb-watchdog.png`

Found while debugging: after a Mac slept and woke, the knob was left unmounted while macOS still listed it, so nothing got through. This watchdog notices, drops off the USB bus for 200 ms and reconnects, forcing a fresh enumeration. It's non-blocking and retries every 30 s so it never stalls Bluetooth use on a power bank. (Implemented; not yet verified against a real sleep cycle.)

```cpp
// After the host sleeps and wakes, the device can end up unmounted while the
// host still lists it. Dropping off the bus briefly forces a fresh
// enumeration. Without a USB host (charger, power bank) the attempts are
// harmless, so retry slowly instead of giving up.
void service() {
  static constexpr uint32_t FIRST_ATTEMPT_MS = 3000;
  static constexpr uint32_t RETRY_EVERY_MS = 30000;
  static constexpr uint32_t OFF_BUS_MS = 200;
  static uint32_t nextAttemptMs = 0;
  static uint32_t reconnectAtMs = 0;
  static bool offBus = false;

  const uint32_t now = millis();
  if (offBus && reached(now, reconnectAtMs)) {
    tud_connect();
    offBus = false;
  }
  if (mounted()) {
    nextAttemptMs = 0;
    return;
  }
  if (nextAttemptMs == 0) {
    nextAttemptMs = now + FIRST_ATTEMPT_MS;
  } else if (!offBus && reached(now, nextAttemptMs)) {
    tud_disconnect();
    offBus = true;
    reconnectAtMs = now + OFF_BUS_MS;
    nextAttemptMs = now + RETRY_EVERY_MS;
  }
}
```

## 7. Bluetooth HID report map: keyboard, mouse, media keys

`ble_hid.cpp`, lines 13–41 · image: `images/code-07-ble-report-map.png`

The Bluetooth HID report map: one descriptor that declares three devices (a keyboard, a mouse with vertical and horizontal wheels, and a consumer-control device for media keys). Hosts read this once at pairing and then accept raw reports, the same bytes the USB side sends.

```cpp

// Keyboard: [modifiers, reserved, 6 keys]. Mouse: [buttons, x, y, wheel, pan].
// Consumer: [16-bit usage, little endian].
static const uint8_t REPORT_MAP[] = {
    // Keyboard
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, REPORT_KEYBOARD,
    0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02,  // modifier bits
    0x95, 0x01, 0x75, 0x08, 0x81, 0x01,  // reserved byte
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65,
    0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00,  // key array
    0xC0,
    // Mouse
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, REPORT_MOUSE,
    0x09, 0x01, 0xA1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01,
    0x95, 0x03, 0x75, 0x01, 0x81, 0x02,  // 3 buttons
    0x95, 0x01, 0x75, 0x05, 0x81, 0x03,  // padding
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 0x15, 0x81, 0x25, 0x7F,
    0x75, 0x08, 0x95, 0x03, 0x81, 0x06,  // x, y, wheel
    0x05, 0x0C, 0x0A, 0x38, 0x02, 0x15, 0x81, 0x25, 0x7F,
    0x75, 0x08, 0x95, 0x01, 0x81, 0x06,  // horizontal pan
    0xC0, 0xC0,
    // Consumer control
    0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, REPORT_CONSUMER,
    0x15, 0x00, 0x26, 0xFF, 0x03, 0x19, 0x00, 0x2A, 0xFF, 0x03,
    0x75, 0x10, 0x95, 0x01, 0x81, 0x00,
    0xC0,
};
```

## 8. Mac and PC shortcut tables

`macros.h`, lines 9–38 · image: `images/code-08-macros.png`

Shortcuts are plain data: a name, modifier keys and one key code. There's a Mac table and a PC table of the same length (a static_assert enforces it), and a double-click in Macro mode switches between them. The choice is saved in flash.

```cpp

struct Macro {
  const char *name;
  uint8_t modifiers;
  uint8_t key;
};

static const Macro MAC_MACROS[] = {
    {"Copy", mod::GUI, key::C},
    {"Paste", mod::GUI, key::V},
    {"Undo", mod::GUI, key::Z},
    {"Redo", mod::GUI | mod::SHIFT, key::Z},
    {"Screenshot", mod::GUI | mod::SHIFT, key::DIGIT_4},
    {"Mission Ctl", mod::CTRL, key::UP_ARROW},
    {"Lock", mod::CTRL | mod::GUI, key::Q},
};

static const Macro PC_MACROS[] = {
    {"Copy", mod::CTRL, key::C},
    {"Paste", mod::CTRL, key::V},
    {"Undo", mod::CTRL, key::Z},
    {"Redo", mod::CTRL, key::Y},
    {"Screenshot", mod::GUI | mod::SHIFT, key::S},
    {"Task View", mod::GUI, key::TAB},
    {"Lock", mod::GUI, key::L},
};

constexpr int MACRO_COUNT = sizeof(MAC_MACROS) / sizeof(MAC_MACROS[0]);
static_assert(MACRO_COUNT == sizeof(PC_MACROS) / sizeof(PC_MACROS[0]),
              "MAC_MACROS and PC_MACROS must have the same length");
```

## 9. Driving the round GC9A01 display

`display.cpp`, lines 10–42 · image: `images/code-09-display-config.png`

LovyanGFX drives the 240x240 round GC9A01 over SPI at 80 MHz with colour inversion on, matching Elecrow's working example. LVGL 9 draws into two 40-line buffers in PSRAM and this driver pushes them to the panel.

```cpp
// Panel settings match Elecrow's Lesson03 example.
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 panel;
  lgfx::Bus_SPI bus;

 public:
  LGFX() {
    auto busConfig = bus.config();
    busConfig.spi_host = SPI2_HOST;
    busConfig.freq_write = 80000000;
    busConfig.freq_read = 20000000;
    busConfig.spi_3wire = true;
    busConfig.pin_sclk = pins::LCD_SCLK;
    busConfig.pin_mosi = pins::LCD_MOSI;
    busConfig.pin_miso = -1;
    busConfig.pin_dc = pins::LCD_DC;
    bus.config(busConfig);
    panel.setBus(&bus);

    auto panelConfig = panel.config();
    panelConfig.pin_cs = pins::LCD_CS;
    panelConfig.pin_rst = pins::LCD_RST;
    panelConfig.memory_width = SCREEN_WIDTH;
    panelConfig.memory_height = SCREEN_HEIGHT;
    panelConfig.panel_width = SCREEN_WIDTH;
    panelConfig.panel_height = SCREEN_HEIGHT;
    panelConfig.invert = true;
    panelConfig.readable = false;
    panelConfig.bus_shared = false;
    panel.config(panelConfig);
    setPanel(&panel);
  }
};
```

## 10. upload.sh: no more dark screen after flashing

`upload.sh`, lines 1–49 · image: `images/code-10-upload-fix.png`

Uploading over the knob's own USB port reboots the ESP32-S3 with a 'force download boot' flag set. The bundled esptool 4.6 resets without clearing it, so the knob came back with a dark screen until it was unplugged. This script uploads, then writes 0 to that one register bit and resets, so the new firmware starts immediately.

```bash
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
```
