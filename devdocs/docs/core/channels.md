---
sidebar_position: 3
title: Channels
description: Sample vs synth channel numbering and how playback dispatches.
---

# Channels

The sequencer speaks in **channel numbers** that map to audio engines. Grid notes store that id in `Note.channel`.

## Map

| Channel | Kind | Audio entry points |
|---------|------|--------------------|
| **1–8** | Sample voices | `sound1`…`sound8` (`AudioPlayArrayResmp`) + per-voice envelope → bitcrush → filter → freeverb path |
| **11** | Poly synth (3 voices) | Oscillator banks `Swaveform*` / mixers / ladder filters in `audioinit.h`; voice logic in `toern_synths.ino` |
| **13–14** | Mono synths | `waveform13_*` / `waveform14_*` chains with LFO/arp support |

Other indices in `maxY`/`NUM_CHANNELS` space exist for UI/param tables; musical triggers focus on the rows above.

## Dispatch

Playback and MIDI note handlers ultimately call into sample trigger paths or:

- `playSound(note, ch)` / `stopSound(note, ch)`  
- `updateSynthVoice(channel)`  
- `switchSynthVoice(...)` for preset morphing  

in `toern_synths.ino`, after consulting mute flags and child-lock restrictions.

## Per-channel parameters

Stored mostly in `SMP.filter_settings`, `SMP.param_settings`, and `SMP.synth_settings`, applied by:

- `setFilters(...)` — `toern_filter.ino`  
- `setParams(...)` — `toern_parameters.ino`  
- Filter UI pages — `toern_filterUI.ino`  

Display values are often **0–32** on the matrix even when the underlying float ranges differ (see local `_internal-docs/VALUE_RANGES.md` if present).

## Mixing / headroom

Sample channels 1–4 and 5–8 sum through separate buses before the end mixer. Headroom constants in `toern.ino` (`MIX_BUS_HEADROOM`, `MIX_END_*`) document the intended simultaneous-voice budget — change them carefully if you hear clipping when many voices fire together.
