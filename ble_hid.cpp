#include "ble_hid.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <BLEServer.h>

static constexpr uint8_t REPORT_KEYBOARD = 1;
static constexpr uint8_t REPORT_MOUSE = 2;
static constexpr uint8_t REPORT_CONSUMER = 3;
static constexpr uint16_t APPEARANCE_KEYBOARD = 0x03C1;

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

static BLECharacteristic *keyboardInput;
static BLECharacteristic *mouseInput;
static BLECharacteristic *consumerInput;
static volatile bool isConnected = false;
static bool isAdvertising = false;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *) override {
    isConnected = true;
    isAdvertising = false;  // the stack stops advertising on connect
  }
  void onDisconnect(BLEServer *) override { isConnected = false; }
};

static void send(BLECharacteristic *input, const uint8_t *report, size_t length) {
  if (!isConnected) return;
  input->setValue(const_cast<uint8_t *>(report), length);
  input->notify();
}

static void sendMouse(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel, int8_t pan) {
  const uint8_t report[] = {buttons, static_cast<uint8_t>(dx), static_cast<uint8_t>(dy),
                            static_cast<uint8_t>(wheel), static_cast<uint8_t>(pan)};
  send(mouseInput, report, sizeof(report));
}

namespace blehid {

void begin() {
  BLEDevice::init("Input Knob");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEHIDDevice *hid = new BLEHIDDevice(server);
  keyboardInput = hid->inputReport(REPORT_KEYBOARD);
  mouseInput = hid->inputReport(REPORT_MOUSE);
  consumerInput = hid->inputReport(REPORT_CONSUMER);
  hid->manufacturer()->setValue("maker-iot");
  hid->pnp(0x02, 0x303A, 0x1001, 0x0100);
  hid->hidInfo(0x00, 0x01);
  hid->reportMap(const_cast<uint8_t *>(REPORT_MAP), sizeof(REPORT_MAP));
  hid->startServices();
  hid->setBatteryLevel(100);

  BLESecurity *security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_BOND);

  BLEAdvertising *advertisingConfig = server->getAdvertising();
  advertisingConfig->setAppearance(APPEARANCE_KEYBOARD);
  advertisingConfig->addServiceUUID(hid->hidService()->getUUID());
  advertisingConfig->setScanResponse(false);
}

bool connected() { return isConnected; }

bool advertising() { return isAdvertising; }

void setAdvertising(bool enabled) {
  if (enabled && !isConnected && !isAdvertising) {
    BLEDevice::startAdvertising();
    isAdvertising = true;
  } else if (!enabled && isAdvertising) {
    BLEDevice::getAdvertising()->stop();
    isAdvertising = false;
  }
}

void consumer(uint16_t usage) {
  const uint8_t pressed[] = {static_cast<uint8_t>(usage), static_cast<uint8_t>(usage >> 8)};
  const uint8_t released[] = {0, 0};
  send(consumerInput, pressed, sizeof(pressed));
  send(consumerInput, released, sizeof(released));
}

void move(int8_t dx, int8_t dy) { sendMouse(0, dx, dy, 0, 0); }

void scroll(int8_t vertical, int8_t horizontal) { sendMouse(0, 0, 0, vertical, horizontal); }

void click(uint8_t buttons) {
  sendMouse(buttons, 0, 0, 0, 0);
  sendMouse(0, 0, 0, 0, 0);
}

void keys(uint8_t modifiers, uint8_t key) {
  const uint8_t pressed[] = {modifiers, 0, key, 0, 0, 0, 0, 0};
  const uint8_t released[8] = {};
  send(keyboardInput, pressed, sizeof(pressed));
  delay(10);
  send(keyboardInput, released, sizeof(released));
}

}  // namespace blehid
