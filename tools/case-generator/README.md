# Case Generator

Parametric laser-cut case designer. Defaults to an editable profile derived from the outer contour of the production **TŒRN M1 FRONT** panel.

## Run

```bash
cd tools
python3 -m http.server 8766
# open http://127.0.0.1:8766/case-generator/
```

Or from this folder: `npm start` (serves the `tools/` parent so `../tool-shell.css` resolves).

## Workflow

1. **Profile** — click-draw free outline (diagonals OK; **Shift** = H/V). Drag corners, click edge/**+** to add one point, **Delete** to remove. Profile is simplified on close/generate.
2. **Viewer** — orbit the finished plywood case with visible finger joints (opaque).
3. **Nest / Export** — generate and inspect all panels from the FRONT profile. Set **kerf**, export SVG.
4. **Features** — add circles / rounded rects on any panel. To reuse a PCB pattern, set a profile corner as the **template origin**, place holes across any ends/walls, then save and apply the named template.

## Tips

- The FRONT preset strips the individual production teeth but includes their 3 mm top/bottom depth in the sloped outer contour.
- Drawn profile = finished outside size. Joints are generated on an inset body so male tabs restore that envelope; gaps stay on the baseline (no inward “negative” notches).
- Colinear mid-edge points are removed automatically before finger generation.
- Hole templates use assembly X/Y/Z offsets from the selected profile origin and persist in project JSON plus the browser library.
- Save JSON to reopen a design later.
