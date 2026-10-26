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
const amp=nodes.find(n=>n.kind==='shaper').connections[0];
const cutDry=amp.connections.find(n=>n.kind==='gain');
const lp=amp.connections.find(n=>n.kind==='filter'&&n.type==='lowpass');
const hp=amp.connections.find(n=>n.kind==='filter'&&n.type==='highpass');
const cutLP=lp.connections[0],cutHP=hp.connections[0];
engine.apply(1,f,16);
assert.equal(cutDry.gain.value,1);assert.equal(cutLP.gain.value,0);assert.equal(cutHP.gain.value,0);
assert.equal(cutDry.connections[0],cutLP.connections[0],'dry and filtered paths meet only after the cutoff');
engine.apply(1,{...f,h:31},16);
assert.equal(cutDry.gain.value,0);assert.equal(cutLP.gain.value,1);
engine.apply(1,{...f,l:8},16);
assert.equal(cutDry.gain.value,0);assert.equal(cutLP.gain.value,0);assert.equal(cutHP.gain.value,1);

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
assert(nodes.filter(n=>n.kind==='sample').every(n=>n.stops.length>0),'Pause releases scheduled sample sources using their configured release');
for(const {n,delay,freq} of effects){assert(!n.disconnected);assert.equal(n.delayTime.value,delay);assert.equal(n.frequency.value,freq);}
console.log('PASS: real simulator graph maps V gain/delay and releases all sources while preserving effect nodes/settings.');

// Sample ADSR is transparent at neutral settings, with only the old reader
// fading on retrigger and configured release used for Note Off / pause.
const neutral=emptyFilt();
for(const ch of [2,3,4,5])engine.loadBuffer(ch,ctx.createBuffer(1,ctx.sampleRate*4,ctx.sampleRate));
engine.trigger(2,127,1,{midiPitch:60,env:neutral,inputPitch:60},3);
const first=nodes.filter(n=>n.kind==='sample').at(-1),firstGain=first.connections[0];
assert.deepEqual(firstGain.gain.calls[0],['value',1,1]);
assert(!firstGain.gain.calls.some(c=>c[0]==='linear'),'neutral envelope has no attack/decay ramp');
assert.equal(first.stops.length,0,'neutral one-shot is not gated before natural sample end');
engine.trigger(2,127,1,{midiPitch:72,env:{...neutral,rel:16},inputPitch:72},3);
const second=nodes.filter(n=>n.kind==='sample').at(-1),secondGain=second.connections[0];
assert.deepEqual(secondGain.gain.calls[0],['value',1,1],'new attack stays at unity');
assert(first.stops.some(t=>Math.abs(t-(1+64/44100+1/8000))<1e-8),'only old sample fades over the retrigger tail');
assert.equal(second.started[2],first.started[2],'playback rate does not truncate the buffer duration twice');
engine.releaseChannel(2,72);
assert(second.stops.some(t=>Math.abs(t-1.500125)<1e-8),'RLSE 16 gives 500ms after Note Off');
// One-shot edits must not generate a short gate.
engine.trigger(3,127,1,{midiPitch:60,env:neutral},4);
assert.equal(nodes.filter(n=>n.kind==='sample').at(-1).stops.length,0,'neutral grid hit remains transparent');
for (const att of [32,31]) for(const rel of [0,1,16,32]) {
 engine.trigger(3,127,1,{midiPitch:60,env:{...neutral,att,rel}},4);
 assert.equal(nodes.filter(n=>n.kind==='sample').at(-1).stops.length,0,'one-shot edits preserve natural sample length');
}
for(const att of [32,31,0]) for(const rel of [0,1,16,32]) {
 engine.trigger(3,127,1,{midiPitch:60,env:{...neutral,att,rel,gated:true}},4);
 const sample=nodes.filter(n=>n.kind==='sample').at(-1);
 assert(sample.stops.some(t=>Math.abs(t-(1.125+rel/32+1/8000))<1e-8),'fixed one-step gate independent of A/R');
}
engine.trigger(6,127,3,{midiPitch:60,env:{...neutral,dec:0,sus:0,rel:16}},7);
const minimumSustain=nodes.filter(n=>n.kind==='sample').at(-1).connections[0].gain;
assert(minimumSustain.calls.some(c=>c[0]==='value'&&c[1]===1/32),'legacy SUST 0 remains audible');
// Trim + pitch leave just 20ms. Even maximum A/D/R must fit that region.
engine.loadBuffer(7,ctx.createBuffer(1,800,8000));
for(const trimEnd of [21,60])for(const inv of [false,true]) {
 engine.trigger(7,127,4,{midiPitch:72,seek:20,end:trimEnd,inv,env:{att:0,dec:32,sus:16,rel:32}},8);
 const sample=nodes.filter(n=>n.kind==='sample').at(-1),gain=sample.connections[0].gain;
 const end=4+(trimEnd-20)/100*.1/2;
 assert.equal(sample.stops.length,0,'short one-shot plays to natural end');
 assert(gain.calls.some(c=>c[0]==='linear'&&c[1]===1&&c[2]>4&&c[2]<end),'attack reaches peak within the short sample');
 assert(gain.calls.every(c=>c.length<3||c[2]<=end+1e-8),'ADSR does not run beyond audio');
}
console.log('PASS: maximum ADSR fits trimmed/high-pitched/reversed sample duration.');
