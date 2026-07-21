---
sidebar_position: 3
title: Synths
description: Poly channel 11 and mono channels 13–14.
---

# Synths

Synth code lives mainly in [`toern_synths.ino`](https://github.com/Soundpauli/toern/blob/main/toern_synths.ino), with graph nodes in `audioinit.h` and defaults/presets in `toern_parameters.ino`.

## Channel 11 — poly

- Up to `POLY_VOICES` (3) simultaneous voices  
- Multi-oscillator stacks per voice (`Swaveform*`, mixers, ladder filters, envelopes)  
- Instrument presets: BASS, KEYS, CHPT, PAD, WOW, ORG, FLT, LEAD, ARP, BRSS  
- Preset application: `applySynthInstrumentPreset` / `*_synth(...)` helpers  

## Channels 13–14 — mono

- Dual oscillators each (`waveform13_*`, `waveform14_*`)  
- ADSR, LFO, arpeggiator parameters in synth/filter setting tables  
- Share the crush → filter → reverb pattern with other instruments  

## Voice lifecycle

| Function | Role |
|----------|------|
| `playSound(note, ch)` | Start a voice for a sequencer/MIDI note |
| `stopSound(note, ch)` | Release / stop |
| `updateSynthVoice(ch)` | Push current settings into Audio objects |
| `switchSynthVoice(...)` | Change instrument type with parameter bundle |
| `autoOffActiveNotes()` | Housekeeping for hanging voices |

Voice timing/state arrays (`voiceStartTime`, sustain levels, resonance, …) sit at the top of `toern_synths.ino`.

## Interaction with FX

Bitcrush / filter / reverb for synth channels are the same conceptual FX slots as samples, applied through `setFilters` / mixer gain helpers so UI sliders stay consistent across channel types.
