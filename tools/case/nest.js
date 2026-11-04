/** Nest layout + multi-copy sheet packing (MaxRects / shelf). */

import { panelBounds } from "./unfold.js";
import { orientPanelsOuterUp, rotatePanel90CW } from "./orient.js";

/**
 * Place panels in rows with spacing (legacy shelf; no per-panel rotation).
 * @returns {{ items: NestItem[], width, height, gap, padding }}
 */
export function nestPanels(panels, opts = {}) {
  const gap = opts.gap ?? opts.margin ?? 8;
  const padding = opts.padding ?? opts.margin ?? gap;
  const maxRowWidth = opts.maxRowWidth ?? 400;

  const items = [];
  let x = padding;
  let y = padding;
  let rowH = 0;
  let maxX = padding;
  let maxY = padding;

  for (const panel of panels) {
    const bounds = panelBounds(panel);
    const w = bounds.width;
    const h = bounds.height;

    if (x > padding && x + w + padding > maxRowWidth) {
      x = padding;
      y += rowH + gap;
      rowH = 0;
    }

    items.push({
      panel,
      x,
      y,
      bounds,
      rotate90: false,
    });

    maxX = Math.max(maxX, x + w);
    maxY = Math.max(maxY, y + h);
    x += w + gap;
    rowH = Math.max(rowH, h);
  }

  return {
    items,
    width: Math.max(maxX + padding, padding * 2),
    height: Math.max(maxY + padding, padding * 2),
    gap,
    padding,
  };
}

export function nestContentSize(nest) {
  const pad = nest.padding ?? 0;
  return {
    width: Math.max(0, nest.width - 2 * pad),
    height: Math.max(0, nest.height - 2 * pad),
  };
}

export function bestCopyGrid(bw, bh, availW, availH, copies, gap = 8) {
  if (!(bw > 0 && bh > 0 && availW > 0 && availH > 0 && copies >= 1)) return null;
  if (bw > availW + 1e-9 || bh > availH + 1e-9) return null;

  let best = null;
  for (let cols = 1; cols <= copies; cols++) {
    const rows = Math.ceil(copies / cols);
    const usedW = cols * bw + (cols - 1) * gap;
    const usedH = rows * bh + (rows - 1) * gap;
    if (usedW > availW + 1e-9 || usedH > availH + 1e-9) continue;

    const area = usedW * usedH;
    const leftover = availW * availH - area;
    const aspectPenalty = Math.abs(usedW / Math.max(usedH, 1e-9) - availW / Math.max(availH, 1e-9));
    const score = leftover * 0.001 + aspectPenalty + rows * 0.01;
    const candidate = { cols, rows, usedW, usedH, area, score };
    if (!best || candidate.score < best.score) best = candidate;
  }
  return best;
}

export function maxCopiesThatFit(bw, bh, availW, availH, gap = 8) {
  if (!(bw > 0 && bh > 0)) return 0;
  const cols = Math.max(0, Math.floor((availW + gap) / (bw + gap)));
  const rows = Math.max(0, Math.floor((availH + gap) / (bh + gap)));
  return cols * rows;
}

/**
 * MaxRects bottom-left / best-short-side-fit packing with optional 90° rotation.
 * @param {Array<{ panel: object, w: number, h: number }>} pieces
 * @returns {{ items, width, height, gap, padding } | null}
 */
export function packPanelsMaxRects(pieces, { binW, binH = Infinity, gap = 8 } = {}) {
  if (!pieces.length) {
    return { items: [], width: 0, height: 0, gap, padding: 0 };
  }
  if (!(binW > 0)) return null;

  let free = [{ x: 0, y: 0, w: binW, h: binH }];
  const items = [];
  let usedW = 0;
  let usedH = 0;

  for (const piece of pieces) {
    const variants = [];
    const forced = piece.forceRot;
    // forceRot: true → only 90°, false → only 0°, null/undefined → try both
    if (forced !== true) {
      variants.push({ w: piece.w, h: piece.h, rotate90: false, panel: piece.panel });
    }
    if (forced !== false && piece.panelRot90) {
      variants.push({
        w: piece.h,
        h: piece.w,
        rotate90: true,
        panel: piece.panelRot90,
      });
    }
    if (!variants.length && piece.panelRot90) {
      // Degenerate fallback
      variants.push({ w: piece.w, h: piece.h, rotate90: false, panel: piece.panel });
    }

    let best = null;
    for (const v of variants) {
      if (!(v.w > 0 && v.h > 0)) continue;
      for (let fi = 0; fi < free.length; fi++) {
        const fr = free[fi];
        if (v.w > fr.w + 1e-9 || v.h > fr.h + 1e-9) continue;
        const leftoverW = fr.w - v.w;
        const leftoverH = Number.isFinite(fr.h) ? fr.h - v.h : leftoverW;
        const shortSide = Math.min(leftoverW, leftoverH);
        const longSide = Math.max(leftoverW, leftoverH);
        const area = leftoverW * (Number.isFinite(leftoverH) ? leftoverH : leftoverW);
        // Prefer placements that leave a usable strip (long leftover) and sit low/left.
        const score = {
          shortSide,
          longSide,
          area,
          y: fr.y,
          x: fr.x,
          // Slight bias toward the orientation that matches the free slot’s aspect.
          aspect: Math.abs(leftoverW - leftoverH),
          fi,
          v,
        };
        if (
          !best ||
          score.shortSide < best.shortSide - 1e-9 ||
          (Math.abs(score.shortSide - best.shortSide) <= 1e-9 && score.longSide > best.longSide + 1e-9) ||
          (Math.abs(score.shortSide - best.shortSide) <= 1e-9 &&
            Math.abs(score.longSide - best.longSide) <= 1e-9 &&
            score.area < best.area - 1e-9) ||
          (Math.abs(score.shortSide - best.shortSide) <= 1e-9 &&
            Math.abs(score.longSide - best.longSide) <= 1e-9 &&
            Math.abs(score.area - best.area) <= 1e-9 &&
            (score.y < best.y - 1e-9 || (Math.abs(score.y - best.y) <= 1e-9 && score.x < best.x)))
        ) {
          best = score;
        }
      }
    }

    if (!best) return null;

    const { v } = best;
    const fr = free[best.fi];
    const x = fr.x;
    const y = fr.y;
    items.push({
      panel: v.panel,
      x,
      y,
      bounds: panelBounds(v.panel),
      rotate90: v.rotate90,
      copyIndex: piece.copyIndex ?? 0,
    });
    usedW = Math.max(usedW, x + v.w);
    usedH = Math.max(usedH, y + v.h);

    // Occupy panel plus gap (clipped to bin so edge parts don't waste margin).
    const occ = {
      x,
      y,
      w: Math.min(v.w + gap, Math.max(0, binW - x)),
      h: Number.isFinite(binH) ? Math.min(v.h + gap, Math.max(0, binH - y)) : v.h + gap,
    };
    const next = [];
    for (const f of free) {
      if (!rectsOverlap(f, occ)) {
        next.push(f);
        continue;
      }
      next.push(...carveRect(f, occ));
    }
    free = next;
    pruneFreeList(free);
  }

  return {
    items,
    width: usedW,
    height: usedH,
    gap,
    padding: 0,
  };
}

function rectsOverlap(a, b) {
  return !(
    a.x + a.w <= b.x + 1e-9 ||
    b.x + b.w <= a.x + 1e-9 ||
    a.y + a.h <= b.y + 1e-9 ||
    b.y + b.h <= a.y + 1e-9
  );
}

/** Split free rect `f` around occupied `occ` into up to 4 remainders. */
function carveRect(f, occ) {
  const out = [];
  const fx2 = f.x + f.w;
  const fy2 = Number.isFinite(f.h) ? f.y + f.h : Infinity;
  const ox2 = occ.x + occ.w;
  const oy2 = Number.isFinite(occ.h) ? occ.y + occ.h : Infinity;

  // Left
  if (occ.x > f.x + 1e-9) {
    out.push({ x: f.x, y: f.y, w: occ.x - f.x, h: f.h });
  }
  // Right
  if (ox2 < fx2 - 1e-9) {
    out.push({ x: ox2, y: f.y, w: fx2 - ox2, h: f.h });
  }
  // Bottom (smaller y)
  if (occ.y > f.y + 1e-9) {
    const w = Math.min(fx2, ox2) - Math.max(f.x, occ.x);
    if (w > 1e-9) {
      out.push({
        x: Math.max(f.x, occ.x),
        y: f.y,
        w,
        h: occ.y - f.y,
      });
    }
  }
  // Top (larger y)
  if (oy2 < fy2 - 1e-9) {
    const w = Math.min(fx2, ox2) - Math.max(f.x, occ.x);
    if (w > 1e-9) {
      const h = Number.isFinite(fy2) && Number.isFinite(oy2) ? fy2 - oy2 : Infinity;
      out.push({
        x: Math.max(f.x, occ.x),
        y: oy2,
        w,
        h: Number.isFinite(f.h) ? h : Infinity,
      });
    }
  }

  return out.filter((r) => r.w > 1e-9 && (r.h > 1e-9 || !Number.isFinite(r.h)));
}

function pruneFreeList(free) {
  for (let i = 0; i < free.length; i++) {
    for (let j = i + 1; j < free.length; j++) {
      if (containsRect(free[i], free[j])) {
        free.splice(j, 1);
        j--;
      } else if (containsRect(free[j], free[i])) {
        free.splice(i, 1);
        i--;
        break;
      }
    }
  }
}

function containsRect(a, b) {
  return (
    a.x <= b.x + 1e-9 &&
    a.y <= b.y + 1e-9 &&
    a.x + a.w >= b.x + b.w - 1e-9 &&
    a.y + a.h >= b.y + b.h - 1e-9
  );
}

function pieceVariants(panel, copyIndex) {
  const b = panelBounds(panel);
  const rot = rotatePanel90CW(panel);
  return {
    panel,
    panelRot90: rot,
    w: b.width,
    h: b.height,
    copyIndex,
    area: b.width * b.height,
    maxSide: Math.max(b.width, b.height),
    /** true = lock 90°, false = lock 0°, null = allow either */
    forceRot: null,
  };
}

function isMeaningfullyRotatable(piece, eps = 0.5) {
  return Math.abs(piece.w - piece.h) >= eps;
}

function mulberry32(seed) {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

function shuffleCopy(arr, rand) {
  const a = arr.map((p) => ({ ...p }));
  for (let i = a.length - 1; i > 0; i--) {
    const j = Math.floor(rand() * (i + 1));
    [a[i], a[j]] = [a[j], a[i]];
  }
  return a;
}

function sortByArea(list) {
  return list.sort((a, b) => b.area - a.area || b.maxSide - a.maxSide);
}

/**
 * Build packing attempts: selective 90° locks (others stay 0°) for auto-fit.
 */
function buildPackAttempts(pieces, { trials = 0, seed = 1, deep = false } = {}) {
  const attempts = [];
  const push = (list) => attempts.push(list.map((p) => ({ ...p })));

  const cloneFree = () => pieces.map((p) => ({ ...p, forceRot: null }));
  push(sortByArea(cloneFree()));
  push(cloneFree().sort((a, b) => b.maxSide - a.maxSide || b.area - a.area));
  push(cloneFree().sort((a, b) => b.h - a.h || b.w - a.w));
  push(cloneFree().sort((a, b) => b.w - a.w || b.h - a.h));
  push(cloneFree().sort((a, b) => a.copyIndex - b.copyIndex || b.area - a.area));

  push(sortByArea(pieces.map((p) => ({ ...p, forceRot: false }))));
  push(sortByArea(pieces.map((p) => ({ ...p, forceRot: true }))));

  const rotatable = pieces
    .map((p, i) => i)
    .filter((i) => isMeaningfullyRotatable(pieces[i]));

  // Systematic low-popcount masks: rotate those parts 90°, lock the rest at 0°.
  const maxPop = deep ? Math.min(4, rotatable.length) : Math.min(3, rotatable.length);
  for (const mask of rotationMasks(rotatable.length, maxPop)) {
    const rotSet = new Set();
    for (let b = 0; b < rotatable.length; b++) {
      if (mask & (1 << b)) rotSet.add(rotatable[b]);
    }
    push(
      sortByArea(
        pieces.map((p, j) => ({
          ...p,
          forceRot: rotSet.has(j),
        }))
      )
    );
  }

  const rand = mulberry32(seed);
  const nTrials = Math.max(0, Math.min(deep ? 64 : 32, trials | 0));
  for (let t = 0; t < nTrials; t++) {
    push(shuffleCopy(cloneFree(), rand));
  }
  for (const i of rotatable.slice(0, deep ? rotatable.length : 8)) {
    push(
      shuffleCopy(
        pieces.map((p, j) => ({
          ...p,
          forceRot: j === i,
        })),
        rand
      )
    );
  }

  return attempts;
}

/** Yield bitmasks with popcount 0..maxPop over `n` bits. */
function* rotationMasks(n, maxPop) {
  if (n <= 0) {
    yield 0;
    return;
  }
  // Cap explosion for many parts (multi-copy): only up to n=12 full enum.
  if (n > 12) {
    yield 0;
    for (let i = 0; i < n; i++) yield 1 << i;
    for (let i = 0; i < n; i++) {
      for (let j = i + 1; j < n; j++) yield (1 << i) | (1 << j);
    }
    return;
  }
  const limit = 1 << n;
  for (let mask = 0; mask < limit; mask++) {
    let bits = 0;
    for (let x = mask; x; x >>= 1) bits += x & 1;
    if (bits <= maxPop) yield mask;
  }
}

function candidateRank(c, binW, binH) {
  const finiteH = Number.isFinite(binH);
  const fits =
    c.nest.width <= binW + 1e-6 && (!finiteH || c.nest.height <= binH + 1e-6);
  const used = c.nest.width * c.nest.height;
  const sheetArea = finiteH ? binW * binH : binW * Math.max(c.nest.height, 1);
  const waste = Math.max(0, sheetArea - used);
  return {
    fits: fits ? 0 : 1,
    used,
    rotN: c.rotN,
    waste,
    span: c.nest.width + c.nest.height,
  };
}

function compareAutoFit(a, b, binW, binH) {
  const ra = candidateRank(a, binW, binH);
  const rb = candidateRank(b, binW, binH);
  // 1) Must fit the sheet
  if (ra.fits !== rb.fits) return ra.fits - rb.fits;
  // 2) Among fits: rotate as few parts as possible (selective 90°)
  if (ra.fits === 0 && ra.rotN !== rb.rotN) return ra.rotN - rb.rotN;
  // 3) Then pack tighter
  if (Math.abs(ra.used - rb.used) > 1) return ra.used - rb.used;
  if (Math.abs(ra.waste - rb.waste) > 1) return ra.waste - rb.waste;
  return ra.span - rb.span;
}

/**
 * Hill-climb: flip one part’s 90° lock at a time to improve sheet fit.
 * startRotIds: optional Set of panel ids that should start rotated.
 */
function hillClimbRotations(pieces, startRotIds, binW, binH, gap) {
  const rotatable = pieces
    .map((p, i) => i)
    .filter((i) => isMeaningfullyRotatable(pieces[i]));
  if (!rotatable.length) return [];

  let mask = 0;
  if (startRotIds && startRotIds.size) {
    for (let b = 0; b < rotatable.length; b++) {
      const id = pieces[rotatable[b]].panel?.id;
      if (id && startRotIds.has(id)) mask |= 1 << b;
    }
  }

  const tryMask = (m) => {
    const rotSet = new Set();
    for (let b = 0; b < rotatable.length; b++) {
      if (m & (1 << b)) rotSet.add(rotatable[b]);
    }
    const ordered = sortByArea(
      pieces.map((p, j) => ({
        ...p,
        forceRot: rotSet.has(j),
      }))
    );
    const packed = packPanelsMaxRects(ordered, { binW, binH, gap });
    if (!packed || packed.items.length !== pieces.length) return null;
    const rotN = packed.items.filter((it) => it.rotate90 || it.panel?.nestRotate90).length;
    return {
      nest: packed,
      score: packed.width * packed.height + packed.height * 1e-3,
      rotN,
      key: layoutFingerprint(packed),
      rotKey: rotationSignature(packed),
    };
  };

  const out = [];
  let cur = tryMask(mask);
  if (cur) out.push(cur);
  let improved = true;
  let guard = 0;
  while (improved && guard++ < 32) {
    improved = false;
    let best = cur;
    let bestMask = mask;
    for (let b = 0; b < rotatable.length; b++) {
      const nextMask = mask ^ (1 << b);
      const cand = tryMask(nextMask);
      if (!cand) continue;
      out.push(cand);
      if (!best || compareAutoFit(cand, best, binW, binH) < 0) {
        best = cand;
        bestMask = nextMask;
        improved = true;
      }
    }
    cur = best;
    mask = bestMask;
  }
  return out;
}

/**
 * Auto-fit panels into binW×binH using selective per-part 90° rotation.
 * layoutSeed 0 = best auto-fit; >0 cycles alternate layouts.
 */
export function nestPanelsOptimized(
  panels,
  { binW, binH = Infinity, gap = 8, trials = 48, layoutSeed = 0 } = {}
) {
  const pieces = panels.map((p) => pieceVariants(p, p._copyIndex ?? 0));
  const seedBase = (pieces.length * 17 + 11 + (layoutSeed | 0) * 997) >>> 0;
  const deep = pieces.length <= 14;

  const candidates = [];
  const consider = (packed) => {
    if (!packed || packed.items.length !== pieces.length) return;
    const rotN = packed.items.filter((it) => it.rotate90 || it.panel?.nestRotate90).length;
    candidates.push({
      nest: packed,
      score: packed.width * packed.height + packed.height * 1e-3,
      rotN,
      key: layoutFingerprint(packed),
      rotKey: rotationSignature(packed),
    });
  };

  for (const ordered of buildPackAttempts(pieces, { trials, seed: seedBase, deep })) {
    consider(packPanelsMaxRects(ordered, { binW, binH, gap }));
  }

  for (const climbed of hillClimbRotations(pieces, null, binW, binH, gap)) {
    candidates.push(climbed);
  }

  if (candidates.length) {
    const best = [...candidates].sort((a, b) => compareAutoFit(a, b, binW, binH))[0];
    const startRot = new Set(
      best.rotKey
        .split(",")
        .filter((s) => s.endsWith(":1"))
        .map((s) => s.split(":")[0])
    );
    for (const climbed of hillClimbRotations(pieces, startRot, binW, binH, gap)) {
      candidates.push(climbed);
    }
  }

  if (!candidates.length) return null;

  const unique = [];
  const seen = new Set();
  for (const c of candidates) {
    if (seen.has(c.key)) continue;
    seen.add(c.key);
    unique.push(c);
  }
  if (!unique.length) return null;

  const autofitOrder = [...unique].sort((a, b) => compareAutoFit(a, b, binW, binH));
  const varietyOrder = [...unique].sort((a, b) => {
    if (a.rotKey !== b.rotKey) return a.rotKey < b.rotKey ? -1 : 1;
    return compareAutoFit(a, b, binW, binH);
  });

  const ordered = (layoutSeed | 0) === 0 ? autofitOrder : varietyOrder;
  const idx = Math.abs(layoutSeed | 0) % ordered.length;
  const chosen = ordered[idx];
  return {
    ...chosen.nest,
    layoutIndex: idx,
    layoutCount: ordered.length,
    autoFit: (layoutSeed | 0) === 0,
    rotN: chosen.rotN,
  };
}

function rotationSignature(nest) {
  return nest.items
    .map((it) => `${it.panel?.id || "?"}:${it.rotate90 || it.panel?.nestRotate90 ? 1 : 0}`)
    .sort()
    .join(",");
}

function layoutFingerprint(nest) {
  return nest.items
    .map((it) => {
      const id = it.panel?.id || "?";
      const rot = it.rotate90 || it.panel?.nestRotate90 ? 1 : 0;
      return `${id}@${Math.round(it.x * 10)},${Math.round(it.y * 10)},r${rot}`;
    })
    .sort()
    .join("|");
}

/**
 * Optimize nesting so `copies` prints fit on a sheet with outer border safety.
 * Uses per-part 90° rotation (outer face stays up). Prefers flat sheet packing.
 */
export function packPrintsOnSheet(panels, opts = {}) {
  const sheetWidth = Math.max(1, Number(opts.sheetWidth) || 600);
  const sheetHeight = Math.max(1, Number(opts.sheetHeight) || 400);
  const copies = Math.max(1, Math.floor(Number(opts.copies) || 1));
  const border = Math.max(0, Number(opts.border) || 0);
  const gap = Math.max(0, Number(opts.gap) ?? 8);
  const layoutSeed = Math.max(0, Math.floor(Number(opts.layoutSeed) || 0));

  const availW = sheetWidth - 2 * border;
  const availH = sheetHeight - 2 * border;

  if (availW <= 0 || availH <= 0) {
    return emptyPackResult(sheetWidth, sheetHeight, copies, border, gap, "Border leaves no usable sheet area");
  }
  if (!panels.length) {
    return emptyPackResult(sheetWidth, sheetHeight, copies, border, gap, "No panels");
  }

  const oriented = orientPanelsOuterUp(panels);
  const trials = Math.max(32, Math.min(96, 24 + oriented.length * copies * 4));

  const allParts = [];
  for (let c = 0; c < copies; c++) {
    for (const p of oriented) {
      allParts.push({ ...p, id: `${p.id}__c${c}`, _copyIndex: c });
    }
  }

  // Strategy A: auto-fit all parts onto the sheet (selective per-part 90°).
  const combined = nestPanelsOptimized(allParts, {
    binW: availW,
    binH: availH,
    gap,
    trials,
    layoutSeed,
  });
  if (combined && combined.items.length === allParts.length) {
    const items = combined.items.map((it) => ({
      ...it,
      x: it.x + border,
      y: it.y + border,
    }));
    const rotated = items.filter((it) => it.rotate90 || it.panel?.nestRotate90).length;
    const layoutNote =
      combined.layoutCount > 1
        ? ` · layout ${combined.layoutIndex + 1}/${combined.layoutCount}`
        : "";
    const rotNote =
      rotated === 0
        ? ""
        : rotated === 1
          ? ", 1 part 90°"
          : `, ${rotated} parts 90°`;
    return {
      fits: true,
      sheetWidth,
      sheetHeight,
      copies,
      border,
      gap,
      layoutSeed,
      layoutIndex: combined.layoutIndex ?? 0,
      layoutCount: combined.layoutCount ?? 1,
      cols: 1,
      rows: 1,
      rotate90: false,
      nest: {
        items,
        width: sheetWidth,
        height: sheetHeight,
        gap,
        padding: 0,
      },
      blockW: combined.width,
      blockH: combined.height,
      placements: [{ x: 0, y: 0, rotate90: false, index: 0, flat: true }],
      flat: true,
      maxCopies: estimateMaxCopies(oriented, availW, availH, gap, trials),
      message:
        (copies > 1
          ? `Auto-fit ${copies}× on sheet${rotNote}`
          : `Auto-fit on sheet${rotNote}`) + layoutNote,
    };
  }

  // Strategy B: nest one print (with part rotations), tile copies; optional whole-block 90°.
  const binCandidates = uniquePositive([
    availW,
    availH,
    Math.min(availW, availH),
    availW * 0.5,
    availW * 0.66,
    availH * 0.5,
    Math.max(...oriented.map((p) => panelBounds(p).width)),
    Math.max(...oriented.map((p) => Math.max(panelBounds(p).width, panelBounds(p).height))),
  ]);

  let bestTiled = null;
  let bestSingle = null;
  for (const binW of binCandidates) {
    const nest = nestPanelsOptimized(oriented, {
      binW: Math.min(binW, Math.max(availW, availH)),
      binH: Infinity,
      gap,
      trials: Math.max(24, trials >> 1),
      layoutSeed,
    });
    if (!nest) continue;
    const contentW = nest.width;
    const contentH = nest.height;
    for (const rotate90 of [false, true]) {
      const blockW = rotate90 ? contentH : contentW;
      const blockH = rotate90 ? contentW : contentH;
      if (blockW > availW + 1e-9 || blockH > availH + 1e-9) continue;
      const maxN = maxCopiesThatFit(blockW, blockH, availW, availH, gap);
      const grid = bestCopyGrid(blockW, blockH, availW, availH, copies, gap);
      const cand = {
        nest,
        rotate90,
        blockW,
        blockH,
        maxCopies: maxN,
        cols: grid?.cols || 0,
        rows: grid?.rows || 0,
        usedW: grid?.usedW || 0,
        usedH: grid?.usedH || 0,
        score: grid ? grid.usedW * grid.usedH : 1e12,
        grid,
        area: contentW * contentH,
      };
      if (grid && (!bestTiled || cand.score < bestTiled.score)) {
        bestTiled = cand;
      }
      if (
        !bestSingle ||
        maxN > bestSingle.maxCopies ||
        (maxN === bestSingle.maxCopies && cand.area < bestSingle.area)
      ) {
        bestSingle = cand;
      }
    }
  }

  if (bestTiled?.grid) {
    const placements = [];
    let placed = 0;
    for (let row = 0; row < bestTiled.rows && placed < copies; row++) {
      for (let col = 0; col < bestTiled.cols && placed < copies; col++) {
        placements.push({
          x: border + col * (bestTiled.blockW + gap),
          y: border + row * (bestTiled.blockH + gap),
          rotate90: bestTiled.rotate90,
          index: placed,
          flat: false,
        });
        placed += 1;
      }
    }
    if (placements.length === copies) {
      const partRot = bestTiled.nest.items.filter((it) => it.rotate90 || it.panel?.nestRotate90).length;
      const lc = bestTiled.nest.layoutCount || 1;
      const li = bestTiled.nest.layoutIndex ?? 0;
      return {
        fits: true,
        sheetWidth,
        sheetHeight,
        copies,
        border,
        gap,
        layoutSeed,
        layoutIndex: li,
        layoutCount: lc,
        cols: bestTiled.cols,
        rows: bestTiled.rows,
        rotate90: bestTiled.rotate90,
        nest: bestTiled.nest,
        blockW: bestTiled.blockW,
        blockH: bestTiled.blockH,
        placements,
        flat: false,
        maxCopies: bestTiled.maxCopies,
        message: `Fits ${copies}× (${bestTiled.cols}×${bestTiled.rows}${bestTiled.rotate90 ? ", block 90°" : ""}${partRot ? `, ${partRot} parts 90°` : ""})${lc > 1 ? ` · layout ${li + 1}/${lc}` : ""}`,
      };
    }
  }

  const maxCopies = Math.max(
    bestSingle?.maxCopies || 0,
    estimateMaxCopies(oriented, availW, availH, gap, trials)
  );

  // Preview: pack one set into the sheet as flat as possible (shows 90° part rotations).
  const preview =
    nestPanelsOptimized(oriented, { binW: availW, binH: availH, gap, trials, layoutSeed }) ||
    bestSingle?.nest ||
    nestPanelsOptimized(oriented, {
      binW: availW,
      binH: Infinity,
      gap,
      trials: 24,
      layoutSeed,
    }) ||
    nestPanels(oriented, { gap, padding: 0, maxRowWidth: availW });

  const previewItems = (preview.items || []).map((it) => ({
    ...it,
    x: (it.x || 0) + border,
    y: (it.y || 0) + border,
  }));
  const previewFitsSheet =
    preview &&
    preview.width <= availW + 1e-6 &&
    preview.height <= availH + 1e-6 &&
    previewItems.length === oriented.length;

  return {
    fits: false,
    sheetWidth,
    sheetHeight,
    copies,
    border,
    gap,
    layoutSeed,
    layoutIndex: preview?.layoutIndex ?? 0,
    layoutCount: preview?.layoutCount ?? 1,
    cols: bestSingle?.cols || 0,
    rows: bestSingle?.rows || 0,
    rotate90: false,
    nest: {
      items: previewItems,
      width: sheetWidth,
      height: sheetHeight,
      gap,
      padding: 0,
    },
    blockW: preview?.width || 0,
    blockH: preview?.height || 0,
    placements: [{ x: 0, y: 0, rotate90: false, index: 0, flat: true }],
    flat: true,
    maxCopies,
    message: previewFitsSheet
      ? `Does not fit ${copies}× — max ${maxCopies} (1 set packs with part rotations)`
      : `Does not fit ${copies}× — max ${maxCopies} on this sheet`,
  };
}

function estimateMaxCopies(oriented, availW, availH, gap, trials = 32) {
  let lo = 0;
  let hi = Math.min(32, Math.max(2, Math.floor((availW * availH) / 5000)));
  while (lo < hi) {
    const mid = Math.ceil((lo + hi + 1) / 2);
    const parts = [];
    for (let c = 0; c < mid; c++) {
      for (const p of oriented) parts.push({ ...p, id: `${p.id}__c${c}`, _copyIndex: c });
    }
    const packed = nestPanelsOptimized(parts, {
      binW: availW,
      binH: availH,
      gap,
      trials: Math.max(16, trials >> 1),
    });
    if (packed && packed.items.length === parts.length) lo = mid;
    else hi = mid - 1;
  }
  return lo;
}

function uniquePositive(vals) {
  const s = new Set();
  for (const v of vals) {
    if (Number.isFinite(v) && v > 0) s.add(Math.round(v * 1000) / 1000);
  }
  return [...s].sort((a, b) => a - b);
}

function emptyPackResult(sheetWidth, sheetHeight, copies, border, gap, message) {
  return {
    fits: false,
    sheetWidth,
    sheetHeight,
    copies,
    border,
    gap,
    cols: 0,
    rows: 0,
    rotate90: false,
    nest: { items: [], width: 0, height: 0, gap, padding: 0 },
    blockW: 0,
    blockH: 0,
    placements: [],
    flat: false,
    maxCopies: 0,
    message,
  };
}

/**
 * Map a point in nest-local space into a rotated block (90° CW, origin at block top-left).
 */
export function mapNestPoint(x, y, nestW, nestH, rotate90) {
  if (!rotate90) return { x, y };
  return { x: y, y: nestW - x };
}
