#pragma once
#include <stdint.h>

// Round LVGL screen: mode name on top, current value in the middle, a hint
// at the bottom and a dial segment around the rim that follows the knob.

void uiBegin();
void uiSetMode(const char *name, const char *hint, uint32_t color);
void uiSetValue(const char *text);
void uiTurn(int detents);
void uiSetConnected(bool connected);
