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
- Fifth filter page: LFO (cutoff/res on SVF + ladders) and MIDI arpeggiator (same rules as 13–14)

## Channels 13–14 — mono

- Dual oscillators each (`waveform13_*`, `waveform14_*`)
- ADSR; LFO (rate/depth) continuously modulates cutoff and resonance
- MIDI arpeggiator (ARP step + SPAN): holds while Note On is down; does not transpose sequencer/grid notes; SINGLE record writes the sounded pitches
- Each has its own crush/filter path; neither mono synth has a reverb send

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

Channel 11 feeds its shared bitcrusher/filter/reverb chain after the poly voice mix. Channels 13–14 have bitcrusher and filter stages but no freeverb connection. Their first filter page uses CENT instead of RVRB. `setFilters` and parameter helpers apply the voice-specific mappings.
