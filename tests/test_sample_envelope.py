"""Verify actual sample ADSR, neutral transparency, retriggers and release timing."""
from pathlib import Path
import subprocess
import sys
import tempfile
import wave
root=Path(__file__).resolve().parents[1]
code=r'''
#include <cassert>
#include <cmath>
#include <fstream>
#include "src/toern_sample_envelope.h"
using toern_audio::SampleEnvelopeState;
int main(int argc,char **argv){
 SampleEnvelopeState e;
 e.attack(0);e.decay(1000);e.sustain(1);e.release(0);e.noteOn();
 for(int i=-32768;i<=32767;++i)assert(e.render(i)==i);
 // Natural silence leaves no stale attack/forced-release on the next hit.
 for(int n=0;n<30;++n){e.noteOn();for(int i=-32768;i<=32767;i+=251)assert(e.render(i)==i);}
 if(argc>1){
  std::ifstream f(argv[1],std::ios::binary);int16_t sample;
  for(int n=0;n<12;++n){f.clear();f.seekg(0);e.captureTail(nullptr,0);e.noteOn();while(f.read((char*)&sample,2))assert(e.render(sample)==sample);}
 }
 e.noteOff();assert(!e.isActive());assert(e.render(32767)==0);
 // Old reader decays independently; new attack is already at full level.
 e.noteOn();int16_t old[64];for(auto &s:old)s=1000;
 e.captureTail(old,64);e.noteOn();assert(e.render(2000)==3000);
 for(int i=1;i<64;++i){int out=e.render(2000);assert(out>=2000&&out<=3000);}
 assert(e.render(2000)==2000);
 // Repeated retriggers retain the previous tail and converge to the new note.
 e.captureTail(old,64);e.noteOn();e.render(2000);
 e.captureTail(old,64);e.noteOn();assert(e.render(2000)>3000);
 for(int i=0;i<64;++i)e.render(2000);assert(e.render(2000)==2000);
 for(auto &s:old)s=32767;e.captureTail(old,64);e.noteOn();
 assert(e.render(32767)==32767); // Only old tail gives up headroom.
 // Exact 10 ms attack, 20 ms decay to 0.5, sustain, then 100 ms release.
 SampleEnvelopeState shaped(1000);
 shaped.attack(10);shaped.decay(20);shaped.sustain(.5);shaped.release(100);shaped.noteOn();
 assert(shaped.render(10000)==0);
 for(int i=1;i<10;++i)assert(std::abs(shaped.render(10000)-i*1000)<=1);
 assert(shaped.render(10000)==10000);
 for(int i=1;i<20;++i)assert(std::abs(shaped.render(10000)-(10000-i*250))<=1);
 for(int i=0;i<100;++i)assert(shaped.render(10000)==5000);
 shaped.noteOff();shaped.noteOff();
 for(int i=0;i<100;++i){assert(shaped.isActive());assert(std::abs(shaped.render(10000)-(5000-i*50))<=1);}
 assert(!shaped.isActive());assert(shaped.render(10000)==0);
 // Live sustain changes reach the new value without a step discontinuity.
 SampleEnvelopeState live;live.noteOn();assert(live.render(10000)==10000);
 live.sustain(.25f);assert(live.render(10000)==10000);
 for(int i=1;i<64;++i)live.render(10000);assert(live.render(10000)==2500);
 // A sustain change during decay changes its target as well.
 shaped.noteOn();for(int i=0;i<15;++i)shaped.render(10000);shaped.sustain(.25f);
 for(int i=0;i<15;++i)shaped.render(10000);assert(shaped.render(10000)==2500);
 shaped.sustain(.5f);for(int i=0;i<64;++i)shaped.render(10000);
 // Note Off during attack releases from its current level, not full scale.
 shaped.noteOn();for(int i=0;i<5;++i)shaped.render(10000);shaped.noteOff();assert(shaped.render(10000)==5000);
 for(int i=1;i<100;++i)shaped.render(0);assert(!shaped.isActive());
}
'''
with tempfile.TemporaryDirectory(prefix='toern-envelope-') as directory:
 p=Path(directory);(p/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17','-O2',*flags,'-I',str(root),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 args=[str(p/'test')]
 if len(sys.argv)>1:
  with wave.open(sys.argv[1]) as wav:
   assert wav.getsampwidth()==2 and wav.getnchannels()==1
   (p/'original.pcm').write_bytes(wav.readframes(wav.getnframes()))
  args.append(str(p/'original.pcm'))
 subprocess.run(args,check=True)
print('PASS: neutral sample PCM is unchanged, old-tail-only retriggers, rapid retriggers, headroom, ADSR timing and configured release including release during attack.')
