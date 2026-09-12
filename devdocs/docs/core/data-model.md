---
sidebar_position: 2
title: Data model
description: Note grid, Device (SMP), GlobalVars (GLOB), and song arrangement.
---

# Data model

Music and session state live in a few global structures. UI and MIDI are views/controllers over this data.

## `Note` — one grid cell

```cpp
struct Note {
  uint8_t channel;      // 0 = empty; else voice/channel id
  uint8_t velocity;     // 0–127 internally
  uint8_t probability;  // 0–100
  uint8_t condition;    // conditional trigger encoding
} __attribute__((packed));
```

The live pattern grid:

```cpp
EXTMEM Note note[maxlen + 1][maxY + 1];
```

- **X** is step (across pages × matrix width).  
- **Y** is row / pitch lane on the matrix.  
- Empty cells use `channel == 0`.
- Condition bytes `1,2,4,8,16,17–20` encode loop conditions, `21` is F/F
  fill, and `22` is G/L destination-note glide/legato. Unknown values should
  fall back to always-trigger behavior rather than becoming sparse implicitly.

UI often maps displayed velocity (1–16 rows) to MIDI-ish 1–127 with `mapf`.

## `Device SMP` — pattern / project bag

`EXTMEM Device SMP` holds the loaded project’s non-grid settings, including:

| Field area | Meaning |
|------------|---------|
| `bpm`, `file`, `pack` | Tempo, pattern id, samplepack id |
| `wav[]`, `samplePathRel[][]` | Sample browser selection + custom paths |
| `param_settings` / `filter_settings` / `synth_settings` | Per-channel parameter tables |
| `mute[]`, `channelVol[]` | Mutes and channel volumes |
| `globalMutes` / `pageMutes` | Mute behavior depending on pattern mode |
| `sp0Active[]` | Samplepack-0 (per-voice custom sample) flags |
| `songArrangement[64]` | Song mode: pattern id per arrangement slot |

Placing `SMP` in **EXTMEM** (PSRAM) keeps DTCM (RAM1) free for hot code/data.

## `GlobalVars GLOB` — cursor & edit context

`GLOB` tracks ephemeral editor state: current channel, page, cursor `(x,y)`, velocity, copy/paste, seek/trim positions, note-shift amounts, subpattern index, etc.

Playback page vs edit page can diverge (e.g. FLOW mode follows `beatForUI` for the visible page while playback owns `GLOB.page` carefully — see comments in `loop()`).

## Song mode

- `songModeActive` — follow arrangement instead of a single pattern loop.  
- `SMP.songArrangement[i]` — pattern number at song position `i` (0 = empty).  
- `currentSongPosition` — arrangement cursor.

## Persistence

Patterns and settings are written through file-operation helpers (`toern_fileoperations.ino`) and autosave paths triggered from UI/transport (often deferred so SD I/O doesn’t land inside an ISR). EEPROM is used for a smaller set of device preferences (menu address constant `EEPROM_MENU_ADDR`, pulse-clock settings, etc.).

## Invariants worth preserving

- Treat `channel == 0` as empty; don’t leave orphan probability/condition without a channel.  
- Prefer updating `note`/`SMP` first, then redraw — LEDs should mirror state, not invent it.  
- Large buffers belong in `EXTMEM` / `DMAMEM`, not as function-local `static` arrays (those land in RAM1).
