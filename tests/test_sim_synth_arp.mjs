import assert from 'node:assert/strict';
import { createSynthArp, arpSettings } from '../standalone-tools/web-standalone/src/firmware/synthArp.js';
import { createEngine } from '../standalone-tools/web-standalone/src/audio/engine.js';
import { emptyFilt } from '../standalone-tools/web-standalone/src/firmware/const.js';

// Exercise the actual Web Audio synth path and inspect oscillator frequencies.
const oscillators=[];
function param(){return {value:0,setValueAtTime(v){this.value=v;},setTargetAtTime(v){this.value=v;},linearRampToValueAtTime(){},exponentialRampToValueAtTime(){},cancelScheduledValues(){}};}
function node(){return {gain:param(),frequency:param(),Q:param(),pan:param(),delayTime:param(),connect(){},disconnect(){},start(){},stops:[],stop(at){this.stops.push(at);}};}
globalThis.AudioContext=class {
  sampleRate=8000;currentTime=0;state='running';destination=node();
  createGain=node;createDelay=node;createBiquadFilter=node;createStereoPanner=node;createWaveShaper=node;
  createOscillator(){const osc=node();oscillators.push(osc);return osc;}
  createBuffer(channels,length,sampleRate){const data=Array.from({length:channels},()=>new Float32Array(length));return {length,sampleRate,numberOfChannels:channels,duration:length/sampleRate,getChannelData:c=>data[c]};}
};
const engine=createEngine();engine.ensure();
for(const ch of [13,14]) {
  const frequencies=[];
  for(let i=0;i<6;i++) {
    const f=emptyFilt();
    engine.trigger(ch,100,i*.2,{...f,detune:0,oct:0,midiPitch:60,env:f},ch+1);
    frequencies.push(oscillators.at(-1).frequency.value);
  }
  assert(frequencies.every(f=>Math.abs(f-261.625565)<.00001),`CH${ch} repeated notes must not alternate by a tritone at default ARP`);
}
for (const ch of [13,14]) {
  const f=emptyFilt(ch);
  engine.trigger(ch,100,0,{...f,midiPitch:69,detune:0,octaveRaw:f.oct,env:f},ch+1);
  const pair=oscillators.slice(-2).map(osc=>osc.frequency.value);
  assert.deepEqual(pair,[440,440], 'SIM defaults keep both mono voices at neutral CENT and OCTV');
  engine.trigger(ch,100,0,{...f,midiPitch:69,cent:16,semi:0,detune:1,octaveRaw:32,env:f},ch+1);
  const tuned=oscillators.slice(-2).map(osc=>osc.frequency.value);
  assert.equal(tuned[0],440,'mono SEMI, DTNE and OCTV do not shift oscillator 1');
  assert(Math.abs(tuned[1]-440*2**(3+1/12))<1e-8,'oscillator 2 gets firmware OCTV and DTNE');
}
engine.trigger(14,100,0,{midiPitch:60,inputPitch:60,held:true,cent:16,semi:16,detune:0,oct:0},15);
const sustained=oscillators.at(-1);
assert.equal(sustained.stops.length,0,'held MIDI note sustains until Note Off');
engine.releaseChannel(14,60);
assert.equal(sustained.stops.length,1);
assert.equal(arpSettings({arp:16}).enabled,false);
assert.equal(arpSettings({arp:15}).eighths,true);
assert.equal(arpSettings({arp:17}).eighths,false);

const params={11:{arp:16,span:0},13:{arp:17,span:0},14:{arp:15,span:0}};
const events=[], releases=[];
const arp=createSynthArp({params:ch=>params[ch],play:(...args)=>events.push(args),release:ch=>releases.push(ch)});
const clock=(now,playing=false,beat=1,bpm=120)=>({now,when:now/1000,playing,beat,bpm});
arp.noteOn(13,'a',60,100,clock(0));
assert.deepEqual(events.map(e=>e[1]),[60], 'phrase starts at the base note');
arp.service(clock(124));assert.equal(events.length,1);
arp.service(clock(125));assert.equal(events.at(-1)[1],61);
arp.service(clock(250));assert.equal(events.at(-1)[1],60);
arp.noteOff('a');arp.service(clock(1000));assert.equal(events.length,3);
arp.noteOn(13,'a',60,100,clock(1000));assert.equal(events.at(-1)[1],60,'new phrase resets phase');
arp.reset();events.length=0;
arp.noteOn(14,'b',60,100,clock(0));
arp.service(clock(125));assert.equal(events.length,1,'negative ARP uses eighths');
arp.service(clock(250));assert.equal(events.at(-1)[1],61);
arp.reset();events.length=0;
params[13]={arp:32,span:0};
arp.noteOn(13,'a',60,80,clock(0,true));
arp.noteOn(13,'b',64,100,clock(0,true));
assert.equal(events.length,1,'adding a held chord does not advance the clock');
arp.service(clock(500,true));assert.equal(events.length,1);
arp.playhead(clock(125,true,2));assert.equal(events.at(-1)[1],64);
arp.playhead(clock(125,true,2));assert.equal(events.length,2,'same playhead beat must not fire twice');
arp.playhead(clock(250,true,3));assert.equal(events.at(-1)[1],60);
arp.noteOff('a');assert(arp.hasHeld(13));arp.noteOff('b');assert(!arp.hasHeld(13));
arp.reset();events.length=0;
arp.noteOn(11,'c',60,100,clock(0));arp.service(clock(500));assert.equal(events.length,1,'ARP off has no timed pitch steps');
params[11].arp=32;arp.service(clock(501));assert.equal(events.at(-1)[1],60);
arp.service(clock(626));assert.equal(events.at(-1)[1],72,'CH11 uses signed ARP too');
arp.reset();assert(!arp.hasHeld(11));assert(releases.includes(11));
console.log('PASS: actual CH13/14 oscillator pitches stay stable; signed ARP timing, held chords, release, restart, playhead sync and CH11.');
