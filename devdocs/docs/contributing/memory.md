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
| RAM1 / DTCM | (default) | Hot DSP state, mixers, envelopes and reverb tanks; instruction-bank allocation reduces available data/stack space |
| RAM2 | `DMAMEM` | DMA-friendly buffers, some audio/FX state |
| PSRAM | `EXTMEM` | Large grids, `Device SMP`, sample PCM and delay storage |

## Patterns already used

- `EXTMEM Note note[...]` — pattern grid
- `EXTMEM Device SMP` — project/session bag
- Internal DTCM for hot oscillators/ladder state, small mixers, routing and sampler controllers in `audioinit.h` / `toern.ino`
- `DMAMEM` for some float tables (`detune`, `channelOctave`, …)
- Seven Treeverb tank allocations in RAM1; the default balanced profile has six combs and four diffusers per instance

The balanced reverb build leaves **43,360 bytes** of RAM1 free for locals with
MCP included, or **44,448 bytes** with MCP excluded. RAM2 free for heap is
**325,664 / 327,200 bytes** respectively. These are linked build figures, not
runtime stack high-water measurements. The preceding light reverb left about
91.9 KB of RAM1 free; full 8/4 diffusion would leave only a few KB in this layout.
See [reverb profiles and validation](../audio/effects#treeverb). Runtime MCP session
gating skips work but retains its compiled state/frame buffer; only the disabled
build removes those allocations.

## Anti-patterns

- Large `static` locals inside functions → still land in RAM1
- Growing `String` members in big tables → heap + RAM pressure (slider names were moved to PROGMEM index tables for this reason)
- Duplicating the note grid “temporarily” on the stack/DTCM

## When you add features

Ask: does this buffer need single-cycle DTCM access every sample? If not, prefer `EXTMEM`/`DMAMEM`. If you have `_internal-docs/RAM1_OPTIMIZATIONS.md` locally, it lists concrete reclaim wins already applied.
