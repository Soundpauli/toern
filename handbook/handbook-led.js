/* TŒRN matrix previews, selectable panel width; firmware-style 1-based x/y. */
(function () {
  "use strict";

  const TOUCHY_SPRITE = ["     BBBBBBBBBBBB    ", "    BCCCCCCCCCCCCB   ", "    BCEEFEEFDEEECB D ", "OO  BCEFEFEDGDFECBDGD", "OLO BCEEEEEDGGDDDDGGD", " OLOBCEEEEEDGGGGGGGGD", "  OLBCEEFEEDGKJGGKJGD", "  OOBCFEEEEDEJJGGJJED", "   DBCCCCCCCDGGGGGGD ", "    OLOBOLOBOLODOLO  ", "    OLL LLO OLL LLO  "];
const TOUCHY_RAINBOW = [[215,2,29],[255,144,36],[255,255,0],[65,255,76],[0,191,255],[212,38,255]];
const TOUCHY_PALETTE = {
  B:[72,30,5], C:[170,86,22], D:[0,0,0], E:[170,62,105],
  F:[255,4,94], G:[48,48,56], L:[112,112,124], O:[28,28,36], J:[0,0,0], K:[200,195,170]
};
function drawTouchyCat(light, columns, now, tint = TOUCHY_PALETTE.E) {
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
    if (window.HBLayout && window.HBLayout.width === 32) {
      const start = [4, 11, 19, 26][enc - 1] || 4;
      return [start, start + 1, start + 2];
    }
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
      this.w = window.HBLayout ? window.HBLayout.width : 32;
      this.h = 16;
      this.data = new Uint8ClampedArray(this.w * this.h * 4);
      for (let i = 3; i < this.data.length; i += 4) this.data[i] = 255;
      this.cursorX = 0;
      this.cursorY = 0;
    }
    clear() {
      this.menuColors = false;
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
      if (x < 1 || x > this.w || y < 1 || y > this.h) return;
      const ix = (y - 1) * this.w + (x - 1);
      const o = ix * 4;
      this.data[o] = r;
      this.data[o + 1] = g;
      this.data[o + 2] = b;
      this.data[o + 3] = 255;
    }
    indL(enc, code) {
      const [x1, x2, x3] = getIndX(enc);
      const c = this.menuColors ? (code === "O" ? [120, 60, 0] : menuValueColor(indColor(code))) : indColor(code);
      this.set(x1, 1, c[0], c[1], c[2]);
      this.set(x2, 1, c[0], c[1], c[2]);
      this.set(x3, 1, c[0], c[1], c[2]);
      if (this.w === 32) this.set(x3 + 1, 1, ...c);
    }
    indM(enc, code) {
      const [, x2, x3] = getIndX(enc);
      const c = this.menuColors ? (code === "O" ? [120, 60, 0] : menuValueColor(indColor(code))) : indColor(code);
      this.set(x2, 1, c[0], c[1], c[2]);
      this.set(x3, 1, c[0], c[1], c[2]);
      if (this.w === 32) this.set(x3 + 1, 1, ...c);
    }
    blit() {
      const LOGICAL = this.w * 20;
      const HEIGHT = this.h * 20;
      const decoration = this.canvas.hasAttribute("data-decoration");
      const dpr = Math.min(typeof window !== "undefined" ? window.devicePixelRatio || 1 : 1, 2);
      const ctx = this.ctx;
      this.canvas.width = Math.round(LOGICAL * dpr);
      this.canvas.height = Math.round(HEIGHT * dpr);
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      ctx.scale(dpr, dpr);
      ctx.clearRect(0, 0, LOGICAL, HEIGHT);
      ctx.fillStyle = "#0e0e12";
      ctx.fillRect(0, 0, LOGICAL, HEIGHT);

      const pitch = 20;
      const radius = pitch * (decoration ? 0.28 : 0.38);

      // Mirror the device renderer (see tools/colorsheme): hardware brightness ~64/255 means
      // raw dim values like (5,0,0) would be invisible on screen. We apply a gamma lift so the
      // preview matches what a person looking at the actual unit sees: bright voice notes stay
      // bright, idle/base tints become visibly coloured instead of pure black.
      const liftLow = (v) => {
        if (v <= 0) return 0;
        // gamma ≈ 0.45 (sRGB-style) + linear floor for very low channels so 1–6 range remains visible
        const lifted = Math.pow(v / 255, 0.45) * 255;
        const brightness = Math.max(lifted, v < 4 ? v * 12 : lifted);
        return Math.min(255, brightness * (decoration ? 1.65 : 1));
      };

      for (let sy = 0; sy < 16; sy++) {
        for (let sx = 0; sx < this.w; sx++) {
          const o = (sy * this.w + sx) * 4;
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
          if (!decoration && r + g + b > 2) {
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
      if (!decoration && this.cursorX >= 1 && this.cursorX <= this.w && this.cursorY >= 1 && this.cursorY <= 16) {
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

  function menuValueColor(rgb) {
    const peak = Math.max(...rgb);
    return peak ? rgb.map(c => Math.floor(c * 120 / peak)) : rgb.slice();
  }

  function drawText(m, str, startX, startY, r, g, b) {
    if (m.menuColors && startY === 3) [r, g, b] = menuValueColor([r, g, b]);
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
    let startX = m.w - total + 1;
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
    if (m.w === 32) m.set(x3 + 1, 1, r, g, b);
  }

  function largeInd4Parent(m, parentColIdx) {
    const c = menuTextFromCol(parentColIdx);
    indLrgb(m, 4, c[0], c[1], c[2]);
  }

  function rootMenuPage(m, pageIdx, titleColIdx, titleStr) {
    m.clear();
    m.menuColors = true;
    m.indL(4, "G");
    drawMenuPageDots(m, pageIdx, 10, m.w);
    const tc = menuTextFromCol(titleColIdx);
    drawText(m, titleStr, 2, 3, tc[0], tc[1], tc[2]);
  }

  function playSubFrame(m, pageIdx, title, enc2, enc3) {
    const slots = [[0,10,13], [3,7,5], [0,4,6], [1,10,13], [2,10,13], [3,10,13], [1,4,6], [1,7,14], [6,10,13], [0,7,14], [2,4,6], [3,4,6]];
    const [index, count, parent] = slots[pageIdx];
    m.clear();
    m.menuColors = true;
    largeInd4Parent(m, parent);
    drawSubmenuDots16(m, index, count, parent);
    const tc = menuTextFromCol(parent);
    drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc2) m.indL(2, enc2);
    if (enc3) m.indL(3, enc3);
  }

  function volSubFrame(m, pageIdx, title, enc1, enc2, enc3) {
    m.clear();
    m.menuColors = true;
    largeInd4Parent(m, 5);
    drawSubmenuDots16(m, pageIdx, 5, 5);
    const tc = title === "ON" ? [0, 120, 0] : title === "OFF" ? [120, 0, 0] : menuTextFromCol(5);
    drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc1) m.indL(1, enc1);
    if (enc2) m.indL(2, enc2);
    if (enc3) m.indL(3, enc3);
  }

  function recSubFrame(m, pageIdx, title, enc2, enc3) {
    m.clear();
    m.menuColors = true;
    largeInd4Parent(m, 7);
    drawSubmenuDots16(m, pageIdx, 5, 7);
    const tc = menuTextFromCol(7);
    if (title) drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc2) m.indL(2, enc2);
    if (enc3) m.indL(3, enc3);
  }

  function midiSubFrame(m, pageIdx, title, enc1, enc2, enc3) {
    m.clear();
    m.menuColors = true;
    largeInd4Parent(m, 8);
    drawSubmenuDots16(m, pageIdx, 10, 8);
    const tc = menuTextFromCol(8);
    drawText(m, title, 2, 10, tc[0], tc[1], tc[2]);
    if (enc1) m.indL(1, enc1);
    if (enc2) m.indL(2, enc2);
    if (enc3) m.indL(3, enc3);
  }

  function etcSubFrame(m, pageIdx, title, enc1, enc2, enc3) {
    const slots = [[0,5,14], [2,7,14], [1,5,14], [9,10,13], [3,7,14], [4,7,14], [2,5,14], [5,7,14], [4,10,13], [6,7,14], [5,10,13], [4,5,14]];
    const [index, count, parent] = slots[pageIdx];
    m.clear();
    m.menuColors = true;
    largeInd4Parent(m, parent);
    drawSubmenuDots16(m, index, count, parent);
    const tc = menuTextFromCol(parent);
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
    const sx = x => m.w === 16 ? x : 1 + Math.floor((x - 1) * (m.w - 1) / 15);
    for (let x = sx(2); x <= sx(3); x++) {
      for (let y = 1; y <= vy; y++) m.set(x, y, y * y, Math.max(0, 20 - y), 0);
    }
    const dw = UI.DIM_WHITE;
    for (const x of [sx(5), sx(6), sx(7), sx(8)]) {
      m.set(x, 1, ...dw); m.set(x, 16, ...dw);
    }
    for (let y = 2; y <= 15; y++) {
      m.set(sx(5), y, ...dw); m.set(sx(8), y, ...dw);
    }
    const barColors = [[100, 0, 0], [150, 50, 0], [150, 150, 0], [0, 150, 150], [0, 200, 0]];
    const barY = 2 + (probStep - 1) * 3;
    for (let x = sx(6); x <= sx(7); x++) {
      m.set(x, barY, ...barColors[probStep - 1]);
      m.set(x, barY + 1, ...barColors[probStep - 1]);
    }
    const [ix1, ix2, ix3] = getIndX(3);
    const tc = condStep === 2 ? [0, 0, 255] : [0, 200, 0];
    drawText(m, "1", ix1, 11, ...tc);
    m.set(ix3 + 1, 10, ...dw); m.set(ix3, 9, ...dw);
    m.set(ix2, 8, ...dw); m.set(ix1, 7, ...dw);
    drawText(m, condStep === 2 ? "2" : "1", ix1 + 1, 2, ...tc);
    const [, cvX1, cvX2] = getIndX(4);
    for (let x = cvX1; x <= cvX2; x++) {
      for (let y = 1; y <= cv; y++) m.set(x, y, 0, Math.max(0, 20 - y), y * y);
    }
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
      // MIDI EXT locked → green digits, red inbound arrow (encoder cue)
      arrowR = 255; arrowG = 40; arrowB = 40;
      m.indL(3, "R");
    } else if (mode === "ext_analog") {
      textR = 255; textG = 140; textB = 0;
      arrowR = 255; arrowG = 160; arrowB = 40;
      m.indL(3, "O");
    } else {
      textR = 255; textG = 40; textB = 40;
      // MIDI EXT hunting → red digits + red arrow
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
      m.set(m.w - 2, cy - 1, arrowR, arrowG, arrowB);
      m.set(m.w - 1, cy,     arrowR, arrowG, arrowB);
      m.set(m.w - 2, cy + 1, arrowR, arrowG, arrowB);
    } else {
      // "<"  three pixels: (15, cy-1), (14, cy), (15, cy+1)
      m.set(m.w - 1, cy - 1, arrowR, arrowG, arrowB);
      m.set(m.w - 2, cy,     arrowR, arrowG, arrowB);
      m.set(m.w - 1, cy + 1, arrowR, arrowG, arrowB);
    }
  }

  /* Filter page data. Four sliders per page, one short letter above each
   * (always exactly one character — that's all the device renders at rest).
   * The `names` are the full words that the device prints across the matrix
   * while you are turning the corresponding encoder. */
  const FILTER_PAGES = [
    { letters: ["H", "L", "R", "B"], vals: [18, 14, 22, 10], names: ["HCUT", "LCUT", "RVRB", "BITC"] },
    { letters: ["C", "R", "F", "D"], vals: [16, 20, 12,  8], names: ["CUT", "RES", "FLT", "DLAY"] },
    { letters: ["W", "I", "C", "S"], vals: [10, 14, 18, 12], names: ["WAVE", "INST", "CTOF", "SHAP"] },
    { letters: ["A", "D", "S", "R"], vals: [20, 16, 14, 24], names: ["ATCK", "DECY", "SUST", "RELS"] }
  ];
  /* Sample voices page 1 (firmware RES · DTNE · OCTV · TIME). */
  const FILTER_PAGE_SAMPLE_1 = {
    letters: ["R", "D", "O", "T"], vals: [16, 16, 24, 11], names: ["RES", "DTNE", "OCTV", "TIME"]
  };
  function filterPairs(m) { return m.w === 32 ? [[5, 6], [12, 13], [20, 21], [27, 28]] : [[2, 3], [6, 7], [10, 11], [14, 15]]; }

  /* Resting view of a filter page: four vertical sliders; row-12 letters sit on a
   * fixed 16px strip A_D_S_R_ (cols 1,5,9,13 — 3px letter + 1px gap). */
  function drawFilterPageScene(m, pageIdx, pageOverride) {
    const p = pageOverride || FILTER_PAGES[Math.max(0, Math.min(3, pageIdx))] || FILTER_PAGES[0];
    const abbrevStripX = m.w === 32 ? [4, 11, 19, 26] : [1, 5, 9, 13];
    m.clear();
    for (let i = 0; i < 4; i++) {
      const letter = p.letters[i];
      if (!letter) continue;
      drawVerticalSliderLike(m, filterPairs(m)[i][0], filterPairs(m)[i][1], p.vals[i], 32, filterBaseHue(i));
      const tc = filterBaseHue(i);
      drawText(m, letter, abbrevStripX[i], 12, tc[0], tc[1], tc[2]);
    }
  }

  /* "Edit" view of a filter page: same four sliders, but the parameter being
   * turned is highlighted and its full 4-letter name is printed on row 12
   * (5px font → rows 12–16), matching firmware drawSliderName(). */
  function drawFilterPageEditScene(m, pageIdx, editI, pageOverride) {
    const p = pageOverride || FILTER_PAGES[Math.max(0, Math.min(3, pageIdx))] || FILTER_PAGES[0];
    m.clear();
    for (let i = 0; i < 4; i++) {
      const letter = p.letters[i];
      if (!letter) continue;
      drawVerticalSliderLike(m, filterPairs(m)[i][0], filterPairs(m)[i][1], p.vals[i], 32, filterBaseHue(i));
    }
    // Full name on row 12 (same as device); y=13 clipped the top row of the 5px font in preview.
    const idx = Math.max(0, Math.min(3, editI));
    const name = p.names[idx] || "";
    const tc = filterBaseHue(idx);
    if (name) {
      drawText(m, name, m.w === 32 ? Math.floor((m.w - name.length * 4 + 2) / 2) : 1, 12, tc[0], tc[1], tc[2]);
    }
    // Brighten the active slider so the user knows which one is being edited.
    const ax0 = filterPairs(m)[idx][0];
    const ax1 = filterPairs(m)[idx][1];
    for (let y = 1; y <= 10; y++) {
      // Soft halo of the parameter colour next to the active slider.
      m.set(ax0 - 1 < 1 ? 1 : ax0 - 1, y, (tc[0] * 0.15) | 0, (tc[1] * 0.15) | 0, (tc[2] * 0.15) | 0);
      m.set(ax1 + 1 > m.w ? m.w : ax1 + 1, y, (tc[0] * 0.15) | 0, (tc[1] * 0.15) | 0, (tc[2] * 0.15) | 0);
    }
  }

  function drawSongLike(m, pat, pos) {
    m.clear();
    const h1 = ((pat * 16) % 255 + 256) % 255;
    const rgb = chsvToRgb(h1, 255, 255);
    const ps = pat < 10 ? "0" + pat : String(pat);
    drawText(m, ps, 1, 11, rgb[0], rgb[1], rgb[2]);
    const poss = pos < 10 ? "0" + pos : String(pos);
    drawText(m, poss, m.w - 6, 11, 0, 255, 255);
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
    let startX = m.w + 1 - tw;
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
      for (let x = 1; x <= m.w; x++) m.set(x, y, b[0], b[1], b[2]);
    }
    for (let x = 1; x <= m.w - 3; x += 4) m.set(x, 1, 10, 10, 10);
    drawPageSlotsRow16(m, 1, 1, []);
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

  /** Top row (y=16): eight page slots on a wide panel, sixteen on a single panel. */
  function drawPageSlotsRow16(m, editPage, playPage, hasMask) {
    for (let p = 1; p <= 256 / m.w; p++) {
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
        r = 1;
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
      for (let x = 1; x <= m.w; x++) m.set(x, y, baseTint[0], baseTint[1], baseTint[2]);
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
  const bootStartedAt = performance.now();
  // icons.h logo_rows; timing and dual sine curves from drawLogoAnimationFrame().
  const BOOT_LOGO = [0, 0, 0x07f8, 0x1ff8, 0x1980, 0x1980, 0x1980, 0x19f0,
    0x19f0, 0x1980, 0x1980, 0x1980, 0x1ff8, 0x07f8, 0, 0];

  function drawStartup(m) {
    m.clear();
    const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    const t = reducedMotion ? 3000 : (performance.now() - bootStartedAt) % 5000;
    const sineFade = Math.min(1, t / 1000);
    const logoFade = Math.max(0, Math.min(1, (t - 1000) / 2500));
    const fadeOut = t <= 3500 ? 1 : Math.max(0, 1 - (t - 3500) / 1500);
    const sec = t / 1000;
    const amplitude = 3.1 + .45 * Math.sin(sec * 1.4);
    const startX = (m.w - 16) / 2 + 1;
    const logoOn = (x, y) => x >= 0 && x < 16 && y >= 0 && y < 16 && ((BOOT_LOGO[y] >> (15 - x)) & 1);
    const wavePixel = (x, y, color) => {
      const firstY=Math.floor(y), fraction=y-firstY;
      for(let i=0;i<2;i++) {
        const pixelY=firstY+i, coverage=i?fraction:1-fraction;
        if(pixelY<1||pixelY>16||coverage<=0)continue;
        let touches = false;
        for (let dy = -1; dy <= 1; dy++) {
          for (let dx = -1; dx <= 1; dx++) touches ||= !!logoOn(x + dx - startX, pixelY + dy - 1);
        }
        const level = sineFade * fadeOut * (touches ? 1 - .9 * logoFade : 1) * coverage;
        const ink=color.map(c=>Math.round(c*level));
        if(ink.some(c=>c>0))m.set(x,pixelY,...ink);
      }
    };
    for (let x = 1; x <= m.w; x++) {
      const phase = (x - 1) / (m.w - 1) * 4 * Math.PI;
      wavePixel(x, 8.5 + .85 + amplitude * Math.sin(phase + .55 - sec * 2.6), [3, 8, 28]);
      wavePixel(x, 8.5 - .85 + amplitude * Math.sin(phase - sec * 5.4), [26, 26, 26]);
    }
    for (let y = 0; y < 16; y++) {
      for (let x = 0; x < 16; x++) {
        if (!logoOn(x, y)) continue;
        const hue = ((x + y) * 20 + Math.floor((sec * .22 % 1) * 360)) % 360;
        m.set(startX + x, y + 1, ...chsvToRgb(hue * 255 / 360, 255, Math.round(255 * logoFade * fadeOut)));
      }
    }
  }

  const scenes = {
    boot_cover: drawStartup,
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
      rootMenuPage(m, 0, 1, "PATT");
    },
    mr_dat(m) {
      rootMenuPage(m, 0, 1, "PATT");
    },
    mr_kit(m) {
      rootMenuPage(m, 1, 2, "KIT");
    },
    mr_wav(m) {
      rootMenuPage(m, 2, 3, "SMP");
    },
    mr_bpm(m) {
      rootMenuPage(m, 3, 4, "BPM");
    },
    mr_vol(m) {
      rootMenuPage(m, 4, 5, "VOL");
    },
    mr_play(m) {
      rootMenuPage(m, 5, 6, "SETT");
    },
    mr_rec(m) {
      rootMenuPage(m, 6, 7, "REC");
    },
    mr_midi(m) {
      rootMenuPage(m, 7, 8, "MIDI");
    },
    mr_song(m) {
      rootMenuPage(m, 8, 13, "MODE");
    },
    mr_etc(m) {
      rootMenuPage(m, 9, 14, "ETC");
    },
    vol_main(m) {
      volSubFrame(m, 0, "MAIN", "", "", "O");
      drawText(m, "72", 2, 3, 120, 60, 0);
    },
    vol_gain(m) {
      volSubFrame(m, 1, "ALL", "N", "O", "G");
      drawText(m, "1.0x", 2, 3, 0, 255, 0);
      const cols = [
        [0, 200, 200],
        [200, 100, 0],
        [200, 40, 200],
        [0, 255, 0]
      ];
      for (let i = 0; i < 4; i++) {
        const filled = 2;
        const x0 = 1 + i * 4;
        const c = cols[i];
        for (let k = 0; k < 4; k++) {
          if (k < filled) m.set(x0 + k, 8, c[0], c[1], c[2]);
        }
      }
    },
    vol_lout(m) {
      volSubFrame(m, 2, "LOUT", "", "", "M");
      drawText(m, "12", 2, 3, 120, 0, 120);
    },
    vol_prev(m) {
      volSubFrame(m, 3, "PREV", "", "G", "O");
      drawText(m, "35", 2, 3, 120, 60, 0);
    },
    vol_2ch(m) {
      m.clear();
      m.menuColors = true;
      largeInd4Parent(m, 14);
      drawSubmenuDots16(m, 7, 10, 14);
      const tc = menuTextFromCol(14);
      drawText(m, "2-CH", 2, 10, tc[0], tc[1], tc[2]);
      drawText(m, "M+P", 2, 3, 0, 120, 0);
      m.indL(3, "G");
    },
    vol_spkr(m) {
      volSubFrame(m, 4, "SPKR", "", "", "G");
      drawText(m, "ON", 2, 3, 0, 120, 0);
    },
    vol_hfc(m) {
      m.clear();
      m.menuColors = true;
      largeInd4Parent(m, 14);
      drawSubmenuDots16(m, 9, 10, 14);
      const tc = menuTextFromCol(14);
      drawText(m, "HFC", 2, 10, tc[0], tc[1], tc[2]);
      drawText(m, "0", 2, 3, 120, 0, 0);
      m.indL(3, "R");
    },
    adv_usbp(m) {
      m.clear();
      m.menuColors = true;
      largeInd4Parent(m, 14);
      drawSubmenuDots16(m, 8, 10, 14);
      const tc = menuTextFromCol(14);
      drawText(m, "USBP", 2, 10, tc[0], tc[1], tc[2]);
      drawText(m, "OFF", 2, 3, 120, 0, 0);
      m.indL(3, "R");
    },
    lk_flw(m) {
      playSubFrame(m, 0, "FLOW", "", "G");
      drawText(m, "ON", 2, 3, 0, 120, 0);
    },
    lk_prev(m) {
      volSubFrame(m, 3, "PREV", "", "R", "O");
      drawText(m, "PRSS", 2, 3, 120, 0, 0);
    },
    lk_prev_on(m) {
      volSubFrame(m, 3, "PREV", "", "G", "O");
      drawText(m, "35", 2, 3, 120, 60, 0);
    },
    lk_prev_sync(m) {
      volSubFrame(m, 3, "PREV", "", "Y", "O");
      drawText(m, "SYNC", 2, 3, 120, 120, 0);
    },
    bpm_ext_ok(m) {
      drawBPMScreenLike(m, 124.0, "ext_ok");
    },
    bpm_ext_warn(m) {
      drawBPMScreenLike(m, 124.0, "ext_bad");
    },
    bpm_ext_analog(m) {
      drawBPMScreenLike(m, 120.0, "ext_analog");
    },
    filter_p0(m) { drawFilterPageScene(m, 0); },
    filter_p1(m) { drawFilterPageScene(m, 1); },
    filter_p1_sample(m) { drawFilterPageScene(m, 1, FILTER_PAGE_SAMPLE_1); },
    filter_p2(m) { drawFilterPageScene(m, 2); },
    filter_p3(m) { drawFilterPageScene(m, 3); },
    filter_p0_named(m) { drawFilterPageEditScene(m, 0, 1); /* LCUT */ },
    filter_p1_named(m) { drawFilterPageEditScene(m, 1, 1); /* RES */ },
    filter_p1_sample_named(m) { drawFilterPageEditScene(m, 1, 3, FILTER_PAGE_SAMPLE_1); /* TIME */ },
    filter_p2_named(m) { drawFilterPageEditScene(m, 2, 1); /* INST */ },
    filter_p3_named(m) { drawFilterPageEditScene(m, 3, 2); /* SUST */ },
    lk_view(m) {
      playSubFrame(m, 2, "VIEW", "", "W");
      drawText(m, "FULL", 2, 3, 200, 200, 200);
    },
    lk_pmd(m) {
      playSubFrame(m, 3, "PMOD", "", "Y");
      drawText(m, "SONG", 2, 3, 255, 255, 0);
    },
    lk_loop(m) {
      playSubFrame(m, 4, "LOOP", "", "G");
      drawText(m, String(256 / m.w), 2, 3, 0, 200, 255);
    },
    lk_vmod(m) {
      playSubFrame(m, 5, "VMOD", "", "G");
      drawText(m, "ON", 2, 3, 0, 120, 0);
    },
    lk_ctrl(m) {
      playSubFrame(m, 6, "CTRL", "", "G");
      drawText(m, "PAGE", 2, 3, 0, 255, 0);
    },
    lk_leds(m) {
      playSubFrame(m, 7, "LEDS", "", "G");
      drawText(m, m.w === 32 ? "2B" : "1B", 2, 3, 0, 255, 0);
    },
    lk_pong(m) {
      playSubFrame(m, 8, "PONG", "", "B");
      const on = true;
      if (on) {
        drawText(m, "ON", 2, 3, 0, 120, 0);
        drawText(m, "42", 10, 3, 0, 150, 255);
        const bx = 3 + (hbPongPhase % (m.w - 6));
        m.set(bx, 6, 0, 200, 255);
        m.set(bx + 1, 6, 0, 160, 220);
      } else {
        drawText(m, "OFF", 2, 3, 120, 0, 0);
      }
    },
    lk_crsr(m) {
      playSubFrame(m, 9, "CRSR", "", "G");
      drawText(m, "CHNR", 2, 3, 150, 200, 0);
    },
    lk_draw(m) {
      playSubFrame(m, 10, "DRAW", "", "G");
      drawText(m, "L+R", 2, 3, 0, 255, 0);
    },
    lk_mute(m) {
      playSubFrame(m, 11, "MUTE", "Y", "W");
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
      drawText(m, "SENS", 2, 3, 0, 255, 255);
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
    md_clmp(m) {
      midiSubFrame(m, 4, "FOLD", "", "", "G");
      drawText(m, "ON", 2, 3, 0, 120, 0);
    },
    md_sync(m) {
      midiSubFrame(m, 5, "SNC1", "Y", "W", "");
      drawText(m, "-12", 2, 3, 0, 255, 0);
    },
    md_ppqn(m) {
      midiSubFrame(m, 7, "PQ-O", "W", "G", "G");
      drawText(m, "+24", 2, 3, 0, 255, 0);
    },
    md_side(m) {
      midiSubFrame(m, 8, "TRIG", "", "", "G");
      drawText(m, "1", 2, 3, 255, 0, 0);
    },
    md_cin(m) {
      midiSubFrame(m, 6, "PQ-I", "W", "", "G");
      drawText(m, "+4", 2, 3, 0, 255, 0);
    },
    md_wrte(m) {
      midiSubFrame(m, 9, "OVWR", "", "", "R");
      drawText(m, "OFF", 2, 3, 120, 0, 0);
    },
    et_adv(m) {
      m.clear();
      largeInd4Parent(m, 14);
      drawSubmenuDots16(m, 3, 5, 14);
      const tc = menuTextFromCol(14);
      drawText(m, "ADV", 2, 10, tc[0], tc[1], tc[2]);
    },
    md_touchy(m) {
      m.clear(); largeInd4Parent(m, 13); drawSubmenuDots16(m, 8, 10, 13);
      const tc = menuTextFromCol(13);
      drawText(m, "TCHY", 2, 10, tc[0], tc[1], tc[2]);
      drawText(m, "OFF", 2, 3, 120, 0, 0); m.indL(3, "R");
    },
    touchy(m) {
      m.clear();
      const reduced = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
      drawTouchyCat((x,y,color)=>m.set(x,y,...color),m.w,reduced?4200:performance.now());
    },
    md_song(m) {
      m.clear();
      largeInd4Parent(m, 13);
      drawSubmenuDots16(m, 7, 10, 13);
      const tc = menuTextFromCol(13);
      drawText(m, "SONG", 2, 10, tc[0], tc[1], tc[2]);
    },
    et_info(m) {
      etcSubFrame(m, 0, "INFO", "", "", "");
      drawText(m, "3.0x", 2, 3, 255, 220, 0);
    },
    et_ram(m) {
      etcSubFrame(m, 1, "RAM", "", "", "G");
      drawText(m, "A42", 2, 3, 0, 255, 0);
    },
    et_sd(m) {
      etcSubFrame(m, 2, "SD", "", "", "");
      drawText(m, "WAIT", 2, 3, 255, 220, 0);
    },
    et_auto(m) {
      etcSubFrame(m, 3, "AUTO", "G", "Y", "W");
      for (let x = 1; x <= m.w; x++) {
        if (x >= Math.round(m.w * 3 / 16) && x <= m.w / 2) m.set(x, 8, 255, 165, 0);
        else if (x > m.w / 2 && x <= m.w * .75) m.set(x, 8, 0, 255, 0);
        else m.set(x, 8, 0, 0, 0);
      }

    },
    et_lght(m) {
      etcSubFrame(m, 4, "LGHT", "", "", "G");
      drawText(m, "ON", 2, 3, 0, 120, 0);
    },
    et_colr(m) {
      etcSubFrame(m, 5, "COLR", "", "", "G");
      drawText(m, "0", 2, 3, 0, 255, 0);
    },
    et_batt(m) {
      etcSubFrame(m, 6, "BATT", "", "", "");
      drawText(m, "78%", 2, 3, 0, 255, 0);
    },
    et_eyes(m) {
      etcSubFrame(m, 7, "EYES", "", "", "G");
      drawText(m, "1MIN", 2, 3, 0, 255, 0);
    },
    et_chld(m) {
      etcSubFrame(m, 8, "CHLD", "", "", "R");
      drawText(m, "OFF", 2, 3, 120, 0, 0);
    },
    et_fire(m) {
      etcSubFrame(m, 9, "FIRE", "O", "O", "G");
      drawText(m, "1", 1, 3, 255, 140, 20);
      drawText(m, "8", 5, 3, 255, 180, 60);
      drawText(m, "A", 12, 3, 255, 220, 120);
      for (let x = m.w / 2 + 1; x <= m.w; x++) m.set(x, 8, 140, 90, 20);
    },
    et_img(m) {
      etcSubFrame(m, 10, "IMG", "", "", "G");
      drawText(m, "ON", 2, 3, 0, 120, 0);
    },
    et_rset(m) {
      etcSubFrame(m, 11, "RSET", "", "", "O");
      for (let x = 1; x <= m.w; x++) m.set(x, 8, 0, 0, 0);
      drawText(m, "SD", 2, 3, 255, 140, 0);
    },
    et_rset_efx(m) {
      etcSubFrame(m, 11, "RSET", "", "", "O");
      for (let x = 1; x <= m.w; x++) m.set(x, 8, 0, 0, 0);
      drawText(m, "EFX", 2, 3, 255, 100, 0);
    },
    et_rset_full(m) {
      etcSubFrame(m, 11, "RSET", "", "", "O");
      for (let x = 1; x <= m.w; x++) m.set(x, 8, 0, 0, 0);
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
      drawText(m, "KIT", 2, 3, tc[0], tc[1], tc[2]);
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
        for (let x = 1; x <= m.w; x++) m.set(x, y, fill[0], fill[1], fill[2]);
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
      for (let p = 1; p <= m.w; p++) m.set(p, 16, 0, 0, 0);
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
      for (let x = 1; x <= m.w; x++) m.set(x, 3, dimB[0], dimB[1], dimB[2]);
      for (let x = 1; x <= m.w / 4; x++) m.set(x, 3, g[0], g[1], g[2]);
      for (let x = Math.ceil(m.w * .75); x <= m.w; x++) m.set(x, 3, rd[0], rd[1], rd[2]);
      const peaks = [5, 7, 6, 9, 10, 8, 9, 7, 6, 8, 9, 8, 7, 6, 8, 7];
      for (let i = 0; i < m.w; i++) {
        const xh = i + 1;
        for (let yy = 5; yy <= peaks[Math.floor(i * 16 / m.w)]; yy++) {
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
      for (let x = 1; x <= m.w; x++) {
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
    /** Matrix anatomy: example of the grid during DRAW-mode playback.
     * y=1 = beat guide, y=16 = page strip (one tile per page),
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
  window.addEventListener("hb-layout-change", initAll);
  // The cover and TOUCHY animate; other instructional previews keep their chosen scene.
  if (document.querySelector("canvas[data-scene='boot_cover'], canvas[data-scene='touchy']") && !window.matchMedia("(prefers-reduced-motion: reduce)").matches) {
    setInterval(() => {
      if (document.hidden) return;
      document.querySelectorAll("canvas[data-scene='boot_cover'], canvas[data-scene='touchy']").forEach(cv => {
        const rect = cv.getBoundingClientRect();
        if (rect.bottom >= 0 && rect.top <= window.innerHeight) runScene(cv.getAttribute("data-scene"), cv);
      });
    }, 50);
  }
})();
