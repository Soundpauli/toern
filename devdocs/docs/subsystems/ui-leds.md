---
sidebar_position: 1
title: UI & LEDs
description: Matrix drawing, encoder RGB, and optional strip ripples.
---

# UI & LEDs

The 16×16 (optionally dual-module) matrix is both display and note grid. Drawing primitives live in `toern_ui.ino`; optional external strip effects in `toern_leds.ino`.

**Y axis:** row 1 (`y == 1`) is lower than row 16. Down = toward row 1 (decrease `y`). Up = toward row 16 (increase `y`).

## Matrix primitives

| Function | Role |
|----------|------|
| `light(x, y, color)` | Set a logical grid pixel |
| `light_single(...)` | Address a specific module |
| `drawBase` / `drawCursor` / `drawPages` / `drawStatus` | Compose the main sequencer view |
| `drawIndicator` | Encoder-related on-matrix cues |
| `drawNumber` / overlays | Transient values (volume, channel, load %). `drawPageNrOverlay` flashes the page number on the right when CRSR is CHNR and CTRL is PAGE: digits on rows 11–15, no underline. The channel overlay stays on the left. |

Colors come from `colors.h` and mode `knobcolor[]` values.

## FastLED constraints

- `FASTLED_ALLOW_INTERRUPTS 0` — LED show must not nest badly with audio.
- Prefer **one** show per frame from the main loop path.
- WS2812 data pin and module count: `DATA_PIN`, `LED_MODULES`, runtime `maxX`.

## Encoder RGB

`i2cEncoderLibV2` LEDs mirror mode colors and status (e.g. blink helpers `triggerExternalOneBlink`). I2C writes belong in `loop()`, not in the playback ISR.

## External strip

`toern_leds.ino` maintains an optional WS2812 strip: length/enable settings, `triggerLedStripRipple(channel)` on note triggers, `updateLedStrip()` from the main loop.

The strip controller uses **Teensy pin 24 / J11.5 pin 1**, configured by `LED_STRIP_PIN` in `toern.ino`. It uses WS2812Serial independently of the matrix on pin 17. External button D uses pin **14 / J14-1**, so enabling external buttons does not disable the strip. `ETC → LGHT` continues to control strip enablement.

## Fader I²C diagnostics

Fader ADC reads retain the upstream seesaw conversion wait of 500 µs. LED
feedback sends at most one slider update every 20 ms across the panel; failed
buffer/SHOW writes remain pending instead of marking the bar as displayed.
Set `TOERN_FADER_LEDS=0` in `toern_hardware.h` for a comparison build that keeps
ADC/volume control active but skips NeoPixel setup and updates.
For further isolation, `TOERN_FADER_DIAGNOSTIC_MODE` is 0=normal, 1=no runtime
fader polling, 2=ADC reads only (no mute, amplitude, overlay or LED targets).
Modes 1/2 still initialize the faders, retain unity velocity scales, and leave
external buttons active. Use `TOERN_FADER_LEDS=0` with these comparison builds.
Panel speed
remains approximately 35 kHz in either build.

A Teensy fault report survives startup and is printed once when a serial monitor
is attached in `loop()`, without writing to SD. Breadcrumb 1 is `0xFAD0iiSS`:
`ii` is the zero-based fader index; `SS` is 0=idle, 1=ADC, 2=LED buffer,
3=SHOW, 4=amplitude update, 5=mute handling. This records main-loop context;
the report's fault address and program counter identify where the fault occurred.

The ramp mixers additionally validate pointers and block headers against the
configured AudioMemory pool before sample access or release. An invalid block
is discarded without dereferencing/releasing it, and the first incident is
reported on USB serial as `AUDIO BLOCK FAULT` with mixer type, delay index,
input and phase. The header diagnostic also prints the pool base, expected
block index, stored block index, reference count and a `reason` bit mask:
1=outside pool, 2=misaligned, 4=index mismatch, 8=zero reference count.
Header values are only read for an aligned pointer inside the pool; otherwise
index/reference fields use sentinel values 65535/255. Phase 1 is receipt from the input queue, 2 is rechecking after
output allocation, 3 is an invalid allocator result (input 255). Breadcrumbs
2/3/4 record bad pointer, mixer address, and `(phase << 8) | input` if a later
processor fault occurs. This contains a detected invalid pointer; it does not
repair or establish the source of memory corruption. Do not interpret these
fields using older diagnostic builds, which only wrote breadcrumb 1.

## Current display contract

PATT shows the slot number and load/save indicators. Occupied slots use green numbers; empty slots use blue. There is no USED/NEW text. `showSaveSuccessAnimation` draws a green checkmark; `showSaveFailureAnimation` draws a red cross. Both yield during animation to keep audio service running.

In note detail, the condition fraction belongs under encoder 3 and voice volume under encoder 4 in both panel layouts. The matrix is a display controlled by the encoders and touch pads, not a matrix of physical playable buttons.
