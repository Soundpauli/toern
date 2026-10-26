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

For sample voices 1–8, HCUT=32 with LCUT=0 selects a real dry connection from the channel amp to filter-mixer input 3. HCUT 0–31 uses the existing low-pass path; LCUT above 0 selects high-pass. The four route gains crossfade together. The bypass applies in firmware and SIM; synth cutoff semantics are unchanged.

This reverb path exists on sample voices 1, 2 and 5–8. Samples 3 and 4 bypass reverb. Poly voice 11 has a shared bitcrusher/filter/reverb chain after its oscillator/ladder stages; mono voices 13 and 14 have bitcrusher/filter stages without reverb.

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

## Sample envelopes and velocity

Newly drawn notes and default auditions use velocity 127 in firmware and SIM. Existing pattern velocities and incoming MIDI velocities remain unchanged.

Sample voices 1–8 use `AudioSampleEnvelope` and `SampleEnvelopeState`. FN/touch 5 on sample ADSR page 2 toggles ONE (default) and GATE. The per-voice setting is a tagged value in spare `param_settings[ch][13]`, preserving the Device layout; legacy/unrecognized values select ONE. ADSR/full filter reset restores ONE. ONE sustains to natural EOF regardless of ADSR edits. GATE sends Note Off after one sixteenth-note step captured from BPM at trigger, independent of A/D/R; held MIDI always waits for explicit Note Off. A/H/D fit proportionally within remaining reader output (minus one frame) or the smaller gate duration. R is excluded from fitting and is capped to remaining PCM at Note Off. Decay is skipped at unity Sustain. Reader duration accounts for trim, final pitch/detune, direction, channels and active TIME/WSOLA. Pause uses Release; retrigger uses the old-only residual below. SIM uses the same modes and independent tempo gate. Neutral defaults remain A32/D32/S32/R0. Mode changes apply on the next hit.

Before a retrigger resets the reader, `snapshotTail()` reads 64 samples at the old rate/velocity. The old envelope shapes this residual and fades it over 64 samples (1.45 ms); the new reader starts immediately with its own ADSR. The incoming attack is never faded to make room for the old reader. If both exceed the 16-bit voice connection's headroom, only the old residual is reduced. Rapid retriggers include any remaining old tail. The reader helper lives in `src/sample_reader.h`; the dependency patch script removes the library’s former fixed 2 ms release override. `tests/test_sample_envelope.py` verifies neutral PCM equality, old-only tails, repeated triggers and ADSR/release timing; an optional WAV argument checks a real mono 16-bit sample.

Delay wet/feedback gain multiplication truncates toward zero for both polarities. An arithmetic right shift used to round negative sub-LSB feedback back to −1 indefinitely. The corrected arithmetic lets the real ring history decay to exact silence without clearing audible echoes or imposing a pause timeout. `tests/test_pause_effects.py` closes the actual delay/feedback loop at 0.68 and verifies both polarities drain.

## Pause and effects tails

Pause releases every sample envelope, all three poly slots via `releaseAllPolySynthVoices()`, and mono voices 13/14. It does not clear delay/freeverb buffers or reset their settings. `pendingPauseAudioCleanup` is serviced independently of SD autosave by `servicePauseAudioCleanup()`: sample readers stop only after their envelopes become inactive and the quiet guard has elapsed. Existing effect buffers continue decaying naturally. Fresh Play/audition cancels pending cleanup.

Do not add a fixed tail timeout or clear effects in the autosave path. A quiet envelope is not proof that a long feedback/reverb tail has finished. `tests/test_pause_effects.py` exercises release behavior and actual DSP-tail decay.
