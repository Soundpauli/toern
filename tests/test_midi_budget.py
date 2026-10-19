"""Compile the actual firmware message loop against a fake MIDI input queue."""
from pathlib import Path
import subprocess
import tempfile
import sys
source = Path('toern_midi.ino').read_text()
a = source.index('  static const uint8_t MAX_MIDI_MESSAGES_PER_LOOP')
b = source.index('  // If this device is sending MIDI Clock', a)
loop = source[a:b]
harness = r'''
#include <cstdint>
#include <vector>
#include <cassert>
#include <iostream>
namespace midi { enum { Clock=1, NoteOn, NoteOff, Stop, PitchBend, SongPosition, TimeCodeQuarterFrame }; }
std::vector<int> events;
struct FakeMidi {
 std::vector<int> messages; unsigned at=0; int type=0;
 bool read() { if (at == messages.size()) return false; type=messages[at++];
   if (type==midi::Clock || type==midi::NoteOn) events.push_back(type); return true; }
 uint8_t getType(){return type;} uint8_t getChannel(){return 1;}
 uint8_t getData1(){return 60;} uint8_t getData2(){return 100;}
} MIDI;
void logMidiRxNoteOff(int,int,int){} void logMidiRxSimple(const char*){}
void logMidiRxPitchBend(int,int){} void logMidiRxSongPosition(int){}
void handleNoteOff(int,int,int){events.push_back(midi::NoteOff);}
void handleStop(){events.push_back(midi::Stop);}
void applyExternalFastFilter(int){events.push_back(midi::PitchBend);}
void handleSongPosition(int){events.push_back(midi::SongPosition);}
void handleTimeCodeQuarterFrame(int){events.push_back(midi::TimeCodeQuarterFrame);}
void pump() { LOOP }
int main(){
 MIDI.messages={midi::Clock,midi::NoteOn,midi::NoteOff,
 midi::PitchBend,midi::SongPosition,midi::TimeCodeQuarterFrame,midi::Stop,
 midi::PitchBend,midi::SongPosition,midi::TimeCodeQuarterFrame,midi::Stop,
 midi::PitchBend,midi::SongPosition,midi::TimeCodeQuarterFrame,midi::Stop,
 midi::PitchBend,midi::SongPosition,midi::TimeCodeQuarterFrame,midi::Stop,
 midi::NoteOff,midi::Stop,midi::Clock,midi::NoteOn,midi::NoteOff};
 pump(); assert(MIDI.at==19); assert(events.size()==19);
 while(MIDI.at<MIDI.messages.size()) pump();
 assert(events==MIDI.messages);
 std::cout<<"MIDI burst preserves Note Off, Stop, clock and callback messages\n";
}
'''.replace('LOOP', loop)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp); (p/'midi.cpp').write_text(harness)
 flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,str(p/'midi.cpp'),'-o',str(p/'midi')],check=True)
 subprocess.run([str(p/'midi')],check=True)
