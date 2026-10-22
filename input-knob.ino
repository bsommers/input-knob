// Input Knob: USB volume / scroll / macro controller for the Elecrow
// CrowPanel 1.28" ESP32-S3 Rotary Display.
//
//   Turn          volume, scroll wheel, or pick a macro (depends on mode)
//   Click         mute / middle click / run selected macro
//   Double click  play-pause / toggle horizontal scroll
//   Long press    next mode

#include "config.h"
#include "display.h"
#include "hid.h"
#include "knob.h"
#include "macros.h"
#include "ui.h"

enum class Mode : uint8_t { Volume, Scroll, Macro, Count };

static Mode mode = Mode::Volume;
static bool horizontalScroll = false;
static int selectedMacro = 0;
static bool wasConnected = false;

static void showMode() {
  switch (mode) {
    case Mode::Volume:
      uiSetMode("Volume", "click mute | 2x play", 0x3498DB);
      uiSetValue("Turn me");
      break;
    case Mode::Scroll:
      uiSetMode("Scroll", "click middle | 2x axis", 0x20C878);
      uiSetValue(horizontalScroll ? "Horizontal" : "Vertical");
      break;
    case Mode::Macro:
      uiSetMode("Macro", "click to run", 0xF39C12);
      uiSetValue(MACROS[selectedMacro].name);
      break;
    default:
      break;
  }
  uiSetConnected(hid::connected());
}

static void onTurn(int detents) {
  uiTurn(detents);
  switch (mode) {
    case Mode::Volume:
      hid::volume(detents);
      uiSetValue(detents > 0 ? "Vol +" : "Vol -");
      break;
    case Mode::Scroll: {
      const int8_t amount = static_cast<int8_t>(constrain(detents * SCROLL_LINES_PER_DETENT, -127, 127));
      if (horizontalScroll) {
        hid::scroll(0, amount);
      } else {
        hid::scroll(-amount, 0);  // clockwise scrolls down
      }
      break;
    }
    case Mode::Macro:
      selectedMacro = ((selectedMacro + detents) % MACRO_COUNT + MACRO_COUNT) % MACRO_COUNT;
      uiSetValue(MACROS[selectedMacro].name);
      break;
    default:
      break;
  }
}

static void onButton(ButtonEvent event) {
  if (event == ButtonEvent::LongPress) {
    mode = static_cast<Mode>((static_cast<uint8_t>(mode) + 1) % static_cast<uint8_t>(Mode::Count));
    showMode();
    return;
  }

  switch (mode) {
    case Mode::Volume:
      if (event == ButtonEvent::Click) {
        hid::mute();
        uiSetValue("Mute");
      } else {
        hid::playPause();
        uiSetValue("Play/Pause");
      }
      break;
    case Mode::Scroll:
      if (event == ButtonEvent::Click) {
        hid::middleClick();
      } else {
        horizontalScroll = !horizontalScroll;
        uiSetValue(horizontalScroll ? "Horizontal" : "Vertical");
      }
      break;
    case Mode::Macro:
      if (event == ButtonEvent::Click) hid::runMacro(MACROS[selectedMacro]);
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  displayBegin();
  uiBegin();
  knobBegin();
  hid::begin();
  showMode();
}

void loop() {
  const int detents = knobTakeDetents();
  if (detents != 0) onTurn(detents);

  const ButtonEvent event = buttonPoll();
  if (event != ButtonEvent::None) onButton(event);

  const bool connected = hid::connected();
  if (connected != wasConnected) {
    wasConnected = connected;
    uiSetConnected(connected);
  }

  displayLoop();
  delay(2);
}
