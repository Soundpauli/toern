"""AGEN and motif notes must share a pitch domain, irrespective of patch tuning."""
from pathlib import Path
import subprocess,tempfile,sys
root=Path(__file__).resolve().parents[2]/'platformio'
s=(root/'toern_helpers.ino').read_text();a=s.rindex('FLASHMEM static int autogenReferenceMidi(');b=s.index('{',a)+1;depth=1
while depth:
 depth+=(s[b]=='{')-(s[b]=='}');b+=1
code=r'''
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include "src/toern_midi_pitch.h"
#include "src/toern_autogen_register.h"
#define FLASHMEM
#define constrain(v,a,b) std::clamp((v),(a),(b))
struct Note{uint8_t midiPitch=255;};
FUNCTION
int main(){
 for(int ch=1;ch<=8;ch++)for(int row=1;row<=16;row++)for(int root=0;root<=96;root+=12){
  int raw=autogenReferenceMidi(Note{},ch,row);
  assert(toern_midi_pitch::sampleNote(root,raw)==root+row-ch-1);
 }
 for(int row=1;row<=16;row++)for(int oct=1;oct<=7;oct++)for(int transpose=-12;transpose<=12;transpose++){
  int raw=autogenReferenceMidi(Note{},11,row);
  assert(toern_midi_pitch::polyNote(raw,transpose,oct)==12*oct+transpose+row-12);
 }
 for(int ch:{13,14})for(int row=1;row<=16;row++)for(int cent=0;cent<=32;cent++)for(int oct=0;oct<=32;oct++){
  int raw=autogenReferenceMidi(Note{},ch,row),index=std::clamp(48+row-ch-1,36,62);
  double patch=cent*1.5-24+12*int(oct*6.0/32-3);
  assert(raw+patch==index+12+patch);
 }
 for(int reference=24;reference<=96;reference++)for(int pitch=0;pitch<=127;pitch++){
  int result=toern_autogen_register::nearSource(pitch,reference);
  assert(std::abs(result-reference)<=6&&result%12==pitch%12);
 }
 for(int reference=24;reference<=96;reference++)for(int key=0;key<12;key++)for(bool minor:{false,true})
  for(int phrase=0;phrase<32;phrase++)for(int ch:{13,14})for(int step=0;step<32;step++){
   int n=toern_autogen_register::monoPitch(reference,key,minor,phrase,ch,step,113);
   assert(std::abs(n-reference)<=6);
   bool inScale=false;for(unsigned i=0;i<7;i++)inScale|=n%12==toern_autogen_music::scalePc(key,minor,i);
   assert(inScale);
  }
 assert(toern_autogen_register::phrasePages(16)==4&&toern_autogen_register::phrasePages(32)==2);
 for(int ch:{13,14})for(int step=0;step<16;step++){
  int held=toern_autogen_register::monoPitch(60,0,false,2,ch,step,113,0);
  int soft=toern_autogen_register::monoPitch(60,0,false,2,ch,step,113,1);
  assert(held==60&&std::abs(soft-60)<=2);
 }
}
'''.replace('FUNCTION',s[a:b])
with tempfile.TemporaryDirectory(prefix='agen-register-') as t:
 p=Path(t);(p/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17','-O2',*flags,'-I',str(root),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: motif/sample root equality, CH11 octave/transpose equality, CH13/14 CENT/OCTV equality, bounded in-key motion and four-bar phrase lengths.')
