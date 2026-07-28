# Case Generator

Parametric laser-cut case designer. Defaults to an editable profile derived from the outer contour of the production **TŒRN M1 FRONT** panel.

> **Attention:** this is (still) only a demo. If you want to print your own case, use [`TOERN_M1_FINAL.dxf`](./TOERN_M1_FINAL.dxf) (kerf: ca. −0.125 mm) and adapt to your needs. Do not think the results will be 110% perfect ;)

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
4. **Features** — add circles / rounded rects on any panel. Set a profile corner as the **template origin** (FRONT default: bottom-left `(0,0)`), then apply the built-in **TŒRN revG holes** template (from `TOERN_M1_FINAL.dxf`, nudged toward `toern_revG`) or save your own.

## Tips

- The FRONT preset strips the individual production teeth but includes their 3 mm top/bottom depth in the sloped outer contour.
- Drawn profile = finished outside size. Joints are generated on an inset body so male tabs restore that envelope; gaps stay on the baseline (no inward “negative” notches).
- Colinear mid-edge points are removed automatically before finger generation.
- Hole templates use assembly X/Y/Z offsets from the selected profile origin and persist in project JSON plus the browser library.
- Save JSON to reopen a design later.
