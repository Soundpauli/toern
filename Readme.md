# TŒRN `[tɜːn]`

**DIY open-source hardware sampler-sequencer** — by [warft_ctrl](mailto:jpkuntoff@gmail.com)

> Say it like *turn*. As in: turn the knobs. Turn a doodle into a beat. Turn “I can’t play piano” into “wait, that slap though.”

TŒRN is a sequenced musical device for beginners, makers, and anyone who likes blinking lights more than plugin menus. Think **Etch-A-Sketch™ energy**, but instead of drawing a wobbly staircase you draw drums, synths, and happy accidents — on a vivid **16×16 RGB LED matrix** that is both the screen *and* the keyboard.

It sits on a Teensy 4.1, runs from a Micro SD card full of your WAVs, and is happy on a desk, in a backpack, or on battery at the park where strangers will ask “what *is* that?”

**Status:** under very active development. Things move fast. Stay tuned, and don’t be surprised if yesterday’s weird bug is today’s charming feature (or the other way around).

---

## What it is (in plain language)

You get:

- **8 sample voices** for drums, one-shots, loops, and “that weird recording from the metro”
- **1 three-voice polyphonic synth** with presets (bass, keys, chiptune, pads… the usual suspects)
- **2 monophonic synths** with LFO and arpeggiator for leads that refuse to sit still
- A **16-step × 16-page** sequencer (256 steps of room to overthink), pattern chaining into songs, MIDI in/out, filters, bitcrush, reverb, and enough real-time knobs to keep your hands busy and your laptop jealous

No laptop required once it’s built. Samples live on the SD card. Patterns autosave. You twist, tap, and suddenly it’s 2 a.m.

For the friendly guided tour (first beat in about a minute), open the **[handbook](./handbook/index.html)** in this repo.

---

## A tiny history

TŒRN builds on the legacy of the **NI404**, with more features, fewer cables dangling off the side, and a custom PCB where everything mounts cleanly. The older “extra boards and hope” era is over.

Meet the glow-up:

- **RGB-I2C rotary encoders** — smoother, prettier, and they light up like they mean it  
- **Custom PCB** — order it, put it together; the hard wiring homework is mostly done  
- **USB-C** — because micro-USB was a character-building experience we no longer need  
- **LiPo charging port** — optional battery for true “make beats on a train” energy (battery not included; we trust you with electricity)  
- **Proper audio I/O** — 6.35 mm headphone out, line in/out, mic in, *plus* a built-in mic for “sample the room right now” moments  
- **TRS MIDI in/out** — play nice with the rest of your setup  
- **On/off switch** — underrated luxury  
- **Touch buttons** — for the things knobs shouldn’t monopolize  
- **Laser-cut acrylic case** — files included; looks finished, not “breadboard cosplay”  
- **JST connectors** — **no soldering required** for the usual build path  

Open source as always: schematics, code, and design files are here for you to poke, fork, and improve.

---

## Features (the longer version)

### Sequencer

- **16 channels** in spirit: 8 sample voices (1–8), one 3-voice poly synth (11), two mono synths (13–14)  
- **16 steps per page × 16 pages** → 256 steps per song arrangement space  
- Store up to **999 patterns**, **999 samples**, **100 samplepacks**  
- **Autosave / autoload** so your 3 a.m. genius doesn’t vanish at boot  
- **Switch patterns live** without stopping playback  
- **Song mode**: chain up to **64** patterns into something resembling a composition  
- Pattern ops: copy/paste, transpose page/channel (±1 octave), smart random notes (major/minor + rhythm ideas), clear page/channel  

### Sample voices (channels 1–8)

- Up to **~12 seconds** per voice at 44.1 kHz  
- Browse and load samples **while playing** (yes, really)  
- Per-voice start/end trim, forward/reverse  
- Samplepacks (including pack 0 = custom per-voice assignment)  
- Record from **built-in mic / line-in / mic-in** straight in the sample browser  
- Save or merge used samples back to the SD card as a pack  

### Synth voices

**Channel 11 — poly synth**

- 3 oscillators per voice × 3 voices  
- Presets: BASS, KEYS, CHPT, PAD, WOW, ORG, FLT, LEAD, ARP, BRSS  
- Per-voice ADSR, filter envelope, pan, volume, pitch (cents & semitones), waveform  

**Channels 13–14 — mono synths**

- 2 oscillators each  
- ADSR, LFO (rate/depth), arpeggiator (step/span)  
- Waveforms: sine, saw, square, triangle  

### Effects (all channels)

- **ADSR** — attack / decay / sustain / release (0–32 range)  
- **Bitcrusher** — bit depth 1–16 (0 = bypass), sample-rate reduction down to glorious crunch  
- **Filter** — LP / HP / BP, 0–10 kHz, resonance 0.7–5.0, smooth transitions  
- **Reverb** — room size, damping, wet/dry  
- Fast filter access on encoder #3 during runtime (assignable default per channel)  

### MIDI, clock & extras

- TRS MIDI in/out, clock send/receive, notes, transport  
- BPM **40–300**, internal clock, external MIDI clock, tap tempo via touch  
- Live recording / looping (touch-hold), up to ~12 s per voice  
- Note probability, velocity, condition triggers (those “only every other bar” tricks)  
- Global transpose, per-channel detune & octave, fine cents on the poly synth  
- Mute / solo, pattern modes (OFF, ON, SONG, NEXT)  
- Stereo routing options, optional external WS2812 strip for ripple eye-candy synced to the music  

---

## Tech specs

| | |
|---|---|
| **Brain** | Teensy 4.1 + Audio Board |
| **Memory** | 16 MB PSRAM; Micro SD slot (up to 32 GB class of card you already own) |
| **Audio** | 44.1 kHz, 16-bit mono WAV |
| **Controls** | 4× RGB illuminated I2C rotary-push encoders, 3 touch switches |
| **Display** | 16×16 RGB LED matrix (FastLED); up to 2 modules → 32×16 |
| **LED strip** | External 5 V WS2812 connector (configurable length) |
| **I/O** | 6.35 mm headphones, line in/out, mic in; built-in mic; TRS MIDI in/out |
| **Power** | USB-C or optional LiPo + charging |

---

## SD card over USB (Menu → ETC → SD)

TŒRN does **not** mount the Micro SD as USB mass storage. That would be too easy, and also fight the audio engine. Instead:

While **Menu → ETC → SD** is open (screen: WAIT → OK), you can browse and transfer files over **USB Serial**.

### Primary — browser tool

1. On the device: open **Menu → ETC → SD**  
2. On a computer: open **[https://sdtool.tyng.app](https://sdtool.tyng.app)** in desktop **Chrome or Edge**  
3. Click **Connect**, pick the Teensy serial port  
4. Stay on ETC → SD for the whole session  

WAV uploads are converted in the browser to **44.1 kHz mono 16-bit**. Drag folders, multi-file drops, and zip archives are supported.

### Secondary — local CLI / scripting

Any USB Type that includes Serial (e.g. **Serial + MIDI**):

```bash
cd standalone-tools/sd-tool-standalone
pip install pyserial
python3 toern_sd.py -p /dev/cu.usbmodemXXXX list /
# also: put / rm / mkdir / get
python3 toern_sd.py web       # optional local UI at http://127.0.0.1:8787
```

Insert or eject the SD card only with the unit **off** — the firmware re-indexes on boot.

---

## Repo map (where stuff lives)

| Path | What’s there |
|------|----------------|
| `toern.ino` + `toern_*.ino` | Firmware (the fun / terrifying part) |
| `src/` | Modified audio bits (resampler, freeverb DMA) |
| `PCB/` | Hardware revisions (currently rev G territory) |
| `handbook/` | Human-friendly operator’s guide (how to *use* the device) |
| `devdocs/` | [Docusaurus](./devdocs/) code **and** hardware (rev G) docs |
| `tools/` / `standalone-tools/` | Helpers; SD file tool lives in `standalone-tools/sd-tool-standalone` |
| `website/` | Project site bits |

---

## External libraries & modifications

This project leans on several libraries. Some are stock; some got lovingly (and necessary-ly) poked.

### Modified

- **ResamplingReader.h** (`teensy-variable-playback`)  
  Variable rates, interpolation, multi-channel, loop types (repeat / ping-pong), crossfading — in `src/resamplerReader.h`

- **Freeverb** (`effect_freeverb_dmabuf`)  
  DMAMEM-friendly reverb for Teensy 4.1 — `src/effect_freeverb_dmabuf.*`

- **FastLED**  
  `FASTLED_ALLOW_INTERRUPTS 0` so LEDs and audio don’t argue mid-beat; custom `FastLEDshow()` / `FastLEDclear()` wrappers

- **MIDI**  
  Bigger Serial8 buffers for dense clock traffic (`RX 2048`, `TX 128`) and custom TRS MIDI settings

### Standard / upstream

- **TeensyPolyphony** by Nic Newdigate — polyphonic heart of the thing  
- **Audio Library** (PJRC)  
- **WS2812Serial**, **i2cEncoderLibV2**, **FastTouch**

---

## Get one / get involved

- **Build or fork it** — everything lives in this repo. Makers, developers, and musicians are all invited to tweak and share.  
- **Prefer a finished unit?** Don’t want to become a part-time PCB archaeologist? Write to **jpkuntoff@gmail.com** for a pre-assembled TŒRN or a custom collaboration.  
- **Handbook** — start at [`handbook/index.html`](./handbook/index.html).  
- **Code docs** — architecture & hardware (rev G) in [`devdocs/`](./devdocs/); published at **`/docs/`** on the site (`cd website && npm run build:docs`). Local preview: `cd devdocs && npm start` → `/docs/`.  
- **Issues & ideas** — open an issue or leave a note; open source gets better when people actually poke it.

---

## License

- **Software / code**: [MIT](./LICENSE) — free for personal and commercial use.  
- **Hardware design files** (schematics, PCB layouts, Gerbers, etc.): [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/) — personal / non-commercial use and modification. **Commercial hardware use needs written consent.**

Commercial hardware licensing: Jan aka **warft_ctrl** — jpkuntoff@gmail.com (Hamburg, Germany)

---

## Thank you

Huge thanks to **Paul Stoffregen** and the PJRC crew for Teensy, to the wider open-source audio community, and an especially loud shoutout to **Nic Newdigate** for teensy-polyphony — basically the soul of this project.

Made with too many late nights and exactly the right number of RGB LEDs.

**Jan aka warft_ctrl** (formerly soundpauli)  
Hamburg, 2026
