"""TŒRN REMOTE v1 client. Does not import MCP; also usable by other remotes."""
from __future__ import annotations

import json
import struct
import threading
import time
import zlib
from typing import Any


class RemoteError(RuntimeError):
    pass


class DeviceRejected(RemoteError):
    pass


class ToernRemote:
    def __init__(self, serial_factory=None, timeout: float = 2.0):
        self.lock = threading.RLock()
        self.serial_factory = serial_factory
        self.timeout = timeout
        self.serial = None
        self.port: str | None = None
        self._id = 0
        self._stop = threading.Event()

    def connect(self, port: str) -> dict[str, Any]:
        with self.lock:
            self.disconnect()
            factory = self.serial_factory
            if factory is None:
                import serial
                factory = serial.Serial
            options = dict(port=port, baudrate=115200, timeout=0.1, write_timeout=self.timeout)
            # POSIX advisory exclusivity; competing browser/SD clients must close first.
            import os
            if os.name == "posix":
                options["exclusive"] = True
            self.serial = factory(**options)
            self.port = port
            try:
                # Teensy USB Serial requires DTR to have been asserted for 15ms.
                time.sleep(0.05)
                state = self.request("HELLO")
                if state.get("protocol") != 1:
                    raise RemoteError("Unsupported Toern REMOTE protocol")
            except Exception:
                if self.serial is not None:
                    self.serial.close()
                self.serial = None
                self.port = None
                raise
            self._stop = threading.Event()
            threading.Thread(target=self._keepalive, args=(self._stop,), daemon=True).start()
            return state

    def disconnect(self) -> dict[str, bool]:
        with self.lock:
            self._stop.set()
            if self.serial is not None:
                try:
                    self.request("RELEASE")
                except Exception:
                    pass  # Firmware lease also releases remote controls after loss.
                finally:
                    self.serial.close()
                    self.serial = None
                    self.port = None
            return {"connected": False}

    def _keepalive(self, stop: threading.Event):
        while not stop.wait(0.75):
            with self.lock:
                if stop.is_set():
                    return
                try:
                    self.request("KEEPALIVE")
                except Exception:
                    stop.set()
                    return

    def request(self, operation: str, *args: int) -> dict[str, Any]:
        with self.lock:
            if self.serial is None:
                raise RemoteError("Not connected; call connect first")
            if operation not in {"HELLO", "STATE", "DISPLAY", "KEEPALIVE", "RELEASE", "TURN", "BUTTON", "TOUCH", "FILTERS", "MENUS", "PERF"}:
                raise ValueError("Unknown remote operation")
            if any(type(arg) is not int for arg in args):
                raise ValueError("Protocol arguments must be integers")
            self._id = self._id % 2147483647 + 1
            request_id = self._id
            command = " ".join(["REMOTE", str(request_id), operation, *map(str, args)]) + "\n"
            try:
                encoded = command.encode("ascii")
                if self.serial.write(encoded) != len(encoded):
                    raise RemoteError("Incomplete serial write")
                deadline = time.monotonic() + self.timeout
                buffered = bytearray()
                while time.monotonic() < deadline:
                    piece = self.serial.read_until(b"\n", 65536)
                    buffered.extend(piece)
                    if len(buffered) > 65536:
                        raise RemoteError("Oversized serial reply")
                    if not buffered.endswith(b"\n"):
                        continue
                    line = bytes(buffered).strip()
                    buffered.clear()
                    if not line.startswith(b"REMOTE "):
                        continue  # Boot banners and diagnostics are not replies.
                    reply = json.loads(line[7:])
                    if reply.get("id") != request_id:
                        continue
                    if not reply.get("ok"):
                        raise DeviceRejected(str(reply.get("error", "Device rejected command")))
                    return reply
                raise TimeoutError("Toern did not reply; command outcome is unknown. No automatic retry.")
            except DeviceRejected:
                # A well-framed device rejection leaves the transport usable.
                raise
            except Exception:
                # A timeout/parse/transport failure makes further mutation unsafe.
                self._stop.set()
                self.serial.close()
                self.serial = None
                self.port = None
                raise

    def get_state(self):
        return self.request("STATE")

    @staticmethod
    def _index(value: int, maximum: int):
        if type(value) is not int or not 1 <= value <= maximum:
            raise ValueError(f"Control index must be 1..{maximum}")

    def turn_encoder(self, encoder: int, steps: int):
        self._index(encoder, 4)
        if type(steps) is not int or not steps or not -64 <= steps <= 64:
            raise ValueError("Steps must be -64..64, excluding zero")
        with self.lock:
            self.request("TURN", encoder, steps)
            # Acceptance occurs before the regular input poll. Wait for consumption,
            # then a further poll so dependent UI handlers can run.
            deadline = time.monotonic() + self.timeout
            while time.monotonic() < deadline:
                time.sleep(0.04)
                state = self.get_state()
                if state.get("turn_error"):
                    raise RemoteError(state["turn_error"])
                if not state["pending_turns"]:
                    time.sleep(0.04)
                    return self.get_state()
            self.request("RELEASE")
            raise TimeoutError("Encoder input was not consumed; queued input cancelled")

    def set_control(self, kind: str, index: int, down: bool):
        if kind not in ("button", "touch"):
            raise ValueError("Kind must be button or touch")
        self._index(index, 4 if kind == "button" else 5)
        if type(down) is not bool:
            raise ValueError("Down must be a boolean")
        with self.lock:
            self.request(kind.upper(), index, int(down))
            time.sleep(0.08)  # Touch debounce and ordinary main-loop input polling.
            return self.get_state()

    def gesture(self, buttons: list[int], touches: list[int], hold_ms: int = 120,
                encoder: int | None = None, steps: int = 1):
        if type(hold_ms) is not int or not 80 <= hold_ms <= 2000:
            raise ValueError("Hold duration must be 80..2000 ms")
        if len(set(buttons)) != len(buttons) or len(set(touches)) != len(touches):
            raise ValueError("Duplicate controls in gesture")
        for index in buttons:
            self._index(index, 4)
        for index in touches:
            self._index(index, 5)
        if encoder is not None:
            self._index(encoder, 4)
            if type(steps) is not int or not steps or not -64 <= steps <= 64:
                raise ValueError("Steps must be -64..64, excluding zero")
        with self.lock:
            initial = self.get_state()
            if any(initial["buttons"]) or any(initial["remote_touches"]):
                raise RemoteError("Release held controls before starting a managed gesture")
            try:
                # FN/pads settle before encoder presses, preserving FN classification.
                for index in touches:
                    self.request("TOUCH", index, 1)
                time.sleep(0.08)
                for index in buttons:
                    self.request("BUTTON", index, 1)
                if encoder is not None:
                    self.turn_encoder(encoder, steps)
                # Explicit heartbeats during long managed gestures; the background
                # keepalive intentionally cannot interleave while this lock is held.
                deadline = time.monotonic() + hold_ms / 1000
                while time.monotonic() < deadline:
                    time.sleep(min(0.4, max(0, deadline - time.monotonic())))
                    self.request("KEEPALIVE")
            finally:
                for index in reversed(buttons):
                    self.request("BUTTON", index, 0)
                time.sleep(0.08)  # Process modified releases while FN is still held.
                for index in reversed(touches):
                    self.request("TOUCH", index, 0)
            time.sleep(0.1)
            return self.get_state()


def display_png(frame: dict, scale: int = 16) -> bytes:
    """Enlarge the logical RGB frame with nearest-neighbor scaling, no image deps."""
    w, h = frame["width"], frame["height"]
    if type(w) is not int or type(h) is not int or not 1 <= w <= 128 or not 1 <= h <= 32:
        raise RemoteError("Invalid display dimensions")
    rgb = bytes.fromhex(frame["rgb"])
    if len(rgb) != w * h * 3:
        raise RemoteError("Incomplete RGB frame")
    rows = []
    for y in range(h):
        row = b"\0" + b"".join(rgb[(y*w+x)*3:(y*w+x+1)*3] * scale for x in range(w))
        rows.extend([row] * scale)
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w*scale, h*scale, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(b"".join(rows))) + chunk(b"IEND", b""))
