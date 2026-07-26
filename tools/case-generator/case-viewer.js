/**
 * Case Viewer — realistic assembled laser-cut case.
 * Exterior-only panels (no cavity faces), non-overlapping joint ownership,
 * priority z-buffer → fully opaque sides without z-fighting.
 */

import {
  dist,
  normalize,
  sub,
  outwardNormal,
  ensureCCW,
} from "./joints.js";
import { unfold } from "./unfold.js";

/** Neutral matte grey (faces slightly lighter than edge grain). */
const MATTE = {
  faceA: [178, 182, 188],
  faceB: [168, 172, 178],
  faceEnd: [188, 192, 198],
  edge: [108, 112, 118],
};

/** Push outer faces out slightly so mating panels never share a plane. */
const FACE_EPS = 0.08;

/**
 * @returns {{ kind: string, note?: string, faces, edges, panels, bounds } | { error: string }}
 */
export function buildAssembledCase(doc) {
  const { panels, error } = unfold(doc);
  if (error || !panels.length) {
    return { error: error || "Close a profile to view the assembled case." };
  }

  const t = Math.max(0.05, doc.thickness || 3);

  if (doc.fixedPanels?.length) {
    return meshM1WoodBox(doc, t);
  }

  return meshFingeredAssembly(panels, t, doc);
}

/**
 * Real fingered panels posed as the finished case (exterior shell).
 * Placement matches nest mating: panel-local coords map so every male tip
 * lands in its counterpart female gap (ends ↔ wall long edges, wall ↔ wall).
 */
function meshFingeredAssembly(panels, t, doc) {
  const depth = Math.max(t * 2, doc.depth || 1);
  const faces = [];
  const edges = [];
  const infos = [];
  // Profile is Y-down (canvas); 3D is Y-up so the case matches the drawing.
  const yUp = (y) => -y;

  let panelOrd = 0;
  for (const panel of panels) {
    infos.push({ id: panel.id, label: panel.label, kind: panel.kind });
    const prio = 100 + panelOrd * 3;
    panelOrd++;

    if (panel.kind === "end") {
      const shift = panel.originShift || { x: 0, y: 0 };
      // Wall long-edge males reach z≈−t (End B) and z≈depth+t (End A).
      // Flush each end's outer face with those tips; thickness goes into the cavity
      // so the painted face normals point outward (±Z) and stay camera-visible.
      const endA = panel.id === "endA";
      addWoodPanel(faces, edges, panel.outerPoints, (p, s) => ({
        x: p.x + shift.x,
        y: yUp(p.y + shift.y),
        // End B: outer at −t (s=0), inner at 0. End A: outer at depth+t (s=0), inner at depth.
        z: endA ? depth + t - s : -t + s,
      }), t, MATTE.faceEnd, panel.id, {
        outerAt: t,
        priority: prio,
        holes: panel.holes,
      });
    } else if (panel.kind === "wall" && panel.edgeA && panel.edgeB) {
      const a = panel.edgeA;
      const b = panel.edgeB;
      const u = normalize(sub(b, a));
      const out2 = outwardNormal(u);
      const face = (panel.edgeIndex || 0) % 2 === 0 ? MATTE.faceA : MATTE.faceB;
      // Full fingered outline: long-edge males interlock with the ends;
      // side males only exist at ~90° corners (see unfold).
      const edgeLen = dist(a, b);
      const w = panel.width || edgeLen;
      // If the wall was padded wider than the profile edge, center the run so
      // fingers stay registered to the nest mating along a→b.
      const along0 = (w - edgeLen) * 0.5;
      addWoodPanel(faces, edges, panel.outerPoints, (p, s) => {
        const along = p.x - along0;
        return {
          x: a.x + u.x * along + out2.x * s,
          y: yUp(a.y + u.y * along + out2.y * s),
          z: p.y,
        };
      }, t, face, panel.id, {
        outerAt: t, // s=t is outside the body baseline
        priority: prio,
        holes: panel.holes,
      });
    }
  }

  if (!faces.length) {
    return { error: "No panels to assemble." };
  }

  return {
    kind: "assembled",
    note: `Assembled case · finger joints · t ${fmt(t)} mm · depth ${fmt(depth)} mm`,
    faces,
    edges: dedupeEdges(edges),
    panels: infos,
    bounds: boundsOfPoints(faces.flatMap((f) => f.pts)),
  };
}

/** M1 approximate box — non-overlapping panel volumes. */
function meshM1WoodBox(doc, t) {
  const panels = doc.fixedPanels;
  const top = panels.find((p) => /top/i.test(p.label)) || panels[0];
  const left = panels.find((p) => /left/i.test(p.label));
  const front = panels.find((p) => /front/i.test(p.label));
  const w = top?.width || 100;
  const d = top?.height || 60;
  const h = left?.width || front?.height || 40;

  const faces = [];
  const edges = [];
  const infos = [
    { id: "bottom", label: "Bottom", kind: "end" },
    { id: "top", label: "Top", kind: "end" },
    { id: "front", label: "Front", kind: "wall" },
    { id: "back", label: "Back", kind: "wall" },
    { id: "left", label: "Left", kind: "wall" },
    { id: "right", label: "Right", kind: "wall" },
  ];

  const rect = (ww, hh) => [
    { x: 0, y: 0 },
    { x: ww, y: 0 },
    { x: ww, y: hh },
    { x: 0, y: hh },
  ];

  // Bottom / top own the horizontal slabs; walls sit between them (no overlap).
  addWoodPanel(faces, edges, rect(w, d), (p, s) => ({ x: p.x, y: p.y, z: s }), t, MATTE.faceB, "bottom", {
    outerAt: 0,
    priority: 100,
  });
  addWoodPanel(faces, edges, rect(w, d), (p, s) => ({ x: p.x, y: p.y, z: h - s }), t, MATTE.faceEnd, "top", {
    outerAt: 0,
    priority: 103,
  });

  const zh = Math.max(0.1, h - 2 * t);
  addWoodPanel(faces, edges, rect(w, zh), (p, s) => ({ x: p.x, y: s, z: t + p.y }), t, MATTE.faceA, "front", {
    outerAt: 0,
    priority: 106,
  });
  addWoodPanel(faces, edges, rect(w, zh), (p, s) => ({ x: p.x, y: d - s, z: t + p.y }), t, MATTE.faceB, "back", {
    outerAt: 0,
    priority: 109,
  });
  addWoodPanel(faces, edges, rect(d, zh), (p, s) => ({ x: s, y: p.x, z: t + p.y }), t, MATTE.faceA, "left", {
    outerAt: 0,
    priority: 112,
  });
  addWoodPanel(faces, edges, rect(d, zh), (p, s) => ({ x: w - s, y: p.x, z: t + p.y }), t, MATTE.faceB, "right", {
    outerAt: 0,
    priority: 115,
  });

  return {
    kind: "assembled",
    note: `TŒRN M1 · approx. ${fmt(w)}×${fmt(d)}×${fmt(h)} mm`,
    faces,
    edges: dedupeEdges(edges),
    panels: infos,
    bounds: boundsOfPoints(faces.flatMap((f) => f.pts)),
  };
}

/**
 * Extrude cut outline into a plate — outer + cavity faces + rim.
 * Both faces get cut outlines when they face the camera (needed so diagonal
 * walls read clearly when looking into the case).
 * Optional `opts.holes` punch openings through the plate (caps + bore walls).
 */
function addWoodPanel(faces, edges, ring2d, map, thickness, faceRgb, panelId, opts = {}) {
  if (!ring2d?.length || thickness <= 0) return;
  const outerAt = opts.outerAt ?? 0;
  const priority = opts.priority ?? 100;
  const ring = ensureCCW(ring2d);
  const innerS = outerAt === 0 ? thickness : 0;
  const outerS = outerAt;

  let outer = ring.map((p) => map(p, outerS));
  let inner = ring.map((p) => map(p, innerS));

  // Outward normal from outer face (ring is CCW in param space; map may flip)
  const n = polygonNormal(outer);
  const midO = centroid(outer);
  const midI = centroid(inner);
  const toOuter = sub3(midO, midI);
  if (dot3(n, toOuter) < 0) {
    outer = [...outer].reverse();
  }
  const nOut = polygonNormal(outer);

  // Inner face winds opposite so its normal points into the cavity
  let innerFace = [...inner].reverse();
  const nIn = polygonNormal(innerFace);
  if (dot3(nIn, toOuter) > 0) {
    // still pointing outward — flip
    innerFace = [...innerFace].reverse();
  }
  const nInn = polygonNormal(innerFace);

  const pushedOut = outer.map((p) => ({
    x: p.x + nOut.x * FACE_EPS,
    y: p.y + nOut.y * FACE_EPS,
    z: p.z + nOut.z * FACE_EPS,
  }));
  const pushedIn = innerFace.map((p) => ({
    x: p.x - nOut.x * FACE_EPS,
    y: p.y - nOut.y * FACE_EPS,
    z: p.z - nOut.z * FACE_EPS,
  }));

  // Slightly darker cavity face so inside/outside read apart
  const innerRgb = faceRgb.map((c) => Math.round(c * 0.82));

  faces.push({
    pts: pushedOut,
    rgb: faceRgb,
    role: "face",
    side: "outer",
    panelId,
    priority: priority + 2,
    cutouts2d: (opts.holes || [])
      .map(holeRing2d)
      .filter((r) => r && r.length >= 3)
      .map((r) => ensureCCW(r)),
    map2d: map,
    mapS: outerS,
  });
  faces.push({
    pts: pushedIn,
    rgb: innerRgb,
    role: "face",
    side: "inner",
    panelId,
    priority: priority + 1,
    cutouts2d: (opts.holes || [])
      .map(holeRing2d)
      .filter((r) => r && r.length >= 3)
      .map((r) => ensureCCW(r)),
    map2d: map,
    mapS: innerS,
  });

  const nR = ring.length;
  const outRaw = ring.map((p) => {
    const q = map(p, outerS);
    return {
      x: q.x + nOut.x * FACE_EPS,
      y: q.y + nOut.y * FACE_EPS,
      z: q.z + nOut.z * FACE_EPS,
    };
  });
  const inRaw = ring.map((p) => map(p, innerS));

  for (let i = 0; i < nR; i++) {
    const j = (i + 1) % nR;
    if (dist(ring[i], ring[j]) < 1e-9) continue;
    const a = outRaw[i];
    const b = outRaw[j];
    const c = inRaw[j];
    const d = inRaw[i];
    let rim = [a, b, c, d];
    const rn = polygonNormal(rim);
    const rc = centroid(rim);
    const away = sub3(rc, midI);
    if (dot3(rn, away) < 0) rim = [a, d, c, b];

    faces.push({
      pts: rim,
      rgb: MATTE.edge,
      role: "edge",
      panelId,
      priority: priority,
    });
  }

  addPanelHoleBores(faces, opts.holes || [], map, outerS, innerS, nOut, panelId, priority);

  void edges;
  void nInn;
}

/** Bore walls through the plate — double-sided so the tunnel reads from either end. */
const HOLE_BORE = [72, 76, 82];

function addPanelHoleBores(faces, holes, map, outerS, innerS, nOut, panelId, priority) {
  for (const hole of holes) {
    const ring2d = holeRing2d(hole);
    if (!ring2d || ring2d.length < 3) continue;
    const ring = ensureCCW(ring2d);

    const outRing = ring.map((p) => {
      const q = map(p, outerS);
      return {
        x: q.x + nOut.x * FACE_EPS,
        y: q.y + nOut.y * FACE_EPS,
        z: q.z + nOut.z * FACE_EPS,
      };
    });
    const inRing = ring.map((p) => {
      const q = map(p, innerS);
      return {
        x: q.x - nOut.x * FACE_EPS,
        y: q.y - nOut.y * FACE_EPS,
        z: q.z - nOut.z * FACE_EPS,
      };
    });

    const holeMid = {
      x: (centroid(outRing).x + centroid(inRing).x) * 0.5,
      y: (centroid(outRing).y + centroid(inRing).y) * 0.5,
      z: (centroid(outRing).z + centroid(inRing).z) * 0.5,
    };

    const nH = ring.length;
    for (let i = 0; i < nH; i++) {
      const j = (i + 1) % nH;
      if (dist(ring[i], ring[j]) < 1e-9) continue;
      const a = outRing[i];
      const b = outRing[j];
      const c = inRing[j];
      const d = inRing[i];
      let bore = [a, b, c, d];
      // Face toward the hole axis so looking into the opening sees the wall.
      const bn = polygonNormal(bore);
      const bc = centroid(bore);
      const inward = sub3(holeMid, bc);
      if (dot3(bn, inward) < 0) bore = [a, d, c, b];

      faces.push({
        pts: bore,
        rgb: HOLE_BORE,
        role: "edge",
        panelId,
        priority: priority + 0.5,
      });
      // Opposite winding for the other viewing direction through the bore.
      faces.push({
        pts: [...bore].reverse(),
        rgb: HOLE_BORE,
        role: "edge",
        panelId,
        priority: priority + 0.4,
      });
    }
  }
}

/** Panel-local hole outline as a closed polyline. */
function holeRing2d(hole) {
  if (!hole) return null;
  if (hole.type === "circle" && hole.r > 0) {
    const n = Math.max(12, Math.min(32, Math.ceil(hole.r * 4)));
    const pts = [];
    for (let i = 0; i < n; i++) {
      const a = (i / n) * Math.PI * 2;
      pts.push({
        x: hole.cx + Math.cos(a) * hole.r,
        y: hole.cy + Math.sin(a) * hole.r,
      });
    }
    return pts;
  }
  if (hole.type === "roundRect" && hole.w > 0 && hole.h > 0) {
    return roundRectRing2d(hole.x, hole.y, hole.w, hole.h, hole.r || 0);
  }
  if (hole.type === "path" && Array.isArray(hole.points) && hole.points.length >= 3) {
    return hole.points.map((p) => ({ x: p.x, y: p.y }));
  }
  return null;
}

function roundRectRing2d(x, y, w, h, r) {
  const rr = Math.max(0, Math.min(r, w / 2, h / 2));
  if (rr < 1e-6) {
    return [
      { x, y },
      { x: x + w, y },
      { x: x + w, y: y + h },
      { x, y: y + h },
    ];
  }
  const pts = [];
  const arc = (cx, cy, a0, a1) => {
    const steps = Math.max(3, Math.ceil((Math.abs(a1 - a0) / (Math.PI / 2)) * 4));
    for (let i = 0; i <= steps; i++) {
      const t = a0 + (a1 - a0) * (i / steps);
      pts.push({ x: cx + Math.cos(t) * rr, y: cy + Math.sin(t) * rr });
    }
  };
  // CCW from top-left going right (Y-down canvas coords).
  arc(x + rr, y + rr, Math.PI, Math.PI * 1.5);
  arc(x + w - rr, y + rr, Math.PI * 1.5, Math.PI * 2);
  arc(x + w - rr, y + h - rr, 0, Math.PI * 0.5);
  arc(x + rr, y + h - rr, Math.PI * 0.5, Math.PI);
  return pts;
}

function polygonNormal(pts) {
  // Newell — stable for non-planar-ish quads
  let nx = 0;
  let ny = 0;
  let nz = 0;
  const n = pts.length;
  for (let i = 0; i < n; i++) {
    const a = pts[i];
    const b = pts[(i + 1) % n];
    nx += (a.y - b.y) * (a.z + b.z);
    ny += (a.z - b.z) * (a.x + b.x);
    nz += (a.x - b.x) * (a.y + b.y);
  }
  // Fix Newell formula — use standard:
  nx = 0;
  ny = 0;
  nz = 0;
  for (let i = 0; i < n; i++) {
    const a = pts[i];
    const b = pts[(i + 1) % n];
    nx += (a.y * b.z) - (a.z * b.y);
    ny += (a.z * b.x) - (a.x * b.z);
    nz += (a.x * b.y) - (a.y * b.x);
  }
  const len = Math.hypot(nx, ny, nz) || 1;
  return { x: nx / len, y: ny / len, z: nz / len };
}

function centroid(pts) {
  let x = 0;
  let y = 0;
  let z = 0;
  for (const p of pts) {
    x += p.x;
    y += p.y;
    z += p.z;
  }
  const n = pts.length || 1;
  return { x: x / n, y: y / n, z: z / n };
}

function sub3(a, b) {
  return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
}

function dot3(a, b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

function dedupeEdges(edges) {
  const seen = new Set();
  const out = [];
  const r = (n) => Math.round(n * 4) / 4;
  for (const e of edges) {
    const k1 = `${r(e.a.x)},${r(e.a.y)},${r(e.a.z)}`;
    const k2 = `${r(e.b.x)},${r(e.b.y)},${r(e.b.z)}`;
    const k = k1 < k2 ? `${k1}|${k2}` : `${k2}|${k1}`;
    if (seen.has(k)) continue;
    seen.add(k);
    out.push(e);
  }
  return out;
}

function boundsOfPoints(pts) {
  let minX = Infinity;
  let minY = Infinity;
  let minZ = Infinity;
  let maxX = -Infinity;
  let maxY = -Infinity;
  let maxZ = -Infinity;
  for (const p of pts) {
    minX = Math.min(minX, p.x);
    minY = Math.min(minY, p.y);
    minZ = Math.min(minZ, p.z);
    maxX = Math.max(maxX, p.x);
    maxY = Math.max(maxY, p.y);
    maxZ = Math.max(maxZ, p.z);
  }
  return {
    minX,
    minY,
    minZ,
    maxX,
    maxY,
    maxZ,
    cx: (minX + maxX) / 2,
    cy: (minY + maxY) / 2,
    cz: (minZ + maxZ) / 2,
    size: Math.max(maxX - minX, maxY - minY, maxZ - minZ, 1),
  };
}

// —— Camera ——
// Y-up world. Pitch < 0 looks down from above. Screen: +Y → up.

const DEFAULT_YAW = Math.PI / 4;
const DEFAULT_PITCH = -Math.atan(1 / Math.sqrt(2));

export function createViewerCamera() {
  return {
    yaw: DEFAULT_YAW,
    pitch: DEFAULT_PITCH,
    zoom: 1,
    fitZoom: 1,
  };
}

export function fitViewerCamera(cam, bounds, rect) {
  if (!bounds) return cam;
  cam.yaw = DEFAULT_YAW;
  cam.pitch = DEFAULT_PITCH;
  if (!rect?.width || !rect?.height) {
    cam.zoom = 2;
    cam.fitZoom = 2;
    return cam;
  }
  const corners = [
    [bounds.minX, bounds.minY, bounds.minZ],
    [bounds.maxX, bounds.minY, bounds.minZ],
    [bounds.minX, bounds.maxY, bounds.minZ],
    [bounds.maxX, bounds.maxY, bounds.minZ],
    [bounds.minX, bounds.minY, bounds.maxZ],
    [bounds.maxX, bounds.minY, bounds.maxZ],
    [bounds.minX, bounds.maxY, bounds.maxZ],
    [bounds.maxX, bounds.maxY, bounds.maxZ],
  ];
  let minX = Infinity;
  let maxX = -Infinity;
  let minY = Infinity;
  let maxY = -Infinity;
  for (const [x, y, z] of corners) {
    const r = rotatePoint(
      { x: x - bounds.cx, y: y - bounds.cy, z: z - bounds.cz },
      cam.yaw,
      cam.pitch
    );
    minX = Math.min(minX, r.x);
    maxX = Math.max(maxX, r.x);
    minY = Math.min(minY, r.y);
    maxY = Math.max(maxY, r.y);
  }
  cam.zoom =
    Math.min(rect.width / Math.max(1e-3, maxX - minX), rect.height / Math.max(1e-3, maxY - minY)) * 0.78;
  cam.fitZoom = cam.zoom;
  return cam;
}

function rotatePoint(p, yaw, pitch) {
  // Yaw around +Y, then pitch around +X. View looks toward −Z (larger z = closer).
  const cy = Math.cos(yaw);
  const sy = Math.sin(yaw);
  const x1 = p.x * cy - p.z * sy;
  const z1 = p.x * sy + p.z * cy;
  const cp = Math.cos(pitch);
  const sp = Math.sin(pitch);
  return { x: x1, y: p.y * cp - z1 * sp, z: p.y * sp + z1 * cp };
}

function projectPoint(p, cam, bounds, rect) {
  const r = rotatePoint(
    { x: p.x - bounds.cx, y: p.y - bounds.cy, z: p.z - bounds.cz },
    cam.yaw,
    cam.pitch
  );
  const s = cam.zoom || 1;
  return {
    x: rect.width / 2 + r.x * s,
    y: rect.height / 2 - r.y * s,
    depth: r.z,
  };
}

function faceNormal(face) {
  return polygonNormal(face.pts);
}

/** Studio key / fill / rim + soft sky hemisphere. */
function faceShade(face, cam) {
  const n = rotatePoint(faceNormal(face), cam.yaw, cam.pitch);
  const key = norm3(-0.45, 0.85, 0.55);
  const fill = norm3(0.75, 0.15, 0.35);
  const rim = norm3(0.15, -0.55, -0.65);
  const ndot = (L) => Math.max(0, n.x * L.x + n.y * L.y + n.z * L.z);
  const hemi = 0.5 + 0.5 * n.y;
  let lit =
    0.28 +
    0.18 * hemi +
    0.42 * ndot(key) +
    0.18 * ndot(fill) +
    0.1 * ndot(rim);
  // Soft specular from key — reads volume without looking plastic
  const hx = key.x;
  const hy = key.y + 0.15;
  const hz = key.z + 1;
  const halfLen = Math.hypot(hx, hy, hz) || 1;
  const spec = Math.max(0, (n.x * hx + n.y * hy + n.z * hz) / halfLen);
  lit += 0.12 * Math.pow(spec, 28);
  if (face.role === "edge") lit *= 0.72;
  return Math.max(0.16, Math.min(1.05, lit));
}

function norm3(x, y, z) {
  const len = Math.hypot(x, y, z) || 1;
  return { x: x / len, y: y / len, z: z / len };
}

/** Soft contact shadow under the model (offset with orbit for a light-cast feel). */
function drawGroundShadow(ctx, rect, cam) {
  const cx = rect.width / 2 + Math.sin(cam.yaw || 0) * rect.width * 0.04;
  const cy = rect.height * 0.78;
  const rw = Math.min(rect.width, rect.height);
  // Outer soft pool
  const g0 = ctx.createRadialGradient(cx, cy, 2, cx, cy, rw * 0.34);
  g0.addColorStop(0, "rgba(20, 24, 32, 0.28)");
  g0.addColorStop(0.45, "rgba(20, 24, 32, 0.12)");
  g0.addColorStop(1, "rgba(20, 24, 32, 0)");
  ctx.fillStyle = g0;
  ctx.beginPath();
  ctx.ellipse(cx, cy, rw * 0.38, rw * 0.1, 0, 0, Math.PI * 2);
  ctx.fill();
  // Tighter contact umbra
  const g1 = ctx.createRadialGradient(cx, cy, 1, cx, cy, rw * 0.16);
  g1.addColorStop(0, "rgba(12, 14, 20, 0.45)");
  g1.addColorStop(0.55, "rgba(12, 14, 20, 0.18)");
  g1.addColorStop(1, "rgba(12, 14, 20, 0)");
  ctx.fillStyle = g1;
  ctx.beginPath();
  ctx.ellipse(cx, cy, rw * 0.2, rw * 0.055, 0, 0, Math.PI * 2);
  ctx.fill();
}

/** >0 if the face faces the camera (normal toward +Z in view space). */
function facingCamera(face, cam) {
  return rotatePoint(faceNormal(face), cam.yaw, cam.pitch).z;
}

/**
 * Fully opaque z-buffered grey case (priority depth for near-ties).
 * opts.otherOpacity (0–1): when a focus panel is set, fade every other panel
 * so joints / cavity stay visible through the shell.
 */
export function drawAssembledCase(ctx, mesh, cam, rect, opts = {}) {
  const { highlightPanelId = null, otherOpacity = 1 } = opts;
  const bounds = mesh.bounds;
  const hasHi = !!highlightPanelId;
  const ghost = hasHi ? Math.max(0, Math.min(1, otherOpacity)) : 1;
  const ghosting = hasHi && ghost < 0.995;

  drawGroundShadow(ctx, rect, cam);

  const opaque = [];
  const glass = [];
  for (const f of mesh.faces) {
    const facing = facingCamera(f, cam);
    if (facing < 0.02) continue;
    const shade = faceShade(f, cam);
    const lift = f.role === "edge" ? 0.28 + 0.72 * shade : 0.38 + 0.62 * shade;
    let rgb = f.rgb.map((c) => Math.min(255, Math.round(c * lift)));
    const isFocus = hasHi && f.panelId === highlightPanelId;
    if (hasHi) {
      if (isFocus) {
        rgb = [
          Math.min(255, Math.round(rgb[0] * 0.55 + 90)),
          Math.min(255, Math.round(rgb[1] * 0.55 + 170)),
          Math.min(255, Math.round(rgb[2] * 0.45 + 220)),
        ];
      } else if (!ghosting) {
        rgb = rgb.map((c) => Math.round(c * 0.42 + 18));
      }
    }
    const item = {
      face: f,
      rgb,
      priority: (f.priority || 0) + facing * 0.5,
      alpha: isFocus || !ghosting ? 1 : ghost,
    };
    if (item.alpha < 0.02) continue;
    if (item.alpha >= 0.995) opaque.push(item);
    else glass.push(item);
  }

  const byDepth = (a, b) => {
    const ca = centroid(a.face.pts);
    const cb = centroid(b.face.pts);
    const da = projectPoint(ca, cam, bounds, rect).depth;
    const db = projectPoint(cb, cam, bounds, rect).depth;
    return da - db;
  };
  opaque.sort(byDepth);
  glass.sort(byDepth);

  const maxSide = 1000;
  const scale =
    Math.max(rect.width, rect.height) > maxSide
      ? maxSide / Math.max(rect.width, rect.height)
      : 1;
  const rw = Math.max(1, Math.round(rect.width * scale));
  const rh = Math.max(1, Math.round(rect.height * scale));
  const srect = { width: rw, height: rh };

  const color = new Uint8ClampedArray(rw * rh * 4);
  const zbuf = new Float32Array(rw * rh);
  const pbuf = new Float32Array(rw * rh);
  zbuf.fill(-Infinity);
  pbuf.fill(-Infinity);

  // Opaque first (writes depth), then translucent overlays (depth-test only).
  for (const item of opaque) {
    const proj = item.face.pts.map((p) => projectPoint(p, cam, bounds, srect));
    const cutouts = projectFaceCutouts(item.face, cam, bounds, srect);
    rasterizePolygon(proj, item.rgb, item.priority, color, zbuf, pbuf, rw, rh, {
      alpha: 1,
      writeDepth: true,
      cutouts,
    });
  }
  for (const item of glass) {
    const proj = item.face.pts.map((p) => projectPoint(p, cam, bounds, srect));
    const cutouts = projectFaceCutouts(item.face, cam, bounds, srect);
    rasterizePolygon(proj, item.rgb, item.priority, color, zbuf, pbuf, rw, rh, {
      alpha: item.alpha,
      writeDepth: false,
      cutouts,
    });
  }

  // Cut outlines: panel edges + punched hole rims on faces.
  for (const item of [...opaque, ...glass]) {
    if (item.face.role !== "face" && item.face.role !== "edge") continue;
    const isFocus = hasHi && item.face.panelId === highlightPanelId;
    if (hasHi && !isFocus && item.alpha < 0.08) continue;
    if (hasHi && !isFocus && !ghosting) continue;
    if (item.face.role === "face") {
      const proj = item.face.pts.map((p) => projectPoint(p, cam, bounds, srect));
      const outlineRgb = isFocus || !hasHi ? (hasHi ? [28, 86, 140] : [32, 36, 44]) : [70, 76, 88];
      const outlineStr = isFocus || !hasHi ? (hasHi ? 0.95 : 0.88) : 0.35 * item.alpha;
      strokeProjectedLoop(proj, outlineRgb, color, zbuf, rw, rh, outlineStr);
      const cutouts = projectFaceCutouts(item.face, cam, bounds, srect);
      for (const cut of cutouts) {
        strokeProjectedLoop(cut, outlineRgb, color, zbuf, rw, rh, outlineStr);
      }
    }
  }

  const tmp = drawAssembledCase._tmp || (drawAssembledCase._tmp = document.createElement("canvas"));
  if (tmp.width !== rw || tmp.height !== rh) {
    tmp.width = rw;
    tmp.height = rh;
  }
  tmp.getContext("2d").putImageData(new ImageData(color, rw, rh), 0, 0);
  ctx.imageSmoothingEnabled = scale < 1;
  ctx.drawImage(tmp, 0, 0, rect.width, rect.height);

  if (mesh.note) {
    ctx.fillStyle = "#9aa3b0";
    ctx.font = "12px Inter, sans-serif";
    ctx.fillText(mesh.note, 16, 28);
  }
}

const DEPTH_EPS = 0.35;

function projectFaceCutouts(face, cam, bounds, rect) {
  if (!face?.cutouts2d?.length || typeof face.map2d !== "function") return [];
  const s = face.mapS ?? 0;
  const out = [];
  for (const ring2d of face.cutouts2d) {
    if (!ring2d?.length) continue;
    const ring3d = ring2d.map((p) => face.map2d(p, s));
    out.push(ring3d.map((p) => projectPoint(p, cam, bounds, rect)));
  }
  return out;
}

function strokeProjectedLoop(proj, rgb, color, zbuf, w, h, strength) {
  const n = proj.length;
  for (let i = 0; i < n; i++) {
    const a = proj[i];
    const b = proj[(i + 1) % n];
    if (Math.hypot(b.x - a.x, b.y - a.y) < 0.4) continue;
    strokeVisibleEdge(a, b, rgb, color, zbuf, w, h, strength);
  }
}

function pointInProjectedPoly(x, y, poly) {
  let inside = false;
  const n = poly.length;
  for (let i = 0, j = n - 1; i < n; j = i++) {
    const xi = poly[i].x;
    const yi = poly[i].y;
    const xj = poly[j].x;
    const yj = poly[j].y;
    if ((yi > y) !== (yj > y) && x < ((xj - xi) * (y - yi)) / (yj - yi) + xi) {
      inside = !inside;
    }
  }
  return inside;
}

function rasterizePolygon(proj, rgb, priority, color, zbuf, pbuf, w, h, opts = {}) {
  if (proj.length < 3) return;
  const alpha = opts.alpha == null ? 1 : opts.alpha;
  const writeDepth = opts.writeDepth !== false;
  const cutouts = opts.cutouts || [];
  if (alpha < 0.02) return;
  const [cr, cg, cb] = rgb;
  let p0 = proj[0];
  let p1 = proj[1];
  let p2 = null;
  for (let i = 2; i < proj.length; i++) {
    const cross = (p1.x - p0.x) * (proj[i].y - p0.y) - (p1.y - p0.y) * (proj[i].x - p0.x);
    if (Math.abs(cross) > 1e-4) {
      p2 = proj[i];
      break;
    }
  }
  if (!p2) return;
  const den = (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
  if (Math.abs(den) < 1e-8) return;
  const ax = ((p1.depth - p0.depth) * (p2.y - p0.y) - (p2.depth - p0.depth) * (p1.y - p0.y)) / den;
  const ay = ((p2.depth - p0.depth) * (p1.x - p0.x) - (p1.depth - p0.depth) * (p2.x - p0.x)) / den;
  const az = p0.depth - ax * p0.x - ay * p0.y;

  let minX = Infinity;
  let maxX = -Infinity;
  let minY = Infinity;
  let maxY = -Infinity;
  for (const p of proj) {
    minX = Math.min(minX, p.x);
    maxX = Math.max(maxX, p.x);
    minY = Math.min(minY, p.y);
    maxY = Math.max(maxY, p.y);
  }
  minX = Math.max(0, Math.floor(minX));
  maxX = Math.min(w - 1, Math.ceil(maxX));
  minY = Math.max(0, Math.floor(minY));
  maxY = Math.min(h - 1, Math.ceil(maxY));
  if (minX > maxX || minY > maxY) return;

  const n = proj.length;
  const invA = 1 - alpha;
  for (let y = minY; y <= maxY; y++) {
    const ys = y + 0.5;
    const xs = [];
    for (let i = 0; i < n; i++) {
      const a = proj[i];
      const b = proj[(i + 1) % n];
      if ((a.y <= ys && b.y > ys) || (b.y <= ys && a.y > ys)) {
        const t = (ys - a.y) / (b.y - a.y);
        xs.push(a.x + t * (b.x - a.x));
      }
    }
    if (xs.length < 2) continue;
    xs.sort((u, v) => u - v);
    const pairs = xs.length & 1 ? xs.length - 1 : xs.length;
    for (let k = 0; k + 1 < pairs; k += 2) {
      const x0 = Math.max(minX, Math.ceil(xs[k]));
      const x1 = Math.min(maxX, Math.floor(xs[k + 1]));
      for (let x = x0; x <= x1; x++) {
        const px = x + 0.5;
        if (cutouts.length) {
          let punched = false;
          for (const cut of cutouts) {
            if (pointInProjectedPoly(px, ys, cut)) {
              punched = true;
              break;
            }
          }
          if (punched) continue;
        }
        const z = ax * px + ay * ys + az;
        const idx = y * w + x;
        const zPrev = zbuf[idx];
        if (writeDepth) {
          if (z > zPrev + DEPTH_EPS) {
            // clearly nearer
          } else if (z < zPrev - DEPTH_EPS) {
            continue;
          } else if (priority < pbuf[idx]) {
            continue;
          } else if (priority === pbuf[idx] && z <= zPrev) {
            continue;
          }
          zbuf[idx] = z;
          pbuf[idx] = priority;
          const o = idx * 4;
          color[o] = cr;
          color[o + 1] = cg;
          color[o + 2] = cb;
          color[o + 3] = 255;
        } else {
          // Translucent: only draw if in front of (or on) existing opaque depth
          if (z + DEPTH_EPS < zPrev) continue;
          const o = idx * 4;
          if (!color[o + 3]) {
            color[o] = Math.round(cr * alpha);
            color[o + 1] = Math.round(cg * alpha);
            color[o + 2] = Math.round(cb * alpha);
            color[o + 3] = Math.round(255 * alpha);
          } else {
            color[o] = Math.round(color[o] * invA + cr * alpha);
            color[o + 1] = Math.round(color[o + 1] * invA + cg * alpha);
            color[o + 2] = Math.round(color[o + 2] * invA + cb * alpha);
            color[o + 3] = 255;
          }
        }
      }
    }
  }
}

function dist3(a, b) {
  return Math.hypot(a.x - b.x, a.y - b.y, a.z - b.z);
}

function strokeVisibleEdge(a, b, rgb, color, zbuf, w, h, strength = 1) {
  const dx = b.x - a.x;
  const dy = b.y - a.y;
  const dz = b.depth - a.depth;
  const steps = Math.max(2, Math.ceil(Math.hypot(dx, dy) * 1.6));
  const [r, g, bl] = rgb;
  const s = Math.max(0, Math.min(1, strength));
  // Pull slightly toward camera so the line wins over its own fill / adjacent rim
  const zBias = 0.9;
  const zSlop = 1.8;
  for (let i = 0; i <= steps; i++) {
    const t = i / steps;
    const x = a.x + dx * t;
    const y = a.y + dy * t;
    const z = a.depth + dz * t + zBias;
    const xi = Math.round(x);
    const yi = Math.round(y);
    if (xi < 0 || yi < 0 || xi >= w || yi >= h) continue;
    for (const [ox, oy, kMul] of [
      [0, 0, 1],
      [1, 0, 0.7],
      [-1, 0, 0.7],
      [0, 1, 0.7],
      [0, -1, 0.7],
    ]) {
      const x2 = xi + ox;
      const y2 = yi + oy;
      if (x2 < 0 || y2 < 0 || x2 >= w || y2 >= h) continue;
      const j = y2 * w + x2;
      // Occluded by something clearly nearer
      if (z + zSlop < zbuf[j]) continue;
      const p = j * 4;
      if (!color[p + 3]) continue;
      const k = s * kMul;
      color[p] = Math.round(color[p] * (1 - k) + r * k);
      color[p + 1] = Math.round(color[p + 1] * (1 - k) + g * k);
      color[p + 2] = Math.round(color[p + 2] * (1 - k) + bl * k);
      color[p + 3] = 255;
    }
  }
}

function fmt(n) {
  return String(Math.round(n * 10) / 10);
}

export const buildCaseMesh = buildAssembledCase;
export const createOrbitCamera = createViewerCamera;
export const fitOrbitCamera = fitViewerCamera;
export const drawCaseMesh = drawAssembledCase;
