#include "usb_hid.h"

#include <Arduino.h>

#if !defined(ARDUINO_USB_MODE) || ARDUINO_USB_MODE != 0
#error "Select Tools > USB Mode > USB-OTG (TinyUSB). Hardware CDC mode cannot expose HID."
#endif

#include <USB.h>
#include <USBHIDConsumerControl.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>
#include <esp32-hal-tinyusb.h>

#if !ARDUINO_USB_CDC_ON_BOOT
// With "USB CDC On Boot" disabled the sketch owns the USB serial port, so the
// device name can be set before USB starts. It still honours the 1200-baud
// reboot-to-bootloader request that uploads rely on.
USBCDC USBSerial(0);
#endif

static USBHIDConsumerControl consumerDevice;
static USBHIDKeyboard keyboardDevice;
static USBHIDMouse mouseDevice;

static bool reached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

namespace usbhid {

// With "USB CDC On Boot" enabled the core has already started USB before
// setup(), so the name below can't be applied and the host sees ESP32S3_DEV.
void begin() {
#if !ARDUINO_USB_CDC_ON_BOOT
  USB.productName("Input Knob");
  USB.manufacturerName("maker-iot");
  USBSerial.begin();
#endif
  consumerDevice.begin();
  keyboardDevice.begin();
  mouseDevice.begin();
  USB.begin();
}

bool mounted() { return static_cast<bool>(USB); }

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

void consumer(uint16_t usage) {
  if (!consumerDevice.press(usage)) USBSerial.printf("[usb] consumer 0x%03X press failed\n", usage);
  if (!consumerDevice.release()) USBSerial.println("[usb] consumer release failed");
}

void move(int8_t dx, int8_t dy) { mouseDevice.move(dx, dy); }

void scroll(int8_t vertical, int8_t horizontal) { mouseDevice.move(0, 0, vertical, horizontal); }

void click(uint8_t buttons) { mouseDevice.click(buttons); }

void keys(uint8_t modifiers, uint8_t key) {
  KeyReport report = {modifiers, 0, {key, 0, 0, 0, 0, 0}};
  keyboardDevice.sendReport(&report);
  delay(10);
  report = {};
  keyboardDevice.sendReport(&report);
}

}  // namespace usbhid
