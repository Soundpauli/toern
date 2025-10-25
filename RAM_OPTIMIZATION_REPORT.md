# RAM Optimization Report

## Summary
Analyzed the codebase for RAM optimization opportunities on Teensy 4.1. Moved constant data from RAM to PROGMEM (flash memory) to free up precious RAM for runtime operations.

## Memory Layout Analysis

### Large Buffers (Correctly Placed in EXTMEM/DMAMEM)
These are already optimized and use external RAM:
- `sampled[9][~1MB]` = ~9.5MB in EXTMEM for audio sample storage ✅
- `note[maxlen+1][maxY+1]` = pattern data in EXTMEM ✅
- `tmp[maxlen+1][maxY+1]` = temporary pattern storage in EXTMEM ✅
- `original[maxlen+1][maxY+1]` = pattern backup in EXTMEM ✅
- `recBuffer[BUFFER_SAMPLES]` = ~882KB in EXTMEM for audio recording ✅
- `particles[256]` = ~6KB in DMAMEM for animation ✅

## Optimizations Implemented

### 1. **notes.h** - 540 bytes saved
- `notesArray[108]` → PROGMEM (432 bytes)
- `pianoNoteNames[27]` → PROGMEM (108 bytes)

### 2. **toern.ino** - ~100 bytes saved
- `instTypeNames[]` → PROGMEM
- `sndTypeNames[]` → PROGMEM  
- `waveformNames[]` → PROGMEM
- `activeMidiSetType[6]` → PROGMEM

### 3. **icons.h** - ~2KB saved
All icon coordinate arrays moved to PROGMEM:
- `logo[16][16]` (256 bytes)
- `noSD[48][2]` (96 bytes)
- `number[10][24][2]` (480 bytes)
- `icon_rec`, `icon_rec2`, `icon_settings`, `icon_mic` (~100 bytes)
- `icon_move`, `icon_line` (~72 bytes)
- `icon_delete[49][2]` (98 bytes)
- `icon_samplepack`, `icon_sample` (~68 bytes)
- `icon_loadsave`, `icon_loadsave2` (~38 bytes)
- `icon_bpm[33][2]` (66 bytes)
- `icon_hourglass[23][2]` (46 bytes)
- `icon_new[5][2]` (10 bytes)
- All helper icons: `helper_load`, `helper_folder`, `helper_seek`, etc. (~100 bytes)

### 4. **font_3x5.h** - Already optimized ✅
- `alphabet[95][4]` already marked FLASHMEM

### 5. **colors.h** - Consider for future optimization
Color arrays like `col_Folder[]`, `filter_col[]`, `col[]`, `col_base[]`, `filterColors[]` could be moved to PROGMEM if RAM becomes critical (~500-800 bytes potential savings). Not implemented yet as CRGB arrays require careful handling with PROGMEM access.

## Total RAM Saved: ~2.6-3KB

This frees up approximately 2.6-3KB of precious DTCM RAM (the fast on-chip RAM) which is critical for:
- Stack space
- Fast variable access
- Real-time audio processing
- MIDI buffer handling

## Additional Optimization Opportunities (Not Implemented)

### Potential Further Savings:

1. **Peak Value Arrays** (~4KB)
   - `peakValues[512]` = 2KB
   - `peakRecValues[512]` = 2KB
   - Could reduce to `[256]` each if visualization resolution can be halved

2. **Animation Particles** (~6KB in DMAMEM)
   - `particles[256]` could be reduced to `[128]` if animation complexity allows

3. **Color Arrays** (~500-800 bytes)
   - Move CRGB arrays in `colors.h` to PROGMEM (requires pgm_read_dword_near() access)

4. **EEPROM Storage Candidates**
   - Menu configuration data could be stored in EEPROM and loaded on demand
   - Last used settings/preferences
   - Calibration data

## How to Access PROGMEM Data

When reading from PROGMEM on Teensy/ARM Cortex, use:
```cpp
// For bytes/integers:
uint8_t value = pgm_read_byte(&array[index]);

// For floats:
float freq = pgm_read_float(&fullFrequencies[i]);

// For pointers:
const char* name = (const char*)pgm_read_ptr(&pianoNoteNames[i]);
```

## Verification

To verify RAM usage after these changes, check the build output:
```
Sketch uses X bytes (Y%) of program storage space.
Global variables use Z bytes (W%) of dynamic memory.
```

The dynamic memory (Z) should be reduced by ~2.6KB after these optimizations.

## Next Steps

1. Compile and test all functionality
2. Monitor RAM usage in build output
3. If more RAM is needed, consider:
   - Reducing peak value array sizes
   - Moving color arrays to PROGMEM
   - Identifying any other large const arrays
   - Using EEPROM for configuration data

## Notes

- EXTMEM and DMAMEM are separate from the main DTCM RAM, so those allocations don't impact the 512KB-1MB internal RAM
- The Teensy 4.1 has 1MB of DTCM RAM, but stack, heap, and static variables all compete for this space
- Audio processing requires consistent memory access times, so keeping frequently accessed data in fast RAM is important
- PROGMEM data stays in flash (2MB+) which is abundant but slower to access than RAM

