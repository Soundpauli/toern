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
  for (let page = firstPage; page <= lastPage; page++) {
    const distance = Math.abs(page - focusPage);
    const pageWeight = distance === 0 ? 4 : distance === 1 ? 2 : 1;
    const start = (page - 1) * COLS + 1;
    const end = Math.min(STEPS + 1, start + COLS);
    ctx.sourcePages++;
    for (let c = start; c < end; c++) {
      const phase = ((c - start) % 16);
      for (let row = 1; row <= 16; row++) {
        const ch = note[c][row].channel;
        if (!ch || ch === exclude || !roleFor(ch)) continue;
        const velocityWeight = Math.max(1, (note[c][row].velocity / 24) | 0);
        const weight = pageWeight * velocityWeight;
        ctx.stepWeight[phase] = u16(ctx.stepWeight[phase] + weight);
        ctx.stepVelocity[phase] = u32(ctx.stepVelocity[phase] + note[c][row].velocity * weight);
        const role = roleFor(ch);
        addRole(ctx, role, phase, weight);
        ctx.totalWeight = u32(ctx.totalWeight + weight);
        ctx.empty = false;
        if (role === "bass" || role === "keys" || role === "pad") {
          ctx.rowWeight[row] = u16(ctx.rowWeight[row] + weight);
          const pc = pitchClass(filt, ch, row);
          ctx.pitchClassWeight[pc] = u16(ctx.pitchClassWeight[pc] + weight);
        }
      }
    }
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
        let multiplier = -2;
        if (degree === 0) multiplier = 5;
        else if (degree === 2 || degree === 4) multiplier = 3;
        else if (degree >= 0) multiplier = 1;
        score += ctx.pitchClassWeight[pc] * multiplier;
      }
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

function scaleRow(ctx, lowRegister, motion) {
  const major = [0, 2, 4, 5, 7, 9, 11];
  const minor = [0, 2, 3, 5, 7, 8, 10];
  const scale = ctx.isMinor ? minor : major;
  const root = ctx.empty ? rnd(1, 6) : ctx.rootRow;
  let degree = motion % 7;
  if (degree < 0) degree += 7;
  let row = 1 + ((root - 1 + scale[degree]) % 12);
  if (lowRegister) {
    while (row > 8) row -= 7;
    if (row < 1) row = 1;
  } else {
    while (row < 5) row += 7;
    while (row > 16) row -= 12;
  }
  return clamp(row, 1, 16);
}

function makeHarmony(ctx, variation) {
  let tonal = 0;
  for (let pc = 0; pc < 12; pc++) tonal += ctx.pitchClassWeight[pc];
  const none = tonal === 0;
  const harm = {
    rootPc: none ? (variation * 5) % 12 : (ctx.rootRow - 1) % 12,
    isMinor: none ? ((variation / 2) & 1) === 0 : ctx.isMinor,
    degree: [0, 5, 2, 6],
  };
  const majorProg = [[0, 4, 5, 3], [0, 3, 4, 0], [0, 5, 3, 4], [5, 3, 0, 4]];
  const minorProg = [[0, 5, 2, 6], [0, 3, 5, 4], [0, 5, 3, 4], [0, 2, 5, 6]];
  const prog = (harm.isMinor ? minorProg : majorProg)[(variation / 3) & 3];
  harm.degree = prog.slice();
  return harm;
}

function fillScale(harm) {
  const major = [0, 2, 4, 5, 7, 9, 11];
  const minor = [0, 2, 3, 5, 7, 8, 10];
  const intervals = harm.isMinor ? minor : major;
  return intervals.map((n) => (harm.rootPc + n) % 12);
}

function chordPcs(harm, bar, withSeventh) {
  const scale = fillScale(harm);
  const deg = harm.degree[clamp(bar, 0, 3)] % 7;
  const out = [scale[deg], scale[(deg + 2) % 7], scale[(deg + 4) % 7]];
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

function pickPitchClass(harm, bar, role, prevPc) {
  const seventh = role === "keys" && rnd(100) < 40;
  const chord = chordPcs(harm, bar, seventh);
  const scale = fillScale(harm);
  const pickChord = (rootFifth) => {
    if (rootFifth) {
      const r = rnd(100);
      if (r < 55) return chord[0];
      if (r < 85) return chord[2];
      return chord[1];
    }
    return chord[rnd(0, chord.length)];
  };
  const pickScaleOther = () => {
    for (let attempt = 0; attempt < 8; attempt++) {
      const pc = scale[rnd(0, 7)];
      if (!chord.includes(pc)) return pc;
    }
    return scale[rnd(0, 7)];
  };
  const pickChromatic = () => {
    const base = prevPc >= 0 ? prevPc : chord[0];
    const delta = rnd(100) < 50 ? 1 : -1;
    return (base + delta + 12) % 12;
  };
  if (role === "bass") {
    const r = rnd(100);
    if (r < 55) return chord[0];
    if (r < 85) return chord[2];
    if (r < 95) return pickScaleOther();
    return pickChromatic();
  }
  if (role === "pad") {
    const r = rnd(100);
    if (r < 88) return pickChord(true);
    if (r < 97) return pickScaleOther();
    return pickChromatic();
  }
  const r = rnd(100);
  if (r < 60) return pickChord(false);
  if (r < 90) return pickScaleOther();
  return pickChromatic();
}

function toneRow(filt, harm, bar, role, channel, prevPc, lo, hi, prefer) {
  return pcToRow(filt, pickPitchClass(harm, bar, role, prevPc), channel, lo, hi, prefer);
}

function chordLaneRow(filt, harm, bar, channel, variation) {
  const seventh = ((variation + bar) % 4) === 0;
  const chord = chordPcs(harm, bar, seventh);
  const inversion = ((variation / 3 | 0) + bar) % 3;
  const idx = channel === 13 ? inversion : (inversion + (seventh ? 3 : 2)) % chord.length;
  return pcToRow(filt, chord[idx], channel, 1, 16, channel === 13 ? 7 : 11);
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

function maxPhase(weights) {
  let maximum = 0;
  for (let phase = 0; phase < 16; phase++) maximum = Math.max(maximum, weights[phase]);
  return maximum;
}

function relativeWeight(value, maximum) {
  if (!maximum) return 0;
  return clamp((value * 100 / maximum) | 0, 0, 100);
}

function percussionRow(ctx, role, phase, structural, variation, motif, ) {
  let home = 5;
  let lo = 1;
  let hi = 16;
  let wander = 2;
  if (role === "kick") { home = 3; lo = 1; hi = 7; wander = structural ? 1 : 2; }
  else if (role === "snare") { home = 5; lo = 2; hi = 11; wander = structural ? 2 : 3; }
  else if (role === "hat") { home = 7; lo = 3; hi = 14; wander = 4; }
  else if (role === "clap") { home = 9; lo = 4; hi = 14; wander = structural ? 2 : 3; }
  else if (role === "tom") { home = 6; lo = 2; hi = 12; wander = 4; }
  if ((phase === 0 && (variation & 1)) || rnd(100) < 18) motif.v += rnd(1, 3);
  let row = home;
  if (role === "hat" || role === "clap") {
    row = scaleRow(ctx, false, motif.v + ((phase / 2) | 0) + (variation % 5));
    if (Math.abs(row - home) > wander + 2) {
      row = home + (row > home ? wander : -wander) + rnd(-1, 2);
    }
  } else if (role === "tom") {
    row = phase >= 12
      ? home + ((phase + variation + motif.v) % 5) - 1
      : home + (((motif.v + ((phase / 4) | 0)) % 3)) - 1;
  } else {
    let offset = structural ? rnd(-1, 2) : rnd(-wander, wander + 1);
    if (!structural && (phase === 3 || phase === 6 || phase === 11 || phase === 14)) {
      offset += (variation & 1) ? 1 : -1;
    }
    row = home + offset + ((motif.v % 3) - 1);
  }
  if (rnd(100) < (structural ? 8 : 22)) row += rnd(-wander, wander + 1);
  return clamp(row, lo, hi);
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
    if (weighted > 0 && weighted <= 127) velocity = ((velocity + weighted) / 2) | 0;
  }
  const quarter = phase === 0 || phase === 4 || phase === 8 || phase === 12;
  velocity += quarter ? rnd(5, 14) : rnd(-14, 8);
  return clamp(velocity, 24, 127);
}

function tempoDensity(bpm) {
  bpm = clamp(bpm | 0, 40, 300);
  if (bpm >= 190) return 58;
  if (bpm >= 155) return 72;
  if (bpm <= 65) return 88;
  if (bpm <= 90) return 96;
  return 100;
}

function generatePage(note, filt, page, channel, ctx, harm, groove, variation, bpm) {
  const role = roleFor(channel);
  if (!role) return;
  const start = (page - 1) * COLS + 1;
  const end = Math.min(STEPS + 1, start + COLS);
  const densityTempo = tempoDensity(bpm);
  const motif = { v: rnd(0, 5) + (variation % 7) };
  let prevPc = -1;
  const maxStep = maxPhase(ctx.stepWeight);
  const maxHat = maxPhase(ctx.hatWeight);
  const maxClap = maxPhase(ctx.clapWeight);
  const maxBass = maxPhase(ctx.bassWeight);

  for (let c = start; c < end; c++) {
    const pageOffset = c - start;
    const segment = (pageOffset / 16) | 0;
    const phase = pageOffset % 16;
    const bar = (phase / 4) | 0;
    const quarter = phase === 0 || phase === 4 || phase === 8 || phase === 12;
    const segmentShift = segment > 0 ? groove.secondHalfShift : 0;
    const snareA = clamp(4 + groove.snareShiftA + segmentShift, 3, 5);
    const snareB = clamp(12 + groove.snareShiftB - segmentShift, 11, 13);
    const backbeat = phase === snareA || phase === snareB;
    const chordChange = phase % 4 === 0;
    let structural = false;
    let pitched = false;
    let chance = 0;
    let row = 1;
    let baseVelocity = 92;

    if (role === "kick") {
      chance = phase === 0 ? 96 : phase === 8 ? 82 : 0;
      structural = phase === 0 || phase === 8;
      pitched = true;
      if (ctx.bassWeight[phase]) chance = Math.max(chance, 62);
      if ((phase === 6 || phase === 10 || phase === 15) && rnd(100) < 35) chance = 24;
      baseVelocity = 112;
    } else if (role === "snare") {
      if (backbeat) {
        chance = 82;
        if (ctx.kickWeight[phase]) chance += 8;
        if (ctx.clapWeight[phase]) chance -= 22;
        if (!ctx.kickWeight[phase] && relativeWeight(ctx.stepWeight[phase], maxStep) > 70) chance -= 12;
        structural = true;
      } else {
        const prev = (phase + 15) % 16;
        const next = (phase + 1) % 16;
        const near = next === snareA || next === snareB || prev === snareA || prev === snareB;
        const ghost = near || euclid(phase, 3, groove.tomRotation);
        chance = ghost ? 10 : 1;
        if (ctx.kickWeight[phase] || ctx.kickWeight[next]) chance += 8;
        const density = relativeWeight(ctx.stepWeight[phase], maxStep);
        chance += density < 25 ? 7 : density > 70 ? -5 : 0;
      }
      pitched = true;
      baseVelocity = 102;
    } else if (role === "hat") {
      const pulses = groove.hatPulses - (densityTempo < 70 ? 1 : 0);
      const rotation = (groove.hatRotation + (segment > 0 ? groove.secondHalfRotation : 0)) % 16;
      const candidate = euclid(phase, Math.max(3, pulses), rotation);
      const density = relativeWeight(ctx.stepWeight[phase], maxStep);
      chance = candidate ? 84 - ((density / 3) | 0) : 2;
      if (ctx.kickWeight[phase] || ctx.snareWeight[phase]) chance -= 8;
      if (ctx.snareWeight[(phase + 15) % 16]) chance += 8;
      const existing = relativeWeight(ctx.hatWeight[phase], maxHat);
      chance = (chance * (100 - ((existing / 2) | 0)) / 100) | 0;
      structural = candidate && chance >= 65;
      pitched = true;
      if (phase === 15) chance = Math.max(chance, 10);
      baseVelocity = 78;
    } else if (role === "clap") {
      const rotation = (groove.clapRotation + (segment > 0 ? groove.secondHalfRotation : 0)) % 16;
      const candidate = euclid(phase, groove.clapPulses, rotation);
      if (backbeat) {
        chance = ctx.snareWeight[phase] ? 43 : 69;
        structural = true;
      } else if (candidate) {
        const density = relativeWeight(ctx.stepWeight[phase], maxStep);
        chance = density < 50 ? 28 : 16;
      } else chance = phase === 15 ? 8 : 2;
      const existing = relativeWeight(ctx.clapWeight[phase], maxClap);
      chance = (chance * (100 - ((existing / 2) | 0)) / 100) | 0;
      pitched = true;
      baseVelocity = 94;
    } else if (role === "tom") {
      const rotation = (groove.tomRotation + (segment > 0 ? groove.secondHalfRotation : 0)) % 16;
      chance = euclid(phase, 3, rotation) ? 18 : 2;
      if (phase >= 13) chance = Math.max(chance, 30);
      pitched = true;
      if (ctx.snareWeight[phase] || ctx.tomWeight[phase]) chance = (chance / 3) | 0;
      baseVelocity = 88;
    } else if (role === "bass") {
      chance = ctx.kickWeight[phase] ? 76 : quarter ? 38 : 5;
      structural = !!ctx.kickWeight[phase] || phase === 0 || phase === 8;
      if (ctx.empty) chance = quarter ? 62 : (phase === 6 || phase === 14) ? 12 : 2;
      const existing = relativeWeight(ctx.bassWeight[phase], maxBass);
      if (existing > 0) chance = (chance * Math.max(20, 70 - ((existing / 2) | 0)) / 100) | 0;
      if (ctx.harmonicWeight[phase] && !ctx.kickWeight[phase]) chance = (chance * 65 / 100) | 0;
      if (!quarter && ctx.snareWeight[(phase + 1) % 16]) chance = Math.max(chance, 17);
      if (!ctx.kickWeight[phase] && relativeWeight(ctx.stepWeight[phase], maxStep) > 75) {
        chance = (chance * 70 / 100) | 0;
      }
      baseVelocity = 104;
      row = toneRow(filt, harm, bar, role, channel, prevPc, 1, 8, 3);
      prevPc = pitchClass(filt, channel, row);
    } else if (role === "keys") {
      baseVelocity = 82;
      if (chordChange) {
        chance = phase === 0 || phase === 8 ? 88 : 62;
        structural = true;
      } else if (phase === 2 || phase === 6 || phase === 10 || phase === 14) chance = 10;
      else chance = 2;
      if (channel === 7 && ctx.harmonicWeight[phase]) chance = (chance * (structural ? 65 : 45) / 100) | 0;
      if (!structural) chance = (chance * densityTempo / 100) | 0;
      chance = clamp(chance, 0, 100);
      const gate = (variation * 29 + phase * 17 + segment * 37) % 100;
      if (gate < chance) {
        row = channel === 13 || channel === 14
          ? chordLaneRow(filt, harm, bar, channel, variation + segment * 5)
          : toneRow(filt, harm, bar, role, channel, prevPc, 1, 16, 9);
        prevPc = pitchClass(filt, channel, row);
      } else continue;
    } else if (role === "pad") {
      baseVelocity = 72;
      if (phase === 0 || phase === 8) { chance = 78; structural = true; }
      else if (phase === 4 || phase === 12) chance = 18;
      else chance = 0;
      if (!structural) chance = (chance * densityTempo / 100) | 0;
      chance = clamp(chance, 0, 100);
      if (rnd(100) < chance) {
        row = toneRow(filt, harm, bar, role, channel, prevPc, 1, 16, 10);
        prevPc = pitchClass(filt, channel, row);
        placeLimited(note, c, row, channel, velocityFor(ctx, phase, baseVelocity));
      }
      continue;
    }

    if (role !== "keys" && role !== "pad") {
      if (!structural) chance = (chance * densityTempo / 100) | 0;
      chance = clamp(chance, 0, 100);
      if (rnd(100) >= chance) continue;
    }
    if (pitched) row = percussionRow(ctx, role, phase, structural, variation, motif);
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
