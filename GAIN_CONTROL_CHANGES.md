# Input Gain Control Changes Summary

## Overview
Encoder 1 (normally Channel Volume in `ctrlMode=1`) now controls **Input Gain** when the cursor is on row `y=1` in `DRAW` mode.

## Features

### 1. Input Gain Control
- **Context**: Active only in `DRAW` mode when `y=1` and `ctrlMode=1` (Volume Control Mode).
- **Control**: 
  - If `recMode == 1` (Mic): Controls `micGain` (0-63). Encoder color: **Red**.
  - If `recMode != 1` (Line): Controls `lineInLevel` (0-15). Encoder color: **Blue**.
- **Hardware Update**: Updates `sgtl5000_1.micGain()` or `sgtl5000_1.lineInLevel()` immediately.

### 2. Input Monitoring
- **Auto-Monitor**: When at `y=1` in `DRAW` mode, the audio input is routed to the output (monitor) so the user can hear the level adjustments.
- **Routing**: Sets `mixer_end.gain(3, val)` based on current gain level (mapped to 0.0-0.8 range).
- **Cleanup**: Mutes the monitor channel when leaving `y=1` or `DRAW` mode (unless in `RECORD_MODE`).

### 3. UI Overlay
- **Visual Feedback**: Shows a vertical bar overlay on the LED matrix (column 6-7) when gain changes or upon entry.
- **Gradient**: **Blue-to-Violet** gradient (Hue 160-240) to distinguish it from the volume overlay (Red-to-Green).
- **Persistence**: Shows for 600ms after last change.

## Technical Implementation & Fixes

### State Management & Robustness
To ensure the encoder value remains synchronized with the internal gain variable and doesn't reset to 0 or get stuck at incorrect limits (16), several layers of state tracking were implemented in `toern.ino`:

1.  **Row Change Detection (`yChanged`)**: 
    -   Tracks `lastY` in the outer scope of the control block to reliably detect entry into `y=1` from any other row.
    -   Triggers initialization of the encoder value to the stored `micGain`/`lineInLevel`.

2.  **Mode Change Detection (`modeChangedGlobal`)**:
    -   Added `lastSeenMode` tracking at the start of `checkEncoders()` to detect switching back to `DRAW` mode from menus or other modes.
    -   Forces encoder re-initialization upon return to `DRAW` mode, fixing the "reset to 0 after menu" bug.

3.  **External Reset Protection (Play/Pause Fix)**:
    -   Functions like `play()` or `toggleCopyPaste` call `refreshCtrlEncoderConfig()`, which resets encoders to default volume limits (max 16).
    -   **Fix**: The loop continuously checks if `currentMode->maxValues[1]` matches the expected gain limit (63 or 15).
    -   If a mismatch is found (indicating an external reset occurred), it immediately re-applies the correct limits and **re-triggers initialization**. This restores the correct gain value before any incorrect value (0 from the reset) can be processed.

4.  **Volume Control Handover**:
    -   Sets `ctrlLastChannel = -1` when in `y=1`.
    -   Ensures that moving *away* from `y=1` forces the normal channel volume logic to re-initialize, preventing it from inheriting incorrect settings from the gain control.

## Files Modified
- **`toern.ino`**: 
  -   Main logic inserted into `checkEncoders()` (within `ctrlMode=1` block).
  -   Added `showInputGainOverlay()` helper.
  -   Added global mode tracking logic.
- **`toern_ui.ino`**: 
  -   Added `drawInputGainOverlay()` function for the visual feedback.

