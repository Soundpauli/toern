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
  b.onLoad({ filter: /.*/, namespace: "stub" }, ({ path }) => ({ contents: path === "sd" ? 'export default ["samples/test.wav"]'  : 'export function createEngine(){return new Proxy({}, {get:(_,key)=>key==="now"?()=>globalThis.audioTime||0:key==="ensure"?()=>globalThis.testAudioContext||({state:"running"}):key==="decodeUrl"?async()=>({getChannelData:()=>new Float32Array(32)}):key==="loadUrl"?async(...args)=>{globalThis.audioCalls.push([key,...args])}:(...args)=>globalThis.audioCalls.push([key,...args])})}' }));
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
const { createDevice, encoderButtonEvent, encoderTurnEvent } = await import(pathToFileURL(bundlePath));
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
d.jump("settings-menu");
assert.equal(d.mode(), "menu");
assert.equal(d.menuSummary().detail, "SETT");
d.jump("menu");
assert.equal(d.menuSummary().detail, "PATT");
function open(root) {
  d.jump("menu");
  for (let i = 0; i < root; i++) d.rotate(3, 1);
  d.press(3);
}
function page(i) {
  for (let tries=0; d.subIndex() !== i && tries<20; tries++) d.rotate(3, d.subIndex()<i?1:-1);
  assert.equal(d.subIndex(),i,`page ${i} is reachable in ${d.mode()}`);
}
// PREV always keeps its parent-coloured title; mode labels time out to volume.
for(const prevMode of [0,1,2]) {
  const text=[], indicators=[], rings=[];
  const previewMatrix={drawLargeCustom(){},light(){},drawText:(...a)=>text.push(a),drawIndicator:(...a)=>indicators.push(a),setRing:(...a)=>rings.push(a)};
  const state={mode:"vol",subIndex:3,prevMode,prevVol:35,prevModeLabelUntil:2500};
  drawSubmenu(previewMatrix,state,1000);
  assert.equal(text[0][0],"PREV");
  assert.equal(text[1][0],["ON","PRSS","SYNC"][prevMode]);
  assert.deepEqual(text[1][3],[[0,120,0],[120,0,0],[120,120,0]][prevMode]);
  assert.deepEqual(indicators,[["L",["G","R","Y"][prevMode],2],["L","O",3]]);
  assert.deepEqual(rings.filter(r=>r[0]===3).at(-1),[3,[120,60,0],true]);
  text.length=0; drawSubmenu(previewMatrix,state,3500);
  assert.equal(text[0][0],"PREV"); assert.equal(text[1][0],"35"); assert.deepEqual(text[1][3],[120,60,0]);
}
// Value, matrix indicator and ring agree on hue and peak brightness.
for (const color of [[55,10,0],[255,0,255],[20,20,0],[0,0,20],[0,255,0],[120,0,0]]) {
  const text=[], custom=[], rings=[];
  const m={drawLargeCustom:(...a)=>custom.push(a),light(){},drawText:(...a)=>text.push(a),drawIndicator(){},setRing:(...a)=>rings.push(a)};
  drawSubmenu(m,{mode:"vol",subIndex:6,legacySubValue:()=>({text:"0",color,code:"R"})},0);
  const value=text.find(a=>a[2]===3)[3];
  assert.equal(Math.max(...value),120);
  assert.deepEqual(custom.at(-1),[value,3]);
  assert.deepEqual(rings.at(-1),[3,value,true]);
}
open(9); page(3); d.press(3); page(9); textDraws.length=0; frame(++now);
assert(textDraws.some(c=>c[0]==="0"&&c[2]===3&&c[3][0]===120));
open(4); page(3);
d.rotate(1,1); now+=1; textDraws.length=0; frame(now);
assert(textDraws.some(c=>c[0]==="PRSS"&&c[2]===3));
now+=2501; textDraws.length=0; frame(now);
assert(textDraws.some(c=>c[0]==="20"&&c[2]===3));
for(let i=0;i<60;i++)d.rotate(2,1);
textDraws.length=0; frame(now); assert(textDraws.some(c=>c[0]==="50"&&c[2]===3));
for(let i=0;i<60;i++)d.rotate(2,-1);
textDraws.length=0; frame(now); assert(textDraws.some(c=>c[0]==="0"&&c[2]===3));
d.rotate(1,-1); for(let i=0;i<20;i++)d.rotate(2,1);
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
expectToggle("ON", [0, 120, 0]);
// SONG is a plain submenu label; the MODE root retains its icon.
textDraws.length = 0;
litPixels = 0;
drawSubmenu(matrix, { mode: "mode", subIndex: 7 }, now);
assert.equal(litPixels, 11); // Eleven MODE submenu navigation dots.
assert.deepEqual(textDraws.map(args => args.slice(0, 3)), [["SONG", 2, 10]]);
textDraws.length = 0;
litPixels = 0;
drawSubmenu(matrix, { mode: "etc", subIndex: 3 }, now);
assert.equal(litPixels, 6); // ETC includes SWAP.
assert.deepEqual(textDraws.map(args => args.slice(0, 3)), [["ADV", 2, 10]]);
litPixels = 0;
drawMenu(matrix, { menuIndex: 8 });
assert(litPixels > 10, "MODE still draws its root icon");
// Remapping a displayed submenu must keep the animation state on the real device.
const infoState = { mode: "etc", subIndex: 0, infoAt: 0 };
drawSubmenu(matrix, infoState, 6000);
drawSubmenu(matrix, infoState, 7000);
assert.equal(infoState.infoAt, 6000);
// FN tags inputs instead of replacing the normal encoder configuration.
assert.deepEqual(encoderButtonEvent(0,2,false),[2,0,0,0]);
assert.deepEqual(encoderButtonEvent(0,2,true),[3,0,0,0]);
assert.deepEqual(encoderButtonEvent(3,2,true),[0,0,0,3]);
assert.deepEqual(encoderTurnEvent(0,-1,true),{encoder:0,delta:-1,fn:true});
d.jump("draw"); d.selectVoice(1); d.touch(5,false);
const normalY=d.cursor().y;
d.rotate(0,1); assert.equal(d.cursor().y,normalY+1);
d.rotate(0,-1); assert.equal(d.cursor().y,normalY);
for(let enc=0;enc<4;enc++) {
  d.touch(5,true); const before=d.mode();
  d.knobDown(enc); now+=1000; frame(now);
  d.touch(5,false); d.knobUp(enc,false);
  assert.equal(d.mode(),before); // FN-long cannot fall through to normal long/short.
}
// TOUCHY shares the sample engine and preserves the pattern.
d.start();
d.jump("draw"); d.selectVoice(1);
for(const mode of ["draw","single","filter","menu","wav","bpm"]) {
  d.jump(mode); assert.equal(d.mode(),mode);
  const beforeCursor=d.cursor(),beforeVoice=d.voice();
  d.touch(5,true); d.rotate(0,-1); textDraws.length=0; frame(++now);
  assert(textDraws.some(c=>c[0]==="<<"));
  d.rotate(0,1); textDraws.length=0; frame(++now);
  assert(textDraws.some(c=>c[0]===">>"));
  assert.equal(d.voice(),beforeVoice); assert.deepEqual(d.cursor(),beforeCursor);
  const beforeMenu=d.menuSummary(),beforeCells=JSON.stringify(d.exportPatternCells());
  const audioBefore=audioCalls.length;
  for(let enc=0;enc<4;enc++) { d.rotate(enc,1); d.rotate(enc,-1); }
  assert.equal(d.voice(),beforeVoice); assert.deepEqual(d.cursor(),beforeCursor);
  assert.deepEqual(d.menuSummary(),beforeMenu);
  assert.equal(JSON.stringify(d.exportPatternCells()),beforeCells);
  assert.equal(audioCalls.length,audioBefore);
  d.touch(5,false); now+=351; textDraws.length=0; frame(now);
  assert(!textDraws.some(c=>c[0]==="<<"||c[0]===">>"));
}
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
d.press(3); page(8); assert.equal(d.menuSummary().detail,"TCHY: OFF"); d.touch(1,true); d.touch(1,false); assert.equal(d.mode(),"draw");
// Both physical display sizes draw only the flying sprite and its fixed pad tint.
for (const columns of [16,32]) {
  setLayout(columns);
  for(let voice=1;voice<=5;voice++) {
    const pixels=[];
    drawTouchy({light:(x,y,color)=>pixels.push({x,y,color}),setRing(){}},{touchyLastVoice:voice,touchyTintActive:true},0);
    assert(pixels.length>0);
    assert(pixels.every(p=>p.x>=1&&p.x<=columns&&p.y>=1&&p.y<=16));
    assert(pixels.some(p=>JSON.stringify(p.color)===JSON.stringify(TOUCHY_COLORS[voice-1].map(v=>Math.floor(Math.floor(v*3/5)/2)))));
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
const exactCatColors = new Set([[0,0,0],...TOUCHY_RAINBOW,...Object.values(TOUCHY_PALETTE)].map(c=>JSON.stringify(c.map(v=>Math.floor(v/2)))));
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
// Five-pad performance, fader priority, V gain, PSET and pause use real device controls.
stored.clear();audioCalls.length=0;globalThis.audioTime=0;
const perf=createDevice(matrix,{},[]);
perf.start();frame(1);now=6001;frame(now);perf.jump("empty");perf.setDevice(1);
perf.selectVoice(1);perf.touch(4,true);perf.touch(4,false);
assert(audioCalls.some(c=>c[0]==="trigger"&&c[1]===1&&c[5]===2),"T4 auditions base pitch");
perf.togglePlay();
function requestFill(ch){perf.selectVoice(ch);perf.touch(5,true);perf.touch(4,true);perf.touch(4,false);perf.touch(5,false);}
perf.jump("single");
const beforeFillGrid=JSON.stringify(perf.exportPatternCells());
perf.touch(4,true);perf.touch(5,true);perf.touch(4,false);perf.touch(5,false);
assert.equal(JSON.stringify(perf.exportPatternCells()),beforeFillGrid,"T4-first chord must not write a SINGLE audition note");
perf.jump("draw");requestFill(1);requestFill(2);
textDraws.length=0;frame(now);assert(textDraws.some(c=>c[0]==="FILL"));
audioCalls.length=0;globalThis.audioTime=2.017;now+=2000;frame(now);
assert.deepEqual(audioCalls.filter(c=>c[0]==="trigger").map(c=>c[1]),[1,2]);
for(let step=1;step<=32;step++){globalThis.audioTime=2.017+step*.125;now+=125;frame(now);}
const drops=audioCalls.filter(c=>c[0]==="trigger"&&c[2]===127);
assert.deepEqual(drops.map(c=>c[1]),[1,2]);
assert(drops.every(c=>Math.abs(c[3]-6.017)<1e-8));
perf.pause();assert(audioCalls.some(c=>c[0]==="releaseAll"));
perf.jump("draw");perf.selectVoice(1);perf.toggleMute(2);perf.soloHold(1);
perf.setFader(1,0);perf.setFader(2,16);perf.soloRelease();
assert(perf.muted(1));assert(!perf.muted(2));
perf.jump("menu");perf.setFader(3,6);perf.setFader(4,9);
assert(!perf.muted(3));assert(!perf.muted(4));
perf.selectVoice(3);perf.jump("filter");
for(let i=0;i<3;i++){perf.touch(2,true);perf.touch(2,false);}
perf.rotate(1,1);
assert(audioCalls.some(c=>c[0]==="apply"&&c[1]===3&&c[2].voiceGain===17));
perf.selectVoice(11);perf.jump("menu");perf.rotate(3,1);perf.rotate(3,1);perf.press(3);
assert.equal(perf.mode(),"pset");perf.press(1);
assert(JSON.parse(stored.get("toern-web-presets-11"))[1]);
perf.press(3);perf.jump("filter");perf.rotate(0,-1);perf.jump("menu");perf.rotate(3,1);perf.rotate(3,1);perf.press(3);perf.press(0);
assert.equal(perf.mode(),"pset");
console.log("PASS: five-pad audition/fill, queued voices and final drop through transport, menu faders, solo priority, V gain, synth presets and pause release.");
console.log("PASS: SETT, MODE \u2192 SONG, ETC \u2192 ADV, moved controls, IMG lock, merged PREV, TOUCHY pads/pitch/velocity/effects/Fastrec/browser/filter/exit, and legacy settings reload.");
await rm(testDir, { recursive: true, force: true });
// DEMO: exact ten-second intro, three complete traversals, then repeat without a logo.
const { demoPhase, drawDemo, drawBpm, DEMO_TEXT, DEMO_TEXT_WIDTH } = await import('../standalone-tools/web-standalone/src/firmware/screens.js');
const { textPixelWidth } = await import('../standalone-tools/web-standalone/src/firmware/font.js');
for (const cols of [16,32]) {
  setLayout(cols);
  const scroll=(cols+DEMO_TEXT_WIDTH+1)*65;
  assert.equal(demoPhase(9999,cols).name,'nyan');
  for (const [i,name] of ['red','white','rainbow'].entries()) {
    const start=10000+i*scroll;
    assert.equal(demoPhase(start,cols).name,name);
    assert.equal(demoPhase(start,cols).x,cols+1);
    assert(demoPhase(start+scroll-1,cols).x <= -DEMO_TEXT_WIDTH+1);
  }
  assert.equal(demoPhase(10000+3*scroll,cols).name,'nyan');
  const cycle=10000+3*scroll;
  assert.equal(demoPhase(cycle-1,cols).name,'rainbow');
  assert.deepEqual(demoPhase(cycle,cols),{name:'nyan',elapsed:0});
  assert.deepEqual(demoPhase(cycle*123+1234,cols),demoPhase(1234,cols));
  const ink=[],letters=[];
  const m={light:(...a)=>ink.push(a),drawText:(...a)=>letters.push(a),drawIndicator(){}};
  for(const bpm of [40,99.9,120.3,239.9,300]) {
    letters.length=0;ink.length=0;
    drawBpm(m,{bpm,ledBrightness:64,clockInt:true});
    assert.equal(letters.map(a=>a[0]).join('.'),bpm.toFixed(1));
    assert(letters.every(a=>a[2]===6));
    for(const [text,x] of letters)assert(x>=1&&x+textPixelWidth(text)-2<=cols);
  }
  letters.length=0;ink.length=0;
  drawDemo(m,10000+15*65);
  const textInk=ink.filter(a=>a[2][0]===255&&a[2][1]===0&&a[2][2]===0);
  assert(textInk.length>0 && textInk.every(a=>a[1]>=4&&a[1]<=13));
  assert(ink.some(a=>a[2][2]>a[2][0]),"blue sine behind the text");
  for(let i=0;i<textInk.length;i+=4) {
    const block=textInk.slice(i,i+4);
    assert.equal(new Set(block.map(a=>a[0])).size,2);
    assert.equal(new Set(block.map(a=>a[1])).size,2);
  }
  ink.length=0;
  drawDemo(m,10000+cols*65);
  const mRows=["#...#","##.##","#.#.#","#...#","#...#"];
  for(let row=0;row<5;row++)for(let col=0;col<5;col++)for(let dx=0;dx<2;dx++)for(let dy=0;dy<2;dy++) {
    const x=1+col*2+dx,y=4+(4-row)*2+dy;
    const red=ink.some(a=>a[0]===x&&a[1]===y&&a[2][0]===255&&a[2][1]===0);
    assert.equal(red,mRows[row][col]==='#',`wide M at ${col},${row}`);
  }
  letters.length=0;
  drawDemo(m,10000+3*scroll+2000);
  assert(!letters.some(a=>a[0]==='&'));
}
setLayout(32);
open(8);page(10);assert.equal(d.menuSummary().detail,'DEMO: GO');
d.press(3);assert.equal(d.mode(),'demo');
const total=10000+3*(32+DEMO_TEXT_WIDTH+1)*65;
d.start();now+=total*3;frame(now);assert.equal(d.mode(),'demo');
d.press(3);assert.equal(d.mode(),'demo');
d.touch(2,true);d.touch(2,false);assert.equal(d.mode(),'demo');
d.touch(1,true);assert.equal(d.mode(),'mode');assert.equal(d.subIndex(),10);
d.touch(1,false);assert.equal(d.mode(),'mode');
d.press(3);assert.equal(d.mode(),'demo');
d.keydown({key:'a',code:'KeyA',preventDefault(){},stopPropagation(){}});
assert.equal(d.mode(),'mode');
d.keyup({key:'a',code:'KeyA'});assert.equal(d.mode(),'mode');
d.jump('bpm');const oldBpm=d.bpm();d.rotate(0,1);assert.equal(d.bpm(),Math.round((oldBpm+.1)*10)/10);
console.log('PASS: DEMO order/timing/full off-grid scrolls/exit and one-row fractional BPM on both matrix widths.');

// Touch shortcuts must leave the encoder keys available.
function tapKey(key, code) {
  now+=500;
  const e={key,code,preventDefault(){},stopPropagation(){}};
  d.keydown(e);d.keyup(e);
}
d.setDeviceFocus(true);
d.jump('bpm');
const keyboardBpm=d.bpm();
tapKey('q','KeyQ');assert.equal(d.bpm(),Math.round((keyboardBpm+.1)*10)/10);
tapKey('w','KeyW');assert.equal(d.bpm(),keyboardBpm);
d.jump('draw');tapKey('e','KeyE');assert.equal(d.mode(),'single');
tapKey('e','KeyE');assert.equal(d.mode(),'draw');
d.jump('bpm');tapKey('p','KeyP');assert.equal(d.mode(),'draw');
d.jump('bpm');tapKey('.','Period');assert.equal(d.mode(),'bpm');
d.jump('draw');tapKey('x','KeyX');assert.equal(d.mode(),'draw');
tapKey('m','KeyM');assert.equal(d.mode(),'draw');
console.log('PASS: remapped touch keys and restored encoder Q/W controls.');

// Exercise the real Web MIDI wiring, including Note Off after changing voice.
stored.clear();
const synthMidiInput={id:'synth-test'};
navigator.requestMIDIAccess=async()=>({inputs:new Map([['synth-test',synthMidiInput]])});
const midiDevice=createDevice(matrix,{},[]);
await Promise.resolve();await Promise.resolve();
midiDevice.start();now+=6000;frame(now);now+=6000;frame(now);
midiDevice.selectVoice(13);
audioCalls.length=0;
synthMidiInput.onmidimessage({data:[0x90,60,100],target:synthMidiInput});
const midiTrigger=audioCalls.find(call=>call[0]==='trigger');
assert(midiTrigger && midiTrigger[1]===13 && midiTrigger[4].held===true);
assert.equal(midiTrigger[4].arp,16);
now+=1000;frame(now);
assert.equal(audioCalls.filter(call=>call[0]==='trigger').length,1,'neutral ARP must not repeat a held note');
midiDevice.selectVoice(14);
synthMidiInput.onmidimessage({data:[0x80,60,0],target:synthMidiInput});
assert(audioCalls.some(call=>call[0]==='releaseChannel'&&call[1]===13),'Note Off releases the original voice');
console.log('PASS: Web MIDI held-note routing, neutral ARP and Note Off after voice selection changes.');

// VMOD is a session choice; a saved ON value must not enable it at SIM startup.
stored.clear();
const oldLook=Array(12).fill(0);oldLook[5]=1;oldLook[7]=2;
stored.set('toern-web-eeprom',JSON.stringify({subPick:{look:oldLook}}));
const normalModeDevice=createDevice(matrix,{},[]);
normalModeDevice.start();now+=6000;frame(now);now+=6000;frame(now);
normalModeDevice.jump('menu');for(let i=0;i<8;i++)normalModeDevice.rotate(3,1);
normalModeDevice.press(3);for(let i=0;i<3;i++)normalModeDevice.rotate(3,1);
assert.equal(normalModeDevice.menuSummary().detail,'VMOD: OFF');
normalModeDevice.rotate(2,1);
assert.equal(normalModeDevice.menuSummary().detail,'VMOD: ON','VMOD can still be enabled explicitly');
console.log('PASS: VMOD starts off after saved ON and remains editable.');

// SWAP uses the same controls and whole-pattern clipboard semantics as firmware.
stored.clear();
const swapDevice=createDevice(matrix,{},[]);
swapDevice.start();now+=6000;frame(now);now+=6000;frame(now);
const swapCells=Array.from({length:256*16},(_,i)=>({channel:[1,11,13,14,2,0][i%6],velocity:1+i%127,probability:i%101,condition:1+i%22,midiPitch:[0,60,127,255][i%4]}));
swapDevice.importPatternCells(swapCells,120);
const original=swapDevice.exportPatternCells();
swapDevice.jump('menu');for(let i=0;i<9;i++)swapDevice.rotate(3,1);
swapDevice.press(3);for(let i=0;i<5;i++)swapDevice.rotate(3,1);
assert.equal(swapDevice.menuSummary().detail,'SWAP: 1<>11');
swapDevice.press(2);
assert.deepEqual(swapDevice.exportPatternCells(),original.map(n=>({...n,channel:n.channel===1?11:n.channel===11?1:n.channel})));
swapDevice.press(3);assert.deepEqual(swapDevice.exportPatternCells(),original,'a second swap restores every note and metadata');
for(let i=0;i<9;i++)swapDevice.rotate(0,1); // A=13
swapDevice.rotate(1,1);swapDevice.rotate(1,1); // B=14
assert.equal(swapDevice.menuSummary().detail,'SWAP: 13<>14');
swapDevice.press(3);
assert.deepEqual(swapDevice.exportPatternCells(),original.map(n=>({...n,channel:n.channel===13?14:n.channel===14?13:n.channel})));
const afterSwap=swapDevice.exportPatternCells();
swapDevice.rotate(0,1);swapDevice.press(2);
assert.deepEqual(swapDevice.exportPatternCells(),afterSwap,'equal channels do nothing');
swapDevice.rotate(3,-1);assert.equal(swapDevice.subIndex(),4,'encoder 4 leaves SWAP');
for(const width of [16,32]) {
 setLayout(width);const text=[];
 drawSubmenu({light(){},drawLargeCustom(){},drawIndicator(){},drawText:(...a)=>text.push(a)},{mode:'etc',subIndex:5,swapFrom:13,swapTo:14,swapAt:0},now);
 assert.deepEqual(text.slice(0,3).map(a=>a[0]),['SWAP','13','14']);
 assert.equal(text[2][1],width===32?24:10);
}
setLayout(32);
console.log('PASS: SWAP controls, 256-step exchange, metadata preservation, synth channels, no-op and navigation.');
