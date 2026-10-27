---
sidebar_position: 4
title: SD & storage
description: Pattern files, samplepacks, and the serial SD server.
---

# SD & storage

The Micro SD card holds samples, patterns, and settings. Firmware never exposes USB mass storage; file transfer uses a **serial protocol** while the SD menu page is open.

## Firmware pieces

| File | Role |
|------|------|
| `toern_fileoperations.ino` | Pattern/project load & save, autosave integration |
| `toern_sample.ino` | Sample file reads, preview, audio-safe yields |
| `toern_sd_serial.ino` | USB serial file server (`list` / `put` / `get` / …) |
| `toern_helpers.ino` | Browse list building, path helpers |

## Host tools

- Browser: [sdtool.tyng.app](https://sdtool.tyng.app) (Web Serial)
- CLI and local browser server: `standalone-tools/sd-tool-standalone/toern_sd.py`

Protocol details belong with those tools; the device side is `sdSerialServerPollNeedSdHint()` whenever a host program has the port open.

## Rules of thumb

- Don’t mount/unmount the card while powered if the firmware assumes a stable volume — users are told to insert/eject powered off.
- Autosave and settings backup should stay in `loop()`, often deferred after UI.
- During SD I/O with audio running, use the `sdIo*` helpers so the codec keeps ticking.

## Card layout and save semantics

- Root `1.txt` … `999.txt`: binary patterns (notes + sound settings + mutes + page arrangement).
- Root `1-A.txt` … `999-E.txt`: optional A–E subpattern families, each a complete compatible pattern file.
- Root `autosaved.txt`: session recovery, independent of the last numbered slot.
- `0/`: scratch/custom samples; `1/` … `999/`: kits, each with `1.wav` … `8.wav`.
- `samples/`: browser library; it is not embedded in a pattern.
- `settings.txt`: device-preference backup; the filename does not make a binary pattern editable text.

Pattern saves validate `<destination>.tmp`, keep the previous generation as `<destination>.bak`, then install the new file. Recovery prefers a valid destination, otherwise a valid destination-specific temporary file or backup. The old shared `pattern.tmp` is not promoted because it has no slot identity. Loading stages the complete payload before changing notes/settings.

Saving an empty grid manually clears its numbered PATT slot and recovery companions. Autosave preserves an empty session. The five-second cooldown is updated only after a successful autosave. The power switch simply cuts power; it does not trigger saving.

KIT writes check copied bytes and WAV headers and report success/failure. They are **not atomic across all eight files**. PATT save does not save sample audio, and PATT load does not reload the matching KIT.

## File subpattern persistence

A pattern remains `N.txt` until a lettered slot is saved. Switching subpatterns while playing never writes to the SD card; switching while paused saves the outgoing slot as `N-<slot>.txt` (and promotes `N.txt` to `N-A.txt`). While playing, slot A falls back to reading `N.txt` until `N-A.txt` is saved. Other slots use `N-B.txt` … `N-E.txt`. While FN is held, only the on-screen page of the chosen letter is read, straight from that one file. The rest of the file is read after the letter settles, still without checking siblings, temps, or backups. Playback swaps at the PMOD:NEXT boundary; stopped playback swaps on release. PATT resolves the selected slot for the current family, or A when opening another family. PATT Save writes the selected slot. A manual empty PATT Save deletes that slot; automatic paused-switch saves preserve blank grids as real files. ETC → RSET → FILE removes lettered slots and their transaction companions as well as legacy files.

The unchanged note blob, `Device` payload and optional TPIT pitch extension are followed by an optional eight-byte `TSUB` session-identity tail: magic (4), version 1, slot index 0–4, lettered-family flag, reserved byte. No `Device` fields were added or moved. Older files default to A; a stored slot past E is read as A. Older readers may ignore the tail. Autosave records the current slot identity so startup recovery resumes it.

Subpattern changes use a 21,930-byte RAM2 staging grid (including border cells), plus a one-page preview buffer, leaving PSRAM usage unchanged. `tests/test_file_subpatterns.py` compiles the actual firmware writer, loader, validator and slot code with a mock SD card under address/undefined-behavior sanitizers. It covers A–E families, no-save switches, deferred playback, the pre-release visible page, hasNotes, pitch/length metadata, SD failure, FILE re-entry and autosave identity. `tests/test_pattern_slots.cpp` checks filename/selection bounds and loop-boundary rules.

## Synth filter presets

`toern_fileoperations.ino` owns PSET files: `presets/<ch>/<nnn>.pset`, ch=11/13/14 and nnn=001…099. `saveVoicePreset()` and `loadVoicePreset()` handle that voice's filter/synth/parameter settings. Saving uses the pattern transaction helper with temporary/backup files and validation. Presets are separate from numbered patterns, sample kits and global preferences. Keep each voice's selected slot independent.

Sample V gain remains part of the existing binary pattern layout; its tagged spare-slot representation does not change `sizeof(Device)`. See [firmware edits](../contributing/firmware-edits#sample-voice-gain).
