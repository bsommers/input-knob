#pragma once
#include <stdint.h>

// USB HID transport (TinyUSB composite: keyboard + mouse + consumer control).

namespace usbhid {
void begin();
void service();  // call every loop; re-enumerates after a lost host connection
bool mounted();

void consumer(uint16_t usage);
void move(int8_t dx, int8_t dy);
void scroll(int8_t vertical, int8_t horizontal);
void click(uint8_t buttons);
void keys(uint8_t modifiers, uint8_t key);
}  // namespace usbhid
