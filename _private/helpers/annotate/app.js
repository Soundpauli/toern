/* TŒRN Image Annotate — canvas editor */
(() => {
  "use strict";

  const VERSION = 2;
  const HANDLE = 7;
  const HIT = 10;

  const $ = (sel) => document.querySelector(sel);
  const $$ = (sel) => [...document.querySelectorAll(sel)];

  const canvas = $("#canvas");
  const ctx = canvas.getContext("2d");
  const stage = $("#stage");
  const dropHint = $("#drop-hint");

  const FONT_STACKS = {
    jetbrains: '"JetBrains Mono", ui-monospace, monospace',
    space: '"Space Grotesk", system-ui, sans-serif',
    inter: '"Inter", system-ui, sans-serif',
    bebas: '"Bebas Neue", sans-serif',
    system: "system-ui, sans-serif",
  };

  function fontKeyFromStack(stack) {
    const s = stack || "";
    if (s.includes("JetBrains")) return "jetbrains";
    if (s.includes("Space Grotesk")) return "space";
    if (s.includes("Inter")) return "inter";
    if (s.includes("Bebas")) return "bebas";
    return "system";
  }

  const defaultStyle = () => ({
    fontFamily: FONT_STACKS.jetbrains,
    titleSize: 22,
    subSize: 15,
    titleColor: "#0e1622",
    subColor: "#3a3845",
    lineColor: "#0e1622",
    lineWidth: 1.7,
    dotRadius: 3.5,
    halo: true,
    haloColor: "#e8e8ec",
    /** Border thickness (px) around text glyphs */
    haloTextWidth: 4,
    /** Border thickness (px) around leader lines and end dots */
    haloLineWidth: 4,
    maxWidth: 280,
    titleSubGap: 6,
    /** @type {"on"|"left"|"right"} */
    lineAttach: "on",
    /** Vertical gap under text ("on") or side gap (left/right) */
    lineMargin: 12,
    /** "on" only: extend under-line past text on each side */
    lineXGap: 8,
    /** "on" only: shift under-line vs text (positive = line moves right / text sits more left on line) */
    lineXOffset: 0,
  });

  /** @type {{
   *  version: number,
   *  image: null | { dataUrl: string, naturalWidth: number, naturalHeight: number },
   *  transform: { rotation: number, crop: {x:number,y:number,w:number,h:number} },
   *  bgColor: string,
   *  style: ReturnType<typeof defaultStyle>,
   *  annotations: Array<any>
   * }} */
  let project = emptyProject();

  /** @type {HTMLImageElement | null} */
  let imgEl = null;

  let mode = "image"; // image | annotate | select
  /** @type {Set<string>} */
  let selectedIds = new Set();
  /** last-focused annotation for the title/subtitle fields */
  let primaryId = null;
  let view = { scale: 1, panX: 40, panY: 40 };
  let history = [];
  let historyIndex = -1;

  // interaction state
  let drag = null;
  let spaceDown = false;

  function snapEnabled() {
    const el = $("#snap-enabled");
    return !el || el.checked;
  }

  function snapGrid() {
    const n = +$("#snap-grid")?.value;
    return Number.isFinite(n) && n > 0 ? n : 50;
  }

  function snapVal(v) {
    if (!snapEnabled()) return v;
    const g = snapGrid();
    return Math.round(v / g) * g;
  }

  function snapPoint(p) {
    return { x: snapVal(p.x), y: snapVal(p.y) };
  }

  function emptyProject() {
    return {
      version: VERSION,
      image: null,
      transform: { rotation: 0, crop: { x: 0, y: 0, w: 100, h: 100 } },
      bgColor: "#e8e8ec",
      style: defaultStyle(),
      annotations: [],
    };
  }

  function uid() {
    return "a_" + Math.random().toString(36).slice(2, 10);
  }

  function clone(obj) {
    return JSON.parse(JSON.stringify(obj));
  }

  function pushHistory() {
    const snap = clone(project);
    history = history.slice(0, historyIndex + 1);
    history.push(snap);
    if (history.length > 80) history.shift();
    historyIndex = history.length - 1;
    updateHistoryButtons();
  }

  function clearSelection() {
    selectedIds = new Set();
    primaryId = null;
  }

  function selectOnly(id) {
    selectedIds = new Set(id ? [id] : []);
    primaryId = id || null;
  }

  function selectAllAnns() {
    selectedIds = new Set(project.annotations.map((a) => a.id));
    primaryId = project.annotations[0] ? project.annotations[0].id : null;
  }

  function toggleSelect(id) {
    if (selectedIds.has(id)) selectedIds.delete(id);
    else selectedIds.add(id);
    primaryId = selectedIds.has(id) ? id : (selectedIds.values().next().value || null);
  }

  function isSelected(id) {
    return selectedIds.has(id);
  }

  function undo() {
    if (historyIndex <= 0) return;
    historyIndex -= 1;
    project = clone(history[historyIndex]);
    clearSelection();
    syncUiFromProject().then(draw);
    updateHistoryButtons();
  }

  function redo() {
    if (historyIndex >= history.length - 1) return;
    historyIndex += 1;
    project = clone(history[historyIndex]);
    clearSelection();
    syncUiFromProject().then(draw);
    updateHistoryButtons();
  }

  function updateHistoryButtons() {
    $("#btn-undo").disabled = historyIndex <= 0;
    $("#btn-redo").disabled = historyIndex >= history.length - 1;
  }

  // —— Rotated image metrics ——
  function rotRad() {
    return (project.transform.rotation * Math.PI) / 180;
  }

  function rotatedBounds(nw, nh, deg) {
    const r = (deg * Math.PI) / 180;
    const c = Math.cos(r);
    const s = Math.sin(r);
    const corners = [
      [0, 0],
      [nw, 0],
      [nw, nh],
      [0, nh],
    ].map(([x, y]) => {
      const cx = x - nw / 2;
      const cy = y - nh / 2;
      return [cx * c - cy * s, cx * s + cy * c];
    });
    const xs = corners.map((p) => p[0]);
    const ys = corners.map((p) => p[1]);
    const minX = Math.min(...xs);
    const maxX = Math.max(...xs);
    const minY = Math.min(...ys);
    const maxY = Math.max(...ys);
    return { w: maxX - minX, h: maxY - minY, minX, minY };
  }

  function sourceSize() {
    if (!imgEl) return { w: 0, h: 0 };
    return rotatedBounds(imgEl.naturalWidth, imgEl.naturalHeight, project.transform.rotation);
  }

  function docSize() {
    const c = project.transform.crop;
    return { w: Math.max(1, c.w), h: Math.max(1, c.h) };
  }

  // Style is global — per-annotation overrides were confusing and often blocked live edits
  function styleOf(_ann) {
    return { ...defaultStyle(), ...project.style };
  }

  function measureLabel(ann, measureCtx) {
    const st = styleOf(ann);
    const c = measureCtx || ctx;
    const maxW = st.maxWidth;
    c.font = `700 ${st.titleSize}px ${st.fontFamily}`;
    const title = ann.title || "Label";
    const titleW = Math.min(maxW, c.measureText(title).width);
    let subH = 0;
    let subW = 0;
    if (ann.subtitle) {
      c.font = `400 ${st.subSize}px ${st.fontFamily}`;
      subW = Math.min(maxW, c.measureText(ann.subtitle).width);
      subH = st.subSize + st.titleSubGap;
    }
    const w = Math.max(titleW, subW);
    const h = st.titleSize + subH;
    return { w, h, titleW, subW };
  }

  /** Y where the leader meets the label. */
  function labelLineY(ann, box) {
    const st = project.style;
    const modeAttach = st.lineAttach || "on";
    const gap = Math.max(0, st.lineMargin || 0);
    // "on" = under the text block + configurable vertical gap
    if (modeAttach === "on") return ann.label.y + box.h + gap;
    // left/right = vertical center of the text block
    return ann.label.y + box.h / 2;
  }

  /** Under-line X span for "on" mode (text sits above; gaps/offset configurable). */
  function onLineSpan(ann, box) {
    const st = project.style;
    const xGap = Math.max(0, st.lineXGap || 0);
    const xOff = st.lineXOffset || 0;
    return {
      left: ann.label.x - xGap + xOff,
      right: ann.label.x + box.w + xGap + xOff,
    };
  }

  /** Near-side approach point (for elbow layout). */
  function labelAttachPoint(ann) {
    const st = project.style;
    const box = measureLabel(ann);
    const lineY = labelLineY(ann, box);
    const modeAttach = st.lineAttach || "on";
    const gap = Math.max(0, st.lineMargin || 0);
    const left = ann.label.x;
    const right = ann.label.x + box.w;

    if (modeAttach === "left") return { x: left - gap, y: lineY };
    if (modeAttach === "right") return { x: right + gap, y: lineY };
    // "on" — approach the near end of the under-line span
    const span = onLineSpan(ann, box);
    const fromLeft = ann.target.x < ann.label.x + box.w / 2;
    return fromLeft ? { x: span.left, y: lineY } : { x: span.right, y: lineY };
  }

  /** Full leader polyline. "on" runs under the label so text sits on top of the line. */
  function leaderPoints(ann) {
    const st = project.style;
    const box = measureLabel(ann);
    const lineY = labelLineY(ann, box);
    const gap = Math.max(0, st.lineMargin || 0);
    const modeAttach = st.lineAttach || "on";
    const left = ann.label.x;
    const right = ann.label.x + box.w;

    if (modeAttach === "left") {
      return [ann.target, ann.elbow, { x: left - gap, y: lineY }];
    }
    if (modeAttach === "right") {
      return [ann.target, ann.elbow, { x: right + gap, y: lineY }];
    }
    const span = onLineSpan(ann, box);
    const fromLeft = ann.target.x < ann.label.x + box.w / 2;
    if (fromLeft) {
      return [ann.target, ann.elbow, { x: span.left, y: lineY }, { x: span.right, y: lineY }];
    }
    return [ann.target, ann.elbow, { x: span.right, y: lineY }, { x: span.left, y: lineY }];
  }

  /** Horizontal elbow into the label attach point (keeps mid-segment tidy). */
  function relayoutElbow(ann) {
    const attach = labelAttachPoint(ann);
    ann.elbow.y = attach.y;
    ann.elbow.x = (ann.target.x + attach.x) / 2;
  }

  // —— Drawing ——
  function resizeCanvas() {
    const rect = stage.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    canvas.width = Math.max(1, Math.floor(rect.width * dpr));
    canvas.height = Math.max(1, Math.floor(rect.height * dpr));
    canvas.style.width = rect.width + "px";
    canvas.style.height = rect.height + "px";
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  }

  function drawRotatedImage(targetCtx, dx, dy) {
    if (!imgEl) return;
    const nw = imgEl.naturalWidth;
    const nh = imgEl.naturalHeight;
    const bounds = rotatedBounds(nw, nh, project.transform.rotation);
    targetCtx.save();
    targetCtx.translate(dx + bounds.w / 2, dy + bounds.h / 2);
    targetCtx.rotate(rotRad());
    targetCtx.drawImage(imgEl, -nw / 2, -nh / 2);
    targetCtx.restore();
  }

  /** Draw the cropped document into an offscreen or export canvas at 1:1 crop pixels.
   *  Annotations are stored in rotated-source space and offset by -crop. */
  function renderDocument(outCtx, scale, opts) {
    const s = scale || 1;
    const withAnn = !opts || opts.annotations !== false;
    const { w, h } = docSize();
    const crop = project.transform.crop;

    outCtx.save();
    outCtx.scale(s, s);
    outCtx.fillStyle = project.bgColor;
    outCtx.fillRect(0, 0, w, h);

    outCtx.beginPath();
    outCtx.rect(0, 0, w, h);
    outCtx.clip();
    drawRotatedImage(outCtx, -crop.x, -crop.y);

    if (withAnn) {
      outCtx.translate(-crop.x, -crop.y);
      for (const ann of project.annotations) {
        drawAnnotation(outCtx, ann, false);
      }
    }
    outCtx.restore();
  }

  function drawAnnotation(c, ann, showHandles) {
    const st = styleOf(ann);
    const pts = leaderPoints(ann);

    function strokeLeader(width, color) {
      c.save();
      c.strokeStyle = color;
      c.lineWidth = width;
      c.lineCap = "round";
      c.lineJoin = "round";
      c.beginPath();
      c.moveTo(pts[0].x, pts[0].y);
      for (let i = 1; i < pts.length; i++) c.lineTo(pts[i].x, pts[i].y);
      c.stroke();
      c.restore();
    }

    const borderLine = Math.max(0, st.haloLineWidth != null ? +st.haloLineWidth : 4);
    const borderText = Math.max(0, st.haloTextWidth != null ? +st.haloTextWidth : 4);
    const haloOn = !!st.halo;

    // Draw line FIRST so text sits on top ("label on top of line")
    // Border = outer stroke wider by 2×border on each side of the ink
    if (haloOn && borderLine > 0) {
      strokeLeader(st.lineWidth + borderLine * 2, st.haloColor);
    }
    strokeLeader(st.lineWidth, st.lineColor);

    c.save();
    if (st.dotRadius > 0) {
      if (haloOn && borderLine > 0) {
        c.fillStyle = st.haloColor;
        c.beginPath();
        c.arc(ann.target.x, ann.target.y, st.dotRadius + borderLine, 0, Math.PI * 2);
        c.fill();
      }
      c.fillStyle = st.lineColor;
      c.beginPath();
      c.arc(ann.target.x, ann.target.y, st.dotRadius, 0, Math.PI * 2);
      c.fill();
    }
    c.restore();

    // text on top of the under-label segment
    const title = ann.title || "Label";
    c.save();
    if (haloOn && borderText > 0) {
      // stroke is centered on glyph edge → 2× gives ~borderText outside
      c.strokeStyle = st.haloColor;
      c.lineWidth = borderText * 2;
      c.lineJoin = "round";
      c.miterLimit = 2;
      c.font = `700 ${st.titleSize}px ${st.fontFamily}`;
      c.strokeText(title, ann.label.x, ann.label.y + st.titleSize);
      if (ann.subtitle) {
        c.font = `400 ${st.subSize}px ${st.fontFamily}`;
        c.strokeText(ann.subtitle, ann.label.x, ann.label.y + st.titleSize + st.titleSubGap + st.subSize);
      }
    }
    c.fillStyle = st.titleColor;
    c.font = `700 ${st.titleSize}px ${st.fontFamily}`;
    c.fillText(title, ann.label.x, ann.label.y + st.titleSize);
    if (ann.subtitle) {
      c.fillStyle = st.subColor;
      c.font = `400 ${st.subSize}px ${st.fontFamily}`;
      c.fillText(ann.subtitle, ann.label.x, ann.label.y + st.titleSize + st.titleSubGap + st.subSize);
    }
    c.restore();

    if (showHandles) {
      drawHandle(c, ann.target, "#2eb9ff");
      drawHandle(c, ann.elbow, "#ffcc00");
      const box = measureLabel(ann, c);
      c.save();
      c.strokeStyle = "rgba(46,185,255,0.85)";
      c.lineWidth = 1;
      c.setLineDash([4, 3]);
      c.strokeRect(ann.label.x - 2, ann.label.y - 2, box.w + 4, box.h + 4);
      c.restore();
      drawHandle(c, { x: ann.label.x, y: ann.label.y }, "#34c759");
    }
  }

  function drawHandle(c, p, color) {
    c.save();
    c.fillStyle = color;
    c.strokeStyle = "#fff";
    c.lineWidth = 1.5;
    c.beginPath();
    c.rect(p.x - HANDLE / 2, p.y - HANDLE / 2, HANDLE, HANDLE);
    c.fill();
    c.stroke();
    c.restore();
  }

  function drawCropOverlay(screenCtx) {
    // In image mode, show full rotated source with dim outside crop, crop rect handles
    if (!imgEl) return;
    const bounds = sourceSize();
    const crop = project.transform.crop;

    screenCtx.save();
    applyView(screenCtx);

    // full rotated image
    drawRotatedImage(screenCtx, 0, 0);

    // dim outside crop
    screenCtx.fillStyle = "rgba(0,0,0,0.45)";
    screenCtx.fillRect(0, 0, bounds.w, crop.y);
    screenCtx.fillRect(0, crop.y, crop.x, crop.h);
    screenCtx.fillRect(crop.x + crop.w, crop.y, bounds.w - crop.x - crop.w, crop.h);
    screenCtx.fillRect(0, crop.y + crop.h, bounds.w, bounds.h - crop.y - crop.h);

    // crop border
    screenCtx.strokeStyle = "#2eb9ff";
    screenCtx.lineWidth = 1.5 / view.scale;
    screenCtx.strokeRect(crop.x, crop.y, crop.w, crop.h);

    // handles
    const hs = [
      ["nw", crop.x, crop.y],
      ["n", crop.x + crop.w / 2, crop.y],
      ["ne", crop.x + crop.w, crop.y],
      ["e", crop.x + crop.w, crop.y + crop.h / 2],
      ["se", crop.x + crop.w, crop.y + crop.h],
      ["s", crop.x + crop.w / 2, crop.y + crop.h],
      ["sw", crop.x, crop.y + crop.h],
      ["w", crop.x, crop.y + crop.h / 2],
    ];
    for (const [, x, y] of hs) {
      drawHandle(screenCtx, { x, y }, "#2eb9ff");
    }

    // Annotations live in rotated-source space (crop does not move them)
    for (const ann of project.annotations) {
      drawAnnotation(screenCtx, ann, isSelected(ann.id));
    }

    screenCtx.restore();
  }

  function applyView(c) {
    c.translate(view.panX, view.panY);
    c.scale(view.scale, view.scale);
  }

  function draw() {
    const rect = stage.getBoundingClientRect();
    ctx.clearRect(0, 0, rect.width, rect.height);

    if (!imgEl) {
      dropHint.hidden = false;
      return;
    }
    dropHint.hidden = true;

    if (mode === "image") {
      drawCropOverlay(ctx);
      return;
    }

    // annotate / select: show cropped frame; annotations stay in source space
    const { w, h } = docSize();
    const crop = project.transform.crop;
    ctx.save();
    applyView(ctx);
    ctx.fillStyle = project.bgColor;
    ctx.fillRect(0, 0, w, h);
    ctx.beginPath();
    ctx.rect(0, 0, w, h);
    ctx.clip();
    drawRotatedImage(ctx, -crop.x, -crop.y);
    ctx.translate(-crop.x, -crop.y);
    for (const ann of project.annotations) {
      drawAnnotation(ctx, ann, mode === "select" && isSelected(ann.id));
    }
    ctx.restore();
  }

  // —— Coordinate mapping ——
  function screenToWorld(clientX, clientY) {
    const rect = stage.getBoundingClientRect();
    const x = (clientX - rect.left - view.panX) / view.scale;
    const y = (clientY - rect.top - view.panY) / view.scale;
    return { x, y };
  }

  /** Pointer → rotated-source space (annotations live here; crop is only a window). */
  function worldToSource(p) {
    if (mode === "image") return p;
    const c = project.transform.crop;
    return { x: p.x + c.x, y: p.y + c.y };
  }

  // —— Hit testing ——
  function hitHandle(p, point) {
    return Math.hypot(p.x - point.x, p.y - point.y) <= HIT / view.scale;
  }

  function hitAnnotation(docPt) {
    for (let i = project.annotations.length - 1; i >= 0; i--) {
      const ann = project.annotations[i];
      if (hitHandle(docPt, ann.target)) return { ann, part: "target" };
      if (hitHandle(docPt, ann.elbow)) return { ann, part: "elbow" };
      if (hitHandle(docPt, ann.label)) return { ann, part: "label" };
      const box = measureLabel(ann);
      if (
        docPt.x >= ann.label.x &&
        docPt.x <= ann.label.x + box.w &&
        docPt.y >= ann.label.y &&
        docPt.y <= ann.label.y + box.h
      ) {
        return { ann, part: "labelbox" };
      }
    }
    return null;
  }

  function hitCropHandle(world) {
    const crop = project.transform.crop;
    const pts = [
      ["nw", crop.x, crop.y],
      ["n", crop.x + crop.w / 2, crop.y],
      ["ne", crop.x + crop.w, crop.y],
      ["e", crop.x + crop.w, crop.y + crop.h / 2],
      ["se", crop.x + crop.w, crop.y + crop.h],
      ["s", crop.x + crop.w / 2, crop.y + crop.h],
      ["sw", crop.x, crop.y + crop.h],
      ["w", crop.x, crop.y + crop.h / 2],
    ];
    for (const [name, x, y] of pts) {
      if (hitHandle(world, { x, y })) return name;
    }
    if (
      world.x >= crop.x &&
      world.x <= crop.x + crop.w &&
      world.y >= crop.y &&
      world.y <= crop.y + crop.h
    ) {
      return "move";
    }
    return null;
  }

  // —— Image load ——
  function loadImageFromFile(file) {
    if (!file || !file.type.startsWith("image/")) return;
    const reader = new FileReader();
    reader.onload = () => {
      const dataUrl = reader.result;
      const image = new Image();
      image.onload = () => {
        imgEl = image;
        project = emptyProject();
        project.image = {
          dataUrl,
          naturalWidth: image.naturalWidth,
          naturalHeight: image.naturalHeight,
        };
        const b = rotatedBounds(image.naturalWidth, image.naturalHeight, 0);
        project.transform.crop = { x: 0, y: 0, w: b.w, h: b.h };
        project.bgColor = $("#bg-color").value;
        selectedIds = new Set();
        primaryId = null;
        history = [];
        historyIndex = -1;
        pushHistory();
        setMode("image");
        fitView();
        syncUiFromProject().then(draw);
      };
      image.src = dataUrl;
    };
    reader.readAsDataURL(file);
  }

  async function syncUiFromProject() {
    if (project.image && (!imgEl || imgEl.src !== project.image.dataUrl)) {
      await new Promise((resolve, reject) => {
        const image = new Image();
        image.onload = () => {
          imgEl = image;
          resolve();
        };
        image.onerror = reject;
        image.src = project.image.dataUrl;
      });
    }
    $("#img-rotate").value = project.transform.rotation;
    $("#img-rotate-num").value = project.transform.rotation;
    $("#bg-color").value = project.bgColor;
    fillStyleInputs(project.style);
    refreshAnnList();
    refreshAnnFields();
  }

  function fillStyleInputs(st) {
    const s = { ...defaultStyle(), ...st };
    $("#style-font").value = fontKeyFromStack(s.fontFamily);
    $("#style-title-size").value = s.titleSize;
    $("#style-sub-size").value = s.subSize;
    $("#style-title-color").value = s.titleColor;
    $("#style-sub-color").value = s.subColor;
    $("#style-line-color").value = s.lineColor;
    $("#style-line-width").value = s.lineWidth;
    $("#style-dot").value = s.dotRadius;
    $("#style-halo").checked = !!s.halo;
    $("#style-halo-color").value = s.haloColor;
    $("#style-halo-text").value = s.haloTextWidth != null ? s.haloTextWidth : 4;
    $("#style-halo-line").value = s.haloLineWidth != null ? s.haloLineWidth : 4;
    $("#style-max-width").value = s.maxWidth;
    syncLineAttachUi(s);
  }

  function syncLineAttachUi(st) {
    const s = st || project.style;
    const attach = s.lineAttach || "on";
    const el = $("#style-line-attach");
    if (el) el.value = attach;
    const m = $("#style-line-margin");
    if (m) {
      m.value = s.lineMargin != null ? s.lineMargin : 12;
      m.disabled = false;
    }
    const xg = $("#style-line-x-gap");
    if (xg) {
      xg.value = s.lineXGap != null ? s.lineXGap : 8;
      xg.disabled = attach !== "on";
    }
    const xo = $("#style-line-x-offset");
    if (xo) {
      xo.value = s.lineXOffset != null ? s.lineXOffset : 0;
      xo.disabled = attach !== "on";
    }
    const gapHint = $("#style-gap-hint");
    if (gapHint) {
      gapHint.textContent = attach === "on" ? "px under text" : "px beside label";
    }
  }

  function readStyleInputs() {
    const key = $("#style-font").value || "jetbrains";
    return {
      fontFamily: FONT_STACKS[key] || FONT_STACKS.jetbrains,
      titleSize: +$("#style-title-size").value,
      subSize: +$("#style-sub-size").value,
      titleColor: $("#style-title-color").value,
      subColor: $("#style-sub-color").value,
      lineColor: $("#style-line-color").value,
      lineWidth: +$("#style-line-width").value,
      dotRadius: +$("#style-dot").value,
      halo: $("#style-halo").checked,
      haloColor: $("#style-halo-color").value,
      haloTextWidth: Math.max(0, +$("#style-halo-text").value || 0),
      haloLineWidth: Math.max(0, +$("#style-halo-line").value || 0),
      maxWidth: +$("#style-max-width").value,
      titleSubGap: 6,
      lineAttach: $("#style-line-attach").value || "on",
      lineMargin: +$("#style-line-margin").value || 0,
      lineXGap: +$("#style-line-x-gap").value || 0,
      lineXOffset: +$("#style-line-x-offset").value || 0,
    };
  }

  /** Apply inspector values to global style and redraw. Returns the new style. */
  function applyStyleFromUi(opts) {
    const st = readStyleInputs();
    project.style = st;
    // Drop stale per-item overrides so live globals always win
    for (const ann of project.annotations) ann.style = null;
    syncLineAttachUi(st);
    if (!opts || opts.relayout !== false) {
      const list = project.annotations;
      for (const ann of list) relayoutElbow(ann);
    }
    draw();
    return st;
  }

  function fitView() {
    if (!imgEl) return;
    const rect = stage.getBoundingClientRect();
    const size = mode === "image" ? sourceSize() : docSize();
    const pad = 48;
    const sx = (rect.width - pad * 2) / size.w;
    const sy = (rect.height - pad * 2) / size.h;
    view.scale = Math.max(0.2, Math.min(4, Math.min(sx, sy)));
    view.panX = (rect.width - size.w * view.scale) / 2;
    view.panY = (rect.height - size.h * view.scale) / 2;
    $("#view-zoom").value = view.scale;
    $("#view-zoom-val").textContent = Math.round(view.scale * 100) + "%";
    draw();
  }

  // —— Modes ——
  function setMode(next, opts) {
    const prev = mode;
    mode = next;
    stage.dataset.mode = mode;
    $$(".ann-mode").forEach((b) => b.classList.toggle("is-active", b.dataset.mode === mode));
    $$(".ann-panel[data-panel]").forEach((p) => {
      if (p.dataset.panel === "image") p.hidden = mode !== "image";
      if (p.dataset.panel === "annotate") p.hidden = mode === "image";
    });
    $("#ann-hint").textContent =
      mode === "annotate"
        ? "Click a point on the image, then drag to place the label."
        : "Click to select · Shift/⌘-click to multi-select · ⌘A select all. Drag target / elbow / label.";
    const skipFit = opts && opts.skipFit;
    if (!skipFit && (prev === "image") !== (mode === "image")) fitView();
    else draw();
  }

  // —— Annotations ——
  function selected() {
    return project.annotations.find((a) => a.id === primaryId) || null;
  }

  function selectedAnns() {
    return project.annotations.filter((a) => selectedIds.has(a.id));
  }

  function refreshAnnList() {
    const ul = $("#ann-list");
    ul.innerHTML = "";
    project.annotations.forEach((ann, i) => {
      const li = document.createElement("li");
      if (isSelected(ann.id)) li.classList.add("is-selected");
      li.innerHTML = `<strong>${i + 1}.</strong> <span>${escapeHtml(ann.title || "Untitled")}</span>`;
      li.addEventListener("click", (e) => {
        if (e.shiftKey || e.metaKey || e.ctrlKey) toggleSelect(ann.id);
        else selectOnly(ann.id);
        setMode("select", { skipFit: true });
        refreshAnnList();
        refreshAnnFields();
        draw();
      });
      ul.appendChild(li);
    });
    const n = selectedIds.size;
    $("#btn-delete").disabled = n === 0;
    const selCount = $("#sel-count");
    if (selCount) selCount.textContent = n ? `${n} selected` : "None selected";
  }

  function escapeHtml(s) {
    return String(s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function refreshAnnFields() {
    const ann = selected();
    const multi = selectedIds.size > 1;
    $("#ann-title").value = ann && !multi ? ann.title : "";
    $("#ann-subtitle").value = ann && !multi ? ann.subtitle : "";
    $("#ann-title").disabled = !ann || multi;
    $("#ann-subtitle").disabled = !ann || multi;
    if (ann && ann.style) fillStyleInputs(styleOf(ann));
    else fillStyleInputs(project.style);
    $("#btn-delete").disabled = selectedIds.size === 0;
  }

  function createAnnotation(docX, docY) {
    const t = snapPoint({ x: docX, y: docY });
    const label = snapPoint({ x: t.x - 220, y: t.y - 16 });
    const elbow = snapPoint({ x: (t.x + label.x) / 2, y: t.y });
    const ann = {
      id: uid(),
      title: "Label",
      subtitle: "",
      target: t,
      elbow,
      label,
      style: null,
    };
    project.annotations.push(ann);
    selectOnly(ann.id);
    refreshAnnList();
    refreshAnnFields();
    return ann;
  }

  // —— Align ——
  function targetsForAlign() {
    const sel = selectedAnns();
    return sel.length ? sel : project.annotations.slice();
  }

  function alignStack(side) {
    const list = targetsForAlign();
    if (!list.length) return;
    const margin = +$("#align-margin").value || 36;
    const gap = +$("#align-gap").value || 24;
    const crop = project.transform.crop;

    if (side === "left") project.style.lineAttach = "left";
    if (side === "right") project.style.lineAttach = "right";
    syncLineAttachUi();

    list.sort((a, b) => a.label.y - b.label.y);

    let y = crop.y + margin;
    for (const ann of list) {
      const box = measureLabel(ann);
      if (side === "left") ann.label.x = crop.x + margin;
      else ann.label.x = crop.x + crop.w - margin - box.w;
      ann.label.y = y;
      relayoutElbow(ann);
      y += box.h + gap;
    }
    snapElbowGutter(list);
    pushHistory();
    draw();
  }

  function alignDistribute() {
    const list = targetsForAlign();
    if (list.length < 2) return;
    const gap = +$("#align-gap").value || 24;
    list.sort((a, b) => a.label.y - b.label.y);
    let y = list[0].label.y;
    for (let i = 0; i < list.length; i++) {
      const ann = list[i];
      if (i > 0) {
        const prev = list[i - 1];
        const prevBox = measureLabel(prev);
        y = prev.label.y + prevBox.h + gap;
      }
      ann.label.y = y;
      relayoutElbow(ann);
    }
    snapElbowGutter(list);
    pushHistory();
    draw();
  }

  function snapElbowGutter(list) {
    const anns = list || targetsForAlign();
    if (!anns.length) return;
    const gutters = anns.map((ann) => {
      const attach = labelAttachPoint(ann);
      return (ann.target.x + attach.x) / 2;
    });
    gutters.sort((a, b) => a - b);
    const gutter = gutters[Math.floor(gutters.length / 2)];
    for (const ann of anns) {
      const attach = labelAttachPoint(ann);
      ann.elbow.x = gutter;
      ann.elbow.y = attach.y;
    }
  }

  function alignElbows() {
    const list = targetsForAlign();
    if (!list.length) return;
    for (const ann of list) relayoutElbow(ann);
    snapElbowGutter(list);
    pushHistory();
    draw();
  }

  // —— Crop ops ——
  function clampCrop() {
    const b = sourceSize();
    const c = project.transform.crop;
    c.w = Math.max(20, Math.min(c.w, b.w));
    c.h = Math.max(20, Math.min(c.h, b.h));
    c.x = Math.max(0, Math.min(c.x, b.w - c.w));
    c.y = Math.max(0, Math.min(c.y, b.h - c.h));
  }

  function applyAspect(ratio) {
    const b = sourceSize();
    let w = b.w;
    let h = b.w / ratio;
    if (h > b.h) {
      h = b.h;
      w = b.h * ratio;
    }
    project.transform.crop = {
      x: (b.w - w) / 2,
      y: (b.h - h) / 2,
      w,
      h,
    };
    pushHistory();
    draw();
  }

  // —— Pointer events ——
  stage.addEventListener("pointerdown", (e) => {
    if (!imgEl) return;
    if (e.button === 1 || spaceDown || (e.button === 0 && e.altKey)) {
      drag = { type: "pan", x: e.clientX, y: e.clientY, panX: view.panX, panY: view.panY };
      stage.classList.add("is-panning");
      stage.setPointerCapture(e.pointerId);
      return;
    }
    if (e.button !== 0) return;

    const world = screenToWorld(e.clientX, e.clientY);

    if (mode === "image") {
      const handle = hitCropHandle(world);
      if (handle) {
        drag = {
          type: "crop",
          handle,
          start: world,
          crop: { ...project.transform.crop },
        };
        stage.setPointerCapture(e.pointerId);
      }
      return;
    }

    const doc = worldToSource(world);

    if (mode === "annotate") {
      const ann = createAnnotation(doc.x, doc.y);
      drag = { type: "place", annId: ann.id, start: doc };
      stage.setPointerCapture(e.pointerId);
      draw();
      return;
    }

    if (mode === "select") {
      const hit = hitAnnotation(doc);
      if (hit) {
        if (e.shiftKey || e.metaKey || e.ctrlKey) toggleSelect(hit.ann.id);
        else if (!isSelected(hit.ann.id)) selectOnly(hit.ann.id);
        else primaryId = hit.ann.id;
        refreshAnnList();
        refreshAnnFields();
        const moving = selectedAnns();
        drag = {
          type: "ann",
          part: hit.part === "labelbox" ? "label" : hit.part,
          annId: hit.ann.id,
          start: doc,
          orig: clone(hit.ann),
          // group-move labels/elbows/targets for multi-select when dragging label
          group: moving.map((a) => clone(a)),
        };
        stage.setPointerCapture(e.pointerId);
        draw();
      } else {
        clearSelection();
        refreshAnnList();
        refreshAnnFields();
        draw();
      }
    }
  });

  stage.addEventListener("pointermove", (e) => {
    if (!drag) return;
    if (drag.type === "pan") {
      view.panX = drag.panX + (e.clientX - drag.x);
      view.panY = drag.panY + (e.clientY - drag.y);
      draw();
      return;
    }

    const world = screenToWorld(e.clientX, e.clientY);

    if (drag.type === "crop") {
      const c0 = drag.crop;
      const dx = world.x - drag.start.x;
      const dy = world.y - drag.start.y;
      let { x, y, w, h } = c0;
      const hdl = drag.handle;
      if (hdl === "move") {
        x = c0.x + dx;
        y = c0.y + dy;
      } else {
        if (hdl.includes("e")) w = c0.w + dx;
        if (hdl.includes("s")) h = c0.h + dy;
        if (hdl.includes("w")) {
          x = c0.x + dx;
          w = c0.w - dx;
        }
        if (hdl.includes("n")) {
          y = c0.y + dy;
          h = c0.h - dy;
        }
      }
      project.transform.crop = { x, y, w, h };
      clampCrop();
      draw();
      return;
    }

    const doc = worldToSource(world);
    const ann = project.annotations.find((a) => a.id === drag.annId);
    if (!ann) return;

    if (drag.type === "place") {
      const snapped = snapPoint({ x: doc.x, y: doc.y - 10 });
      ann.label.x = snapped.x;
      ann.label.y = snapped.y;
      ann.elbow.x = snapVal((ann.target.x + ann.label.x) / 2);
      ann.elbow.y = snapVal((ann.target.y + ann.label.y) / 2);
      draw();
      return;
    }

    if (drag.type === "ann") {
      let dx = doc.x - drag.start.x;
      let dy = doc.y - drag.start.y;

      if (drag.part === "label" && drag.group && drag.group.length > 1) {
        // Snap the primary label, then move the whole group by that snapped delta
        const primary = drag.group.find((g) => g.id === drag.annId) || drag.group[0];
        const snappedLabel = snapPoint({
          x: primary.label.x + dx,
          y: primary.label.y + dy,
        });
        dx = snappedLabel.x - primary.label.x;
        dy = snappedLabel.y - primary.label.y;
        for (const snap of drag.group) {
          const a = project.annotations.find((x) => x.id === snap.id);
          if (!a) continue;
          a.target.x = snap.target.x + dx;
          a.target.y = snap.target.y + dy;
          a.elbow.x = snap.elbow.x + dx;
          a.elbow.y = snap.elbow.y + dy;
          a.label.x = snap.label.x + dx;
          a.label.y = snap.label.y + dy;
        }
      } else {
        const a = project.annotations.find((x) => x.id === drag.annId);
        if (!a) return;
        if (drag.part === "target") {
          const p = snapPoint({
            x: drag.orig.target.x + dx,
            y: drag.orig.target.y + dy,
          });
          a.target.x = p.x;
          a.target.y = p.y;
        } else if (drag.part === "elbow") {
          const p = snapPoint({
            x: drag.orig.elbow.x + dx,
            y: drag.orig.elbow.y + dy,
          });
          a.elbow.x = p.x;
          a.elbow.y = p.y;
        } else if (drag.part === "label") {
          const p = snapPoint({
            x: drag.orig.label.x + dx,
            y: drag.orig.label.y + dy,
          });
          a.label.x = p.x;
          a.label.y = p.y;
        }
      }
      draw();
    }
  });

  stage.addEventListener("pointerup", (e) => {
    if (!drag) return;
    const t = drag.type;
    drag = null;
    stage.classList.remove("is-panning");
    try {
      stage.releasePointerCapture(e.pointerId);
    } catch (_) {}
    if (t === "place") {
      // createAnnotation already pushed history; push final geometry
      pushHistory();
      setMode("select", { skipFit: true });
      refreshAnnFields();
      $("#ann-title").focus();
      $("#ann-title").select();
    } else if (t === "crop" || t === "ann") {
      pushHistory();
    }
    refreshAnnList();
  });

  stage.addEventListener("wheel", (e) => {
    if (!imgEl) return;
    e.preventDefault();
    const rect = stage.getBoundingClientRect();
    const mx = e.clientX - rect.left;
    const my = e.clientY - rect.top;
    const before = screenToWorld(e.clientX, e.clientY);
    const factor = e.deltaY > 0 ? 0.92 : 1.08;
    view.scale = Math.max(0.2, Math.min(4, view.scale * factor));
    view.panX = mx - before.x * view.scale;
    view.panY = my - before.y * view.scale;
    $("#view-zoom").value = view.scale;
    $("#view-zoom-val").textContent = Math.round(view.scale * 100) + "%";
    draw();
  }, { passive: false });

  // —— Drag & drop ——
  ["dragenter", "dragover"].forEach((ev) => {
    stage.addEventListener(ev, (e) => {
      e.preventDefault();
      stage.classList.add("is-dragover");
    });
  });
  ["dragleave", "drop"].forEach((ev) => {
    stage.addEventListener(ev, (e) => {
      e.preventDefault();
      stage.classList.remove("is-dragover");
    });
  });
  stage.addEventListener("drop", (e) => {
    const file = e.dataTransfer?.files?.[0];
    if (file) loadImageFromFile(file);
  });

  // —— UI bindings ——
  $$(".ann-mode").forEach((btn) => {
    btn.addEventListener("click", () => setMode(btn.dataset.mode));
  });

  $("#btn-open").addEventListener("click", () => $("#file-image").click());
  $("#file-image").addEventListener("change", (e) => {
    const f = e.target.files?.[0];
    if (f) loadImageFromFile(f);
    e.target.value = "";
  });

  $("#view-zoom").addEventListener("input", (e) => {
    const rect = stage.getBoundingClientRect();
    const cx = rect.width / 2;
    const cy = rect.height / 2;
    const before = {
      x: (cx - view.panX) / view.scale,
      y: (cy - view.panY) / view.scale,
    };
    view.scale = +e.target.value;
    view.panX = cx - before.x * view.scale;
    view.panY = cy - before.y * view.scale;
    $("#view-zoom-val").textContent = Math.round(view.scale * 100) + "%";
    draw();
  });
  $("#btn-fit").addEventListener("click", fitView);
  $("#btn-reset-view").addEventListener("click", () => {
    view = { scale: 1, panX: 40, panY: 40 };
    $("#view-zoom").value = 1;
    $("#view-zoom-val").textContent = "100%";
    fitView();
  });

  function applyRotationLive(deg) {
    const prevDeg = project.transform.rotation;
    if (!imgEl || deg === prevDeg) {
      project.transform.rotation = deg;
      $("#img-rotate").value = deg;
      $("#img-rotate-num").value = deg;
      draw();
      return;
    }
    const nw = imgEl.naturalWidth;
    const nh = imgEl.naturalHeight;
    const fromB = rotatedBounds(nw, nh, prevDeg);
    const toB = rotatedBounds(nw, nh, deg);

    const mapPt = (p) => {
      // source AABB → natural → new source AABB
      const cx = p.x - fromB.w / 2;
      const cy = p.y - fromB.h / 2;
      const r0 = (-prevDeg * Math.PI) / 180;
      const nx = cx * Math.cos(r0) - cy * Math.sin(r0) + nw / 2;
      const ny = cx * Math.sin(r0) + cy * Math.cos(r0) + nh / 2;
      const r1 = (deg * Math.PI) / 180;
      const sx = nx - nw / 2;
      const sy = ny - nh / 2;
      return {
        x: sx * Math.cos(r1) - sy * Math.sin(r1) + toB.w / 2,
        y: sx * Math.sin(r1) + sy * Math.cos(r1) + toB.h / 2,
      };
    };

    for (const ann of project.annotations) {
      ann.target = mapPt(ann.target);
      ann.elbow = mapPt(ann.elbow);
      ann.label = mapPt(ann.label);
    }

    // Remap crop rectangle corners
    const c = project.transform.crop;
    const corners = [
      mapPt({ x: c.x, y: c.y }),
      mapPt({ x: c.x + c.w, y: c.y }),
      mapPt({ x: c.x + c.w, y: c.y + c.h }),
      mapPt({ x: c.x, y: c.y + c.h }),
    ];
    const xs = corners.map((p) => p.x);
    const ys = corners.map((p) => p.y);
    project.transform.rotation = deg;
    project.transform.crop = {
      x: Math.min(...xs),
      y: Math.min(...ys),
      w: Math.max(...xs) - Math.min(...xs),
      h: Math.max(...ys) - Math.min(...ys),
    };
    clampCrop();

    $("#img-rotate").value = deg;
    $("#img-rotate-num").value = deg;
    draw();
  }
  $("#img-rotate").addEventListener("input", (e) => {
    applyRotationLive(+e.target.value);
  });
  $("#img-rotate").addEventListener("change", () => {
    pushHistory();
    fitView();
  });
  $("#img-rotate-num").addEventListener("change", (e) => {
    applyRotationLive(+e.target.value);
    pushHistory();
    fitView();
  });
  $$("[data-rot]").forEach((b) => {
    b.addEventListener("click", () => {
      applyRotationLive(+b.dataset.rot);
      pushHistory();
      fitView();
    });
  });

  $("#crop-reset").addEventListener("click", () => {
    const b = sourceSize();
    project.transform.crop = { x: 0, y: 0, w: b.w, h: b.h };
    pushHistory();
    draw();
  });
  $$("[data-aspect]").forEach((b) => {
    b.addEventListener("click", () => {
      const [a, c] = b.dataset.aspect.split(":").map(Number);
      applyAspect(a / c);
    });
  });
  $("#bg-color").addEventListener("change", (e) => {
    project.bgColor = e.target.value;
    pushHistory();
    draw();
  });

  $("#ann-title").addEventListener("input", (e) => {
    const ann = selected();
    if (!ann) return;
    ann.title = e.target.value;
    refreshAnnList();
    draw();
  });
  $("#ann-title").addEventListener("change", () => pushHistory());
  $("#ann-subtitle").addEventListener("input", (e) => {
    const ann = selected();
    if (!ann) return;
    ann.subtitle = e.target.value;
    draw();
  });
  $("#ann-subtitle").addEventListener("change", () => pushHistory());

  $("#btn-delete").addEventListener("click", () => {
    if (!selectedIds.size) return;
    project.annotations = project.annotations.filter((a) => !selectedIds.has(a.id));
    clearSelection();
    pushHistory();
    refreshAnnList();
    refreshAnnFields();
    draw();
  });

  $("#btn-select-all").addEventListener("click", () => {
    selectAllAnns();
    setMode("select", { skipFit: true });
    refreshAnnList();
    refreshAnnFields();
    draw();
  });
  $("#btn-select-none").addEventListener("click", () => {
    clearSelection();
    refreshAnnList();
    refreshAnnFields();
    draw();
  });

  $("#align-left").addEventListener("click", () => alignStack("left"));
  $("#align-right").addEventListener("click", () => alignStack("right"));
  $("#align-distribute").addEventListener("click", alignDistribute);
  $("#align-elbows").addEventListener("click", alignElbows);

  // Live style — every control updates all callouts immediately
  const STYLE_IDS = [
    "style-font",
    "style-title-size",
    "style-sub-size",
    "style-title-color",
    "style-sub-color",
    "style-line-color",
    "style-line-width",
    "style-dot",
    "style-halo",
    "style-halo-color",
    "style-halo-text",
    "style-halo-line",
    "style-max-width",
    "style-line-attach",
    "style-line-margin",
    "style-line-x-gap",
    "style-line-x-offset",
  ];
  STYLE_IDS.forEach((id) => {
    const el = $("#" + id);
    if (!el) return;
    const apply = () => {
      const needLayout =
        id === "style-line-attach" ||
        id === "style-line-margin" ||
        id === "style-line-x-gap" ||
        id === "style-line-x-offset" ||
        id === "style-title-size" ||
        id === "style-sub-size" ||
        id === "style-font" ||
        id === "style-max-width";
      applyStyleFromUi({ relayout: needLayout });
    };
    el.addEventListener("input", apply);
    el.addEventListener("change", apply);
    el.addEventListener("change", () => pushHistory());
  });

  $("#btn-undo").addEventListener("click", undo);
  $("#btn-redo").addEventListener("click", redo);

  // —— Export / project ——
  function canvasToBlob(canvas, type) {
    return new Promise((resolve) => canvas.toBlob(resolve, type || "image/png"));
  }

  function downloadBlob(blob, filename) {
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = filename;
    a.click();
    URL.revokeObjectURL(a.href);
  }

  function makeDocCanvas(scale, withAnn) {
    const { w, h } = docSize();
    const out = document.createElement("canvas");
    out.width = Math.round(w * scale);
    out.height = Math.round(h * scale);
    renderDocument(out.getContext("2d"), scale, { annotations: withAnn });
    return out;
  }

  async function exportPng(withAnn) {
    if (!imgEl) return;
    const scale = +$("#export-scale").value || 2;
    const out = makeDocCanvas(scale, withAnn !== false);
    const blob = await canvasToBlob(out);
    if (blob) downloadBlob(blob, withAnn === false ? "annotate-image.png" : "annotate.png");
  }

  function escapeXml(s) {
    return String(s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  async function exportHtml() {
    if (!imgEl) return;
    const scale = 1;
    const crop = project.transform.crop;
    const { w, h } = docSize();
    const imgCanvas = makeDocCanvas(scale, false);
    const dataUrl = imgCanvas.toDataURL("image/png");

    const parts = [];
    parts.push(`<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>Annotated image</title>
<style>
  body { margin: 0; background: #111; display: grid; place-items: center; min-height: 100vh; }
  .ann-export {
    position: relative;
    width: min(100vw, ${w}px);
    background: ${project.bgColor};
    line-height: 0;
  }
  .ann-export img { width: 100%; height: auto; display: block; }
  .ann-export svg {
    position: absolute; inset: 0; width: 100%; height: 100%;
    overflow: visible; pointer-events: none;
  }
</style>
</head>
<body>
<figure class="ann-export">
  <img src="${dataUrl}" width="${w}" height="${h}" alt="Annotated"/>
  <svg viewBox="0 0 ${w} ${h}" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">`);

    for (const ann of project.annotations) {
      const st = styleOf(ann);
      const cropOff = (p) => ({ x: p.x - crop.x, y: p.y - crop.y });
      const pts = leaderPoints(ann).map(cropOff);
      const label = cropOff(ann.label);
      const title = ann.title || "Label";

      const pathD = pts.map((p, i) => `${i ? "L" : "M"}${p.x} ${p.y}`).join(" ");
      const borderLine = Math.max(0, st.haloLineWidth != null ? +st.haloLineWidth : 4);
      const borderText = Math.max(0, st.haloTextWidth != null ? +st.haloTextWidth : 4);
      const haloOn = !!st.halo;
      if (haloOn && borderLine > 0) {
        parts.push(
          `<path d="${pathD}" fill="none" stroke="${escapeXml(st.haloColor)}" stroke-width="${st.lineWidth + borderLine * 2}" stroke-linecap="round" stroke-linejoin="round"/>`
        );
      }
      parts.push(
        `<path d="${pathD}" fill="none" stroke="${escapeXml(st.lineColor)}" stroke-width="${st.lineWidth}" stroke-linecap="round" stroke-linejoin="round"/>`
      );
      if (st.dotRadius > 0) {
        if (haloOn && borderLine > 0) {
          parts.push(
            `<circle cx="${pts[0].x}" cy="${pts[0].y}" r="${st.dotRadius + borderLine}" fill="${escapeXml(st.haloColor)}"/>`
          );
        }
        parts.push(`<circle cx="${pts[0].x}" cy="${pts[0].y}" r="${st.dotRadius}" fill="${escapeXml(st.lineColor)}"/>`);
      }
      const textHalo =
        haloOn && borderText > 0
          ? ` stroke="${escapeXml(st.haloColor)}" stroke-width="${borderText * 2}" stroke-linejoin="round" paint-order="stroke fill"`
          : "";
      parts.push(
        `<text x="${label.x}" y="${label.y + st.titleSize}" font-family="${escapeXml(st.fontFamily)}" font-size="${st.titleSize}" font-weight="700" fill="${escapeXml(st.titleColor)}"${textHalo}>${escapeXml(title)}</text>`
      );
      if (ann.subtitle) {
        parts.push(
          `<text x="${label.x}" y="${label.y + st.titleSize + st.titleSubGap + st.subSize}" font-family="${escapeXml(st.fontFamily)}" font-size="${st.subSize}" font-weight="400" fill="${escapeXml(st.subColor)}"${textHalo}>${escapeXml(ann.subtitle)}</text>`
        );
      }
    }

    parts.push(`</svg>
</figure>
</body>
</html>`);

    const blob = new Blob([parts.join("\n")], { type: "text/html;charset=utf-8" });
    downloadBlob(blob, "annotate.html");
  }

  function saveJson() {
    const data = clone(project);
    data.version = VERSION;
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
    downloadBlob(blob, "annotate-project.json");
  }

  function migrateProject(data) {
    if (!data || typeof data.version !== "number") return null;
    if (data.version > VERSION) return null;
    // v1 stored annotations in crop space — lift into source space
    if (data.version < 2 && data.transform && data.transform.crop && Array.isArray(data.annotations)) {
      const c = data.transform.crop;
      for (const ann of data.annotations) {
        for (const key of ["target", "elbow", "label"]) {
          if (ann[key]) {
            ann[key].x += c.x;
            ann[key].y += c.y;
          }
        }
      }
    }
    data.version = VERSION;
    return data;
  }

  async function loadJsonFile(file) {
    const text = await file.text();
    let data;
    try {
      data = migrateProject(JSON.parse(text));
    } catch (_) {
      alert("Invalid JSON.");
      return;
    }
    if (!data) {
      alert("Unsupported or missing project version.");
      return;
    }
    project = data;
    if (!project.style) project.style = defaultStyle();
    else project.style = { ...defaultStyle(), ...project.style };
    clearSelection();
    history = [];
    historyIndex = -1;
    await syncUiFromProject();
    pushHistory();
    setMode(project.annotations.length ? "select" : "image");
    fitView();
  }

  $("#btn-export").addEventListener("click", () => exportPng(true));
  $("#btn-export-image").addEventListener("click", () => exportPng(false));
  $("#btn-export-html").addEventListener("click", exportHtml);
  $("#btn-save").addEventListener("click", saveJson);
  $("#btn-load").addEventListener("click", () => $("#file-json").click());
  $("#file-json").addEventListener("change", (e) => {
    const f = e.target.files?.[0];
    if (f) loadJsonFile(f);
    e.target.value = "";
  });

  // —— Keyboard ——
  window.addEventListener("keydown", (e) => {
    if (e.code === "Space" && !e.repeat && !isTyping(e)) {
      spaceDown = true;
      e.preventDefault();
    }
    const meta = e.metaKey || e.ctrlKey;
    if (meta && e.key.toLowerCase() === "z") {
      e.preventDefault();
      if (e.shiftKey) redo();
      else undo();
    }
    if (meta && e.key.toLowerCase() === "y") {
      e.preventDefault();
      redo();
    }
    if ((e.key === "Delete" || e.key === "Backspace") && selectedIds.size && !isTyping(e)) {
      e.preventDefault();
      $("#btn-delete").click();
    }
    if (e.key === "Escape") {
      clearSelection();
      refreshAnnList();
      refreshAnnFields();
      draw();
    }
    if (meta && e.key.toLowerCase() === "a" && !isTyping(e)) {
      e.preventDefault();
      selectAllAnns();
      setMode("select", { skipFit: true });
      refreshAnnList();
      refreshAnnFields();
      draw();
    }
  });
  window.addEventListener("keyup", (e) => {
    if (e.code === "Space") spaceDown = false;
  });

  function isTyping(e) {
    const t = e.target;
    return t && (t.tagName === "INPUT" || t.tagName === "TEXTAREA" || t.tagName === "SELECT" || t.isContentEditable);
  }

  // —— Resize ——
  const ro = new ResizeObserver(() => {
    resizeCanvas();
    draw();
  });
  ro.observe(stage);

  // init
  resizeCanvas();
  setMode("image");
  updateHistoryButtons();
  dropHint.hidden = false;

  // Test / debug hooks
  window.__ANN_TEST__ = {
    getProject: () => project,
    getStyle: () => ({ ...project.style }),
    leaderPoints: (ann) => leaderPoints(ann),
    measureLabel: (ann) => measureLabel(ann),
    applyStyleFromUi,
    selectAllAnns,
    selectedCount: () => selectedIds.size,
    loadBlank(w, h) {
      const c = document.createElement("canvas");
      c.width = w || 800;
      c.height = h || 600;
      const g = c.getContext("2d");
      g.fillStyle = "#ccd";
      g.fillRect(0, 0, c.width, c.height);
      g.fillStyle = "#334";
      g.fillRect(200, 150, 400, 300);
      return new Promise((resolve) => {
        c.toBlob((blob) => {
          const file = new File([blob], "test.png", { type: "image/png" });
          const reader = new FileReader();
          reader.onload = () => {
            const dataUrl = reader.result;
            const image = new Image();
            image.onload = () => {
              imgEl = image;
              project = emptyProject();
              project.image = { dataUrl, naturalWidth: image.naturalWidth, naturalHeight: image.naturalHeight };
              const b = rotatedBounds(image.naturalWidth, image.naturalHeight, 0);
              project.transform.crop = { x: 0, y: 0, w: b.w, h: b.h };
              history = [];
              historyIndex = -1;
              pushHistory();
              setMode("annotate", { skipFit: true });
              fitView();
              syncUiFromProject().then(() => {
                draw();
                resolve(project);
              });
            };
            image.src = dataUrl;
          };
          reader.readAsDataURL(file);
        }, "image/png");
      });
    },
    addAnn(partial) {
      const ann = {
        id: uid(),
        title: "Label",
        subtitle: "Sub",
        target: { x: 400, y: 300 },
        elbow: { x: 280, y: 300 },
        label: { x: 40, y: 280 },
        style: null,
        ...partial,
      };
      project.annotations.push(ann);
      selectOnly(ann.id);
      relayoutElbow(ann);
      refreshAnnList();
      refreshAnnFields();
      draw();
      return ann;
    },
  };
})();
