/** Document model, defaults, clone, validate */

/**
 * Outer contour of the production FRONT panel with individual teeth removed.
 * Drawn Y-down (canvas), mirrored horizontally vs the DXF source so the
 * stepped lip sits on the left. Origin at the bottom-left outer corner (0,0);
 * Y is negative toward the top of the panel (height 30.6 mm). Baseline expanded
 * by the default 3 mm finger depth; X uses 0…246 mm.
 */
export const TOERN_FRONT_PROFILE = [
  { x: 0, y: -30.6 },
  { x: 71.999, y: -30.6 },
  { x: 81.795, y: -13.5 },
  { x: 246, y: -13.5 },
  { x: 246, y: 0 },
  { x: 0, y: 0 },
];

export function createDefaultDoc() {
  return {
    version: 1,
    units: "mm",
    name: "TŒRN M1 Front Profile",
    thickness: 3,
    fingerLength: 14,
    /** Ignore joints when any finger segment would be ≤ this width (mm). */
    minFingerWidth: 5,
    kerf: -0.125,
    /** When true, swap male/female on every joint (ends female, walls male on long edges). */
    invertFingers: true,
    /** Per-panel finger invert (relative to global). Ends share one setting. */
    invertPanels: {},
    depth: 160,
    snap: { enabled: true, grid: 3 },
    /** Laser sheet packing for export / nest preview */
    sheet: {
      width: 600,
      height: 400,
      copies: 1,
      /** Safety margin from sheet outer edge to nearest cut (mm). */
      border: 10,
      /** Gap between panels and between copies (mm). */
      gap: 8,
      /** Cycles alternate near-optimal nest layouts ("Try new layout"). */
      layoutSeed: 0,
    },
    // Editable profile derived from the nominal FRONT panel outer contour.
    profile: {
      points: cloneData(TOERN_FRONT_PROFILE),
      closed: true,
    },
    fixedPanels: null,
    features: [],
    templateDatum: null,
    templates: [],
  };
}

function cloneData(value) {
  return JSON.parse(JSON.stringify(value));
}

export function cloneDoc(doc) {
  return JSON.parse(JSON.stringify(doc));
}

export function uid(prefix = "id") {
  return `${prefix}-${Math.random().toString(36).slice(2, 9)}`;
}

export function normalizeDoc(raw) {
  const base = createDefaultDoc();
  if (!raw || typeof raw !== "object") return base;

  const doc = cloneDoc(base);
  if (typeof raw.name === "string") doc.name = raw.name;
  if (typeof raw.thickness === "number") doc.thickness = raw.thickness;
  if (typeof raw.fingerLength === "number") doc.fingerLength = Math.max(10, raw.fingerLength);
  if (typeof raw.minFingerWidth === "number") {
    doc.minFingerWidth = Math.max(0.5, Math.min(50, raw.minFingerWidth));
  }
  if (typeof raw.kerf === "number") doc.kerf = raw.kerf;
  if (typeof raw.invertFingers === "boolean") doc.invertFingers = raw.invertFingers;
  if (raw.invertPanels && typeof raw.invertPanels === "object") {
    doc.invertPanels = {};
    for (const [id, v] of Object.entries(raw.invertPanels)) {
      if (v) doc.invertPanels[String(id)] = true;
    }
    // Ends are the same blank — keep flags synced.
    if (doc.invertPanels.endA || doc.invertPanels.endB) {
      doc.invertPanels.endA = true;
      doc.invertPanels.endB = true;
    }
  }
  if (typeof raw.depth === "number") doc.depth = raw.depth;
  if (raw.snap && typeof raw.snap === "object") {
    if (typeof raw.snap.enabled === "boolean") doc.snap.enabled = raw.snap.enabled;
    if (typeof raw.snap.grid === "number") doc.snap.grid = raw.snap.grid;
  }
  if (raw.sheet && typeof raw.sheet === "object") {
    if (typeof raw.sheet.width === "number") doc.sheet.width = Math.max(1, raw.sheet.width);
    if (typeof raw.sheet.height === "number") doc.sheet.height = Math.max(1, raw.sheet.height);
    if (typeof raw.sheet.copies === "number") {
      doc.sheet.copies = Math.max(1, Math.min(200, Math.floor(raw.sheet.copies)));
    }
    if (typeof raw.sheet.border === "number") doc.sheet.border = Math.max(0, raw.sheet.border);
    if (typeof raw.sheet.gap === "number") doc.sheet.gap = Math.max(0, raw.sheet.gap);
    if (typeof raw.sheet.layoutSeed === "number") {
      doc.sheet.layoutSeed = Math.max(0, Math.floor(raw.sheet.layoutSeed));
    }
  }
  if (raw.profile && Array.isArray(raw.profile.points)) {
    doc.profile.points = raw.profile.points
      .filter((p) => p && typeof p.x === "number" && typeof p.y === "number")
      .map((p) => ({ x: p.x, y: p.y }));
    doc.profile.closed = raw.profile.closed !== false && doc.profile.points.length >= 3;
  }
  if (Array.isArray(raw.fixedPanels)) {
    doc.fixedPanels = raw.fixedPanels.length ? cloneData(raw.fixedPanels) : null;
  } else if (raw.fixedPanels === null) {
    doc.fixedPanels = null;
  }
  if (Array.isArray(raw.features)) {
    doc.features = raw.features
      .map((f) => normalizeFeature(f))
      .filter(Boolean);
  }
  if (raw.templateDatum && typeof raw.templateDatum === "object") {
    const vertexIndex = Number(raw.templateDatum.vertexIndex);
    if (
      Number.isInteger(vertexIndex) &&
      vertexIndex >= 0 &&
      vertexIndex < doc.profile.points.length
    ) {
      const p = doc.profile.points[vertexIndex];
      doc.templateDatum = { vertexIndex, x: p.x, y: p.y };
    }
  }
  if (Array.isArray(raw.templates)) {
    doc.templates = raw.templates.map(normalizeTemplate).filter(Boolean);
  }
  return doc;
}

function normalizeFeature(f) {
  if (!f || typeof f !== "object" || !f.panelId) return null;
  if (f.type === "circle") {
    const feature = {
      id: f.id || uid("feat"),
      type: "circle",
      panelId: String(f.panelId),
      cx: Number(f.cx) || 0,
      cy: Number(f.cy) || 0,
      r: Math.max(0.1, Number(f.r) || 1),
    };
    if (f.templateId) feature.templateId = String(f.templateId);
    return feature;
  }
  if (f.type === "roundRect") {
    const feature = {
      id: f.id || uid("feat"),
      type: "roundRect",
      panelId: String(f.panelId),
      x: Number(f.x) || 0,
      y: Number(f.y) || 0,
      w: Math.max(0.1, Number(f.w) || 1),
      h: Math.max(0.1, Number(f.h) || 1),
      r: Math.max(0, Math.min(100, Number(f.r) || 0)), // percent of shorter side
    };
    if (f.templateId) feature.templateId = String(f.templateId);
    return feature;
  }
  if (f.type === "text") {
    const feature = {
      id: f.id || uid("feat"),
      type: "text",
      panelId: String(f.panelId),
      x: Number(f.x) || 0,
      y: Number(f.y) || 0,
      text: String(f.text ?? "TEXT").slice(0, 120),
      size: Math.max(0.5, Number(f.size) || 8),
      letterSpacing: Number.isFinite(Number(f.letterSpacing)) ? Number(f.letterSpacing) : 0,
      rotation: Number.isFinite(Number(f.rotation)) ? Number(f.rotation) : 0,
    };
    if (f.templateId) feature.templateId = String(f.templateId);
    return feature;
  }
  return null;
}

function normalizeTemplate(raw) {
  if (!raw || typeof raw !== "object" || !Array.isArray(raw.entries)) return null;
  const entries = raw.entries.map(normalizeTemplateEntry).filter(Boolean);
  if (!entries.length) return null;
  return {
    id: raw.id ? String(raw.id) : uid("tpl"),
    name: String(raw.name || "Hole template").slice(0, 120),
    createdAt: Number(raw.createdAt) || Date.now(),
    entries,
  };
}

function normalizeTemplateEntry(raw) {
  if (!raw || typeof raw !== "object") return null;
  if (raw.type !== "circle" && raw.type !== "roundRect") return null;
  const face = raw.face;
  if (face !== "endA" && face !== "endB" && face !== "wall") return null;
  const entry = {
    type: raw.type,
    face,
    dx: Number(raw.dx) || 0,
    dy: Number(raw.dy) || 0,
  };
  if (raw.frame === "outer") entry.frame = "outer";
  if (typeof raw.z === "number" && Number.isFinite(raw.z)) entry.z = raw.z;
  if (face === "wall") {
    if (entry.z == null) entry.z = 0;
    const nx = Number(raw.normal?.x);
    const ny = Number(raw.normal?.y);
    if (!Number.isFinite(nx) || !Number.isFinite(ny)) return null;
    entry.normal = { x: nx, y: ny };
  }
  if (raw.type === "circle") {
    entry.r = Math.max(0.1, Number(raw.r) || 1);
  } else {
    entry.w = Math.max(0.1, Number(raw.w) || 1);
    entry.h = Math.max(0.1, Number(raw.h) || 1);
    entry.r = Math.max(0, Math.min(100, Number(raw.r) || 0));
  }
  return entry;
}

export function serializeDoc(doc) {
  return JSON.stringify(doc, null, 2);
}

export function parseDoc(text) {
  return normalizeDoc(JSON.parse(text));
}

export function snapValue(v, enabled, grid) {
  if (!enabled || !grid || grid <= 0) return v;
  return Math.round(v / grid) * grid;
}

export function snapPoint(p, snap) {
  return {
    x: snapValue(p.x, snap.enabled, snap.grid),
    y: snapValue(p.y, snap.enabled, snap.grid),
  };
}

export function profileEdgeCount(doc) {
  const pts = doc.profile.points;
  if (!doc.profile.closed || pts.length < 3) return 0;
  return pts.length;
}

export function panelIdsForDoc(doc) {
  if (doc.fixedPanels?.length) {
    return doc.fixedPanels.map((p) => p.id);
  }
  const n = profileEdgeCount(doc);
  if (n < 3) return [];
  const ids = ["endA", "endB"];
  for (let i = 0; i < n; i++) ids.push(`wall-${i}`);
  return ids;
}

/** True when this panel’s fingers are inverted vs the global default. */
export function isPanelFingersInverted(doc, panelId) {
  if (!doc?.invertPanels) return false;
  if (panelId === "endA" || panelId === "endB") {
    return !!(doc.invertPanels.endA || doc.invertPanels.endB);
  }
  return !!doc.invertPanels[panelId];
}

/** Toggle/set per-panel invert. End A/B stay linked (identical blanks). */
export function setPanelFingersInverted(doc, panelId, inverted) {
  if (!doc.invertPanels || typeof doc.invertPanels !== "object") doc.invertPanels = {};
  const on = !!inverted;
  if (panelId === "endA" || panelId === "endB") {
    if (on) {
      doc.invertPanels.endA = true;
      doc.invertPanels.endB = true;
    } else {
      delete doc.invertPanels.endA;
      delete doc.invertPanels.endB;
    }
    return;
  }
  if (on) doc.invertPanels[panelId] = true;
  else delete doc.invertPanels[panelId];
}

/**
 * Male flag for a panel edge that mates with `mateId`.
 * Global invert and per-panel invert (XOR across the joint) stay complementary.
 */
export function jointStartMale(doc, baseMale, panelId, mateId) {
  let m = !!baseMale;
  if (doc.invertFingers) m = !m;
  if (isPanelFingersInverted(doc, panelId) !== isPanelFingersInverted(doc, mateId)) {
    m = !m;
  }
  return m;
}

/** Effective invert mode for clearance / diagonal style on a joint. */
export function jointInvertMode(doc, panelId, mateId) {
  const panelXor =
    isPanelFingersInverted(doc, panelId) !== isPanelFingersInverted(doc, mateId);
  return !!doc.invertFingers !== panelXor;
}

export function panelLabel(panelId, doc) {
  if (doc?.fixedPanels?.length) {
    const p = doc.fixedPanels.find((x) => x.id === panelId);
    if (p?.label) return p.label;
  }
  if (panelId === "endA") return "End A";
  if (panelId === "endB") return "End B";
  const m = /^wall-(\d+)$/.exec(panelId);
  if (m) return `Wall ${Number(m[1]) + 1}`;
  return panelId;
}

export function clearFixedPanels(doc) {
  doc.fixedPanels = null;
}

export function restoreToernFrontProfile(doc) {
  const fresh = createDefaultDoc();
  doc.name = fresh.name;
  doc.thickness = fresh.thickness;
  doc.fingerLength = fresh.fingerLength;
  doc.minFingerWidth = fresh.minFingerWidth;
  doc.kerf = fresh.kerf;
  doc.invertFingers = fresh.invertFingers;
  doc.invertPanels = {};
  doc.depth = fresh.depth;
  doc.profile = cloneData(fresh.profile);
  doc.fixedPanels = null;
  doc.features = [];
  doc.templateDatum = null;
  doc.templates = [];
}
