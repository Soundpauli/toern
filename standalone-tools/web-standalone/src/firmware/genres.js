export const GENRES = [
  { name: "BLNK", color: [100, 100, 100], bpm: 100 },
  { name: "TECH", color: [255, 100, 0], bpm: 128 },
  { name: "HIPH", color: [255, 0, 255], bpm: 85 },
  { name: "DNB", color: [0, 255, 0], bpm: 174 },
  { name: "HOUS", color: [0, 100, 255], bpm: 124 },
  { name: "AMBT", color: [255, 255, 0], bpm: 70 },
];

const MINOR = [2, 4, 5, 7, 9, 11, 12, 14];
const MAJOR = [2, 4, 6, 7, 9, 11, 13, 14];

function roll(n) { return Math.floor(Math.random() * n); }
function jitter(v, spread) { return v + roll(spread * 2 + 1) - spread; }
function has(list, beat) { return list.includes(beat); }

function put(note, step, row, ch, vel, prob = 100, cond = 1) {
  if (step < 1 || row < 1 || row > 15) return;
  const n = note[step][row];
  n.channel = ch;
  n.velocity = Math.max(1, Math.min(127, jitter(vel, 3)));
  n.probability = prob;
  n.condition = cond;
  n.midiPitch = 255;
}

function degree(scale, page, beat) {
  return scale[(page + Math.floor((beat - 1) / 4)) % scale.length];
}

function clearNotes(note, steps) {
  for (let x = 1; x <= steps; x++) {
    for (let y = 1; y <= 16; y++) {
      const n = note[x][y];
      if (!n) continue;
      n.channel = 0;
      n.velocity = 100;
      n.probability = 100;
      n.condition = 1;
      n.midiPitch = 255;
    }
  }
}

function drums(note, step, beat, kick, snare, hatEvery, open, perc) {
  if (has(kick.on, beat)) put(note, step, 3, 1, beat === 1 ? kick.vel + 6 : kick.vel);
  if (has(snare.on, beat)) {
    const ghost = snare.ghost?.includes(beat);
    put(note, step, 5, 2, ghost ? snare.soft : snare.vel, ghost ? snare.prob : 100);
  }
  if (beat % hatEvery === 0) put(note, step, 7, 3, beat % 4 === 0 ? 78 : 70);
  if (has(open, beat)) put(note, step, 8, 4, 90, 85);
  if (has(perc, beat)) put(note, step, 6, 5, 62, 40);
}

function techno(note, step, beat, page) {
  const kick = [
    [1, 5, 9, 13],
    [1, 3, 5, 7, 9, 11, 13, 15],
    [1, 4, 7, 9, 12, 15],
    [1, 2, 5, 6, 9, 10, 13, 14],
  ][page % 4];
  const snare = page % 3 === 1
    ? { on: [3, 5, 11, 13], ghost: [3, 11], vel: 80, soft: 58, prob: 45 }
    : page % 3 === 2
      ? { on: [4, 5, 6, 12, 13, 14], ghost: [4, 6, 12, 14], vel: 75, soft: 62, prob: 60 }
      : { on: [5, 13], vel: 112, soft: 112, prob: 100 };
  drums(note, step, beat, { on: kick, vel: 118 }, snare, 2, [5, 13], [2, 6, 10, 14]);
  const bass = page % 2 === 0 ? [1, 9] : [1, 3, 5, 7, 9, 11, 13, 15];
  if (has(bass, beat)) put(note, step, degree(MINOR, page, beat), 6, 102);
  if (roll(100) < 18) put(note, step, degree(MINOR, page, beat + 2), 7, 84, 70);
}

function hiphop(note, step, beat, page) {
  const kick = [
    [1, 7, 11],
    [1, 3, 7, 11, 15],
    [1, 4, 7, 10, 13],
    [1, 2, 7, 8, 11, 12],
  ][page % 4];
  const snare = page % 3 === 1
    ? { on: [3, 5, 11, 13], ghost: [3, 11], vel: 100, soft: 70, prob: 50 }
    : { on: [5, 13], vel: 114, soft: 114, prob: 100 };
  drums(note, step, beat, { on: kick, vel: 118 }, snare, 2, [13], [6, 14]);
  if (has([1, 7, 11], beat)) put(note, step, degree(MINOR, page, beat), 6, 108);
  if (roll(100) < 12) put(note, step, degree(MINOR, page, beat + 4), 8, 70, 60, 2);
}

function dnb(note, step, beat, page) {
  const kick = [
    [1, 9],
    [1, 5, 9, 13],
    [1, 7, 11],
    [1, 3, 9, 11],
  ][page % 4];
  const snare = { on: [5, 13], vel: 120, soft: 120, prob: 100 };
  drums(note, step, beat, { on: kick, vel: 120 }, snare, 2, [7, 15], [3, 11]);
  if (has([1, 4, 9, 12], beat)) put(note, step, degree(MINOR, page, beat), 6, 110);
  if (roll(100) < 16) put(note, step, degree(MINOR, page, beat + 1), 7, 78, 55, 2);
}

function house(note, step, beat, page) {
  const kick = [
    [1, 5, 9, 13],
    [1, 3, 5, 7, 9, 11, 13, 15],
    [1, 5, 7, 9, 13, 15],
  ][page % 3];
  const snare = page % 4 === 1
    ? { on: [3, 5, 11, 13], ghost: [3, 11], vel: 85, soft: 56, prob: 45 }
    : page % 4 === 3
      ? { on: [2, 5, 8, 13], vel: 106, soft: 106, prob: 100 }
      : { on: [5, 13], vel: 106, soft: 106, prob: 100 };
  const open = page % 2 === 0 ? [5, 13] : [3, 5, 11, 13];
  drums(note, step, beat, { on: kick, vel: 114 }, snare, 2, open, [2, 6, 10, 14]);
  if (has([1, 9], beat) || (page % 2 === 1 && has([1, 5, 9, 13], beat))) {
    put(note, step, degree(MAJOR, page, beat), 6, 100);
  }
  if (has([1, 5, 9, 13], beat)) put(note, step, degree(MAJOR, page, beat + 4), 7, 72, 80);
  if (roll(100) < 14) put(note, step, degree(MAJOR, page, beat + 2), 8, 64, 50, 2);
}

function ambient(note, step, beat, page) {
  const chance = page % 3 === 0 ? 15 : page % 3 === 1 ? 25 : 35;
  if (roll(100) < chance) {
    const row = Math.min(15, degree(MAJOR, page, beat) + (page % 2 === 0 ? 2 : 0));
    put(note, step, row, 5 + (page % 4), 48, 70, page % 3 === 0 ? 2 : 1);
  }
  if (roll(100) < (page % 2 === 0 ? 8 : 15)) put(note, step, 6, 3, 38, 55, 4);
  if (roll(100) < 12) put(note, step, degree(MINOR, page, beat), 6, 54, 55, 4);
  if (roll(100) < 18) put(note, step, Math.min(15, degree(MAJOR, page, beat) + 3), 7, 46, 65, 2);
  if (roll(100) < 5) put(note, step, 5, 2, 44, 35, 8);
}

const WRITE = [null, techno, hiphop, dnb, house, ambient];

export function generateGenre(note, type, pages, cols, steps) {
  clearNotes(note, steps);
  const write = WRITE[type];
  if (!write || pages < 1) return;
  for (let page = 0; page < pages; page++) {
    const start = page * cols;
    for (let i = 0; i < cols; i++) {
      const beat = (i % 16) + 1;
      write(note, start + i + 1, beat, page);
    }
  }
}
