---
slug: /
sidebar_position: 1
title: Introduction
description: Developer docs for how TŒRN firmware is structured — not the device handbook.
---

# TŒRN code docs

These pages explain **how the firmware is organized** and how the major pieces cooperate: sequencer data, UI modes, the Teensy Audio graph, MIDI/clock, and SD I/O — plus **hardware design docs** for the current PCB (`toern_revG`).

If you want to *use* the device (first beat, controls, sequencing), open the **[operator handbook](https://github.com/Soundpauli/toern/tree/main/handbook)** instead.

## What you are looking at

TŒRN runs on a **Teensy 4.1** as an Arduino multi-file sketch:

| Layer | Role |
|-------|------|
| `toern.ino` | Globals, mode state, setup/loop, playback timers, most interaction glue |
| `toern_*.ino` | Focused subsystems (UI draw, menu, MIDI, samples, …) compiled into one sketch |
| `*.h` | Shared tables (notes, colors, fonts) and the audio object graph (`audioinit.h`) |
| `src/` | Forked/custom audio code (resampler, DMA freeverb) |

Arduino concatenates all `.ino` files in the sketch folder; headers and `src/` are normal includes/compile units.

## Start here

1. [Architecture overview](./architecture/overview) — mental model of the whole system  
2. [File map](./architecture/file-map) — which file owns which concern  
3. [Main loop](./architecture/main-loop) — ISR vs loop work (timing-critical path)  
4. [Modes](./core/modes) — how encoders and screens switch behavior  
5. [Hardware overview (rev G)](./hardware/overview) — KiCad board, connectors, power  

## Explicit non-goals

- Device operation / music-making tutorials → handbook  
- Step-by-step “build the kit” / faceplate tour → handbook hardware page (these docs are design/pinout oriented)  
- Marketing / shop site → `website/` (local, not this docs tree)
