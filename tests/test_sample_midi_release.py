"""Exercise actual MIDI Note Off handling with overlapping sample keys."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_midi.ino').read_text()
start = source.index('void handleNoteOff(uint8_t midiChannel, uint8_t pitch, uint8_t velocity) {')
end = source.index('\nvoid handleStart()', start)
harness = r'''
#include <cassert>
#include <cstdint>
bool MIDI_NOTE_RECEIVE=true,MIDI_VOICE_SELECT=true;
bool midiHeldNote[17][128]{},persistentNoteOn[17]{},noteOnTriggered[17]{};
uint8_t midiHeldVelocity[17][128]{};
int16_t pressedKeyCount[17]{},midiHeldCh11SoundNote[128]{};
struct Env {int releases=0; void noteOff(){++releases;}} env[17];
Env *envelopes[17]{};
int mapMidiToLogicalChannel(int channel,uint8_t){return channel;}
bool synthMidiArpEnabled(int){return false;}
void stopSound(int,int){} void midiArpResetVoice(int){} void stopSynthChannel(int){}
FUNCTION
int main(){
 for(int ch=1;ch<=8;++ch){
  envelopes[ch]=&env[ch];
  midiHeldNote[ch][60]=midiHeldNote[ch][64]=true;
  midiHeldVelocity[ch][60]=90;midiHeldVelocity[ch][64]=110;
  pressedKeyCount[ch]=2;
  handleNoteOff(ch,60,0);
  assert(!midiHeldNote[ch][60]&&midiHeldVelocity[ch][60]==0);
  assert(midiHeldNote[ch][64]&&pressedKeyCount[ch]==1&&env[ch].releases==0);
  // A duplicate or unrelated Note Off cannot cut the remaining held key.
  handleNoteOff(ch,60,0);handleNoteOff(ch,72,0);
  assert(pressedKeyCount[ch]==1&&env[ch].releases==0);
  noteOnTriggered[ch]=true;  handleNoteOff(ch,64,0);
  assert(pressedKeyCount[ch]==0&&env[ch].releases==0&&!persistentNoteOn[ch]);
  assert(!midiHeldNote[ch][64]&&midiHeldVelocity[ch][64]==0);
  // Without an in-flight ADSR gate, the last key starts release immediately.
  midiHeldNote[ch][72]=true;pressedKeyCount[ch]=1;noteOnTriggered[ch]=false;persistentNoteOn[ch]=true;
  handleNoteOff(ch,72,0);
  assert(pressedKeyCount[ch]==0&&env[ch].releases==1);
 }
 midiHeldNote[1][60]=true;pressedKeyCount[1]=1;
 const int before=env[1].releases;
 MIDI_NOTE_RECEIVE=false;handleNoteOff(1,60,0);
 assert(midiHeldNote[1][60]&&env[1].releases==before);
 MIDI_NOTE_RECEIVE=true;handleNoteOff(1,255,0);
 assert(midiHeldNote[1][60]&&env[1].releases==before);
 // Voices without an envelope still maintain their held-key bookkeeping.
 envelopes[1]=nullptr;handleNoteOff(1,60,0);assert(pressedKeyCount[1]==0);
}
'''.replace('FUNCTION', source[start:end])
with tempfile.TemporaryDirectory(prefix='toern-midi-release-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: all eight sample voices stay open after the final held key; duplicate/disabled/invalid releases are safe.')
