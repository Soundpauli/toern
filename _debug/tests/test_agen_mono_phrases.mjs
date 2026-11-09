import assert from 'node:assert/strict';
import {createAutogen} from '../../tools/sim/src/firmware/autogen.js';
import {emptyNote} from '../../tools/sim/src/firmware/const.js';
for(const width of [16,32]){
 const note=Array.from({length:257},()=>Array.from({length:17},emptyNote));
 for(const [step,row,ch,pitch] of [[1,2,1,255],[1,7,6,60],[3,14,13,60],[11,14,13,64],[1,15,14,48],[9,15,14,48],[1,12,11,60]])Object.assign(note[step][row],{channel:ch,midiPitch:pitch});
 const c=createAutogen({note,columns:()=>width,steps:()=>256,rows:16,emptyNote,bpm:()=>120});c.start({edit:1,page:1});
 const payload=ch=>c.performance.slice((c.state().page-1)*width+1,c.state().page*width+1).map(col=>col.filter(n=>n.channel===ch).map(n=>[n.midiPitch,n.velocity,n.probability,n.condition]));
 let previous=null,holds=0;
 for(let gen=0;gen<12;gen++){
  const state=c.state(),now={section:state.section,voices:[payload(13),payload(14)]};
  if(previous&&gen%Math.ceil(64/width)!==0&&now.section===previous.section){
   for(let voice=0;voice<2;voice++)if(previous.voices[voice].flat().length){assert.deepEqual(now.voices[voice],previous.voices[voice]);holds++;}
  }
  const offset=(state.page-1)*width;
  for(const col of c.performance.slice(offset+1,offset+width+1))assert(col.filter(n=>n.channel===11).length<=1,'single CH11 source stays single-note');
  previous=now;while(c.state().generation===gen)c.advance();
 }
 assert(holds>=4,'mono phrases repeat while the arranger keeps advancing');
}
console.log('PASS: CH13/14 repeat phrases across page boundaries; CH11 single-note source stays monophonic on both layouts.');
