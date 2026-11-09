// Exercise real simulator generation with every voice as the only allowed target.
import assert from 'node:assert/strict';
import { drawRandomsMultiPage } from '../../tools/sim/src/firmware/random.js';
import { emptyFilt, emptyNote, setLayout } from '../../tools/sim/src/firmware/const.js';
const channels=[1,2,3,4,5,6,7,8,11,13,14];
const random=Math.random;
let seed=1;
Math.random=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;};
try {
  for(const width of [16,32]) {
    setLayout(width);
    const makeNote=()=>Array.from({length:257},()=>Array.from({length:17},emptyNote));
    const filt=Array.from({length:16},(_,ch)=>emptyFilt(ch));
    for(const allowed of channels) {
      let generated=0;
      for(let trial=1;trial<=80;trial++) {
        seed=trial;const note=makeNote();
        drawRandomsMultiPage({note,filt,page:2,loopLength:0,bpm:120,isMuted:ch=>ch!==allowed});
        for(let x=1;x<=256;x++)for(let y=1;y<=16;y++)if(note[x][y].channel) {
          assert(x>width&&x<=width*2,'other pages are untouched');
          assert.equal(note[x][y].channel,allowed,'only the sole unmuted voice gets notes');generated++;
        }
      }
      assert(generated>0,`unmuted voice ${allowed} is still eligible on ${width} columns`);
    }
    for(const allMuted of [false,true])for(let trial=1;trial<=80;trial++) {
      seed=trial;const note=makeNote();
      Object.assign(note[width+1][6],{channel:6,velocity:71,probability:33,condition:7,midiPitch:43});
      Object.assign(note[width+5][13],{channel:13,velocity:81,probability:62,condition:3,midiPitch:69});
      Object.assign(note[1][3],{channel:2,velocity:79,probability:25,condition:8,midiPitch:36});
      const before=structuredClone(note),muted=new Set(allMuted?channels:[1,3,6,14]);
      drawRandomsMultiPage({note,filt,page:2,loopLength:0,bpm:120,isMuted:ch=>muted.has(ch)});
      for(let x=1;x<=256;x++)for(let y=1;y<=16;y++) {
        const old=before[x][y],now=note[x][y];
        if(x<=width||x>width*2||old.channel)assert.deepEqual(now,old);
        if(!old.channel&&now.channel) {
          assert(!muted.has(now.channel),'muted voices never receive new notes');
          assert(![6,13].includes(now.channel),'populated voices are preserved as a whole');
        }
      }
      if(allMuted)assert.deepEqual(note,before,'all-muted generation is a no-op');
    }
  }
} finally { Math.random=random;setLayout(32); }
console.log('PASS: SIM generation respects all 11 voice mutes, preserves occupied voices/metadata, and leaves all-muted/other pages unchanged on both layouts.');
