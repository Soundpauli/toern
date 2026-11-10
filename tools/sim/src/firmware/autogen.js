import { neutralGridMidi } from "./pitch.js";
// Browser counterpart of the firmware's temporary, section-driven AGEN workspace.
// The saved bank stays immutable; an editable motif steers two separate performance pages.
// 64-bar cycle: a short rise returns to the groove (kick and bass come back).
// The Break is a melodic solo (every drum rests), the Solo a drum solo (every
// tonal voice rests). The fill and drop close the cycle once, not every 16 bars.
const FORM = [['Groove',8],['Variation',8],['Build',2],['Groove',8],['Variation',8],['Break',4],
  ['Groove',8],['Groove',8],['Solo',2],['Build',2],['Fill',2],['Drop',4]];
export const SECTIONS = FORM.flatMap(([section])=>[section,section]);
const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
// AGEN menu order; LONG keeps the drawn motif and doubles groove, variation and drop.
export const MOTION = {HOLD:0,SOFT:1,LONG:2,LIVE:3};
export function phrasePasses(section, generation, columns, bpm, longForm=false) {
  if(section==='Build'||section==='Fill')return 1;
  let bars = FORM[Math.floor(generation/2)%FORM.length][1];
  if(longForm&&(section==='Groove'||section==='Variation'||section==='Drop'))bars*=2;
  const limit=longForm?48000:24000;
  while(bars>1 && bars*240000>Math.max(40,bpm)*limit)bars/=2;
  return Math.max(1, Math.floor(bars*16/(2*columns)));
}
export const SCALE = {MAJOR:0,MINOR:1,HARMONIC:2,DORIAN:3,PENTA:4,BLUES:5};
const SCALE_PCS = [
  [0,2,4,5,7,9,11],[0,2,3,5,7,8,10],[0,2,3,5,7,8,11],[0,2,3,5,7,9,10],[0,3,5,7,10],[0,3,5,6,7,10],
];
const major=SCALE_PCS[0], minorScale=SCALE_PCS[1];
export function scaleCount(sc){return SCALE_PCS[sc%6].length;}
export function scalePc(root, sc, degree) {
  const pcs = SCALE_PCS[typeof sc === "boolean" ? (sc ? SCALE.MINOR : SCALE.MAJOR) : sc%6];
  return (root + pcs[degree % pcs.length]) % 12;
}
function degreeIndex(sc, generation) {
  const slots=[0,1,2,3,3,0,1,2], slot=slots[Math.floor(generation/2)%8];
  if(sc===SCALE.PENTA||sc===SCALE.BLUES)return [0,3,2,4][slot];
  return ((sc===SCALE.MINOR||sc===SCALE.HARMONIC)?[0,5,3,6]:[0,5,3,4])[slot];
}
// lock >= 0 pins the chord to a scale degree read from the drawn progression.
function chordPitchClasses(root, sc, generation, lock=-1) {
  const s = typeof sc === "boolean" ? (sc ? SCALE.MINOR : SCALE.MAJOR) : sc;
  const d = lock>=0 ? lock : degreeIndex(s, generation);
  return [0,1,2,3].map(t=>scalePc(root,s,d+t*2));
}
// Firmware drawnDegree: the scale degree whose stacked chord best explains the
// drawn pitch classes; the lowest note breaks ties as the root.
export function drawnDegree(root, sc, pcs, lowestPc) {
  let best=0,bestScore=-Infinity;
  for(let d=0;d<scaleCount(sc);d++){
    const tones=[];let score=0;
    [5,3,3,1].forEach((weight,t)=>{const pc=scalePc(root,sc,d+t*2);if(tones.includes(pc))return;tones.push(pc);if(pcs.includes(pc))score+=weight;});
    score-=3*new Set(pcs.filter(pc=>!tones.includes(pc))).size;
    if(scalePc(root,sc,d)===lowestPc)score+=2;
    if(score>bestScore){bestScore=score;best=d;}
  }
  return best;
}
// Firmware accent: voices drawn at one level get a metric groove; the drop leans on the beat.
export function accent(section, ch, step, velocity, flat) {
  const beat=step%4;let level=velocity;
  if(flat){
    let percent=(ch<=5?[100,78,90,78]:[100,88,94,88])[beat];
    if(ch===1&&beat)percent=92;
    if(ch===3&&beat===2)percent=100;
    level=Math.floor(velocity*percent/100);
  }
  if(section==='Drop'&&beat===0)level+=Math.floor(level/10);
  return clamp(level,1,127);
}
const nearestPc=(reference,pc)=>{let best=reference,d=Infinity;for(let n=0;n<=127;n++)if(n%12===pc&&Math.abs(n-reference)<d){best=n;d=Math.abs(n-reference);}return best;};
// Firmware fitPitch: snap the drawn note to the chord on the beat, otherwise the scale.
export function fitPitch(reference, root, sc, generation, channel, step, lock=-1) {
  const chord = chordPitchClasses(root, sc, generation, lock);
  if ((channel === 6 || channel === 14) && step % 4 === 0) return nearestPc(reference, chord[0]);
  const scaleId = typeof sc === "boolean" ? (sc ? SCALE.MINOR : SCALE.MAJOR) : sc;
  const choices = step % 4 === 0 ? chord.slice(0,3) : [...Array(scaleCount(scaleId))].map((_,d)=>scalePc(root,scaleId,d));
  let best = reference, distance = Infinity;
  for (const pc of choices) {
    const n = nearestPc(reference, pc), gap = Math.abs(n - reference);
    if (gap < distance) { best = n; distance = gap; }
  }
  return best;
}
export function bassHit(step, density, seed) {
  if(step%4===0)return true;
  if(!density)return false;
  const h=((seed*1664525)+step*1013904223)>>>8;
  let chance=density===1?22:density===2?48:72;
  if(step%2)chance=Math.floor(chance*2/3);
  return h%100<chance;
}
export function bassPitch(reference, root, sc, generation, step, seed, lock=-1) {
  const rootPc = chordPitchClasses(root, sc, generation, lock)[0];
  const locked = nearestPc(reference, rootPc);
  if(step%4===0)return locked;
  const h=(seed+step*977)>>>8;
  if(h%4===0)return nearestPc(reference+12, rootPc);
  return locked;
}
export function melodyPitch(reference, root, sc, generation, channel, step, seed=113, motion=MOTION.LIVE, density=2, lock=-1) {
  const section=SECTIONS[generation%SECTIONS.length], bass=channel===6||channel===14;
  const scaleId = typeof sc === "boolean" ? (sc ? SCALE.MINOR : SCALE.MAJOR) : sc;
  const chord=chordPitchClasses(root,scaleId,generation,lock).slice(0,3);
  const scale=[...Array(scaleCount(scaleId))].map((_,d)=>scalePc(root,scaleId,d));
  const baseline=fitPitch(reference,root,scaleId,generation,channel,step,lock);
  if(motion===MOTION.HOLD||motion===MOTION.LONG||(bass&&step%4===0))return baseline;
  const allowed=step%4===0?chord:scale;
  let h=(seed ^ Math.imul(Math.floor(generation/2),374761393) ^ Math.imul(channel,668265263) ^ Math.imul(step,2246822519))>>>0;
  h=Math.imul(h^(h>>>13),1274126177)>>>0;h=(h^(h>>>16))>>>0;
  let chance=section==='Variation'||section==='Fill'?90:section==='Build'?80:section==='Break'?65:section==='Drop'?45:40;
  if(motion===MOTION.SOFT&&chance>35)chance=35;
  if(density===0&&chance>18)chance=18;
  else if(density===1&&chance>32)chance=32;
  if(h%100>=chance)return baseline;
  let delta=[2,4,3,7,5,-2,4,-5][(Math.floor(step/2)+channel+Math.floor(generation/2))%8];
  if(generation%2)delta=-delta;
  if(motion===MOTION.SOFT)delta=delta>0?2:-2;
  const target=reference+delta;
  let best=baseline, distance=Infinity;
  for(let pitch=Math.max(0,reference-12);pitch<=Math.min(127,reference+12);pitch++) {
    if(!allowed.includes(pitch%12))continue;
    const gap=Math.abs(pitch-target);
    if(gap<distance){best=pitch;distance=gap;}
  }
  return best;
}

export function voicingPitches(reference,root,sc,generation,seed=113,stable=false,tones=3,lock=-1) {
  const scaleId = typeof sc === "boolean" ? (sc ? SCALE.MINOR : SCALE.MAJOR) : sc;
  tones=Math.max(3,Math.min(4,tones));
  const chord=chordPitchClasses(root,scaleId,generation,lock).slice(0,tones);
  if(stable)return chord.map(pc=>nearestPc(reference,pc));
  const inversion=(generation+seed)%tones;
  let base=reference,distance=Infinity;
  for(let n=0;n<=127;n++)if(n%12===chord[0]&&Math.abs(n-reference)<distance){base=n;distance=Math.abs(n-reference);}
  const notes=[...Array(tones)].map((_,i)=>{const tone=(i+inversion)%tones;return base+(chord[tone]-chord[0]+12)%12+(tone<inversion?12:0);});
  let center=Math.trunc(notes.reduce((a,b)=>a+b,0)/tones),shift=0;
  while(center+shift>reference+6)shift-=12;
  while(center+shift<reference-6)shift+=12;
  while(notes[0]+shift<24)shift+=12;
  while(notes[tones-1]+shift>108)shift-=12;
  return notes.map(n=>n+shift);
}
export function encodePitch(pitch,octaveShift) {
  pitch-=octaveShift;while(pitch<0)pitch+=12;while(pitch>127)pitch-=12;return pitch;
}

export const AGEN_MOTIF_COLORS = [[0,0,0],[30,4,18],[230,30,150],[210,120,20],[30,190,90]];
export const AGEN_STATUS_COLORS = [[0,0,0],[4,12,30],[40,100,230],[210,120,20],[30,190,90]];
export function statusInk(x,y,progress,ready,now) {
  const ring=[[2,0],[3,0],[4,1],[4,2],[4,3],[3,4],[2,4],[1,4],[0,3],[0,2],[0,1],[1,0]];
  const i=ring.findIndex(p=>p[0]===x&&p[1]===y);
  if(i>=0)return i<progress?(!ready&&progress>=12?3:2):1;
  if(ready)return ((x===1&&y===2)||(x===2&&y===3)||(x===3&&(y===1||y===2)))?4:0;
  const spinner=[[2,1],[3,1],[3,2],[3,3],[2,3],[1,3],[1,2],[1,1]];
  const j=spinner.findIndex(p=>p[0]===x&&p[1]===y),frame=Math.floor(now/240)%8;
  return j>=0&&(frame+8-j)%8<3?3:0;
}
// Match the firmware's volume-only performance automation.
export function volumeScale(section, ch, bass, progress) {
  const p=clamp(progress,0,1);
  if(section==='Solo')return 16;
  if(section==='Break')return ch===1||bass?0:ch<=5?7:16;
  if(section==='Build')return ch===1||bass?0:ch===2?5+p*11:ch<=5?6+p*9:10+p*6;
  if(section==='Fill')return ch===1||bass?0:ch===2||ch===5?12+p*4:12;
  if(section==='Drop')return 16;
  const triangle=1-Math.abs(2*p-1);
  if(ch===3||ch===5)return 11+triangle*5;
  if(ch===7||ch===8||(!bass&&(ch===11||ch===13||ch===14)))return 12+triangle*4;
  return 16;
}
export function inferHarmony(columns) {
  const events=columns.flatMap(col=>col.flatMap((n,row)=>n.channel>5?[{ch:n.channel,pitch:n.midiPitch<=127?n.midiPitch:neutralGridMidi(n.channel,row)}]:[]));
  let best=-1, result={root:0,minor:false};
  for(const minor of [false,true])for(let root=0;root<12;root++) {
    const scale=minor?minorScale:major;
    const matches=events.filter(n=>scale.includes((n.pitch-root+120)%12)).length;
    const tonic=events.filter(n=>n.pitch%12===root).length;
    const bass=events.filter(n=>(n.ch===6||n.ch===11||n.ch===14)&&n.pitch%12===root).length;
    const score=matches*100+tonic*2+bass*8;
    if(score>best){best=score;result={root,minor};}
  }
  return result;
}
export function nearSource(pitch,reference){
 while(pitch>reference+6)pitch-=12;while(pitch<reference-6)pitch+=12;
 return pitch<0?pitch+12:pitch>127?pitch-12:pitch;
}
export function monoPitch(reference,root,sc,generation,ch,step,seed=113,motion=MOTION.LIVE,density=2,lock=-1){
 const scaleId = typeof sc === "boolean" ? (sc ? SCALE.MINOR : SCALE.MAJOR) : sc;
 if(motion===MOTION.HOLD)return reference;
 if(!density&&step%2)return reference;
 const chord=chordPitchClasses(root,scaleId,generation,lock),scale=[...Array(scaleCount(scaleId))].map((_,d)=>scalePc(root,scaleId,d));
 let target=reference;const h=(seed+step*977+ch*37+generation*101)>>>0;
 if(motion===MOTION.LIVE&&density>=2&&SECTIONS[generation%SECTIONS.length]==='Variation'&&step%4&&h%4===0)target+=(h&4)?2:-2;
 const pcs=ch===14&&step%4===0?[chord[0]]:step%4===0?chord.slice(0,3):scale;
 let best=reference,gap=Infinity;
 for(let n=Math.max(0,reference-12);n<=Math.min(127,reference+12);n++)if(pcs.includes(n%12)&&Math.abs(n-target)<gap){best=n;gap=Math.abs(n-target);}
 best=nearSource(best,reference);
 if((motion===MOTION.SOFT||density<2)&&Math.abs(best-reference)>2)return reference;
 return best;
}
export function createAutogen({ note, columns, steps, rows, emptyNote, bpm, params=()=>({}), isMuted=()=>false, volume=()=>16 }) {
  let source = null, saved = null, generation = 0, pass = 0, page = 1, stepInPage = 0;
  let section = 'Groove', repeats = 1, lowCh11 = false;
  let foundation=0,registers=Array(16).fill(60),shifts=Array(16).fill(0);
  let key = 0, scale = [0,2,4,5,7,9,11];
  // Firmware default is SOFT / AUTO scale / MED density.
  let motion = MOTION.SOFT, seed = 113, scaleChoice = 0, density = 1;
  let motifPage=1, performancePages=[2,3], fingerprint=[], revision=0, removed=0, muteSignature='';
  let performanceView=true,previousSection=null,builtRevision=-1;
  const removedByPage=new Map();
  const performance=note.map(col=>col.map(n=>({...n})));
  const motif = () => note.slice((motifPage-1)*columns()+1,motifPage*columns()+1).map(col=>col.map(n=>({...n})));
  function fingerprints() {
    const result=Array(16).fill(2166136261),start=(motifPage-1)*columns()+1;
    for(let col=0;col<columns();col++)for(let row=1;row<=rows;row++) {
      const n=note[start+col][row],ch=n.channel;if(!ch||ch>=16)continue;
      for(const value of [col,row,n.velocity,n.probability,n.condition,n.midiPitch])result[ch]=Math.imul(result[ch]^(value??0),16777619)>>>0;
    }
    return result;
  }
  function sourceRegisters() {
    const voices=source.flatMap(col=>col.flatMap((n,row)=>n.channel===11?[n.midiPitch<=127?n.midiPitch:neutralGridMidi(11,row)]:[]));
    lowCh11=voices.length>0&&voices.reduce((a,b)=>a+b,0)/voices.length<48&&!source.some(col=>col.filter(n=>n.channel===11).length>1);
    for(const ch of [11,13,14]) {
      const pitches=source.flatMap(col=>col.flatMap((n,row)=>n.channel===ch?[n.midiPitch<=127?n.midiPitch:neutralGridMidi(ch,row)]:[]));
      const bass=ch===14||(ch===11&&lowCh11),reference=pitches.length?pitches.reduce((a,b)=>a+b,0)/pitches.length:bass?36:60;
      registers[ch]=Math.floor(reference);
    }
  }
  const manualSignature = () => Array.from({length:15},(_,ch)=>`${isMuted(ch)?1:0}${volume(ch)>0?1:0}`).join('');
  function poll() {
    if(!source)return false;
    const next=fingerprints(),changed=[];
    for(let ch=1;ch<15;ch++)if(next[ch]!==fingerprint[ch])changed.push(ch);
    const signature=manualSignature(),controlsChanged=signature!==muteSignature;muteSignature=signature;
    if(changed.length) {
      source=motif();fingerprint=next;sourceRegisters();
      for(const ch of changed){if(source.some(col=>col.some(n=>n.channel===ch)))removed&=~(1<<ch);else removed|=1<<ch;}
    }
    if(changed.length)removedByPage.set(motifPage,removed);
    if(changed.length||controlsChanged){revision++;return true;}
    return false;
  }
  const resolvedScale = () => scaleChoice ? Math.min(SCALE.BLUES, scaleChoice-1) : (scale[2]===3?SCALE.MINOR:SCALE.MAJOR);
  const state = () => ({ active: !!source, performanceView, section, generation, page, remaining: repeats-pass, ready: true,
    motion, scale: scaleChoice, density, motifPage, motifSlot:motifPage<=8?motifPage:9, playingSlot:10+performancePages.indexOf(page), otherSlot:10+(performancePages.indexOf(page)^1), revision,
    progress: Math.min(columns()>=32?17:5,Math.floor((pass*columns()+stepInPage)*(columns()>=32?17:5)/Math.max(1,repeats*columns()-1))) });
  function build() {
    section = SECTIONS[generation % SECTIONS.length];
    foundation=[6,14,11,13,7,8,1].find(ch=>!isMuted(ch)&&volume(ch)>0&&source.some(col=>col.some(n=>n.channel===ch)))||0;
    if(!foundation&&['Break','Build','Fill'].includes(section))section='Groove';
    // A melodic break needs a melodic voice; a drum solo needs drums.
    if(section==='Break'&&foundation<=5)section='Groove';
    if(section==='Solo'&&![1,2,3,4,5].some(ch=>!isMuted(ch)&&volume(ch)>0&&source.some(col=>col.some(n=>n.channel===ch))))section='Variation';
    const longForm = motion === MOTION.LONG;
    const sc = resolvedScale();
    const pitchOf=(n,row)=>n.midiPitch<=127?n.midiPitch:neutralGridMidi(n.channel,row);
    // Drawn CH11 progression: each chord holds until the next; the page loops.
    const stepDegree=Array(columns()).fill(-1);
    {
      let current=-1,first=-1,moves=false;
      source.forEach((col,x)=>{
        const chord=col.flatMap((n,row)=>n.channel===11?[pitchOf(n,row)]:[]);
        if(chord.length>=2){
          current=drawnDegree(key,sc,chord.map(p=>p%12),Math.min(...chord)%12);
          if(first<0)first=current;else if(current!==first)moves=true;
        }
        stepDegree[x]=current;
      });
      for(let x=0;x<stepDegree.length;x++)stepDegree[x]=!moves?-1:stepDegree[x]<0?current:stepDegree[x];
    }
    // Voices drawn at one velocity (three or more hits) were not shaped by hand.
    const flat=new Set();
    for(let ch=1;ch<15;ch++){
      const levels=source.flatMap(col=>col.filter(n=>n.channel===ch).map(n=>n.velocity));
      if(levels.length>=3&&levels.every(v=>v===levels[0]))flat.add(ch);
    }
    const heldPitch=Array(16).fill(-1),heldSource=Array(16).fill(-1);
    const phrase = generation + 1; // firmware arrangeAutogenChannel(..., agen.generation+1)
    repeats = phrasePasses(section, generation, columns(), bpm(), longForm);
    const width = columns(), offset = (page-1)*width;
    const previous=performance.slice((page===1?1:0)*width+1,(page===1?2:1)*width+1).map(col=>col.map(n=>({...n})));
    const held=new Set();
    if(generation>0&&generation%Math.ceil(64/width)!==0&&revision===builtRevision&&previousSection===section)
      for(const ch of [13,14])if(!isMuted(ch)&&volume(ch)>0&&previous.some(col=>col.some(n=>n.channel===ch)))held.add(ch);
    for (let x = 1; x <= width; x++) for (let y = 1; y <= rows; y++) {
      // Develop the editable motif without writing generated notes into it.
      const src = source[(Math.floor(generation/2) * width + x-1) % source.length]?.[y];
      const n = performance[offset+x][y];
      Object.assign(n, emptyNote(), src || {});
      if (!n.channel) continue;
      const ch = n.channel, perc = ch <= 5, lock = stepDegree[x-1], tie = src.condition === 22;
      // Full solos: the drum solo rests every tonal voice, the melodic break every drum.
      if((section==='Solo'&&!perc)||(section==='Break'&&perc)){Object.assign(n,emptyNote());continue;}
      if(held.has(ch)){Object.assign(n,emptyNote());continue;}
      if(isMuted(ch)||volume(ch)<=0){Object.assign(n,emptyNote());continue;}
      const reference = n.midiPitch<=127?n.midiPitch:neutralGridMidi(ch,y);
      const bass = ch===6||ch===14||(ch===11&&lowCh11);
      if (isMuted(ch) || (section === 'Build' && (ch === 1 || bass)&&ch!==foundation) ||
          (section === 'Break' && ch!==foundation && bass) ||
          (section === 'Fill' && generation%2===1 && ((perc && x === width) || (bass && x >= width-1)))) {
        Object.assign(n, emptyNote()); continue;
      }
      if(section==='Build'&&generation%2===1&&x===width&&ch!==foundation){Object.assign(n,emptyNote());continue;}
      if(section==='Build'&&ch===3&&(x-1)%4!==2){Object.assign(n,emptyNote());continue;}
      // A held note is a head plus G/L cells; they keep the head's pitch and fate.
      const tied = !perc && tie && heldSource[ch] === reference;
      if (!tied && bass && !bassHit(x-1, density, seed+ch)) { heldSource[ch]=reference;heldPitch[ch]=-1;Object.assign(n, emptyNote()); continue; }
      if (tied && heldPitch[ch] < 0) { Object.assign(n, emptyNote()); continue; }
      if (!perc) {
        const target=reference;
        if(tied)n.midiPitch=heldPitch[ch];
        else if(bass)n.midiPitch=bassPitch(target,key,sc,phrase,x-1,seed,lock);
        else if(ch===13||ch===14)n.midiPitch=monoPitch(target,key,sc,phrase,ch,x-1,seed,motion,density,lock);
        else n.midiPitch=nearSource(melodyPitch(target,key,sc,phrase,ch,x-1,seed,motion,density,lock),target);
        heldSource[ch]=reference;heldPitch[ch]=n.midiPitch;
      }
      n.condition=tie&&!perc?22:0;
      n.probability = tied || ch === 1 || bass || x % 4 === 1 ? 100 : Math.max(75,n.probability || 100);
      if (!tied) n.velocity = accent(section, ch, x-1, n.velocity, flat.has(ch));
      if (section === 'Fill' && perc) n.velocity = Math.min(127,n.velocity+12);
    }
    // Preserve three distinct chord tones at every source chord onset.
    if(!lowCh11&&!isMuted(11)&&volume(11)>0&&source.some(col=>col.some(n=>n.channel===11)))for(let x=1;x<=width;x++){
      const original=source[(Math.floor(generation/2)*width+x-1)%source.length];
      const chord=original.filter(n=>n.channel===11);
      if(chord.length<2)continue;
      for(const n of performance[offset+x])if(n.channel===11)Object.assign(n,emptyNote());
      // CH11 has three voices. A held chord keeps its G/L cells so it sustains; a
      // drawn progression keeps its chord and, while the groove settles, its close voicing.
      const lock=stepDegree[x-1],heldChord=chord.every(n=>n.condition===22),voices=Math.min(chord.length,3);
      const colour=density>=2&&voices===3&&(section==='Variation'||section==='Drop');
      const stable=longForm||colour||(lock>=0&&(section==='Groove'||section==='Break'));
      const reference=Math.floor(original.flatMap((n,row)=>n.channel===11?[pitchOf(n,row)]:[]).reduce((a,b)=>a+b,0)/chord.length);
      const pitches=voicingPitches(lock>=0?reference:registers[11],key,sc,phrase,seed,stable,colour?4:3,lock);
      const velocity=heldChord?chord[0].velocity:accent(section,11,x-1,chord[0]?.velocity||88,flat.has(11));
      for(let i=0;i<voices;i++){
        let row=clamp(12+i,1,rows);if(performance[offset+x][row].channel)row=performance[offset+x].findIndex((n,y)=>y>0&&!n.channel);
        if(row>0)Object.assign(performance[offset+x][row],emptyNote(),{channel:11,midiPitch:encodePitch(pitches[colour&&i===2?3:i],shifts[11]),velocity,condition:heldChord?22:0});
      }
    }
    // Snare rise with an audible foundation. A short rise returns to the groove;
    // only the rise at the end of the 64-bar cycle is followed by the fill and drop.
    if (section === 'Build'&&!(removed&(1<<2))&&!isMuted(2)&&volume(2)>0) for (let x=1;x<=width;x++) {
      if(generation%2===1&&x===width)continue;
      const position=(generation%2)*width+x-1,total=2*width;
      if (x % (position>total*3/4?1:position>total/2?2:4) !== 0) continue;
      let row=!performance[offset+x][3].channel||performance[offset+x][3].channel===2?3:performance[offset+x].findIndex((n,y)=>y>0&&!n.channel);
      if(row>0)Object.assign(performance[offset+x][row],emptyNote(),{channel:2,velocity:58+Math.floor(position*56/total),probability:100});
    }

    for(const ch of held)for(let x=0;x<width;x++)for(let y=1;y<=rows;y++){
      const n=previous[x][y];if(n.channel!==ch)continue;
      let row=!performance[offset+x+1][y].channel?y:performance[offset+x+1].findIndex((cell,r)=>r>0&&!cell.channel);
      if(row>0)Object.assign(performance[offset+x+1][row],n,{midiPitch:n.midiPitch<=127?n.midiPitch:neutralGridMidi(ch,y)});
    }
    previousSection=section;builtRevision=revision;

  }
  function start(glob) {
    if (source) return state();
    saved = { glob: {...glob}, notes: note.map(col=>col.map(n=>({...n}))) };
    performanceView=true;motifPage=glob.edit;performancePages=[1,2];
    source=motif();fingerprint=fingerprints();revision=0;removed=0;removedByPage.clear();muteSignature=manualSignature();
    sourceRegisters();
    shifts.fill(0);previousSection=null;builtRevision=-1;
    seed = 1 + Math.floor(Math.random() * 255);
    const harmony=inferHarmony(source);
    key=harmony.root;scale=harmony.minor?minorScale:major;
    generation=0;pass=0;page=performancePages[0];stepInPage=0;
    build(); return state();
  }
  function setMotion(level) {
    const next = Math.max(MOTION.HOLD, Math.min(MOTION.LIVE, level | 0));
    if (next === motion) return state();
    motion = next;
    if (source) { revision++; build(); }
    return state();
  }
  function setScale(level) {
    const next = Math.max(0, Math.min(6, level | 0));
    if (next === scaleChoice) return state();
    scaleChoice = next;
    if (source) { revision++; build(); }
    return state();
  }
  function setDensity(level) {
    const next = Math.max(0, Math.min(3, level | 0));
    if (next === density) return state();
    density = next;
    if (source) { revision++; build(); }
    return state();
  }
  function advance() {
    if (!source) return null;
    poll();stepInPage++;
    if(stepInPage>=columns()) {
      stepInPage=0;pass++;
      if(pass>=repeats){pass=0;generation++;page=performancePages[page===performancePages[0]?1:0];build();}
    }
    return (page-1)*columns()+stepInPage+1;
  }
  function stop(glob) {
    if(!source)return;
    for(let x=1;x<saved.notes.length;x++)for(let y=1;y<=rows;y++)Object.assign(note[x][y],saved.notes[x][y]);
    Object.assign(glob,saved.glob);source=null;saved=null;
  }
  return { start, stop, advance, state, poll, performance, setMotion, setScale, setDensity,
    toggleView(){if(source)performanceView=!performanceView;return state();},
    selectMotif(page) {
      if(!source||performanceView||page===motifPage)return;
      removedByPage.set(motifPage,removed);motifPage=page;removed=removedByPage.get(page)||0;
      source=motif();fingerprint=fingerprints();sourceRegisters();revision++;
    }, original:()=>saved?.notes,
    volume(ch) {
      const bass=ch===6||ch===14||(ch===11&&lowCh11);
      const progress=((generation%2)*repeats*columns()+pass*columns()+stepInPage)/Math.max(1,2*repeats*columns()-1);
      if(isMuted(ch))return 0;
      let level=performanceView?volumeScale(section,ch,bass,progress):16;
      if(ch===foundation&&['Break','Build','Fill'].includes(section))level=Math.max(12,level);
      return source ? level/16 : 1;
    } };
}
