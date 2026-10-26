"""Run the actual wide mixer/limiter and HCUT routing against adversarial sums."""
from pathlib import Path
import subprocess
import re
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
filter_source=(root/'toern_filter.ino').read_text()
filters=filter_source[filter_source.index('#define NUM_MIXER_CHANNELS'):filter_source.index('// ch13/14 CUT')]
sketch=(root/'toern.ino').read_text()
defines='\n'.join(re.findall(r'^#define (?:MIX_BUS_HEADROOM|MIX_END_SAMPLES_GAIN|MIX_END_SYNTH_GAIN|MIX_GAIN_UNITY|MIX_GAIN_MAX) .+$',sketch,re.M))
a=sketch.index('static inline uint8_t clampMixGain(')
b=sketch.index('\n}',sketch.index('FLASHMEM void applyMixBusGains()',a))+2
bus_code=defines+'\n#define FLASHMEM\nAudioMixer4Limited mixer1,mixer2,mixer_end;\nuint8_t mixGainMaster=10,mixGain14=10,mixGain58=10,mixGainSynth=10;\n'+sketch[a:b]
# Verify the firmware graph really connects all eight sample dry taps.
graph=(root/'audioinit.h').read_text()
for ch in range(1,9): assert f'patchSampleDry{ch}(amp{ch}, 0, filtermixer{ch}, 3)' in graph

stub=r'''
#pragma once
#include <cstdint>
#include <vector>
#define AUDIO_BLOCK_SAMPLES 128
struct audio_block_t { int16_t data[AUDIO_BLOCK_SAMPLES]={}; };
class AudioStream {
public:
 AudioStream(int,audio_block_t**) {}
 virtual void update()=0;
 audio_block_t *inputs[4]={};
 std::vector<int16_t> output;
 static int alive;
 static audio_block_t *allocate(){++alive;return new audio_block_t;}
 static void release(audio_block_t *p){if(p){--alive;delete p;}}
 void inject(int ch,int value){auto *p=allocate();for(auto &v:p->data)v=value;inputs[ch]=p;}
protected:
 audio_block_t *receiveReadOnly(int ch){auto *p=inputs[ch];inputs[ch]=nullptr;return p;}
 void transmit(audio_block_t *p){output.assign(p->data,p->data+AUDIO_BLOCK_SAMPLES);}
};
'''
harness=r'''
#include <cassert>
#include <cmath>
#include <algorithm>
#include "src/mixer_limited.h"
int AudioStream::alive=0;
BUS_CODE

struct Filter {float hz=0;void frequency(float value){hz=value;}};
struct Gain {float values[4]={};void gain(int ch,float value){assert(ch<4);values[ch]=value;}};
constexpr int NUM_FILTERS=15,maxfilterResolution=32,HCUT=0,LOWCUT=1;
struct {float filter_settings[15][2]={};} SMP;
Filter fs[15];Filter *filters[15];Gain gs[15];Gain *filtermixers[15];
const int ALL_CHANNELS[]={1,2,3,4,5,6,7,8,11,13,14};const int NUM_ALL_CHANNELS=11;
#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
FILTERS
int peak(const AudioStream &s){int v=0;for(auto x:s.output)v=std::max(v,std::abs((int)x));return v;}
void wire(AudioStream &from,AudioStream &to,int ch){auto *b=AudioStream::allocate();for(int i=0;i<128;++i)b->data[i]=from.output[i];to.inputs[ch]=b;}
int main(){
 applyMixBusGains();
 for(int n=0;n<4;++n){
  mixer1.inject(0,30000);mixer1.update();
  auto *block=AudioStream::allocate();std::copy(mixer1.output.begin(),mixer1.output.end(),block->data);
  mixer_end.inputs[0]=block;mixer_end.update();
 }
 for(auto x:mixer_end.output)assert(x==30000); // actual gain=1.0 path is unity
 assert(std::abs(MIX_END_SYNTH_GAIN/MIX_END_SAMPLES_GAIN-3.6f)<.0001f);

 // One source at unity is unchanged after exactly 16 samples of lookahead.
 AudioMixer4Limited one;
 one.inject(0,30000);one.update();
 for(int i=0;i<128;++i)assert(one.output[i]==(i<16?0:30000));
 one.inject(0,30000);one.update();for(auto x:one.output)assert(x==30000);
 // Sum cancellation must happen BEFORE limiting, not clip each input to int16.
 AudioMixer4Limited cancel;cancel.gain(0,4);cancel.gain(1,4);
 for(int n=0;n<3;++n){cancel.inject(0,30000);cancel.inject(1,-30000);cancel.update();assert(peak(cancel)==0);}
 // Full banks, both polarities, gain up to 4x, sustained overload and recovery.
 for(int sign:{-1,1})for(int channels:{1,2,3,4}){
  AudioMixer4Limited m;for(int ch=0;ch<4;++ch)m.gain(ch,4);
  for(int n=0;n<80;++n){for(int ch=0;ch<channels;++ch)m.inject(ch,sign*32767);m.update();assert(peak(m)<=32701);if(n>2)assert(peak(m)>32600);}
  for(int n=0;n<200;++n){m.inject(0,1000);m.update();assert(peak(m)<=32701);}
  assert(std::abs(m.output.back()-4000)<=1);
 }
 // Eight samples + synth + input monitoring + full-scale preview, through
 // the actual three-stage bank -> master -> output graph (worst-case in-phase).
 for(int sign:{-1,1}) {
  AudioMixer4Limited bankA,bankB,synth,master,output;
  master.gain(2,MIX_END_SYNTH_GAIN);
  for(int n=0;n<80;++n){
   for(int ch=0;ch<4;++ch){bankA.inject(ch,sign*32767);bankB.inject(ch,sign*32767);}
   for(int ch=0;ch<3;++ch)synth.inject(ch,sign*32767);
   bankA.update();bankB.update();synth.update();
   wire(bankA,master,0);wire(bankB,master,1);wire(synth,master,2);master.inject(3,sign*32767);master.update();
   wire(master,output,0);output.inject(1,sign*32767);output.update();
   for(auto *m:{&bankA,&bankB,&synth,&master,&output})assert(peak(*m)<=32701);
  }
 }
 // Same constant source: gain is continuous at boundaries around a peak.
 toern_audio::MixLimiter<128> limiter;float incoming[128]={};int16_t out[128];
 for(auto &v:incoming)v=10000;
 limiter.process(incoming,out);limiter.process(incoming,out);
 for(auto &v:incoming)v=100000;
 limiter.process(incoming,out);const float before=(float)out[127]/10000;
 limiter.process(incoming,out);const float after=(float)out[0]/100000;
 assert(std::abs(before-after)<.00011f);
 assert(AudioStream::alive==0);
 for(int ch:ALL_CHANNELS){filters[ch]=&fs[ch];filtermixers[ch]=&gs[ch];SMP.filter_settings[ch][HCUT]=32;initFilterTransition(ch);}
 for(int ch=1;ch<=8;++ch){
  assert(gs[ch].values[0]==0&&gs[ch].values[3]==1);
  applyVoiceCuts(ch);for(int t=0;t<40;++t)updateMixerGains(ch);
  assert(gs[ch].values[0]==0&&gs[ch].values[3]==1);
  SMP.filter_settings[ch][HCUT]=31;applyVoiceCuts(ch);
  for(int t=0;t<40;++t){updateMixerGains(ch);float sum=0;for(float g:gs[ch].values)sum+=g;assert(std::abs(sum-1)<.00001f);}
  assert(gs[ch].values[0]==1&&gs[ch].values[3]==0);
  SMP.filter_settings[ch][HCUT]=32;SMP.filter_settings[ch][LOWCUT]=8;applyVoiceCuts(ch);
  for(int t=0;t<40;++t)updateMixerGains(ch);
  assert(gs[ch].values[2]==1&&gs[ch].values[3]==0);
 }
 // Synth cutoff semantics stay on their existing filter path.
 for(int ch:{11,13,14}){applyVoiceCuts(ch);for(int t=0;t<40;++t)updateMixerGains(ch);assert(gs[ch].values[0]==1&&gs[ch].values[3]==0);}
}
'''.replace('\nFILTERS\n','\n'+filters+'\n').replace('\nBUS_CODE\n','\n'+bus_code+'\n')
with tempfile.TemporaryDirectory(prefix='toern-gain-') as directory:
 p=Path(directory);(p/'AudioStream.h').write_text(stub);(p/'test.cpp').write_text(harness)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,'-I',str(p),'-I',str(root),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: actual mixer unity, signed sums, cancellation, four-per-bank/eight samples+synth+monitor+preview, lookahead continuity/recovery, and HCUT dry/LP/HP transitions.')
