// Input Knob: USB / Bluetooth volume, scroll, macro and trackpad controller
// for the Elecrow CrowPanel 1.28" ESP32-S3 Rotary Display.
//
//   Turn          volume, scroll wheel, pick a macro, or scroll (trackpad)
//   Click         mute / middle click / run selected macro / left click
//   Double click  play-pause / toggle horizontal scroll / Mac <-> PC shortcuts / double click
//   Long press    next mode
//
//   Touch (Trackpad mode)  drag moves the cursor, tap = left click, hold = right click
//   Touch (other modes)    tap = click, swipe left/right = next/previous mode,
//                          swipe up/down = page scroll (Scroll) or next/previous macro (Macro)

#include <Preferences.h>

#include "config.h"
#include "display.h"
#include "hid.h"
#include "keys.h"
#include "knob.h"
#include "leds.h"
#include "macros.h"
#include "touch.h"
#include "ui.h"

enum class Mode : uint8_t { Volume, Scroll, Macro, Trackpad, Count };

static constexpr uint32_t MODE_COLORS[] = {0x3498DB, 0x20C878, 0xF39C12, 0x9B59B6};
static constexpr uint32_t MUTE_COLOR = 0xE74C3C;
static constexpr uint32_t MACRO_RUN_COLOR = 0xFFFFFF;

static Mode mode = Mode::Volume;
static bool horizontalScroll = false;
static int selectedMacro = 0;
static bool macShortcuts = true;
static hid::Link shownLink = hid::Link::None;
static Preferences settings;

static uint32_t modeColor() { return MODE_COLORS[static_cast<uint8_t>(mode)]; }

static const Macro &currentMacro() {
  return macShortcuts ? MAC_MACROS[selectedMacro] : PC_MACROS[selectedMacro];
}

static void showMode() {
  switch (mode) {
    case Mode::Volume:
      uiSetMode("Volume", "click mute | 2x play", modeColor());
      uiSetValue("Turn me");
      break;
    case Mode::Scroll:
      uiSetMode("Scroll", "click middle | 2x axis", modeColor());
      uiSetValue(horizontalScroll ? "Horizontal" : "Vertical");
      break;
    case Mode::Macro:
      uiSetMode(macShortcuts ? "Macro (Mac)" : "Macro (PC)", "click run | 2x Mac/PC", modeColor());
      uiSetValue(currentMacro().name);
      break;
    case Mode::Trackpad:
      uiSetMode("Trackpad", "tap click | hold right", modeColor());
      uiSetValue("Drag here");
      break;
    default:
      break;
  }
  ledsSetModeColor(modeColor());
}

static void changeMode(int step) {
  constexpr int count = static_cast<int>(Mode::Count);
  mode = static_cast<Mode>(((static_cast<int>(mode) + step) % count + count) % count);
  showMode();
}

static void scrollBy(int lines) {
  const int8_t amount = static_cast<int8_t>(constrain(lines, -127, 127));
  if (horizontalScroll) {
    hid::scroll(0, amount);
  } else {
    hid::scroll(-amount, 0);  // positive (clockwise / swipe up) scrolls down
  }
}

static void onTurn(int detents) {
  uiTurn(detents);
  ledsTurn(detents);
  switch (mode) {
    case Mode::Volume:
      hid::volume(detents);
      uiSetValue(detents > 0 ? "Vol +" : "Vol -");
      break;
    case Mode::Scroll:
      scrollBy(detents * SCROLL_LINES_PER_DETENT);
      break;
    case Mode::Macro:
      selectedMacro = ((selectedMacro + detents) % MACRO_COUNT + MACRO_COUNT) % MACRO_COUNT;
      uiSetValue(currentMacro().name);
      break;
    case Mode::Trackpad:
      hid::scroll(-static_cast<int8_t>(constrain(detents * SCROLL_LINES_PER_DETENT, -127, 127)), 0);
      break;
    default:
      break;
  }
}

static void onButton(ButtonEvent event) {
  if (event == ButtonEvent::LongPress) {
    changeMode(1);
    return;
  }

  uint32_t flash = modeColor();
  switch (mode) {
    case Mode::Volume:
      if (event == ButtonEvent::Click) {
        hid::mute();
        uiSetValue("Mute");
        flash = MUTE_COLOR;
      } else {
        hid::playPause();
        uiSetValue("Play/Pause");
      }
      break;
    case Mode::Scroll:
      if (event == ButtonEvent::Click) {
        hid::click(MOUSE_BUTTON_MIDDLE);
      } else {
        horizontalScroll = !horizontalScroll;
        uiSetValue(horizontalScroll ? "Horizontal" : "Vertical");
      }
      break;
    case Mode::Macro:
      if (event == ButtonEvent::Click) {
        hid::runMacro(currentMacro());
        flash = MACRO_RUN_COLOR;
      } else {
        macShortcuts = !macShortcuts;
        settings.putBool("mac", macShortcuts);
        showMode();
      }
      break;
    case Mode::Trackpad:
      hid::click(MOUSE_BUTTON_LEFT);
      if (event == ButtonEvent::DoubleClick) hid::click(MOUSE_BUTTON_LEFT);
      break;
    default:
      break;
  }
  ledsFlash(flash);
}

static int accelerate(int16_t delta) {
  return lroundf(delta * (TRACKPAD_GAIN + TRACKPAD_ACCELERATION * abs(delta)));
}

static void onTrackpadTouch(const TouchUpdate &touch) {
  if (touch.dx != 0 || touch.dy != 0) hid::moveCursor(accelerate(touch.dx), accelerate(touch.dy));
  if (touch.gesture == TouchGesture::Tap) {
    hid::click(MOUSE_BUTTON_LEFT);
    uiSetValue("Click");
    ledsFlash(modeColor());
  } else if (touch.gesture == TouchGesture::LongPress) {
    hid::click(MOUSE_BUTTON_RIGHT);
    uiSetValue("Right click");
    ledsFlash(modeColor());
  }
}

static void onVerticalSwipe(int direction) {  // +1 = swipe up
  if (mode == Mode::Scroll) {
    scrollBy(direction * SWIPE_SCROLL_LINES);
    uiSetValue(direction > 0 ? "Page down" : "Page up");
  } else if (mode == Mode::Macro) {
    onTurn(direction);
  }
}

static void onTouch(const TouchUpdate &touch) {
  if (mode == Mode::Trackpad) {
    onTrackpadTouch(touch);
    return;
  }
  switch (touch.gesture) {
    case TouchGesture::Tap:
      onButton(ButtonEvent::Click);
      break;
    case TouchGesture::SwipeLeft:
      changeMode(1);
      break;
    case TouchGesture::SwipeRight:
      changeMode(-1);
      break;
    case TouchGesture::SwipeUp:
      onVerticalSwipe(1);
      break;
    case TouchGesture::SwipeDown:
      onVerticalSwipe(-1);
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
  ledsBegin();
  knobBegin();
  if (!touchBegin()) Serial.println("[touch] CST816D not responding");
  hid::begin();
  showMode();
  uiSetLink(shownLink);
}

void loop() {
  const int detents = knobTakeDetents();
  if (detents != 0) onTurn(detents);

  const ButtonEvent event = buttonPoll();
  if (event != ButtonEvent::None) onButton(event);

  onTouch(touchPoll());

  hid::service();
  const hid::Link link = hid::link();
  if (link != shownLink) {
    shownLink = link;
    uiSetLink(link);
    ledsSetLink(link);
  }

  ledsLoop();
  displayLoop();
  delay(2);
}
