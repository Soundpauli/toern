# Stereo Channel Routing (2-CH) Feature

## Overview

The "2-CH" feature allows routing a selected channel (1-8) to the left output while all other channels go to the right output. Preview is muted when this mode is active.

## Current Implementation Status

**⚠️ LIMITATION:** The current implementation routes **channel groups** rather than individual channels:
- Channels 1-4 are grouped in `mixer1`
- Channels 5-8 are grouped in `mixer2`
- Both groups are mixed in `mixer_end` before reaching the stereo outputs

**Current behavior:**
- Selecting channel 1-4: Routes `mixer1` group (channels 1-4) to L, `mixer2` group (channels 5-8) to R
- Selecting channel 5-8: Routes `mixer2` group (channels 5-8) to L, `mixer1` group (channels 1-4) to R

This is an **approximation** - you cannot isolate a single channel (e.g., channel 3) from the others in its group (1-4) with the current audio routing.

## Required Audio Routing Changes for True Per-Channel Routing

To enable true per-channel stereo routing, you need to modify `audioinit.h`:

### Step 1: Add New Mixers

Add two new 8-input mixers after the existing mixers:

```cpp
EXTMEM AudioMixer4 mixer_channelL;  // 8 inputs for channels 1-8 -> Left
EXTMEM AudioMixer4 mixer_channelR;  // 8 inputs for channels 1-8 -> Right
```

### Step 2: Route Individual Channels

Route each `freeverbmixer` (1-8) to both new mixers:

```cpp
// Channel 1
EXTMEM AudioConnection patchCord_ch1_L(freeverbmixer1, 0, mixer_channelL, 0);
EXTMEM AudioConnection patchCord_ch1_R(freeverbmixer1, 0, mixer_channelR, 0);

// Channel 2
EXTMEM AudioConnection patchCord_ch2_L(freeverbmixer2, 0, mixer_channelL, 1);
EXTMEM AudioConnection patchCord_ch2_R(freeverbmixer2, 0, mixer_channelR, 1);

// ... repeat for channels 3-8
// Channel 3: mixer_channelL input 2, mixer_channelR input 2
// Channel 4: mixer_channelL input 3, mixer_channelR input 3
// Channel 5: mixer_channelL input 4, mixer_channelR input 4
// Channel 6: mixer_channelL input 5, mixer_channelR input 5
// Channel 7: mixer_channelL input 6, mixer_channelR input 6
// Channel 8: mixer_channelL input 7, mixer_channelR input 7
```

### Step 3: Route to Stereo Outputs

Connect the new mixers to `mixer_stereoL` and `mixer_stereoR` using unused inputs (2 or 3):

```cpp
EXTMEM AudioConnection patchCord_chL_stereo(mixer_channelL, 0, mixer_stereoL, 2);
EXTMEM AudioConnection patchCord_chR_stereo(mixer_channelR, 0, mixer_stereoR, 2);
```

### Step 4: Update Routing Function

Modify `applyStereoChannelRouting()` in `toern.ino` to use the new mixers:

```cpp
FLASHMEM void applyStereoChannelRouting() {
  if (stereoChannel == 0) {
    // OFF: Normal routing via mixer_end
    mixer_stereoL.gain(0, 1.0f); // mixer_end -> L
    mixer_stereoL.gain(2, 0.0f);  // mixer_channelL muted
    mixer_stereoR.gain(0, 1.0f); // mixer_end -> R
    mixer_stereoR.gain(2, 0.0f);  // mixer_channelR muted
    mixer_stereoL.gain(1, 1.0f); // preview -> L
    mixer_stereoR.gain(1, 1.0f); // preview -> R
    return;
  }
  
  // Channel selected: route via mixer_channelL/R
  mixer_stereoL.gain(0, 0.0f);  // mixer_end muted on L
  mixer_stereoR.gain(0, 0.0f);  // mixer_end muted on R
  mixer_stereoL.gain(1, 0.0f);  // preview muted
  mixer_stereoR.gain(1, 0.0f);  // preview muted
  
  // Route selected channel to L, others to R
  for (int ch = 1; ch <= 8; ch++) {
    int mixerIdx = ch - 1; // 0-7 for mixer inputs
    if (ch == stereoChannel) {
      mixer_channelL.gain(mixerIdx, 1.0f); // Selected channel -> L
      mixer_channelR.gain(mixerIdx, 0.0f);  // Muted on R
    } else {
      mixer_channelL.gain(mixerIdx, 0.0f);  // Muted on L
      mixer_channelR.gain(mixerIdx, 1.0f);  // Other channels -> R
    }
  }
  
  // Enable the new mixers on stereo outputs
  mixer_stereoL.gain(2, 1.0f); // mixer_channelL -> L
  mixer_stereoR.gain(2, 1.0f); // mixer_channelR -> R
}
```

## Usage

1. Navigate to VOL menu
2. Select "2-CH" page
3. Use encoder 2 to select:
   - **0 (OFF)**: Normal routing (all channels to both L/R, preview active)
   - **1-8**: Selected channel goes to L, all others to R, preview muted

## Notes

- When a channel is selected, preview is automatically muted
- The setting is saved to EEPROM and persists across reboots
- This feature takes precedence over SPLT mode when active
- Synths (channels 11, 13, 14) are not affected by this routing (they remain in mixer_end)

## Future Enhancements

- Add support for routing synths (11, 13, 14) to left/right
- Allow multiple channel selection
- Add panning controls per channel

