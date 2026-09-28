import { Midi } from "@tonejs/midi";

const TRACK_COLORS = [
  "rgb(139, 0, 0)", "rgb(255, 69, 0)", "rgb(255, 255, 0)", "rgb(0, 139, 0)",
  "rgb(0, 140, 130)", "rgb(0, 0, 255)", "rgb(140, 0, 120)", "rgb(220, 100, 100)",
];

export async function parseMidiFile(file) {
  const midi = new Midi(await file.arrayBuffer());
  const notes = [];
  const tracks = [];
  let id = 0;
  midi.tracks.forEach((track, index) => {
    if (!track.notes.length) return;
    for (const n of track.notes) {
      notes.push({
        track: id,
        note: n.midi,
        velocity: n.velocity,
        time: n.time,
        duration: n.duration,
      });
    }
    tracks.push({
      id,
      name: track.name || `Track ${index + 1}`,
      color: TRACK_COLORS[id % TRACK_COLORS.length],
      noteCount: track.notes.length,
    });
    id += 1;
  });
  const bpm = midi.header.tempos[0]?.bpm || 120;
  return { notes, tracks, bpm };
}

/** Fold a MIDI pitch into grid rows 1–15 for a given voice (like firmware MIDI-in). */
export function foldRow(midiNote, channel) {
  // Voice 1 row 1 = MIDI 47 (B2), row 2 = MIDI 48 (C3). row = midi - 47 + channel.
  const BASE = 47;
  let pitch = midiNote;
  let row = pitch - BASE + channel;
  while (row > 15) { pitch -= 12; row = pitch - BASE + channel; }
  while (row < 1) { pitch += 12; row = pitch - BASE + channel; }
  return { pitch, row };
}

/** Map MIDI notes onto the TŒRN grid (16 steps per page, rows 1–15).
 *  Transpose on (default): octave-fold into range and play from the grid row.
 *  Transpose off: still draw at the folded row, but keep the original MIDI pitch
 *  (same idea as firmware MIDI → NOTE CLAMP = OFF).
 */
export function mapNotesToGrid(notes, bpm = 120, subdivision = 16, timeOffset = 0, transpose = true) {
  if (!notes.length) return [];
  const minTime = Math.min(...notes.map((n) => n.time));
  const stepDur = (60 / Math.max(1, bpm)) * (4 / Math.max(1, subdivision));
  const occupied = new Set();
  const out = [];

  for (const note of notes) {
    const absolute = Math.floor((note.time - minTime + timeOffset) / stepDur + 1e-9);
    if (absolute < 0) continue;
    const page = Math.floor(absolute / 16);
    const x = absolute % 16;
    const channel = note.track + 1;
    const natural = note.note - 47 + channel;
    const inRange = natural >= 1 && natural <= 15;
    const folded = foldRow(note.note, channel);
    if (folded.row < 1 || folded.row > 15) continue;

    // Always place on the folded row so out-of-range notes stay visible.
    const row = folded.row;
    // Transpose off → keep absolute pitch; on → play from row (midiPitch none).
    const midiPitch = transpose ? 255 : note.note;

    const key = `${note.track}-${page}-${x}-${row - 1}`;
    if (occupied.has(key)) continue;
    occupied.add(key);
    out.push({
      track: note.track,
      note: note.note,
      original: note.note,
      midiPitch,
      velocity: note.velocity,
      time: note.time,
      duration: note.duration,
      x,
      y: row - 1,
      page,
      play: true,
      outOfRange: !inRange,
      transpose,
    });
  }
  return out;
}

/** Drop overlapping notes: one per track+step, then one per cell (highest velocity). */
export function filterOverlappingNotes(notes, priority = "highest") {
  const byStep = new Map();
  for (const note of notes) {
    const key = `${note.page}-${note.x}-${note.track}`;
    const prev = byStep.get(key);
    if (!prev) byStep.set(key, note);
    else if (priority === "highest" ? note.y > prev.y : note.y < prev.y) byStep.set(key, note);
  }
  const byCell = new Map();
  for (const note of byStep.values()) {
    const key = `${note.page}-${note.x}-${note.y}`;
    const prev = byCell.get(key);
    if (!prev || note.velocity > prev.velocity) byCell.set(key, note);
  }
  return [...byCell.values()];
}

/** Build apply payload from an explicit track→channel map (values 1–8).
 *  Folding + pitch policy are decided here for the *assigned* voice so remapping
 *  a track does not drop absolute pitches when transpose is off.
 */
export function buildImportCells(gridNotes, trackToChannel, transpose = true) {
  const toCh = trackToChannel instanceof Map
    ? trackToChannel
    : new Map(Object.entries(trackToChannel || {}).map(([id, ch]) => [Number(id), Number(ch)]));
  const cells = [];
  const pages = new Set();
  for (const n of gridNotes) {
    const ch = toCh.get(n.track);
    if (!ch) continue;
    const step = n.page * 16 + n.x + 1;
    const src = Number.isFinite(n.original) ? n.original : n.note;
    if (!Number.isFinite(src) || src < 0 || src > 127) continue;
    const wantTranspose = n.transpose != null ? !!n.transpose : !!transpose;
    const folded = foldRow(src, ch);
    const row = folded.row;
    if (step < 1 || step > 256 || row < 1 || row > 15) continue;
    pages.add(n.page + 1);
    cells.push({
      step,
      row,
      channel: ch,
      velocity: Math.max(1, Math.min(127, Math.round(n.velocity * 127))),
      // Off: keep original pitch while still drawing at the folded row.
      midiPitch: wantTranspose ? 255 : src,
      probability: 100,
    });
  }
  return { cells, pages: [...pages].sort((a, b) => a - b) };
}
