#pragma once
#include <stdint.h>

// Rotary encoder (interrupt driven) and its push button (polled, debounced).

enum class ButtonEvent : uint8_t { None, Click, DoubleClick, LongPress };

void knobBegin();

// Detents turned since the last call: positive = clockwise.
int knobTakeDetents();

// Call every loop. A single click is reported after the double-click window
// closes; a long press fires while the button is still held.
ButtonEvent buttonPoll();
