import assert from 'node:assert/strict';
import {drawTriggers,drawTimer,drawCursor,drawAutogenStatus} from '../../tools/sim/src/firmware/screens.js';
import {emptyNote,setLayout} from '../../tools/sim/src/firmware/const.js';
for(const width of [16,32]){
 setLayout(width);
 const grid=()=>Array.from({length:257},()=>Array.from({length:17},emptyNote));
 const note=grid(),performance=grid();
 Object.assign(note[4*width+7][14],{channel:13});
 Object.assign(performance[2][3],{channel:2});Object.assign(performance[5][7],{channel:6});
 Object.assign(performance[width+9][11],{channel:11});
 const s={note,agenPerformance:performance,agen:{active:true,performanceView:true,page:1,progress:6,ready:true},
  GLOB:{edit:5,x:4*width+7,y:14,singleMode:true,currentChannel:13},mode:'single',simpleNotes:true,
  mute:Array(16).fill(false),playing:true,beat:4,voiceMode:false,fireVoice:0};
 const saved=structuredClone(s.GLOB),pixels=new Map(),matrix={light:(x,y,c)=>pixels.set(`${x},${y}`,c),setCursor:(x,y)=>pixels.set(`${x},${y}`,'cursor')};
 drawTriggers(matrix,s,0);
 assert(pixels.has('2,3')&&pixels.has('5,7'),'AGEN shows all voices at their generated grid positions');
 assert(!pixels.has('7,14'),'the motif does not leak into the performance view');
 assert.deepEqual(s.GLOB,saved,'rendering never redirects the edit cursor');
 pixels.clear();s.agen.page=2;s.beat=width+4;drawTriggers(matrix,s,0);
 assert(pixels.has('9,11')&&!pixels.has('2,3'),'AGEN follows the newly playing buffer');
 drawTimer(matrix,s);assert.deepEqual(pixels.get('4,2'),[0,50,0],'performance timebar is green');
 pixels.clear();s.pulse=100;s.pulseDir=1;drawCursor(matrix,s,0);
 assert(pixels.has('7,14')&&pixels.size>=1,'the cursor stays visible on the performance view');
 assert.deepEqual(s.GLOB,saved,'the cursor never edits or redirects in the performance view');
 s.agen.performanceView=false;drawTriggers(matrix,s,0);
 assert(pixels.has('7,14')&&!pixels.has('9,11'),'FN motif view restores the editable source grid');
 drawTimer(matrix,s);assert.deepEqual(pixels.get('4,2'),[28,0,0],'motif keeps its red timebar');
 for(const performanceView of [false,true])for(const ready of [false,true])for(const progress of [0,6,12]){
  Object.assign(s.agen,{performanceView,ready,progress});pixels.clear();drawAutogenStatus(matrix,s,0);
  const ring=pixels.get(`${width-2},16`);
  if(performanceView)assert(ring[2]>ring[0]&&ring[2]>ring[1],'AGEN ring is blue, including late preparation');
  else assert(ring[0]>ring[1]*3&&ring[2]>ring[1]*3,'motif ring is pink');
 }
}
console.log('PASS: following generated grid, every voice in SINGLE, separate motif, green/red timebars, pink/blue clocks and untouched editor on both layouts.');
