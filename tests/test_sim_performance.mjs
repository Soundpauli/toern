import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {SUBS, MENU_PAGES} from '../standalone-tools/web-standalone/src/firmware/screens.js';
import {foldRow} from '../standalone-tools/web-standalone/src/firmware/pitch.js';
import {createFillScheduler, fillHitDue, syncDelays, syncHasEffect} from '../standalone-tools/web-standalone/src/firmware/performance.js';
import {filterPagesFor, emptyFilt} from '../standalone-tools/web-standalone/src/firmware/const.js';
import {INSTRUMENT_NAMES, instrumentDefaults} from '../standalone-tools/web-standalone/src/firmware/instruments.js';
for (const cols of [16,32]) {
  const f=createFillScheduler();
  f.request(1);f.request(2);f.request(1);
  assert.deepEqual(f.step(2,cols),[]);
  const start=f.step(1,cols);
  assert.deepEqual(start.map(h=>h.ch),[1,2]);
  assert(start.every(h=>h.velocity===100));
  let hits=[];
  for(let step=1;step<=2*cols;step++) {
    const batch=f.step((step%cols)+1,cols);
    if(step<2*cols)assert(!batch.some(h=>h.velocity===127));
    hits.push(...batch);
  }
  assert.deepEqual(hits.filter(h=>h.velocity===127).map(h=>h.ch),[1,2]);
  assert.deepEqual(f.step(2,cols),[]);
  const note={condition:21,velocity:42,midiPitch:72};
  assert(f.grid(14,cols,note,3));assert(!f.grid(14,cols,note,3));
  for(let i=1;i<2*cols;i++)f.step(2,cols);
  const drop=f.step(2,cols);
  assert.equal(note.condition,1);
  assert.deepEqual(drop,[{ch:14,row:3,velocity:127,midiPitch:72,sub:0}]);
  f.request(1);f.reset();assert.deepEqual(f.step(1,cols),[]);
  // Re-arming one voice leaves the other one's end boundary intact.
  f.request(1);f.request(2);f.step(1,cols);
  for(let i=1;i<cols;i++)f.step(i+1,cols);
  f.request(1);f.step(1,cols);
  for(let i=1;i<cols;i++)f.step(i+1,cols);
  assert.deepEqual(f.step(1,cols).filter(h=>h.velocity===127).map(h=>h.ch),[2]);
  // Five cadence sections and the last half-step hit mirror firmware fillHitDue.
  const length=2*cols;
  for(let sub=0;sub<length*4;sub++) {
    const position=Math.floor(sub/4),section=Math.min(4,Math.floor(position/Math.ceil(length/5)));
    const expected=sub!==0 && (section===4 ? sub%4===0||sub%4===2 : sub%4===0&&position%[8,4,2,1][section]===0);
    assert.equal(fillHitDue(sub,length),expected);
  }
}
for(const send of [-127,-20,0,20,127])for(const receive of [-127,-20,0,20,127]) {
  assert.deepEqual(syncDelays(send,receive),{send:Math.max(send,0)+Math.max(-receive,0),receive:Math.max(receive,0)+Math.max(-send,0)});
}
assert(!syncHasEffect({clockInt:false,analogExtClock:true,midiTransport:1,midiSend:0},0,-20));
assert(syncHasEffect({clockInt:true,analogExtClock:false,midiTransport:0,midiSend:0},1,-20));
assert(!syncHasEffect({clockInt:true,analogExtClock:false,midiTransport:0,midiSend:0},1,20));
for(let ch=1;ch<=8;ch++)assert.equal(filterPagesFor(ch)[3][1].key,'voiceGain');
for(const ch of [11,13,14])assert(!filterPagesFor(ch).flat().some(s=>s?.key==='voiceGain'));
assert.equal(emptyFilt().voiceGain,16);assert.equal(emptyFilt().stretch,11);
assert.equal(INSTRUMENT_NAMES.length,13);assert.equal(instrumentDefaults(12).inst,12);
assert.equal(instrumentDefaults(12).cent,5);
console.log('PASS: simulator fills, exact full-velocity drops, independent re-arm, stop cancellation, signed SYNC, V gain, TIME, and all 13 instrument controls.');

// Guard menu/instrument drift against the current firmware rather than another copy.
const menuSource=readFileSync(new URL('../toern_menu.ino',import.meta.url),'utf8');
for(const [table,actual] of [['menuPages',MENU_PAGES.map(p=>p.name)],['recsPages',SUBS.recs],['midiPages',SUBS.midi],['lookPages',SUBS.look]]) {
 const body=menuSource.match(new RegExp('MenuPage '+table+'\\[[^\\]]+\\] = \\{([\\s\\S]*?)\\};'))?.[1];
 assert(body,table);
 assert.deepEqual(actual,[...body.matchAll(/\{"([^"]+)"/g)].map(m=>m[1]),table+' matches firmware');
}
const source=readFileSync(new URL('../toern.ino',import.meta.url),'utf8');
const names=source.match(/instTypeNames\[\][^=]*= \{([^}]+)\}/)[1];
assert.deepEqual(INSTRUMENT_NAMES,[...names.matchAll(/"([^"]+)"/g)].map(m=>m[1]));
const defaults=readFileSync(new URL('../toern_parameters.ino',import.meta.url),'utf8');
for(const [key,name] of Object.entries({cut:'Control1',res:'Control2',flt:'Control3',att:'Attack',dec:'Decay',sus:'Sustain',rel:'Release',cent:'CentSlider'})) {
 const body=defaults.match(new RegExp('kCh11'+name+'Default\\[13\\] = \\{([\\s\\S]*?)\\};'))[1].replace(/\/\/[^\n]*/g,'');
 const values=[...body.matchAll(/\d+/g)].map(m=>Number(m[0]));
 assert.deepEqual(INSTRUMENT_NAMES.map((_,i)=>instrumentDefaults(i)[key]),values,key+' defaults match firmware');
}
assert.equal(foldRow(60,1).row,2);
assert.equal(foldRow(60,8).row,9);
assert.equal(foldRow(48,13).row,2);
assert.equal(foldRow(48,14).row,3);
console.log('PASS: simulator menu order and all instrument defaults match current firmware; MIDI fold roots match sample/synth voices.');
