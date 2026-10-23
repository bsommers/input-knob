#pragma once
#include <stdint.h>

#include "hid.h"

// Round LVGL screen: connection status and mode name on top, current value in
// the middle, a hint at the bottom and a dial segment around the rim that
// follows the knob.

void uiBegin();
void uiSetMode(const char *name, const char *hint, uint32_t color);
void uiSetValue(const char *text);
void uiTurn(int detents);
void uiSetLink(hid::Link link);
