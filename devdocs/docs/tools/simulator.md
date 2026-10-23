---
sidebar_position: 3
title: Browser simulator
description: Five-pad controls, fills, faders, effects, presets and browser boundaries.
---

# Browser simulator

**Hosted build:** [sim.tyng.app](https://sim.tyng.app/). **Source:** [`standalone-tools/web-standalone/`](https://github.com/Soundpauli/toern/tree/main/standalone-tools/web-standalone).

This is a JavaScript model of the instrument, with Web Audio synthesis and scheduling. It does not execute the Teensy firmware. Both **16×16** and **32×16** panel layouts use the current control vocabulary; the selector changes matrix geometry, not firmware version. The INFO version follows `VERSION` in `toern.ino` (currently v3.02). Local changes reach the hosted site only after a separate deployment.

## Five touch pads

All five pads remain visible on desktop and mobile. Pointer capture supports holding more than one pad on a touch screen. **F1–F5** provide hold/release keyboard controls without conflicting with voice-number shortcuts.

| Pad | Normal operation | TOUCHY |
| --- | --- | --- |
| T1 / F1 | SINGLE / exit; ordered T1/T2 gestures | Voice 1 |
| T2 / F2 | Menu; filter-page selection | Voice 2 |
| T3 / F3 | Recording, tap tempo or page reset according to mode | Voice 3 |
| T4 / F4 | Audition selected voice; SINGLE+playing records a base-pitch hit | Voice 4 |
| T5 / F5 | Hold FN; encoder 1 sets backward/forward sample direction | Voice 5 |

While playing, hold **T4+T5** to request a fill for the current voice. It starts on the next page downbeat, builds for two pages, and ends with one velocity-127 hit on the following downbeat. The centered FILL confirmation lasts one second in the request voice's color. Release both pads before another request. Different voices queue together and keep independent timing; re-arming one does not restart others. Pause cancels pending fills and drops. Grid F/F uses the same cadence and final downbeat; its trigger condition returns to normal on completion. The physical external panel instead uses **C+D** and has no fifth/FN button; the simulator models five capacitive pads.

## Faders, filters and presets

Open **Faders · voices 1–8** for eight independent controls. They work in every screen, including menus and solo, with concurrent matrix level animations. Bottom mutes; movement above bottom unmutes. Present fader positions override restored solo/full-mute snapshots; untouched simulated faders leave saved mutes intact. Fader amplitude is separate from channel volume and V gain. The browser does not emulate ADC deadzones, I²C conversion waits, electrical noise or seesaw LEDs.

Sample filter pages contain HCUT/LCUT/RVRB/BITC, RES/DTNE/OCTV/TIME, ADSR, and DLAY/VOLUME. **V** is sample-only: −16 = silence, 0 = unity, +16 = 2×. **TIME** defaults to 11/OFF and uses a granular overlap-add browser approximation of the hardware's WSOLA. DLAY follows N/32 of a beat with feedback. Pause releases sources but keeps delay/reverb nodes alive for natural tails. The shared browser reverb and oscillator recipes remain approximations of the per-voice Teensy graph.

All 13 ch11 instrument selections are present: BASS, KEYS, CHPT, PAD, WOW, ORG, FLT, LEAD, ARP, BRSS, 303, DXEP, DXBS. The ch11 default is DXBS. `instruments.js` mirrors the firmware's filter/register/ADSR tables; browser oscillator timbres do not reproduce the hardware's three-oscillator/FM-style recipes exactly.

SMP becomes **PSET** on voices 11/13/14. Encoder 4 selects 1–99, encoder 1 loads, encoder 2 saves, encoder 4 short exits. Slots are independent per voice, stored under `toern-web-presets-<voice>` in browser storage. They do not write `.pset` files on the instrument.

## Menu and clock parity

Root menus: PATT, KIT, SMP/PSET, BPM, VOL, SETT, REC, MIDI, MODE, ETC. REC includes HPF. MIDI order is CH, TRAN, SEND, RCVE, FOLD, SYNC, PQ-I, PQ-O, TRIG, OVWR. Display-to-storage mappings in `screens.js` preserve legacy preference slots when menu order changes.

SYNC uses **`SYC>` / `SYC<`**, signed −127…+127 ms. Positive delays the selected start path; negative contributes to the opposite path. `performance.js` shares this math with the renderer. A red `--` marks an unavailable path, including analog EXT; values remain stored/editable. Offsets apply at start, not to MIDI notes or an already running sequence. Browser Web MIDI input and synthetic analog pulses approximate hardware timing; PQ-I/PQ-O and TRIG indicators do not drive physical GPIO, and this simulator does not send a hardware-equivalent MIDI Clock stream.

CHNR covers sample and synth voices, with synth text/border positioned at the current firmware coordinates. PATT occupancy uses color without USED/NEW text. MODE includes TOUCHY, IMG, VMOD, SONG and the existing performance options.

## Storage and transfer

Browser pattern snapshot version 2 stores notes (including preserved MIDI pitch), filters, channel volumes and BPM. Legacy note-array snapshots still load. Sample buffers/recordings, browser PSET slots and fader positions are separate; snapshots are not binary Teensy `Device` structures. Web Serial pattern transfer still covers grid notes and tempo, not complete sound kits, effects or synth presets. Check the transfer dialog for whether the operation changes device RAM or writes a numbered file.

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
