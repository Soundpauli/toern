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

While playing, hold **T4+T5** to request a fill for the current voice. It starts on the next page downbeat, builds for two pages, and ends with one velocity-127 hit on the following downbeat. The centered FILL confirmation lasts one second in the request voice's color. Release both pads before another request. Different voices queue together and keep independent timing; re-arming one does not restart others. Pause cancels pending fills and drops. Grid F/F uses the same cadence and final downbeat; its trigger condition returns to normal on completion. The physical external panel instead uses **C+D** with D mapped to FN; the simulator models five capacitive pads.

## Filters and presets

Sample filter pages contain HCUT/LCUT/RVRB/BITC, RES/DTNE/OCTV/TIME, ADSR, and DLAY/VOLUME. **V** is sample-only: −16 = silence, 0 = unity, +16 = 2×. **TIME** defaults to 11/OFF and uses a granular overlap-add browser approximation of the hardware's WSOLA. DLAY follows N/32 of a beat with feedback. Pause releases sources but keeps delay/reverb nodes alive for natural tails. The shared browser reverb and oscillator recipes remain approximations of the per-voice Teensy graph.

All 13 ch11 instrument selections are present: BASS, KEYS, CHPT, PAD, WOW, ORG, FLT, LEAD, ARP, BRSS, 303, DXEP, DXBS. The ch11 default is DXBS. `instruments.js` mirrors the firmware's filter/register/ADSR tables; browser oscillator timbres do not reproduce the hardware's three-oscillator/FM-style recipes exactly.

SMP becomes **PSET** on voices 11/13/14. Encoder 4 selects 1–99, encoder 1 loads, encoder 2 saves, encoder 4 short exits. Slots are independent per voice, stored under `toern-web-presets-<voice>` in browser storage. They do not write `.pset` files on the instrument.

The left sidebar holds the current mode/status and help, panel layout, assistive controls and Mobile / iPad link. The right sidebar starts with New empty pattern and Load demo pattern. Desktop sidebars grow with their content, allowing the whole page to scroll to all controls. The introduction appears at startup unless “Don’t show again” is selected.

## Menu and clock parity

Root menus: PATT, KIT, SMP/PSET, BPM, VOL, SETT, REC, MIDI, MODE, ETC. REC includes HPF. MIDI order is CH, TRAN, SEND, RCVE, FOLD, SYNC, PQ-I, PQ-O, TRIG, OVWR. Display-to-storage mappings in `screens.js` preserve legacy preference slots when menu order changes.

SETT → SWAP exchanges two channels’ notes across the entire 256-step pattern in SIM and firmware. Encoders 2 and 3 select channels (1–8, 11, 13, 14); press encoder 4 / Enter to swap immediately (encoder 1 exits the menu). Hold either chooser while rotating to set the octave shift for notes leaving that channel (−8…+8, initially 0); its signed octave value appears only while held. Shifts apply on OK and clamp pitches to MIDI 0–127. Native grid notes acquire an absolute MIDI pitch only for a nonzero shift. Note positions, velocity, probability and conditions are preserved; zero shift preserves stored MIDI pitch; sound settings stay with their channels. SWAP reuses the copy/paste buffer and clears an armed copy. Selecting the same channel twice does nothing.

SYNC uses **`SYC>` / `SYC<`**, signed −127…+127 ms. Positive delays the selected start path; negative contributes to the opposite path. `performance.js` shares this math with the renderer. A red `--` marks an unavailable path, including analog EXT; values remain stored/editable. Offsets apply at start, not to MIDI notes or an already running sequence. Browser Web MIDI input and synthetic analog pulses approximate hardware timing; PQ-I/PQ-O and TRIG indicators do not drive physical GPIO, and this simulator does not send a hardware-equivalent MIDI Clock stream.

CHNR covers sample and synth voices, with synth text/border positioned at the current firmware coordinates. PATT occupancy uses color without USED/NEW text. MODE includes TOUCHY, IMG, VMOD, SONG and DEMO. DEMO runs NyanCat for 10 seconds, scrolls MATZE SAMPLER at 2× width and height completely across the grid in red, bright white and rainbow, with the boot animation’s blue and white sine curves behind the text. The rainbow pass loops directly back to NyanCat; DEMO does not show the TŒRN logo. The entire sequence loops until Touch 1 / E or button A (A key) is pressed, returning to MODE → DEMO. Encoder 4 starts it. The sidebar Menu shortcut opens the root menu at SETT; DEMO remains available under MODE. Firmware MODE → DEMO now uses the same sequence, with the shared TOUCHY cat and boot sine-curve renderers. Touch 1 maps to button A on the external panel. BPM displays one decimal place on one row, with encoder 1 editing tenths and encoder 4 whole BPM.

Shadowed monophonic notes render at 20% brightness in DRAW and SINGLE, without playhead flashing. A guaranteed higher note at the same step must cover the lower note's condition; fills and CH11 chords remain protected. These notes can be replaced by painting, which resets their metadata. New notes, default auditions and initial TOUCHY velocities use 127; stored and incoming MIDI velocities remain unchanged. Copy feedback reads `c+p`. Resetting all filter pages displays a blue checkmark for 800 ms.

## Firmware pitch parity

Firmware is authoritative for tuning. CH13/14 CENT shifts both oscillators ±24 semitones; OCTV uses raw 0–32 (center 16) and shifts only oscillator 2 by an integer −3…+3 octaves, with C++ truncation. DTNE applies ±1 semitone only to oscillator 2; SEMI does not affect these mono voices. The browser now renders both oscillators. CH11 uses the firmware note table, integer register, per-preset oscillator-1 cents, and C++ round-away-from-zero for SEMI. Voice defaults and reset defaults follow `toern_parameters.ino`, with one explicit SIM default exception: CH13/14 CENT=16 (neutral), rather than the firmware’s stored defaults of 0/8; mono OCTV=16 is neutral in both. Existing saved/custom patches retain their settings; reset them explicitly if desired.

`python3 tests/test_sim_pitch_parity.py` compiles the real firmware mono calculation and compares 8,965 tuning cases to the SIM, including all CH11 presets/SEMI/registers. Browser DSP and synth recipes remain approximations; arbitrary custom device settings cannot be inferred from a notes-and-tempo USB transfer. Matching physical playback requires the same kit and voice settings.

## Synth arpeggiator

ARP is centered at 16 (OFF). Positive values use sixteenths, negative values eighths; magnitude sets 1–12 semitones, and SPAN sets a 2–16-step ping-pong range. As in the firmware MIDI arp, only held MIDI notes drive it: ordinary sequencer, grid and pad triggers do not advance a hidden pitch counter. A new held phrase starts on its lowest note, chords cycle low-to-high with octave extension, the playhead clocks it while playing, and BPM clocks it while stopped. Note Off, velocity-zero Note On, pause and MIDI all-notes-off clear the appropriate held state. In YPOS, Note Off follows the voice that took the matching Note On even if the selected channel changed. CH11 also supports the arp. Held non-arp notes sustain until Note Off; CH13/14 grid playback does not steal a held MIDI voice.

`node tests/test_sim_synth_arp.mjs` tests the actual oscillator frequency calls and arp timing/state; the menu test also exercises Web MIDI routing. LFO remains a browser approximation (filter-frequency modulation); it is not a pitch randomizer.

## MIDI import assignments

Transpose out-of-range notes defaults off, including after Reset to MIDI. Original MIDI pitches remain stored while notes are drawn inside the grid. GM drum-channel notes split into five groups using the firmware's `companionRoleForChannel` assignments: 1 BD, 2 snare, 3 hat/cymbals, 4 clap, 5 tom/other percussion. Drum hits use their voice's base pitch rather than treating the GM drum key as a melody pitch.

Programs and track names guide melodic parts to CH11 instruments/lead, CH13 keys, CH14 bass, and CH8 vocals/pad. These dedicated voices are filled first; extra compatible parts use sample fallbacks CH6 bass / CH7 keys, with CH11 available for extra keys/pads if no lead claims it. Fallback priority follows descending note count. A single keys-only MIDI part goes to CH11 before CH13. Other parts stay visibly unassigned for manual selection. Channels 9, 10 and 12 are not playable SIM voices. This is a role approximation, not a General MIDI soundfont. MIDI import keeps the current kit and synth parameters; it does not apply SIM-only preset changes or force neutral tuning. The Reset sound controls button resets SIM voice controls using the CH11 preset tables and mono neutral CENT defaults without replacing kit samples. USB current-pattern transfer sends notes and tempo, so the real device needs a matching kit/settings.

Overlap filtering always keeps one pitch per assigned voice/step for sample voices 1–8 and mono voice CH14. The checkbox controls only CH11 and defaults OFF on initial load and reset; CH13 note data stays intact regardless of its value. Reassigning a track reapplies this policy using its destination voice. Cross-track collisions are resolved after voice assignment; with transpose off, melody notes can move to another free display row while retaining their absolute pitch. The start-page slider selects a 256-step window from the entire song and updates the SIM immediately. Page units follow the panel layout: 16 pages at 16 steps or 8 pages at 32 steps. The displayed song range is rebased to SIM page 1; changing tempo, subdivision, offset or panel layout recomputes and clamps it. The MIDI sidebar SEND TO DEVICE button sends the currently selected window. VMOD starts OFF on every SIM startup, including when the saved session had it ON; it can still be enabled in MODE. The pattern remains limited to 256 steps and 15 simultaneous grid cells. MIDI parsing/mapping regression: `node tests/test_sim_midi_import.mjs`.

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
