#!/usr/bin/env python3
"""Render NotebookLM source images for Input Knob.

Builds HTML pages for code snippets, diagrams and illustrations of the round
screen, screenshots them with headless Chrome and trims the margins.

Usage: python3 notebooklm/render.py   (from the repo root or anywhere)
"""
import html
import math
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageChops
from pygments import highlight
from pygments.formatters import HtmlFormatter
from pygments.lexers import get_lexer_for_filename

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
OUT = HERE / "images"
CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
BACKGROUND = (15, 17, 21)
FONTS = (
    '<link href="https://fonts.googleapis.com/css2?family=Montserrat:wght@500;600'
    '&family=JetBrains+Mono&family=Inter:wght@400;600&display=swap" rel="stylesheet">'
)

# (slug, file, first line, last line, title)
SNIPPETS = [
    ("01-pin-map", "board.h", 7, 32, "The hardware pin map"),
    ("02-encoder-isr", "knob.cpp", 9, 48, "Encoder: quadrature decoding in an interrupt"),
    ("03-button-gestures", "knob.cpp", 50, 92, "Button: click, double-click and long press"),
    ("04-mode-actions", "input-knob.ino", 83, 104, "What a turn does in each mode"),
    ("05-router", "hid.cpp", 13, 50, "Routing every action to USB or Bluetooth"),
    ("06-usb-watchdog", "usb_hid.cpp", 36, 65, "Recovering USB after the computer sleeps"),
    ("07-ble-report-map", "ble_hid.cpp", 13, 41, "Bluetooth HID report map: keyboard, mouse, media keys"),
    ("08-macros", "macros.h", 9, 38, "Mac and PC shortcut tables"),
    ("09-display-config", "display.cpp", 10, 42, "Driving the round GC9A01 display"),
    ("10-upload-fix", "upload.sh", 1, 49, "upload.sh: no more dark screen after flashing"),
    ("11-touch-gestures", "touch.cpp", 59, 100, "Touch: taps, holds and swipes from raw coordinates"),
    ("12-trackpad", "input-knob.ino", 152, 167, "Trackpad mode: cursor, left and right click"),
    ("13-led-ring", "leds.cpp", 62, 86, "LED ring: flashes, pairing breath, mode glow and needle"),
]

NOTES = {
    "01-pin-map": "Every GPIO the firmware touches, taken from Elecrow's examples and schematic. GPIO1 and GPIO2 must be driven high or the LCD has no power. The encoder is on 45/42 and its push switch on 41 (active low).",
    "02-encoder-isr": "Each change on either encoder pin runs a tiny interrupt handler. It combines the previous and current A/B levels into a 4-bit index and looks up +1, -1 or 0 in a 16-entry table, which rejects contact bounce. Four quarter-steps make one detent, the click you feel. Because it's interrupt-driven, fast turns aren't lost while the screen redraws.",
    "03-button-gestures": "The button is polled with a 25 ms debounce. A long press (600 ms) fires while you're still holding, so mode changes feel immediate. A single click is only reported after the 300 ms double-click window closes; that small delay is the price of having double-click at all.",
    "04-mode-actions": "The whole personality of the knob: one switch on the current mode. Volume sends media keys, Scroll sends mouse-wheel steps (clockwise scrolls down), Macro moves through the shortcut list. The dial ring and labels update on every turn.",
    "05-router": "Modes never know how the knob is connected. Every action goes through this router: USB if a computer has enumerated the knob, otherwise Bluetooth if a device is paired, otherwise nothing. service() turns Bluetooth advertising on after 3 seconds without USB, and off as soon as USB is back.",
    "06-usb-watchdog": "Found while debugging: after a Mac slept and woke, the knob was left unmounted while macOS still listed it, so nothing got through. This watchdog notices, drops off the USB bus for 200 ms and reconnects, forcing a fresh enumeration. It's non-blocking and retries every 30 s so it never stalls Bluetooth use on a power bank. (Implemented; not yet verified against a real sleep cycle.)",
    "07-ble-report-map": "The Bluetooth HID report map: one descriptor that declares three devices (a keyboard, a mouse with vertical and horizontal wheels, and a consumer-control device for media keys). Hosts read this once at pairing and then accept raw reports, the same bytes the USB side sends.",
    "08-macros": "Shortcuts are plain data: a name, modifier keys and one key code. There's a Mac table and a PC table of the same length (a static_assert enforces it), and a double-click in Macro mode switches between them. The choice is saved in flash.",
    "09-display-config": "LovyanGFX drives the 240x240 round GC9A01 over SPI at 80 MHz with colour inversion on, matching Elecrow's working example. LVGL 9 draws into two 40-line buffers in PSRAM and this driver pushes them to the panel.",
    "11-touch-gestures": "The touch chip can recognise gestures itself, but this code works from raw coordinates every 10 ms so touch uses the same timings as the knob. Barely moving is a tap, or a long press after 600 ms (fired while you hold). Travelling 50 px along one axis is a swipe. Every poll also reports how far the finger moved, which drives the trackpad.",
    "12-trackpad": "Trackpad mode in a few lines: finger movement becomes cursor movement, with acceleration so slow drags are precise and fast flicks cross the screen. A tap is a left click and a touch-and-hold is a right click. Turning the knob scrolls.",
    "13-led-ring": "Each frame (about 50 per second) the LED ring picks one effect: an action flash fading over 300 ms wins; otherwise orange breathing while Bluetooth is pairing; otherwise a dim glow in the mode colour, with one brighter 'needle' LED that steps with each click of the knob and fades back.",
    "10-upload-fix": "Uploading over the knob's own USB port reboots the ESP32-S3 with a 'force download boot' flag set. The bundled esptool 4.6 resets without clearing it, so the knob came back with a dark screen until it was unplugged. This script uploads, then writes 0 to that one register bit and resets, so the new firmware starts immediately.",
}

# (slug, Mermaid source, title)
DIAGRAMS = [
    ("diagram-system", """flowchart LR
    User([You]) -->|turn / click| Knob[Encoder + button]
    Knob --> Modes[Mode logic]
    Modes --> Screen[Round LVGL screen]
    Modes --> Router{USB mounted?}
    Router -->|yes| USB[USB HID] --> PC([Computer])
    Router -->|no| BLE[Bluetooth LE HID] --> Any([Computer, phone, tablet])""",
     "How Input Knob works"),
    ("diagram-link-states", """stateDiagram-v2
    [*] --> UsbWait: boot
    UsbWait --> Usb: host enumerates
    UsbWait --> Advertising: USB unmounted 3 s
    Usb --> UsbLost: unmounted (unplug, host sleep)
    UsbLost --> Usb: re-enumerated
    UsbLost --> Advertising: still unmounted after 3 s
    Advertising --> Bluetooth: host connects
    Bluetooth --> Advertising: host disconnects
    Advertising --> Usb: host enumerates
    Bluetooth --> Usb: host enumerates""",
     "Choosing USB or Bluetooth automatically"),
    ("diagram-turn-sequence", """sequenceDiagram
    participant E as Encoder ISR
    participant L as loop()
    participant A as onTurn()
    participant H as hid router
    participant T as USB or BLE
    participant C as Computer
    E->>E: count quarter-steps
    L->>L: knobTakeDetents()
    L->>A: +1 detent
    A->>A: screen shows Vol +
    A->>H: hid::volume(1)
    H->>T: consumer(VOLUME_UP)
    T->>C: HID press + release""",
     "One click of the knob, from pin to computer"),
    ("diagram-modules", """graph TD
    INO[input-knob.ino] --> KNOB[knob.cpp]
    INO --> UI[ui.cpp]
    INO --> DISP[display.cpp]
    INO --> HID[hid.cpp router]
    INO --> PREF[(Preferences / NVS)]
    UI --> LVGL[[LVGL 9.1.0]]
    DISP --> LVGL
    DISP --> LGFX[[LovyanGFX 1.2.26]]
    HID --> USBH[usb_hid.cpp]
    HID --> BLEH[ble_hid.cpp]
    USBH --> TUSB[[TinyUSB]]
    BLEH --> BD[[Bluedroid BLE]]""",
     "Firmware modules"),
]

BLUE, GREEN, ORANGE, RED = "#3498DB", "#20C878", "#F39C12", "#E74C3C"
LINKS = {
    "usb": ("USB", GREEN),
    "bt": ("Bluetooth", BLUE),
    "pairing": ("Bluetooth pairing...", ORANGE),
    "none": ("No connection", RED),
}
# (slug, link, mode, mode colour, value, hint, dial detents from 12 o'clock, caption)
SCREENS = [
    ("screen-volume-usb", "usb", "Volume", BLUE, "Vol +", "click mute | 2x play", 2, "Volume mode over USB, after turning clockwise"),
    ("screen-volume-mute", "usb", "Volume", BLUE, "Mute", "click mute | 2x play", 2, "A click mutes the computer"),
    ("screen-scroll", "usb", "Scroll", GREEN, "Vertical", "click middle | 2x axis", -3, "Scroll mode: the knob is a scroll wheel"),
    ("screen-macro-mac", "usb", "Macro (Mac)", ORANGE, "Copy", "click run | 2x Mac/PC", 5, "Macro mode with Mac shortcuts"),
    ("screen-macro-pc", "usb", "Macro (PC)", ORANGE, "Task View", "click run | 2x Mac/PC", 8, "Double-click switches to PC shortcuts"),
    ("screen-bt-pairing", "pairing", "Volume", BLUE, "Turn me", "click mute | 2x play", 0, "On a charger: advertising over Bluetooth"),
    ("screen-bt-connected", "bt", "Volume", BLUE, "Vol -", "click mute | 2x play", -2, "Paired and working over Bluetooth"),
    ("screen-trackpad", "usb", "Trackpad", "#9B59B6", "Drag here", "tap click | hold right", 0, "Trackpad mode: drag to move, tap to click"),
    ("screen-no-connection", "none", "Volume", BLUE, "Turn me", "click mute | 2x play", 0, "No USB host and Bluetooth not connected"),
]


def page(body, css=""):
    return f"""<!doctype html><html><head><meta charset="utf-8">{FONTS}
<style>
html, body {{ margin: 0; background: rgb{BACKGROUND}; color: #e6e6e6; font-family: Inter, Helvetica, sans-serif; }}
.wrap {{ padding: 48px; display: inline-block; }}
.title {{ font-size: 30px; font-weight: 600; margin: 0 0 6px; }}
.sub {{ font-size: 18px; color: #8b93a1; margin: 0 0 24px; }}
{css}
</style></head><body><div class="wrap">{body}</div></body></html>"""


def shoot(html_text, name, width=1700, height=2600, wait_ms=4000):
    with tempfile.NamedTemporaryFile("w", suffix=".html", delete=False) as f:
        f.write(html_text)
        src = f.name
    out = OUT / f"{name}.png"
    subprocess.run(
        [CHROME, "--headless=new", "--disable-gpu", "--hide-scrollbars",
         "--force-device-scale-factor=2", f"--window-size={width},{height}",
         f"--virtual-time-budget={wait_ms}", f"--screenshot={out}", f"file://{src}"],
        check=True, capture_output=True,
    )
    trim(out)
    Path(src).unlink()
    print("wrote", out.relative_to(REPO))


def trim(path, pad=48):
    image = Image.open(path).convert("RGB")
    diff = ImageChops.difference(image, Image.new("RGB", image.size, BACKGROUND))
    box = diff.getbbox()
    if box:
        left, top, right, bottom = box
        image = image.crop((max(left - pad, 0), max(top - pad, 0),
                            min(right + pad, image.width), min(bottom + pad, image.height)))
    image.save(path)


def render_snippets():
    formatter = HtmlFormatter(style="monokai", linenos="inline", noclasses=True)
    for slug, file, first, last, title in SNIPPETS:
        lines = (REPO / file).read_text().splitlines()[first - 1:last]
        formatter.linenostart = first
        code = highlight("\n".join(lines), get_lexer_for_filename(file), formatter)
        body = f"""<p class="title">{html.escape(title)}</p>
<p class="sub">{file} · lines {first}–{last} · github.com/bsommers/input-knob</p>
<div class="code">{code}</div>"""
        css = """.code { background:#272822; border-radius:14px; padding:22px 26px; }
.code pre { font-family:'JetBrains Mono', Menlo, monospace; font-size:19px; line-height:1.45; margin:0; }
.code .linenos { color:#75715e !important; background:none !important; padding-right:18px; }"""
        shoot(page(body, css), f"code-{slug}")


def write_snippets_doc():
    fence = {"sh": "bash", "ino": "cpp", "cpp": "cpp", "h": "cpp"}
    parts = [
        "# Input Knob: annotated code snippets\n",
        "Excerpts from the Input Knob firmware (github.com/bsommers/input-knob), copied verbatim with line numbers. "
        "Each has a matching image in `images/code-*.png`.\n",
    ]
    for number, (slug, file, first, last, title) in enumerate(SNIPPETS, 1):
        lines = (REPO / file).read_text().splitlines()[first - 1:last]
        language = fence[file.rsplit(".", 1)[1]]
        parts.append(f"## {number}. {title}\n\n`{file}`, lines {first}–{last} · image: `images/code-{slug}.png`\n\n"
                     f"{NOTES[slug]}\n\n```{language}\n" + "\n".join(lines) + "\n```\n")
    (HERE / "02-code-snippets.md").write_text("\n".join(parts))
    print("wrote", (HERE / "02-code-snippets.md").relative_to(REPO))


def render_diagrams():
    for slug, source, title in DIAGRAMS:
        body = f"""<p class="title">{html.escape(title)}</p>
<p class="sub">Input Knob firmware · github.com/bsommers/input-knob</p>
<div class="card"><pre class="mermaid">{html.escape(source)}</pre></div>
<script src="https://cdn.jsdelivr.net/npm/mermaid@10.9.1/dist/mermaid.min.js"></script>
<script>mermaid.initialize({{ startOnLoad: true, theme: 'dark', themeVariables: {{ fontSize: '20px' }} }});</script>"""
        css = ".card { background:#1b1f27; border-radius:14px; padding:32px; min-width:900px; }"
        shoot(page(body, css), slug, wait_ms=8000)


def arc_path(cx, cy, r, start_deg, end_deg):
    # LVGL arc angles: 0 = 3 o'clock, increasing clockwise (screen y points down).
    s, e = math.radians(start_deg), math.radians(end_deg)
    x1, y1 = cx + r * math.cos(s), cy + r * math.sin(s)
    x2, y2 = cx + r * math.cos(e), cy + r * math.sin(e)
    large = 1 if (end_deg - start_deg) % 360 > 180 else 0
    return f"M {x1:.2f} {y1:.2f} A {r} {r} 0 {large} 1 {x2:.2f} {y2:.2f}"


def screen_svg(link, mode, colour, value, hint, detents):
    # Coordinates follow ui.cpp on the 240x240 panel.
    link_text, link_colour = LINKS[link]
    angle = (270 + detents * 15) % 360
    ring = arc_path(120, 120, 111, angle, angle + 30)
    return f"""<svg viewBox="0 0 240 240" width="480" height="480">
  <defs><clipPath id="round"><circle cx="120" cy="120" r="120"/></clipPath></defs>
  <g clip-path="url(#round)">
    <rect width="240" height="240" fill="#000"/>
    <circle cx="120" cy="120" r="111" fill="none" stroke="#202020" stroke-width="10"/>
    <path d="{ring}" fill="none" stroke="{colour}" stroke-width="10" stroke-linecap="round"/>
    <text x="120" y="40" fill="{link_colour}" font-size="12" text-anchor="middle" font-family="Montserrat">{html.escape(link_text)}</text>
    <text x="120" y="70" fill="{colour}" font-size="20" text-anchor="middle" font-family="Montserrat" font-weight="500">{html.escape(mode)}</text>
    <text x="120" y="130" fill="#fff" font-size="28" text-anchor="middle" font-family="Montserrat" font-weight="500">{html.escape(value)}</text>
    <text x="120" y="190" fill="#909090" font-size="14" text-anchor="middle" font-family="Montserrat">{html.escape(hint)}</text>
  </g>
</svg>"""


def render_screens():
    css = """.bezel { display:inline-block; padding:22px; border-radius:50%;
  background: radial-gradient(circle at 30% 25%, #e9ecef, #9aa1a9 55%, #6c737b); }
.screen { border-radius:50%; overflow:hidden; display:block; }
.note { font-size:15px; color:#8b93a1; margin-top:20px; }
.grid { display:grid; grid-template-columns:repeat(4, auto); gap:40px; }
.cell p { text-align:center; font-size:16px; color:#c9ced6; margin:12px 0 0; max-width:300px; }
.grid .bezel { padding:12px; } .grid svg { width:260px; height:260px; }"""
    note = '<p class="note">Illustration of the knob\'s screen, drawn from the layout in ui.cpp. Not a photo.</p>'
    cells = []
    for slug, link, mode, colour, value, hint, detents, caption in SCREENS:
        svg = screen_svg(link, mode, colour, value, hint, detents)
        body = f"""<p class="title">{html.escape(caption)}</p>
<div class="bezel"><div class="screen">{svg}</div></div>{note}"""
        shoot(page(body, css), slug)
        cells.append(f'<div class="cell"><div class="bezel"><div class="screen">{svg}</div></div>'
                     f"<p>{html.escape(caption)}</p></div>")
    body = f"""<p class="title">Input Knob screens</p>
<p class="sub">Every mode and connection state</p><div class="grid">{''.join(cells)}</div>{note}"""
    shoot(page(body, css), "screen-overview", width=1500)


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True)
    write_snippets_doc()
    render_snippets()
    render_diagrams()
    render_screens()
