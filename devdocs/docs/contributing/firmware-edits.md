---
sidebar_position: 2
title: Firmware edits
description: Where a new menu page and a new saved setting have to land.
---

# Firmware edits

The sketch is one program. A new on-screen setting touches several files. GAIN (menu id 53) is the recent example: defines in `toern.ino`, EEPROM bytes, draw, encoder handling, `startNew()`, and the handbook.

Build with PlatformIO (`pio run` from the repo root). See [Build setup](./build-setup). UI functions are marked `FLASHMEM` so they stay out of ITCM. Keep an eye on the ITCM line in the build output; it has to stay under 256 KB. Large buffers go in `EXTMEM` or `DMAMEM` — see [Memory](./memory).

## Add a menu page

Pages are `MenuPage` rows near the top of [`toern_menu.ino`](../../../toern_menu.ino). Each row has a 4-character name and an integer `mainSetting`. That integer is the `case` label in the draw and input switches. Pick one that is not already in `menuPages`, `lookPages`, `recsPages`, `midiPages`, `volPages`, or `etcPages`.

1. Add the row and bump the matching `*_PAGES_COUNT`.
2. Draw it in `drawMainSettingStatus`.
3. If an encoder changes a value, handle it in `handleAdditionalFeatureControls`. Reset encoder min/max when leaving the page, the same way ids 50 and 53 do.
4. If a knob **click** does something, handle it in `switchMenu` and in the button match inside `checkMode` (`toern.ino`), so the click is not treated as a generic menu action.
5. If the value must survive reboot, add an EEPROM offset as below, write the default in both the virgin-EEPROM path of `loadMenuFromEEPROM` and in `startNew()` (`toern_helpers.ino`), and apply it from `applyAudioSettingsFromGlobals` when it affects audio.
6. If a player can see it, update the handbook page for that menu.

## EEPROM bytes

Settings live at absolute address `EEPROM_DATA_START + offset` with `EEPROM_DATA_START` = **43**. A magic byte `0x5A` sits at absolute address **42**. Sample-pack 0 flags sit at absolute address **200** (`EEPROM_SP0_STATE_ADDR`), outside this table.

`SETTINGS_EEPROM_BLOCK_LEN` is **48**, so offsets **0 through 47** are copied to the SD settings backup. The next free offset is **48**. Raise `SETTINGS_EEPROM_BLOCK_LEN` in the same change. Inserting a byte in the middle shifts every saved unit. A shorter backup leaves the missing bytes at 0; offset 42 absent means VMOD off, offset 43 absent means FIRE off. Offset 44 absent or out of 1–25 loads as count 8. Offset 45 absent loads as size 1. Offset 46 absent loads as gravity 0. Offset 47 absent loads as colour 8.

Offsets 32–33 and 34–35 are `uint16` values (`EEPROM.put`). Offset 33 is also written as a single format byte. Leave 32–35 alone.

| Offset | What |
|--------|------|
| 0 | recMode |
| 1 | clockMode |
| 2 | transportMode |
| 3 | patternMode |
| 4 | voiceSelect |
| 5 | fastRecMode |
| 6 | recChannelClear |
| 7 | previewVol |
| 8 | flowMode |
| 9 | micGain |
| 10 | PPQN pulse packed byte |
| 11 | simpleNotesView |
| 12 | loopLength |
| 13 | ledMode |
| 14 | ctrlMode |
| 15 | lineOutLevelSetting |
| 16 | lineInLevel |
| 17 | headphone volume (`GLOB.vol`) |
| 18 | cursorType |
| 19 | showChannelNr |
| 20 | previewTriggerMode |
| 21 | drawMode |
| 22 | colorScheme |
| 23 | stereoChannel |
| 24 | midiSendMode |
| 25 | ledStripEnabled |
| 26 | ledBrightness |
| 27 | spkrEnabled |
| 28 | midiNoteReceive |
| 29 | transportSendDelayMs |
| 30 | PPQN pulse width, ms |
| 31 | transportRcveDelayMs |
| 32–33 | codecHfCut (`uint16`) |
| 34–35 | draw-R mute mask (`uint16`) |
| 36 | childLockEnabled |
| 37 | MIDI pitch clamp |
| 38 | mixGain14 |
| 39 | mixGain58 |
| 40 | mixGainSynth |
| 41 | mixGainMaster |
| 42 | voiceMode (VMOD), 0 = off, 1 = on |
| 43 | fireVoice (ETC → FIRE), 0 = off, 15 = all voices, else voice 1–8, 11, 13, or 14 |
| 44 | fireLevel (ETC → FIRE), particle count 1–25. Encoder 2. Out of range loads as 8 |
| 45 | fireSize (ETC → FIRE), particle size 1–4. Encoder 1. Out of range loads as 1 |
| 46 | fireGravity (ETC → FIRE), 0–8. 0 floats up. Encoder 3 after one click. Above 8 loads as 0 |
| 47 | fireColor (ETC → FIRE), 0–8. 0 white, 8 voice colour. Encoder 3 after a second click. Above 8 loads as 8 |

Named constants for 38–41 are `EEPROM_MIX_GAIN_*` in `toern.ino`. Offsets 42 through 47 are raw bytes at `EEPROM_DATA_START + offset`.
