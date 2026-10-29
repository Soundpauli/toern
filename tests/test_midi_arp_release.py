"""Exercise firmware MIDI handlers and arp scheduling together, including release."""
from pathlib import Path
import re,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1]
def function(file,name):
 s=(root/file).read_text();clean=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',lambda m:' ' *len(m[0]),s)
 m=re.search(r'(?m)^(?:static )?(?:inline )?(?:void|bool|int|uint8_t) '+name+r'\([^;{}]*\)\s*\{',clean);assert m,name
 end=clean.index('{',m.start())+1;depth=1
 while depth:depth+=(clean[end]=='{')-(clean[end]=='}');end+=1
 return s[m.start():end]
code=r'''
#include <cassert>
#include <cstdint>
#include <climits>
#include <cmath>
#include <algorithm>
#include <vector>
#include "toern_midi_pitch.h"
#include "toern_pattern_io.h"
using std::abs;using std::lroundf;
#define constrain(x,a,b) std::min(std::max((x),(a)),(b))
namespace toern_audio {struct StateGuard{};}
constexpr int maxY=16,ARP_CENTER=16;
constexpr uint8_t NOTE_MIDI_PITCH_NONE=255;
struct {int currentChannel=11;bool singleMode=false;}GLOB;
struct {float bpm=120;}SMP;
float octave[2]={4,0};int transpose=0,voiceSelect=0;
bool MIDI_NOTE_RECEIVE=true,MIDI_VOICE_SELECT=false,MIDI_NOTE_CLAMP=false,MIDI_WRITE_OVERWRITE=false,isNowPlaying=false;
bool midiHeldNote[17][128]{},persistentNoteOn[17]{},noteOnTriggered[17]{},activeNotes[128]{};
uint8_t midiHeldVelocity[17][128]{};
int16_t pressedKeyCount[17]{},midiHeldCh11SoundNote[128]{};
uint32_t arpStepCounter[15]{};uint16_t sequencerRecordBeat=1;
uint16_t midiArpLastBeat[15];unsigned long midiArpLastMs[15]{};
int16_t midiArpLastCh11Note=INT16_MIN;bool midiArpKick[15]{};
int arp[15]{};unsigned long now=0;
struct Env{int off=0;void noteOff(){off++;}}env[17];Env *envelopes[17];
int notes[15],fires[15]{},releases[15]{};
void stopSound(int pitch,int){if(notes[11]==pitch){notes[11]=-1;releases[11]++;}}
void playSound(int pitch,int,int){notes[11]=pitch;fires[11]++;}
void stopSynthChannel(int ch){notes[ch]=-1;releases[ch]++;persistentNoteOn[ch]=false;}
void playSynthMidi(int ch,uint8_t pitch,int,bool){notes[ch]=pitch;fires[ch]++;}
void playSynth(int ch,int pitch,int,bool){notes[ch]=pitch;fires[ch]++;}
int arpSigned(int ch){return arp[ch];}bool arpUsesEighths(int ch){return arp[ch]<0;}
int arpSpanSteps(int){return 4;}int arpPingPongIndex(uint32_t step,int span){int p=step%(2*(span-1));return p>=span?2*(span-1)-p:p;}
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
unsigned long millis(){return now;}uint16_t quantizedMidiRecordBeat(){return sequencerRecordBeat;}
uint8_t midiPitchToGridRow(int ch,uint8_t pitch){return toern_midi_pitch::gridRow(ch,pitch,4);}
bool preserveIncomingMidiPitch(){return !MIDI_NOTE_CLAMP;}
bool isChildVoiceDisabled(int){return false;}
void logMidiRxNoteOn(uint8_t,uint8_t,uint8_t){}
void playIncomingMidiNote(int ch,uint8_t pitch,uint8_t,uint8_t){
 notes[ch]=pitch;fires[ch]++;
 if(ch==11)midiHeldCh11SoundNote[pitch]=(int16_t)pitch;
}
bool placeLiveGridNote(uint8_t,uint8_t,uint8_t,uint8_t,bool,bool){return true;}
void handleNoteOff(uint8_t,uint8_t,uint8_t);
static void midiArpFireChannel(int);
void midiArpRestart(int);
FUNCTIONS
struct IoNote{uint8_t channel,velocity,probability,condition,midiPitch;};
struct IoSettings{uint8_t value=0;};
static std::vector<uint8_t> ioBytes(256*16*4+2+sizeof(IoSettings),0);
struct IoFile{
 size_t position=0;bool open=false;
 operator bool()const{return open;}size_t size(){return ioBytes.size();}
 int read(uint8_t *dst,size_t n){assert(n<=512);n=std::min(n,ioBytes.size()-position);memcpy(dst,ioBytes.data()+position,n);position+=n;now+=2;return n;}
 void close(){open=false;}
};
struct IoFS{IoFile open(const char*){return {0,true};}}ioFs;
int main(){
 for(int i=0;i<15;i++){midiArpLastBeat[i]=0xFFFF;notes[i]=-1;envelopes[i]=&env[i];}
 for(bool playing:{false,true})for(int ch:{11,13,14})for(int setting:{-16,-1,1,16}){
  isNowPlaying=playing;GLOB.currentChannel=ch;arp[ch]=setting;
  handleNoteOn(1,60,90);assert(pressedKeyCount[ch]==1&&notes[ch]>=0);
  for(int i=0;i<4;i++){now+=500;sequencerRecordBeat++;serviceMidiArp();serviceMidiArpOnPlayhead(sequencerRecordBeat);}
  handleNoteOn(1,64,110);assert(pressedKeyCount[ch]==2);
  handleNoteOff(1,60,0);assert(pressedKeyCount[ch]==1);
  handleNoteOff(1,64,0);assert(pressedKeyCount[ch]==0&&notes[ch]==-1);
  const int count=fires[ch];for(int i=0;i<4;i++){now+=500;serviceMidiArp();serviceMidiArpOnPlayhead(++sequencerRecordBeat);}
  assert(fires[ch]==count);
  handleNoteOn(1,60,100);handleNoteOn(1,60,0);assert(pressedKeyCount[ch]==0&&notes[ch]==-1);
 }
 // YPOS Note Off must follow the voice that took Note On, not the new selection.
 MIDI_VOICE_SELECT=false;
 for(int ch:{11,13,14}){
  GLOB.currentChannel=ch;arp[ch]=0;handleNoteOn(1,60,100);
  GLOB.currentChannel=1;handleNoteOff(1,60,0);
  assert(pressedKeyCount[ch]==0&&notes[ch]==-1);
 }
 // MIDI Note Off is serviced while a cooperative SD read is still in progress.
 ioBytes[256*16*4]=0xFF;ioBytes[256*16*4+1]=0xFE;
 toern_pattern_io::Reader<IoFile,IoNote,IoSettings,256,16> job;
 job.begin("pattern.txt",false);isNowPlaying=false;GLOB.currentChannel=11;arp[11]=16;
 handleNoteOn(1,60,100);unsigned polls=0;
 while(job.busy()){
  job.tick(ioFs);serviceMidiArp();
  if(++polls==12){assert(job.busy());handleNoteOff(1,60,0);assert(notes[11]==-1&&pressedKeyCount[11]==0);}
  if(polls>12)assert(notes[11]==-1);
 }
 assert(polls>12&&job.result()==toern_pattern_io::Result::Valid);
}
'''
funcs=function('toern_midi.ino','collectHeldMidiNotes')+function('toern_midi.ino','mapMidiToLogicalChannel')
for name in ['synthMidiArpEnabled','midiArpResetVoice','midiArpRestart','midiArpOnHeldNote','midiArpFireChannel','serviceMidiArpOnPlayhead','serviceMidiArp']:
 funcs+=function('toern.ino',name)+'\n'
funcs+=function('toern_midi.ino','handleNoteOn')+function('toern_midi.ino','handleNoteOff')
code=code.replace('FUNCTIONS',funcs)
with tempfile.TemporaryDirectory(prefix='toern-midi-arp-release-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: MIDI arp key release on CH11/13/14, YPOS channel change, stopped/playing, both directions, overlapping keys and velocity-zero release.')
