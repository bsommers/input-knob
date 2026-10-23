#pragma once
#include <stdint.h>

// Bluetooth LE HID transport (HID over GATT), advertised as "Input Knob".

namespace blehid {
void begin();
bool connected();
bool advertising();
void setAdvertising(bool enabled);  // no-op while a host is connected

void consumer(uint16_t usage);
void move(int8_t dx, int8_t dy);
void scroll(int8_t vertical, int8_t horizontal);
void click(uint8_t buttons);
void keys(uint8_t modifiers, uint8_t key);
}  // namespace blehid
