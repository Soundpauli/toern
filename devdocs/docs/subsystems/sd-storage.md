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

PATT entry and each file-selection change default to loading saved sound settings (`SMP_LOAD_SETTINGS=true`, sideload encoder position 0). Turning encoder 3 clockwise enables sideload for that selection (position 1, red triangle), retaining current samples, effects, envelopes, synth settings and volumes in RAM and live audio. Counterclockwise disables sideload. Saving always captures current sound settings. Renderer-cache invalidation does not itself clear sideload.

### Browser confirmations

`toern_pattern_slots.ino` renders PATT/KIT overwrite and PATT/KIT/SMP delete confirmations; `toern_pattern_slots.ino` schedules the cooperative family jobs; `toern_fileoperations.ino` retains ordinary load/autosave and sample/KIT actions. NO is the initial choice; only encoder 4 accepts. Touch 2 cancels with a blue neutral-face animation. The action snapshots the destination and browser positions before the confirmation encoder is reused. SMP captures a bounded WAV path and rejects directories or traversal; KIT checks all eight WAV slots for occupancy. PATT deletion removes the entire A–E family and its `.tmp`/`.bak` generations, clearing live notes only if that family was loaded. A is removed last so failed companion cleanup retains the base file. Browser selection and SMP Seek End are restored after cancellation/deletion. The sample browser's physical encoder 2 maps to logical slot 2 when FN counters are restored.

AUTOSAVE is shown in full (AUTO/SAVE on 16 columns) and remains load-only in PATT. Recording and fast capture block confirmation actions. The simulator models deletion of bundled read-only assets for its current session; it does not modify the packaged files.

### Loaded file and browser selection

PATT selection lives in `loadSaveTrack.pos[3]`; browsing never changes `SMP.file`.
Load and Save As receive explicit destinations. Successful numbered loads use the
selected number even if the file contains an older embedded identity. Save As
serializes the destination identity and adopts it only after the transaction
commits; failed writes leave the live identity unchanged. Autosave retains the
live identity. Selecting an empty destination defers its identity until NEW is
actually generated.

The PATT browser indexes canonical A–E family filenames one directory entry per loop, once per visit
or storage revision. Rendering only consults the cache. AUTOSAVE availability is checked separately. Slot turns and slow SD/display frames reuse that index.
Saves, deletes and host SD mutations invalidate it; elapsed time does not.

### A–E file subpatterns

Enable `SETT → SUBP` (persistent, OFF by default). In DRAW, FN + encoder 2
selects A–E without wrapping; FN + encoder 3 previews the populated pages of
the selected letter. Sounds and settings stay live. The centered letter/page
label is green for existing data and blue for an empty slot; another slot's
preview uses a yellow playhead.

Releasing FN requests the selection. Once validation and metadata preparation
finish, stopped playback installs it; playing playback waits for the PMOD:NEXT
boundary. Missing a boundary defers the switch. The pending letter/page overlay
remains visible while waiting. VMOD previews use the selected bank rather than
the live voice-grid mapping, including correct shadowed-note dimming.

Six fixed RAM2 banks hold five resident letters and one unpublished staging
bank. The active `note` grid is a pointer. The timing interrupt publishes a bank
pointer and small precomputed metadata; it never copies/scans the whole grid or
touches SD. Fixed-size grid initialization/metadata work runs in the main loop
with interrupts enabled. Returning to a resident letter does no SD reads.
Live edits remain in their resident bank when switching during playback.

`src/toern_pattern_io.h` validates/serializes incrementally. Each job tick does
at most one 512-byte read or write, or one filesystem transition, then returns
to the application loop. MIDI, encoders and display can run between blocks.
A single SD operation still depends on card latency; this is a work budget,
not a hard real-time guarantee. Temporary writers use Teensy's
`FILE_WRITE_BEGIN` followed by an explicit truncate step; SdFat's `O_TRUNC` flag
must not be passed through the 8-bit `SD.open()` mode argument. The existing baseline manual-load/autosave paths
are unchanged apart from the compatible identity extension.

Automatic dirty-bank writes run only when stopped. New selection/transport
requests preempt those writes, retaining RAM data and cleaning incomplete owned
temporaries. Failures keep the dirty bank for a throttled retry. A staged slot
uses the same marker/size/pitch validation as ordinary patterns and tries its
primary, temporary, and backup generations. A valid backup is never mistaken
for a new blank slot; corrupt/unreadable data never replaces the live bank.

A uses `N.txt`; B–E use `N-B.txt` through `N-E.txt`. `N-A.txt` is ignored.
Family Save As and deletion are cooperative jobs too. Save As uses all resident
RAM letters (including live edits), copies uncached siblings with recovery,
and stages/validates all incoming files before installation. A failed rename
rolls back installed primaries. The target identity is adopted only after
successful installation/cleanup. Recovery-only destinations still require NO/YES
overwrite confirmation. The UI shows nonblocking WAIT/OK/ERR feedback. Deleted blank letters
stay deleted until explicitly saved or edited. Autosave appends the eight-byte
`TSUB` identity extension after the pitch extension, preserving the family,
active letter, and deleted-letter mask without changing `sizeof(Device)`.
Legacy autosaves without that extension restore A.

PATT dots extend through the last letter containing notes, with a minimum of A.
For A/C with notes and B/D/E empty, the display shows three dots. Trailing empty
letters do not extend the count; internal empty letters remain included. Empty
files stay available for editing, loading, overwrite protection, and deletion.
The loaded family's current RAM contents override older disk contents, so an
empty active E does not force five dots. Other families' content summaries are
validated and cached in bounded read jobs, separate from the file-presence index.
An empty destination shows the loaded family's content extent in red.
The compact 16-column folder leaves room for all five dots. These file
subpatterns are independent of the existing page/mute SUBPATTERN mode.
