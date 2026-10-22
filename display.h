#pragma once

// Powers the LCD, starts LovyanGFX and registers it as the LVGL display.
void displayBegin();

// Run LVGL timers; call every loop.
void displayLoop();
