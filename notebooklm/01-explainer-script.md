# Input Knob: explainer script

A narrated script for a 6–8 minute video and a matching slide deck about **Input Knob**, open-source firmware that turns the Elecrow CrowPanel 1.28" ESP32-S3 rotary display into a volume, scroll and shortcut controller for computers, phones and tablets.

Each scene has **Narration** (spoken), **On screen** (visual and image file to use), and **Slide** (title and bullets for the deck). Image files are in `images/`. Screen images are illustrations drawn from the firmware's layout, not photographs.

Project: github.com/bsommers/input-knob · License: Apache 2.0

---

## Scene 1 — The hook (0:00–0:30)

**Narration**
How many times a day do you reach for volume? Mute before a call, skip a bit louder, scroll through a long document, copy, paste, undo. Each one is a key combination or a trip to a menu. What if all of that lived on one dial on your desk, with a little round screen that tells you exactly what it's going to do?

**On screen**: `images/screen-overview.png`, slow zoom across the eight screens.

**Slide**: *One dial for volume, scrolling and shortcuts*
- Physical control for the things you do all day
- A round screen that says what the knob will do
- Works with computers, phones and tablets

---

## Scene 2 — The hardware (0:30–1:10)

**Narration**
The hardware is the Elecrow CrowPanel 1.28-inch rotary display. In the middle is a 240-by-240 round colour screen. Around it is a knob that clicks as you turn it, and the whole knob pushes down like a button. Inside is an ESP32-S3: a microcontroller with native USB and Bluetooth Low Energy, 16 megabytes of flash and 8 megabytes of PSRAM. The screen is also a touch panel, and there's a ring of five RGB LEDs, and Input Knob uses both. Out of the box it runs a demo. Input Knob replaces that demo with something you'd actually use every day.

**On screen**: `images/code-01-pin-map.png` (the pin map), then `images/screen-volume-usb.png`.

**Slide**: *Elecrow CrowPanel 1.28" rotary display*
- 240×240 round IPS screen (GC9A01)
- Detented rotary encoder plus push button
- ESP32-S3: native USB, Bluetooth LE, 16 MB flash, 8 MB PSRAM
- CST816D touch panel and 5 WS2812 RGB LEDs, both used

---

## Scene 3 — What it does (1:10–2:20)

**Narration**
Input Knob has three modes, and you switch between them with a long press. Every mode uses the same four gestures: turn, click, double-click and long press.

In **Volume** mode, turning changes the computer's volume, a click mutes, and a double-click plays or pauses music.

In **Scroll** mode, the knob becomes a scroll wheel with real detents, so you can move through a document one notch at a time. A click is a middle-click, and a double-click switches between vertical and horizontal scrolling, which is great for timelines and wide spreadsheets.

In **Macro** mode, turning picks a shortcut by name — Copy, Paste, Undo, Redo, Screenshot, Mission Control, Lock — and a click sends it. A double-click switches between Mac shortcuts using Command and PC shortcuts using Control and the Windows key, and the knob remembers your choice.

The screen always shows the current mode, the last action, a hint for click and double-click, and a coloured ring that moves as you turn.

**On screen**: `images/screen-volume-usb.png` → `images/screen-volume-mute.png` → `images/screen-scroll.png` → `images/screen-macro-mac.png` → `images/screen-macro-pc.png`.

**Slide**: *Three modes, one set of gestures*

| Mode | Turn | Click | Double-click |
|---|---|---|---|
| Volume | volume up / down | mute | play / pause |
| Scroll | scroll wheel | middle click | vertical ↔ horizontal |
| Macro | choose a shortcut | run it | Mac ↔ PC shortcuts |

Long press: next mode.

---

## Scene 4 — No drivers, no app (2:20–2:50)

**Narration**
Here's the part that makes it work everywhere: the knob doesn't need any software on your computer. It introduces itself as an ordinary keyboard, mouse and media-key device, using the same HID standard every keyboard uses. So macOS, Windows, Linux, iPhone, iPad and Android all accept it the moment it's connected.

**On screen**: `images/diagram-system.png`.

**Slide**: *Standard HID: nothing to install*
- Appears as keyboard + mouse + media keys
- Volume and play/pause are media keys
- Scrolling is a mouse wheel; macros are key combinations

---

## Scene 5 — USB or Bluetooth, automatically (2:50–3:40)

**Narration**
Plug the knob into a computer and it uses USB. Power it from a phone charger or a power bank instead, and after three seconds without a computer it starts advertising over Bluetooth as "Input Knob". Pair it once from your computer's or phone's Bluetooth settings and it reconnects on its own. There's no switch to flip: the top of the screen just tells you whether you're on USB, Bluetooth, pairing, or not connected.

Under the hood, every action goes through one small router. The modes never know how the knob is connected; they just say "volume up", and the router sends it to USB if a computer is listening, or to Bluetooth otherwise.

**On screen**: `images/screen-bt-pairing.png` → `images/screen-bt-connected.png` → `images/diagram-link-states.png` → `images/code-05-router.png`.

**Slide**: *Connects itself*
- On a computer: USB
- On a charger: Bluetooth LE, named "Input Knob"
- Status always visible at the top of the screen
- One router, two transports sending identical HID reports

---

## Scene 6 — Inside the firmware (3:40–4:50)

**Narration**
The firmware is a single Arduino sketch, split into small modules. The knob itself is read with an interrupt: every time either encoder pin changes, a tiny handler looks up whether that was a step forward or back in a sixteen-entry table, which also filters out contact bounce. Four of those quarter-steps make one click you can feel. Because it runs on interrupts, fast turns aren't lost while the screen is redrawing.

The button has its own logic for click, double-click and long press. The long press fires while you're still holding, so switching modes feels instant.

The screen is drawn with LVGL, a popular embedded graphics library, on top of LovyanGFX, which drives the round display at 80 megahertz.

Follow one click of the knob: the interrupt counts it, the main loop picks it up, the Volume mode updates the screen and asks for "volume up", the router picks USB or Bluetooth, and the computer receives a media-key press and release.

**On screen**: `images/diagram-modules.png` → `images/code-02-encoder-isr.png` → `images/code-03-button-gestures.png` → `images/diagram-turn-sequence.png`.

**Slide**: *How a turn becomes volume*
1. Encoder interrupt counts quarter-steps
2. Main loop converts four into one detent
3. Current mode updates the screen and calls the HID router
4. Router sends a media key over USB or Bluetooth

---

## Scene 7 — What we learned debugging it (4:50–6:00)

**Narration**
Three real problems came up while building this, and each one taught something.

First, the screen said "USB not connected" even though the computer was clearly talking to it. The Arduino core starts USB before the program's setup code runs, so the "connected" event had already fired before anyone was listening. The fix was to ask the USB stack directly whether it's connected, instead of waiting for an event.

Second, after the Mac went to sleep and woke up, the knob went silent. The Mac still listed it, but the knob had lost its connection and never got it back. The firmware now watches for that: if USB stays disconnected for three seconds, it briefly drops off the bus and reconnects, which makes the computer set it up again. This fix is in the firmware but still needs testing against a real sleep cycle.

Third, after every firmware upload, the screen stayed dark until the knob was unplugged. The upload process reboots the chip with a "stay in download mode" flag set, and the version of the upload tool bundled with the Arduino core resets without clearing it. A small script, upload.sh, now clears that one bit after uploading, and the knob starts straight away.

**On screen**: `images/screen-no-connection.png` → `images/code-06-usb-watchdog.png` → `images/code-10-upload-fix.png`.

**Slide**: *Three bugs, three lessons*
- "USB not connected": the event fired before the program was listening → query state directly
- Silent after Mac sleep: device lost its connection → reconnect watchdog (untested on a real sleep)
- Dark screen after upload: a leftover download-mode flag → upload.sh clears it

---

## Scene 8 — Make it yours (6:00–6:40)

**Narration**
Customising is deliberately simple. Shortcuts are just a table: a name, the modifier keys, and one key. Add a line to the Mac table and the matching line to the PC table, and it shows up on the knob. Encoder direction, button timings, scroll speed and screen brightness are each one line in a config file. And adding a whole new mode — say, skipping tracks — is a few lines in the main sketch, because the router already handles USB and Bluetooth for you.

**On screen**: `images/code-08-macros.png` → `images/code-04-mode-actions.png`.

**Slide**: *Customise in a few lines*
- Shortcuts: one line each in `macros.h`
- Feel: `config.h` (direction, timing, scroll speed, brightness)
- New mode: add a case to three functions in `input-knob.ino`

---

## Scene 9 — Get started (6:40–7:15)

**Narration**
To build your own: install the Arduino IDE with the ESP32 board package, copy the exact LVGL and LovyanGFX versions that Elecrow ships, clone the repository, and run upload.sh with the knob plugged in. The first time, over the factory firmware, you may need to hold the BOOT button while plugging it in. A few seconds later the screen lights up with "Volume" in blue and "USB" in green, and turning the knob turns your volume.

**On screen**: `images/screen-volume-usb.png`.

**Slide**: *Build one*
1. Arduino IDE + esp32 board package 3.x
2. Copy Elecrow's `lvgl` 9.1.0, `LovyanGFX` 1.2.26 and `lv_conf.h`
3. `git clone github.com/bsommers/input-knob`
4. `./upload.sh`

---

## Scene 10 — Touch and light (7:15–7:50)

**Narration**
The round screen is also a touch panel, and the newest version uses it. A fourth mode, Trackpad, turns the screen into a tiny laptop trackpad: drag to move the cursor, tap to left-click, touch and hold to right-click, and turn the knob to scroll. In the other modes, a tap does the same as pressing the knob, a swipe left or right changes mode, and a swipe up or down scrolls a page or steps through the shortcut list. The five RGB LEDs glow in the current mode's colour, a brighter LED follows the knob like a needle, they flash red when you mute, and they breathe orange while Bluetooth is pairing. Input Knob is open source under the Apache 2.0 license, so the next feature could be yours.

**On screen**: `images/screen-overview.png`.

**Slide**: *Touch and light*
- Trackpad mode: drag, tap = left click, hold = right click, knob = scroll
- Tap = knob click; swipe left/right = change mode; swipe up/down = page / next shortcut
- LED ring: mode colour, turn needle, mute and macro flashes, pairing breathing
- Open source, Apache 2.0: github.com/bsommers/input-knob

---

## Key facts (for grounding — please keep these accurate)

- Hardware: Elecrow CrowPanel 1.28" HMI ESP32 Rotary Display; ESP32-S3, GC9A01 240×240 round IPS, CST816D touch, rotary encoder with push switch, 5× WS2812B LEDs on GPIO48.
- Software: Arduino (esp32 core 3.0.7 tested), LVGL 9.1.0, LovyanGFX 1.2.26, TinyUSB (USB HID), Bluedroid (Bluetooth LE HID). Flash use about 42% of a 3 MB app partition.
- Connection: USB whenever a computer has enumerated the knob; otherwise Bluetooth LE advertising as "Input Knob" after 3 s. One Bluetooth host at a time.
- Volume is relative (up/down steps); the knob cannot show the computer's actual volume level.
- The board has no battery; Bluetooth use needs a charger or power bank.
- Tested on hardware: USB volume/mute/play-pause on macOS; Bluetooth pairing and control with a Mac; upload.sh starting the firmware without a replug.
- Not yet tested: recovery after Mac sleep/wake, Windows, iOS/Android, the Mac/PC shortcut switch on a PC.
- Touch panel: Trackpad mode (drag, tap = left click, hold = right click) and tap/swipe gestures in other modes. LED ring: mode glow, turn needle, action flashes, pairing breathing.
- Screen images in this notebook are illustrations rendered from the firmware's UI layout, not photographs of the device.
- Do not state a price, a release date, or performance benchmarks; none are documented.
