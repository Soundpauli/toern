"""Test real input, grid playback/output and Note Off pitch round trips."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
sketch=(root/'toern.ino').read_text()
def function(file,name):
    source=(root/file).read_text()
    clean=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]),source)
    match=re.search(r'(?m)^(?:static )?(?:inline )?(?:void|bool|int|float|uint8_t) '+name+r'\([^;{}]*\)\s*\{',clean)
    assert match,name
    end=clean.index('{',match.start())+1;depth=1
    while depth:
        depth+=(clean[end]=='{')-(clean[end]=='}');end+=1
    return source[match.start():end]
a=sketch.index('  float frequency;',sketch.index('static void playSynthInternal'))
b=sketch.index('  float WaveFormVelocity',a)
mono='float monoFrequency(int ch,int b,int midiPitchOverride){'+sketch[a:b]+'return frequency;}\n'
harness=r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>
#include <climits>
#include "toern_midi_pitch.h"
#define PROGMEM
#include "notes.h"
#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))
const uint8_t NOTE_MIDI_PITCH_NONE=255;
struct Note{uint8_t channel=1,midiPitch=255;};
float octave[2]={4,0},channelOctave[17]={};
int transpose=0,SampleRate[17]={},voiceSelect=0;
bool MIDI_NOTE_CLAMP=false,MIDI_NOTE_SEND=true;
int samplePlayed=0,polyPlayed=0,monoRow=0,monoMidi=0;
int16_t midiHeldCh11SoundNote[128]={};
void triggerSamplerVoice(int,int pitch,int,bool){samplePlayed=pitch;}
void playSound(int pitch,int,int){polyPlayed=pitch;}
void playSynth(int,int row,int,bool){monoRow=row;monoMidi=-1;}
void playSynthMidi(int,uint8_t pitch,int,bool){monoMidi=pitch;}
constexpr int maxY=16;
bool isChildVoiceDisabled(int){return false;}
uint8_t sequencerMidiOutCount=0,sequencerMidiOutPitch[16]={},sequencerMidiOutChannel[16]={};
struct {std::vector<int> ons,offs;void sendNoteOn(int p,int,int){ons.push_back(p);}void sendNoteOff(int p,int,int){offs.push_back(p);}} MIDI;
bool isNowPlaying=true,MIDI_WRITE_OVERWRITE=false;
struct {bool singleMode=true;} GLOB;
uint32_t arpStepCounter[15]={};int arpInputPitch=60,recordedPitch=-1,recordedRow=-1;
int16_t midiArpLastCh11Note=INT16_MIN;
constexpr int ARP_CENTER=32;
int collectHeldMidiNotes(int,uint8_t *p,uint8_t *v,int){p[0]=arpInputPitch;v[0]=100;return 1;}
int arpPingPongIndex(int,int){return 0;} int arpSpanSteps(int){return 1;} int arpSigned(int){return 1;}
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
void stopSound(int,int){}
bool placeLiveGridNote(uint8_t,uint8_t row,uint8_t,uint8_t pitch,bool,bool){recordedPitch=pitch;recordedRow=row;return true;}
FUNCTIONS
int main(){
 // Live and stored incoming sample C4 use the same root as a native base-row hit.
 for(int ch=1;ch<=8;++ch)for(int rate=1;rate<=3;++rate){
  SampleRate[ch]=rate;
  MIDI_NOTE_CLAMP=false;playIncomingMidiNote(ch,60,midiPitchToGridRow(ch,60),100);
  assert(samplePlayed==12*rate);
  for(int row=1;row<=15;++row){
   Note cell;cell.channel=ch;
   int native=samplePitchForNote(cell,ch,row),sent=midiPitchForOutput(cell,row);
   playIncomingMidiNote(ch,sent,midiPitchToGridRow(ch,sent),100);assert(samplePlayed==native);
   cell.midiPitch=sent;assert(samplePitchForNote(cell,ch,row)==native);
  }
 }
 // Existing native synth frequencies survive sending to a second, neutral receiver.
 for(int ch:{13,14})for(int row=1;row<=15;++row){
  Note cell;cell.channel=ch;int sent=midiPitchForOutput(cell,row);
  assert(std::fabs(monoFrequency(ch,row,-1)-monoFrequency(ch,0,sent))<0.1f);
 }
 assert(std::fabs(monoFrequency(13,0,69)-440.0f)<0.001f);
 // Home row is channel+1 and sounds C4 on every voice (CH11 table index 48).
 for(int ch:{1,2,8,11,13,14}){
  const int home=ch+1;Note cell;cell.channel=ch;octave[0]=4;transpose=0;
  assert(midiPitchForOutput(cell,home)==60);
  if(ch==11)assert(ch11PitchForNote(cell,home)==48);
 }
 octave[0]=4;transpose=0;MIDI_NOTE_CLAMP=false;
 playIncomingMidiNote(11,60,midiPitchToGridRow(11,60),100);
 assert(polyPlayed==48&&midiHeldCh11SoundNote[60]==48);
 for(int oct=1;oct<=7;++oct)for(int trans:{-12,0,12})for(int row=1;row<=15;++row){
  octave[0]=oct;transpose=trans;Note cell;cell.channel=11;
  int native=ch11PitchForNote(cell,row),sent=midiPitchForOutput(cell,row);
  assert(sent==native+12);
  transpose=0;octave[0]=4;MIDI_NOTE_CLAMP=false;
  playIncomingMidiNote(11,sent,midiPitchToGridRow(11,sent),100);
  assert(polyPlayed==native);transpose=trans;
 }
 octave[0]=4;transpose=0;MIDI_NOTE_CLAMP=false;
 playIncomingMidiNote(11,69,midiPitchToGridRow(11,69),100);
 assert(polyPlayed==57&&midiHeldCh11SoundNote[69]==57);
 assert(std::fabs(synthNoteFrequency(polyPlayed)-440.0f)<0.001f);
 // Octave/transpose remain patch tuning for preserved MIDI.
 octave[0]=3;transpose=2;playIncomingMidiNote(11,69,1,100);assert(polyPlayed==47);
 octave[0]=4;transpose=2;
 MIDI_NOTE_CLAMP=true;playIncomingMidiNote(11,69,midiPitchToGridRow(11,69),100);
 assert(polyPlayed==47); // FOLD plays the folded grid pitch + transpose
 MIDI_NOTE_CLAMP=false;octave[0]=4;transpose=0;
 // FOLD is octave-only; every supported voice retains incoming pitch class.
 for(int ch:{1,2,8,11,13,14})for(int p=0;p<128;++p){
  int row=midiPitchToGridRow(ch,p);assert(row>=1&&row<=15);
  Note cell;cell.channel=ch;int folded=midiPitchForOutput(cell,row);
  assert((folded-p)%12==0);
  MIDI_NOTE_CLAMP=true;playIncomingMidiNote(ch,p,row,100);
  if(ch<=8)assert(samplePlayed==samplePitchForNote(cell,ch,row));
  if(ch==11)assert(polyPlayed==ch11PitchForNote(cell,row));
  if(ch>=13)assert(monoRow==row&&monoMidi==-1);
  MIDI_NOTE_CLAMP=false;cell.midiPitch=p;assert(midiPitchForOutput(cell,row)==p);
 }
 // Preserved Note On and the queued Note Off both retain the exact MIDI number.
 for(int p=0;p<128;++p){
  Note cell;cell.midiPitch=p;MidiSendNoteOn(midiPitchForOutput(cell,2),1,100);
  MidiReleaseSequencerNotes();assert(MIDI.ons.back()==p&&MIDI.offs.back()==p);
 }
 // ARP playback and recorded cells obey the same preserved/folded pitch policy.
 for(int ch:{11,13,14})for(int p:{0,48,60,72,127}){
  arpInputPitch=p;MIDI_NOTE_CLAMP=false;midiArpFireChannel(ch);
  assert(recordedPitch==p&&recordedRow==midiPitchToGridRow(ch,p));
  if(ch==11)assert(polyPlayed==toern_midi_pitch::polyNote(p,0));
  else assert(monoMidi==p);
  MIDI_NOTE_CLAMP=true;midiArpFireChannel(ch);assert(recordedPitch==255);
  if(ch>=13)assert(monoRow==midiPitchToGridRow(ch,p)&&monoMidi==-1);
 }
 MIDI_NOTE_CLAMP=false;
 // KEYS mode keeps its existing unpitched sample-voice trigger behavior.
 voiceSelect=2;SampleRate[1]=3;playIncomingMidiNote(1,90,2,100);assert(samplePlayed==36);
}
'''
functions=''
for n in ['noteHasMidiPitch','midiPitchForOutput','samplePitchForNote','ch11PitchForNote']:
    functions+=function('toern.ino',n)+'\n'
for n in ['midiPitchToGridRow','preserveIncomingMidiPitch','playIncomingMidiNote','MidiSendNoteOn','MidiReleaseSequencerNotes']:
    functions+=function('toern_midi.ino',n)+'\n'
functions+=mono+function('toern_synths.ino','synthNoteFrequency')+'\n'+function('toern.ino','midiArpFireChannel')
harness=harness.replace('FUNCTIONS',functions)
with tempfile.TemporaryDirectory(prefix='toern-midi-pitch-') as tmp:
    p=Path(tmp);(p/'pitch.cpp').write_text(harness)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,'-I',str(root/'src'),'-I',str(root),str(p/'pitch.cpp'),'-o',str(p/'pitch')],check=True)
    subprocess.run([str(p/'pitch')],check=True)
print('PASS: sample/synth live and stored pitch, native-grid round trips, octave-only FOLD, A4=440, unchanged Note On/Off and KEYS.')
