"""Run the actual delay/reverb DSP and pause cleanup against host audio blocks."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]

def function(path, signature):
    source = (root / path).read_text()
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

arduino = r'''
#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
using std::min;
constexpr int AUDIO_BLOCK_SAMPLES=128;
constexpr float AUDIO_SAMPLE_RATE_EXACT=44100;
inline void __disable_irq(){} inline void __enable_irq(){}
'''
audio = r'''
#pragma once
#include <Arduino.h>
#include <vector>
#include <cassert>
struct audio_block_t {uint8_t refs,reserved;uint16_t index;int16_t data[AUDIO_BLOCK_SAMPLES];};
class AudioStream {
 audio_block_t **queue;
 unsigned count;
public:
 std::vector<audio_block_t> output;
 AudioStream(unsigned n,audio_block_t **q):queue(q),count(n){for(unsigned i=0;i<n;i++)queue[i]=nullptr;}
 virtual void update()=0;
 void feed(int value,unsigned channel=0){assert(channel<count&&!queue[channel]);auto b=new audio_block_t{};for(auto &v:b->data)v=value;queue[channel]=b;}
 audio_block_t *receiveReadOnly(unsigned ch=0){auto b=queue[ch];queue[ch]=nullptr;return b;}
 audio_block_t *allocate(){return new audio_block_t{};}
 void release(audio_block_t *b){delete b;}
 void transmit(audio_block_t *b,unsigned=0){output.push_back(*b);}
};
'''
dsp = r'''
#include "effect_delay_ext.h"
#include "effect_freeverb_dmabuf.h"
#include <cassert>
bool audible(AudioStream &s){for(auto &b:s.output)for(auto v:b.data)if(v!=0)return true;return false;}
void delayTest(int length){
 std::vector<int16_t> storage(length);
 AudioEffectDelayExternal d;d.begin(storage.data(),storage.size());
 d.delay(0,120.0f);d.delay(1,250.0f);
 for(int i=0;i<200;i++){d.feed(12000);d.update();}
 assert(audible(d));d.output.clear();
 // Pause ends source input, but echoes must survive envelope closure + 20 ms.
 for(int i=0;i<10;i++)d.update();
 assert(audible(d));d.output.clear();
 // The entire history drains naturally through the ring, without a reset.
 for(int i=0;i<600;i++)d.update();
 d.output.clear();
 for(int i=0;i<100;i++)d.update();
 assert(!audible(d));d.output.clear();
 // Existing taps still work without reapplying settings on the next note.
 for(int i=0;i<200;i++){d.feed(8000);d.update();}
 assert(audible(d));
}
int main(){
 delayTest(16384);delayTest(16387);
 AudioEffectFreeverbDMAMEM verb;verb.roomsize(1.0f);verb.damping(0.4f);
 for(int i=0;i<60;i++){verb.feed(i<8?10000:0);verb.update();}
 assert(audible(verb));verb.output.clear();
 // High-room-size reverb remains audible well beyond the old cleanup window.
 for(int i=0;i<100;i++)verb.update();
 assert(audible(verb));verb.output.clear();
 // Let the real DSP reach silence rather than clearing tanks after a timer.
 for(int i=0;i<10000;i++){verb.update();verb.output.clear();}
 for(int i=0;i<100;i++)verb.update();
 assert(!audible(verb));verb.output.clear();
 for(int i=0;i<60;i++){verb.feed(i<8?10000:0);verb.update();}
 assert(audible(verb));
}
'''
cleanup = r'''
#include <cassert>
#include <cstdint>
namespace toern_audio {struct StateGuard {};}
unsigned now=0;
struct elapsedMillis {
 unsigned start=0;
 operator unsigned() const{return now-start;}
 elapsedMillis &operator=(unsigned value){start=now-value;return *this;}
};
struct Envelope {
 bool active=false;int offs=0;
 bool isActive(){return active;}
 void noteOff(){offs++;} // Simulate a release which finishes on a later audio tick.
} env[15],poly[3][3];
Envelope *envelopes[15]={};
Envelope *Senvelope1[3]={&poly[0][0],&poly[0][1],&poly[0][2]};
Envelope *Senvelope2[3]={&poly[1][0],&poly[1][1],&poly[1][2]};
Envelope *SenvelopeFilter1[3]={&poly[2][0],&poly[2][1],&poly[2][2]};
struct Voice {int stops=0;void stop(){stops++;}} readers[9];
Voice *voices[9]={};
struct Effect {int clears=0;void clear(){clears++;}} verbs[15];
Effect *freeverbs[15]={};
const int VOICE_DELAY_COUNT=11;
Effect voiceDelay[11],voiceDelayFb[11],voiceDelayMix[11];
bool pendingPauseAudioCleanup=false,isNowPlaying=true,isRecording=false,fastRecordActive=false;
elapsedMillis pauseEffectsQuietMs,pauseSdWaitMs;
const int POLY_VOICES=3,INSTRUMENT_CHANNELS=2,NOTE_EMPTY=-32768;
int noteArray[2][3]={{40,43,47},{51,55,59}};
void releasePolyVoice(int i){Senvelope1[i]->noteOff();Senvelope2[i]->noteOff();SenvelopeFilter1[i]->noteOff();}
bool MIDI_CLOCK_SEND=false,MIDI_TRANSPORT_SEND=false;
namespace midi{const int Stop=0;}
struct {void sendRealTime(int){}} MIDI;
void cancelMidiTransportStart(){}
void MidiReleaseSequencerNotes(){} void clearLiveRecordSkip(){} void deleteActiveCopy(){}
void stopSynthChannel(int){} void resetMidiPressedKeyCount11to14(){} void resetSequencedSynthLegato(){}
struct {void end(){}} fillTimer;
struct {int page=2;} GLOB;
bool ctrlVolumeOverlayActive=true,pendingStartOnBar=true,fillHasTriggered=true,fillRunning=true;
bool noteOnTriggered[15]={},persistentNoteOn[15]={};
int sequencerRecordStepStartedUs=5,lastFlowPage=2,sequencerSynthTick=2,beat=2,beatForUI=2,loopCount=2;
int fillSubTick=1,fillStartSubTick=1,fillTriggerStep=1,fillTriggerRow=1,fillActiveChannel=1,fillActiveVelocity=1,fillActiveRow=1,fillActiveMidiPitch=1;
const int NOTE_MIDI_PITCH_NONE=255;
bool pendingPauseUIUpdate=false,pendingPauseSkipSave=false;
int pauseSdSettleGen=0;
uint16_t touchFillPendingMask=0;
uint32_t fillStepsRemaining=0;
struct TouchFillState {bool running=false;uint32_t startSubTick=0,stepsRemaining=0;};
struct FillDropState {bool pending=false;uint8_t row=0,midiPitch=255;};
FillDropState fillDrops[15];
TouchFillState touchFills[15];
''' + function('toern.ino','static void resetTouchFills() {')+'\n'+function('toern_synths.ino','void releaseAllPolySynthVoices() {') + '\n' + function('toern.ino','void pause(bool skipSave) {') + '\n' + function('toern.ino','void servicePauseAudioCleanup() {') + r'''
int main(){
 for(int i=0;i<15;i++){envelopes[i]=&env[i];freeverbs[i]=&verbs[i];env[i].active=true;}
 for(int i=0;i<9;i++)voices[i]=&readers[i];
 for(auto &group:poly)for(auto &e:group)e.active=true;
 noteOnTriggered[11]=persistentNoteOn[11]=true;touchFillPendingMask=(1u<<11);touchFills[11].running=true;fillDrops[11].pending=true;fillStepsRemaining=12;
 pause(true);assert(!isNowPlaying && pendingPauseSkipSave && pendingPauseAudioCleanup && touchFillPendingMask==0&&!touchFills[11].running&&!fillDrops[11].pending&&fillStepsRemaining==0);
 assert(!noteOnTriggered[11]&&!persistentNoteOn[11]);
 for(auto &e:env)assert(e.offs==1);
 for(auto &group:poly)for(auto &e:group)assert(e.offs==1);
 for(auto &group:noteArray)for(auto note:group)assert(note==NOTE_EMPTY);
 now+=100;servicePauseAudioCleanup();assert(verbs[11].clears==0);
 for(auto &e:env)e.active=false;
 for(auto &group:poly)for(auto &e:group)e.active=false;
 now+=19;servicePauseAudioCleanup();assert(verbs[11].clears==0&&readers[0].stops==0);
 now+=1;servicePauseAudioCleanup();assert(!pendingPauseAudioCleanup);
 for(int i=0;i<11;i++)assert(voiceDelay[i].clears==0&&voiceDelayFb[i].clears==0&&voiceDelayMix[i].clears==0);
 assert(verbs[11].clears==0);for(auto &r:readers)assert(r.stops==1);
 servicePauseAudioCleanup();assert(verbs[11].clears==0);
 // Restart/capture cancels cleanup instead of silencing new audio.
 for(int which=0;which<3;which++){
  pendingPauseAudioCleanup=true;
  isNowPlaying=which==0;isRecording=which==1;fastRecordActive=which==2;
  servicePauseAudioCleanup();assert(!pendingPauseAudioCleanup&&verbs[11].clears==0);
 }
}
'''
with tempfile.TemporaryDirectory(prefix='toern-pause-effects-') as tmp:
    tmp = Path(tmp)
    (tmp/'Arduino.h').write_text(arduino)
    (tmp/'AudioStream.h').write_text(audio)
    (tmp/'utility').mkdir()
    (tmp/'utility/dspinst.h').write_text('')
    flags = ['c++','-std=c++17','-O2','-D__ARM_ARCH_7EM__','-I',str(tmp),'-I',str(root/'src')]
    sdk = Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():
        flags += ['-isystem',str(sdk)]
    for name, code, sources in [
        ('dsp',dsp,[root/'src/effect_delay_ext.cpp',root/'src/effect_freeverb_dmabuf.cpp']),
        ('cleanup',cleanup,[]),
    ]:
        cpp=tmp/f'{name}.cpp';exe=tmp/name;cpp.write_text(code)
        subprocess.run(flags+[str(cpp),*map(str,sources),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
print('PASS: pause releases voices without clearing effects; long delay/reverb tails decay naturally to silence and retain their settings.')
