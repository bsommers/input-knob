#include "display.h"

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>

#include "board.h"
#include "config.h"

// Panel settings match Elecrow's Lesson03 example.
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 panel;
  lgfx::Bus_SPI bus;

 public:
  LGFX() {
    auto busConfig = bus.config();
    busConfig.spi_host = SPI2_HOST;
    busConfig.freq_write = 80000000;
    busConfig.freq_read = 20000000;
    busConfig.spi_3wire = true;
    busConfig.pin_sclk = pins::LCD_SCLK;
    busConfig.pin_mosi = pins::LCD_MOSI;
    busConfig.pin_miso = -1;
    busConfig.pin_dc = pins::LCD_DC;
    bus.config(busConfig);
    panel.setBus(&bus);

    auto panelConfig = panel.config();
    panelConfig.pin_cs = pins::LCD_CS;
    panelConfig.pin_rst = pins::LCD_RST;
    panelConfig.memory_width = SCREEN_WIDTH;
    panelConfig.memory_height = SCREEN_HEIGHT;
    panelConfig.panel_width = SCREEN_WIDTH;
    panelConfig.panel_height = SCREEN_HEIGHT;
    panelConfig.invert = true;
    panelConfig.readable = false;
    panelConfig.bus_shared = false;
    panel.config(panelConfig);
    setPanel(&panel);
  }
};

static LGFX gfx;
static constexpr int DRAW_BUFFER_LINES = 40;

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
  const int width = area->x2 - area->x1 + 1;
  const int height = area->y2 - area->y1 + 1;
  gfx.pushImage(area->x1, area->y1, width, height, reinterpret_cast<lgfx::rgb565_t *>(pixels));
  lv_display_flush_ready(display);
}

static void *allocBuffer(size_t size) {
  void *buffer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  return buffer ? buffer : malloc(size);
}

void displayBegin() {
  pinMode(pins::POWER_LED, OUTPUT);
  digitalWrite(pins::POWER_LED, LOW);
  pinMode(pins::POWER_LCD, OUTPUT);
  pinMode(pins::POWER_EXT, OUTPUT);
  digitalWrite(pins::POWER_LCD, HIGH);
  digitalWrite(pins::POWER_EXT, HIGH);

  ledcAttach(pins::BACKLIGHT, 5000, 8);
  ledcWrite(pins::BACKLIGHT, BACKLIGHT_PERCENT * 255UL / 100UL);

  gfx.init();
  gfx.fillScreen(TFT_BLACK);

  lv_init();
  lv_tick_set_cb(millis);

  const size_t bufferSize = SCREEN_WIDTH * DRAW_BUFFER_LINES * sizeof(uint16_t);
  lv_display_t *display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(display, flush);
  lv_display_set_buffers(display, allocBuffer(bufferSize), allocBuffer(bufferSize), bufferSize,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void displayLoop() { lv_timer_handler(); }
