# Timer and Timer Display Explanation

## Overview

The timer system tracks playback position and displays a visual indicator on the LED matrix. The behavior differs between **Normal Mode**, **Pattern Mode**, and **FLOW Mode**.

## Key Variables

- **`beat`**: Global beat counter (1, 2, 3, ... up to MAX_STEPS = 256)
- **`beatForUI`**: Snapshot of `beat` used for display (updated at start of `playNote()`)
- **`maxX`**: Display width in columns (16 or 32, depending on LED modules)
- **`GLOB.page`**: Currently displayed page (what you see on screen)
- **`GLOB.edit`**: Currently editable page (what you're editing)

## Timer Display Logic

The timer is drawn by `drawTimer()` function:

```cpp
void drawTimer() {
  unsigned int timer = ((beatForUI - 1) % maxX + 1);
  
  // Calculate which page beatForUI belongs to
  unsigned int beatForUIPage = (beatForUI - 1) / maxX + 1;

  // Determine which page to check against for timer display
  extern bool SMP_FLOW_MODE;
  bool shouldShowTimer = false;
  
  if (SMP_FLOW_MODE) {
    // In FLOW mode, GLOB.page is calculated from beatForUI (the beat being played),
    // so it should match beatForUIPage. Show timer if they match.
    shouldShowTimer = (beatForUIPage == GLOB.page);
  } else {
    // In normal/pattern mode, GLOB.edit is what the user is viewing/editing
    // GLOB.page can automatically switch in normal mode, but GLOB.edit only
    // changes when user manually switches pages via encoder
    // We want to show timer if beatForUI belongs to the page being edited
    shouldShowTimer = (beatForUIPage == GLOB.edit);
  }

  // Show timer if conditions are met
  if (shouldShowTimer) {
    // Highlights the current beat position (x-coordinate) on all rows
    for (unsigned int y = 1; y < maxY; y++) {
      int ch = note[((GLOB.page - 1) * maxX) + timer][y].channel;
      light(timer, y, CRGB(10, 0, 0));  // Dim red background
      
      if (ch > 0 && !getMuteState(ch)) {
        light(timer, y, UI_BRIGHT_WHITE);  // Bright white for active notes
      }
    }
  }
}
```

**Key points:**
- **FLOW Mode**: Timer displays when `beatForUI` belongs to `GLOB.page` (which automatically follows the beat)
- **Normal/Pattern Mode**: Timer displays when `beatForUI` belongs to `GLOB.edit` (the page being edited/viewed)
- `timer` is the X position within the current page: `((beatForUI - 1) % maxX + 1)`
- Timer highlights the entire column (all Y rows) at the current beat position
- Active notes at that position are shown in bright white

## Normal Mode

In **Normal Mode** (`SMP_PATTERN_MODE = false`):

1. **Beat progression**: `beat` increments continuously: 1 → 2 → 3 → ... → 256 → 1 (loops)
2. **Page following**: `GLOB.page` automatically follows the beat position:
   ```cpp
   uint16_t newPage = (beat - 1) / maxX + 1;
   GLOB.page = newPage;
   ```
3. **Display**: Timer shows on whichever page contains the current beat
4. **Looping**: When `beat` exceeds the last page with notes, it resets to 1

**Example with maxX=16, 2 pages (beats 1-32):**

```
Beat 1-16:   Page 1 displayed, timer at x=1..16
Beat 17-32:  Page 2 displayed, timer at x=1..16
Beat 33:     Loops back to beat 1, Page 1 displayed
```

## Pattern Mode

In **Pattern Mode** (`SMP_PATTERN_MODE = true`):

1. **Beat progression**: `beat` wraps within the current pattern (page):
   ```cpp
   unsigned int pageStart = (GLOB.edit - 1) * maxX + 1;
   unsigned int pageEnd   = pageStart + maxX - 1;
   
   beat++;
   if (beat < pageStart || beat > pageEnd) {
     beat = pageStart;  // Wrap within page
   }
   ```
2. **Page display**: `GLOB.page` always equals `GLOB.edit` (stays on editable page)
3. **Display**: Timer always shows on the editable page, wrapping within that page
4. **Manual page switching**: When you change `GLOB.edit` (via encoder), `beat` preserves its relative position within the new page

**Example with maxX=16, 2 pages (beats 1-32), editing Page 1:**

```
Beat 1-16:   Page 1 displayed, timer at x=1..16
Beat 17:     Wraps to beat 1 (pageStart), timer at x=1
Beat 1-16:   Page 1 displayed, timer at x=1..16 (loops)
```

**If you switch to Page 2 (GLOB.edit = 2):**

```
Beat 17-32:  Page 2 displayed, timer at x=1..16
Beat 33:     Wraps to beat 17 (pageStart of Page 2), timer at x=1
Beat 17-32:  Page 2 displayed, timer at x=1..16 (loops)
```

## FLOW Mode

In **FLOW Mode** (`SMP_FLOW_MODE = true`):

1. **Beat progression**: `beat` increments continuously: 1 → 2 → 3 → ... → 256 → 1 (loops)
2. **Page following**: Both `GLOB.page` and `GLOB.edit` automatically follow the timer position:
   ```cpp
   uint16_t timerPage = (beatForUI - 1) / maxX + 1;
   GLOB.page = timerPage;
   GLOB.edit = timerPage;  // Display follows timer position
   ```
3. **Display**: The display automatically switches pages to follow the beat being played
4. **Timer display**: Timer shows on the page that matches the current beat
5. **Looping**: When `beat` exceeds the last page with notes, it resets to 1

**Key difference from Normal Mode**: In FLOW mode, `GLOB.edit` (which controls what page is displayed) automatically follows the beat, so the display always shows the page being played. In Normal mode, `GLOB.edit` only changes manually, so you can view a different page than what's playing.

**Example with maxX=16, 2 pages (beats 1-32):**

```
Beat 1-16:   Page 1 displayed, timer at x=1..16
Beat 17-32:  Page 2 displayed, timer at x=1..16  ← Display automatically switches!
Beat 33:     Loops back to beat 1, Page 1 displayed
```

## Example: 2 Pages (Beat 1-32) with maxX=16

### Normal Mode Behavior

```
Time    beat    GLOB.page  timer (x)  Display
─────────────────────────────────────────────────
T0      1       1          1          Page 1, timer at x=1
T1      2       1          2          Page 1, timer at x=2
...
T15     16      1          16         Page 1, timer at x=16
T16     17      2          1          Page 2, timer at x=1  ← Page switches!
T17     18      2          2          Page 2, timer at x=2
...
T31     32      2          16         Page 2, timer at x=16
T32     33      1          1          Page 1, timer at x=1  ← Loops back
```

**Key behavior:**
- Page automatically switches when beat crosses page boundary
- Timer position resets to x=1 when entering a new page
- Display follows the playback position

### Pattern Mode Behavior (Editing Page 1)

```
Time    beat    GLOB.page  GLOB.edit  timer (x)  Display
─────────────────────────────────────────────────────────────
T0      1       1          1          1          Page 1, timer at x=1
T1      2       1          1          2          Page 1, timer at x=2
...
T15     16      1          1          16         Page 1, timer at x=16
T16     17      1          1          1          Page 1, timer at x=1  ← Wraps within page!
T17     18      1          1          2          Page 1, timer at x=2
...
T31     32      1          1          16         Page 1, timer at x=16
T32     1       1          1          1          Page 1, timer at x=1  ← Loops
```

**Key behavior:**
- Page stays fixed at `GLOB.edit` (Page 1)
- Beat wraps within the page (1-16, then back to 1)
- Timer position wraps within the page (x=1..16, then back to x=1)

### Pattern Mode Behavior (Editing Page 2)

If you manually switch to Page 2 (`GLOB.edit = 2`):

```
Time    beat    GLOB.page  GLOB.edit  timer (x)  Display
─────────────────────────────────────────────────────────────
T0      17      2          2          1          Page 2, timer at x=1
T1      18      2          2          2          Page 2, timer at x=2
...
T15     32      2          2          16         Page 2, timer at x=16
T16     17      2          2          1          Page 2, timer at x=1  ← Wraps within page!
```

**Key behavior:**
- Beat is now in range 17-32 (Page 2's beat range)
- Page stays fixed at Page 2
- Beat wraps within Page 2 (17-32, then back to 17)

### FLOW Mode Behavior

```
Time    beat    beatForUI  GLOB.page  GLOB.edit  timer (x)  Display
─────────────────────────────────────────────────────────────────────────────
T0      1       1          1          1          1          Page 1, timer at x=1
T1      2       2          1          1          2          Page 1, timer at x=2
...
T15     16      16         1          1          16         Page 1, timer at x=16
T16     17      17         2          2          1          Page 2, timer at x=1  ← Auto-switch!
T17     18      18         2          2          2          Page 2, timer at x=2
...
T31     32      32         2          2          16         Page 2, timer at x=16
T32     33      1          1          1          1          Page 1, timer at x=1  ← Loops back
```

**Key behavior:**
- Both `GLOB.page` and `GLOB.edit` automatically follow the beat position
- Display automatically switches pages to show what's playing
- Timer always shows on the current page
- Page calculation uses `beatForUI` (not `beat`) to ensure display matches what's playing

## Why `beatForUI` Exists

**`beatForUI` is needed to prevent race conditions between the interrupt-driven playback and the main loop display.**

### The Problem

1. **`playNote()` runs in an interrupt**: It's called by `IntervalTimer playTimer` (hardware timer interrupt)
2. **`drawTimer()` runs in main loop**: Called from `loop()` which runs asynchronously
3. **`beat` is modified during `playNote()`**: It gets incremented (`beat++`) at the END of the function

### The Solution

`beatForUI` captures the beat value **before** it's incremented:

```cpp
void playNote() {
  // Step 1: Capture current beat for UI (BEFORE incrementing)
  beatForUI = beat;  // ← Snapshot of beat that's being played NOW
  
  // Step 2: Play notes, handle count-in, etc. (uses current beat)
  // ... all the note playing logic ...
  
  // Step 3: Advance to next beat (at END of function)
  beat++;  // ← Now beat points to NEXT beat
}
```

### When `beatForUI` is Manipulated

**`beatForUI` is updated in exactly 2 places:**

1. **At start of `playNote()`** (line 4839):
   ```cpp
   beatForUI = beat;  // Capture beat before incrementing
   ```
   - This happens every beat during playback
   - Ensures UI shows the beat that's currently being played

2. **On pause/reset** (line 4725):
   ```cpp
   beat = 1;
   beatForUI = beat;  // Keep UI in sync with reset position
   ```
   - Ensures UI immediately reflects the reset state

### Why Not Just Use `beat` Directly?

If `drawTimer()` read `beat` directly, there would be a race condition:

```
Timeline:
T0: playNote() interrupt fires
T1: beat = 5 (current beat)
T2: [playNote() processes beat 5]
T3: beat++ → beat = 6 (now points to NEXT beat)
T4: [main loop calls drawTimer()]
T5: drawTimer() reads beat → sees 6 (WRONG! should show 5)
```

With `beatForUI`:
```
Timeline:
T0: playNote() interrupt fires
T1: beat = 5, beatForUI = 5 (snapshot)
T2: [playNote() processes beat 5]
T3: beat++ → beat = 6
T4: [main loop calls drawTimer()]
T5: drawTimer() reads beatForUI → sees 5 (CORRECT!)
```

### Is `beatForUI` Needed?

**Yes, it's necessary** because:
- **Thread safety**: Prevents race conditions between interrupt (playNote) and main loop (drawTimer)
- **Display accuracy**: Ensures UI shows the beat that was actually played, not the next one
- **Display stability**: Provides a stable snapshot that doesn't change mid-frame during display rendering

**Alternative approaches** (not used):
- Using atomic operations (overkill for this use case)
- Disabling interrupts during display (would cause audio glitches)
- Using a mutex (not available/needed on Teensy)

## Bug: Timer Display Glitch on Page Boundaries

**Problem**: When transitioning from one page to the next, there's a brief moment where the timer incorrectly shows x=16 before switching to x=1. Additionally, the timer wasn't showing at x=16 when legitimately on page 1. This affected Normal Mode and FLOW Mode.

### Root Cause

The issue occurs because `beatForUI` is captured **before** the page boundary logic runs:

```cpp
void playNote() {
  beatForUI = beat;  // ← Captures OLD beat (e.g., 16)
  
  // ... play notes ...
  
  beat++;            // ← Increments to NEW beat (e.g., 17)
  checkPages();      // ← Updates GLOB.page based on NEW beat (e.g., page 2)
}
```

**Timeline when beat goes from 16 → 17:**

```
T0: beat = 16, beatForUI = 16 (captured)
T1: beat++ → beat = 17
T2: checkPages() → GLOB.page = 2 (page switches!)
T3: drawTimer() runs:
    - beatForUI = 16 (belongs to page 1)
    - GLOB.page = 2 (automatically switched)
    - GLOB.edit = 2 (user viewing page 2)
    - timer = ((16 - 1) % 16 + 1) = 16  ← WRONG! Should be 1
```

The timer calculation uses `beatForUI = 16` (from old page) but `GLOB.page = 2` (new page), causing a mismatch.

**Key insight**: In normal mode, `GLOB.page` automatically switches to follow the beat, but `GLOB.edit` only changes when the user manually switches pages via encoder. This means `GLOB.page` and `GLOB.edit` can be different during automatic page transitions.

### Fix Applied

The fix uses different logic for FLOW mode vs Normal/Pattern mode:

**For FLOW Mode:**
- `GLOB.page` and `GLOB.edit` are calculated from `beatForUI` (not `beat`)
- Timer shows if `beatForUIPage == GLOB.page` (which should always match)
- This ensures display and timer are synchronized with the beat being played

**For Normal/Pattern Mode:**
- Timer shows if `beatForUIPage == GLOB.edit` (the page being viewed/edited)
- This prevents timer from showing when viewing a different page than what's playing

```cpp
void drawTimer() {
  unsigned int timer = ((beatForUI - 1) % maxX + 1);
  unsigned int beatForUIPage = (beatForUI - 1) / maxX + 1;
  
  bool shouldShowTimer = false;
  
  if (SMP_FLOW_MODE) {
    // FLOW mode: GLOB.page matches beatForUI, so check against GLOB.page
    shouldShowTimer = (beatForUIPage == GLOB.page);
  } else {
    // Normal/Pattern mode: check against GLOB.edit (what user is viewing)
    shouldShowTimer = (beatForUIPage == GLOB.edit);
  }
  
  if (shouldShowTimer) {
    // ... draw timer ...
  }
}
```

**What this fixes:**

1. **Timer shows at x=16 when legitimately on page 1**:
   - Normal/Pattern: `beatForUI = 16` → `beatForUIPage = 1`, `GLOB.edit = 1` → timer shows ✓
   - FLOW: `beatForUI = 16` → `beatForUIPage = 1`, `GLOB.page = 1` → timer shows ✓

2. **Timer doesn't show incorrectly when transitioning to page 2**:
   - Normal/Pattern: `beatForUI = 16` (page 1), `GLOB.edit = 2` → timer hidden ✓
   - FLOW: `beatForUI = 16` (page 1), but `GLOB.page` already updated to 2 → timer hidden ✓

3. **FLOW mode display shows correct page**:
   - `GLOB.page` and `GLOB.edit` calculated from `beatForUI` (not `beat`)
   - Display always matches the page being played ✓

This prevents the timer from displaying incorrectly during page transitions and ensures the timer shows correctly at x=16 in all modes.

## Code Flow

### During Playback (`playNote()` function)

1. **Update UI beat**: `beatForUI = beat;` (capture snapshot at start)
2. **Play notes**: Process all notes for current beat
3. **FLOW Mode**: Calculate page from `beatForUI`, update both `GLOB.page` and `GLOB.edit` to follow timer
4. **Pattern Mode**: Wrap beat within current page, keep `GLOB.page = GLOB.edit`
5. **Normal Mode**: Let beat increment freely, update `GLOB.page` via `checkPages()` (but `GLOB.edit` stays manual)
6. **Advance beat**: `beat++` (at end, points to next beat)
7. **Display**: `drawTimer()` is called from main loop if `isNowPlaying == true` (reads `beatForUI`)

**FLOW Mode page update**:
```cpp
if (SMP_FLOW_MODE && isNowPlaying) {
  uint16_t timerPage = (beatForUI - 1) / maxX + 1;  // Use beatForUI, not beat
  GLOB.page = timerPage;
  GLOB.edit = timerPage;  // Display follows timer position
}
```

**Note**: Using `beatForUI` (not `beat`) ensures `GLOB.page` matches the beat being played, not the next beat.

### Page Calculation

- **Page from beat**: `page = (beat - 1) / maxX + 1`
- **Beat range for page N**: `pageStart = (N - 1) * maxX + 1` to `pageEnd = N * maxX`
- **X position within page**: `x = ((beat - 1) % maxX) + 1`

## Visual Example

For **maxX=16, 2 pages**:

```
Page 1 (beats 1-16):        Page 2 (beats 17-32):
┌─────────────────┐         ┌─────────────────┐
│ x=1  x=2 ...x=16│         │ x=1  x=2 ...x=16│
│  ↓              │         │  ↓              │
│ beat 1-16        │         │ beat 17-32      │
└─────────────────┘         └─────────────────┘
```

**Normal Mode**: Timer moves left→right on Page 1, then automatically switches to Page 2 and continues. Display can show a different page than what's playing (manual page selection).

**Pattern Mode**: Timer moves left→right on the editable page, then wraps back to the left edge of the same page. Page stays fixed until manually changed.

**FLOW Mode**: Timer moves left→right on Page 1, then automatically switches to Page 2. Display automatically follows the timer position, always showing the page being played.

## Summary

| Aspect | Normal Mode | Pattern Mode | FLOW Mode |
|--------|-------------|--------------|-----------|
| **Beat range** | 1 to MAX_STEPS (256) | Wraps within current page (1-16, 17-32, etc.) | 1 to MAX_STEPS (256) |
| **Page switching** | `GLOB.page` automatic, `GLOB.edit` manual | Manual only (via encoder) | Both automatic (follow timer) |
| **GLOB.page** | Follows beat position | Always equals GLOB.edit | Follows beatForUI position |
| **GLOB.edit** | Manual only (user selection) | Manual only (user selection) | Automatic (follows timer) |
| **Timer display** | Shows if `beatForUI` belongs to `GLOB.edit` | Shows if `beatForUI` belongs to `GLOB.edit` | Shows if `beatForUI` belongs to `GLOB.page` |
| **Display behavior** | Can view different page than playing | Shows editable page only | Always shows page being played |
| **Use case** | Long sequences, song mode | Short patterns, live performance | Live performance, visual feedback |

