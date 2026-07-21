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
  const char *name;
  int32_t minValues[4];
  int32_t maxValues[4];
  int32_t pos[4];       // current encoder positions
  uint32_t knobcolor[4];
};
```

When `switchMode(newMode)` runs, it stores `oldMode`, updates `currentMode`, and rewrites each encoder’s min/max/counter/RGB from the mode’s fields.

## Named modes (non-exhaustive)

| Mode variable | Name string | Typical job |
|---------------|-------------|-------------|
| `draw` | `DRAW` | Paint notes on the grid |
| `singleMode` | `SINGLE` | Single-voice / focused editing |
| `filterMode` | `FILTERMODE` | Per-channel FX / synth params (sliders) |
| `velocity` | `VELOCITY` | Velocity / probability / condition editing |
| `noteShift` | `NOTE_SHIFT` | Transpose / shift notes |
| `set_Wav` | `SET_WAV` | Sample browser / assign |
| `recordMode` | `RECORD_MODE` | Record into a sample slot |
| `set_SamplePack` | `SET_SAMPLEPACK` | Samplepack selection |
| `loadSaveTrack` | `LOADSAVE_TRACK` | Pattern load/save |
| `menu` | `MENU` | Top-level menu shell |
| `songMode` | `SONGMODE` | Song arrangement |
| `subpatternMode` | `SUBPATTERN` | Subpattern / mute-related UI |
| `volume_bpm` | `VOLUME_BPM` | BPM / related transport UI |
| `newFileMode` | `NEW_FILE` | New file helpers |

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
