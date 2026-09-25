import { COLS, ROWS } from "./const.js";
import { ALPHABET, textPixelWidth } from "./font.js";

const PITCH = 22;
const LED = 7.5;
const WIDE_STARTS = [4, 11, 19, 26];

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
  const pix = new Uint8ClampedArray(COLS * ROWS * 4);
  const rings = [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]];
  canvas.width = COLS * PITCH;
  canvas.height = ROWS * PITCH;

  function clear() {
    pix.fill(0);
    for (let i = 3; i < pix.length; i += 4) pix[i] = 255;
    for (let i = 0; i < 4; i++) rings[i] = [0, 0, 0];
  }

  function light(x, y, rgb) {
    if (x < 1 || y < 1 || x > COLS || y > ROWS) return;
    const i = ((y - 1) * COLS + (x - 1)) * 4;
    pix[i] = rgb[0];
    pix[i + 1] = rgb[1];
    pix[i + 2] = rgb[2];
    pix[i + 3] = 255;
  }

  function pixel(x, y) {
    const i = ((y - 1) * COLS + (x - 1)) * 4;
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

  function drawNumber(count, color, topY) {
    const buffer = String(Math.round(count));
    let startX = COLS + 1 - textPixelWidth(buffer);
    if (startX < 1) startX = 1;
    drawText(buffer, startX, topY, color);
  }

  function indicatorXs(encoderNum) {
    const slot = WIDE_STARTS[encoderNum - 1] || 1;
    return [slot, slot + 1, slot + 2, Math.min(COLS, slot + 3)];
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

  const shown = new Int32Array(COLS * ROWS).fill(-1);
  const gamma = new Uint8Array(256);
  let brightness = 64;
  let gammaFor = -1;
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  function setBrightness(value) { brightness = value; }

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
    for (let y = 1; y <= ROWS; y++) {
      const cy = (ROWS - y) * PITCH + PITCH / 2;
      for (let x = 1; x <= COLS; x++) {
        const k = (y - 1) * COLS + (x - 1);
        const i = k * 4;
        const key = (pix[i] << 16) | (pix[i + 1] << 8) | pix[i + 2];
        if (shown[k] === key) continue;
        shown[k] = key;
        const cx = (x - 1) * PITCH + PITCH / 2;
        ctx.fillStyle = "#000";
        ctx.fillRect(cx - PITCH / 2, cy - PITCH / 2, PITCH, PITCH);
        ctx.beginPath();
        ctx.fillStyle = key ? `rgb(${gamma[pix[i]]},${gamma[pix[i + 1]]},${gamma[pix[i + 2]]})` : "#070707";
        ctx.arc(cx, cy, LED, 0, Math.PI * 2);
        ctx.fill();
      }
    }
  }

  return {
    clear, light, pixel, drawChar, drawText, drawNumber,
    drawIndicator, drawLargeCustom, setRing, present, rings, setBrightness,
  };
}
