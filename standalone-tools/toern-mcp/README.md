# TŒRN MCP remote (first milestone)

A local stdio MCP server controls a USB-connected TŒRN through its existing
serial port. It requires firmware built from this revision: older firmware
supports SD exchange but does **not** understand `REMOTE` commands.

Implemented: state readback, a PNG of the last presented matrix (including
transient overlays), encoder turns, encoder button holds/releases, all five
touch pads, timed gestures, semantic menu/filter JSON and filter resets. These feed the regular firmware input handlers,
so menu navigation and paint/erase use the same mode-dependent controls as the
panel. Direct grid/parameter editing tools and a browser remote are later work.

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
      "command": "/absolute/path/to/toern/standalone-tools/toern-mcp/.venv/bin/python",
      "args": ["/absolute/path/to/toern/standalone-tools/toern-mcp/server.py"]
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
```

- IDs: integers `1..2147483647`; the bridge increments them and matches replies.
- TURN: encoder `1..4`, nonzero detents `-64..64`. One queued turn per encoder;
  another turn for the same encoder returns `BUSY` until consumed. Pending turns
  are consumed once in the normal encoder poll. No unbounded event queue.
- BUTTON: encoder `1..4`, state `0/1`.
- TOUCH: pad `1..5`, state `0/1`, routed through normal touch debounce.
- RELEASE: clear remote holds and pending turns. Physical holds survive.
- HELLO and STATE return JSON state; KEEPALIVE returns a minimal acknowledgement.
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
python3 tests/test_remote_firmware.py
python3 tests/test_remote_client.py
standalone-tools/toern-mcp/.venv/bin/python tests/test_remote_mcp.py
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
.venv/bin/python audit_device.py --port /dev/cu.usbmodemXXXX --report /tmp/toern-audit.json
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
