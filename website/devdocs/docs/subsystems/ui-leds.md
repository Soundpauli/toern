---
sidebar_position: 1
title: UI & LEDs
description: Matrix drawing, encoder RGB, and optional strip ripples.
---

# UI & LEDs

The 16×16 (optionally dual-module) matrix is both display and note grid. Drawing primitives live in `toern_ui.ino`; optional external strip effects in `toern_leds.ino`.

## Matrix primitives

| Function | Role |
|----------|------|
| `light(x, y, color)` | Set a logical grid pixel |
| `light_single(...)` | Address a specific module |
| `drawBase` / `drawCursor` / `drawPages` / `drawStatus` | Compose the main sequencer view |
| `drawIndicator` | Encoder-related on-matrix cues |
| `drawNumber` / overlays | Transient values (volume, channel, load %) |

Colors come from `colors.h` and mode `knobcolor[]` values.

## FastLED constraints

- `FASTLED_ALLOW_INTERRUPTS 0` — LED show must not nest badly with audio.  
- Prefer **one** show per frame from the main loop path.  
- WS2812 data pin and module count: `DATA_PIN`, `LED_MODULES`, runtime `maxX`.

## Encoder RGB

`i2cEncoderLibV2` LEDs mirror mode colors and status (e.g. blink helpers `triggerExternalOneBlink`). I2C writes belong in `loop()`, not in the playback ISR.

## External strip

`toern_leds.ino` maintains an optional WS2812 strip: length/enable settings, `triggerLedStripRipple(channel)` on note triggers, `updateLedStrip()` from the main loop.
