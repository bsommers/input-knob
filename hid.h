#pragma once
#include <stdint.h>

#include "macros.h"

// Host-facing input actions, routed to USB when a computer has enumerated the
// knob over the cable, otherwise to Bluetooth.

namespace hid {
enum class Link : uint8_t { None, Usb, Bluetooth, Pairing };

void begin();
void service();  // call every loop
Link link();

void volume(int steps);  // positive = up
void mute();
void playPause();
void scroll(int8_t vertical, int8_t horizontal);
void middleClick();
void runMacro(const Macro &macro);
}  // namespace hid
