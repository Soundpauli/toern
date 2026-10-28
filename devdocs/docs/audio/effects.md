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

**envelope → bitcrusher → filter → Treeverb → mixers → output**

This reverb path exists on sample voices 1, 2 and 5–8. Samples 3 and 4 bypass reverb. Poly voice 11 has a shared bitcrusher/filter/reverb chain after its oscillator/ladder stages; mono voices 13 and 14 have bitcrusher/filter stages without reverb.

Samples 1–8 bypass the filter when HCUT is 32 and LCUT is 0. A connection from each sample amplifier to filter-mixer input 3 carries the dry signal; four-channel gain smoothing crossfades between dry, lowpass, and highpass. Effects downstream still receive the complete voice. Synth filter routing is unchanged.

Incoming sample MIDI notes track held keys per voice. Note Off clears that key and starts the configured envelope release only after the last key is released. A velocity-zero Note On follows the same release path. The dependency patch preserves the configured release rather than forcing 2 ms.

## UI value vs audio value

The matrix usually shows **0–32** (or enums as text). Mapping functions convert those into Hz, resonance, wet/dry, bit depth, etc. When adding a parameter:

1. Extend the settings array / `FilterType` or related enum if needed.
2. Teach `setFilters` / `setParams` the mapping.
3. Add a slider definition for filter UI (`SliderDefEntry` / name index tables).
4. Keep display enums in sync with `enumNames` pointers.

## Treeverb

Seven independent `AudioEffectTreeverb` objects serve sample voices 1, 2, 5–8
and synth voice 11. The default light network uses four parallel comb filters
and two diffusion stages at the full audio sample rate, replacing eight combs
and four diffusers. Room, damping, wet/dry routing, delay routing, and saved
settings retain their existing controls. The lighter tail has a different
color and density; wet gain is normalized against the reference network's
impulse-response energy rather than guaranteed to sound identical.

`roomsize(0)` bypasses wet processing and clears history on the next update.
Nonzero settings keep natural tails after Pause. `ETC → RSET → RVRB` still
bypasses reverb and bitcrusher across voices. Buffers remain in RAM1. This build frees 89,376 bytes there, increasing
local-variable headroom from 59,040 to 148,416 bytes.

`platformio.ini` compiles `src/effect_treeverb.cpp` with
`TOERN_TREEVERB_LIGHT=1`. Set the flag to `0` for the heavier Treeverb reference.
The old Freeverb source remains available for host comparison tests, outside
the firmware build. `tests/test_treeverb.py` checks both modes under address
and undefined-behavior sanitizers, including tail energy, overload, live
parameter changes, bypass/reset, allocation failure, and IRQ restoration.
`tests/test_pause_effects.py` checks natural tail decay through Pause.

## Sample page 1 — TIME

On sample channels, filter UI page 1 includes **TIME** (`ARR_STRETCH`): independent WSOLA time-stretch, separate from OCTV/DTNE pitch. Default **11 = off**. Implementation and player wiring are covered under [Samples](./samples#time--independent-stretch-wsola).

## Fast filter encoder

During play, encoder #3 can adjust a per-channel “default fast filter” target (`defaultFastFilter`, `setDefaultFastFilterValue`) without entering full filter mode — useful when tracing runtime parameter paths.

## Sample page 3 — DLAY and VOLUME

DLAY is a per-voice PSRAM delay on all sound voices; it follows BPM. Slider N selects N/32 of one quarter-note beat, 0 disables the tap, dry remains unity and the tap mix is 0.5. Feedback varies from 0.52 to 0.68. Synths expose DLAY on CRF, samples on page 3.

Only samples 1–8 expose **V** / VOLUME (`ARR_GAIN`) beside DLAY, encoder 2. The raw 0–32 value displays −16…+16: raw 0 = silence, raw 16 = unity, raw 32 = 2×. `sampleFilterGain[ch-1]` scales the complete voice output after effects/delay and before the sample bus. It is independent of channel volume, bitcrusher compensation and fader amplitude. Page reset and full reset restore raw 16. See [storage encoding](../contributing/firmware-edits#sample-voice-gain).

## Pause and effects tails

Pause releases every sample envelope, all three poly slots via `releaseAllPolySynthVoices()`, and mono voices 13/14. It does not clear delay/reverb buffers or reset their settings. `pendingPauseAudioCleanup` is serviced independently of SD autosave by `servicePauseAudioCleanup()`: sample readers stop only after their envelopes become inactive and the quiet guard has elapsed. Existing effect buffers continue decaying naturally. Fresh Play/audition cancels pending cleanup.

Do not add a fixed tail timeout or clear effects in the autosave path. A quiet envelope is not proof that a long feedback/reverb tail has finished. `tests/test_pause_effects.py` exercises release behavior and actual DSP-tail decay.

## Velocity and display feedback

New notes and default auditions use velocity 127 in firmware and simulator, including the initial TOUCHY pad velocities. Existing saved note velocities and incoming MIDI velocities retain their values.

A lower monophonic note overridden by a guaranteed higher note at the same step is rendered at 20% of its normal RGB brightness. The playhead does not flash that overridden note. Probabilistic or nonmatching conditional higher notes do not dim an audible lower note; CH11 polyphonic notes remain unchanged. Copy-armed feedback uses the lowercase `c+p` label.

Resetting all filter pages shows a blue checkmark for 800 ms. The filter renderer includes the overlay before early rate-limited presentations and restores the page when feedback expires.

## Peak-limited buses

Sample banks, the synth bus, final mix and headphone/USB routing use `AudioMixer4Limited`. It sums signed inputs in float before limiting, with 16 samples of lookahead per stage and 50 ms gain recovery. Default sample bank and final sample gains are unity; the final synth gain is 3.6 to retain the prior relative balance. The bank → final → route path adds 48 samples (about 1.09 ms). Earlier clipping inside a voice or effect cannot be repaired by the bus limiter.

Sample readers and standard Teensy envelopes are unchanged. Sampler registration uses the reader-plus-envelope overload, preserving Note On and configured Note Off behavior while the bus controls its own gain. `tests/test_audio_gain.py` covers overload, cancellation, lookahead and recovery through the real mixer code.

## Sustain, line output and effect reset

Sustain is clamped to 1–32 in parameter application and slider editing, including CH11 preset/reset defaults. Other ADSR minima are unchanged. LOUT displays 1–19 from quietest to loudest; codec values run in reverse from 31 to 13 and are passed directly to the codec at boot and during menu editing.

`ETC → RSET → RVRB` zeros only reverb and bitcrusher on all playable voices, immediately applying those two settings. Other patch parameters and note data remain intact. SWAP is under SETT after MUTE. BPM places its decimal row at y=4 with a white point and violet fraction/encoder-1 indicator; CTRL=PAGE feedback no longer depends on CHNR.
