#pragma once
#include <stdint.h>

// CST816D capacitive touch panel: one finger, polled over I2C. Gestures are
// recognised here from raw coordinates rather than the chip's own gesture
// codes, so taps, long presses and swipes use the same timings as the knob.

enum class TouchGesture : uint8_t { None, Tap, LongPress, SwipeLeft, SwipeRight, SwipeUp, SwipeDown };

struct TouchUpdate {
  TouchGesture gesture;
  int16_t dx;  // finger movement since the last update, in screen pixels
  int16_t dy;
};

bool touchBegin();  // false if the controller doesn't answer
TouchUpdate touchPoll();

// Drop the current touch and ignore new ones for TOUCH_IGNORE_AFTER_TURN_MS.
// Called on every knob detent: the panel sits inside the knob, so a finger on
// the glass while turning shouldn't become a tap or swipe.
void touchSuppress();
