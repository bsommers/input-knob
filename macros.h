#pragma once
#include <stdint.h>

#include "config.h"

// Keyboard shortcuts offered in Macro mode. Keys are pressed in order and then
// released together. Use KEY_* codes (USBHIDKeyboard.h) or plain characters.

#define KEY_CODE_LEFT_CTRL 0x80
#define KEY_CODE_LEFT_SHIFT 0x81
#define KEY_CODE_LEFT_ALT 0x82
#define KEY_CODE_LEFT_GUI 0x83
#define KEY_CODE_UP_ARROW 0xDA

#if HOST_IS_MAC
#define CMD KEY_CODE_LEFT_GUI
#else
#define CMD KEY_CODE_LEFT_CTRL
#endif

struct Macro {
  const char *name;
  uint8_t keys[4];
};

static const Macro MACROS[] = {
    {"Copy", {CMD, 'c'}},
    {"Paste", {CMD, 'v'}},
    {"Undo", {CMD, 'z'}},
    {"Redo", {CMD, KEY_CODE_LEFT_SHIFT, 'z'}},
#if HOST_IS_MAC
    {"Screenshot", {CMD, KEY_CODE_LEFT_SHIFT, '4'}},
    {"Mission Ctl", {KEY_CODE_LEFT_CTRL, KEY_CODE_UP_ARROW}},
    {"Lock", {KEY_CODE_LEFT_CTRL, CMD, 'q'}},
#else
    {"Screenshot", {KEY_CODE_LEFT_GUI, KEY_CODE_LEFT_SHIFT, 's'}},
    {"Task View", {KEY_CODE_LEFT_GUI, '\t'}},
    {"Lock", {KEY_CODE_LEFT_GUI, 'l'}},
#endif
};

constexpr int MACRO_COUNT = sizeof(MACROS) / sizeof(MACROS[0]);
