#!/usr/bin/env bash
# Render assets/banner.png and assets/social-preview.png from assets/logo.svg
# with headless Chrome. Usage: bash assets/render-brand.sh
set -euo pipefail
ASSETS=$(cd "$(dirname "$0")" && pwd)
S=$(mktemp -d)
SVG=$(cat "$ASSETS/logo.svg")
render() {
  local NAME=$1 W=$2 H=$3 RADIUS=$4 LOGO=$5 H1=$6 P=$7 TAG=$8 SCALE=$9
  cat > "$S/$NAME.html" <<EOF
<!doctype html><html><head><meta charset="utf-8">
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@800;900&family=Inter:wght@500;600&display=swap" rel="stylesheet">
<style>
html,body{margin:0;width:${W}px;height:${H}px;overflow:hidden;background:transparent}
.card{box-sizing:border-box;width:${W}px;height:${H}px;border-radius:${RADIUS}px;
 background:radial-gradient(ellipse at 22% 50%,#1c2433 0%,#0b0e14 55%,#06080b 100%);
 display:flex;align-items:center;gap:56px;padding:0 80px;position:relative;overflow:hidden;font-family:Inter,sans-serif}
.card:before{content:"";position:absolute;inset:0;background:
 radial-gradient(circle at 22% 50%,rgba(155,89,182,.30),transparent 34%),
 radial-gradient(circle at 85% 120%,rgba(52,152,219,.20),transparent 42%);}
.logo{position:relative;width:${LOGO}px;flex:none}
.logo svg{width:100%;height:auto;display:block}
.text{position:relative}
h1{display:inline-block;margin:0;font-family:Orbitron,sans-serif;font-weight:900;font-size:${H1}px;letter-spacing:4px;line-height:1;
 background:linear-gradient(90deg,#3498DB 0%,#20C878 35%,#F39C12 70%,#9B59B6 100%);-webkit-background-clip:text;color:transparent;
 filter:drop-shadow(0 0 18px rgba(155,89,182,.35))}
p{margin:22px 0 0;color:#c9d1dc;font-size:${P}px;font-weight:500;line-height:1.35}
.tags{margin-top:26px;display:flex;gap:14px;flex-wrap:wrap}
.tags span{font-size:${TAG}px;font-weight:600;color:#e6ebf2;border:1.5px solid rgba(255,255,255,.18);
 background:rgba(255,255,255,.05);border-radius:999px;padding:8px 18px}
</style></head><body><div class="card">
<div class="logo">$SVG</div>
<div class="text"><h1>INPUT<br>KNOB</h1>
<p>Volume, scroll, shortcuts and a trackpad<br>on one round ESP32-S3 dial</p>
<div class="tags"><span>USB</span><span>Bluetooth LE</span><span>Touch</span><span>LVGL</span></div></div>
</div></body></html>
EOF
  "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" --headless=new --disable-gpu --hide-scrollbars \
    --default-background-color=00000000 --window-size=$W,$H --force-device-scale-factor=$SCALE \
    --virtual-time-budget=5000 --screenshot="$ASSETS/$NAME.png" "file://$S/$NAME.html" 2>/dev/null
}
render banner 1600 520 36 380 112 34 22 2
render social-preview 1280 640 0 400 92 29 21 1
