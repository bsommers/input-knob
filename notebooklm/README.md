# NotebookLM sources

Sources for generating a video overview, slide deck, audio overview or other artifacts about Input Knob in [NotebookLM](https://notebooklm.google.com).

## Upload these

| Source | What it gives NotebookLM |
|---|---|
| `01-explainer-script.md` | Scene-by-scene narration, slide outlines and a key-facts list. **Upload first.** |
| `02-code-snippets.md` | Thirteen annotated code excerpts, copied verbatim from the firmware |
| `images/screen-*.png` (10) | Illustrations of the knob's screen in each mode and connection state |
| `images/diagram-*.png` (4) | System flow, USB/Bluetooth state machine, turn-to-host sequence, module graph |
| `images/code-*.png` (13) | Syntax-highlighted code, matching `02-code-snippets.md` |
| `../README.md`, `../docs/*.md` | Optional: full project docs for deeper answers |

Markdown files can be uploaded directly, or pasted as "Copied text". If your NotebookLM doesn't accept images as sources, keep them for editing the generated slides and video yourself.

## Suggested prompts

**Video overview** (Customize):
> Follow the scene order in "Input Knob: explainer script". Audience: makers and hobbyists who own or are considering the Elecrow CrowPanel 1.28" knob. Keep it practical and upbeat, about 7 minutes. Use the screen illustrations and diagrams as visuals. Stick to the "Key facts" section; describe untested features as untested.

**Slide deck**:
> Make a 10-slide deck using the "Slide" outline in each scene of the explainer script. One idea per slide, use the matching image from each scene.

**Audio overview**:
> A two-host conversation about building a USB and Bluetooth volume knob, focusing on the three debugging stories in scene 7 and how the USB/Bluetooth router works.

## Regenerating the images

The images are generated from the code, so they stay in sync after changes:

```bash
python3 notebooklm/render.py
```

Needs Google Chrome, Python 3 with Pillow and Pygments, and internet access (Google Fonts and the Mermaid CDN).
