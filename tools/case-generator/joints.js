/** Finger-joint edge geometry */

export function dist(a, b) {
  const dx = b.x - a.x;
  const dy = b.y - a.y;
  return Math.hypot(dx, dy);
}

export function sub(a, b) {
  return { x: a.x - b.x, y: a.y - b.y };
}

export function add(a, b) {
  return { x: a.x + b.x, y: a.y + b.y };
}

export function scale(v, s) {
  return { x: v.x * s, y: v.y * s };
}

export function normalize(v) {
  const len = Math.hypot(v.x, v.y) || 1;
  return { x: v.x / len, y: v.y / len };
}

/** Outward normal for CCW polygon edge direction */
export function outwardNormal(dir) {
  return { x: dir.y, y: -dir.x };
}

/**
 * Offset a CCW polygon by `delta` along outward normals (positive expands).
 * Simple miter join; adequate for finger-jointed laser outlines.
 */
export function offsetPolygon(points, delta) {
  if (!points?.length || !delta) return points.map((p) => ({ x: p.x, y: p.y }));
  const pts = ensureCCW(points);
  const n = pts.length;
  const out = [];
  for (let i = 0; i < n; i++) {
    const prev = pts[(i - 1 + n) % n];
    const cur = pts[i];
    const next = pts[(i + 1) % n];
    const d0 = normalize(sub(cur, prev));
    const d1 = normalize(sub(next, cur));
    const n0 = outwardNormal(d0);
    const n1 = outwardNormal(d1);
    let nx = n0.x + n1.x;
    let ny = n0.y + n1.y;
    const len = Math.hypot(nx, ny);
    if (len < 1e-9) {
      nx = n0.x;
      ny = n0.y;
    } else {
      nx /= len;
      ny /= len;
      // miter scale
      const dot = Math.max(-1, Math.min(1, n0.x * nx + n0.y * ny));
      const miter = Math.min(4, 1 / Math.max(0.2, dot));
      nx *= miter;
      ny *= miter;
    }
    out.push({ x: cur.x + nx * delta, y: cur.y + ny * delta });
  }
  return out;
}

export function ensureCCW(points) {
  let area = 0;
  const n = points.length;
  for (let i = 0; i < n; i++) {
    const a = points[i];
    const b = points[(i + 1) % n];
    area += a.x * b.y - b.x * a.y;
  }
  if (area < 0) return points.slice().reverse();
  return points.slice();
}

/** Default floor (mm): fingers at or below this are ignored (plain edge). */
export const MIN_FINGER_LENGTH = 5;

/** @deprecated Use MIN_FINGER_LENGTH — same default floor for all edges. */
export const MIN_FINGER_LENGTH_DIAGONAL = MIN_FINGER_LENGTH;

/** True if a finger segment is strictly longer than `min` (default 5mm). */
export function isValidFingerLength(w, min = MIN_FINGER_LENGTH) {
  return w > min;
}

/** True if edge is horizontal or vertical. */
export function isAxisAligned(a, b, eps = 1e-6) {
  return Math.abs(b.x - a.x) <= eps || Math.abs(b.y - a.y) <= eps;
}

/**
 * True when prev→cur→next is a convex ~90° corner on a CCW profile.
 * Wall–wall side fingers only assemble cleanly at these corners; diagonal
 * and concave corners would drive male tabs through the neighboring face.
 */
export function isConvexOrthogonalCorner(prev, cur, next, eps = 0.12) {
  const u = normalize(sub(cur, prev));
  const v = normalize(sub(next, cur));
  // CCW profile: left turn (positive cross) ⇒ convex vertex
  const cross = u.x * v.y - u.y * v.x;
  if (cross <= 1e-9) return false;
  return Math.abs(u.x * v.x + u.y * v.y) <= eps;
}

/**
 * True when prev→cur→next is a concave vertex on a CCW profile (right turn).
 * Finger tabs on edges into a concave corner self-intersect or pierce neighbors.
 */
export function isConcaveCorner(prev, cur, next) {
  const u = normalize(sub(cur, prev));
  const v = normalize(sub(next, cur));
  return u.x * v.y - u.y * v.x < -1e-9;
}

/**
 * Symmetric finger widths along an edge of length L.
 * Inner fingers are exactly `fingerLength`; both end fingers share the remainder
 * equally (longer or shorter) so the pattern stays symmetric. Odd count so both
 * ends have the same gender.
 *
 * Returns `null` when no layout has every segment above `minFingerLength`.
 *
 * @param {number} L
 * @param {number} fingerLength
 * @param {{ minFingerLength?: number }} [opts]
 * @returns {number[] | null}
 */
export function fingerWidths(L, fingerLength, opts = {}) {
  const min = opts.minFingerLength ?? MIN_FINGER_LENGTH;
  if (!(L > min)) return null;

  const preferred = fingerLength || min + 1e-6;
  const f = Math.max(min + 1e-6, preferred);

  // Single span — only valid if the whole edge clears the minimum
  if (L < f * 1.25) {
    return isValidFingerLength(L, min) ? [L] : null;
  }

  const minEnd = Math.max(f * 0.25, min + 1e-6);
  let bestN = 0;
  let bestScore = Infinity;

  for (let n = 1; n <= 199; n += 2) {
    if (n === 1) {
      // Prefer a real alternating joint whenever three+ fingers can fit.
      // A single span is only a plain tongue/groove and leaves invert mode
      // with a flat female edge (no interlocking cut).
      if (L >= 3 * (min + 1e-6) + f * 0.5) continue;
      if (!isValidFingerLength(L, min)) continue;
      const score = Math.abs(L - f) + 1e6;
      if (score < bestScore) {
        bestScore = score;
        bestN = 1;
      }
      continue;
    }
    const inner = n - 2;
    const endW = (L - inner * f) / 2;
    if (!isValidFingerLength(endW, min)) continue;
    if (endW < minEnd) continue;
    if (endW > f * 2.5) continue;
    const score = Math.abs(endW - f);
    if (score < bestScore) {
      bestScore = score;
      bestN = n;
    }
  }

  if (!bestN) return null;
  if (bestN === 1) return [L];
  const endW = (L - (bestN - 2) * f) / 2;
  if (!isValidFingerLength(endW, min) || !isValidFingerLength(f, min)) return null;
  const widths = [endW];
  for (let i = 0; i < bestN - 2; i++) widths.push(f);
  widths.push(endW);
  return widths;
}

/** Finger count (odd), from symmetric layout. 0 if no valid fingers. */
export function fingerCount(L, fingerLength, opts) {
  return fingerWidths(L, fingerLength, opts)?.length || 0;
}

/**
 * Emit finger zigzag along edge from `start` to `end`.
 *
 * Male segments protrude outward by thickness. Female/gap segments stay on the
 * nominal baseline (never cut inward) — that avoids self-intersecting “negative”
 * corner notches when two female ends meet.
 *
 * Callers that want finished outside dimensions equal to the drawn profile
 * should pass an inset body polygon so male tips restore the outer envelope.
 *
 * Every finger is a true rectangle in edge space: sides run along the outward
 * normal (±90° to the edge), tips run parallel to the edge. Works for diagonal
 * profile edges the same way as axis-aligned ones.
 *
 * `endClearance`: default plain baseline at both ends. `startClearance` and
 * `finishClearance` override either side for asymmetric butt-joint keep-outs.
 *
 * Returns { points, startMale, endMale }. Plain baseline when no finger exceeds min.
 */
export function fingerEdgePoints(start, end, {
  thickness,
  fingerLength,
  startMale,
  kerf = 0,
  endClearance = 0,
  startClearance,
  finishClearance,
  minFingerLength,
}) {
  const L = dist(start, end);
  const plain = () => ({
    points: [
      { x: start.x, y: start.y },
      { x: end.x, y: end.y },
    ],
    startMale: false,
    endMale: false,
  });

  if (L < 1e-9) {
    return {
      points: [{ x: start.x, y: start.y }],
      startMale: false,
      endMale: false,
    };
  }

  const dir = normalize(sub(end, start));
  const out = outwardNormal(dir);
  const clearStart = Math.min(
    Math.max(0, startClearance ?? endClearance),
    L * 0.9
  );
  const clearEnd = Math.min(
    Math.max(0, finishClearance ?? endClearance),
    Math.max(0, L * 0.9 - clearStart)
  );
  const usable = L - clearStart - clearEnd;
  const min = minFingerLength ?? MIN_FINGER_LENGTH;

  // Fingers at or below the min width are ignored (plain join)
  if (!isValidFingerLength(usable, min)) return plain();

  const widths = fingerWidths(usable, fingerLength, { minFingerLength: min });
  if (!widths?.length || widths.some((w) => !isValidFingerLength(w, min))) {
    return plain();
  }

  const n = widths.length;
  const maleDepth = Math.max(0, thickness - kerf * 0.5);
  const fingerMale = (i) => (startMale ? i % 2 === 0 : i % 2 === 1);

  const at = (s, elevated) => {
    const p = add(start, scale(dir, s));
    return elevated ? add(p, scale(out, maleDepth)) : { x: p.x, y: p.y };
  };

  const pts = [];
  const push = (p) => {
    const last = pts[pts.length - 1];
    if (last && dist(last, p) < 1e-9) return;
    pts.push(p);
  };

  // Height-toggle walk: only move along `dir` or along `out` → every corner is 90°.
  // With no clearance, keep start/end elevated when the end finger is male so
  // ortho male–male corners can still form an outer L bridge.
  let elevated = clearStart <= 1e-9 && fingerMale(0);
  if (clearStart > 1e-9) {
    push({ x: start.x, y: start.y });
    push(at(clearStart, false));
    elevated = false;
  } else {
    push(at(0, elevated));
  }

  let cursor = clearStart;
  for (let i = 0; i < n; i++) {
    const wantMale = fingerMale(i);
    const s0 = cursor;
    const s1 = cursor + widths[i];
    if (wantMale !== elevated) {
      push(at(s0, wantMale));
      elevated = wantMale;
    }
    push(at(s1, elevated));
    cursor = s1;
  }

  if (clearEnd > 1e-9) {
    if (elevated) {
      push(at(cursor, false));
      elevated = false;
    }
    push({ x: end.x, y: end.y });
  }

  const geomStartMale = clearStart > 1e-9 ? false : fingerMale(0);
  const geomEndMale = clearEnd > 1e-9 ? false : fingerMale(n - 1);

  return {
    points: simplifyColinear(pts),
    startMale: geomStartMale,
    endMale: geomEndMale,
  };
}

/** Back-compat: plain point array for tests / callers that expect points only. */
export function fingerEdgePointList(start, end, spec) {
  return fingerEdgePoints(start, end, spec).points;
}

function simplifyColinear(pts) {
  if (pts.length < 3) return pts;
  const out = [pts[0]];
  for (let i = 1; i < pts.length - 1; i++) {
    const a = out[out.length - 1];
    const b = pts[i];
    const c = pts[i + 1];
    const abx = b.x - a.x;
    const aby = b.y - a.y;
    const bcx = c.x - b.x;
    const bcy = c.y - b.y;
    const cross = abx * bcy - aby * bcx;
    const dot = abx * bcx + aby * bcy;
    // Drop near-duplicates and exact backtracks on same line
    if (Math.hypot(b.x - a.x, b.y - a.y) < 1e-9) continue;
    if (Math.abs(cross) < 1e-8 && dot >= 0) continue;
    out.push(b);
  }
  out.push(pts[pts.length - 1]);
  return out;
}

/**
 * Clean a profile polyline before finger generation.
 * - drop near-duplicate consecutive points
 * - merge colinear intermediate vertices
 */
export function simplifyPolyline(points, { closed = false, eps = 1e-4 } = {}) {
  if (!points?.length) return [];
  let pts = points.map((p) => ({ x: p.x, y: p.y }));

  const dedupe = (list) => {
    const out = [];
    for (const p of list) {
      if (!out.length || dist(out[out.length - 1], p) > eps) out.push(p);
    }
    if (closed && out.length > 1 && dist(out[0], out[out.length - 1]) <= eps) out.pop();
    return out;
  };

  pts = dedupe(pts);
  const minPts = closed ? 3 : 2;
  if (pts.length < minPts) return pts;

  let guard = 0;
  let changed = true;
  while (changed && pts.length >= minPts && guard++ < 64) {
    changed = false;
    const next = [];
    const n = pts.length;
    for (let i = 0; i < n; i++) {
      if (!closed && (i === 0 || i === n - 1)) {
        next.push(pts[i]);
        continue;
      }
      const prev = pts[(i - 1 + n) % n];
      const cur = pts[i];
      const nxt = pts[(i + 1) % n];
      const abx = cur.x - prev.x;
      const aby = cur.y - prev.y;
      const bcx = nxt.x - cur.x;
      const bcy = nxt.y - cur.y;
      const cross = abx * bcy - aby * bcx;
      const scale = Math.max(Math.hypot(abx, aby), Math.hypot(bcx, bcy), 1);
      if (Math.abs(cross) <= eps * scale) {
        changed = true;
        continue;
      }
      next.push(cur);
    }
    if (next.length < minPts) break;
    pts = dedupe(next);
  }
  return pts;
}

function orient(a, b, c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

function nearlySame(a, b, eps = 1e-6) {
  return Math.hypot(a.x - b.x, a.y - b.y) <= eps;
}

/** True if closed polygon edges properly cross (not just shared vertices). */
export function polygonSelfIntersects(points, eps = 1e-7) {
  if (!points || points.length < 4) return false;
  const n = points.length;
  for (let i = 0; i < n; i++) {
    const a = points[i];
    const b = points[(i + 1) % n];
    for (let j = i + 1; j < n; j++) {
      if (j === i + 1 || (i === 0 && j === n - 1)) continue;
      const c = points[j];
      const d = points[(j + 1) % n];
      if (
        nearlySame(a, c, eps) ||
        nearlySame(a, d, eps) ||
        nearlySame(b, c, eps) ||
        nearlySame(b, d, eps)
      ) {
        continue;
      }
      const o1 = orient(a, b, c);
      const o2 = orient(a, b, d);
      const o3 = orient(c, d, a);
      const o4 = orient(c, d, b);
      if (o1 * o2 < 0 && o3 * o4 < 0) return true;
    }
  }
  return false;
}

/**
 * Corner bridge between two fingered edges.
 * Male–male on ~orthogonal corners: outer L via tipP+tipC−corner.
 * Male–male on diagonal / non-ortho corners: bevel (no bridge point) —
 * the outer parallelogram tip is only valid for axis-aligned joins and
 * otherwise creates self-intersecting triangular spikes.
 * Mixed / female: visit nominal corner (gaps on baseline).
 */
function cornerBridgePoints(prev, cur, corner) {
  const tipP = prev.points[prev.points.length - 1];
  const tipC = cur.points[0];
  if (!tipP || !tipC) {
    return [{ x: corner.x, y: corner.y }];
  }

  const maleMale = prev.endMale === true && cur.startMale === true;
  if (!maleMale) {
    return [{ x: corner.x, y: corner.y }];
  }

  if (!cornersNearlyOrthogonal(prev, cur)) {
    // Bevel: path connects tipP → tipC directly (no miter spike)
    return [];
  }

  const outer = {
    x: tipP.x + tipC.x - corner.x,
    y: tipP.y + tipC.y - corner.y,
  };
  // Guard: absurd miter (numerical / near-straight) → bevel
  const miter = Math.hypot(outer.x - corner.x, outer.y - corner.y);
  const tipSpan = Math.hypot(tipP.x - tipC.x, tipP.y - tipC.y) + 1e-9;
  if (miter > tipSpan * 1.5) return [];

  return [outer];
}

/** True when consecutive edges meet at ~90° (rectilinear corner). */
function cornersNearlyOrthogonal(prev, cur, eps = 0.12) {
  const d0 = normalize(sub(prev.cornerEnd, prev.cornerStart));
  const d1 = normalize(sub(cur.cornerEnd, cur.cornerStart));
  return Math.abs(d0.x * d1.x + d0.y * d1.y) <= eps;
}

/**
 * Build a closed path for a polygon with per-edge finger specs.
 * edgeSpecs[i] = null (plain) | { startMale, thickness, fingerLength, kerf }
 *
 * Orthogonal male–male corners get an outer L; diagonal corners are beveled
 * so the outline never self-intersects.
 */
export function fingeredPolygon(points, edgeSpecs) {
  const pts = ensureCCW(points);
  const n = pts.length;
  const edges = [];

  for (let i = 0; i < n; i++) {
    const a = pts[i];
    const b = pts[(i + 1) % n];
    const spec = edgeSpecs[i];
    if (!spec) {
      edges.push({
        points: [
          { x: a.x, y: a.y },
          { x: b.x, y: b.y },
        ],
        startMale: null,
        endMale: null,
        cornerStart: a,
        cornerEnd: b,
      });
    } else {
      const fe = fingerEdgePoints(a, b, spec);
      edges.push({
        points: fe.points,
        startMale: fe.startMale,
        endMale: fe.endMale,
        cornerStart: a,
        cornerEnd: b,
      });
    }
  }

  const path = [];
  for (let i = 0; i < n; i++) {
    const prev = edges[(i - 1 + n) % n];
    const cur = edges[i];
    const corner = cur.cornerStart;

    if (i === 0) {
      if (cur.startMale === null) {
        path.push({ x: corner.x, y: corner.y });
      }
      path.push(...cur.points);
      continue;
    }

    path.push(...cornerBridgePoints(prev, cur, corner));
    path.push(...cur.points);
  }

  // Close last → first
  const last = edges[n - 1];
  const first = edges[0];
  path.push(...cornerBridgePoints(last, first, first.cornerStart));

  return simplifyColinear(path);
}

/** True if every edge is axis-aligned (rectilinear profile). */
export function isOrthogonalPolygon(points, eps = 1e-6) {
  if (!points || points.length < 3) return false;
  const n = points.length;
  for (let i = 0; i < n; i++) {
    const a = points[i];
    const b = points[(i + 1) % n];
    const dx = Math.abs(b.x - a.x);
    const dy = Math.abs(b.y - a.y);
    if (dx > eps && dy > eps) return false;
    if (dx <= eps && dy <= eps) return false;
  }
  return true;
}

/**
 * Rectangle from (0,0) to (w,h), CCW: BL, BR, TR, TL
 * edgeSpecs: [bottom, right, top, left]
 */
export function fingeredRect(w, h, edgeSpecs) {
  const points = [
    { x: 0, y: 0 },
    { x: w, y: 0 },
    { x: w, y: h },
    { x: 0, y: h },
  ];
  return fingeredPolygon(points, edgeSpecs);
}

/** SVG path `d` from points (closed). */
export function pointsToPathD(points, closed = true) {
  if (!points.length) return "";
  let d = `M ${fmt(points[0].x)} ${fmt(points[0].y)}`;
  for (let i = 1; i < points.length; i++) {
    d += ` L ${fmt(points[i].x)} ${fmt(points[i].y)}`;
  }
  if (closed) d += " Z";
  return d;
}

function fmt(n) {
  const v = Math.round(n * 1000) / 1000;
  return String(v);
}

/** Circle path approximating with SVG circle element helpers */
export function circlePathD(cx, cy, r) {
  // Use two arcs for a full circle
  const x0 = fmt(cx + r);
  const y0 = fmt(cy);
  const rr = fmt(r);
  return `M ${x0} ${y0} A ${rr} ${rr} 0 1 0 ${fmt(cx - r)} ${y0} A ${rr} ${rr} 0 1 0 ${x0} ${y0} Z`;
}

export function roundRectPathD(x, y, w, h, r) {
  const rr = Math.max(0, Math.min(r, w / 2, h / 2));
  if (rr <= 0) {
    return pointsToPathD([
      { x, y },
      { x: x + w, y },
      { x: x + w, y: y + h },
      { x, y: y + h },
    ]);
  }
  const x2 = x + w;
  const y2 = y + h;
  return [
    `M ${fmt(x + rr)} ${fmt(y)}`,
    `L ${fmt(x2 - rr)} ${fmt(y)}`,
    `A ${fmt(rr)} ${fmt(rr)} 0 0 1 ${fmt(x2)} ${fmt(y + rr)}`,
    `L ${fmt(x2)} ${fmt(y2 - rr)}`,
    `A ${fmt(rr)} ${fmt(rr)} 0 0 1 ${fmt(x2 - rr)} ${fmt(y2)}`,
    `L ${fmt(x + rr)} ${fmt(y2)}`,
    `A ${fmt(rr)} ${fmt(rr)} 0 0 1 ${fmt(x)} ${fmt(y2 - rr)}`,
    `L ${fmt(x)} ${fmt(y + rr)}`,
    `A ${fmt(rr)} ${fmt(rr)} 0 0 1 ${fmt(x + rr)} ${fmt(y)}`,
    "Z",
  ].join(" ");
}
