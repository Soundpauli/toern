---
sidebar_position: 3
title: Menu system
description: Nested menu state machine and settings surfaces.
---

# Menu system

[`toern_menu.ino`](https://github.com/Soundpauli/toern/blob/main/toern_menu.ino) implements the on-device settings UI entered via the `MENU` mode.

## Structure

Top-level pages fan into submenus flagged by booleans such as:

- `inLookSubmenu`
- `inRecsSubmenu`
- `inMidiSubmenu`
- `inVolSubmenu`
- `inEtcSubmenu`

MODE, ETC and ETC → ADV share `inEtcSubmenu` and `currentEtcPage`; `currentSystemMenuGroup` selects the active page array. `systemMenuParentPage()` restores MODE or ETC when leaving. Each other family has a `current*Page` index. Exiting restores draw vs single via `menuEnteredFromSingleMode`.

## Notable ETC behavior

**Menu → ETC → SD** toggles the USB serial SD server (`toern_sd_serial.ino`). `loop()` watches `inEtcSubmenu` + the active menu setting id and calls `sdSerialServerSetActive` / `sdSerialServerPoll`.

## Menu hierarchy

- **SETT:** VIEW, CTRL, DRAW, MUTE.
- **MODE:** FLOW, PMOD, LOOP, VMOD, CHLD, IMG, PONG, SONG, AUTO. MODE occupies the former SONG root slot and retains its icon and colour. Press encoder 4 on SONG to open the existing arranger.
- **ETC:** INFO, SD, BATT, ADV, RSET.
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
