/** Feature helpers */

import { uid } from "./model.js";

export const TEXT_FONT_FAMILY = '"Bebas Neue", "BebasNeue", Impact, sans-serif';
export const TEXT_FONT_WEIGHT = "700";

/**
 * Corner radius in mm from percent of the shorter side.
 * Same scale as CSS border-radius %: 50% = fully pill (half of min(w,h)).
 */
export function roundRectRadiusMm(w, h, rPercent) {
  const pct = Math.max(0, Math.min(100, Number(rPercent) || 0));
  const maxR = 0.5 * Math.min(w, h);
  return Math.min(maxR, (pct / 100) * Math.min(w, h));
}

export function createCircleFeature(panelId, cx, cy, r) {
  return {
    id: uid("feat"),
    type: "circle",
    panelId,
    cx,
    cy,
    r: Math.max(0.1, r),
  };
}

export function createRoundRectFeature(panelId, x, y, w, h, rPercent) {
  return {
    id: uid("feat"),
    type: "roundRect",
    panelId,
    x,
    y,
    w: Math.max(0.1, w),
    h: Math.max(0.1, h),
    r: Math.max(0, Math.min(100, rPercent)), // percent of shorter side
  };
}

/**
 * Engrave/cut text. Anchor (x,y) is the visual center. Font is Bebas Neue bold.
 * @param {number} letterSpacing extra space between glyphs in mm
 * @param {number} rotation degrees CW
 */
export function createTextFeature(panelId, x, y, opts = {}) {
  return {
    id: uid("feat"),
    type: "text",
    panelId,
    x: Number(x) || 0,
    y: Number(y) || 0,
    text: String(opts.text ?? "TEXT").slice(0, 120),
    size: Math.max(0.5, Number(opts.size) || 8),
    letterSpacing: Number.isFinite(Number(opts.letterSpacing))
      ? Number(opts.letterSpacing)
      : 0,
    rotation: Number.isFinite(Number(opts.rotation)) ? Number(opts.rotation) : 0,
  };
}

/** Local (unrotated) text box size in mm — Bebas Neue is condensed. */
export function estimateTextSizeMm(text, size, letterSpacing = 0) {
  const t = String(text || "");
  const n = Math.max(1, t.length);
  const sizeMm = Math.max(0.5, Number(size) || 8);
  const spacing = Number(letterSpacing) || 0;
  // Empirical average advance for Bebas Neue caps (~0.5em)
  const w = n * sizeMm * 0.5 + Math.max(0, n - 1) * spacing;
  const h = sizeMm * 1.05;
  return { w: Math.max(sizeMm * 0.3, w), h };
}

function textLocalCorners(feature) {
  const { w, h } = estimateTextSizeMm(feature.text, feature.size, feature.letterSpacing);
  const hw = w / 2;
  const hh = h / 2;
  return [
    { x: -hw, y: -hh },
    { x: hw, y: -hh },
    { x: hw, y: hh },
    { x: -hw, y: hh },
  ];
}

function rotateLocalPoint(p, deg) {
  const rad = ((Number(deg) || 0) * Math.PI) / 180;
  const c = Math.cos(rad);
  const s = Math.sin(rad);
  return { x: p.x * c - p.y * s, y: p.x * s + p.y * c };
}

/** World-space corners of text box, rotated around center. */
export function textWorldCorners(feature) {
  return textLocalCorners(feature).map((p) => {
    const r = rotateLocalPoint(p, feature.rotation);
    return { x: feature.x + r.x, y: feature.y + r.y };
  });
}

export function hitTestFeature(feature, px, py, tol = 2) {
  if (feature.type === "circle") {
    const d = Math.hypot(px - feature.cx, py - feature.cy);
    return Math.abs(d - feature.r) <= tol || d <= feature.r;
  }
  if (feature.type === "roundRect") {
    return (
      px >= feature.x - tol &&
      px <= feature.x + feature.w + tol &&
      py >= feature.y - tol &&
      py <= feature.y + feature.h + tol
    );
  }
  if (feature.type === "text") {
    // Inverse-rotate point into text local frame, then AABB test
    const rad = (-(Number(feature.rotation) || 0) * Math.PI) / 180;
    const c = Math.cos(rad);
    const s = Math.sin(rad);
    const dx = px - feature.x;
    const dy = py - feature.y;
    const lx = dx * c - dy * s;
    const ly = dx * s + dy * c;
    const { w, h } = estimateTextSizeMm(feature.text, feature.size, feature.letterSpacing);
    const pad = tol;
    return (
      lx >= -w / 2 - pad &&
      lx <= w / 2 + pad &&
      ly >= -h / 2 - pad &&
      ly <= h / 2 + pad
    );
  }
  return false;
}

/** Axis-aligned bounds in panel mm. */
export function featureBounds(feature) {
  if (!feature) return null;
  if (feature.type === "circle") {
    return {
      x: feature.cx - feature.r,
      y: feature.cy - feature.r,
      w: feature.r * 2,
      h: feature.r * 2,
    };
  }
  if (feature.type === "roundRect") {
    return { x: feature.x, y: feature.y, w: feature.w, h: feature.h };
  }
  if (feature.type === "text") {
    const corners = textWorldCorners(feature);
    const xs = corners.map((p) => p.x);
    const ys = corners.map((p) => p.y);
    const minX = Math.min(...xs);
    const minY = Math.min(...ys);
    return {
      x: minX,
      y: minY,
      w: Math.max(...xs) - minX,
      h: Math.max(...ys) - minY,
    };
  }
  return null;
}

export function boundsIntersect(a, b) {
  if (!a || !b) return false;
  return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

/** Snap `v` to a grid whose origin is `origin` (not world 0). */
export function snapRelative(v, origin, enabled, grid) {
  if (!enabled || !grid || grid <= 0) return v;
  const o = Number(origin) || 0;
  return o + Math.round((v - o) / grid) * grid;
}

/** Center point used for measure / spacing. */
export function featureCenter(feature) {
  if (!feature) return null;
  if (feature.type === "circle") return { x: feature.cx, y: feature.cy };
  if (feature.type === "roundRect") {
    return { x: feature.x + feature.w / 2, y: feature.y + feature.h / 2 };
  }
  if (feature.type === "text") return { x: feature.x, y: feature.y };
  return null;
}

/** Translate a feature so its center moves by (dx, dy). */
export function moveFeatureByDelta(feature, dx, dy) {
  if (!feature) return;
  if (feature.type === "circle") {
    feature.cx += dx;
    feature.cy += dy;
  } else if (feature.type === "roundRect" || feature.type === "text") {
    feature.x += dx;
    feature.y += dy;
  }
}

/**
 * Move feature B so the distance from fixed point A to B's center becomes `distance`.
 * @param {"along"|"horizontal"|"vertical"} axis
 * @returns {{ ok: boolean, error?: string }}
 */
export function setFeatureDistanceFromPoint(featureB, pointA, distance, axis = "along") {
  if (!featureB || !pointA) return { ok: false, error: "Missing feature or anchor" };
  const dist = Math.max(0, Number(distance));
  if (!Number.isFinite(dist)) return { ok: false, error: "Invalid distance" };

  const b0 = featureCenter(featureB);
  if (!b0) return { ok: false, error: "Unknown feature type" };

  let nx = b0.x;
  let ny = b0.y;
  const dx = b0.x - pointA.x;
  const dy = b0.y - pointA.y;

  if (axis === "horizontal") {
    const sign = dx === 0 ? (dy >= 0 ? 1 : -1) : Math.sign(dx) || 1;
    nx = pointA.x + sign * dist;
    ny = b0.y;
  } else if (axis === "vertical") {
    const sign = dy === 0 ? (dx >= 0 ? 1 : -1) : Math.sign(dy) || 1;
    nx = b0.x;
    ny = pointA.y + sign * dist;
  } else {
    const cur = Math.hypot(dx, dy);
    if (cur < 1e-9) {
      nx = pointA.x + dist;
      ny = pointA.y;
    } else {
      const s = dist / cur;
      nx = pointA.x + dx * s;
      ny = pointA.y + dy * s;
    }
  }

  moveFeatureByDelta(featureB, nx - b0.x, ny - b0.y);
  return { ok: true };
}
