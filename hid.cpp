#include "hid.h"

#include <Arduino.h>

#if !defined(ARDUINO_USB_MODE) || ARDUINO_USB_MODE != 0
#error "Select Tools > USB Mode > USB-OTG (TinyUSB). Hardware CDC mode cannot expose HID."
#endif

#include <USB.h>
#include <esp32-hal-tinyusb.h>
#include <USBHIDConsumerControl.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>

static USBHIDConsumerControl consumer;
static USBHIDKeyboard keyboard;
static USBHIDMouse mouse;

static void tapConsumer(uint16_t usage) {
  if (!consumer.press(usage)) Serial.printf("[hid] consumer 0x%03X press failed\n", usage);
  if (!consumer.release()) Serial.println("[hid] consumer release failed");
}

namespace hid {

// With "USB CDC On Boot" enabled the core has already called USB.begin()
// before setup(), so this only prepares the HID report queues.
void begin() {
  consumer.begin();
  keyboard.begin();
  mouse.begin();
  USB.begin();
}

bool connected() { return static_cast<bool>(USB); }

// After the host sleeps and wakes, the device can end up unmounted while the
// host still lists it. Dropping off the bus briefly forces a fresh enumeration.
void service() {
  static constexpr uint32_t RECONNECT_AFTER_MS = 3000;
  static uint32_t lostSinceMs = 0;

  if (connected()) {
    lostSinceMs = 0;
    return;
  }
  const uint32_t now = millis();
  if (lostSinceMs == 0) {
    lostSinceMs = now;
  } else if (now - lostSinceMs >= RECONNECT_AFTER_MS) {
    tud_disconnect();
    delay(200);
    tud_connect();
    lostSinceMs = now;
  }
}

void volume(int steps) {
  const uint16_t usage =
      steps > 0 ? CONSUMER_CONTROL_VOLUME_INCREMENT : CONSUMER_CONTROL_VOLUME_DECREMENT;
  for (int i = abs(steps); i > 0; --i) tapConsumer(usage);
}

void mute() { tapConsumer(CONSUMER_CONTROL_MUTE); }

void playPause() { tapConsumer(CONSUMER_CONTROL_PLAY_PAUSE); }

void scroll(int8_t vertical, int8_t horizontal) { mouse.move(0, 0, vertical, horizontal); }

void middleClick() { mouse.click(MOUSE_MIDDLE); }

void runMacro(const Macro &macro) {
  for (uint8_t key : macro.keys) {
    if (key == 0) break;
    keyboard.press(key);
  }
  delay(10);
  keyboard.releaseAll();
}

}  // namespace hid
