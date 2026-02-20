Attention: This project is currently under very active development. 
Stay tuned for exciting updates!

---

## External Libraries & Modifications

This project uses several external libraries, some of which have been modified from their standard versions:

### Modified Libraries

- **ResamplingReader.h** (for `teensy-variable-playback` library)
  - Custom implementation added to support variable playback rates with interpolation
  - Enhanced with multi-channel support, loop types (repeat/ping-pong), and crossfading capabilities
  - Located in `src/resamplerReader.h`

- **Freeverb Effect** (`effect_freeverb_dmabuf`)
  - Modified version using DMAMEM for improved memory management on Teensy 4.1
  - Custom implementation optimized for the audio processing pipeline
  - Located in `src/effect_freeverb_dmabuf.h` and `src/effect_freeverb_dmabuf.cpp`

- **FastLED Library**
  - Configured with `FASTLED_ALLOW_INTERRUPTS 0` to prevent timing conflicts with audio processing
  - Custom wrapper functions (`FastLEDshow()`, `FastLEDclear()`) for coordinated display updates

- **MIDI Library**
  - Custom instance created with modified buffer sizes:
    - `SERIAL8_RX_BUFFER_SIZE 2048` (increased from default 64 for high-frequency clock messages)
    - `SERIAL8_TX_BUFFER_SIZE 128` (increased for safety)
  - Custom MIDI settings struct for TRS MIDI communication

### Standard Libraries Used

- **TeensyPolyphony** by Nic Newdigate — Core polyphonic audio engine
- **Audio Library** (PJRC) — Standard Teensy audio processing
- **WS2812Serial** — LED matrix driver
- **i2cEncoderLibV2** — Rotary encoder control
- **FastTouch** — Touch button interface

---

## License

- **Software/code**: MIT License — Free for personal and commercial use (see [LICENSE](./LICENSE)).
- **Hardware design files** (schematics, PCB layouts, Gerbers, etc.):  
  Licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/).  
  Free to use and modify for personal, non-commercial purposes only.  
  Commercial use is **not permitted** without prior written consent.

For commercial licensing of the hardware, please contact:  
Jan-Peter Kuntoff — jpkuntoff@gmail.com (Hamburg, Germany)


# TŒRN [tɜːn]:
**DIY Open-Source Hardware Sampler-Sequencer**  
A sequenced driven musical device for beginners, makers and enthusiasts.


---

## INTRODUCING TŒRN  
The TŒRN (spoken as: [tɜːn]) builds on the legacy of the NI404 with **enhanced features**, a streamlined design, and superior functionality. From upgraded RGB-I2C rotary encoders to a custom PCB with everything pre-assembled, tœrn_x408 is designed to be the ultimate **DIY sampler-sequencer**—accessible, powerful, and inspiring.  

No more additional cables or extensions: every component is directly mounted on the PCB, ensuring ease of assembly and a clean build. With **new connectivity options**, expanded audio capabilities, and an updated design, this device is ready to meet the needs of modern music creators.  

---

## WHAT'S NEW IN TŒRN 

- **Upgraded Encoders**: High-quality RGB-I2C rotary encoders for smoother and more precise control.  
- **Custom PCB**: Fully assembled, simplifying the building process—just order and put it together.  
- **USB-C Port**: Replacing micro-USB for faster and more reliable connectivity.  
- **Battery Charging Port**: Added support for internal LiPo batteries (not included) for true portability.  
- **Expanded Audio I/O**:  
  - **6.35mm headphone output** for studio-grade monitoring.  
  - **Line-in and line-out jacks (6.35mm)** for external audio integration.  
  - **Mic-in (6.35mm)** for vocal recording.  
  - **Built-in internal microphone** for direct sampling without extra hardware.  
- **TRS MIDI In/Out Ports**: Enhanced MIDI capabilities for seamless integration into larger setups.  
- **On/Off Switch**: Power control for added convenience.  
- **Touch Buttons**: For enabling advanced features and future expansions.  
- **New Casing**: Laser-cut acrylic housing for a polished, durable finish (files included).  
- **No soldering required**: All connections are JST-Connectors for easy building

---

## OPEN-SOURCE AND COMMUNITY-DRIVEN  
As always, TŒRN remains **fully open-source**, with all schematics, code, and design files available for download. The project encourages collaboration and creativity, inviting users to tweak, modify, and share their own innovations.  

---

## FEATURES  

- **Playful Design**: Inspired by the Etch-A-Sketch™ for intuitive and beginner-friendly music creation.  
- **Vivid 16x16 RGB LED Grid**: Dynamic visual feedback for a seamless creative process.  
- **Real-Time Control**: Adjust parameters like BPM, volume, effects, and more on the fly—ideal for live performances.  
- **Customizable Workflow**: Load your own samples (WAV format) via SD card. Supports 8 sample voices plus 3 synth voices.  

### **Sequencer**
- **16 channels total**: 8 sample voices (channels 1-8), 1 three-voice polyphonic synth (channel 11), 2 monophonic synths (channels 13-14)
- **Pattern Structure**: 16 steps per page, 16 pages (256 steps total per song)
- **Storage**: Up to 999 patterns, 999 samples, 100 samplepacks
- **Autosave/autoload** functionality for convenience
- **Real-time pattern switching** during playback without stopping
- **Song Mode**: Chain up to 64 patterns into complete songs
- **Pattern Operations**: Copy/paste patterns, transpose page/channel (±1 octave), intelligent random note generation (major/minor scales, rhythm patterns), clear page/channel

### **Sample Voices** (Channels 1-8)
- **8 parallel sample voices** with individual sample assignment
- Each voice can load samples up to 12 seconds at 44.1kHz
- **Sample Browser**: Manage up to 999 samples from SD card in real-time during playback
- **Sample Trimming**: Individual start/end points per voice (0-100% seek/seekEnd)
- **Sample Direction**: Forward/reverse playback control
- **Samplepack Support**: Up to 100 samplepacks, live loading during playback
- **Samplepack 0**: Custom per-voice sample assignment
- **Recording**: Direct recording from built-in mic/line-in/mic-in in sample browser (input selectable via settings)
- **Save/Merge**: Save used samples as samplepack to SD card

### **Synth Voices**
- **Channel 11**: Three-voice polyphonic synth
  - 3 oscillators per voice (POLY_VOICES = 3)
  - 10 instrument presets: BASS, KEYS, CHPT (chiptune), PAD, WOW, ORG (organ), FLT (flute), LEAD, ARP (arpeggio), BRSS (brass)
  - Independent ADSR envelope per voice
  - Independent filter envelope per voice
  - Per-voice panning, volume, pitch offset (cents and semitones), waveform selection
- **Channels 13-14**: Two monophonic synths
  - 2 oscillators each
  - Full ADSR envelope control
  - LFO modulation (rate, depth)
  - Arpeggiator (step, span)
  - Waveform selection (SINE, SAW, SQUARE, TRIANGLE)

### **Effects** (All Channels)
- **ADSR Envelope**: Attack, Decay, Sustain, Release (0-32 range)
- **Bitcrusher**: Bit depth (1-16 bits, 0 = bypass), sample rate reduction (1000Hz to 44117Hz)
- **Filter**: Low-pass, High-pass, Band-pass filters
  - Frequency: 0-10000 Hz
  - Resonance: 0.7-5.0
  - Smooth filter transitions
- **Reverb**: Room size (0.0-0.79), damping (0.01-0.8), wet/dry blend
- **Fast Filter Access**: Assignable default filter per channel, directly accessible on encoder #3 during runtime

### **MIDI**
- **MIDI Input**: TRS MIDI in, global MIDI channel selection (1-16), MIDI clock receive, note input, transport control
- **MIDI Output**: TRS MIDI out, global MIDI out (channels 1-14), single MIDI out channel selection (1-16 per voice), MIDI clock send, note output

### **Clock & Timing**
- **BPM Range**: 40-300 BPM
- **Clock Modes**: Internal clock, external MIDI clock (via TRS), tap tempo via touch controls

### **Additional Features**
- **Live Recording**: 8-channel live-looping, up to 12 seconds per voice, touch-hold recording
- **Note Properties**: Velocity (1-16, maps to MIDI 1-127), probability (0%, 25%, 50%, 75%, 100%), condition triggers (1/1, 1/2, 1/4, 1/8, 1/X, 2/1, 4/1, 8/1, X/1)
- **Pitch & Tuning**: Global transpose (semitones), per-channel detune (-12 to +12 semitones), per-channel octave shift (-3 to +3 octaves), fine tuning (-50 to +50 cents for synth channel 11)
- **Mute/Solo**: Mute individual channels or entire patterns, solo mode
- **Pattern Mode**: OFF, ON, SONG, NEXT modes
- **Stereo Routing**: Configurable stereo channel routing (main/preview split, L+R channel separation)
- **LED Strip**: External WS2812 strip connector for ripple visualization synced to audio and triggered notes

---

## TECH SPECS 

- **Microcontroller**: Teensy 4.1 + Audio Board
- **Memory**: 16MB PSRAM, slot for up to 32GB Micro SD card
- **Audio Sample Rate**: 44.1kHz, 16-bit WAV (mono)
- **Encoders**: 4x RGB illuminated I2C rotary-push encoders
- **Display**: 16x16 RGB LED Matrix (powered by FastLED), supports up to 2 modules (32x16)
- **LED Strip**: 5V WS2812 strip connectable externally via onboard connector (ripple visualization, configurable length)
- **Audio I/O**: 
  - 6.35mm headphone output
  - 6.35mm line-in and line-out jacks
  - 6.35mm mic-in jack
  - Built-in internal microphone
- **MIDI**: TRS MIDI in/out ports
- **Power**: USB-C or optional LiPo battery with charging port
- **Touch Controls**: 3 touch-sensitive switches  
---

## GET INVOLVED  

All source code and 3D files are available under the MIT License. Whether you're a maker, developer, or musician, the tœrn_x408 invites you to explore, modify, and share your ideas.  


**Order a TŒRN**: Don’t want to build your own? Contact us at **JPKuntoff@gmail.com** to purchase a pre-assembled unit or collaborate on your custom build.  

---

## THANK YOU  
Special thanks to Paul Stoffregen and the PJRC team for the incredible Teensy platform, as well as the open-source community for contributing essential libraries. A heartfelt shoutout to Nic Newdigate for the teensy-polyphony library, the soul of this project.  

Jan from SP_ctrl (formaly known as soundpauli)
Hamburg, February 2026  
