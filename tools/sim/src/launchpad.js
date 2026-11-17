/**
 * Novation Launchpad → TŒRN sim mixer surface (Web MIDI).
 *
 * Programmer-layout 8×8 grid (bottom row = 1):
 *   columns 1–8 → voices 1–8
 *   row 1       → mute / unmute (below the volume meter)
 *   rows 2–6    → channel volume steps (0 / 4 / 8 / 12 / 16)
 *   row 7       → solo that voice (toggle; another column retargets)
 *   row 8       → select currentVoice
 *
 * Phase 1: browser sim only. Device USB-host path is later.
 */

const VOL_LEVELS = [0, 4, 8, 12, 16];

/** Novation velocity colour indices (programmer / Live palette). */
const COL = {
  off: 0,
  dim: 1,
  white: 3,
  red: 5,
  amber: 9,
  yellow: 13,
  green: 21,
  greenDim: 23,
  blue: 41,
  cyan: 37,
};

const PROGRAMMER_SYSEX = {
  // F0 00 20 29 02 <model> 0E 01 F7
  miniMk3: [0xf0, 0x00, 0x20, 0x29, 0x02, 0x0d, 0x0e, 0x01, 0xf7],
  x: [0xf0, 0x00, 0x20, 0x29, 0x02, 0x0c, 0x0e, 0x01, 0xf7],
  proMk3: [0xf0, 0x00, 0x20, 0x29, 0x02, 0x0e, 0x0e, 0x01, 0xf7],
  mk2: [0xf0, 0x00, 0x20, 0x29, 0x02, 0x18, 0x22, 0x00, 0xf7], // session layout select varies; best-effort
};

export function isLaunchpadName(name = "") {
  const n = String(name);
  return /launchpad/i.test(n) || /\blpminimk3\b/i.test(n) || /\blpx\b/i.test(n) || /\blppro/i.test(n);
}

/** Programmer pad: note = row*10 + col, row 1 bottom … 8 top, col 1–8 left→right. */
export function noteToCell(note) {
  if (!Number.isInteger(note) || note < 11 || note > 88) return null;
  const row = Math.floor(note / 10);
  const col = note % 10;
  if (row < 1 || row > 8 || col < 1 || col > 8) return null;
  return { row, col, voice: col };
}

export function cellToNote(row, col) {
  if (row < 1 || row > 8 || col < 1 || col > 8) return null;
  return row * 10 + col;
}

/** Volume pad rows are 2–6 (mute sits on row 1 below the meter). */
export function volumeForRow(row) {
  if (row < 2 || row > 6) return null;
  return VOL_LEVELS[row - 2];
}

/** Returns lit volume row 2–6, or 0 when volume is silent. */
export function volumeRowFor(vol) {
  const v = Math.max(0, Math.min(16, Math.round(vol)));
  if (v <= 0) return 0;
  let best = 2;
  let bestDist = Infinity;
  for (let i = 0; i < VOL_LEVELS.length; i++) {
    const d = Math.abs(VOL_LEVELS[i] - v);
    if (d < bestDist) {
      bestDist = d;
      best = i + 2;
    }
  }
  return best;
}

function modelKey(name = "") {
  const n = String(name).toLowerCase();
  if (n.includes("mini") && n.includes("mk3")) return "miniMk3";
  if (n.includes("launchpad x") || /\blpx\b/.test(n)) return "x";
  if (n.includes("pro") && n.includes("mk3")) return "proMk3";
  if (n.includes("mk2")) return "mk2";
  if (n.includes("mini")) return "miniMk3";
  return "miniMk3";
}

/**
 * @param {object} device  sim device API
 * @param {{ onStatus?: (text: string) => void }} [opts]
 */
export function createLaunchpadController(device, opts = {}) {
  const onStatus = typeof opts.onStatus === "function" ? opts.onStatus : () => {};
  let access = null;
  let output = null;
  let connectedName = "";
  let raf = 0;
  let lastPaint = "";
  let disposed = false;

  function status(text) {
    onStatus(text);
  }

  function pickOutput(midiAccess) {
    const outs = [...midiAccess.outputs.values()].filter((o) => isLaunchpadName(o.name));
    if (!outs.length) return null;
    // Prefer non-DAW / "MIDI" custom ports when several appear.
    return (
      outs.find((o) => /midi\s*out|programmer|lpminimk3 midi$|standalone/i.test(o.name)) ||
      outs.find((o) => !/daw/i.test(o.name)) ||
      outs[0]
    );
  }

  function enterProgrammer(out) {
    if (!out) return;
    const sysex = PROGRAMMER_SYSEX[modelKey(out.name)];
    try {
      out.send(sysex);
    } catch {
      /* sysex may be blocked */
    }
  }

  function light(note, color) {
    if (!output || note == null) return;
    try {
      output.send([0x90, note, color & 0x7f]);
    } catch {
      /* port gone */
    }
  }

  function clearGrid() {
    for (let row = 1; row <= 8; row++) {
      for (let col = 1; col <= 8; col++) light(cellToNote(row, col), COL.off);
    }
  }

  function paint() {
    if (!output || disposed) return;
    const selected = device.voice?.() ?? 1;
    const soloOn = !!device.soloActive?.();
    const bits = [];
    for (let col = 1; col <= 8; col++) {
      const ch = col;
      const muted = !!device.muted?.(ch);
      const vol = device.channelVolume?.(ch) ?? 16;
      // mute below volume meter
      light(cellToNote(1, col), muted ? COL.red : COL.dim);
      bits.push(`m${col}:${muted ? 1 : 0}`);
      // vol 0 → no meter LEDs; otherwise light rows 2..litRow.
      const litRow = volumeRowFor(vol);
      for (let row = 2; row <= 6; row++) {
        const note = cellToNote(row, col);
        let color = COL.off;
        if (litRow && row <= litRow) {
          color = muted ? COL.dim : (row === litRow ? COL.green : COL.greenDim);
        }
        light(note, color);
        bits.push(`${note}:${color}`);
      }
      // solo
      const thisSolo = soloOn && selected === ch;
      light(cellToNote(7, col), thisSolo ? COL.yellow : COL.dim);
      bits.push(`s${col}:${thisSolo ? 1 : 0}`);
      // select
      light(cellToNote(8, col), selected === ch ? COL.white : COL.blue);
      bits.push(`v${col}:${selected === ch ? 1 : 0}`);
    }
    const sig = bits.join("|");
    if (sig !== lastPaint) lastPaint = sig;
  }

  function handlePad(row, col, velocity) {
    if (velocity <= 0) return;
    const voice = col;
    if (row === 1) {
      device.toggleMute?.(voice);
      return;
    }
    if (row >= 2 && row <= 6) {
      const vol = volumeForRow(row);
      device.setChannelVolume?.(voice, vol);
      return;
    }
    if (row === 7) {
      if (device.soloActive?.() && device.voice?.() === voice) {
        device.soloRelease?.();
      } else {
        device.soloHold?.(voice);
      }
      return;
    }
    if (row === 8) {
      device.selectVoice?.(voice);
    }
  }

  function onControlMessage(ev) {
    const name = ev.target?.name || "";
    if (!isLaunchpadName(name)) return false;
    const [statusByte, data1, data2 = 0] = ev.data || [];
    if (statusByte == null) return true;
    const type = statusByte & 0xf0;
    // Note on/off (and velocity-0 note-on as off). Also accept channel aftertouch-free pads.
    if (type === 0x90 || type === 0x80) {
      const cell = noteToCell(data1);
      if (!cell) return true; // swallow scene/logo buttons for now
      if (type === 0x90 && data2 > 0) handlePad(cell.row, cell.col, data2);
      return true;
    }
    // Control-change pads on some layouts — ignore but claim.
    if (type === 0xb0) return true;
    return true;
  }

  function attachOutput(midiAccess) {
    const next = pickOutput(midiAccess);
    if (next?.id === output?.id) return;
    output = next;
    connectedName = output?.name || "";
    if (output) {
      enterProgrammer(output);
      clearGrid();
      paint();
      status(`Launchpad: ${connectedName}`);
    } else {
      status("Launchpad: disconnected");
    }
  }

  function tick() {
    if (disposed) return;
    paint();
    raf = requestAnimationFrame(tick);
  }

  function onStateChange() {
    if (!access) return;
    attachOutput(access);
  }

  async function start() {
    device.setMidiControlHandler?.(onControlMessage);
    if (!navigator.requestMIDIAccess) {
      status("Launchpad: Web MIDI unavailable");
      return;
    }
    try {
      access = await navigator.requestMIDIAccess({ sysex: true });
    } catch {
      try {
        access = await navigator.requestMIDIAccess({ sysex: false });
      } catch {
        status("Launchpad: MIDI permission denied");
        return;
      }
    }
    // Prefer device's shared access for port enumeration when ready.
    const shared = device.midiAccess?.();
    if (shared) access = shared;
    access.onstatechange = onStateChange;
    attachOutput(access);
    if (!raf) raf = requestAnimationFrame(tick);

    // If device MIDI arrives later (async bind), re-attach.
    const waitShared = setInterval(() => {
      if (disposed) {
        clearInterval(waitShared);
        return;
      }
      const mid = device.midiAccess?.();
      if (mid && mid !== access) {
        access = mid;
        access.onstatechange = onStateChange;
        attachOutput(access);
        clearInterval(waitShared);
      }
    }, 250);
    setTimeout(() => clearInterval(waitShared), 8000);
  }

  function stop() {
    disposed = true;
    if (raf) cancelAnimationFrame(raf);
    raf = 0;
    device.setMidiControlHandler?.(null);
    clearGrid();
    output = null;
    status("Launchpad: off");
  }

  return { start, stop, paint, isLaunchpadName, noteToCell, cellToNote, volumeForRow };
}
