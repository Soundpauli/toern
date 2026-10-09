---
sidebar_position: 2
title: MIDI & clock
description: MIDI I/O, master/slave clock, and transport handling.
---

# MIDI & clock

[`toern_midi.ino`](https://github.com/Soundpauli/toern/blob/main/toern_midi.ino) owns MIDI parsing, clock generation/following, and note/transport bridging into the sequencer/synths.

## Hardware / library setup

- TRS MIDI via Serial (custom `MidiSettings`, enlarged Serial8 buffers in `toern.ino`)  
- `MIDI.h` handlers for note on/off, clock, start/stop  
- Analog pulse I/O: CLK-OUT pin 37 (`pulseClock*`), CLK-IN pin 38, SIDEC pin 28 (`sidechainOnVoice`, MIDI → SIDE). The CLK-IN GPIO interrupt is attached only in BPM orange EXT.

## Clock paths

| Direction | Entry points |
|-----------|----------------|
| Soft MIDI clock out | `updateMidiClockOutput`, `midiClockTick` |
| External clock in | `myClock` (MIDI F8) and `analogClockIsr` (J26, orange EXT only) |
| Sequencer beat | `playTimer` / playback aligned with transport state |

`checkMidi()` is called early in `loop()` for lower latency.

## Notes ↔ engine

- Incoming notes → `handleNoteOn` / `handleNoteOff` → sample or synth triggers (respecting mutes / child lock)  
- Outgoing notes → `MidiSendNoteOn` and related send helpers when MIDI out is enabled  

## Editing safely

Clock math and ISR interplay are easy to break. Prefer small, well-tested changes; there are local planning notes (`_internal-docs/MIDI_CLOCK_RELIABILITY_PLAN.md`) describing past reliability work if you have that folder checked out.
