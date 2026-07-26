/** Profile → panels with finger-jointed outer paths */

import {
  dist,
  add,
  sub,
  scale,
  normalize,
  outwardNormal,
  ensureCCW,
  fingeredPolygon,
  fingeredRect,
  pointsToPathD,
  circlePathD,
  roundRectPathD,
  offsetPolygon,
  simplifyPolyline,
  polygonSelfIntersects,
  isAxisAligned,
  isConvexOrthogonalCorner,
  isValidFingerLength,
  MIN_FINGER_LENGTH,
} from "./joints.js";
import { panelLabel, jointStartMale, jointInvertMode } from "./model.js";
import { roundRectRadiusMm } from "./features.js";

/**
 * @returns {{ panels: Panel[], error?: string }}
 * Panel: { id, label, kind, width, height, outerPath, holes: [{d}], localBounds }
 */
export function unfold(doc) {
  if (doc.fixedPanels?.length) {
    return unfoldFixed(doc);
  }

  const rawPts = doc.profile.points;
  if (!doc.profile.closed || rawPts.length < 3) {
    return { panels: [], error: "Close a profile with at least 3 vertices." };
  }

  const simplified = simplifyPolyline(rawPts, { closed: true });
  if (simplified.length < 3) {
    return { panels: [], error: "Profile needs at least 3 distinct corners after cleanup." };
  }
  if (polygonSelfIntersects(simplified)) {
    return { panels: [], error: "Profile edges cross — fix the outline before generating." };
  }

  const thickness = Math.max(0.05, doc.thickness);
  const fingerLength = Math.max(10, doc.fingerLength);
  const minFinger = Math.max(0.5, doc.minFingerWidth ?? MIN_FINGER_LENGTH);
  const kerf = doc.kerf;
  const jointDepth = Math.max(0, thickness - kerf * 0.5);
  const depth = Math.max(thickness * 2, doc.depth);

  // Drawn profile is the finished outside envelope. Finger baselines sit on an
  // inset body so male tabs restore (rather than exceed) that envelope, and
  // female gaps never cut inward as “negative” notches.
  const outerProfile = ensureCCW(simplified);
  const bodyProfile = offsetPolygon(outerProfile, -jointDepth);
  if (bodyProfile.length < 3 || polygonSelfIntersects(bodyProfile)) {
    return {
      panels: [],
      error: "Profile is too small for the material thickness — enlarge it or reduce thickness.",
    };
  }
  const bodyArea = (() => {
    let a = 0;
    for (let i = 0; i < bodyProfile.length; i++) {
      const p = bodyProfile[i];
      const q = bodyProfile[(i + 1) % bodyProfile.length];
      a += p.x * q.y - q.x * p.y;
    }
    return a / 2;
  })();
  if (!(bodyArea > 1)) {
    return {
      panels: [],
      error: "Profile is too small for the material thickness — enlarge it or reduce thickness.",
    };
  }

  const profile = bodyProfile;
  const n = profile.length;

  // Concave corners use a square-cut butt trim on the outgoing wall. Convex
  // corners retain their full run and finger-joint eligibility.
  const wallStartTrims = profile.map((a, i) => {
    const prev = profile[(i - 1 + n) % n];
    const b = profile[(i + 1) % n];
    const u = normalize(sub(a, prev));
    const v = normalize(sub(b, a));
    const prevOut = outwardNormal(u);
    const nextOut = outwardNormal(v);
    const advance = prevOut.x * v.x + prevOut.y * v.y;
    if (advance <= 1e-6) return 0;

    const normalDot = prevOut.x * nextOut.x + prevOut.y * nextOut.y;
    const required = thickness * (1 - Math.min(0, normalDot)) / advance;
    return Math.min(required, dist(a, b) * 0.8);
  });

  /**
   * Per-edge joint. Fingers only when every finger exceeds minFingerWidth; else plain.
   * Diagonals use end clearance + a shorter pitch when needed.
   * opts.panelId / opts.mateId drive global + per-panel invert for this joint.
   */
  const edgeJoint = (startMaleBase, a, b, opts = {}) => {
    const panelId = opts.panelId;
    const mateId = opts.mateId;
    const startMale =
      panelId && mateId
        ? jointStartMale(doc, startMaleBase, panelId, mateId)
        : startMaleBase;
    const invertJoint =
      panelId && mateId ? jointInvertMode(doc, panelId, mateId) : !!doc.invertFingers;
    const base = {
      thickness,
      fingerLength,
      startMale,
      kerf,
      minFingerLength: minFinger,
    };
    const extraStart = Math.max(0, opts.startClearance || 0);
    const extraEnd = Math.max(0, opts.finishClearance || 0);
    if (isAxisAligned(a, b)) {
      const L = dist(a, b);
      const usable = L - extraStart - extraEnd;
      return isValidFingerLength(usable, minFinger)
        ? {
            ...base,
            startClearance: extraStart,
            finishClearance: extraEnd,
          }
        : null;
    }

    const L = dist(a, b);
    // In inverted mode the wall owns the outer male runs and the end cap owns
    // the alternating runs. Let that complementary pair cover the whole
    // diagonal edge; plain corner clearance would leave visible holes.
    const clear = invertJoint
      ? 0
      : Math.max(thickness, Math.min(L * 0.2, thickness * 1.25));
    const clearStart = Math.min(L * 0.9, clear + extraStart);
    const clearEnd = Math.min(
      Math.max(0, L * 0.9 - clearStart),
      clear + extraEnd
    );
    const usable = Math.max(0, L - clearStart - clearEnd);
    if (!isValidFingerLength(usable, minFinger)) return null;

    const f = Math.min(
      fingerLength,
      Math.max(minFinger + 1e-6, usable / 5)
    );
    if (!isValidFingerLength(f, minFinger)) return null;

    return {
      ...base,
      fingerLength: f,
      startClearance: clearStart,
      finishClearance: clearEnd,
    };
  };

  // --- End caps — female on active runs by default (invertFingers off restores male) ---
  // Bounds from the outer envelope so nest size matches the drawn profile.
  // End A/B share one blank; invert uses endA (synced with endB in the doc).
  const endBounds = boundsOf(outerProfile);
  const endEdgeSpecs = [];
  for (let i = 0; i < n; i++) {
    const a = profile[i];
    const b = profile[(i + 1) % n];
    endEdgeSpecs.push(
      edgeJoint(true, a, b, {
        startClearance: wallStartTrims[i],
        panelId: "endA",
        mateId: `wall-${i}`,
      })
    );
  }
  const endPath = fingeredPolygon(profile, endEdgeSpecs);
  const endLocal = shiftToOrigin(endPath, endBounds);

  const panels = [];

  panels.push({
    id: "endA",
    label: panelLabel("endA"),
    kind: "end",
    width: endBounds.width,
    height: endBounds.height,
    outerPoints: endLocal.map((p) => ({ ...p })),
    outerPath: pointsToPathD(endLocal),
    holes: [],
    originShift: { x: endBounds.minX, y: endBounds.minY },
    nominal: { type: "polygon", points: shiftToOrigin(outerProfile, endBounds) },
  });

  panels.push({
    id: "endB",
    label: panelLabel("endB"),
    kind: "end",
    width: endBounds.width,
    height: endBounds.height,
    // Same outline as End A (symmetric / interchangeable)
    outerPoints: endLocal.map((p) => ({ ...p })),
    outerPath: pointsToPathD(endLocal),
    holes: [],
    originShift: { x: endBounds.minX, y: endBounds.minY },
    nominal: { type: "polygon", points: shiftToOrigin(outerProfile, endBounds) },
  });

  // --- Walls ---
  for (let i = 0; i < n; i++) {
    const nominalA = profile[i];
    const b = profile[(i + 1) % n];
    const fullEdgeLen = dist(nominalA, b);
    const edgeDir = normalize(sub(b, nominalA));
    const startTrim = Math.min(
      wallStartTrims[i],
      Math.max(0, fullEdgeLen - 0.05)
    );
    const a = add(nominalA, scale(edgeDir, startTrim));
    const edgeLen = dist(a, b);
    // Wall body matches the inset profile edge. Side tabs at ortho corners
    // restore the outer envelope — never pad short walls up to finger pitch
    // (that stacks with side tabs and doubles the blank).
    const w = edgeLen;
    const h = depth;

    // Edges: bottom (→ endB), right (→ next wall), top (→ endA), left (→ prev wall)
    // Default (invert): ends female ⇒ wall long edges male. Off swaps that.
    // Wall top is walked b→a in the CCW rect — keep the same startMale as bottom.
    const wallId = `wall-${i}`;
    const rightMate = `wall-${(i + 1) % n}`;
    const leftMate = `wall-${(i - 1 + n) % n}`;
    const bottom = edgeJoint(false, a, b, { panelId: wallId, mateId: "endB" });
    const top = edgeJoint(false, a, b, { panelId: wallId, mateId: "endA" });
    // Side edges: right walked bottom→top, left walked top→bottom (CCW).
    // Finger sides only at convex ~90° corners — diagonal / concave corners
    // cannot interlock in-plane without piercing the neighbor face.
    const prev = profile[(i - 1 + n) % n];
    const next = profile[(i + 2) % n];
    const leftOk = isConvexOrthogonalCorner(prev, a, b);
    const rightOk = isConvexOrthogonalCorner(a, b, next);
    const sideBase = i % 2 === 0;
    const rightOrtho = rightOk
      ? {
          thickness,
          fingerLength,
          startMale: jointStartMale(doc, sideBase, wallId, rightMate),
          kerf,
          minFingerLength: minFinger,
        }
      : null;
    const leftOrtho = leftOk
      ? {
          thickness,
          fingerLength,
          startMale: jointStartMale(doc, sideBase, wallId, leftMate),
          kerf,
          minFingerLength: minFinger,
        }
      : null;

    const edgeSpecs = [bottom, rightOrtho, top, leftOrtho];
    const outerPoints = fingeredRect(w, h, edgeSpecs);

    panels.push({
      id: `wall-${i}`,
      label: panelLabel(`wall-${i}`),
      kind: "wall",
      width: w,
      height: h,
      edgeIndex: i,
      edgeLength: edgeLen,
      startTrim,
      profileEdgeA: { x: nominalA.x, y: nominalA.y },
      // Body-edge this wall sits on (for 3D assembly)
      edgeA: { x: a.x, y: a.y },
      edgeB: { x: b.x, y: b.y },
      outerPoints,
      outerPath: pointsToPathD(outerPoints),
      holes: [],
      originShift: { x: 0, y: 0 },
      nominal: { type: "rect", w, h },
    });
  }

  // Attach features as hole paths in panel-local coordinates
  applyFeatures(panels, doc.features || []);

  return { panels };
}

function boundsOf(points) {
  let minX = Infinity;
  let minY = Infinity;
  let maxX = -Infinity;
  let maxY = -Infinity;
  for (const p of points) {
    minX = Math.min(minX, p.x);
    minY = Math.min(minY, p.y);
    maxX = Math.max(maxX, p.x);
    maxY = Math.max(maxY, p.y);
  }
  return {
    minX,
    minY,
    maxX,
    maxY,
    width: maxX - minX,
    height: maxY - minY,
  };
}

function shiftToOrigin(points, bounds) {
  return points.map((p) => ({
    x: p.x - bounds.minX,
    y: p.y - bounds.minY,
  }));
}

function applyFeatures(panels, features) {
  const byId = new Map(panels.map((p) => [p.id, p]));
  for (const f of features) {
    const panel = byId.get(f.panelId);
    if (!panel) continue;

    // Features are authored in nominal panel space (origin at panel local 0,0
    // after end-cap shift). For ends, coords are relative to shifted profile.
    if (f.type === "circle") {
      panel.holes.push({
        id: f.id,
        type: "circle",
        d: circlePathD(f.cx, f.cy, f.r),
        cx: f.cx,
        cy: f.cy,
        r: f.r,
      });
    } else if (f.type === "roundRect") {
      const rMm = roundRectRadiusMm(f.w, f.h, f.r);
      panel.holes.push({
        id: f.id,
        type: "roundRect",
        d: roundRectPathD(f.x, f.y, f.w, f.h, rMm),
        x: f.x,
        y: f.y,
        w: f.w,
        h: f.h,
        r: rMm,
        rPercent: f.r,
      });
    }
  }
}

/**
 * Materialize baked DXF panels. Source has no kerf; apply signed kerf here.
 * Convention: outward_offset = -kerf (kerf -0.125 → expand outer slightly for press-fit).
 */
function unfoldFixed(doc) {
  const kerf = doc.kerf || 0;
  const delta = -kerf; // expand when kerf negative
  const panels = [];

  for (const src of doc.fixedPanels) {
    const outerBase = ensureCCW((src.outer || src.outerPoints || []).map((p) => ({ x: p.x, y: p.y })));
    const outerPoints = offsetPolygon(outerBase, delta);
    const holes = [];

    for (const h of src.holes || []) {
      if (h.type === "circle") {
        const r = Math.max(0.05, h.r - delta);
        holes.push({
          type: "circle",
          d: circlePathD(h.cx, h.cy, r),
          cx: h.cx,
          cy: h.cy,
          r,
        });
      } else if (h.type === "path" && Array.isArray(h.points)) {
        const pts = offsetPolygon(ensureCCW(h.points), -delta); // holes shrink when outer expands
        holes.push({
          type: "path",
          d: pointsToPathD(pts),
          points: pts,
        });
      }
    }

    panels.push({
      id: src.id,
      label: src.label || panelLabel(src.id, doc),
      kind: "fixed",
      width: src.width,
      height: src.height,
      outerPoints,
      outerPath: pointsToPathD(outerPoints),
      holes,
      originShift: { x: 0, y: 0 },
      nominal: { type: "rect", w: src.width, h: src.height },
    });
  }

  applyFeatures(panels, doc.features || []);
  return { panels };
}

export function panelBounds(panel) {
  const pts = panel.outerPoints || [];
  if (!pts.length) {
    return { minX: 0, minY: 0, maxX: panel.width || 0, maxY: panel.height || 0, width: panel.width || 0, height: panel.height || 0 };
  }
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
  return {
    minX,
    minY,
    maxX,
    maxY,
    width: maxX - minX,
    height: maxY - minY,
  };
}
