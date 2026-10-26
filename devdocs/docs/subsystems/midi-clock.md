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

`checkMidi()` is called early in `loop()` for lower latency. `TimestampedSerial`
(`src/toern_midi_serial.h`) wraps the Teensy Serial8 interrupt: the core UART
handler runs first, then received bytes and `micros()` timestamps enter a 511-byte
usable FIFO in RAM2. MIDI callbacks consume those timestamps in the main loop,
so note callbacks, drawing and storage work do not change the measured clock
intervals. UART priority is restored to 32 after `Serial8.begin()`. Timestamps
represent UART ISR service time; bytes drained in one hardware FIFO batch share
a timestamp. Long interrupt masking, UART errors and a full receive FIFO still
require hardware diagnosis. Queue overflow resets tempo acquisition.

MIDI EXT uses `ClockFollower` (`src/toern_midi_clock.h`). It acquires after six
elapsed F8 intervals, then refines a rolling 24-interval estimate every six
clocks. The first timestamp is an endpoint, not an interval. The estimate stays
fractional; interval consistency determines the green acquisition indicator.
The indicator expires after 500 ms without F8. Playback freewheels at its last
period if clock stops; after a gap over two seconds the next clock resets the
estimator. Master clock availability is selected by `clockMode`, independently
of MIDI SEND being CLCK, NOTE or BOTH.

Playback stays on `playTimer`. Updates synchronize `playNoteInterval` and the
fill timer period, with bounded phase correction toward external sixteenth-note
boundaries plus the configured receive offset. Phase correction slews by up to
5% of a step period without restarting the timer or firing extra notes. The
phase correction is separate from the displayed tempo. Incoming MIDI Start
arms playback for the first subsequent F8; Stop cancels armed/delayed starts
and does not debounce external transport.

INT tempo editing uses encoder 4 for whole BPM and encoder 1 for tenths
(0–9). The matrix displays one decimal place on a single row (for example `120.3`), with the clock arrow below. Encoder 2
retains brightness and encoder 3 retains clock/source selection. Pattern files
still store float BPM. Playback and clock output use fractional timer periods,
quantized by the Teensy PIT hardware rather than integer BPM or microseconds.

MIDI → **PQ-I** (J26 input) immediately precedes **PQ-O** (J25 output). Both show a fixed header, white timing value, red OFF, and a signed green/red PPQN rate. On PQ-I, encoder 1 sets hold-off (1–32 ms), encoder 3 selects OFF/rate and clicks toggle input polarity (+ rising / − falling). OFF detaches the input interrupt and cancels pending 1/2-PPQN substeps. PQ-O retains pulse width, STOP/CONT, output OFF/rate and output polarity. Input settings remain separate from output settings.

## Notes ↔ engine

- Incoming notes → `handleNoteOn` / `handleNoteOff` → sample or synth triggers (respecting mutes / child lock)
- Outgoing notes → `MidiSendNoteOn` and related send helpers when MIDI out is enabled

Pitch conversion is shared in `src/toern_midi_pitch.h`. With FOLD off, synth input
uses standard MIDI pitch (69 = 440 Hz before patch tuning), live and recorded
playback use the same conversion, and preserved note numbers are sent unchanged.
Sample voices use MIDI 60 as their original/root hit; sample tuning OCTV/DTNE
still applies. Native grid output inverts each voice's existing pitch mapping:
samples use their voice-relative row, ch11 uses its current grid octave/transpose,
and ch13/14 use their note-table index plus 12. Native patterns keep their local
sound. FOLD wraps by octaves into that voice's playable grid rows, preserving
pitch class; the mono synths avoid their duplicated low-C rows. Arpeggiator
preview and recording follow the same FOLD policy. KEYS routing is unchanged.

## Editing safely

Clock math and ISR interplay are easy to break. Prefer small, well-tested changes; there are local planning notes (`_internal-docs/MIDI_CLOCK_RELIABILITY_PLAN.md`) describing past reliability work if you have that folder checked out.

## USB and input budget

The USB build exposes MIDI, Audio and Serial interfaces, but musical note/clock/transport parsing and sending use **Serial8/TRS**. No musical `usbMIDI` read/send path is implemented. USB audio output is separately connected to the two output mixers; see [Audio graph](../audio/graph).

`checkMidi()` stops reading after sixteen non-callback messages per loop. Clock and Note On run through callbacks and do not consume that count. Unread messages remain queued for the next call instead of being discarded. This bounds ordinary message work; it does not guarantee that the hardware UART buffer cannot overflow under excessive input.

MIDI → **TRIG** selects the voice that pulses J27. J27 emits only for sequencer/fill hits while playing; paint, grid audition, sample previews and live MIDI do not emit pulses. Sampler callers explicitly opt in, and the shared pulse function rejects stopped transport. J27 emits a positive pulse (idle LOW), independently of J25 clock polarity; its width follows MIDI → PQ-O. Clock and sidechain pulse expiry share one PIT timer with independent deadlines, leaving the other three channels for sequencer, fills and MIDI clock. It is a digital trigger output, not onboard sidechain compression. MIDI → **FOLD** folds incoming pitch into the visible grid; **OVWR** chooses whether incoming MIDI may overwrite occupied cells; voice-select **SEL** follows the selected voice.

## SYNC — `SYC>` / `SYC<`

`SYC>` (`transportSendDelayMs`, EEPROM offset 29) and `SYC<` (`transportRcveDelayMs`, offset 31) are signed −127…+127 ms start offsets. Encoder 2 selects the path; encoder 3 edits in 1 ms steps. These are the former SNC1/SNC2 fields; storage and transport math are unchanged.

```text
send delay = max(SYC>, 0) + max(-SYC<, 0)
receive delay = max(SYC<, 0) + max(-SYC>, 0)
```

The master sends MIDI Start and its first Clock immediately, then defers TOERN's own first sequencer step. Incoming MIDI Start waits for the next F8 and applies the receive delay from that clock’s UART timestamp. Local Play in MIDI EXT arms for the next page downbeat before the receive delay. Orange analog EXT starts from pulses and bypasses both offsets. These settings do not delay Stop or MIDI notes and do not re-phase running playback when edited.

`syncDelayHasEffect()` determines whether the selected signed value contributes to an available path. In master mode the send path is active; the receive path also exists if transport GET is enabled. In MIDI EXT the receive path exists even with transport GET off, because local Play can arm on clock. In analog EXT neither path exists. A negative value tests the opposite path. Inactive values render **red `--`** while retaining their stored value and editability.
