---
title: MCP remote control
description: Serial panel control, semantic JSON readback and automatic MCP session gating.
---

# MCP remote control

**Source:** [`tools/toern-mcp/`](https://github.com/Soundpauli/toern/tree/main/tools/toern-mcp)

The local Python stdio MCP server controls a USB-connected TŒRN over the existing serial port. It feeds the normal firmware input handlers: encoder turns, encoder buttons and all five touch pads. These support painting, single mode, menu navigation, filter pages and resetting the current filter page or all filters for a voice. Controls retain their normal mode-dependent meaning.

Read-only tools expose device state, menu items, filter names/values/defaults, matrix pixels as JSON, optional matrix PNGs and audio performance telemetry. See the source directory's README for installation, MCP host configuration and the complete tool/protocol reference. The bridge requires firmware with REMOTE support; close other clients using the same serial port before connecting.

## Automatic session gating

MCP is inactive after boot; no menu switch is needed.

| Event | Firmware behavior |
|-------|-------------------|
| USB serial opened for SD transfer or diagnostics | MCP stays inactive |
| `REMOTE <id> HELLO` | Starts a session and enables matrix capture |
| Valid session requests / `KEEPALIVE` | Renew the three-second lease |
| `RELEASE` | Cancels remote holds and queued turns; session stays active |
| `BYE` | Releases remote controls, cancels queued turns and stops capture |
| USB serial closes or lease expires | Ends the session as above |

The bridge sends keepalives every 750 ms and sends `BYE` on disconnect. Physical controls continue working, and ending a remote session does not clear physical button holds. After expiry, requests return `NO_SESSION` until a new `HELLO`; use the MCP `connect` tool to reconnect. Input gestures are not automatically replayed.

While inactive, `remoteService()` returns immediately and the firmware skips display frame copies and remote input reconciliation. Matrix capture begins on the next presented frame after `HELLO`, including temporary overlays. Before that frame, `DISPLAY` returns `NO_FRAME`; the matrix/PNG tools briefly retry this read-only request. Ending a session invalidates its captured frame so a later connection cannot receive a previous session's image.

Session cleanup runs in the main loop. Existing blocking file operations can delay timeout handling; it does not run in the audio interrupt.

## Build with or without MCP

From the repository root:

```bash
pio run                       # MCP included, inactive until HELLO
pio run -e teensy41-no-mcp     # MCP excluded
```

The second environment sets `TOERN_REMOTE_CONTROL=0`. Arduino builds can supply the same compiler define; it defaults to `1`. Exclusion removes remote input state, the matrix frame buffer, semantic menu/filter readback and performance telemetry. A small serial reply remains so clients receive `DISABLED` instead of waiting for a timeout. The SD serial dispatcher, USB MIDI/audio configuration and audio DSP optimizations remain available.

Measured build sizes for the current 816 MHz, smallest-code configuration:

| Resource | MCP included | MCP excluded | Freed by exclusion |
|----------|--------------|--------------|--------------------|
| RAM1 free for locals | 43,360 bytes | 44,448 bytes | 1,088 bytes |
| RAM2 free for heap | 325,664 bytes | 327,200 bytes | 1,536 bytes |
| Flash free for files | 7,737,348 bytes | 7,742,468 bytes | 5,120 bytes |

RAM1 instruction-bank allocation stays the same. These are build-specific figures, not runtime heap measurements. Automatic session inactivity saves processing work; compile-time exclusion also saves memory. The CH11 DSP and audio graph improvements are independent of this switch.

## Firmware implementation

| File | Role |
|------|------|
| `platformio/src/toern_remote_input.h` | Build switch, bounded input state and lease |
| `toern_remote.ino` | Session lifecycle, parser, controls and matrix capture |
| `toern_remote_ui.ino` | Semantic menu/filter and cursor readback |
| `toern_remote_perf.ino` | On-demand audio performance counters |
| `toern_sd_serial.ino` | Shared line dispatch; REMOTE bypasses SD client activation |
| `tools/toern-mcp/remote.py` | Serial framing, keepalives, gestures and disconnect |
| `tools/toern-mcp/server.py` | MCP tool definitions |

Only one complete serial command is dispatched per main-loop iteration. Remote commands do not enter the SD transfer session merely to control the panel. Frame copies happen alongside normal LED presentation only during an active remote session.

## Validation

Both PlatformIO environments build successfully. Linked-symbol inspection confirms the disabled build omits remote state, frame capture, menu readback and performance handlers while retaining the SD poller. Firmware tests cover inactive capture, HELLO activation, BYE, expiry including timer wrap, queued-turn cancellation, preservation of physical holds and serial PING coexistence. Client tests cover disconnect, disabled-build errors and first-frame retries.

```bash
python3 _debug/tests/test_remote_firmware.py
python3 _debug/tests/test_remote_client.py
python3 _debug/tests/test_remote_mcp.py
```

The MCP stdio test requires the bridge's Python dependencies. These checks build and test the code; they do not flash the device.
