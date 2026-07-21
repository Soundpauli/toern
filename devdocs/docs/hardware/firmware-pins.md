---
sidebar_position: 4
title: Firmware pin map
description: How toern.ino pin defines line up with toern_revG nets.
---

# Firmware ↔ rev G pin map

Defines live in [`toern.ino`](https://github.com/Soundpauli/toern/blob/main/toern.ino). Net names below are from the rev G PCB.

## Core defines

| Firmware | Teensy pin | Board use |
|----------|------------|-----------|
| `DATA_PIN` | **17** | WS2812 / matrix data (`17_A3_TX4_SDA1` net family) |
| `INT_PIN` | **27** | Shared encoder interrupt (pad 5 on encoder sockets) |
| `INT_SD` | **10** | microSD chip-select |
| `SWITCH_1` | **2** | Touch / TTP223 — single mode |
| `SWITCH_2` | **3** | Touch — menu |
| `SWITCH_3` | **4** | Touch — record |
| `SWITCH_4` | **6** | Extra switch net (board-dependent) |
| `SWITCH_5` | **39** | Extra switch net (board-dependent) |
| `BATT_ADC_PIN` | **A16** (40) | VBAT divider |

MIDI on **Serial8** uses pins **34/35** (`RX8`/`TX8`) with enlarged buffers — TRS jacks go through the opto / buffer stage (`TLP2361` `U4`, Schmitt `74LVC2G14` `U5`).

## Buses (by convention)

| Bus | Pins | Devices |
|-----|------|---------|
| I²C0 | 18 / 19 | Encoders + SGTL5000 control |
| SPI (SD) | 10–13 | microSD `J8` |
| I²S (codec) | (Audio library defaults on Teensy 4 + SGTL wiring) | `U3` data/clocks — see schematic |
| USB | D+/D− pads on `U6` | Via USB-C `J4` |

## When changing pins

1. Update the PCB net / jumper **and** the `#define`s.  
2. Re-check encoder INT (must be common open-drain style as wired).  
3. Re-check SD CS vs any other SPI CS uses.  
4. Battery divider ratios in firmware must match the assembled resistors.
