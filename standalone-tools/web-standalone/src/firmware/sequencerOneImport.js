/** Audio Damage Sequencer One backup → TŒRN pattern cells (gate + pitch). */

import JSZip from "jszip";
import { foldRow } from "./pitch.js";

const HEADER = 12;
const STEP_BYTES = 14;
const STEP_SLOTS = 64;
const PATT_MIN = HEADER + STEP_SLOTS * STEP_BYTES + 1;
const GATE_VEL = [50, 70, 90, 110, 127];
const ROOT_MIDI = 48; // C3 + chromatic degree 0–12

const PATT_RE = /(?:^|\/)PATT_([A-D])(\d{2})\.DAT$/i;
const GLOBALS_RE = /(?:^|\/)GLOBALS\.DAT$/i;

/**
 * @param {Uint8Array} bytes
 * @param {string} [id] e.g. "A00"
 */
export function parsePattDat(bytes, id = "") {
  if (!bytes || bytes.length < PATT_MIN) {
    throw new Error(`PATT too small (${bytes?.length || 0} B)`);
  }
  const lengthRaw = bytes[HEADER + STEP_SLOTS * STEP_BYTES];
  const length = Math.max(1, Math.min(STEP_SLOTS, lengthRaw || 1));
  const steps = [];
  let noteCount = 0;
  for (let i = 0; i < length; i++) {
    const o = HEADER + i * STEP_BYTES;
    const flags = bytes[o];
    const pitch = bytes[o + 1];
    const gate = bytes[o + 2];
    const active = flags !== 0;
    if (active) noteCount += 1;
    steps.push({ flags, pitch, gate, active });
  }
  return { id, length, steps, noteCount };
}

/** Map Sequencer One pitch/flags → MIDI note (chromatic 0–12 + octave nibble). */
export function stepMidi(step, rootMidi = ROOT_MIDI) {
  const oct = ((step.flags >> 4) & 0xf) - 2;
  const deg = Math.max(0, Math.min(12, step.pitch | 0));
  return Math.max(0, Math.min(127, rootMidi + deg + oct * 12));
}

export function stepVelocity(step) {
  const g = Math.max(0, Math.min(4, step.gate | 0));
  return GATE_VEL[g];
}

/**
 * @param {{ steps: Array<{flags:number,pitch:number,gate:number,active:boolean}> }} pattern
 * @param {{ channel?: number, rootMidi?: number }} [opts]
 * @returns {{ cells: Array, pages: number[] }}
 */
export function patternToImportCells(pattern, opts = {}) {
  const channel = opts.channel || 11;
  const rootMidi = opts.rootMidi != null ? opts.rootMidi : ROOT_MIDI;
  const cells = [];
  const pages = new Set();
  for (let i = 0; i < pattern.steps.length; i++) {
    const step = pattern.steps[i];
    if (!step.active) continue;
    const midi = stepMidi(step, rootMidi);
    const folded = foldRow(midi, channel);
    if (folded.row < 1 || folded.row > 15) continue;
    const abs = i + 1;
    if (abs < 1 || abs > 256) continue;
    pages.add(Math.floor(i / 16) + 1);
    cells.push({
      step: abs,
      row: folded.row,
      channel,
      velocity: stepVelocity(step),
      midiPitch: midi,
      probability: 100,
    });
  }
  return { cells, pages: [...pages].sort((a, b) => a - b) };
}

/** Flat 256×16 cell list for encodePatternFile / importPatternCells. */
export function patternToFlatCells(pattern, opts = {}) {
  const { cells } = patternToImportCells(pattern, opts);
  const flat = Array.from({ length: 256 * 16 }, () => ({
    channel: 0,
    velocity: 0,
    probability: 100,
    condition: 1,
    midiPitch: 255,
  }));
  for (const c of cells) {
    const i = (c.step - 1) * 16 + (c.row - 1);
    flat[i] = {
      channel: c.channel,
      velocity: c.velocity,
      probability: 100,
      condition: 1,
      midiPitch: c.midiPitch,
    };
  }
  return flat;
}

function parseGlobalsBpm(bytes) {
  if (!bytes || bytes.length < 28) return null;
  const raw = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(24, true);
  if (raw >= 400 && raw <= 3000) return raw / 10;
  return null;
}

/**
 * @param {File|Blob|ArrayBuffer|Uint8Array} input
 * @returns {Promise<{ name: string, bpm: number, patterns: Array }>}
 */
export async function parseSequencerOneZip(input) {
  const zip = await JSZip.loadAsync(input);
  const paths = Object.keys(zip.files).filter((p) => !zip.files[p].dir);
  let bpm = 120;
  const globPath = paths.find((p) => GLOBALS_RE.test(p));
  if (globPath) {
    const g = await zip.file(globPath).async("uint8array");
    const parsed = parseGlobalsBpm(g);
    if (parsed != null) bpm = parsed;
  }

  const pattPaths = paths.filter((p) => PATT_RE.test(p));
  pattPaths.sort((a, b) => {
    const ma = a.match(PATT_RE);
    const mb = b.match(PATT_RE);
    const bank = (ma[1].toUpperCase().charCodeAt(0) - mb[1].toUpperCase().charCodeAt(0));
    if (bank) return bank;
    return Number(ma[2]) - Number(mb[2]);
  });

  if (!pattPaths.length) throw new Error("No PATT_*.DAT files in this ZIP.");

  const patterns = [];
  for (const path of pattPaths) {
    const m = path.match(PATT_RE);
    const id = `${m[1].toUpperCase()}${m[2]}`;
    const bytes = await zip.file(path).async("uint8array");
    patterns.push(parsePattDat(bytes, id));
  }

  const name = input?.name || "Sequencer One backup";
  return { name, bpm, patterns };
}
