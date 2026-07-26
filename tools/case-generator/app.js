import {
  createDefaultDoc,
  cloneDoc,
  normalizeDoc,
  serializeDoc,
  parseDoc,
  snapPoint,
  snapValue,
  panelIdsForDoc,
  panelLabel,
  clearFixedPanels,
  restoreToernFrontProfile,
} from "./model.js";
import { dist, simplifyPolyline, polygonSelfIntersects, isAxisAligned } from "./joints.js";
import { unfold, panelBounds } from "./unfold.js";
import { exportSvg, buildCutModel } from "./svg-export.js";
import {
  createCircleFeature,
  createRoundRectFeature,
  hitTestFeature,
  roundRectRadiusMm,
} from "./features.js";
import {
  buildAssembledCase,
  createViewerCamera,
  fitViewerCamera,
  drawAssembledCase,
} from "./case-viewer.js";
import {
  currentTemplateDatum,
  captureFeatureTemplate,
  applyFeatureTemplate,
} from "./feature-templates.js";

// —— State ——
let doc = createDefaultDoc();
let mode = "profile"; // profile | features | view3d | nest
let selectedVertex = -1;
let selectedFeatureId = null;
let featureTool = "circle"; // circle | roundRect | select
let selectedPanelId = "panel-0";
let hoverWorld = null;
let hoverRaw = null;
let shiftDown = false;
let dragging = null; // { type, ... }
let settingTemplateDatum = false;
let templateLibrary = [];

const TEMPLATE_LIBRARY_KEY = "toern.case-generator.templates.v1";

const view = {
  scale: 4, // px per mm (base), multiplied by zoom
  zoom: 1,
  panX: 40,
  panY: 40,
};

/** Orbit camera for Case Viewer */
const cam3d = createViewerCamera();
let highlightPanelId = null;
/** Opacity of non-focused panels in the viewer (0–1). */
let otherPanelsOpacity = 1;

/** Coalesced paint — avoids overlapping canvas/mesh rebuild races */
let paintRaf = 0;
let meshCache = { key: "", mesh: null };

const undoStack = [];
const redoStack = [];
const MAX_UNDO = 80;

// —— DOM ——
const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
const stage = document.getElementById("stage");
const modeHint = document.getElementById("mode-hint");

const els = {
  thickness: document.getElementById("param-thickness"),
  finger: document.getElementById("param-finger"),
  minFinger: document.getElementById("param-min-finger"),
  kerf: document.getElementById("param-kerf"),
  depth: document.getElementById("param-depth"),
  invertFingers: document.getElementById("param-invert-fingers"),
  snapEnabled: document.getElementById("snap-enabled"),
  snapGrid: document.getElementById("snap-grid"),
  zoom: document.getElementById("view-zoom"),
  zoomVal: document.getElementById("view-zoom-val"),
  featurePanel: document.getElementById("feature-panel"),
  featR: document.getElementById("feat-r"),
  featW: document.getElementById("feat-w"),
  featH: document.getElementById("feat-h"),
  featRR: document.getElementById("feat-rr"),
  nestLabels: document.getElementById("nest-labels"),
  view3dPanel: document.getElementById("view3d-panel"),
  view3dGhost: document.getElementById("view3d-ghost"),
  view3dGhostVal: document.getElementById("view3d-ghost-val"),
  fileJson: document.getElementById("file-json"),
  templateOriginStatus: document.getElementById("template-origin-status"),
  templateName: document.getElementById("template-name"),
  templateSelect: document.getElementById("template-select"),
  templateStatus: document.getElementById("template-status"),
};

function pxPerMm() {
  return view.scale * view.zoom;
}

function worldToScreen(p) {
  return {
    x: p.x * pxPerMm() + view.panX,
    y: p.y * pxPerMm() + view.panY,
  };
}

function screenToWorld(sx, sy) {
  return {
    x: (sx - view.panX) / pxPerMm(),
    y: (sy - view.panY) / pxPerMm(),
  };
}

function resizeCanvas() {
  const rect = stage.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  const w = Math.max(1, Math.floor(rect.width * dpr));
  const h = Math.max(1, Math.floor(rect.height * dpr));
  if (canvas.width !== w || canvas.height !== h) {
    canvas.width = w;
    canvas.height = h;
  }
  canvas.style.width = `${rect.width}px`;
  canvas.style.height = `${rect.height}px`;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  render();
}

// —— Undo ——
function pushUndo() {
  undoStack.push(cloneDoc(doc));
  if (undoStack.length > MAX_UNDO) undoStack.shift();
  redoStack.length = 0;
  updateUndoButtons();
}

function undo() {
  if (!undoStack.length) return;
  redoStack.push(cloneDoc(doc));
  doc = undoStack.pop();
  syncParamsFromDoc();
  refreshInspectors();
  render();
  updateUndoButtons();
}

function redo() {
  if (!redoStack.length) return;
  undoStack.push(cloneDoc(doc));
  doc = redoStack.pop();
  syncParamsFromDoc();
  refreshInspectors();
  render();
  updateUndoButtons();
}

function updateUndoButtons() {
  document.getElementById("btn-undo").disabled = !undoStack.length;
  document.getElementById("btn-redo").disabled = !redoStack.length;
}

// —— Params sync ——
function syncParamsFromDoc() {
  els.thickness.value = doc.thickness;
  els.finger.value = doc.fingerLength;
  if (els.minFinger) els.minFinger.value = doc.minFingerWidth ?? 5;
  els.kerf.value = doc.kerf;
  els.depth.value = doc.depth;
  if (els.invertFingers) els.invertFingers.checked = !!doc.invertFingers;
  els.snapEnabled.checked = doc.snap.enabled;
  els.snapGrid.value = doc.snap.grid;
}

function num(v, fallback) {
  const n = Number(v);
  return Number.isFinite(n) ? n : fallback;
}

function snapW(p) {
  return snapPoint(p, doc.snap);
}

/** Optional Shift axis-lock from `from` toward `to`, then snap. */
function placePoint(from, to, axisLock = shiftDown) {
  if (!from || !axisLock) return snapW(to);
  const dx = Math.abs(to.x - from.x);
  const dy = Math.abs(to.y - from.y);
  if (dx >= dy) return snapW({ x: to.x, y: from.y });
  return snapW({ x: from.x, y: to.y });
}

function pointOnSegment(p, a, b, tol) {
  const abx = b.x - a.x;
  const aby = b.y - a.y;
  const len = Math.hypot(abx, aby);
  if (len < 1e-9) return dist(p, a) <= tol ? { t: 0, point: { ...a }, dist: dist(p, a) } : null;
  const t = Math.max(0, Math.min(1, ((p.x - a.x) * abx + (p.y - a.y) * aby) / (len * len)));
  const point = { x: a.x + abx * t, y: a.y + aby * t };
  const d = dist(p, point);
  if (d > tol) return null;
  return { t, point, dist: d };
}

/** Closest edge under pointer. Returns { index, t, point, a, b } or null. */
function hitTestEdge(world, tol) {
  const pts = doc.profile.points;
  const n = pts.length;
  if (n < 2) return null;
  const count = doc.profile.closed ? n : n - 1;
  let best = null;
  for (let i = 0; i < count; i++) {
    const a = pts[i];
    const b = pts[(i + 1) % n];
    const hit = pointOnSegment(world, a, b, tol);
    if (!hit) continue;
    if (hit.t < 0.04 || hit.t > 0.96) continue;
    if (!best || hit.dist < best.dist) {
      best = { index: i, t: hit.t, point: hit.point, a, b, dist: hit.dist };
    }
  }
  return best;
}

function edgeMidpoints() {
  const pts = doc.profile.points;
  const n = pts.length;
  if (n < 2) return [];
  const count = doc.profile.closed ? n : n - 1;
  const mids = [];
  for (let i = 0; i < count; i++) {
    const a = pts[i];
    const b = pts[(i + 1) % n];
    if (dist(a, b) < 1e-3) continue;
    mids.push({
      index: i,
      point: { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 },
    });
  }
  return mids;
}

/**
 * Insert a single point on an edge. Returns new vertex index, or -1 if not needed.
 */
function insertEdgePoint(edgeIndex, world) {
  const pts = doc.profile.points;
  const n = pts.length;
  const a = pts[edgeIndex];
  const b = pts[(edgeIndex + 1) % n];
  const hit = pointOnSegment(world, a, b, dist(a, b) + 1) || { point: world, t: 0.5 };
  const p = snapW(hit.point);
  const eps = Math.max(0.05, (doc.snap.grid || 1) * 0.05);
  if (dist(p, a) <= eps || dist(p, b) <= eps) return -1;
  // Project: if after snap still essentially on the line between a-b and
  // coincides with an existing point, skip
  for (const q of pts) {
    if (dist(p, q) <= eps) return -1;
  }
  pts.splice(edgeIndex + 1, 0, p);
  if (doc.templateDatum && edgeIndex + 1 <= doc.templateDatum.vertexIndex) {
    doc.templateDatum.vertexIndex++;
  }
  return edgeIndex + 1;
}

function syncDatumToProfile() {
  const index = doc.templateDatum?.vertexIndex;
  const point = Number.isInteger(index) ? doc.profile.points[index] : null;
  if (!point) {
    doc.templateDatum = null;
    return;
  }
  doc.templateDatum.x = point.x;
  doc.templateDatum.y = point.y;
}

function deleteSelectedVertex() {
  if (selectedVertex < 0) return;
  const pts = doc.profile.points;
  if (doc.profile.closed && pts.length <= 3) {
    alert("A closed profile needs at least 3 corners.");
    return;
  }
  if (!doc.profile.closed && pts.length <= 1) {
    pushUndo();
    pts.length = 0;
    doc.templateDatum = null;
    selectedVertex = -1;
    refreshProfileUi();
    return;
  }
  pushUndo();
  clearFixedPanels(doc);
  if (doc.templateDatum) {
    if (doc.templateDatum.vertexIndex === selectedVertex) {
      doc.templateDatum = null;
    } else if (doc.templateDatum.vertexIndex > selectedVertex) {
      doc.templateDatum.vertexIndex--;
    }
  }
  pts.splice(selectedVertex, 1);
  if (pts.length < 3) doc.profile.closed = false;
  selectedVertex = pts.length ? Math.min(selectedVertex, pts.length - 1) : -1;
  dragging = null;
  refreshProfileUi();
}

// —— Modes ——
function updateModeHint() {
  if (mode === "profile" && settingTemplateDatum) {
    modeHint.textContent = "Click a profile corner to set the template assembly origin (0,0,0).";
    return;
  }
  const hints = {
    profile: !doc.profile.closed
      ? doc.profile.points.length === 0
        ? "Click to place corners (free / diagonal OK). Hold Shift for H/V. Click the first point to close."
        : "Click to add corners. Hold Shift for H/V. Click first point / Close / Enter to finish. Esc cancels."
      : "Drag corners freely. Click edge or + to add one point. Delete removes selection. Simplify runs on close/generate.",
    features: "Select a panel, choose Circle or Round rect, click to place. Use Select to edit.",
    view3d: "Case Viewer — drag to orbit · scroll to zoom · Fit frames the assembled case.",
    nest: doc.fixedPanels?.length
      ? "Imported fixed-panel nest. Pan with Space/Alt-drag, scroll to zoom."
      : "Flat nest of cut panels. Pan with Space/Alt-drag, scroll to zoom. Export SVG when ready.",
  };
  modeHint.textContent = hints[mode] || "";
}

function setMode(next) {
  const changed = mode !== next;
  mode = next;
  document.querySelectorAll(".cg-mode").forEach((btn) => {
    btn.classList.toggle("is-active", btn.dataset.mode === mode);
  });
  document.querySelectorAll(".cg-panel").forEach((panel) => {
    panel.hidden = panel.dataset.panel !== mode;
  });
  updateModeHint();
  // Only clear selection when switching modes — not on every edit refresh
  if (changed) {
    selectedVertex = -1;
    selectedFeatureId = null;
    dragging = null;
    if (next !== "profile") settingTemplateDatum = false;
  }
  refreshInspectors();
  if (changed && (mode === "nest" || mode === "features" || mode === "view3d")) fitView();
  else render();
}

function refreshProfileUi() {
  updateModeHint();
  refreshInspectors();
  render();
}

function loadTemplateLibrary() {
  try {
    const raw = JSON.parse(localStorage.getItem(TEMPLATE_LIBRARY_KEY) || "[]");
    return normalizeDoc({ templates: Array.isArray(raw) ? raw : [] }).templates;
  } catch {
    return [];
  }
}

function persistTemplateLibrary() {
  try {
    localStorage.setItem(TEMPLATE_LIBRARY_KEY, JSON.stringify(templateLibrary));
  } catch {
    // Private browsing / storage denial: project JSON still retains templates.
  }
}

function mergeTemplateLibraries(...libraries) {
  const merged = new Map();
  for (const library of libraries) {
    for (const template of library || []) merged.set(template.id, template);
  }
  return [...merged.values()].sort((a, b) => a.name.localeCompare(b.name));
}

function addTemplateToProject(template) {
  doc.templates = mergeTemplateLibraries(doc.templates, [template]);
}

function syncTemplateUi() {
  const datum = currentTemplateDatum(doc);
  const unavailable = !!doc.fixedPanels?.length || !doc.profile.closed;
  const originButton = document.getElementById("btn-set-template-origin");
  originButton.disabled = unavailable;
  originButton.textContent = settingTemplateDatum ? "Click a profile corner…" : "Set template origin";
  els.templateOriginStatus.textContent = datum
    ? `Vertex #${datum.vertexIndex + 1} · assembly origin ${fmt(datum.x)}, ${fmt(datum.y)}, 0`
    : unavailable
      ? "Available for a closed generated profile."
      : "Not set. Choose a profile corner as assembly (0,0,0).";

  const previous = els.templateSelect.value;
  els.templateSelect.innerHTML = "";
  if (!templateLibrary.length) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = "(no saved templates)";
    els.templateSelect.appendChild(option);
  } else {
    for (const template of templateLibrary) {
      const option = document.createElement("option");
      option.value = template.id;
      option.textContent = `${template.name} (${template.entries.length})`;
      els.templateSelect.appendChild(option);
    }
    els.templateSelect.value = templateLibrary.some((t) => t.id === previous)
      ? previous
      : templateLibrary[0].id;
  }
  document.getElementById("btn-save-template").disabled = unavailable || !datum || !doc.features.length;
  document.getElementById("btn-apply-template").disabled = unavailable || !datum || !templateLibrary.length;
  document.getElementById("btn-delete-template").disabled = !templateLibrary.length;
}

// —— Inspectors ——
function refreshInspectors() {
  refreshVertexList();
  refreshFeaturePanelSelect();
  refreshFeatureList();
  refreshNestList();
  refreshView3dPanelList();
  refreshVertexEdit();
  refreshFeatureEdit();
  syncTemplateUi();
}

function refreshVertexList() {
  const ul = document.getElementById("vertex-list");
  ul.innerHTML = "";
  doc.profile.points.forEach((p, i) => {
    const li = document.createElement("li");
    li.className = i === selectedVertex ? "is-active" : "";
    const datum = doc.templateDatum?.vertexIndex === i ? " · origin" : "";
    li.innerHTML = `<span>#${i + 1}${datum}</span><span class="cg-muted">${fmt(p.x)}, ${fmt(p.y)}</span>`;
    li.addEventListener("click", () => {
      selectedVertex = i;
      refreshInspectors();
      render();
    });
    ul.appendChild(li);
  });
}

function refreshVertexEdit() {
  const box = document.getElementById("vertex-edit");
  if (selectedVertex < 0 || selectedVertex >= doc.profile.points.length) {
    box.hidden = true;
    return;
  }
  box.hidden = false;
  const p = doc.profile.points[selectedVertex];
  document.getElementById("vtx-x").value = p.x;
  document.getElementById("vtx-y").value = p.y;
}

function refreshFeaturePanelSelect() {
  const ids = panelIdsForDoc(doc);
  const sel = els.featurePanel;
  const prev = selectedPanelId;
  sel.innerHTML = "";
  if (!ids.length) {
    sel.innerHTML = `<option value="">(close profile first)</option>`;
    selectedPanelId = "";
    return;
  }
  for (const id of ids) {
    const opt = document.createElement("option");
    opt.value = id;
    opt.textContent = panelLabel(id, doc);
    sel.appendChild(opt);
  }
  selectedPanelId = ids.includes(prev) ? prev : ids[0];
  sel.value = selectedPanelId;
}

function refreshFeatureList() {
  const ul = document.getElementById("feature-list");
  ul.innerHTML = "";
  const feats = doc.features.filter((f) => f.panelId === selectedPanelId);
  if (!feats.length) {
    const li = document.createElement("li");
    li.innerHTML = `<span class="cg-muted">None</span>`;
    li.style.cursor = "default";
    ul.appendChild(li);
    return;
  }
  for (const f of feats) {
    const li = document.createElement("li");
    li.className = f.id === selectedFeatureId ? "is-active" : "";
    const detail =
      f.type === "circle"
        ? `⌀ ${fmt(f.r * 2)}`
        : `${fmt(f.w)}×${fmt(f.h)} r${fmt(f.r)}%`;
    li.innerHTML = `<span>${f.type}</span><span class="cg-muted">${detail}</span>`;
    li.addEventListener("click", () => {
      selectedFeatureId = f.id;
      featureTool = "select";
      syncFeatureToolButtons();
      refreshInspectors();
      render();
    });
    ul.appendChild(li);
  }
}

function refreshFeatureEdit() {
  const box = document.getElementById("feature-edit");
  const fields = document.getElementById("feature-edit-fields");
  const f = doc.features.find((x) => x.id === selectedFeatureId);
  if (!f) {
    box.hidden = true;
    fields.innerHTML = "";
    return;
  }
  box.hidden = false;
  if (f.type === "circle") {
    fields.innerHTML = `
      <label class="cg-metric" for="edit-cx"><span>CX</span><input type="number" id="edit-cx" step="0.1" value="${f.cx}" /></label>
      <label class="cg-metric" for="edit-cy"><span>CY</span><input type="number" id="edit-cy" step="0.1" value="${f.cy}" /></label>
      <label class="cg-metric" for="edit-r"><span>R</span><input type="number" id="edit-r" min="0.1" step="0.1" value="${f.r}" /></label>
    `;
    bindEdit(["edit-cx", "edit-cy", "edit-r"], () => {
      pushUndo();
      f.cx = num(document.getElementById("edit-cx").value, f.cx);
      f.cy = num(document.getElementById("edit-cy").value, f.cy);
      f.r = Math.max(0.1, num(document.getElementById("edit-r").value, f.r));
      refreshFeatureList();
      render();
    });
  } else {
    fields.innerHTML = `
      <label class="cg-metric" for="edit-x"><span>X</span><input type="number" id="edit-x" step="0.1" value="${f.x}" /></label>
      <label class="cg-metric" for="edit-y"><span>Y</span><input type="number" id="edit-y" step="0.1" value="${f.y}" /></label>
      <label class="cg-metric" for="edit-w"><span>W</span><input type="number" id="edit-w" min="0.1" step="0.1" value="${f.w}" /></label>
      <label class="cg-metric" for="edit-h"><span>H</span><input type="number" id="edit-h" min="0.1" step="0.1" value="${f.h}" /></label>
      <label class="cg-metric" for="edit-rr"><span>R %</span><input type="number" id="edit-rr" min="0" max="100" step="1" value="${f.r}" /></label>
    `;
    bindEdit(["edit-x", "edit-y", "edit-w", "edit-h", "edit-rr"], () => {
      pushUndo();
      f.x = num(document.getElementById("edit-x").value, f.x);
      f.y = num(document.getElementById("edit-y").value, f.y);
      f.w = Math.max(0.1, num(document.getElementById("edit-w").value, f.w));
      f.h = Math.max(0.1, num(document.getElementById("edit-h").value, f.h));
      f.r = Math.max(0, Math.min(100, num(document.getElementById("edit-rr").value, f.r)));
      refreshFeatureList();
      render();
    });
  }
}

function bindEdit(ids, onChange) {
  for (const id of ids) {
    const el = document.getElementById(id);
    if (!el) continue;
    el.addEventListener("change", onChange);
  }
}

function refreshNestList() {
  const ul = document.getElementById("nest-list");
  ul.innerHTML = "";
  const { panels, error } = unfold(doc);
  if (error) {
    const li = document.createElement("li");
    li.innerHTML = `<span class="cg-muted">${error}</span>`;
    ul.appendChild(li);
    return;
  }
  for (const p of panels) {
    const b = panelBounds(p);
    const li = document.createElement("li");
    li.innerHTML = `<span>${p.label}</span><span class="cg-muted">${fmt(b.width)}×${fmt(b.height)}</span>`;
    li.style.cursor = "default";
    ul.appendChild(li);
  }
}

function refreshView3dPanelList() {
  const sel = els.view3dPanel;
  const ul = document.getElementById("view3d-list");
  if (!sel || !ul) return;

  const mesh = getMesh3d();
  const panels = mesh && !mesh.error ? mesh.panels || [] : [];

  const prev = highlightPanelId || "";
  sel.innerHTML = `<option value="">Whole case</option>`;
  for (const p of panels) {
    const opt = document.createElement("option");
    opt.value = p.id;
    opt.textContent = p.label || p.id;
    sel.appendChild(opt);
  }
  if (prev && panels.some((p) => p.id === prev)) {
    sel.value = prev;
    highlightPanelId = prev;
  } else {
    sel.value = "";
    highlightPanelId = null;
  }

  ul.innerHTML = "";
  for (const p of panels) {
    const li = document.createElement("li");
    li.className = p.id === highlightPanelId ? "is-active" : "";
    li.innerHTML = `<span>${p.label || p.id}</span><span class="cg-muted">${p.kind || ""}</span>`;
    li.style.cursor = "pointer";
    li.addEventListener("click", () => {
      highlightPanelId = highlightPanelId === p.id ? null : p.id;
      if (sel) sel.value = highlightPanelId || "";
      refreshView3dPanelList();
      render();
    });
    ul.appendChild(li);
  }
}

function fmt(n) {
  return String(Math.round(n * 100) / 100);
}

function fmtN(n) {
  return fmt(n);
}

// —— Fit view ——
function fitView() {
  const rect = stage.getBoundingClientRect();
  let minX = 0;
  let minY = 0;
  let maxX = 100;
  let maxY = 60;

  if (mode === "nest") {
    const model = buildCutModel(doc);
    if (model.nest) {
      maxX = model.nest.width;
      maxY = model.nest.height;
    }
  } else if (mode === "view3d") {
    const mesh = getMesh3d();
    if (mesh && !mesh.error) {
      fitViewerCamera(cam3d, mesh.bounds, rect);
    }
    syncZoomFromCam3d();
    render();
    return;
  } else if (mode === "features") {
    const { panels } = unfold(doc);
    const panel = panels.find((p) => p.id === selectedPanelId);
    if (panel) {
      const b = panelBounds(panel);
      maxX = b.width;
      maxY = b.height;
    }
  } else {
    const pts = doc.profile.points;
    if (pts.length) {
      minX = Math.min(...pts.map((p) => p.x));
      minY = Math.min(...pts.map((p) => p.y));
      maxX = Math.max(...pts.map((p) => p.x));
      maxY = Math.max(...pts.map((p) => p.y));
    }
  }

  const w = Math.max(1, maxX - minX);
  const h = Math.max(1, maxY - minY);
  const pad = 40;
  const sx = (rect.width - pad * 2) / w;
  const sy = (rect.height - pad * 2) / h;
  const s = Math.max(0.5, Math.min(sx, sy, 20));
  view.scale = s;
  view.zoom = 1;
  els.zoom.value = "1";
  els.zoomVal.textContent = "100%";
  view.panX = pad - minX * s + (rect.width - pad * 2 - w * s) / 2;
  view.panY = pad - minY * s + (rect.height - pad * 2 - h * s) / 2;
  render();
}

// —— Render ——
/** Schedule a single paint on the next animation frame (coalesces bursts). */
function render() {
  if (paintRaf) return;
  paintRaf = requestAnimationFrame(() => {
    paintRaf = 0;
    paint();
  });
}

function paint() {
  const rect = stage.getBoundingClientRect();
  if (rect.width < 1 || rect.height < 1) return;
  ctx.clearRect(0, 0, rect.width, rect.height);

  if (mode === "view3d") {
    drawView3dMode(rect);
    return;
  }

  drawGrid(rect);

  if (mode === "profile") drawProfileMode(rect);
  else if (mode === "features") drawFeaturesMode(rect);
  else drawNestMode(rect);
}

function drawGrid(rect) {
  const ppm = pxPerMm();
  const grid = doc.snap.grid || 10;
  // Major every 10mm
  const step = grid * ppm;
  if (step < 4) return;

  const origin = worldToScreen({ x: 0, y: 0 });
  ctx.save();
  ctx.strokeStyle = "rgba(255,255,255,0.04)";
  ctx.lineWidth = 1;

  const startX = origin.x % step;
  const startY = origin.y % step;
  ctx.beginPath();
  for (let x = startX; x < rect.width; x += step) {
    ctx.moveTo(x, 0);
    ctx.lineTo(x, rect.height);
  }
  for (let y = startY; y < rect.height; y += step) {
    ctx.moveTo(0, y);
    ctx.lineTo(rect.width, y);
  }
  ctx.stroke();

  // Axes
  ctx.strokeStyle = "rgba(46,185,255,0.35)";
  ctx.beginPath();
  ctx.moveTo(origin.x, 0);
  ctx.lineTo(origin.x, rect.height);
  ctx.moveTo(0, origin.y);
  ctx.lineTo(rect.width, origin.y);
  ctx.stroke();
  ctx.restore();
}

function drawProfileMode() {
  const pts = doc.profile.points;
  const ppm = pxPerMm();
  const rect = stage.getBoundingClientRect();

  if (pts.length === 0) {
    ctx.save();
    ctx.fillStyle = "rgba(139,155,181,0.85)";
    ctx.font = "15px Inter, sans-serif";
    ctx.textAlign = "center";
    ctx.fillText("Click to draw profile", rect.width / 2, rect.height / 2 - 8);
    ctx.font = "12px JetBrains Mono, monospace";
    ctx.fillStyle = "rgba(139,155,181,0.55)";
    ctx.fillText(`snap ${fmtN(doc.snap.grid)} mm · depth ${fmtN(doc.depth)} mm`, rect.width / 2, rect.height / 2 + 14);
    ctx.restore();
    return;
  }

  // Depth ghost: show wall thickness hint as offset preview of extrusion label
  if (pts.length >= 1) {
    ctx.save();
    ctx.fillStyle = "rgba(46,185,255,0.75)";
    ctx.font = "12px JetBrains Mono, monospace";
    const label = worldToScreen({ x: pts[0].x, y: pts[0].y - 8 });
    ctx.fillText(`depth ${fmtN(doc.depth)} mm · t ${fmtN(doc.thickness)} mm`, label.x, label.y);
    ctx.restore();
  }

  if (pts.length >= 2) {
    ctx.save();
    ctx.strokeStyle = "#2eb9ff";
    ctx.lineWidth = 2;
    ctx.beginPath();
    const s0 = worldToScreen(pts[0]);
    ctx.moveTo(s0.x, s0.y);
    for (let i = 1; i < pts.length; i++) {
      const s = worldToScreen(pts[i]);
      ctx.lineTo(s.x, s.y);
    }
    if (doc.profile.closed) ctx.closePath();
    ctx.stroke();

    // Edge length labels + add-point handles
    ctx.fillStyle = "#8b9bb5";
    ctx.font = "11px JetBrains Mono, monospace";
    const count = doc.profile.closed ? pts.length : pts.length - 1;
    for (let i = 0; i < count; i++) {
      const a = pts[i];
      const b = pts[(i + 1) % pts.length];
      if (!doc.profile.closed && i === pts.length - 1) break;
      const mid = worldToScreen({ x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 });
      const len = dist(a, b);
      ctx.fillText(`${fmtN(len)}`, mid.x + 8, mid.y - 8);
    }
    ctx.restore();

    if (doc.profile.closed || pts.length >= 2) {
      for (const mid of edgeMidpoints()) {
        const s = worldToScreen(mid.point);
        const hover =
          hoverWorld &&
          dist(hoverWorld, mid.point) <= 10 / ppm;
        ctx.beginPath();
        ctx.arc(s.x, s.y, hover ? 6 : 4.5, 0, Math.PI * 2);
        ctx.fillStyle = hover ? "#2eb9ff" : "rgba(46,185,255,0.35)";
        ctx.fill();
        ctx.strokeStyle = "#0e1420";
        ctx.lineWidth = 1;
        ctx.stroke();
        ctx.fillStyle = hover ? "#061018" : "rgba(232,238,248,0.9)";
        ctx.font = "bold 11px Inter, sans-serif";
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        ctx.fillText("+", s.x, s.y + 0.5);
      }
      ctx.textAlign = "start";
      ctx.textBaseline = "alphabetic";
    }
  }

  // Rubber band + live length
  if (!doc.profile.closed && pts.length >= 1 && (hoverRaw || hoverWorld)) {
    const last = pts[pts.length - 1];
    const tip = placePoint(last, hoverRaw || hoverWorld, shiftDown);
    const a = worldToScreen(last);
    const b = worldToScreen(tip);
    const len = dist(last, tip);
    const ortho = isAxisAligned(last, tip);
    ctx.save();
    ctx.strokeStyle = ortho ? "rgba(62, 220, 120, 0.9)" : "rgba(46,185,255,0.55)";
    ctx.setLineDash([6, 4]);
    ctx.lineWidth = ortho ? 2 : 1.5;
    ctx.beginPath();
    ctx.moveTo(a.x, a.y);
    ctx.lineTo(b.x, b.y);
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.fillStyle = ortho ? "#3edc78" : "#2eb9ff";
    ctx.font = "12px JetBrains Mono, monospace";
    ctx.fillText(`${fmtN(len)} mm`, (a.x + b.x) / 2 + 8, (a.y + b.y) / 2 - 8);
    ctx.restore();
  }

  // Vertices
  const datumIndex = currentTemplateDatum(doc)?.vertexIndex ?? -1;
  pts.forEach((p, i) => {
    const s = worldToScreen(p);
    if (i === datumIndex) {
      ctx.beginPath();
      ctx.arc(s.x, s.y, 9, 0, Math.PI * 2);
      ctx.strokeStyle = "#3edc78";
      ctx.lineWidth = 2.5;
      ctx.stroke();
    }
    ctx.beginPath();
    ctx.arc(s.x, s.y, i === selectedVertex ? 6 : 4.5, 0, Math.PI * 2);
    ctx.fillStyle = i === selectedVertex ? "#ff3b30" : "#e8eef8";
    ctx.fill();
    ctx.strokeStyle = "#0e1420";
    ctx.lineWidth = 1.5;
    ctx.stroke();
  });

  // Close hint ring
  if (!doc.profile.closed && pts.length >= 3 && hoverWorld) {
    if (dist(hoverWorld, pts[0]) * ppm < 12) {
      const s = worldToScreen(pts[0]);
      ctx.beginPath();
      ctx.arc(s.x, s.y, 10, 0, Math.PI * 2);
      ctx.strokeStyle = "#ff3b30";
      ctx.lineWidth = 2;
      ctx.stroke();
    }
  }
}

function drawFeaturesMode() {
  const { panels, error } = unfold(doc);
  if (error) {
    ctx.fillStyle = "#8b9bb5";
    ctx.font = "14px Inter, sans-serif";
    ctx.fillText(error, 24, 40);
    return;
  }
  const panel = panels.find((p) => p.id === selectedPanelId);
  if (!panel) return;

  // Draw panel outer + holes in world = panel local mm
  drawPanelPaths(panel, 0, 0, true);

  // Nominal outline guide
  ctx.save();
  ctx.strokeStyle = "rgba(255,255,255,0.15)";
  ctx.setLineDash([4, 4]);
  if (panel.nominal.type === "rect") {
    const a = worldToScreen({ x: 0, y: 0 });
    const b = worldToScreen({ x: panel.nominal.w, y: panel.nominal.h });
    ctx.strokeRect(a.x, a.y, b.x - a.x, b.y - a.y);
  }
  ctx.restore();

  // Features from doc (editable positions)
  const feats = doc.features.filter((f) => f.panelId === selectedPanelId);
  for (const f of feats) {
    drawFeatureOverlay(f, f.id === selectedFeatureId);
  }

  // Placement ghost
  if (hoverWorld && (featureTool === "circle" || featureTool === "roundRect")) {
    const ghost =
      featureTool === "circle"
        ? { type: "circle", cx: hoverWorld.x, cy: hoverWorld.y, r: num(els.featR.value, 3) }
        : {
            type: "roundRect",
            x: hoverWorld.x - num(els.featW.value, 12) / 2,
            y: hoverWorld.y - num(els.featH.value, 8) / 2,
            w: num(els.featW.value, 12),
            h: num(els.featH.value, 8),
            r: num(els.featRR.value, 25),
          };
    drawFeatureOverlay(ghost, false, true);
  }

  ctx.fillStyle = "rgba(46,185,255,0.8)";
  ctx.font = "12px JetBrains Mono, monospace";
  const title = worldToScreen({ x: 0, y: -6 });
  ctx.fillText(panel.label, title.x, title.y);
}

function drawFeatureOverlay(f, selected, ghost = false) {
  ctx.save();
  ctx.strokeStyle = ghost ? "rgba(46,185,255,0.5)" : selected ? "#ff3b30" : "#2eb9ff";
  ctx.lineWidth = selected ? 2 : 1.5;
  ctx.setLineDash(ghost ? [4, 3] : []);
  if (f.type === "circle") {
    const c = worldToScreen({ x: f.cx, y: f.cy });
    ctx.beginPath();
    ctx.arc(c.x, c.y, f.r * pxPerMm(), 0, Math.PI * 2);
    ctx.stroke();
  } else if (f.type === "roundRect") {
    const a = worldToScreen({ x: f.x, y: f.y });
    const w = f.w * pxPerMm();
    const h = f.h * pxPerMm();
    const r = roundRectRadiusMm(f.w, f.h, f.r) * pxPerMm();
    roundRectPath(ctx, a.x, a.y, w, h, r);
    ctx.stroke();
  }
  ctx.restore();
}

function roundRectPath(c, x, y, w, h, r) {
  c.beginPath();
  c.moveTo(x + r, y);
  c.arcTo(x + w, y, x + w, y + h, r);
  c.arcTo(x + w, y + h, x, y + h, r);
  c.arcTo(x, y + h, x, y, r);
  c.arcTo(x, y, x + w, y, r);
  c.closePath();
}

function drawPanelPaths(panel, ox, oy, useWorld = true) {
  const toScreen = (x, y) => {
    if (useWorld) return worldToScreen({ x: x + ox, y: y + oy });
    return { x: (x + ox) * pxPerMm() + view.panX, y: (y + oy) * pxPerMm() + view.panY };
  };

  ctx.save();
  ctx.strokeStyle = "#e8eef8";
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  panel.outerPoints.forEach((p, i) => {
    const s = toScreen(p.x, p.y);
    if (i === 0) ctx.moveTo(s.x, s.y);
    else ctx.lineTo(s.x, s.y);
  });
  ctx.closePath();
  ctx.stroke();

  ctx.strokeStyle = "#2eb9ff";
  for (const hole of panel.holes) {
    if (hole.type === "circle") {
      const s = toScreen(hole.cx, hole.cy);
      ctx.beginPath();
      ctx.arc(s.x, s.y, hole.r * pxPerMm(), 0, Math.PI * 2);
      ctx.stroke();
    } else if (hole.type === "roundRect") {
      const s = toScreen(hole.x, hole.y);
      roundRectPath(ctx, s.x, s.y, hole.w * pxPerMm(), hole.h * pxPerMm(), hole.r * pxPerMm());
      ctx.stroke();
    }
  }
  ctx.restore();
}

function drawNestMode() {
  const model = buildCutModel(doc);
  if (model.error || !model.nest) {
    ctx.fillStyle = "#8b9bb5";
    ctx.font = "14px Inter, sans-serif";
    ctx.fillText(model.error || "No nest", 24, 40);
    return;
  }

  const showLabels = els.nestLabels.checked;
  for (const item of model.nest.items) {
    drawPanelPaths(item.panel, item.x, item.y, true);
    if (showLabels) {
      const s = worldToScreen({ x: item.x + 1, y: item.y + 4 });
      ctx.fillStyle = "rgba(139,155,181,0.9)";
      ctx.font = "11px JetBrains Mono, monospace";
      ctx.fillText(item.panel.label, s.x, s.y);
    }
  }
}

function meshCacheKey(d) {
  // Stable enough to skip rebuilds while orbiting / resizing
  return JSON.stringify({
    t: d.thickness,
    f: d.fingerLength,
    mf: d.minFingerWidth ?? 5,
    k: d.kerf,
    d: d.depth,
    inv: !!d.invertFingers,
    closed: d.profile?.closed,
    pts: d.profile?.points,
    feats: d.features,
    fixed: d.fixedPanels?.map((p) => p.id) || null,
  });
}

function getMesh3d() {
  const key = meshCacheKey(doc);
  if (meshCache.key === key && meshCache.mesh) return meshCache.mesh;
  const mesh = buildAssembledCase(doc);
  meshCache = { key, mesh };
  return mesh;
}

function syncZoomFromCam3d() {
  // Slider shows relative zoom vs last Fit (100% = fitted)
  const base = cam3d.fitZoom || cam3d.zoom || 1;
  const z = Math.max(0.25, Math.min(6, (cam3d.zoom || 1) / base));
  view.zoom = z;
  els.zoom.value = String(z);
  els.zoomVal.textContent = `${Math.round(z * 100)}%`;
}

function applyZoomToCam3d() {
  const base = cam3d.fitZoom || cam3d.zoom || 1;
  cam3d.zoom = base * Math.max(0.25, view.zoom);
}

function drawView3dMode(rect) {
  // Warm workshop backdrop
  const bg = ctx.createRadialGradient(
    rect.width * 0.45,
    rect.height * 0.35,
    10,
    rect.width * 0.5,
    rect.height * 0.55,
    Math.max(rect.width, rect.height) * 0.75
  );
  bg.addColorStop(0, "#2a2218");
  bg.addColorStop(0.55, "#16120e");
  bg.addColorStop(1, "#0a0908");
  ctx.fillStyle = bg;
  ctx.fillRect(0, 0, rect.width, rect.height);

  const mesh = getMesh3d();
  if (mesh.error) {
    ctx.fillStyle = "#b8a890";
    ctx.font = "14px Inter, sans-serif";
    ctx.fillText(mesh.error, 24, 40);
    return;
  }

  drawAssembledCase(ctx, mesh, cam3d, rect, {
    highlightPanelId,
    otherOpacity: otherPanelsOpacity,
  });
}

// —— Pointer interaction ——
function canvasPos(e) {
  const rect = canvas.getBoundingClientRect();
  return { x: e.clientX - rect.left, y: e.clientY - rect.top };
}

function onPointerDown(e) {
  if (e.button !== 0 && e.button !== 1) return;
  const sp = canvasPos(e);
  const raw = screenToWorld(sp.x, sp.y);
  const world = snapW(raw);

  if (mode === "view3d") {
    if (e.button === 0 || e.button === 1) {
      dragging = {
        type: "orbit",
        x: sp.x,
        y: sp.y,
        yaw: cam3d.yaw,
        pitch: cam3d.pitch,
      };
      e.preventDefault();
    }
    return;
  }

  if (e.button === 1 || e.altKey || spaceDown) {
    dragging = { type: "pan", x: sp.x, y: sp.y, panX: view.panX, panY: view.panY };
    e.preventDefault();
    return;
  }

  if (mode === "profile") onProfileDown(raw, world);
  else if (mode === "features") onFeaturesDown(world);
}

function onProfileDown(raw, world) {
  const pts = doc.profile.points;
  const hitR = Math.max(3, 12 / pxPerMm());
  const edgeTol = Math.max(4, 14 / pxPerMm());

  if (settingTemplateDatum) {
    const index = pts.findIndex((p) => dist(raw, p) <= hitR * 1.5);
    if (index < 0) return;
    pushUndo();
    doc.templateDatum = { vertexIndex: index, x: pts[index].x, y: pts[index].y };
    settingTemplateDatum = false;
    selectedVertex = index;
    refreshProfileUi();
    return;
  }

  // Hit existing vertex
  for (let i = 0; i < pts.length; i++) {
    if (dist(raw, pts[i]) <= hitR * (i === 0 && !doc.profile.closed && pts.length >= 3 ? 1.5 : 1)) {
      if (i === 0 && !doc.profile.closed && pts.length >= 3) {
        closeProfile();
        return;
      }
      selectedVertex = i;
      dragging = { type: "vertex", index: i };
      pushUndo();
      if (doc.fixedPanels?.length) clearFixedPanels(doc);
      refreshProfileUi();
      return;
    }
  }

  // Click edge / + → insert exactly one point
  if (pts.length >= 2) {
    const edge = hitTestEdge(raw, edgeTol);
    const midHit = edgeMidpoints().find((m) => dist(raw, m.point) <= hitR);
    const target = edge || (midHit ? { index: midHit.index, point: midHit.point } : null);
    if (target) {
      pushUndo();
      clearFixedPanels(doc);
      const idx = insertEdgePoint(target.index, target.point || world);
      if (idx >= 0) {
        selectedVertex = idx;
        dragging = { type: "vertex", index: idx };
      }
      refreshProfileUi();
      return;
    }
  }

  if (doc.profile.closed) {
    selectedVertex = -1;
    refreshProfileUi();
    return;
  }

  // Open polyline: free place (Shift = axis lock)
  pushUndo();
  clearFixedPanels(doc);
  const next =
    pts.length === 0 ? world : placePoint(pts[pts.length - 1], raw, shiftDown);
  if (pts.length && dist(pts[pts.length - 1], next) < 1e-6) return;
  pts.push(next);
  selectedVertex = pts.length - 1;
  refreshProfileUi();
}

function closeProfile() {
  const pts = doc.profile.points;
  if (pts.length < 3) return;
  const oldDatum = currentTemplateDatum(doc);
  const cleaned = simplifyPolyline(pts, { closed: true });
  if (cleaned.length < 3) {
    alert("Need at least 3 distinct corners after cleanup.");
    return;
  }
  if (polygonSelfIntersects(cleaned)) {
    alert("Profile edges cross — adjust the outline before closing.");
    return;
  }
  pushUndo();
  clearFixedPanels(doc);
  doc.profile.points = cleaned;
  doc.profile.closed = true;
  if (oldDatum) {
    const index = cleaned.findIndex((p) => dist(p, oldDatum) < 1e-6);
    doc.templateDatum =
      index >= 0 ? { vertexIndex: index, x: cleaned[index].x, y: cleaned[index].y } : null;
  }
  selectedVertex = -1;
  dragging = null;
  refreshProfileUi();
}

function onFeaturesDown(world) {
  if (!selectedPanelId) return;

  if (featureTool === "select") {
    const feats = doc.features.filter((f) => f.panelId === selectedPanelId);
    const tol = 3 / pxPerMm();
    let hit = null;
    for (let i = feats.length - 1; i >= 0; i--) {
      if (hitTestFeature(feats[i], world.x, world.y, Math.max(tol, 1.5))) {
        hit = feats[i];
        break;
      }
    }
    selectedFeatureId = hit ? hit.id : null;
    if (hit) {
      pushUndo();
      dragging = {
        type: "feature",
        id: hit.id,
        ox: world.x - (hit.type === "circle" ? hit.cx : hit.x),
        oy: world.y - (hit.type === "circle" ? hit.cy : hit.y),
      };
    }
    refreshInspectors();
    render();
    return;
  }

  pushUndo();
  if (featureTool === "circle") {
    const r = Math.max(0.1, num(els.featR.value, 3));
    const f = createCircleFeature(selectedPanelId, world.x, world.y, r);
    doc.features.push(f);
    selectedFeatureId = f.id;
  } else if (featureTool === "roundRect") {
    const w = Math.max(0.1, num(els.featW.value, 12));
    const h = Math.max(0.1, num(els.featH.value, 8));
    const r = Math.max(0, Math.min(100, num(els.featRR.value, 25)));
    const f = createRoundRectFeature(selectedPanelId, world.x - w / 2, world.y - h / 2, w, h, r);
    doc.features.push(f);
    selectedFeatureId = f.id;
  }
  refreshInspectors();
  render();
}

function onPointerMove(e) {
  const sp = canvasPos(e);
  const raw = screenToWorld(sp.x, sp.y);
  hoverRaw = raw;
  hoverWorld = snapW(raw);

  if (dragging?.type === "orbit") {
    const dx = sp.x - dragging.x;
    const dy = sp.y - dragging.y;
    // Grab-the-object: drag right → object turns right (yaw decreases)
    cam3d.yaw = dragging.yaw - dx * 0.01;
    cam3d.pitch = Math.max(
      -Math.PI / 2 + 0.05,
      Math.min(Math.PI / 2 - 0.05, dragging.pitch + dy * 0.01)
    );
    render();
    return;
  }

  if (dragging?.type === "pan") {
    view.panX = dragging.panX + (sp.x - dragging.x);
    view.panY = dragging.panY + (sp.y - dragging.y);
    render();
    return;
  }

  if (dragging?.type === "vertex") {
    const i = dragging.index;
    const pts = doc.profile.points;
    const n = pts.length;
    if (i < 0 || i >= n) return;
    let anchor = null;
    if (shiftDown) {
      // Axis-lock vs previous neighbor when possible
      if (n >= 2) anchor = pts[(i - 1 + n) % n];
    }
    pts[i] = placePoint(anchor, raw, shiftDown && !!anchor);
    if (doc.templateDatum?.vertexIndex === i) syncDatumToProfile();
    refreshVertexList();
    refreshVertexEdit();
    syncTemplateUi();
    render();
    return;
  }

  if (dragging?.type === "feature") {
    const f = doc.features.find((x) => x.id === dragging.id);
    if (f) {
      if (f.type === "circle") {
        f.cx = hoverWorld.x - dragging.ox;
        f.cy = hoverWorld.y - dragging.oy;
      } else {
        f.x = hoverWorld.x - dragging.ox;
        f.y = hoverWorld.y - dragging.oy;
      }
      refreshFeatureEdit();
      render();
    }
    return;
  }

  if (mode === "profile" || mode === "features") render();
}

function onPointerUp() {
  dragging = null;
}

let spaceDown = false;

// —— Events ——
function bindUI() {
  document.querySelectorAll(".cg-mode").forEach((btn) => {
    btn.addEventListener("click", () => setMode(btn.dataset.mode));
  });

  ["thickness", "finger", "minFinger", "kerf", "depth"].forEach((key) => {
    const map = {
      thickness: "thickness",
      finger: "fingerLength",
      minFinger: "minFingerWidth",
      kerf: "kerf",
      depth: "depth",
    };
    const el = els[key];
    if (!el) return;
    el.addEventListener("change", () => {
      pushUndo();
      let v = num(el.value, doc[map[key]]);
      if (key === "finger") {
        v = Math.max(10, v);
        el.value = v;
      }
      if (key === "minFinger") {
        v = Math.max(0.5, Math.min(50, v));
        el.value = v;
      }
      doc[map[key]] = v;
      refreshInspectors();
      render();
    });
  });

  els.invertFingers?.addEventListener("change", () => {
    pushUndo();
    doc.invertFingers = !!els.invertFingers.checked;
    render();
  });

  els.snapEnabled.addEventListener("change", () => {
    doc.snap.enabled = els.snapEnabled.checked;
    render();
  });
  els.snapGrid.addEventListener("change", () => {
    doc.snap.grid = Math.max(0.1, num(els.snapGrid.value, 10));
    render();
  });

  els.zoom.addEventListener("input", () => {
    view.zoom = num(els.zoom.value, 1);
    els.zoomVal.textContent = `${Math.round(view.zoom * 100)}%`;
    if (mode === "view3d") applyZoomToCam3d();
    render();
  });

  els.view3dPanel?.addEventListener("change", () => {
    highlightPanelId = els.view3dPanel.value || null;
    refreshView3dPanelList();
    render();
  });

  els.view3dGhost?.addEventListener("input", () => {
    otherPanelsOpacity = Math.max(0, Math.min(1, num(els.view3dGhost.value, 100) / 100));
    if (els.view3dGhostVal) {
      els.view3dGhostVal.textContent = `${Math.round(otherPanelsOpacity * 100)}%`;
    }
    render();
  });

  document.getElementById("btn-fit").addEventListener("click", fitView);
  document.getElementById("btn-undo").addEventListener("click", undo);
  document.getElementById("btn-redo").addEventListener("click", redo);

  document.getElementById("btn-draw-profile").addEventListener("click", () => {
    pushUndo();
    clearFixedPanels(doc);
    doc.profile.points = [];
    doc.profile.closed = false;
    doc.features = [];
    doc.templateDatum = null;
    settingTemplateDatum = false;
    selectedVertex = -1;
    refreshInspectors();
    setMode("profile");
    fitView();
  });

  document.getElementById("btn-close-profile").addEventListener("click", () => {
    closeProfile();
  });

  document.getElementById("btn-set-template-origin").addEventListener("click", () => {
    if (doc.fixedPanels?.length || !doc.profile.closed) return;
    settingTemplateDatum = !settingTemplateDatum;
    selectedVertex = -1;
    updateModeHint();
    syncTemplateUi();
    render();
  });

  document.getElementById("btn-clear-profile").addEventListener("click", () => {
    pushUndo();
    clearFixedPanels(doc);
    doc.profile.points = [];
    doc.profile.closed = false;
    doc.features = [];
    doc.templateDatum = null;
    settingTemplateDatum = false;
    selectedVertex = -1;
    dragging = null;
    refreshProfileUi();
  });

  document.getElementById("btn-rect-preset").addEventListener("click", () => {
    pushUndo();
    clearFixedPanels(doc);
    doc.profile.points = [
      { x: 0, y: 0 },
      { x: 100, y: 0 },
      { x: 100, y: 60 },
      { x: 0, y: 60 },
    ];
    doc.profile.closed = true;
    doc.features = [];
    doc.templateDatum = null;
    settingTemplateDatum = false;
    selectedVertex = -1;
    dragging = null;
    refreshProfileUi();
    fitView();
  });

  document.getElementById("btn-front-profile").addEventListener("click", () => {
    pushUndo();
    restoreToernFrontProfile(doc);
    syncParamsFromDoc();
    selectedVertex = -1;
    selectedFeatureId = null;
    selectedPanelId = "panel-0";
    dragging = null;
    settingTemplateDatum = false;
    refreshProfileUi();
    fitView();
  });

  document.getElementById("btn-delete-vertex").addEventListener("click", () => {
    deleteSelectedVertex();
  });

  document.getElementById("vtx-x").addEventListener("change", () => {
    if (selectedVertex < 0) return;
    pushUndo();
    clearFixedPanels(doc);
    const pts = doc.profile.points;
    const i = selectedVertex;
    pts[i].x = num(document.getElementById("vtx-x").value, pts[i].x);
    if (doc.templateDatum?.vertexIndex === i) syncDatumToProfile();
    document.getElementById("vtx-x").value = pts[i].x;
    refreshInspectors();
    render();
  });
  document.getElementById("vtx-y").addEventListener("change", () => {
    if (selectedVertex < 0) return;
    pushUndo();
    clearFixedPanels(doc);
    const pts = doc.profile.points;
    const i = selectedVertex;
    pts[i].y = num(document.getElementById("vtx-y").value, pts[i].y);
    if (doc.templateDatum?.vertexIndex === i) syncDatumToProfile();
    document.getElementById("vtx-y").value = pts[i].y;
    refreshInspectors();
    render();
  });

  els.featurePanel.addEventListener("change", () => {
    selectedPanelId = els.featurePanel.value;
    selectedFeatureId = null;
    refreshInspectors();
    fitView();
  });

  document.querySelectorAll("[data-feat]").forEach((btn) => {
    btn.addEventListener("click", () => {
      featureTool = btn.dataset.feat;
      syncFeatureToolButtons();
      // show/hide param groups
      document.getElementById("feat-circle-params").hidden = featureTool !== "circle";
      document.getElementById("feat-rect-params").hidden = featureTool !== "roundRect";
    });
  });

  document.getElementById("btn-delete-feature").addEventListener("click", () => {
    if (!selectedFeatureId) return;
    pushUndo();
    doc.features = doc.features.filter((f) => f.id !== selectedFeatureId);
    selectedFeatureId = null;
    refreshInspectors();
    render();
  });

  document.getElementById("btn-save-template").addEventListener("click", () => {
    const generated = unfold(doc);
    if (generated.error) {
      alert(generated.error);
      return;
    }
    const result = captureFeatureTemplate(doc, generated.panels, els.templateName.value);
    if (result.error) {
      alert(result.error);
      return;
    }
    pushUndo();
    addTemplateToProject(result.template);
    templateLibrary = mergeTemplateLibraries(templateLibrary, [result.template]);
    persistTemplateLibrary();
    els.templateName.value = "";
    els.templateStatus.textContent = `Saved “${result.template.name}” with ${result.template.entries.length} holes${result.skipped ? ` · skipped ${result.skipped}` : ""}.`;
    refreshInspectors();
  });

  document.getElementById("btn-apply-template").addEventListener("click", () => {
    const template = templateLibrary.find((item) => item.id === els.templateSelect.value);
    if (!template) return;
    const generated = unfold(doc);
    if (generated.error) {
      alert(generated.error);
      return;
    }
    const result = applyFeatureTemplate(doc, generated.panels, template);
    if (result.error) {
      alert(result.error);
      return;
    }
    if (!result.features.length) {
      els.templateStatus.textContent = `No holes applied · ${result.skipped} entries do not land on a valid face.`;
      return;
    }
    pushUndo();
    doc.features.push(...result.features);
    addTemplateToProject(template);
    selectedFeatureId = result.features[0].id;
    els.templateStatus.textContent = `Applied ${result.features.length} holes${result.skipped ? ` · skipped ${result.skipped}` : ""}.`;
    refreshInspectors();
    render();
  });

  document.getElementById("btn-delete-template").addEventListener("click", () => {
    const id = els.templateSelect.value;
    if (!id) return;
    pushUndo();
    templateLibrary = templateLibrary.filter((template) => template.id !== id);
    doc.templates = (doc.templates || []).filter((template) => template.id !== id);
    persistTemplateLibrary();
    els.templateStatus.textContent = "Template deleted.";
    refreshInspectors();
  });

  els.nestLabels.addEventListener("change", render);

  document.getElementById("btn-save").addEventListener("click", () => {
    const blob = new Blob([serializeDoc(doc)], { type: "application/json" });
    downloadBlob(blob, "case-design.json");
  });

  document.getElementById("btn-load").addEventListener("click", () => els.fileJson.click());
  els.fileJson.addEventListener("change", async () => {
    const file = els.fileJson.files?.[0];
    if (!file) return;
    try {
      const text = await file.text();
      pushUndo();
      doc = parseDoc(text);
      templateLibrary = mergeTemplateLibraries(templateLibrary, doc.templates);
      persistTemplateLibrary();
      syncParamsFromDoc();
      selectedVertex = -1;
      selectedFeatureId = null;
      settingTemplateDatum = false;
      refreshInspectors();
      fitView();
    } catch (err) {
      alert(`Could not load JSON: ${err.message}`);
    }
    els.fileJson.value = "";
  });

  document.getElementById("btn-export").addEventListener("click", () => {
    try {
      const svg = exportSvg(doc, { labels: false });
      const blob = new Blob([svg], { type: "image/svg+xml" });
      downloadBlob(blob, "case-cut.svg");
    } catch (err) {
      alert(err.message);
    }
  });

  canvas.addEventListener("pointerdown", (e) => {
    canvas.setPointerCapture?.(e.pointerId);
    onPointerDown(e);
  });
  canvas.addEventListener("pointermove", onPointerMove);
  canvas.addEventListener("pointerup", onPointerUp);
  canvas.addEventListener("pointercancel", onPointerUp);
  window.addEventListener("pointermove", onPointerMove);
  window.addEventListener("pointerup", onPointerUp);

  canvas.addEventListener(
    "wheel",
    (e) => {
      e.preventDefault();
      if (mode === "view3d") {
        const factor = e.deltaY > 0 ? 1 / 1.1 : 1.1;
        cam3d.zoom = Math.max(0.05, Math.min(80, (cam3d.zoom || 1) * factor));
        if (!cam3d.fitZoom) cam3d.fitZoom = cam3d.zoom;
        syncZoomFromCam3d();
        render();
        return;
      }
      const sp = canvasPos(e);
      const before = screenToWorld(sp.x, sp.y);
      const factor = e.deltaY > 0 ? 0.9 : 1.1;
      view.zoom = Math.min(6, Math.max(0.25, view.zoom * factor));
      els.zoom.value = String(view.zoom);
      els.zoomVal.textContent = `${Math.round(view.zoom * 100)}%`;
      const after = screenToWorld(sp.x, sp.y);
      view.panX += (after.x - before.x) * pxPerMm();
      view.panY += (after.y - before.y) * pxPerMm();
      render();
    },
    { passive: false }
  );

  window.addEventListener("keydown", (e) => {
    if (e.code === "Space") spaceDown = true;
    if (e.key === "Shift") {
      shiftDown = true;
      if (mode === "profile") render();
    }
    const meta = e.metaKey || e.ctrlKey;
    if (meta && e.key === "z" && !e.shiftKey) {
      e.preventDefault();
      undo();
    } else if (meta && (e.key === "Z" || (e.key === "z" && e.shiftKey))) {
      e.preventDefault();
      redo();
    } else if (e.key === "Escape" && mode === "profile" && !doc.profile.closed) {
      e.preventDefault();
      pushUndo();
      doc.profile.points = [];
      selectedVertex = -1;
      dragging = null;
      refreshProfileUi();
    } else if (e.key === "Enter" && mode === "profile" && !doc.profile.closed && doc.profile.points.length >= 3) {
      e.preventDefault();
      document.getElementById("btn-close-profile").click();
    } else if ((e.key === "Delete" || e.key === "Backspace") && mode === "profile" && selectedVertex >= 0) {
      if (document.activeElement?.tagName === "INPUT") return;
      e.preventDefault();
      deleteSelectedVertex();
    } else if ((e.key === "Delete" || e.key === "Backspace") && mode === "features" && selectedFeatureId) {
      if (document.activeElement?.tagName === "INPUT") return;
      e.preventDefault();
      document.getElementById("btn-delete-feature").click();
    }
  });
  window.addEventListener("keyup", (e) => {
    if (e.code === "Space") spaceDown = false;
    if (e.key === "Shift") {
      shiftDown = false;
      if (mode === "profile") render();
    }
  });

  window.addEventListener("resize", resizeCanvas);
}

function syncFeatureToolButtons() {
  document.querySelectorAll("[data-feat]").forEach((btn) => {
    btn.classList.toggle("is-active", btn.dataset.feat === featureTool);
  });
  document.getElementById("feat-circle-params").hidden = featureTool !== "circle";
  document.getElementById("feat-rect-params").hidden = featureTool !== "roundRect";
}

function downloadBlob(blob, name) {
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = name;
  a.click();
  URL.revokeObjectURL(a.href);
}

// —— Boot ——
templateLibrary = mergeTemplateLibraries(loadTemplateLibrary(), doc.templates);
bindUI();
syncParamsFromDoc();
syncFeatureToolButtons();
updateUndoButtons();
setMode("profile");
resizeCanvas();
fitView();
