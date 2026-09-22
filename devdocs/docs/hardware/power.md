---
sidebar_position: 3
title: Power
description: USB-C, LiPo charger, regulators, switch, and load-switched accessory rail.
---

# Power (rev H)

## Sources

1. **USB-C (`J4`)** — VBUS into the charger / power-path IC; D+/D− to the Teensy USB pads.  
2. **LiPo (`J16`)** — VBAT / GND into **BQ24075** (`IC1`).

The BQ24075 provides USB-friendly charging and power-path management (system rail while charging). Status LEDs **D4** (red) / **D5** (yellow) hang off charger status pins.

## Main switch

**EG2219 (`S1`)** sits between the charger system output path and the board **+5V** distribution used by Teensy VIN-side power, encoder 5V, PAM8403, etc.

`JP1` is a 3-pad solder jumper and it **ships open**. The center pad goes to the “on” side of **S1**. The switch common is the board **+5V** rail (Teensy VIN, the LDOs, encoder 5 V, PAM8403). With no bridge, sliding the switch does nothing and the rail stays dead.

Bridge **one** side only:

| Bridge | What runs |
|--------|-----------|
| Center ↔ **VYS** (pad B, the end opposite the pin-1 mark) | Normal use. +5V comes from the BQ24075 system output. USB and the LiPo both feed the board through the charger, and **S1** switches that rail. |
| Center ↔ **VBUS** (pad A, the pin-1 end) | USB only. +5V is raw USB 5 V, ahead of the charger output. Unplugging USB kills the rail even if a battery is connected. Useful while checking the board before you trust the charger path. |

Bridging both ends shorts USB 5 V to the charger output.

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

- **1M (`R15`)** top (VBAT → A16), **1.5M (`R19`)** bottom (A16 → GND), plus smoothing cap  
- Nominal divider ratio: `1.5 / (1.0 + 1.5) = 0.600`; firmware uses calibrated `0.595`

See [Firmware pin map](./firmware-pins) for the ADC constants.

## Speaker amp

**PAM8403 (`U13`)** runs from **+5V**, takes analog feed from the audio path, and drives **`J10`**. Shutdown / control ties into Teensy GPIO (net `30_CRX3` on the enable-related pin in the PCB netlist).
