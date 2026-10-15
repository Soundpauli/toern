// Port of drawRandoms / companion generation in toern_helpers.ino.
// Arduino random(hi) is [0, hi). random(lo, hi) is [lo, hi).

import { COLS, PAGES, ROWS, STEPS } from "./const.js";

function rnd(a, b) {
  if (b === undefined) return Math.floor(Math.random() * a);
  return a + Math.floor(Math.random() * (b - a));
}
function u16(n) { return n & 0xffff; }
function u32(n) { return n >>> 0; }
function clamp(v, lo, hi) { return v < lo ? lo : v > hi ? hi : v; }

function roleFor(channel) {
  switch (channel) {
    case 1: return "kick";
    case 2: return "snare";
    case 3: return "hat";
    case 4: return "clap";
    case 5: return "tom";
    case 6:
    case 11: return "bass";
    case 7:
    case 13:
    case 14: return "keys";
    case 8: return "pad";
    default: return "";
  }
}

function defaultHomeRow(role) {
  switch (role) {
    case "kick": return 3;
    case "snare": return 5;
    case "hat": return 7;
    case "clap": return 9;
    case "tom": return 6;
    case "bass": return 3;
    case "keys": return 9;
    case "pad": return 10;
    default: return 5;
  }
}

function pitchAdjust(filt, channel) {
  if (channel < 1 || channel > 8) return { detune: 0, oct: 0 };
  const f = filt[channel];
  // Match firmware: DTNE ±1 semi (fine), OCTV whole semis ±24 (slider 0..48 center 24).
  const detune = ((f.detune ?? 16) / 32) * 2 - 1;
  const octSemis = (f.oct ?? 24) - 24;
  return { detune, oct: octSemis };
}

function pitchClass(filt, channel, row) {
  const adj = pitchAdjust(filt, channel);
  let noteValue = row - 1;
  if (channel >= 1 && channel <= 8) {
    noteValue = 12 + row - (channel + 1) + Math.round(adj.detune) + adj.oct;
  } else if (channel === 11) {
    noteValue = 12 * -1 + row - 1;
  } else if (channel === 13 || channel === 14) {
    noteValue = row - channel + 47;
  }
  noteValue %= 12;
  return noteValue < 0 ? noteValue + 12 : noteValue;
}

function cellPitchClass(filt, cell, channel, row) {
  if (cell.midiPitch !== undefined && cell.midiPitch !== 255 && cell.midiPitch <= 127) {
    return cell.midiPitch % 12;
  }
  return pitchClass(filt, channel, row);
}

function effectivePages() { return PAGES; }

function highestPage(note) {
  let last = 0;
  for (let p = 1; p <= PAGES; p++) {
    const base = (p - 1) * COLS;
    for (let x = 1; x <= COLS; x++) {
      for (let y = 1; y <= ROWS; y++) {
        if (note[base + x][y].channel > 0) { last = p; break; }
      }
      if (last === p) break;
    }
  }
  return last || 1;
}

function activePages(note, loopLength, edit) {
  let pages = loopLength > 0 ? loopLength : highestPage(note);
  if (pages < edit) pages = edit;
  return clamp(pages, 1, effectivePages());
}

function emptyContext() {
  const channelHomeRow = Array(16).fill(5);
  for (let ch = 1; ch < 16; ch++) channelHomeRow[ch] = defaultHomeRow(roleFor(ch));
  return {
    stepWeight: Array(16).fill(0),
    stepVelocity: Array(16).fill(0),
    kickWeight: Array(16).fill(0),
    snareWeight: Array(16).fill(0),
    clapWeight: Array(16).fill(0),
    hatWeight: Array(16).fill(0),
    tomWeight: Array(16).fill(0),
    bassWeight: Array(16).fill(0),
    harmonicWeight: Array(16).fill(0),
    rowWeight: Array(17).fill(0),
    pitchClassWeight: Array(12).fill(0),
    barPitchClassWeight: Array.from({ length: 4 }, () => Array(12).fill(0)),
    barBassPcWeight: Array.from({ length: 4 }, () => Array(12).fill(0)),
    channelHomeRow,
    channelMeanVel: Array(16).fill(96),
    channelHitCount: Array(16).fill(0),
    syncopationScore: 0,
    totalWeight: 0,
    sourcePages: 0,
    rootRow: 1,
    isMinor: false,
    empty: true,
  };
}

function addRole(ctx, role, phase, weight) {
  const bag = {
    kick: ctx.kickWeight, snare: ctx.snareWeight, clap: ctx.clapWeight,
    hat: ctx.hatWeight, tom: ctx.tomWeight, bass: ctx.bassWeight,
  }[role];
  if (bag) bag[phase] = u16(bag[phase] + weight);
  else if (role === "keys" || role === "pad") ctx.harmonicWeight[phase] = u16(ctx.harmonicWeight[phase] + weight);
}

function analyze(note, filt, firstPage, lastPage, focusPage, exclude) {
  const ctx = emptyContext();
  const limit = effectivePages();
  firstPage = clamp(firstPage, 1, limit);
  lastPage = clamp(lastPage, firstPage, limit);
  focusPage = clamp(focusPage, 1, limit);

  const chRowHist = Array.from({ length: 16 }, () => Array(17).fill(0));
  const chVelSum = Array(16).fill(0);
  let onBeatWeight = 0;
  let offBeatWeight = 0;

  for (let page = firstPage; page <= lastPage; page++) {
    const distance = Math.abs(page - focusPage);
    const pageWeight = distance === 0 ? 6 : distance === 1 ? 2 : 1;
    const start = (page - 1) * COLS + 1;
    const end = Math.min(STEPS + 1, start + COLS);
    ctx.sourcePages++;
    for (let c = start; c < end; c++) {
      const phase = ((c - start) % 16);
      const bar = clamp((phase / 4) | 0, 0, 3);
      for (let row = 1; row <= 16; row++) {
        const cell = note[c][row];
        const ch = cell.channel;
        if (!ch || ch >= 16 || !roleFor(ch)) continue;

        chRowHist[ch][row] += pageWeight;
        chVelSum[ch] += cell.velocity;
        ctx.channelHitCount[ch]++;

        if (ch === exclude) continue;
        const velocityWeight = Math.max(1, (cell.velocity / 20) | 0);
        const weight = pageWeight * velocityWeight;
        ctx.stepWeight[phase] = u16(ctx.stepWeight[phase] + weight);
        ctx.stepVelocity[phase] = u32(ctx.stepVelocity[phase] + cell.velocity * weight);
        const role = roleFor(ch);
        addRole(ctx, role, phase, weight);
        ctx.totalWeight = u32(ctx.totalWeight + weight);
        ctx.empty = false;

        if ((phase & 3) === 0) onBeatWeight += weight;
        else offBeatWeight += weight;

        if (role === "bass" || role === "keys" || role === "pad") {
          const pc = cellPitchClass(filt, cell, ch, row);
          let harmW = role === "bass" ? weight * 2 : weight;
          if ((phase & 3) === 0) harmW += pageWeight;
          ctx.rowWeight[row] = u16(ctx.rowWeight[row] + weight);
          ctx.pitchClassWeight[pc] = u16(ctx.pitchClassWeight[pc] + harmW);
          if (distance === 0) {
            ctx.barPitchClassWeight[bar][pc] = u16(ctx.barPitchClassWeight[bar][pc] + harmW * 2);
            if (role === "bass") {
              ctx.barBassPcWeight[bar][pc] = u16(ctx.barBassPcWeight[bar][pc] + harmW * 3);
            }
          } else {
            ctx.barPitchClassWeight[bar][pc] = u16(ctx.barPitchClassWeight[bar][pc] + pageWeight);
            if (role === "bass") {
              ctx.barBassPcWeight[bar][pc] = u16(ctx.barBassPcWeight[bar][pc] + pageWeight * 2);
            }
          }
        }
      }
    }
  }

  for (let ch = 1; ch < 16; ch++) {
    let bestR = 0;
    for (let r = 1; r <= 16; r++) {
      if (chRowHist[ch][r] > bestR) {
        bestR = chRowHist[ch][r];
        ctx.channelHomeRow[ch] = r;
      }
    }
    if (ctx.channelHitCount[ch] > 0) {
      ctx.channelMeanVel[ch] = clamp((chVelSum[ch] / ctx.channelHitCount[ch]) | 0, 40, 124);
    }
  }

  if (onBeatWeight + offBeatWeight > 0) {
    ctx.syncopationScore = clamp(((offBeatWeight * 100) / (onBeatWeight + offBeatWeight)) | 0, 0, 100);
  }

  let best = 0;
  for (let row = 1; row <= 16; row++) {
    if (ctx.rowWeight[row] > best) {
      best = ctx.rowWeight[row];
      ctx.rootRow = row;
    }
  }
  const majorScale = [0, 2, 4, 5, 7, 9, 11];
  const minorScale = [0, 2, 3, 5, 7, 8, 10];
  const degreeWeights = [6, 2, 4, 3, 5, 3, 2];
  let bestScore = -32768;
  let bestRootPc = (ctx.rootRow - 1) % 12;
  let bestMinor = false;
  for (let root = 0; root < 12; root++) {
    for (let mode = 0; mode < 2; mode++) {
      const scale = mode ? minorScale : majorScale;
      let score = 0;
      for (let pc = 0; pc < 12; pc++) {
        let degree = -1;
        for (let d = 0; d < 7; d++) {
          if ((root + scale[d]) % 12 === pc) { degree = d; break; }
        }
        const multiplier = degree >= 0 ? degreeWeights[degree] : -4;
        score += ctx.pitchClassWeight[pc] * multiplier;
      }
      score += ctx.barBassPcWeight[0][root] * 4;
      score += ctx.barPitchClassWeight[0][root] * 2;
      if (score > bestScore) {
        bestScore = score;
        bestRootPc = root;
        bestMinor = mode !== 0;
      }
    }
  }
  if (ctx.empty) bestRootPc = (ctx.rootRow - 1) % 12;
  ctx.rootRow = bestRootPc + 1;
  ctx.isMinor = bestMinor;
  return ctx;
}

function clearChannelPage(note, page, channel) {
  const start = (page - 1) * COLS + 1;
  const end = Math.min(STEPS + 1, start + COLS);
  for (let c = start; c < end; c++) {
    for (let row = 1; row <= 16; row++) {
      if (note[c][row].channel !== channel) continue;
      note[c][row].channel = 0;
      note[c][row].velocity = 100;
      note[c][row].probability = 100;
      note[c][row].condition = 1;
      note[c][row].midiPitch = 255;
    }
  }
}

function fillScale(harm) {
  const major = [0, 2, 4, 5, 7, 9, 11];
  const minor = [0, 2, 3, 5, 7, 8, 10];
  const intervals = harm.isMinor ? minor : major;
  return intervals.map((n) => (harm.rootPc + n) % 12);
}

function makeHarmony(ctx, variation) {
  let tonal = 0;
  for (let pc = 0; pc < 12; pc++) tonal += ctx.pitchClassWeight[pc];
  const none = tonal === 0;
  const harm = {
    rootPc: none ? (variation * 5) % 12 : (ctx.rootRow - 1) % 12,
    isMinor: none ? (((variation / 2) | 0) & 1) === 0 : ctx.isMinor,
    degree: [0, 5, 2, 6],
    chordRootPc: [0, 0, 0, 0],
    barHasContext: [false, false, false, false],
  };
  const majorProg = [[0, 4, 5, 3], [0, 3, 4, 0], [0, 5, 3, 4], [5, 3, 0, 4]];
  const minorProg = [[0, 5, 2, 6], [0, 3, 5, 4], [0, 5, 3, 4], [0, 2, 5, 6]];
  const prog = (harm.isMinor ? minorProg : majorProg)[((variation / 3) | 0) & 3];
  const scale = fillScale(harm);

  for (let bar = 0; bar < 4; bar++) {
    harm.degree[bar] = prog[bar];
    harm.chordRootPc[bar] = scale[prog[bar] % 7];
    harm.barHasContext[bar] = false;

    let barTotal = 0;
    let bestBassW = 0;
    let bestBassPc = -1;
    for (let pc = 0; pc < 12; pc++) {
      barTotal += ctx.barPitchClassWeight[bar][pc];
      if (ctx.barBassPcWeight[bar][pc] > bestBassW) {
        bestBassW = ctx.barBassPcWeight[bar][pc];
        bestBassPc = pc;
      }
    }
    if (barTotal === 0) continue;
    harm.barHasContext[bar] = true;

    let bestDeg = prog[bar];
    let bestScore = -32768;
    for (let d = 0; d < 7; d++) {
      const rPc = scale[d];
      const tPc = scale[(d + 2) % 7];
      const fPc = scale[(d + 4) % 7];
      const sPc = scale[(d + 6) % 7];
      let sc =
        ctx.barPitchClassWeight[bar][rPc] * 5 +
        ctx.barPitchClassWeight[bar][tPc] * 3 +
        ctx.barPitchClassWeight[bar][fPc] * 3 +
        ctx.barPitchClassWeight[bar][sPc] * 1 +
        ctx.barBassPcWeight[bar][rPc] * 6;
      if (d === prog[bar]) sc += 2;
      if (sc > bestScore) {
        bestScore = sc;
        bestDeg = d;
      }
    }
    harm.degree[bar] = bestDeg;
    harm.chordRootPc[bar] = bestBassPc >= 0 ? bestBassPc : scale[bestDeg];
  }
  return harm;
}

function chordPcs(harm, bar, withSeventh) {
  const scale = fillScale(harm);
  const b = clamp(bar, 0, 3);
  const deg = harm.degree[b] % 7;
  const out = [harm.barHasContext[b] ? harm.chordRootPc[b] : scale[deg], scale[(deg + 2) % 7], scale[(deg + 4) % 7]];
  if (withSeventh) out.push(scale[(deg + 6) % 7]);
  return out;
}

function pcToRow(filt, pc, channel, lo, hi, prefer) {
  pc = ((pc % 12) + 12) % 12;
  lo = clamp(lo, 1, 16);
  hi = clamp(hi, lo, 16);
  prefer = clamp(prefer, lo, hi);
  let best = lo;
  let bestDist = 99;
  for (let row = lo; row <= hi; row++) {
    if (pitchClass(filt, channel, row) !== pc) continue;
    const dist = Math.abs(row - prefer);
    if (dist < bestDist) { bestDist = dist; best = row; }
  }
  return bestDist < 99 ? best : prefer;
}

function degreeToPc(harm, bar, role, motifStep, strongBeat) {
  const chord = chordPcs(harm, bar, role === "keys");
  const scale = fillScale(harm);
  const baseDeg = harm.degree[clamp(bar, 0, 3)] % 7;

  if (role === "bass") {
    if (strongBeat || motifStep === 0) return chord[0];
    if (motifStep === 1) return chord[2];
    if (motifStep === 2) return chord[1];
    return scale[(baseDeg + motifStep + 7) % 7];
  }
  if (role === "pad") {
    const idx = ((motifStep % 3) + 3) % 3;
    return chord[idx];
  }
  if (strongBeat) {
    const idx = ((motifStep % chord.length) + chord.length) % chord.length;
    return chord[idx];
  }
  const deg = (baseDeg + motifStep + 14) % 7;
  return scale[deg];
}

function chordLaneRow(filt, harm, bar, channel, variation, prevRow) {
  const seventh = ((variation + bar) % 3) === 0;
  const chord = chordPcs(harm, bar, seventh);
  const n = chord.length;
  const inversion = (((variation / 3) | 0) + bar) % 3;
  let idx = channel === 13 ? inversion % n : (inversion + 1 + (seventh ? 1 : 0)) % n;
  if (channel === 14 && chord[idx] === chord[inversion % n]) {
    idx = (idx + 1) % n;
  }
  const prefer = prevRow >= 1 && prevRow <= 16 ? prevRow : (channel === 13 ? 7 : 11);
  const lo = channel === 13 ? 3 : 6;
  const hi = channel === 13 ? 12 : 16;
  return pcToRow(filt, chord[idx], channel, lo, hi, prefer);
}

function euclid(phase, pulses, rotation) {
  pulses = clamp(pulses, 0, 16);
  let shifted = (phase - rotation) % 16;
  if (shifted < 0) shifted += 16;
  return pulses > 0 && ((shifted * pulses) % 16) < pulses;
}

function mixHash(value) {
  value = u32(value);
  value = u32(value ^ (value >>> 16));
  value = Math.imul(value, 0x7feb352d) >>> 0;
  value = u32(value ^ (value >>> 15));
  value = Math.imul(value, 0x846ca68b) >>> 0;
  return u32(value ^ (value >>> 16));
}

function softShift(value) {
  const bucket = value % 4;
  return bucket === 0 ? -1 : bucket === 3 ? 1 : 0;
}

function makeGroove(ctx, page, variation) {
  let hash = mixHash(u32(variation * 131 + page * 977 + ctx.totalWeight));
  for (let phase = 0; phase < 16; phase++) {
    hash = mixHash(u32(hash + ctx.stepWeight[phase] * (phase + 17)));
  }
  return {
    hatPulses: 3 + (hash & 3),
    hatRotation: (hash >>> 3) & 15,
    clapPulses: 2 + ((hash >>> 7) % 3),
    clapRotation: (hash >>> 11) & 15,
    tomRotation: (hash >>> 15) & 15,
    snareShiftA: softShift(hash >>> 19),
    snareShiftB: softShift(hash >>> 22),
    secondHalfShift: softShift(hash >>> 25),
    secondHalfRotation: 3 + ((hash >>> 28) % 7),
  };
}

function percussionRow(channel, role, phase, structural, variation, ctx) {
  const home = (channel < 16 && ctx.channelHomeRow[channel] >= 1)
    ? ctx.channelHomeRow[channel]
    : defaultHomeRow(role);
  if (structural) return clamp(home, 1, 16);

  if (role === "kick" || role === "snare" || role === "clap") {
    if (phase >= 14 && ((variation + phase) % 4 === 0)) {
      return clamp(home + ((variation & 1) ? 1 : -1), 1, 16);
    }
    return clamp(home, 1, 16);
  }
  if (role === "hat") {
    if ((phase & 3) === 2 && ((variation + phase) % 3 === 0)) {
      return clamp(home + 1, 1, 16);
    }
    return clamp(home, 1, 16);
  }
  if (role === "tom") {
    const tomContour = [1, 0, 0, -1];
    const delta = phase >= 12 ? tomContour[phase - 12] : ((phase & 4) ? 0 : 1);
    return clamp(home + delta, 1, 16);
  }
  return clamp(home, 1, 16);
}

function freeRow(note, step, preferred) {
  preferred = clamp(preferred, 1, 16);
  if (note[step][preferred].channel === 0) return preferred;
  for (let distance = 1; distance <= 4; distance++) {
    const up = preferred + distance;
    const down = preferred - distance;
    if (up <= 16 && note[step][up].channel === 0) return up;
    if (down >= 1 && note[step][down].channel === 0) return down;
  }
  return 0;
}

function stepLimit(channel) { return channel === 11 ? 3 : 1; }

function placeLimited(note, step, preferredRow, channel, velocity) {
  let used = 0;
  for (let row = 1; row <= 16; row++) if (note[step][row].channel === channel) used++;
  if (used >= stepLimit(channel)) return false;
  const row = freeRow(note, step, preferredRow);
  if (!row) return false;
  const n = note[step][row];
  n.channel = channel;
  n.velocity = clamp(velocity, 1, 127);
  n.probability = 100;
  n.condition = 1;
  n.midiPitch = 255;
  return true;
}

function velocityFor(ctx, phase, base) {
  let velocity = base;
  if (ctx.stepWeight[phase] > 0) {
    const weighted = (ctx.stepVelocity[phase] / ctx.stepWeight[phase]) | 0;
    if (weighted > 0 && weighted <= 127) velocity = ((velocity * 2 + weighted) / 3) | 0;
  }
  const quarter = (phase & 3) === 0;
  const eighth = (phase & 1) === 0;
  velocity += quarter ? rnd(6, 13) : (eighth ? rnd(-4, 5) : rnd(-14, -3));
  return clamp(velocity, 28, 126);
}

function tempoDensity(bpm) {
  bpm = clamp(bpm | 0, 40, 300);
  if (bpm >= 190) return 58;
  if (bpm >= 155) return 72;
  if (bpm <= 65) return 88;
  if (bpm <= 90) return 96;
  return 100;
}

const MOTIF_CONTOURS = [
  [0, 0, 2, 1, 0, 4, 2, 1],
  [0, 2, 4, 2, 0, -1, 1, 2],
  [0, 4, 2, 0, 2, 1, 0, -1],
  [2, 1, 0, 2, 4, 2, 1, 0],
  [0, 1, 2, 4, 2, 1, 0, 2],
  [4, 2, 1, 0, 2, 4, 3, 2],
  [0, 0, 4, 4, 2, 2, 1, 0],
  [0, 2, 1, 3, 2, 0, -1, 0],
];

function generatePage(note, filt, page, channel, ctx, harm, groove, variation, bpm) {
  let role = roleFor(channel);
  if (!role) return;
  const start = (page - 1) * COLS + 1;
  const end = Math.min(STEPS + 1, start + COLS);
  const densityTempo = tempoDensity(bpm);
  const seedHash = mixHash(u32(Math.imul(variation, 2654435761) + page * 40503 + channel * 977));

  const ch11AsPolyComp = channel === 11 && ctx.channelHitCount[6] >= 3;
  if (ch11AsPolyComp) role = "keys";

  const contour = MOTIF_CONTOURS[seedHash & 7];
  let callMask8 = 0;
  for (let s = 0; s < 8; s++) {
    const k = ctx.kickWeight[s] > 0 || ctx.kickWeight[s + 8] > 0;
    const sn = ctx.snareWeight[s] > 0 || ctx.clapWeight[s] > 0 || ctx.snareWeight[s + 8] > 0 || ctx.clapWeight[s + 8] > 0;
    const bs = ctx.bassWeight[s] > 0 || ctx.bassWeight[s + 8] > 0;
    const hm = ctx.harmonicWeight[s] > 0 || ctx.harmonicWeight[s + 8] > 0;
    const stepH = mixHash(u32(seedHash + s * 313));

    if (role === "bass") {
      const onKick = k && !sn;
      const pocketSync = !k && !sn && (s === 2 || s === 3 || s === 6 || s === 7) &&
        ((stepH % 100) < (38 + ((ctx.syncopationScore / 3) | 0)));
      if (s === 0 || onKick || pocketSync || (ctx.empty && (s === 0 || s === 3 || s === 6))) {
        callMask8 |= (1 << s);
      }
    } else if (role === "keys") {
      const chordBeat = s === 0 || s === 4;
      const offPocket = !sn && (!bs || s === 2 || s === 6) &&
        (s === 2 || s === 3 || s === 6 || s === 7) &&
        ((stepH % 100) < (channel === 7 ? 45 : 32));
      if (chordBeat && (stepH % 100) < 82) callMask8 |= (1 << s);
      else if (offPocket && !hm) callMask8 |= (1 << s);
    }
  }
  if (role === "bass" && (callMask8 & 0x0f) === 0) callMask8 |= 0x01;
  if (role === "keys" && callMask8 === 0) callMask8 |= 0x11;

  let prevRow = (channel < 16 && ctx.channelHomeRow[channel] >= 1)
    ? ctx.channelHomeRow[channel]
    : defaultHomeRow(role);

  for (let c = start; c < end; c++) {
    const pageOffset = c - start;
    const segment = (pageOffset / 16) | 0;
    const phase = pageOffset % 16;
    const bar = (phase / 4) | 0;
    const halfStep = phase & 7;
    const secondHalf = phase >= 8;
    const isTurnaround = phase >= 12;
    const quarter = (phase & 3) === 0;
    const segmentShift = segment > 0 ? groove.secondHalfShift : 0;
    const snareA = clamp(4 + groove.snareShiftA + segmentShift, 3, 5);
    const snareB = clamp(12 + groove.snareShiftB - segmentShift, 11, 13);
    const backbeat = phase === snareA || phase === snareB;
    let structural = false;
    let pitched = false;
    let chance = 0;
    let row = prevRow;
    let baseVelocity = (channel < 16 && ctx.channelHitCount[channel] > 0)
      ? ctx.channelMeanVel[channel]
      : 94;

    if (role === "kick") {
      pitched = true;
      baseVelocity = 114;
      const onSnare = ctx.snareWeight[phase] > 0 || ctx.clapWeight[phase] > 0 || backbeat;
      if (phase === 0) {
        chance = 98;
        structural = true;
      } else if (phase === 8 && !onSnare) {
        chance = (bpm >= 150 && (variation & 1)) ? 35 : 88;
        structural = chance >= 80;
      } else if (ctx.bassWeight[phase] > 0 && !onSnare) {
        chance = 78;
        structural = phase === 4 || phase === 12;
      } else if (!onSnare && (phase === 6 || phase === 10 || phase === 14 || phase === 15)) {
        const preSnareOrBar = (phase + 1 === snareB || phase >= 14);
        chance = preSnareOrBar ? 38 : 18;
      } else if (!onSnare && phase === 4 && (variation % 3 === 0)) {
        chance = 76;
        structural = true;
      }
    } else if (role === "snare") {
      pitched = true;
      baseVelocity = 106;
      if (backbeat) {
        chance = ctx.clapWeight[phase] ? 72 : 94;
        structural = true;
      } else {
        const next = (phase + 1) & 15;
        const pocketEmpty = ctx.kickWeight[phase] === 0 && ctx.bassWeight[phase] === 0;
        const ghostPocket = pocketEmpty && (phase === 7 || phase === 9 || phase === 11 || phase === 15);
        const turnaroundFill = phase >= 14 && (segment > 0 || (variation & 1));
        if (turnaroundFill) {
          chance = 52;
          baseVelocity = 78;
        } else if (ghostPocket && ctx.kickWeight[next] === 0) {
          chance = 26;
          baseVelocity = 62;
        } else {
          chance = 0;
        }
      }
    } else if (role === "hat") {
      pitched = true;
      baseVelocity = 80;
      const pulses = groove.hatPulses + (densityTempo >= 90 ? 2 : 0);
      const rotation = (groove.hatRotation + (segment > 0 ? groove.secondHalfRotation : 0)) & 15;
      const offbeat8th = (phase & 3) === 2;
      const even8th = (phase & 1) === 0;
      const isEuclid = euclid(phase, clamp(pulses, 4, 8), rotation);
      if (offbeat8th) {
        chance = 92;
        structural = true;
        baseVelocity = 88;
      } else if (even8th || isEuclid) {
        chance = ctx.snareWeight[phase] > 0 ? 55 : 80;
        structural = true;
        baseVelocity = 74;
      } else {
        const preSnare = (((phase + 1) & 15) === snareA || ((phase + 1) & 15) === snareB);
        chance = (preSnare || isTurnaround) ? 38 : 14;
        baseVelocity = 58;
      }
    } else if (role === "clap") {
      pitched = true;
      baseVelocity = 96;
      const rotation = (groove.clapRotation + (segment > 0 ? groove.secondHalfRotation : 0)) & 15;
      const isEuclid = euclid(phase, groove.clapPulses, rotation);
      if (backbeat) {
        chance = ctx.snareWeight[phase] ? 64 : 88;
        structural = true;
      } else if (isEuclid && ctx.kickWeight[phase] === 0 && (phase === 3 || phase === 6 || phase === 11 || phase === 14 || phase === 15)) {
        chance = 34;
        baseVelocity = 78;
      }
    } else if (role === "tom") {
      pitched = true;
      baseVelocity = 88;
      const pocketFree = ctx.kickWeight[phase] === 0 && ctx.snareWeight[phase] === 0;
      if (isTurnaround && pocketFree && (phase === 13 || phase === 14 || phase === 15)) {
        chance = 58;
        structural = phase === 14;
      } else if (pocketFree && (phase === 3 || phase === 6 || phase === 11) && euclid(phase, 3, groove.tomRotation)) {
        chance = 28;
      }
    } else if (role === "bass") {
      baseVelocity = 106;
      let motifHit = ((callMask8 >>> halfStep) & 1) !== 0;
      if (secondHalf && isTurnaround) {
        motifHit = (phase === 12 && ctx.snareWeight[12] === 0) || phase === 14 || (phase === 15 && (variation & 1));
      }
      if (ctx.kickWeight[phase] > 0 && ctx.snareWeight[phase] === 0) {
        chance = 92;
        structural = true;
      } else if (motifHit && ctx.snareWeight[phase] === 0) {
        chance = secondHalf ? 82 : 88;
        structural = phase === 0 || phase === 8;
      } else {
        chance = 0;
      }
      if (chance > 0) {
        let degStep = contour[halfStep];
        if (phase === 0 || phase === 8 || (ctx.kickWeight[phase] > 0 && quarter)) degStep = 0;
        else if (phase === 14) degStep = 4;
        else if (phase === 15) degStep = 6;
        else if (secondHalf) degStep = contour[(halfStep + 2) & 7];
        const pc = degreeToPc(harm, bar, "bass", degStep, quarter || ctx.kickWeight[phase] > 0);
        const prefer = (phase === 14 || (!quarter && (halfStep & 1))) ? 5 : 3;
        row = pcToRow(filt, pc, channel, 1, 8, prefer);
        prevRow = row;
      }
    } else if (role === "keys") {
      baseVelocity = ch11AsPolyComp ? 84 : 86;
      let motifHit = ((callMask8 >>> halfStep) & 1) !== 0;
      if (secondHalf && phase >= 13) {
        motifHit = phase === 14 || (phase === 15 && channel === 7);
      }
      if (channel === 13 || channel === 14) {
        let chordStab = phase === 0 || phase === 6 || phase === 8 || phase === 12 ||
          (phase === 3 && (variation & 1)) ||
          (phase === 14 && ((variation + segment) & 1));
        if (ctx.snareWeight[phase] > 0 && !quarter) chordStab = false;
        if (!chordStab) continue;
        row = chordLaneRow(filt, harm, bar, channel, variation + segment * 3, prevRow);
        prevRow = row;
        placeLimited(note, c, row, channel, velocityFor(ctx, phase, baseVelocity));
        continue;
      }
      if (ch11AsPolyComp) {
        const compHit = phase === 0 || phase === 6 || phase === 8 || phase === 12 ||
          (phase === 2 && ctx.bassWeight[2] === 0) ||
          (phase === 14 && ctx.snareWeight[14] === 0);
        if (!compHit) continue;
        const chord = chordPcs(harm, bar, true);
        const voices = (phase === 0 || phase === 8) ? 3 : 2;
        const vel = velocityFor(ctx, phase, baseVelocity);
        for (let v = 0; v < voices; v++) {
          const pc = chord[(v + bar + (variation & 1)) % chord.length];
          const r = pcToRow(filt, pc, channel, 4, 15, 6 + v * 3);
          placeLimited(note, c, r, channel, Math.max(36, vel - v * 6));
        }
        continue;
      }
      if (!motifHit) continue;
      let degStep = contour[halfStep];
      if (secondHalf) {
        degStep = isTurnaround ? (phase === 14 ? 2 : 0) : (degStep + 2);
      }
      const pc = degreeToPc(harm, bar, "keys", degStep, quarter);
      const prefer = clamp(prevRow + (degStep > 2 ? 1 : -1), 5, 14);
      row = pcToRow(filt, pc, channel, 3, 16, prefer);
      prevRow = row;
      placeLimited(note, c, row, channel, velocityFor(ctx, phase, baseVelocity));
      continue;
    } else if (role === "pad") {
      baseVelocity = 74;
      const padHit = phase === 0 || phase === 8 || (phase === 12 && harm.degree[3] !== harm.degree[2]);
      if (!padHit) continue;
      const toneIdx = (bar + (variation & 1) + segment) % 3;
      const pc = degreeToPc(harm, bar, "pad", toneIdx, true);
      row = pcToRow(filt, pc, channel, 4, 15, prevRow);
      prevRow = row;
      placeLimited(note, c, row, channel, velocityFor(ctx, phase, baseVelocity));
      continue;
    }

    if (chance <= 0) continue;
    if (!structural) chance = (chance * densityTempo / 100) | 0;
    chance = clamp(chance, 0, 100);
    if (rnd(100) >= chance) continue;
    if (pitched) row = percussionRow(channel, role, phase, structural, variation, ctx);
    placeLimited(note, c, row, channel, velocityFor(ctx, phase, baseVelocity));
  }
}

export function drawRandoms({ note, filt, channel, page, loopLength, bpm }) {
  if (!roleFor(channel)) return false;
  page = clamp(page | 0, 1, effectivePages());
  const context = analyze(note, filt, 1, activePages(note, loopLength, page), page, channel);
  const seed = rnd(0, 256);
  const harm = makeHarmony(context, seed);
  const groove = makeGroove(context, page, seed);
  clearChannelPage(note, page, channel);
  generatePage(note, filt, page, channel, context, harm, groove, seed, bpm);
  return true;
}

export function drawRandomsMultiPage({ note, filt, page, loopLength, bpm }) {
  page = clamp(page | 0, 1, effectivePages());
  const actPages = activePages(note, loopLength, page);
  const seed = rnd(1, 256);

  const pick = Array(16).fill(false);
  pick[1] = rnd(100) < 90;
  const backbeatPick = rnd(100);
  if (backbeatPick < 48) pick[2] = true;
  else if (backbeatPick < 78) pick[4] = true;
  else { pick[2] = true; pick[4] = true; }
  pick[3] = rnd(100) < 82;
  pick[5] = rnd(100) < 35;

  const bassPick = rnd(100);
  if (bassPick < 45) pick[6] = true;
  else if (bassPick < 82) pick[11] = true;
  else { pick[6] = true; pick[11] = true; }

  const harmPick = rnd(100);
  if (harmPick < 38) pick[7] = true;
  else if (harmPick < 76) { pick[13] = true; pick[14] = true; }
  else { pick[7] = true; pick[8] = true; }
  if (!pick[8] && rnd(100) < 30) pick[8] = true;

  const base = (page - 1) * COLS;
  for (let x = 1; x <= COLS; x++) {
    for (let y = 1; y <= ROWS; y++) {
      note[base + x][y].channel = 0;
      note[base + x][y].midiPitch = 255;
    }
  }

  const baseCtx = analyze(note, filt, 1, actPages, page, 0);
  const pageHarm = makeHarmony(baseCtx, seed);
  const pageGroove = makeGroove(baseCtx, page, seed);

  const order = [1, 2, 4, 3, 5, 6, 11, 13, 14, 7, 8];
  for (const ch of order) {
    if (!pick[ch]) continue;
    const ctx = analyze(note, filt, 1, Math.max(actPages, page), page, ch);
    generatePage(note, filt, page, ch, ctx, pageHarm, pageGroove, seed, bpm);
  }
  return true;
}
