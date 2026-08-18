/** Panel transforms for nest/export (outer-face-up, in-plane rotation). */

import { pointsToPathD, circlePathD, roundRectPathD, ensureCCW } from "./joints.js";
import { panelBounds } from "./unfold.js";

/**
 * Mirror panel in X about its bounding-box center. Keeps winding CCW.
 * Used so End B / walls show their outer face when looking down at the sheet.
 */
export function mirrorPanelX(panel) {
  const b = panelBounds(panel);
  const flipX = (x) => b.minX + b.maxX - x;
  const outerPoints = ensureCCW(
    (panel.outerPoints || []).map((p) => ({ x: flipX(p.x), y: p.y }))
  );
  return {
    ...panel,
    outerPoints,
    outerPath: pointsToPathD(outerPoints),
    holes: (panel.holes || []).map((h) => mirrorHoleX(h, flipX)),
    nestMirrorX: true,
  };
}

/**
 * Rotate panel 90° CW in-plane (outer face stays up). Rebases to positive bbox.
 */
export function rotatePanel90CW(panel) {
  const b = panelBounds(panel);
  // (x,y) → (b.minY + b.maxY - y? no): local (x,y) → (y - minY, maxX - x) then shift
  const map = (x, y) => ({
    x: y - b.minY,
    y: b.maxX - x,
  });
  const outerPoints = ensureCCW(
    (panel.outerPoints || []).map((p) => map(p.x, p.y))
  );
  const nb = boundsOfPts(outerPoints);
  const shift = (p) => ({ x: p.x - nb.minX, y: p.y - nb.minY });
  const shifted = outerPoints.map(shift);
  const sb = boundsOfPts(shifted);
  return {
    ...panel,
    width: sb.width,
    height: sb.height,
    outerPoints: shifted,
    outerPath: pointsToPathD(shifted),
    holes: (panel.holes || []).map((h) => rotateHole90CW(h, map, nb)),
    nestRotate90: true,
  };
}

/**
 * Nest/export orientation: outer face toward viewer (sheet up).
 * End A matches the drawn profile (already outer-up).
 * End B is mirrored (opposite end of the case).
 * Walls are mirrored so looking down matches the exterior view.
 */
export function orientPanelsOuterUp(panels) {
  return (panels || []).map((panel) => {
    if (panel.kind === "wall") return mirrorPanelX(panel);
    if (panel.kind === "end" && panel.id === "endB") return mirrorPanelX(panel);
    return panel;
  });
}

function mirrorHoleX(h, flipX) {
  if (h.type === "circle") {
    const cx = flipX(h.cx);
    return { ...h, cx, d: circlePathD(cx, h.cy, h.r) };
  }
  if (h.type === "roundRect") {
    const x = flipX(h.x + h.w);
    return {
      ...h,
      x,
      d: roundRectPathD(x, h.y, h.w, h.h, h.r),
    };
  }
  if (h.type === "text") {
    return {
      ...h,
      x: flipX(h.x),
      rotation: -(Number(h.rotation) || 0),
    };
  }
  // Path-only hole: skip geometric fields
  return h;
}

function rotateHole90CW(h, map, nb) {
  if (h.type === "circle") {
    const p = map(h.cx, h.cy);
    const cx = p.x - nb.minX;
    const cy = p.y - nb.minY;
    return { ...h, cx, cy, d: circlePathD(cx, cy, h.r) };
  }
  if (h.type === "roundRect") {
    const c = [
      map(h.x, h.y),
      map(h.x + h.w, h.y),
      map(h.x + h.w, h.y + h.h),
      map(h.x, h.y + h.h),
    ];
    const xs = c.map((p) => p.x - nb.minX);
    const ys = c.map((p) => p.y - nb.minY);
    const x = Math.min(...xs);
    const y = Math.min(...ys);
    const w = Math.max(...xs) - x;
    const hgt = Math.max(...ys) - y;
    // r is stored in mm for nest holes
    const r = Math.min(h.r, w / 2, hgt / 2);
    return { ...h, x, y, w, h: hgt, r, d: roundRectPathD(x, y, w, hgt, r) };
  }
  if (h.type === "text") {
    const p = map(h.x, h.y);
    return {
      ...h,
      x: p.x - nb.minX,
      y: p.y - nb.minY,
      rotation: (Number(h.rotation) || 0) + 90,
    };
  }
  return h;
}

function boundsOfPts(pts) {
  let minX = Infinity;
  let minY = Infinity;
  let maxX = -Infinity;
  let maxY = -Infinity;
  for (const p of pts) {
    minX = Math.min(minX, p.x);
    minY = Math.min(minY, p.y);
    maxX = Math.max(maxX, p.x);
    maxY = Math.max(maxY, p.y);
  }
  return { minX, minY, maxX, maxY, width: maxX - minX, height: maxY - minY };
}
