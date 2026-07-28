---
sidebar_position: 3
title: MIDI convert
description: MIDI2TŒRN — map MIDI files onto the sequencer grid and export pattern blobs.
---

# MIDI convert (MIDI2TŒRN)

**Live:** [https://toern.live/tools/convertmidi/](https://toern.live/tools/convertmidi/)  
**Source:** [`tools/convertmidi/`](https://github.com/Soundpauli/toern/tree/main/tools/convertmidi) (Vite + React app in `app/`)

## What it does

- Import a **`.mid`** file (`@tonejs/midi`)
- Map notes onto the **TŒRN grid** (pages × steps × rows / voices)
- Preview playback in the browser (Tone.js)
- Tune BPM, subdivision, track enable, overlap filtering, transpose / note priority
- Export a **`pattern_page_N.txt`** binary-ish blob matching the firmware’s note table layout (`channel`, `velocity`, …) for the current page

Voice / pitch mapping follows device conventions (documented in `app/src/utils/midiParser.ts` — e.g. voice 1 row relationships to MIDI note numbers).

## Code map

| Path | Role |
|------|------|
| `app/src/App.tsx` | Shell, upload, page state |
| `app/src/utils/midiParser.ts` | Parse MIDI → grid notes |
| `app/src/components/NoteGrid.tsx` | Grid UI + pattern export bytes |
| `app/src/components/PatternDownload.tsx` | Download control |
| `assets/` | Built static assets for the site path `/tools/convertmidi/` |

## Local development

```bash
cd tools/convertmidi/app
npm install
npm run dev
```

Production build (used by the website tool path):

```bash
cd tools/convertmidi/app
npm run build
```

`website/package.json` runs this as part of `build:convertmidi` when building the main site.
