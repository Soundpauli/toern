"""Full WAV playback must retain final frames, silence and quiet tail detail."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = (Path(__file__).resolve().parents[2] / 'platformio')
code = r'''
#include "src/toern_sample_window.h"
#include <cassert>
#include <vector>
int main(){
 for(uint32_t frames:{1u,2u,3u,7u,9410u,11296u,53233u,529199u}){
  for(unsigned end:{0u,100u}){
   auto w=toern_sample::window(frames,0,end);
   assert(w.first==0&&w.end==frames);
   std::vector<int16_t> pcm(frames,0);
   pcm.back()=-1; // Quiet detail at the very last frame must survive.
   std::vector<int16_t> loaded(pcm.begin()+w.first,pcm.begin()+w.end);
   assert(loaded==pcm&&loaded.back()==-1);
  }
  for(unsigned start=0;start<100;++start)for(unsigned end=start+1;end<=100;++end){
   auto w=toern_sample::window(frames,start,end);
   assert(w.first==uint64_t(frames)*start/100);
   assert(w.first<w.end&&w.end<=frames);
  }
 }
 auto w=toern_sample::window(10000,25,75);assert(w.first==2500&&w.end==7500);
 w=toern_sample::window(0,0,100);assert(w.first==0&&w.end==0);
 w=toern_sample::window(100,500,500);assert(w.first==99&&w.end==100);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    source, binary = Path(tmp)/'test.cpp', Path(tmp)/'test'
    source.write_text(code)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
sample=(root/'toern_sample.ino').read_text()
assert sample.count('toern_sample::window(')==3
assert 'seekEndPct - 1' not in sample and 'seekEndPct -= 1' not in sample
print('PASS: full frame counts, last-frame quiet PCM, explicit trims and all three loader paths')
