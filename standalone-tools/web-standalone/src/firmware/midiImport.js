import { foldRow } from "./pitch.js";
export { foldRow } from "./pitch.js";
import { Midi } from "@tonejs/midi";

const TRACK_COLORS = [
  "rgb(139, 0, 0)", "rgb(255, 69, 0)", "rgb(255, 255, 0)", "rgb(0, 139, 0)",
  "rgb(0, 140, 130)", "rgb(0, 0, 255)", "rgb(140, 0, 120)", "rgb(220, 100, 100)",
];

// GM percussion groups share one TŒRN sample voice each. Pitched percussion
// (marimba etc.) stays melodic unless the MIDI channel is the drum channel.
// Drum/sample roles follow companionRoleForChannel(); synth priorities are
// the SIM MIDI-import layout: CH11 instruments, CH13 keys, CH14 bass.
export const MIDI_VOICE_LABELS = {
  1: "BD", 2: "Snare", 3: "Closed hat", 4: "Clap", 5: "Tom / percussion",
  6: "Bass sample", 7: "Keys sample", 8: "Vocals / pad",
  11: "Instrument synth", 13: "Keys synth", 14: "Bass synth",
};
export const DRUM_ROLES = {
  kick: { channel: 1, sample: 0, label: "Kick" },
  snare: { channel: 2, sample: 1, label: "Snare" },
  hat: { channel: 3, sample: 2, label: "Hi-hat / cymbals" },
  clap: { channel: 4, sample: 3, label: "Clap" },
  tom: { channel: 5, sample: 4, label: "Tom / percussion" },
};
function drumRole(pitch) {
  if ([35, 36].includes(pitch)) return "kick";
  if ([37, 38, 40].includes(pitch)) return "snare";
  if ([42, 44, 46].includes(pitch)) return "hat";
  if (pitch === 39) return "clap";
  if ([41, 43, 45, 47, 48, 50].includes(pitch)) return "tom";
  if ([51, 53, 59].includes(pitch)) return "hat";
  if ([49, 52, 55, 57].includes(pitch)) return "hat";
  return "tom";
}
function namedDrumRole(name) {
  const n = name.toLowerCase();
  if (/\b(kick|bd|bass drum)\b/.test(n)) return "kick";
  if (/\b(snare|rim|sd)\b/.test(n)) return "snare";
  if (/\b(hi[ -]?hat|hats?|hh)\b/.test(n)) return "hat";
  if (/\bclaps?\b/.test(n)) return "clap";
  if (/\btoms?\b/.test(n)) return "tom";
  if (/\bride\b/.test(n)) return "hat";
  if (/\b(crash|cymbal)\b/.test(n)) return "hat";
  if (/\b(perc(ussion)?|shaker|tambourine)\b/.test(n)) return "tom";
  return null;
}
function melodicRole(track) {
  const p = track.instrument.number;
  const name = track.name.toLowerCase();
  // Explicit musical names help exported files with stale program changes.
  if (/\b(bass|sub)\b/.test(name)) return "bass";
  if (/\b(pad|strings?|choir|violin|cello|scifi|atmosphere)\b/.test(name)) return "pad";
  if (/\b(piano|keys?|organ|chords?)\b/.test(name)) return "keys";
  if (/\b(guitar|lead|pluck|flute|brass)\b/.test(name)) return "lead";
  if (p >= 32 && p <= 39) return "bass";
  if ((p >= 88 && p <= 103) || (p >= 40 && p <= 55)) return "pad";
  if (p < 24) return "keys";
  return "lead";
}
function hasChords(notes) {
  let end = -Infinity;
  for (const n of [...notes].sort((a, b) => a.time - b.time)) {
    if (n.time < end - .02) return true;
    end = Math.max(end, n.time + n.duration);
  }
  return false;
}
export async function parseMidiFile(file) {
  const midi = new Midi(await file.arrayBuffer());
  const notes = [], tracks = [], drums = new Map();
  midi.tracks.forEach((track, index) => {
    if (!track.notes.length) return;
    const name = track.name || `Track ${index + 1}`;
    const namedRole = namedDrumRole(name);
    const percussion = track.instrument.percussion || (track.instrument.number === 0 && (!!namedRole || /\b(drums?|drum kit)\b/i.test(name)));
    const groups = new Map();
    for (const n of track.notes) {
      const role = percussion ? namedRole || drumRole(n.midi) : melodicRole(track);
      let target = percussion ? drums.get(role) : groups.get(role);
      if (!target) {
        const id = tracks.length;
        target = {
          id, name: percussion ? DRUM_ROLES[role].label : name,
          color: TRACK_COLORS[id % TRACK_COLORS.length], noteCount: 0,
          percussion, role, program: track.instrument.number,
          instrument: track.instrument.name || "Unknown instrument",
          family: track.instrument.family, midiChannel: track.channel + 1,
          polyphonic: !percussion && hasChords(track.notes),
        };
        tracks.push(target);
        (percussion ? drums : groups).set(role, target);
      }
      target.noteCount++;
      notes.push({ track: target.id, note: n.midi, velocity: n.velocity,
        time: n.time, duration: n.duration, percussion, role });
    }
  });
  return { notes, tracks, bpm: midi.header.tempos[0]?.bpm || 120 };
}

/** Assign by TŒRN roles with the SIM import synth layout. */
export function suggestMidiSlots(tracks) {
  const slots = Object.fromEntries(Object.keys(MIDI_VOICE_LABELS).map(ch => [ch, null]));
  for (const t of tracks.filter(t => t.percussion)) slots[DRUM_ROLES[t.role].channel] = t.id;
  const melodic = tracks.filter(t => !t.percussion).sort((a,b) => b.noteCount - a.noteCount || a.id - b.id);
  // Reserve instrument/keys/bass synths before using compatible sample voices.
  const preferences = { bass: [14,6], keys: [13,7,11], pad: [8,7,11], lead: [11,7] };
  const assigned = new Set();
  // Give each family its dedicated voice first, then use fallbacks by
  // musical activity. A quiet string part must not displace the main melody.
  for (const [role, ch] of [["bass",14], ["keys",13], ["pad",8], ["lead",11]]) {
    const t = melodic.find(t => t.role === role);
    if (t) { slots[ch] = t.id; assigned.add(t.id); }
  }
  for (const t of melodic) {
    if (assigned.has(t.id)) continue;
    const ch = preferences[t.role].find(ch => slots[ch] == null);
    if (ch != null) slots[ch] = t.id;
  }
  return slots;
}
/** Fold a MIDI pitch into grid rows 1–15 for a given voice (like firmware MIDI-in). */

/** Map MIDI notes onto the TŒRN grid (16 steps per page, rows 1–15).
 *  Transpose on: octave-fold into range and play from the grid row.
 *  Transpose off (default): still draw at the folded row, but keep the original MIDI pitch
 *  (same idea as firmware MIDI → NOTE CLAMP = OFF).
 */
export function mapNotesToGrid(notes, bpm = 120, subdivision = 16, timeOffset = 0, transpose = false) {
  if (!notes.length) return [];
  const minTime = notes.reduce((min, n) => Math.min(min, n.time), Infinity);
  const stepDur = (60 / Math.max(1, bpm)) * (4 / Math.max(1, subdivision));
  const occupied = new Set();
  const out = [];

  for (const note of notes) {
    const absolute = Math.floor((note.time - minTime + timeOffset) / stepDur + 1e-9);
    if (absolute < 0) continue;
    const page = Math.floor(absolute / 16);
    const x = absolute % 16;
    const channel = note.track + 1;
    const folded = foldRow(note.note, channel);
    if (folded.row < 1 || folded.row > 15) continue;

    // Always place on the folded row so out-of-range notes stay visible.
    const row = folded.row;
    // Transpose off → keep absolute pitch; on → play from row (midiPitch none).
    const midiPitch = transpose ? 255 : note.note;

    const key = `${note.track}-${page}-${x}-${row - 1}-${transpose ? "folded" : note.note}`;
    if (occupied.has(key)) continue;
    occupied.add(key);
    out.push({
      percussion: !!note.percussion,
      role: note.role,
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
      transpose,
    });
  }
  return out;
}

/** One note per assigned sample voice/step; CH13 filtering is optional.
 * Without an assignment map, retain the legacy per-track filtering helper.
 */
export function filterOverlappingNotes(notes, priority = "highest", channelMap = null, filterCh13 = true) {
  const shouldFilter = note => {
    if (!channelMap) return true;
    const ch = channelMap.get(note.track);
    return (ch >= 1 && ch <= 8) || (ch === 13 && filterCh13);
  };
  const keyFor = note => `${note.page}-${note.x}-${channelMap ? channelMap.get(note.track) : note.track}`;
  const pitch = note => note.original ?? note.note ?? note.y;
  const byStep = new Map();
  for (const note of notes) {
    if (!shouldFilter(note)) continue;
    const key = keyFor(note), prev = byStep.get(key);
    if (!prev || (priority === "highest" ? pitch(note) > pitch(prev) : pitch(note) < pitch(prev)) ||
        (pitch(note) === pitch(prev) && note.velocity > prev.velocity)) byStep.set(key,note);
  }
  // Keep CH11/14 polyphonic note data and unassigned tracks intact. Shared grid
  // cells are resolved later, after channel assignment.
  return notes.filter(note => !shouldFilter(note) || byStep.get(keyFor(note)) === note);
}

/** Select a 256-step window in the active SIM panel's page units, rebased to step 1. */
export function selectMidiPageWindow(gridNotes, startPage = 1, pageSteps = 32) {
  pageSteps = pageSteps <= 16 ? 16 : 32;
  const windowPages = 256 / pageSteps;
  const totalSteps = gridNotes.reduce((max,n) => Math.max(max, n.page * 16 + n.x + 1), 0);
  const totalPages = Math.max(1, Math.ceil(totalSteps / pageSteps));
  const maxStartPage = Math.max(1, totalPages - windowPages + 1);
  startPage = Math.max(1, Math.min(maxStartPage, Math.floor(Number(startPage) || 1)));
  const first = (startPage - 1) * pageSteps;
  const notes = gridNotes.flatMap(n => {
    const step = n.page * 16 + n.x - first;
    return step < 0 || step >= 256 ? [] : [{ ...n, page: Math.floor(step / 16), x: step % 16 }];
  });
  return { notes, startPage, endPage: Math.min(totalPages, startPage + windowPages - 1),
    totalPages, maxStartPage, windowPages };
}

/** Build apply payload from an explicit track→channel map (values 1–8).
 *  Folding + pitch policy are decided here for the *assigned* voice so remapping
 *  a track does not drop absolute pitches when transpose is off.
 */
export function buildImportCells(gridNotes, trackToChannel, transpose = false) {
  const toCh = trackToChannel instanceof Map
    ? trackToChannel
    : new Map(Object.entries(trackToChannel || {}).map(([id, ch]) => [Number(id), Number(ch)]));
  const cells = [];
  const pages = new Set();
  const occupied = new Set();
  // Reserve drum rows first. Absolute-pitch notes can use another display row
  // without changing pitch when several voices hit the same grid cell.
  for (const n of [...gridNotes].sort((a,b) => Number(!!b.percussion) - Number(!!a.percussion))) {
    const ch = toCh.get(n.track);
    if (!ch) continue;
    const step = n.page * 16 + n.x + 1;
    const src = Number.isFinite(n.original) ? n.original : n.note;
    if (!Number.isFinite(src) || src < 0 || src > 127) continue;
    const wantTranspose = n.transpose != null ? !!n.transpose : !!transpose;
    const folded = foldRow(src, ch);
    let row = n.percussion ? ch + 1 : folded.row;
    if (step < 1 || step > 256 || row < 1 || row > 15) continue;
    if (occupied.has(`${step}:${row}`) && !n.percussion && !wantTranspose) {
      row = Array.from({length:15}, (_,i) => i+1)
        .sort((a,b) => Math.abs(a-folded.row)-Math.abs(b-folded.row))
        .find(y => !occupied.has(`${step}:${y}`)) || 0;
    }
    if (!row || occupied.has(`${step}:${row}`)) continue;
    occupied.add(`${step}:${row}`);
    pages.add(n.page + 1);
    cells.push({
      step,
      row,
      channel: ch,
      velocity: Math.max(1, Math.min(127, Math.round(n.velocity * 127))),
      // Off: keep original pitch while still drawing at the folded row.
      midiPitch: n.percussion || wantTranspose ? 255 : src,
      probability: 100,
    });
  }
  return { cells, pages: [...pages].sort((a, b) => a - b) };
}
