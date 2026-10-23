#pragma once
#include <stdint.h>

// User-tunable behaviour. Macros live in macros.h.

// Flip if clockwise turns the volume down.
constexpr bool ENCODER_REVERSED = false;

constexpr uint32_t BUTTON_DEBOUNCE_MS = 25;
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 300;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 600;

// Wheel units sent per encoder detent in scroll mode.
constexpr int8_t SCROLL_LINES_PER_DETENT = 1;

constexpr uint8_t BACKLIGHT_PERCENT = 60;

// Touch: a press that moves less than TOUCH_TAP_SLOP_PX is a tap (or a long
// press after BUTTON_LONG_PRESS_MS); a swipe travels TOUCH_SWIPE_MIN_PX.
constexpr int16_t TOUCH_TAP_SLOP_PX = 12;
constexpr int16_t TOUCH_SWIPE_MIN_PX = 50;

// Trackpad cursor speed: pixels moved = finger delta x (gain + acceleration x speed).
constexpr float TRACKPAD_GAIN = 2.0f;
constexpr float TRACKPAD_ACCELERATION = 0.15f;

// Wheel units sent by a vertical swipe in Scroll mode.
constexpr int8_t SWIPE_SCROLL_LINES = 10;

// LED ring brightness cap (0-255). Flip LED_RING_REVERSED if the needle runs
// against the knob.
constexpr uint8_t LED_BRIGHTNESS = 60;
constexpr bool LED_RING_REVERSED = false;
