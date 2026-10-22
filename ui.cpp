#include "ui.h"

#include <lvgl.h>

static constexpr int DEGREES_PER_DETENT = 15;
static constexpr int DIAL_SEGMENT_DEGREES = 30;

static lv_obj_t *dial;
static lv_obj_t *modeLabel;
static lv_obj_t *valueLabel;
static lv_obj_t *hintLabel;
static const char *modeHint = "";
static int dialAngle = 270;  // 12 o'clock in LVGL's arc coordinates

static void drawDial() {
  lv_arc_set_angles(dial, dialAngle, dialAngle + DIAL_SEGMENT_DEGREES);
}

void uiBegin() {
  lv_obj_t *screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

  dial = lv_arc_create(screen);
  lv_obj_set_size(dial, 232, 232);
  lv_obj_center(dial);
  lv_obj_remove_style(dial, nullptr, LV_PART_KNOB);
  lv_obj_remove_flag(dial, LV_OBJ_FLAG_CLICKABLE);
  lv_arc_set_bg_angles(dial, 0, 360);
  lv_obj_set_style_arc_width(dial, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_width(dial, 10, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(dial, lv_color_hex(0x202020), LV_PART_MAIN);
  drawDial();

  modeLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(modeLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(modeLabel, LV_ALIGN_TOP_MID, 0, 50);

  valueLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(valueLabel, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(valueLabel, lv_color_white(), 0);
  lv_obj_align(valueLabel, LV_ALIGN_CENTER, 0, 0);

  hintLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(hintLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hintLabel, LV_ALIGN_BOTTOM_MID, 0, -45);
}

void uiSetMode(const char *name, const char *hint, uint32_t color) {
  lv_label_set_text(modeLabel, name);
  lv_obj_set_style_text_color(modeLabel, lv_color_hex(color), 0);
  lv_obj_set_style_arc_color(dial, lv_color_hex(color), LV_PART_INDICATOR);
  modeHint = hint;
  lv_label_set_text(hintLabel, hint);
  lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x909090), 0);
}

void uiSetValue(const char *text) { lv_label_set_text(valueLabel, text); }

void uiTurn(int detents) {
  dialAngle = ((dialAngle + detents * DEGREES_PER_DETENT) % 360 + 360) % 360;
  drawDial();
}

void uiSetConnected(bool connected) {
  lv_label_set_text(hintLabel, connected ? modeHint : "USB not connected");
  lv_obj_set_style_text_color(hintLabel, lv_color_hex(connected ? 0x909090 : 0xE74C3C), 0);
}
