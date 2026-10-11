// Exercise the real simulator controls with mocked audio, SD and browser boundaries.
import assert from "node:assert/strict";
import { drawSubmenu, drawMenu } from "../standalone-tools/web-standalone/src/firmware/screens.js";
import { build } from "../standalone-tools/web-standalone/node_modules/esbuild/lib/main.js";
import { mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
const testDir = await mkdtemp(join(tmpdir(), "toern-menu-test-"));
const bundlePath = join(testDir, "device.mjs");
const devicePath = fileURLToPath(new URL("../standalone-tools/web-standalone/src/firmware/device.js", import.meta.url));
await build({ entryPoints: [devicePath], outfile: bundlePath, bundle: true, format: "esm", platform: "node", plugins: [{ name: "test-boundaries", setup(b) {
  b.onResolve({ filter: /virtual:sd/ }, () => ({ path: "sd", namespace: "stub" }));
  b.onResolve({ filter: /audio\/engine\.js/ }, () => ({ path: "engine", namespace: "stub" }));
  b.onLoad({ filter: /.*/, namespace: "stub" }, ({ path }) => ({ contents: path === "sd" ? "export default []" : 'export function createEngine(){return new Proxy({}, {get:(_,key)=>key==="now"?()=>0:key==="ensure"?()=>({state:"running"}):()=>{}})}' }));
} }] });
let now = 1, frame;
globalThis.performance = { now: () => now };
globalThis.requestAnimationFrame = (fn) => {
  frame = fn;
};
globalThis.setTimeout = () => 0;
globalThis.setInterval = () => 0;
const stored = /* @__PURE__ */ new Map();
globalThis.localStorage = { getItem: (k) => stored.get(k) ?? null, setItem: (k, v) => stored.set(k, v) };
globalThis.navigator = {};
globalThis.window = { addEventListener() {
} };
globalThis.document = { activeElement: null, addEventListener() {
} };
const { createDevice } = await import(pathToFileURL(bundlePath));
const textDraws = [];
let litPixels = 0;
const matrix = new Proxy({
  rings: Array.from({ length: 4 }, () => [0, 0, 0]),
  pixels: new Uint8Array(32 * 16 * 3),
  drawText: (...args) => textDraws.push(args),
  light: () => { litPixels++; },
}, { get: (o, k) => o[k] ?? (() => {}) });
const d = createDevice(matrix, {}, []);
d.start();
frame(now);
now = 6e3;
frame(now);
assert.equal(d.mode(), "draw");
function open(root) {
  d.jump("menu");
  for (let i = 0; i < root; i++) d.rotate(3, 1);
  d.press(3);
}
function page(i) {
  while (d.subIndex() < i) d.rotate(3, 1);
  while (d.subIndex() > i) d.rotate(3, -1);
}
open(5);
assert.equal(d.menuSummary().detail, "VIEW: FULL");
d.rotate(2, 1);
assert.equal(d.menuSummary().detail, "VIEW: EASY");
page(1);
d.rotate(2, 1);
assert.equal(d.menuSummary().detail, "CTRL: VOL");
page(3);
d.rotate(1, 1);
d.rotate(2, 1);
frame(now);
open(8);
assert.equal(d.mode(), "mode");
assert.match(d.menuSummary().detail, /^FLOW:/);
page(1);
d.rotate(2, 1);
assert.equal(d.menuSummary().detail, "PMOD: ON");
page(4);
d.rotate(2, 1);
assert.equal(d.menuSummary().detail, "CHLD: ON");
page(5);
d.rotate(2, 1);
page(3);
d.rotate(2, 1);
assert.equal(d.menuSummary().detail, "VMOD: OFF");
page(7);
d.press(3);
assert.equal(d.mode(), "song");
open(9);
page(3);
d.press(3);
assert.equal(d.mode(), "adv");
assert.match(d.menuSummary().detail, /^CRSR:/);
d.rotate(2, 1);
assert.equal(d.menuSummary().detail, "CRSR: CHNR");
page(4);
d.rotate(2, 1);
assert.equal(d.menuSummary().detail, "COLR: 1");
page(6);
d.rotate(2, 1);
d.rotate(0, 1);
d.rotate(1, 1);
d.press(2);
d.rotate(2, 1);
frame(now);
open(4);
page(3);
d.rotate(1, 1);
d.rotate(2, 1);
frame(now);
let saved = JSON.parse(stored.get("toern-web-eeprom"));
assert.equal(saved.subPick.look[1], 1);
assert.equal(saved.subPick.look[2], 1);
assert.equal(saved.subPick.look[6], 1);
assert.equal(saved.subPick.look[9], 1);
assert.equal(saved.subPick.etc[5], 1);
assert.equal(saved.subPick.etc[8], 1);
assert.equal(saved.subPick.etc[10], 1);
const reloaded = createDevice(matrix, {}, []);
reloaded.start();
frame(6001);
frame(12e3);
reloaded.jump("menu");
for (let i = 0; i < 5; i++) reloaded.rotate(3, 1);
reloaded.press(3);
assert.equal(reloaded.menuSummary().detail, "VIEW: EASY");
// Toggle labels use one position and one color across different menu families.
d.start();
function expectToggle(text, color, y = 3) {
  textDraws.length = 0;
  frame(now);
  const label = textDraws.find(args => args[0] === text);
  assert.deepEqual(label, [text, 2, y, color]);
}
open(8);
page(6);
expectToggle("OFF", [120, 0, 0]);
d.rotate(2, 1);
expectToggle("ON", [0, 120, 0]);
page(5);
d.rotate(2, -1);
expectToggle("OFF", [120, 0, 0]);
page(2);
expectToggle("OFF", [120, 0, 0]);
open(9);
page(3);
d.press(3);
page(3);
expectToggle("OFF", [120, 0, 0]);
open(4);
page(3);
d.rotate(1, -1);
expectToggle("ON", [0, 120, 0], 10);
// SONG is a plain submenu label; the MODE root retains its icon.
textDraws.length = 0;
litPixels = 0;
drawSubmenu(matrix, { mode: "mode", subIndex: 7 }, now);
assert.equal(litPixels, 9); // Only the nine submenu navigation dots.
assert.deepEqual(textDraws.map(args => args.slice(0, 3)), [["SONG", 2, 10]]);
textDraws.length = 0;
litPixels = 0;
drawSubmenu(matrix, { mode: "etc", subIndex: 3 }, now);
assert.equal(litPixels, 5); // ADV has only the five ETC navigation dots.
assert.deepEqual(textDraws.map(args => args.slice(0, 3)), [["ADV", 2, 10]]);
litPixels = 0;
drawMenu(matrix, { menuIndex: 8 });
assert(litPixels > 10, "MODE still draws its root icon");
// Remapping a displayed submenu must keep the animation state on the real device.
const infoState = { mode: "etc", subIndex: 0, infoAt: 0 };
drawSubmenu(matrix, infoState, 6000);
drawSubmenu(matrix, infoState, 7000);
assert.equal(infoState.infoAt, 6000);
console.log("PASS: SETT, MODE \u2192 SONG, ETC \u2192 ADV, moved controls, IMG lock, merged PREV, and legacy settings reload.");
await rm(testDir, { recursive: true, force: true });
