# Display Value Ranges Reference

This document lists all parameter ranges **as displayed on screen** in the Toern sequencer.

## Filter Settings (Filter Mode)
All filter settings are displayed as **numeric values 0-32** on screen:

- **PASS**: Displayed as **0-32**
- **FREQUENCY**: Displayed as **0-32**
- **REVERB**: Displayed as **0-32**
- **BITCRUSHER**: Displayed as **0-32**
- **DETUNE**: Displayed as **0-32**
- **OCTAVE**: Displayed as **0-32**
- **RES** (Resonance): Displayed as **0-32**
- **RATE**: Displayed as **0-32**
- **AMOUNT**: Displayed as **0-32**
- **OFFSET**: Displayed as **0-32**
- **SPEED**: Displayed as **0-32**
- **PITCH**: Displayed as **0-32**
- **ACTIVE**: Displayed as **0-32**
- **EFX**: Displayed as **0-32**
- **FILTER_WAVEFORM**: Displayed as **enum names** (SIN, SQR, SAW, TRI) - 4 options shown as text

## Envelope Parameters (ADSR)
All envelope parameters are displayed as **numeric values 0-32** on screen:

- **ATTACK**: Displayed as **0-32**
- **DECAY**: Displayed as **0-32**
- **SUSTAIN**: Displayed as **0-32**
- **RELEASE**: Displayed as **0-32**

## Synth Settings
Most synth settings are displayed as **numeric values 0-32** on screen:

- **CUTOFF**: Displayed as **0-32**
- **RESONANCE**: Displayed as **0-32**
- **FILTER**: Displayed as **0-32**
- **CENT**: Displayed as **0-32**
- **SEMI**: Displayed as **0-32**
- **INSTRUMENT**: Displayed as **enum names** (BASS, KEYS, CHPT, PAD, WOW, ORG, FLT, LEAD, ARP, BRSS) - 10 options shown as text
- **FORM**: Displayed as **0-32**
- **LFO_RATE**: Displayed as **0-32**
- **LFO_DEPTH**: Displayed as **0-32**
- **LFO_PHASE**: Displayed as **0-32** (repurposed as ARP_SPAN for ch13/14)
- **ARP_STEP**: Displayed as **0-32**

## Note Properties (Velocity Mode)
When editing notes in Velocity Mode, values are displayed as:

- **Velocity**: Displayed as **1-16** (maps to MIDI velocity 1-127 internally)
- **Probability**: Displayed as **1-5** with visual bars:
  - 1 = 0% (red bar)
  - 2 = 25% (orange bar)
  - 3 = 50% (yellow bar)
  - 4 = 75% (turquoise bar)
  - 5 = 100% (green bar)
- **Channel Volume**: Displayed as **0-16** (visual bar, maps to gain 0.0-1.0 internally)
- **Condition**: Displayed as **fraction text** (1/1, 1/2, 1/4, 1/8, 1/X, 2/1, 4/1, 8/1, X/1) - 9 positions shown as fractions

## Audio Settings
- **Main Volume**: Displayed as **0-100** (numeric)
- **Preview Volume**: Displayed as **0-50** (numeric)
- **Line Out Level (LOUT)**: Displayed as **13-31**
- **Mic Gain**: Displayed as **0-64**
- **Line In Level (L-IN)**: Displayed as **0-15**
- **Channel Volume**: Displayed as **0-16** (visual bar on screen)

## Sequencer Settings
- **BPM**: Displayed as **40-300**
- **Pages**: Displayed as **1-16**
- **Pattern Length**: Displayed as **1 to 256 steps**
- **Beat Position**: Displayed as **1 to 256**
- **X Position (Step)**: Displayed as **1 to 256**
- **Y Position (Row/Channel)**: Displayed as **1-16**

## Sample Management
- **Folders**: Displayed as **0 to (folder count - 1)**, max 32 folders
- **Files per Folder**: Displayed as **1-999**
- **Sample Pack ID**: Displayed as **1-99**
- **File ID**: Displayed as **1-999** (within a folder)

## Menu Settings
- **Menu Pages**: Displayed as **1-10**
- **LOOK/PLAY Pages**: Displayed as **1-10**
- **RECS Pages**: Displayed as **1-3**
- **MIDI Pages**: Displayed as **1-2**
- **VOL Pages**: Displayed as **1-7**
- **ETC Pages**: Displayed as **1-2**

## Song Mode
- **Song Position**: Displayed as **1-64**
- **Pattern Selection**: Displayed as **1-16**

## Velocity Mode (Encoder Display)
When in Velocity Mode, encoders show:

- **Encoder 0 (Velocity)**: Displayed as **1-16** (numeric)
- **Encoder 1 (Probability)**: Displayed as **1-5** (numeric, with visual bar)
- **Encoder 2 (Channel Volume)**: Displayed as **0-16** (numeric, with visual bar)
- **Encoder 3 (Condition)**: Displayed as **1-9** (shown as fraction text: 1/1, 1/2, 1/4, 1/8, 1/X, 2/1, 4/1, 8/1, X/1)

## Note Shift Mode
- **Encoder 0 (Source Channel)**: Displayed as **7-9**
- **Encoder 1 (Target Channel)**: Displayed as **7-9**
- **Encoder 2 (Filter)**: Displayed as **0-32**
- **Encoder 3 (Target Row)**: Displayed as **7-9**

## Draw/Single Mode
- **Encoder 0 (Y/Channel)**: Displayed as **1-16**
- **Encoder 1 (Page)**: Displayed as **1-16**
- **Encoder 2 (Filter/Parameter)**: Displayed as **0-32**
- **Encoder 3 (X/Step)**: Displayed as **1 to 256**

## Filter Mode
- **All Encoders**: Displayed as **0-32** (numeric values shown in corner)

## Volume/BPM Mode
- **Encoder 0**: Displayed as **11-30** (volume range, but volume controls removed)
- **Encoder 1**: Displayed as **11-30** (volume range, but volume controls removed)
- **Encoder 2**: Displayed as **0-1** (INT/EXT toggle)
- **Encoder 3**: Displayed as **40-300** (BPM)

## Set WAV Mode
- **Encoder 0 (Seek Start)**: Displayed as **0-100** (percentage)
- **Encoder 1 (Seek End)**: Displayed as **1-100** (percentage, must be > Seek Start)
- **Encoder 2 (Folder)**: Displayed as **0 to (folder count - 1)**, max 31 folders
- **Encoder 3 (File)**: Displayed as **1-999** (files in folder + 1 for NEW slot)

## Record Mode
- **Encoder 0 (Seek)**: Displayed as **0-100** (percentage)
- **Encoder 1 (Folder)**: Displayed as **1-10**
- **Encoder 2 (File)**: Displayed as **1-9999**
- **Encoder 3 (File)**: Displayed as **1-999**

## Load/Save Track Mode
- **Encoder 0**: Displayed as **1-1**
- **Encoder 1**: Displayed as **1-1**
- **Encoder 2**: Displayed as **0-1**
- **Encoder 3**: Displayed as **1-99**

## Sample Pack Mode
- **Encoder 0**: Displayed as **1-1**
- **Encoder 1**: Displayed as **1-1**
- **Encoder 2**: Displayed as **1-99**
- **Encoder 3**: Displayed as **1-99**

## MIDI Settings
- **MIDI Channel**: Displayed as **1-16**
- **MIDI In/Out**: Displayed as **Boolean** (ON/OFF text)
- **Send/Receive Control**: Displayed as **Boolean** (ON/OFF text)

## Display Settings
- **LED Modules**: Displayed as **1-2**
- **Matrix Width**: **16** (fixed)
- **Max X (Steps per page)**: Displayed as **16 or 32** (depending on LED modules)
- **Max Y (Rows/Channels)**: Displayed as **16**

## Input Monitoring
- **State**: Displayed as **0-2**
  - 0 = OFF
  - 1 = ON (only when y==1)
  - 2 = ALL (always on)

## Preview Mode
- **Mode**: Displayed as **0-1**
  - 0 = PREVIEW_MODE_ON (auto-preview)
  - 1 = PREVIEW_MODE_PRESS (preview on press)

## Pattern Mode (PMOD)
- **Mode**: Displayed as **-1, 1, 2**
  - -1 = OFF
  - 1 = PATTERN
  - 2 = SONG

## Transport Mode
- **Mode**: Displayed as **-1, 1, 2**
  - -1 = OFF
  - 1 = GET
  - 2 = SEND

## Voice Select
- **Mode**: Displayed as **-1, 1, 2**
  - -1 = YPOS
  - 1 = MIDI
  - 2 = KEYS

## Draw Mode
- **Mode**: Displayed as **0-1**
  - 0 = L+R mode
  - 1 = R mode

## Reset Menu Option
- **Option**: Displayed as **0-1**
  - 0 = EFX (reset effects)
  - 1 = SD (rescan SD card)

## File Operations
- **Track/Song Save Slot**: Displayed as **1-99**
- **Sample Pack Save Slot**: Displayed as **1-99**

## Condition Display Format
When displayed on screen, conditions appear as **fraction text**:

- Position 1 = **"1/1"** (green)
- Position 2 = **"1/2"** (blue)
- Position 3 = **"1/4"** (violet)
- Position 4 = **"1/8"** (orange)
- Position 5 = **"1/X"** (turquoise, represents 1/16)
- Position 6 = **"2/1"** (light blue)
- Position 7 = **"4/1"** (light violet)
- Position 8 = **"8/1"** (light orange)
- Position 9 = **"X/1"** (light turquoise, represents 16/1)

## Display Format Notes
- **Numeric values**: Most parameters show raw numbers (0-32, 1-16, etc.)
- **Enum values**: Some parameters show text labels (WAVEFORM: SIN/SQR/SAW/TRI, INSTRUMENT: BASS/KEYS/etc.)
- **Visual bars**: Some values have visual representations (probability bars, channel volume bars)
- **Percentages**: Seek positions are shown as 0-100%
- **Fractions**: Conditions are shown as fraction text (1/1, 1/2, etc.)
- **Colors**: Different values may have different colors on the LED display (e.g., condition fractions have specific colors)

## Internal vs Display Values
- Most filter/parameter/synth values are stored internally as 0-32 but displayed as 0-32
- Velocity is stored internally as 0-127 (MIDI) but displayed as 1-16
- Probability is stored internally as 0-100% but displayed as 1-5 (with visual bars)
- Condition is stored internally as values 1,2,4,8,16,17,18,19,20 but displayed as fraction text
- WAVEFORM is stored internally as 0-16 but displayed as enum names (0-3 range)
- Channel numbers are displayed as 1-16 (1-indexed) but stored internally as 0-15 (0-indexed)
- Page numbers are displayed as 1-16 (1-indexed)
- Pattern/step positions are displayed as 1-256 (1-indexed)
