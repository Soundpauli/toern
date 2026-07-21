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
- Optional pulse-clock output helpers (`pulseClock*`, EEPROM load/store)

## Clock paths

| Direction | Entry points |
|-----------|----------------|
| Soft MIDI clock out | `updateMidiClockOutput`, `midiClockTick` |
| External clock in | `myClock`, averaging / BPM stability helpers |
| Sequencer beat | `playTimer` / playback aligned with transport state |

`checkMidi()` is called early in `loop()` for lower latency.

## Notes ↔ engine

- Incoming notes → `handleNoteOn` / `handleNoteOff` → sample or synth triggers (respecting mutes / child lock)  
- Outgoing notes → `MidiSendNoteOn` and related send helpers when MIDI out is enabled  

## Editing safely

Clock math and ISR interplay are easy to break. Prefer small, well-tested changes; there are local planning notes (`_internal-docs/MIDI_CLOCK_RELIABILITY_PLAN.md`) describing past reliability work if you have that folder checked out.
