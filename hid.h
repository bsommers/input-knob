#pragma once
#include <stdint.h>

#include "macros.h"

// Host-facing input actions. This is the only module that knows about the
// transport, so a Bluetooth implementation can replace hid.cpp later.

namespace hid {
void begin();
bool connected();

void volume(int steps);  // positive = up
void mute();
void playPause();
void scroll(int8_t vertical, int8_t horizontal);
void middleClick();
void runMacro(const Macro &macro);
}  // namespace hid
