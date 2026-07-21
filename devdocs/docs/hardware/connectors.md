---
sidebar_position: 2
title: Connectors & pinouts
description: Audio jacks, MIDI, USB-C, microSD, and JST expansion headers on toern_revG.
---

# Connectors & pinouts (rev G)

Pin tables below are taken from the **rev G PCB netlist** (`toern_revG.kicad_pcb`). Teensy pin names use the schematic global-label style (`18_A4_SDA` = digital pin 18 / A4 / SDA, etc.).

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
| **J9** | JST-PH 1×03 | Load-switched rail via **TPS22918** (`U8`, enable on Teensy `36`) — typically external LED-strip / accessory 5V |
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

## Expansion JST-PH 1×05

| Ref | Pad 1 | Pad 2 | Pad 3 | Pad 4 | Pad 5 |
|-----|-------|-------|-------|-------|-------|
| **J11** | `24` SCL2 | `25` SDA2 | `31` CTX3 | **+3.3V** | GND |
| **J11.5** | `24` SCL2 | `25` SDA2 | `31` CTX3 | **+5V** | GND |
| **J12** | `22` | `5` IN2 | `15` | +3.3V | GND |
| **J13** | `2` OUT2 | `4` BCLK2 | `3` LRCLK2 | `6` | `39` |
| **J14** | `14` | `9` | `16` | +3.3V | GND |
| **J15** | `33` MCLK2 | `32` | `41` | +3.3V | GND |

Handbook note: **J11** exposes pin **31** (+ GND) for a DIY pulse-clock / trigger cable (MIDI → PPQN). Prefer the **3.3V** header (`J11`) for logic-level accessories unless the peripheral needs 5V (`J11.5`).

## Codec I²C

SGTL5000 (`U3`) control lines:

- SDA → Teensy `18`  
- SCL → Teensy `19`  

Same I²C bus as the encoder chain (distinct 7-bit addresses).

## Schematic PDF

Full readable schematic: [`PCB/toern_revG/schematic.pdf`](https://github.com/Soundpauli/toern/blob/main/PCB/toern_revG/schematic.pdf).
