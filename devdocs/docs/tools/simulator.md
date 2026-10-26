---
sidebar_position: 3
title: Browser simulator
description: Five-pad controls, fills, faders, effects, presets and browser boundaries.
---

# Browser simulator

**Hosted build:** [sim.tyng.app](https://sim.tyng.app/). **Source:** [`standalone-tools/web-standalone/`](https://github.com/Soundpauli/toern/tree/main/standalone-tools/web-standalone).

This is a JavaScript model of the instrument, with Web Audio synthesis and scheduling. It does not execute the Teensy firmware. Both **16×16** and **32×16** panel layouts use the current control vocabulary; the selector changes matrix geometry, not firmware version. The INFO version follows `VERSION` in `toern.ino` (currently v3.02). Local changes reach the hosted site only after a separate deployment.

## Five touch pads

All five pads remain visible on desktop and mobile. Pointer capture supports holding more than one pad on a touch screen. **E, P, period (.), X and M** provide hold/release keyboard controls without conflicting with voice-number shortcuts.

| Pad | Normal operation | TOUCHY |
| --- | --- | --- |
| T1 / E | SINGLE / exit; ordered T1/T2 gestures | Voice 1 |
| T2 / P | Menu; filter-page selection | Voice 2 |
| T3 / . | Recording, tap tempo or page reset according to mode | Voice 3 |
| T4 / X | Audition selected voice; SINGLE+playing records a base-pitch hit | Voice 4 |
| T5 / M | Hold FN; encoder 1 sets backward/forward sample direction | Voice 5 |

While playing, hold **T4+T5** to request a fill for the current voice. It starts on the next page downbeat, builds for two pages, and ends with one velocity-127 hit on the following downbeat. The centered FILL confirmation lasts one second in the request voice's color. Release both pads before another request. Different voices queue together and keep independent timing; re-arming one does not restart others. Pause cancels pending fills and drops. Grid F/F uses the same cadence and final downbeat; its trigger condition returns to normal on completion. The physical external panel instead uses **C+D** and has no fifth/FN button; the simulator models five capacitive pads.

## Filters and presets

Sample filter pages contain HCUT/LCUT/RVRB/BITC, RES/DTNE/OCTV/TIME, ADSR, and DLAY/VOLUME. **V** is sample-only: −16 = silence, 0 = unity, +16 = 2×. **TIME** defaults to 11/OFF and uses a granular overlap-add browser approximation of the hardware's WSOLA. DLAY follows N/32 of a beat with feedback. Pause releases sources but keeps delay/reverb nodes alive for natural tails. The shared browser reverb and oscillator recipes remain approximations of the per-voice Teensy graph.

All 13 ch11 instrument selections are present: BASS, KEYS, CHPT, PAD, WOW, ORG, FLT, LEAD, ARP, BRSS, 303, DXEP, DXBS. The ch11 default is DXBS. `instruments.js` mirrors the firmware's filter/register/ADSR tables; browser oscillator timbres do not reproduce the hardware's three-oscillator/FM-style recipes exactly.

SMP becomes **PSET** on voices 11/13/14. Encoder 4 selects 1–99, encoder 1 loads, encoder 2 saves, encoder 4 short exits. Slots are independent per voice, stored under `toern-web-presets-<voice>` in browser storage. They do not write `.pset` files on the instrument.

The left sidebar holds the current mode/status and help, panel layout, assistive controls and Mobile / iPad link. The right sidebar starts with New empty pattern and Load demo pattern. The introduction appears at startup unless “Don’t show again” is selected.

## Menu and clock parity

Root menus: PATT, KIT, SMP/PSET, BPM, VOL, SETT, REC, MIDI, MODE, ETC. REC includes HPF. MIDI order is CH, TRAN, SEND, RCVE, FOLD, SYNC, PQ-I, PQ-O, TRIG, OVWR. Display-to-storage mappings in `screens.js` preserve legacy preference slots when menu order changes.

SYNC uses **`SYC>` / `SYC<`**, signed −127…+127 ms. Positive delays the selected start path; negative contributes to the opposite path. `performance.js` shares this math with the renderer. A red `--` marks an unavailable path, including analog EXT; values remain stored/editable. Offsets apply at start, not to MIDI notes or an already running sequence. Browser Web MIDI input and synthetic analog pulses approximate hardware timing; PQ-I/PQ-O and TRIG indicators do not drive physical GPIO, and this simulator does not send a hardware-equivalent MIDI Clock stream.

CHNR covers sample and synth voices, with synth text/border positioned at the current firmware coordinates. PATT occupancy uses color without USED/NEW text. MODE includes TOUCHY, IMG, VMOD, SONG and DEMO. DEMO runs NyanCat for 10 seconds, scrolls MATZE SAMPLER at 2× width and height completely across the grid in red, bright white and rainbow, with the boot animation’s blue and white sine curves behind the text. The rainbow pass loops directly back to NyanCat; DEMO does not show the TŒRN logo. The entire sequence loops until Touch 1 / E or button A (A key) is pressed, returning to MODE → DEMO. Encoder 4 starts it. The sidebar Menu shortcut opens the root menu at SETT; DEMO remains available under MODE. Firmware MODE → DEMO now uses the same sequence, with the shared TOUCHY cat and boot sine-curve renderers. Touch 1 maps to button A on the external panel. BPM displays one decimal place on one row, with encoder 1 editing tenths and encoder 4 whole BPM.

## Storage and transfer

Browser pattern snapshot version 2 stores notes (including preserved MIDI pitch), filters, channel volumes and BPM. Legacy note-array snapshots still load. Sample buffers/recordings, browser PSET slots and fader positions are separate; snapshots are not binary Teensy `Device` structures. Web Serial pattern transfer still covers grid notes and tempo, not complete sound kits, effects or synth presets. The red SEND TO DEVICE and green GET FROM DEVICE buttons beside the simulator transfer the current pattern in RAM. Transfer patterns contains only the Pattern file browser; its upload control replaces a matching filename on the real device in the selected folder.

## Development and verification

```bash
cd standalone-tools/web-standalone
npm ci
npm run dev
npm run build
cd ../..
node tests/test_sim_performance.mjs
node tests/test_sim_audio.mjs
node tests/test_menu_navigation.mjs
```

`src/firmware/device.js` owns state/input, `screens.js` renders the matrix, `performance.js` owns fill/SYNC rules, `instruments.js` mirrors ch11 control defaults, `pitch.js` mirrors grid-fold roots, and `src/audio/engine.js` owns browser DSP. `main.js` binds the five pads, pointer/keyboard controls and simulated faders. Prefer deterministic transport tests to wall-clock sleeps; always check both matrix widths and mobile layout. Deploy `dist/` only after validation and an explicit deployment request.
