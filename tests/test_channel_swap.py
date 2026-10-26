"""Compile the actual firmware swap function and verify complete note exchanges."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern.ino').read_text()
a = source.index('FLASHMEM void swapChannelNotes(')
b = source.index('\nvoid toggleCopyPaste()', a)
function = source[a:b]
harness = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "src/toern_midi_pitch.h"
#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))
#define FLASHMEM
constexpr unsigned MAX_STEPS=256,maxY=16;
struct Note { uint8_t channel,velocity,probability,condition,midiPitch; };
Note note[258][17],tmp[258][17],original[258][17];
struct { bool activeCopy=true; uint8_t copyChannel=11; } GLOB;
namespace toern_audio { struct StateGuard {}; }
int updates=0;
void updateLastPage(){++updates;}
FUNCTION
int main(){
 const uint8_t channels[]={0,1,2,3,4,5,6,7,8,11,13,14};
 for(unsigned x=1;x<=MAX_STEPS;++x)for(unsigned y=1;y<=maxY;++y)
  note[x][y]={channels[(x+y)%12],uint8_t(x%128),uint8_t(y%101),uint8_t(y+1),uint8_t(x%256)};
 std::memcpy(original,note,sizeof(note));
 for(int from:channels)for(int to:channels){
  std::memcpy(note,original,sizeof(note));GLOB.activeCopy=true;GLOB.copyChannel=11;
  const int before=updates;
  swapChannelNotes(from,to,0,0);
  const bool valid=from&&to&&from!=to;
  assert(updates==before+valid);
  assert(GLOB.activeCopy==!valid);
  assert(GLOB.copyChannel==(valid?0:11));
  for(unsigned x=0;x<258;++x)for(unsigned y=0;y<17;++y){
   Note expected=original[x][y];
   if(valid&&x>=1&&x<=256&&y>=1&&y<=16)
    expected.channel=expected.channel==from?to:expected.channel==to?from:expected.channel;
   assert(std::memcmp(&expected,&note[x][y],sizeof(Note))==0);
  }
  swapChannelNotes(from,to,0,0);
  assert(std::memcmp(original,note,sizeof(note))==0);
 }
 for(int invalid:{9,10,12,15,255}){swapChannelNotes(invalid,1,0,0);swapChannelNotes(1,invalid,0,0);}
 assert(std::memcmp(original,note,sizeof(note))==0);
 for(int shift:{-8,-1,0,1,8}) {
  std::memcpy(note,original,sizeof(note));
  swapChannelNotes(1,11,shift,-shift);
  for(unsigned x=1;x<=256;++x)for(unsigned y=1;y<=16;++y){
   Note expected=original[x][y];
   if(expected.channel==1||expected.channel==11){
    const int delta=expected.channel==1?shift:-shift;
    expected.channel=expected.channel==1?11:1;
    if(delta){
     int pitch=expected.midiPitch<=127?expected.midiPitch:toern_midi_pitch::gridMidi(expected.channel,y,4,0);
     expected.midiPitch=constrain(pitch+12*delta,0,127);
    }
   }
   assert(std::memcmp(&expected,&note[x][y],sizeof(Note))==0);
  }
 }
}
'''.replace('FUNCTION', function)
with tempfile.TemporaryDirectory(prefix='toern-swap-') as directory:
    path=Path(directory)
    (path/'test.cpp').write_text(harness)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,'-I',str(root),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('PASS: firmware SWAP all voice pairs, full pattern/row16, metadata, clipboard, invalid/same-channel no-op and reversibility.')
