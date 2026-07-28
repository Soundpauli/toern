---
sidebar_position: 2
title: Memory
description: RAM1 vs DMAMEM vs EXTMEM placement on Teensy 4.1.
---

# Memory

Teensy 4.1 memory is easy to exhaust in **RAM1 (DTCM)** even when PSRAM is free. The firmware deliberately parks large structures in slower memories.

## Regions (practical view)

| Region | Keyword | Use for |
|--------|---------|---------|
| RAM1 / DTCM | (default) | Hot code, small frequently touched state |
| RAM2 | `DMAMEM` | DMA-friendly buffers, some audio/FX state |
| PSRAM | `EXTMEM` | Large grids, `Device SMP`, bulky Audio objects |

## Patterns already used

- `EXTMEM Note note[...]` — pattern grid  
- `EXTMEM Device SMP` — project/session bag  
- `EXTMEM` on many Audio mixers/oscillators in `audioinit.h`  
- `DMAMEM` for some float tables (`detune`, `channelOctave`, …)  
- Freeverb DMA buffers via `effect_freeverb_dmabuf`

## Anti-patterns

- Large `static` locals inside functions → still land in RAM1  
- Growing `String` members in big tables → heap + RAM pressure (slider names were moved to PROGMEM index tables for this reason)  
- Duplicating the note grid “temporarily” on the stack/DTCM  

## When you add features

Ask: does this buffer need single-cycle DTCM access every sample? If not, prefer `EXTMEM`/`DMAMEM`. If you have `_internal-docs/RAM1_OPTIMIZATIONS.md` locally, it lists concrete reclaim wins already applied.
