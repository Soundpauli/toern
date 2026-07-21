# TŒRN Code Docs

Developer documentation for the TŒRN firmware — how the code is structured and how the pieces talk to each other.

This is **not** the operator handbook (device usage). That lives in [`../handbook/`](../handbook/) on the site, and in the repo-root `handbook/` tree.

## Layout (important for Netlify)

| Path | Role |
|------|------|
| `website/devdocs/` | This Docusaurus app (source of truth) |
| `website/docs/` | Built static files served at `/docs/` |
| `website/scripts/sync-docs.js` | `npm run build:docs` helper |

Netlify uses **`base = website`**, so the docs app must live **inside** `website/` (a repo-root `devdocs/` sibling is invisible to that build).

## Local preview

```bash
cd website/devdocs
npm install
npm start
```

Open the URL from the terminal (routes are under `/docs/…`).  
`baseUrl` is `/docs/` to match production: https://toern.live/docs/

## Production build

```bash
cd website
npm run build:docs   # or full site: npm run build
```

Details: [docs → Contributing → This docs site](./docs/contributing/docs-site.md).

## Docs sections

| Section | What it covers |
|---------|----------------|
| Architecture | Big picture, file map, main loop / ISR split |
| Core concepts | Modes, note/pattern data, channel map |
| Audio | Teensy Audio graph, samples, synths, FX |
| Subsystems | UI/LEDs, MIDI/clock, menu, SD |
| Hardware (rev G) | KiCad board map, connectors, power, fab exports |
| Contributing | Firmware build, memory tips, **this docs site** |

Edit Markdown under `docs/`. Sidebar order is in `sidebars.js`.
