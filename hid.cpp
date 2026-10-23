#include "hid.h"

#include <Arduino.h>

#include "ble_hid.h"
#include "keys.h"
#include "usb_hid.h"

// How long USB must be unmounted before Bluetooth starts advertising. Covers
// boot-time enumeration and the USB watchdog's reconnect.
static constexpr uint32_t BLUETOOTH_AFTER_USB_LOST_MS = 3000;

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

void volume(int steps) {
  const uint16_t usage = steps > 0 ? media::VOLUME_UP : media::VOLUME_DOWN;
  for (int i = abs(steps); i > 0; --i) consumer(usage);
}

void mute() { consumer(media::MUTE); }

void playPause() { consumer(media::PLAY_PAUSE); }

void scroll(int8_t vertical, int8_t horizontal) {
  if (useUsb()) {
    usbhid::scroll(vertical, horizontal);
  } else if (useBluetooth()) {
    blehid::scroll(vertical, horizontal);
  }
}

void moveCursor(int dx, int dy) {
  const int8_t x = constrain(dx, -127, 127);
  const int8_t y = constrain(dy, -127, 127);
  if (useUsb()) {
    usbhid::move(x, y);
  } else if (useBluetooth()) {
    blehid::move(x, y);
  }
}

void click(uint8_t buttons) {
  if (useUsb()) {
    usbhid::click(buttons);
  } else if (useBluetooth()) {
    blehid::click(buttons);
  }
}

void runMacro(const Macro &macro) {
  if (useUsb()) {
    usbhid::keys(macro.modifiers, macro.key);
  } else if (useBluetooth()) {
    blehid::keys(macro.modifiers, macro.key);
  }
}

}  // namespace hid
