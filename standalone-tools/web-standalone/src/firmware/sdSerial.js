/** Web Serial client for TŒRN Menu → ETC → SD (same protocol as sd-tool-standalone). */

export const BAUD = 115200;
export const TEENSY_FILTERS = [{ usbVendorId: 0x16c0 }];

const IO_TIMEOUT_MS = 60000;
const XFER_IDLE_MS = 8000;
const LIST_LINE_TIMEOUT_MS = 12000;
const KEEPALIVE_MS = 4000;
const HANDSHAKE_MS = 45000;

function sdLog(msg, level) {
  const line = `[sd] ${msg}`;
  if (level === "err") console.error(line);
  else if (level === "ok") console.log(line);
  else console.debug(line);
}

const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
    t[n] = c >>> 0;
  }
  return t;
})();

export function crc32Hex(data) {
  let c = 0xffffffff;
  for (let i = 0; i < data.length; i++) c = CRC_TABLE[(c ^ data[i]) & 0xff] ^ (c >>> 8);
  return ((c ^ 0xffffffff) >>> 0).toString(16).toUpperCase().padStart(8, "0");
}

function concatBytes(a, b) {
  const out = new Uint8Array(a.length + b.length);
  out.set(a, 0);
  out.set(b, a.length);
  return out;
}

export function isHiddenName(name) {
  return !!name && name.startsWith(".");
}

export function parentSdPath(path) {
  path = (path || "/").replace(/\\/g, "/").replace(/\/+$/, "");
  if (!path || path === "/") return "/";
  const idx = path.lastIndexOf("/");
  if (idx <= 0) return "/";
  return path.slice(0, idx) || "/";
}

export function joinSdPath(parent, name) {
  parent = (parent || "/").replace(/\\/g, "/");
  name = String(name || "").replace(/^\/+|\/+$/g, "").replace(/\\/g, "/");
  if (parent === "" || parent === "/") return "/" + name;
  return parent.replace(/\/+$/, "") + "/" + name;
}

// --- Web Serial SD client ----------------------------------------------------

export class ToernSdSerial {
  constructor() {
    this.port = null;
    this.reader = null;
    this.writer = null;
    this._rxBuf = new Uint8Array(0);
    this._rxLen = 0;
    this._chain = Promise.resolve();
    this._dataWait = null;
    this._keepaliveTimer = null;
    this._closed = false;
    this._connecting = false;
    this._opBusy = 0;
    this._readLoopRunning = false;
    this._readGen = 0;
    this._disconnectBound = null;
    this.onDisconnect = null;
  }

  get connected() {
    return !!this.port && !this._closed;
  }

  _enqueue(fn) {
    const run = this._chain.then(
      () => this._runOp(fn),
      () => this._runOp(fn),
    );
    this._chain = run.catch(() => {});
    return run;
  }

  async _runOp(fn) {
    this._opBusy++;
    try {
      return await fn();
    } finally {
      this._opBusy--;
    }
  }

  async _sleep(ms) {
    await new Promise((r) => setTimeout(r, ms));
  }

  _clearRx() {
    this._rxBuf = new Uint8Array(0);
    this._rxLen = 0;
  }

  _wakeWaiters(errMsg) {
    if (!this._dataWait) return;
    const wake = this._dataWait;
    this._dataWait = null;
    try { wake(errMsg || "not connected"); } catch (_) {}
  }

  async _safeReleaseStreams() {
    const reader = this.reader;
    const writer = this.writer;
    const port = this.port;
    this.reader = null;
    this.writer = null;
    this.port = null;
    this._clearRx();
    this._wakeWaiters("not connected");
    if (reader) {
      try { await reader.cancel(); } catch (_) {}
      try { reader.releaseLock(); } catch (_) {}
    }
    if (writer) {
      try { await writer.abort(); } catch (_) {}
      try { writer.releaseLock(); } catch (_) {}
    }
    if (port) {
      try { await port.close(); } catch (_) {}
    }
    // Chrome needs a beat before the same (or re-enumerated) port can open again.
    await this._sleep(250);
  }

  async _releasePort() {
    this._stopKeepalive();
    this._unbindDisconnect();
    await this._safeReleaseStreams();
  }

  _unbindDisconnect() {
    if (this.port && this._disconnectBound) {
      try { this.port.removeEventListener("disconnect", this._disconnectBound); } catch (_) {}
    }
    this._disconnectBound = null;
  }

  async _pickPort(preferred) {
    if (preferred) {
      try {
        // Prefer a live port object from getPorts() over a stale handle after USB re-enum.
        const granted = await navigator.serial.getPorts();
        for (const p of granted) {
          if (p === preferred) return p;
        }
        if (granted.length === 1) return granted[0];
        if (granted.length > 1) return preferred;
        return preferred;
      } catch (_) {
        return preferred;
      }
    }
    const granted = await navigator.serial.getPorts();
    if (granted.length) return granted[0];
    return null;
  }

  async _waitForReappear(timeoutMs = 4000) {
    const now = await navigator.serial.getPorts();
    if (now.length) return now[0];
    sdLog("waiting for USB re-enumerate…");
    return new Promise((resolve) => {
      let done = false;
      const finish = (port) => {
        if (done) return;
        done = true;
        clearTimeout(timer);
        try { navigator.serial.removeEventListener("connect", onConnect); } catch (_) {}
        resolve(port || null);
      };
      const onConnect = (ev) => finish(ev.target);
      const timer = setTimeout(async () => {
        const ports = await navigator.serial.getPorts().catch(() => []);
        finish(ports[0] || null);
      }, timeoutMs);
      navigator.serial.addEventListener("connect", onConnect);
    });
  }

  async _openPort(port) {
    sdLog("openPort: closing existing if needed");
    if (port.readable || port.writable) {
      try { await port.close(); } catch (_) {}
      await this._sleep(400);
    }
    sdLog("openPort: port.open()");
    await port.open({ baudRate: BAUD, bufferSize: 256 * 1024 });
    this.port = port;
    this._closed = false;
    this.reader = port.readable.getReader();
    this.writer = port.writable.getWriter();
    this._clearRx();
    this._readGen += 1;
    const gen = this._readGen;
    this._startReadLoop(gen);
    // Teensy often reboots when the port opens — give USB + sketch time to come back.
    sdLog("openPort: settling 2200ms (USB blip / possible reboot)");
    await this._sleep(2200);
    if (this._rxLen) {
      const text = new TextDecoder("utf-8", { fatal: false }).decode(this._rxBuf.subarray(0, Math.min(120, this._rxLen)));
      sdLog("openPort: RX during settle (" + this._rxLen + "B): " + JSON.stringify(text));
    } else {
      sdLog("openPort: no RX during settle — open Menu → ETC → SD if needed");
    }
  }

  // Wait for SD banner or a successful PING. Opening the port can reboot the Teensy
  // and drop out of Menu → ETC → SD; keep pinging so the user can re-enter.
  async _handshakeSd(onStatus, onNeedSd) {
    const deadline = performance.now() + HANDSHAKE_MS;
    let n = 0;
    let sawNeedSd = false;
    const markNeedSd = () => {
      sawNeedSd = true;
      if (onNeedSd) onNeedSd(true);
    };
    const clearNeedSd = () => {
      if (onNeedSd) onNeedSd(false);
    };

    const tryConsumeLine = async (timeoutMs) => {
      try {
        const line = (await this._readLine(timeoutMs)).trim();
        sdLog("handshake ← " + JSON.stringify(line));
        return line;
      } catch (_) {
        return null;
      }
    };

    // Consume banner already in RX (do not ignore it / do not glue with next PING).
    while (this._findLineEnd() >= 0) {
      const existing = await tryConsumeLine(500);
      if (existing && existing.startsWith("OK")) {
        clearNeedSd();
        return existing;
      }
      if (existing && /NEED_SD/i.test(existing)) markNeedSd();
      if (!existing) break;
    }

    while (performance.now() < deadline) {
      if (this._closed) throw new Error("not connected");
      n += 1;
      const leftSec = Math.max(1, Math.ceil((deadline - performance.now()) / 1000));
      const hint = sawNeedSd
        ? `Device alive — open Menu → ETC → SD (${leftSec}s)`
        : `Menu → ETC → SD · PING ${n} (${leftSec}s)`;
      if (onStatus) onStatus(hint);
      sdLog(`handshake PING #${n}`);
      try {
        await this._write("PING\n");
      } catch (err) {
        sdLog("handshake write failed: " + (err && err.message || err), "err");
        throw err;
      }
      const line = await tryConsumeLine(1800);
      if (line && line.startsWith("OK")) {
        clearNeedSd();
        return line;
      }
      if (line && /NEED_SD/i.test(line)) {
        markNeedSd();
        sdLog("device says NEED_SD — re-enter Menu → ETC → SD", "err");
      }
      await this._sleep(350);
    }
    throw new Error(
      sawNeedSd
        ? "Device is online but not on SD page — open Menu → ETC → SD (WAIT), then Connect again if needed."
        : "No serial reply — reflash firmware with SD serial, then Menu → ETC → SD. Opening USB can reboot Teensy; re-enter SD after Connect.",
    );
  }

  async connect(existingPort, onStatus, onNeedSd) {
    if (!navigator.serial) {
      throw new Error("Web Serial not supported — use Chrome or Edge");
    }
    await this.close();
    this._closed = false;
    this._connecting = true;
    this._clearRx();
    this._chain = Promise.resolve();

    let port = existingPort || null;
    if (!port) {
      try {
        port = await navigator.serial.requestPort({ filters: TEENSY_FILTERS });
      } catch (err) {
        if (err && err.name === "NotFoundError") {
          port = await navigator.serial.requestPort();
        } else {
          this._connecting = false;
          throw err;
        }
      }
    }

    let lastErr = null;
    // At most 2 opens: first try + one reopen if USB re-enumerated. Do NOT reopen
    // on ping timeout (that reboots Teensy again and kicks you out of SD).
    for (let attempt = 1; attempt <= 2; attempt++) {
      try {
        sdLog(`connect: open ${attempt}/2`);
        if (onStatus) onStatus(`Opening USB… (${attempt}/2)`);
        if (attempt > 1) {
          await this._sleep(1500);
          const fresh = await this._waitForReappear(4000);
          if (fresh) port = fresh;
          else port = await this._pickPort(port);
          if (!port) throw new Error("serial port not available after disconnect");
        }
        await this._openPort(port);
        if (onStatus) onStatus("Waiting for Menu → ETC → SD…");
        const msg = await this._handshakeSd(onStatus, onNeedSd);
        sdLog("connect: ready → " + msg, "ok");
        if (onNeedSd) onNeedSd(false);
        this._connecting = false;
        this._unbindDisconnect();
        this._disconnectBound = () => {
          if (this._connecting) {
            sdLog("disconnect event ignored (still connecting)");
            return;
          }
          sdLog("disconnect event → device lost", "err");
          this._handleLost("Disconnected (device lost)");
        };
        port.addEventListener("disconnect", this._disconnectBound);
        this._startKeepalive();
        return msg;
      } catch (err) {
        lastErr = err;
        const msg = String(err && err.message || err);
        sdLog(`connect open ${attempt} failed: ${msg}`, "err");
        this._readGen += 1;
        try { await this._releasePort(); } catch (_) {}

        // Only reopen if the port itself died — not when SD menu is missing.
        const usbDied = (err && err.name === "NetworkError")
          || /device has been lost|Failed to open|not available|not connected/i.test(msg);
        const needSdOnly = /NEED_SD|not on SD|No serial reply|No SD response/i.test(msg);
        if (needSdOnly || !usbDied || attempt === 2) break;
        sdLog("USB lost during open — will reopen once");
        await this._sleep(1800);
      }
    }
    this._connecting = false;
    this._closed = true;
    throw lastErr || new Error("connect failed");
  }

  _startReadLoop(gen) {
    this._readLoopRunning = true;
    (async () => {
      try {
        while (!this._closed && this._readGen === gen && this.reader) {
          const reader = this.reader;
          const { value, done } = await reader.read();
          if (this._readGen !== gen || this._closed) break;
          if (done) {
            sdLog("readLoop: stream done", "err");
            break;
          }
          if (value && value.length) {
            this._pushRx(new Uint8Array(value));
          }
        }
        if (!this._closed && !this._connecting && this._readGen === gen) {
          sdLog("readLoop ended → treat as lost", "err");
          this._handleLost("Disconnected (device lost)");
        }
      } catch (err) {
        sdLog(`readLoop error: ${err && err.message || err}`, "err");
        if (!this._closed && !this._connecting && this._readGen === gen) {
          this._handleLost("Disconnected (device lost)");
        }
      } finally {
        if (this._readGen === gen) this._readLoopRunning = false;
      }
    })().catch((err) => {
      sdLog(`readLoop unhandled: ${err && err.message || err}`, "err");
    });
  }

  _pushRx(chunk) {
    if (!chunk || !chunk.length || this._closed) return;
    // Always copy — Web Serial may reuse the underlying buffer.
    const add = chunk instanceof Uint8Array ? chunk : new Uint8Array(chunk);
    if (this._rxLen === 0) {
      this._rxBuf = add.slice();
      this._rxLen = add.length;
    } else {
      const next = new Uint8Array(this._rxLen + add.length);
      next.set(this._rxBuf.subarray(0, this._rxLen), 0);
      next.set(add, this._rxLen);
      this._rxBuf = next;
      this._rxLen += add.length;
    }
    if (this._dataWait) {
      const wake = this._dataWait;
      this._dataWait = null;
      wake();
    }
  }

  // Wait until RX has at least minLen bytes (installs waiter before re-check to avoid races).
  _waitUntilRx(minLen, timeoutMs) {
    if (this._closed) return Promise.reject(new Error("not connected"));
    if (this._rxLen >= minLen) return Promise.resolve();
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        if (this._dataWait === wake) this._dataWait = null;
        if (this._closed) reject(new Error("not connected"));
        else if (this._rxLen >= minLen) resolve();
        else reject(new Error("timeout waiting for data from device"));
      }, Math.max(1, timeoutMs));
      const wake = (errMsg) => {
        if (this._closed || errMsg) {
          clearTimeout(timer);
          if (this._dataWait === wake) this._dataWait = null;
          reject(new Error(errMsg || "not connected"));
          return;
        }
        if (this._rxLen < minLen) {
          this._dataWait = wake;
          return;
        }
        clearTimeout(timer);
        if (this._dataWait === wake) this._dataWait = null;
        resolve();
      };
      this._dataWait = wake;
      if (this._closed) {
        clearTimeout(timer);
        if (this._dataWait === wake) this._dataWait = null;
        reject(new Error("not connected"));
      } else if (this._rxLen >= minLen) {
        clearTimeout(timer);
        if (this._dataWait === wake) this._dataWait = null;
        resolve();
      }
    });
  }

  _takeRx(n) {
    if (n <= 0) return new Uint8Array(0);
    if (n > this._rxLen) throw new Error("internal: short RX buffer");
    const out = this._rxBuf.slice(0, n); // copy
    const remain = this._rxLen - n;
    if (remain <= 0) {
      this._clearRx();
    } else {
      this._rxBuf = this._rxBuf.slice(n, this._rxLen);
      this._rxLen = remain;
    }
    return out;
  }

  // Index of first line ending (\n, or bare \r not followed by \n). -1 if none yet.
  _findLineEnd() {
    for (let i = 0; i < this._rxLen; i++) {
      const b = this._rxBuf[i];
      if (b === 0x0a) return i;
      if (b === 0x0d) {
        if (i + 1 < this._rxLen) return this._rxBuf[i + 1] === 0x0a ? i + 1 : i;
        // CR at end of buffer — wait for possible LF
        return -1;
      }
    }
    return -1;
  }

  async close() {
    sdLog("close()");
    this._stopKeepalive();
    this._unbindDisconnect();
    this._closed = true;
    this._readGen = (this._readGen || 0) + 1;
    await this._safeReleaseStreams();
    await new Promise((r) => setTimeout(r, 0));
  }

  _handleLost(msg) {
    if (this._closed) return;
    sdLog("_handleLost: " + (msg || "Disconnected"), "err");
    const cb = this.onDisconnect;
    this._closed = true;
    this._readGen = (this._readGen || 0) + 1;
    this._stopKeepalive();
    this._unbindDisconnect();
    this._wakeWaiters("not connected");
    const reader = this.reader;
    const writer = this.writer;
    const port = this.port;
    this.reader = null;
    this.writer = null;
    this.port = null;
    this._clearRx();
    // Abort pending read/write so LIST can't hang forever after USB drop.
    if (reader) {
      try { reader.cancel().catch(() => {}); } catch (_) {}
      try { reader.releaseLock(); } catch (_) {}
    }
    if (writer) {
      try { writer.abort().catch(() => {}); } catch (_) {}
      try { writer.releaseLock(); } catch (_) {}
    }
    if (port) {
      try { port.close().catch(() => {}); } catch (_) {}
    }
    if (cb) {
      try { cb(msg || "Disconnected"); } catch (_) {}
    }
  }

  _startKeepalive() {
    this._stopKeepalive();
    sdLog("keepalive started");
    let fails = 0;
    this._keepaliveTimer = setInterval(() => {
      if (!this.connected || this._opBusy > 0 || state.busy) return;
      this.ping().then(() => {
        fails = 0;
      }).catch(async (err) => {
        const msg = String(err && err.message || err);
        sdLog("keepalive ping failed (" + (fails + 1) + "): " + msg, "err");
        // After a failed GET the Teensy may still be waiting for ACK — unblock it.
        if (/ERR ACK|unexpected response/i.test(msg)) {
          try {
            for (let i = 0; i < 3; i++) await this._write("ACK\n");
            this._clearRx();
            await this.ping();
            fails = 0;
            sdLog("keepalive recovered after ACK", "ok");
            return;
          } catch (_) {}
        }
        fails += 1;
        if (fails >= 2) this._handleLost("Disconnected (device lost)");
      });
    }, KEEPALIVE_MS);
  }

  _stopKeepalive() {
    if (this._keepaliveTimer) {
      clearInterval(this._keepaliveTimer);
      this._keepaliveTimer = null;
    }
  }

  async _readLine(timeoutMs = IO_TIMEOUT_MS) {
    const deadline = performance.now() + timeoutMs;
    while (true) {
      if (this._closed) throw new Error("not connected");
      const nl = this._findLineEnd();
      if (nl >= 0) {
        let lineBytes = this._takeRx(nl + 1);
        // Trim one trailing LF and/or CR.
        if (lineBytes.length && lineBytes[lineBytes.length - 1] === 0x0a) {
          lineBytes = lineBytes.subarray(0, lineBytes.length - 1);
        }
        if (lineBytes.length && lineBytes[lineBytes.length - 1] === 0x0d) {
          lineBytes = lineBytes.subarray(0, lineBytes.length - 1);
        }
        let line = new TextDecoder("utf-8", { fatal: false }).decode(lineBytes);
        // Defense: never return a multi-line blob if buffer math ever glitches.
        if (/[\r\n]/.test(line)) {
          const parts = line.split(/\r\n|\n|\r/).filter((p, i, arr) => p.length || i < arr.length - 1);
          line = parts[0] || "";
          const rest = parts.slice(1).filter((p) => p.length);
          if (rest.length) {
            const enc = new TextEncoder();
            const back = rest.map((p) => enc.encode(p + "\n"));
            let total = 0;
            for (const b of back) total += b.length;
            const merged = new Uint8Array(total + this._rxLen);
            let off = 0;
            for (const b of back) { merged.set(b, off); off += b.length; }
            if (this._rxLen) merged.set(this._rxBuf.subarray(0, this._rxLen), off);
            this._rxBuf = merged;
            this._rxLen = merged.length;
          }
        }
        return line;
      }
      const left = deadline - performance.now();
      if (left <= 0) throw new Error("timeout waiting for line from device");
      await this._waitUntilRx(this._rxLen + 1, left);
    }
  }

  async _readExact(n, idleTimeoutMs = XFER_IDLE_MS, onProgress = null) {
    if (n <= 0) {
      if (onProgress) onProgress(0, 0);
      return new Uint8Array(0);
    }
    const started = performance.now();
    let lastProgress = 0;
    while (this._rxLen < n) {
      if (this._closed) throw new Error("not connected");
      try {
        await this._waitUntilRx(this._rxLen + 1, idleTimeoutMs);
      } catch (err) {
        const got = this._rxLen;
        throw new Error(
          `transfer stalled at ${got}/${n} bytes (${Math.round((got / n) * 100)}%) after ${Math.round((performance.now() - started) / 1000)}s — ${err.message || err}`,
        );
      }
      if (onProgress && this._rxLen !== lastProgress) {
        lastProgress = this._rxLen;
        onProgress(Math.min(this._rxLen, n), n);
      }
    }
    const out = this._takeRx(n);
    if (onProgress) onProgress(n, n);
    return out;
  }

  async _write(data) {
    if (!this.writer || this._closed) throw new Error("not connected");
    const bytes = typeof data === "string" ? new TextEncoder().encode(data) : data;
    try {
      await this.writer.write(bytes);
    } catch (err) {
      sdLog("write failed: " + (err && err.message || err), "err");
      throw err;
    }
  }

  _expectOk(line, prefix = "OK") {
    if (line.startsWith("ERR")) throw new Error(line);
    if (!line.startsWith(prefix)) throw new Error("unexpected response: " + line);
    return line;
  }

  ping() {
    return this._enqueue(async () => {
      if (this._rxLen) {
        sdLog(`ping: draining ${this._rxLen} stale RX bytes`);
        this._clearRx();
      }
      await this._write("PING\n");
      return this._expectOk(await this._readLine(8000));
    });
  }

  listDir(path = "/", hideDot = true) {
    return this._enqueue(async () => {
      if (this._rxLen) {
        sdLog(`listDir: draining ${this._rxLen} stale RX bytes`);
        this._clearRx();
      }
      const cmd = "LIST " + path + "\n";
      sdLog("listDir → " + JSON.stringify(cmd.trim()));
      await this._write(cmd);
      const entries = [];
      let lines = 0;
      while (true) {
        if (this._closed) throw new Error("not connected");
        const line = (await this._readLine(LIST_LINE_TIMEOUT_MS)).trim();
        lines++;
        if (lines <= 8 || line === "OK" || line.startsWith("ERR")) {
          sdLog(`listDir ← [${lines}] ${JSON.stringify(line)}`);
        }
        if (line === "OK") break;
        if (line.startsWith("ERR")) throw new Error(line);
        if (line.startsWith("OK ")) {
          sdLog("listDir: skip banner/ping line");
          continue;
        }
        if (line.startsWith("D ")) {
          const name = line.slice(2).trim();
          if (!name) continue;
          if (hideDot && isHiddenName(name)) continue;
          entries.push({ type: "dir", name, size: null, duration_ms: null });
        } else if (line.startsWith("F ")) {
          const rest = line.slice(2).trim();
          const parts = rest.split(/\s+/);
          if (parts.length < 2) continue;
          const size = parseInt(parts[0], 10);
          let duration_ms = null;
          let name;
          if (parts.length >= 3) {
            const dur = parseInt(parts[1], 10);
            name = parts.slice(2).join(" ");
            duration_ms = Number.isFinite(dur) && dur >= 0 ? dur : null;
          } else {
            name = parts.slice(1).join(" ");
          }
          if (!name) continue;
          if (hideDot && isHiddenName(name)) continue;
          entries.push({ type: "file", name, size, duration_ms });
        } else if (line) {
          sdLog("listDir: ignore unknown line " + JSON.stringify(line));
        }
      }
      sdLog(`listDir done: ${entries.length} entries (${lines} lines)`, "ok");
      entries.sort((a, b) => {
        if (a.type !== b.type) return a.type === "dir" ? -1 : 1;
        return a.name.localeCompare(b.name, undefined, { sensitivity: "base" });
      });
      return entries;
    });
  }

  mkdir(path) {
    return this._enqueue(async () => {
      if (this._rxLen) this._clearRx();
      sdLog("mkdir → " + path);
      await this._write("MKDIR " + path + "\n");
      const line = await this._readLine(15000);
      sdLog("mkdir ← " + JSON.stringify(line));
      this._expectOk(line);
    });
  }

  rm(path) {
    return this._enqueue(async () => {
      if (this._rxLen) this._clearRx();
      sdLog("rm → " + path);
      await this._write("RM " + path + "\n");
      const line = await this._readLine(15000);
      sdLog("rm ← " + JSON.stringify(line));
      this._expectOk(line);
    });
  }

  rename(src, dst) {
    return this._enqueue(async () => {
      if (this._rxLen) this._clearRx();
      sdLog("mv → " + src + " " + dst);
      await this._write("MV " + src + " " + dst + "\n");
      const line = await this._readLine(15000);
      sdLog("mv ← " + JSON.stringify(line));
      this._expectOk(line);
    });
  }

  putBytes(remote, data, onProgress) {
    return this._enqueue(async () => {
      const bytes = data instanceof Uint8Array ? data : new Uint8Array(data);
      const csum = crc32Hex(bytes);
      await this._write(`PUT ${remote} ${bytes.length} ${csum}\n`);
      const ready = await this._readLine();
      if (ready !== "READY") {
        throw new Error(ready.startsWith("ERR") ? ready : "expected READY, got: " + ready);
      }
      // USB CDC ignores baud rate — must wait for ACK or Teensy RX overruns and hard-faults.
      const block = 8192;
      for (let i = 0; i < bytes.length; i += block) {
        const end = Math.min(i + block, bytes.length);
        await this._write(bytes.subarray(i, end));
        const ack = await this._readLine(IO_TIMEOUT_MS);
        if (ack !== "ACK") {
          throw new Error(ack.startsWith("ERR") ? ack : "expected ACK, got: " + ack);
        }
        if (onProgress) onProgress(end, bytes.length);
      }
      this._expectOk(await this._readLine(IO_TIMEOUT_MS));
    });
  }

  getBytes(remote, onProgress) {
    return this._enqueue(async () => {
      const resync = async () => {
        for (let i = 0; i < 4; i++) {
          try { await this._write("ACK\n"); } catch (_) {}
        }
        await this._sleep(50);
        this._clearRx();
        try {
          await this._write("PING\n");
          const line = await this._readLine(3000);
          sdLog("resync ← " + JSON.stringify(line));
          if (/ERR ACK/i.test(line)) {
            this._clearRx();
            await this._write("PING\n");
            await this._readLine(3000);
          }
        } catch (err) {
          sdLog("resync ping: " + (err && err.message || err), "err");
          this._clearRx();
        }
      };

      try {
        if (this._rxLen) {
          sdLog(`getBytes: draining ${this._rxLen} stale RX bytes`);
          this._clearRx();
        }
        sdLog("GET → " + remote);
        await this._write("GET " + remote + "\n");
        const header = (await this._readLine(IO_TIMEOUT_MS)).trim();
        sdLog("GET header ← " + JSON.stringify(header));
        if (header.startsWith("ERR")) throw new Error(header);
        // New: "OK <size>" then data then "CRC <hex>"
        // Legacy: "OK <size> <crc>" (reject — needs matching firmware)
        const mNew = /^OK\s+(\d+)$/.exec(header);
        const mOld = /^OK\s+(\d+)\s+([0-9A-Fa-f]{1,8})$/.exec(header);
        if (!mNew && !mOld) throw new Error("unexpected response: " + header);
        if (mOld && !mNew) {
          throw new Error("Device firmware too old for GET — reflash with ACK+CRC trailer support");
        }
        const size = parseInt(mNew[1], 10);

        await this._write("ACK\n");

        const out = new Uint8Array(size);
        const block = 512;
        let off = 0;
        while (off < size) {
          const n = Math.min(block, size - off);
          const piece = await this._readExact(n, XFER_IDLE_MS, (got) => {
            if (onProgress) onProgress(off + got, size);
          });
          out.set(piece, off);
          off += n;
          await this._write("ACK\n");
          if (onProgress) onProgress(off, size);
          // Let the browser service Web Serial / audio between blocks.
          await this._sleep(0);
        }

        const crcLine = (await this._readLine(IO_TIMEOUT_MS)).trim();
        sdLog("GET crc ← " + JSON.stringify(crcLine));
        const cm = /^CRC\s+([0-9A-Fa-f]{1,8})$/i.exec(crcLine);
        if (!cm) throw new Error("expected CRC trailer, got: " + crcLine);
        const expect = cm[1].toUpperCase().padStart(8, "0");
        const got = crc32Hex(out);
        if (got !== expect) throw new Error(`CRC mismatch: got ${got} want ${expect}`);
        sdLog(`GET ok: ${size} bytes`, "ok");
        return out;
      } catch (err) {
        sdLog("GET failed — resync: " + (err && err.message || err), "err");
        try { await resync(); } catch (_) {}
        throw err;
      }
    });
  }

  async rmRecursive(path) {
    path = path.replace(/\\/g, "/");
    if (!path || path === "/") throw new Error("ERR REFUSE_ROOT");
    const parent = parentSdPath(path);
    const name = path.replace(/\/+$/, "").split("/").pop();
    let match = null;
    try {
      const entries = await this.listDir(parent, false);
      match = entries.find((e) => e.name === name) || null;
    } catch (_) {
      match = null;
    }
    if (!match) {
      await this.rm(path);
      return;
    }
    if (match.type === "dir") {
      const children = await this.listDir(path, false);
      for (const child of children) {
        await this.rmRecursive(joinSdPath(path, child.name));
      }
    }
    await this.rm(path);
  }
}

