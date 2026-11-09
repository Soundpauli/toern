# Playback stall diagnostic checkpoint — 2026-11-09

Branch: `codex/fix-playback-stalls`, based on `3d6b5f0`.

This preserves the diagnostic firmware that the user reported as running well.
It is not a confirmed root-cause fix. Earlier runs exhibited approximately
1.3-second pauses in both sound and the grid; CH11 MIDI arpeggiation appeared
connected to reproduction. Removing the experimental interrupt tracer preceded
this checkpoint; that tracer is not included here.

## Validation

On the original device, with the saved CH11 motif and settings restored:

- Automated CH11 note, chord, register, and release exercises ran continuously.
- A subsequent 312-second passive capture recorded zero late playback steps,
  including 37 AGEN generations; the user also confirmed MIDI input seemed good.
- Maximum measured audio processing time was approximately 1.68 milliseconds.

These observations establish a stable test run, not that the intermittent stall
has been eliminated. Device/card changes during earlier tests confounded comparisons.

## Temporary instrumentation

`REMOTE ... PERF` adds playback call/gap/late counters, guarded-section duration,
loop detail durations, and raw 32-bit audio cycle maxima. The existing short audio
CPU counters can wrap during long excursions. `audio_raw_cycles` contains total
ISR maximum, individual node maximum, node address, and vtable address. Addresses
must be interpreted against the ELF from the same build.

`detail_us` indices are pending sample notes, WAV sync preview, SD preview,
mixer gains, automatic note release, final LED update, final yield, and earlier yield.
The late-step threshold is the configured step interval plus 20 ms; deliberate
transport or clock changes must be considered when interpreting it.

The diagnostic `DNOTE pitch velocity` remote command invokes the CH11 MIDI note
handlers (zero velocity releases). It requires a live remote session and is not
part of the standard host-client allowlist. It bypasses the physical UART/parser,
so it cannot validate those parts of MIDI input.

The PlatformIO dependency hook applies `scripts/patch_audio_timing.py` to the
shared Teensy framework AudioStream.cpp. This makes the diagnostic build
reproducible, but the framework patch persists outside this worktree after a
build and must be removed before unrelated production builds. No additional
interrupt or GPT sampler is installed. These diagnostics should be removed or
properly gated before a production merge.

Local reproduction artifacts (not tracked) are preserved under
`/tmp/toern-stable-stall-diagnostic`; the saved motif and settings are
`/tmp/toern-stalling-motif.bin` and `/tmp/toern-stalling-motif-settings.json`.
