import { canReplaceShadowed } from "./paint.js";
import { INSTRUMENT_NAMES } from "./instruments.js";
import { syncHasEffect } from "./performance.js";
import { COL, COL_BASE, COLS, ROWS, PAGES, SCHEMES, filterPagesFor } from "./const.js";
import { blend, hsv, nscale, indicatorColor } from "./matrix.js";
import { ALPHABET, glyphWidth, textPixelWidth } from "./font.js";
import { showIcon } from "./icons.js";
import { GENRES } from "./genres.js";

const UI_RED = [120, 0, 0];
const UI_GREEN = [0, 120, 0];
const UI_BLUE = [0, 0, 120];
const UI_WHITE = [120, 120, 120];
const UI_YELLOW = [120, 120, 0];
const UI_ORANGE = [120, 60, 0];
const UI_BRIGHT_WHITE = [150, 150, 150];
const UI_BRIGHT_GREEN = [0, 150, 0];
const UI_DIM_RED = [20, 0, 0];
const UI_DIM_GREEN = [0, 20, 0];
const FILTER_WIDE = [[5, 6], [12, 13], [20, 21], [27, 28]];
const FILTER_NARROW = [[2, 3], [6, 7], [10, 11], [14, 15]];
function filterCols() { return COLS <= 16 ? FILTER_NARROW : FILTER_WIDE; }
const FILTER_NAMES = ["HCUT", "LCUT", "RVRB", "BITC"];
const FILTER_COLORS = [[255, 0, 0], [255, 64, 0], [255, 128, 0], [255, 191, 0]];
const LOGO = [
  0x0000, 0x0000, 0x07f8, 0x1ff8, 0x1980, 0x1980, 0x1980, 0x19f0,
  0x19f0, 0x1980, 0x1980, 0x1980, 0x1ff8, 0x07f8, 0x0000, 0x0000,
];

export const MENU_PAGES = [
  { name: "PATT", label: "PATT", icon: "folder", col: 1 },
  { name: "KIT", label: "KIT", icon: "pack", col: 2 },
  { name: "SMP", label: "SMP", icon: "sample", col: 3 },
  { name: "BPM", label: "BPM", icon: "clock", col: 4 },
  { name: "VOL", label: "VOL", icon: "volume", col: 5 },
  { name: "SETT", label: "SETT", icon: "settings", col: 6 },
  { name: "REC", label: "REC", icon: "record", col: 7 },
  { name: "MIDI", label: "MIDI", icon: "midi", col: 8 },
  { name: "MODE", label: "MODE", icon: "song", col: 13 },
  { name: "ETC", label: "ETC", icon: "pattern", col: 14 },
];

// Navigation is separate from the persisted legacy setting slots.
export const SUBS = {
  look: ["VIEW", "CTRL", "DRAW", "MUTE"],
  recs: ["INPT", "MIC", "HPF", "L-IN", "TRIG", "CLR"],
  midi: ["CH", "TRAN", "SEND", "RCVE", "FOLD", "SYNC", "PQ-I", "PQ-O", "TRIG", "OVWR"],
  vol: ["MAIN", "GAIN", "LOUT", "PREV", "SPKR"],
  mode: ["FLOW", "PMOD", "LOOP", "VMOD", "CHLD", "IMG", "PONG", "SONG", "TCHY", "AUTO", "DEMO"],
  etc: ["INFO", "SCON", "BATT", "ADV", "RSET", "SWAP"],
  adv: ["CRSR", "LEDS", "RAM", "LGHT", "COLR", "EYES", "FIRE", "2-CH", "USBP", "HFC"],
  pat: ["TECH", "HIPH", "DNB", "HOUS", "AMBT"],
};
const SUB_SLOTS = {
  recs: [["recs", 0], ["recs", 1], ["recs", 5], ["recs", 2], ["recs", 3], ["recs", 4]],
  midi: [["midi", 0], ["midi", 1], ["midi", 2], ["midi", 3], ["midi", 4], ["midi", 5], ["midi", 8], ["midi", 6], ["midi", 7], ["midi", 9]],
  look: [["look", 2], ["look", 6], ["look", 10], ["look", 11]],
  vol: [["vol", 0], ["vol", 1], ["vol", 2], ["vol", 3], ["vol", 5]],
  mode: [["look", 0], ["look", 3], ["look", 4], ["look", 5], ["etc", 8], ["etc", 10], ["look", 8], ["song", 0], ["touchy", 0], ["etc", 3]],
  etc: [["etc", 0], ["etc", 2], ["etc", 6], ["adv", 0], ["etc", 11]],
  adv: [["look", 9], ["look", 7], ["etc", 1], ["etc", 4], ["etc", 5], ["etc", 7], ["etc", 9], ["vol", 4], ["vol", 7], ["vol", 6]],
};
export function resolveSubPage(mode, index) {
  const slot = SUB_SLOTS[mode]?.[index];
  return slot ? { mode: slot[0], index: slot[1] } : { mode, index };
}

export const CIN_PPQN_TABLE = [1, 2, 4, 8, 12, 16, 24, 32];
export const PPQN_RATE_LABELS = ["OFF", "1", "2", "4", "8", "12", "16", "24", "32"];
export const SIDE_VOICE_CHOICES = [0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14];
export const ETC_FIRE_INDEX = 9;
export const ETC_IMG_INDEX = 10;
export const ETC_RSET_INDEX = 11;
export const ETC_EYES_INDEX = 7;
export const ETC_CHLD_INDEX = 8;

const SUB_PARENT = { look: 6, recs: 7, midi: 8, vol: 5, mode: 13, etc: 14, adv: 14 };

function scaleTo(rgb, target) {
  const max = Math.max(rgb[0], rgb[1], rgb[2]);
  if (!max) return rgb.slice();
  const scale = Math.min(255, Math.round((target * 255) / max));
  return nscale(rgb, scale);
}

function pal(s) { return SCHEMES[s.scheme || 0] || SCHEMES[0]; }

function menuText(colIndex) {
  return scaleTo(COL[colIndex] || [120, 120, 120], 120);
}

function pageDots(matrix, count, index, on, off) {
  const start = Math.max(1, COLS - count + 1);
  for (let i = 0; i < count; i++) matrix.light(start + i, ROWS, i === index ? on : off);
}

function sx(x16) {
  return 1 + (x16 - 1) * (COLS - 1) / 15 | 0;
}

export function drawBase(matrix, s) {
  if (s.imageMode) {
    for (let y = 1; y < ROWS; y++) {
      for (let x = 1; x <= COLS; x++) matrix.light(x, y, [0, 0, 0]);
    }
  } else if (!s.GLOB.singleMode) {
    let colors = 0;
    for (let y = 1; y < ROWS; y++) {
      const muted = s.mute[y - 1];
      const row = s.drawBaseColorMode && !muted ? pal(s).base[colors] : [0, 0, 0];
      for (let x = 1; x <= COLS; x++) matrix.light(x, y, row);
      colors++;
    }
  } else {
    const ch = s.GLOB.currentChannel;
    let base = (pal(s).base[ch] || [0, 0, 0]).slice();
    if (s.mute[ch]) base = nscale(base, 128);
    const hi = nscale(blend(blend(base, [255, 255, 255], 5), [255, 255, 255], 5), 190);
    for (let y = 1; y < ROWS; y++) {
      const color = y === ch + 1 ? hi : base;
      for (let x = 1; x <= COLS; x++) matrix.light(x, y, color);
    }
  }
  const helper = s.monitor === 0 ? [10, 10, 10] : s.monitor === 1 ? [30, 30, 0] : [0, 30, 0];
  for (let x = 1; x <= COLS - 3; x += 4) matrix.light(x, 1, helper);
  drawStatus(matrix, s);
  drawPages(matrix, s);
  if (s.GLOB.singleMode && s.mute[s.GLOB.currentChannel]) {
    const on = ((performance.now() / 300) & 1) === 0;
    const bar = on ? [255, 255, 0] : [0, 0, 0];
    const x0 = COLS > 16 ? 16 : 8;
    for (let x = x0; x <= COLS; x++) matrix.light(x, 16, bar);
  }
}

function drawStatus(matrix, s) {
  const y = s.GLOB.y;
  if (s.copyArmed && (s.mode === "draw" || s.mode === "single")) {
    matrix.drawIndicator("M", "X", 1);
    matrix.drawIndicator("M", "G", 4);
    matrix.setRing(2, [0, 0, 0]);
    matrix.setRing(3, [0, 0, 0]);
    return;
  }
  if (s.mode === "draw" && y === 16) {
    matrix.drawIndicator("M", "R", 1);
    matrix.drawIndicator("M", "Y", 4);
  }
  if (s.GLOB.singleMode && y === 16) {
    matrix.drawIndicator("M", "R", 1);
    matrix.drawIndicator("M", "W", 2);
    matrix.drawIndicator("M", "Y", 4);
  }
  if (s.fnHeld && (s.mode === "draw" || s.mode === "single")) {
    matrix.drawIndicator("M", "M", 2);
  }
}

function voiceGlobalPage(s) {
  return Math.floor(((s.beat || 1) - 1) / COLS) + 1;
}

export function drawPages(matrix, s) {
  if (s.voiceMode && s.pmode !== 2) {
    if (s.refreshVoiceLens) s.refreshVoiceLens();
    const ch = s.GLOB.currentChannel || 0;
    const mask = s.voiceMask?.[ch] || 0;
    const len = Math.max(1, s.voiceLen?.[ch] || 1);
    const playP = s.playing && s.voicePageFor ? s.voicePageFor(ch, voiceGlobalPage(s)) : 0;
    for (let x = 1; x <= COLS; x++) matrix.light(x, ROWS, [0, 0, 0]);
    for (let p = 1; p <= PAGES; p++) {
      const has = (mask & (1 << (p - 1))) !== 0;
      let color;
      if (playP === p && s.GLOB.edit === p) color = [255, 255, 50];
      else if (playP === p) color = [0, 255, 0];
      else if (s.GLOB.edit === p) color = [255, 255, 0];
      else color = has ? [220, 220, 220] : [1, 0, 0];
      matrix.light(p, ROWS, color);
    }
    return;
  }
  const loop = s.loopLength;
  for (let x = 1; x <= COLS; x++) matrix.light(x, ROWS, loop > 0 ? [10, 0, 0] : [0, 0, 0]);
  for (let p = 1; p <= PAGES; p++) {
    let color;
    if (loop > 0 && p > loop) color = [10, 0, 0];
    else if (s.GLOB.page === p && s.GLOB.edit === p) color = [255, 255, 50];
    else if (s.GLOB.page === p) color = [0, 255, 0];
    else if (s.GLOB.edit === p) color = [255, 255, 0];
    else color = s.hasPageNotes(p) ? [0, 0, 35] : [1, 0, 0];
    matrix.light(p, ROWS, color);
  }
}

const COND_BLINK = {
  2: [0, 0, 255], 4: [200, 0, 255], 8: [255, 100, 0], 16: [0, 200, 200],
  17: [0, 150, 255], 18: [150, 0, 255], 19: [255, 150, 0], 20: [0, 255, 200], 22: [0, 255, 120],
};

export function drawTriggers(matrix, s, now) {
  const blink = ((now / 300) | 0) & 1;
  const easy = s.simpleNotes && !s.GLOB.singleMode;
  const voiceGrid = s.voiceMode && s.playing && s.pmode !== 2;
  const col = pal(s).col;
  const baseCol = pal(s).base;
  for (let ix = 1; ix <= COLS; ix++) {
    for (let iy = 1; iy <= ROWS; iy++) {
      let cell, step;
      if (voiceGrid && s.voiceOwnedStep) {
        const src = s.voiceOwnedStep(iy, ix, voiceGlobalPage(s));
        if (!src) continue;
        step = src;
        cell = s.note[step][iy];
      } else {
        step = (s.GLOB.edit - 1) * COLS + ix;
        cell = s.note[step][iy];
      }
      const ch = cell.channel;
      if (!ch) continue;
      const shadowed = canReplaceShadowed(s.note,step,iy,0);
      const drawNoteColor = color => matrix.light(ix,iy,shadowed ? color.map(c=>Math.floor(c/5)) : color);

      if (easy && !voiceGrid) {
        const voiceY = ch + 1;
        drawNoteColor([0, 0, 0]);
        if (voiceY >= 1 && voiceY <= ROWS) {
          matrix.light(ix, voiceY, s.mute[ch] ? baseCol[ch] : col[ch]);
        }
        continue;
      }
      if (s.mute[ch]) {
        drawNoteColor(s.GLOB.singleMode ? [20, 20, 20] : (baseCol[ch] || [0, 0, 0]));
        continue;
      }
      if (s.GLOB.singleMode && ch !== s.GLOB.currentChannel) {
        drawNoteColor(baseCol[ch] || [0, 0, 0]);
        continue;
      }
      if (s.fullMute) {
        drawNoteColor(baseCol[ch] || [0, 0, 0]);
        continue;
      }
      if (s.GLOB.singleMode && ch === s.GLOB.currentChannel) {
        let color = (col[ch] || UI_BRIGHT_WHITE).slice();
        const cond = cell.condition || 1;
        if (COND_BLINK[cond] && blink === 0) color = COND_BLINK[cond];
        else if (cell.probability < 100 && blink === 0) {
          const red = Math.round(255 - (cell.probability / 100) * 175);
          color = [red, 0, 0];
        }
        drawNoteColor(color);
      } else if (s.GLOB.singleMode) {
        drawNoteColor(baseCol[ch] || [0, 0, 0]);
      } else {
        drawNoteColor(col[ch] || [255, 255, 255]);
      }
    }
  }
}

const FIRE_POOL = 25;
const FIRE_FLIES = Array.from({ length: FIRE_POOL }, () => ({ x: 0, y: 0, drift: 1, life: 0, ch: 0 }));
let fireSeenCol = 0;
let fireSeenPage = 0;
let fireTickAt = 0;
let firePoolActive = false;

function fireClear() {
  if (!firePoolActive) return;
  for (const f of FIRE_FLIES) f.life = 0;
  firePoolActive = false;
}

function fireLiving() {
  let n = 0;
  for (const f of FIRE_FLIES) if (f.life) n++;
  return n;
}

function fireOldestSlot() {
  let slot = 0;
  let best = 255;
  for (let i = 0; i < FIRE_POOL; i++) {
    if (!FIRE_FLIES[i].life) return i;
    if (FIRE_FLIES[i].life < best) { best = FIRE_FLIES[i].life; slot = i; }
  }
  return slot;
}

function fireTrim(cap) {
  for (;;) {
    let living = 0;
    let oldest = -1;
    let best = 255;
    for (let i = 0; i < FIRE_POOL; i++) {
      if (!FIRE_FLIES[i].life) continue;
      living++;
      if (FIRE_FLIES[i].life < best) { best = FIRE_FLIES[i].life; oldest = i; }
    }
    if (living <= cap || oldest < 0) return;
    FIRE_FLIES[oldest].life = 0;
  }
}

function firePixel(matrix, x, y, playX, color) {
  if (x < 1 || x > COLS || y < 1 || y >= ROWS || x === playX) return;
  matrix.light(x, y, color);
}

function fireSpawnOne(x, y, ch, n) {
  if (ch < 1 || ch > 14) return;
  const drift = n & 1 ? 1 : -1;
  let nx = x + 1;
  let ny = y - drift;
  if (nx > COLS) nx = x;
  if (ny < 1) ny = 1;
  if (ny >= ROWS) ny = ROWS - 1;
  if (nx < 1) nx = 1;
  if (ny < 1 || ny >= ROWS) return;
  const f = FIRE_FLIES[fireOldestSlot()];
  f.x = nx;
  f.y = ny;
  f.drift = drift;
  f.life = 10;
  f.ch = ch;
  firePoolActive = true;
}

function fireSpawnColumn(hits) {
  const cap = Math.max(1, Math.min(FIRE_POOL, hits.level | 0));
  if (!hits.n) return;
  const room = cap - fireLiving();
  let born = room > 0 ? room : Math.floor((cap + 3) / 4);
  if (born < 1) born = 1;
  if (born > cap) born = cap;
  for (let n = 0; n < born; n++) fireSpawnOne(hits.x[n % hits.n], hits.y[n % hits.n], hits.ch[n % hits.n], n);
  fireTrim(cap);
}

function fireDraw(matrix, s, playX) {
  if (!firePoolActive) return;
  if (!s.fireVoice || !s.fireLevel) {
    fireClear();
    return;
  }
  const now = performance.now();
  const step = now - fireTickAt >= 32;
  if (step) fireTickAt = now;
  const col = pal(s).col;
  const gravPeriod = s.fireGravity ? 9 - (s.fireGravity | 0) : 0;
  const size = s.fireSize || 1;
  const colorness = s.fireColor == null ? 8 : s.fireColor;
  let any = false;
  for (const f of FIRE_FLIES) {
    if (!f.life) continue;
    if (step) {
      f.life--;
      if ((f.life & 1) === 0) {
        f.x += 1;
        if ((f.life & 2) === 0) f.y -= f.drift;
      }
      if (gravPeriod > 0 && f.life % gravPeriod === 0) f.x -= 1;
    }
    if (!f.life || f.y < 1 || f.y >= ROWS || f.x < 1 || f.x > COLS || f.ch < 1 || f.ch > 14) {
      f.life = 0;
      continue;
    }
    any = true;
    if (f.x === playX) continue;
    const voice = col[f.ch] || [255, 160, 40];
    const base = colorness <= 0 ? [230, 230, 230] : colorness >= 8 ? voice : blend([230, 230, 230], voice, colorness * 32);
    const scaled = base.map((v) => Math.round(v * f.life * 25 / 255));
    const dim = (amt) => scaled.map((v) => Math.round(v * amt));
    if (size >= 2) {
      firePixel(matrix, f.x, f.y - 1, playX, dim(0.43));
      firePixel(matrix, f.x, f.y + 1, playX, dim(0.43));
    }
    if (size >= 3) firePixel(matrix, f.x + 1, f.y, playX, dim(0.31));
    if (size >= 4) {
      firePixel(matrix, f.x + 1, f.y - 1, playX, dim(0.2));
      firePixel(matrix, f.x + 1, f.y + 1, playX, dim(0.2));
    }
    if (f.life > 5 && f.x > 1) firePixel(matrix, f.x - 1, f.y, playX, dim(0.25));
    firePixel(matrix, f.x, f.y, playX, scaled);
  }
  if (!any) firePoolActive = false;
}

export function drawTimer(matrix, s) {
  const page = Math.floor((s.beat - 1) / COLS) + 1;
  const voiceGrid = s.voiceMode && s.playing && s.pmode !== 2;
  if (!voiceGrid && page !== s.GLOB.edit) {
    fireClear();
    return;
  }
  const x = ((s.beat - 1) % COLS) + 1;
  const fireOn = s.fireVoice && s.fireLevel;
  if (!fireOn) fireClear();
  let fresh = false;
  const hits = { x: [], y: [], ch: [], n: 0, level: s.fireLevel | 0 };
  if (fireOn) {
    fresh = x !== fireSeenCol || page !== fireSeenPage;
    if (fresh) {
      fireSeenCol = x;
      fireSeenPage = page;
    }
  }
  for (let y = 1; y < ROWS; y++) {
    let ch = 0;
    let step;
    if (voiceGrid && s.voiceOwnedStep) {
      step = s.voiceOwnedStep(y, x, voiceGlobalPage(s));
      ch = step ? s.note[step][y].channel : 0;
    } else {
      step = (s.GLOB.edit - 1) * COLS + x;
      ch = s.note[step][y].channel;
    }
    if (ch > 0 && canReplaceShadowed(s.note,step,y,0)) continue;
    matrix.light(x, y, [28, 0, 0]);
    if (ch > 0 && !s.mute[ch]) {
      const lit = !s.GLOB.singleMode || s.GLOB.currentChannel === ch;
      if (lit) {
        matrix.light(x, y, [255, 255, 255]);
        if (fireOn && fresh && hits.n < 15 && (s.fireVoice === 15 || ch === s.fireVoice)) {
          hits.x[hits.n] = x;
          hits.y[hits.n] = y;
          hits.ch[hits.n] = ch;
          hits.n++;
        }
      }
    } else if (ch > 0 && !s.GLOB.singleMode) matrix.light(x, y, [0, 0, 0]);
  }
  if (fireOn) {
    if (fresh && hits.n) fireSpawnColumn(hits);
    fireDraw(matrix, s, x);
  }
}

export function drawCursor(matrix, s, now = performance.now()) {
  s.pulse += s.pulseDir * 8;
  if (s.pulse > 230) s.pulseDir = -1;
  if (s.pulse < 1) s.pulseDir = 1;
  const x = ((s.GLOB.x - 1) % COLS) + 1;
  const y = s.GLOB.y;
  const ch = s.note[s.GLOB.x][y].channel;
  const col = pal(s).col;
  if (s.cursorType === 2) {
    const bpm = s.bpm >= 40 ? s.bpm : 120;
    const stepMs = Math.max(10, (60000 / bpm * 2) / 16);
    const radius = (Math.floor(now / stepMs) % 16) + 1;
    const r2 = radius * radius;
    for (let dx = -radius; dx <= radius; dx++) for (let dy = -radius; dy <= radius; dy++) {
      const d = dx * dx + dy * dy;
      if (d < r2 || d > r2 + 2) continue;
      matrix.light(x + dx, y + dy, [50, 50, 50]);
    }
  }
  if (s.GLOB.singleMode && ch && ch !== s.GLOB.currentChannel) {
    matrix.light(x, y, pal(s).base[ch] || [0, 0, 0]);
  } else if (ch) matrix.light(x, y, nscale(col[ch], s.pulse));
  else matrix.light(x, y, hsv(s.pulse, 255, 255));
  matrix.setCursor(x, y);
}

function drawChannelNr(matrix, s) {
  const ch = s.chNr;
  const color = pal(s).col[ch] || UI_WHITE;
  const text = String(ch);
  const width = textPixelWidth(text);
  const endX = Math.min(COLS, 2 + width);
  const top = ch >= 1 && ch <= 8;
  const textY = top ? 12 : 3;
  const borderY = top ? 10 : 9;
  const bgY0 = top ? 11 : 2;
  const bgY1 = top ? 16 : 8;
  for (let x = 1; x <= endX; x++) for (let y = bgY0; y <= bgY1; y++) matrix.light(x, y, [0, 0, 0]);
  for (let x = 1; x <= endX; x++) matrix.light(x, borderY, color);
  matrix.drawText(text, 2, textY, color);
}

function drawPageNr(matrix, s) {
  const page = s.pageNr || 1;
  const color = UI_WHITE;
  const text = String(page);
  const width = textPixelWidth(text);
  let textStartX = COLS + 1 - width;
  if (textStartX < 1) textStartX = 1;
  const boxStartX = Math.max(1, textStartX - 1);
  const boxEndX = Math.min(COLS, textStartX + width);
  for (let x = boxStartX; x <= boxEndX; x++) for (let y = 11; y <= 15; y++) matrix.light(x, y, [0, 0, 0]);
  matrix.drawText(text, textStartX, 11, color);
}

export function drawVelocity(matrix, s) {
  matrix.setRing(1, [255, 68, 0]);
  matrix.setRing(2, [0, 255, 136]);
  matrix.setRing(3, [136, 136, 136]);
  matrix.setRing(4, [0, 68, 255]);
  const { v, p, c, vol } = s.vel;
  const velX1 = sx(2);
  const velX2 = sx(3);
  for (let x = velX1; x <= velX2; x++) {
    for (let y = 1; y < v + 1; y++) matrix.light(x, y, [Math.min(255, y * y), Math.max(0, 20 - y), 0]);
  }
  const pL = sx(5); const pR = sx(8); const iL = sx(6); const iR = sx(7);
  const frame = [30, 30, 30];
  for (const x of [pL, iL, iR, pR]) { matrix.light(x, 1, frame); matrix.light(x, 16, frame); }
  for (let y = 2; y <= 15; y++) { matrix.light(pL, y, frame); matrix.light(pR, y, frame); }
  const bars = [
    null,
    { y: 2, c: [100, 0, 0] }, { y: 5, c: [150, 50, 0] }, { y: 8, c: [150, 150, 0] },
    { y: 11, c: [0, 150, 150] }, { y: 14, c: [0, 200, 0] },
  ];
  const bar = bars[p];
  if (bar) for (let x = iL; x <= iR; x++) { matrix.light(x, bar.y, bar.c); matrix.light(x, bar.y + 1, bar.c); }
  const fracs = [
    ["1", "1", [0, 200, 0]], ["1", "2", [0, 0, 255]], ["1", "4", [200, 0, 255]],
    ["1", "8", [255, 100, 0]], ["1", "X", [0, 200, 200]], ["2", "1", [0, 150, 255]],
    ["4", "1", [150, 0, 255]], ["8", "1", [255, 150, 0]], ["X", "1", [0, 255, 200]],
    ["F", "F", [255, 255, 0]], ["G", "L", [0, 255, 120]],
  ];
  const [num, den, tc] = fracs[c - 1] || fracs[0];
  const ix1 = COLS <= 16 ? 9 : 19;
  const ix2 = ix1 + 1;
  const ix3 = ix1 + 2;
  matrix.drawText(num, ix1, 11, tc);
  matrix.light(ix3 + 1, 10, frame);
  matrix.light(ix3, 9, frame);
  matrix.light(ix2, 8, frame);
  matrix.light(ix1, 7, frame);
  matrix.drawText(den, ix1 + 1, 2, tc);
  for (let x = COLS <= 16 ? 14 : 27; x <= (COLS <= 16 ? 15 : 28); x++) {
    for (let y = 1; y < vol + 1; y++) matrix.light(x, y, [0, Math.max(0, 20 - y), Math.min(255, y * y)]);
  }
}

const FILTER_PAGES_COLORS = [
  [[255, 0, 0], [255, 64, 0], [255, 128, 0], [255, 191, 0]],
  [[0, 80, 255], [0, 140, 255], [0, 200, 220], [0, 220, 180]],
  [[80, 255, 0], [140, 255, 0], [180, 220, 0], [80, 180, 40]],
  [[255, 0, 180], [255, 80, 160], [180, 80, 255], [80, 180, 255]],
];
const WAVE_NAMES = ["SIN", "SQR", "SAW", "TRI"];
const INST_NAMES = INSTRUMENT_NAMES;

export function drawFilter(matrix, s, now) {
  const f = s.filt[s.GLOB.currentChannel];
  const pages = filterPagesFor(s.GLOB.currentChannel);
  const page = pages[s.filterPage || 0] || pages[0];
  const pageColors = FILTER_PAGES_COLORS[s.filterPage || 0] || FILTER_PAGES_COLORS[0];
  pageColors.forEach((c, i) => matrix.setRing(i + 1, c));
  for (let p = 0; p < pages.length; p++) matrix.light(COLS - pages.length + 1 + p, ROWS, p === (s.filterPage || 0) ? [255, 255, 255] : [30, 30, 30]);
  const focus = s.filterTouch;
  const active = now - s.filterTouchAt < 1000 && focus >= 0 && focus < 4 && page[focus];
  for (let i = 0; i < 4; i++) {
    const spec = page[i];
    if (!spec) continue;
    if (active && i !== focus) continue;
    const val = f[spec.key] ?? 0;
    const max = spec.max ?? 32;
    const display = Math.max(1, Math.min(10, Math.round((val / max) * 9) + 1));
    const color = blend([0, 0, 0], pageColors[i], Math.round((val / max) * 255));
    const [x0, x1] = filterCols()[i];
    const isBipolarMid =
      (spec.key === "oct" && val === Math.floor(max / 2)) ||
      ((spec.key === "detune" || spec.key === "cent" || spec.key === "semi" || spec.key === "arp" || spec.key === "voiceGain") && val === 16);
    for (let y = 1; y <= 10; y++) {
      let c = [0, 0, 0];
      if (y === display) c = i === f.fast ? [255, 255, 0] : [255, 255, 255];
      else if (isBipolarMid && y === 1) c = [0, 255, 0];
      else if (y < display) c = color;
      matrix.light(x0, y, c);
      matrix.light(x1, y, c);
    }
    if (!active) matrix.drawText(spec.name[0], x0, 12, pageColors[i]);
  }
  if (active) {
    const spec = page[focus];
    const val = f[spec.key] ?? 0;
    const [x0, x1] = filterCols()[focus];
    const width = textPixelWidth(spec.name);
    let tx = Math.floor((COLS - width + 1) / 2);
    if (tx < 1) tx = 1;
    matrix.drawText(spec.name, tx, 12, pageColors[focus]);
    const bipolarKeys = new Set(["detune", "oct", "cent", "semi", "arp", "voiceGain"]);
    let label;
    if (spec.key === "voiceGain") label = (val > 16 ? "+" : "") + String(val - 16);
    else if (spec.key === "stretch") label = val === 11 ? "OFF" : String(val - 11);
    else if (spec.key === "wave") label = WAVE_NAMES[val] || String(val);
    else if (spec.key === "inst") label = INST_NAMES[val] || String(val);
    else if (spec.key === "detune" && (spec.max ?? 32) === 32) {
      label = String(Math.round((val / 32) * 200 - 100)); // cents
    } else if (spec.key === "oct" && (spec.max ?? 48) === 48) {
      label = String(val - 24); // whole semis ±2 oct
    } else if (bipolarKeys.has(spec.key) && (spec.max ?? 32) === 32) label = String(val - 16);
    else label = String(val);
    const t = Math.round((val / (spec.max ?? 32)) * 255);
    let valueColor = (spec.key === "res" || spec.key === "b" || spec.key === "voiceGain")
      ? blend([0, 255, 0], [255, 0, 0], t)
      : blend([255, 0, 0], [0, 255, 0], t);
    if (spec.key === "oct" && (spec.max ?? 48) === 48) {
      const semis = val - 24;
      if (semis === 0 || semis === 12 || semis === -12 || semis === 24 || semis === -24)
        valueColor = [255, 255, 255];
    } else if (bipolarKeys.has(spec.key) && val === 16) {
      valueColor = [255, 255, 255];
    }
    matrix.drawText(label, focus < 2 ? x1 + 4 : Math.max(1, x0 - 10), 5, valueColor);
  }
}

export function drawFilterCheck(matrix, value, color) {
  const active = Math.round((value / 32) * COLS);
  for (let x = 1; x <= COLS; x++) {
    if (x <= active) {
      const t = (x - 1) / Math.max(1, active - 1);
      matrix.light(x, ROWS, [Math.round(255 * (1 - t) + color[0] * t), Math.round(255 * (1 - t) + color[1] * t), Math.round(255 * (1 - t) + color[2] * t)]);
    } else matrix.light(x, ROWS, [0, 0, 0]);
  }
  matrix.drawNumber(value, color, 8);
}

export function drawMenu(matrix, s) {
  const page = MENU_PAGES[s.menuIndex];
  const tc = menuText(page.col);
  matrix.drawIndicator("L", "G", 4);
  pageDots(matrix, MENU_PAGES.length, s.menuIndex, UI_RED, UI_BLUE);
  showIcon(matrix, page.icon, 2, 1, scaleTo(tc, 20));
  const label = page.name === "SMP" && [11,13,14].includes(s.GLOB.currentChannel) ? "PSET" : page.name === "SMP" && (s.GLOB.currentChannel < 1 || s.GLOB.currentChannel > 8) ? "(-)" : page.label;
  matrix.drawText(label, 2, 3, tc);
}

export function isSoon(mode, index) {
  const slot = resolveSubPage(mode, index);
  return !!SOON[slot.mode]?.has(slot.index);
}

const SOON = {
  look: new Set([7]),
  midi: new Set(),
  vol: new Set([2]),
  etc: new Set([1, 3, 4, 6]), // RAM AUTO LGHT BATT stay stubbed; 2 is SCON
};

function drawLength(matrix, s, y, withNumber) {
  const n = Math.max(1, Math.min(s.genreLength || 1, PAGES));
  for (let x = 1; x <= Math.min(COLS, 16); x++) matrix.light(x, y, x <= n ? [255, 0, 255] : [0, 0, 0]);
  if (withNumber) matrix.drawNumber(n, [255, 0, 255], 3);
}

export function drawNew(matrix, s) {
  const genre = GENRES[s.genre] || GENRES[0];
  matrix.drawText("NEW", 6, 12, [0, 255, 255]);
  matrix.drawText(genre.name, 2, 3, genre.color);
  matrix.drawLargeCustom(genre.color, 3);
  matrix.drawIndicator("M", "G", 1);
  if (s.genre) {
    drawLength(matrix, s, 10, false);
    matrix.drawIndicator("L", "M", 2);
  }
  matrix.drawIndicator("L", "Y", 3);
  matrix.drawIndicator("L", "X", 4);
}

// Menu values and their indicators share a 120 peak level; grid rendering is unchanged.
function menuValueColor(rgb) {
  const peak = Math.max(...rgb);
  return peak ? rgb.map(c => Math.floor(c * 120 / peak)) : rgb.slice();
}
function menuMatrix(matrix) {
  return new Proxy(matrix, { get(target, key) {
    if (key === "drawText") return (text, x, y, color) => target.drawText(text, x, y, y === 3 ? menuValueColor(color) : color);
    if (key === "setRing") return (n, color) => target.setRing?.(n, menuValueColor(color), true);
    if (key === "drawLargeCustom") return (color, n) => {
      const value = menuValueColor(color);
      target.drawLargeCustom(value, n); target.setRing?.(n, value, true);
    };
    if (key === "drawIndicator") return (size, code, n) => {
      const color = code === "O" ? UI_ORANGE : menuValueColor(indicatorColor(code));
      target.drawIndicator(size, code, n);
      if (size === "L") target.drawLargeCustom(color, n);
      target.setRing?.(n, color, true);
    };
    return target[key];
  } });
}

export function drawSubmenu(matrix, s, now = performance.now()) {
  matrix = menuMatrix(matrix);
  const pages = SUBS[s.mode];
  const name = pages[s.subIndex];
  const visibleMode = s.mode, visibleIndex = s.subIndex;
  const parent = menuText(SUB_PARENT[s.mode]);
  const dim = scaleTo(parent, 20);
  matrix.drawLargeCustom(parent, 4);
  pageDots(matrix, pages.length, s.subIndex, parent, dim);
  if (s.mode === "pat") {
    const genre = GENRES[(s.subIndex || 0) + 1];
    matrix.drawText(pages[s.subIndex], 2, 10, genre.color);
    drawLength(matrix, s, 8, true);
    matrix.drawLargeCustom(genre.color, 3);
    matrix.drawIndicator("L", "V", 4);
    return;
  }
  if (s.mode === "etc" && name === "SWAP") {
    matrix.drawText(s.swapHeld?.some(Boolean) ? "OCT" : s.swapAt && now-s.swapAt<600 ? "OK" : "SWAP", 2, 10, parent);
    matrix.drawText(s.swapHeld?.[0] ? `${s.swapOctaves[0]>=0?"+":""}${s.swapOctaves[0]}` : String(s.swapFrom), 1, 3, COL[s.swapFrom]);
    matrix.drawText(s.swapHeld?.[1] ? `${s.swapOctaves[1]>=0?"+":""}${s.swapOctaves[1]}` : String(s.swapTo), COLS >= 24 ? 24 : 10, 3, COL[s.swapTo]);
    if (COLS >= 24) matrix.drawText("<>",12,3,UI_WHITE);
    else { matrix.light(8,4,UI_WHITE); matrix.light(9,4,UI_WHITE); }
    matrix.drawLargeCustom(COL[s.swapFrom],2);
    matrix.drawLargeCustom(COL[s.swapTo],3);
    matrix.drawIndicator("L",s.swapFrom===s.swapTo?"R":"G",4);
    return;
  }
  const { mode: pageMode, index: pageIndex } = resolveSubPage(s.mode, s.subIndex);
  if (visibleMode === "mode" && name === "DEMO") {
    matrix.drawText("DEMO", 2, 10, parent);
    matrix.drawText("GO", 2, 3, UI_GREEN);
    return;
  }
  if (visibleMode === "etc" && visibleIndex === 1) {
    matrix.drawText("SCON", 2, 10, parent);
    matrix.drawText("WAIT", 2, 3, UI_YELLOW);
    return;
  }
  if (pageMode === "etc" && pageIndex === 0) {
    drawInfo(matrix, s, now, parent);
    return;
  }
  s.infoAt = 0;
  if ((visibleMode === "etc" && visibleIndex === 3) || (visibleMode === "mode" && visibleIndex === 7)) {
    matrix.drawText(name, 2, 10, parent);
    return;
  }
  if (pageMode === "vol" && pageIndex === 3) {
    const code=["G","R","Y"][s.prevMode] || "G";
    const color=[UI_GREEN,UI_RED,UI_YELLOW][s.prevMode] || UI_GREEN;
    const orange=[120,60,0];
    const showMode=now<(s.prevModeLabelUntil || 0);
    matrix.drawText("PREV",2,10,parent);
    matrix.drawText(showMode ? ["ON","PRSS","SYNC"][s.prevMode] : String(s.prevVol),2,3,showMode ? color : orange);
    matrix.drawIndicator("L",code,2);
    matrix.drawIndicator("L","O",3);
    matrix.setRing?.(2,color);
    matrix.setRing?.(3,orange);
    return;
  }
  matrix.drawText(name, 2, 10, parent);
  if (pageMode === "look" && pageIndex === 11) {
    for (let uch = 1; uch <= 16; uch++) {
      const bit = uch === 16 ? 0 : uch;
      let dot = (s.muteMask & (1 << bit)) ? UI_GREEN.slice() : UI_RED.slice();
      if (uch === s.muteSel) dot = [Math.min(255, dot[0] + 100), Math.min(255, dot[1] + 60), Math.min(255, dot[2] + 60)];
      matrix.light(uch, 3, dot);
    }
    matrix.drawIndicator("L", "Y", 2);
    matrix.drawIndicator("L", "W", 3);
    return;
  }
  if (pageMode === "etc" && pageIndex === ETC_FIRE_INDEX) {
    const value = s.legacySubValue(pageMode, pageIndex);
    const wide = COLS >= 24;
    const focus = s.fireFocus | 0;
    let voiceColor = value.color;
    if (value.text !== "OFF" && value.text !== "ALL") voiceColor = pal(s).col[Number(value.text)] || value.color;
    matrix.drawText(String(s.fireSize || 1), 1, 3, [255, 140, 20]);
    matrix.drawText(String(s.fireLevel || 1), wide ? 8 : 5, 3, [255, 180, 60]);
    if (wide) {
      matrix.drawText(String(s.fireGravity || 0), 14, 3, focus === 1 ? [220, 220, 220] : [70, 70, 70]);
      matrix.drawText(String(s.fireColor == null ? 8 : s.fireColor), 20, 3, focus === 2 ? [255, 220, 120] : [80, 70, 30]);
      matrix.drawText(value.text, 26, 3, voiceColor);
    } else if (focus === 1) {
      matrix.drawText(String(s.fireGravity || 0), 12, 3, [220, 220, 220]);
    } else if (focus === 2) {
      matrix.drawText(String(s.fireColor == null ? 8 : s.fireColor), 12, 3, [255, 220, 120]);
    } else {
      const label = value.text === "ALL" ? "A" : value.text === "OFF" ? "-" : value.text;
      matrix.drawText(label, label.length > 1 ? 10 : 12, 3, voiceColor);
    }
    const half = Math.max(1, Math.floor(COLS / 2));
    const gDots = Math.round(((s.fireGravity || 0) * half) / 8);
    const cDots = Math.round(((s.fireColor == null ? 8 : s.fireColor) * (COLS - half)) / 8);
    for (let x = 1; x <= COLS; x++) {
      if (x <= half && x <= gDots) matrix.light(x, 8, focus === 1 ? [255, 255, 255] : [180, 180, 180]);
      else if (x > half && x - half <= cDots) matrix.light(x, 8, focus === 2 ? [255, 200, 60] : [140, 90, 20]);
    }
    matrix.drawIndicator("L", "O", 1);
    matrix.drawIndicator("L", "O", 2);
    matrix.drawIndicator("L", focus === 1 ? "W" : focus === 2 ? "Y" : value.text === "OFF" ? "R" : "G", 3);
    return;
  }
  if (pageMode === "midi" && pageIndex === 5) {
    // SYNC SNC1/SNC2
    const edit = s.syncEdit | 0;
    const ms = edit === 0 ? (s.syncSendMs | 0) : (s.syncRcveMs | 0);
    matrix.drawText(edit === 0 ? "SYC>" : "SYC<", 2, 10, parent);
    const active = syncHasEffect(s, edit, ms);
    matrix.drawText(active ? String(ms) : "--", 2, 3, !active || edit === 1 ? UI_RED : UI_GREEN);
    matrix.drawIndicator("L", "Y", 2);
    matrix.drawIndicator("L", "W", 3);
    return;
  }
  if (pageMode === "midi" && pageIndex === 6) {
    // PPQN out
    const focus = s.ppqnFocus | 0;
    const width = s.ppqnWidthMs || 1;
    const stop = s.ppqnStopWithPlay ? "STOP" : "CONT";
    const rate = PPQN_RATE_LABELS[s.ppqnRateSel | 0] || "OFF";
    let shown = rate;
    let code = rate === "OFF" ? "R" : "G";
    if (focus === 1) { shown = stop; code = s.ppqnStopWithPlay ? "R" : "G"; }
    else if (focus === 2) { shown = String(width); code = "W"; }
    matrix.drawText(shown, 2, 3, code === "R" ? UI_RED : code === "G" ? UI_GREEN : UI_WHITE);
    matrix.drawIndicator("L", "W", 1);
    matrix.drawIndicator("L", s.ppqnStopWithPlay ? "R" : "G", 2);
    matrix.drawIndicator("L", code, 3);
    return;
  }
  if (pageMode === "midi" && pageIndex === 8) {
    // CIN hold + PPQN
    const focus = s.cinFocus | 0;
    const hold = clampUi(s.cinHoldMs || 1, 1, 32);
    const ppqn = CIN_PPQN_TABLE[s.cinPpqnIndex | 0] || 4;
    const shown = focus === 1 ? String(hold) : s.cinEnabled === false ? "OFF" : (s.cinPolarityPos === false ? "-" : "+") + ppqn;
    matrix.drawText(shown, 2, 3, focus === 1 ? UI_WHITE : s.cinEnabled === false || s.cinPolarityPos === false ? UI_RED : UI_GREEN);
    matrix.drawIndicator("L", focus === 1 ? "W" : "G", 1);
    matrix.drawIndicator("L", focus === 1 ? "G" : "W", 3);
    return;
  }
  const value = pageMode === "touchy"
    ? { text: s.touchyEnabled ? "ON" : "OFF", color: s.touchyEnabled ? UI_GREEN : UI_RED, code: s.touchyEnabled ? "G" : "R" }
    : s.legacySubValue(pageMode, pageIndex);
  matrix.drawText(value.text, 2, 3, value.color);
  matrix.drawLargeCustom(value.color, 3);
}

function clampUi(v, lo, hi) { return v < lo ? lo : v > hi ? hi : v; }

export function drawLoadSave(matrix, s) {
  const isNew = s.slot < 0;
  const exists = isNew ? true : s.slotExists("toern-web-patterns", s.slot);
  showIcon(matrix, "folder", 2, 7, isNew ? UI_BLUE : (exists ? UI_GREEN : UI_DIM_RED));
  if (isNew) matrix.drawText("+", 4, 6, UI_WHITE);
  matrix.drawIndicator("L", "X", 4);
  matrix.setRing(3, [0, 0, 0]);
  if (!isNew && exists && !s.loadSoundSettings) {
    const x = COLS <= 16 ? 9 : 19;
    for (const [dx,y] of [[0,3],[2,3],[1,4],[0,5],[2,5]]) matrix.light(x+dx,y,[255,0,0]);
    matrix.setRing(3, [255, 0, 0]);
  }
  if (isNew) {
    // Same indicators as Autosave when present: green load opens NEW, no save.
    matrix.drawIndicator("M", "G", 1);
    matrix.drawIndicator("M", "E", 2);
  } else if (s.slot === 0) {
    matrix.drawIndicator("M", exists ? "G" : "E", 1);
    matrix.drawIndicator("M", "E", 2);
    matrix.drawText("A", 11, 11, exists ? UI_BRIGHT_GREEN : UI_BLUE);
  } else if (exists) {
    matrix.drawIndicator("M", "G", 1);
    matrix.drawIndicator("M", "D", 2);
    matrix.drawNumber(s.slot, UI_BRIGHT_GREEN, 11);
  } else {
    matrix.drawIndicator("M", "E", 1);
    matrix.drawIndicator("M", "R", 2);
    matrix.drawNumber(s.slot, UI_BLUE, 11);
  }
}

export function drawPack(matrix, s) {
  const exists = s.slotExists("toern-web-packs", s.packSlot);
  showIcon(matrix, "pack", 2, 7, exists ? UI_GREEN : UI_DIM_RED);
  matrix.drawIndicator("M", exists ? "G" : "E", 1);
  matrix.drawIndicator("M", exists ? "D" : "R", 2);
  matrix.drawIndicator("L", "X", 4);
  matrix.drawNumber(s.packSlot, exists ? UI_BRIGHT_GREEN : UI_BLUE, 11);
}

function fileSizeColor(bytes) {
  const kb = 1024;
  const green = 500 * kb;
  const blue = 1200 * kb;
  const red = 1707 * kb;
  if (bytes <= green) return [0, 255, 0];
  if (bytes <= blue) {
    const t = (bytes - green) / (blue - green);
    return [0, Math.round(255 * (1 - t)), Math.round(255 * t)];
  }
  const t = Math.min(1, (bytes - blue) / (red - blue));
  return [Math.round(255 * t), 0, Math.round(255 * (1 - t))];
}

function spectral(y) {
  const hue = Math.round(((y - 5) / 5) * 170);
  const sector = Math.floor(hue / 60);
  const rem = hue % 60;
  const q = Math.round((255 * (60 - rem)) / 60);
  const t = Math.round((255 * rem) / 60);
  const rgb = sector === 0 ? [255, t, 0] : sector === 1 ? [q, 255, 0] : sector === 2 ? [0, 255, t] : [0, q, 255];
  return rgb.map((c) => Math.round(c * 0.4));
}

function drawMarquee(matrix, text, color, now, file) {
  if (!matrix.__marquee || matrix.__marquee.text !== text) {
    matrix.__marquee = { text, offset: 0, dir: 1, next: now + 300 };
  }
  const mark = matrix.__marquee;
  const width = textPixelWidth(text);
  const initial = file && text.length > 4 ? 4 : 1;
  if (text.length === 1) {
    matrix.drawText(text, 14, 12, color);
    return;
  }
  const offsetMax = Math.max(0, initial + width - 15);
  if (text.length > 4 && width > 14 && now >= mark.next) {
    if (mark.dir > 0) {
      mark.offset += 1;
      if (mark.offset >= offsetMax) { mark.offset = offsetMax; mark.dir = -1; mark.next = now + 500; }
      else mark.next = now + 50;
    } else {
      mark.offset -= 1;
      if (mark.offset <= 0) { mark.offset = 0; mark.dir = 1; mark.next = now + 500; }
      else mark.next = now + 50;
    }
  }
  matrix.drawText(text, initial - mark.offset, 12, color);
}

function drawDepth(matrix, dir) {
  const rel = dir.startsWith("samples/") ? dir.slice(8) : "";
  let x = 1;
  const put = (ch, color) => {
    const w = glyphWidth(ch);
    if (x + w > COLS + 1) return false;
    matrix.drawText(ch, x, 6, color);
    x += w + 1;
    return true;
  };
  if (!put(".", [0, 220, 0])) return;
  for (const seg of rel.split("/").filter(Boolean)) {
    if (!put(">", [255, 220, 0])) return;
    if (!put(seg[0] || "?", [200, 230, 255])) return;
  }
}

export function drawWave(matrix, s, now) {
  const item = s.browseItems[s.browse];
  const file = !!s.wavFile;
  matrix.drawIndicator("L", "P", 1);
  if (file) {
    matrix.drawIndicator("L", "Y", 2);
    matrix.drawIndicator("L", "G", 3);
  }
  matrix.drawIndicator("L", "W", 4);
  matrix.setRing(1, [255, 0, 255]);
  matrix.setRing(2, file ? [255, 255, 0] : [0, 0, 0]);
  matrix.setRing(3, file ? [0, 255, 0] : [0, 0, 0]);
  matrix.setRing(4, [255, 255, 255]);
  if (file) {
    const ch = s.GLOB.currentChannel;
    const inv = !!s.invOf[ch];
    const seek = inv ? 100 - s.endOf[ch] : s.seekOf[ch];
    const end = inv ? 100 - s.seekOf[ch] : s.endOf[ch];
    const seekStartX = Math.max(1, Math.min(COLS, Math.round(1 + (seek / 100) * (COLS - 1))));
    const seekEndX = Math.max(seekStartX + 1, Math.min(COLS, Math.round(1 + (end / 100) * (COLS - 1))));
    for (let x = 1; x <= COLS; x++) matrix.light(x, 3, [0, 0, 50]);
    for (let x = 1; x <= seekStartX; x++) matrix.light(x, 3, [0, 80, 0]);
    for (let x = seekEndX; x <= COLS; x++) matrix.light(x, 3, [80, 0, 0]);
    if (s.peaks) {
      const max = Math.max(0.05, ...s.peaks);
      const gain = Math.min(10, 1 / max);
      for (let x = 1; x <= COLS; x++) {
        const pos = seek + ((x - 1) / (COLS - 1)) * (end - seek);
        let idx = Math.max(0, Math.min(s.peaks.length - 1, (pos / 100) * (s.peaks.length - 1)));
        if (inv) idx = s.peaks.length - 1 - idx;
        const lo = Math.floor(idx);
        const hi = Math.min(s.peaks.length - 1, lo + 1);
        const v = (s.peaks[lo] * (1 - (idx - lo)) + s.peaks[hi] * (idx - lo)) * gain;
        const yPeak = Math.max(5, Math.min(10, Math.round(5 + v * 5)));
        for (let y = 5; y <= yPeak; y++) matrix.light(x, y, spectral(y));
      }
    }
  } else drawDepth(matrix, s.browseDir || "samples");
  const color = file ? fileSizeColor(item?.size || 0) : [255, 255, 0];
  drawMarquee(matrix, s.wavName || "-", color, now || 0, file);
}

export function drawSong(matrix, s) {
  matrix.drawIndicator("L", "M", 2);
  matrix.drawIndicator("L", "N", 4);
  matrix.drawText(String(s.songPattern).padStart(2, "0"), 1, 11, hsv(s.songPattern * 16, 255, 255));
  matrix.drawText(String(s.songPos).padStart(2, "0"), 10, 11, [0, 255, 255]);
  const stored = s.song[s.songPos - 1];
  if (stored > 0) matrix.drawText(">" + String(stored).padStart(2, "0"), 1, 5, hsv(stored * 16, 255, 255));
  for (let i = 0; i < COLS && i < 64; i++) {
    const pos = i;
    const pattern = s.song[pos];
    const color = pos === s.songPos - 1 ? [200, 200, 0] : pattern > 0 ? hsv(pattern * 16, 255, 80) : [5, 5, 5];
    matrix.light(i + 1, 3, color);
  }
}

export function drawBoot(matrix, now, bootAt, wavesOnly = false) {
  const t = wavesOnly ? Math.max(0, now - bootAt) : Math.min(now - bootAt, 5000);
  const sineFade = Math.min(1, t / 1000);
  const logoFade = wavesOnly ? 0 : Math.max(0, Math.min(1, (t - 1000) / 2500));
  const fadeOut = wavesOnly || t <= 3500 ? 1 : Math.max(0, 1 - (t - 3500) / 1500);
  const timeSec = t * 0.001;
  const amp = 3.1 + 0.45 * Math.sin(timeSec * 1.4);
  const center = (ROWS + 1) / 2;
  const logoX0 = ((COLS - 16) / 2) + 1;
  for (let x = 1; x <= COLS; x++) {
    const phase = ((x - 1) / (COLS - 1)) * Math.PI * 4;
    const level = sineFade * fadeOut;
    const wavePixel = (waveY, baseColor) => {
      const firstY = Math.floor(waveY), fraction = waveY - firstY;
      for(let i=0;i<2;i++) {
        const y=firstY+i, coverage=i?fraction:1-fraction;
        if(y<1||y>ROWS||coverage<=0)continue;
        let touches=false;
        for(let dy=-1;dy<=1;dy++)for(let dx=-1;dx<=1;dx++) {
          const lx=x+dx-logoX0, ly=y+dy-1;
          if(lx>=0&&lx<16&&ly>=0&&ly<16&&((LOGO[ly]>>(15-lx))&1))touches=true;
        }
        const ink=baseColor.map(c=>Math.round(c*level*coverage*(touches?1-.9*logoFade:1)));
        if(ink.some(c=>c>0))matrix.light(x,y,ink);
      }
    };
    wavePixel(center + 0.85 + amp * Math.sin(phase + 0.55 - timeSec * 2.6), [3,8,28]);
    wavePixel(center - 0.85 + amp * Math.sin(phase - timeSec * 5.4), [26,26,26]);
  }
  if (logoFade * fadeOut > 0) {
    for (let y = 0; y < 16; y++) {
      for (let x = 0; x < 16; x++) {
        if (((LOGO[y] >> (15 - x)) & 1) === 0) continue;
        const hue = Math.round((((x + y) * 20 + ((timeSec * 0.22) % 1) * 360) % 360) * 255 / 360);
        matrix.light(logoX0 + x, y + 1, hsv(hue, 255, Math.round(255 * logoFade * fadeOut)));
      }
    }
  }
  return !wavesOnly && t >= 5000;
}

const VERSION = "v3.02";
const INFO_MSG = "   Thank you for using TOERN. Shout out to Matzesampler, Sabrina, Hairy and all others for supporting me. Jan";

function drawInfo(matrix, s, now, color) {
  matrix.drawText("INFO", 2, 10, color);
  if (!s.infoAt) s.infoAt = now;
  if (now - s.infoAt < 2000) {
    matrix.drawText(VERSION, 2, 3, [255, 255, 0]);
    return;
  }
  const offset = Math.floor((now - s.infoAt - 2000) / 60);
  const msgX = COLS + 1 - offset;
  matrix.drawText(VERSION, 2 - offset, 3, [255, 255, 0]);
  matrix.drawText(INFO_MSG, msgX, 3, UI_WHITE);
  if (msgX < -textPixelWidth(INFO_MSG) - 2) s.infoAt = now;
}

export function drawBpm(matrix, s) {
  const b = s.ledBrightness;
  const leds = Math.max(1, Math.min(16, Math.floor(((b - 3) * 16) / 252) + 1));
  for (let x = 1; x <= leds; x++) {
    const color = b === 64 ? [255, 255, 0] : [16 * x, 16 * x, 16 * x];
    matrix.light(x, 15, color);
    matrix.light(x, 16, color);
  }
  const analogExt = !s.clockInt && s.analogExtClock;
  const arrowCode = s.clockInt ? "G" : analogExt ? "O" : "R";
  matrix.drawIndicator("L", "W", 2);
  matrix.drawIndicator("L", arrowCode, 3);
  matrix.drawIndicator("L", "N", 4);
  const value = Math.round(s.bpm * 10);
  const digitColor = s.clockInt ? [0, 120, 120] : analogExt ? [255, 140, 0] : [0, 255, 0];
  const whole = String(Math.floor(value / 10));
  const width = textPixelWidth(whole) + 4;
  const start = Math.max(1, Math.floor((COLS - width) / 2) + 1);
  matrix.drawText(whole, start, 6, digitColor);
  const dot = start + textPixelWidth(whole);
  matrix.light(dot, 6, digitColor);
  matrix.drawText(String(value % 10), dot + 1, 6, digitColor);
  const x = COLS - 1;
  const now = performance.now();
  const pulseFresh = analogExt && s.analogPulseAt && (now - s.analogPulseAt) < 80;
  const color = s.clockInt ? [0, 255, 0] : analogExt
    ? (pulseFresh ? [255, 180, 40] : [180, 90, 0])
    : [255, 0, 0];
  if (s.clockInt) {
    matrix.light(x + 1, 12, color);
    matrix.light(x, 11, color);
    matrix.light(x, 13, color);
  } else {
    matrix.light(x - 1, 12, color);
    matrix.light(x, 11, color);
    matrix.light(x, 13, color);
  }
}

/** Comical eyes screensaver. Geometry and timing follow drawScreensaverEye / drawScreensaverMatrix. */
const eyesState = {
  dreamOffsetY: 0, dreamPhase: 0, lastDreamMs: 0,
  lastBlinkMs: 0, nextBlinkInterval: 3000, blinking: false,
  targetPupilX: 0, targetPupilY: 0, pupilX: 0, pupilY: 0,
  lastMoveMs: 0, nextMoveInterval: 1500, lastInterpMs: 0,
};

function lightEye(matrix, x, y, color) {
  if (x < 1 || y < 1 || x > COLS || y > ROWS) return;
  matrix.light(x, y, color);
}

function drawScreensaverEye(matrix, baseX, baseY, closed, color, pColor, pupilOffX, pupilOffY) {
  if (closed) {
    for (let dx = 0; dx < 5; dx++) lightEye(matrix, baseX + dx, baseY + 3, color);
    lightEye(matrix, baseX + 1, baseY + 4, color);
    lightEye(matrix, baseX + 2, baseY + 4, color);
    lightEye(matrix, baseX + 3, baseY + 4, color);
    return;
  }
  lightEye(matrix, baseX + 1, baseY + 6, color);
  lightEye(matrix, baseX + 2, baseY + 6, color);
  lightEye(matrix, baseX + 3, baseY + 6, color);
  lightEye(matrix, baseX + 0, baseY + 5, color);
  lightEye(matrix, baseX + 4, baseY + 5, color);
  for (let dy = 1; dy <= 4; dy++) {
    lightEye(matrix, baseX + 0, baseY + dy, color);
    lightEye(matrix, baseX + 4, baseY + dy, color);
  }
  lightEye(matrix, baseX + 1, baseY + 0, color);
  lightEye(matrix, baseX + 2, baseY + 0, color);
  lightEye(matrix, baseX + 3, baseY + 0, color);

  let pupilX = baseX + 1 + pupilOffX;
  let pupilY = baseY + 2 + pupilOffY;
  if (pupilX < baseX + 1) pupilX = baseX + 1;
  if (pupilX > baseX + 2) pupilX = baseX + 2;
  if (pupilY < baseY + 1) pupilY = baseY + 1;
  if (pupilY > baseY + 3) pupilY = baseY + 3;
  lightEye(matrix, pupilX, pupilY, pColor);
  lightEye(matrix, pupilX + 1, pupilY, pColor);
  lightEye(matrix, pupilX, pupilY + 1, pColor);
  lightEye(matrix, pupilX + 1, pupilY + 1, pColor);
}

export function drawEyesScreensaver(matrix, s, now) {
  now = Math.floor(now);
  const idleMs = now - Math.floor(s.lastActivityAt || now);
  const asleep = idleMs >= 90000;
  const outline = asleep ? 7 : 15;
  const pupil = asleep ? 7 : 80;
  const eyeColor = [outline, outline, outline];
  const pupilColor = [pupil, pupil, pupil];
  const totalWidth = 12;
  const leftEyeX = Math.floor((COLS - totalWidth) / 2) + 1;
  const rightEyeX = leftEyeX + 7;
  let eyeY = 5;
  const st = eyesState;

  if (asleep) {
    if (now - st.lastDreamMs >= 3000) {
      st.lastDreamMs = now;
      st.dreamPhase = (st.dreamPhase + 1) % 4;
      st.dreamOffsetY = st.dreamPhase === 1 ? 1 : st.dreamPhase === 3 ? -1 : 0;
    }
    eyeY += st.dreamOffsetY;
  } else {
    st.dreamOffsetY = 0;
    st.dreamPhase = 0;
  }

  if (!asleep) {
    if (!st.blinking && now - st.lastBlinkMs >= st.nextBlinkInterval) {
      st.blinking = true;
      st.lastBlinkMs = now;
    }
    if (st.blinking && now - st.lastBlinkMs >= 150) {
      st.blinking = false;
      st.nextBlinkInterval = 2500 + (now % 2500);
    }
  } else {
    st.blinking = false;
  }

  if (!asleep) {
    if (now - st.lastMoveMs >= st.nextMoveInterval) {
      st.lastMoveMs = now;
      st.targetPupilX = -1 + (now % 3);
      st.targetPupilY = -1 + (Math.floor(now / 7) % 3);
      st.nextMoveInterval = 800 + (now % 1200);
    }
    if (now - st.lastInterpMs >= 100) {
      st.lastInterpMs = now;
      if (st.pupilX < st.targetPupilX) st.pupilX++;
      else if (st.pupilX > st.targetPupilX) st.pupilX--;
      if (st.pupilY < st.targetPupilY) st.pupilY++;
      else if (st.pupilY > st.targetPupilY) st.pupilY--;
    }
  } else {
    st.pupilX = 0;
    st.pupilY = 0;
  }

  const closed = st.blinking || asleep;
  drawScreensaverEye(matrix, leftEyeX, eyeY, closed, eyeColor, pupilColor, st.pupilX, st.pupilY);
  drawScreensaverEye(matrix, rightEyeX, eyeY, closed, eyeColor, pupilColor, st.pupilX, st.pupilY);
}

export function drawRecord(matrix, s, now) {
  matrix.setRing(1, [255, 255, 0]);
  matrix.drawIndicator("L", "Y", 1);
  if (s.recOn) matrix.setRing(2, [0, 0, 0]);
  else {
    matrix.setRing(2, [255, 0, 0]);
    matrix.drawIndicator("L", "R", 2);
  }
  matrix.setRing(3, [0, 255, 0]);
  matrix.drawIndicator("L", "G", 3);
  matrix.setRing(4, [0, 0, 255]);
  matrix.drawIndicator("L", "X", 4);
  const level = Math.max(0, Math.min(16, Math.round((s.recLevel || 0) * 16)));
  for (let y = 1; y <= level; y++) matrix.light(2, y, [255, 0, 0]);
  if (s.recOn) {
    const sec = Math.max(0, (now - s.recAt) / 1000);
    matrix.drawText(sec.toFixed(1).padStart(4, " "), 3, 5, [255, 140, 0]);
  } else if (s.recPlay) {
    const sec = Math.max(0, (now - s.recPlayAt) / 1000);
    matrix.drawText(sec.toFixed(1).padStart(4, " "), 3, 5, [0, 255, 0]);
  } else matrix.drawText("RDY", 5, 5, [255, 100, 0]);
}

function drawOk(matrix, t) {
  const pts = [[-3, -1], [-2, -2], [-1, -3], [0, -2], [1, -1], [2, 0], [3, 1], [4, 2]];
  const reveal = Math.min(pts.length, Math.floor((t * pts.length) / 420));
  let bri = 220;
  if (t < 120) bri = Math.round(40 + (t / 120) * 180);
  else if (t > 780) bri = Math.round(220 - ((t - 780) / 220) * 200);
  for (let i = 0; i < reveal; i++) matrix.light(16 + pts[i][0], 8 + pts[i][1], [0, bri, 0]);
}

function drawRecordingBorder(matrix, dotted = false) {
  const red = [255, 0, 0];
  for (let x = 1; x <= COLS; x++) {
    if (!dotted || (x & 1)) {
      matrix.light(x, 1, red);
      matrix.light(x, ROWS, red);
    }
  }
  for (let y = 2; y < ROWS; y++) {
    if (!dotted || (y & 1)) {
      matrix.light(1, y, red);
      matrix.light(COLS, y, red);
    }
  }
}

export const TOUCHY_COLORS = [[255,64,64], [255,170,32], [64,255,128], [64,128,255], [220,64,255]];
export const TOUCHY_SPRITE = ["     BBBBBBBBBBBB    ", "    BCCCCCCCCCCCCB   ", "    BCEEFEEFDEEECB D ", "OO  BCEFEFEDGDFECBDGD", "OLO BCEEEEEDGGDDDDGGD", " OLOBCEEEEEDGGGGGGGGD", "  OLBCEEFEEDGKJGGKJGD", "  OOBCFEEEEDEJJGGJJED", "   DBCCCCCCCDGGGGGGD ", "    OLOBOLOBOLODOLO  ", "    OLL LLO OLL LLO  "];
export const TOUCHY_RAINBOW = [[215,2,29],[255,144,36],[255,255,0],[65,255,76],[0,191,255],[212,38,255]];
export const TOUCHY_PALETTE = {
  B:[72,30,5], C:[170,86,22], D:[0,0,0], E:[170,62,105],
  F:[255,4,94], G:[48,48,56], L:[112,112,124], O:[28,28,36], J:[0,0,0], K:[200,195,170]
};
export function drawTouchyCat(light, columns, now, tint = TOUCHY_PALETTE.E) {
  const phase = Math.floor(now / 250) % 4;
  // The nose reaches the last column, then floats back only 20% of panel width.
  const x0 = Math.round(columns - 20 - Math.floor(columns * .2) * (1 - Math.cos(now * Math.PI / 3000)) / 2);
  const top = Math.round(14 + .65 * Math.sin(now * Math.PI / 1800));
  // Fixed leg roots; only the toes tuck in/out, in opposing pairs.
  const sprite = TOUCHY_SPRITE.map(row=>row.split(""));
  const tails = [
    ["OO  ","OLO "," OLO","  OL","  OO"],
    ["    ","OO  ","OLLO"," OLL","  OO"],
    ["    ","    "," OOO","OLLL","OOOO"]
  ];
  const tail = tails[[0,1,2,1][phase]];
  for(let y=0;y<5;y++)for(let x=0;x<4;x++)sprite[y+3][x]=tail[y][x];
  const toes=["OLL","OLO","LLO","OLO"];
  [4,8,12,16].forEach((x,leg)=>{
    const toe=toes[(phase+(leg%2)*2)%4];
    for(let dx=0;dx<3;dx++) {
      sprite[9][x+dx]="OLO"[dx];
      sprite[10][x+dx]=toe[dx];
    }
  });
  for(let x=1;x<=columns;x++) {
    const wave = Math.round(.65*Math.sin(x*.6-now*Math.PI/600));
    for(let y=1;y<=16;y++) {
      const row=top-4+wave-y;
      let color=x<x0+5 && row>=0 && row<6 ? TOUCHY_RAINBOW[row] : [0,0,0];
      const pixel=sprite[top-y]?.[x-x0];
      if(pixel && pixel!==" ")color=pixel==="E"?tint:TOUCHY_PALETTE[pixel];
      light(x,y,color.map(v=>Math.floor(v/2)));
    }
  }
}

export function drawTouchy(matrix, s, now) {
  const tint = s.touchyTintActive ? TOUCHY_COLORS[(s.touchyLastVoice || 1)-1].map(v=>Math.floor(v*3/5)) : TOUCHY_PALETTE.E;
  drawTouchyCat((x,y,color)=>matrix.light(x,y,color),COLS,now,tint);
  for(let i=0;i<4;i++)matrix.setRing?.(i+1,TOUCHY_COLORS[i].map(v=>Math.floor(v/2)),true);
}

export const DEMO_TEXT = "MATZE SAMPLER";
export const DEMO_TEXT_SCALE = 2;
export const DEMO_M_GLYPH = [5, 0x1f, 0x02, 0x04, 0x02, 0x1f];
export function demoGlyph(ch) { return ch === "M" ? DEMO_M_GLYPH : ALPHABET[ch.charCodeAt(0) - 32]; }
export const DEMO_TEXT_WIDTH = [...DEMO_TEXT].reduce((width,ch)=>width+(demoGlyph(ch)[0]+1)*DEMO_TEXT_SCALE,0);
export function demoPhase(elapsed, columns = COLS) {
  const scrollMs = (columns + DEMO_TEXT_WIDTH + 1) * 65;
  const cycleMs = 10000 + 3 * scrollMs;
  let t = Math.max(0, elapsed) % cycleMs;
  if (t < 10000) return { name: "nyan", elapsed: t };
  t -= 10000;
  for (const name of ["red", "white", "rainbow"]) {
    if (t < scrollMs) return { name, elapsed: t, x: columns + 1 - Math.floor(t / 65) };
    t -= scrollMs;
  }
  return { name: "nyan", elapsed: 0 };
}
export function drawDemo(matrix, elapsed) {
  const phase = demoPhase(elapsed);
  if (phase.name === "nyan") {
    drawTouchyCat((x,y,color)=>matrix.light(x,y,color), COLS, phase.elapsed);
  } else if (["red", "white", "rainbow"].includes(phase.name)) {
    drawBoot(matrix, elapsed, 0, true);
    // Double each font pixel in both dimensions; scroll entirely off-grid.
    let x = phase.x;
    for (const ch of DEMO_TEXT) {
      const color = phase.name === "red" ? [255,0,0] : phase.name === "white" ? [255,255,255] : hsv((x*12 + elapsed/25)%256,255,255);
      const glyph = demoGlyph(ch);
      for (let col = 0; col < glyph[0]; col++) for (let row = 0; row < 5; row++) {
        const bits = glyph[col + 1];
        if (!(bits & (1 << row))) continue;
        for (let dx = 0; dx < DEMO_TEXT_SCALE; dx++) for (let dy = 0; dy < DEMO_TEXT_SCALE; dy++)
          matrix.light(x + col * DEMO_TEXT_SCALE + dx, 4 + (4 - row) * DEMO_TEXT_SCALE + dy, color);
      }
      x += (glyph[0] + 1) * DEMO_TEXT_SCALE;
    }


  }
}

export function renderFrame(matrix, s, now) {
  matrix.clear();
  matrix.setBrightness(s.ledBrightness);
  matrix.setCursor(0, 0);
  if (s.mode !== "etc") s.infoAt = 0;
  if (s.okAt && now - s.okAt < 1000 && !(s.fillOverlayUntil > now)) {
    drawOk(matrix, now - s.okAt);
    matrix.present();
    return;
  }
  const eyesIdle = s.eyesIdleMs || 0;
  if (eyesIdle > 0 && s.lastActivityAt && (now - s.lastActivityAt) >= eyesIdle
      && (s.mode === "draw" || s.mode === "single") && !s.playing) {
    drawEyesScreensaver(matrix, s, now);
    matrix.present();
    return;
  }
  if (s.mode === "demo") {
    drawDemo(matrix, now - s.demoAt);
  } else if (s.mode === "boot") {
    if (drawBoot(matrix, now, s.bootAt)) s.mode = "draw";
  } else if (s.mode === "draw" || s.mode === "single" || s.mode === "shift") {
    const voice = (pal(s).col[s.GLOB.currentChannel] || [255, 0, 0]).slice();
    matrix.setRing(1, voice);
    matrix.setRing(2, [0, 0, 0]);
    matrix.setRing(3, [0, 255, 0]);
    matrix.setRing(4, voice);
    if (s.playing) {
      const fade = Math.min(1, Math.max(0, s.beatPos || 0));
      matrix.setRing(3, [Math.round(0x55 + (255 - 0x55) * (1 - fade) ** 2), 0, 0], true);
    }
    if (s.solo) {
      matrix.setRing(1, [255, 255, 255]);
      matrix.setRing(2, [0, 0, 0]);
      matrix.setRing(3, [0, 255, 0]);
      matrix.setRing(4, [255, 0, 255]);
    }
    if (s.fastRecActive) {
      matrix.setRing(3, [255, 40, 0]);
    }
    drawBase(matrix, s);
    drawTriggers(matrix, s, now);
    if (s.playing) drawTimer(matrix, s);
    if (now - s.filterFlashAt < 800 && s.filterFlash) {
      const i = FILTER_NAMES.indexOf(s.filterFlash);
      const spec = (filterPagesFor(s.GLOB.currentChannel)[s.filt[s.GLOB.currentChannel].fastPage || 0] || filterPagesFor(s.GLOB.currentChannel)[0])[i];
      if (spec) drawFilterCheck(matrix, s.filt[s.GLOB.currentChannel][spec.key] * 32 / (spec.max || 32), FILTER_COLORS[i] || UI_WHITE);
    }
    if (s.volBarAt && now - s.volBarAt < 600) {
      const vol = s.volBar || 0;
      for (const x of [12, 13]) {
        if (!vol) matrix.light(x, 1, [60, 0, 0]);
        else for (let y = 1; y <= vol; y++) matrix.light(x, y, hsv(Math.round((y / 16) * 96), 255, Math.round(180 + (y / 16) * 75)));
      }
    }
    drawCursor(matrix, s, now);
    if (s.chNrAt && now - s.chNrAt < 800 && s.mode === "draw" && ([1,2,3,4,5,6,7,8,11,13,14].includes(s.chNr))) drawChannelNr(matrix, s);
    if (s.pageNrAt && now - s.pageNrAt < 800 && (s.mode === "draw" || s.mode === "single")) drawPageNr(matrix, s);
    if (s.copyArmed) {
      const text = "c+p";
      const width = textPixelWidth(text) - 1;
      const x = Math.max(1, Math.floor((COLS - width) / 2) + 1);
      const y = Math.floor((ROWS - 5) / 2) + 1;
      for (let xx = x - 1; xx <= x + width; xx++) {
        for (let yy = y - 1; yy <= y + 5; yy++) {
          if (xx >= 1 && xx <= COLS && yy >= 1 && yy <= ROWS) matrix.light(xx, yy, [0, 0, 0]);
        }
      }
      matrix.drawText(text, x, y, [255, 255, 0]);
    }
    if (s.solo && now - s.soloArrowAt < 250 && s.soloArrow) matrix.drawText(s.soloArrow, 7, 8, [255, 255, 255]);
    if (s.mode === "shift") matrix.drawText("SHFT", 2, 11, [120, 120, 0]);
    if (s.sensCountActive) {
      const n = s.sensCountSec || 3;
      const col = n === 1 ? [255, 255, 255] : n === 2 ? [0, 255, 0] : [255, 255, 0];
      const rec = "REC";
      const recW = rec.length * 4 - 1;
      const recX = Math.floor((COLS - recW) / 2) + 1;
      matrix.drawText(String(n), Math.floor(COLS / 2) - 1, 3, col);
      matrix.drawText(rec, recX, 9, [255, 0, 0]);
      drawRecordingBorder(matrix, true);
    } else if (s.fastRecActive) drawRecordingBorder(matrix);
  } else if (s.mode === "velocity") drawVelocity(matrix, s);
  else if (s.mode === "filter") drawFilter(matrix, s, now);
  else if (s.mode === "menu") drawMenu(matrix, s);
  else if (s.mode === "touchy") drawTouchy(matrix, s, now);
  else if (SUBS[s.mode]) drawSubmenu(matrix, s, now);
  else if (s.mode === "pset") {
    matrix.drawText("PSET", 2, 10, UI_WHITE);
    matrix.drawText(String(s.presetSlot), 2, 3, s.presetExists() ? UI_GREEN : UI_BLUE);
    matrix.drawIndicator("L", "G", 1); matrix.drawIndicator("L", "R", 2); matrix.drawIndicator("L", "X", 4);
  }
  else if (s.mode === "dat") drawLoadSave(matrix, s);
  else if (s.mode === "new") drawNew(matrix, s);
  else if (s.mode === "kit") drawPack(matrix, s);
  else if (s.mode === "wav") drawWave(matrix, s, now);
  else if (s.mode === "rec") {
    drawRecord(matrix, s, now);
    if (s.recOn) drawRecordingBorder(matrix);
  }
  else if (s.mode === "song") drawSong(matrix, s);
  else if (s.mode === "bpm") drawBpm(matrix, s);
  if (s.mode !== "touchy" && s.fnArrow && now - s.fnArrowAt < 350) matrix.drawText(s.fnArrow, 7, 8, UI_WHITE);
  for (let ch = 1; ch <= 8; ch++) {
    if (!s.faderAt?.[ch] || now - s.faderAt[ch] >= 800) continue;
    const height = s.faderPositions[ch] || 0;
    const x = Math.floor((ch - 1) * COLS / 8) + 1;
    const color = pal(s).col[ch];
    for (let y = 1; y <= height; y++) {
      matrix.light(x, y, y === height ? UI_WHITE : nscale(color, 150));
      if (x < COLS) matrix.light(x + 1, y, nscale(color, 90));
    }
    if (!height) matrix.light(x, 1, UI_RED);
  }
  if (s.fillOverlayUntil > now) {
    const text = "FILL", width = textPixelWidth(text);
    const x = Math.max(1, Math.floor((COLS - width) / 2) + 1);
    for (let xx = x - 1; xx <= x + width; xx++) for (let y = 5; y <= 11; y++) matrix.light(xx, y, [0,0,0]);
    matrix.drawText(text, x, 6, pal(s).col[s.fillOverlayCh] || UI_WHITE);
  }
  if (s.mode === "filter" && s.filterResetUntil > now) {
    const cx = Math.floor((COLS + 1) / 2), cy = Math.floor((ROWS + 1) / 2);
    for (let x = cx-4; x <= cx+5; x++) for (let y = cy-4; y <= cy+3; y++) matrix.light(x, y, [0, 0, 0]);
    const points = [[-3,-1],[-2,-2],[-1,-3],[0,-2],[1,-1],[2,0],[3,1],[4,2]];
    for (const [x,y] of points) matrix.light(cx+x,cy+y,[0,0,255]);
  }
  matrix.present();
}
