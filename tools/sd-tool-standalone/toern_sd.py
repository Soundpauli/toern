#!/usr/bin/env python3
"""TŒRN SD card helper over USB Serial.

On the device open Menu → ETC → SD (file server is active while that page is open).
Use with USB Type Serial + MIDI (or any Serial type).

CLI:
  python3 toern_sd.py ports
  python3 toern_sd.py -p /dev/cu.usbmodemXXXX ping
  python3 toern_sd.py -p /dev/cu.usbmodemXXXX list /
  python3 toern_sd.py -p /dev/cu.usbmodemXXXX put ./kick.wav /kick.wav
  python3 toern_sd.py -p /dev/cu.usbmodemXXXX get /kick.wav ./kick.wav
  python3 toern_sd.py -p /dev/cu.usbmodemXXXX rm /kick.wav

Web explorer (Web Serial in the browser; also used by sdtool.tyng.app static deploy):
  python3 toern_sd.py web
  # opens http://127.0.0.1:8787
"""

from __future__ import annotations

import argparse
import json
import mimetypes
import re
import sys
import threading
import time
import urllib.parse
import webbrowser
import zlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

BAUD = 115200
IO_TIMEOUT = 60.0
WEB_HOST = "127.0.0.1"
WEB_PORT = 8787


def _require_serial():
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        print("Missing dependency: pip install pyserial", file=sys.stderr)
        sys.exit(1)
    return serial, list_ports


def crc32_hex(data: bytes) -> str:
    return f"{zlib.crc32(data) & 0xFFFFFFFF:08X}"


def join_sd_path(parent: str, name: str) -> str:
    parent = (parent or "/").replace("\\", "/")
    name = name.strip("/").replace("\\", "/")
    if parent in ("", "/"):
        return "/" + name
    return parent.rstrip("/") + "/" + name


def parent_sd_path(path: str) -> str:
    path = (path or "/").replace("\\", "/").rstrip("/")
    if not path or path == "/":
        return "/"
    idx = path.rfind("/")
    if idx <= 0:
        return "/"
    return path[:idx] or "/"


def is_hidden_name(name: str) -> bool:
    return bool(name) and name.startswith(".")


def list_serial_ports() -> list[dict[str, str]]:
    _, list_ports = _require_serial()
    out = []
    for p in list_ports.comports():
        desc = p.description or ""
        hwid = p.hwid or ""
        manuf = getattr(p, "manufacturer", None) or ""
        product = getattr(p, "product", None) or ""
        blob = f"{p.device} {desc} {hwid} {manuf} {product}".lower()
        is_teensy = ("teensy" in blob) or ("16c0:" in blob.replace(" ", ""))
        is_midi = ("midi" in blob) or ("midix" in blob)
        out.append({
            "device": p.device,
            "description": desc,
            "hwid": hwid,
            "manufacturer": manuf,
            "product": product,
            "teensy": is_teensy,
            "teensy_midi": is_teensy and is_midi,
        })
    # Prefer Teensy MIDI (e.g. MIDIx16), then any Teensy, then others
    out.sort(key=lambda x: (
        0 if x.get("teensy_midi") else 1 if x.get("teensy") else 2,
        x["device"].lower(),
    ))
    return out


def open_port(port: str):
    serial, _ = _require_serial()
    ser = serial.Serial(port, BAUD, timeout=1.0, write_timeout=IO_TIMEOUT)
    time.sleep(0.3)
    ser.reset_input_buffer()
    return ser


def drain_banner(ser) -> None:
    old = ser.timeout
    ser.timeout = 0.2
    while ser.readline():
        pass
    ser.timeout = old


def read_line(ser, timeout: float = IO_TIMEOUT) -> str:
    deadline = time.monotonic() + timeout
    buf = bytearray()
    while time.monotonic() < deadline:
        b = ser.read(1)
        if not b:
            continue
        if b == b"\n":
            return buf.decode("utf-8", errors="replace").rstrip("\r")
        if b != b"\r":
            buf += b
    raise TimeoutError("timeout waiting for line from device")


def expect_ok(line: str, prefix: str = "OK") -> str:
    if line.startswith("ERR"):
        raise RuntimeError(line)
    if not line.startswith(prefix):
        raise RuntimeError(f"unexpected response: {line}")
    return line


class ToernSd:
    """Thread-safe serial SD client."""

    def __init__(self, port: str):
        self.port = port
        self._lock = threading.RLock()
        self.ser = open_port(port)
        drain_banner(self.ser)

    def close(self) -> None:
        with self._lock:
            if self.ser and self.ser.is_open:
                self.ser.close()

    def ping(self) -> str:
        with self._lock:
            self.ser.write(b"PING\n")
            self.ser.flush()
            return expect_ok(read_line(self.ser))

    def list_dir(self, path: str = "/", *, hide_dot: bool = True) -> list[dict[str, Any]]:
        with self._lock:
            self.ser.write(f"LIST {path}\n".encode("utf-8"))
            self.ser.flush()
            entries: list[dict[str, Any]] = []
            while True:
                line = read_line(self.ser)
                if line == "OK":
                    break
                if line.startswith("ERR"):
                    raise RuntimeError(line)
                if line.startswith("D "):
                    name = line[2:].strip()
                    if hide_dot and is_hidden_name(name):
                        continue
                    entries.append({
                        "type": "dir",
                        "name": name,
                        "size": None,
                        "duration_ms": None,
                    })
                elif line.startswith("F "):
                    rest = line[2:].strip()
                    # F <size> <duration_ms> <name...>  (duration_ms = -1 if unknown)
                    parts = rest.split(None, 2)
                    if len(parts) < 2:
                        continue
                    size = int(parts[0])
                    duration_ms: int | None = None
                    if len(parts) == 3:
                        try:
                            dur = int(parts[1])
                            name = parts[2].strip()
                            duration_ms = dur if dur >= 0 else None
                        except ValueError:
                            name = rest[len(parts[0]) :].strip()
                    else:
                        name = parts[1].strip()
                    if hide_dot and is_hidden_name(name):
                        continue
                    entries.append({
                        "type": "file",
                        "name": name,
                        "size": size,
                        "duration_ms": duration_ms,
                    })
            entries.sort(key=lambda e: (0 if e["type"] == "dir" else 1, e["name"].lower()))
            return entries

    def rename(self, src: str, dst: str) -> None:
        with self._lock:
            self.ser.write(f"MV {src} {dst}\n".encode("utf-8"))
            self.ser.flush()
            expect_ok(read_line(self.ser))

    def rm(self, path: str) -> None:
        with self._lock:
            self.ser.write(f"RM {path}\n".encode("utf-8"))
            self.ser.flush()
            expect_ok(read_line(self.ser))

    def mkdir(self, path: str) -> None:
        with self._lock:
            self.ser.write(f"MKDIR {path}\n".encode("utf-8"))
            self.ser.flush()
            expect_ok(read_line(self.ser))

    def put_bytes(self, remote: str, data: bytes) -> None:
        with self._lock:
            csum = crc32_hex(data)
            self.ser.write(f"PUT {remote} {len(data)} {csum}\n".encode("utf-8"))
            self.ser.flush()
            ready = read_line(self.ser)
            if ready != "READY":
                raise RuntimeError(ready if ready.startswith("ERR") else f"expected READY, got: {ready}")
            # USB CDC ignores baud — wait for ACK after each block or Teensy RX overruns.
            block = 8192
            sent = 0
            while sent < len(data):
                n = min(block, len(data) - sent)
                self.ser.write(data[sent : sent + n])
                self.ser.flush()
                ack = read_line(self.ser, timeout=IO_TIMEOUT)
                if ack != "ACK":
                    raise RuntimeError(ack if ack.startswith("ERR") else f"expected ACK, got: {ack}")
                sent += n
            expect_ok(read_line(self.ser, timeout=IO_TIMEOUT))

    def get_bytes(self, remote: str) -> bytes:
        with self._lock:
            self.ser.write(f"GET {remote}\n".encode("utf-8"))
            self.ser.flush()
            header = read_line(self.ser)
            if header.startswith("ERR"):
                raise RuntimeError(header)
            parts = header.split()
            # New firmware: OK <size>  … data … CRC <hex>
            if len(parts) == 2 and parts[0] == "OK":
                size = int(parts[1])
            elif len(parts) >= 3 and parts[0] == "OK":
                raise RuntimeError("Device firmware too old for GET — reflash (OK size + CRC trailer)")
            else:
                raise RuntimeError(f"unexpected response: {header}")
            block = 512
            self.ser.write(b"ACK\n")
            self.ser.flush()
            chunks: list[bytes] = []
            got_n = 0
            while got_n < size:
                need = min(block, size - got_n)
                deadline = time.monotonic() + IO_TIMEOUT
                piece = bytearray()
                while len(piece) < need:
                    if time.monotonic() > deadline:
                        raise TimeoutError("timeout receiving file data")
                    block_read = self.ser.read(need - len(piece))
                    if not block_read:
                        continue
                    piece.extend(block_read)
                    deadline = time.monotonic() + IO_TIMEOUT
                chunks.append(bytes(piece))
                got_n += need
                self.ser.write(b"ACK\n")
                self.ser.flush()
            data = b"".join(chunks)
            crc_line = read_line(self.ser, timeout=IO_TIMEOUT)
            cparts = crc_line.split()
            if len(cparts) != 2 or cparts[0].upper() != "CRC":
                raise RuntimeError(f"expected CRC trailer, got: {crc_line}")
            expect = cparts[1].upper().zfill(8)
            got = crc32_hex(data)
            if got != expect:
                raise RuntimeError(f"CRC mismatch: got {got} want {expect}")
            return data

    def rm_recursive(self, path: str) -> None:
        path = path.replace("\\", "/")
        if path in ("", "/"):
            raise RuntimeError("ERR REFUSE_ROOT")
        parent = parent_sd_path(path)
        name = path.rstrip("/").rsplit("/", 1)[-1]
        entries = self.list_dir(parent, hide_dot=False)
        match = next((e for e in entries if e["name"] == name), None)
        if match is None:
            self.rm(path)
            return
        if match["type"] == "dir":
            for child in self.list_dir(path, hide_dot=False):
                self.rm_recursive(join_sd_path(path, child["name"]))
        self.rm(path)


class SdSession:
    """Shared web/CLI session: connect/disconnect serial from the UI."""

    def __init__(self) -> None:
        self._lock = threading.RLock()
        self.sd: ToernSd | None = None
        self._keepalive_stop = threading.Event()
        self._keepalive_thread: threading.Thread | None = None

    def status(self) -> dict[str, Any]:
        with self._lock:
            if self.sd is None:
                return {"connected": False, "port": None}
            return {"connected": True, "port": self.sd.port}

    def require(self) -> ToernSd:
        with self._lock:
            if self.sd is None:
                raise RuntimeError("not connected")
            return self.sd

    def _start_keepalive(self) -> None:
        self._stop_keepalive()
        self._keepalive_stop.clear()

        def loop() -> None:
            while not self._keepalive_stop.wait(2.0):
                with self._lock:
                    if self.sd is None:
                        continue
                    try:
                        self.sd.ping()
                    except Exception:
                        try:
                            self.sd.close()
                        except Exception:
                            pass
                        self.sd = None
                        break

        self._keepalive_thread = threading.Thread(target=loop, name="toern-sd-keepalive", daemon=True)
        self._keepalive_thread.start()

    def _stop_keepalive(self) -> None:
        self._keepalive_stop.set()
        t = self._keepalive_thread
        self._keepalive_thread = None
        if t and t.is_alive() and t is not threading.current_thread():
            t.join(timeout=1.0)

    def connect(self, port: str) -> dict[str, Any]:
        port = (port or "").strip()
        if not port:
            raise RuntimeError("missing port")
        self._stop_keepalive()
        with self._lock:
            if self.sd is not None and self.sd.port != port:
                self.sd.close()
                self.sd = None
            if self.sd is None:
                client = ToernSd(port)
                try:
                    msg = client.ping()
                except Exception:
                    client.close()
                    raise
                self.sd = client
            else:
                msg = self.sd.ping()
            port_out = self.sd.port
        self._start_keepalive()
        return {"connected": True, "port": port_out, "message": msg}

    def disconnect(self) -> None:
        self._stop_keepalive()
        with self._lock:
            if self.sd is not None:
                self.sd.close()
                self.sd = None


# --- CLI wrappers -----------------------------------------------------------

def cmd_ping(sd: ToernSd) -> None:
    print(sd.ping())


def cmd_list(sd: ToernSd, path: str) -> None:
    for e in sd.list_dir(path):
        if e["type"] == "dir":
            print(f"D {e['name']}")
        else:
            dur = e.get("duration_ms")
            dur_s = f"{(dur / 1000):.2f}s" if isinstance(dur, int) else "-"
            print(f"F {e['size']} {dur_s} {e['name']}")


def cmd_rm(sd: ToernSd, path: str) -> None:
    sd.rm_recursive(path)
    print("OK")


def cmd_mkdir(sd: ToernSd, path: str) -> None:
    sd.mkdir(path)
    print("OK")


def cmd_rename(sd: ToernSd, src: str, dst: str) -> None:
    sd.rename(src, dst)
    print("OK")


def cmd_put(sd: ToernSd, local: Path, remote: str) -> None:
    data = local.read_bytes()
    sd.put_bytes(remote, data)
    print(f"uploaded {local} -> {remote} ({len(data)} bytes, crc={crc32_hex(data)})")


def cmd_get(sd: ToernSd, remote: str, local: Path) -> None:
    data = sd.get_bytes(remote)
    local.parent.mkdir(parents=True, exist_ok=True)
    local.write_bytes(data)
    print(f"downloaded {remote} -> {local} ({len(data)} bytes, crc={crc32_hex(data)})")


def cmd_ports() -> None:
    ports = list_serial_ports()
    if not ports:
        print("No serial ports found.")
        return
    for p in ports:
        print(f"{p['device']}\t{p['description']}\t{p['hwid']}")


# --- Web UI -----------------------------------------------------------------

def _resolve_website_assets() -> dict[str, Path]:
    """Locate shared CSS/favicon for local repo layout or flat server deploy."""
    here = Path(__file__).resolve().parent
    website = here.parents[1]  # .../website
    repo_assets = {
        "/styles.css": website / "styles.css",
        "/tool-shell.css": website / "tools" / "tool-shell.css",
        "/favicon.svg": website / "favicon.svg",
    }
    flat_assets = {
        "/styles.css": here / "styles.css",
        "/tool-shell.css": here / "tool-shell.css",
        "/favicon.svg": here / "favicon.svg",
    }
    for mapping in (repo_assets, flat_assets):
        if all(p.is_file() for p in mapping.values()):
            return mapping
    return repo_assets


STATIC_FILES = _resolve_website_assets()


def load_explorer_html() -> str:
    path = Path(__file__).with_name("explorer.html")
    return path.read_text(encoding="utf-8")


def _guess_content_type(path: Path) -> str:
    ctype = mimetypes.guess_type(str(path))[0]
    return ctype or "application/octet-stream"



def parse_multipart(headers, rfile) -> dict[str, Any]:
    """Minimal multipart/form-data parser (path field + file upload)."""
    ctype = headers.get("Content-Type", "")
    m = re.search(r"boundary=([^;]+)", ctype, flags=re.I)
    if not m:
        raise RuntimeError("missing multipart boundary")
    boundary = m.group(1).strip().strip('"').encode("ascii", errors="ignore")
    length = int(headers.get("Content-Length", "0") or 0)
    body = rfile.read(length)
    sep = b"--" + boundary
    parts = body.split(sep)
    out: dict[str, Any] = {}
    for part in parts:
        if not part or part in (b"--", b"--\r\n", b"--\n"):
            continue
        if part.startswith(b"--"):
            break
        if part.startswith(b"\r\n"):
            part = part[2:]
        elif part.startswith(b"\n"):
            part = part[1:]
        if part.endswith(b"\r\n"):
            part = part[:-2]
        elif part.endswith(b"\n"):
            part = part[:-1]
        header_blob, _, content = part.partition(b"\r\n\r\n")
        if not _:
            header_blob, _, content = part.partition(b"\n\n")
        header_text = header_blob.decode("utf-8", errors="replace")
        name_m = re.search(r'name="([^"]+)"', header_text, flags=re.I)
        if not name_m:
            continue
        name = name_m.group(1)
        filename_m = re.search(r'filename="([^"]*)"', header_text, flags=re.I)
        if filename_m is not None:
            out[name] = {"filename": filename_m.group(1), "data": content}
        else:
            out[name] = content.decode("utf-8", errors="replace")
    return out


def _json_bytes(obj: Any, status: int = 200) -> tuple[int, bytes, str]:
    return status, json.dumps(obj).encode("utf-8"), "application/json; charset=utf-8"


def make_handler(session: SdSession):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, fmt: str, *args: Any) -> None:
            sys.stderr.write("%s - %s\n" % (self.address_string(), fmt % args))

        def _send(self, status: int, body: bytes, content_type: str, extra: dict | None = None) -> None:
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(body)))
            headers = {"Cache-Control": "no-store"}
            if extra:
                headers.update(extra)
            for k, v in headers.items():
                self.send_header(k, v)
            self.end_headers()
            self.wfile.write(body)

        def _read_json(self) -> dict:
            length = int(self.headers.get("Content-Length", "0") or 0)
            raw = self.rfile.read(length) if length else b"{}"
            if not raw:
                return {}
            return json.loads(raw.decode("utf-8"))

        def do_GET(self) -> None:  # noqa: N802
            parsed = urllib.parse.urlparse(self.path)
            path = parsed.path
            qs = urllib.parse.parse_qs(parsed.query)

            try:
                if path in ("/", "/index.html"):
                    body = load_explorer_html().encode("utf-8")
                    self._send(200, body, "text/html; charset=utf-8")
                    return

                if path in STATIC_FILES:
                    fpath = STATIC_FILES[path]
                    if not fpath.is_file():
                        raise FileNotFoundError(path)
                    body = fpath.read_bytes()
                    self._send(200, body, _guess_content_type(fpath), {"Cache-Control": "public, max-age=3600"})
                    return

                if path == "/api/ports":
                    status, body, ct = _json_bytes({"ok": True, "ports": list_serial_ports()})
                    self._send(status, body, ct)
                    return

                if path == "/api/status":
                    status, body, ct = _json_bytes({"ok": True, **session.status()})
                    self._send(status, body, ct)
                    return

                if path == "/api/ping":
                    msg = session.require().ping()
                    st = session.status()
                    status, body, ct = _json_bytes({"ok": True, "message": msg, **st})
                    self._send(status, body, ct)
                    return

                if path == "/api/list":
                    sd = session.require()
                    sd_path = (qs.get("path") or ["/"])[0]
                    entries = sd.list_dir(sd_path, hide_dot=True)
                    status, body, ct = _json_bytes({
                        "ok": True,
                        "path": sd_path,
                        "device": sd.port,
                        "entries": entries,
                    })
                    self._send(status, body, ct)
                    return

                if path == "/api/download":
                    sd = session.require()
                    sd_path = (qs.get("path") or [""])[0]
                    name = (qs.get("name") or [Path(sd_path).name or "file"])[0]
                    inline = (qs.get("inline") or ["0"])[0] in ("1", "true", "yes")
                    if not sd_path or is_hidden_name(Path(sd_path).name):
                        raise RuntimeError("ERR BAD_PATH")
                    data = sd.get_bytes(sd_path)
                    ctype = mimetypes.guess_type(name)[0] or "application/octet-stream"
                    if inline and name.lower().endswith(".wav"):
                        ctype = "audio/wav"
                    disp = ("inline" if inline else "attachment") + f'; filename="{name}"'
                    self._send(200, data, ctype, {"Content-Disposition": disp})
                    return

                status, body, ct = _json_bytes({"error": "not found"}, 404)
                self._send(status, body, ct)
            except Exception as e:  # noqa: BLE001
                code = 400 if str(e) == "not connected" else 500
                status, body, ct = _json_bytes({"error": str(e)}, code)
                self._send(status, body, ct)

        def do_POST(self) -> None:  # noqa: N802
            parsed = urllib.parse.urlparse(self.path)
            path = parsed.path
            try:
                if path == "/api/connect":
                    payload = self._read_json()
                    result = session.connect(str(payload.get("port") or ""))
                    status, body, ct = _json_bytes({"ok": True, **result})
                    self._send(status, body, ct)
                    return

                if path == "/api/disconnect":
                    session.disconnect()
                    status, body, ct = _json_bytes({"ok": True, "connected": False})
                    self._send(status, body, ct)
                    return

                if path == "/api/mkdir":
                    sd = session.require()
                    payload = self._read_json()
                    sd_path = str(payload.get("path") or "")
                    name = Path(sd_path.rstrip("/")).name
                    if not sd_path or is_hidden_name(name):
                        raise RuntimeError("ERR BAD_PATH")
                    sd.mkdir(sd_path)
                    status, body, ct = _json_bytes({"ok": True})
                    self._send(status, body, ct)
                    return

                if path == "/api/rm":
                    sd = session.require()
                    payload = self._read_json()
                    sd_path = str(payload.get("path") or "")
                    if not sd_path or sd_path in ("/", "") or is_hidden_name(Path(sd_path.rstrip("/")).name):
                        raise RuntimeError("ERR BAD_PATH")
                    sd.rm_recursive(sd_path)
                    status, body, ct = _json_bytes({"ok": True})
                    self._send(status, body, ct)
                    return

                if path == "/api/rename":
                    sd = session.require()
                    payload = self._read_json()
                    src = str(payload.get("from") or "")
                    dst = str(payload.get("to") or "")
                    src_name = Path(src.rstrip("/")).name
                    dst_name = Path(dst.rstrip("/")).name
                    if (not src or not dst or src in ("/", "") or dst in ("/", "")
                            or is_hidden_name(src_name) or is_hidden_name(dst_name)):
                        raise RuntimeError("ERR BAD_PATH")
                    sd.rename(src, dst)
                    status, body, ct = _json_bytes({"ok": True})
                    self._send(status, body, ct)
                    return

                if path == "/api/upload":
                    sd = session.require()
                    ctype = self.headers.get("Content-Type", "")
                    if "multipart/form-data" not in ctype:
                        raise RuntimeError("expected multipart upload")
                    form = parse_multipart(self.headers, self.rfile)
                    dest_dir = str(form.get("path") or "/")
                    file_item = form.get("file")
                    if not isinstance(file_item, dict) or not file_item.get("filename"):
                        raise RuntimeError("missing file")
                    filename = Path(str(file_item["filename"])).name
                    if not filename or is_hidden_name(filename):
                        raise RuntimeError("ERR BAD_NAME")
                    data = file_item["data"]
                    if not isinstance(data, (bytes, bytearray)):
                        raise RuntimeError("invalid file data")
                    remote = join_sd_path(dest_dir, filename)
                    sd.put_bytes(remote, bytes(data))
                    status, body, ct = _json_bytes({"ok": True, "path": remote, "size": len(data)})
                    self._send(status, body, ct)
                    return

                status, body, ct = _json_bytes({"error": "not found"}, 404)
                self._send(status, body, ct)
            except Exception as e:  # noqa: BLE001
                code = 400 if str(e) == "not connected" else 500
                status, body, ct = _json_bytes({"error": str(e)}, code)
                self._send(status, body, ct)

    return Handler


def cmd_web(host: str, port: int, open_browser: bool) -> None:
    session = SdSession()
    handler = make_handler(session)
    server = ThreadingHTTPServer((host, port), handler)
    url = f"http://{host}:{port}/"
    print(f"TŒRN SD explorer at {url}")
    print("Web Serial: Connect in Chrome/Edge (device: Menu → ETC → SD). Ctrl+C to stop.")
    if open_browser:
        try:
            webbrowser.open(url)
        except Exception:  # noqa: BLE001
            pass
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping.")
    finally:
        session.disconnect()
        server.server_close()


def main() -> int:
    ap = argparse.ArgumentParser(description="TŒRN SD card access via Menu → ETC → SD serial protocol")
    ap.add_argument("-p", "--port", help="serial port (e.g. /dev/cu.usbmodemXXXX)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("ports", help="list serial ports")
    sub.add_parser("ping", help="check device SD file server (ETC → SD)")

    p_list = sub.add_parser("list", help="list directory on SD")
    p_list.add_argument("path", nargs="?", default="/")

    p_rm = sub.add_parser("rm", help="delete file or folder (recursive)")
    p_rm.add_argument("path")

    p_mkdir = sub.add_parser("mkdir", help="create directory")
    p_mkdir.add_argument("path")

    p_ren = sub.add_parser("rename", help="rename file or folder")
    p_ren.add_argument("src")
    p_ren.add_argument("dst")

    p_put = sub.add_parser("put", help="upload local file to SD")
    p_put.add_argument("local")
    p_put.add_argument("remote")

    p_get = sub.add_parser("get", help="download SD file to local path")
    p_get.add_argument("remote")
    p_get.add_argument("local")

    p_web = sub.add_parser("web", help="open local web file explorer")
    p_web.add_argument("--host", default=WEB_HOST)
    p_web.add_argument("--http-port", type=int, default=WEB_PORT, dest="http_port")
    p_web.add_argument("--no-browser", action="store_true", help="do not open a browser")

    args = ap.parse_args()

    if args.cmd == "ports":
        cmd_ports()
        return 0

    if args.cmd == "web":
        try:
            cmd_web(args.host, args.http_port, open_browser=not args.no_browser)
        except (RuntimeError, TimeoutError, OSError) as e:
            print(f"error: {e}", file=sys.stderr)
            return 1
        return 0

    if not args.port:
        print("error: --port is required (try: toern_sd.py ports)", file=sys.stderr)
        return 2

    sd = ToernSd(args.port)
    try:
        if args.cmd == "ping":
            cmd_ping(sd)
        elif args.cmd == "list":
            cmd_list(sd, args.path)
        elif args.cmd == "rm":
            cmd_rm(sd, args.path)
        elif args.cmd == "mkdir":
            cmd_mkdir(sd, args.path)
        elif args.cmd == "rename":
            cmd_rename(sd, args.src, args.dst)
        elif args.cmd == "put":
            cmd_put(sd, Path(args.local), args.remote)
        elif args.cmd == "get":
            cmd_get(sd, args.remote, Path(args.local))
        else:
            print(f"unknown command: {args.cmd}", file=sys.stderr)
            return 2
    except (RuntimeError, TimeoutError, OSError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    finally:
        sd.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
