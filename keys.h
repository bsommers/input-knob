#pragma once
#include <stdint.h>

// Raw HID keyboard codes (USB HID Usage Tables, page 0x07). Both the USB and
// Bluetooth transports send these unchanged.

namespace mod {
constexpr uint8_t CTRL = 0x01;
constexpr uint8_t SHIFT = 0x02;
constexpr uint8_t ALT = 0x04;
constexpr uint8_t GUI = 0x08;  // Cmd on Mac, Windows key on PC
}  // namespace mod

namespace key {
constexpr uint8_t C = 0x06;
constexpr uint8_t L = 0x0F;
constexpr uint8_t Q = 0x14;
constexpr uint8_t S = 0x16;
constexpr uint8_t V = 0x19;
constexpr uint8_t Y = 0x1C;
constexpr uint8_t Z = 0x1D;
constexpr uint8_t DIGIT_4 = 0x21;
constexpr uint8_t TAB = 0x2B;
constexpr uint8_t UP_ARROW = 0x52;
}  // namespace key

// Consumer page (0x0C) usages.
namespace media {
constexpr uint16_t PLAY_PAUSE = 0x00CD;
constexpr uint16_t MUTE = 0x00E2;
constexpr uint16_t VOLUME_UP = 0x00E9;
constexpr uint16_t VOLUME_DOWN = 0x00EA;
}  // namespace media

constexpr uint8_t MOUSE_BUTTON_MIDDLE = 0x04;
