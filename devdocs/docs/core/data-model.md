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
  uint8_t midiPitch;    // 0–127 preserved MIDI pitch; 255 = derive from Y
} __attribute__((packed));
```

The live pattern grid:

```cpp
EXTMEM Note note[maxlen + 1][maxY + 1];
```

- **X** is step (across pages × matrix width).
- **Y** is row / pitch lane on the matrix.
- Empty cells use `channel == 0`.
- `midiPitch == 255` keeps the historical row-derived pitch. With
  `MIDI > FOLD` off, recorded SEL/MIDI notes store their absolute incoming
  pitch here while Y remains a folded display/edit position.
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
| `songArrangement[64]` | Song mode: page number per arrangement slot |

Placing `SMP` in **EXTMEM** (PSRAM) keeps DTCM (RAM1) free for hot code/data.

## `GlobalVars GLOB` — cursor & edit context

`GLOB` tracks ephemeral editor state: current channel, page, cursor `(x,y)`, velocity, copy/paste, seek/trim positions, note-shift amounts, subpattern index, etc.

Playback page vs edit page can diverge (e.g. FLOW mode follows `beatForUI` for the visible page while playback owns `GLOB.page` carefully — see comments in `loop()`).

Voice mode (`voiceMode`, SETT → VMOD) gives each channel its own loop length: the highest page that contains any cell of that channel. The transport cycle is the longest voice. At global page G, voice C reads page `((G − 1 + offset) mod len) + 1` in the same column. The offset is set when encoder 2 cues that voice. Notes are matched by `note.channel`, so pitches on other rows stay with the voice. While it is on, FLOW, LOOP, and pattern-mode page lock do not steer playback or the page dots. Song mode is left alone. Encoder 2 steps only pages that contain the current voice (full range if the voice is empty). Changing voice snaps the edit page to that voice. Encoder 4 does not change pages while voice mode is playing. Condition repeats count that voice’s wraps (`voiceLoopCount`).

## Song mode

- `songModeActive` — follow arrangement instead of a single pattern loop.
- `SMP.songArrangement[i]` — page number in the current pattern at song position `i` (0 = empty).
- `currentSongPosition` — arrangement cursor.

## Persistence

Patterns and settings are written through file-operation helpers (`toern_fileoperations.ino`) and autosave paths triggered from UI/transport (often deferred so SD I/O doesn’t land inside an ISR). EEPROM is used for a smaller set of device preferences (menu address constant `EEPROM_MENU_ADDR`, pulse-clock settings, etc.).

Pattern files retain the legacy `4096 × 4-byte Note` prefix, `FF FE` marker,
and raw `SMP` payload. New saves append an optional `TPIT` version-1 extension
containing one `midiPitch` byte per grid cell. New firmware treats files without
that extension as row-derived; older firmware ignores the trailing extension.

## Invariants worth preserving

- Treat `channel == 0` as empty; reset its probability/condition and
  `midiPitch` metadata rather than leaving orphan state.
- Prefer updating `note`/`SMP` first, then redraw — LEDs should mirror state, not invent it.
- Large buffers belong in `EXTMEM` / `DMAMEM`, not as function-local `static` arrays (those land in RAM1).

Saves use destination-specific `N.txt.tmp` / `N.txt.bak` (or `autosaved.txt.tmp` / `.bak`). A valid destination wins; recovery otherwise validates a complete temporary file or the previous generation. Shared `pattern.tmp` is not promoted. Manual and automatic saves of empty grids store valid blank files and preserve every other subpattern. PATT browsing never changes the loaded family or its save destination. Loading stages the full note/settings payload before replacing live state.

## Recall and boundaries

PATT uses slots **1–999**; slot **0/A** loads `autosaved.txt` and is load-only. KIT uses numbered folders **1–999**, with **0** as the scratch/custom-sample kit. A pattern stores notes, sound settings, mutes and the 64-slot page arrangement, but not sample PCM. Loading it does not reload sample buffers: load its matching KIT separately.

Encoder 3 on PATT toggles applying loaded sound settings to audio objects. The `SMP` payload is still read into memory when that option is off; it is not a notes-only import. There are no `.dat` files or sidecars.

Pausing playback and successful sample loads request autosave. The five-second cooldown advances only after a successful save. The physical power switch does not save. Pattern loading stages the complete base payload before changing live state; legacy files without `TPIT` remain readable.

SONG holds **64 page assignments within the current pattern**, not saved pattern IDs. Clearing an assignment leaves later slots in place. Standard sequencing holds 256 steps: 16 pages × 16 columns or 8 pages × 32 columns. PMD OFF advances pages, ON loops the selected edit page; SONG and NEXT select their existing alternate playback paths.

PATT indexes canonical root filenames once on entry or a storage revision (1,000-byte RAM2 mask table). Detents read this index without SD access. Green dots count selected-family saved slots; red dots preview the loaded family (including its active RAM slot) for Save As into an empty selected number. Dots start two pixels right of the folder (one blank column), with one dark column between dots. FN+encoder 2 opens DEL? / NO for the selected whole A–E family; confirmed deletion removes A last and reports cleanup failures. Only deletion of the loaded family clears live notes. Save As copies siblings with a 512-byte buffer and writes the active slot from RAM; it refuses destinations with existing transaction files, rolls back newly created target files after failure, and adopts the target only after success. Occupied selections enter PATTERN_CONFIRM (SAVE? / NO) before Save As overwrites the selected family.

PATTERN_CONFIRM snapshots the selected target and defaults to NO (cancel). Encoder 4 selects NO/YES; encoder 4 short press executes. DEL?/SAVE? remains red. NO text, encoder 4 ring and indicator are blue; YES uses green. Touch 2 cancels. No grid cursor is drawn on this screen. Cancellation calls the general `showCanceledAnimation()` blue neutral-face reveal/fade and returns to the originating browser. Occupied-family Save As stages every source letter before rotating any destination primary into its .bak file; install failure restores primaries already moved and keeps the live source identity. After success, target-only letters and their recovery files are removed. Same-family Save confirms and updates the active letter while preserving its siblings. Family replacement is not atomic across a power failure.

Confirmation keeps its origin mode and browser positions. Delete completion/failure and all cancellation return there; a deleted sample selection is clamped to the refreshed list. Successful overwrite still returns to DRAW. Only encoder 4 press executes NO/YES; encoder 2 is ignored and its confirmation ring/indicator are off.
