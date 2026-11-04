# CH11 CPU and responsiveness investigation

Measured on the connected Teensy 4.1 on 2 November 2026, at 816 MHz / 126 BPM,
using the current two-page project and its DXEP CH11 patch. The patch uses reverb,
delay, LFO, and up to three simultaneous CH11 notes. Existing AudioStream cycle
counters were read on demand through `REMOTE PERF`; no audio ISR instrumentation
was added. CPU figures are audio-update CPU percentages, not overall main-loop CPU.

## Findings

CH11's graph has nine oscillators and six oversampled ladder filters: three
polyphonic voices, with independent left/right filtering. The graph is unchanged
from `42c5382`. A three-note chord activates all six filters. The original
size-optimized implementation kept their working state in external PSRAM and
called the tiny LPF helper four times per oversampled sample, per filter.

Measured bottleneck: the original three-voice samples spent **48.74 percentage
points in the ladders**, with **97.84% total audio CPU**. CH11 reverb and delay
averaged about 1.18% and 0.86% respectively. Those effects were not the main cause.

There was also a separate MCP input regression: the physical-button shadow could
remain held after the original button FSM had recovered from a lost release.
Managed gestures would then reject further input. The shadow now follows FSM
recovery, and fresh physical push edges retain the original callback's recovery
behavior even if the preceding release was missed. Real remote holds still
suppress duplicate combined transitions and survive physical releases.

## Hardware results

| Build | Mean playback CPU | 95th percentile | Observed maximum | Mean CPU with 3 active voices | Ladder CPU with 3 active voices |
|---|---:|---:|---:|---:|---:|
| Original PSRAM DSP, `-Os` | 63.96% | 80.13% | 98.54% | 97.84% | 48.74% |
| DTCM DSP, `-Os` | 58.50% | 80.74% | 83.38% | 80.39% | 38.65% |
| DTCM DSP, ladder-only `-O2` | 55.19% | 73.05% | 76.00% | 72.40% | 30.71% |

With CH11 muted on the optimized build: mean 39.78%, 95th percentile 45.39%,
observed maximum 45.90%. The ladders then averaged 0.06% collectively.

The original run sampled 18 seconds (116 samples); each candidate sampled 30
seconds (209 samples). The muted candidate run sampled 8 seconds (57 samples).
The user edited one additional note between the original and candidate runs:
174 notes originally, 175 in both candidate passes. The table therefore includes
per-active-voice grouping rather than claiming identical whole-project workload.
There were only two original three-active-voice samples; the optimized run had 26.
These are observed samples, not a worst-case scheduling guarantee.

A final repeat on the flashed build sampled another 12 seconds (85 samples):
54.60% mean CPU, 71.94% 95th percentile, 73.83% observed maximum, and 71.14%
mean during three-active-voice samples. Muted mean was 39.21%. All 175 notes
and visible patch values were restored; the device was returned to DRAW, paused,
matching its pre-test transport state.

95th-percentile PERF reply latency fell from 24.43 ms originally to 15.88 ms in
the optimized pass. A filter adjustment and restoration during full playback
completed successfully; its 224 ms tool duration includes the client's intentional
settling delays. No pattern edits were made by the performance test. Notes/BPM
were checked by exact RAM-image round trip, and visible patch parameters were
restored and verified after each test flash. The user's live pattern/patch state
was held in the test process during flashing; no project backup file was created.

## Changes

- Move 29 hot CH11 DSP objects to DTCM: oscillators, stereo voice mixers, ladders,
  output envelopes and polyphonic summers. This uses **21,664 bytes** of internal
  RAM and initially left **105,312 bytes** reported for local variables. The
  follow-up graph relocation below leaves **91,936 bytes**.
- Keep large sample buffers and delay lines in external PSRAM.
- Compile only the upstream ladder translation unit with `-O2`; leave the sketch
  and other libraries size-optimized. No `-ffast-math`, interpolation changes,
  reduced polyphony, lower sample rate or effect disabling.
- Preserve the upstream source; PlatformIO middleware changes only this object's
  compile flags. Disassembly confirms LPF/tanh per-sample calls are inlined.
- Recover stale physical button shadows and preserve fresh push-edge recovery.
- Add read-only `get_performance` MCP telemetry, including active voices,
  ladder/oscillator CPU, reverb/delay CPU, audio memory and input recoveries.

## Verification

```sh
pio run
python3 _debug/tests/test_ch11_dsp_memory.py
python3 _debug/tests/test_audio_dsp_build.py
python3 _debug/tests/test_remote_firmware.py
python3 _debug/tests/test_poly_sleep.py
python3 _debug/tests/test_remote_client.py
python3 _debug/tests/test_fn_inputs.py
python3 _debug/tests/test_touchy_controls.py
```

The ELF check verifies real linker addresses and a 24 KiB hot-state budget. The
build test verifies local optimization and inlining without changing global
optimization. Input tests exercise missed releases, FSM recovery, independent
physical/remote holds, watchdog expiry and normal/FN turns.

The reported intermittent post-flash hang after KIT loading remains a separate
startup issue; this investigation does not establish that it is fixed. No audio
loopback/timbre comparison or exhaustive worst-case preset benchmark was performed.


## Follow-up: mixers, routes and sampler controllers

At the user's request, the remaining small mixers (including delay feedback/mix
arrays and the preview amplifier), all 269 static routing connections, and nine
sampler controllers were moved to internal DTCM. The `SMP` settings object, sample
PCM and delay storage remain external. No connections or DSP algorithms changed.

A fresh before/after test used the same 175-note current RAM image and native
patch values, at 126 BPM. Both playback passes sampled 24 seconds (168 and 169
samples). Notes/BPM round-tripped exactly, visible patch values were restored,
and filter editing/menu entry and exit worked during playback. The original
running transport state was restored.

| Metric | Before this relocation | After |
|---|---:|---:|
| Mean playback CPU | 54.84% | 51.68% |
| 95th percentile CPU | 73.71% | 70.18% |
| Observed maximum CPU | 76.71% | 71.00% |
| Mean CPU with 3 active CH11 voices | 72.41% | 68.60% |
| 95th-percentile PERF reply | 16.18 ms | 14.65 ms |
| RAM1 variables | 189,600 B | 202,976 B |
| RAM1 code + padding | 229,376 B | 229,376 B |
| RAM1 free for locals/stack | 105,312 B | 91,936 B |

The net RAM1 cost is **13,376 bytes (13.06 KiB)**; small alignment differences
explain the difference from the symbol-size estimate. Code occupies the same
RAM1 bank allocation. Remaining RAM1 reserve is **89.78 KiB**. With CH11 muted,
the candidate averaged 37.14% audio CPU, peaking at 41.70% over a 6-second sample.

`_debug/tests/test_audio_graph_memory.py` checks actual ELF addresses for all small
mixers, routes and sampler controllers, a 16 KiB budget for that graph/control
state, and that `SMP` and the large audio buffers remain in PSRAM.

## USB capture diagnostics (3 November 2026)

The PlatformIO dependency patch fixes Teensy 4's partial USB TX underrun clear:
`len` counts stereo frames, so zero-fill starts at a 32-bit frame offset rather
than a 16-bit sample offset. Already copied packet audio is preserved and the
missing stereo tail is cleared. The nominal USB packet cadence is unchanged.

`REMOTE PERF` now includes cumulative `usb_tx` counters:

- `overruns`: output queue full events.
- `discarded_frames`: unconsumed stereo frames discarded by those events.
- `underruns`: output packets requiring zero-fill.
- `silent_frames`: stereo frames zero-filled in those packets.

These count device-to-host output, unlike the stock `usb_audio_*` counters for
host-to-device input. Read before and after a passive test and subtract baselines;
startup/stream transitions can increment them. Counters are 32-bit, wrap modulo
2^32, and reset on reboot. Individual reads are atomic, but the four readings
are not a transactional snapshot. No interrupt-time serial logging is added.
The fix and counters are present in both PlatformIO builds; only MCP exposes PERF.
An unpatched Arduino IDE core reports `usb_tx: null`, not false zero-loss data.

This repairs a concrete packet-corruption defect, not a confirmed explanation
for all timing jumps. Verify with simultaneous analogue and USB recording and
counter deltas before claiming the recording loss is resolved.
