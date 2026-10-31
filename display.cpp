#include "display.h"

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>

#include "board.h"
#include "config.h"

// Elecrow's 1.46" 360x360 panel (P146B001-IPS-CTP-V2, JD9855 controller).
// Init sequence comes from Elecrow's 1.46" LovyanGFX Panel_ST77961.hpp.
struct Panel_JD9855 : public lgfx::Panel_LCD {
  Panel_JD9855() {
    _cfg.panel_width = _cfg.memory_width = 360;
    _cfg.panel_height = _cfg.memory_height = 360;
    _cfg.dummy_read_pixel = 8;
  }

 protected:
  const uint8_t *getInitCommands(uint8_t listno) const override {
    static constexpr uint8_t list0[] = {
        0xDE, 1, 0x00,
        0xDF, 2, 0x98, 0x55,
        0xB2, 1, 0x1F,
        0xB7, 4, 0x00, 0x1D, 0x00, 0x45,
        0xBB, 6, 0x1B, 0x64, 0xC4, 0x1E, 0x3E, 0xF5,
        0xBC, 4, 0x03, 0x20, 0xF3, 0xC0,
        0xC0, 2, 0x22, 0xA1,
        0xC3, 11, 0x00, 0x02, 0x2A, 0x0B, 0x08, 0x48, 0x08, 0x04, 0x62, 0x30, 0x30,
        0xC4, 11, 0x40, 0x00, 0xAD, 0x68, 0x43, 0x07, 0x04, 0x16, 0x43, 0x07, 0x04,
        0xC8, 32,
            0x3F, 0x36, 0x30, 0x2D, 0x2F, 0x31, 0x2B, 0x2A,
            0x27, 0x25, 0x1F, 0x14, 0x10, 0x09, 0x04, 0x00,
            0x3F, 0x36, 0x30, 0x2D, 0x2F, 0x31, 0x2B, 0x2A,
            0x27, 0x25, 0x1F, 0x14, 0x10, 0x09, 0x04, 0x00,
        0xD3, 2, 0x28, 0x13,
        0xD9, 6, 0x00, 0x00, 0xFF, 0x00, 0xF0, 0x00,
        0xDE, 1, 0x01,
        0xB7, 8, 0x13, 0xE7, 0x64, 0x39, 0x06, 0x36, 0x19, 0x1C,
        0xBE, 1, 0x00,
        0xC1, 3, 0x00, 0x4A, 0x80,
        0xC2, 4, 0x00, 0x16, 0xDA, 0xE7,
        0xC7, 8, 0x00, 0x00, 0x00, 0x38, 0x08, 0x08, 0x00, 0x01,
        0xC8, 6, 0x00, 0x00, 0x00, 0x00, 0x15, 0x3B,
        0xC9, 5, 0x00, 0x16, 0x06, 0x04, 0x0A,
        0xCA, 5, 0x08, 0x35, 0x16, 0x1F, 0x1F,
        0xCB, 5, 0x01, 0x16, 0x07, 0x05, 0x0B,
        0xCC, 5, 0x09, 0x35, 0x16, 0x1F, 0x1F,
        0xCD, 5, 0x01, 0x16, 0x09, 0x0B, 0x05,
        0xCE, 5, 0x07, 0x15, 0x1F, 0x16, 0x1F,
        0xCF, 5, 0x00, 0x16, 0x08, 0x0A, 0x04,
        0xD0, 5, 0x06, 0x15, 0x1F, 0x16, 0x1F,
        0xD1, 2, 0x02, 0x30,
        0xD2, 5, 0x02, 0x03, 0x52, 0xDF, 0xDD,
        0xD3, 3, 0x3B, 0x04, 0x48,
        0xD5, 7, 0x10, 0x10, 0x07, 0x07, 0x0F, 0x94, 0x26,
        0xD6, 3, 0x00, 0x00, 0x40,
        0xD7, 3, 0x00, 0x00, 0x20,
        0xDE, 1, 0x02,
        0xB6, 1, 0x1C,
        0xDE, 1, 0x00,
        0x4D, 1, 0x00,
        0x4E, 1, 0x00,
        0x4F, 1, 0x00,
        0x4C, 1 + CMD_INIT_DELAY, 0x01, 10,
        0x4C, 1, 0x00,
        0x35, 0,
        0x3A, 1, 0x05,
        0x2A, 4, 0x00, 0x00, 0x01, 0x67,
        0x2B, 4, 0x00, 0x00, 0x01, 0x67,
        CMD_SLPOUT, CMD_INIT_DELAY, 120,
        CMD_DISPON, 0,
        0xFF, 0xFF,
    };
    return listno == 0 ? list0 : nullptr;
  }

  void setColorDepth_impl(lgfx::color_depth_t depth) override {
    _write_depth = ((int)depth & lgfx::color_depth_t::bit_mask) > 16
                       ? lgfx::rgb888_3Byte
                       : lgfx::rgb565_2Byte;
    _read_depth = _write_depth;
  }
};

// Panel settings match Elecrow's Lesson03 examples for 1.28" and 1.46".
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 panel128;
  Panel_JD9855 panel146;
  lgfx::Bus_SPI bus;

 public:
  void configure(BoardModel model) {
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

    if (model == BoardModel::CrowPanel146) {
      panel146.setBus(&bus);
      auto panelConfig = panel146.config();
      panelConfig.pin_cs = pins::LCD_CS;
      panelConfig.pin_rst = pins::LCD_RST;
      panelConfig.memory_width = 360;
      panelConfig.memory_height = 360;
      panelConfig.panel_width = 360;
      panelConfig.panel_height = 360;
      panelConfig.invert = false;
      panelConfig.rgb_order = true;
      panelConfig.dummy_read_pixel = 8;
      panelConfig.dummy_read_bits = 1;
      panelConfig.readable = false;
      panelConfig.bus_shared = false;
      panel146.config(panelConfig);
      setPanel(&panel146);
    } else {
      panel128.setBus(&bus);
      auto panelConfig = panel128.config();
      panelConfig.pin_cs = pins::LCD_CS;
      panelConfig.pin_rst = pins::LCD_RST;
      panelConfig.memory_width = 240;
      panelConfig.memory_height = 240;
      panelConfig.panel_width = 240;
      panelConfig.panel_height = 240;
      panelConfig.invert = true;
      panelConfig.readable = false;
      panelConfig.bus_shared = false;
      panel128.config(panelConfig);
      setPanel(&panel128);
    }
  }
};

static LGFX gfx;
static constexpr int DRAW_BUFFER_LINES = 40;

BoardModel boardModel() {
  if constexpr (CROWPANEL_SIZE == 146) return BoardModel::CrowPanel146;
  if constexpr (CROWPANEL_SIZE == 128) return BoardModel::CrowPanel128;

  static bool detected = false;
  static BoardModel model = BoardModel::CrowPanel128;
  if (!detected) {
    // On the 1.46" board, GPIO17 (POWER_RGB) feeds an NPN transistor base
    // through a 10K resistor to GND (~1.1V with internal 45K pull-up);
    // on the 1.28" board GPIO17 is unconnected (~3.3V with pull-up).
    pinMode(pins::POWER_RGB, INPUT_PULLUP);
    delay(1);
    const int mv = analogReadMilliVolts(pins::POWER_RGB);
    model = mv < 2200 ? BoardModel::CrowPanel146 : BoardModel::CrowPanel128;
    detected = true;
  }
  return model;
}

int boardScreenSize() {
  return boardModel() == BoardModel::CrowPanel146 ? 360 : 240;
}

int boardLedCount() {
  return boardModel() == BoardModel::CrowPanel146 ? 8 : 5;
}

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
  const BoardModel model = boardModel();

  pinMode(pins::POWER_LED, OUTPUT);
  digitalWrite(pins::POWER_LED, LOW);
  pinMode(pins::POWER_LCD, OUTPUT);
  pinMode(pins::POWER_EXT, OUTPUT);
  pinMode(pins::POWER_RGB, OUTPUT);
  digitalWrite(pins::POWER_LCD, HIGH);
  digitalWrite(pins::POWER_EXT, HIGH);
  digitalWrite(pins::POWER_RGB, HIGH);

  ledcAttach(pins::BACKLIGHT, 5000, 8);
  ledcWrite(pins::BACKLIGHT, BACKLIGHT_PERCENT * 255UL / 100UL);

  gfx.configure(model);
  gfx.init();
  gfx.fillScreen(TFT_BLACK);

  lv_init();
  lv_tick_set_cb(millis);

  const int screenSize = boardScreenSize();
  const size_t bufferSize = screenSize * DRAW_BUFFER_LINES * sizeof(uint16_t);
  lv_display_t *display = lv_display_create(screenSize, screenSize);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(display, flush);
  lv_display_set_buffers(display, allocBuffer(bufferSize), allocBuffer(bufferSize), bufferSize,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void displayLoop() { lv_timer_handler(); }
