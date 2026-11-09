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
- CLI and local browser server: `tools/sdtool/toern_sd.py`

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

KIT saving runs cooperatively **during playback**. Overwrite discovery, file
open/close/rename and data transfer happen in separate loop passes. Each transfer
is at most **2 KB**, using an internal-RAM transfer buffer with interrupts enabled.
RAM copying and its corresponding write share a pass; there is no artificial
inter-operation delay. MIDI and
controls run between passes. PATT and KIT transfers share the renderer in
`platformio/src/toern_storage_visual.h`, including KIT loading at boot. It shows the menu
icon, a green outward load arrow or red inward save arrow, percentage and progress
bar. Completion reveals a green check or red X without text or blocking delays.
Pause-triggered autosave reveals a white check over the current screen only after
an actual successful write; its result does not clear the frame or paint a black
background. Cooldown skips show no check. The 32×16 (`led == 2`) and 16×16 (`led == 1`)
layouts have separate spacing. Background KIT progress leaves DRAW usable, and
resumes after any pause-autosave acknowledgement.
`_debug/tests/test_storage_visual.py` checks real icon/font pixels, bounds and animation
colors on both sizes; `_debug/tests/test_storage_feedback.py` checks progress and
autosave feedback timing.
Only the raw aligned transfer buffer is `DMAMEM`; the writer state and `File`
objects live in normally initialized RAM. Teensy's DMA section is `NOLOAD` and
cannot hold a statically initialized writer object. The compiled placement is
checked by `_debug/tests/test_audio_graph_memory.py`.

The writer stages all eight WAVs as `<voice>.wav.kit-tmp` and checks their full
read-back CRC before installing any of them. It exports current loaded PCM,
including custom samples whose deferred SP0 copy is still pending; KIT 0 cannot
delete its own input. Existing primaries move to `.kit-bak` during installation.
Write/validation failures leave the old kit intact; failed installs and
cancellation attempt rollback. Navigation and sound-control edits can continue;
recording or replacing a sample cancels an in-progress save. A committed save
does not relabel newly loaded samples. Kit-ID/SP0 EEPROM persistence waits for
stopped, settled audio; disk saving itself continues during play.

Each newly saved non-empty KIT WAV can include an optional `tsrc` RIFF chunk
after its PCM data: a 128-byte, null-terminated path relative to `samples/`.
The writer snapshots the loaded voice's source, not the browser/preview row.
Metadata is staged, CRC-verified and rolled back with the audio in the same WAV;
there is no separate source-map file that could get out of sync. PCM stays at
offset 44, and the data-chunk size excludes metadata. KIT loading restores that
path and its loaded identity; entering SMP selects the original directory/file
when available, with the white `>` mark. Returning after previewing a neighbour
selects the loaded source again. Missing sources fall back to ordinary browsing;
the KIT's own PCM remains playable. Legacy KITs without metadata cannot reveal
their original source filenames; load the original WAVs and resave the KIT to
add that information. Recorded/unknown sources are left without a browser origin.

A card operation can still take longer than its usual latency; a sector budget
is not a hard real-time guarantee. Power loss across the eight renames is **not
atomic**. A subsequent save can recover a missing primary from its `.kit-bak`;
ordinary kit loading does not perform a whole-kit transaction recovery.
Regression: `python3 _debug/tests/test_kit_save_audio.py`. PATT save does not save sample
audio, and PATT load does not reload the matching KIT.

Hardware validation of the 2 KB writer saved KIT 500 twice while the current
339-note pattern continued playing: approximately 11.5 seconds to the OK display
and 12.7 seconds including that display on the repeat. Both USB recordings had
no capture errors; peak measured audio CPU was about 32%. These timings depend
on sample lengths and the SD card, and do not establish identical load/save
speed or an exhaustive audio-quality result.

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

Enable `SETT → SUBP` with encoder 3 (persistent, OFF by default) and load a numbered family. FN is capacitive T5 or external button D. Selection is independent of cursor row (`y == 16` is not required). In DRAW, FN + encoder 2
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

`platformio/src/toern_pattern_io.h` validates/serializes incrementally. Each job tick does
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
overwrite confirmation. Save As uses the shared progress and animated result
screen; deletion retains its wait indicator and animated result. Deleted blank letters
stay deleted until explicitly saved or edited. Autosave appends the eight-byte
`TSUB` identity extension after the pitch extension, preserving the family,
active letter, and deleted-letter mask without changing `sizeof(Device)`.
Legacy autosaves without that extension restore A.

PATT dots extend through the last letter containing notes, with a minimum of A.
The loaded numbered family has a blue `>` beside its number, independent of
edits. Other destinations, NEW and AUTOSAVE have no marker; no dirty comparison
or additional storage reads run for this indicator.
For A/C with notes and B/D/E empty, the display shows three dots. Trailing empty
letters do not extend the count; internal empty letters remain included. Empty
files stay available for editing, loading, overwrite protection, and deletion.
The loaded family's current RAM contents override older disk contents, so an
empty active E does not force five dots. Other families' content summaries are
validated and cached in bounded read jobs, separate from the file-presence index.
An empty destination shows the loaded family's content extent in red.
The compact 16-column folder leaves room for all five dots. These file
subpatterns are independent of the existing page/mute SUBPATTERN mode.

KIT browsing updates the selected number immediately. `platformio/src/toern_kit_browser.h`
waits 35 ms after the last turn, then probes at most one voice WAV per loop.
Availability (including empty slots and kits without voice 1) is cached for the
browser visit. Mode entry and save feedback invalidate the cache explicitly;
slow frames do not. `_debug/tests/test_kit_browser.py` covers this behavior.

With MCP compiled in, passive main-loop timing remains available through `PERF`
even after an idle run without a connected client. `loop_stalls` counts sections
over 50 ms; `last_stall_stage`, `last_stall_us`, `loop_max_us`, `stage_max_us` and
`temp_centi` help distinguish serial, background, render, storage and input work
from audio DSP usage. Explicit transfers and flashing/restoration can also
produce long sections: compare the counters before and after an idle test.
The counters perform no serial output, allocation or interrupt masking; replies
are sent only on request. They are omitted in the no-MCP build.
