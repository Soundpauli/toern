import { COL, COL_BASE, COLS, ROWS, PAGES, SCHEMES, filterPagesFor } from "./const.js";
import { blend, hsv, nscale } from "./matrix.js";
import { glyphWidth, textPixelWidth } from "./font.js";
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
  { name: "DAT", label: "FILE", icon: "folder", col: 1 },
  { name: "PAT", label: "PAT", icon: "pattern", col: 3 },
  { name: "KIT", label: "PACK", icon: "pack", col: 2 },
  { name: "WAV", label: "WAVE", icon: "sample", col: 3 },
  { name: "BPM", label: "BPM", icon: "clock", col: 4 },
  { name: "VOL", label: "VOL", icon: "volume", col: 5 },
  { name: "SETT", label: "SETT", icon: "settings", col: 6 },
  { name: "RECS", label: "REC", icon: "record", col: 7 },
  { name: "MIDI", label: "MIDI", icon: "midi", col: 8 },
  { name: "SONG", label: "SONG", icon: "song", col: 13 },
  { name: "ETC", label: "ETC", icon: "pattern", col: 14 },
];

export const SUBS = {
  look: ["FLW", "PREV", "VIEW", "PMD", "LOOP", "CTRL", "LEDS", "PONG", "CRSR", "DRAW", "MUTE"],
  recs: ["INPT", "MIC", "L-IN", "TRIG", "CLR"],
  midi: ["CH", "TRAN", "SEND", "RCVE", "CLMP", "SYNC", "PPQN"],
  vol: ["MAIN", "GAIN", "LOUT", "PREV", "2-CH", "SPKR", "HFC"],
  etc: ["INFO", "RAM", "SD", "AUTO", "LGHT", "COLR", "BATT", "CHLD", "RSET"],
  pat: ["TECH", "HIPH", "DNB", "HOUS", "AMBT"],
};

const SUB_PARENT = { look: 6, recs: 7, midi: 8, vol: 5, etc: 14, pat: 3 };

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
  if (!s.GLOB.singleMode) {
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
}

function drawStatus(matrix, s) {
  const y = s.GLOB.y;
  if (s.mode === "draw" && y === 16) {
    matrix.drawIndicator("M", "R", 1);
    matrix.drawIndicator("M", "Y", 4);
  }
  if (s.GLOB.singleMode && y === 16) {
    matrix.drawIndicator("M", "R", 1);
    matrix.drawIndicator("M", "W", 2);
    matrix.drawIndicator("M", "Y", 4);
  }
}

export function drawPages(matrix, s) {
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
  const base = (s.GLOB.edit - 1) * COLS;
  const easy = s.simpleNotes && !s.GLOB.singleMode;
  const col = pal(s).col;
  const baseCol = pal(s).base;
  for (let ix = 1; ix <= COLS; ix++) {
    for (let iy = 1; iy <= ROWS; iy++) {
      const cell = s.note[base + ix][iy];
      const ch = cell.channel;
      if (!ch) continue;
      if (easy) {
        const voiceY = ch + 1;
        matrix.light(ix, iy, [0, 0, 0]);
        if (voiceY >= 1 && voiceY <= ROWS) {
          matrix.light(ix, voiceY, s.mute[ch] ? baseCol[ch] : col[ch]);
        }
        continue;
      }
      if (s.mute[ch]) {
        matrix.light(ix, iy, s.GLOB.singleMode ? [20, 20, 20] : (baseCol[ch] || [0, 0, 0]));
        continue;
      }
      if (s.GLOB.singleMode && ch !== s.GLOB.currentChannel) {
        matrix.light(ix, iy, baseCol[ch] || [0, 0, 0]);
        continue;
      }
      if (s.fullMute) {
        matrix.light(ix, iy, baseCol[ch] || [0, 0, 0]);
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
        matrix.light(ix, iy, color);
      } else if (s.GLOB.singleMode) {
        matrix.light(ix, iy, baseCol[ch] || [0, 0, 0]);
      } else {
        matrix.light(ix, iy, col[ch] || [255, 255, 255]);
      }
    }
  }
}

export function drawTimer(matrix, s) {
  const page = Math.floor((s.beat - 1) / COLS) + 1;
  if (page !== s.GLOB.edit) return;
  const x = ((s.beat - 1) % COLS) + 1;
  for (let y = 1; y < ROWS; y++) {
    const ch = s.note[(s.GLOB.edit - 1) * COLS + x][y].channel;
    matrix.light(x, y, [28, 0, 0]);
    if (ch > 0 && !s.mute[ch]) {
      if (!s.GLOB.singleMode || s.GLOB.currentChannel === ch) matrix.light(x, y, [255, 255, 255]);
    } else if (ch > 0 && !s.GLOB.singleMode) matrix.light(x, y, [0, 0, 0]);
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
  for (let x = 1; x <= 5; x++) for (let y = 11; y <= 16; y++) matrix.light(x, y, [0, 0, 0]);
  for (let x = 1; x <= 5; x++) matrix.light(x, 10, color);
  matrix.drawText(String(ch), 2, 12, color);
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
  const ix1 = 19;
  const ix2 = 20;
  const ix3 = 21;
  matrix.drawText(num, ix1, 11, tc);
  matrix.light(ix3 + 1, 10, frame);
  matrix.light(ix3, 9, frame);
  matrix.light(ix2, 8, frame);
  matrix.light(ix1, 7, frame);
  matrix.drawText(den, ix1 + 1, 2, tc);
  for (let x = 27; x <= 28; x++) {
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
const INST_NAMES = ["BASS", "KEYS", "CHPT", "PAD", "WOW", "ORG", "FLT", "LEAD", "ARP", "BRSS"];

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
    for (let y = 1; y <= 10; y++) {
      let c = [0, 0, 0];
      if (y === display) c = i === f.fast ? [255, 255, 0] : [255, 255, 255];
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
    const label = spec.key === "wave" ? WAVE_NAMES[val] || String(val) : spec.key === "inst" ? INST_NAMES[val] || String(val) : String(val);
    matrix.drawText(label, focus < 2 ? x1 + 4 : Math.max(1, x0 - 10), 5, blend([255, 0, 0], [0, 255, 0], Math.round((val / (spec.max ?? 32)) * 255)));
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
  const label = page.name === "WAV" && (s.GLOB.currentChannel < 1 || s.GLOB.currentChannel > 8) ? "(-)" : page.label;
  matrix.drawText(label, 2, 3, tc);
}

export function isSoon(mode, index) {
  return !!SOON[mode]?.has(index);
}

const SOON = {
  look: new Set([7]),
  recs: new Set([0, 1, 2, 3, 4]),
  midi: new Set([0, 1, 2, 3, 4, 5, 6]),
  vol: new Set([2, 5, 6]),
  etc: new Set([1, 3, 4, 5, 6]),
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
  if (s.genre) {
    drawLength(matrix, s, 10, false);
    matrix.drawIndicator("L", "V", 4);
    matrix.light(10, 1, [255, 0, 255]);
  } else matrix.drawIndicator("L", "N", 4);
}

export function drawSubmenu(matrix, s, now = performance.now()) {
  const pages = SUBS[s.mode];
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
  if (s.mode === "etc" && s.subIndex === 0) {
    drawInfo(matrix, s, now, parent);
    return;
  }
  s.infoAt = 0;
  const lookTitle = ["FLOW", "PREV", "VIEW", "PMODE", "LOOP", "CTRL", "LEDS", "PONG", "CRSR", "DRAW", "MUTE"];
  const name = s.mode === "look" ? lookTitle[s.subIndex] : pages[s.subIndex];
  matrix.drawText(name, 2, 10, parent);
  if (s.mode === "look" && s.subIndex === 10) {
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
  const value = s.subValue(s.mode, s.subIndex);
  matrix.drawText(value.text, 2, 3, value.color);
  if (value.code) matrix.drawIndicator("L", value.code, 3);
  else matrix.setRing?.(3, value.color);
}

export function drawLoadSave(matrix, s) {
  const exists = s.slotExists("toern-web-patterns", s.slot);
  showIcon(matrix, "folder", 2, 7, exists ? UI_GREEN : UI_DIM_RED);
  matrix.drawIndicator("L", "X", 4);
  if (s.slot === 0) {
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

export function drawBoot(matrix, now, bootAt) {
  const t = Math.min(now - bootAt, 5000);
  const sineFade = Math.min(1, t / 1000);
  const logoFade = Math.max(0, Math.min(1, (t - 1000) / 2500));
  const fadeOut = t <= 3500 ? 1 : Math.max(0, 1 - (t - 3500) / 1500);
  const timeSec = t * 0.001;
  const amp = 3.1 + 0.45 * Math.sin(timeSec * 1.4);
  const center = (ROWS + 1) / 2;
  const logoX0 = ((COLS - 16) / 2) + 1;
  for (let x = 1; x <= COLS; x++) {
    const phase = ((x - 1) / (COLS - 1)) * Math.PI * 4;
    const level = sineFade * fadeOut;
    const blue = Math.round(center + 0.85 + amp * Math.sin(phase + 0.55 - timeSec * 2.6));
    const white = Math.round(center - 0.85 + amp * Math.sin(phase - timeSec * 5.4));
    if (blue >= 1 && blue <= ROWS) matrix.light(x, blue, [Math.round(3 * level), Math.round(8 * level), Math.round(28 * level)]);
    if (white >= 1 && white <= ROWS) matrix.light(x, white, [Math.round(26 * level), Math.round(26 * level), Math.round(26 * level)]);
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
  return t >= 5000;
}

const VERSION = "v2.7";
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

function drawBpm(matrix, s) {
  const b = s.ledBrightness;
  const leds = Math.max(1, Math.min(16, Math.floor(((b - 3) * 16) / 252) + 1));
  for (let x = 1; x <= leds; x++) {
    const color = b === 64 ? [255, 255, 0] : [16 * x, 16 * x, 16 * x];
    matrix.light(x, 15, color);
    matrix.light(x, 16, color);
  }
  matrix.drawIndicator("L", "W", 2);
  matrix.drawIndicator("L", s.clockInt ? "G" : "R", 3);
  matrix.drawIndicator("L", "N", 4);
  const text = String(Math.round(s.bpm)).padStart(3, " ");
  matrix.drawText(text, 2, 6, s.clockInt ? [0, 120, 120] : [0, 255, 0]);
  const x = COLS - 1;
  const color = s.clockInt ? [0, 255, 0] : [255, 0, 0];
  if (s.clockInt) {
    matrix.light(x + 1, 8, color);
    matrix.light(x, 7, color);
    matrix.light(x, 9, color);
  } else {
    matrix.light(x - 1, 8, color);
    matrix.light(x, 7, color);
    matrix.light(x, 9, color);
  }
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

export function renderFrame(matrix, s, now) {
  matrix.clear();
  matrix.setBrightness(s.ledBrightness);
  matrix.setCursor(0, 0);
  if (s.mode !== "etc") s.infoAt = 0;
  if (s.okAt && now - s.okAt < 1000) {
    drawOk(matrix, now - s.okAt);
    matrix.present();
    return;
  }
  if (s.mode === "boot") {
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
      matrix.setRing(3, [255, 0, 255]);
      matrix.setRing(4, [0, 0, 0]);
    }
    drawBase(matrix, s);
    drawTriggers(matrix, s, now);
    if (s.playing) drawTimer(matrix, s);
    if (now - s.filterFlashAt < 800 && s.filterFlash) {
      const i = FILTER_NAMES.indexOf(s.filterFlash);
      drawFilterCheck(matrix, s.filt[s.GLOB.currentChannel][["h", "l", "r", "b"][i]], FILTER_COLORS[i] || UI_WHITE);
    }
    if (s.volBarAt && now - s.volBarAt < 600) {
      const vol = s.volBar || 0;
      for (const x of [12, 13]) {
        if (!vol) matrix.light(x, 1, [60, 0, 0]);
        else for (let y = 1; y <= vol; y++) matrix.light(x, y, hsv(Math.round((y / 16) * 96), 255, Math.round(180 + (y / 16) * 75)));
      }
    }
    drawCursor(matrix, s, now);
    if (s.chNrAt && now - s.chNrAt < 800 && s.mode === "draw" && s.GLOB.y <= 9) drawChannelNr(matrix, s);
    if (s.solo && now - s.soloArrowAt < 250 && s.soloArrow) matrix.drawText(s.soloArrow, 7, 8, [255, 255, 255]);
    if (s.mode === "shift") matrix.drawText("SHFT", 2, 11, [120, 120, 0]);
  } else if (s.mode === "velocity") drawVelocity(matrix, s);
  else if (s.mode === "filter") drawFilter(matrix, s, now);
  else if (s.mode === "menu") drawMenu(matrix, s);
  else if (SUBS[s.mode]) drawSubmenu(matrix, s, now);
  else if (s.mode === "dat") drawLoadSave(matrix, s);
  else if (s.mode === "new") drawNew(matrix, s);
  else if (s.mode === "kit") drawPack(matrix, s);
  else if (s.mode === "wav") drawWave(matrix, s, now);
  else if (s.mode === "rec") drawRecord(matrix, s, now);
  else if (s.mode === "song") drawSong(matrix, s);
  else if (s.mode === "bpm") drawBpm(matrix, s);
  matrix.present();
}
