#pragma once

// Elecrow CrowPanel 1.28" ESP32-S3 Rotary Display (240x240 GC9A01 + CST816D).
// Pin numbers come from Elecrow's example sketches and board schematic:
// https://github.com/Elecrow-RD/CrowPanel-1.28inch-HMI-ESP32-Rotary-Display-240-240-IPS-Round-Touch-Knob-Screen

namespace pins {
constexpr int POWER_LCD = 1;   // LCD 3V3 rail enable, active high
constexpr int POWER_EXT = 2;   // UART/I2C header 5V rail enable, active high
constexpr int POWER_LED = 40;  // red power LED, active low
constexpr int BACKLIGHT = 46;  // PWM

constexpr int LCD_SCLK = 10;
constexpr int LCD_MOSI = 11;
constexpr int LCD_DC = 3;
constexpr int LCD_CS = 9;
constexpr int LCD_RST = 14;

constexpr int TOUCH_SDA = 6;
constexpr int TOUCH_SCL = 7;
constexpr int TOUCH_RST = 13;
constexpr int TOUCH_INT = 5;

constexpr int ENC_A = 45;
constexpr int ENC_B = 42;
constexpr int ENC_SW = 41;  // active low, needs pull-up

constexpr int RGB_LEDS = 48;  // 5x WS2812B ring
}  // namespace pins

constexpr int SCREEN_WIDTH = 240;
constexpr int SCREEN_HEIGHT = 240;
