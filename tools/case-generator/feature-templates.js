/** Reusable feature templates in outer-case assembly coordinates.

 * Frame (entry.frame === "outer", default for new captures):
 * - Origin: chosen profile corner on End B’s outer face → assembly (0,0,0)
 * - X/Y: outer envelope / outer wall face (thickness along outward normals)
 * - Z: 0 at End B outer face; End A outer at depth + 2·thickness;
 *       wall body local Y maps to Z = thickness + localY
 *
 * Legacy entries without `frame: "outer"` keep wall Z as panel-local Y and
 * wall XY on the body edge (pre-thickness).
 */

import { uid } from "./model.js";
import { normalize, sub, add, scale, outwardNormal } from "./joints.js";

const EDGE_TOLERANCE_MM = 0.75;

export function currentTemplateDatum(doc) {
  const index = doc.templateDatum?.vertexIndex;
  if (!Number.isInteger(index) || !doc.profile?.points?.[index]) return null;
  const p = doc.profile.points[index];
  const thickness = Math.max(0.05, Number(doc.thickness) || 3);
  const depth = Math.max(thickness * 2, Number(doc.depth) || thickness * 2);
  return {
    vertexIndex: index,
    x: p.x,
    y: p.y,
    /** Assembly Z of End B outer face (origin). */
    z: 0,
    thickness,
    depth,
    /** Assembly Z of End A outer face. */
    zEndA: depth + 2 * thickness,
  };
}

export function captureFeatureTemplate(doc, panels, name) {
  const datum = currentTemplateDatum(doc);
  if (!datum) return { error: "Set a profile template origin first." };
  if (doc.fixedPanels?.length) {
    return { error: "Templates require an editable generated profile." };
  }

  const byId = new Map(panels.map((panel) => [panel.id, panel]));
  const entries = [];
  let skipped = 0;
  for (const feature of doc.features || []) {
    const panel = byId.get(feature.panelId);
    const entry = panel ? featureToEntry(feature, panel, datum) : null;
    if (entry) entries.push(entry);
    else skipped++;
  }
  if (!entries.length) return { error: "Place at least one feature before saving a template." };

  return {
    template: {
      id: uid("tpl"),
      name: String(name || "Hole template").trim().slice(0, 120) || "Hole template",
      createdAt: Date.now(),
      entries,
    },
    skipped,
  };
}

export function applyFeatureTemplate(doc, panels, template) {
  const datum = currentTemplateDatum(doc);
  if (!datum) {
    return { features: [], skipped: template?.entries?.length || 0, error: "Set a profile template origin first." };
  }
  if (doc.fixedPanels?.length) {
    return {
      features: [],
      skipped: template?.entries?.length || 0,
      error: "Templates require an editable generated profile.",
    };
  }

  const byId = new Map(panels.map((panel) => [panel.id, panel]));
  const walls = panels.filter((panel) => panel.kind === "wall" && panel.edgeA && panel.edgeB);
  const features = [];
  let skipped = 0;

  for (const entry of template?.entries || []) {
    const feature = entryToFeature(entry, datum, byId, walls, template.id);
    if (feature) features.push(feature);
    else skipped++;
  }
  return { features, skipped };
}

function featureToEntry(feature, panel, datum) {
  const center = featureCenter(feature);
  const shape = featureShape(feature);
  if (!center || !shape) return null;
  const t = datum.thickness;

  if (panel.id === "endA" || panel.id === "endB") {
    const shift = panel.originShift || { x: 0, y: 0 };
    return {
      ...shape,
      face: panel.id,
      frame: "outer",
      dx: center.x + shift.x - datum.x,
      dy: center.y + shift.y - datum.y,
      // End B outer = 0; End A outer = depth + 2t
      z: panel.id === "endA" ? datum.zEndA : 0,
    };
  }

  if (panel.kind !== "wall" || !panel.edgeA || !panel.edgeB) return null;
  const dir = normalize(sub(panel.edgeB, panel.edgeA));
  const normal = outwardNormal(dir);
  const along0 = ((panel.width || panel.edgeLength) - panel.edgeLength) * 0.5;
  const along = center.x - along0;
  // Outer-face XY (body edge + thickness along outward normal).
  const outer = add(add(panel.edgeA, scale(dir, along)), scale(normal, t));
  return {
    ...shape,
    face: "wall",
    frame: "outer",
    dx: outer.x - datum.x,
    dy: outer.y - datum.y,
    // Z from End B outer: wall local Y=0 sits on End B inner (z=t).
    z: t + center.y,
    normal,
  };
}

function entryToFeature(entry, datum, byId, walls, templateId) {
  const outerFrame = entry.frame === "outer";
  const t = datum.thickness;
  const target = { x: datum.x + entry.dx, y: datum.y + entry.dy };

  if (entry.face === "endA" || entry.face === "endB") {
    const panel = byId.get(entry.face);
    if (!panel) return null;
    const shift = panel.originShift || { x: 0, y: 0 };
    const local = { x: target.x - shift.x, y: target.y - shift.y };
    if (!pointInNominalPanel(local, panel)) return null;
    return featureAt(entry, panel.id, local.x, local.y, templateId);
  }

  if (entry.face !== "wall") return null;

  const wantedNormal = entry.normal || { x: 0, y: 0 };
  // Outer-frame targets lie on the outer face; project back to the body edge.
  const bodyTarget = outerFrame
    ? add(target, scale(normalize(wantedNormal), -t))
    : target;

  const match = findWallForPoint(bodyTarget, wantedNormal, walls);
  if (!match) return null;

  let localY;
  if (outerFrame) {
    localY = Number(entry.z) - t;
  } else {
    localY = Number(entry.z);
  }
  if (!Number.isFinite(localY) || localY < -1e-6 || localY > match.panel.height + 1e-6) {
    return null;
  }
  const along0 = ((match.panel.width || match.panel.edgeLength) - match.panel.edgeLength) * 0.5;
  return featureAt(entry, match.panel.id, match.along + along0, localY, templateId);
}

function findWallForPoint(point, wantedNormal, walls) {
  let best = null;
  for (const panel of walls) {
    const delta = sub(panel.edgeB, panel.edgeA);
    const length = Math.hypot(delta.x, delta.y);
    if (length < 1e-9) continue;
    const dir = { x: delta.x / length, y: delta.y / length };
    const rel = sub(point, panel.edgeA);
    const along = rel.x * dir.x + rel.y * dir.y;
    if (along < -EDGE_TOLERANCE_MM || along > length + EDGE_TOLERANCE_MM) continue;
    const clamped = Math.max(0, Math.min(length, along));
    const nearest = {
      x: panel.edgeA.x + dir.x * clamped,
      y: panel.edgeA.y + dir.y * clamped,
    };
    const distance = Math.hypot(point.x - nearest.x, point.y - nearest.y);
    if (distance > EDGE_TOLERANCE_MM) continue;
    const normal = outwardNormal(dir);
    const dot = normal.x * wantedNormal.x + normal.y * wantedNormal.y;
    if (dot < 0.5) continue;
    const score = distance + (1 - dot) * 10;
    if (!best || score < best.score) best = { panel, along: clamped, score };
  }
  return best;
}

function featureCenter(feature) {
  if (feature.type === "circle") return { x: feature.cx, y: feature.cy };
  if (feature.type === "roundRect") {
    return { x: feature.x + feature.w * 0.5, y: feature.y + feature.h * 0.5 };
  }
  return null;
}

function featureShape(feature) {
  if (feature.type === "circle") return { type: "circle", r: feature.r };
  if (feature.type === "roundRect") {
    return { type: "roundRect", w: feature.w, h: feature.h, r: feature.r };
  }
  return null;
}

function featureAt(entry, panelId, cx, cy, templateId) {
  if (entry.type === "circle") {
    return { id: uid("feat"), type: "circle", panelId, cx, cy, r: entry.r, templateId };
  }
  if (entry.type === "roundRect") {
    return {
      id: uid("feat"),
      type: "roundRect",
      panelId,
      x: cx - entry.w * 0.5,
      y: cy - entry.h * 0.5,
      w: entry.w,
      h: entry.h,
      r: entry.r,
      templateId,
    };
  }
  return null;
}

function pointInNominalPanel(point, panel) {
  if (panel.nominal?.type === "rect") {
    return (
      point.x >= -1e-6 &&
      point.y >= -1e-6 &&
      point.x <= panel.nominal.w + 1e-6 &&
      point.y <= panel.nominal.h + 1e-6
    );
  }
  const ring = panel.nominal?.points;
  if (!ring?.length) return false;
  let inside = false;
  for (let i = 0, j = ring.length - 1; i < ring.length; j = i++) {
    const a = ring[i];
    const b = ring[j];
    const crosses =
      a.y > point.y !== b.y > point.y &&
      point.x < ((b.x - a.x) * (point.y - a.y)) / (b.y - a.y || 1e-12) + a.x;
    if (crosses) inside = !inside;
  }
  return inside;
}
