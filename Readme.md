# TŒRN `[tɜːn]`

**Open-source sequencer and sampler** — by [warft_ctrl](mailto:info@toern.live)

> Say it like *turn*. As in: turn the knobs. Turn a doodle into a beat. Turn “I can’t play piano” into “wait, that slap though.”

TŒRN is a sequenced musical device for beginners, makers, and anyone who likes blinking lights more than plugin menus. Think **Etch-A-Sketch™ energy**, but instead of drawing a wobbly staircase you draw drums, synths, and happy accidents — on a vivid **32×16 RGB LED panel** (standard; 16×16 optional) that is both the screen *and* the keyboard.

It sits on a Teensy 4.1, runs from a Micro SD card full of your WAVs, and is happy on a desk, in a backpack, or on battery at the park where strangers will ask “what *is* that?”

**Status:** under very active development. Things move fast. Stay tuned, and don’t be surprised if yesterday’s weird bug is today’s charming feature (or the other way around).

---

## What it is (in plain language)

You get:

- **8 sample voices** for drums, one-shots, loops, and “that weird recording from the metro”
- **1 three-voice polyphonic synth** with presets (bass, keys, chiptune, pads… the usual suspects)
- **2 monophonic synths** with LFO and arpeggiator for leads that refuse to sit still
- A **32-step × 8-page** sequencer (256 steps of room to overthink; 16×16 mode uses 16 steps × 16 pages), page chaining into songs, MIDI in/out, filters, bitcrush, reverb, and enough real-time knobs to keep your hands busy and your laptop jealous

No laptop required once it’s built. Samples live on the SD card. Patterns autosave. You twist, tap, and suddenly it’s 2 a.m.

For the friendly guided tour (first beat in about a minute), open the **[handbook](docs/handbook/index.html)** in this repo.

---

## A tiny history

TŒRN builds on the legacy of the **NI404**, with more features, fewer cables dangling off the side, and a custom PCB where everything mounts cleanly. The older “extra boards and hope” era is over.

Meet the glow-up:

- **RGB-I2C rotary encoders** — smoother, prettier, and they light up like they mean it
- **Custom PCB** — codec, charging, and jacks on one board
- **USB-C** — because micro-USB was a character-building experience we no longer need
- **LiPo charging port** — optional battery for true “make beats on a train” energy (battery not included; we trust you with electricity)
- **Proper audio I/O** — 6.35 mm headphone out, line in/out, mic in, *plus* a built-in mic for “sample the room right now” moments
- **TRS MIDI in/out** — play nice with the rest of your setup
- **On/off switch** — underrated luxury
- **Touch buttons** — for the things knobs shouldn’t monopolize
- **Laser-cut acrylic case** — files included; looks finished, not “breadboard cosplay”
- **JST connectors** — expansion leads plug in

Open source as always: schematics, code, and design files are here for you to poke, fork, and improve.

---

## Feature list

### Sequencer

- **11 instrument voices** on a 16-row grid: 8 sample voices (1–8), one 3-voice poly synth (11), two mono synths (13–14)
- Standard **32 steps per page × 8 pages** → 256 steps per pattern; optional 16×16 mode uses 16 steps × 16 pages
- **Voice mode**: each voice keeps its own pattern length. It loops the pages that hold its notes, pitches included, while the other voices keep looping on their own
- **FIRE** (ETC → ADV menu): optional playhead sparks when a chosen voice’s notes light white
- Store up to **999 patterns**, **999 samples**, **999 samplepacks**
- **Autosave / autoload** so your 3 a.m. genius doesn’t vanish at boot
- **Switch patterns live** without stopping playback
- **Song mode**: chain up to **64** page assignments from the current pattern
- Pattern ops: copy/paste, transpose page/channel (±1 octave), smart random notes (major/minor + rhythm ideas), clear page/channel
- **SWAP** exchanges notes between voices, with octave offsets; optional **SUBP** file subpatterns extend arrangements

### Sample voices (channels 1–8)

- Up to **~12 seconds** per voice at 44.1 kHz
- Browse and load samples **while playing** (yes, really)
- Per-voice start/end trim, forward/reverse
- **TIME** stretches or shortens samples independently of pitch
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
- ADSR, LFO on cutoff/resonance, MIDI arpeggiator (step/span; does not pitch-shift grid notes)
- Waveforms: sine, saw, square, triangle

### Effects (all channels)

- **ADSR** — attack / decay / sustain / release (0–32 range)
- **Bitcrusher** — bit depth 1–16 (0 = bypass), sample-rate reduction down to glorious crunch
- **Filter** — LP / HP / BP, 0–10 kHz, resonance 0.7–5.0, smooth transitions
- **Reverb** — room size, damping, wet/dry; denser six-comb/four-diffuser network on reverb-equipped voices
- Fast filter access on encoder #3 during runtime (assignable default per channel)

### MIDI, clock & extras

- TRS MIDI in/out, clock send/receive, notes, transport
- BPM **40–300**, internal clock, external MIDI clock, tap tempo via touch
- Live recording / looping (touch-hold), up to ~12 s per voice
- Note probability, velocity, condition triggers (those “only every other bar” tricks)
- Global transpose, per-channel detune & octave, fine cents on the poly synth
- Mute / solo, pattern modes (OFF, ON, SONG, NEXT)
- Stereo routing options
- USB **Audio + Serial**: send the mix to a computer while using the SD tool; musical MIDI uses the TRS ports
- Analog clock **out (J25)**, clock **in (J26)** and voice-triggered **sidechain out (J27)**
- **TOUCHY** performance pads, live fills, and per-voice audition
- Browser tools for firmware flashing, SD transfers, sample conversion and the simulator
- **Demo animation:** open **ETC → INFO**, press **touch3 / BUTTON_C**; press and release **Touch 1 / BUTTON_A** to return to INFO

---

## Tech specs

| | |
|---|---|
| **Brain** | Teensy 4.1, PSRAM fitted. Audio codec is the SGTL5000 on the carrier |
| **Memory** | 16 MB PSRAM target; Micro SD (handbook suggests up to 64 GB, SanDisk Ultra) |
| **Audio** | 44.1 kHz, 16-bit mono WAV |
| **Controls** | 4× RGB illuminated I2C rotary-push encoders, 5 capacitive touch buttons |
| **Display** | Standard [32×16 RGB LED panel](https://de.aliexpress.com/item/1005010071864259.html), 512 LEDs (FastLED); 16×16 optional |
| **I/O** | 6.35 mm headphones, line in/out, mic in; built-in mic; TRS MIDI in/out |
| **Power** | USB-C or optional LiPo + charging |

---

## Repo map (where stuff lives)

| Path | Contents |
|------|----------|
| `platformio/` | Arduino sketch tabs, headers, custom audio in `src/`, build scripts and `platformio.ini` |
| `hardware/PCB/` | KiCad boards, schematics, manufacturing files and [purchasing BOMs](hardware/PCB/BOM/README.md) |
| `hardware/housing/` | Fusion archives, laser DXFs and panel previews |
| `docs/devdocs/` | Developer documentation → [docs.toern.live](https://docs.toern.live/) |
| `docs/handbook/` | Operator handbook → [handbook.toern.live](https://handbook.toern.live/) |
| `docs/BUILD.md`, `docs/info.md` | Hardware assembly and firmware build instructions |
| `tools/firmware/` | Browser firmware flasher (GPL-3.0) |
| `tools/sdtool/` | Browser SD tool |
| `tools/sim/` | Browser simulator |
| `tools/audioconvert/` | Standalone sample converter |
| `tools/case/` | Standalone case generator; `case.toern.live` planned, not deployed |
| `tools/colorsheme/`, `tools/_shared/` | Color editor and shared tool assets |
| `_debug/` | Host tests, analysis scripts and local diagnostic captures |
| `_private/` | Local business data/helpers; annotation tool in `_private/helpers/annotate/` |
| `website/` | Public project website only; local/deploy-only |

See [the complete directory map](docs/REPOSITORY_LAYOUT.md) for local-only items marked `***`.

Build from the repository root with `pio run -d platformio`, or `cd platformio` first.

For a personal hardware build, follow the [assembly and laser-cutting guide](docs/BUILD.md).
Start with the [BOM directory](hardware/PCB/BOM/README.md): check assembly-part stock
before ordering the PCB. The [pre-order list](hardware/PCB/BOM/preorder-parts.csv)
lists parts to secure for JLCPCB assembly. Then buy the
Teensy, PSRAM, encoders, matrix, cables and
case hardware separately. The website’s **Build** chapter covers the same assembly order.

---

## External libraries & modifications

This project leans on several libraries. Some are stock; some got lovingly (and necessary-ly) poked.

### Modified

- **ResamplingReader.h** (`teensy-variable-playback`)
  Variable rates, interpolation, multi-channel, loop types (repeat / ping-pong), crossfading — in `platformio/src/resamplerReader.h`

- **Treeverb** (`effect_treeverb`)
  Full-rate reverb with balanced 6/4 diffusion in RAM1 — `platformio/src/effect_treeverb.*`; the older `effect_freeverb_dmabuf` remains a host-test reference.

- **FastLED**
  `FASTLED_ALLOW_INTERRUPTS 0` so LEDs and audio don’t argue mid-beat; custom `FastLEDshow()` / `FastLEDclear()` wrappers

- **MIDI**
  Bigger Serial8 buffers for dense clock traffic (`RX 2048`, `TX 128`) and custom TRS MIDI settings

### Standard / upstream

- **TeensyPolyphony** by Nic Newdigate — polyphonic heart of the thing
- **Audio Library** (PJRC)
- **WS2812Serial**, **i2cEncoderLibV2**, **FastTouch**

---

## Get one

Order a TŒRN at **[toern.live](https://toern.live/)**. Questions go to **info@toern.live**.

When it arrives, open the **[handbook](docs/handbook/index.html)** and start at First beat. Power is USB-C. A battery and a microSD card are not in the parcel; the buy page and the handbook hardware chapter name what to get.

Firmware changes: [`info.md`](docs/info.md) to flash, [`docs/devdocs/`](docs/devdocs) for how the code is shaped, [`firmware-edits`](docs/devdocs/docs/contributing/firmware-edits.md) for a new menu page. Fabricating a board from the design files is [`BUILD.md`](docs/BUILD.md) — that is not how a TŒRN is ordered.

---

## License

- **Software / code**: [MIT](LICENSE) — free for personal and commercial use.
- **Firmware loader** (`tools/firmware/`): [GPL-3.0](tools/firmware/LICENSE), because that tool is Teensy Loader Javascript. GPL-3.0 applies only to that folder.
- **Hardware design files** (schematics, PCB layouts, Gerbers, housings, etc.): [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/) — personal / non-commercial use and modification. **Jan-Peter Kuntoff is the only person authorized to sell TŒRN hardware** (bare PCB, assembled PCB, kit, or unit), including boards sold **without a Teensy**. Teensy 4.1 is PJRC’s; it is not licensed in this repo. Anyone else needs written consent to sell.

Commercial hardware licensing: Jan aka **warft_ctrl** — info@toern.live (Hamburg, Germany)

---

## Thank you

Huge thanks to **Paul Stoffregen** and the PJRC crew for Teensy, to the wider open-source audio community, and an especially loud shoutout to **Nic Newdigate** for teensy-polyphony — basically the soul of this project.

Made with too many late nights and exactly the right number of RGB LEDs.

**Jan aka warft_ctrl** (formerly soundpauli)
Hamburg, 2026
