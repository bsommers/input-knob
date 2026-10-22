#include "knob.h"

#include <Arduino.h>

#include "board.h"
#include "config.h"

// Quadrature state table, indexed by (previous AB << 2) | current AB.
static const int8_t TRANSITIONS[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0,
};
static constexpr int QUARTER_STEPS_PER_DETENT = 4;

static portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;
static volatile int32_t quarterSteps = 0;
static volatile uint8_t encoderState = 0;

static uint8_t readEncoder() {
  return (digitalRead(pins::ENC_A) << 1) | digitalRead(pins::ENC_B);
}

static void IRAM_ATTR onEncoderEdge() {
  const uint8_t current = readEncoder();
  portENTER_CRITICAL_ISR(&encoderMux);
  quarterSteps += TRANSITIONS[(encoderState << 2) | current];
  encoderState = current;
  portEXIT_CRITICAL_ISR(&encoderMux);
}

void knobBegin() {
  pinMode(pins::ENC_A, INPUT);
  pinMode(pins::ENC_B, INPUT);
  pinMode(pins::ENC_SW, INPUT_PULLUP);
  encoderState = readEncoder();
  attachInterrupt(digitalPinToInterrupt(pins::ENC_A), onEncoderEdge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pins::ENC_B), onEncoderEdge, CHANGE);
}

int knobTakeDetents() {
  portENTER_CRITICAL(&encoderMux);
  const int detents = quarterSteps / QUARTER_STEPS_PER_DETENT;
  quarterSteps -= detents * QUARTER_STEPS_PER_DETENT;
  portEXIT_CRITICAL(&encoderMux);
  return ENCODER_REVERSED ? -detents : detents;
}

ButtonEvent buttonPoll() {
  static bool lastRaw = false;
  static bool pressed = false;
  static uint32_t lastChangeMs = 0;
  static uint32_t pressedAtMs = 0;
  static uint32_t releasedAtMs = 0;
  static bool longFired = false;
  static uint8_t pendingClicks = 0;

  const uint32_t now = millis();
  const bool raw = digitalRead(pins::ENC_SW) == LOW;
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChangeMs = now;
  }

  if (raw != pressed && now - lastChangeMs >= BUTTON_DEBOUNCE_MS) {
    pressed = raw;
    if (pressed) {
      pressedAtMs = now;
      longFired = false;
    } else if (!longFired) {
      releasedAtMs = now;
      if (++pendingClicks >= 2) {
        pendingClicks = 0;
        return ButtonEvent::DoubleClick;
      }
    }
  }

  if (pressed && !longFired && now - pressedAtMs >= BUTTON_LONG_PRESS_MS) {
    longFired = true;
    pendingClicks = 0;
    return ButtonEvent::LongPress;
  }

  if (!pressed && pendingClicks == 1 && now - releasedAtMs >= BUTTON_DOUBLE_CLICK_MS) {
    pendingClicks = 0;
    return ButtonEvent::Click;
  }

  return ButtonEvent::None;
}
