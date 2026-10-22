#include "hid.h"

#include <Arduino.h>

#if !defined(ARDUINO_USB_MODE) || ARDUINO_USB_MODE != 0
#error "Select Tools > USB Mode > USB-OTG (TinyUSB). Hardware CDC mode cannot expose HID."
#endif

#include <USB.h>
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
