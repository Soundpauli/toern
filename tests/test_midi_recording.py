"""Exercise live grid writes and held synth continuation in the actual firmware."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def function(file, name):
    source = (root / file).read_text()
    # Preserve offsets while ignoring comments/strings when balancing braces.
    clean = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                   lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), source)
    match = re.search(r'(?m)^(?:static )?(?:void|bool|int|uint8_t) '+name+r'\([^;{}]*\)\s*\{', clean)
    assert match, name
    end = clean.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (clean[end] == '{') - (clean[end] == '}')
        end += 1
    return source[match.start():end]

harness = r'''
using uint8_t=unsigned char; using uint16_t=unsigned short; using int16_t=short;
#define assert(x) do {if(!(x))__builtin_trap();} while(false)
const int maxY=16,maxlen=257,defaultVelocity=100;
const uint8_t NOTE_MIDI_PITCH_NONE=255,NOTE_CONDITION_GLIDE=22;
struct Note {uint8_t channel=0,velocity=100,probability=100,condition=1,midiPitch=255;};
Note note[maxlen][maxY+1];
struct {int currentChannel=13; bool singleMode=true;} GLOB;
bool MIDI_NOTE_RECEIVE=true,MIDI_NOTE_CLAMP=true,MIDI_VOICE_SELECT=false,MIDI_WRITE_OVERWRITE=false,isNowPlaying=true;
int voiceSelect=0,auditions=0,releases=0; unsigned int beat=3;
bool activeNotes[128]={},midiHeldNote[17][128]={},persistentNoteOn[17]={},childDisabled=false;
uint8_t midiHeldVelocity[17][128]={};
int16_t pressedKeyCount[17]={};
bool synthMidiArpEnabled(int){return false;}
void midiArpOnHeldNote(int){}
volatile uint16_t liveRecordSkipBeat=0,liveRecordSkipRows=0;
void noInterrupts(){} void interrupts(){}
bool isChildVoiceDisabled(int){return childDisabled;}
uint16_t quantizedMidiRecordBeat(){return beat;}
void extendLastPageForLiveNote(uint8_t, uint8_t, unsigned int){}
void logMidiRxNoteOn(uint8_t,uint8_t,uint8_t){}
void playIncomingMidiNote(int,uint8_t,uint8_t,uint8_t){auditions++;}
void handleNoteOff(uint8_t,uint8_t pitch,uint8_t){releases++;midiHeldNote[GLOB.currentChannel][pitch]=false;}
'''
for name in ['clearLiveRecordSkip','markLiveRecordSkip','consumeLiveRecordSkip','placeLiveGridNote']:
    harness += function('toern.ino', name)+'\n'
for name in ['mapMidiToLogicalChannel','midiPitchToGridRow','preserveIncomingMidiPitch','handleNoteOn','midiRecordCellMatches','onBeatTick']:
    harness += function('toern_midi.ino', name)+'\n'
harness += r'''
int main(){
  handleNoteOn(1,60,88);
  assert(note[3][2].channel==13 && note[3][2].velocity==88 && note[3][2].midiPitch==255 && auditions==1);
  assert(consumeLiveRecordSkip(3,2) && !consumeLiveRecordSkip(3,2));
  onBeatTick(4,3); assert(note[4][2].condition==NOTE_CONDITION_GLIDE && note[4][2].velocity==88);
  note[5][2].channel=2; onBeatTick(5,4); assert(note[5][2].channel==2);
  MIDI_WRITE_OVERWRITE=true; onBeatTick(5,4); assert(note[5][2].channel==13 && note[5][2].condition==NOTE_CONDITION_GLIDE);
  handleNoteOn(1,60,0); assert(releases==1 && auditions==1);
  onBeatTick(6,5); assert(note[6][2].channel==0);
  MIDI_NOTE_CLAMP=false; beat=7; handleNoteOn(1,72,99);
  assert(note[7][14].midiPitch==72 && note[7][14].velocity==99);
  clearLiveRecordSkip(); assert(liveRecordSkipBeat==0 && liveRecordSkipRows==0);
  childDisabled=true; beat=8; handleNoteOn(1,60,99); assert(note[8][2].channel==0 && auditions==2);
  childDisabled=false; GLOB.singleMode=false; handleNoteOn(1,60,99); assert(note[8][2].channel==0 && auditions==3);
  GLOB.singleMode=true; isNowPlaying=false; handleNoteOn(1,60,99); assert(note[8][2].channel==0 && auditions==4);
}
'''
with tempfile.TemporaryDirectory(prefix='toern-midi-recording-') as directory:
    cpp=Path(directory)/'recording.cpp'; binary=Path(directory)/'recording'
    cpp.write_text(harness)
    subprocess.run(['c++','-std=c++17',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS: live MIDI recording, preserved pitch, OVWR, held synth continuation and duplicate-hit skip marks.')
