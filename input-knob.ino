// Input Knob: USB / Bluetooth volume, scroll and macro controller for the
// Elecrow CrowPanel 1.28" ESP32-S3 Rotary Display.
//
//   Turn          volume, scroll wheel, or pick a macro (depends on mode)
//   Click         mute / middle click / run selected macro
//   Double click  play-pause / toggle horizontal scroll / Mac <-> PC shortcuts
//   Long press    next mode

#include <Preferences.h>

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
static bool macShortcuts = true;
static hid::Link shownLink = hid::Link::None;
static Preferences settings;

static const Macro &currentMacro() {
  return macShortcuts ? MAC_MACROS[selectedMacro] : PC_MACROS[selectedMacro];
}

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
      uiSetMode(macShortcuts ? "Macro (Mac)" : "Macro (PC)", "click run | 2x Mac/PC", 0xF39C12);
      uiSetValue(currentMacro().name);
      break;
    default:
      break;
  }
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
      uiSetValue(currentMacro().name);
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
      if (event == ButtonEvent::Click) {
        hid::runMacro(currentMacro());
      } else {
        macShortcuts = !macShortcuts;
        settings.putBool("mac", macShortcuts);
        showMode();
      }
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  settings.begin("input-knob");
  macShortcuts = settings.getBool("mac", true);

  displayBegin();
  uiBegin();
  knobBegin();
  hid::begin();
  showMode();
  uiSetLink(shownLink);
}

void loop() {
  const int detents = knobTakeDetents();
  if (detents != 0) onTurn(detents);

  const ButtonEvent event = buttonPoll();
  if (event != ButtonEvent::None) onButton(event);

  hid::service();
  const hid::Link link = hid::link();
  if (link != shownLink) {
    shownLink = link;
    uiSetLink(link);
  }

  displayLoop();
  delay(2);
}
