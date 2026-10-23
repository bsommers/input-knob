#pragma once
#include <stdint.h>

#include "hid.h"

// 5-LED WS2812B ring: a dim glow in the mode colour, a needle that follows the
// knob, short flashes for actions, and breathing orange while pairing.

void ledsBegin();
void ledsLoop();  // call every loop; animates at about 50 fps
void ledsSetModeColor(uint32_t color);
void ledsSetLink(hid::Link link);
void ledsTurn(int detents);
void ledsFlash(uint32_t color);
