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
