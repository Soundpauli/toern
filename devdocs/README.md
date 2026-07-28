# TŒRN Code Docs

Developer documentation for the TŒRN firmware — how the code is structured and how the pieces talk to each other.

This is **not** the operator handbook (device usage). That lives in the repo-root [`handbook/`](../handbook/) tree (copied into `website/handbook/` for deploy, same pattern as this docs site).

## Layout

| Path | Role |
|------|------|
| `devdocs/` | This Docusaurus app (source of truth in the repo) |
| `website/docs/` | Built static files served at `/docs/` (copied in manually for deploy) |

Like `handbook/`, sources live at the repo root. Publish by building here and copying the output into `website/docs/` before deploy.

## Local preview

```bash
cd devdocs
npm install
npm start
```

Open the URL from the terminal (routes are under `/docs/…`).  
`baseUrl` is `/docs/` to match production: https://toern.live/docs/

## Production build (then copy into website)

```bash
cd devdocs
npm install
npm run build
# copy build/ → website/docs/  (manual, like handbook → website/handbook/)
```

Or from the website package (if present locally):

```bash
cd website
npm run build:docs   # uses scripts/sync-docs.js against ../devdocs
```

Details: [docs → Contributing → This docs site](./docs/contributing/docs-site.md).

## Docs sections

| Section | What it covers |
|---------|----------------|
| Architecture | Big picture, file map, main loop / ISR split |
| Core concepts | Modes, note/pattern data, channel map |
| Audio | Teensy Audio graph, samples, synths, FX |
| Subsystems | UI/LEDs, MIDI/clock, menu, SD |
| Tools | Sample converter, MIDI convert, SD tool, firmware loader, color editor |
| Hardware (rev G) | KiCad board map, connectors, power, fab exports |
| Contributing | Firmware build, memory tips, this docs site |

Edit Markdown under `docs/`. Sidebar order is in `sidebars.js`.
