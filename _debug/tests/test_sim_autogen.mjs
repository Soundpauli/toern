import assert from 'node:assert/strict';
import { createAutogen, SECTIONS, phrasePasses, melodyPitch, volumeScale, statusInk, AGEN_STATUS_COLORS } from '../../tools/sim/src/firmware/autogen.js';
import { drawAutogenStatus } from '../../tools/sim/src/firmware/screens.js';
import { emptyNote, setLayout } from '../../tools/sim/src/firmware/const.js';
for (const columns of [16,32]) {
  setLayout(columns);
  const note=Array.from({length:257},()=>Array.from({length:17},emptyNote));
  for(let x=1;x<=columns*3;x++) {
    if(x%4===1)Object.assign(note[x][2],{channel:1});
    if(x%4===3)Object.assign(note[x][3],{channel:2});
    if(x%2===1)Object.assign(note[x][4],{channel:3});
    if(x%8===1)Object.assign(note[x][12],{channel:11,midiPitch:36});
    if(x%4===1)Object.assign(note[x][14],{channel:13,midiPitch:64});
  }
  const original=structuredClone(note),glob={page:1,edit:1,x:3,y:4};
  const controller=createAutogen({note,columns:()=>columns,steps:()=>256,rows:16,emptyNote,bpm:()=>120});
  controller.start(glob);
  assert.deepEqual(controller.original(),original);
  const sections=[];
  for(let generation=0;generation<32;generation++) {
    const state=controller.state();sections.push(state.section);
    assert.equal(state.generation,generation);
    const offset=(state.page-1)*columns;
    if(state.section==='Build'){assert(controller.performance.slice(offset+1,offset+columns+1).flat().every(n=>n.channel!==1));assert(controller.performance.slice(offset+1,offset+columns+1).flat().some(n=>n.channel===11),'bass foundation survives the build');assert.equal(state.remaining,1);}
    if(state.section==='Fill' && generation%2===1) {
      assert(controller.performance[offset+columns].every(n=>!n.channel||n.channel>5));
      for(const x of [columns-1,columns]) assert(controller.performance[offset+x].every(n=>n.channel!==11));
    }
    const passes=state.remaining;
    for(let i=0;i<passes*columns;i++) {
      const next=controller.advance();
      assert(next>=1&&next<=2*columns);
      if(i<passes*columns-1)assert.equal(controller.state().generation,generation,'section changes only after final full page');
    }
    assert.equal(controller.state().generation,generation+1);
    assert.equal(controller.state().page,state.page===1?2:1);
  }
  assert.deepEqual(sections, [...SECTIONS,...SECTIONS]);
  controller.stop(glob);assert.deepEqual(note,original);assert.equal(controller.state().active,false);
  assert.deepEqual(glob,{page:1,edit:1,x:3,y:4});
  for(const bpm of [40,60,90,120,180,240])for(let g=0;g<24;g++) {
    const section=SECTIONS[g%SECTIONS.length],passes=phrasePasses(section,g,columns,bpm);
    assert(Number.isInteger(passes)&&passes>=1&&passes<=8);
    // A single 32-step pass at 40 BPM itself takes 12 seconds.
    const cap=section==='Build'?32000:section==='Break'?24000:16000;
    assert(passes===1||2*passes*columns*15000/bpm<=cap);
  }
  for(const ready of [false,true])for(let progress=0;progress<=12;progress++)for(const now of [0,240,720,1800]) {
    const pixels=new Map();let text=0;
    const matrix={light:(x,y,c)=>{assert(x>=1&&x<=columns&&y>=1&&y<=16);pixels.set(`${x},${y}`,c);},drawText:()=>text++};
    drawAutogenStatus(matrix,{agen:{page:1,progress,ready}},now);
    assert.equal(text,0,'AGEN uses an icon, never a numeral');
    for(let x=0;x<5;x++)for(let y=0;y<5;y++)assert.deepEqual(pixels.get(`${columns-4+x},${16-y}`),AGEN_STATUS_COLORS[statusInk(x,y,progress,ready,now)===3&&(x===0||x===4||y===0||y===4)?2:statusInk(x,y,progress,ready,now)]);
    assert.deepEqual(pixels.get('11,16'),ready?[220,180,0]:[200,0,0]);
  }
}
for(const ready of [false,true])for(let progress=0;progress<=12;progress++) {
  const lit=[];for(let y=0;y<5;y++)for(let x=0;x<5;x++)lit.push(statusInk(x,y,progress,ready,0));
  assert.equal(lit.filter(v=>v===4).length,ready?4:0);
  assert(lit.filter(Boolean).length>=15,'icon never blinks off');
}
console.log('PASS: simulator AGEN sections, boundaries, gaps, source restoration, BPM caps, volume-only automation and activity icon on 16/32-column panels.');

let moved=0,bigMoves=0;
for(let step=0;step<32;step++)for(let generation=2;generation<=3;generation++) {
  const pitch=melodyPitch(60,0,false,generation,13,step);
  assert([0,2,4,5,7,9,11].includes(pitch%12));
  assert(Math.abs(pitch-60)<=12);moved+=pitch!==60;bigMoves+=Math.abs(pitch-60)>=4;
}
assert(moved>32&&bigMoves>16,'variations audibly develop the melody');
assert.equal(phrasePasses('Build',8,32,120)*2*32/16,4);
assert.equal(phrasePasses('Fill',10,32,120)*2*32/16,4);

for(const section of new Set(SECTIONS))for(const ch of [1,2,3,5,6,7,8,11,13,14])for(const p of [0,.25,.5,.75,1]) {
  const bass=ch===6||ch===14;
  const level=volumeScale(section,ch,bass,p);
  assert(level>=0&&level<=16);
  if(section==='Drop')assert.equal(level,16);
  if(['Break','Build','Fill'].includes(section)&&(ch===1||bass))assert.equal(level,0);
}

// Low chords retain three ordered voices and compensate patch registers once.
const {voicingPitches,encodePitch}=await import('../../tools/sim/src/firmware/autogen.js');
for(let g=0;g<16;g++)for(const minor of [false,true])for(let root=0;root<12;root++){
 const chord=voicingPitches(60,root,minor,g),next=voicingPitches(60,root,minor,g+1);
 assert.equal(new Set(chord).size,3);assert(chord[0]<chord[1]&&chord[1]<chord[2]&&chord[2]-chord[0]<=12);
 assert.notDeepEqual(chord,next);
}
assert.equal(encodePitch(60,-24),84);assert.equal(encodePitch(36,-12),48);
for(const width of [16,32]) {
 const note=Array.from({length:257},()=>Array.from({length:17},emptyNote));
 for(const [row,pitch] of [[11,36],[12,40],[13,43]])Object.assign(note[1][row],{channel:11,midiPitch:pitch});
 const original=structuredClone(note),glob={page:1,edit:1,x:1,y:12};
 const c=createAutogen({note,columns:()=>width,steps:()=>256,rows:16,emptyNote,bpm:()=>110,params:ch=>({cent:ch===11?5:ch===13?0:8})});
 c.start(glob);
 const chord=c.performance[(c.state().page-1)*width+1].filter(n=>n.channel===11).map(n=>n.midiPitch);
 assert.equal(chord.length,3);assert.equal(new Set(chord).size,3);assert(chord.every(n=>n>=24&&n<=60));
 c.stop(glob);assert.deepEqual(note,original);
 // Hats alone, or a muted bass, cannot create a roll or unsupported break.
 for(const mutedBass of [false,true]){
  for(const col of note)for(const n of col)Object.assign(n,emptyNote());
  Object.assign(note[1][4],{channel:3});if(mutedBass)Object.assign(note[1][7],{channel:6,midiPitch:36});
  const sparse=createAutogen({note,columns:()=>width,steps:()=>256,rows:16,emptyNote,bpm:()=>110,isMuted:ch=>ch!==3});
  sparse.start(glob);
  for(let i=0;i<width*80;i++){
   assert(!['Build','Fill','Break'].includes(sparse.state().section));
   const off=(sparse.state().page-1)*width;
   assert(sparse.performance.slice(off+1,off+width+1).flat().every(n=>!n.channel||n.channel===3));sparse.advance();
  }
  sparse.stop(glob);
 }
}
console.log('PASS: three-tone chord variation, sounding registers, untouched source and sparse/muted buildup protection.');

// AGEN plays separate workspace pages while the pink motif remains editable.
for(const width of [16,32]) {
 const note=Array.from({length:257},()=>Array.from({length:17},emptyNote)),muted=Array(16).fill(false);
 Object.assign(note[1][2],{channel:1});Object.assign(note[1][14],{channel:13,midiPitch:60});
 const original=structuredClone(note),glob={page:1,edit:1,x:1,y:14};
 const controller=createAutogen({note,columns:()=>width,steps:()=>256,rows:16,emptyNote,bpm:()=>120,isMuted:ch=>muted[ch]});
 controller.start(glob);assert.equal(controller.state().motifPage,1);assert.equal(controller.state().playingSlot,10);
 assert.deepEqual(note.slice(1,width+1),original.slice(1,width+1),'generation never edits the pink motif');
 const playing=structuredClone(controller.performance.slice(1,width+1));
 Object.assign(note[1][14],emptyNote());Object.assign(note[3][14],emptyNote(),{channel:13,midiPitch:67,velocity:79,probability:83,condition:3});
 assert(controller.poll());assert.equal(controller.state().revision,1);
 assert.deepEqual(controller.performance.slice(1,width+1),playing,'an edit never rewrites the playing page');
 function boundary(){const generation=controller.state().generation;while(controller.state().generation===generation)controller.advance();}
 const variations=new Set(),editedRolls=[];let editedDrop=false;
 for(let response=0;response<12;response++){
  boundary();const offset=(controller.state().page-1)*width;
  variations.add(JSON.stringify(controller.performance.slice(offset+1,offset+width+1)));
  if(controller.state().section==='Build')editedRolls.push(controller.performance.slice(offset+1,offset+width+1).filter(col=>col.some(n=>n.channel===2)).length);
  if(controller.state().section==='Drop')editedDrop=true;
  assert.equal(note[3][14].midiPitch,67,'edited source remains authoritative while output develops');
  assert.equal(note[1][14].channel,0,'deleted source onset stays deleted');
 }
 assert.equal(editedRolls.length,2);assert(editedRolls[1]>editedRolls[0]&&editedRolls[0]>0&&editedDrop,'buildup accelerates then drops after editing');
 assert(variations.size>=3,'edited sources develop while mono phrases repeat more calmly');
 controller.toggleView();
 Object.assign(note[width+5][14],emptyNote(),{channel:13,midiPitch:72});
 controller.selectMotif(2);boundary();
 assert.equal(controller.state().motifPage,2);
 assert.equal(note[width+5][14].midiPitch,72);
 assert.notEqual(controller.performance[(controller.state().page-1)*width+5][14].midiPitch,255);
 controller.selectMotif(1);boundary();assert.equal(note[3][14].midiPitch,67);
 muted[13]=true;controller.poll();boundary();
 let offset=(controller.state().page-1)*width;assert(!controller.performance.slice(offset+1,offset+width+1).flat().some(n=>n.channel===13));
 muted[13]=false;controller.poll();boundary();offset=(controller.state().page-1)*width;
 assert(controller.performance.slice(offset+1,offset+width+1).flat().some(n=>n.channel===13));
 for(let col=1;col<=width;col++)for(const n of note[col])Object.assign(n,emptyNote());
 controller.poll();boundary();offset=(controller.state().page-1)*width;
 assert(controller.performance.slice(offset+1,offset+width+1).flat().every(n=>!n.channel),'cleared motif stays silent');
 controller.stop(glob);assert.deepEqual(note,original);
}
console.log('PASS: pink editable motif, virtual 10/11, continued development after source edits, mute/unmute, silence and original restoration.');
