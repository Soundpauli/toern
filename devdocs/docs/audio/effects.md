---
sidebar_position: 4
title: Effects & parameters
description: Filters, bitcrush, reverb, and how UI values become Audio settings.
---

# Effects & parameters

Per-channel FX and synth parameters are stored in `SMP` tables and applied by dedicated modules.

## Modules

| File | Role |
|------|------|
| `toern_filter.ino` | `setFilters`, mixer gain smoothing, filter transitions |
| `toern_filterUI.ino` | Matrix sliders, pages, encoder colors for filter mode |
| `toern_parameters.ino` | `setParams`, defaults, envelope/synth default packs, reset |

## FX chain (conceptual)

For a typical sample channel:

**envelope → bitcrusher → filter → freeverb → mixers → output**

This reverb path exists on sample voices 1, 2 and 5–8. Samples 3 and 4 bypass reverb. Poly voice 11 has a shared bitcrusher/filter/reverb chain after its oscillator/ladder stages; mono voices 13 and 14 have bitcrusher/filter stages without reverb.

Samples 1–8 bypass the filter when HCUT is 32 and LCUT is 0. A connection from each sample amplifier to filter-mixer input 3 carries the dry signal; four-channel gain smoothing crossfades between dry, lowpass, and highpass. Effects downstream still receive the complete voice. Synth filter routing is unchanged.

Incoming sample MIDI notes track held keys per voice. Note Off clears that key and starts the configured envelope release only after the last key is released. A velocity-zero Note On follows the same release path. The dependency patch preserves the configured release rather than forcing 2 ms.

## UI value vs audio value

The matrix usually shows **0–32** (or enums as text). Mapping functions convert those into Hz, resonance, wet/dry, bit depth, etc. When adding a parameter:

1. Extend the settings array / `FilterType` or related enum if needed.
2. Teach `setFilters` / `setParams` the mapping.
3. Add a slider definition for filter UI (`SliderDefEntry` / name index tables).
4. Keep display enums in sync with `enumNames` pointers.

## Sample page 1 — TIME

On sample channels, filter UI page 1 includes **TIME** (`ARR_STRETCH`): independent WSOLA time-stretch, separate from OCTV/DTNE pitch. Default **11 = off**. Implementation and player wiring are covered under [Samples](./samples#time--independent-stretch-wsola).

## Fast filter encoder

During play, encoder #3 can adjust a per-channel “default fast filter” target (`defaultFastFilter`, `setDefaultFastFilterValue`) without entering full filter mode — useful when tracing runtime parameter paths.

## Sample page 3 — DLAY and VOLUME

DLAY is a per-voice PSRAM delay on all sound voices; it follows BPM. Slider N selects N/32 of one quarter-note beat, 0 disables the tap, dry remains unity and the tap mix is 0.5. Feedback varies from 0.52 to 0.68. Synths expose DLAY on CRF, samples on page 3.

Only samples 1–8 expose **V** / VOLUME (`ARR_GAIN`) beside DLAY, encoder 2. The raw 0–32 value displays −16…+16: raw 0 = silence, raw 16 = unity, raw 32 = 2×. `sampleFilterGain[ch-1]` scales the complete voice output after effects/delay and before the sample bus. It is independent of channel volume, bitcrusher compensation and fader amplitude. Page reset and full reset restore raw 16. See [storage encoding](../contributing/firmware-edits#sample-voice-gain).

## Pause and effects tails

Pause releases every sample envelope, all three poly slots via `releaseAllPolySynthVoices()`, and mono voices 13/14. It does not clear delay/freeverb buffers or reset their settings. `pendingPauseAudioCleanup` is serviced independently of SD autosave by `servicePauseAudioCleanup()`: sample readers stop only after their envelopes become inactive and the quiet guard has elapsed. Existing effect buffers continue decaying naturally. Fresh Play/audition cancels pending cleanup.

Do not add a fixed tail timeout or clear effects in the autosave path. A quiet envelope is not proof that a long feedback/reverb tail has finished. `tests/test_pause_effects.py` exercises release behavior and actual DSP-tail decay.
