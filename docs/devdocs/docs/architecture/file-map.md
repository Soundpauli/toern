---
sidebar_position: 2
title: File map
description: What each firmware file is responsible for.
---

# File map

Arduino builds every `.ino` in `platformio/` as one program. Treat the filenames as modules even though they share a global namespace.

## Sketch root: `platformio/`

| File | Responsibility |
|------|----------------|
| [`toern.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern.ino) | Version, includes, enums, `Mode`/`Note`/`Device`/`GlobalVars`, encoder + touch input, mode switching, playback timers, `setup`/`loop`, most glue |
| [`toern_ui.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_ui.ino) | LED matrix drawing: grid, cursor, status, BPM screen, overlays |
| [`toern_touchy.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_touchy.ino) | TOUCHY input mapping and flying cat display; reuses voice audition, pitch/effects, Fastrec, SMP and FILTER |
| [`toern_menu.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_menu.ino) | Nested menu UI (PATT / KIT / SMP / BPM / VOL / SETT / REC / MIDI / MODE / ETC), settings persistence hooks |
| [`toern_helpers.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_helpers.ino) | Pattern generation helpers, sample browser lists, misc utilities |
| [`toern_sample.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_sample.ino) | Sample preview, trim, load into voices, SD I/O yielding while audio runs |
| [`toern_synths.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_synths.ino) | Poly/mono synth voice allocation, presets, `playSound` / `stopSound` |
| [`toern_midi.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_midi.ino) | MIDI in/out, clock master/slave, transport, pulse-clock options |
| [`toern_filter.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_filter.ino) | Apply filter/FX graph parameters; smooth mixer gain transitions |
| [`toern_filterUI.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_filterUI.ino) | Filter-mode sliders on the matrix, encoder color cues |
| [`toern_parameters.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_parameters.ino) | Defaults and parameter setters (`setParams`, envelopes, synth presets) |
| [`toern_fileoperations.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_fileoperations.ino) | Pattern/project save & load on SD |
| [`toern_sd_serial.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_sd_serial.ino) | USB-serial SD file server (any screen; ETC → SD shows SCON) |

## Headers

| File | Responsibility |
|------|----------------|
| [`audioinit.h`](https://github.com/Soundpauli/toern/blob/main/platformio/audioinit.h) | Full Teensy Audio object graph + connections |
| [`notes.h`](https://github.com/Soundpauli/toern/blob/main/platformio/notes.h) | Pitch/frequency tables |
| [`colors.h`](https://github.com/Soundpauli/toern/blob/main/platformio/colors.h) | Channel / UI color definitions |
| [`font_3x5.h`](https://github.com/Soundpauli/toern/blob/main/platformio/font_3x5.h) | Tiny bitmap font for matrix text |
| [`icons.h`](https://github.com/Soundpauli/toern/blob/main/platformio/icons.h) | Icon bitmaps for menu/status |

## `platformio/src/` (custom audio and storage helpers)

| File | Responsibility |
|------|----------------|
| [`platformio/src/resamplerReader.h`](https://github.com/Soundpauli/toern/blob/main/platformio/src/resamplerReader.h) | Modified variable-rate sample playback (from teensy-variable-playback lineage) |
| [`platformio/src/effect_treeverb.*`](https://github.com/Soundpauli/toern/blob/main/platformio/src/effect_treeverb.h) | Active per-voice reverb; balanced six-comb/four-diffuser default |
| [`platformio/src/effect_freeverb_dmabuf.*`](https://github.com/Soundpauli/toern/blob/main/platformio/src/effect_freeverb_dmabuf.h) | Legacy Freeverb reference for host comparison tests; excluded from the firmware build |

## Other trees (not the firmware sketch)

| Path | Role |
|------|------|
| `docs/handbook/` | End-user guide (HTML); published at [handbook.toern.live](https://handbook.toern.live/) |
| `hardware/PCB/toern_revH/` | Current KiCad board — see [Hardware overview](../hardware/overview) |
| `tools/` | Host-side utilities — see [Tools overview](../tools/overview) |
| `docs/devdocs/` | **This** Docusaurus site (code + hardware + tools docs) → [docs.toern.live](https://docs.toern.live/) |
| `website/` | Project website (local / deploy-only; gitignored) |
| `_private/internal-docs/` | Scratch notes / plans (gitignored) |

## Size intuition

`toern.ino` is the largest file by far (~8k+ lines). Prefer adding focused logic to an existing `toern_*.ino` (or a new one) rather than growing the main file further when practical.

`platformio/src/pattern_transaction.h` provides destination-specific pattern commit, recovery and explicit slot deletion helpers. Tests for storage and MIDI live under `_debug/tests/`.
