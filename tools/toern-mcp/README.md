# TŒRN MCP remote (first milestone)

A local stdio MCP server controls a USB-connected TŒRN through its existing
serial port. It requires firmware built from this revision: older firmware
supports SD exchange but does **not** understand `REMOTE` commands.

Implemented: state readback, a PNG of the last presented matrix (including
transient overlays), encoder turns, encoder button holds/releases, all five
touch pads, timed gestures, semantic menu/filter JSON and filter resets. These feed the regular firmware input handlers,
so menu navigation and paint/erase use the same mode-dependent controls as the
panel. Direct grid/parameter editing tools and a browser remote are later work.

## Automatic on/off and builds

MCP is inactive after boot. Only `REMOTE <id> HELLO` activates a session;
opening USB serial for SD transfers does not. While inactive the firmware skips
matrix frame copies and remote input reconciliation. The bridge sends keepalives
every 750 ms. Disconnect sends `BYE`, immediately cancels remote controls and
stops capture. USB disconnect or a 3-second lease timeout does the same. Physical
controls continue working. `RELEASE` cancels remote holds/turns without ending
the session; commands after expiry require a new `HELLO` (`connect` in MCP).

Capture starts with the next presented frame after `HELLO`. Until then `DISPLAY`
returns `NO_FRAME`; matrix/PNG tools retry this read-only request briefly.

The default `pio run` build includes these automatically gated capabilities.
To omit MCP input, display capture, menu/filter readback and performance telemetry:

```sh
pio run -e teensy41-no-mcp
```

This sets `TOERN_REMOTE_CONTROL=0`; Arduino builds can set the same compiler
define. Disabled builds report `DISABLED` to remote requests. SD transfer, USB
MIDI/audio and the audio performance improvements remain available. Runtime
inactivity saves work; only the disabled build removes MCP code/state/frame RAM.
On this build, exclusion frees 1,088 bytes in RAM1, 1,536 bytes in RAM2 and
5,120 bytes in flash; the RAM1 instruction-bank allocation stays the same.

## Install and run

Use Python 3.10 or newer. From this directory:

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python server.py
```

The last command starts the stdio MCP server; it waits for MCP messages and is
not an interactive terminal. An MCP host can launch it with a configuration
like this (replace both paths with the absolute paths on your machine):

```json
{
  "mcpServers": {
    "toern": {
      "command": "/absolute/path/to/toern/tools/toern-mcp/.venv/bin/python",
      "args": ["/absolute/path/to/toern/tools/toern-mcp/server.py"]
    }
  }
}
```

Build the firmware with `pio run` at the repository root and flash using your
normal loader. This setup does not flash automatically or alter MCP host settings.

The bridge uses the official [MCP Python SDK v1 API](https://github.com/modelcontextprotocol/python-sdk/tree/v1.x),
with a `<2` dependency bound to keep the FastMCP interface consistent.

## Tools and an initial session

1. `list_devices()` lists Teensy serial devices (USB vendor ID `0x16c0`).
2. `connect(port)` verifies REMOTE protocol version 1.
3. `get_state()` reads mode, root/submenu setting ID, cursor, page, selected
   voice, playback, BPM, loaded kit, browsed kit, mode encoder positions, and
   input state. `positions` are the firmware's logical mode positions, **not**
   necessarily physical encoder order (SMP swaps two positions).
   State also includes read-only `autogen` diagnostics: activity, generation, section,
   audible foundation voice, remaining passes, octave-register shifts, and the
   sounding CH11 pitches at the current page onset. These support live transition
   checks without changing the pattern or patch.
   `get_performance()` reads existing audio CPU counters, CH11 ladder/oscillator
   costs, memory use and active poly voices without pausing playback. CPU fields
   ending in `_bp` use basis points: 100 bp = 1% CPU.
4. `get_display()` returns an enlarged PNG with logical top row first. Pixels
   preserve the rendered matrix's RGB values and software dimming. It represents
   the last presented frame; recording freezes the display just as on hardware.
   `get_matrix()` returns sparse JSON pixels instead of an image: 0-based x/y,
   top-left origin, black pixels omitted.
   `get_menu()` returns the actual active group, item, setting ID and sibling list.
   `get_filters(voice)` returns all pages, slider names, native values, maxima and
   defaults for voices 1–8, 11, 13 and 14. Omit voice to use the selected one.
   `reset_filters(scope="page")` resets the open page through touch 3;
   `reset_filters(scope="voice")` holds encoder 4 to reset every page of the open
   voice, including its synth instrument and envelopes. Both require FILTERMODE.
5. `turn_encoder(encoder=4, steps=1)` turns a physical encoder by detents,
   respecting its active step and bounds. Indices are **1-based**. Calls wait
   until the main loop consumes the turn, then read state after another poll.
6. `press_control(kind="button", index=1, hold_ms=120)` presses and releases
   encoder button 1. Use `kind="touch"` for a pad; pad 5 is FN. Long-press
   thresholds remain those of the firmware/current screen.
7. `perform_gesture(buttons=[2], touches=[5], hold_ms=800)` holds FN plus encoder
   button 2. Optional `encoder=1, steps=-1` turns a knob during the hold. Touches
   settle first, then encoder buttons press; button releases run before touch
   releases. Managed gestures require existing encoder/remote touch holds to be
   released first.
8. `set_control(kind, index, down)` supports individually managed holds.
   `release_controls()` releases all remote holds/cancels queued turns.
9. `disconnect()` releases remote controls and closes the port.

Input acceptance is not an assertion that the UI performed a particular action.
The current screen, FN, long-press timing, recording, and child restrictions all
still apply. Read state/display to verify the result. Remote turns are rejected
during recording. Captures contain the LED matrix, not encoder RGB rings or audio.

Close the SD explorer, firmware loader, and other serial clients before connecting.
The bridge owns one port and serializes all operations. SD binary transfers retain
their existing framing; this milestone does not expose SD transfers through MCP
or share a connection with the separate SD tool. On POSIX the port is also opened
with an advisory exclusive lock.

## Wire protocol: REMOTE v1

ASCII lines terminated by `\n`, fitting the existing 192-byte command buffer:

```text
REMOTE 1 HELLO
REMOTE 2 STATE
REMOTE 3 DISPLAY
REMOTE 4 TURN 4 -2
REMOTE 5 BUTTON 1 1
REMOTE 6 BUTTON 1 0
REMOTE 7 TOUCH 5 1
REMOTE 8 TOUCH 5 0
REMOTE 9 KEEPALIVE
REMOTE 10 RELEASE
REMOTE 11 BYE
```

- IDs: integers `1..2147483647`; the bridge increments them and matches replies.
- TURN: encoder `1..4`, nonzero detents `-64..64`. One queued turn per encoder;
  another turn for the same encoder returns `BUSY` until consumed. Pending turns
  are consumed once in the normal encoder poll. No unbounded event queue.
- BUTTON: encoder `1..4`, state `0/1`.
- TOUCH: pad `1..5`, state `0/1`, routed through normal touch debounce.
- RELEASE: clear remote holds and pending turns, keeping the session alive. Physical holds survive.
- BYE: release remote controls and end the session; replies with a minimal acknowledgement.
- HELLO and STATE return JSON state; KEEPALIVE returns a minimal acknowledgement.
- PERF returns audio CPU/memory and per-node CH11 costs without stopping play.
- MENUS returns the active group/item and its sibling list from the real menu tables.
- FILTERS [voice] returns slider names, values, maxima and defaults from the real
  slider definitions. Omit voice for the selected voice. These are read-only.
- DISPLAY returns `{id, ok, width, height, rgb}`. RGB is a hex string in row-major
  logical coordinates (left to right, top to bottom), undoing serpentine wiring
  and optional panel rotation. Before the first presentation it returns `NO_FRAME`.

Replies are one line prefixed `REMOTE `, followed by JSON:

```text
REMOTE {"id":4,"ok":false,"error":"BUSY"}
```

State responses also include `single_mode`, `demo_active`, selected filter page/count,
and the cursor cell (voice, velocity, probability, condition). DEMO keeps the
serial dispatcher alive so remote touch 1 can exit it.

State responses include `protocol:1`, `firmware`, `pending_turns`, and
`turn_error` (`null` or `ENCODER_IO` when hardware range/step reads fail). Input replies
report acceptance/current state; buttons/touches are processed over subsequent
ordinary input polls. TURN completion can be observed with `pending_turns:false`.
Control holds and queued turns expire after **3 seconds** without a valid remote
request, or when USB Serial closes. The bridge sends a heartbeat every 750ms and
also during long gestures. Lease release occurs when the firmware next services
its main loop; blocking pre-existing file operations can delay it.

Only one complete line command is dispatched per firmware loop. Overlong lines
are discarded through the next newline. `REMOTE` traffic bypasses the SD-menu
connection handler so it does not pause playback just to connect. Color binary
commands still work on COLR; partial remote lines are kept out of that parser.

Timeouts and malformed transport replies close the client connection. Mutations
are **never automatically retried**: an interrupted command may have executed.
IDs correlate replies; firmware does not deduplicate commands with reused IDs.

## Verification

From the repository root:

```sh
python3 _debug/tests/test_remote_firmware.py
python3 _debug/tests/test_remote_client.py
tools/toern-mcp/.venv/bin/python _debug/tests/test_remote_mcp.py
pio run
```

The firmware test compiles the real remote handlers with deterministic hardware
stubs and address/undefined-behavior sanitizers. Host tests simulate fragmented
serial replies, command errors, timeouts, gestures, and PNG data. These checks do
not substitute for hardware validation of audio timing and panel parity.


## Hardware audit

See [the hardware checklist](HARDWARE-AUDIT.md) for the item-by-item results.
To repeat the audit with an attached device running this firmware:

```sh
 .venv/bin/python ../../_debug/analysis/audit_device.py --port /dev/cu.usbmodemXXXX --report /tmp/toern-audit.json
```

This is a **mutating hardware test**: it paints/erases one empty test cell in
SINGLE, adjusts and restores menu/filter controls, and resets all 11 playable
voices to defaults. It leaves the device in DRAW, paused. It makes no backup.
It does not load/save/delete files, record samples, launch generation or apply
wiping/global-reset operations. CHLD is navigated but not enabled because enabling
it rewrites mute masks. Use `--section menus` for navigation-only coverage;
`filters`, `settings`, `special`, and `reset` run individual sections. With
`--section filters`, `--voice 11` restricts the filter pass to one voice.

The audit reads fresh slider values after each INST change: restoring an instrument
reapplies its preset to sibling controls, so an earlier page snapshot is stale.


## CH11 performance

The polyphonic CH11 DSP objects, small mixers, audio routing connections and
sampler controllers live in internal DTCM; large samples, delay lines and the
`SMP` settings structure remain in PSRAM. The canonical build still uses smallest-code globally;
only `Audio/filter_ladder.cpp` uses `-O2` so the per-sample helpers inline. The
fourfold oversampling and FIR interpolation are retained. See
[the measured performance investigation](PERFORMANCE.md).

The default reverb uses `TOERN_TREEVERB_QUALITY=1`: six combs and four diffusion
stages. `0` selects the preceding light 4/2 network; `2` is the full 8/4 reference
with very little RAM1 headroom. `get_performance()` includes `reverb_quality`,
and the USB audio runner records it in its report. See [reverb measurements](../../docs/devdocs/docs/audio/effects.md#treeverb).

## USB audio quality tests

The hardware playback runner uses actual USB recordings and source WAV comparisons:

```sh
.venv/bin/pip install -r ../../_debug/analysis/requirements-audio-test.txt
.venv/bin/python ../../_debug/analysis/audio_quality.py --port /dev/cu.usbmodemXXXX \
  --full-reset --channels 1,2 --reverb --output /tmp/toern-audio-quality
```

`--full-reset` executes **RSET → FULL**, clearing the current pattern/settings first. Omit it to preserve the entry state. The test temporarily adjusts filters and runs a test sequence, restores filters and the RAM pattern, and uses a non-saving pause for the temporary sequence. It leaves WAV captures and `report.json` in the output directory and exits nonzero for failed checks. It makes no pattern backup file.

Checks cover repeated dry playback, original-WAV fidelity, clipping-related distortion, added clicks, missing blocks, silence, source-aware attack/decay/sustain, explicit release and real sequencer timing. `--reverb` additionally checks maximum-room tails, persistent noise and bypass recovery. Capture status and measured CPU load accompany the cases. The recorder selects the Teensy USB input, not the computer microphone.

References normally come from the loaded kit. After loading a custom voice, use `--reference '1=/absolute/path/to/07 Kick 8.wav'` to compare it against that local source; the test also verifies every frame of its saved SP0 PCM. Custom references cannot be combined with FULL, which would reload kit 1. The reference runner supports sample voices 1–8; CH1/CH2 are the hardware test targets. It does not certify the separate synth paths or analog outputs.

Default sample playback preserves the entire source tail; trim and ADSR changes are explicit. The stock envelope's eight-sample zero-attack ramp and output limiter remain modeled, and the report separately includes SNR against the unmodified WAV. See [sample playback developer notes](../../docs/devdocs/docs/audio/samples.md) for the restart fix, exact frame windows and validation details.
