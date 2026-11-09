// GPL-3.0 — line-oriented Web Serial transport for setup and diagnostics.
import { fromHex, crc32, toHex } from './setup-settings.js';
export const wait = ms => new Promise(resolve => setTimeout(resolve, ms));
export class SetupConnection {
  constructor(serial = globalThis.navigator?.serial, { prefix = 'CONFIG', replyFilter = () => true, timeoutMessage = 'No settings reply. Install the latest Beta or BETA-MODULR above, wait for TŒRN to restart, then reconnect.' } = {}) { this.prefix = prefix; this.replyFilter = replyFilter; this.timeoutMessage = timeoutMessage; this.onMessage = () => {}; this.serial = serial; this.port = null; this.pending = null; this.onDisconnect = () => {}; }
  async connect() {
    if (!this.serial) throw new Error('Use Chrome or Edge on a desktop computer to connect over USB.');
    if (this.port) return;
    const port = await this.serial.requestPort({ filters: [{ usbVendorId: 0x16c0 }] });
    try {
      await port.open({ baudRate: 115200 });
      this.port = port;
      this.reader = port.readable.getReader();
      this.disconnected = () => { this.rejectPending(new Error('TŒRN disconnected. Reconnect to continue.')); void this.close().then(() => this.onDisconnect()); };
      port.addEventListener('disconnect', this.disconnected);
      this.readTask = this.read();
      await wait(250);
    } catch (error) { await this.close(); throw error; }
  }
  rejectPending(error) {
    if (!this.pending) return;
    clearTimeout(this.pending.timer);
    this.pending.reject(error); this.pending = null;
  }
  async read() {
    const decoder = new TextDecoder(); let buffer = '';
    try {
      while (true) {
        const { value, done } = await this.reader.read();
        if (done) break;
        buffer += decoder.decode(value, { stream: true });
        if (buffer.length > 16384) throw new Error('Unexpected serial output. Disconnect and try again.');
        let end;
        while ((end = buffer.indexOf('\n')) >= 0) {
          const line = buffer.slice(0, end).trim(); buffer = buffer.slice(end + 1);
          const prefix = `${this.prefix} `;
          if (!line.startsWith(prefix)) continue;
          try {
            const payload = JSON.parse(line.slice(prefix.length));
            this.onMessage(payload);
            const pending = this.pending;
            if (pending && this.replyFilter(payload)) {
              this.pending = null; clearTimeout(pending.timer); pending.resolve(payload);
            }
          } catch { this.rejectPending(new Error('Invalid reply from device.')); }
        }
      }
      this.rejectPending(new Error('The USB connection closed. Reconnect to continue.'));
    } catch (error) { this.rejectPending(error); }
    finally {
      if (this.port) {
        const port = this.port;
        queueMicrotask(() => { if (this.port === port) void this.close().then(() => this.onDisconnect()); });
      }
    }
  }
  async request(command, timeout = 8000) {
    if (!this.port?.writable) throw new Error('Connect your TŒRN first.');
    if (this.pending) throw new Error('Another device operation is still running.');
    let response;
    const reply = new Promise((resolve, reject) => {
      const timer = setTimeout(() => { this.pending = null; reject(new Error(this.timeoutMessage)); }, timeout);
      this.pending = { resolve, reject, timer };
    });
    // Attach rejection handling immediately, including while USB write is pending.
    const result = reply.then(value => ({ value }), error => ({ error }));
    let writer;
    try {
      writer = this.port.writable.getWriter();
      await writer.write(new TextEncoder().encode(`${this.prefix} ${command}\n`));
    } catch (error) { this.rejectPending(error); }
    finally { writer?.releaseLock(); }
    response = await result;
    if (response.error) throw response.error;
    if (!response.value.ok) {
      const messages = { BUSY: 'The device is still settling. Wait a moment and try again.', RECORDING: 'Finish recording on TŒRN, then apply your setup again.', CHANGED: 'A setting changed on TŒRN during setup. Keep the knobs still and try again.', VALUES: 'The device rejected these settings. Install the latest Beta and reconnect.', LAYOUT: 'The display layout changed. Reconnect and try again.', VERIFY: 'The device could not verify its saved settings. Reconnect and try again.' };
      const error = new Error(messages[response.value.error] || `Device settings error: ${response.value.error}`);
      error.code = response.value.error; throw error;
    }
    return response.value;
  }
  async get() {
    const state = await this.request('GET');
    if (state.protocol !== 1 || !/^[\da-f]{8}$/i.test(state.crc)) throw new Error('This firmware does not support guided setup. Install the latest Beta above.');
    const bytes = fromHex(state.hex);
    if (crc32(bytes) !== state.crc.toUpperCase()) throw new Error('Settings check failed. Reconnect and try again.');
    return { ...state, bytes };
  }
  async apply(buildSettings) {
    await this.request('STOP');
    // STOP releases envelopes; the device refuses flash writes until audio is quiet.
    await wait(350);
    for (let attempt = 0; attempt < 16; ++attempt) {
      const before = await this.get();
      const bytes = buildSettings(before.bytes);
      try {
        const result = await this.request(`SET ${before.crc} ${toHex(bytes)}`, 12000);
        const after = await this.get();
        if (after.hex.toUpperCase() !== toHex(bytes) || result.crc?.toUpperCase() !== crc32(bytes)) throw new Error('The saved settings do not match. Reconnect and apply again.');
        return { backup: result.backup, firmware: after.firmware };
      } catch (error) {
        if (error.code !== 'BUSY' || attempt === 15) throw error;
        await wait(250);
      }
    }
  }
  async close() {
    const port = this.port;
    this.port = null;
    this.rejectPending(new Error('USB connection closed.'));
    if (!port) return;
    port.removeEventListener('disconnect', this.disconnected);
    try { await this.reader?.cancel(); await this.readTask; } catch {}
    try { this.reader?.releaseLock(); } catch {}
    this.reader = null;
    try { await port.close(); } catch {}
  }
}
