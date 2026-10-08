import {
  COLS, COND_VALUE, DUMMY_PACKS, FILTER_CH, LONG_MS, PAGES, PAINT_ROWS, PROB, ROWS, SOUND_CH, STEPS, WAVS, emptyFilt, emptyNote, filterPagesFor, setLayout,
} from "./const.js";
import { createEngine } from "../audio/engine.js";
import { MENU_PAGES, SUBS, isSoon, renderFrame } from "./screens.js";
import { listDir, packWavs, parsePattern, patternUrl, sampleWavs } from "./library.js";
import { drawRandoms } from "./random.js";
import { GENRES, generateGenre } from "./genres.js";

function clamp(v, lo, hi) { return v < lo ? lo : v > hi ? hi : v; }

const SUB_VALUES = {
  look: [
    { opts: ["OFF", "ON"], colors: [[120, 0, 0], [0, 120, 0]], codes: ["R", "G"] },
    { opts: ["ON", "PRSS", "SYNC"], colors: [[0, 120, 0], [120, 120, 0], [0, 0, 120]], codes: ["G", "Y", "X"] },
    { opts: ["FULL", "EASY"], colors: [[0, 0, 120], [0, 120, 0]], codes: ["X", "G"] },
    { opts: ["OFF", "ON", "SONG", "NEXT"], colors: [[120, 0, 0], [0, 120, 0], [255, 255, 0], [0, 255, 255]], codes: ["R", "G", "Y", "N"] },
    { opts: ["OFF", "1", "2", "3", "4", "5", "6", "7", "8"], colors: [[100, 100, 100]], codes: ["G"] },
    { opts: ["OFF", "ON"], colors: [[120, 0, 0], [0, 120, 0]], codes: ["R", "G"] },
    { opts: ["PAGE", "VOL"], colors: [[0, 120, 0], [120, 60, 0]], codes: ["G", "O"] },
    { opts: ["1", "1B", "2", "2B"], colors: [[0, 120, 0]], codes: ["G"] },
    { opts: ["OFF", "ON"], colors: [[120, 0, 0], [0, 120, 0]], codes: ["R", "G"] },
    { opts: ["NORM", "CHNR", "BIG"], colors: [[150, 100, 0], [150, 200, 0], [150, 255, 0]], codes: ["G"] },
    { opts: ["L+R", "R"], colors: [[0, 120, 0]], codes: ["G"] },
    { opts: ["ALL"], colors: [[0, 120, 0]], codes: ["G"] },
  ],
  recs: [
    { opts: ["LINE", "MIC"], colors: [[0, 0, 120], [120, 120, 120]], codes: ["X", "W"] },
    { opts: ["0"], colors: [[0, 120, 120]], codes: ["N"] },
    { opts: ["0"], colors: [[0, 0, 120]], codes: ["X"] },
    { opts: ["OFF", "SENS", "-CON", "+CON"], colors: [[120, 0, 0], [0, 120, 0], [20, 20, 0], [0, 0, 20]], codes: ["R", "G", "Y", "X"] },
    { opts: ["OFF", "ON", "FIX", "ON1", "CLIC"], colors: [[120, 0, 0], [0, 120, 0], [120, 120, 0], [0, 120, 120], [120, 0, 120]], codes: ["R", "G", "Y", "N", "M"] },
  ],
  midi: [
    { opts: ["YPOS", "MIDI", "KEYS"], colors: [[0, 120, 0], [0, 0, 120], [120, 0, 120]], codes: ["G", "X", "M"] },
    { opts: ["OFF", "GET", "SEND"], colors: [[120, 0, 0], [0, 120, 0], [0, 0, 120]], codes: ["R", "G", "X"] },
    { opts: ["CLCK", "NOTE", "BOTH"], colors: [[120, 120, 0], [0, 120, 0], [0, 0, 120]], codes: ["Y", "G", "X"] },
    { opts: ["OFF", "NOTE"], colors: [[120, 0, 0], [0, 120, 0]], codes: ["R", "G"] },
    { opts: ["OFF", "ON"], colors: [[120, 0, 0], [0, 120, 0]], codes: ["R", "G"] },
    { opts: ["0"], colors: [[120, 120, 120]], codes: ["W"] },
    { opts: ["OFF"], colors: [[120, 0, 0]], codes: ["R"] },
  ],
  vol: [
    { opts: ["80"], colors: [[55, 10, 0]], codes: ["O"] },
    { opts: ["1.0"], colors: [[120, 120, 120]], codes: ["W"] },
    { opts: ["10"], colors: [[50, 0, 50]], codes: ["P"] },
    { opts: ["8"], colors: [[0, 120, 120]], codes: ["N"] },
    { opts: ["OFF", "M+P", "L+R"], colors: [[120, 0, 0], [0, 120, 0], [0, 0, 120]], codes: ["R", "G", "X"] },
    { opts: ["OFF", "ON"], colors: [[120, 0, 0], [0, 120, 0]], codes: ["R", "G"] },
    { opts: ["0"], colors: [[120, 120, 0]], codes: ["Y"] },
  ],
  etc: [
    { opts: ["TOERN"], colors: [[120, 120, 120]], codes: ["W"] },
    { opts: ["RAM"], colors: [[0, 120, 0]], codes: ["G"] },
    { opts: ["OFF"], colors: [[120, 0, 0]], codes: ["R"] },
    { opts: ["OFF"], colors: [[120, 120, 0]], codes: ["Y"] },
    { opts: ["OFF", "ON"], colors: [[120, 0, 0], [0, 120, 0]], codes: ["R", "G"] },
    { opts: ["0", "1", "2"], colors: [[0, 120, 0]], codes: ["G"] },
    { opts: ["--"], colors: [[120, 120, 0]], codes: ["Y"] },
    { opts: ["OFF", "ON"], colors: [[120, 0, 0], [0, 120, 0]], codes: ["R", "G"] },
    { opts: ["OFF", "ALL", "1", "2", "3", "4", "5", "6", "7", "8", "11", "13", "14"], colors: [[120, 0, 0], [255, 220, 120], [255, 80, 20]], codes: ["R", "Y", "O"] },
    { opts: ["SD", "EFX", "FULL", "FILE", "PACK", "ASAV"], colors: [[120, 60, 0]], codes: ["O"] },
  ],
};

export function createDevice(matrix, statusEl, rings) {
  const engine = createEngine();
  const note = Array.from({ length: STEPS + 1 }, () => Array.from({ length: ROWS + 1 }, emptyNote));
  const filt = Array.from({ length: 16 }, emptyFilt);
  const chVol = Array(16).fill(16);
  const shownRing = [];
  const shownAngle = [0, 0, 0, 0];
  let shownStatus = "";
  const fired = [];
  const wavOf = Array.from({ length: 16 }, (_, ch) => (ch >= 1 && ch <= 8 ? ch - 1 : 0));
  const seekOf = Array(16).fill(0);
  const endOf = Array(16).fill(100);
  const invOf = Array(16).fill(false);
  const mute = Array(16).fill(false);
  const song = Array(64).fill(0);
  const subPick = { look: Array(12).fill(0), recs: Array(5).fill(0), midi: Array(7).fill(0), vol: Array(7).fill(0), etc: Array(10).fill(0) };
  // Firmware EEPROM defaults: TRIG=SENS, CLR=ON, FLOW=ON
  subPick.recs[3] = 1;
  subPick.recs[4] = 1;
  subPick.look[0] = 1;
  subPick.look[7] = 2;

  const s = {
    GLOB: { x: 1, y: 2, edit: 1, page: 1, currentChannel: 1, singleMode: false },
    mode: "boot", note, filt, mute, song, playing: false, beat: 1, pulse: 40, pulseDir: 1,
    menuIndex: 0, subIndex: 0, slot: 1, packSlot: 1, wavName: "KICK",
    genre: 0, genreLength: 8,
    seekOf, endOf, invOf, browse: 0, browseDir: "samples", browseItems: listDir("samples", { parent: false }),
    wavFile: false, peaks: null, shownWav: "", sampleUrl: Array(16).fill(null),
    vel: { v: 8, p: 5, c: 1, vol: 16, backSingle: false }, okAt: 0,
    filterTouch: 0, filterTouchAt: 0, filterFlash: "", filterFlashAt: 0, filterPage: 0,
    drawBaseColorMode: true, monitor: 0, loopLength: 0, simpleNotes: false, clockInt: true,
    flow: true, voiceMode: false, fireVoice: 0, fireLevel: 8, fireSize: 1, fireGravity: 0, fireColor: 8, fireFocus: 0, voiceLen: Array(16).fill(0), voiceMask: Array(16).fill(0), voicePages: 1,
    voiceLoop: Array(16).fill(1), voiceOffset: Array(16).fill(0), voiceEdit: Array(16).fill(0),
    pmode: 0, ctrlVol: false, prevMode: 0, drawR: false, cursorType: 0,
    scheme: 0, childLock: false, copyArmed: false, solo: false, soloSaved: null, soloArrow: "", soloArrowAt: 0,
    volBar: 0, volBarAt: 0, chVol,
    muteMask: 0x0006, muteSel: 1, fullMute: false, fullMuteSaved: null,
    ledBrightness: 64, chNr: 1, chNrAt: 0, pageNr: 1, pageNrAt: 0, infoAt: 0, knobAngle: [0, 0, 0, 0],
    mainVol: 80, gain: 1, prevVol: 8, stereo: 0,
    micGain: 20, lineInLevel: 5,
    fastRecMode: 1, recChannelClear: 1, recInput: 0,
    fastRecActive: false,
    songPos: 1, songPattern: 1, bpm: 120, bootAt: 0,
    hasPageNotes, slotExists, subValue,
    deviceFocus: true,
    assistKeyGate: false,
    onAnnounce: null,
  };

  let nextAt = 0;
  const taps = [];
  const filterPages = Array(16).fill(0);
  let clip = null;
  let clipCh = 0;
  const hold = {};
  const touch = { l: false, m: false, first: 0, chord: false };

  function stepSec() { return 60 / s.bpm / 4; }
  function channelOfRow(y) { return y - 1; }
  function localX(globalX) { return ((globalX - 1) % COLS) + 1; }
  function viewPage(page, local) {
    page = clamp(page, 1, PAGES);
    const column = local ?? localX(s.GLOB.x);
    const changed = page !== s.GLOB.edit;
    s.GLOB.edit = page;
    s.GLOB.x = (page - 1) * COLS + column;
    if (!changed) return;
    if (!s.playing) {
      s.GLOB.page = page;
      return;
    }
    if (s.pmode === 1) {
      const step = ((s.beat - 1) % COLS) + 1;
      s.GLOB.page = page;
      s.beat = (page - 1) * COLS + step;
    }
  }
  function syncChannel() {
    if (s.GLOB.singleMode) return;
    const ch = channelOfRow(s.GLOB.y);
    if (ch >= 0 && ch <= 15) s.GLOB.currentChannel = ch;
  }
  function wavOpts(ch) {
    const f = filt[ch];
    return {
      index: wavOf[ch] || 0, seek: seekOf[ch], end: endOf[ch], inv: invOf[ch],
      // Sample voices: DTNE ±1 semi (fine), OCTV whole semis ±24 (±2 oct).
      // wav.oct is in octaves for engine (semis/12); raw slider is 0..48 center 24.
      detune: ((f.detune ?? 16) / 32) * 2 - 1, oct: ((f.oct ?? 24) - 24) / 12,
      env: { att: f.att, dec: f.dec, sus: f.sus, rel: f.rel },
      wave: f.wave || 0, inst: f.inst || 0, cent: f.cent ?? 16, semi: f.semi || 0,
      lfoR: f.lfoR || 0, lfoD: f.lfoD || 0, span: f.span || 0, arp: f.arp || 0,
    };
  }
  const SAMPLE_ROOT = "samples";
  const browseDirOf = Array(16).fill(SAMPLE_ROOT);
  const browseIdxOf = Array(16).fill(0);
  let peakToken = 0;
  function currentFile() {
    return s.browseItems[s.browse] || null;
  }
  function isWav(item) {
    return !!(item?.url && /\.wav$/i.test(item.name));
  }
  function labelOf(item) {
    if (!item) return "-";
    return item.type === "file" ? item.name.replace(/\.wav$/i, "") : item.name;
  }
  function previewOnSelect() {
    return s.prevMode === 0 || (s.prevMode === 2 && !s.playing);
  }
  async function previewWav(force) {
    const item = currentFile();
    if (!isWav(item)) return;
    if (!force && !previewOnSelect()) return;
    const ch = s.GLOB.currentChannel;
    const ctx = engine.ensure();
    if (ctx.state === "suspended") ctx.resume();
    engine.setBus({ main: s.mainVol, preview: s.prevVol, stereo: s.stereo });
    const buffer = await engine.decodeUrl(item.url);
    engine.preview({ index: wavOf[ch] || 0, seek: seekOf[ch], end: endOf[ch], inv: invOf[ch], velocity: Math.round(s.prevVol * 8), buffer, env: wavOpts(ch).env });
  }
  async function scanPeaks(item) {
    const token = ++peakToken;
    s.peaks = null;
    if (!isWav(item)) return;
    const buf = await engine.decodeUrl(item.url);
    if (token !== peakToken) return;
    const data = buf.getChannelData(0);
    const peaks = [];
    const n = 32;
    for (let i = 0; i < n; i++) {
      const a = Math.floor((i / n) * data.length);
      const b = Math.max(a + 1, Math.floor(((i + 1) / n) * data.length));
      let peak = 0;
      for (let k = a; k < b; k += 8) peak = Math.max(peak, Math.abs(data[k]));
      peaks.push(peak);
    }
    s.peaks = peaks;
  }
  function refreshBrowse() {
    const ch = clamp(s.GLOB.currentChannel, 1, 8);
    s.browseDir = browseDirOf[ch] || SAMPLE_ROOT;
    s.browseItems = listDir(s.browseDir, { parent: s.browseDir !== SAMPLE_ROOT });
    s.browse = clamp(browseIdxOf[ch] || 0, 0, Math.max(0, s.browseItems.length - 1));
    browseIdxOf[ch] = s.browse;
    const item = currentFile();
    s.wavFile = isWav(item);
    s.wavName = labelOf(item);
    s.shownWav = s.wavFile ? item.url : "";
    if (s.wavFile) scanPeaks(item);
    else { peakToken++; s.peaks = null; }
  }
  function rememberFolder(ch, leaf) {
    refreshBrowse();
    const idx = s.browseItems.findIndex((it) => it.type === "dir" && it.name === leaf);
    const pick = idx >= 0 ? idx : (s.browseItems[0]?.type === "up" && s.browseItems.length > 1 ? 1 : 0);
    s.browse = pick;
    browseIdxOf[ch] = pick;
  }
  function goParent(ch) {
    if ((browseDirOf[ch] || SAMPLE_ROOT) === SAMPLE_ROOT) return;
    const parts = browseDirOf[ch].split("/");
    const leaf = parts.pop();
    browseDirOf[ch] = parts.join("/") || SAMPLE_ROOT;
    rememberFolder(ch, leaf);
  }
  function browserPress() {
    const ch = clamp(s.GLOB.currentChannel, 1, 8);
    const item = currentFile();
    if (!item) return;
    if (item.type === "up") { goParent(ch); return; }
    if (item.type === "dir") {
      browseDirOf[ch] = `${s.browseDir}/${item.name}`;
      browseIdxOf[ch] = 0;
      refreshBrowse();
      if (s.browseItems[0]?.type === "up" && s.browseItems.length > 1) {
        s.browse = 1;
        browseIdxOf[ch] = 1;
      }
      return;
    }
    goParent(ch);
  }
  function playCell(x, y) {
    if (s.playing) return;
    const n = note[x][y];
    if (!n || !n.channel || !SOUND_CH.has(n.channel)) return;
    engine.ensure();
    const ctx = engine.ensure();
    if (ctx.state === "suspended") ctx.resume();
    engine.trigger(n.channel, n.velocity, engine.now(), { ...wavOpts(n.channel), midiPitch: n.midiPitch }, y);
  }
  function applyVoiceEdit(ch) {
    if (s.mode !== "draw" || !s.voiceMode || s.pmode === 2 || s.ctrlVol || s.childLock) return;
    refreshVoiceLens();
    const pages = filledPages(ch);
    let page = s.voiceEdit[ch] || 1;
    if (pages.length) {
      page = nearestFilled(ch, page);
      s.voiceEdit[ch] = page;
    } else page = clamp(page, 1, PAGES);
    viewPage(page);
  }
  function selectVoice(ch) {
    if (!((ch >= 1 && ch <= 8) || ch === 11 || ch === 13 || ch === 14)) return;
    s.GLOB.currentChannel = ch;
    s.GLOB.y = ch + 1;
    if (s.mode === "draw" || s.mode === "single") {
      s.GLOB.x = (s.GLOB.edit - 1) * COLS + localX(s.GLOB.x);
    }
    if (s.mode === "draw") applyVoiceEdit(ch);
    if (s.mode === "wav" && ch <= 8) refreshBrowse();
  }
  function drawRandomPage() {
    drawRandoms({
      note, filt, channel: s.GLOB.currentChannel, page: s.GLOB.edit,
      loopLength: s.loopLength, bpm: s.bpm,
    });
    queueAutosave();
  }
  function clearPage() {
    const base = (s.GLOB.edit - 1) * COLS;
    const only = s.mode === "single" ? s.GLOB.currentChannel : 0;
    for (let x = 1; x <= COLS; x++) {
      for (let y = 1; y <= ROWS; y++) {
        const n = note[base + x][y];
        if (only && n.channel !== only) continue;
        n.channel = 0;
      }
    }
    queueAutosave();
  }
  function paintCell(x, y) {
    if (y < 1 || y > 15) return false;
    if (!s.GLOB.singleMode && !PAINT_ROWS.has(y)) return false;
    const n = note[x][y];
    if (s.GLOB.singleMode) {
      // Never overwrite notes belonging to other voices (matches firmware paint/paintMode)
      if (n.channel && n.channel !== s.GLOB.currentChannel) return false;
      if (!n.channel) {
        n.velocity = 100;
        n.probability = 100;
        n.condition = 1;
      }
      n.channel = s.GLOB.currentChannel;
      queueAutosave();
      return true;
    }
    if (!n.channel) {
      n.channel = channelOfRow(y);
      n.velocity = 100;
      n.probability = 100;
      n.condition = 1;
    }
    queueAutosave();
    return true;
  }
  function paintPress(x, y) {
    if (y === 16) { toggleCopy(); return; }
    if (s.drawR && note[x][y].channel) { eraseCell(x, y); return; }
    if (paintCell(x, y)) playCell(x, y);
  }
  function eraseCell(x, y) {
    if (y === 16) { clearColumn(x); return; }
    const n = note[x][y];
    if (!n) return;
    if (s.GLOB.singleMode && n.channel !== s.GLOB.currentChannel) return;
    n.channel = 0;
    queueAutosave();
  }
  function clearColumn(x) {
    for (let y = 1; y <= ROWS; y++) {
      if (s.GLOB.singleMode && note[x][y].channel !== s.GLOB.currentChannel) continue;
      note[x][y].channel = 0;
    }
    queueAutosave();
  }
  function toggleCopy() {
    const base = (s.GLOB.edit - 1) * COLS;
    if (!s.copyArmed) {
      clip = [];
      clipCh = s.GLOB.currentChannel;
      for (let x = 1; x <= COLS; x++) for (let y = 1; y <= ROWS; y++) {
        const n = note[base + x][y];
        const keep = !s.GLOB.singleMode || n.channel === clipCh;
        clip.push(keep && n.channel ? { ...n } : null);
      }
      s.copyArmed = true;
      return;
    }
    let i = 0;
    for (let x = 1; x <= COLS; x++) for (let y = 1; y <= ROWS; y++) {
      const src = clip[i++];
      const dst = note[base + x][y];
      if (s.GLOB.singleMode) {
        if (dst.channel === s.GLOB.currentChannel) dst.channel = 0;
        if (src && src.channel === clipCh) Object.assign(dst, src, { channel: s.GLOB.currentChannel });
      } else if (src) Object.assign(dst, emptyNote(), src);
      else dst.channel = 0;
    }
    s.copyArmed = false;
    queueAutosave();
  }
  function shiftNotes(dx, dy, pageOnly) {
    const ch = s.GLOB.currentChannel;
    const last = pageOnly ? s.GLOB.edit : (s.loopLength > 0 ? Math.min(s.loopLength, PAGES) : (() => {
      let hi = 1;
      for (let p = 1; p <= PAGES; p++) if (hasPageNotes(p)) hi = p;
      return hi;
    })());
    const x0 = pageOnly ? (s.GLOB.edit - 1) * COLS + 1 : 1;
    const x1 = last * COLS;
    const hits = [];
    for (let x = x0; x <= x1; x++) for (let y = 1; y <= 15; y++) {
      const n = note[x][y];
      if (n.channel !== ch) continue;
      hits.push({ x, y, n: { ...n } });
      Object.assign(n, emptyNote());
    }
    for (const hit of hits) {
      let x = hit.x + dx;
      if (dx) {
        if (x < x0) x = x1;
        else if (x > x1) x = x0;
      }
      let y = hit.y + dy;
      if (y < 1) y = 15;
      else if (y > 15) y = 1;
      const midi = hit.n.midiPitch <= 127 ? clamp(hit.n.midiPitch + dy, 0, 127) : 255;
      Object.assign(note[x][y], hit.n, { midiPitch: midi });
    }
    queueAutosave();
  }
  function hasPageNotes(p) {
    const base = (p - 1) * COLS;
    for (let x = 1; x <= COLS; x++) for (let y = 1; y <= ROWS; y++) if (note[base + x][y].channel) return true;
    return false;
  }
  function pageCount() {
    // Firmware: LOOP > 0 forces length; otherwise lastPage = highest page with notes.
    // PMOD OFF still wraps at lastPage (not all PAGES). PMOD ON loops one page in tick().
    // VMOD ignores LOOP; the cycle is the longest voice.
    if (!s.voiceMode && s.loopLength > 0) return Math.min(s.loopLength, PAGES);
    let last = 1;
    for (let p = 1; p <= PAGES; p++) if (hasPageNotes(p)) last = p;
    return last;
  }
  function patternEnd() {
    if (s.pmode === 2 || s.pmode === 3) return s.GLOB.page * COLS;
    return pageCount() * COLS;
  }
  function condOk(cond, step) {
    if (cond <= 1) return true;
    if (cond === 2) return step % 2 === 0;
    if (cond === 4) return step % 4 === 0;
    if (cond === 8) return step % 8 === 0;
    if (cond === 16) return step % 16 === 0;
    if (cond === 21) return (step % 16) >= 12;
    if (cond === 17) return true;
    if (cond === 18) return step % 2 === 0;
    if (cond === 19) return step % 4 === 0;
    if (cond === 20) return step % 8 === 0;
    return true;
  }
  function channelHeard(ch) {
    return !s.mute[ch];
  }
  function applyMix() {
    engine.ensure();
    engine.setBus({ main: s.mainVol, preview: s.prevVol, stereo: s.stereo });
    for (const ch of SOUND_CH) engine.apply(ch, filt[ch], chVol[ch], s.gain);
  }
  function liveMix() {
    for (const ch of SOUND_CH) engine.apply(ch, filt[ch], channelHeard(ch) ? chVol[ch] : 0, s.gain);
  }
  function applyFilt(ch) { if (SOUND_CH.has(ch)) engine.apply(ch, filt[ch], chVol[ch], s.gain); }
  function clearNotes() {
    for (let x = 1; x <= STEPS; x++) for (let y = 1; y <= ROWS; y++) Object.assign(note[x][y], emptyNote());
  }
  function resetEffects() {
    for (const ch of SOUND_CH) Object.assign(filt[ch], emptyFilt());
    chVol.fill(16);
    s.vel.vol = 16;
    applyMix();
  }
  function runReset() {
    const opt = subPick.etc[9] || 0;
    if (opt === 0) refreshBrowse();
    else if (opt === 1) resetEffects();
    else if (opt === 2) resetFull();
    else if (opt === 3) {
      const all = loadStore("toern-web-patterns");
      for (let i = 1; i <= 99; i++) delete all[String(i)];
      localStorage.setItem("toern-web-patterns", JSON.stringify(all));
    } else if (opt === 5) {
      clearNotes();
      patternDirty = true;
      saveSlot("toern-web-patterns", 0, snapshot());
    }
    s.okAt = performance.now();
    finishLoad();
  }
  function resetFull() {
    resetEffects();
    clearNotes();
    queueAutosave();
  }
  function heard(ch) {
    if (s.mute[ch]) return false;
    if (s.fullMute) return false;
    return true;
  }
  function refreshVoiceLens() {
    const len = Array(16).fill(0);
    const mask = Array(16).fill(0);
    for (let p = 1; p <= PAGES; p++) {
      const base = (p - 1) * COLS;
      for (let x = 1; x <= COLS; x++) {
        for (let y = 1; y <= ROWS; y++) {
          const ch = note[base + x][y].channel;
          if (!ch || ch > 15) continue;
          if (p > len[ch]) len[ch] = p;
          mask[ch] |= 1 << (p - 1);
        }
      }
    }
    let longest = 1;
    for (let c = 1; c < 16; c++) if (len[c] > longest) longest = len[c];
    s.voiceLen = len;
    s.voiceMask = mask;
    s.voicePages = longest;
  }
  s.refreshVoiceLens = refreshVoiceLens;
  function voicePageFor(ch, gPage) {
    const len = s.voiceLen[ch] > 0 ? s.voiceLen[ch] : 1;
    let idx = (Math.max(1, gPage) - 1) + (s.voiceOffset[ch] || 0);
    idx %= len;
    if (idx < 0) idx += len;
    return idx + 1;
  }
  function voiceOwnedStep(row, col, gPage) {
    for (let ch = 1; ch < 16; ch++) {
      if (!s.voiceLen[ch]) continue;
      const src = (voicePageFor(ch, gPage) - 1) * COLS + col;
      if (note[src]?.[row]?.channel === ch) return src;
    }
    return 0;
  }
  s.voicePageFor = voicePageFor;
  s.voiceOwnedStep = voiceOwnedStep;
  function filledPages(ch) {
    const mask = s.voiceMask[ch] || 0;
    const pages = [];
    for (let p = 1; p <= PAGES && p <= 16; p++) if (mask & (1 << (p - 1))) pages.push(p);
    return pages;
  }
  function stepFilled(ch, from, dir) {
    const pages = filledPages(ch);
    if (dir > 0) {
      for (const p of pages) if (p > from) return p;
    } else {
      for (let i = pages.length - 1; i >= 0; i--) if (pages[i] < from) return pages[i];
    }
    return 0;
  }
  function nearestFilled(ch, page) {
    const pages = filledPages(ch);
    if (!pages.length) return page;
    let best = pages[0];
    let dist = 99;
    for (const p of pages) {
      const d = Math.abs(page - p);
      if (d < dist) { dist = d; best = p; }
    }
    return best;
  }
  function cueVoice(ch, page) {
    const len = s.voiceLen[ch] > 0 ? s.voiceLen[ch] : 1;
    const wrapped = ((Math.max(1, page) - 1) % len) + 1;
    const gPage = s.playing ? Math.floor(((s.beat || 1) - 1) / COLS) + 1 : (s.GLOB.page || 1);
    let off = (wrapped - 1) - (gPage - 1);
    off %= len;
    if (off < 0) off += len;
    s.voiceOffset[ch] = off;
  }
  function voiceStep(row, beat) {
    if (!s.voiceMode || s.pmode === 2) return beat;
    const col = ((beat - 1) % COLS) + 1;
    const gPage = Math.floor((beat - 1) / COLS) + 1;
    return voiceOwnedStep(row, col, gPage);
  }
  function bumpVoiceLoops(prevBeat, beat) {
    const prevPage = Math.floor((Math.max(1, prevBeat) - 1) / COLS) + 1;
    const newPage = Math.floor((Math.max(1, beat) - 1) / COLS) + 1;
    if (prevPage === newPage) return;
    for (let c = 1; c < 16; c++) {
      const len = s.voiceLen[c];
      if (!len) continue;
      const prevV = voicePageFor(c, prevPage);
      const newV = voicePageFor(c, newPage);
      if (prevV === len && newV === 1) {
        s.voiceLoop[c] = (s.voiceLoop[c] || 1) + 1;
        if (s.voiceLoop[c] > 256) s.voiceLoop[c] = 1;
      }
    }
  }
  function condOkVoice(cond, loopN) {
    const n = loopN < 1 ? 1 : loopN;
    if (cond <= 1 || cond === 17 || cond === 21 || cond === 22) return true;
    if (cond <= 16) return n % cond === 0;
    const x = cond === 18 ? 4 : cond === 19 ? 8 : cond === 20 ? 16 : 2;
    return n % x === 1;
  }
  function fire(step, when) {
    fired.push({ step, when });
    if (fired.length > 8) fired.shift();
    if (s.voiceMode) refreshVoiceLens();
    // Sample voices 1–8 are monophonic (one sampler per channel, like firmware
    // retrigger). If several notes share a step+voice, keep the highest row only.
    const samplePick = new Map();
    const otherHits = [];
    for (let y = 1; y <= ROWS; y++) {
      const src = s.voiceMode && s.pmode !== 2 ? voiceStep(y, step) : step;
      if (!src) continue;
      const n = note[src][y];
      if (!n.channel || !SOUND_CH.has(n.channel) || !heard(n.channel)) continue;
      if (Math.random() * 100 >= n.probability) continue;
      const ok = s.voiceMode && s.pmode !== 2
        ? condOkVoice(n.condition, s.voiceLoop[n.channel] || 1)
        : condOk(n.condition, step);
      if (!ok) continue;
      if (n.channel >= 1 && n.channel <= 8) {
        const prev = samplePick.get(n.channel);
        if (!prev || y > prev.y) samplePick.set(n.channel, { n, y });
      } else {
        otherHits.push({ n, y });
      }
    }
    const hits = [...samplePick.values(), ...otherHits];
    for (const { n, y } of hits) {
      const wav = { ...wavOpts(n.channel), midiPitch: n.midiPitch };
      if (s.prevMode === 2 && n.channel === s.GLOB.currentChannel && s.mode === "wav") wav.index = s.browse;
      engine.trigger(n.channel, n.velocity, when, wav, y);
    }
  }
  function togglePlay() {
    const ctx = engine.ensure();
    if (ctx.state === "suspended") ctx.resume();
    applyMix();
    s.playing = !s.playing;
    if (!s.playing) queueAutosave();
    if (s.playing) {
      if (s.voiceMode && s.pmode !== 2) {
        refreshVoiceLens();
        s.voiceLoop = Array(16).fill(1);
        s.beat = 1;
        s.GLOB.page = 1;
      } else if (s.pmode === 1) {
        s.GLOB.page = s.GLOB.edit;
        s.beat = (s.GLOB.edit - 1) * COLS + 1;
      } else {
        s.beat = 1;
        s.GLOB.page = 1;
      }
      const t = engine.now();
      nextAt = t + stepSec();
      fire(s.beat, t);
    }
  }
  function enterFilter() {
    if (!FILTER_CH.has(s.GLOB.currentChannel)) return;
    engine.ensure();
    applyFilt(s.GLOB.currentChannel);
    s.filterTouch = 0;
    s.filterPage = filterPages[s.GLOB.currentChannel] || 0;
    s.mode = "filter";
  }
  function enterVelocity() {
    const n = note[s.GLOB.x][s.GLOB.y];
    if (!n.channel) return;
    s.vel.backSingle = s.GLOB.singleMode;
    s.vel.v = clamp(Math.round(1 + ((n.velocity - 1) * 15) / 126), 1, 16);
    s.vel.p = Math.max(1, PROB.indexOf(n.probability) + 1);
    s.vel.c = Math.max(1, COND_VALUE.indexOf(n.condition) + 1);
    s.vel.vol = chVol[s.GLOB.currentChannel] ?? 12;
    s.mode = "velocity";
  }
  function applyVelocity() {
    const n = note[s.GLOB.x][s.GLOB.y];
    if (!n.channel) return;
    const velocity = Math.round(1 + ((s.vel.v - 1) * 126) / 15);
    const probability = PROB[s.vel.p - 1] ?? 100;
    const condition = COND_VALUE[s.vel.c - 1] ?? 1;
    if (s.GLOB.singleMode) {
      n.velocity = velocity; n.probability = probability; n.condition = condition;
    } else {
      for (let x = 1; x <= STEPS; x++) for (let y = 1; y <= ROWS; y++) {
        if (note[x][y].channel === n.channel) note[x][y].velocity = velocity;
      }
      n.probability = probability; n.condition = condition;
    }
    chVol[s.GLOB.currentChannel] = s.vel.vol;
    applyFilt(s.GLOB.currentChannel);
    queueAutosave();
  }
  function nudgeFast(dir) {
    const ch = s.GLOB.currentChannel;
    if (!FILTER_CH.has(ch)) return;
    const f = filt[ch];
    const pages = filterPagesFor(ch);
    const spec = (pages[filterPages[ch]] || pages[0])[f.fast];
    if (!spec) return;
    f[spec.key] = clamp(f[spec.key] + dir, 0, spec.max ?? 32);
    applyFilt(ch);
    s.filterFlash = spec.name;
    s.filterFlashAt = performance.now();
  }
  const EEPROM_KEY = "toern-web-eeprom";
  let persist = false;
  let deviceChosen = false;
  let patternDirty = false;
  let autosaveTimer = 0;
  function loadStore(key) { try { return JSON.parse(localStorage.getItem(key) || "{}"); } catch { return {}; } }
  function demoPattern(id) {
    if (id !== 1 && id !== 2) return null;
    const data = [];
    const put = (step, y, ch) => { data[(step - 1) * ROWS + (y - 1)] = [ch, 100, 100, 1]; };
    for (let i = 0; i < STEPS * ROWS; i++) data.push(0);
    if (id === 1) {
      for (const x of [1, 5, 9, 13]) put(x, 2, 1);
      for (const x of [5, 13]) put(x, 3, 2);
      for (let x = 1; x <= 16; x += 2) put(x, 4, 3);
    } else {
      for (const x of [1, 8, 11, 14]) put(x, 2, 1);
      put(5, 3, 2);
      put(13, 3, 2);
      for (let x = 1; x <= 16; x++) put(x, 5, 4);
    }
    return data;
  }
  function slotExists(key, id) {
    if (key === "toern-web-packs" && DUMMY_PACKS[id]) return true;
    if (key === "toern-web-patterns" && id === 0 && patternUrl(0)) return true;
    if (key === "toern-web-patterns" && demoPattern(id)) return true;
    return loadStore(key)[String(id)] != null;
  }
  function applyPack(pack) {
    if (!pack || !pack.wavs) return;
    for (let ch = 1; ch <= 8; ch++) wavOf[ch] = pack.wavs[ch - 1] ?? 0;
    s.wavName = WAVS[wavOf[s.GLOB.currentChannel]] || WAVS[0];
  }
  function saveSlot(key, id, value) {
    const all = loadStore(key);
    all[String(id)] = value;
    localStorage.setItem(key, JSON.stringify(all));
  }
  function saveEeprom() {
    if (!persist) return;
    const data = {
      subPick: {
        look: subPick.look.slice(), recs: subPick.recs.slice(), midi: subPick.midi.slice(),
        vol: subPick.vol.slice(), etc: subPick.etc.slice(),
      },
      mainVol: s.mainVol, gain: s.gain, prevVol: s.prevVol, stereo: s.stereo,
      fireLevel: s.fireLevel, fireSize: s.fireSize, fireGravity: s.fireGravity, fireColor: s.fireColor,
      micGain: s.micGain, lineInLevel: s.lineInLevel,
      bpm: s.bpm, clockInt: s.clockInt, muteMask: s.muteMask, packSlot: s.packSlot, ledBrightness: s.ledBrightness,
      deviceChosen,
    };
    try { localStorage.setItem(EEPROM_KEY, JSON.stringify(data)); } catch { /* storage full */ }
  }
  function loadEeprom() {
    let data = null;
    try { data = JSON.parse(localStorage.getItem(EEPROM_KEY) || "null"); } catch { data = null; }
    if (!data) {
      applySub("look", 0);
      loadPack(1);
      return;
    }
    if (Array.isArray(data.subPick?.look) && data.subPick.look.length === 11 && subPick.look.length === 12) {
      data.subPick.look = data.subPick.look.slice();
      data.subPick.look.splice(5, 0, 0);
    }
    if (Array.isArray(data.subPick?.etc) && data.subPick.etc.length === 9 && subPick.etc.length === 10) {
      data.subPick.etc = data.subPick.etc.slice();
      data.subPick.etc.splice(8, 0, 0);
    }
    if (data.fireLevel == null && Array.isArray(data.subPick?.etc) && data.subPick.etc[8] > 0) {
      data.subPick.etc[8] += 1;
    }
    for (const mode of Object.keys(subPick)) {
      const saved = data.subPick?.[mode];
      if (!Array.isArray(saved)) continue;
      for (let i = 0; i < subPick[mode].length; i++) {
        const spec = SUB_VALUES[mode][i];
        if (Number.isFinite(saved[i])) subPick[mode][i] = clamp(saved[i], 0, (spec?.opts.length || 1) - 1);
      }
    }
    if (Number.isFinite(data.fireLevel)) s.fireLevel = data.fireLevel < 1 ? 8 : clamp(data.fireLevel, 1, 25);
    if (Number.isFinite(data.fireSize)) s.fireSize = clamp(data.fireSize, 1, 4);
    if (Number.isFinite(data.fireGravity)) s.fireGravity = clamp(data.fireGravity, 0, 8);
    if (Number.isFinite(data.fireColor)) s.fireColor = clamp(data.fireColor, 0, 8);
    if (Number.isFinite(data.mainVol)) s.mainVol = clamp(data.mainVol, 0, 100);
    if (Number.isFinite(data.gain)) s.gain = clamp(Math.round(data.gain * 10) / 10, 0, 2);
    if (Number.isFinite(data.prevVol)) s.prevVol = clamp(data.prevVol, 0, 16);
    if (Number.isFinite(data.micGain)) s.micGain = clamp(data.micGain, 0, 63);
    if (Number.isFinite(data.lineInLevel)) s.lineInLevel = clamp(data.lineInLevel, 0, 15);
    if (Number.isFinite(data.bpm)) s.bpm = clamp(data.bpm, 40, 240);
    if (typeof data.clockInt === "boolean") s.clockInt = data.clockInt;
    if (Number.isFinite(data.muteMask)) s.muteMask = data.muteMask & 0xffff;
    if (Number.isFinite(data.ledBrightness)) s.ledBrightness = clamp(data.ledBrightness, 3, 255);
    if (!data.deviceChosen) subPick.look[7] = 2;
    s.packSlot = Number.isFinite(data.packSlot) && data.packSlot >= 1 && data.packSlot <= 99 ? data.packSlot : 1;
    applySub("look", 0);
    applyLedLayout();
    applySub("etc", 5);
    applySub("etc", 7);
    applySub("vol", 4);
    applySub("recs", 0);
    applySub("recs", 3);
    applySub("recs", 4);
    loadPack(s.packSlot);
  }
  function loadPack(slot) {
    s.packSlot = slot;
    const urls = packWavs(slot);
    if (!urls.some(Boolean)) return;
    urls.forEach((url, i) => { if (url) engine.loadUrl(i + 1, url); });
    s.wavName = `PACK ${slot}`;
  }
  function patternHasNotes(data) {
    return Array.isArray(data) && data.some((n) => (Array.isArray(n) ? n[0] : n));
  }
  function queueAutosave() {
    patternDirty = true;
    clearTimeout(autosaveTimer);
    autosaveTimer = setTimeout(() => saveSlot("toern-web-patterns", 0, snapshot()), 400);
  }
  function autoload() {
    s.mute[1] = false;
    const stored = loadStore("toern-web-patterns")["0"];
    if (patternHasNotes(stored)) {
      restore(stored);
      s.mute[1] = false;
      return;
    }
    const url = patternUrl(0) || patternUrl(1);
    if (!url) return;
    parsePattern(url).then((data) => {
      if (!data || patternDirty || !patternHasNotes(data)) return;
      restore(data);
      s.mute[1] = false;
      saveSlot("toern-web-patterns", 0, data);
    });
  }
  function applyLedLayout() {
    const wide = subPick.look[7] >= 2;
    setLayout(wide ? 32 : 16);
    matrix.layout(COLS);
    s.GLOB.x = clamp(s.GLOB.x, 1, STEPS);
    s.GLOB.edit = clamp(Math.floor((s.GLOB.x - 1) / COLS) + 1, 1, PAGES);
    if (!s.playing) s.GLOB.page = s.GLOB.edit;
    if (s.GLOB.page > PAGES) s.GLOB.page = PAGES;
  }
  function snapshot() {
    const out = [];
    for (let x = 1; x <= STEPS; x++) for (let y = 1; y <= ROWS; y++) {
      const n = note[x][y];
      out.push(n.channel
        ? [n.channel, n.velocity, n.probability, n.condition, n.midiPitch <= 127 ? n.midiPitch : 255]
        : 0);
    }
    return out;
  }
  function restore(data) {
    for (let x = 1; x <= STEPS; x++) for (let y = 1; y <= ROWS; y++) Object.assign(note[x][y], emptyNote());
    if (!Array.isArray(data)) return;
    let i = 0;
    for (let x = 1; x <= STEPS; x++) for (let y = 1; y <= ROWS; y++) {
      const n = data[i++];
      if (Array.isArray(n)) {
        note[x][y].channel = n[0]; note[x][y].velocity = n[1];
        note[x][y].probability = n[2]; note[x][y].condition = n[3];
        note[x][y].midiPitch = n[4] <= 127 ? n[4] : 255;
      }
    }
    queueAutosave();
  }
  function subValue(mode, index) {
    if (mode === "vol" && index === 0) return { text: String(s.mainVol), color: [55, 10, 0], code: "O" };
    if (mode === "vol" && index === 1) return { text: s.gain.toFixed(1), color: [120, 120, 120], code: "W" };
    if (mode === "vol" && index === 3) return { text: String(s.prevVol), color: [0, 120, 120], code: "N" };
    if (mode === "recs" && index === 1) return { text: String(s.micGain), color: [0, 120, 120], code: "N" };
    if (mode === "recs" && index === 2) return { text: String(s.lineInLevel), color: [0, 0, 120], code: "X" };
    const spec = SUB_VALUES[mode][index];
    const pick = subPick[mode][index] % spec.opts.length;
    return { text: spec.opts[pick], color: spec.colors[Math.min(pick, spec.colors.length - 1)], code: spec.codes[Math.min(pick, spec.codes.length - 1)] };
  }
  function applySub(mode, index) {
    if (mode === "look") {
      s.flow = subPick.look[0] === 1;
      s.prevMode = subPick.look[1];
      s.simpleNotes = subPick.look[2] === 1;
      s.pmode = subPick.look[3];
      s.loopLength = subPick.look[4];
      s.voiceMode = subPick.look[5] === 1;
      s.ctrlVol = subPick.look[6] === 1;
      s.cursorType = subPick.look[9];
      s.drawR = subPick.look[10] === 1;
      if (index === 6) deviceChosen = true;
      applyLedLayout();
      if (!s.drawR && s.fullMute) exitFullMute(0);
    }
    if (mode === "etc") {
      if (index === 5) s.scheme = subPick.etc[5];
      if (index === 7) s.childLock = subPick.etc[7] === 1;
      const fireChoices = [0, 15, 1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14];
      const fireIdx = subPick.etc[8] | 0;
      s.fireVoice = fireChoices[fireIdx] ?? 0;
    }
    if (mode === "vol") {
      s.stereo = subPick.vol[4] || 0;
      applyMix();
    }
    if (mode === "recs") {
      s.recInput = subPick.recs[0] || 0;
      s.fastRecMode = subPick.recs[3] || 0;
      s.recChannelClear = subPick.recs[4] || 0;
    }
    saveEeprom();
  }
  let playlist = null;
  let playIdx = 0;
  let lastRandomPreview = null;
  function stepRandom(dir) {
    if (!playlist) playlist = sampleWavs();
    if (!playlist.length) return;
    const ch = s.GLOB.currentChannel;
    playIdx = (playIdx + (dir > 0 ? 1 : -1) + playlist.length) % playlist.length;
    const file = playlist[playIdx];
    lastRandomPreview = file;
    s.soloArrow = dir < 0 ? "?<" : "?>";
    s.soloArrowAt = performance.now();
    engine.loadUrl(ch, file.url).then(() => {
      s.sampleUrl[ch] = file.url;
      engine.trigger(ch, 110, engine.now(), { ...wavOpts(ch), seek: 0, end: 100 }, ch + 1);
    });
  }
  function loadLastRandom() {
    const ch = s.GLOB.currentChannel;
    if (ch < 1 || ch > 8) return;
    const file = lastRandomPreview || (playlist && playlist[playIdx]);
    if (!file) return;
    engine.loadUrl(ch, file.url).then(() => {
      s.sampleUrl[ch] = file.url;
      s.wavName = (file.name || "SAMPLE").replace(/\.wav$/i, "").slice(0, 8);
      engine.trigger(ch, 110, engine.now(), { ...wavOpts(ch), seek: 0, end: 100 }, ch + 1);
    });
  }
  function enterSolo() {
    if (s.solo || s.childLock) return;
    s.soloSaved = s.mute.slice();
    for (let i = 0; i < s.mute.length; i++) s.mute[i] = i !== s.GLOB.currentChannel;
    s.solo = true;
  }
  function exitSolo() {
    if (!s.solo) return;
    if (s.soloSaved) s.soloSaved.forEach((v, i) => { s.mute[i] = v; });
    s.solo = false;
    s.soloSaved = null;
    s.soloArrow = "";
  }
  function backToGrid() { s.mode = s.GLOB.singleMode ? "single" : "draw"; }
  function finishLoad() {
    s.GLOB.singleMode = false;
    s.mode = "draw";
  }
  function writeGenre(type) {
    const pages = type === 0 ? 0 : clamp(s.genreLength, 1, PAGES);
    generateGenre(note, type, pages, COLS, STEPS);
    const drift = type === 0 ? 0 : Math.floor(Math.random() * 9) - 4;
    s.bpm = clamp((GENRES[type]?.bpm || 100) + drift, 40, 240);
    s.GLOB.edit = 1;
    s.GLOB.page = 1;
    s.GLOB.x = 1;
    s.beat = 1;
    saveEeprom();
    queueAutosave();
    finishLoad();
  }
  function openMenuPage() {
    const name = MENU_PAGES[s.menuIndex].name;
    if (name === "DAT") s.mode = "dat";
    else if (name === "KIT") s.mode = "kit";
    else if (name === "WAV" && s.GLOB.currentChannel >= 1 && s.GLOB.currentChannel <= 8) {
      refreshBrowse();
      s.mode = "wav";
      previewWav();
    } else if (name === "BPM") s.mode = "bpm";
    else if (name === "VOL") { s.subIndex = 0; s.mode = "vol"; }
    else if (name === "SETT") { s.subIndex = 0; s.mode = "look"; }
    else if (name === "RECS") { s.subIndex = 0; s.mode = "recs"; }
    else if (name === "MIDI") { s.subIndex = 0; s.mode = "midi"; }
    else if (name === "SONG") s.mode = "song";
    else if (name === "ETC") { s.subIndex = 0; s.mode = "etc"; }
  }

  function turn(enc, dir) {
    if (s.mode === "boot") return;
    engine.click();
    s.knobAngle[enc] += dir * 15;
    if (hold[enc]) hold[enc].turned = true;
    if (s.mode === "velocity") {
      if (enc === 0) s.vel.v = clamp(s.vel.v + dir, 1, 16);
      else if (enc === 1) s.vel.p = clamp(s.vel.p + dir, 1, 5);
      else if (enc === 2) s.vel.c = clamp(s.vel.c + dir, 1, 11);
      else s.vel.vol = clamp(s.vel.vol + dir, 0, 16);
      applyVelocity();
      return;
    }
    if (s.mode === "filter") {
      const page = filterPagesFor(s.GLOB.currentChannel)[filterPages[s.GLOB.currentChannel]] || filterPagesFor(s.GLOB.currentChannel)[0];
      const spec = page[enc];
      if (!spec) return;
      filt[s.GLOB.currentChannel][spec.key] = clamp(filt[s.GLOB.currentChannel][spec.key] + dir, 0, spec.max ?? 32);
      s.filterTouch = enc;
      s.filterTouchAt = performance.now();
      applyFilt(s.GLOB.currentChannel);
      return;
    }
    if (s.mode === "shift") {
      if (enc === 3) shiftNotes(dir, 0, false);
      else if (enc === 0) shiftNotes(0, -dir, false);
      else if (enc === 1) shiftNotes(0, -dir, true);
      return;
    }
    if (s.solo && (s.mode === "draw" || s.mode === "single")) {
      const ch = s.GLOB.currentChannel;
      if (enc === 0 && ch >= 1 && ch <= 8) {
        invOf[ch] = dir < 0;
        s.soloArrow = dir < 0 ? "<<" : ">>";
        s.soloArrowAt = performance.now();
      } else if (enc === 2) nudgeFast(dir);
      else if (enc === 3 && ch >= 1 && ch <= 8) stepRandom(dir);
      return;
    }
    if (s.mode === "draw" || s.mode === "single") {
      if (enc === 0) {
        const prevY = s.GLOB.y;
        s.GLOB.y = clamp(s.GLOB.y + dir, 1, ROWS);
        syncChannel();
        if (s.GLOB.y !== prevY) applyVoiceEdit(s.GLOB.currentChannel);
        if (s.GLOB.y !== prevY && s.cursorType === 1 && s.mode === "draw" && s.GLOB.currentChannel >= 1 && s.GLOB.currentChannel <= 8) {
          s.chNr = s.GLOB.currentChannel;
          s.chNrAt = performance.now();
          s.pageNrAt = 0;
        }
      } else if (enc === 1) {
        if (s.ctrlVol) {
          const ch = s.GLOB.currentChannel;
          chVol[ch] = clamp((chVol[ch] ?? 16) + dir, 0, 16);
          s.volBar = chVol[ch];
          s.volBarAt = performance.now();
          applyFilt(ch);
        } else if (!s.childLock) {
          const prev = s.GLOB.edit;
          if (s.voiceMode && s.pmode !== 2) {
            refreshVoiceLens();
            const ch = s.GLOB.currentChannel;
            const pages = filledPages(ch);
            let next = s.GLOB.edit + dir;
            if (pages.length) {
              next = pages.includes(s.GLOB.edit) ? (stepFilled(ch, s.GLOB.edit, dir) || s.GLOB.edit) : nearestFilled(ch, s.GLOB.edit);
            } else next = clamp(next, 1, PAGES);
            viewPage(next);
            s.voiceEdit[ch] = s.GLOB.edit;
            cueVoice(ch, s.GLOB.edit);
          } else viewPage(s.GLOB.edit + dir);
          if (s.GLOB.edit !== prev && s.cursorType === 1) {
            s.pageNr = s.GLOB.edit;
            s.pageNrAt = performance.now();
            s.chNrAt = 0;
          }
        }
      } else if (enc === 2) nudgeFast(dir);
      else {
        // Encoder 4 only moves the cursor. Paint/erase is short-press (or
        // firmware paintMode via long-press) — not "button down while turning",
        // which the pointer gesture always looks like.
        let page = s.GLOB.edit;
        let local = localX(s.GLOB.x) + dir;
        const lockVoicePage = s.voiceMode && s.playing && s.pmode !== 2;
        if (!s.childLock && !lockVoicePage && local > COLS && page < PAGES) { page += 1; local = 1; }
        else if (!s.childLock && !lockVoicePage && local < 1 && page > 1) { page -= 1; local = COLS; }
        else local = clamp(local, 1, COLS);
        viewPage(page, local);
      }
      return;
    }
    if (s.mode === "menu" && enc === 3) s.menuIndex = clamp(s.menuIndex + dir, 0, MENU_PAGES.length - 1);
    if (s.mode === "look" && s.subIndex === 11 && enc === 1) {
      s.muteSel = clamp(s.muteSel + dir, 1, 16);
      return;
    }
    if (s.mode === "bpm" && enc === 1) {
      s.ledBrightness = clamp(s.ledBrightness + dir * 4, 3, 255);
      saveEeprom();
      return;
    }
    if (s.mode === "new") {
      if (enc === 2) s.genre = clamp(s.genre + dir, 0, GENRES.length - 1);
      else if (enc === 1 && s.genre) s.genreLength = clamp(s.genreLength + dir, 1, PAGES);
      return;
    }
    if (s.mode === "pat") {
      if (enc === 2) s.subIndex = clamp(s.subIndex + dir, 0, SUBS.pat.length - 1);
      else if (enc === 3) s.genreLength = clamp(s.genreLength + dir, 1, PAGES);
      return;
    }
    if (s.mode === "etc" && s.subIndex === 8) {
      if (enc === 0) { s.fireSize = clamp((s.fireSize || 1) + dir, 1, 4); saveEeprom(); return; }
      if (enc === 1) { s.fireLevel = clamp((s.fireLevel || 1) + dir, 1, 25); saveEeprom(); return; }
      if (enc === 2 && s.fireFocus === 1) { s.fireGravity = clamp((s.fireGravity | 0) + dir, 0, 8); saveEeprom(); return; }
      if (enc === 2 && s.fireFocus === 2) { s.fireColor = clamp((s.fireColor == null ? 8 : s.fireColor) + dir, 0, 8); saveEeprom(); return; }
    }
    if (SUBS[s.mode] && enc === 3) s.subIndex = clamp(s.subIndex + dir, 0, SUBS[s.mode].length - 1);
    if (SUBS[s.mode] && enc === 2) {
      if (s.mode === "vol" && s.subIndex === 0) s.mainVol = clamp(s.mainVol + dir, 0, 100);
      else if (s.mode === "vol" && s.subIndex === 1) s.gain = clamp(Math.round((s.gain + dir * 0.1) * 10) / 10, 0, 2);
      else if (s.mode === "vol" && s.subIndex === 3) s.prevVol = clamp(s.prevVol + dir, 0, 16);
      else if (s.mode === "recs" && s.subIndex === 1) { s.micGain = clamp(s.micGain + dir, 0, 63); saveEeprom(); return; }
      else if (s.mode === "recs" && s.subIndex === 2) { s.lineInLevel = clamp(s.lineInLevel + dir, 0, 15); saveEeprom(); return; }
      else if (s.mode === "look" && s.subIndex === 11) {
        const bit = 1 << (s.muteSel === 16 ? 0 : s.muteSel);
        s.muteMask = dir > 0 ? s.muteMask | bit : s.muteMask & ~bit;
        saveEeprom();
        return;
      }
      else {
        const spec = SUB_VALUES[s.mode][s.subIndex];
        subPick[s.mode][s.subIndex] = clamp(subPick[s.mode][s.subIndex] + (dir > 0 ? 1 : -1), 0, spec.opts.length - 1);
      }
      applySub(s.mode, s.subIndex);
    }
    if (s.mode === "dat" && enc === 3) s.slot = clamp(s.slot + dir, 0, 99);
    if (s.mode === "kit" && enc === 3) {
      s.packSlot = clamp(s.packSlot + dir, 0, 99);
      saveEeprom();
    }
    if (s.mode === "wav") {
      const ch = s.GLOB.currentChannel;
      if (s.wavFile && enc === 0) seekOf[ch] = clamp(seekOf[ch] + dir, 0, endOf[ch] - 1);
      else if (s.wavFile && enc === 1) endOf[ch] = clamp(endOf[ch] + dir, seekOf[ch] + 1, 100);
      else if (enc === 3) {
        s.browse = clamp(s.browse + dir, 0, Math.max(0, s.browseItems.length - 1));
        browseIdxOf[ch] = s.browse;
        const item = currentFile();
        s.wavFile = isWav(item);
        s.wavName = labelOf(item);
        if (s.wavFile && item.url !== s.shownWav) {
          seekOf[ch] = 0;
          endOf[ch] = 100;
          s.shownWav = item.url;
          scanPeaks(item);
        } else if (!s.wavFile) {
          s.shownWav = "";
          peakToken++;
          s.peaks = null;
        }
      }
      if (s.wavFile && (enc === 0 || enc === 1 || enc === 3)) previewWav(false);
    }
    if (s.mode === "bpm" && enc === 2 && s.clockInt !== (dir > 0)) { s.clockInt = dir > 0; saveEeprom(); }
    if (s.mode === "bpm" && enc === 3) { s.bpm = clamp(s.bpm + dir, 40, 240); saveEeprom(); }
    if (s.mode === "song" && enc === 1) s.songPattern = clamp(s.songPattern + dir, 1, 16);
    if (s.mode === "song" && enc === 3) s.songPos = clamp(s.songPos + dir, 1, 64);
  }

  function exitFullMute(enc) {
    s.fullMute = false;
    for (let ch = 0; ch < s.mute.length; ch++) {
      if (enc === 0) s.mute[ch] = !!s.fullMuteSaved?.[ch];
      else if (enc === 1) s.mute[ch] = ch !== 1 && ch !== 2;
      else if (enc === 2) s.mute[ch] = !(s.muteMask & (1 << ch));
      else s.mute[ch] = false;
    }
  }

  function shortPress(enc) {
    if (s.mode === "shift") {
      if (enc === 1) backToGrid();
      return;
    }
    if (s.mode === "draw" || s.mode === "single") {
      if (s.solo) {
        // Last encoder short press loads the last random preview (firmware 0201 / 0001).
        if (enc === 3) {
          loadLastRandom();
          return;
        }
      }
      if (s.drawR && s.fullMute && s.GLOB.y < 16) {
        exitFullMute(enc);
        return;
      }
      if (s.drawR && enc === 0 && s.GLOB.y < 16 && !s.childLock) {
        s.fullMuteSaved = s.mute.slice();
        s.fullMute = true;
        return;
      }
      if (enc === 0) eraseCell(s.GLOB.x, s.GLOB.y);
      if (enc === 1) {
        if (s.mode === "single" && s.GLOB.y === 16) s.mode = "shift";
        else s.mute[s.GLOB.currentChannel] = !s.mute[s.GLOB.currentChannel];
      }
      if (enc === 2) togglePlay();
      if (enc === 3) {
        if (s.drawR) paintPress(s.GLOB.x, s.GLOB.y);
        else paintPress(s.GLOB.x, s.GLOB.y);
      }
      return;
    }
    if (s.mode === "filter" && enc <= 3) { filt[s.GLOB.currentChannel].fast = enc; return; }
    if (s.mode === "new" && enc === 0) {
      writeGenre(s.genre);
      return;
    }
    if (s.mode === "new" && enc === 3) { s.mode = "dat"; return; }
    if (s.mode === "pat" && enc === 3) {
      writeGenre(s.subIndex + 1);
      return;
    }
    if (s.mode === "dat" && enc === 0) {
      if (s.slot !== 0 && !slotExists("toern-web-patterns", s.slot)) { s.mode = "new"; return; }
      const url = patternUrl(s.slot);
      const stored = loadStore("toern-web-patterns")[String(s.slot)];
      if (s.slot === 0 && Array.isArray(stored)) {
        restore(stored);
        finishLoad();
      } else if (url) {
        parsePattern(url).then((data) => {
          if (!data) return;
          restore(data);
          finishLoad();
        });
      } else {
        restore(stored != null ? stored : demoPattern(s.slot));
        finishLoad();
      }
    }
    if (s.mode === "dat" && enc === 1 && s.slot !== 0) {
      saveSlot("toern-web-patterns", s.slot, snapshot());
      s.okAt = performance.now();
      finishLoad();
      return;
    }
    if (s.mode === "kit" && enc === 0) {
      const urls = packWavs(s.packSlot);
      if (urls.some(Boolean)) {
        urls.forEach((url, i) => { if (url) engine.loadUrl(i + 1, url); });
        s.wavName = `PACK ${s.packSlot}`;
      } else {
        const stored = loadStore("toern-web-packs")[String(s.packSlot)];
        applyPack(stored && stored.wavs ? stored : DUMMY_PACKS[s.packSlot]);
      }
      finishLoad();
      saveEeprom();
    }
    if (s.mode === "kit" && enc === 1) {
      saveSlot("toern-web-packs", s.packSlot, {
        name: DUMMY_PACKS[s.packSlot]?.name || "PACK",
        wavs: [1, 2, 3, 4, 5, 6, 7, 8].map((ch) => wavOf[ch] || 0),
      });
      s.okAt = performance.now();
      finishLoad();
      return;
    }
    if (s.mode === "wav" && enc === 1 && s.wavFile) {
      invOf[s.GLOB.currentChannel] = !invOf[s.GLOB.currentChannel];
      previewWav(true);
    }
    if (s.mode === "wav" && enc === 0) previewWav(true);
    if (s.mode === "wav" && enc === 3) browserPress();
    if (s.mode === "rec") {
      if (enc === 1) startMicRec();
      else if (enc === 2) {
        if (s.recOn) stopMicRec();
        else playMicRec();
      } else if (enc === 3) {
        closeMic();
        s.mode = "wav";
      }
      return;
    }
    if (s.mode === "wav" && enc === 2) {
      const item = currentFile();
      if (item?.url && item.name.endsWith(".wav")) {
        const ch = s.GLOB.currentChannel;
        engine.loadUrl(ch, item.url);
        s.sampleUrl[ch] = item.url;
        s.wavName = item.name.replace(/\.wav$/i, "").slice(0, 8);
        finishLoad();
      }
    }
    if (s.mode === "song" && enc === 0) song[s.songPos - 1] = 0;
    if (s.mode === "song" && (enc === 1 || enc === 3)) song[s.songPos - 1] = s.songPattern;
    if (s.mode === "song" && enc === 2) toggleSong();
    if (s.mode === "etc" && s.subIndex === 8 && enc === 2) { s.fireFocus = ((s.fireFocus | 0) + 1) % 3; return; }
    if (s.mode === "etc" && s.subIndex === 9 && enc === 3) { runReset(); return; }
    if (enc === 3 && s.mode === "menu") openMenuPage();
  }

  function cycleFilter(dir = 1) {
    const ch = s.GLOB.currentChannel;
    const n = filterPagesFor(ch).length;
    filterPages[ch] = (filterPages[ch] + dir + n) % n;
    s.filterPage = filterPages[ch];
  }
  function resetFilterPage(ch) {
    const page = filterPagesFor(ch)[filterPages[ch]] || filterPagesFor(ch)[0];
    const defaults = emptyFilt();
    for (const spec of page) if (spec) filt[ch][spec.key] = defaults[spec.key];
    applyFilt(ch);
  }
  function resetVoiceFilters(ch) {
    Object.assign(filt[ch], emptyFilt());
    applyFilt(ch);
  }
  function tapTempo() {
    if (!s.clockInt) return;
    const now = performance.now();
    if (taps.length && now - taps[taps.length - 1] > 2000) taps.length = 0;
    taps.push(now);
    if (taps.length > 4) taps.shift();
    if (taps.length < 2) return;
    let sum = 0;
    for (let i = 1; i < taps.length; i++) sum += taps[i] - taps[i - 1];
    const bpm = Math.round(60000 / (sum / (taps.length - 1)));
    s.bpm = clamp(bpm, 40, 240);
    saveEeprom();
  }
  let recStream = null;
  let recProc = null;
  let recChunks = [];
  let recTake = null;
  async function openMic() {
    if (recStream) return recStream;
    const ctx = engine.ensure();
    if (ctx.state === "suspended") await ctx.resume();
    recStream = await navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false } });
    const src = ctx.createMediaStreamSource(recStream);
    const sink = ctx.createGain();
    sink.gain.value = 0;
    recProc = ctx.createScriptProcessor(2048, 1, 1);
    recProc.onaudioprocess = (e) => {
      const data = e.inputBuffer.getChannelData(0);
      let peak = 0;
      for (let i = 0; i < data.length; i += 8) peak = Math.max(peak, Math.abs(data[i]));
      s.recLevel = peak;
      if (s.recOn) recChunks.push(new Float32Array(data));
    };
    src.connect(recProc);
    recProc.connect(sink);
    sink.connect(ctx.destination);
    return recStream;
  }
  async function startMicRec() {
    if (s.recOn) return;
    try {
      await openMic();
    } catch {
      s.wavName = "MIC?";
      return;
    }
    recChunks = [];
    s.recOn = true;
    s.recPlay = false;
    s.recAt = performance.now();
  }
  function stopMicRec() {
    if (!s.recOn) return;
    s.recOn = false;
    const ctx = engine.ensure();
    const parts = recChunks;
    recChunks = [];
    let n = 0;
    for (const part of parts) n += part.length;
    if (!n) return;
    const buffer = ctx.createBuffer(1, n, ctx.sampleRate);
    const out = buffer.getChannelData(0);
    let o = 0;
    for (const part of parts) { out.set(part, o); o += part.length; }
    const ch = s.GLOB.currentChannel;
    recTake = buffer;
    engine.loadBuffer(ch, buffer);
    s.sampleUrl[ch] = null;
    s.wavName = "REC";
    s.peaks = null;
  }
  function playMicRec() {
    if (!recTake) return;
    const ch = s.GLOB.currentChannel;
    engine.preview({ index: 0, seek: 0, end: 100, inv: false, velocity: 100, buffer: recTake, env: wavOpts(ch).env });
    s.recPlay = true;
    s.recPlayAt = performance.now();
  }
  function closeMic() {
    if (s.recOn) stopMicRec();
    recProc?.disconnect();
    recProc = null;
    recStream?.getTracks().forEach((t) => t.stop());
    recStream = null;
  }
  function clearChannelNotes(ch) {
    for (let x = 1; x <= STEPS; x++) {
      for (let y = 1; y <= ROWS; y++) {
        if (note[x][y].channel === ch) Object.assign(note[x][y], emptyNote());
      }
    }
  }
  function placeRecTrigger(atBeat = true) {
    const ch = s.GLOB.currentChannel;
    if (ch < 1 || ch > 8) return;
    const y = atBeat ? ch + 1 : s.GLOB.y;
    const x = atBeat ? (s.playing ? s.beat : s.GLOB.x) : s.GLOB.x;
    if (x < 1 || x > STEPS || y < 1 || y > 15) return;
    Object.assign(note[x][y], emptyNote(), {
      channel: ch,
      velocity: 100,
      probability: 100,
      condition: 1,
    });
  }
  async function startFastRec() {
    if (s.fastRecActive || s.recOn) return;
    const ch = s.GLOB.currentChannel;
    if (ch < 1 || ch > 8) return;
    if (s.recChannelClear === 1) clearChannelNotes(ch);
    if (s.recChannelClear !== 2) placeRecTrigger(true);
    try {
      await openMic();
    } catch {
      s.wavName = "MIC?";
      return;
    }
    recChunks = [];
    s.recOn = true;
    s.recPlay = false;
    s.recAt = performance.now();
    s.fastRecActive = true;
    s.wavName = "REC…";
  }
  function stopFastRec() {
    if (!s.fastRecActive) return;
    s.fastRecActive = false;
    stopMicRec();
    queueAutosave();
  }
  function onTouch3(down) {
    if (s.mode === "filter") {
      if (down) resetFilterPage(s.GLOB.currentChannel);
      return;
    }
    if (s.mode === "bpm") {
      if (down) tapTempo();
      return;
    }
    if (s.mode === "wav" && s.GLOB.currentChannel >= 1 && s.GLOB.currentChannel <= 8) {
      if (!down) return;
      s.recOn = false;
      s.recPlay = false;
      s.recLevel = 0;
      s.fastRecActive = false;
      s.mode = "rec";
      openMic().catch(() => { s.wavName = "MIC?"; });
      return;
    }
    if (s.mode !== "draw" && s.mode !== "single") return;
    const ch = s.GLOB.currentChannel;
    const clr = s.recChannelClear;
    const trig = s.fastRecMode;

    if (down && clr === 4 && s.playing) {
      const y = s.GLOB.y;
      if (y >= 1 && y <= 15 && ch >= 1 && ch <= 8) {
        Object.assign(note[s.beat][y], emptyNote(), {
          channel: ch, velocity: 100, probability: 100, condition: 1,
        });
        queueAutosave();
      }
      return;
    }
    if (down && s.playing && s.GLOB.y === 1) {
      togglePlay();
      return;
    }
    // TRIG=SENS: hold T3 to record directly into current sample voice
    if (trig === 1 && clr !== 3) {
      if (down) {
        if (s.GLOB.y <= 1 || ch < 1 || ch > 8) return;
        startFastRec();
      } else if (s.fastRecActive) {
        stopFastRec();
      }
    }
  }
  function onMeta(down) {
    touch.m = down;
    if (down && s.mode === "filter") { cycleFilter(); return; }
    if (down) {
      if (!touch.first) touch.first = 2;
      if (touch.l && touch.first === 2 && (s.mode === "draw" || s.mode === "single")) { enterFilter(); touch.chord = true; }
      if (touch.l && touch.first === 1 && (s.mode === "draw" || s.mode === "single") && s.GLOB.currentChannel >= 1 && s.GLOB.currentChannel <= 8) {
        refreshBrowse();
        s.mode = "wav";
        previewWav();
        touch.chord = true;
      }
      return;
    }
    if (!touch.chord && touch.first === 2 && !touch.l) {
      if (s.childLock && (s.mode === "draw" || s.mode === "single")) touch.arm = true;
      else if (s.mode === "draw" || s.mode === "single") s.mode = "menu";
      else if (s.mode !== "filter" && s.mode !== "velocity") backToGrid();
    }
    if (!touch.l) { touch.first = 0; touch.chord = false; }
  }
  function toggleSingle() {
    if (s.mode === "single") {
      s.GLOB.singleMode = false;
      s.mode = "draw";
      return;
    }
    if (s.GLOB.y >= 16) return;
    const ch = channelOfRow(s.GLOB.y);
    if ((ch >= 1 && ch <= 8) || ch === 11 || ch === 13 || ch === 14) {
      s.GLOB.currentChannel = ch;
      s.GLOB.singleMode = true;
      s.mode = "single";
    }
  }
  function onTouchL(down) {
    touch.l = down;
    if (down) {
      if (s.mode === "filter") {
        s.GLOB.singleMode = false;
        s.mode = "draw";
        touch.chord = true;
        return;
      }
      if (!touch.first) touch.first = 1;
      if (touch.m && touch.first === 2 && (s.mode === "draw" || s.mode === "single")) { enterFilter(); touch.chord = true; }
      if (touch.m && touch.first === 1 && s.GLOB.currentChannel >= 1 && s.GLOB.currentChannel <= 8) {
        refreshBrowse();
        s.mode = "wav";
        previewWav();
        touch.chord = true;
      }
      return;
    }
    if (touch.arm && !touch.m && (s.mode === "draw" || s.mode === "single")) {
      touch.arm = false;
      s.mode = "menu";
    } else if (!touch.chord && touch.first === 1 && !touch.m && s.mode === "draw" && s.GLOB.y === 1) {
      togglePlay();
    } else if (!touch.chord && touch.first === 1 && !touch.m && (s.mode === "draw" || s.mode === "single")) {
      if (s.mode === "single") {
        s.GLOB.currentChannel = channelOfRow(s.GLOB.y);
        s.GLOB.singleMode = false;
        s.mode = "draw";
      } else if (s.GLOB.y < 16) {
        const ch = channelOfRow(s.GLOB.y);
        if ((ch >= 1 && ch <= 8) || ch === 11 || ch === 13 || ch === 14) {
          s.GLOB.currentChannel = ch;
          s.GLOB.singleMode = true;
          s.mode = "single";
        }
      }
    }
    if (!touch.chord && s.mode !== "draw" && s.mode !== "single" && s.mode !== "boot") backToGrid();
    if (!touch.m) { touch.first = 0; touch.chord = false; }
  }

  function isTab(e) { return e.code === "Tab" || e.key === "Tab"; }
  function isEncoder2Click(e) {
    return e.key === "Alt" || e.code === "AltLeft" || e.code === "AltRight"
      || e.key === "Shift" || e.code === "ShiftLeft" || e.code === "ShiftRight";
  }
  function isTouch1Key(e) { return e.key === "ß" || e.key === "ẞ" || e.key === "-" || e.code === "Minus"; }
  function isTouch2Key(e) { return e.key === "´" || e.code === "Equal"; }

  /** True when hardware-sim keys should capture Tab/Space/Enter/arrows. */
  function deviceKeysActive() {
    // Global capture unless assistive mode gates keys to the focused device.
    if (!s.assistKeyGate) return true;
    const ae = document.activeElement;
    if (!ae || ae === document.body) return !!s.deviceFocus;
    if (ae.id === "matrix") return true;
    if (ae.closest?.(".jump")) return false;
    if (ae.closest?.(".key-legend")) return false;
    const tag = ae.tagName;
    if (tag === "INPUT" || tag === "TEXTAREA" || tag === "SELECT" || tag === "BUTTON") return false;
    if (ae.isContentEditable) return false;
    return !!s.deviceFocus;
  }

  function cellAtCursor() {
    const n = note[s.GLOB.x]?.[s.GLOB.y];
    if (!n?.channel) return { empty: true, step: s.GLOB.x, row: s.GLOB.y, page: s.GLOB.edit };
    return {
      empty: false,
      step: s.GLOB.x,
      row: s.GLOB.y,
      page: s.GLOB.edit,
      channel: n.channel,
      velocity: n.velocity,
      probability: n.probability,
      condition: n.condition,
    };
  }

  function pageNotes() {
    const base = (s.GLOB.edit - 1) * COLS;
    const out = [];
    for (let x = 1; x <= COLS; x++) {
      for (let y = 1; y <= ROWS; y++) {
        const n = note[base + x][y];
        if (!n?.channel) continue;
        if (s.GLOB.singleMode && n.channel !== s.GLOB.currentChannel) continue;
        out.push({
          col: x,
          row: y,
          step: base + x,
          channel: n.channel,
          velocity: n.velocity,
          probability: n.probability,
        });
      }
    }
    return out;
  }

  function menuSummary() {
    const mode = s.mode;
    if (mode === "draw" || mode === "single") {
      return { title: mode === "single" || s.GLOB.singleMode ? "Single" : "Draw", detail: `Page ${s.GLOB.edit}` };
    }
    if (SUBS[mode]) {
      const pages = SUBS[mode];
      const name = pages[s.subIndex] || mode;
      const value = subValue(mode, s.subIndex);
      return { title: mode.toUpperCase(), detail: `${name}: ${value.text}` };
    }
    if (mode === "menu") {
      const page = MENU_PAGES[s.menuIndex];
      return { title: "Menu", detail: page?.name || String(s.menuIndex + 1) };
    }
    if (mode === "dat") return { title: "FILE", detail: s.slot === 0 ? "Autosave" : `Slot ${s.slot}` };
    if (mode === "bpm") return { title: "BPM", detail: String(s.bpm) };
    if (mode === "wav") return { title: "WAV", detail: s.wavName || "" };
    if (mode === "filter") return { title: "Filter", detail: `Voice ${s.GLOB.currentChannel}` };
    if (mode === "velocity") return { title: "Velocity", detail: `Voice ${s.GLOB.currentChannel}` };
    return { title: mode, detail: "" };
  }

  function announceState() {
    if (typeof s.onAnnounce !== "function") return;
    const cell = cellAtCursor();
    const label = s.mode === "draw" && s.GLOB.singleMode ? "single" : s.mode;
    let cellText = "empty";
    if (!cell.empty) cellText = `voice ${cell.channel}, velocity ${cell.velocity}`;
    const menu = menuSummary();
    const parts = [
      label,
      `page ${s.GLOB.edit}`,
      s.playing ? "playing" : "stopped",
      `voice ${s.GLOB.currentChannel}`,
    ];
    if (s.mode === "draw" || s.mode === "single") {
      parts.push(`cursor step ${s.GLOB.x}, row ${s.GLOB.y}`, cellText);
    } else {
      parts.push(menu.title, menu.detail);
    }
    s.onAnnounce(parts.filter(Boolean).join(". "));
  }

  function keydown(e) {
    const active = deviceKeysActive();
    const ue = e.code === "BracketLeft" || e.key === "ü" || e.key === "Ü" || e.key === "[";
    const turning = ["KeyQ", "KeyW", "KeyR", "KeyT", "KeyU", "KeyI", "KeyP", "BracketLeft", "ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown"].includes(e.code) || ue;
    const steal = isTab(e) || isEncoder2Click(e) || isTouch1Key(e) || isTouch2Key(e) || e.key === " " || e.key === "Enter" || e.key === "ArrowLeft" || e.key === "ArrowRight" || e.key === "ArrowUp" || e.key === "ArrowDown";
    if (steal && active) {
      e.preventDefault();
      e.stopPropagation();
    }
    if (!active) return;
    if (e.repeat && !turning) return;
    if (s.mode === "boot") return;
    if (e.key === "Escape") {
      if (s.fastRecActive) stopFastRec();
      if (s.mode === "rec") closeMic();
      if (s.mode === "single") {
        s.GLOB.singleMode = false;
        s.mode = "draw";
      } else if (s.mode !== "draw") backToGrid();
      announceState();
      return;
    }
    if (e.key >= "1" && e.key <= "8" && !e.metaKey && !e.ctrlKey && !e.altKey) { selectVoice(Number(e.key)); announceState(); return; }
    if (e.code === "KeyF") {
      if (s.mode === "filter") { s.GLOB.singleMode = false; s.mode = "draw"; announceState(); return; }
      if (s.mode === "draw" || s.mode === "single") { enterFilter(); announceState(); return; }
    }
    if (e.code === "KeyS" && (s.mode === "draw" || s.mode === "single")) { toggleSingle(); announceState(); return; }
    if (e.code === "KeyL" && s.GLOB.currentChannel >= 1 && s.GLOB.currentChannel <= 8) {
      if (s.mode === "wav") { backToGrid(); announceState(); return; }
      if (s.mode === "draw" || s.mode === "single") { refreshBrowse(); s.mode = "wav"; previewWav(); announceState(); return; }
    }
    if (e.code === "KeyD" && (s.mode === "draw" || s.mode === "single")) { clearPage(); announceState(); return; }
    const beforeX = s.GLOB.x;
    const beforeY = s.GLOB.y;
    if (e.code === "KeyQ") turn(0, 1);
    else if (e.code === "KeyW") turn(0, -1);
    else if (e.key === "ArrowDown") turn(0, -1);
    else if (e.key === "ArrowUp") turn(0, 1);
    else if (e.code === "KeyR") turn(1, -1);
    else if (e.code === "KeyT") turn(1, 1);
    else if (e.code === "KeyU") turn(2, -1);
    else if (e.code === "KeyI") turn(2, 1);
    else if (e.key === "ArrowLeft" || e.code === "KeyP") turn(3, -1);
    else if (e.key === "ArrowRight" || ue) turn(3, 1);
    else if (isTab(e)) {
      if (s.mode === "single" && s.GLOB.y === 16) hold[0] = { at: performance.now(), turned: false, fired: false };
      else {
        hold[0] = { at: performance.now(), turned: false, fired: true };
        shortPress(0);
      }
    } else if (isEncoder2Click(e)) {
      if ((s.mode === "draw" || s.mode === "single") && s.GLOB.y !== 16) hold[1] = { at: performance.now(), turned: false, fired: false };
      else shortPress(1);
    } else if (e.key === " ") {
      if (s.mode === "filter") togglePlay();
      else hold[2] = { at: performance.now(), turned: false };
    } else if (e.key === "Enter") {
      if (s.mode === "velocity") { backToGrid(); announceState(); return; }
      if (s.mode === "draw" || s.mode === "single" || s.mode === "filter") hold[3] = { at: performance.now(), turned: false, had: !!note[s.GLOB.x][s.GLOB.y].channel };
      else shortPress(3);
    } else if (e.key === "m" || e.key === "M") onMeta(true);
    else if (isTouch1Key(e)) onTouchL(true);
    else if (isTouch2Key(e)) onMeta(true);
    if (s.GLOB.x !== beforeX || s.GLOB.y !== beforeY) announceState();
  }
  function keyup(e) {
    if (!deviceKeysActive()) return;
    if (isTab(e) && hold[0]) {
      const h = hold[0]; hold[0] = null;
      if (!h.fired && !h.turned) shortPress(0);
      announceState();
    } else if (isEncoder2Click(e)) {
      const h = hold[1]; hold[1] = null;
      if (s.solo) exitSolo();
      else if (h && !h.fired && !h.turned) shortPress(1);
      announceState();
    } else if (e.key === " " && hold[2]) {
      const h = hold[2]; hold[2] = null;
      if (!h.fired && !h.turned) shortPress(2);
      announceState();
    } else if (e.key === "Enter" && hold[3]) {
      const h = hold[3]; hold[3] = null;
      if (h.fired) return;
      if (!h.turned) shortPress(3);
      announceState();
    } else if (e.key === "m" || e.key === "M") onMeta(false);
    else if (isTouch1Key(e)) onTouchL(false);
    else if (isTouch2Key(e)) onMeta(false);
  }
  function tick(now) {
    if (!s.bootAt) s.bootAt = now;
    if (s.mode === "single" && s.GLOB.y === 16 && hold[0] && !hold[0].turned && !hold[0].fired && now - hold[0].at >= LONG_MS) {
      hold[0].fired = true;
      drawRandomPage();
    }
    if ((s.mode === "draw" || s.mode === "single") && hold[3] && !hold[3].turned && !hold[3].fired && hold[3].had && now - hold[3].at >= LONG_MS) {
      hold[3].fired = true; enterVelocity();
    }
    if (s.mode === "filter" && hold[3] && !hold[3].turned && !hold[3].fired && now - hold[3].at >= LONG_MS) {
      hold[3].fired = true;
      resetVoiceFilters(s.GLOB.currentChannel);
    }
    if ((s.mode === "draw" || s.mode === "single") && hold[2] && !hold[2].turned && !hold[2].fired && now - hold[2].at >= LONG_MS) {
      hold[2].fired = true; enterFilter();
    }
    if (hold[1] && !hold[1].turned && !s.solo && now - hold[1].at >= LONG_MS && s.GLOB.y !== 16 && (s.mode === "draw" || s.mode === "single")) {
      enterSolo();
    }
    if (s.playing && s.clockInt) {
      const t = engine.now();
      while (nextAt <= t + 0.05) {
        if (s.voiceMode && s.pmode !== 2) {
          refreshVoiceLens();
          const end = Math.max(1, s.voicePages || 1) * COLS;
          const prev = s.beat;
          if (s.beat >= end) {
            s.beat = 1;
            s.GLOB.page = 1;
          } else {
            s.beat += 1;
            s.GLOB.page = Math.floor((s.beat - 1) / COLS) + 1;
          }
          bumpVoiceLoops(prev, s.beat);
        } else if (s.pmode === 1) {
          const start = (s.GLOB.page - 1) * COLS + 1;
          const end = start + COLS - 1;
          if (s.beat < start || s.beat > end) s.beat = start + ((s.beat - 1) % COLS);
          else s.beat += 1;
          if (s.beat > end) s.beat = start;
        } else {
          const end = patternEnd();
          if (s.beat >= end) {
            if (s.pmode === 3 && s.GLOB.edit !== s.GLOB.page) {
              s.GLOB.page = s.GLOB.edit;
              s.beat = (s.GLOB.page - 1) * COLS + 1;
            } else if (s.pmode === 2) {
              if (!advanceSong()) { s.playing = false; break; }
            } else if (s.pmode === 3) {
              s.beat = (s.GLOB.page - 1) * COLS + 1;
            } else {
              s.beat = 1;
              s.GLOB.page = 1;
            }
          } else {
            s.beat += 1;
            s.GLOB.page = Math.floor((s.beat - 1) / COLS) + 1;
          }
        }
        if (s.flow && !s.voiceMode && s.pmode !== 3) s.GLOB.edit = s.GLOB.page;
        fire(s.beat, Math.max(nextAt, t));
        nextAt += stepSec();
      }
    }
    const audioNow = engine.now();
    while (fired.length && fired[0].when <= audioNow) {
      const f = fired.shift();
      s.pulseStep = f.step;
      s.pulseAt = f.when;
    }
    s.beatPos = s.playing && s.pulseStep
      ? (((s.pulseStep - 1) % 4) + (audioNow - s.pulseAt) / stepSec()) / 4
      : 0;
    renderFrame(matrix, s, now);
    const label = s.mode === "draw" && s.GLOB.singleMode ? "single" : s.mode;
    const status = `${label}  ·  page ${s.GLOB.edit}  ·  ${s.playing ? "play" : "stop"}`;
    if (status !== shownStatus) statusEl.textContent = shownStatus = status;
    rings.forEach((el, i) => {
      const [r, g, b] = matrix.rings[i];
      const color = `rgb(${r},${g},${b})`;
      if (color !== shownRing[i]) el.style.setProperty("--enc", shownRing[i] = color);
      if (s.knobAngle[i] !== shownAngle[i]) el.style.setProperty("--turn", `${shownAngle[i] = s.knobAngle[i]}deg`);
    });
    requestAnimationFrame(tick);
  }
  function cellTaken(col, row) {
    if (s.mode !== "draw" && s.mode !== "single") return false;
    const n = note[(s.GLOB.edit - 1) * COLS + col]?.[row];
    if (!n?.channel) return false;
    if (s.GLOB.singleMode && n.channel !== s.GLOB.currentChannel) return false;
    return true;
  }
  function hover(col, row) {
    if (s.mode !== "draw" && s.mode !== "single") return;
    if (col < 1 || row < 1 || col > COLS || row > ROWS) return;
    if (s.GLOB.y === row && localX(s.GLOB.x) === col) return;
    const prevY = s.GLOB.y;
    s.GLOB.y = row;
    s.GLOB.x = (s.GLOB.edit - 1) * COLS + col;
    syncChannel();
    if (s.GLOB.y !== prevY && s.cursorType === 1 && s.mode === "draw" && s.GLOB.currentChannel >= 1 && s.GLOB.currentChannel <= 8) {
      s.chNr = s.GLOB.currentChannel;
      s.chNrAt = performance.now();
    }
  }
  function pointer(col, row, erase, audition) {
    if (s.mode !== "draw" && s.mode !== "single") return;
    if (col < 1 || row < 1 || col > COLS || row > ROWS) return;
    s.GLOB.y = row;
    s.GLOB.x = (s.GLOB.edit - 1) * COLS + col;
    syncChannel();
    if (erase) eraseCell(s.GLOB.x, s.GLOB.y);
    else if (audition) paintPress(s.GLOB.x, s.GLOB.y);
    else paintCell(s.GLOB.x, s.GLOB.y);
  }
  function advanceSong() {
    for (let n = 0; n < 64; n++) {
      s.songPos = s.songPos >= 64 ? 1 : s.songPos + 1;
      const pat = song[s.songPos - 1];
      if (!pat) continue;
      const page = ((pat - 1) % PAGES) + 1;
      s.GLOB.page = page;
      s.GLOB.edit = page;
      s.beat = (page - 1) * COLS + 1;
      return true;
    }
    return false;
  }
  function toggleSong() {
    if (s.playing) {
      s.playing = false;
      queueAutosave();
      subPick.look[3] = 0;
      applySub("look", 3);
      return;
    }
    subPick.look[3] = 2;
    applySub("look", 3);
    const pat = song[s.songPos - 1] || s.songPattern;
    const page = ((pat - 1) % PAGES) + 1;
    s.GLOB.page = page;
    s.GLOB.edit = page;
    s.beat = (page - 1) * COLS + 1;
    applyMix();
    s.playing = true;
    const t = engine.now();
    nextAt = t + stepSec();
    fire(s.beat, t);
  }
  loadEeprom();
  autoload();
  persist = true;
  window.addEventListener("pagehide", () => {
    saveEeprom();
    saveSlot("toern-web-patterns", 0, snapshot());
  });
  return {
    start() { requestAnimationFrame(tick); },
    keydown, keyup,
    pointer(col, row, erase, audition) {
      pointer(col, row, erase, audition);
      announceState();
    },
    selectVoice(ch) { selectVoice(ch); announceState(); },
    toggleMute(ch) {
      if (s.solo || s.childLock) return;
      if (!((ch >= 1 && ch <= 8) || ch === 11 || ch === 13 || ch === 14)) return;
      s.mute[ch] = !s.mute[ch];
      liveMix();
      announceState();
    },
    unmute(ch) {
      if (s.childLock) return;
      if (!((ch >= 1 && ch <= 8) || ch === 11 || ch === 13 || ch === 14)) return;
      if (!s.mute[ch]) return;
      s.mute[ch] = false;
      liveMix();
      announceState();
    },
    soloHold(ch) {
      selectVoice(ch);
      if (!s.solo) enterSolo();
      else {
        for (let i = 0; i < s.mute.length; i++) s.mute[i] = i !== ch;
      }
      liveMix();
      announceState();
    },
    soloRelease() { exitSolo(); liveMix(); announceState(); },
    press(enc) { shortPress(enc); },
    knobDown(enc) {
      const grid = s.mode === "draw" || s.mode === "single";
      if (enc === 0 && s.mode === "single" && s.GLOB.y === 16) hold[0] = { at: performance.now(), turned: false, fired: false };
      if (enc === 1 && grid && s.GLOB.y !== 16) hold[1] = { at: performance.now(), turned: false, fired: false };
      if (enc === 2 && grid) hold[2] = { at: performance.now(), turned: false, fired: false };
      if (enc === 3 && (grid || s.mode === "filter")) hold[3] = { at: performance.now(), turned: false, fired: false, had: !!note[s.GLOB.x][s.GLOB.y].channel };
    },
    knobUp(enc, turned) {
      if (enc === 0) {
        const h = hold[0];
        hold[0] = null;
        if (h?.fired || turned) return;
        shortPress(0);
        return;
      }
      if (enc === 1) {
        const h = hold[1];
        hold[1] = null;
        if (s.solo) { exitSolo(); return; }
        if (turned || h?.fired) return;
        shortPress(1);
        return;
      }
      if (enc === 2 || enc === 3) {
        const h = hold[enc];
        hold[enc] = null;
        if (h?.fired || turned) return;
        shortPress(enc);
        return;
      }
      if (!turned) shortPress(enc);
    },
    touch(id, down) {
      if (id === 1) onTouchL(down);
      else if (id === 2) onMeta(down);
      else onTouch3(down);
    },
    rotate(enc, dir) { turn(enc, dir); },
    hover, cellTaken, velocity: enterVelocity,
    voice() { return s.GLOB.currentChannel; },
    muted(ch) { return !!s.mute[ch]; },
    gridMode() { return s.mode === "draw" || s.mode === "single"; },
    cursor() {
      return { on: s.mode === "draw" || s.mode === "single", x: localX(s.GLOB.x), y: s.GLOB.y, step: s.GLOB.x };
    },
    cellAtCursor,
    pageNotes,
    menuSummary,
    announceState,
    setDeviceFocus(on) { s.deviceFocus = !!on; },
    deviceFocus() { return !!s.deviceFocus; },
    setAssistKeyGate(on) { s.assistKeyGate = !!on; },
    onAnnounce(fn) { s.onAnnounce = fn; },
    playing() { return !!s.playing; },
    page() { return s.GLOB.edit; },
    togglePlay() { togglePlay(); announceState(); },
    pause() {
      if (s.playing) togglePlay();
    },
    clearPage() { clearPage(); announceState(); },
    randomPage() {
      if (s.mode !== "single") return;
      drawRandomPage();
      announceState();
    },
    enterShift() {
      if (s.mode !== "single") return;
      s.mode = "shift";
      announceState();
    },
    moveCursor(col, row) {
      if (s.mode !== "draw" && s.mode !== "single") return;
      hover(col, row);
      announceState();
    },
    paintAtCursor() {
      if (s.mode !== "draw" && s.mode !== "single") return;
      paintCell(s.GLOB.x, s.GLOB.y);
      announceState();
    },
    eraseAtCursor() {
      if (s.mode !== "draw" && s.mode !== "single") return;
      eraseCell(s.GLOB.x, s.GLOB.y);
      announceState();
    },
    mode() { return s.mode; },
    subIndex() { return s.subIndex; },
    soon() { return isSoon(s.mode, s.subIndex); },
    bpm() { return s.bpm; },
    exportPatternCells() {
      const cells = [];
      for (let x = 1; x <= STEPS; x++) {
        for (let y = 1; y <= ROWS; y++) {
          const n = note[x][y];
          if (n.channel) {
            cells.push({
              channel: n.channel,
              velocity: n.velocity,
              probability: n.probability,
              condition: n.condition,
              midiPitch: n.midiPitch <= 127 ? n.midiPitch : 255,
            });
          } else {
            cells.push({ channel: 0, velocity: 0, probability: 100, condition: 1, midiPitch: 255 });
          }
        }
      }
      return cells;
    },
    importPatternCells(cells, bpm) {
      if (s.mode === "boot") return;
      for (let x = 1; x <= STEPS; x++) for (let y = 1; y <= ROWS; y++) Object.assign(note[x][y], emptyNote());
      let i = 0;
      for (let x = 1; x <= STEPS; x++) {
        for (let y = 1; y <= ROWS; y++) {
          const c = cells[i++];
          if (!c || !c.channel) continue;
          Object.assign(note[x][y], {
            channel: c.channel,
            velocity: c.velocity || 100,
            probability: c.probability != null ? c.probability : 100,
            condition: c.condition != null ? c.condition : 1,
            midiPitch: c.midiPitch <= 127 ? c.midiPitch : 255,
          });
        }
      }
      if (Number.isFinite(bpm)) s.bpm = clamp(Math.round(bpm), 40, 240);
      s.GLOB.edit = 1;
      s.GLOB.page = 1;
      s.GLOB.x = 1;
      s.beat = 1;
      saveEeprom();
      queueAutosave();
      finishLoad();
    },
    cols() { return COLS; },
    loadGenre(type) {
      if (s.mode === "boot") return;
      writeGenre(type);
    },
    importMidi(cells, bpm) {
      if (s.mode === "boot") return;
      for (let x = 1; x <= STEPS; x++) for (let y = 1; y <= ROWS; y++) Object.assign(note[x][y], emptyNote());
      for (const cell of cells || []) {
        const n = note[cell.step]?.[cell.row];
        if (!n) continue;
        n.channel = cell.channel;
        n.velocity = cell.velocity;
        n.probability = cell.probability != null ? cell.probability : 100;
        n.condition = 1;
        n.midiPitch = cell.midiPitch <= 127 ? cell.midiPitch : 255;
      }
      if (Number.isFinite(bpm)) s.bpm = clamp(Math.round(bpm), 40, 240);
      s.GLOB.edit = 1;
      s.GLOB.page = 1;
      s.GLOB.x = 1;
      s.beat = 1;
      saveEeprom();
      queueAutosave();
      finishLoad();
    },
    setDevice(gen) {
      deviceChosen = true;
      const rotated = subPick.look[7] % 2 === 1;
      subPick.look[7] = gen === 1 ? (rotated ? 1 : 0) : (rotated ? 3 : 2);
      applyLedLayout();
      saveEeprom();
    },
    jump(name) {
      if (s.mode === "boot") return;
      if (name === "empty") {
        resetFull();
        s.okAt = performance.now();
        finishLoad();
        return;
      }
      if (name === "draw") { s.GLOB.singleMode = false; s.mode = "draw"; return; }
      if (name === "single") {
        const ch = s.GLOB.currentChannel;
        if (!((ch >= 1 && ch <= 8) || ch === 11 || ch === 13 || ch === 14)) return;
        s.GLOB.singleMode = true;
        s.mode = "single";
        return;
      }
      if (name === "filter") { enterFilter(); return; }
      if (name === "menu") { s.menuIndex = 0; s.mode = "menu"; return; }
      if (name === "wav") {
        if (s.GLOB.currentChannel < 1 || s.GLOB.currentChannel > 8) return;
        refreshBrowse();
        s.mode = "wav";
        previewWav();
        return;
      }
      if (name === "velocity") { enterVelocity(); return; }
      if (name === "bpm") { s.mode = "bpm"; return; }
      if (name === "song") { s.mode = "song"; return; }
      if (name === "play") togglePlay();
    },
  };
}
