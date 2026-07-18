/* TOERN matrix preview — 16×16, 1-based x/y like firmware light(). Font from font_3x5.h. */
(function () {
  "use strict";

  const ALPH = [[3,0,0,0],[1,23,0,0],[3,3,0,3],[3,31,10,31],[3,11,31,13],[3,25,4,19],[3,10,21,26],[1,3,0,0],[2,14,17,0],[2,17,14,0],[3,5,2,5],[3,4,14,4],[1,24,0,0],[3,4,4,4],[1,16,0,0],[3,24,4,3],[3,31,17,31],[3,18,31,16],[3,25,21,18],[3,17,21,31],[3,7,4,31],[3,23,21,29],[3,30,21,29],[3,1,1,31],[3,31,21,31],[3,23,21,29],[1,10,0,0],[1,26,0,0],[3,4,10,17],[3,10,10,10],[3,17,10,4],[3,1,21,3],[3,14,17,23],[3,31,5,31],[3,31,21,27],[3,31,17,17],[3,31,17,14],[3,31,21,17],[3,31,5,1],[3,31,17,29],[3,31,4,31],[3,17,31,17],[3,24,16,31],[3,31,4,27],[3,31,16,16],[3,31,6,31],[3,31,1,31],[3,31,17,31],[3,31,5,7],[3,15,25,15],[3,31,5,26],[3,23,21,29],[3,1,31,1],[3,31,16,31],[3,15,16,31],[3,31,12,31],[3,27,4,27],[3,7,28,7],[3,25,21,19],[2,31,17,0],[3,3,4,24],[2,17,31,0],[3,2,1,2],[3,16,16,16],[2,1,2,0],[3,12,18,30],[3,31,18,12],[3,30,18,18],[3,12,18,31],[3,12,26,20],[3,4,31,5],[3,18,21,14],[3,31,4,24],[1,29,0,0],[2,16,13,0],[3,31,4,26],[1,31,0,0],[3,30,4,30],[3,30,2,30],[3,30,18,30],[3,31,9,6],[3,6,9,31],[3,30,2,4],[3,20,22,10],[3,2,15,18],[3,30,16,30],[3,14,16,14],[3,30,8,30],[3,26,4,26],[3,23,20,15],[3,26,18,22],[3,4,31,17],[1,31,0,0],[3,17,31,4],[3,4,12,8]];

  const COL0 = [
    [0, 0, 0], [255, 0, 0], [255, 69, 0], [255, 255, 0], [0, 200, 0], [0, 255, 255], [0, 0, 255], [255, 0, 255],
    [220, 100, 100], [0, 0, 0], [0, 0, 0], [120, 120, 120], [0, 0, 0], [0, 255, 70], [255, 50, 50]
  ];
  const COLB0 = [
    [0, 0, 0], [5, 0, 0], [18, 4, 0], [10, 10, 0], [0, 4, 0], [0, 5, 5], [0, 0, 4], [15, 1, 4],
    [22, 12, 16], [0, 0, 0], [0, 0, 0], [6, 6, 6], [0, 0, 0], [0, 10, 4], [18, 4, 4]
  ];

  const UI = {
    WHITE: [120, 120, 120],
    RED: [120, 0, 0],
    GREEN: [0, 120, 0],
    BLUE: [0, 0, 120],
    CYAN: [0, 120, 120],
    BRIGHT_GREEN: [0, 150, 0],
    BRIGHT_WHITE: [150, 150, 150],
    DIM_WHITE: [30, 30, 30],
    PLAYHEAD_BG: [10, 0, 0]
  };

  function menuTextFromCol(idx) {
    const c = COL0[idx] || [0, 0, 0];
    const m = Math.max(c[0], c[1], c[2], 1);
    const t = 120;
    return [(c[0] * t / m) | 0, (c[1] * t / m) | 0, (c[2] * t / m) | 0];
  }

  function indColor(code) {
    switch (code) {
      case "G": return [0, 255, 0];
      case "R": return [255, 0, 0];
      case "X": return [0, 0, 255];
      case "W": return [255, 255, 255];
      case "N": return [0, 255, 255];
      case "M": return [255, 0, 255];
      case "D": return [100, 0, 0];
      case "E": return [0, 100, 0];
      case "Y": return [255, 220, 0];
      case "B": return [0, 140, 255];
      case "O": return [255, 140, 0];
      case "P": return [255, 0, 255];
      default: return [0, 0, 0];
    }
  }

  function getIndX(enc) {
    switch (enc) {
      case 1: return [1, 2, 3];
      case 2: return [5, 6, 7];
      case 3: return [9, 10, 11];
      case 4: return [13, 14, 15];
      default: return [1, 2, 3];
    }
  }

  class Mtx {
    constructor(canvas) {
      this.canvas = canvas;
      this.ctx = canvas.getContext("2d");
      this.w = 16;
      this.h = 16;
      this.data = new Uint8ClampedArray(this.w * this.h * 4);
      for (let i = 3; i < this.data.length; i += 4) this.data[i] = 255;
      this.cursorX = 0;
      this.cursorY = 0;
    }
    clear() {
      for (let i = 0; i < this.data.length; i += 4) {
        this.data[i] = 0;
        this.data[i + 1] = 0;
        this.data[i + 2] = 0;
        this.data[i + 3] = 255;
      }
      this.cursorX = 0;
      this.cursorY = 0;
    }
    cursor(x, y) {
      this.cursorX = x | 0;
      this.cursorY = y | 0;
    }
    set(x, y, r, g, b) {
      if (x < 1 || x > 16 || y < 1 || y > 16) return;
      const ix = (y - 1) * this.w + (x - 1);
      const o = ix * 4;
      this.data[o] = r;
      this.data[o + 1] = g;
      this.data[o + 2] = b;
      this.data[o + 3] = 255;
    }
    indL(enc, code) {
      const [x1, x2, x3] = getIndX(enc);
      const c = indColor(code);
      this.set(x1, 1, c[0], c[1], c[2]);
      this.set(x2, 1, c[0], c[1], c[2]);
      this.set(x3, 1, c[0], c[1], c[2]);
    }
    indM(enc, code) {
      const [, x2, x3] = getIndX(enc);
      const c = indColor(code);
      this.set(x2, 1, c[0], c[1], c[2]);
      this.set(x3, 1, c[0], c[1], c[2]);
    }
    blit() {
      const LOGICAL = 320;
      const dpr = Math.min(typeof window !== "undefined" ? window.devicePixelRatio || 1 : 1, 2);
      const ctx = this.ctx;
      this.canvas.width = Math.round(LOGICAL * dpr);
      this.canvas.height = Math.round(LOGICAL * dpr);
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      ctx.scale(dpr, dpr);
      ctx.fillStyle = "#0e0e12";
      ctx.fillRect(0, 0, LOGICAL, LOGICAL);

      const pitch = LOGICAL / 16;
      const radius = pitch * 0.38;

      // Mirror the device renderer (see tools/colorsheme): hardware brightness ~64/255 means
      // raw dim values like (5,0,0) would be invisible on screen. We apply a gamma lift so the
      // preview matches what a person looking at the actual unit sees: bright voice notes stay
      // bright, idle/base tints become visibly coloured instead of pure black.
      const liftLow = (v) => {
        if (v <= 0) return 0;
        // gamma ≈ 0.45 (sRGB-style) + linear floor for very low channels so 1–6 range remains visible
        const lifted = Math.pow(v / 255, 0.45) * 255;
        return Math.min(255, Math.max(lifted, v < 4 ? v * 12 : lifted));
      };

      for (let sy = 0; sy < 16; sy++) {
        for (let sx = 0; sx < 16; sx++) {
          const o = (sy * 16 + sx) * 4;
          const r = this.data[o];
          const g = this.data[o + 1];
          const b = this.data[o + 2];
          const dx = sx;
          const dy = 15 - sy;
          const cx = dx * pitch + pitch * 0.5;
          const cy = dy * pitch + pitch * 0.5;
          const lr = liftLow(r) | 0;
          const lg = liftLow(g) | 0;
          const lb = liftLow(b) | 0;
          // Subtle bloom for lit pixels so the preview reads like a real LED
          if (r + g + b > 2) {
            const bright = (lr + lg + lb) / 3;
            ctx.shadowColor = `rgba(${lr},${lg},${lb},0.55)`;
            ctx.shadowBlur = Math.min(pitch * 0.9, 6 + bright / 40);
          } else {
            ctx.shadowColor = "transparent";
            ctx.shadowBlur = 0;
          }
          ctx.beginPath();
          ctx.arc(cx, cy, radius, 0, Math.PI * 2);
          // Unlit cells get a faint dark-grey base instead of pure black so the
          // 16×16 grid is always visible in the previews — mirrors how the
          // device itself never quite "disappears" between LEDs in real light.
          if (r + g + b <= 2) {
            ctx.fillStyle = "#1b1b22";
          } else {
            ctx.fillStyle = `rgb(${lr},${lg},${lb})`;
          }
          ctx.fill();
        }
      }
      ctx.shadowColor = "transparent";
      ctx.shadowBlur = 0;

      // White cursor ring on top, drawn just outside the LED dot for clarity.
      if (this.cursorX >= 1 && this.cursorX <= 16 && this.cursorY >= 1 && this.cursorY <= 16) {
        const dx = this.cursorX - 1;
        const dy = 16 - this.cursorY;
        const cx = dx * pitch + pitch * 0.5;
        const cy = dy * pitch + pitch * 0.5;
        ctx.lineWidth = 1.8;
        ctx.strokeStyle = "rgba(255,255,255,0.9)";
        ctx.beginPath();
        ctx.arc(cx, cy, radius + pitch * 0.16, 0, Math.PI * 2);
        ctx.stroke();
        ctx.lineWidth = 1;
        ctx.strokeStyle = "rgba(255,255,255,0.35)";
        ctx.beginPath();
        ctx.arc(cx, cy, radius + pitch * 0.28, 0, Math.PI * 2);
        ctx.stroke();
      }
    }
  }

  function drawChar(m, c, x, y, r, g, b) {
    if (c < 32 || c > 126) return;
    const idx = c - 32;
    const glp = ALPH[idx];
    if (!glp) return;
    const width = glp[0];
    for (let col = 0; col < width; col++) {
      let columnData = glp[col + 1];
      let flipped = 0;
      for (let row = 0; row < 5; row++) {
        if (columnData & (1 << row)) flipped |= 1 << (4 - row);
      }
      for (let row = 0; row < 5; row++) {
        if (flipped & (1 << row)) m.set(x + col, y + row, r, g, b);
      }
    }
  }

  function drawText(m, str, startX, startY, r, g, b) {
    let xo = startX;
    for (let i = 0; i < str.length; i++) {
      const ch = str.charCodeAt(i);
      drawChar(m, ch, xo, startY, r, g, b);
      const w = ALPH[ch - 32] ? ALPH[ch - 32][0] : 3;
      xo += w + 1;
    }
  }

  function drawMenuPageDots(m, pageIndex, total, maxX) {
    let startX = maxX - total + 1;
    if (startX < 1) startX = 1;
    const red = [200, 20, 20];
    const blue = [35, 85, 220];
    for (let i = 0; i < total; i++) {
      const c = i === pageIndex ? red : blue;
      m.set(startX + i, 16, c[0], c[1], c[2]);
    }
  }

  function dimRgb(rgb, f) {
    return [((rgb[0] * f) | 0) + 0, ((rgb[1] * f) | 0) + 0, ((rgb[2] * f) | 0) + 0];
  }

  function drawSubmenuDots16(m, activeIdx, total, parentColIdx) {
    const p = menuTextFromCol(parentColIdx);
    const d = dimRgb(p, 0.38);
    let startX = 16 - total + 1;
    if (startX < 1) startX = 1;
    for (let i = 0; i < total; i++) {
      const c = i === activeIdx ? p : d;
      m.set(startX + i, 16, c[0], c[1], c[2]);
    }
  }

  function indLrgb(m, enc, r, g, b) {
    const [x1, x2, x3] = getIndX(enc);
    m.set(x1, 1, r, g, b);
    m.set(x2, 1, r, g, b);
    m.set(x3, 1, r, g, b);
  }

  function largeInd4Parent(m, parentColIdx) {
    const c = menuTextFromCol(parentColIdx);
    indLrgb(m, 4, c[0], c[1], c[2]);
  }

  function rootMenuPage(m, pageIdx, titleColIdx, titleStr) {
    m.clear();
    m.indL(4, "G");
    drawMenuPageDots(m, pageIdx, 10, 16);
    const tc = menuTextFromCol(titleColIdx);
    drawText(m, titleStr, 2, 3, tc[0], tc[1], tc[2]);
  }

  function playSubFrame(m, pageIdx, title, enc2, enc3) {
    m.clear();
    largeInd4Parent(m, 6);
    drawSubmenuDots16(m, pageIdx, 11, 6);
    const tc = menuTextFromCol(6);
    drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc2) m.indL(2, enc2);
    if (enc3) m.indL(3, enc3);
  }

  function volSubFrame(m, pageIdx, title, enc3) {
    m.clear();
    largeInd4Parent(m, 5);
    drawSubmenuDots16(m, pageIdx, 6, 5);
    const tc = menuTextFromCol(5);
    drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc3) m.indL(3, enc3);
  }

  function recSubFrame(m, pageIdx, title, enc2, enc3) {
    m.clear();
    largeInd4Parent(m, 7);
    drawSubmenuDots16(m, pageIdx, 5, 7);
    const tc = menuTextFromCol(7);
    if (title) drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc2) m.indL(2, enc2);
    if (enc3) m.indL(3, enc3);
  }

  function midiSubFrame(m, pageIdx, title, enc1, enc2, enc3) {
    m.clear();
    largeInd4Parent(m, 8);
    drawSubmenuDots16(m, pageIdx, 6, 8);
    const tc = menuTextFromCol(8);
    drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc1) m.indL(1, enc1);
    if (enc2) m.indL(2, enc2);
    if (enc3) m.indL(3, enc3);
  }

  function etcSubFrame(m, pageIdx, title, enc1, enc2, enc3) {
    m.clear();
    largeInd4Parent(m, 14);
    drawSubmenuDots16(m, pageIdx, 6, 14);
    const tc = menuTextFromCol(14);
    drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc1) m.indL(1, enc1);
    if (enc2) m.indL(2, enc2);
    if (enc3) m.indL(3, enc3);
  }

  function chsvToRgb(h, s, v) {
    h = ((h % 256) + 256) % 256;
    const hue = (h / 255) * 360;
    const S = s / 255;
    const V = v / 255;
    const C = V * S;
    const X = C * (1 - Math.abs(((hue / 60) % 2) - 1));
    const m = V - C;
    let rp = 0,
      gp = 0,
      bp = 0;
    if (hue < 60) {
      rp = C;
      gp = X;
    } else if (hue < 120) {
      rp = X;
      gp = C;
    } else if (hue < 180) {
      gp = C;
      bp = X;
    } else if (hue < 240) {
      gp = X;
      bp = C;
    } else if (hue < 300) {
      rp = X;
      bp = C;
    } else {
      rp = C;
      bp = X;
    }
    return [((rp + m) * 255) | 0, ((gp + m) * 255) | 0, ((bp + m) * 255) | 0];
  }

  function filterBaseHue(encI) {
    const hues = [0, 14, 28, 42];
    return chsvToRgb(hues[encI] | 0, 255, 255);
  }

  function blendBlackToward(br, bg, bb, t) {
    t = Math.max(0, Math.min(255, t)) / 255;
    return [(br * t) | 0, (bg * t) | 0, (bb * t) | 0];
  }

  function drawVerticalSliderLike(m, x0, x1, val, maxVal, baseRgb) {
    const displayResolution = 10;
    let displayVal = Math.round((val / maxVal) * (displayResolution - 1)) + 1;
    displayVal = Math.max(1, Math.min(displayResolution, displayVal));
    const blendVal = Math.round((val / maxVal) * 255);
    const col = blendBlackToward(baseRgb[0], baseRgb[1], baseRgb[2], blendVal);
    for (let y = 1; y <= displayResolution; y++) {
      let c;
      if (y === displayVal) c = [255, 255, 255];
      else if (y < displayVal) c = col;
      else c = [0, 0, 0];
      m.set(x0, y, c[0], c[1], c[2]);
      m.set(x1, y, c[0], c[1], c[2]);
    }
  }

  function drawVelocityLike(m, vy, probStep, cv, condStep) {
    m.clear();
    for (let x = 2; x <= 3; x++) {
      for (let y = 1; y < vy + 1; y++) {
        m.set(x, y, y * y, Math.max(0, 20 - y), 0);
      }
    }
    const dw = UI.DIM_WHITE;
    m.set(5, 1, dw[0], dw[1], dw[2]);
    m.set(6, 1, dw[0], dw[1], dw[2]);
    m.set(7, 1, dw[0], dw[1], dw[2]);
    m.set(8, 1, dw[0], dw[1], dw[2]);
    m.set(5, 16, dw[0], dw[1], dw[2]);
    m.set(6, 16, dw[0], dw[1], dw[2]);
    m.set(7, 16, dw[0], dw[1], dw[2]);
    m.set(8, 16, dw[0], dw[1], dw[2]);
    for (let y = 2; y <= 15; y++) {
      m.set(5, y, dw[0], dw[1], dw[2]);
      m.set(8, y, dw[0], dw[1], dw[2]);
    }
    for (let x = 6; x < 8; x++) {
      if (probStep === 1) {
        m.set(x, 2, 100, 0, 0);
        m.set(x, 3, 100, 0, 0);
      }
      if (probStep === 2) {
        m.set(x, 5, 150, 50, 0);
        m.set(x, 6, 150, 50, 0);
      }
      if (probStep === 3) {
        m.set(x, 8, 150, 150, 0);
        m.set(x, 9, 150, 150, 0);
      }
      if (probStep === 4) {
        m.set(x, 11, 0, 150, 150);
        m.set(x, 12, 0, 150, 150);
      }
      if (probStep === 5) {
        m.set(x, 14, 0, 200, 0);
        m.set(x, 15, 0, 200, 0);
      }
    }
    const cvX = [9, 10, 11];
    for (let x = cvX[1]; x <= cvX[2]; x++) {
      for (let y = 1; y < cv + 1; y++) {
        m.set(x, y, 0, Math.max(0, 20 - y), y * y);
      }
    }
    let num = "1",
      den = "1",
      tc = [0, 200, 0];
    if (condStep === 2) {
      num = "1";
      den = "2";
      tc = [0, 0, 255];
    }
    drawText(m, num, 13, 11, tc[0], tc[1], tc[2]);
    m.set(16, 10, dw[0], dw[1], dw[2]);
    m.set(15, 9, dw[0], dw[1], dw[2]);
    m.set(14, 8, dw[0], dw[1], dw[2]);
    m.set(13, 7, dw[0], dw[1], dw[2]);
    drawText(m, den, 14, 2, tc[0], tc[1], tc[2]);
  }

  function drawBPMLike(m, bpm, intMode) {
    drawBPMScreenLike(m, bpm, intMode ? "int" : "ext_ok");
  }

  function drawBPMScreenLike(m, bpm, mode) {
    m.clear();
    m.indL(2, "W");
    m.indL(4, "N");
    const n = Math.max(0, Math.min(999, Math.round(bpm))) | 0;
    const buf = ("   " + n).slice(-3);
    let textR, textG, textB, arrowR, arrowG, arrowB;
    if (mode === "int") {
      textR = 0; textG = 255; textB = 255;
      // INT mode → green outbound arrow (device is the master)
      arrowR = 0; arrowG = 220; arrowB = 0;
      m.indL(3, "G");
    } else if (mode === "ext_ok") {
      textR = 0; textG = 255; textB = 0;
      // EXT mode locked → green inbound arrow
      arrowR = 0; arrowG = 220; arrowB = 0;
      m.indL(3, "G");
    } else {
      textR = 255; textG = 40; textB = 40;
      // EXT mode with no clock → red arrow
      arrowR = 255; arrowG = 30; arrowB = 30;
      m.indL(3, "R");
    }
    drawText(m, buf, 2, 6, textR, textG, textB);

    // Tiny chevron on the right side of the screen: three lit cells forming
    // either ">" (INT — clock going out) or "<" (EXT — clock coming in).
    // Centred on row 8 in columns 14–15.
    const cy = 8;
    if (mode === "int") {
      // ">"  three pixels: (14, cy-1), (15, cy), (14, cy+1)
      m.set(14, cy - 1, arrowR, arrowG, arrowB);
      m.set(15, cy,     arrowR, arrowG, arrowB);
      m.set(14, cy + 1, arrowR, arrowG, arrowB);
    } else {
      // "<"  three pixels: (15, cy-1), (14, cy), (15, cy+1)
      m.set(15, cy - 1, arrowR, arrowG, arrowB);
      m.set(14, cy,     arrowR, arrowG, arrowB);
      m.set(15, cy + 1, arrowR, arrowG, arrowB);
    }
  }

  /* Filter page data. Four sliders per page, one short letter above each
   * (always exactly one character — that's all the device renders at rest).
   * The `names` are the full words that the device prints across the matrix
   * while you are turning the corresponding encoder. */
  const FILTER_PAGES = [
    { letters: ["P", "F", "R", "B"], vals: [18, 14, 22, 10], names: ["PAN", "FREQ", "RVRB", "BITC"] },
    { letters: ["C", "R", "F", "" ], vals: [16, 20, 12,  0], names: ["CHRS", "RESO", "FLNG", ""    ] },
    { letters: ["W", "I", "C", "S"], vals: [10, 14, 18, 12], names: ["WAVE", "INST", "CTOF", "SHAP"] },
    { letters: ["A", "D", "S", "R"], vals: [20, 16, 14, 24], names: ["ATCK", "DECY", "SUST", "RELS"] }
  ];
  const FILTER_PAIRS = [[2, 3], [6, 7], [10, 11], [14, 15]];

  /* Resting view of a filter page: four vertical sliders; row-12 letters sit on a
   * fixed 16px strip A_D_S_R_ (cols 1,5,9,13 — 3px letter + 1px gap). */
  function drawFilterPageScene(m, pageIdx) {
    const p = FILTER_PAGES[Math.max(0, Math.min(3, pageIdx))] || FILTER_PAGES[0];
    const abbrevStripX = [1, 5, 9, 13];
    m.clear();
    for (let i = 0; i < 4; i++) {
      const letter = p.letters[i];
      if (!letter) continue;
      drawVerticalSliderLike(m, FILTER_PAIRS[i][0], FILTER_PAIRS[i][1], p.vals[i], 32, filterBaseHue(i));
      const tc = filterBaseHue(i);
      drawText(m, letter, abbrevStripX[i], 12, tc[0], tc[1], tc[2]);
    }
  }

  /* "Edit" view of a filter page: same four sliders, but the parameter being
   * turned is highlighted and its full 4-letter name is printed on row 12
   * (5px font → rows 12–16), matching firmware drawSliderName(). */
  function drawFilterPageEditScene(m, pageIdx, editI) {
    const p = FILTER_PAGES[Math.max(0, Math.min(3, pageIdx))] || FILTER_PAGES[0];
    m.clear();
    for (let i = 0; i < 4; i++) {
      const letter = p.letters[i];
      if (!letter) continue;
      drawVerticalSliderLike(m, FILTER_PAIRS[i][0], FILTER_PAIRS[i][1], p.vals[i], 32, filterBaseHue(i));
    }
    // Full name on row 12 (same as device); y=13 clipped the top row of the 5px font in preview.
    const idx = Math.max(0, Math.min(3, editI));
    const name = p.names[idx] || "";
    const tc = filterBaseHue(idx);
    if (name) {
      drawText(m, name, 1, 12, tc[0], tc[1], tc[2]);
    }
    // Brighten the active slider so the user knows which one is being edited.
    const ax0 = FILTER_PAIRS[idx][0];
    const ax1 = FILTER_PAIRS[idx][1];
    for (let y = 1; y <= 10; y++) {
      // Soft halo of the parameter colour next to the active slider.
      m.set(ax0 - 1 < 1 ? 1 : ax0 - 1, y, (tc[0] * 0.15) | 0, (tc[1] * 0.15) | 0, (tc[2] * 0.15) | 0);
      m.set(ax1 + 1 > 16 ? 16 : ax1 + 1, y, (tc[0] * 0.15) | 0, (tc[1] * 0.15) | 0, (tc[2] * 0.15) | 0);
    }
  }

  function drawSongLike(m, pat, pos) {
    m.clear();
    const h1 = ((pat * 16) % 255 + 256) % 255;
    const rgb = chsvToRgb(h1, 255, 255);
    const ps = pat < 10 ? "0" + pat : String(pat);
    drawText(m, ps, 1, 11, rgb[0], rgb[1], rgb[2]);
    const poss = pos < 10 ? "0" + pos : String(pos);
    drawText(m, poss, 10, 11, 0, 255, 255);
    m.indL(2, "M");
    m.indL(4, "N");
  }

  function drawLoadSaveLike(m, fileNum, txtExists) {
    m.clear();
    m.indM(1, txtExists ? "G" : "E");
    m.indM(2, txtExists ? "D" : "R");
    m.indL(4, "X");
    const numC = txtExists ? UI.BRIGHT_GREEN : UI.BLUE;
    const s = String(fileNum);
    let tw = 0;
    for (let i = 0; i < s.length; i++) tw += (ALPH[s.charCodeAt(i) - 32][0] + 1);
    let startX = 17 - tw;
    if (startX < 1) startX = 1;
    drawText(m, s, startX, 11, numC[0], numC[1], numC[2]);
  }

  function drawDrawGrid(m, notes, playX) {
    m.clear();
    // Fill voice lanes with their base tint so the grid reads like the real device,
    // not an empty black field. Rows 2-15 map to voices 1-14 (firmware: y=row, voice=y-1).
    for (let y = 2; y <= 15; y++) {
      const voice = y - 1;
      const b = COLB0[voice] || [0, 0, 0];
      for (let x = 1; x <= 16; x++) m.set(x, y, b[0], b[1], b[2]);
    }
    for (const n of notes) {
      const c = COL0[n.ch] || [40, 40, 40];
      m.set(n.x, n.y, c[0], c[1], c[2]);
    }
    for (let y = 2; y <= 15; y++) {
      const p = UI.PLAYHEAD_BG;
      m.set(playX, y, p[0], p[1], p[2]);
      const hit = notes.some((n) => n.x === playX && n.y === y);
      if (hit) {
        const w = UI.BRIGHT_WHITE;
        m.set(playX, y, w[0], w[1], w[2]);
      }
    }
  }

  /** Bottom row (y=16) page slots — colours follow firmware drawPages(): edit+play, play, edit, has-notes, empty. */
  function drawPageSlotsRow16(m, editPage, playPage, hasMask) {
    for (let p = 1; p <= 16; p++) {
      const has = !!(hasMask && hasMask[p]);
      let r, g, b;
      if (playPage === editPage && p === editPage) {
        r = 255;
        g = 255;
        b = 50;
      } else if (p === playPage) {
        r = 0;
        g = 255;
        b = 0;
      } else if (p === editPage) {
        r = 255;
        g = 255;
        b = 0;
      } else if (has) {
        r = 0;
        g = 0;
        b = 35;
      } else {
        r = 10;
        g = 0;
        b = 0;
      }
      m.set(p, 16, r, g, b);
    }
  }

  function drawSingleGridBody(m, curCh, notes, playX) {
    // SINGLE mode: the whole grid uses the focused voice's colour. Idle steps
    // are tinted with the dim base tone of that voice; lit notes are the
    // bright tone of that same voice. There is no per-row colour change — the
    // display is "unicolored" because you are looking at one voice only.
    m.clear();
    const baseTint = COLB0[curCh] || [0, 0, 0];
    const liteTint = COL0[curCh]  || [40, 40, 40];
    for (let y = 2; y <= 15; y++) {
      for (let x = 1; x <= 16; x++) m.set(x, y, baseTint[0], baseTint[1], baseTint[2]);
    }
    for (const n of notes) {
      m.set(n.x, n.y, liteTint[0], liteTint[1], liteTint[2]);
    }
    // Playhead column overlay (dim white tint), brighter where it hits a note.
    const p = UI.PLAYHEAD_BG;
    for (let y = 2; y <= 15; y++) m.set(playX, y, p[0], p[1], p[2]);
    for (const n of notes) {
      if (n.x === playX) {
        const w = UI.BRIGHT_WHITE;
        m.set(n.x, n.y, w[0], w[1], w[2]);
      }
    }
  }

  let hbPongPhase = 0;

  const scenes = {
    draw(m) {
      drawDrawGrid(
        m,
        [
          { x: 3, y: 5, ch: 1 },
          { x: 7, y: 8, ch: 2 },
          { x: 12, y: 6, ch: 3 },
          { x: 4, y: 12, ch: 5 }
        ],
        7
      );
      m.cursor(7, 8);
    },
    single(m) {
      // Focus voice 3. Whole grid in that voice's colour; a small melody scattered.
      const cur = 3;
      drawSingleGridBody(
        m,
        cur,
        [
          { x: 2,  y: 6  },
          { x: 4,  y: 8  },
          { x: 7,  y: 5  },
          { x: 10, y: 9  },
          { x: 13, y: 7  },
          { x: 15, y: 10 }
        ],
        8
      );
      m.cursor(8, 8);
    },
    paint(m) {
      drawDrawGrid(
        m,
        [
          { x: 5, y: 6, ch: 1 },
          { x: 9, y: 10, ch: 2 }
        ],
        9
      );
      // Active paint column = the column held by the right hand (visualised as a
      // brighter playhead-like tint). Cursor sits where the next stroke lands.
      const cx = 11;
      for (let y = 2; y <= 15; y++) {
        const c = UI.PLAYHEAD_BG;
        m.set(cx, y, Math.min(255, c[0] + 30), Math.min(255, c[1] + 30), Math.min(255, c[2] + 30));
      }
      m.cursor(cx, 8);
      m.indL(4, "G");
    },
    unpaint(m) {
      drawDrawGrid(
        m,
        [
          { x: 6, y: 8, ch: 3 },
          { x: 6, y: 9, ch: 3 }
        ],
        6
      );
      const cx = 6;
      for (let y = 2; y <= 15; y++) m.set(cx, y, 80, 0, 0);
      m.set(6, 8, 255, 80, 80);
      m.set(6, 9, 255, 80, 80);
      m.cursor(cx, 8);
      m.indL(4, "R");
    },
    menu(m) {
      rootMenuPage(m, 0, 1, "FILE");
    },
    mr_dat(m) {
      rootMenuPage(m, 0, 1, "FILE");
    },
    mr_kit(m) {
      rootMenuPage(m, 1, 2, "PACK");
    },
    mr_wav(m) {
      rootMenuPage(m, 2, 3, "WAVE");
    },
    mr_bpm(m) {
      rootMenuPage(m, 3, 4, "BPM");
    },
    mr_vol(m) {
      rootMenuPage(m, 4, 5, "VOL");
    },
    mr_play(m) {
      rootMenuPage(m, 5, 6, "PLAY");
    },
    mr_rec(m) {
      rootMenuPage(m, 6, 7, "REC");
    },
    mr_midi(m) {
      rootMenuPage(m, 7, 8, "MIDI");
    },
    mr_song(m) {
      rootMenuPage(m, 8, 13, "SONG");
    },
    mr_etc(m) {
      rootMenuPage(m, 9, 14, "ETC");
    },
    vol_main(m) {
      volSubFrame(m, 0, "MAIN", "O");
      drawText(m, "72", 2, 3, 200, 80, 0);
    },
    vol_lout(m) {
      volSubFrame(m, 1, "LOUT", "M");
      drawText(m, "12", 2, 3, 255, 180, 200);
    },
    vol_prev(m) {
      volSubFrame(m, 2, "PREV", "G");
      drawText(m, "35", 2, 3, 0, 160, 80);
    },
    vol_2ch(m) {
      volSubFrame(m, 3, "2-CH", "X");
      drawText(m, "M+P", 6, 3, 0, 200, 0);
    },
    vol_spkr(m) {
      volSubFrame(m, 4, "SPKR", "G");
      drawText(m, "ON", 2, 3, 0, 255, 0);
    },
    vol_hfc(m) {
      volSubFrame(m, 5, "HFC", "R");
      drawText(m, "120", 2, 3, 200, 40, 40);
    },
    lk_flw(m) {
      playSubFrame(m, 0, "FLOW", "", "G");
      drawText(m, "ON", 2, 3, 0, 255, 0);
    },
    lk_prev(m) {
      playSubFrame(m, 1, "PREV", "", "B");
      drawText(m, "PRSS", 2, 3, 0, 150, 255);
    },
    lk_prev_on(m) {
      playSubFrame(m, 1, "PREV", "", "G");
      drawText(m, "ON", 2, 3, 0, 255, 0);
    },
    bpm_ext_ok(m) {
      drawBPMScreenLike(m, 124.0, "ext_ok");
    },
    bpm_ext_warn(m) {
      drawBPMScreenLike(m, 124.0, "ext_bad");
    },
    filter_p0(m) { drawFilterPageScene(m, 0); },
    filter_p1(m) { drawFilterPageScene(m, 1); },
    filter_p2(m) { drawFilterPageScene(m, 2); },
    filter_p3(m) { drawFilterPageScene(m, 3); },
    filter_p0_named(m) { drawFilterPageEditScene(m, 0, 1); /* FREQ */ },
    filter_p1_named(m) { drawFilterPageEditScene(m, 1, 1); /* RESO */ },
    filter_p2_named(m) { drawFilterPageEditScene(m, 2, 1); /* INST */ },
    filter_p3_named(m) { drawFilterPageEditScene(m, 3, 2); /* SUST */ },
    lk_view(m) {
      playSubFrame(m, 2, "VIEW", "", "W");
      drawText(m, "FULL", 2, 3, 200, 200, 200);
    },
    lk_pmd(m) {
      playSubFrame(m, 3, "PMODE", "", "Y");
      drawText(m, "SONG", 2, 3, 255, 255, 0);
    },
    lk_loop(m) {
      playSubFrame(m, 4, "LOOP", "", "G");
      drawText(m, "16", 2, 3, 0, 200, 255);
    },
    lk_ctrl(m) {
      playSubFrame(m, 5, "CTRL", "", "G");
      drawText(m, "PAGE", 2, 3, 0, 255, 0);
    },
    lk_leds(m) {
      playSubFrame(m, 6, "LEDS", "", "G");
      drawText(m, "2B", 2, 3, 0, 255, 0);
    },
    lk_pong(m) {
      playSubFrame(m, 7, "PONG", "", "B");
      const on = true;
      if (on) {
        drawText(m, "ON", 10, 3, 0, 255, 0);
        drawText(m, "42", 2, 3, 0, 150, 255);
        const bx = 3 + (hbPongPhase % 10);
        m.set(bx, 6, 0, 200, 255);
        m.set(bx + 1, 6, 0, 160, 220);
      } else {
        drawText(m, "OFF", 6, 3, 255, 0, 0);
      }
    },
    lk_crsr(m) {
      playSubFrame(m, 8, "CRSR", "", "G");
      drawText(m, "CHNR", 2, 3, 150, 200, 0);
    },
    lk_draw(m) {
      playSubFrame(m, 9, "DRAW", "", "G");
      drawText(m, "L+R", 2, 3, 0, 255, 0);
    },
    lk_mute(m) {
      playSubFrame(m, 10, "MUTE", "W", "Y");
      const sel = 5;
      for (let uch = 1; uch <= 16; uch++) {
        const inMask = uch === 2 || uch === 5 || uch === 8 || uch === 12;
        let r = inMask ? 0 : 255,
          g = inMask ? 255 : 0,
          b = 0;
        if (uch === sel) {
          r = Math.min(255, r + 80);
          g = Math.min(255, g + 60);
        }
        m.set(uch, 3, r, g, b);
      }
    },
    rc_inpt(m) {
      recSubFrame(m, 0, "INPT", "", "W");
      drawText(m, "MIC", 2, 3, 255, 255, 255);
    },
    rc_mic(m) {
      recSubFrame(m, 1, "", "", "R");
      drawText(m, "MIC", 2, 10, 120, 0, 80);
      drawText(m, "48", 2, 3, 180, 0, 0);
    },
    rc_lin(m) {
      recSubFrame(m, 2, "", "", "X");
      drawText(m, "L-IN", 2, 10, 120, 0, 80);
      drawText(m, "22", 2, 3, 0, 0, 180);
    },
    rc_trig(m) {
      recSubFrame(m, 3, "TRIG", "W", "W");
      drawText(m, "SENS", 2, 3, 0, 255, 0);
    },
    rc_clr(m) {
      recSubFrame(m, 4, "CLR", "", "W");
      drawText(m, "FIX", 2, 3, 255, 255, 0);
    },
    md_ch(m) {
      midiSubFrame(m, 0, "CH", "", "", "X");
      drawText(m, "MIDI", 2, 3, 0, 0, 255);
    },
    md_tran(m) {
      midiSubFrame(m, 1, "TRAN", "", "", "G");
      drawText(m, "GET", 2, 3, 0, 255, 0);
    },
    md_send(m) {
      midiSubFrame(m, 2, "SEND", "", "", "Y");
      drawText(m, "BOTH", 2, 3, 255, 220, 0);
    },
    md_rcve(m) {
      midiSubFrame(m, 3, "RCVE", "", "", "G");
      drawText(m, "NOTE", 2, 3, 0, 255, 0);
    },
    md_sync(m) {
      midiSubFrame(m, 4, "SNC1", "Y", "W", "");
      drawText(m, "-12", 2, 3, 0, 255, 0);
    },
    md_ppqn(m) {
      midiSubFrame(m, 5, "PPQN", "", "G", "G");
      drawText(m, "+24", 2, 3, 0, 255, 0);
    },
    et_info(m) {
      etcSubFrame(m, 0, "INFO", "", "", "");
      drawText(m, "1.0", 2, 3, 255, 220, 0);
    },
    et_sd(m) {
      etcSubFrame(m, 1, "SD", "", "", "");
      drawText(m, "WAIT", 2, 3, 255, 220, 0);
    },
    et_auto(m) {
      etcSubFrame(m, 1, "AUTO", "G", "Y", "W");
      for (let x = 1; x <= 16; x++) {
        if (x >= 3 && x <= 8) m.set(x, 8, 255, 165, 0);
        else if (x >= 9 && x <= 12) m.set(x, 8, 0, 255, 0);
        else m.set(x, 8, 0, 0, 0);
      }
      drawText(m, "PAGE", 2, 3, 80, 80, 120);
    },
    et_lght(m) {
      etcSubFrame(m, 2, "LGHT", "", "", "G");
      drawText(m, "ON", 2, 3, 0, 255, 0);
    },
    et_colr(m) {
      etcSubFrame(m, 3, "COLR", "", "", "G");
      drawText(m, "0", 2, 3, 0, 255, 0);
    },
    et_batt(m) {
      etcSubFrame(m, 4, "BATT", "", "", "");
      drawText(m, "78%", 2, 3, 0, 255, 0);
    },
    et_rset(m) {
      etcSubFrame(m, 5, "RSET", "", "", "O");
      for (let x = 1; x <= 16; x++) m.set(x, 8, 0, 0, 0);
      drawText(m, "SD", 2, 3, 255, 140, 0);
    },
    et_rset_efx(m) {
      etcSubFrame(m, 5, "RSET", "", "", "O");
      for (let x = 1; x <= 16; x++) m.set(x, 8, 0, 0, 0);
      drawText(m, "EFX", 2, 3, 255, 100, 0);
    },
    et_rset_full(m) {
      etcSubFrame(m, 5, "RSET", "", "", "O");
      for (let x = 1; x <= 16; x++) m.set(x, 8, 0, 0, 0);
      drawText(m, "FULL", 2, 3, 255, 60, 0);
    },
    filter(m) {
      drawFilterPageScene(m, 1);
    },
    velocity(m) {
      drawVelocityLike(m, 9, 3, 10, 2);
    },
    bpm(m) {
      drawBPMScreenLike(m, 128, "int");
    },
    song(m) {
      drawSongLike(m, 7, 23);
    },
    loadsave(m) {
      drawLoadSaveLike(m, 3, true);
    },
    pack(m) {
      m.clear();
      m.indM(1, "G");
      m.indM(2, "R");
      m.indL(4, "X");
      const tc = menuTextFromCol(2);
      drawText(m, "PACK", 2, 3, tc[0], tc[1], tc[2]);
    },
    mute(m) {
      // Show 8 voice lanes (rows 2-9 here = voices 1-8) with voices 2 and 5 muted (dim base tint),
      // others playing (bright col[]). A few example notes are overlaid in white.
      m.clear();
      const muted = { 2: true, 5: true };
      const notes = [
        { x: 3, y: 2, ch: 1 },
        { x: 7, y: 2, ch: 1 },
        { x: 5, y: 3, ch: 2 },
        { x: 9, y: 6, ch: 5 },
        { x: 12, y: 4, ch: 3 }
      ];
      for (let voice = 1; voice <= 8; voice++) {
        const y = voice + 1;
        const isMuted = !!muted[voice];
        const c = isMuted ? COLB0[voice] : COL0[voice];
        const fill = isMuted
          ? c.map((v) => Math.round(v * 0.6))
          : c.map((v) => Math.round(v * 0.18));
        for (let x = 1; x <= 16; x++) m.set(x, y, fill[0], fill[1], fill[2]);
      }
      for (const n of notes) {
        if (!muted[n.ch]) {
          const c = COL0[n.ch] || [120, 120, 120];
          m.set(n.x, n.y, c[0], c[1], c[2]);
        }
      }
      m.cursor(7, 5);
    },
    newfile(m) {
      m.clear();
      const tc = menuTextFromCol(2);
      drawText(m, "NEW", 2, 8, tc[0], tc[1], tc[2]);
      m.indL(4, "G");
    },
    page_slots(m) {
      m.clear();
      const has = [];
      has[2] = has[5] = has[9] = has[14] = true;
      drawPageSlotsRow16(m, 6, 6, has);
    },
    row16_draw(m) {
      const notes = [
        { x: 3, y: 5, ch: 1 },
        { x: 7, y: 8, ch: 2 },
        { x: 12, y: 6, ch: 3 }
      ];
      drawDrawGrid(m, notes, 8);
      const has = [];
      has[3] = has[7] = has[12] = true;
      drawPageSlotsRow16(m, 7, 4, has);
      m.cursor(7, 8);
      m.indM(1, "R");
      m.indM(4, "Y");
    },
    row16_single(m) {
      const cur = 4;
      const notes = [
        { x: 3,  y: 6  },
        { x: 5,  y: 9  },
        { x: 9,  y: 7  },
        { x: 12, y: 10 },
        { x: 14, y: 6  }
      ];
      drawSingleGridBody(m, cur, notes, 6);
      const has = [];
      has[4] = has[8] = true;
      drawPageSlotsRow16(m, 4, 4, has);
      m.cursor(8, 16);
      m.indM(1, "R");
      m.indM(2, "W");
      m.indM(4, "Y");
    },
    copy_active(m) {
      const notes = [
        { x: 5, y: 6, ch: 2 },
        { x: 10, y: 9, ch: 2 }
      ];
      drawDrawGrid(m, notes, 9);
      for (let p = 1; p <= 16; p++) m.set(p, 16, 0, 0, 0);
      m.cursor(10, 9);
      m.indL(1, "X");
      m.indL(4, "Y");
    },
    note_shift(m) {
      const cur = 3;
      const notes = [
        { x: 4,  y: 7  },
        { x: 7,  y: 9  },
        { x: 10, y: 6  },
        { x: 12, y: 11 }
      ];
      drawSingleGridBody(m, cur, notes, 6);
      const has = [];
      has[4] = true;
      drawPageSlotsRow16(m, 4, 4, has);
      m.cursor(8, 16);
      m.indL(1, "G");
      m.indL(4, "X");
    },
    /** SET_WAV: row 1 = L P / L Y / L G / L W; y=3 trim bar; y=5–10 peaks (simplified). */
    set_wav_trim(m) {
      m.clear();
      m.indL(1, "P");
      m.indL(2, "Y");
      m.indL(3, "G");
      m.indL(4, "W");
      const dimB = [0, 0, 50];
      const g = [0, 80, 0];
      const rd = [80, 0, 0];
      for (let x = 1; x <= 16; x++) m.set(x, 3, dimB[0], dimB[1], dimB[2]);
      for (let x = 1; x <= 4; x++) m.set(x, 3, g[0], g[1], g[2]);
      for (let x = 12; x <= 16; x++) m.set(x, 3, rd[0], rd[1], rd[2]);
      const peaks = [5, 7, 6, 9, 10, 8, 9, 7, 6, 8, 9, 8, 7, 6, 8, 7];
      for (let i = 0; i < 16; i++) {
        const xh = i + 1;
        for (let yy = 5; yy <= peaks[i]; yy++) {
          const t = (yy - 5) / 5;
          m.set(xh, yy, (200 * (1 - t)) | 0, (60 + 140 * t) | 0, (20 + 80 * t) | 0);
        }
      }
      drawText(m, "KICK", 4, 12, 0, 220, 100);
    },
    /** Directory selected: no waveform — path breadcrumb on y=6 (see drawSampleBrowserDepthVisual). */
    set_wav_depth(m) {
      m.clear();
      m.indL(1, "P");
      m.indL(2, "Y");
      m.indL(3, "G");
      m.indL(4, "W");
      for (let x = 1; x <= 16; x++) {
        for (let y = 2; y <= 10; y++) m.set(x, y, 0, 0, 0);
      }
      drawChar(m, ".", 1, 6, 0, 220, 0);
      drawChar(m, ">", 3, 6, 255, 220, 0);
      drawChar(m, "d", 5, 6, 200, 230, 255);
      drawChar(m, ">", 7, 6, 255, 220, 0);
      drawChar(m, "K", 9, 6, 200, 230, 255);
      drawText(m, "DRUMS", 3, 12, 255, 255, 0);
    },
    /** RECORD_MODE idle: threshold col 1, level col 2, RDY text (showDoRecord). */
    record_mode(m) {
      m.clear();
      m.indL(1, "Y");
      m.indL(2, "R");
      m.indL(3, "G");
      m.indL(4, "X");
      for (let y = 1; y <= 7; y++) {
        const c = y <= 5 ? [255, 255, 0] : [255, 200, 0];
        m.set(1, y, c[0], c[1], c[2]);
      }
      for (let y = 1; y <= 5; y++) m.set(2, y, 255, 40, 40);
      drawText(m, "RDY", 5, 5, 255, 100, 0);
    },
    /** Matrix anatomy: faithful resting view of the grid in DRAW mode.
     * y=1 = encoder hint row (4 dim cues), y=16 = page strip (one tile per page),
     * y=2..15 = voice lanes tinted by their col_base[]; a couple of bright notes
     * and the live playhead column give the viewer a real reference. */
    matrix_anatomy(m) {
      drawDrawGrid(
        m,
        [
          { x: 3, y: 5, ch: 1 },
          { x: 9, y: 8, ch: 4 },
          { x: 13, y: 12, ch: 6 }
        ],
        7
      );
      // Page strip: pages 1+2 exist, page 1 is current.
      const has = []; has[1] = has[2] = true;
      drawPageSlotsRow16(m, 1, 2, has);
      m.cursor(9, 8);
      m.indM(1, "G");
      m.indM(2, "Y");
      m.indM(3, "G");
      m.indM(4, "W");
    }
  };

  function runScene(name, canvas) {
    const fn = scenes[name];
    if (!fn || !canvas) return;
    const m = new Mtx(canvas);
    fn(m);
    m.blit();
  }

  function initAll() {
    document.querySelectorAll("canvas.led-matrix").forEach((cv) => {
      const sc = cv.getAttribute("data-scene");
      if (sc) runScene(sc, cv);
    });
    if (typeof window !== "undefined" && !window.__hbPongTimer) {
      window.__hbPongTimer = setInterval(() => {
        hbPongPhase++;
        document.querySelectorAll("canvas.led-matrix[data-scene='lk_pong']").forEach((cv) => runScene("lk_pong", cv));
      }, 380);
    }
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", initAll);
  else initAll();
})();
