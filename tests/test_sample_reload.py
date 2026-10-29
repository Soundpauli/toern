"""Run real reload/trigger functions: fade first, stop before PCM writes."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
sample=(root/'toern_sample.ino').read_text()
main=(root/'toern.ino').read_text()
def function(text,signature):
    start=text.index(signature);end=text.index('{',start)+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
harness=r'''
#include <cassert>
#include <cstdint>
#include <set>
constexpr unsigned maxFiles=9;
unsigned now=0;int guardDepth=0;bool irq=true;
constexpr int IRQ_SOFTWARE=0;
#define NVIC_IS_ENABLED(irqNumber) irq
namespace toern_audio {struct StateGuard {StateGuard(){++guardDepth;}~StateGuard(){--guardDepth;}};}
struct elapsedMicros {unsigned start=now;operator unsigned()const{return now-start;}};
volatile bool channelSampleReloadBusy[maxFiles]={};
struct Voice {
 bool playing=true;float amp=1,target=1;unsigned fadeStart=0;int stops=0;
 bool isPlaying(){return playing;}
 void setAmplitude(float v){target=v;fadeStart=now;if(!playing)amp=v;}
 void stop(){assert(guardDepth>0);if(irq&&playing)assert(amp==0);playing=false;++stops;}
} voice[9];
Voice *voices[9]={};
struct Env {void noteOff(){assert(guardDepth>0);}} env[15];
Env *envelopes[15]={};
struct Sampler {
 int ons=0,offs=0;std::set<int> keys;
 void noteEvent(uint8_t pitch,uint8_t velocity,bool on,bool){
  assert(guardDepth>0);if(on&&velocity){++ons;keys.insert(pitch);}else{++offs;keys.erase(pitch);}
 }
 void removeAllSamples(){assert(guardDepth>0);}
} _samplers[9];
struct Fade {
 unsigned start=0;int outs=0;
 void fadeOut(unsigned ms){assert(ms==3);start=now;++outs;}
} sdPreviewFade;
struct AudioPlaySdWav {
 bool playing=true;int stops=0;
 bool isPlaying(){return playing;}
 void stop(){assert(!irq||now-sdPreviewFade.start>=8000);playing=false;++stops;}
} playSdWav1;
struct AudioEffectEnvelope {void noteOff(){}} envelope0;
float playAmpForVoice(int){return .5f;}
void yield();
void delayMicroseconds(unsigned us){
 assert(guardDepth==0);now+=us;
 for(auto &v:voice)if(v.target==0&&now-v.fadeStart>=6000)v.amp=0;
}
'''
harness+='\n'.join([
    function(sample,'bool isChannelSampleReloadBusy('),
    function(main,'void samplerNoteEvent('),
    function(main,'void samplerRemoveSamples('),
    function(sample,'void beginChannelSampleReload('),
    function(sample,'void endChannelSampleReload('),
    function(sample,'void stopSdPreviewIfPlaying()'),
])
harness+=r'''
void yield(){
 assert(guardDepth==0);
 for(unsigned ch=0;ch<9;++ch)if(channelSampleReloadBusy[ch])
   samplerNoteEvent(ch,60,127,true,true); // Sequencer trigger during fade.
}
int main(){
 for(unsigned ch=0;ch<9;++ch){
  voices[ch]=&voice[ch];envelopes[ch]=&env[ch];
  _samplers[ch].keys={0,35,60,97,127};
  const unsigned start=now;
  beginChannelSampleReload(ch);
  assert(now-start==8000&&channelSampleReloadBusy[ch]);
  assert(voice[ch].stops==1&&!voice[ch].playing);
  assert(_samplers[ch].keys.empty()&&_samplers[ch].offs==128&&_samplers[ch].ons==0);
  samplerNoteEvent(ch,60,127,true,true);assert(_samplers[ch].ons==0);
  samplerNoteEvent(ch,60,0,false,false);assert(_samplers[ch].offs==129);
  endChannelSampleReload(ch);
  assert(!channelSampleReloadBusy[ch]&&voice[ch].amp==.5f);
  samplerNoteEvent(ch,60,127,true,true);assert(_samplers[ch].ons==1);
  const unsigned idleStart=now;beginChannelSampleReload(ch);assert(now==idleStart);
  endChannelSampleReload(ch);
 }
 // A caller which already masked audio must never wait for that IRQ.
 irq=false;voice[1].playing=true;const unsigned start=now;
 beginChannelSampleReload(1);assert(now==start);endChannelSampleReload(1);
 beginChannelSampleReload(99);endChannelSampleReload(99);
 irq=true;unsigned previewStart=now;stopSdPreviewIfPlaying();
 assert(now-previewStart==8000&&sdPreviewFade.outs==1&&playSdWav1.stops==1);
 previewStart=now;stopSdPreviewIfPlaying();assert(now==previewStart);
}
'''
# Both RAM-preview fill paths and sampler registration must stop the old reader.
preview=function(sample,'FLASHMEM void previewSample(bool setMaxSampleLength, bool playAudio) {')
load=function(sample,'bool loadPreviewToChannel(')
for body in (preview,load):
    assert body.index('beginChannelSampleReload(0)') < body.index('previewFile.read(&sampled[0]')
    assert body.index('endChannelSampleReload(0)') > body.index('previewFile.read(&sampled[0]')
assert 'min(LPREVIEW_CHUNK, (size_t)(maxToRead - plen))' in load
assert 'sdPreviewFade.fadeIn(0);' in function(sample,'static void startSdPreviewNow(')
assert 'patchCord216(playSdWav1, 0, sdPreviewFade, 0)' in (root/'audioinit.h').read_text()
with tempfile.TemporaryDirectory() as directory:
    cpp=Path(directory)/'reload.cpp';binary=Path(directory)/'reload'
    cpp.write_text(harness)
    flags=['c++','-std=c++17','-fsanitize=address,undefined']
    sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():flags+=['-isystem',str(sdk)]
    subprocess.run(flags+[str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS: reload fades before stop, blocks retriggers, clears every MIDI key and protects RAM preview writes')
