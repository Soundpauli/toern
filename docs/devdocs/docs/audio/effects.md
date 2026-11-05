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

Sample voices 1–8 are one-shots when SUSTAIN is above 0: a grid hit or MIDI Note On opens the envelope and leaves it in sustain so the file can play to the end. SUSTAIN 0 is allowed on samples (synths still floor at 1). With SUSTAIN 0 and RELEASE 0 the hit dies through decay — it ends before the file, but the quiet end of the decay still sounds. MIDI Note Off does not start release (RELEASE 0 would otherwise chop). Pause, mute, and reload still send noteOff. The dependency patch preserves the configured release rather than forcing 2 ms.

## UI value vs audio value

The matrix usually shows **0–32** (or enums as text). Mapping functions convert those into Hz, resonance, wet/dry, bit depth, etc. When adding a parameter:

1. Extend the settings array / `FilterType` or related enum if needed.
2. Teach `setFilters` / `setParams` the mapping.
3. Add a slider definition for filter UI (`SliderDefEntry` / name index tables).
4. Keep display enums in sync with `enumNames` pointers.

## Treeverb

Seven independent `AudioEffectTreeverb` objects serve sample voices 1, 2, 5–8
and synth voice 11. The default balanced network uses **six parallel comb filters
and four diffusion stages** at the full audio sample rate. This restores all four
diffusion stages and adds two reflection paths over the preceding light network.
Room, damping, wet/dry routing, delay routing, dry playback and saved settings
retain their existing controls. Wet gain is normalized against the full reference
network's impulse-response energy; this is a change in reflection density, not
an intentional volume boost.

`roomsize(0)` bypasses wet processing and clears history on the next update.
Nonzero settings keep natural tails after Pause. `ETC → RSET → RVRB` still
bypasses reverb and bitcrusher across voices. Buffers remain in RAM1. The balanced
build leaves **43,360 bytes** free for local variables; the preceding light build
left about 91.9 KB. RAM2 allocation is unchanged. Restoring the full eight-comb
network would leave only a few KB of RAM1, so it remains a reference option.

`platformio.ini` compiles `platformio/src/effect_treeverb.cpp` with
`TOERN_TREEVERB_QUALITY=1`. Available profiles are:

| Value | Profile | Combs / diffusers |
|-------|---------|-------------------|
| `0` | Light | 4 / 2 |
| `1` | Balanced (default) | 6 / 4 |
| `2` | Full reference | 8 / 4 |

Explicit legacy `TOERN_TREEVERB_LIGHT=1/0` builds still map to light/full when
`TOERN_TREEVERB_QUALITY` is not supplied. MCP `get_performance` reports
`reverb_quality` using the values above.

The old Freeverb source remains available for host comparison tests, outside
the firmware build. `_debug/tests/test_treeverb.py` checks all three profiles under address
and undefined-behavior sanitizers, including tail energy, overload, live
parameter changes, bypass/reset, allocation failure, and IRQ restoration.
`_debug/tests/test_pause_effects.py` checks natural tail decay through Pause.

### Balanced-profile validation, 2026-11-02

For a fixed impulse at room 0.75/damping 0.25, nonzero reflection coverage in the
25–150 ms window increased from **11.8%** (light) to **59.5%** (balanced); full
reference is **60.4%**. Balanced integrated wet energy was **1.05×** full reference
(about +0.21 dB), and all sixteen room/damping combinations stayed within the
existing approximately ±1 dB energy bounds. Coverage is a signal-density metric,
not a subjective sound-quality score.

On the device, all **22** USB playback/ADSR/reverb cases passed with the supplied
CH1 `07 Kick 8.wav` and kit 1 CH2 snare. Dry output matched the preceding firmware's
measurements, both maximum-room tails decayed to digital silence, and capture
reported no overflows or clipping failures. Maximum reverb added about **2.25 CPU
percentage points** over bypass per tested voice, versus approximately 1.74–1.83
in the preceding recordings: an estimated **0.42–0.51 point** increase. These are
per-case differences against each recording's own bypass; other background patch
settings differed between sessions. They do not certify dense CH11 chord headroom.

## Sample page 1 — TIME

On sample channels, filter UI page 1 includes **TIME** (`ARR_STRETCH`): independent WSOLA time-stretch, separate from OCTV/DTNE pitch. Default **11 = off**. Implementation and player wiring are covered under [Samples](./samples#time--independent-stretch-wsola).

## Fast filter encoder

During play, encoder #3 can adjust a per-channel “default fast filter” target (`defaultFastFilter`, `setDefaultFastFilterValue`) without entering full filter mode — useful when tracing runtime parameter paths.

## Sample page 3 — DLAY and VOLUME

DLAY is a per-voice PSRAM delay on all sound voices; it follows BPM. Slider N selects N/32 of one quarter-note beat, 0 disables the tap, dry remains unity and the tap mix is 0.5. Feedback varies from 0.52 to 0.68. Synths expose DLAY on CRF, samples on page 3.

Only samples 1–8 expose **V** / VOLUME (`ARR_GAIN`) beside DLAY, encoder 2. The raw 0–32 value displays −16…+16: raw 0 = silence, raw 16 = unity, raw 32 = 2×. `sampleFilterGain[ch-1]` scales the complete voice output after effects/delay and before the sample bus. It is independent of channel volume, bitcrusher compensation and fader amplitude. Page reset and full reset restore raw 16. See [storage encoding](../contributing/firmware-edits#sample-voice-gain).

## ATTACK response

`getAttackDuration(channel)` in `platformio/toern_parameters.ino` supplies both `setParams(ATTACK, channel)` and `getNoteDuration()`. For sample voices 1–8:

```text
maximumMs = loadedSampleLen[channel] * 1000 / AUDIO_SAMPLE_RATE_EXACT
attackMs = (32 - ATTC) / 32 * maximumMs
```

ATTC 32 requests 0 ms, 16 requests half the loaded PCM duration and 0 requests its full duration. This uses the loaded buffer (including its zero-tail pad) at root playback rate; pitch and TIME do not rescale the envelope per note. `samplerAddSample()` reapplies ATTACK after registering a replacement, covering file loads, trims and recordings. An empty sample has a zero-duration maximum. The stock envelope still implements `attack(0)` with its native minimum ramp.

Synth voices retain the linear 0–2000 ms range. Decay and release remain 0–1000 ms. Sample sustain allows 0–32; synth sustain floors at 1. `_debug/tests/test_attack_curve.py` covers all 33 settings, multiple sample lengths, replacement, note gates and simulator timing. The browser uses the selected buffer span for sample attack, retains its 1 ms scheduling floor and the fixed synth range.

## Pause and effects tails

Pause releases every sample envelope, all three poly slots via `releaseAllPolySynthVoices()`, and mono voices 13/14. It does not clear delay/reverb buffers or reset their settings. `pendingPauseAudioCleanup` is serviced independently of SD autosave by `servicePauseAudioCleanup()`: sample readers stop only after their envelopes become inactive and the quiet guard has elapsed. Existing effect buffers continue decaying naturally. Fresh Play/audition cancels pending cleanup.

Do not add a fixed tail timeout or clear effects in the autosave path. A quiet envelope is not proof that a long feedback/reverb tail has finished. `_debug/tests/test_pause_effects.py` exercises release behavior and actual DSP-tail decay.

## Velocity and display feedback

New notes and default auditions use velocity 127 in firmware and simulator, including the initial TOUCHY pad velocities. Existing saved note velocities and incoming MIDI velocities retain their values.

A lower monophonic note overridden by a guaranteed higher note at the same step is rendered at 20% of its normal RGB brightness. The playhead does not flash that overridden note. Probabilistic or nonmatching conditional higher notes do not dim an audible lower note; CH11 polyphonic notes remain unchanged. Copy-armed feedback uses the lowercase `c+p` label.

Resetting all filter pages shows a blue checkmark for 800 ms. The filter renderer includes the overlay before early rate-limited presentations and restores the page when feedback expires.

## Peak-limited buses

Sample banks, the synth bus, final mix and headphone/USB routing use `AudioMixer4Limited`. It sums signed inputs in float before limiting, with 16 samples of lookahead per stage and 50 ms gain recovery. Default sample bank and final sample gains are unity; the final synth gain is 3.6 to retain the prior relative balance. The bank → final → route path adds 48 samples (about 1.09 ms). Earlier clipping inside a voice or effect cannot be repaired by the bus limiter.

Sample readers and standard Teensy envelopes are unchanged. Sampler registration uses the reader-plus-envelope overload, preserving Note On and configured Note Off behavior while the bus controls its own gain. `_debug/tests/test_audio_gain.py` covers overload, cancellation, lookahead and recovery through the real mixer code.

## Sustain, line output and effect reset

Sustain is clamped to 0–32 for sample voices 1–8 and 1–32 for synth voices in parameter application and slider editing, including CH11 preset/reset defaults. `toern_audio::sustainSliderMin()` centralizes the lower bound. Other ADSR minima are unchanged. LOUT displays 1–19 from quietest to loudest; codec values run in reverse from 31 to 13 and are passed directly to the codec at boot and during menu editing.

`ETC → RSET → RVRB` zeros only reverb and bitcrusher on all playable voices, immediately applying those two settings. Other patch parameters and note data remain intact. SWAP is under SETT after MUTE. BPM places its decimal row at y=4 with a white point and violet fraction/encoder-1 indicator; CTRL=PAGE feedback no longer depends on CHNR.
