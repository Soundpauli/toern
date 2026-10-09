/** Audio → TŒRN pattern: tempo fit, grid and quantization over analysis.json
 *  produced by audio_import/toern_analyze.py. All times are seconds. */

export const STEPS = 256;
const NEUTRAL_STORED_PITCH = 72;
const TAU = Math.PI * 2;

export const LANES = [
  { id: "kick", label: "Kick", ch: 1, kind: "drum" },
  { id: "snare", label: "Snare", ch: 2, kind: "drum" },
  { id: "bass", label: "Bass", ch: 3, kind: "mono" },
  { id: "lead", label: "Lead/Keys", ch: 4, kind: "mono" },
  { id: "hat", label: "HiHat", ch: 5, kind: "drum" },
  { id: "perc", label: "Perc", ch: 6, kind: "drum", off: true },
  { id: "vocal", label: "Vocal", ch: 11, kind: "synth" },
];

export const ASSIGN_CHS = [1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14];

export function isSampleCh(ch) { return ch >= 1 && ch <= 8; }

/** Draw row a voice's notes sit on by default (firmware basePitchRowForChannel). */
function homeRow(ch) {
  if (isSampleCh(ch)) return ch + 1;
  if (ch === 11) return 12;
  return ch - 11;
}

/** Root pitch of the sample a lane plays (user override wins). */
export function laneRoot(analysis, st, lane) {
  const cut = st.lanes[lane.id]?.cut;
  if (cut && cut.root != null) return cut.root;
  return analysis.samples?.[lane.id]?.root ?? null;
}

export function laneChannel(st, lane) {
  return st.lanes[lane.id]?.ch ?? lane.ch;
}

export function laneEvents(analysis, lane) {
  if (lane.kind === "drum") return analysis.drums.filter((e) => e.lane === lane.id);
  return analysis[lane.id] || [];
}

function gridWeights(analysis) {
  const w = { kick: 1, snare: 1, hat: 0.5, perc: 0.5 };
  return analysis.drums.map((e) => ({ t: e.t, w: e.s * (w[e.lane] ?? 0.5), lane: e.lane }));
}

function coherence(points, period) {
  let re = 0, im = 0, sum = 0;
  const k = TAU / period;
  for (const p of points) {
    re += p.w * Math.cos(k * p.t);
    im += p.w * Math.sin(k * p.t);
    sum += p.w;
  }
  return { mag: sum ? Math.hypot(re, im) / sum : 0, phase: Math.atan2(im, re) };
}

/** Constant-tempo fit: BPM + 16th phase maximizing drum-onset alignment. */
export function fitTempo(analysis, bpmGuess, range = 0.04) {
  const pts = gridWeights(analysis);
  if (!pts.length) return { bpm: bpmGuess, phase: 0, lock: 0 };
  const scan = (lo, hi, step) => {
    let best = { bpm: bpmGuess, mag: -1, phase: 0 };
    for (let b = lo; b <= hi; b += step) {
      const c = coherence(pts, 15 / b);
      if (c.mag > best.mag) best = { bpm: b, mag: c.mag, phase: c.phase };
    }
    return best;
  };
  let best = scan(bpmGuess * (1 - range), bpmGuess * (1 + range), 0.004);
  best = scan(best.bpm - 0.006, best.bpm + 0.006, 0.0002);
  const P = 15 / best.bpm;
  const phase = ((best.phase / TAU) * P + P) % P;
  return { bpm: best.bpm, phase, lock: best.mag };
}

/** Pick which 16th is the beat and which beat is the bar's "1". */
export function fitBeatPhase(analysis, bpm, phase16) {
  const P = 15 / bpm;
  const kicks = analysis.drums.filter((e) => e.lane === "kick");
  const snares = analysis.drums.filter((e) => e.lane === "snare");
  const align = (evs, t0, period) => evs.reduce((a, e) => a + e.s * Math.cos((TAU * (e.t - t0)) / period), 0);
  // Beats carry the most drum energy overall; using every lane keeps this robust
  // when the kick/snare split is unsure. Hats count less (they often sit on 8ths).
  const weighted = gridWeights(analysis).map((p) => ({ t: p.t, s: p.lane === "hat" ? p.w * 0.5 : p.w }));
  let beat = phase16, bestScore = -Infinity;
  for (let k = 0; k < 4; k++) {
    const t0 = phase16 + k * P;
    const score = align(weighted, t0, 4 * P);
    if (score > bestScore) { bestScore = score; beat = t0; }
  }
  // Kick on 1/3 and snare on 2/4 decides the half-bar phase.
  let bar = beat; bestScore = -Infinity;
  for (let j = 0; j < 2; j++) {
    const t0 = beat + j * 4 * P;
    const score = align(kicks, t0, 8 * P) + align(snares, t0 + 4 * P, 8 * P);
    if (score > bestScore) { bestScore = score; bar = t0; }
  }
  return { beatPhase: beat % (4 * P), barPhase: bar % (16 * P) };
}

/** Time of beat index k on the fitted grid (k = 0 is the first beat ≥ 0 s). */
export function beatTime(grid, k) {
  return grid.beatPhase + k * (60 / grid.bpm);
}

export function nearestBeat(grid, t) {
  return Math.max(0, Math.round((t - grid.beatPhase) / (60 / grid.bpm)));
}

/** First beat index that is a bar "1" at or after beat k. */
export function barBeatAtOrAfter(grid, k) {
  const beatSec = 60 / grid.bpm;
  const offset = Math.round(((grid.barPhase - grid.beatPhase) / beatSec)) % 4;
  const rel = ((k - offset) % 4 + 4) % 4;
  return rel === 0 ? k : k + (4 - rel);
}

export function autoStartBeat(analysis, grid) {
  const k = nearestBeat(grid, analysis.autoStart ?? 0);
  const bar = barBeatAtOrAfter(grid, Math.max(0, k - 2));
  return bar;
}

/** Step boundary times for the pattern window. times.length = steps + 1. */
export function buildStepTimes(analysis, st) {
  const steps = st.bars * 16;
  const times = new Float64Array(steps + 1);
  const nudge = (st.nudgeMs || 0) / 1000 + (st.stepShift || 0) * (15 / st.grid.bpm);
  if (st.mode === "beats" && analysis.beats?.length > 1) {
    const beats = analysis.beats;
    const t0 = beatTime(st.grid, st.startBeat);
    let i0 = 0;
    for (let i = 1; i < beats.length; i++) if (Math.abs(beats[i] - t0) < Math.abs(beats[i0] - t0)) i0 = i;
    const local = beats.slice(i0, i0 + st.bars * 4 + 1);
    const ivals = [];
    for (let i = 1; i < local.length; i++) ivals.push(local[i] - local[i - 1]);
    ivals.sort((a, b) => a - b);
    const q = ivals.length ? ivals[ivals.length >> 1] : 60 / st.grid.bpm;
    while (local.length < st.bars * 4 + 1) local.push(local[local.length - 1] + q);
    for (let b = 0; b < st.bars * 4; b++) {
      const d = (local[b + 1] - local[b]) / 4;
      for (let s = 0; s < 4; s++) times[b * 4 + s] = local[b] + d * s + nudge;
    }
    times[steps] = local[st.bars * 4] + nudge;
  } else {
    const P = 15 / st.grid.bpm;
    const t0 = beatTime(st.grid, st.startBeat) + nudge;
    for (let i = 0; i <= steps; i++) times[i] = t0 + i * P;
  }
  return times;
}

/** Fractional step position of t on the grid (may be < 0 or ≥ steps). */
function stepPos(times, t) {
  const n = times.length - 1;
  if (t < times[0]) return (t - times[0]) / (times[1] - times[0]);
  if (t >= times[n]) return n + (t - times[n]) / (times[n] - times[n - 1]);
  let lo = 0, hi = n;
  while (hi - lo > 1) {
    const mid = (lo + hi) >> 1;
    if (times[mid] <= t) lo = mid; else hi = mid;
  }
  return lo + (t - times[lo]) / (times[lo + 1] - times[lo]);
}

function percentile(values, p) {
  if (!values.length) return 1;
  const s = [...values].sort((a, b) => a - b);
  return s[Math.min(s.length - 1, Math.floor(p * (s.length - 1)))];
}

function median(values) {
  if (!values.length) return 0;
  const s = [...values].sort((a, b) => a - b);
  return s[s.length >> 1];
}

/** Snap one lane. Returns kept hits plus timing stats. */
function snapLane(analysis, lane, times, cfg, tolerance) {
  const steps = times.length - 1;
  const off = (cfg.offsetMs || 0) / 1000;
  const evs = laneEvents(analysis, lane);
  const errs = [];
  const errMs = [];
  const byStep = new Map();
  for (const e of evs) {
    const u = stepPos(times, e.t + off);
    const step = Math.round(u);
    if (step < 0 || step >= steps) continue;
    const err = u - step;
    const stepSec = times[step + 1] - times[step];
    errs.push(err);
    errMs.push(err * stepSec * 1000);
    if (Math.abs(err) > tolerance) continue;
    const score = lane.kind === "drum" ? e.s : e.v * Math.sqrt(Math.max(0.04, e.d));
    const prev = byStep.get(step);
    if (!prev || score > prev.score) byStep.set(step, { step, e, score });
  }
  let hits = [...byStep.values()].sort((a, b) => a.step - b.step);
  if (lane.kind !== "drum") {
    const out = [];
    for (const h of hits) {
      const last = out[out.length - 1];
      if (last && last.e.p === h.e.p) {
        const P = times[h.step + 1] - times[h.step];
        if (h.e.t <= last.e.t + last.e.d + Math.max(0.06, P * 0.4)) continue;
      }
      out.push(h);
    }
    hits = out;
  }
  const stats = {
    found: errs.length,
    kept: hits.length,
    meanAbsMs: errMs.length ? errMs.reduce((a, b) => a + Math.abs(b), 0) / errMs.length : 0,
    medianMs: median(errMs),
    offGrid: errs.filter((x) => Math.abs(x) > 0.25).length,
  };
  return { hits, stats };
}

function foldRow(row) {
  while (row > 15) row -= 12;
  while (row < 1) row += 12;
  return row;
}

/** Build TŒRN cells for device.importMidi from analysis + UI settings. */
export function quantize(analysis, st) {
  const times = buildStepTimes(analysis, st);
  const steps = times.length - 1;
  const bpm = st.mode === "beats" ? 60 / ((times[steps] - times[0]) / (st.bars * 4)) : st.grid.bpm;
  const taken = new Set();
  const cells = [];
  const stats = {};
  const place = (step, row) => {
    for (let d = 0; d < 15; d++) {
      for (const r of d ? [row + d, row - d] : [row]) {
        if (r < 1 || r > 15) continue;
        const key = step * 16 + r;
        if (!taken.has(key)) { taken.add(key); return r; }
      }
    }
    return 0;
  };
  // Drums first so their home rows are always free.
  const ordered = [...LANES].sort((a, b) => (a.kind === "drum" ? 0 : 1) - (b.kind === "drum" ? 0 : 1));
  for (const lane of ordered) {
    const cfg = st.lanes[lane.id] || {};
    const { hits, stats: s } = snapLane(analysis, lane, times, cfg, st.tolerance ?? 0.5);
    stats[lane.id] = s;
    if (!cfg.on) continue;
    const ch = laneChannel(st, lane);
    const sampled = isSampleCh(ch);
    const drum = lane.kind === "drum";
    const root = laneRoot(analysis, st, lane);
    if (sampled && !drum && root == null) continue;
    const vmax = percentile(laneEvents(analysis, lane).map((e) => e.s ?? e.v), 0.95);
    for (const { step, e } of hits) {
      const velocity = drum
        ? Math.round(55 + 72 * Math.sqrt(Math.min(1, e.s / vmax)))
        : Math.max(40, Math.min(127, e.v));
      // Original pitch everywhere; the row is only where the note is drawn.
      // Sample voices store 72 + (pitch − sample root); synths store the absolute pitch.
      let midiPitch, row;
      if (drum) {
        midiPitch = sampled ? NEUTRAL_STORED_PITCH : 60;
        row = homeRow(ch);
      } else if (sampled) {
        midiPitch = Math.max(0, Math.min(127, NEUTRAL_STORED_PITCH + e.p - root));
        row = foldRow(homeRow(ch) + (e.p - root));
      } else {
        midiPitch = Math.max(0, Math.min(127, e.p));
        row = foldRow(homeRow(ch) + (e.p - 60));
      }
      const r = place(step, row);
      if (!r) continue;
      cells.push({ step: step + 1, row: r, channel: ch, velocity, midiPitch, probability: 100 });
    }
  }
  return { cells, stats, times, bpm };
}

/** Per-lane offsets that cancel each lane's median timing error. */
export function autoLaneOffsets(analysis, st) {
  const times = buildStepTimes(analysis, { ...st, nudgeMs: st.nudgeMs });
  const out = {};
  for (const lane of LANES) {
    const { stats } = snapLane(analysis, lane, times, { offsetMs: 0 }, 0.5);
    out[lane.id] = stats.found >= 4 && Math.abs(stats.medianMs) >= 3 ? -Math.round(stats.medianMs) : 0;
  }
  return out;
}

/** Initial settings for a freshly loaded analysis. */
export function defaultSettings(analysis) {
  const fit = fitTempo(analysis, analysis.bpm);
  const ph = fitBeatPhase(analysis, fit.bpm, fit.phase);
  const grid = { bpm: fit.bpm, lock: fit.lock, ...ph };
  const st = {
    mode: "fixed",
    grid,
    bars: 16,
    stepShift: 0,
    nudgeMs: 0,
    tolerance: 0.5,
    lanes: Object.fromEntries(LANES.map((l) => [l.id, { on: !l.off, offsetMs: 0, ch: l.ch, cut: null }])),
  };
  st.startBeat = autoStartBeat(analysis, grid);
  return st;
}

/** Refit phase (and optionally BPM) after the user changes tempo. */
export function refitGrid(analysis, bpm, searchBpm) {
  const fit = searchBpm ? fitTempo(analysis, bpm, 0.03) : (() => {
    const pts = gridWeights(analysis);
    const P = 15 / bpm;
    const c = coherence(pts, P);
    return { bpm, phase: ((c.phase / TAU) * P + P) % P, lock: c.mag };
  })();
  const ph = fitBeatPhase(analysis, fit.bpm, fit.phase);
  return { bpm: fit.bpm, lock: fit.lock, ...ph };
}
