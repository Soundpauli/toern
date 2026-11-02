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
- Instrument presets: BASS, KEYS, CHPT, PAD, WOW, ORG, FLT, LEAD, ARP, BRSS, 303, DXEP, DXBS
- Default ch11 instrument: DXBS (index 12); selecting INST loads matching CUT/RES/FLT, register, waveform and ADSR defaults. Saved user values remain authoritative until INST is selected again.
- Preset application: `applySynthInstrumentPreset` / `*_synth(...)` helpers
- Fifth filter page: LFO (cutoff/res on SVF + ladders) and MIDI arpeggiator (same rules as 13–14)

Keyboard and grid velocity use the same per-slot output gain after the ladder
filters and amplitude envelopes (`velocity / 127`). Oscillator drive stays at
full level so soft notes retain the patch's filter character; velocity 1 gets
nonzero gain, and editing a patch does not reset a sounding note's velocity gain.
The 303 preset retains its explicit velocity-dependent filter accent.

Live MIDI and arp notes prefer a completely idle poly slot over one whose
amplitude or filter envelope is still releasing. If all slots are busy, they
reclaim the oldest released slot before stealing a held note. This avoids the
envelope's forced retrigger fade when an unused slot is available.
`tests/test_ch11_articulation.py` covers both trigger paths and voice allocation.

## Channels 13–14 — mono

- Dual oscillators each (`waveform13_*`, `waveform14_*`)
- ADSR; LFO (rate/depth) continuously modulates cutoff and resonance
- MIDI arpeggiator (ARP step + SPAN): holds while Note On is down; does not transpose sequencer/grid notes; SINGLE record writes the sounded pitches
- Each has its own crush/filter path; neither mono synth has a reverb send

## Voice lifecycle

MIDI arp phrase starts collect the opening Note On burst for **12 ms** before
selecting step zero from the held pitches sorted low to high. This makes a
simultaneously played chord start on its lowest note regardless of keyboard
scan / MIDI delivery order. Subsequent arp timing is unchanged. Duplicate Note
Ons do not restart the collection window; releasing every key before it expires
cancels the pending start. Non-arp playing remains immediate.

`tests/test_midi_arp_release.py` checks all six three-note arrival orders on
CH11/13/14, both timing rates and stopped/playing transport, plus duplicate and
release behavior.

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

Synth PSET slots store custom filter-page settings independently for ch11, ch13 and ch14. See [SD storage](../subsystems/sd-storage#synth-filter-presets). Pause releases all poly slots and mono envelopes while effects decay naturally.
