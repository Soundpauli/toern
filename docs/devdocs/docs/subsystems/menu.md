---
sidebar_position: 3
title: Menu system
description: Nested menu state machine and settings surfaces.
---

# Menu system

[`toern_menu.ino`](https://github.com/Soundpauli/toern/blob/main/platformio/toern_menu.ino) implements the on-device settings UI entered via the `MENU` mode.

## Structure

Top-level pages fan into submenus flagged by booleans such as:

- `inLookSubmenu`
- `inRecsSubmenu`
- `inMidiSubmenu`
- `inVolSubmenu`
- `inEtcSubmenu`

MODE, ETC and ETC → ADV share `inEtcSubmenu` and `currentEtcPage`; `currentSystemMenuGroup` selects the active page array. `systemMenuParentPage()` restores MODE or ETC when leaving. Each other family has a `current*Page` index. Exiting restores draw vs single via `menuEnteredFromSingleMode`.

## Notable ETC behavior

**Menu → ETC → SD** shows **SCON** / **WAIT** / **OK**. The USB serial server (`toern_sd_serial.ino`) is polled from `loop()` on every screen, and only after a host program has opened the port.

## Menu hierarchy

- **SETT:** VIEW, CTRL, DRAW, MUTE, SWAP, SUBP. SUBP is persistent, defaults OFF and is selected with encoder 3. See [A–E subpatterns](./sd-storage#ae-file-subpatterns).
- **MODE:** FLOW, PMOD, LOOP, VMOD, CHLD, IMG, PONG, SONG, TCHY, AGEN. MODE occupies the former SONG root slot and retains its icon and colour. Press encoder 4 on SONG to open the existing arranger.
- **ETC:** INFO, SD, BATT, ADV, RSET. RSET → RVRB sets reverb and bitcrusher controls to zero/bypass on all voices, preserving the other sound settings and notes. Turn encoder 3 to select RVRB, then press encoder 2 to apply.
- **ETC → ADV:** CRSR, LEDS, RAM, LGHT, COLR, EYES, FIRE. Press encoder 4 on ADV to enter.
- **VOL → PREV:** encoder 2 selects ON / PRSS / SYNC; encoder 3 retains preview volume (0–50). The former SETT → PREV leaf is removed.

Setting IDs, EEPROM offsets and audio routing are unchanged. The simulator separates these navigation groups from its historical persisted setting arrays, so existing browser settings remain valid.

## Settings backup

Menu changes can mark settings dirty (`markSettingsBackupDirty`) and later flush via `serviceSettingsBackup()` from `loop()` — keeping EEPROM/SD writes off the audio ISR path.

## Adding a menu item

The file list and the EEPROM offset table are in [Firmware edits](../contributing/firmware-edits). Short version: a free `mainSetting` id, a `MenuPage` row, a draw `case`, an encoder `case`, and a new EEPROM offset at the end of the settings block if it must survive reboot.

## Current labels

Root families: **PATT, KIT, SMP, BPM, VOL, SETT, REC, MIDI, MODE, ETC**. MIDI uses **SEL**, **FOLD**, **OVWR** and **TRIG** for voice selection, pitch folding, occupied-cell overwrite and selected-voice analog trigger output. REC → TRIG is the existing recording-trigger setting, a separate control. Internal identifiers and EEPROM offsets retain their historical names.

A green animated checkmark confirms a successful manual pattern or kit save; a red animated cross signals failure. PATT slot occupancy is communicated by colour, with no USED/NEW status text. The NEW generator remains a separate screen when loading an empty numbered slot.

### TOUCHY performance mode

MODE → TCHY (setting ID 63) is a runtime-only OFF/ON toggle on encoder 3; it does not add EEPROM fields. Enabling pauses the sequencer. Returning to DRAW/SINGLE is redirected to `touchyMode` until disabled. `toern_touchy.ino` maps touch pads 1–5 to existing voices 1–5 through `triggerCurrentVoiceAudition`, sets `GLOB.currentChannel` on each hit, and draws the flying pixel cat instead of the grid. Pad colours are fixed. Encoder rotations edit the last pad-triggered voice: 1 = OCTV pitch (−24…+24 semitones), 2 = per-voice runtime velocity (1…127), 3 = REVERB (disabled on voices 3/4), 4 = LOWCUT highpass. Existing filter setters and audition playback are reused. Voices 1–5 run at full channel volume while enabled; their prior volumes are restored on disable. Short presses open `set_Wav` for voices 1–4. Hold encoder 1 to start existing Fastrec on the last pad-triggered voice; release stops it. Capture skips pattern-note clearing/placement in TOUCHY. The capture loop polls only encoder 1 for release, without pad scans or LED updates. Long encoder 3 selects the last pad-triggered voice and opens `filterMode`; long encoder 4 opens the root at MODE. The entry hold is consumed until release. Returning from browser/filter restores controls for the last pad-triggered voice. Fastrec stays bound to its starting channel. Transport start and idle EYES are suppressed while performing in TOUCHY. Disable TOUCHY and exit the menu to return to the grid; playback stays stopped. Audio players, filters, sample loading and pattern storage are unchanged.

TOUCHY pad input latches after a hit and rearms only after 60 ms continuously at or below the normal press threshold. The prior 70% release threshold was too low for some untouched pads and could leave them latched after one hit. Stable release timing still rejects brief held-pad noise. The display uses a dark 21×11 cat with a black face outline, a full leftward rainbow, and whole-pixel rendering without colour interpolation for crisp oscillation within the rightmost 20% of the panel.

### AGEN: autonomous temporary evolution

On device, **MODE → AGEN → encoder 4 click** toggles self-playing evolution.
Encoder 1 retains one-shot AUTO generation with the existing base-range and
additional-page controls. Encoder 4 is green and the page shows ON while the
autonomous performance is active. No MCP connection is involved.

AGEN starts from the currently selected page. A workspace is allocated with
`extmem_malloc` on entry and freed on exit, preferring PSRAM and falling back to
RAM2. Allocation failure leaves the original bank and transport unchanged. No
fixed note/context workspace is reserved while AGEN is off. Its small scale,
chord/progression and arrangement tables use flash (`PROGMEM`); heavy musical
helpers execute from flash. Larger optional theory/motion libraries could be
loaded from SD on entry, but no such library or live SD traffic is required.

Two pages alternate while the original bank stays untouched. A third, invisible
workspace page holds an immutable copy of the starting motif. At the last
physical page the second performance page wraps to page 1. The key, mode and
starting registers are captured once: every variation refers to that foundation,
rather than re-inferring a key from the preceding mutation. Strong melodic beats
fit diatonic chord tones; passing notes stay in the scale. Bass beats land on the
chord root. Harmony holds through transitions and resolves to the tonic on the
drop. Suitable CH11 registers receive three-note diatonic voicings; existing
low CH11 bass stays bass; when both mono voices are free and the voice budget
allows it, CH13/14 supply the matching chord third/fifth. Tonal sample rows are moved only to a free correctly
tuned row, avoiding off-key collision fallbacks.

Groove/variation sections lead into a stripped break, an accelerating build,
a short fill and a drop that restores the kick/bass groove. Ghost notes, hats,
pickups and rolls develop the drums. Phrases repeat randomly 1/4/8 times,
favoring longer groove sections; fills last one pass, drops hold 4/8 passes.
Nonstructural notes use role-dependent probabilities (roughly 60–95%); structural
anchors stay reliable unless the source explicitly specified a lower probability.
One voice is prepared per main-loop pass. The sequencer only publishes complete
pages at boundaries; late jobs repeat the playing page.

On DRAW/SINGLE, y=16 shows only the two performance-page indicators: white for
playing, red for the unfinished next page, yellow for a complete waiting page.
A right-aligned number at rows 11–15 counts the remaining passes, including the
current one. While waiting for a late builder it stays at 1. It uses the same
font/placement as the normal page-number helper on 16×16 and 32×16 panels.
An empty source gets a restrained starter. Companion additions are bounded to
seven active voices unless the original already contains more.

AGEN also performs per-voice volume moves through `applyChannelControlVolume`,
the same DSP path used by physical encoder 2 in CTRL=VOL. It fades kick/bass out
for breaks, lifts levels through builds, varies supporting voices, and restores
the starting kick/bass level for drops. Moves stay at or below each voice's user
ceiling and advance in small steps; muted starting voices remain muted. A manual
encoder-2 turn updates that voice's ceiling and holds automation for two seconds.
External moves synchronize the selected volume counter without being mistaken
for another manual turn or repeating DSP/feedback work. Sample readers retain
their existing amplitude-ramp path; physical fader gain/bottom stops stay effective.
Starting channel volumes, last-nonzero values, mute bytes and sample-reader gain
ownership are restored on exit before autosave. Automation state lives inside
the on-demand workspace.

At an eligible fill-section boundary, AGEN can queue one audible percussion
voice (snare, tom, clap or hat) through the existing touch/F/F fill engine. Its
one-page request ends with the engine's velocity-127 drop on the following
page downbeat. Late requests are skipped. Manual fills keep their two-page
length; AGEN exit cancels only fills it owns. Duration/ownership fields occupy
existing `TouchFillState` padding, preserving the 12-byte per-voice state.
`_debug/tests/test_touch_fill.py` checks both durations, ownership and exact downbeats;
AGEN state/music tests check ceilings, manual hold, restoration and volume curves.

Toggle OFF or pause to restore the original pattern and selected page. Opening
PATT, NEW, SONG or TOUCHY, changing display width, starting recording, or selecting
a file subpattern also ends AGEN. Pattern saving/loading restores the original
bank first. Deferred subpattern work is suspended while AGEN runs, and startup
is rejected while bank writes/switches are pending. Temporary notes and mutes
are never autosaved; kit and sound settings remain live. AGEN is not persisted
as an enabled preference across reboot.

`platformio/src/toern_autogen.h` owns boundary/repeat scheduling; `toern_helpers.ino` owns the
background builder and temporary-bank lifecycle. `_debug/tests/test_autogen.py` tests
both 16- and 32-step pages, late jobs, randomized phrase lengths, source safety,
empty starters, allocation failure/release, immutable harmony/motif and restoration.
`_debug/tests/test_autogen_visual_music.py` renders both layouts and checks all 12 major/
minor keys, chord/bass constraints, register bounds and arrangement contrasts.
The normal generation regression remains `_debug/tests/test_generation_preserves_notes.py`.

### Input transport failures and stall diagnostics

The pinned DuPPa library previously returned `0xFF` after a failed status read,
then dispatched its bits as button/turn events. The PlatformIO patch now checks
register selection and exact read lengths, returns no status events on failure,
and requests exactly two bytes for an integer counter. Main-loop polling skips
position changes after a transport error.

`toern_i2c.cpp` records transaction failures and maximum duration. After setup,
a failure suppresses further primary-bus transactions for 8 ms, or 100 ms after
a transaction taking over 5 ms. This prevents repeated timeouts across the
panel in the same loop. Startup probes and secondary buses retain their normal
behavior. No Serial output or interrupt masking is added by this guard.

MCP `PERF` exposes `i2c_errors`, `i2c_max_us`, `i2c_skipped`; passive loop timing
now separates encoder/input processing, faders/cursor, buttons and touch polling.
Error counts may include startup probes; compare changes over the test interval.
Native tests inject NACKs and short reads into the actual patched driver
(`_debug/tests/test_encoder_failures.py`) and exercise wrapper cooldown/recovery
(`_debug/tests/test_i2c_startup.py`).
