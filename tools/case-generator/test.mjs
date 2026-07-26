import { createDefaultDoc, serializeDoc, parseDoc } from "./model.js";
import { unfold, panelBounds } from "./unfold.js";
import { exportSvg } from "./svg-export.js";
import {
  dist,
  fingerCount,
  fingerWidths,
  fingerEdgePointList,
  fingerEdgePoints,
  fingeredRect,
  fingeredPolygon,
  simplifyPolyline,
  polygonSelfIntersects,
  isAxisAligned,
} from "./joints.js";
import { createCircleFeature, createRoundRectFeature, roundRectRadiusMm } from "./features.js";
import { buildAssembledCase } from "./case-viewer.js";
import {
  currentTemplateDatum,
  captureFeatureTemplate,
  applyFeatureTemplate,
} from "./feature-templates.js";

const doc = createDefaultDoc();
if (doc.fixedPanels !== null) {
  throw new Error("default should use generated FRONT profile, not fixed panels");
}
if (!doc.profile.closed || doc.profile.points.length !== 6) {
  throw new Error("default should load the six-corner FRONT profile");
}
const frontWidth =
  Math.max(...doc.profile.points.map((p) => p.x)) -
  Math.min(...doc.profile.points.map((p) => p.x));
const frontHeight =
  Math.max(...doc.profile.points.map((p) => p.y)) -
  Math.min(...doc.profile.points.map((p) => p.y));
if (Math.abs(frontWidth - 246) > 1e-6 || Math.abs(frontHeight - 30.6) > 1e-6) {
  throw new Error(`FRONT nominal dimensions ${frontWidth}×${frontHeight}`);
}
const frontLeftHeight = Math.abs(doc.profile.points[5].y - doc.profile.points[0].y);
if (Math.abs(frontLeftHeight - 13.5) > 1e-6) {
  throw new Error(`FRONT left outer height should be 7.5+3+3, got ${frontLeftHeight}`);
}
if (doc.kerf !== -0.125) throw new Error("default kerf");
if (doc.fingerLength !== 20) throw new Error("default finger");
if (doc.minFingerWidth !== 5) throw new Error("default min finger width");
if (doc.invertFingers !== true) throw new Error("default should invert fingers");

const { panels, error } = unfold(doc);
if (error) throw new Error(error);
if (panels.length !== 8) throw new Error(`FRONT profile expected 8 panels, got ${panels.length}`);
const frontEnd = panels.find((p) => p.id === "endA");
const endXs = frontEnd.outerPoints.map((p) => p.x);
const endYs = frontEnd.outerPoints.map((p) => p.y);
if (Math.min(...endXs) < -0.2 || Math.max(...endXs) > 246.2) {
  throw new Error("FRONT end male tips should restore the 0–246 outer envelope");
}
if (Math.min(...endYs) < -0.2 || Math.max(...endYs) > 30.8) {
  throw new Error("FRONT end male tips should restore the outer Y envelope");
}
// No inward “negative” notches on the nest outlines.
for (const p of panels) {
  const b = panelBounds(p);
  if (!(b.width > 0 && b.height > 0)) throw new Error(`bad bounds ${p.id}`);
  if (!p.outerPath.startsWith("M ")) throw new Error(`bad path ${p.id}`);
}

const svg = exportSvg(doc);
if (!svg.includes('width="') || !svg.includes("mm")) throw new Error("svg missing mm units");
if (!svg.includes('class="cut"')) throw new Error("svg missing cut class");
if (/NaN|undefined|Infinity/.test(svg)) throw new Error("bad numbers in svg");

// Click-draw style closed profile → generative
doc.profile = {
  points: [
    { x: 0, y: 0 },
    { x: 100, y: 0 },
    { x: 100, y: 60 },
    { x: 0, y: 60 },
  ],
  closed: true,
};
doc.features = [createCircleFeature("endA", 50, 30, 4)];
doc.features.push(createRoundRectFeature("wall-0", 5, 5, 20, 10, 50));
if (Math.abs(roundRectRadiusMm(20, 10, 50) - 5) > 1e-9) {
  throw new Error("50% should fully round short side (5mm)");
}
const gen = unfold(doc);
if (gen.error) throw new Error(gen.error);
if (gen.panels.length !== 6) throw new Error(`generative expected 6 panels, got ${gen.panels.length}`);

if (fingerCount(100, 20) % 2 === 0) throw new Error("finger count should be odd");
if (fingerCount(4, 20) !== 0) {
  throw new Error("edges that cannot fit fingers >5mm should be ignored");
}
if (fingerCount(5, 20) !== 0) {
  throw new Error("fingers of exactly 5mm are invalid; only >5mm counts");
}
if (fingerCount(100, 5) < 1) {
  throw new Error("preferred pitch 5mm should still lay out fingers >5mm on a long edge");
}
// User-set min finger width: raise floor → short edges go plain; lower → allow smaller
if (fingerCount(8, 20, { minFingerLength: 10 }) !== 0) {
  throw new Error("8mm edge must be plain when min finger is 10mm");
}
if (fingerCount(8, 20, { minFingerLength: 3 }) < 1) {
  throw new Error("8mm edge should finger when min is 3mm");
}

{
  const highMin = createDefaultDoc();
  highMin.fixedPanels = null;
  highMin.profile = {
    closed: true,
    points: [
      { x: 0, y: 0 },
      { x: 80, y: 0 },
      { x: 80, y: 60 },
      { x: 0, y: 60 },
    ],
  };
  highMin.depth = 40;
  highMin.thickness = 3;
  highMin.fingerLength = 20;
  highMin.minFingerWidth = 100;
  highMin.features = [];
  const { panels: hp, error: he } = unfold(highMin);
  if (he) throw new Error(he);
  const wall = hp.find((p) => p.kind === "wall");
  // Min 100mm > every edge — all joints stay plain
  const ys = wall.outerPoints.map((p) => p.y);
  if (Math.min(...ys) < -0.5 || Math.max(...ys) > wall.height + 0.5) {
    throw new Error("high minFingerWidth should omit long-edge fingers");
  }
}

const w100 = fingerWidths(100, 20);
if (w100[0] !== w100[w100.length - 1]) throw new Error("end fingers must match");
if (w100.length >= 3) {
  for (let i = 1; i < w100.length - 1; i++) {
    if (Math.abs(w100[i] - 20) > 1e-9) throw new Error(`inner finger ${i} should be 20, got ${w100[i]}`);
  }
}
const sum100 = w100.reduce((a, b) => a + b, 0);
if (Math.abs(sum100 - 100) > 1e-6) throw new Error("widths must sum to edge length");

const w110 = fingerWidths(110, 20);
if (Math.abs(w110[0] - w110[w110.length - 1]) > 1e-9) throw new Error("110 ends asymmetric");
if (w110.length >= 3 && w110.slice(1, -1).some((x) => Math.abs(x - 20) > 1e-9)) {
  throw new Error("110 inners must be 20");
}

const edge = fingerEdgePointList(
  { x: 0, y: 0 },
  { x: 100, y: 0 },
  { thickness: 3, fingerLength: 20, startMale: true, kerf: -0.125 }
);
if (edge.length < 4) throw new Error("finger edge too short");

// Diagonal fingers must be true 90° rectangles in edge space (not slanted lead-ins)
{
  const diag = fingerEdgePoints(
    { x: 0, y: 0 },
    { x: 80, y: 60 },
    { thickness: 6, fingerLength: 15, startMale: true, kerf: 0, endClearance: 6 }
  );
  if (diag.points.length < 6) throw new Error("diagonal finger edge too short");
  for (let i = 1; i < diag.points.length - 1; i++) {
    const a = diag.points[i - 1];
    const b = diag.points[i];
    const c = diag.points[i + 1];
    const ux = b.x - a.x;
    const uy = b.y - a.y;
    const vx = c.x - b.x;
    const vy = c.y - b.y;
    const lu = Math.hypot(ux, uy);
    const lv = Math.hypot(vx, vy);
    if (lu < 1e-6 || lv < 1e-6) continue;
    const dot = (ux / lu) * (vx / lv) + (uy / lu) * (vy / lv);
    if (Math.abs(dot) > 0.05) {
      throw new Error(`diagonal finger corner ${i} must be 90°, dot=${dot}`);
    }
  }
}

const joint = (m) => ({ thickness: 3, fingerLength: 20, startMale: m, kerf: -0.125 });
const rectMale = fingeredRect(100, 40, [joint(true), joint(true), joint(true), joint(true)]);
const rectFemale = fingeredRect(100, 40, [joint(false), joint(false), joint(false), joint(false)]);

function assertAxisAligned(pts, label) {
  for (let i = 0; i < pts.length; i++) {
    const a = pts[i];
    const b = pts[(i + 1) % pts.length];
    const dx = Math.abs(b.x - a.x);
    const dy = Math.abs(b.y - a.y);
    if (dx > 1e-6 && dy > 1e-6) {
      throw new Error(`${label}: diagonal segment at ${i} (${a.x},${a.y})→(${b.x},${b.y})`);
    }
  }
}
assertAxisAligned(rectMale, "male");
assertAxisAligned(rectFemale, "female");

function selfIntersects(pts) {
  const n = pts.length;
  const ori = (a, b, c) => (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  const near = (p, q) => Math.hypot(p.x - q.x, p.y - q.y) < 1e-6;
  for (let i = 0; i < n; i++) {
    const a = pts[i];
    const b = pts[(i + 1) % n];
    for (let j = i + 1; j < n; j++) {
      if (j === i + 1 || (i === 0 && j === n - 1)) continue;
      const c = pts[j];
      const d = pts[(j + 1) % n];
      if (near(a, c) || near(a, d) || near(b, c) || near(b, d)) continue;
      const o1 = ori(a, b, c);
      const o2 = ori(a, b, d);
      const o3 = ori(c, d, a);
      const o4 = ori(c, d, b);
      if (o1 * o2 < 0 && o3 * o4 < 0) return true;
    }
  }
  return false;
}
if (selfIntersects(rectMale)) throw new Error("male rect self-intersects");
if (selfIntersects(rectFemale)) throw new Error("female rect self-intersects");

// Small 40×40 case must not produce impossible/negative finger crossings
doc.profile = {
  points: [
    { x: 0, y: 0 },
    { x: 40, y: 0 },
    { x: 40, y: 40 },
    { x: 0, y: 40 },
  ],
  closed: true,
};
doc.depth = 160;
const small = unfold(doc);
if (small.error) throw new Error(small.error);
for (const p of small.panels) {
  if (selfIntersects(p.outerPoints)) {
    throw new Error(`self-intersecting fingers on ${p.label}`);
  }
}

const endA = small.panels.find((p) => p.id === "endA");
const endB = small.panels.find((p) => p.id === "endB");
if (!endA || !endB) throw new Error("missing end panels");
if (endA.outerPoints.length !== endB.outerPoints.length) {
  throw new Error("End A/B should be the same form");
}
for (let i = 0; i < endA.outerPoints.length; i++) {
  const a = endA.outerPoints[i];
  const b = endB.outerPoints[i];
  if (Math.hypot(a.x - b.x, a.y - b.y) > 1e-6) {
    throw new Error("End A/B outlines must match");
  }
}

// Ortho ends keep outer L corners (not a direct tip-to-tip bevel across the corner)
{
  const joint = (m) => ({ thickness: 3, fingerLength: 20, startMale: m, kerf: -0.125 });
  const rect = [
    { x: 0, y: 0 },
    { x: 100, y: 0 },
    { x: 100, y: 60 },
    { x: 0, y: 60 },
  ];
  const path = fingeredPolygon(
    rect,
    rect.map(() => joint(true))
  );
  // Outer tip near (0,0) should reach ≈ (-t,-t)
  const outer = path.find((p) => p.x < -1 && p.y < -1);
  if (!outer) throw new Error("ortho male–male should keep outer L corner");
}

// Diagonal / house profile must not self-intersect; short fingers allowed
doc.profile = {
  points: [
    { x: 0, y: 0 },
    { x: 120, y: 0 },
    { x: 120, y: 50 },
    { x: 70, y: 90 },
    { x: 0, y: 50 },
  ],
  closed: true,
};
doc.features = [];
const house = unfold(doc);
if (house.error) throw new Error(house.error);
for (const p of house.panels) {
  if (selfIntersects(p.outerPoints)) {
    throw new Error(`diagonal corner self-intersects on ${p.label}`);
  }
}

// Skewed quad — all edges diagonal
doc.profile = {
  points: [
    { x: 0, y: 0 },
    { x: 100, y: 10 },
    { x: 90, y: 70 },
    { x: 10, y: 60 },
  ],
  closed: true,
};
const skew = unfold(doc);
if (skew.error) throw new Error(skew.error);
for (const p of skew.panels) {
  if (selfIntersects(p.outerPoints)) {
    throw new Error(`skew self-intersects on ${p.label}`);
  }
}
// Diagonal edges with invert cover the run without self-intersecting corners
{
  const end = skew.panels.find((p) => p.id === "endA");
  const xs = end.outerPoints.map((p) => p.x);
  const ys = end.outerPoints.map((p) => p.y);
  if (Math.max(...xs) - Math.min(...xs) < 80 || Math.max(...ys) - Math.min(...ys) < 50) {
    throw new Error("skew end should keep near-outer extent after inset-body tabs");
  }
}

// simplify drops mid-colinear + near-duplicate points
const withColinear = simplifyPolyline(
  [
    { x: 0, y: 0 },
    { x: 50, y: 0 },
    { x: 100, y: 0 },
    { x: 100, y: 60 },
    { x: 0, y: 60 },
    { x: 0, y: 0.00001 },
  ],
  { closed: true }
);
if (withColinear.length !== 4) {
  throw new Error(`simplify should leave 4 corners, got ${withColinear.length}`);
}

// Edge insert adds exactly one vertex (same rule as app insertEdgePoint)
function insertOne(pts, edgeIndex, world, eps = 0.05) {
  const n = pts.length;
  const a = pts[edgeIndex];
  const b = pts[(edgeIndex + 1) % n];
  if (dist(world, a) <= eps || dist(world, b) <= eps) return -1;
  for (const q of pts) {
    if (dist(world, q) <= eps) return -1;
  }
  pts.splice(edgeIndex + 1, 0, { ...world });
  return edgeIndex + 1;
}
const insertPts = [
  { x: 0, y: 0 },
  { x: 100, y: 0 },
  { x: 100, y: 60 },
  { x: 0, y: 60 },
];
const before = insertPts.length;
const idx = insertOne(insertPts, 0, { x: 50, y: 0 });
if (idx !== 1 || insertPts.length !== before + 1) {
  throw new Error("edge insert must add exactly one vertex");
}
if (insertOne(insertPts, 0, { x: 0, y: 0 }) !== -1) {
  throw new Error("insert on existing vertex should be skipped");
}

// Diagonal triangle profile unfolds without self-intersecting finger paths
doc.profile = {
  points: [
    { x: 0, y: 0 },
    { x: 120, y: 0 },
    { x: 40, y: 80 },
  ],
  closed: true,
};
doc.features = [];
doc.depth = 100;
if (polygonSelfIntersects(doc.profile.points)) {
  throw new Error("triangle profile should not self-intersect");
}
const diag = unfold(doc);
if (diag.error) throw new Error(diag.error);
if (diag.panels.length !== 5) {
  throw new Error(`diagonal triangle expected 5 panels, got ${diag.panels.length}`);
}
for (const p of diag.panels) {
  if (selfIntersects(p.outerPoints)) {
    throw new Error(`self-intersecting fingers on diagonal ${p.label}`);
  }
}

// Non-ortho corners must not get wall–wall side fingers (they pierce).
{
  const house = createDefaultDoc();
  house.fixedPanels = null;
  house.profile = {
    closed: true,
    points: [
      { x: 0, y: 0 },
      { x: 100, y: 0 },
      { x: 100, y: 40 },
      { x: 50, y: 70 },
      { x: 0, y: 40 },
    ],
  };
  house.depth = 50;
  house.thickness = 3;
  house.fingerLength = 10;
  house.features = [];
  const { panels: hp, error: he } = unfold(house);
  if (he) throw new Error(he);
  const roof = hp.filter((p) => p.kind === "wall" && !isAxisAligned(p.edgeA, p.edgeB));
  if (roof.length < 2) throw new Error("house profile should have diagonal walls");
  for (const w of roof) {
    const xs = w.outerPoints.map((p) => p.x);
    const minX = Math.min(...xs);
    const maxX = Math.max(...xs);
    if (minX < -0.5 || maxX > w.width + 0.5) {
      throw new Error(`${w.id} diagonal wall must not have side finger protrusions`);
    }
  }
  if (hp.some((p) => p.kind === "wall" && p.startTrim > 1e-6)) {
    throw new Error("non-overlapping convex walls must keep their full length");
  }
}

// A wall that would enter the previous wall's material is shortened just enough
// for its complete thickness to clear; unaffected corners remain full length.
{
  const dent = createDefaultDoc();
  dent.fixedPanels = null;
  dent.profile = {
    closed: true,
    points: [
      { x: 0, y: 0 },
      { x: 120, y: 0 },
      { x: 120, y: 80 },
      { x: 70, y: 80 },
      { x: 40, y: 40 },
      { x: 0, y: 40 },
    ],
  };
  dent.depth = 50;
  dent.thickness = 6;
  dent.fingerLength = 15;
  dent.features = [];
  const { panels: dp, error: de } = unfold(dent);
  if (de) throw new Error(de);
  const walls = dp.filter((p) => p.kind === "wall");
  const trimmed = walls.filter((p) => p.startTrim > 1e-6);
  if (trimmed.length !== 1 || trimmed[0].id !== "wall-4") {
    throw new Error("only the physically overlapping outgoing wall should shorten");
  }
  if (walls.some((p) => p.id !== "wall-4" && p.startTrim > 1e-6)) {
    throw new Error("non-overlapping wall corners must remain square and full length");
  }
}

// Regression: inverted short diagonal has outward male tabs (not inward notches).
{
  const inverted = createDefaultDoc();
  inverted.fixedPanels = null;
  inverted.invertFingers = true;
  inverted.profile = {
    closed: true,
    points: [
      { x: 0, y: 30 },
      { x: 0, y: 50 },
      { x: 100, y: 50 },
      { x: 100, y: 10 },
      { x: 60, y: 10 },
      { x: 40, y: 30 },
    ],
  };
  inverted.depth = 160;
  inverted.thickness = 3;
  inverted.fingerLength = 20;
  inverted.minFingerWidth = 5;
  inverted.kerf = -0.125;
  inverted.features = [];
  const { panels: ip, error: ie } = unfold(inverted);
  if (ie) throw new Error(ie);
  const shortDiagonal = ip.find((p) => p.id === "wall-0");
  if (!shortDiagonal) throw new Error("expected inverted short diagonal wall");
  const tipRuns = [];
  for (let i = 0; i < shortDiagonal.outerPoints.length - 1; i++) {
    const a = shortDiagonal.outerPoints[i];
    const b = shortDiagonal.outerPoints[i + 1];
    if (a.y < -1 && b.y < -1 && Math.abs(a.y - b.y) < 1e-6) {
      tipRuns.push(Math.abs(b.x - a.x));
    }
  }
  if (tipRuns.length !== 2 || tipRuns.some((w) => w < 9)) {
    throw new Error(`short inverted diagonal should have two long tabs, got ${tipRuns}`);
  }
  const bottom = shortDiagonal.outerPoints.filter((p) => p.y < -1);
  const minX = Math.min(...bottom.map((p) => p.x));
  const maxX = Math.max(...bottom.map((p) => p.x));
  if (minX > 1e-6 || maxX < shortDiagonal.width - 1e-6) {
    throw new Error("inverted wall tabs must reach both connected edge corners");
  }
}

// Case Viewer builds assembled shells for generated profiles.
// Profile datum templates round-trip holes across both ends and multiple walls.
{
  const source = createDefaultDoc();
  source.fixedPanels = null;
  source.profile = {
    closed: true,
    points: [
      { x: 0, y: 0 },
      { x: 100, y: 0 },
      { x: 100, y: 60 },
      { x: 0, y: 60 },
    ],
  };
  source.depth = 100;
  source.features = [
    createCircleFeature("endA", 20, 20, 2),
    createRoundRectFeature("endB", 65, 35, 12, 8, 25),
    createCircleFeature("wall-0", 30, 40, 1.5),
    createRoundRectFeature("wall-1", 15, 65, 10, 6, 20),
  ];
  source.templateDatum = { vertexIndex: 3, x: 0, y: 60 };
  const sourceUnfold = unfold(source);
  if (sourceUnfold.error) throw new Error(sourceUnfold.error);
  const captured = captureFeatureTemplate(source, sourceUnfold.panels, "PCB fixture");
  if (captured.error || captured.template.entries.length !== 4) {
    throw new Error(`expected four captured template entries: ${captured.error || ""}`);
  }
  const datum = currentTemplateDatum(source);
  if (!datum || datum.x !== 0 || datum.y !== 60) throw new Error("template datum");

  const applied = applyFeatureTemplate(source, sourceUnfold.panels, captured.template);
  if (applied.error || applied.skipped || applied.features.length !== 4) {
    throw new Error(`template round trip failed: ${applied.error || applied.skipped}`);
  }
  const wall0 = applied.features.find((f) => f.panelId === "wall-0");
  const wall1 = applied.features.find((f) => f.panelId === "wall-1");
  const endA = applied.features.find((f) => f.panelId === "endA");
  const endB = applied.features.find((f) => f.panelId === "endB");
  if (!wall0 || Math.abs(wall0.cx - 30) > 1e-6 || Math.abs(wall0.cy - 40) > 1e-6) {
    throw new Error("wall template X/Z round trip");
  }
  if (!wall1 || Math.abs(wall1.x - 15) > 1e-6 || Math.abs(wall1.y - 65) > 1e-6) {
    throw new Error("oriented wall round-rect round trip");
  }
  if (!endA || Math.abs(endA.cx - 20) > 1e-6 || Math.abs(endA.cy - 20) > 1e-6) {
    throw new Error("End A template round trip");
  }
  if (!endB || Math.abs(endB.x - 65) > 1e-6 || Math.abs(endB.y - 35) > 1e-6) {
    throw new Error("End B template round trip");
  }
  if (applied.features.some((f) => f.templateId !== captured.template.id)) {
    throw new Error("applied features should retain template source ID");
  }

  // Translating profile + datum keeps all local panel positions unchanged.
  const translated = parseDoc(serializeDoc(source));
  translated.profile.points = translated.profile.points.map((p) => ({
    x: p.x + 10,
    y: p.y + 20,
  }));
  translated.templateDatum = { vertexIndex: 3, x: 10, y: 80 };
  translated.features = [];
  const translatedUnfold = unfold(translated);
  const reapplied = applyFeatureTemplate(
    translated,
    translatedUnfold.panels,
    captured.template
  );
  if (reapplied.features.length !== 4 || reapplied.skipped) {
    throw new Error("translated datum template application");
  }
  const translatedWall = reapplied.features.find((f) => f.panelId === "wall-0");
  if (!translatedWall || Math.abs(translatedWall.cx - 30) > 1e-6) {
    throw new Error("translated profile should preserve wall-local offset");
  }

  // Project JSON normalizes datum, templates, and feature source metadata.
  source.templates = [captured.template];
  source.features.push(...applied.features);
  const loaded = parseDoc(serializeDoc(source));
  if (
    loaded.templates.length !== 1 ||
    loaded.templates[0].entries.length !== 4 ||
    loaded.templateDatum?.vertexIndex !== 3 ||
    !loaded.features.some((f) => f.templateId === captured.template.id)
  ) {
    throw new Error("template project JSON normalization");
  }
  const badDatum = JSON.parse(serializeDoc(source));
  badDatum.templateDatum.vertexIndex = 99;
  if (parseDoc(JSON.stringify(badDatum)).templateDatum !== null) {
    throw new Error("invalid template datum must be cleared on load");
  }

  // A substantially different profile reports entries that no longer land on faces.
  const changed = parseDoc(serializeDoc(source));
  changed.profile.points = [
    { x: 0, y: 0 },
    { x: 50, y: 0 },
    { x: 50, y: 30 },
    { x: 0, y: 30 },
  ];
  changed.templateDatum = { vertexIndex: 3, x: 0, y: 30 };
  changed.features = [];
  const changedUnfold = unfold(changed);
  const invalid = applyFeatureTemplate(changed, changedUnfold.panels, captured.template);
  if (invalid.skipped < 1) throw new Error("invalid template faces should be reported");
}

const meshDiag = buildAssembledCase(doc);
if (meshDiag.error || meshDiag.kind !== "assembled" || meshDiag.faces.length < 6) {
  throw new Error("case viewer should build assembled shell for closed profile");
}
const frontDoc = createDefaultDoc();
const meshFront = buildAssembledCase(frontDoc);
if (meshFront.error || meshFront.kind !== "assembled" || meshFront.faces.length < 6) {
  throw new Error("FRONT profile should produce assembled case viewer geometry");
}

console.log("ok", {
  frontPanels: panels.map((p) => `${p.label} ${p.width}x${p.height}`),
  holes: panels.reduce((n, p) => n + p.holes.length, 0),
  svgBytes: svg.length,
  fingerCount100: fingerCount(100, 20),
  generativePts: rectMale.length,
});
