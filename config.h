#pragma once
#include <stdint.h>

// User-tunable behaviour. Macros live in macros.h.

// Set to 0 when the host is Windows or Linux: shortcuts then use Ctrl instead of Cmd.
#define HOST_IS_MAC 1

// Flip if clockwise turns the volume down.
constexpr bool ENCODER_REVERSED = false;

constexpr uint32_t BUTTON_DEBOUNCE_MS = 25;
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 300;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 600;

// Wheel units sent per encoder detent in scroll mode.
constexpr int8_t SCROLL_LINES_PER_DETENT = 1;

constexpr uint8_t BACKLIGHT_PERCENT = 60;
