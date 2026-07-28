---
sidebar_position: 3
title: Power
description: USB-C, LiPo charger, regulators, switch, and load-switched accessory rail.
---

# Power (rev G)

## Sources

1. **USB-C (`J4`)** — VBUS into the charger / power-path IC; D+/D− to the Teensy USB pads.  
2. **LiPo (`J16`)** — VBAT / GND into **BQ24075** (`IC1`).

The BQ24075 provides USB-friendly charging and power-path management (system rail while charging). Status LEDs **D4** (red) / **D5** (yellow) hang off charger status pins.

## Main switch

**EG2219 (`S1`)** sits between the charger system output path and the board **+5V** distribution used by Teensy VIN-side power, encoder 5V, PAM8403, etc.

`JP1` is a 3-pad solder jumper between **VBUS**, a center node, and **VYS** (charger SYS) — for alternate power wiring / bring-up; leave as designed unless you know you need to change it.

## Regulated rails

| Regulator | Part | Rail |
|-----------|------|------|
| `U1`, `U7` | SPX3819 3.3V | `+3.3V` (logic / SD / parts of codec digital) |
| `U2` | SPX3819 1.8V | `+1V8` (SGTL5000) |
| Codec analog | derived | `+3.3VA` net for SGTL analog supply |

Keep digital and analog returns as in the layout — don’t casually short `+3.3V` and `+3.3VA` plane strategy when editing copper.

## Load switch (accessory / strip)

**TPS22918 (`U8`)**:

- Input from **+5V**  
- Enable from Teensy pin **`36`**  
- Switched output feeds **`J9`** (with GND)

Firmware can gate power to an external WS2812 strip / accessory so it isn’t drawing when disabled.

## Battery sense (firmware)

Firmware reads battery through **`A16` (pin 40)** with a divider documented in `toern.ino`:

- Target: **1.5M** top (VBAT → A16), **1M** bottom (A16 → GND), plus smoothing cap  
- BOM includes **1.5M (`R19`)** and **1M (`R15`)** matching that intent  

See [Firmware pin map](./firmware-pins) for the ADC constants.

## Speaker amp

**PAM8403 (`U13`)** runs from **+5V**, takes analog feed from the audio path, and drives **`J10`**. Shutdown / control ties into Teensy GPIO (net `30_CRX3` on the enable-related pin in the PCB netlist).
