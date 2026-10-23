#pragma once
#include <stdint.h>

#include "keys.h"

// Keyboard shortcuts offered in Macro mode: one modifier set plus one key.
// Double-click in Macro mode switches between the Mac and PC tables; keep
// them the same length so the selection carries over.

struct Macro {
  const char *name;
  uint8_t modifiers;
  uint8_t key;
};

static const Macro MAC_MACROS[] = {
    {"Copy", mod::GUI, key::C},
    {"Paste", mod::GUI, key::V},
    {"Undo", mod::GUI, key::Z},
    {"Redo", mod::GUI | mod::SHIFT, key::Z},
    {"Screenshot", mod::GUI | mod::SHIFT, key::DIGIT_4},
    {"Mission Ctl", mod::CTRL, key::UP_ARROW},
    {"Lock", mod::CTRL | mod::GUI, key::Q},
};

static const Macro PC_MACROS[] = {
    {"Copy", mod::CTRL, key::C},
    {"Paste", mod::CTRL, key::V},
    {"Undo", mod::CTRL, key::Z},
    {"Redo", mod::CTRL, key::Y},
    {"Screenshot", mod::GUI | mod::SHIFT, key::S},
    {"Task View", mod::GUI, key::TAB},
    {"Lock", mod::GUI, key::L},
};

constexpr int MACRO_COUNT = sizeof(MAC_MACROS) / sizeof(MAC_MACROS[0]);
static_assert(MACRO_COUNT == sizeof(PC_MACROS) / sizeof(PC_MACROS[0]),
              "MAC_MACROS and PC_MACROS must have the same length");
