"""Compile real pulse output code with a four-channel PIT and GPIO edge recorder."""
from pathlib import Path
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'toern_midi.ino').read_text()
shared=source[source.index('static inline uint8_t pulseIdleLevel()'):source.index('static uint8_t sidechainVoice =')]
# Substitute only the ARM status-register read for the host stub.
shared=shared.replace('__asm__ volatile("mrs %0, primask" : "=r"(primask_) :: "memory");', 'primask_ = __get_PRIMASK();')
a=source.index('void sidechainOnVoice(uint8_t ch) {')
trigger=source[a:source.index('static uint8_t analogClockInPpqn()',a)]
a=source.index('void setPulseClockPolarityPositive(bool positive) {')
polarity=source[a:source.index('void togglePulseClockPolarity()',a)]
sketch=(root/'toern.ino').read_text()
a=sketch.index('void triggerSamplerVoice(int ch, int pitch, int vel, bool retrigger, bool sendSidechain) {')
sampler=sketch[a:sketch.index('// Restore amp gains',a)]
harness=r'''
#include <cstdint>
#include <climits>
#include <cassert>
#include <vector>
#include <cmath>
#include <algorithm>
#define LOW 0
#define HIGH 1
#define OUTPUT 1
#define PULSE_CLOCK_PIN 37
#define SIDECHAIN_OUT_PIN 28
namespace toern_audio {constexpr uint8_t kTimerPriority=224; struct StateGuard {}; }
uint32_t nowUs=0,mask=0;
uint32_t micros(){return nowUs;}
uint32_t __get_PRIMASK(){return mask;}
void __disable_irq(){mask=1;}
void __enable_irq(){mask=0;}
struct Edge{int pin,level;uint32_t time;};
std::vector<Edge> edges;
int levels[40]={};
void digitalWriteFast(int pin,int level){if(levels[pin]!=level){levels[pin]=level;edges.push_back({pin,level,nowUs});}}
void pinMode(int,int){}
// Three PIT channels already belong to sequencer, fills and MIDI clock.
struct IntervalTimer {
 static inline int used=3;
 bool active=false;void(*callback)()=nullptr;uint32_t deadline=0;
 void priority(uint8_t n){assert(n==224);}
 bool begin(void(*fn)(),uint32_t us){
  if(!active){if(used>=4)return false;active=true;++used;}
  callback=fn;deadline=nowUs+us;return true;
 }
 void end(){if(active){active=false;--used;}}
};
IntervalTimer pulseEndTimer;
bool pulseClockPolarityPositive=true,isNowPlaying=true;
uint8_t pulseClockWidthMs=12,sidechainVoice=1;
volatile bool pulseClockPinActive=false,sidechainPinActive=false;
volatile uint32_t pulseClockEndUs=0,sidechainEndUs=0;
void pulseClockSaveToEEPROM(){}
SHARED
TRIGGER
POLARITY
bool fastRecordActive=false,pendingPauseAudioCleanup=true;uint8_t fastRecordChannel=1;
bool sliderControlled=false;
bool sampleReloadBusy=false;
bool noteOnTriggered[17]{};
bool isChannelSampleReloadBusy(unsigned){return sampleReloadBusy;}
float neoSliderVelScale[4]={1,1,1,1},neoSliderLastVelNorm[4]={},detune[9]={};
int noteOns=0,noteOffs=0;
int effectiveNoteVelocity(int n){return n;}
bool isNeoSliderControlledChannel(int ch){return sliderControlled&&ch>=1&&ch<=4;}
void samplerNoteEvent(int,uint8_t,uint8_t,bool on,bool){if(on)++noteOns;else ++noteOffs;}
struct Voice {float rate=1;void snapAmplitude(float){} void setPlaybackRate(float r){rate=r;}};
Voice *voices[9]={};
#define constrain(x,lo,hi) std::clamp((x),(lo),(hi))
void triggerSamplerVoice(int ch,int pitch,int vel,bool retrigger,bool sendSidechain=false);
SAMPLER
void advance(uint32_t us){
 while(pulseEndTimer.active && (uint32_t)(pulseEndTimer.deadline-nowUs)<=us){
  uint32_t d=pulseEndTimer.deadline-nowUs;nowUs+=d;us-=d;pulseEndTimer.callback();
 }
 nowUs+=us;
}
int rises(int pin){int n=0;for(auto e:edges)if(e.pin==pin&&e.level==HIGH)++n;return n;}
void reset(){pulseEndTimer.end();IntervalTimer::used=3;nowUs=0;mask=0;
 pulseClockPinActive=sidechainPinActive=false;pulseClockPolarityPositive=true;sidechainVoice=1;
 for(auto &v:levels)v=0;edges.clear();pulseClockWidthMs=12;}
int main(){
 sampleReloadBusy=true;triggerSamplerVoice(1,60,127,true,true);
 assert(noteOns==0&&edges.empty());sampleReloadBusy=false;
 // Painting/audition stays audible, but never pulses J27, even during playback.
 reset();isNowPlaying=false;triggerSamplerVoice(1,60,100,true);
 assert(noteOns==1&&edges.empty()&&!pendingPauseAudioCleanup);
 isNowPlaying=true;triggerSamplerVoice(1,60,100,true);
 assert(noteOns==2&&edges.empty());
 // Only an explicit sequencer/fill hit emits; a later Note Off adds no edge.
 triggerSamplerVoice(1,60,100,true,true);assert(rises(28)==1);
 advance(12000);const auto endEdges=edges.size();
 samplerNoteEvent(1,60,0,false,false);advance(100000);
 assert(rises(28)==1&&edges.size()==endEdges&&noteOffs==1);
 // A zero-volume fader or active recording rejects the hit before sidechain.
 reset();sliderControlled=true;neoSliderVelScale[0]=0;
 triggerSamplerVoice(1,60,100,true,true);assert(edges.empty());
 sliderControlled=false;fastRecordActive=true;
 triggerSamplerVoice(1,60,100,true,true);assert(edges.empty());fastRecordActive=false;
 reset();isNowPlaying=false;sidechainOnVoice(1);assert(edges.empty());
 isNowPlaying=true;sidechainOnVoice(2);assert(edges.empty());sidechainOnVoice(1);
 assert(levels[28]==HIGH&&rises(28)==1&&IntervalTimer::used==4);
 advance(11999);assert(levels[28]==HIGH);advance(1);
 assert(levels[28]==LOW&&rises(28)==1&&IntervalTimer::used==3);
 advance(1000000);assert(rises(28)==1); // Expiry is not a second trigger.
 reset();setPulseClockPolarityPositive(false);assert(levels[28]==LOW);
 sidechainOnVoice(1);advance(5000);setPulseClockPolarityPositive(true);advance(7000);
 assert(levels[28]==LOW&&rises(28)==1);
 reset();pulseClockEmit();advance(5000);sidechainOnVoice(1);
 assert(IntervalTimer::used==4);advance(7000);assert(levels[37]==LOW&&levels[28]==HIGH);
 advance(5000);assert(levels[28]==LOW&&IntervalTimer::used==3&&rises(28)==1);
 reset();pulseClockEmit();sidechainOnVoice(1);advance(1000);pulseClockForceIdle();
 assert(levels[37]==LOW&&levels[28]==HIGH&&IntervalTimer::used==4);
 advance(11000);assert(levels[28]==LOW&&rises(28)==1);
 reset();sidechainOnVoice(1);advance(5000);sidechainOnVoice(1);advance(7000);
 assert(levels[28]==HIGH&&rises(28)==1);advance(5000);assert(levels[28]==LOW);
 advance(100000);sidechainOnVoice(1);advance(12000);assert(rises(28)==2);
 reset();nowUs=0xFFFFFF00u;sidechainOnVoice(1);advance(12000);
 assert(levels[28]==LOW&&rises(28)==1);
 reset();IntervalTimer::used=4;sidechainOnVoice(1);
 assert(levels[28]==LOW&&!sidechainPinActive&&!pulseEndTimer.active);
 reset();mask=1;sidechainOnVoice(1);assert(mask==1);advance(12000);assert(mask==1);
 // Full-range MIDI/sample conversion may request a pitch outside the event byte.
 reset();Voice voice;voices[1]=&voice;
 for(int pitch: {-48,-24,130}){
  triggerSamplerVoice(1,pitch,100,true);
  float expected=powf(2.0f,(pitch-36.0f)/12.0f);
  assert(std::fabs(voice.rate-expected)<expected*0.0001f);
 }
 assert(edges.empty());
}
'''.replace('SHARED',shared).replace('TRIGGER',trigger).replace('POLARITY',polarity).replace('SAMPLER',sampler)
with tempfile.TemporaryDirectory(prefix='toern-sidechain-') as tmp:
    p=Path(tmp);(p/'pulses.cpp').write_text(harness)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,str(p/'pulses.cpp'),'-o',str(p/'pulses')],check=True)
    subprocess.run([str(p/'pulses')],check=True)
print('PASS: previews and Note Off stay quiet; sequencer emits one positive sidechain pulse, independent clock polarity/deadlines, overlap, retrigger, timer capacity and micros wrap.')
