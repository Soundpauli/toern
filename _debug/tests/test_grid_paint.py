"""Verify the real C++/JS paint policy with the same audio-shadow scenarios."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile
root=(Path(__file__).resolve().parents[2] / 'platformio')
cases=[]
# name, blocked voice, upper voice, upper row/step, upper probability/condition,
# blocked condition, brush, expected
for ch in [1,2,3,4,5,6,7,8,11,13,14]:
    cases.append((f'voice {ch}',ch,ch,9,1,100,1,1,3 if ch!=3 else 4,ch!=11))
cases += [
 ('no higher note',1,0,9,1,100,1,1,2,False),
 ('other voice',1,2,9,1,100,1,1,2,False),
 ('other step',1,1,9,2,100,1,1,2,False),
 ('lower row',1,1,2,1,100,1,1,2,False),
 ('chance winner',1,1,9,1,99,1,1,2,False),
 ('conditional winner',1,1,9,1,100,2,1,2,False),
 ('matching condition',1,1,9,1,100,2,2,2,True),
 ('unconditional covers condition',1,1,9,1,100,1,4,2,True),
 ('glide winner',13,13,9,1,100,22,1,2,True),
 ('fill winner',1,1,9,1,100,21,1,2,False),
 ('fill blocker',1,1,9,1,100,1,21,2,False),
 ('same voice brush',1,1,9,1,100,1,1,1,False),
 ('empty',0,0,9,1,100,1,1,2,False),
 ('row16 winner',14,14,16,1,100,1,1,2,True),
]
# MULT (maxAudible=3): one covering note is still audible.
multi_cases=[
 ('mult one cover',1,[(9,100)],False),
 ('mult two covers',1,[(9,100),(11,100)],False),
 ('mult three covers',1,[(9,100),(11,100),(13,100)],True),
 ('mult chance not cover',1,[(9,99),(11,100),(13,100)],False),
]
cpp='''#include <cassert>
#include "src/toern_grid_paint.h"
struct Note { unsigned channel=0, probability=100, condition=1; };
int main(){
'''
for name,ch,upper,y,step,prob,cond,lowercond,brush,expected in cases:
    cpp+='''{ Note grid[257][17]{};
 grid[1][3]={%d,100,%d}; grid[%d][%d]={%d,%d,%d};
 assert(toern_grid_paint::canReplaceShadowed(grid,1,3,%d,256,16)==%s); }
'''%(ch,lowercond,step,y,upper,prob,cond,brush,str(expected).lower())
for name,ch,covers,expected in multi_cases:
    cpp+='{ Note grid[257][17]{}; grid[1][3]={%d,100,1};\n'%ch
    for y,prob in covers:
        cpp+=' grid[1][%d]={%d,%d,1};\n'%(y,ch,prob)
    cpp+=' assert(toern_grid_paint::canReplaceShadowed(grid,1,3,2,256,16,3)==%s); }\n'%str(expected).lower()
cpp+='}'
with tempfile.TemporaryDirectory(prefix='toern-paint-') as directory:
    p=Path(directory);(p/'paint.cpp').write_text(cpp)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,'-I',str(root),str(p/'paint.cpp'),'-o',str(p/'paint')],check=True)
    subprocess.run([str(p/'paint')],check=True)
js='''import assert from 'node:assert/strict';
import {canReplaceShadowed,shadowSlots} from '../tools/sim/src/firmware/paint.js';
const cases=CASES;
for(const [name,ch,upper,y,step,prob,cond,lowercond,brush,expected] of cases){
 const grid=Array.from({length:257},()=>Array.from({length:17},()=>({channel:0,probability:100,condition:1})));
 grid[1][3]={channel:ch,probability:100,condition:lowercond};
 grid[step][y]={channel:upper,probability:prob,condition:cond};
 assert.equal(canReplaceShadowed(grid,1,3,brush),expected,name);
}
const multi=MULTI;
for(const [name,ch,covers,expected] of multi){
 const grid=Array.from({length:257},()=>Array.from({length:17},()=>({channel:0,probability:100,condition:1})));
 grid[1][3]={channel:ch,probability:100,condition:1};
 for(const [y,prob] of covers) grid[1][y]={channel:ch,probability:prob,condition:1};
 assert.equal(canReplaceShadowed(grid,1,3,2,256,16,3),expected,name);
}
assert.equal(shadowSlots(1,1),3);
assert.equal(shadowSlots(1,0),1);
assert.equal(shadowSlots(13,1),1);
'''.replace('CASES',json.dumps(cases)).replace('MULTI',json.dumps(multi_cases))
js += r"""
import {drawTriggers,drawTimer} from '../tools/sim/src/firmware/screens.js';
function emptyFilt(){return {trig:1};}
for(const ch of [1,11,13,14])for(const singleMode of [false,true])for(const selected of [ch,2])for(const muted of [false,true])for(const probability of [99,100]) {
 const note=Array.from({length:257},()=>Array.from({length:17},()=>({channel:0,probability:100,condition:1})));
 note[1][3]={channel:ch,probability:100,condition:1};
 const filt=Array.from({length:16},()=>emptyFilt());
 filt[ch]={trig:0}; // MONO for sample voices so one cover still dims
 const s={note,filt,GLOB:{singleMode,currentChannel:selected,edit:1},mute:{[ch]:muted}};
 let color;
 const matrix={light(x,y,c){if(x===1&&y===3)color=c.slice();}};
 drawTriggers(matrix,s,301);const baseline=color;
 note[1][9]={channel:ch,probability,condition:1};
 drawTriggers(matrix,s,301);
 const expected=ch!==11&&probability===100?baseline.map(c=>Math.floor(c/5)):baseline;
 assert.deepEqual(color,expected,`dim ch${ch} single=${singleMode} selected=${selected} muted=${muted} probability=${probability}`);
 if(ch!==11&&probability===100) {
   Object.assign(s,{beat:1,page:1,playing:true});
   drawTimer(matrix,s);
   assert.deepEqual(color,expected,'playhead must preserve the dim shadowed note');
 }
}
// MULT: up to three same-step notes stay bright.
{
 const note=Array.from({length:257},()=>Array.from({length:17},()=>({channel:0,probability:100,condition:1})));
 const filt=Array.from({length:16},()=>({trig:1}));
 note[1][3]={channel:1,probability:100,condition:1};
 note[1][6]={channel:1,probability:100,condition:1};
 note[1][9]={channel:1,probability:100,condition:1};
 const s={note,filt,GLOB:{singleMode:false,currentChannel:1,edit:1},mute:{}};
 const lit={};
 const matrix={light(x,y,c){lit[`${x},${y}`]=c.slice();}};
 drawTriggers(matrix,s,301);
 for(const y of [3,6,9]) assert.deepEqual(lit[`1,${y}`],[255,0,0],'MULT top-3 stay bright');
 note[1][12]={channel:1,probability:100,condition:1};
 drawTriggers(matrix,s,301);
 assert.deepEqual(lit['1,3'],[51,0,0],'fourth MULT note dims');
 assert.deepEqual(lit['1,6'],[255,0,0],'MULT 2nd stays bright');
 assert.deepEqual(lit['1,9'],[255,0,0],'MULT 3rd stays bright');
 assert.deepEqual(lit['1,12'],[255,0,0],'highest MULT stays bright');
}
console.log('PASS: SINGLE and DRAW shadowed notes use 20% brightness; audible and polyphonic notes are unchanged.');
"""
subprocess.run(['node','--input-type=module','-e',js],cwd=root,check=True)
print(f'PASS: {len(cases)+len(multi_cases)} firmware/SIM shadow-paint cases, including polyphony, probability, conditions and fills.')
