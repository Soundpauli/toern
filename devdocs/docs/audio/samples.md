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

## UI modes

- `SET_WAV` — browser / assign
- `RECORD_MODE` — capture into a slot
- `SET_SAMPLEPACK` — pack selection

Drawing for waveforms/peaks lives with UI helpers (`showWave`, peak scan functions in `toern_sample.ino`).

## File format and recording

Browser samples and kit slots use **44.1 kHz, mono, 16-bit PCM WAV**. The handbook specifies up to **12 seconds per sample voice**. The line-input connector is stereo TRS on rev H, but `patchCord220` connects only input channel 0 to `audioInputAmp` and `queue1`; recordings are mono. USB audio is an output to a computer, not a sampler input.

KIT saves the current eight sample buffers; PATT saves the note/settings data separately. A failed KIT save does not clear the custom-sample flags or claim success. KIT writes are checked but are not a whole-kit transaction.
