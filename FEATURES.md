# TŒRN Device Features

Complete feature list based on codebase analysis.

## SAMPLE VOICES
- **8 sample voices** (channels 1-8)
  - Each voice can load samples up to 12 seconds at 44.1kHz
  - Individual sample trimming (start/end points) per voice
  - Support for up to 999 samples from SD card
  - Sample browser with folder/file organization
  - Live sample loading during playback
  - Sample direction control (forward/reverse)
  - Samplepack 0 support (custom samples per voice)

## SYNTH VOICES
- **1 three-voice polyphonic synth** (channel 11)
  - 3 oscillators per voice (POLY_VOICES = 3)
  - 10 instrument presets: BASS, KEYS, CHPT (chiptune), PAD, WOW, ORG (organ), FLT (flute), LEAD, ARP (arpeggio), BRSS (brass)
  - Independent ADSR envelope per voice
  - Independent filter envelope per voice
  - Per-voice panning and volume control
  - Per-voice pitch offset (cents and semitones)
  - Per-voice waveform selection

- **2 two-oscillator monophonic synths** (channels 13-14)
  - 2 oscillators each
  - Full ADSR envelope control
  - LFO modulation (rate, depth)
  - Arpeggiator (step, span)
  - Waveform selection (SINE, SAW, SQUARE, TRIANGLE)

## ENVELOPES (ADSR)
- **ADSR envelope on all voices** (channels 1-8, 11, 13-14)
  - Attack: 0-32 (maps to milliseconds)
  - Decay: 0-32 (maps to milliseconds)
  - Sustain: 0-32 (maps to level 0-1.0)
  - Release: 0-32 (maps to milliseconds)
- **Filter envelope** (for synth channel 11)
  - Independent ADSR for filter modulation
  - Separate attack, decay, sustain, release controls

## EFFECTS
- **Bitcrusher** (all channels)
  - Bit depth: 1-16 bits (0 = bypass)
  - Sample rate reduction: 1000Hz to 44117Hz
  - Loudness compensation

- **Reverb** (all channels)
  - Room size: 0.0-0.79
  - Damping: 0.01-0.8 (bright to dark)
  - Wet/dry blend with automatic normalization

- **Filters** (all channels)
  - Low-pass filter
  - High-pass filter
  - Band-pass filter
  - Frequency: 0-10000 Hz
  - Resonance: 0.7-5.0
  - Smooth filter transitions

## PITCH & TUNING
- **Transposing**
  - Global transpose (semitones)
  - Per-channel detune: -12 to +12 semitones
  - Per-channel octave shift: -3 to +3 octaves
  - Fine tuning (cents): -50 to +50 cents per voice (synth channel 11)
  - Semitone offset: -2 to +2 semitones per voice (synth channel 11)

## SEQUENCER FEATURES
- **Pattern Structure**
  - 16-bar patterns
  - 16 pages (256 bars per song)
  - Up to 999 patterns storage
  - Real-time pattern switching during playback

- **Note Properties**
  - **Velocity**: 1-16 (maps to MIDI 1-127 internally)
  - **Probability**: 0%, 25%, 50%, 75%, 100%
  - **Condition triggers**: 1/1, 1/2, 1/4, 1/8, 1/X, 2/1, 4/1, 8/1, X/1
  - Per-note channel assignment

- **Song Mode**
  - Pattern chaining (up to 64 positions)
  - Configurable song arrangement
  - Automatic pattern progression

- **Pattern Operations**
  - Copy/paste patterns (page to page, page to voice)
  - Clear page / clear channel
  - Transpose page/channel (±1 octave)
  - Intelligent random note generation (major/minor scales, rhythm patterns)
  - Auto-extend algorithm for custom pattern lengths

- **Playback Modes**
  - Pattern mode (OFF, ON, SONG, NEXT)
  - Manual live pattern switching
  - Mute individual channels or entire patterns
  - Solo mode

## SYNTH MODULATION
- **LFO** (channels 13-14)
  - Rate: 0-2 Hz
  - Depth: 0-100% modulation
  - Applied to filter frequency

- **Arpeggiator** (channels 13-14)
  - Step: 0-12 semitones
  - Span: 2-16 steps (ping-pong pattern)
  - Independent per channel

## AUDIO I/O
- **Inputs**
  - Built-in internal microphone
  - Line-in (6.35mm)
  - Mic-in (6.35mm)
  - Recording directly in sample browser

- **Outputs**
  - Headphone output (6.35mm)
  - Line-out (6.35mm)
  - Stereo output

## MIDI
- **MIDI Input**
  - TRS MIDI in
  - Global MIDI channel selection (1-16)
  - MIDI clock receive
  - MIDI note input
  - Transport control

- **MIDI Output**
  - TRS MIDI out
  - Global MIDI out (channels 1-14)
  - Single MIDI out channel selection (1-16, per voice)
  - MIDI clock send
  - MIDI note output

## CLOCK & TIMING
- **BPM Range**: 40-300 BPM
- **Clock Modes**
  - Internal clock
  - External MIDI clock (via TRS)
  - Tap tempo via touch controls

## SAMPLE MANAGEMENT
- **Sample Browser**
  - Folder/file organization
  - Manifest system (map.txt)
  - Up to 999 samples
  - Real-time browsing during playback
  - Preview mode (ON/PRESS)
  - Peak visualization
  - Sample trimming (seek/seekEnd)
  - Sample reversal

- **Samplepacks**
  - Up to 100 samplepacks
  - Live loading during playback
  - Save/merge samples as samplepack
  - Samplepack 0 (custom per-voice samples)

## DRUM VOICES
- **3 analog drum voices** (can replace 3 sample voices)
  - Bass drum
  - Snare
  - Hi-hat
  - Each with tone height, decay, frequency manipulation

## UI & DISPLAY
- **16x16 RGB LED Matrix**
  - Visual feedback for all operations
  - Pattern visualization
  - Peak waveform display
  - Color-coded channels
  - Condition/probability visual indicators

- **4 RGB I2C Rotary Encoders**
  - Push-button functionality
  - Color-coded per mode
  - Real-time parameter control

- **Touch Buttons**
  - 3 touch-sensitive switches
  - Mode switching
  - Quick access functions

## STORAGE
- **Pattern Storage**: Up to 999 patterns
- **Sample Storage**: Up to 999 samples
- **Samplepack Storage**: Up to 100 samplepacks
- **SD Card**: Up to 32GB Micro SD card support
- **Autosave/Autoload**: Automatic pattern/sample loading

## ADDITIONAL FEATURES
- **Live Recording**
  - 8-channel live-looping
  - Up to 12 seconds per voice
  - Touch-hold recording
  - One-shot recording

- **Context-Aware Randomization**
  - Major/minor scale awareness
  - Rhythm pattern generation
  - Intelligent note placement

- **Per-Channel Gain Control**
  - Adjustable volume per channel
  - Fast filter access per channel

- **Regenerative Pong Mode**
  - Ambient, evolving textures
  - Experimental music creation

- **Multiple View Templates**
  - Optimized workflows
  - Different creative scenarios

- **Fast Filter Access**
  - Encoder #3 quick filter control
  - Per-channel default filter assignment

## TECHNICAL SPECIFICATIONS
- **Microcontroller**: Teensy 4.1 + Audio Board
- **Memory**: 16MB PSRAM
- **Audio Sample Rate**: 44.1kHz
- **Audio Format**: 16-bit WAV (mono)
- **Polyphony**: 3 voices (synth channel 11), 1 voice (synth channels 13-14)
- **Oscillators**: 3 per voice (ch11), 2 per voice (ch13-14)
- **Waveforms**: Sine, Sawtooth, Square, Triangle, Pulse (with variable pulse width)
