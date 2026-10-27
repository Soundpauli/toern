---
sidebar_position: 1
title: Modes
description: How Mode structs drive encoder ranges and UI behavior.
---

# Modes

Almost every screen is a `Mode` instance. The global `Mode *currentMode` tells the rest of the firmware what the four encoders mean and which drawing path to take.

## `Mode` shape

Defined in `toern.ino`:

```cpp
struct Mode {
  String name;
  unsigned int minValues[4];
  unsigned int maxValues[4];
  unsigned int pos[4];       // current encoder positions
  uint32_t knobcolor[4];
};
```

When `switchMode(newMode)` runs, it stores `oldMode`, updates `currentMode`, and rewrites each encoder’s min/max/counter/RGB from the mode’s fields.

## Named modes (non-exhaustive)

| Mode variable | Name string | Typical job |
|---------------|-------------|-------------|
| `draw` | `DRAW` | Paint notes on the grid |
| `touchyMode` | `TCHY` | Five-pad sample performance with existing voice players |
| `singleMode` | `SINGLE` | Single-voice / focused editing |
| `filterMode` | `FILTERMODE` | Per-channel FX / synth params (sliders) |
| `velocity` | `VELOCITY` | Note detail: velocity / probability / condition / voice volume |
| `noteShift` | `NOTE_SHIFT` | Transpose / shift notes |
| `set_Wav` | `SET_WAV` | SMP: sample browser / assign |
| `recordMode` | `RECORD_MODE` | Record into a sample slot |
| `set_SamplePack` | `SET_SAMPLEPACK` | KIT: sample-kit selection |
| `loadSaveTrack` | `LOADSAVE_TRACK` | PATT: pattern load/save |
| `menu` | `MENU` | Top-level menu shell |
| `songMode` | `SONGMODE` | Song arrangement |
| `subpatternMode` | `SUBPATTERN` | Subpattern / mute-related UI |
| `bpmMode` | `BPM` | Tempo / clock source / brightness |
| `newFileMode` | `NEW_FILE` | Genre generator after loading an empty numbered slot |

## How input finds the right handler

1. Hardware callbacks / polling update encoder counters.
2. `checkMode` / button chord logic decides whether to `switchMode`.
3. Mode-specific branches in `toern.ino` (and UI files) read `currentMode->pos[i]` and mutate `note` / `GLOB` / `SMP`.
4. Draw code compares `currentMode == &draw` (pointer equality) to pick the correct LED rendering.

Pointer comparison against the global mode objects is the canonical check — don’t compare `name` strings in hot paths.

## Adding a mode

1. Declare a `Mode myMode = { ... }` next to the others.
2. Wire an entry gesture (button combo, menu item, encoder hold).
3. Handle encoder deltas while `currentMode == &myMode`.
4. Add drawing in `toern_ui.ino` (or a dedicated helper).
5. On exit, restore encoder config for the destination mode (`switchMode` already rebinds ranges).

## Operator labels and gestures

Menu labels are **PATT, KIT, SMP, BPM, VOL, SETT, REC, MIDI, MODE, ETC**. The BPM screen uses internal mode `bpmMode` with the name `BPM`; output volume is handled separately by VOL. Tap touch 1 to toggle DRAW/SINGLE on a sound voice; tap touch 2 to open the menu. The short touch-1 chord grace period lets the second pad join a sample-browser/filter gesture; it is not a hold requirement. Note detail maps encoders 1–4 to velocity, probability, condition/ratchet and whole-voice volume.

See the [handbook controls](https://handbook.toern.live/controls.html) and [note detail](https://handbook.toern.live/note-detail.html) for complete gestures.

TOUCHY is enabled with MODE → TCHY, is OFF after reboot, and stops sequencer playback. Touch 1–5 trigger voices 1–5 without writing notes. Encoders 1–4 edit the last pad-triggered voice’s pitch, velocity, reverb (where available), and highpass. Channel volumes stay full while enabled and are restored on disable. Short presses open SMP for voices 1–4. Hold encoder 1 to fast-record into the last pad-triggered voice and release to stop; recording does not change pattern notes. Hold encoder 3 for the last pad-triggered voice’s FILTER, and encoder 4 for the main menu. Existing browser/filter exits return to TOUCHY until its MODE toggle is OFF.

## Performance-fill arbitration

`pollTouchFillChord()` selects capacitive **touch 4+5** when `exttouch=false`, or external **C+D** when `exttouch=true`. It runs before REC/button handling and FN dispatch. `handleTouchFillChord()` accepts only during playback, outside TOUCHY and sample capture. It consumes the remaining held input until both release, preventing FN/audition or REC fallthrough. Requests capture the selected voice, not the future cursor selection.

`touchFillPendingMask` accumulates voices. `servicePendingTouchFill()` starts them together on the next page downbeat. `touchFills[ch]` owns independent timing, so re-requesting a voice does not reset others. Duration is `2 * maxX` sequencer steps, with the five-section `fillHitDue()` cadence. `currentFillStepStart()` rounds forward to the step boundary to avoid callback-order offsets.

`serviceFillStepBoundaries()` owns completion; the fill subtick timer must not clear the state early. At the exact following downbeat it queues one velocity-127 drop per voice. `triggerPendingFillDrops()` runs after normal grid/synth finishing work so a lower-velocity grid hit cannot replace the drop. Grid F/F demotes its trigger condition to normal on completion; requested fills never write the grid. Pause/start calls `resetTouchFills()` to cancel queued fills and drops.

The centered FILL overlay stores the requesting voice and expires after 1000 ms. `FastLEDshow()` composes it at presentation time; it must not alter the retained menu/grid framebuffer.

## Synth preset mode

`loadSavePreset` / `LOADSAVE_PRESET` is reached through SMP on ch11/13/14, displayed as **PSET**. Encoder 4 selects 1–99, encoder 1 loads, encoder 2 saves, encoder 4 short exits. Sample voices still enter `set_Wav`. Presets are scoped to each synth voice; see [SD storage](../subsystems/sd-storage).

## A–E file subpatterns

Enable file subpatterns in `MENU → SETT → SUBP` (`OFF` by default, `ON` to enable). When `SUBP` is `OFF`, holding FN and turning encoder 2 does not open the subpattern selector or show the overlay/indicator.

When `SUBP` is `ON` in DRAW, hold FN and turn encoder 2 to select A–E. The overlay shows both the subpattern letter and page number (`A1`, `B1`, …) in the center of the grid, like FILL, and the rest of the grid keeps rendering. While the selector is open, turning encoder 3 steps the target page through the pages that have notes in that subpattern (`hasNotes`). Turning encoder 2 in normal `CTRL: PAGE` mode also always flashes the page number overlay. Green means that letter's file opened with data, blue means empty. While another slot is shown over the playing pattern, the playhead column is yellow instead of red. No other letter, temporary file, or backup is checked.

Release FN to arm the slot. Playback keeps sounding the previous slot until the same boundary PMOD:NEXT uses, then switches to the new slot. If the new slot has notes on the selected/current page, that page is kept; otherwise it starts at the first page with notes in that subpattern. Stopped playback installs it immediately. An empty slot becomes a blank grid with the current sounds. `hasNotes` updates when the slot is installed, and the X cursor stays in its column. Loading or saving a pattern does not scan the other letters. Selecting the current slot leaves it unchanged.

Encoder 2's FN turn is consumed before it can change the page or the channel volume, including detents that arrive on the release itself. The volume path is skipped for the whole gesture, so CTRL:VOL does not move or redraw. Switching subpatterns while playing never writes to the SD card; switching while paused saves the outgoing slot (and a newly created blank slot) after the grid is showing. An unreadable target leaves the current grid in place. Recording, child lock and image mode do not open the selector. This is manual A–E selection; it does not add automatic song chaining.

These file slots are separate from the existing `subpatternMode` page/mute screen. See [SD storage](../subsystems/sd-storage) for filenames and recovery.
