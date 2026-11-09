import { neutralGridMidi } from "./pitch.js";
// Browser counterpart of the firmware's temporary, section-driven AGEN workspace.
// The saved bank stays immutable; an editable motif steers two separate performance pages.
// 64-bar cycle: a short rise and a short dip each return to the groove (kick and
// bass come back). The fill and drop close the cycle once, not every 16 bars.
const FORM = [['Groove',8],['Variation',8],['Build',2],['Groove',8],['Variation',8],['Break',2],
  ['Groove',8],['Groove',8],['Variation',4],['Build',2],['Fill',2],['Drop',4]];
export const SECTIONS = FORM.flatMap(([section])=>[section,section]);
const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
export function phrasePasses(section, generation, columns, bpm) {
  if(section==='Build'||section==='Fill')return 1;
  let bars = FORM[Math.floor(generation/2)%FORM.length][1];
  const limit=section==='Build'?32000:24000;
  while(bars>1 && bars*240000>Math.max(40,bpm)*limit)bars/=2;
  return Math.max(1, Math.floor(bars*16/(2*columns)));
}
const major=[0,2,4,5,7,9,11], minorScale=[0,2,3,5,7,8,10];
function chordPitchClasses(root, minor, generation) {
  const scale=minor?minorScale:major, slots=[0,1,2,3,3,0,1,2];
  const degree=(minor?[0,5,3,6]:[0,5,3,4])[slots[Math.floor(generation/2)%8]];
  return [0,2,4].map(t=>(root+scale[(degree+t)%7])%12);
}
export function melodyPitch(reference, root, minor, generation, channel, step, seed=113, motion=2) {
  const section=SECTIONS[generation%SECTIONS.length], bass=channel===6||channel===14;
  const chord=chordPitchClasses(root,minor,generation), scale=(minor?minorScale:major).map(pc=>(root+pc)%12);
  const allowed=bass&&step%4===0?[chord[0]]:step%4===0?chord:scale;
  let h=(seed ^ Math.imul(Math.floor(generation/2),374761393) ^ Math.imul(channel,668265263) ^ Math.imul(step,2246822519))>>>0;
  h=Math.imul(h^(h>>>13),1274126177)>>>0;h=(h^(h>>>16))>>>0;
  let chance=bass?50:section==='Variation'||section==='Fill'?90:section==='Build'?80:section==='Break'?65:section==='Drop'?45:40;
  if(motion<2&&chance>35)chance=35;
  let delta=[2,4,3,7,5,-2,4,-5][(Math.floor(step/2)+channel+Math.floor(generation/2))%8];
  if(generation%2)delta=-delta;
  if(bass||motion<2)delta=delta>0?2:-2;
  if(!motion)delta=0;
  if(h%100>=chance || (bass&&step%4===0))delta=0;
  let best=reference, distance=Infinity;
  for(let pitch=Math.max(0,reference-12);pitch<=Math.min(127,reference+12);pitch++) {
    const gap=Math.abs(pitch-reference-delta);
    if(allowed.includes(pitch%12)&&gap<distance){best=pitch;distance=gap;}
  }
  return best;
}

export function voicingPitches(reference,root,minor,generation,seed=113) {
  const chord=chordPitchClasses(root,minor,generation), inversion=(generation+seed)%3;
  let base=reference,distance=Infinity;
  for(let n=0;n<=127;n++)if(n%12===chord[0]&&Math.abs(n-reference)<distance){base=n;distance=Math.abs(n-reference);}
  const notes=[0,1,2].map(i=>{const tone=(i+inversion)%3;return base+(chord[tone]-chord[0]+12)%12+(tone<inversion?12:0);});
  let center=Math.trunc(notes.reduce((a,b)=>a+b,0)/3),shift=0;
  while(center+shift>reference+6)shift-=12;
  while(center+shift<reference-6)shift+=12;
  while(notes[0]+shift<24)shift+=12;
  while(notes[2]+shift>108)shift-=12;
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
  if(section==='Break')return ch===1||bass?0:ch<=5?7:12;
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
export function monoPitch(reference,root,minor,generation,ch,step,seed=113,motion=2){
 if(!motion)return reference;
 const chord=chordPitchClasses(root,minor,generation),scale=(minor?minorScale:major).map(pc=>(root+pc)%12);
 let target=reference;const h=(seed+step*977+ch*37+generation*101)>>>0;
 if(motion>=2&&SECTIONS[generation%SECTIONS.length]==='Variation'&&step%4&&h%4===0)target+=(h&4)?2:-2;
 const pcs=ch===14&&step%4===0?[chord[0]]:step%4===0?chord:scale;
 let best=reference,gap=Infinity;
 for(let n=Math.max(0,reference-12);n<=Math.min(127,reference+12);n++)if(pcs.includes(n%12)&&Math.abs(n-target)<gap){best=n;gap=Math.abs(n-target);}
 best=nearSource(best,reference);
 if(motion<2&&Math.abs(best-reference)>2)return reference;
 return best;
}
export function createAutogen({ note, columns, steps, rows, emptyNote, bpm, params=()=>({}), isMuted=()=>false, volume=()=>16 }) {
  let source = null, saved = null, generation = 0, pass = 0, page = 1, stepInPage = 0;
  let section = 'Groove', repeats = 1, lowCh11 = false;
  let foundation=0,registers=Array(16).fill(60),shifts=Array(16).fill(0);
  let key = 0, scale = [0,2,4,5,7,9,11];
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
  const state = () => ({ active: !!source, performanceView, section, generation, page, remaining: repeats-pass, ready: true,
    motifPage, motifSlot:motifPage<=8?motifPage:9, playingSlot:10+performancePages.indexOf(page), otherSlot:10+(performancePages.indexOf(page)^1), revision,
    progress: Math.min(columns()>=32?17:5,Math.floor((pass*columns()+stepInPage)*(columns()>=32?17:5)/Math.max(1,repeats*columns()-1))) });
  function build() {
    section = SECTIONS[generation % SECTIONS.length];
    foundation=[6,14,11,13,7,8,1].find(ch=>!isMuted(ch)&&volume(ch)>0&&source.some(col=>col.some(n=>n.channel===ch)))||0;
    if(!foundation&&['Break','Build','Fill'].includes(section))section='Groove';
    repeats = phrasePasses(section, generation, columns(), bpm());
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
      const ch = n.channel, perc = ch <= 5;
      if(held.has(ch)){Object.assign(n,emptyNote());continue;}
      if(isMuted(ch)||volume(ch)<=0){Object.assign(n,emptyNote());continue;}
      const reference = n.midiPitch<=127?n.midiPitch:neutralGridMidi(ch,y);
      const bass = ch===6||ch===14||(ch===11&&lowCh11);
      if (isMuted(ch) || (section === 'Build' && (ch === 1 || bass)&&ch!==foundation) ||
          (section === 'Break' && ch!==foundation && (ch === 1 || bass || (perc && x % 4 !== 1))) ||
          (section === 'Fill' && generation%2===1 && ((perc && x === width) || (bass && x >= width-1)))) {
        Object.assign(n, emptyNote()); continue;
      }
      if(section==='Build'&&generation%2===1&&x===width&&ch!==foundation){Object.assign(n,emptyNote());continue;}
      if(section==='Build'&&ch===3&&(x-1)%4!==2){Object.assign(n,emptyNote());continue;}
      if (!perc) {
        const target=reference;
        n.midiPitch=(ch===13||ch===14)?monoPitch(target,key,scale[2]===3,generation,ch,x-1):nearSource(melodyPitch(target,key,scale[2]===3,generation,bass?14:ch,x-1),target);
      }
      n.condition=0;
      n.probability = ch === 1 || bass || x % 4 === 1 ? 100 : Math.max(75,n.probability || 100);
      if (section === 'Break') n.velocity = Math.max(1, Math.round(n.velocity*.7));
      if (section === 'Fill' && perc) n.velocity = Math.min(127,n.velocity+12);
    }
    // Preserve three distinct chord tones at every source chord onset.
    if(!lowCh11&&!isMuted(11)&&volume(11)>0&&source.some(col=>col.some(n=>n.channel===11)))for(let x=1;x<=width;x++){
      const original=source[(Math.floor(generation/2)*width+x-1)%source.length];
      const chord=original.filter(n=>n.channel===11);
      if(chord.length<2)continue;
      for(const n of performance[offset+x])if(n.channel===11)Object.assign(n,emptyNote());
      const pitches=voicingPitches(registers[11],key,scale[2]===3,generation);
      for(let i=0;i<Math.min(chord.length,3);i++){
        let row=clamp(12+i,1,rows);if(performance[offset+x][row].channel)row=performance[offset+x].findIndex((n,y)=>y>0&&!n.channel);
        if(row>0)Object.assign(performance[offset+x][row],emptyNote(),{channel:11,midiPitch:encodePitch(pitches[i],shifts[11]),velocity:chord[0]?.velocity||88});
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
    const harmony=inferHarmony(source);
    key=harmony.root;scale=harmony.minor?minorScale:major;
    generation=0;pass=0;page=performancePages[0];stepInPage=0;
    build(); return state();
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
  return { start, stop, advance, state, poll, performance,
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
