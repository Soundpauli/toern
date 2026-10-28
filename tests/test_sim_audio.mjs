// Exercise the real Web Audio graph with deterministic nodes, including future voices.
import assert from 'node:assert/strict';
import {createEngine} from '../standalone-tools/web-standalone/src/audio/engine.js';
import {emptyFilt} from '../standalone-tools/web-standalone/src/firmware/const.js';
const nodes=[];
class Param {
  value=1; calls=[];
  setTargetAtTime(v,...rest){this.value=v;this.calls.push(['target',v,...rest]);}
  setValueAtTime(v,...rest){this.value=v;this.calls.push(['value',v,...rest]);}
  linearRampToValueAtTime(v,...rest){this.value=v;this.calls.push(['linear',v,...rest]);}
  exponentialRampToValueAtTime(v,...rest){this.value=v;this.calls.push(['exponential',v,...rest]);}
  cancelScheduledValues(...args){this.calls.push(['cancel',...args]);}
}
function node(kind){
 const n={kind,gain:new Param(),frequency:new Param(),Q:new Param(),pan:new Param(),delayTime:new Param(),playbackRate:new Param(),connections:[],stops:[],
 connect(to){this.connections.push(to);},disconnect(){this.disconnected=true;},start(...args){this.started=args;},stop(t){this.stops.push(t);}};
 nodes.push(n);return n;
}
globalThis.AudioContext=class {
 sampleRate=8000;currentTime=1;state='running';destination={};
 createGain(){return node('gain');}createBiquadFilter(){return node('filter');}createWaveShaper(){return node('shaper');}
 createStereoPanner(){return node('pan');}createDelay(){return node('delay');}
 createOscillator(){return node('osc');}createBufferSource(){return node('sample');}
 createBuffer(ch,length,sr){const data=Array.from({length:ch},()=>new Float32Array(length));return{numberOfChannels:ch,length,sampleRate:sr,duration:length/sr,getChannelData:i=>data[i]};}
};
const engine=createEngine(),ctx=engine.ensure(),f=emptyFilt();
f.voiceGain=32;f.dlay=16;f.r=24;engine.setBus({bpm:120});engine.apply(1,f,16,1,.5);
assert(nodes.some(n=>n.gain.calls.some(c=>c[0]==='target'&&c[1]===2)),'V +16 doubles the separate output gain');
assert(nodes.some(n=>n.delayTime.calls.some(c=>c[1]===.25)),'DLAY 16 is half a beat at 120 BPM');
engine.trigger(1,127,1.04,{index:0,seek:0,end:100,env:{att:32,dec:16,sus:16,rel:16},stretch:22},2);
for(const ch of [11,11,11,13,14])engine.trigger(ch,127,1.04,{env:{att:32,dec:16,sus:16,rel:16}},2);
const effects=nodes.filter(n=>n.kind==='delay'||n.kind==='filter').map(n=>({n,delay:n.delayTime.value,freq:n.frequency.value}));
engine.trigger(13,127,1.04,{midiPitch:69,cent:16,semi:16,oct:0,detune:0,env:{att:32,dec:16,sus:16,rel:16}},2);
assert(nodes.filter(n=>n.kind==='osc').at(-1).frequency.calls.some(c=>c[1]===440),'MIDI A4 uses standard frequency');
engine.trigger(1,100,1.04,{midiPitch:60,index:0,seek:0,end:100,oct:0,detune:0},2);
assert.equal(nodes.filter(n=>n.kind==='sample').at(-1).playbackRate.value,1,'Sample original pitch is MIDI 60');
engine.releaseAll();
assert(nodes.filter(n=>n.kind==='osc').every(n=>n.stops.includes(1.04)),'Pause releases every poly and mono voice');
assert(nodes.filter(n=>n.kind==='sample').every(n=>n.stops.includes(1.04)),'Pause releases scheduled sample sources');
for(const {n,delay,freq} of effects){assert(!n.disconnected);assert.equal(n.delayTime.value,delay);assert.equal(n.frequency.value,freq);}
console.log('PASS: real simulator graph maps V gain/delay and releases all sources while preserving effect nodes/settings.');

// Sample bypass routes the complete signal, including effect sends, around both cuts.
for (let ch=1; ch<=8; ch++) {
  engine.trigger(ch,100,ctx.currentTime,{index:0,seek:0,end:100,held:true,env:{att:32,dec:0,sus:32,rel:16}},ch+1);
  const sample=nodes.filter(n=>n.kind==='sample').at(-1);
  assert.equal(sample.stops.length,0,'held sample waits for MIDI release');
  const amp=sample.connections[0].connections[0].connections[0].connections[0];
  const hp=amp.connections.find(n=>n.kind==='filter');
  const bypass=amp.connections.find(n=>n.kind==='gain');
  const filtered=hp.connections[0].connections[0];
  const output=bypass.connections[0];
  assert(filtered.connections.includes(output));
  assert(output.connections.some(n=>n.kind==='delay'),'delay receives the bypass sum');
  for (const [h,l,dry] of [[32,0,true],[31,0,false],[32,1,false],[32,0,true]]) {
    engine.apply(ch,{...emptyFilt(),h,l},16);
    assert.equal(bypass.gain.value,dry?1:0);
    assert.equal(filtered.gain.value,dry?0:1);
  }
  engine.releaseChannel(ch);
  assert.equal(sample.stops.at(-1),ctx.currentTime+.5+.02,'configured release is used');
}
console.log('PASS: all sample channels bypass cuts smoothly, feed effects from the sum, and honor held MIDI release.');
