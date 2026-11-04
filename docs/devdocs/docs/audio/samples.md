---
sidebar_position: 2
title: Samples
description: Loading, previewing, and assigning WAV data to voices 1–8.
---

# Samples

Sample voices are channels **1–8**. Logic concentrates in `toern_sample.ino` and browser helpers in `toern_helpers.ino`.

## Responsibilities

- Browse folders/files on the SD card (combined list / manifest hooks)
- Preview a WAV without committing it
- Load into a channel’s playback buffer / resampler
- Trim start/end (`GLOB.seek`, `GLOB.seekEnd`, …)
- Reverse / direction handling
- Independent time-stretch via filter page 1 **TIME** (`ARR_STRETCH` / WSOLA in the resampler)
- Samplepack 0: per-voice custom paths (`SMP.samplePathRel`, `SMP.sp0Active`)
- Record paths feeding the same sample machinery

## Audio-safe SD I/O

Long SD reads must not starve the Audio library. The sample module exposes:

- `sdIoBeginAudioSafe()` / `sdIoEndAudioSafe()`
- `sdIoYield()`
- channel reload busy flags (`beginChannelSampleReload` / `endChannelSampleReload`)

Call these around blocking file work when audio should keep playing.

## Players

`AudioPlayArrayResmp` instances (`sound0`…`sound8` in `audioinit.h`) play from memory with rate control via the custom resampler. Channel 0 is used for preview / utility paths; performance voices are 1–8.

## Default playback fidelity

At neutral settings, a sample plays as a one-shot through its actual end. The default ADSR is **ATTC=32** (0 ms), **DCAY=32**, **SUST=32** (unity) and **RLSE=0**. There is no automatic sample note-off based on the synth gate duration, and releasing a live MIDI key does not chop an in-flight one-shot. Explicit retriggers, mute/stop, trim, or changed filter/ADSR settings can intentionally alter playback.

The loaders use exact PCM frame windows: a **0–100%** selection includes every frame, including quiet tails and frame counts not divisible by three. There is no hidden 1% end trim or automatic fade over the end of the source. Any zero padding is appended **after** the original PCM so effects can settle; it does not replace the tail.

`samplerNoteEvent()` avoids applying the envelope's forced retrigger release to a finished reader or a reader walking digital silence. `ToernPcmActivity` observes rendered PCM and requires two entirely zero blocks; even ±1 LSB counts as signal. This only controls the next note's restart policy. It never stops, trims or fades the current file. A still-sounding sample retains the existing retrigger de-click fade.

The stock Teensy envelope still implements `attack(0)` as eight samples (about 0.181 ms), and the output limiter retains its headroom protection. USB output therefore is not promised to be bit-identical to every source WAV. Loaded PCM can be checked exactly; playback checks compare against both the original WAV and the known native onset.

## Automated USB playback checks

`tools/toern-mcp/audio_quality.py` records the actual Teensy USB audio input at 44.1 kHz stereo. It compares each USB channel with the source PCM, checks repeated attacks, missing blocks, added discontinuities, level/polarity and post-sample silence, then checks attack, decay, sustain and explicit note-off release against source-aware envelope references. It also exercises real alternating sequencer playback. `--reverb` adds maximum-room decay/noise and bypass recovery checks.

Install the bridge's `requirements-audio-test.txt`, then run from the repository root:

```bash
python3 tools/toern-mcp/audio_quality.py \
  --port /dev/cu.usbmodemXXXX --full-reset \
  --channels 1,2 --reverb --output /tmp/toern-audio-quality
```

**`--full-reset` applies RSET → FULL and clears the current pattern/settings before testing.** The ordinary test adjusts filters and uses a temporary pattern, restoring the entry filter values and RAM pattern in `finally`. The temporary sequence is stopped with the SD protocol's non-saving pause so it is not autosaved. No pattern backup file is created. WAV captures, source references, per-case checks and capture/CPU diagnostics go into the output directory; a failed check produces a nonzero exit status.

By default, source references are downloaded from the loaded kit's numbered WAVs. For a custom sample, load it into the voice after resetting and supply its original local WAV:

```bash
python3 tools/toern-mcp/audio_quality.py \
  --port /dev/cu.usbmodemXXXX --channels 1,2 --reverb \
  --reference '1=/path/to/07 Kick 8.wav' \
  --output /tmp/toern-custom-kick-quality
```

Do not combine a custom reference with `--full-reset`: FULL reloads kit 1. Custom references also verify the saved SP0 PCM contains every original frame unchanged, followed only by optional zero padding. The source needs to be 44.1 kHz mono PCM16. The reference suite can target any sample voice **1–8**; default hardware coverage is CH1/CH2. Shared sampler conclusions apply to those eight voices, while CH11/13/14 have separate synth paths and need separate recordings. These are signal regression checks, not a subjective listening score or analog-output measurement.

### Hardware results, 2026-11-02

After RSET → FULL, CH1 used the supplied `DRUMS/07 Kick 8.wav`; CH2 used kit 1's `2.wav`. All **22** USB hardware cases passed. SP0 readback preserved all **9,410** kick frames exactly, with **1,024** zeros appended after them. Dry capture SNR against the unmodified sources was **51.8 dB** for the full-scale kick and **77.5 dB** for the snare; against the modeled native onset it was **64.7 dB** and **109.5 dB** respectively. These residual figures include known envelope/limiter behavior and are not background-noise measurements.

No capture overflow, missing block or added-click failure occurred. The alternating sequencer test captured nine kicks and eight snares with maximum interval error **1.64 ms** and measured peak CPU **14.82%** for that two-voice workload. Both maximum-room reverb tails decayed to digital silence and cleared on bypass. The JSON's `-240 dBFS` is a numeric floor for zero PCM, not a claim about hardware dynamic range. The test runner's non-saving stop was also checked against an unchanged `autosaved.txt` hash.

The preceding original-kit run reproduced a stale-envelope restart failure: CH1's repeated-hit source match fell from 67.6 to 15.4 dB before the fix and remained at 67.6 dB afterward, including the original silent-tail sequencer case. The supplied kick was measured separately; these source files are not interchangeable A/B workloads.

Those initial playback recordings used the four-comb/two-diffuser reverb. The subsequent [balanced reverb upgrade](./effects#treeverb) uses six combs and four diffusers, with separate density, CPU, memory and USB regression checks. The two-sample recording does not establish headroom for dense CH11 chords.

## TIME — independent stretch (WSOLA)

Sample filter page 1 exposes **TIME** next to RES / DTNE / OCTV (`ARR_STRETCH` in the slider tables). UI range is **0–22**; **11 = OFF** (default). Values below 11 shorten (~−10%…0%); above 11 lengthen (~0%…+10%).

- **OCTV / DTNE** change playback rate (pitch moves with rate).
- **TIME** stretches duration around the current pitched rate via WSOLA in `platformio/src/toern_wsola.h`, driven from `platformio/src/resamplerReader.h` (`setTimeStretch` / `_timeStretchSetting`).

Operator-facing detail lives in the handbook sound chapter; keep firmware mappings and those docs in sync when the range changes.

## UI modes

- `SET_WAV` — browser / assign
- `RECORD_MODE` — capture into a slot
- `SET_SAMPLEPACK` — pack selection

Drawing for waveforms/peaks lives with UI helpers (`showWave`, peak scan functions in `toern_sample.ino`).

## File format and recording

Browser samples and kit slots use **44.1 kHz, mono, 16-bit PCM WAV**. The handbook specifies up to **12 seconds per sample voice**. The line-input connector is stereo TRS on rev H, but `patchCord220` connects only input channel 0 to `audioInputAmp` and `queue1`; recordings are mono. USB audio is an output to a computer, not a sampler input.

KIT saves the current eight sample buffers; PATT saves the note/settings data separately. A failed KIT save does not clear the custom-sample flags or claim success. KIT writes are checked but are not a whole-kit transaction.
