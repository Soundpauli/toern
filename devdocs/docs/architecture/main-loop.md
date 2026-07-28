---
sidebar_position: 3
title: Main loop & timers
description: What runs in ISRs vs loop — and why that split matters.
---

# Main loop & timers

Playback must stay steady even when the UI is busy (drawing LEDs, talking I2C to encoders, reading the SD card). The firmware splits work into **timer ISRs** and **`loop()`**.

## Timers

Declared in `toern.ino`:

| Timer | Job |
|-------|-----|
| `playTimer` | Advance sequencer / trigger notes (`playNote` path) |
| `midiClockTimer` | MIDI clock related timing |
| `fillTimer` | Fill triggers at a multiple of beat rate |

`playNote()` is ISR-facing: keep it short. No Serial spam, no SD, no I2C encoder RGB writes, no `FastLED.show()` inside the hot path.

## Main `loop()` responsibilities

Rough order of concerns (see `void loop()` in `toern.ino`):

1. Optional SD serial server poll when Menu → ETC → SD is open  
2. MIDI input (`checkMidi`) early for latency  
3. Deferred settings backup / pause UI / autosave flags  
4. Encoder + button + touch handling (`checkEncoders`, `checkButtons`, `checkTouchInputs`, …)  
5. Mode-specific UI updates and drawing  
6. A single (or carefully limited) LED refresh per frame  

Flags such as `pendingPauseUIUpdate` / `pendingPauseAutoSave` exist so pause can return quickly from timing-sensitive code and finish I2C/SD on a later iteration.

## ISR → loop handoff pattern

```text
ISR sets volatile flag / queues minimal data
        │
        ▼
loop() notices flag
        │
        ▼
does Serial / I2C / SD / FastLED / autosave
```

Example: play-button LED redraw is deferred via `isrPlayButtonTick` so the ISR never blocks on matrix updates.

## SD + audio coexistence

Sample load and file ops call `sdIoBeginAudioSafe()` / `sdIoYield()` / `sdIoEndAudioSafe()` (see `toern_sample.ino`) so the Audio library keeps getting CPU while the SD card stalls.

## Practical rules when editing

1. **If it must happen on the beat** → keep it in the timer path, but only the minimal trigger.  
2. **If it touches hardware slowly** (I2C, SD, Serial, LEDs) → do it from `loop()`.  
3. **Avoid nested `FastLED.show()`** inside draw helpers — prefer one show per frame (see historical notes in `_internal-docs/OPTIMIZATION_PLAN.md` locally).  
4. **Don’t add `yield()` per pixel** in `light()`-style helpers; yield at frame/block granularity.
