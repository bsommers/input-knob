#include "leds.h"

#include <Adafruit_NeoPixel.h>
#include <math.h>

#include "board.h"
#include "config.h"

static constexpr int LED_COUNT = 5;
static constexpr uint32_t FRAME_MS = 20;
static constexpr uint32_t NEEDLE_FADE_MS = 1500;
static constexpr uint32_t FLASH_MS = 300;
static constexpr uint32_t BREATHE_PERIOD_MS = 2000;
static constexpr float GLOW_LEVEL = 0.15f;
static constexpr uint32_t PAIRING_COLOR = 0xF39C12;
static constexpr uint32_t LINK_LOST_COLOR = 0xE74C3C;

static Adafruit_NeoPixel ring(LED_COUNT, pins::RGB_LEDS, NEO_GRB + NEO_KHZ800);
static uint32_t modeColor = 0;
static hid::Link currentLink = hid::Link::None;
static int needle = 0;
static uint32_t needleAtMs = 0;
static uint32_t flashColor = 0;
static uint32_t flashAtMs = 0;
static bool flashing = false;

// Scale a 0xRRGGBB colour by level (0..1) and the global LED_BRIGHTNESS cap.
static uint32_t dimmed(uint32_t color, float level) {
  const float scale = level * LED_BRIGHTNESS / 255.0f;
  return ring.Color(((color >> 16) & 0xFF) * scale, ((color >> 8) & 0xFF) * scale, (color & 0xFF) * scale);
}

static bool isConnected(hid::Link state) {
  return state == hid::Link::Usb || state == hid::Link::Bluetooth;
}

void ledsBegin() {
  ring.begin();
  ring.clear();
  ring.show();
}

void ledsSetModeColor(uint32_t color) { modeColor = color; }

void ledsSetLink(hid::Link next) {
  if (isConnected(currentLink) && !isConnected(next)) ledsFlash(LINK_LOST_COLOR);
  currentLink = next;
}

void ledsTurn(int detents) {
  const int step = LED_RING_REVERSED ? -detents : detents;
  needle = ((needle + step) % LED_COUNT + LED_COUNT) % LED_COUNT;
  needleAtMs = millis();
}

void ledsFlash(uint32_t color) {
  flashColor = color;
  flashAtMs = millis();
  flashing = true;
}

void ledsLoop() {
  static uint32_t lastFrameMs = 0;
  const uint32_t now = millis();
  if (now - lastFrameMs < FRAME_MS) return;
  lastFrameMs = now;

  if (flashing && now - flashAtMs < FLASH_MS) {
    ring.fill(dimmed(flashColor, 1.0f - static_cast<float>(now - flashAtMs) / FLASH_MS));
    ring.show();
    return;
  }
  flashing = false;

  if (currentLink == hid::Link::Pairing) {
    const float phase = static_cast<float>(now % BREATHE_PERIOD_MS) / BREATHE_PERIOD_MS;
    ring.fill(dimmed(PAIRING_COLOR, 0.1f + 0.9f * (0.5f - 0.5f * cosf(2 * PI * phase))));
  } else {
    ring.fill(dimmed(modeColor, GLOW_LEVEL));
    if (needleAtMs != 0 && now - needleAtMs < NEEDLE_FADE_MS) {
      const float fade = 1.0f - static_cast<float>(now - needleAtMs) / NEEDLE_FADE_MS;
      ring.setPixelColor(needle, dimmed(modeColor, GLOW_LEVEL + (1.0f - GLOW_LEVEL) * fade));
    }
  }
  ring.show();
}
