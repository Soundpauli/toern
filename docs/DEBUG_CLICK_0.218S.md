# Periodic Click Investigation (0.218s / 4.6Hz)

## Symptoms
- **Interval:** ~0.218 seconds (matches ~75 audio blocks of 128 samples at 44.1kHz).
- **Trigger:** High amplitude / clipping events.
- **Persistence:** Continues even after audio stops.
- **Context:** Independent of BPM. Happens on battery.
- **Mitigation:** Reducing global volume < 0.5 prevents triggering.

## Theory
- **75 Audio Blocks:** $75 \times 128 = 9600$ samples. $9600 / 44100 \approx 0.2177$s.
- This suggests a periodic resource starvation or state reset occurring every 75 audio interrupts, or a process running at ~4.6Hz interfering with audio.
- The "Triggered by clipping" aspect suggests a latching state (e.g., NaN/Inf in DSP chain, hardware codec state, or error logging/handling latch).

## Ruled Out
- Mixer gains (headroom adjusted).
- Codec DSP (AGC/Pre/Post disabled).
- I2C speed.
- Bitcrushers/Freeverbs (removed/bypassed).
- MIDI clock.
- Race conditions (guarded).
- Power supply noise.

## Potential Culprits to Investigate
1. **FastLED / Display Updates:** If these run every ~218ms or are triggered by levels.
2. **AudioMemory:** Is 64 blocks enough? If we run out, we drop blocks.
3. **Serial/Logging:** (Periodic logging removed, but check error handlers).
4. **Timers:** `elapsedMillis` or `IntervalTimer` with ~218ms period.
5. **SGTL5000 Hardware:** Does the chip itself have a status polling or error correction loop?
6. **NaN/Denormal Propagation:** If a filter goes unstable due to clipping, it might oscillate or produce periodic NaNs.

## Investigation Log
- [x] Scan codebase for ~218ms / 4.6Hz timers.
  - **Result:** FastLED refresh is ~33ms (30 FPS), not 218ms. No timers found at 218ms.
- [x] Check FastLED refresh rate.
  - **Result:** `FastLEDshow()` throttled to `RefreshTime = 1000/TargetFPS = 33ms`. However, `FastLED.show()` itself is **blocking** and can take several milliseconds, potentially interfering with audio ISR deadlines.
- [x] Check AudioMemory usage monitoring.
  - **Result:** `AudioMemory(64)` allocated. This is **very low** for the complex chain:
    - 8 sample channels (envelope→bitcrusher→amp→filter→filtermixer→freeverbmixer)
    - Multiple mixers (mixer1, mixer2, mixer_end, mixer_stereoL/R)
    - Synth channels (11, 13, 14)
    - Preview channel (mixer0)
    - Input monitoring path
  - **Critical:** When `allocate()` fails (in freeverb, filters, etc.), objects return early → **silence/click**.
- [x] Investigate `AudioAnalyzePeak` usage (is it triggering something?).
  - **Result:** Used for preview peak scanning, but not obviously periodic at 218ms.

## Findings

### PRIMARY SUSPECT: AudioMemory Exhaustion
**Evidence:**
- `AudioMemory(64)` is likely insufficient for the full audio chain, especially when multiple channels + reverb are active.
- When blocks run out, `allocate()` returns NULL → audio objects produce silence → **audible click**.
- The **0.218s = 75 blocks** timing could indicate:
  - Periodic block pool exhaustion/recovery cycle
  - Or a cumulative delay from multiple failed allocations

**Test:** Increase `AudioMemory()` to 128 or 256 blocks and see if clicks disappear.

### SECONDARY SUSPECT: FastLED.show() Blocking
**Evidence:**
- `FastLED.show()` is a **blocking SPI operation** that can take 1-5ms depending on LED count.
- If this runs during an audio block update ISR deadline, it can cause a missed block → click.
- The throttling (33ms) doesn't prevent it from coinciding with audio updates.

**Test:** Temporarily disable `FastLED.show()` calls and see if clicks persist.

### TERTIARY SUSPECT: SGTL5000 autoVolumeEnable
**Evidence:**
- `sgtl5000_1.autoVolumeEnable()` is enabled (line 2485). This AGC can periodically adjust gain internally.
- **Volume is NOT changed programmatically** (checked all `sgtl5000_1.volume()` calls):
  - Only called at startup (`initSoundChip()`, `applyAudioSettingsFromGlobals()`)
  - Only called on user encoder input (menu adjustment)
  - `updateVolume()` function exists but is never called (dead code)
- If AGC reacts to clipping by reducing gain, then ramps back up, this could create a periodic pattern matching the 0.218s interval.

**Test:** Disable `autoVolumeEnable()` and see if clicks persist.

### QUATERNARY SUSPECT: ResamplerReader "Zombie" Voices (CRITICAL BUG FOUND)
**Evidence:**
- In `src/resamplerReader.h`, `readNextValue()` was returning `true` (success) when reading out-of-bounds (EOF), instead of `false`.
- **Consequence:** The reader thought it was still playing valid audio (silence) and never triggered the stop logic.
- **Result:** Every voice that ever played became a "zombie" playing silence forever.
- **Link to Symptoms:**
  - **"Continues even after audio stops":** Because the voices never actually stopped.
  - **"Triggered by clipping":** Maybe clipping coincided with many voices active? Or clipping behavior is a separate issue, but the click is due to 8+ zombie voices running simultaneously causing CPU/memory contention?
  - **0.218s interval:** Could be related to the cumulative processing overhead of these zombies hitting a periodic system limit.

**Fix:** Changed `return true` to `return false` in `src/resamplerReader.h` (lines 505, 509).

## Fixes Applied

### 2025-01-09: Fixed "Zombie" Voices in Resampler
- **File:** `src/resamplerReader.h`
- **Change:** Changed `return true` to `return false` when `readNextValue` is out-of-bounds.
- **Rationale:** Prevents voices from playing silence forever after they finish. They now correctly detect EOF and stop.

### 2025-01-09: Fixed Envelope Bug in Sampler
- **File:** `src/sampler.h`
- **Change:** Fixed copy-paste error where `_audioenvelop` was released instead of `_audioenvelop2` during note-off.
- **Rationale:** Corrects envelope behavior for secondary envelopes.

### 2025-01-09: Increased AudioMemory
- **Changed:** `AudioMemory(64)` → `AudioMemory(128)` in `initSoundChip()`
- **Rationale:** 64 blocks is insufficient for the full audio chain. When `allocate()` fails, objects return early → silence/click.
- **Status:** Applied, needs testing.

### Next Steps (if not fixed)
1. **Test FastLED blocking:** Temporarily comment out all `FastLED.show()` calls.
2. **Test autoVolume:** Disable `sgtl5000_1.autoVolumeEnable()`.
3. **Monitor AudioMemory usage:** Add periodic logging of `AudioMemoryUsage()` to see if we're still hitting limits.
