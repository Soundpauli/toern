"""Local stdio MCP bridge for a USB-serial TŒRN running REMOTE v1 firmware."""
import atexit
from typing import Literal

from mcp.server.fastmcp import FastMCP, Image
from serial.tools import list_ports

from remote import ToernRemote, display_png

mcp = FastMCP("Toern")
remote = ToernRemote()
atexit.register(remote.disconnect)


@mcp.tool()
def list_devices() -> list[dict]:
    """List USB serial ports. Teensy normally uses USB vendor ID 0x16c0."""
    return [{"port": p.device, "description": p.description, "vid": p.vid,
             "pid": p.pid, "serial_number": p.serial_number}
            for p in list_ports.comports() if p.vid == 0x16c0]


@mcp.tool()
def connect(port: str) -> dict:
    """Connect to Toern and verify REMOTE v1. Close other serial clients first."""
    return remote.connect(port)


@mcp.tool()
def disconnect() -> dict:
    """Release remote controls and close the serial port."""
    return remote.disconnect()


@mcp.tool()
def get_state() -> dict:
    """Read mode, menu setting, cursor, voice, transport and held input state."""
    return remote.get_state()


@mcp.tool()
def get_performance() -> dict:
    """Read audio CPU, memory use and CH11 per-node costs without stopping playback.

    CPU values ending in _bp use basis points: 100 bp = 1% CPU. Ladder/oscillator
    entries are grouped per poly voice. Active envelopes include release tails.
    """
    return remote.request("PERF")


@mcp.tool()
def get_menu() -> dict:
    """Read active menu group, item and its sibling list as JSON. Does not navigate."""
    return remote.request("MENUS")


@mcp.tool()
def get_filters(voice: int | None = None) -> dict:
    """Read every filter page/slider value, bounds and defaults for a playable voice.

    Voices: 1..8, 11, 13, 14. Omit voice for the selected voice. Pages and encoder
    indices are 1-based. Values use the device's native slider units.
    """
    if voice is not None and voice not in [1,2,3,4,5,6,7,8,11,13,14]:
        raise ValueError("Unsupported voice")
    return remote.request("FILTERS", *([] if voice is None else [voice]))


@mcp.tool()
def reset_filters(scope: Literal["page", "voice"] = "page") -> dict:
    """Reset the open filter page or every page of its voice using panel gestures.

    Requires FILTERMODE. Page reset taps touch 3; whole-voice reset holds encoder
    4. Whole-voice reset also restores the synth instrument/envelopes to defaults.
    """
    if scope not in ("page", "voice"):
        raise ValueError("Scope must be page or voice")
    with remote.lock:
        state = remote.get_state()
        if state["mode"] != "FILTERMODE":
            raise ValueError("Open the voice's filter screen before resetting")
        if scope == "page":
            remote.gesture([], [3], 120)
        else:
            remote.gesture([4], [], 900)
        return remote.request("FILTERS", state["voice"])


@mcp.tool()
def get_matrix() -> dict:
    """Read the displayed matrix as sparse JSON pixels (0-based, top-left origin).

    Black pixels are omitted. Each lit pixel has x, y and a #rrggbb color. This
    is a read-only alternative to get_display's PNG.
    """
    frame = remote.get_display_frame()
    rgb = bytes.fromhex(frame["rgb"])
    w, h = frame["width"], frame["height"]
    if len(rgb) != w*h*3:
        raise ValueError("Incomplete matrix frame")
    pixels = []
    for index in range(w*h):
        color = rgb[index*3:index*3+3]
        if any(color):
            pixels.append({"x": index % w, "y": index // w, "color": "#"+color.hex()})
    return {"width": w, "height": h, "origin": "top-left", "pixels": pixels}


@mcp.tool()
def get_display() -> Image:
    """Read the rendered matrix as an enlarged PNG, top row first. Does not redraw."""
    return Image(data=display_png(remote.get_display_frame()), format="png")


@mcp.tool()
def turn_encoder(encoder: int, steps: int) -> dict:
    """Turn physical encoder 1..4 by -64..64 steps and await input consumption."""
    return remote.turn_encoder(encoder, steps)


@mcp.tool()
def set_control(kind: Literal["button", "touch"], index: int, down: bool) -> dict:
    """Hold/release encoder button 1..4 or touch pad 1..5 (5=FN). Call again to release."""
    return remote.set_control(kind, index, down)


@mcp.tool()
def press_control(kind: Literal["button", "touch"], index: int, hold_ms: int = 120) -> dict:
    """Press and release one control; 120ms is short, longer holds use firmware thresholds."""
    if kind not in ("button", "touch"):
        raise ValueError("Kind must be button or touch")
    return remote.gesture([index] if kind == "button" else [],
                          [index] if kind == "touch" else [], hold_ms)


@mcp.tool()
def perform_gesture(buttons: list[int], touches: list[int], hold_ms: int = 120,
                    encoder: int | None = None, steps: int = 1) -> dict:
    """Hold a combination, optionally turn an encoder, then release. Touch 5 is FN.

    Indices are physical and 1-based. Observe resulting state/display because
    behavior depends on the device mode. Hold duration is limited to 80..2000ms.
    """
    return remote.gesture(buttons, touches, hold_ms, encoder, steps)


@mcp.tool()
def release_controls() -> dict:
    """Release all remote holds and cancel queued turns; physical holds survive."""
    return remote.request("RELEASE")


if __name__ == "__main__":
    mcp.run(transport="stdio")
