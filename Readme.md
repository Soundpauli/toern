Attention: This project is currently under very active development. The code and files are expected to be ready by May 25, 2025. 
At this stage, the project is not yet functional, but exciting updates are on the way. Stay tuned!


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
please use added ResamplingReader.h for lib "teensy-variable-playback"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


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
- **Customizable Workflow**: Load your own samples (WAV format) via SD card. Supports up to 8 voices and an additional onboard synth voice.  

- **Powerful Sequencer**:  
- Autosave/autoload functionality for convenience.  
- Sample Browser: Manage up to 999 samples from the SD card in real-time and during playback  
- 16-bar patterns across 16 pages (256 bars per song).  

- 8 parallel sample voices with added effects: ADSR-envelope, bitcrusher, filter (high/low/pass), reverb (4x), each voice can freely assign one of the 999 samples
- With added effects: ADSR-envelope, bitcrusher, filter (high/low/pass), reverb
- All samples can be cut to size for each voice indiviually (start/end) befor loading to voice
- Recording inbuild-mic/line-in/mic-in directly in sample browser for quick access (input via settings page)
- Possibility to save / merge used samples as samplepack to SD card for easy access
  
- Changing /loading up to 100 samplepacks live during runtime/playing without stopping
- Changing /loading up to 100 patterns or songs live during runtime/playing without stopping
- Selecting global midi-TRS-in-channel (1-16)
- Global midi-TRS out (1-14, for voice 1-14)
- Selecting single midi-TRS out channel (1-16) if output-channel should be fixed / pinned down to a single voice
- Clock: 40 - 300bpm, you can change between send the local or receive an external MIDI-Clock (via TRS)

- Copy+paste of current view/page
- Intelligent random notes generation (based on maj/min) tonescale as well as rythm per page
- Transposing and/or shifting current channel / current page within (+/- 1 octave)
- Delete all notes / current channel notes of a page via shortcut

- **Synth and Filter Features**:
- 3 of the Sample Voices can be switched to each one of three analog drum voices (bassdrum, snare and highhat), each with tonehight / decay / freqency manupilation
- 1 three-voice synth with 3 OSCs per voice with added effects: ADSR-envelope, bitcrusher, filter (high/low/pass), reverb
- 2 one-voiced synths with each 2 OSCs with added effects: ADSR-envelope, bitcrusher, filter (high/low/pass), reverb
- 1 assignable default filter for each channel which can be easily directly accessed during runtime on encoder #3

---

## TECH SPECS 

- **Microcontroller**: Teensy 4.1 + Audio Board.  
- **Memory**: 16MB PSRAM, slot for 32GB SD Micro SD card
- **Encoders**: 4x colorfull illuminated I2C rotary-push encoders
- **Display**: 16x16 RGB LED Matrix (powered by FastLED).  
- **Audio I/O**: 6.35mm jacks for headphones, line-in/out, mic-in; internal microphone.  
- **Power**: USB-C or optional LiPo battery with charging port.  
- **Ports**: USB-C or optional LiPo battery with charging port.  
---

## GET INVOLVED  

All source code and 3D files are available under the MIT License. Whether you're a maker, developer, or musician, the tœrn_x408 invites you to explore, modify, and share your ideas.  


**Order a TŒRN**: Don’t want to build your own? Contact us at **JPKuntoff@gmail.com** to purchase a pre-assembled unit or collaborate on your custom build.  

---

## THANK YOU  
Special thanks to Paul Stoffregen and the PJRC team for the incredible Teensy platform, as well as the open-source community for contributing essential libraries. A heartfelt shoutout to Nic Newdigate for the teensy-polyphony library, the soul of this project.  

Jan from SP_ctrl (formaly known as soundpauli)
Hamburg, April 2025  
