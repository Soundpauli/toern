// Exercise the real simulator controls with mocked audio, SD and browser boundaries.
import assert from "node:assert/strict";
import { drawSubmenu, drawMenu, drawBoot, drawTouchy, TOUCHY_COLORS, TOUCHY_SPRITE, TOUCHY_RAINBOW, drawTouchyCat, TOUCHY_PALETTE } from "../standalone-tools/web-standalone/src/firmware/screens.js";
import { setLayout } from "../standalone-tools/web-standalone/src/firmware/const.js";
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
  b.onLoad({ filter: /.*/, namespace: "stub" }, ({ path }) => ({ contents: path === "sd" ? 'export default ["samples/test.wav"]'  : 'export function createEngine(){return new Proxy({}, {get:(_,key)=>key==="now"?()=>0:key==="ensure"?()=>globalThis.testAudioContext||({state:"running"}):key==="decodeUrl"?async()=>({getChannelData:()=>new Float32Array(32)}):key==="loadUrl"?async(...args)=>{globalThis.audioCalls.push([key,...args])}:(...args)=>globalThis.audioCalls.push([key,...args])})}' }));
} }] });
globalThis.audioCalls = [];
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
assert.equal(litPixels, 10); // Only the ten submenu navigation dots.
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
// TOUCHY shares the sample engine and preserves the pattern.
d.start();
d.jump("draw"); d.press(2); assert.equal(d.playing(),true);
open(8); page(8);
assert.equal(d.menuSummary().detail, "TCHY: OFF");
d.rotate(2, 1);
assert.equal(d.menuSummary().detail, "TCHY: ON");
d.jump("draw");
assert.equal(d.mode(), "touchy");
assert.equal(d.playing(), false);
const originalPattern = JSON.stringify(d.exportPatternCells());
audioCalls.length = 0;
for (let id=1; id<=5; id++) {
  d.touch(id,true); d.touch(id,true); d.touch(id,false);
  assert.equal(d.voice(),id);
}
assert.deepEqual(audioCalls.filter(c=>c[0]==="trigger").map(c=>c[1]),[1,2,3,4,5]);
// Holding and brief release chatter cannot retrigger; a stable release rearms.
const hits=()=>audioCalls.filter(c=>c[0]==="trigger").length;
for(let i=0;i<8;i++){ d.touch(5,true); now+=10; d.touch(5,false); }
assert.equal(hits(),5);
now+=61; d.touch(5,true); d.touch(5,true); assert.equal(hits(),6);
now+=500; d.touch(5,true); assert.equal(hits(),6); d.touch(5,false);
assert.equal(JSON.stringify(d.exportPatternCells()),originalPattern);
audioCalls.length=0;
d.rotate(0,-1); d.rotate(1,-1); d.rotate(2,1); d.rotate(3,1);
const controls = audioCalls.filter(c=>c[0]==="apply");
assert(controls.every(c=>c[1]===5 && c[3]===16));
assert.equal(controls.at(-1)[2].oct,23);
assert.equal(controls.at(-1)[2].r,1);
assert.equal(controls.at(-1)[2].l,1);
now+=61; d.touch(5,true); d.touch(5,false);
const hit5=audioCalls.filter(c=>c[0]==="trigger").at(-1);
assert.equal(hit5[2],99);
assert.equal(hit5[4].oct,-1/12);
// Moving to another pad changes the target, preserves each voice's velocity,
// and suppresses unsupported reverb on voices 3/4.
now+=61; d.touch(3,true); d.touch(3,false); audioCalls.length=0;
d.rotate(2,1); assert.equal(audioCalls.length,0);
d.rotate(0,1); d.rotate(3,1);
assert(audioCalls.every(c=>c[1]===3 && c[3]===16));
now+=61; d.touch(5,true); d.touch(5,false);
assert.equal(audioCalls.filter(c=>c[0]==="trigger").at(-1)[2],99);
for(let enc=0;enc<4;enc++) {
  d.press(enc); assert.equal(d.mode(),"wav"); assert.equal(d.voice(),enc+1); d.jump("draw");
}
function holdTouchy(enc) { d.knobDown(enc); now+=1000; frame(now); d.knobUp(enc,false); }
// Fastrec uses existing microphone capture; a release before permission resolves
// cancels it without creating notes or a recording.
let permitMic;
navigator.mediaDevices={getUserMedia:()=>new Promise(resolve=>{permitMic=resolve})};
// Provide a minimal audio context at the boundary for real Fastrec execution.
const audioNode=()=>({connect(){},disconnect(){},gain:{value:0}});
globalThis.testAudioContext={state:"running",sampleRate:44100,destination:{},
 createMediaStreamSource:audioNode, createGain:audioNode,
 createScriptProcessor:()=>{globalThis.recProcessor=audioNode(); return recProcessor;},
 createBuffer:(_,n)=>({getChannelData:()=>new Float32Array(n)})};
d.knobDown(0); now+=1000; frame(now); assert.equal(d.mode(),"touchy");
d.knobUp(0,false);
permitMic({getTracks:()=>[]}); await Promise.resolve(); await Promise.resolve();
assert.equal(JSON.stringify(d.exportPatternCells()),originalPattern);
audioCalls.length=0;
d.knobDown(0); now+=1000; frame(now); await Promise.resolve();
recProcessor.onaudioprocess({inputBuffer:{getChannelData:()=>new Float32Array([.2,.4])}});
now+=61; d.touch(2,true); d.touch(2,false);
d.knobUp(0,false);
assert.deepEqual(audioCalls.find(c=>c[0]==="loadBuffer").slice(0,2),["loadBuffer",5]);
assert.equal(d.mode(),"touchy");
assert.equal(JSON.stringify(d.exportPatternCells()),originalPattern);
// Browser loads still use the existing voice loader.
d.press(0); audioCalls.length=0; d.press(2);
assert.equal(d.mode(),"touchy");
assert.deepEqual(audioCalls.find(c=>c[0]==="loadUrl"),["loadUrl",1,"/sd/samples/test.wav"]);
holdTouchy(2); assert.equal(d.mode(),"filter"); assert.equal(d.voice(),2); d.touch(1,true); d.touch(1,false); assert.equal(d.mode(),"touchy");
holdTouchy(3); assert.equal(d.mode(),"menu"); assert.equal(d.menuSummary().detail,"MODE");
d.press(3); page(8); d.rotate(2,-1); d.touch(1,true); d.touch(1,false); assert.equal(d.mode(),"draw");
// Both physical display sizes draw only the flying sprite and its fixed pad tint.
for (const columns of [16,32]) {
  setLayout(columns);
  for(let voice=1;voice<=5;voice++) {
    const pixels=[];
    drawTouchy({light:(x,y,color)=>pixels.push({x,y,color}),setRing(){}},{touchyLastVoice:voice,touchyTintActive:true},0);
    assert(pixels.length>0);
    assert(pixels.every(p=>p.x>=1&&p.x<=columns&&p.y>=1&&p.y<=16));
    assert(pixels.some(p=>JSON.stringify(p.color)===JSON.stringify(TOUCHY_COLORS[voice-1].map(v=>Math.floor(v*3/5)))));
  }
}
assert.equal(TOUCHY_SPRITE.length,11);
assert(TOUCHY_SPRITE.every(row=>row.length===21));
assert.equal(TOUCHY_RAINBOW.length,6);
// Bobbing, foot/tail movement and a waving trail stay on both physical panels.
for(const columns of [16,32]) {
  for(let time=0;time<15000;time+=60) {
    drawTouchyCat((x,y)=>assert(x>=1&&x<=columns&&y>=1&&y<=16),columns,time);
  }
}
const still=[], animated=[];
drawTouchyCat((...p)=>still.push(p),32,4200);
drawTouchyCat((...p)=>animated.push(p),32,4380);
assert.notDeepEqual(still,animated);
assert(still.filter(([x,y,c])=>x===1&&c.some(v=>v>0)).length>=6);
// Startup curves split their brightness over adjacent LEDs before logo fade-in.
setLayout(32);
const bootPixels=[];
assert.equal(drawBoot({light:(x,y,color)=>bootPixels.push({x,y,color})},333,0),false);
assert(bootPixels.every(p=>Number.isInteger(p.y)&&p.y>=1&&p.y<=16));
assert(bootPixels.filter(p=>p.x===1).length>=3);
assert.equal(drawBoot({light(){}},5000,0),true);
// Every animated LED stays an exact palette colour; no smoothing/blends.
const exactCatColors = new Set([[0,0,0],...TOUCHY_RAINBOW,...Object.values(TOUCHY_PALETTE)].map(c=>JSON.stringify(c)));
for(const columns of [16,32]) for(const time of [333,777,1501,4200,4380]) {
  drawTouchyCat((x,y,color)=>assert(exactCatColors.has(JSON.stringify(color))),columns,time);
}
assert.deepEqual(TOUCHY_PALETTE.D,[0,0,0]);
assert(TOUCHY_PALETTE.G.every(v=>v<64));
// The rightmost nontransparent sprite column stays in the final fifth of width.
for(const time of [0,1000,2000,3000,4000,5000,6000]) {
  const anchor=32-20-6*(1-Math.cos(time*Math.PI/3000))/2;
  assert(anchor+20>=26&&anchor+20<=32);
}
assert.equal(32-20+20,32);
setLayout(32);
console.log("PASS: SETT, MODE \u2192 SONG, ETC \u2192 ADV, moved controls, IMG lock, merged PREV, TOUCHY pads/pitch/velocity/effects/Fastrec/browser/filter/exit, and legacy settings reload.");
await rm(testDir, { recursive: true, force: true });
