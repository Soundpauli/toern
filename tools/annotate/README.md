# Image Annotate

Standalone tool for handbook-style callout images.

## Run

```bash
cd tools/annotate
python3 -m http.server 8765
# open http://127.0.0.1:8765/
```

Or open `index.html` via any static file server from this folder (needs `../tool-shell.css`).

## Workflow

1. **Open / drop** an image
2. **Image** — rotate, crop (handles + aspect presets), background fill
3. **Annotate** — click a target pixel, drag to place the label
4. **Select** — click / Shift·⌘-click / **Select all** (⌘A); drag target / elbow / label
5. **Align** — stack left/right, distribute Y, snap elbows (acts on selection)
6. **Line → label** — global: text flush on line, or line ends left/right of label with gap
7. **Style** — fonts, sizes, colors, line width, end-dot, text/line halo
8. **Export** — PNG (annotated), image only, or self-contained HTML; Save JSON to reopen

### Tips

- Style panel changes apply **live to all callouts** (font, size, colors, line mode)
- **Label on top of line** draws the leader under the text
- **Select all** is in the top bar (also ⌘A)
- Scroll to zoom; **Space**/Alt-drag to pan
- Crop/rotate keep callouts locked to image points
