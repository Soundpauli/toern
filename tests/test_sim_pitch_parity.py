"""Compare SIM oscillator tuning to compiled current firmware calculations."""
from pathlib import Path
import json, re, subprocess, sys, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'toern.ino').read_text()
a=source.index('  float frequency;',source.index('static void playSynthInternal'))
b=source.index('  float mappedSustain',a)
mono=source[a:b]
poly=(root/'toern_synths.ino').read_text()
cents=[int(x) for x in re.findall(r'cents\[ch\]\[0\] = (-?\d+);',poly)]
assert len(cents)==13
js=r'''
import {synthFrequencies} from './standalone-tools/web-standalone/src/firmware/synthPitch.js';
const cases=[];
function add(ch,row,midi,cent,dtne,oct,semi=16,inst=12){cases.push([ch,row,midi,cent,dtne,oct,semi,inst,...synthFrequencies(ch,row,{midiPitch:midi,cent,detune:(dtne-16)/16,octaveRaw:oct,semi,inst})]);}
for(const ch of [13,14]) {
 for(let midi=0;midi<=127;midi++)add(ch,3,midi,ch===13?0:8,16,16);
 for(let cent=0;cent<=32;cent++)for(let oct=0;oct<=32;oct++)for(const dtne of [0,16,32])add(ch,3,60,cent,dtne,oct);
 for(let row=1;row<=15;row++)add(ch,row,-1,16,16,16);
}
for(let inst=0;inst<13;inst++)for(let semi=0;semi<=32;semi++)for(const cent of [0,5,14,18,32])add(11,3,60,cent,16,16,semi,inst);
console.log(JSON.stringify(cases));
'''
cases=json.loads(subprocess.check_output(['node','--input-type=module','-e',js],cwd=root,text=True))
harness=r'''
#include <cmath>
#include <iostream>
#include <iomanip>
#define PROGMEM
#include "notes.h"
#include "toern_midi_pitch.h"
#define constrain(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
constexpr int CENT=0,DETUNE=1,OCTAVE=2,maxfilterResolution=32;
struct {float synth_settings[16][3]={},filter_settings[16][3]={};} SMP;
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
void calc(int ch,int b,int midiPitchOverride,int vel=100){
MONO
std::cout<<base<<" "<<freq1<<"\n";
}
int main(){int ch,row,midi,cent,dtne,oct,semi,inst;double unused;
std::cout<<std::setprecision(12);
const int presetCents[13]={CENTS};
while(std::cin>>ch>>row>>midi>>cent>>dtne>>oct>>semi>>inst){
SMP.synth_settings[ch][CENT]=cent;SMP.filter_settings[ch][DETUNE]=dtne;SMP.filter_settings[ch][OCTAVE]=oct;
if(ch!=11){calc(ch,row,midi);continue;}
int reg=(int)(1.0f+cent/32.0f*7.0f);
int idx=toern_midi_pitch::polyNote(midi,0,reg)+(int)round(-2.0f+semi/32.0f*4.0f);
float freq=idx>=0&&idx<108?notesArray[idx]:NOTE_C0*powf(2.0f,idx/12.0f);
std::cout<<freq*powf(2.0f,(presetCents[inst]-50.0f+semi/32.0f*100.0f)/1200.0f)<<"\n";
}}
'''.replace('MONO',mono).replace('CENTS',','.join(map(str,cents)))
with tempfile.TemporaryDirectory(prefix='toern-sim-pitch-') as tmp:
 p=Path(tmp);(p/'pitch.cpp').write_text(harness)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,'-I',str(root),'-I',str(root/'src'),str(p/'pitch.cpp'),'-o',str(p/'pitch')],check=True)
 lines=subprocess.check_output([str(p/'pitch')],input='\n'.join(' '.join(map(str,c[:8])) for c in cases)+'\n',text=True).splitlines()
 for case,line in zip(cases,lines,strict=True):
  actual=list(map(float,line.split()));expected=case[8:]
  for a,b in zip(actual,expected,strict=True):
   assert abs(a-b)<=max(1e-5,abs(a)*2e-6),(case,a,b)
print(f'PASS: {len(cases)} SIM tuning cases match firmware C++ (mono pairs, CENT/DTNE/OCTV, MIDI extremes, grid rows, all CH11 presets/SEMI/registers).')
