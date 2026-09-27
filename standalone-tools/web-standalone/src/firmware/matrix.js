import { COLS, ROWS } from "./const.js";
import { ALPHABET, textPixelWidth } from "./font.js";

const PITCH = 22;
const LED = 7.5;
const WIDE_STARTS = [4, 11, 19, 26];
const NARROW_STARTS = [1, 5, 9, 13];

export function hsv(h, s, v) {
  const H = ((h % 256) + 256) % 256 / 256;
  const S = s / 255;
  const V = v / 255;
  const i = Math.floor(H * 6);
  const f = H * 6 - i;
  const p = V * (1 - S);
  const q = V * (1 - f * S);
  const t = V * (1 - (1 - f) * S);
  const tab = [[V, t, p], [q, V, p], [p, V, t], [p, q, V], [t, p, V], [V, p, q]];
  const [r, g, b] = tab[i % 6];
  return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

export function nscale(rgb, scale) {
  return rgb.map((c) => Math.round((c * scale) / 255));
}

export function blend(a, b, amount) {
  return a.map((c, i) => Math.round(c + ((b[i] - c) * amount) / 255));
}

const INDICATOR = {
  G: [0, 255, 0], R: [255, 0, 0], X: [0, 0, 255], W: [255, 255, 255],
  O: [255, 165, 0], H: [0, 191, 255], B: [0, 150, 255], V: [148, 0, 211],
  P: [50, 0, 50], Y: [255, 255, 0], M: [255, 0, 255], D: [100, 0, 0],
  E: [0, 100, 0], N: [0, 255, 255],
};

export function indicatorColor(code, channelColor) {
  if (code === "C") return channelColor || [0, 0, 0];
  return INDICATOR[code] || [0, 0, 0];
}

function normalize(rgb) {
  const max = Math.max(rgb[0], rgb[1], rgb[2]);
  if (!max || max === 255) return rgb;
  return rgb.map((c) => Math.round((c * 255) / max));
}

export function createMatrix(canvas) {
  const ctx = canvas.getContext("2d", { alpha: false });
  let cols = COLS;
  let pix = new Uint8ClampedArray(cols * ROWS * 4);
  const rings = [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]];
  canvas.width = cols * PITCH;
  canvas.height = ROWS * PITCH;

  function clear() {
    overlay.length = 0;
    pix.fill(0);
    for (let i = 3; i < pix.length; i += 4) pix[i] = 255;
    for (let i = 0; i < 4; i++) rings[i] = [0, 0, 0];
  }

  function light(x, y, rgb) {
    if (x < 1 || y < 1 || x > cols || y > ROWS) return;
    const i = ((y - 1) * cols + (x - 1)) * 4;
    pix[i] = rgb[0];
    pix[i + 1] = rgb[1];
    pix[i + 2] = rgb[2];
    pix[i + 3] = 255;
  }

  function pixel(x, y) {
    const i = ((y - 1) * cols + (x - 1)) * 4;
    return [pix[i], pix[i + 1], pix[i + 2]];
  }

  function drawChar(ch, x, y, color) {
    const c = ch.charCodeAt(0);
    if (c < 32 || c > 126) return 0;
    const g = ALPHABET[c - 32];
    const width = g[0];
    for (let col = 0; col < width; col++) {
      const columnData = g[col + 1];
      let flipped = 0;
      for (let row = 0; row < 5; row++) {
        if (columnData & (1 << row)) flipped |= 1 << (4 - row);
      }
      for (let row = 0; row < 5; row++) {
        if (flipped & (1 << row)) light(x + col, y + row, color);
      }
    }
    return width;
  }

  function drawText(text, x, y, color) {
    let cx = x;
    for (const ch of String(text)) cx += drawChar(ch, cx, y, color) + 1;
  }

  const overlay = [];
  function glyphDots(text, x, y) {
    const dots = [];
    let cx = x;
    for (const ch of String(text)) {
      const c = ch.charCodeAt(0);
      if (c < 32 || c > 126) { cx += 1; continue; }
      const g = ALPHABET[c - 32];
      const width = g[0];
      for (let col = 0; col < width; col++) {
        const columnData = g[col + 1];
        let flipped = 0;
        for (let row = 0; row < 5; row++) if (columnData & (1 << row)) flipped |= 1 << (4 - row);
        for (let row = 0; row < 5; row++) if (flipped & (1 << row)) dots.push([cx + col, y + row]);
      }
      cx += width + 1;
    }
    return dots;
  }
  function overlayText(text, x, y, color) {
    overlay.length = 0;
    overlay.push({ dots: glyphDots(text, x, y), color });
  }
  function paintOverlay() {
    if (!overlay.length) return;
    for (const layer of overlay) {
      const [r, g, b] = layer.color;
      ctx.fillStyle = `rgb(${r},${g},${b})`;
      for (const [x, y] of layer.dots) {
        if (x < 1 || y < 1 || x > cols || y > ROWS) continue;
        const cx = (x - 1) * PITCH + PITCH / 2;
        const cy = (ROWS - y) * PITCH + PITCH / 2;
        ctx.beginPath();
        ctx.arc(cx, cy, PITCH * 0.34, 0, Math.PI * 2);
        ctx.fill();
      }
    }
    overlay.length = 0;
  }

  function drawNumber(count, color, topY) {
    const buffer = String(Math.round(count));
    let startX = cols + 1 - textPixelWidth(buffer);
    if (startX < 1) startX = 1;
    drawText(buffer, startX, topY, color);
  }

  function indicatorXs(encoderNum) {
    const slot = (cols <= 16 ? NARROW_STARTS : WIDE_STARTS)[encoderNum - 1] || 1;
    return [slot, slot + 1, slot + 2, Math.min(cols, slot + 3)];
  }

  function setRing(encoderNum, rgb, keepLevel = false) {
    if (encoderNum >= 1 && encoderNum <= 4) rings[encoderNum - 1] = keepLevel ? rgb : normalize(rgb);
  }

  function drawIndicator(size, code, encoderNum, highlight = false, channelColor = null) {
    let color = indicatorColor(code, channelColor);
    if (highlight) color = color.map((c) => Math.min(255, Math.round(c * 1.5)));
    setRing(encoderNum, color);
    const [x1, x2, x3, x4] = indicatorXs(encoderNum);
    if (size === "S") light(x3, 1, color);
    else if (size === "M") { light(x2, 1, color); light(x3, 1, color); light(x4, 1, color); }
    else if (size === "L" || size === "C") {
      light(x1, 1, color); light(x2, 1, color); light(x3, 1, color); light(x4, 1, color);
      if (size === "C") { light(x3, 2, color); light(x3, 3, color); }
    }
  }

  function drawLargeCustom(color, encoderNum) {
    const [x1, x2, x3, x4] = indicatorXs(encoderNum);
    light(x1, 1, color); light(x2, 1, color); light(x3, 1, color); light(x4, 1, color);
    setRing(encoderNum, color);
  }

  let shown = new Int32Array(cols * ROWS).fill(-1);
  let cursorAt = -1;
  function setCursor(x, y) {
    if (cursorAt >= 0) shown[cursorAt] = -1;
    cursorAt = x >= 1 && y >= 1 && x <= cols && y <= ROWS ? (y - 1) * cols + (x - 1) : -1;
    if (cursorAt >= 0) shown[cursorAt] = -1;
  }
  const gamma = new Uint8Array(256);
  let brightness = 64;
  let gammaFor = -1;
  let photo = false;
  let boost = 0;
  let diffuse = 0;
  let blur = 0;
  let size = 70;
  let base = 0;
  let capOn = 61;
  let capOff = 26;
  let capOpacity = 100;
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  function setBrightness(value) { brightness = value; }
  function setPhoto(on) {
    photo = on;
    shown.fill(-1);
  }
  function setLook(next) {
    boost = next.boost;
    diffuse = next.diffuse;
    blur = next.blur;
    size = next.size;
    base = next.base;
    capOn = next.capOn;
    capOff = next.capOff;
    capOpacity = next.capOpacity;
    shown.fill(-1);
  }
  function lift(v, amount) {
    if (!boost || !v || amount <= 0) return v;
    const x = v / 255;
    const curve = Math.max(0.08, 1 - (boost / 400) * 0.92);
    const gamma = 1 + (curve - 1) * amount;
    return Math.round(255 * Math.pow(x, gamma));
  }
  function vivid(r, g, b) {
    const avg = (r + g + b) / 3;
    const ch = (v) => Math.max(0, Math.min(255, Math.round((avg + (v - avg) * 1.55) * 1.35)));
    return [ch(r), ch(g), ch(b)];
  }
  function flatLed(cx, cy, r, g, b, lit) {
    r = lift(r, 1); g = lift(g, 1); b = lift(b, 1);
    [r, g, b] = vivid(r, g, b);
    const frost = diffuse / 100;
    const radius = 2 + (size / 100) * (PITCH * 0.5 - 3);
    if (!r && !g && !b) {
      const dot = ctx.createRadialGradient(cx, cy, 0, cx, cy, LED * 0.7);
      dot.addColorStop(0, "rgba(16,16,18,0.9)");
      dot.addColorStop(1, "rgba(0,0,0,0)");
      ctx.fillStyle = dot;
      ctx.beginPath();
      ctx.arc(cx, cy, LED * 0.7, 0, Math.PI * 2);
      ctx.fill();
      return;
    }
    const alpha = lit ? 1 : 0.55;
    const core = lit ? 0.08 * (1 - frost) : 0;
    const wr = Math.round(r + (255 - r) * core);
    const wg = Math.round(g + (255 - g) * core);
    const wb = Math.round(b + (255 - b) * core);
    const soft = (blur / 100) * (lit ? 1 : base / 100);
    if (soft > 0) {
      const spread = radius * (1 + soft * 2.4);
      const glow = ctx.createRadialGradient(cx, cy, radius, cx, cy, spread);
      glow.addColorStop(0, `rgba(${r},${g},${b},${alpha * 0.85})`);
      glow.addColorStop(1, `rgba(${r},${g},${b},0)`);
      ctx.fillStyle = glow;
      ctx.beginPath();
      ctx.arc(cx, cy, spread, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.fillStyle = `rgba(${wr},${wg},${wb},${alpha})`;
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, Math.PI * 2);
    ctx.fill();
    if (frost > 0) {
      ctx.beginPath();
      ctx.fillStyle = `rgba(255,255,255,${frost * 0.45 * alpha})`;
      ctx.arc(cx, cy, radius, 0, Math.PI * 2);
      ctx.fill();
    }
    const cap = (lit ? capOn : capOff) / 100;
    const cover = capOpacity / 100;
    if (cap > 0 && cover > 0) {
      ctx.beginPath();
      ctx.fillStyle = `rgba(${r},${g},${b},${cover})`;
      ctx.arc(cx, cy, cap * PITCH * 0.5, 0, Math.PI * 2);
      ctx.fill();
    }
  }

  function present() {
    if (gammaFor !== brightness) {
      const f = brightness / 64;
      for (let c = 0; c < 256; c++) {
        const v = Math.min(255, c * f);
        gamma[c] = v <= 0 ? 0 : Math.round(Math.pow(v / 255, 0.42) * 255);
      }
      gammaFor = brightness;
      shown.fill(-1);
    }
    if (photo) {
      ctx.fillStyle = "#000";
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      for (let y = 1; y <= ROWS; y++) {
        const cy = (ROWS - y) * PITCH + PITCH / 2;
        for (let x = 1; x <= cols; x++) {
          const k = (y - 1) * cols + (x - 1);
          const i = k * 4;
          const key = (pix[i] << 16) | (pix[i + 1] << 8) | pix[i + 2];
          const cx = (x - 1) * PITCH + PITCH / 2;
          const raw = Math.max(pix[i], pix[i + 1], pix[i + 2]);
          flatLed(cx, cy, gamma[pix[i]], gamma[pix[i + 1]], gamma[pix[i + 2]], key && raw >= 40);
          if (k === cursorAt) {
            ctx.beginPath();
            ctx.lineWidth = 1.2;
            ctx.strokeStyle = "rgba(255,255,255,0.85)";
            ctx.arc(cx, cy, LED * 0.95, 0, Math.PI * 2);
            ctx.stroke();
          }
        }
      }
      paintOverlay();
      return;
    }
    for (let y = 1; y <= ROWS; y++) {
      const cy = (ROWS - y) * PITCH + PITCH / 2;
      for (let x = 1; x <= cols; x++) {
        const k = (y - 1) * cols + (x - 1);
        const i = k * 4;
        const key = (pix[i] << 16) | (pix[i + 1] << 8) | pix[i + 2];
        if (shown[k] === key) continue;
        shown[k] = key;
        const cx = (x - 1) * PITCH + PITCH / 2;
        const r = gamma[pix[i]];
        const g = gamma[pix[i + 1]];
        const b = gamma[pix[i + 2]];
        ctx.fillStyle = "#000";
        ctx.fillRect(cx - PITCH / 2, cy - PITCH / 2, PITCH, PITCH);
        const raw = Math.max(pix[i], pix[i + 1], pix[i + 2]);
        if (photo) flatLed(cx, cy, r, g, b, key && raw >= 40);
        else if (raw >= 40) {
          const glow = ctx.createRadialGradient(cx, cy, LED * 0.5, cx, cy, LED + 3);
          glow.addColorStop(0, `rgba(${r},${g},${b},0.7)`);
          glow.addColorStop(1, `rgba(${r},${g},${b},0)`);
          ctx.fillStyle = glow;
          ctx.beginPath();
          ctx.arc(cx, cy, LED + 3, 0, Math.PI * 2);
          ctx.fill();
        }
        if (!photo) {
        ctx.beginPath();
        if (key) {
          ctx.fillStyle = `rgb(${r},${g},${b})`;
          ctx.arc(cx, cy, LED, 0, Math.PI * 2);
          ctx.fill();
        } else {
          ctx.save();
          ctx.filter = "blur(1.4px)";
          ctx.fillStyle = "#0c0c0e";
          ctx.arc(cx, cy, LED, 0, Math.PI * 2);
          ctx.fill();
          ctx.restore();
        }
        }
        if (k === cursorAt) {
          ctx.beginPath();
          ctx.lineWidth = 1.6;
          ctx.strokeStyle = "#fff";
          ctx.arc(cx, cy, (photo ? FLAT : LED) + 1.4, 0, Math.PI * 2);
          ctx.stroke();
        }
      }
    }
    paintOverlay();
  }

  function layout(next) {
    const n = next <= 16 ? 16 : 32;
    if (n === cols) return;
    cols = n;
    pix = new Uint8ClampedArray(cols * ROWS * 4);
    shown = new Int32Array(cols * ROWS).fill(-1);
    cursorAt = -1;
    canvas.width = cols * PITCH;
    canvas.height = ROWS * PITCH;
    clear();
  }

  return {
    clear, light, pixel, drawChar, drawText, overlayText, drawNumber,
    drawIndicator, drawLargeCustom, setRing, setCursor, present, rings, setBrightness, setPhoto, setLook, layout,
    cols: () => cols,
  };
}
