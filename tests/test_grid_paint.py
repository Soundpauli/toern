"""Verify the real C++/JS paint policy with the same audio-shadow scenarios."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
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
cpp+='}'
with tempfile.TemporaryDirectory(prefix='toern-paint-') as directory:
    p=Path(directory);(p/'paint.cpp').write_text(cpp)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,'-I',str(root),str(p/'paint.cpp'),'-o',str(p/'paint')],check=True)
    subprocess.run([str(p/'paint')],check=True)
js='''import assert from 'node:assert/strict';
import {canReplaceShadowed} from './standalone-tools/web-standalone/src/firmware/paint.js';
const cases=CASES;
for(const [name,ch,upper,y,step,prob,cond,lowercond,brush,expected] of cases){
 const grid=Array.from({length:257},()=>Array.from({length:17},()=>({channel:0,probability:100,condition:1})));
 grid[1][3]={channel:ch,probability:100,condition:lowercond};
 grid[step][y]={channel:upper,probability:prob,condition:cond};
 assert.equal(canReplaceShadowed(grid,1,3,brush),expected,name);
}
'''.replace('CASES',json.dumps(cases))
subprocess.run(['node','--input-type=module','-e',js],cwd=root,check=True)
print(f'PASS: {len(cases)} firmware/SIM shadow-paint cases.')
