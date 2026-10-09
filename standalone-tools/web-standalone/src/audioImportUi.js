import { COL } from "./firmware/const.js";
import {
  LANES, ASSIGN_CHS, laneEvents, laneChannel, laneRoot, isSampleCh, defaultSettings, quantize, refitGrid,
  autoLaneOffsets, beatTime, nearestBeat, barBeatAtOrAfter,
} from "./firmware/audioImport.js";
import { decodeStem, renderCut, encodeWav, silentWav, MAX_SECONDS, WARN_BYTES, SAMPLE_RATE } from "./firmware/sampleCut.js";

const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
const noteName = (p) => `${NOTE_NAMES[((p % 12) + 12) % 12]}${Math.floor(p / 12) - 1}`;

const $ = (id) => document.querySelector(id);
const POLL_MS = 700;

/**
 * Sidebar "Import Audio": drop an audio file, let the local analyzer split and
 * transcribe it, then shape timing live in the sim and push pattern + samples.
 *
 * @param {object} deps
 * @param {object} deps.device  sim device (importMidi, loadImportVoices, onStep, ...)
 * @param {(slot:number, samples:Record<number,Uint8Array>, status:(t:string)=>void) => Promise<string>} deps.push
 */
export function createAudioImport({ device, push }) {
  const el = {
    drop: $("#audio-drop"), file: $("#audio-file"), dropLabel: $("#audio-drop-label"),
    progress: $("#audio-progress"), fill: $("#audio-progress-fill"), progressText: $("#audio-progress-text"),
    settings: $("#audio-settings"), meta: $("#audio-meta"), err: $("#audio-err"),
    overview: $("#audio-overview"), zoom: $("#audio-zoom"), startReadout: $("#audio-start-readout"),
    mode: $("#audio-mode"), bars: $("#audio-bars"), bpm: $("#audio-bpm"),
    half: $("#audio-half"), double: $("#audio-double"), refit: $("#audio-refit"),
    nudge: $("#audio-nudge"), nudgeReadout: $("#audio-nudge-readout"),
    tol: $("#audio-tol"), tolReadout: $("#audio-tol-readout"),
    lanes: $("#audio-lanes"), autoalign: $("#audio-autoalign"),
    ref: $("#audio-ref"), refReadout: $("#audio-ref-readout"), play: $("#audio-play"),
    slot: $("#audio-slot"), reset: $("#audio-reset"), push: $("#audio-push"), xfer: $("#audio-xfer"),
    ed: $("#audio-sample-ed"),
  };

  /** @type {null | {hash:string, name:string, analysis:any, st:any, q:any, source:AudioBuffer|null, samples:Record<number,Uint8Array>, peaks:any}} */
  let cur = null;
  let applyTimer = 0;
  let zoomBar = 0;
  let busy = false;

  function setErr(msg) {
    el.err.hidden = !msg;
    el.err.textContent = msg || "";
  }

  function setProgress(pct, text) {
    el.progress.hidden = pct == null;
    if (pct == null) return;
    el.fill.style.width = `${Math.max(2, Math.min(100, pct))}%`;
    el.progressText.textContent = text || "";
  }

  // --- loading ---------------------------------------------------------------

  async function api(path, opts) {
    const res = await fetch(`/api/audio/${path}`, opts);
    if (!res.ok) throw new Error(`${path}: HTTP ${res.status}`);
    return res;
  }

  async function analyze(file) {
    try {
      await api("ping");
    } catch {
      throw new Error("Audio import needs the local simulator server (npm run dev) next to audio_import/ with its Python venv.");
    }
    setProgress(1, "Uploading…");
    let st = await (await api(`analyze?name=${encodeURIComponent(file.name)}`, { method: "POST", body: file })).json();
    while (st.state === "running") {
      setProgress(st.pct, `${st.msg || "Analyzing"}… ${st.pct || 0}%`);
      await new Promise((r) => setTimeout(r, POLL_MS));
      st = { ...st, ...(await (await api(`status/${st.hash}`)).json()) };
    }
    if (st.state !== "done") throw new Error(st.error || "Analysis failed");
    return st.hash;
  }

  async function loadFile(file) {
    if (!file || busy) return;
    busy = true;
    setErr("");
    el.xfer.textContent = "";
    el.dropLabel.textContent = file.name;
    try {
      const ctx = device.audioContext();
      const sourceJob = file.arrayBuffer().then((b) => ctx.decodeAudioData(b)).catch(() => null);
      const hash = await analyze(file);
      setProgress(100, "Loading samples…");
      const analysis = await (await api(`file/${hash}/analysis.json`)).json();
      const laneSamples = {};
      for (const [id, info] of Object.entries(analysis.samples || {})) {
        const bytes = new Uint8Array(await (await api(`file/${hash}/${info.file}`)).arrayBuffer());
        laneSamples[id] = { bytes, buffer: await ctx.decodeAudioData(bytes.slice().buffer) };
      }
      const source = await sourceJob;
      cur = { hash, name: file.name, analysis, laneSamples, autoSamples: { ...laneSamples }, source, peaks: null, q: null, st: restore(hash, analysis) };
      closeEditor();
      for (const lane of LANES) {
        if (cur.st.lanes[lane.id]?.cut?.start != null) await recut(lane);
      }
      applyVoices(true);
      zoomBar = 0;
      setProgress(null);
      el.settings.hidden = false;
      renderLanes();
      syncControls();
      apply(true);
    } catch (err) {
      setProgress(null);
      setErr(err?.message || String(err));
    } finally {
      busy = false;
    }
  }

  // --- per-lane sample editor (cut from the stem; auto cut is the default) ----

  const DEFAULT_STEM = { kick: "drums", snare: "drums", hat: "drums", perc: "drums", bass: "bass", lead: "other", vocal: "vocals" };
  /** @type {null | {id:string, lane:any, stem:any, view:[number,number], cut:{start:number,dur:number,root:number|null}, drag:any}} */
  let ed = null;

  function autoCut(id) {
    const a = cur.analysis.samples?.[id];
    return a ? { start: a.start, dur: a.dur, root: a.root ?? null } : null;
  }

  function effectiveCut(id) {
    const auto = autoCut(id);
    const user = cur.st.lanes[id]?.cut;
    return { ...auto, ...(user || {}) };
  }

  function stemUrl(id) {
    const stem = cur.analysis.samples?.[id]?.stem || DEFAULT_STEM[id];
    return `/api/audio/file/${cur.hash}/stems/${stem}.wav`;
  }

  /** Re-render a lane's sample from its stem using the user cut. */
  async function recut(lane) {
    const cut = effectiveCut(lane.id);
    if (cut.start == null) return;
    const stem = await decodeStem(stemUrl(lane.id));
    const clip = renderCut(stem, cut.start, cut.dur);
    const buffer = device.audioContext().createBuffer(1, clip.length, SAMPLE_RATE);
    buffer.copyToChannel(clip, 0);
    cur.laneSamples[lane.id] = { bytes: encodeWav(clip), buffer };
  }

  function closeEditor() {
    ed = null;
    el.ed.hidden = true;
    el.ed.innerHTML = "";
  }

  async function openEditor(id) {
    if (!cur) return;
    if (ed?.id === id) return closeEditor();
    const lane = LANES.find((l) => l.id === id);
    const cut = effectiveCut(id);
    const tonal = lane.kind !== "drum";
    el.ed.hidden = false;
    el.ed.innerHTML = `<div class="audio-ed-head"><strong>${lane.label} sample</strong><span class="jump-readout" data-info>Loading stem…</span><button type="button" data-close aria-label="Close sample editor">×</button></div>
      <canvas class="audio-ed-wave" data-wave width="600" height="90" aria-label="Stem waveform: drag the edges to trim, drag the middle to move, click to place"></canvas>
      <div class="audio-ed-row">
        <button type="button" data-prev title="Previous ${tonal ? "note" : "hit"} in the stem">◀ ${tonal ? "note" : "hit"}</button>
        <button type="button" data-next title="Next ${tonal ? "note" : "hit"} in the stem">${tonal ? "note" : "hit"} ▶</button>
        <button type="button" data-preview>▶ Play</button>
        <button type="button" data-auto title="Back to the automatic cut">Auto</button>
      </div>
      <div class="audio-ed-row">
        <label>Start <input type="number" data-start step="0.001" min="0"> s</label>
        <label>Length <input type="number" data-len step="5" min="10" max="${MAX_SECONDS * 1000}"> ms</label>
        ${tonal ? `<label>Root <input type="number" data-root step="1" min="0" max="127"> <span data-rootname></span></label>` : ""}
      </div>`;
    ed = { id, lane, stem: null, view: [0, 1], cut: { start: cut.start ?? 0, dur: cut.dur ?? 0.4, root: cut.root ?? null }, drag: null };
    centerView();
    syncEditor();
    const q = (sel) => el.ed.querySelector(sel);
    q("[data-close]").addEventListener("click", closeEditor);
    q("[data-preview]").addEventListener("click", previewSample);
    q("[data-auto]").addEventListener("click", revertAuto);
    q("[data-prev]").addEventListener("click", () => jumpHit(-1));
    q("[data-next]").addEventListener("click", () => jumpHit(1));
    q("[data-start]").addEventListener("change", (e) => { ed.cut.start = Math.max(0, Number(e.target.value) || 0); centerView(); commitCut(); });
    q("[data-len]").addEventListener("change", (e) => { ed.cut.dur = clampDur((Number(e.target.value) || 0) / 1000); centerView(); commitCut(); });
    q("[data-root]")?.addEventListener("change", (e) => { ed.cut.root = Math.max(0, Math.min(127, Math.round(Number(e.target.value)))); commitCut(true); });
    el.ed.querySelectorAll("input[type=number]").forEach((inp) => inp.addEventListener("keydown", (e) => {
      if (e.key === "Enter") inp.blur();
    }));
    const wave = q("[data-wave]");
    wave.addEventListener("pointerdown", onWaveDown);
    wave.addEventListener("pointermove", onWaveMove);
    wave.addEventListener("pointerup", onWaveUp);
    wave.addEventListener("pointercancel", onWaveUp);
    try {
      const stem = await decodeStem(stemUrl(id));
      if (ed?.id !== id) return;
      ed.stem = stem;
      syncEditor();
    } catch (err) {
      setErr(err?.message || String(err));
    }
  }

  const clampDur = (d) => Math.max(0.01, Math.min(MAX_SECONDS, d));

  function centerView() {
    const pad = Math.max(0.25, ed.cut.dur * 0.6);
    ed.view = [Math.max(0, ed.cut.start - pad), ed.cut.start + ed.cut.dur + pad];
  }

  function syncEditor() {
    if (!ed) return;
    const q = (sel) => el.ed.querySelector(sel);
    const put = (inp, v) => { if (inp && inp !== document.activeElement) inp.value = v; };
    put(q("[data-start]"), ed.cut.start.toFixed(3));
    put(q("[data-len]"), String(Math.round(ed.cut.dur * 1000)));
    if (q("[data-root]")) {
      put(q("[data-root]"), ed.cut.root == null ? "" : String(ed.cut.root));
      q("[data-rootname]").textContent = ed.cut.root == null ? "" : noteName(ed.cut.root);
    }
    const smp = cur.laneSamples[ed.id];
    const kb = smp ? smp.bytes.length / 1024 : 0;
    const custom = !!cur.st.lanes[ed.id]?.cut;
    const info = q("[data-info]");
    info.textContent = !ed.stem ? "Loading stem…" : `${custom ? "custom" : "auto"} · ${kb.toFixed(0)} KB${kb * 1024 > WARN_BYTES ? " · large, SD upload may be slow" : ""}`;
    info.classList.toggle("warn", kb * 1024 > WARN_BYTES);
    drawEditorWave();
  }

  function drawEditorWave() {
    const cv = el.ed.querySelector("[data-wave]");
    if (!cv) return;
    const w = (cv.width = cv.clientWidth * devicePixelRatio || 600);
    const h = (cv.height = 90 * devicePixelRatio);
    const g = cv.getContext("2d");
    g.fillStyle = "#111";
    g.fillRect(0, 0, w, h);
    const [v0, v1] = ed.view;
    const x = (t) => ((t - v0) / (v1 - v0)) * w;
    const [r, gg, b] = COL[laneChannel(cur.st, ed.lane)];
    g.fillStyle = `rgba(${r},${gg},${b},0.22)`;
    g.fillRect(x(ed.cut.start), 0, x(ed.cut.start + ed.cut.dur) - x(ed.cut.start), h);
    if (ed.stem) {
      const { data, sr } = ed.stem;
      const cols = [];
      let peak = 1e-4;
      for (let px = 0; px < w; px++) {
        const a = Math.max(0, Math.floor((v0 + (px / w) * (v1 - v0)) * sr));
        const z = Math.min(data.length, Math.floor((v0 + ((px + 1) / w) * (v1 - v0)) * sr));
        let lo = 0, hi = 0;
        for (let i = a; i < z; i++) { const v = data[i]; if (v < lo) lo = v; if (v > hi) hi = v; }
        cols.push([lo, hi]);
        peak = Math.max(peak, hi, -lo);
      }
      const k = (h * 0.48) / peak;
      g.fillStyle = "#9aa";
      cols.forEach(([lo, hi], px) => g.fillRect(px, h / 2 - hi * k, 1, Math.max(1, (hi - lo) * k)));
    }
    g.fillStyle = "#fc6";
    for (const e of laneEvents(cur.analysis, ed.lane)) {
      if (e.t < v0 || e.t > v1) continue;
      g.fillRect(x(e.t) - 1, 0, 2, 7 * devicePixelRatio);
    }
    g.fillStyle = `rgb(${r},${gg},${b})`;
    for (const t of [ed.cut.start, ed.cut.start + ed.cut.dur]) g.fillRect(x(t) - devicePixelRatio, 0, 2 * devicePixelRatio, h);
  }

  function waveTime(e) {
    const rect = e.currentTarget.getBoundingClientRect();
    const [v0, v1] = ed.view;
    return { t: v0 + ((e.clientX - rect.left) / rect.width) * (v1 - v0), pxPerS: rect.width / (v1 - v0) };
  }

  function onWaveDown(e) {
    if (!ed) return;
    const { t, pxPerS } = waveTime(e);
    const end = ed.cut.start + ed.cut.dur;
    const near = 6 / pxPerS;
    let mode = "move";
    if (Math.abs(t - ed.cut.start) < near) mode = "start";
    else if (Math.abs(t - end) < near) mode = "end";
    else if (t < ed.cut.start || t > end) ed.cut.start = Math.max(0, t);
    ed.drag = { mode, grab: t - ed.cut.start, end };
    e.currentTarget.setPointerCapture(e.pointerId);
    syncEditor();
  }

  function onWaveMove(e) {
    if (!ed?.drag) return;
    const { t } = waveTime(e);
    const d = ed.drag;
    if (d.mode === "start") {
      ed.cut.start = Math.max(0, Math.min(d.end - 0.01, t));
      ed.cut.dur = clampDur(d.end - ed.cut.start);
    } else if (d.mode === "end") {
      ed.cut.dur = clampDur(t - ed.cut.start);
    } else {
      ed.cut.start = Math.max(0, t - d.grab);
    }
    syncEditor();
  }

  function onWaveUp() {
    if (!ed?.drag) return;
    ed.drag = null;
    centerView();
    commitCut();
  }

  function jumpHit(dir) {
    const tonal = ed.lane.kind !== "drum";
    const pre = tonal ? 0.01 : 0.006;
    const onset = ed.cut.start + pre;
    const evs = laneEvents(cur.analysis, ed.lane).slice().sort((a, b) => a.t - b.t);
    const e = dir > 0 ? evs.find((x) => x.t > onset + 0.03) : evs.reverse().find((x) => x.t < onset - 0.03);
    if (!e) return;
    ed.cut.start = Math.max(0, e.t - pre);
    if (tonal && e.p != null) ed.cut.root = e.p;
    centerView();
    commitCut(tonal);
  }

  async function commitCut(rootChanged = false) {
    if (!ed) return;
    const { id, lane } = ed;
    cur.st.lanes[id].cut = { start: +ed.cut.start.toFixed(5), dur: +ed.cut.dur.toFixed(4), root: ed.cut.root };
    save();
    syncEditor();
    try {
      await recut(lane);
    } catch (err) {
      setErr(err?.message || String(err));
      return;
    }
    applyVoices();
    if (rootChanged) scheduleApply();
    syncEditor();
    previewSample();
  }

  function revertAuto() {
    if (!ed) return;
    const { id } = ed;
    cur.st.lanes[id].cut = null;
    if (cur.autoSamples[id]) cur.laneSamples[id] = cur.autoSamples[id];
    else delete cur.laneSamples[id];
    const auto = autoCut(id);
    if (auto) ed.cut = { ...auto };
    centerView();
    applyVoices();
    scheduleApply();
    syncEditor();
  }

  let previewNode = null;
  function previewSample() {
    const smp = ed && cur.laneSamples[ed.id];
    if (!smp) return;
    const ctx = device.audioContext();
    try { previewNode?.stop(); } catch { /* already stopped */ }
    previewNode = ctx.createBufferSource();
    previewNode.buffer = smp.buffer;
    previewNode.connect(ctx.destination);
    previewNode.start();
  }

  // --- settings persistence (so tweaks survive reloads; Reset = undo all) ----

  function restore(hash, analysis) {
    const fresh = defaultSettings(analysis);
    try {
      const saved = JSON.parse(localStorage.getItem(`toern-audio-${hash}`) || "null");
      if (saved?.grid) return { ...fresh, ...saved, lanes: { ...fresh.lanes, ...saved.lanes } };
    } catch { /* ignore */ }
    return fresh;
  }

  function save() {
    if (!cur) return;
    try { localStorage.setItem(`toern-audio-${cur.hash}`, JSON.stringify(cur.st)); } catch { /* quota */ }
  }

  // --- quantize + apply -------------------------------------------------------

  function apply(first = false) {
    if (!cur) return;
    const { analysis, st } = cur;
    cur.q = quantize(analysis, st);
    device.importMidi(cur.q.cells, cur.q.bpm, { exactBpm: true, keepTransport: !first });
    save();
    syncReadouts();
    drawOverview();
    drawZoom();
  }

  function scheduleApply() {
    clearTimeout(applyTimer);
    applyTimer = setTimeout(() => apply(), 40);
  }

  // --- controls ---------------------------------------------------------------

  function renderLanes() {
    el.lanes.innerHTML = `<thead><tr><th></th><th>Lane</th><th>Voice</th><th>Offset</th><th>Hits · error</th><th></th></tr></thead><tbody>${LANES.map((l) => `<tr data-lane="${l.id}">
        <td><input type="checkbox" data-on aria-label="Use ${l.label}"></td>
        <td><span class="audio-swatch" data-swatch></span>${l.label}</td>
        <td><select data-ch aria-label="${l.label} voice">${ASSIGN_CHS.map((ch) => `<option value="${ch}">CH${ch}</option>`).join("")}</select></td>
        <td><input type="range" data-off min="-80" max="80" step="1" aria-label="${l.label} timing offset"><span class="jump-readout" data-offv></span></td>
        <td class="jump-readout" data-stat></td>
        <td><button type="button" data-edit aria-label="Edit ${l.label} sample" title="Edit sample">✎</button></td>
      </tr>`).join("")}</tbody>`;
    el.lanes.querySelectorAll("tr[data-lane]").forEach((tr) => {
      const id = tr.dataset.lane;
      tr.querySelector("[data-on]").addEventListener("change", (e) => {
        cur.st.lanes[id].on = e.target.checked;
        scheduleApply();
      });
      tr.querySelector("[data-ch]").addEventListener("change", (e) => assignChannel(id, Number(e.target.value)));
      tr.querySelector("[data-off]").addEventListener("input", (e) => {
        cur.st.lanes[id].offsetMs = Number(e.target.value);
        syncReadouts();
        scheduleApply();
      });
      tr.querySelector("[data-off]").addEventListener("dblclick", (e) => {
        e.target.value = "0";
        cur.st.lanes[id].offsetMs = 0;
        scheduleApply();
      });
      tr.querySelector("[data-edit]").addEventListener("click", () => openEditor(id));
    });
  }

  /** One lane per voice: taking an occupied voice swaps the two lanes. */
  function assignChannel(id, ch) {
    const lanes = cur.st.lanes;
    const prev = lanes[id].ch;
    const other = LANES.find((l) => l.id !== id && lanes[l.id].ch === ch);
    if (other) lanes[other.id].ch = prev;
    lanes[id].ch = ch;
    syncControls();
    applyVoices();
    scheduleApply();
  }

  /** Load each sample voice with the sample of the lane assigned to it. */
  function applyVoices(resetSettings = false) {
    const ctx = device.audioContext();
    const buffers = {};
    const names = {};
    for (let ch = 1; ch <= 8; ch++) {
      const lane = LANES.find((l) => laneChannel(cur.st, l) === ch);
      const smp = lane && cur.laneSamples[lane.id];
      buffers[ch] = smp ? smp.buffer : ctx.createBuffer(1, 64, SAMPLE_RATE);
      names[ch] = lane ? lane.label : "EMPTY";
    }
    device.loadImportVoices(buffers, names, { resetSettings });
  }

  function pushSamples() {
    const out = {};
    for (let ch = 1; ch <= 8; ch++) {
      const lane = LANES.find((l) => laneChannel(cur.st, l) === ch);
      out[ch] = (lane && cur.laneSamples[lane.id]?.bytes) || silentWav();
    }
    return out;
  }

  function syncControls() {
    const { st } = cur;
    el.mode.value = st.mode;
    el.bars.value = String(st.bars);
    el.bpm.value = st.grid.bpm.toFixed(2);
    el.nudge.value = String(st.nudgeMs);
    el.tol.value = String(st.tolerance);
    el.lanes.querySelectorAll("tr[data-lane]").forEach((tr) => {
      const cfg = st.lanes[tr.dataset.lane];
      const ch = laneChannel(st, LANES.find((l) => l.id === tr.dataset.lane));
      const [r, g, b] = COL[ch];
      tr.querySelector("[data-on]").checked = !!cfg.on;
      tr.querySelector("[data-off]").value = String(cfg.offsetMs || 0);
      tr.querySelector("[data-ch]").value = String(ch);
      tr.querySelector("[data-swatch]").style.background = `rgb(${r},${g},${b})`;
    });
  }

  function fmtMs(v) {
    const r = Math.round(v);
    return `${r > 0 ? "+" : ""}${r} ms`;
  }

  function syncReadouts() {
    if (!cur) return;
    const { st, q, analysis } = cur;
    const shift = st.stepShift ? ` +${st.stepShift}/16` : "";
    el.startReadout.textContent = `${q.times[0].toFixed(2)}s · beat ${st.startBeat + 1}${shift}`;
    el.nudgeReadout.textContent = fmtMs(st.nudgeMs);
    el.tolReadout.textContent = st.tolerance >= 0.5 ? "keep all" : `±${Math.round(st.tolerance * 100)}% of a step`;
    const lock = Math.round((st.grid.lock || 0) * 100);
    el.meta.textContent = `${cur.name} · ${analysis.duration.toFixed(0)}s · fit ${st.grid.bpm.toFixed(2)} BPM (detected ${analysis.bpm.toFixed(1)}) · grid lock ${lock}% · ${q.cells.length} notes`;
    el.lanes.querySelectorAll("tr[data-lane]").forEach((tr) => {
      const id = tr.dataset.lane;
      const s = q.stats[id];
      const lane = LANES.find((l) => l.id === id);
      const missing = lane.kind !== "drum" && isSampleCh(laneChannel(st, lane)) && laneRoot(analysis, st, lane) == null;
      tr.querySelector("[data-edit]").disabled = !cur.laneSamples[id] && !analysis.samples?.[id];
      tr.querySelector("[data-offv]").textContent = fmtMs(st.lanes[id].offsetMs || 0);
      tr.querySelector("[data-stat]").textContent = missing ? "no sample" : s ? `${s.kept}/${s.found} · ±${Math.round(s.meanAbsMs)}` : "";
      tr.classList.toggle("dim", !st.lanes[id].on);
    });
    const v = Number(el.ref.value);
    el.refReadout.textContent = v ? `${v}%` : "off";
  }

  function moveStart(deltaSteps) {
    if (!cur) return;
    const total = Math.max(0, cur.st.startBeat * 4 + (cur.st.stepShift || 0) + deltaSteps);
    cur.st.startBeat = Math.floor(total / 4);
    cur.st.stepShift = total % 4;
    scheduleApply();
  }

  function setBpm(bpm, search) {
    if (!cur || !(bpm >= 40 && bpm <= 240)) return;
    const t0 = beatTime(cur.st.grid, cur.st.startBeat);
    cur.st.grid = refitGrid(cur.analysis, bpm, search);
    cur.st.startBeat = nearestBeat(cur.st.grid, t0);
    el.bpm.value = cur.st.grid.bpm.toFixed(2);
    scheduleApply();
  }

  el.drop.addEventListener("click", (e) => {
    if (e.target === el.file) return;
    e.preventDefault();
    el.file.click();
  });
  el.file.addEventListener("change", () => {
    const f = el.file.files?.[0];
    if (f) loadFile(f);
    el.file.value = "";
  });
  ["dragenter", "dragover"].forEach((t) => el.drop.addEventListener(t, (e) => { e.preventDefault(); el.drop.classList.add("on"); }));
  ["dragleave", "drop"].forEach((t) => el.drop.addEventListener(t, (e) => { e.preventDefault(); el.drop.classList.remove("on"); }));
  el.drop.addEventListener("drop", (e) => {
    const f = [...(e.dataTransfer?.files || [])].find((x) => /^audio\//.test(x.type) || /\.(mp3|wav|flac|m4a|ogg|aiff?)$/i.test(x.name));
    if (f) loadFile(f);
    else setErr("Drop an audio file (.mp3, .wav, …).");
  });

  document.querySelectorAll(".audio-start [data-start]").forEach((b) => {
    b.addEventListener("click", () => moveStart(Number(b.dataset.start)));
  });
  el.mode.addEventListener("change", () => { cur.st.mode = el.mode.value; scheduleApply(); });
  el.bars.addEventListener("change", () => { cur.st.bars = Number(el.bars.value); zoomBar = 0; scheduleApply(); });
  el.bpm.addEventListener("change", () => setBpm(Number(el.bpm.value), false));
  el.half.addEventListener("click", () => setBpm(cur.st.grid.bpm / 2, false));
  el.double.addEventListener("click", () => setBpm(cur.st.grid.bpm * 2, false));
  el.refit.addEventListener("click", () => setBpm(Number(el.bpm.value) || cur.st.grid.bpm, true));
  el.nudge.addEventListener("input", () => { cur.st.nudgeMs = Number(el.nudge.value); syncReadouts(); scheduleApply(); });
  el.nudge.addEventListener("dblclick", () => { el.nudge.value = "0"; cur.st.nudgeMs = 0; scheduleApply(); });
  el.tol.addEventListener("input", () => { cur.st.tolerance = Number(el.tol.value); syncReadouts(); scheduleApply(); });
  el.autoalign.addEventListener("click", () => {
    if (!cur) return;
    const offs = autoLaneOffsets(cur.analysis, cur.st);
    for (const id of Object.keys(offs)) cur.st.lanes[id].offsetMs = offs[id];
    syncControls();
    scheduleApply();
  });
  el.reset.addEventListener("click", () => {
    if (!cur) return;
    cur.st = defaultSettings(cur.analysis);
    cur.laneSamples = { ...cur.autoSamples };
    zoomBar = 0;
    closeEditor();
    syncControls();
    applyVoices();
    apply();
  });
  el.ref.addEventListener("input", () => {
    syncReadouts();
    if (ref.gain) ref.gain.gain.value = refLevel();
  });
  el.play.addEventListener("click", () => device.togglePlay());
  el.push.addEventListener("click", async () => {
    if (!cur || busy) return;
    const slot = Math.round(Number(el.slot.value));
    if (!(slot >= 1 && slot <= 999)) { setErr("Slot must be 1–999."); return; }
    busy = true;
    el.push.disabled = true;
    setErr("");
    try {
      const line = await push(slot, pushSamples(), (t) => { el.xfer.textContent = t; });
      el.xfer.textContent = `Saved on device: ${line.replace(/^OK\s*/, "")}`;
    } catch (err) {
      if (err?.name !== "NotFoundError") setErr(err?.message || String(err));
      el.xfer.textContent = "";
    } finally {
      busy = false;
      el.push.disabled = false;
    }
  });

  // --- original-track A/B playback -------------------------------------------

  const ref = { src: null, gain: null, at: 0, offset: 0 };

  function refLevel() {
    return (Number(el.ref.value) / 100) ** 2;
  }

  function stopRef(when) {
    if (!ref.src) return;
    const ctx = device.audioContext();
    const t = Math.max(ctx.currentTime, when || 0);
    try {
      ref.gain.gain.setTargetAtTime(0, t, 0.004);
      ref.src.stop(t + 0.03);
    } catch { /* stopped */ }
    ref.src = null;
  }

  function startRef(when, offset) {
    const ctx = device.audioContext();
    stopRef(when);
    const src = ctx.createBufferSource();
    src.buffer = cur.source;
    const gain = ctx.createGain();
    gain.gain.setValueAtTime(0, when);
    gain.gain.linearRampToValueAtTime(refLevel(), when + 0.004);
    src.connect(gain);
    gain.connect(ctx.destination);
    src.start(when, Math.max(0, offset));
    Object.assign(ref, { src, gain, at: when, offset });
  }

  device.onStep((step, when) => {
    if (!cur?.q || !cur.source || !refLevel()) { stopRef(when); return; }
    const times = cur.q.times;
    const steps = times.length - 1;
    if (step > steps) { stopRef(when); return; }
    const want = times[step - 1];
    if (ref.src) {
      if ((step - 1) % 16 !== 0) return;
      const playing = ref.offset + (when - ref.at);
      if (Math.abs(playing - want) < 0.008) return;
    }
    startRef(when, want);
  });
  device.onTransport((playing) => { if (!playing) stopRef(); });

  // --- drawing ----------------------------------------------------------------

  function fitCanvas(c) {
    const dpr = window.devicePixelRatio || 1;
    const w = Math.max(50, Math.round(c.clientWidth * dpr));
    const h = Math.max(20, Math.round(c.clientHeight * dpr));
    if (c.width !== w || c.height !== h) { c.width = w; c.height = h; }
    return { g: c.getContext("2d"), w, h, dpr };
  }

  function drawWave(g, buf, t0, t1, w, h, color) {
    if (!buf) return;
    const data = buf.getChannelData(0);
    const sr = buf.sampleRate;
    g.fillStyle = color;
    for (let x = 0; x < w; x++) {
      const a = Math.max(0, Math.floor((t0 + ((t1 - t0) * x) / w) * sr));
      const b = Math.min(data.length, Math.floor((t0 + ((t1 - t0) * (x + 1)) / w) * sr));
      let mx = 0;
      const stride = Math.max(1, Math.floor((b - a) / 400));
      for (let i = a; i < b; i += stride) { const v = Math.abs(data[i]); if (v > mx) mx = v; }
      const y = mx * h * 0.48;
      g.fillRect(x, h / 2 - y, 1, Math.max(1, y * 2));
    }
  }

  function drawOverview() {
    if (!cur) return;
    const { g, w, h } = fitCanvas(el.overview);
    const dur = cur.analysis.duration;
    g.clearRect(0, 0, w, h);
    g.fillStyle = "#f4f4f2";
    g.fillRect(0, 0, w, h);
    if (!cur.peaks || cur.peaks.w !== w) {
      const off = new OffscreenCanvas(w, h);
      drawWave(off.getContext("2d"), cur.source, 0, dur, w, h, "#9a9ca3");
      cur.peaks = { w, img: off };
    }
    g.drawImage(cur.peaks.img, 0, 0);
    const times = cur.q.times;
    const x0 = (times[0] / dur) * w;
    const x1 = (times[times.length - 1] / dur) * w;
    g.fillStyle = "rgba(255,59,48,0.16)";
    g.fillRect(x0, 0, x1 - x0, h);
    g.fillStyle = "rgba(255,59,48,0.9)";
    g.fillRect(x0, 0, 2, h);
    g.fillRect(x1 - 1, 0, 1, h);
  }

  function drawZoom() {
    if (!cur) return;
    const { g, w, h, dpr } = fitCanvas(el.zoom);
    const times = cur.q.times;
    const steps = times.length - 1;
    zoomBar = Math.max(0, Math.min(steps / 16 - 1, zoomBar));
    const s0 = zoomBar * 16;
    const s1 = Math.min(steps, s0 + 16);
    const stepLen = times[1] - times[0];
    const t0 = times[s0] - stepLen * 2;
    const t1 = times[s1] + stepLen * 0.5;
    const X = (t) => ((t - t0) / (t1 - t0)) * w;
    g.clearRect(0, 0, w, h);
    g.fillStyle = "#fbfbfa";
    g.fillRect(0, 0, w, h);
    const waveH = h * 0.42;
    drawWave(g, cur.source, t0, t1, w, waveH, "#c4c5ca");
    for (let s = s0; s <= s1; s++) {
      const x = Math.round(X(times[s]));
      const isBar = s % 16 === 0;
      const isBeat = s % 4 === 0;
      g.fillStyle = isBar ? "rgba(255,59,48,0.85)" : isBeat ? "rgba(29,32,39,0.45)" : "rgba(29,32,39,0.12)";
      g.fillRect(x, 0, isBar ? 2 : 1, h);
      if (isBar) {
        g.fillStyle = "#ff3b30";
        g.font = `${10 * dpr}px ui-monospace, Menlo, monospace`;
        g.fillText(String(s / 16 + 1), x + 3 * dpr, 10 * dpr);
      }
    }
    const lanes = LANES.filter((l) => cur.st.lanes[l.id]?.on);
    const rowH = (h - waveH) / Math.max(1, lanes.length);
    lanes.forEach((lane, i) => {
      const [r, gg, b] = COL[laneChannel(cur.st, lane)];
      const off = (cur.st.lanes[lane.id].offsetMs || 0) / 1000;
      const y = waveH + i * rowH;
      g.fillStyle = `rgb(${r},${gg},${b})`;
      for (const e of laneEvents(cur.analysis, lane)) {
        const t = e.t + off;
        if (t < t0 || t > t1) continue;
        const x = X(t);
        g.fillRect(x - dpr, y + rowH * 0.15, 2 * dpr, rowH * 0.7);
      }
    });
  }

  el.overview.addEventListener("click", (e) => {
    if (!cur) return;
    const rect = el.overview.getBoundingClientRect();
    const t = ((e.clientX - rect.left) / rect.width) * cur.analysis.duration;
    const k = nearestBeat(cur.st.grid, t);
    cur.st.startBeat = barBeatAtOrAfter(cur.st.grid, Math.max(0, k - 2));
    cur.st.stepShift = 0;
    zoomBar = 0;
    scheduleApply();
  });

  let drag = null;
  el.zoom.addEventListener("pointerdown", (e) => {
    if (!cur) return;
    el.zoom.setPointerCapture(e.pointerId);
    const times = cur.q.times;
    const s0 = zoomBar * 16;
    const span = times[Math.min(times.length - 1, s0 + 16)] - times[s0] + (times[1] - times[0]) * 2.5;
    // Fine control: the waveform moves at a third of the pointer speed.
    drag = { x: e.clientX, nudge: cur.st.nudgeMs, msPerPx: (span * 1000) / el.zoom.getBoundingClientRect().width / 3 };
  });
  el.zoom.addEventListener("pointermove", (e) => {
    if (!drag) return;
    const v = Math.round(Math.max(-120, Math.min(120, drag.nudge - (e.clientX - drag.x) * drag.msPerPx)));
    if (v === cur.st.nudgeMs) return;
    cur.st.nudgeMs = v;
    el.nudge.value = String(v);
    syncReadouts();
    scheduleApply();
  });
  el.zoom.addEventListener("pointerup", () => { drag = null; });
  el.zoom.addEventListener("wheel", (e) => {
    if (!cur) return;
    e.preventDefault();
    zoomBar += Math.sign(e.deltaY || e.deltaX);
    drawZoom();
  }, { passive: false });
  window.addEventListener("resize", () => { if (cur) { drawOverview(); drawZoom(); } });

  return {
    redraw() { if (cur) { drawOverview(); drawZoom(); } },
    stop() { stopRef(); },
  };
}
