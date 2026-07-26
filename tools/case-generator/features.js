/** Feature helpers */

import { uid } from "./model.js";

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
  return false;
}
