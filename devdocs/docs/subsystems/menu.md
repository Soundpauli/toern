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

Each has a `current*Page` index. Exiting restores draw vs single via `menuEnteredFromSingleMode`.

## Notable ETC behavior

**Menu → ETC → SD** toggles the USB serial SD server (`toern_sd_serial.ino`). `loop()` watches `inEtcSubmenu` + the active menu setting id and calls `sdSerialServerSetActive` / `sdSerialServerPoll`.

## Settings backup

Menu changes can mark settings dirty (`markSettingsBackupDirty`) and later flush via `serviceSettingsBackup()` from `loop()` — keeping EEPROM/SD writes off the audio ISR path.

## Adding a menu item

The file list and the EEPROM offset table are in [Firmware edits](../contributing/firmware-edits). Short version: a free `mainSetting` id, a `MenuPage` row, a draw `case`, an encoder `case`, and a new EEPROM offset at the end of the settings block if it must survive reboot.

## Current labels

Root families: **PATT, KIT, SMP, BPM, VOL, SETT, REC, MIDI, SONG, ETC**. MIDI uses **SEL**, **FOLD**, **OVWR** and **TRIG** for voice selection, pitch folding, occupied-cell overwrite and selected-voice analog trigger output. REC → TRIG is the existing recording-trigger setting, a separate control. Internal identifiers and EEPROM offsets retain their historical names.

A green animated checkmark confirms a successful manual pattern or kit save; a red animated cross signals failure. PATT slot occupancy is communicated by colour, with no USED/NEW status text. The NEW generator remains a separate screen when loading an empty numbered slot.
