/**
 * Build templates/revG-holes.json from TOERN_M1_FINAL.dxf + toern_revG PCB cues.
 * Run: node scripts/build-revg-template.mjs
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { createDefaultDoc } from "../model.js";
import { unfold } from "../unfold.js";
import { captureFeatureTemplate } from "../feature-templates.js";
import { createCircleFeature, createRoundRectFeature } from "../features.js";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.join(__dirname, "..");

const doc = createDefaultDoc();
doc.fixedPanels = null;
doc.thickness = 3;
doc.depth = 160;
doc.kerf = -0.125;
doc.fingerLength = 14;
doc.minFingerWidth = 5;
doc.invertFingers = true;
doc.features = [];

const pts = doc.profile.points;
const originIdx = pts.findIndex((p) => Math.abs(p.x) < 1e-6 && Math.abs(p.y) < 1e-6);
if (originIdx < 0) throw new Error("FRONT profile missing (0,0) corner");
doc.templateDatum = {
  vertexIndex: originIdx,
  x: pts[originIdx].x,
  y: pts[originIdx].y,
};

const { panels, error } = unfold(doc);
if (error) throw new Error(error);
const byId = Object.fromEntries(panels.map((p) => [p.id, p]));

// —— Wall 0 (top ~72 mm lip): 4× Neutrik Ø9.6 from DXF Ø9.5 row, even along depth ——
{
  const p = byId["wall-0"];
  const padX = (72 - p.width) / 2;
  const cx = 41.24 - padX;
  const z0 = 20;
  const z1 = 140;
  for (let i = 0; i < 4; i++) {
    const y = z0 + (i * (z1 - z0)) / 3;
    doc.features.push(createCircleFeature("wall-0", cx, y, 4.8));
  }
}

// —— Wall 5 (left): USB-C slot (revG J4) ——
{
  const p = byId["wall-5"];
  const cx = p.width / 2;
  doc.features.push(createRoundRectFeature("wall-5", cx - 4.5, 28 - 1.75, 9.0, 3.5, 100));
}

// —— Wall 3 (right): MIDI TRS Ø6.5 (revG J3/J17) + power switch slot (S1) ——
{
  const p = byId["wall-3"];
  const cx = p.width / 2;
  doc.features.push(createCircleFeature("wall-3", cx, 137, 3.25));
  doc.features.push(createCircleFeature("wall-3", cx, 150, 3.25));
  doc.features.push(createRoundRectFeature("wall-3", cx - 2.0, 47 - 1.0, 4.0, 2.0, 50));
}

// —— Wall 2 (long upper): microSD slot (revG J8) ——
{
  const p = byId["wall-2"];
  const along0 = ((p.width || p.edgeLength) - p.edgeLength) * 0.5;
  const along = p.edgeLength * 0.45;
  const localY = 90;
  doc.features.push(
    createRoundRectFeature("wall-2", along0 + along - 6, localY - 1.1, 12, 2.2, 100)
  );
}

// —— End B: mic + pilot (DXF end bay Ø12 → tighter mic for POM-2244) ——
doc.features.push(createCircleFeature("endB", 40, 15.5, 4.0));
doc.features.push(createCircleFeature("endB", 58, 15.5, 1.5));

// —— End A: DXF right-side pair on the short lip (local Y within y=-13.5…0) ——
doc.features.push(createCircleFeature("endA", 222.4, 20.0, 3.4));
doc.features.push(createCircleFeature("endA", 236.2, 20.0, 3.4));

// —— Wall 4 (bottom): PCB M2 mounts from revG, laser clearance r=1.25 ——
{
  const pcb = { xmin: 30.98, ymin: 25.04, h: 70.96 };
  const mounts = [
    [159.55, 31.15],
    [34.35, 61.85],
    [188.05, 68.8],
    [119.6, 69.5],
    [71.45, 74.7],
  ];
  const p = byId["wall-4"];
  const marginAlong = (p.edgeLength - pcb.h) / 2;
  const along0 = ((p.width || p.edgeLength) - p.edgeLength) * 0.5;
  for (const [px, py] of mounts) {
    const along = marginAlong + (py - pcb.ymin);
    const localY = Math.max(2, Math.min(doc.depth - 2, px - pcb.xmin));
    doc.features.push(createCircleFeature("wall-4", along0 + along, localY, 1.25));
  }
}

const captured = captureFeatureTemplate(doc, panels, "TŒRN revG holes");
if (captured.error) throw new Error(captured.error);

const template = {
  ...captured.template,
  id: "tpl-revg-from-dxf",
  name: "TŒRN revG holes",
  source: {
    dxf: "TOERN_M1_FINAL.dxf",
    pcb: "PCB/toern_revG",
  },
  notes:
    "M1 DXF hole faces/diameters, nudged toward toern_revG (Neutrik row, USB-C, MIDI TRS, microSD, M2 mounts, mic). Template origin: FRONT (0,0).",
};

const outPath = path.join(root, "templates", "revG-holes.json");
fs.mkdirSync(path.dirname(outPath), { recursive: true });
fs.writeFileSync(outPath, JSON.stringify(template, null, 2));
console.log(`wrote ${outPath} (${template.entries.length} entries, skipped ${captured.skipped})`);
