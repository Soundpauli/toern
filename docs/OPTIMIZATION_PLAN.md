# Optimization Plan: RAM1, Speed, and Reliability Improvements

**Goal:** Better quality, better sound quality, and more reliability

**Date:** January 15, 2026

---

## Priority 0 — Critical Reliability/Sound Quality Wins

### 1. Single `FastLEDshow()` Per Frame (Eliminate Nested Calls)

**Problem:** Multiple `FastLEDshow()` calls per frame cause interrupt blocking and audio jitter.

**Impact:** High - Directly affects audio timing stability

**Solution:** Remove all internal `FastLEDshow()` calls from drawing functions. Only call once at the end of frame rendering in main loop.

**Code References:**
- `toern_ui.ino:869-894` - `drawNumber()` calls `FastLEDshow()` internally
- `toern.ino:4626-4658` - Main loop already has one `FastLEDshow()` for draw/single mode
- `toern.ino:5019-5048` - Additional `FastLEDshow()` call after overlay drawing

**Action Items:**
- Remove `FastLEDshow()` from `drawNumber()`, `drawVelocity()`, and other drawing helpers
- Ensure only one `FastLEDshow()` call per frame in main loop
- Use a "frame ready" flag if needed to coordinate multiple drawing functions

---

### 2. Remove Per-Pixel `yield()` Calls

**Problem:** `light()` calls `yield()` on every pixel write. With full grid refreshes, this is hundreds of yields per frame, causing jitter.

**Impact:** High - Adds significant overhead and timing variability

**Solution:** Move `yield()` to once per frame (or per large block), not per pixel.

**Code Reference:**
```cpp
// toern_ui.ino:45-56
void light(unsigned int x, unsigned int y, CRGB color) {
  if (y < 1 || y > maxY || x < 1 || x > maxX) return;
  // ...
  light_single(matrixNum, localX, y, color);
  yield();  // <-- Remove this
}
```

**Action Items:**
- Remove `yield()` from `light()` function
- Add strategic `yield()` calls in main loop (e.g., once per frame, or after large drawing operations)
- Consider yielding only when processing large batches (e.g., after drawing a full row)

---

### 3. Throttle LED Strip Updates When Idle

**Problem:** `updateLedStrip()` clears the entire strip every loop iteration when disabled or paused, wasting CPU cycles.

**Impact:** Medium-High - Steals CPU time from audio/UI processing

**Solution:** Only clear strip on state change, or throttle to very low FPS when idle.

**Code Reference:**
```cpp
// toern_leds.ino:164-191
void updateLedStrip() {
  if (!ledStripEnabled || !isNowPlaying) {
    // Clears entire strip every loop - wasteful!
    for (int i = 0; i < ledStripLength; i++) {
      stripLeds[i] = CRGB::Black;
    }
    return;
  }
  // ...
  // Also clears strip every frame even when playing
  for (int i = 0; i < ledStripLength; i++) {
    stripLeds[i] = CRGB::Black;
  }
}
```

**Action Items:**
- Track previous state (`ledStripEnabled`, `isNowPlaying`)
- Only clear strip when state changes (enabled→disabled, playing→paused)
- Skip `updateLedStrip()` entirely when disabled and not playing
- Consider throttling to 1-2 FPS when idle instead of every loop

---

## Priority 1 — RAM1 Pressure + CPU Churn

### 4. Audit DMAMEM Usage (Move Non-ISR Data to EXTMEM)

**Problem:** DMAMEM uses OCRAM (fast but limited ~512KB). Many arrays don't need DMA/ISR access and waste precious RAM1.

**Impact:** High - Frees RAM1 for audio buffers, improves stability

**Candidates for Migration to EXTMEM:**
- `peakValues[maxPeaks]` - Only used in UI rendering
- `peakRecValues[maxRecPeaks]` - Only used in UI rendering  
- `particles[256]` - Only used during intro animation
- `pageMutes[maxPages][maxY]` - UI state, not ISR-critical
- `pressedKeyCount[maxY]` - UI state
- `touchState[4]` / `lastTouchState[4]` - UI state
- `lastChannelVolBeforeMute[maxY]` - UI state

**Keep in DMAMEM (ISR/DMA Critical):**
- `pendingNotesBuf[MAX_PENDING_NOTES]` - Used by ISR
- `leds[NUM_LEDS]` - DMA for LED output
- `activeNotes[128]` - Used by MIDI ISR handlers
- `noteOnTriggered[maxY]` - Used by audio ISR
- `startTime[maxY]` - Used by audio timing

**Code References:**
```cpp
// toern.ino:361-573
DMAMEM static PendingNote pendingNotesBuf[MAX_PENDING_NOTES];  // Keep
DMAMEM Particle particles[256];  // Move to EXTMEM
DMAMEM bool activeNotes[128];  // Keep (MIDI ISR)
DMAMEM float peakValues[maxPeaks];  // Move to EXTMEM
DMAMEM float peakRecValues[maxRecPeaks];  // Move to EXTMEM
DMAMEM bool touchState[4];  // Move to EXTMEM
DMAMEM bool lastTouchState[4];  // Move to EXTMEM
DMAMEM static uint8_t lastChannelVolBeforeMute[maxY];  // Move to EXTMEM
DMAMEM bool hasNotes[maxPages + 1];  // Consider EXTMEM
DMAMEM unsigned int startTime[maxY];  // Keep (audio timing)
DMAMEM bool noteOnTriggered[maxY];  // Keep (audio ISR)
DMAMEM bool persistentNoteOn[maxY];  // Keep (audio state)
DMAMEM int16_t pressedKeyCount[maxY];  // Move to EXTMEM
```

**Action Items:**
- Review each DMAMEM array's usage (ISR vs main loop)
- Move non-ISR arrays to EXTMEM
- Test audio stability after migration
- Monitor RAM1 usage with `sbrk()` or linker map

---

### 5. Reduce Stack Pressure in `processRecPeaks()`

**Problem:** Local `float interpolatedRecValues[maxX]` allocates on stack every call. With maxX=32, that's 128 bytes per call.

**Impact:** Medium - Stack overflow risk, unnecessary allocation overhead

**Solution:** Move to `static` (or EXTMEM if needed globally).

**Code Reference:**
```cpp
// toern_ui.ino:1858-1882
void processRecPeaks() {
  float interpolatedRecValues[maxX];  // <-- Stack allocation
  // ...
}
```

**Action Items:**
- Change to `static float interpolatedRecValues[maxX]`
- Initialize to zero on first call if needed
- Consider EXTMEM if function is called from ISR context (unlikely)

---

### 6. Implement Dirty-Flag Rendering for Grid/UI

**Problem:** `drawBase()` + `drawTriggers()` redraws entire grid every frame (~30 FPS). Most cells don't change.

**Impact:** Medium-High - Significant CPU savings without audio changes

**Solution:** Track dirty regions (rows/columns) and only redraw changed areas.

**Current Behavior:**
- Every frame: Clear entire grid, redraw base colors, redraw all notes
- Timer cursor updates: Redraws entire column

**Proposed Behavior:**
- Track dirty rows/columns (notes changed, mute state changed, cursor moved)
- Only redraw dirty regions
- Full redraw on mode change or explicit refresh

**Action Items:**
- Add dirty tracking arrays (`bool dirtyRow[maxY]`, `bool dirtyCol[maxX]`)
- Mark dirty when notes change, mutes change, cursor moves
- Modify `drawBase()` and `drawTriggers()` to check dirty flags
- Full redraw on mode switch or user-initiated refresh

---

## Priority 2 — Smaller Wins

### 7. Cache Mute States in `drawTimer()`

**Problem:** `drawTimer()` calls `getMuteState()` per cell, but `drawBase()`/`drawTriggers()` already cache mutes per frame.

**Impact:** Low-Medium - Reduces redundant function calls

**Code Reference:**
```cpp
// toern_ui.ino:1330-1372
void drawTimer() {
  // ...
  if (getMuteState(ch) == false) {  // <-- Called per cell
    // ...
  }
}
```

**Action Items:**
- Pass cached mute array to `drawTimer()` (same cache used by `drawBase()`)
- Or add `drawTimer()` to the same frame cache invalidation logic

---

### 8. Rate-Limit LED Strip Updates

**Problem:** LED strip updates every loop iteration, even when no ripples are active.

**Impact:** Low - Minor CPU savings

**Solution:** Skip update entirely when no active ripples, or throttle to fixed FPS.

**Action Items:**
- Check `ripples[].active` before processing
- Early return if no active ripples
- Consider fixed FPS throttle (30-60 Hz) instead of every loop

---

## Implementation Order Recommendation

1. **Phase 1 (Quick Wins):**
   - Remove per-pixel `yield()` from `light()`
   - Throttle LED strip clearing when idle
   - Move `processRecPeaks()` array to static

2. **Phase 2 (Stability):**
   - Eliminate nested `FastLEDshow()` calls
   - Audit and migrate DMAMEM arrays

3. **Phase 3 (Performance):**
   - Implement dirty-flag rendering
   - Cache mute states in `drawTimer()`

---

## Testing Checklist

After each phase:
- [ ] Audio playback remains stable (no clicks/pops)
- [ ] MIDI timing remains tight (no clock drift)
- [ ] UI remains responsive
- [ ] No visual glitches or flickering
- [ ] RAM1 usage reduced (check with `sbrk()`)
- [ ] CPU load reduced (measure loop time if possible)

---

## Notes

- **RAM1 (OCRAM):** ~512KB total, shared with audio buffers. Every byte counts.
- **EXTMEM:** External RAM (8MB+), slower but fine for UI/state data.
- **DMAMEM:** Must be in RAM1 for DMA operations (LED output, audio buffers).
- **ISR Context:** Timer interrupts (`playNote()`, MIDI handlers) need fast RAM access.

---

## Code Locations Summary

| File | Lines | Function | Issue |
|------|-------|----------|-------|
| `toern_ui.ino` | 45-56 | `light()` | Per-pixel `yield()` |
| `toern_ui.ino` | 869-894 | `drawNumber()` | Internal `FastLEDshow()` |
| `toern_ui.ino` | 1858-1882 | `processRecPeaks()` | Stack allocation |
| `toern_leds.ino` | 164-191 | `updateLedStrip()` | Unnecessary clearing |
| `toern.ino` | 361-573 | Various arrays | DMAMEM usage |
| `toern.ino` | 4626-4658 | `loop()` | Frame rendering |
| `toern.ino` | 5019-5048 | `loop()` | Duplicate `FastLEDshow()` |
