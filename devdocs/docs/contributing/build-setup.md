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

## PlatformIO

The repository includes a pinned PlatformIO environment for Teensy 4.1:

```bash
pio run                         # build .pio/build/teensy41/firmware.hex
pio run --target upload         # build and open the Teensy upload flow
pio device monitor              # 115200 baud USB serial monitor
pio run --target compiledb      # generate compile_commands.json for IDE/AI tooling
```

The environment uses **Faster** optimization and the **MIDI + Serial** USB type.
Serial is retained for diagnostics; USB Audio is not enabled. Its pre-build
script applies the project's sampler, amplitude-ramping, and resampler changes
to the pinned upstream audio libraries, so no manually edited Arduino libraries
are needed.

## Open the sketch

Open `toern.ino` from the repo root (the folder containing all `toern_*.ino` files). Arduino treats that folder as one sketch.

## USB type

For MIDI + serial SD tool use a USB type that includes **Serial** (and MIDI if you want USB MIDI). The SD serial server and debug prints need the serial interface.

## Docs site (this site)

Sources live in repo-root **`devdocs/`** (same pattern as `handbook/`). Publish details: [This docs site](./docs-site).

```bash
cd devdocs
npm install
npm start
```

Node **≥ 20** required (see `package.json` engines).

To produce the static files served under `/docs/`:

```bash
cd devdocs
npm run build
# then copy build/ → website/docs/ for deploy
```

## Operator handbook

End-user docs are static HTML under `handbook/` — separate from this Docusaurus tree.