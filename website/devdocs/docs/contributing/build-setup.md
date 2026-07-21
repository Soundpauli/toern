---
sidebar_position: 1
title: Build setup
description: How to build and flash the Teensy firmware.
---

# Build setup

## Requirements

- [Arduino IDE](https://www.arduino.cc/) or [`arduino-cli`](https://arduino.github.io/arduino-cli/) with **Teensyduino** support  
- Teensy 4.1 board support  
- Libraries referenced from `toern.ino` / README, including:  
  - Teensy Audio  
  - FastLED + WS2812Serial  
  - MIDI  
  - i2cEncoderLibV2  
  - FastTouch  
  - TeensyPolyphony  
  - Mapf  

Custom audio sources in `src/` compile with the sketch — no extra package step.

## Open the sketch

Open `toern.ino` from the repo root (the folder containing all `toern_*.ino` files). Arduino treats that folder as one sketch.

## USB type

For MIDI + serial SD tool use a USB type that includes **Serial** (and MIDI if you want USB MIDI). The SD serial server and debug prints need the serial interface.

## Docs site (this site)

```bash
cd devdocs
npm install
npm start
```

Node **≥ 20** required (see `package.json` engines).

## Operator handbook

End-user docs are static HTML under `handbook/` — separate from this Docusaurus tree.
