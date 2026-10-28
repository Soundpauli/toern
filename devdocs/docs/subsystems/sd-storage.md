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

- Root `1.txt` … `999.txt`: slot A, using the original numbered filename (notes + sound settings + mutes + page arrangement).
- Root `1-B.txt` … `999-E.txt`: optional B–E companions, each a complete compatible pattern file. New firmware never writes `N-A.txt`.
- Root `autosaved.txt`: session recovery, independent of the last numbered slot.
- `0/`: scratch/custom samples; `1/` … `999/`: kits, each with `1.wav` … `8.wav`.
- `samples/`: browser library; it is not embedded in a pattern.
- `settings.txt`: device-preference backup; the filename does not make a binary pattern editable text.

Pattern saves validate `<destination>.tmp`, keep the previous generation as `<destination>.bak`, then install the new file. Recovery prefers a valid destination, otherwise a valid destination-specific temporary file or backup. The old shared `pattern.tmp` is not promoted because it has no slot identity. Loading stages the complete payload before changing notes/settings.

Saving an empty grid manually writes a valid blank slot and preserves all companions. Autosave preserves an empty session. The five-second cooldown is updated only after a successful autosave. The power switch simply cuts power; it does not trigger saving.

KIT writes check copied bytes and WAV headers and report success/failure. They are **not atomic across all eight files**. PATT save does not save sample audio, and PATT load does not reload the matching KIT.

## File subpattern persistence

Slot A always reads and saves `N.txt`; B–E use `N-B.txt` … `N-E.txt`. There is no family-mode flag, A-marker file or promotion. All A saves, including blank grids, preserve B–E companions. Companions belong directly to their number. The PATT browser position selects load, Save As and whole-family deletion targets. `SMP.file` and `subpatternFile` identify the loaded family; FN previews stay bound to it. An explicit successful load of another number opens A. Missing loads keep the loaded identity until the user accepts NEW.

`N-A.txt` is not part of the filename scheme. It is never opened, written, renamed, deleted or used as a fallback by pattern operations. Only the canonical selected destination and its own `.tmp`/`.bak` transaction files participate in PATT recovery.

Switching while playing never writes the SD card; paused switches save the outgoing slot and persist a new blank destination. Preview and staging read the selected slot without scanning siblings. Playback switches at the current playback-mode boundary; stopped playback switches on release. Empty saves never delete files in any save path. Removing a numbered family requires explicit deletion of every slot. Transaction cleanup only affects the saved destination’s own temporary and backup files.

Pending paused-switch saves survive transport restarts. `subpatternOldSavePending` owns the retained outgoing grid; `subpatternSaveNew` owns the new destination until it is persisted. Both block preview staging and letter selection until saving succeeds while stopped. Playback defers retries without clearing either flag or issuing SD writes. Regression tests verify the outgoing edits survive a failed save, resume, blocked selection and later pause, and separately cover a failed blank-destination save. Direct subpattern loading keeps its existing fast read behavior; no additional format-validation pass is introduced.

The unchanged note blob, `Device` payload and optional TPIT pitch extension are followed by an optional eight-byte `TSUB` session-identity tail: magic (4), version 1, slot index 0–4, a reserved byte and a deleted-slot mask. The previous family flag in byte 6 is ignored and written as zero. Byte 7 stores the deleted-slot mask in autosave, preserving explicit deletion across recovery; numbered files write it as zero. No `Device` fields were added or moved. Older files default to A; a stored slot past E is read as A. Older readers may ignore the tail. Autosave records the current slot identity so startup recovery resumes it.

Subpattern changes use a 21,930-byte RAM2 staging grid (including border cells), plus a one-page preview buffer, leaving PSRAM usage unchanged. `tests/test_file_subpatterns.py` compiles the actual firmware writer, loader, validator and slot code with a mock SD card under address/undefined-behavior sanitizers. It covers A–E families, no-save switches, deferred playback, the pre-release visible page, hasNotes, pitch/length metadata, SD failure, FILE re-entry and autosave identity. `tests/test_pattern_slots.cpp` checks filename/selection bounds and loop-boundary rules.

## Synth filter presets

`toern_fileoperations.ino` owns PSET files: `presets/<ch>/<nnn>.pset`, ch=11/13/14 and nnn=001…099. `saveVoicePreset()` and `loadVoicePreset()` handle that voice's filter/synth/parameter settings. Saving uses the pattern transaction helper with temporary/backup files and validation. Presets are separate from numbered patterns, sample kits and global preferences. Keep each voice's selected slot independent.

Sample V gain remains part of the existing binary pattern layout; its tagged spare-slot representation does not change `sizeof(Device)`. See [firmware edits](../contributing/firmware-edits#sample-voice-gain).


PATT indexes canonical root filenames once on entry or a storage revision (1,000-byte RAM2 mask table). Detents read this index without SD access. Green dots count selected-family saved slots; red dots preview the loaded family (including its active RAM slot) for Save As into an empty selected number. Dots start two pixels right of the folder (one blank column), with one dark column between dots. FN+encoder 2 opens DEL? / NO for the selected whole A–E family; confirmed deletion removes A last and reports cleanup failures. Only deletion of the loaded family clears live notes. Save As copies siblings with a 512-byte buffer and writes the active slot from RAM; it refuses destinations with existing transaction files, rolls back newly created target files after failure, and adopts the target only after success. Occupied selections enter PATTERN_CONFIRM (SAVE? / NO) before Save As overwrites the selected family.

PATTERN_CONFIRM snapshots the selected target and defaults to NO (cancel). Encoder 4 selects NO/YES; encoder 4 short press executes. DEL?/SAVE? remains red. NO text, encoder 4 ring and indicator are blue; YES uses green. Touch 2 cancels. No grid cursor is drawn on this screen. Cancellation calls the general `showCanceledAnimation()` blue neutral-face reveal/fade and returns to the originating browser. Occupied-family Save As stages every source letter before rotating any destination primary into its .bak file; install failure restores primaries already moved and keeps the live source identity. After success, target-only letters and their recovery files are removed. Same-family Save confirms and updates the active letter while preserving its siblings. Family replacement is not atomic across a power failure.

PATT sideload `pos[2]` is an opt-in keep-current-sounds toggle: 0=icon OFF, `SMP_LOAD_SETTINGS=true`; 1=red triangle ON, `SMP_LOAD_SETTINGS=false`. It resets to 0 on every PATT entry and number change, including input polling before a same-frame Load click. PATT FN shows only the delete cross (no encoder-2 matrix indicator). Dots moved right one pixel. Confirmation headings start at x=2, y=11; NO/YES starts at x=2, y=5. PATT and KIT numbers use y=11. SAVE? uses compact S/E/? glyphs on 16-column panels; target numbers remain on wide panels only.

Sample browser uses FN+encoder 3 for confirmed deletion; KIT uses FN+encoder 2 with the same cross-only indicator as PATT. Only file rows snapshot a sample path, rejecting folders and truncated paths; the backend removes that exact WAV. KIT snapshots the selected number and removes its eight voice WAVs, then the directory if empty. Both stop SD preview, yield around SD I/O, invalidate browser lists and propagate removal failures. Confirmation is shared: NO default, encoder 4 selects, encoder 4 executes, cancel returns to the originating browser with the generic blue neutral-face animation. Existing RAM audio stays loaded.

Confirmation keeps its origin mode and browser positions. Delete completion/failure and all cancellation return there; a deleted sample selection is clamped to the refreshed list. Successful overwrite still returns to DRAW. Only encoder 4 press executes NO/YES; encoder 2 is ignored and its confirmation ring/indicator are off.
