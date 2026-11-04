# TŒRN MCP hardware audit

Tested on the connected Teensy on **2 November 2026**, through the real local stdio MCP server. The updated firmware built and flashed successfully. No backup was made during this audit.

## Results

- SINGLE: entry/exit, independent pitch row, paint/erase of one initially empty cell, menu return to SINGLE, selected voice preserved into filter mode.
- Filters: **11 playable voices, 47 pages, 170 active sliders**. Every active slider changed and restored; every page reset; page cycling wrapped correctly; whole-voice defaults verified.
- Menus: **63 root/submenu items** reached and checked through semantic JSON and nonempty rendered matrix JSON.
- Settings: **60 reversible physical encoder adjustments** checked and restored across editable submenu pages.
- PATT, KIT, sample browser, BPM, all three synth preset browsers, SONG arranger: open/close verified. Preset slots and song-position browsing verified without loading/saving.
- RSET: all eight selector positions verified; no apply action fired.
- DEMO: launch, serial readback/matrix access while running, and exit through remote touch 1 verified.
- Final pass: all 11 voices reset to defaults again on the final flashed build. Device left in DRAW, paused, cursor (1,2), BPM 88. A follow-up live readback confirmed all 11 voices still matched every reported default; playback had since been started at 88 BPM. These are live runtime defaults; persistence through reboot was not tested.

## Filter voices

| Voice | Pages | Active sliders | Slider edits | Page reset | Whole-voice reset |
|---|---:|---:|---|---|---|
| 1 | 4 | 14 | Pass | Pass | Pass |
| 2 | 4 | 14 | Pass | Pass | Pass |
| 3 | 4 | 14 | Pass | Pass | Pass |
| 4 | 4 | 14 | Pass | Pass | Pass |
| 5 | 4 | 14 | Pass | Pass | Pass |
| 6 | 4 | 14 | Pass | Pass | Pass |
| 7 | 4 | 14 | Pass | Pass | Pass |
| 8 | 4 | 14 | Pass | Pass | Pass |
| 11 | 5 | 20 | Pass | Pass | Pass |
| 13 | 5 | 19 | Pass | Pass | Pass |
| 14 | 5 | 19 | Pass | Pass | Pass |

Each sample voice has four pages; synth voices 11, 13 and 14 have five. Whole-voice reset includes envelopes, delay, sample time/gain and synth instrument/LFO/ARP defaults as applicable. CH11 slider tests refresh their readback after INST changes: restoring an instrument reapplies its preset to sibling controls.

## Every menu item

Navigation checks assert the active group, name, setting ID and a nonempty matrix. “Adjust” identifies physical encoders moved and restored; navigation-only rows do not claim the item’s action was executed.

| Menu | Setting ID | Navigation | Adjustment / action coverage |
|---|---:|---|---|
| ROOT/PATT | 1 | Pass | Navigation; child screens tested separately |
| ROOT/KIT | 2 | Pass | Navigation; child screens tested separately |
| ROOT/SMP | 3 | Pass | Navigation; child screens tested separately |
| ROOT/BPM | 5 | Pass | Navigation; child screens tested separately |
| ROOT/VOL | 26 | Pass | Navigation; child screens tested separately |
| ROOT/SETT | 19 | Pass | Navigation; child screens tested separately |
| ROOT/REC | 20 | Pass | Navigation; child screens tested separately |
| ROOT/MIDI | 21 | Pass | Navigation; child screens tested separately |
| ROOT/MODE | 61 | Pass | Navigation; child screens tested separately |
| ROOT/ETC | 34 | Pass | Navigation; child screens tested separately |
| VOL/MAIN | 27 | Pass | Adjust E3; restored |
| VOL/GAIN | 53 | Pass | Adjust E1, E2, E3; restored |
| VOL/LOUT | 28 | Pass | Adjust E3; restored |
| VOL/PREV | 29 | Pass | Adjust E3; restored |
| VOL/SPKR | 43 | Pass | Adjust E3; restored |
| SETT/VIEW | 17 | Pass | Adjust E3; restored |
| SETT/CTRL | 25 | Pass | Adjust E3; restored |
| SETT/DRAW | 38 | Pass | Adjust E3; restored |
| SETT/MUTE | 47 | Pass | Adjust E2, E3; restored |
| SETT/SWAP | 67 | Pass | Adjust E2, E3; restored |
| SETT/SUBP | 68 | Pass | Adjust E3; restored |
| REC/INPT | 4 | Pass | Adjust E3; restored |
| REC/MIC | 30 | Pass | Adjust E3; restored |
| REC/HPF | 64 | Pass | Adjust E3; restored |
| REC/L-IN | 31 | Pass | Adjust E3; restored |
| REC/TRIG | 11 | Pass | Adjust E3; restored |
| REC/CLR | 12 | Pass | Adjust E3; restored |
| MIDI/CH | 7 | Pass | Adjust E3; restored |
| MIDI/TRAN | 8 | Pass | Adjust E3; restored |
| MIDI/SEND | 13 | Pass | Adjust E3; restored |
| MIDI/RCVE | 44 | Pass | Adjust E3; restored |
| MIDI/FOLD | 51 | Pass | Adjust E3; restored |
| MIDI/SYNC | 45 | Pass | Adjust E2, E3; restored |
| MIDI/PQ-I | 59 | Pass | Adjust E1, E2, E3; restored |
| MIDI/PQ-O | 50 | Pass | Adjust E1, E2, E3; restored |
| MIDI/TRIG | 58 | Pass | Adjust E3; restored |
| MIDI/OVWR | 60 | Pass | Adjust E3; restored |
| MODE/FLOW | 10 | Pass | Adjust E3; restored |
| MODE/PMOD | 9 | Pass | Adjust E3; restored |
| MODE/LOOP | 18 | Pass | Adjust E3; restored |
| MODE/VMOD | 54 | Pass | Adjust E3; restored |
| MODE/CHLD | 48 | Pass | Navigation only: enabling child mode rewrites mute masks |
| MODE/IMG | 56 | Pass | Adjust E3; restored |
| MODE/PONG | 24 | Pass | Adjust E3; restored |
| MODE/SONG | 22 | Pass | Adjust E3; restored |
| MODE/TCHY | 63 | Pass | Adjust E3; restored |
| MODE/AUTO | 15 | Pass | Adjust E1, E2, E3; restored |
| MODE/DEMO | 66 | Pass | Navigation only: read-only/action launcher |
| ETC/INFO | 39 | Pass | Navigation only: read-only/action launcher |
| ETC/SD | 49 | Pass | Navigation only: read-only/action launcher |
| ETC/BATT | 42 | Pass | Navigation only: read-only/action launcher |
| ETC/ADV | 62 | Pass | Navigation only: read-only/action launcher |
| ETC/RSET | 16 | Pass | Adjust E3; restored |
| ADV/CRSR | 32 | Pass | Adjust E3; restored |
| ADV/LEDS | 23 | Pass | Adjust E3; restored |
| ADV/RAM | 52 | Pass | Adjust E3; restored |
| ADV/LGHT | 40 | Pass | Adjust E3; restored |
| ADV/COLR | 41 | Pass | Adjust E3; restored |
| ADV/EYES | 57 | Pass | Adjust E3; restored |
| ADV/FIRE | 55 | Pass | Adjust E1, E2, E3; restored |
| ADV/2-CH | 36 | Pass | Adjust E3; restored |
| ADV/USBP | 65 | Pass | Adjust E3; restored |
| ADV/HFC | 46 | Pass | Adjust E3; restored |

## Additional screens and actions

| Test | Result |
|---|---|
| OPEN/PATT | Pass |
| OPEN/KIT | Pass |
| OPEN/SMP | Pass |
| OPEN/BPM | Pass |
| SINGLE: selected voice through filter entry/page/reset/exit | Pass |
| PSET voice 11: open, browse, close | Pass |
| PSET voice 13: open, browse, close | Pass |
| PSET voice 14: open, browse, close | Pass |
| SONG: open, position, close | Pass |
| RSET: all eight selectors without applying | Pass |
| MODE/DEMO launch and remote exit | Pass |

## Changes made for remote control

- Added `get_menu`, `get_filters`, `get_matrix` and `reset_filters` MCP tools. Filter reset uses the existing touch-3/page and encoder-4/whole-voice gestures.
- State now reports SINGLE, active filter page/count, the cursor cell and DEMO activity.
- Kept USB serial polling ahead of DEMO’s early-return animation loop, so a remote client can inspect and exit DEMO.
- Added the repeatable `audit_device.py` hardware audit.

## Boundaries

Global resets/wipes, file load/save/delete, recording into sample buffers and pattern generation were not executed. CHLD was inspected but not enabled: enabling it rewrites all mute masks. The RSET choices were selected without applying them. MIDI/clock controls were checked for UI/state changes, not externally measured with receiving equipment. Filter parameter readback and reset behavior were verified; no audio-loopback measurement was made.

## Repeat

```sh
.venv/bin/python audit_device.py --port /dev/cu.usbmodemXXXX --report /tmp/toern-audit.json
```

This is a mutating test and ends with all playable voices at defaults. Use `--section menus` for navigation-only checks. See [README.md](README.md) for installation and other sections.
