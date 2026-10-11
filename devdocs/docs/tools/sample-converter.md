---
sidebar_position: 2
title: Sample converter
description: Browser tool that converts audio to 44.1 kHz mono WAV and ships default SD card content.
---

# Sample converter

**Live:** [https://audioconvert.tyng.app/](https://audioconvert.tyng.app/)
**Source:** [`standalone-tools/audio-converter-standalone/`](https://github.com/Soundpauli/toern/tree/main/standalone-tools/audio-converter-standalone)

## What it does

- Accepts a **folder** of files or a single **ZIP**
- Converts supported audio files to **44.1 kHz · mono · 16-bit PCM WAV** (TŒRN’s sample format)
- Runs **entirely in the browser** (ffmpeg.wasm) — files are not uploaded to a server
- Normalizes awkward characters in folder/file names to ASCII
- **SD CARD** section: download **`SD-CARD-CONTENT.zip`** — default packs, settings, and folder layout for a fresh microSD

## Use with the device

1. Convert your samples (or grab the default SD ZIP).
2. Extract converted samples into `samples/` or another browser folder. For a fresh card, extract the default SD ZIP directly into the card root. Numbered KIT folders use `1.wav` … `8.wav`.
3. Or push files while the card stays in the unit via the [SD tool](./sd-tool).

## Code map

| File / area | Role |
|-------------|------|
| `index.html` | UI + SD card instructions |
| `client.js` / `worker.js` | Conversion pipeline |
| `ffmpeg-core.*` | In-browser ffmpeg |
| `SD-CARD-CONTENT/` | Default card tree packaged for download |

## Local run

Serve the folder over HTTP (ffmpeg workers need a proper origin):

```bash
cd standalone-tools/audio-converter-standalone
python3 -m http.server 8080
# open http://127.0.0.1:8080/
```

There is also a Netlify-oriented path under `website/` for related convert-audio functions used by the main site — the standalone tree above is the self-contained public converter.

The converter does not automatically trim files to the handbook’s 12-second sample limit. Prepare the desired excerpt before conversion or trim it for the device. A conversion ZIP preserves the selected folder paths; it is not automatically a numbered KIT.

For packaging the default-card download locally, generate the archive from `SD-CARD-CONTENT/` (the WAV library is partly gitignored):

```bash
cd standalone-tools/audio-converter-standalone
python3 -c "import shutil; shutil.make_archive('SD-CARD-CONTENT', 'zip', 'SD-CARD-CONTENT')"
```

The zip root must contain the card files and numbered folders directly. Deploy it beside `index.html`. Theme source/sync is documented in [Tools overview](./overview).
