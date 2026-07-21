# TŒRN Code Docs

Developer documentation for the TŒRN firmware — how the code is structured and how the pieces talk to each other.

This is **not** the operator handbook (device usage). That lives in [`../handbook/`](../handbook/).

## Local preview

```bash
cd devdocs
npm install
npm start
```

Open http://localhost:3000/docs/ (or the port printed in the terminal).  
`baseUrl` is `/docs/` so the site matches production paths.

## Production (toern.live/docs)

The main Netlify site (`website/`) builds these docs and publishes them at **`/docs/`**:

```bash
cd website
npm run build:docs   # or full: npm run build
```

That runs `devdocs` production build and copies output into `website/docs/`.

## Docs layout

| Section | What it covers |
|---------|----------------|
| Architecture | Big picture, file map, main loop / ISR split |
| Core concepts | Modes, note/pattern data, channel map |
| Audio | Teensy Audio graph, samples, synths, FX |
| Subsystems | UI/LEDs, MIDI/clock, menu, SD |
| Hardware (rev G) | KiCad board map, connectors, power, fab exports |
| Contributing | Build toolchain, RAM placement tips |

Edit Markdown under `docs/`. Sidebar order is in `sidebars.js`.
