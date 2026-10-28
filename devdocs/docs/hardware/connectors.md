---
sidebar_position: 2
title: Connectors & pinouts
description: Audio jacks, MIDI, USB-C, microSD, and JST expansion headers on toern_revH.
---

# Connectors & pinouts (rev H)

Pin tables below follow the **rev H netlist** (`toern_revH.net`). Teensy pin names use the schematic global-label style (`18_A4_SDA` = digital pin 18 / A4 / SDA, etc.). The [schematic PDF](https://github.com/Soundpauli/toern/blob/main/PCB/toern_revH/schematic.pdf) wins if a silk label disagrees.

For *which jack is which on the finished box*, see the handbook hardware page — this doc is the electrical map.

## Panel audio / MIDI / USB

| Ref | Footprint (BOM) | Role |
|-----|-----------------|------|
| **J4** | USB-C receptacle | Host USB (D+/D−, VBUS) |
| **J1** | 6.35 mm Neutrik-style | Headphone out from SGTL5000 HP_L / HP_R / HP_VGND |
| **J5** | 6.35 mm stereo TRS | Line out: tip → C17 → LINEOUT_L; ring → C16 → LINEOUT_R; sleeve GND |
| **J6** | 6.35 mm stereo TRS | Line in: tip → C19 → LINEIN_L; ring → C18 → LINEIN_R; sleeve GND. Firmware records input channel 0 (left) only. |
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
| **J9** | JST-PH 1×03 | Matrix: pin 1 = Teensy **17** through **R20** (220 Ω), pin 2 = GND, pin 3 = switched 5 V from **TPS22918** `U8` (enable is Teensy **36**) |
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

## Remote panel firmware

The single selector `exttouch` in `toern_hardware.h` controls the entire panel:
`false` selects capacitive pads without faders; `true` selects external buttons,
faders, and panel I²C timing. There is no separate
fader flag. With `exttouch=true`, grounded active-low buttons A/B/C/D use Teensy pins
5/22/15/14 respectively. D is touch5 / FN (swapped with touch4). Fill is C+D.
Touch4 / audition has no external button. In REC, D is the touch5 trigger, not
the threshold pad.
Button D is on **J14-1** (pin 14). The LED strip uses **J11.5-1** (pin 24).
The matrix stays on pin 17 / J9-1. Strip operation remains available with
external buttons enabled. The same `exttouch` flag enables the four NeoSliders at 0x30–0x33.

The codec, encoders and faders share primary `Wire`. Remote-panel builds apply
approximately 35 kHz timing; the standard panel stays
at 100 kHz. Timing is applied once per primary `Wire.begin()` by the wrapper,
without additional reapplications after encoder or fader operations. The explicit `Wire.begin()` call in setup remains
commented out; codec initialization and each seesaw probe call it internally,
restoring 100 kHz. The canonical PlatformIO build wraps
`TwoWire::begin()` via `toern_i2c.cpp` and the linker flag in `platformio.ini`,
restoring slow timing before any subsequent transaction. A periodic timer or
restoring timing after seesaw initialization leaves its first probe exposed.
The same wrapper rejects primary-bus transfers until that internal initialization
completes, so early kit/mode encoder writes cannot access an uninitialized
peripheral. `initEncoders()` writes its configuration afterward. Other build
systems must include that source and all three linker flags too.

A matrix stuck showing kit `no.1` does not identify the failing operation:
that frame can remain visible during subsequent initialization. USB serial
(`pio device monitor`, 115200) reports `BOOT` stages for samples, autoload,
codec, encoders and each fader probe. `BOOT codec enabled` only confirms the
library's `enable()` call returned; volume/input/HPF/output setup follows it.
`BOOT codec configured` confirms those remaining calls returned. Codec substep
markers and `audio settings` / `voice settings` markers locate failures before
`BOOT encoders`. Missing faders are skipped after bounded
library retries. Long-cable electrical reliability still requires testing on
the assembled unit; firmware timing alone does not establish the cause of
an intermittent hardware failure.

## Analog clock / sidechain (JST-PH 1×02)

Silk on rev H. Pads: **1** = signal, **2** = GND.

| Silk | Ref | Teensy pin | Path |
|------|-----|------------|------|
| **CLK-OUT(5V)** | **J25** | **37** | MCU → 74AHCT1G125 (`U15`, 5 V) → 1 kΩ → jack |
| **CLK-IN(3-5V)** | **J26** | **38** | Jack → 22 kΩ → Schottky clamp (`D2`) → 74LVC1G17 Schmitt (`U14`, 3.3 V) → MCU |
| **SIDEC-5V** | **J27** | **28** | MCU → 74AHCT1G125 (`U16`, 5 V) → 1 kΩ → jack |

Firmware: MENU → MIDI → PQ-O drives **J25** (rate / width / polarity / STOP). MENU → MIDI → **PQ-I** sets **J26** OFF/rate (1–32 PPQN), hold-off, and input polarity (encoder 3 click: + rising / − falling). PQ-I is immediately before PQ-O with matching readouts. The pin-38 interrupt is attached only in BPM orange EXT with PQ-I enabled and then calls `playNote()` / `playFillNote()` directly; `fillTimer` stays stopped, and `playTimer` only runs as a pulse-relocked one-shot that spaces the extra steps at 1/2 PPQN. **D2** is BAT54S in standard SOT-23 (`toern:BAT54S`: pin 1 anode to GND, pin 2 cathode to +3.3 V, pin 3 common to the clamp node). Early rev H boards used KiCad `D_Schottky_Dual_Series_ACK` (pin 2 = common, pin 3 = cathode), which pulled U14's input to 3.3 V; those boards need D2 removed or bodged. MENU → MIDI → **TRIG** picks which voice pulses **J27** (positive pulse, idle LOW; width follows PQ-O; OFF disables).

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

Full readable schematic: [`PCB/toern_revH/schematic.pdf`](https://github.com/Soundpauli/toern/blob/main/PCB/toern_revH/schematic.pdf).

## Audio routing versus connector wiring

J1, J5 and J6 are stereo TRS connectors. Normal playback sends the main mono mix to both output channels; `VOL → 2-CH` can separate main/preview or sample banks. Those output mixers also feed USB audio. The current onboard recording path reads the left line-input channel only and writes mono WAVs. See [Audio graph](../audio/graph) and the [recording handbook](https://handbook.toern.live/record.html).
