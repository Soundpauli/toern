/** Encode / decode TŒRN pattern files (N.txt / autosaved.txt) for SD transfer. */

const STEPS = 256;
const ROWS = 16;
const NOTE_BYTES = 4;
const MARKER = [0xff, 0xfe];
const PITCH_HEADER = [0x54, 0x50, 0x49, 0x54, 1, 0x10, 0x00, 0]; // TPIT v1, 4096 entries
const MIDI_NONE = 255;

/**
 * @param {Array<{channel:number,velocity:number,probability:number,condition:number,midiPitch:number}>} cells
 *   Flat list in firmware order: for x=1..256, for y=1..16
 * @param {{ bpm?: number, smpTail?: Uint8Array }} [opts]
 */
export function encodePatternFile(cells, opts = {}) {
  const notes = new Uint8Array(STEPS * ROWS * NOTE_BYTES);
  const pitches = new Uint8Array(STEPS * ROWS);
  pitches.fill(MIDI_NONE);

  for (let i = 0; i < STEPS * ROWS; i++) {
    const n = cells[i];
    const o = i * NOTE_BYTES;
    if (n && n.channel) {
      notes[o] = n.channel & 0xff;
      notes[o + 1] = Math.max(0, Math.min(127, n.velocity | 0));
      notes[o + 2] = Math.max(0, Math.min(100, n.probability != null ? n.probability : 100));
      notes[o + 3] = n.condition != null ? n.condition : 1;
      pitches[i] = n.midiPitch <= 127 ? n.midiPitch : MIDI_NONE;
    } else {
      notes[o] = 0;
      notes[o + 1] = 0;
      notes[o + 2] = 100;
      notes[o + 3] = 1;
    }
  }

  const parts = [notes, new Uint8Array(MARKER)];
  if (opts.smpTail && opts.smpTail.length) {
    parts.push(opts.smpTail);
  } else {
    // Minimal SMP stub: float bpm LE + padding so load still succeeds.
    const bpm = Number.isFinite(opts.bpm) ? opts.bpm : 100;
    const stub = new Uint8Array(64);
    new DataView(stub.buffer).setFloat32(0, bpm, true);
    stub[4] = 1; // file slot hint
    stub[8] = 1; // pack hint (unsigned int little-endian low byte)
    parts.push(stub);
  }
  parts.push(new Uint8Array(PITCH_HEADER), pitches);

  let total = 0;
  for (const p of parts) total += p.length;
  const out = new Uint8Array(total);
  let off = 0;
  for (const p of parts) {
    out.set(p, off);
    off += p.length;
  }
  return out;
}

/**
 * @param {Uint8Array} bytes
 * @returns {{ cells: Array, bpm: number|null, smpTail: Uint8Array|null }}
 */
export function decodePatternFile(bytes) {
  const expected = STEPS * ROWS;
  const minBytes = expected * NOTE_BYTES + 2;
  if (!bytes || bytes.length < minBytes) {
    throw new Error(`Pattern too small (${bytes?.length || 0} B, need ≥ ${minBytes})`);
  }

  const cells = [];
  let pos = 0;
  for (let i = 0; i < expected; i++) {
    const ch = bytes[pos];
    const vel = bytes[pos + 1];
    const prob = bytes[pos + 2];
    const cond = bytes[pos + 3];
    pos += 4;
    cells.push({
      channel: ch,
      velocity: vel,
      probability: prob,
      condition: cond,
      midiPitch: MIDI_NONE,
    });
  }

  let smpTail = null;
  let bpm = null;
  if (pos + 2 <= bytes.length && bytes[pos] === 0xff && bytes[pos + 1] === 0xfe) {
    pos += 2;
    const afterMarker = pos;
    // Peek float bpm at start of SMP
    if (pos + 4 <= bytes.length) {
      bpm = new DataView(bytes.buffer, bytes.byteOffset + pos, 4).getFloat32(0, true);
      if (!(bpm >= 40 && bpm <= 300)) bpm = null;
    }
    // Pitch extension header search from end of known notes+marker
    const hdr = PITCH_HEADER;
    let pitchAt = -1;
    for (let i = afterMarker; i + hdr.length + expected <= bytes.length; i++) {
      let ok = true;
      for (let j = 0; j < hdr.length; j++) {
        if (bytes[i + j] !== hdr[j]) { ok = false; break; }
      }
      if (ok) { pitchAt = i; break; }
    }
    if (pitchAt >= 0) {
      smpTail = bytes.slice(afterMarker, pitchAt);
      const pitchData = bytes.subarray(pitchAt + hdr.length, pitchAt + hdr.length + expected);
      for (let i = 0; i < expected; i++) {
        const v = pitchData[i];
        cells[i].midiPitch = (v <= 127 || v === MIDI_NONE) ? v : MIDI_NONE;
      }
    } else {
      smpTail = bytes.slice(afterMarker);
    }
  }

  return { cells, bpm, smpTail };
}

/** Build flat cell list from a note[x][y] grid (1-based, x≤256, y≤16). */
export function cellsFromNoteGrid(getNote) {
  const cells = [];
  for (let x = 1; x <= STEPS; x++) {
    for (let y = 1; y <= ROWS; y++) {
      const n = getNote(x, y);
      if (n && n.channel) {
        cells.push({
          channel: n.channel,
          velocity: n.velocity,
          probability: n.probability,
          condition: n.condition,
          midiPitch: n.midiPitch <= 127 ? n.midiPitch : MIDI_NONE,
        });
      } else {
        cells.push({ channel: 0, velocity: 0, probability: 100, condition: 1, midiPitch: MIDI_NONE });
      }
    }
  }
  return cells;
}
