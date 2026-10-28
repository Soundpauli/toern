"""Check light Treeverb signal behavior and the full-mode reference/lifecycle."""
from pathlib import Path
import ast
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
# Reuse the existing host AudioStream boundary; both actual DSP implementations
# run unchanged (only the hardware SSAT has an equivalent scalar host fallback).
module = ast.parse((root/'tests/test_pause_effects.py').read_text())
stubs = {node.targets[0].id: ast.literal_eval(node.value) for node in module.body
         if isinstance(node,ast.Assign) and isinstance(node.value,ast.Constant)
         and isinstance(node.targets[0],ast.Name) and node.targets[0].id in ('arduino','audio')}
arduino = stubs['arduino']
audio = stubs['audio'].replace('return new audio_block_t{};', 'return failAllocation ? nullptr : new audio_block_t{};')
audio = audio.replace('std::vector<audio_block_t> output;', 'inline static bool failAllocation=false;\n std::vector<audio_block_t> output;')
code = r"""
#include "effect_treeverb.h"
#include "effect_freeverb_dmabuf.h"
#include <cassert>
#include <cstdio>
#include <limits>
uint32_t rng=1;
int16_t randomSample(){rng=rng*1664525u+1013904223u;return (int16_t)(rng>>16);}
#if !TOERN_TREEVERB_LIGHT
void compare(float room,float damp){
 AudioEffectTreeverb tree;AudioEffectFreeverbDMAMEM old;
 tree.roomsize(room);old.roomsize(room);tree.damping(damp);old.damping(damp);
 int16_t pcm[128];
 for(int block=0;block<1400;++block){
  if(block==700){tree.roomsize(.31f);old.roomsize(.31f);tree.damping(.18f);old.damping(.18f);}
  for(int i=0;i<128;++i){
   pcm[i]=block<200?randomSample():block<250?32767:block<300?-32768:0;
  }
  if(block<900){tree.feedSamples(pcm);old.feedSamples(pcm);}
  tree.update();old.update();assert(tree.output.size()==old.output.size());
  assert(!tree.output.empty());
  for(int i=0;i<128;++i)if(tree.output.back().data[i]!=old.output.back().data[i]){
   fprintf(stderr,"Mismatch room=%g damp=%g block=%d sample=%d: %d vs %d\n",room,damp,block,i,tree.output.back().data[i],old.output.back().data[i]);assert(false);}
  tree.output.clear();old.output.clear();
 }
}
#else
void checkLight(float room,float damp){
 AudioEffectTreeverb tree;AudioEffectFreeverbDMAMEM old;
 static_assert(sizeof(tree)<15000,"Light mode must use the smaller tank allocation");
 tree.roomsize(room);old.roomsize(room);tree.damping(damp);old.damping(damp);
 int16_t pcm[128]={};
 double energy=0,reference=0,early=0,late=0;int first=-1;bool laterTail=false;
 for(int block=0;block<1200;++block){
  std::fill(pcm,pcm+128,0);if(block==0)pcm[0]=24000;
  if(block<200){tree.feedSamples(pcm);old.feedSamples(pcm);}
  tree.update();old.update();assert(tree.output.size()==1&&old.output.size()==1);
  for(int i=0;i<128;++i){
   const int v=tree.output.back().data[i],r=old.output.back().data[i];
   energy+=(double)v*v;reference+=(double)r*r;
   if(block<400)early+=(double)v*v;
   if(block>=800)late+=(double)v*v;
   if(v&&first<0)first=block*128+i;
   if(block>170&&v)laterTail=true;
  }
  tree.output.clear();old.output.clear();
 }
 assert(first==1116);assert(energy>0&&reference>0&&late<early);
 const double ratio=energy/reference;
 assert(ratio>.8&&ratio<1.25); // keep integrated wet energy within about +/-1 dB
 if(room==1.f)assert(laterTail); // high room must not become a gated short echo
 printf("light room=%.3f damp=%.2f tail energy/reference=%.3f\n",room,damp,ratio);
 // Bounded arithmetic at both full-scale polarities and with changing controls.
 for(int block=0;block<600;++block){
  if(block==300){tree.roomsize(.31f);tree.damping(.18f);}
  for(auto &v:pcm)v=block<200?randomSample():block<400?32767:-32768;
  tree.feedSamples(pcm);tree.update();assert(tree.output.size()==1);tree.output.clear();
 }
}
#endif
int main(){
 for(float room:{1.f/32,.25f,.72f,1.f})for(float damp:{.01f,.25f,.4f,1.f}){
#if TOERN_TREEVERB_LIGHT
  checkLight(room,damp);
#else
  compare(room,damp);
#endif
 }
 AudioEffectTreeverb tree;
 tree.feed(12000);tree.update();assert(tree.output.empty()); // default bypass consumes its input
 tree.roomsize(1);tree.damping(.4f);
 for(int i=0;i<60;++i){tree.feed(i<8?10000:0);tree.update();}
 bool tail=false;
 for(auto &b:tree.output)for(auto v:b.data)tail|=v!=0;
 assert(tail);tree.output.clear();
 tree.roomsize(0);tree.update();assert(tree.output.empty());
 tree.roomsize(1);
 for(int i=0;i<100;++i){tree.update();for(auto &b:tree.output)for(auto v:b.data)assert(v==0);tree.output.clear();}
 // Queue consumption during pool exhaustion, followed by a clean recovery.
 AudioStream::failAllocation=true;tree.feed(1000);tree.update();assert(tree.output.empty());
 AudioStream::failAllocation=false;tree.feed(0);tree.update();assert(tree.output.size()==1);tree.output.clear();
 // Zero damping must represent +1.0 (32768) rather than wrapping negative.
 // Exercise zero, non-finite controls and both overload polarities under UBSan.
 tree.damping(0);tree.roomsize(1);
 for(int i=0;i<500;++i){tree.feed(i%2?32767:-32768);tree.update();tree.output.clear();}
 tree.damping(std::numeric_limits<float>::quiet_NaN());
 tree.roomsize(std::numeric_limits<float>::quiet_NaN());tree.update();assert(tree.output.empty());
 irqEnabled[0]=false;irqEnabled[1]=false;tree.roomsize(.2f);tree.damping(.2f);
 assert(!irqEnabled[0]&&!irqEnabled[1]);
 printf("PASS: mode %d, 16 room/damping combinations, noise, overload, silence/tails, live controls, bypass reset, pool exhaustion and IRQ restoration.\n",TOERN_TREEVERB_LIGHT);
}
"""
with tempfile.TemporaryDirectory(prefix='toern-treeverb-') as tmp:
 tmp=Path(tmp);(tmp/'Arduino.h').write_text(arduino);(tmp/'AudioStream.h').write_text(audio)
 (tmp/'arm_math.h').write_text('');(tmp/'utility').mkdir();(tmp/'utility/dspinst.h').write_text('')
 (tmp/'test.cpp').write_text(code)
 flags=['c++','-std=c++17','-O2','-fsanitize=address,undefined','-D__ARM_ARCH_7EM__','-I',str(tmp),'-I',str(root/'src')]
 if sys.platform=='darwin':flags+=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1']
 for light in (0,1):
  exe=tmp/f'test{light}'
  subprocess.run(flags+[f'-DTOERN_TREEVERB_LIGHT={light}',str(tmp/'test.cpp'),str(root/'src/effect_treeverb.cpp'),str(root/'src/effect_freeverb_dmabuf.cpp'),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)

# Integration: all seven existing send/return paths use Treeverb, without
# changing sample readers/envelopes or accidentally compiling the old DSP.
import re
sketch=(root/'toern.ino').read_text()
graph=(root/'audioinit.h').read_text()
config=(root/'platformio.ini').read_text()
assert set(re.findall(r'AudioEffectTreeverb\s+(freeverb\d+);',graph)) == {
 'freeverb1','freeverb2','freeverb5','freeverb6','freeverb7','freeverb8','freeverb11'}
assert 'AudioEffectTreeverb *freeverbs[15]' in sketch
assert '#include "src/effect_treeverb.h"' in sketch
assert 'AudioEffectFreeverbDMAMEM' not in graph+sketch
assert '+<src/effect_treeverb.cpp>' in config
assert '+<src/effect_freeverb_dmabuf.cpp>' not in config
assert '-D TOERN_TREEVERB_LIGHT=1' in config
print('PASS: seven Treeverb paths, voice lookup, DSP source selection and default light build flag.')
