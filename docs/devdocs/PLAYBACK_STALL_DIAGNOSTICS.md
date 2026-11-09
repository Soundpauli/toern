# Playback stall diagnostic checkpoint — 2026-11-09

## Root cause (found 2026-11-09, branch `live-agen`)

The ~1.3 s freezes are **not firmware code paths**: the whole CPU, every
interrupt included, is parked for exactly 2^30 CPU cycles (1,315,860 µs at
816 MHz; stalls of 2^31 / 2^32 cycles were also logged).

A SysTick-hooked probe (`src/toern_stall_probe.h`, fires at priority 32 while
PIT/audio are blocked) caught the stuck instruction: a `SCB_CACHE_DCIMVAC`
store inside `AudioInputI2S::isr()` — a plain cache-invalidate that waits
behind a blocked AXI path. The cause matches the PJRC analysis in
<https://forum.pjrc.com/index.php?threads/78171/>: the stock Teensy 4.1 core
(1.162 / "1.62") maps FlexSPI2 as one 32 MB cacheable MPU region, but the board
has 2 × 8 MB PSRAM. Any address in 0x71000000–0x71FFFFFF that happens to sit in
a register can be speculatively prefetched by the Cortex-M7; FlexSPI2 then
waits for its AHB bus timeout, 4 × 2^28 cycles per 256-bit line fill. With a
16.6 MB EXTMEM footprint on this firmware there are plenty of 0x70xxxxxx/
0x71xxxxxx-looking values in registers, which is why dense CH11/CH13 patterns,
MIDI input and AGEN all "correlated" without any of them being the cause.

Fix: `src/toern_psram_mpu.cpp` implements `startup_middle_hook()`, shrinks the
0x70000000 MPU region to the detected `external_psram_size` (8 or 16 MB) and
zeroes `FLEXSPI2_FLSHB1CR0/B2CR0`. Speculative accesses to the unbacked half are
now rejected by the MPU instead of reaching FlexSPI2. `REMOTE PERF` reports
`psram_mb` and `flexspi2_mpu_mb` (16/16 on the dev unit, previously 16/32).

Validation: the saved stalling motif reproduced on the probe build after
~90 s (`playback_late` 1, 1,318,004 µs stall, probe PC in `AudioInputI2S::isr`).
With the MPU fix the same motif ran 15 min / 5,994 steps: `playback_late` 0,
`loop_stalls` 0, `stall_probe_count` 0, `playback_gap_us` ≈ step interval
throughout.

All temporary instrumentation described below (StallDetail, PlaybackTimingScope,
`guardMaxUs`, `audio_raw_cycles` + the framework `AudioStream.cpp` patch, the
`DNOTE` command and the SysTick stall probe) was removed again after the fix
was validated; only `psram_mb` / `flexspi2_mpu_mb` remain in `PERF`. The
notes below are kept for reference should the problem ever need re-probing.

---

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
