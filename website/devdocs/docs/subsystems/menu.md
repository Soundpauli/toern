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

1. Allocate a setting id / page slot in the menu tables.  
2. Handle encoder adjust + button confirm in the menu update path.  
3. Persist if needed (EEPROM and/or settings file).  
4. Ensure drawing covers the new label on the matrix font/icon set.
