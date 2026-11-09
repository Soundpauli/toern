// Compare real simulator LED pixels with an MCP capture (see capture_device_pages.py).
import {readFile,writeFile,mkdtemp,rm} from 'node:fs/promises';
import assert from 'node:assert/strict';
import {gunzipSync} from 'node:zlib';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {build} from '../../tools/sim/node_modules/esbuild/lib/main.js';
import {createMatrix} from '../../tools/sim/src/firmware/matrix.js';
import {filterPagesFor,setLayout,emptyFilt} from '../../tools/sim/src/firmware/const.js';
import {renderFrame,MENU_PAGES,SUBS,resolveSubPage} from '../../tools/sim/src/firmware/screens.js';
const path=process.argv[2]||fileURLToPath(new URL('../fixtures/device-pages-v3.03b.json.gz',import.meta.url));
const bytes=await readFile(path);
const capture=JSON.parse(path.endsWith('.gz')?gunzipSync(bytes).toString():bytes.toString());
const dir=await mkdtemp(join(tmpdir(),'toern-parity-'));
const source=fileURLToPath(new URL('../../tools/sim/src/firmware/device.js',import.meta.url));
await build({entryPoints:[source],outfile:join(dir,'device.mjs'),bundle:true,format:'esm',platform:'node',plugins:[{name:'boundaries',setup(b){
 b.onLoad({filter:/firmware\/device\.js$/},async()=>({contents:(await readFile(source,'utf8')).replace('start() { requestAnimationFrame(tick); },','inspect: () => s, picks: () => subPick, filterPositions: () => filterPages, start() { requestAnimationFrame(tick); },'),loader:'js'}));
 b.onResolve({filter:/virtual:sd/},()=>({path:'sd',namespace:'stub'}));
 b.onResolve({filter:/audio\/engine\.js/},()=>({path:'audio',namespace:'stub'}));
 b.onLoad({filter:/.*/,namespace:'stub'},({path})=>({contents:path==='sd'?'export default []':'export function createEngine(){return new Proxy({},{get:(_,k)=>k==="now"?()=>0:()=>{}})}'}));
}}]});
let now=10000,frame;
globalThis.performance={now:()=>now};globalThis.requestAnimationFrame=f=>frame=f;
globalThis.localStorage={getItem:()=>null,setItem(){}};globalThis.navigator={};globalThis.window={addEventListener(){}};globalThis.document={addEventListener(){}};
globalThis.setTimeout=()=>0;globalThis.setInterval=()=>0;
const {createDevice}=await import(pathToFileURL(join(dir,'device.mjs')));
const ctx=new Proxy({},{get:()=>()=>({addColorStop(){}})}), matrix=createMatrix({getContext:()=>ctx});
const d=createDevice(matrix,{},[]),s=d.inspect(),picks=d.picks();d.start();frame(1);frame(6000);
const groups={ROOT:'menu',VOL:'vol',SETT:'look',REC:'recs',MIDI:'midi',MODE:'mode',ETC:'etc',ADV:'adv'};
const results=[];
function compare(name,expected){
 renderFrame(matrix,s,now);
 const diff=[],actual=[];const map=new Map(expected.pixels.map(p=>[`${p.x},${p.y}`,p.color]));
 for(let y=0;y<16;y++)for(let x=0;x<expected.width;x++){
  const rgb=matrix.pixel(x+1,16-y).map(c=>(c*65)>>8),color='#'+rgb.map(c=>c.toString(16).padStart(2,'0')).join('');
  if(rgb.some(Boolean))actual.push({x,y,color});
  if(color!==(map.get(`${x},${y}`)||'#000000'))diff.push({x,y,expected:map.get(`${x},${y}`)||'#000000',actual:color});
 }
 results.push({name,different:diff.length,expected:expected.pixels.length,actual:actual.length,diff,simulator:{width:expected.width,height:16,pixels:actual}});
}
for(const e of capture.menus){
 setLayout(e.matrix.width);matrix.layout(e.matrix.width);s.GLOB.currentChannel=e.state.voice;s.GLOB.singleMode=false;
 s.mode=groups[e.group];s.menuIndex=e.item.index;s.subIndex=e.item.index;
 const p=e.state.positions, v=p[2],slot=resolveSubPage(s.mode,s.subIndex);
 if(picks[slot.mode] && e.group!=='ROOT' && Number.isInteger(v))picks[slot.mode][slot.index]=Math.max(0,v);
 if(e.group==='VOL'){
  if(e.item.name==='MAIN')s.mainVol=v;
  if(e.item.name==='LOUT')s.lineOutLevel=v;
  if(e.item.name==='PREV'){s.prevVol=v;s.prevMode=p[1];}
 }
 if(e.group==='SETT'&&e.item.name==='VIEW')picks.look[2]=1-v;
 // The captured MIC setting and live RAM reading are fixture inputs, not simulator defaults.
 if(e.group==='REC'&&e.item.name==='INPT')picks.recs[0]=1;
 if(e.group==='ADV'&&e.item.name==='RAM')s.ramLabel='A25';
 if(e.group==='REC'){if(e.item.name==='MIC')s.micGain=v;if(e.item.name==='L-IN')s.lineInLevel=v;}
 if(e.group==='MIDI'){
  if(e.item.name==='SYNC'){s.syncEdit=p[1];s.syncRcveMs=v;s.midiSend=2;s.midiTransport=2;}
  if(e.item.name==='PQ-O'){s.ppqnWidthMs=p[0];s.ppqnStopWithPlay=!p[1];s.ppqnRateSel=v;}
  if(e.item.name==='TRIG')s.sideVoice=[0,1,2,3,4,5,6,7,8,11,13,14][v];
 }
 if(e.group==='MODE'&&e.item.name==='AUTO'){s.autoPages=p[0];s.autoStart=p[1];s.autoEnd=v;}
 compare(`${e.group}/${e.item.name}`,e.matrix);
}
for(const e of capture.filters){
 s.mode='filter';s.ledMode=3;s.GLOB.currentChannel=e.voice;s.filterTouchAt=0;
 assert.equal(filterPagesFor(e.voice).length,e.metadata.pages.length);
 const inst=e.metadata.pages.flatMap(p=>p.sliders).find(s=>s.name==='INST')?.value??12;
 const defaults=emptyFilt(e.voice,inst);
 for(const p of e.metadata.pages) for(const spec of p.sliders){
  const sim=filterPagesFor(e.voice)[p.page-1]?.[spec.encoder-1];assert(sim);
  assert.equal(sim.name,spec.name);assert.equal(sim.max??32,spec.max);
  assert.equal(sim.key==='inst'?12:defaults[sim.key],spec.default,`default ${e.voice}/${spec.name}`);
  s.filt[e.voice][sim.key]=spec.value;
 }
 for(const p of e.pages){s.filterPage=p.page-1;compare(`FILTER/${e.voice}/${p.page}`,p.matrix);}
}
const extra=capture.extra||{pages:[]};
for(const e of [...capture.browsers,...extra.pages.filter(e=>/PSET|SONG|REC/.test(e.name))]){
 const p=e.state.positions;s.GLOB.currentChannel=e.state.voice;s.GLOB.singleMode=false;s.filterTouchAt=0;
 s.mode={PATT:'dat',KIT:'kit',SMP:'wav',BPM:'bpm',SONG:'song',REC:'rec'}[e.name]||'pset';
 if(s.mode==='dat'){s.slot=p[3]-1;s.loadedPatternFile=s.slot;s.slotExists=()=>true;s.loadSoundSettings=p[2]===0;}
 if(s.mode==='kit'){s.packSlot=p[3];s.loadedPackSlot=s.packSlot;s.slotExists=()=>true;}
 if(s.mode==='bpm'){s.bpm=e.state.bpm;s.ledBrightness=64;s.clockInt=!!p[2];}
 if(s.mode==='wav'){s.wavFile=false;s.wavName='BASE';s.browseDir='samples';}
 if(s.mode==='pset'){s.presetSlot=p[3];s.presetExists=()=>e.state.voice===11;}
 if(s.mode==='rec')s.recPeak=1;
 if(s.mode==='song'){s.songPos=p[3];s.songPattern=p[1];}
 compare(`BROWSER/${e.name}`,e.matrix);
}
const held=capture.held||{pages:[]};
for(const e of held.pages){
 if(e.name!=='VELOCITY')continue;
 s.mode='velocity';s.GLOB.currentChannel=e.state.voice;
 const p=e.state.positions;s.vel={v:p[0],p:p[1],c:p[2],vol:p[3]};
 compare('VELOCITY',e.matrix);
}
const focus=capture.focus||{filters:[]};
for(const e of focus.filters){
 s.mode='filter';s.ledMode=3;s.GLOB.currentChannel=e.voice;
 for(const c of e.changes){
  for(const p of c.metadata.pages)for(const spec of p.sliders){const sim=filterPagesFor(e.voice)[p.page-1]?.[spec.encoder-1];if(sim)s.filt[e.voice][sim.key]=spec.value;}
  s.filterPage=c.page-1;d.filterPositions()[e.voice]=c.page-1;
  const spec=filterPagesFor(e.voice)[c.page-1][c.encoder-1];
  s.filt[e.voice][spec.key]=c.before;
  d.rotate(c.encoder-1,c.delta);
  const native=c.metadata.pages[c.page-1].sliders.find(s=>s.encoder===c.encoder).value;
  assert.equal(s.filt[e.voice][spec.key],native,`control ${e.voice}/${c.page}/${spec.name}`);
  compare(`FOCUS/${e.voice}/${c.page}/${c.encoder}`,c.matrix);
 }
}
await writeFile('/tmp/toern-parity-diff.json' ,JSON.stringify(results));
if(process.env.VERBOSE)for(const r of results)console.log(r.name,r.different+'/'+r.expected);
const mismatches=results.filter(r=>r.different);
assert.deepEqual(mismatches.map(r=>[r.name,r.different]),[], 'device pixel mismatches (details: /tmp/toern-parity-diff.json)');
const adjustments=focus.filters.reduce((n,v)=>n+v.changes.length,0);
console.log(`PASS: ${results.length} MCP device captures match real simulator pixels; ${adjustments} real encoder adjustments and all voice filter schemas/defaults match.`);
await rm(dir,{recursive:true,force:true});
