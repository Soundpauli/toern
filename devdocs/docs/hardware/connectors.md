---
sidebar_position: 2
title: Connectors & pinouts
description: Audio jacks, MIDI, USB-C, microSD, and JST expansion headers on toern_revH.
---

# Connectors & pinouts (rev H)

Pin tables below follow the **rev H netlist** (`toern_revH.net`). Teensy pin names use the schematic global-label style (`18_A4_SDA` = digital pin 18 / A4 / SDA, etc.). The [schematic PDF](../../../PCB/toern_revH/schematic.pdf) wins if a silk label disagrees.

For *which jack is which on the finished box*, see the handbook hardware page — this doc is the electrical map.

## Panel audio / MIDI / USB

| Ref | Footprint (BOM) | Role |
|-----|-----------------|------|
| **J4** | USB-C receptacle | Host USB (D+/D−, VBUS) |
| **J1** | 6.35 mm Neutrik-style | Headphone out from SGTL5000 HP_L / HP_R / HP_VGND |
| **J5**, **J6** | 6.35 mm | Line-level paths (AC-coupled nets into/out of the codec side) |
| **J7** | 6.35 mm | Mic / related input path (tip tied toward mic capsule net; sleeve GND) |
| **J3**, **J17** | CUI SJ1-3533 (3.5 mm) | TRS MIDI in/out (opto / serial side via TLP2361 etc.) |
| **MK1** | POM-2244P condenser | Built-in mic → codec MIC / MICBIAS path |
| **J8** | Hirose microSD | SPI SD: CS=`10`, MOSI=`11`, MISO=`12`, SCK=`13`, VDD=+3.3V |

Exact silk labels on a built unit may use friendly names (LINE OUT, MIDI IN, …); match by connector family and position on the edge.

## Power / speaker

| Ref | Type | Pins (PCB) |
|-----|------|------------|
| **J16** | JST-PH 1×02 | `1` GND · `2` VBAT — LiPo |
| **J10** | JST-PH 1×02 | Speaker outputs from PAM8403 (`U13`) |
| **J9** | JST-PH 1×03 | Matrix / strip: pin 1 = Teensy **17** through **R20** (220 Ω), pin 2 = GND, pin 3 = switched 5 V from **TPS22918** `U8` (enable is Teensy **36**) |
| **S1** | EG2219 slide switch | Main power path between charger system rail and +5V distribution |

## Encoder sockets (I2C RGB encoders)

Several **1×05 pin sockets** share the same bus (examples: `J2`, `J18`–`J24`):

| Pad | Net |
|-----|-----|
| 1 | GND |
| 2 | +5V |
| 3 | `18` SDA |
| 4 | `19` SCL |
| 5 | `27` — encoder interrupt (`INT_PIN` in firmware) |

These match Duppa / i2cEncoderLibV2-style modules used in firmware.

## Analog clock / sidechain (JST-PH 1×02)

Silk on rev H. Pads: **1** = signal, **2** = GND.

| Silk | Ref | Teensy pin | Path |
|------|-----|------------|------|
| **CLK-OUT(5V)** | **J25** | **37** | MCU → 74AHCT1G125 (`U15`, 5 V) → 1 kΩ → jack |
| **CLK-IN(3-5V)** | **J26** | **38** | Jack → 22 kΩ → Schottky clamp (`D2`) → 74LVC1G17 Schmitt (`U14`, 3.3 V) → MCU |
| **SIDEC-5V** | **J27** | **28** | MCU → 74AHCT1G125 (`U16`, 5 V) → 1 kΩ → jack |

Firmware: MENU → MIDI → PPQN drives **J25** (rate / width / polarity / STOP). MENU → MIDI → **CIN** sets the **J26** rate (1–32 PPQN) and hold-off. The pin-38 interrupt is attached only in BPM orange EXT and then calls `playNote()` / `playFillNote()` directly; `fillTimer` stays stopped, and `playTimer` only runs as a pulse-relocked one-shot that spaces the extra steps at 1/2 PPQN. Rev H **D2** (BAT54S) is placed with common and cathode swapped, which holds U14's input at 3.3 V. Remove it, or refit with pin 1 = GND, pin 3 = signal, pin 2 = +3.3 V. Without D2 the input is only safe up to 5 V. MENU → MIDI → **SIDE** picks which voice pulses **J27** (same width/polarity as PPQN; OFF disables).

Do not use **J11** pin 31 for clock — that pad is unused GPIO.

## Expansion JST-PH 1×05

| Ref | Pad 1 | Pad 2 | Pad 3 | Pad 4 | Pad 5 |
|-----|-------|-------|-------|-------|-------|
| **J11** | `24` SCL2 | `25` SDA2 | `31` CTX3 | **+3.3V** | GND |
| **J11.5** | `24` SCL2 | `25` SDA2 | `31` CTX3 | **+5V** | GND |
| **J12** | `22` | `5` IN2 | `15` | +3.3V | GND |
| **J13** | `2` OUT2 | `4` BCLK2 | `3` LRCLK2 | `6` | `39` |
| **J14** | `14` | `9` | `16` | +3.3V | GND |
| **J15** | `33` MCLK2 | `32` | `41` | +3.3V | GND |

**J11** / **J11.5** are I2C2 + pin 31 expansion, not the analog clock jacks (those are **J25–J27** above).

## Codec I²C

SGTL5000 (`U3`) control lines:

- SDA → Teensy `18`  
- SCL → Teensy `19`  

Same I²C bus as the encoder chain (distinct 7-bit addresses).

## Schematic PDF

Full readable schematic: [`PCB/toern_revH/schematic.pdf`](../../../PCB/toern_revH/schematic.pdf).
