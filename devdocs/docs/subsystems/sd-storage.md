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
- Root `autosaved.txt`: session recovery, independent of the last numbered slot.
- `0/`: scratch/custom samples; `1/` … `999/`: kits, each with `1.wav` … `8.wav`.
- `samples/`: browser library; it is not embedded in a pattern.
- `settings.txt`: device-preference backup; the filename does not make a binary pattern editable text.

Pattern saves validate `<destination>.tmp`, keep the previous generation as `<destination>.bak`, then install the new file. Recovery prefers a valid destination, otherwise a valid destination-specific temporary file or backup. The old shared `pattern.tmp` is not promoted because it has no slot identity. Loading stages the complete payload before changing notes/settings.

Saving an empty grid manually writes a valid empty numbered pattern; deletion is an explicit confirmed action. Autosave preserves an empty session. The five-second cooldown is updated only after a successful autosave. The power switch simply cuts power; it does not trigger saving.

KIT writes check copied bytes and WAV headers and report success/failure. They are **not atomic across all eight files**. PATT save does not save sample audio, and PATT load does not reload the matching KIT.

## Synth filter presets

`toern_fileoperations.ino` owns PSET files: `presets/<ch>/<nnn>.pset`, ch=11/13/14 and nnn=001…099. `saveVoicePreset()` and `loadVoicePreset()` handle that voice's filter/synth/parameter settings. Saving uses the pattern transaction helper with temporary/backup files and validation. Presets are separate from numbered patterns, sample kits and global preferences. Keep each voice's selected slot independent.

Sample V gain remains part of the existing binary pattern layout; its tagged spare-slot representation does not change `sizeof(Device)`. See [firmware edits](../contributing/firmware-edits#sample-voice-gain).

### Sound settings and sideload

PATT entry and each file-selection change default to loading saved sound settings (`SMP_LOAD_SETTINGS=true`, sideload encoder position 0). Turning encoder 3 clockwise enables sideload for that selection (position 1, red X), retaining current samples, effects, envelopes, synth settings and volumes in RAM and live audio. Counterclockwise disables sideload. Saving always captures current sound settings. Renderer-cache invalidation does not itself clear sideload.

### Browser confirmations

`toern_storage_confirm.ino` implements PATT/KIT overwrite and PATT/KIT/SMP delete confirmations independently of subpatterns. NO is the initial choice; only encoder 4 accepts. Touch 2 cancels with a blue neutral-face animation. The action snapshots the destination and browser positions before the confirmation encoder is reused. SMP captures a bounded WAV path and rejects directories or traversal; KIT checks all eight WAV slots for occupancy. PATT deletion removes the numbered file and its `.tmp`/`.bak` generations, clearing live notes only if that number was loaded. Browser selection and SMP Seek End are restored after cancellation/deletion. The sample browser's physical encoder 2 maps to logical slot 2 when FN counters are restored.

AUTOSAVE is shown in full (AUTO/SAVE on 16 columns) and remains load-only in PATT. Recording and fast capture block confirmation actions. The simulator models deletion of bundled read-only assets for its current session; it does not modify the packaged files.
