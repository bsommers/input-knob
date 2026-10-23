#include "touch.h"

#include <Arduino.h>
#include <Wire.h>

#include "board.h"
#include "config.h"

static constexpr uint8_t ADDRESS = 0x15;
static constexpr uint8_t REG_FINGER_COUNT = 0x02;  // followed by X high, X low, Y high, Y low
static constexpr uint8_t REG_DISABLE_AUTO_SLEEP = 0xFE;
static constexpr uint32_t POLL_EVERY_MS = 10;

static bool readPoint(bool *touching, int16_t *x, int16_t *y) {
  Wire1.beginTransmission(ADDRESS);
  Wire1.write(REG_FINGER_COUNT);
  if (Wire1.endTransmission(false) != 0) return false;
  uint8_t data[5];
  if (Wire1.requestFrom(static_cast<uint16_t>(ADDRESS), sizeof(data)) != sizeof(data)) return false;
  for (uint8_t &value : data) value = Wire1.read();
  *touching = data[0] != 0;
  *x = ((data[1] & 0x0F) << 8) | data[2];
  *y = ((data[3] & 0x0F) << 8) | data[4];
  return true;
}

static TouchGesture swipe(int16_t dx, int16_t dy) {
  if (abs(dx) >= abs(dy)) {
    if (abs(dx) < TOUCH_SWIPE_MIN_PX) return TouchGesture::None;
    return dx > 0 ? TouchGesture::SwipeRight : TouchGesture::SwipeLeft;
  }
  if (abs(dy) < TOUCH_SWIPE_MIN_PX) return TouchGesture::None;
  return dy > 0 ? TouchGesture::SwipeDown : TouchGesture::SwipeUp;
}

bool touchBegin() {
  Wire1.begin(pins::TOUCH_SDA, pins::TOUCH_SCL, 400000);

  // The controller expects a pulse on INT, then a hardware reset.
  pinMode(pins::TOUCH_INT, OUTPUT);
  digitalWrite(pins::TOUCH_INT, HIGH);
  delay(1);
  digitalWrite(pins::TOUCH_INT, LOW);
  delay(1);
  pinMode(pins::TOUCH_INT, INPUT);
  pinMode(pins::TOUCH_RST, OUTPUT);
  digitalWrite(pins::TOUCH_RST, LOW);
  delay(10);
  digitalWrite(pins::TOUCH_RST, HIGH);
  delay(300);

  // Otherwise the chip sleeps after a few idle seconds and stops answering.
  Wire1.beginTransmission(ADDRESS);
  Wire1.write(REG_DISABLE_AUTO_SLEEP);
  Wire1.write(0xFF);
  return Wire1.endTransmission() == 0;
}

TouchUpdate touchPoll() {
  static uint32_t lastPollMs = 0;
  static bool down = false;
  static bool moved = false;
  static bool longFired = false;
  static int16_t startX = 0, startY = 0, lastX = 0, lastY = 0;
  static uint32_t downAtMs = 0;

  TouchUpdate update = {TouchGesture::None, 0, 0};
  const uint32_t now = millis();
  if (now - lastPollMs < POLL_EVERY_MS) return update;
  lastPollMs = now;

  bool touching = false;
  int16_t x = lastX, y = lastY;
  if (!readPoint(&touching, &x, &y)) touching = false;

  if (touching && !down) {
    down = true;
    moved = false;
    longFired = false;
    startX = lastX = x;
    startY = lastY = y;
    downAtMs = now;
  } else if (touching) {
    update.dx = x - lastX;
    update.dy = y - lastY;
    lastX = x;
    lastY = y;
    if (abs(x - startX) > TOUCH_TAP_SLOP_PX || abs(y - startY) > TOUCH_TAP_SLOP_PX) moved = true;
    if (!moved && !longFired && now - downAtMs >= BUTTON_LONG_PRESS_MS) {
      longFired = true;
      update.gesture = TouchGesture::LongPress;
    }
  } else if (down) {
    down = false;
    if (!longFired) {
      update.gesture = moved ? swipe(lastX - startX, lastY - startY) : TouchGesture::Tap;
    }
  }
  return update;
}
