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
- Analog pulse I/O: CLK-OUT pin 37 (`pulseClock*`), CLK-IN pin 38, SIDEC pin 28 (`sidechainOnVoice`, MIDI → TRIG). The CLK-IN GPIO interrupt is attached only in BPM orange EXT.

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

## USB and input budget

The USB build exposes MIDI, Audio and Serial interfaces, but musical note/clock/transport parsing and sending use **Serial8/TRS**. No musical `usbMIDI` read/send path is implemented. USB audio output is separately connected to the two output mixers; see [Audio graph](../audio/graph).

`checkMidi()` stops reading after four non-callback messages per loop. Clock and Note On run through callbacks and do not consume that count. Unread messages remain queued for the next call instead of being discarded. This bounds ordinary message work; it does not guarantee that the hardware UART buffer cannot overflow under excessive input.

MIDI → **TRIG** selects the voice that pulses J27. It is a digital trigger output, not onboard sidechain compression. MIDI → **FOLD** folds incoming pitch into the visible grid; **OVWR** chooses whether incoming MIDI may overwrite occupied cells; voice-select **SEL** follows the selected voice.
