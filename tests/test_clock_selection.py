"""Exercise actual clock selection persistence and J26 listener gating."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
menu = (root / 'toern_menu.ino').read_text()
sketch = (root / 'toern.ino').read_text()
midi = (root / 'toern_midi.ino').read_text()
load = menu[menu.index('  clockMode     ='):menu.index('  transportMode =', menu.index('  clockMode     ='))]
a = sketch.index('  if (currentMode->pos[2] != lastPos2)', sketch.index('FLASHMEM void setBPM()'))
select = sketch[a:sketch.index('  if (clockMode == 1 && (currentMode->pos[3]', a)]
a = midi.index('static void analogClockSyncListen() {')
listen = midi[a:midi.index('// If Play is armed', a)]
harness = r'''
#include <cassert>
#include <cstdint>
#include <algorithm>
#define ANALOG_CLOCK_IN_PIN 38
#define INPUT_PULLDOWN 1
#define INPUT_PULLUP 2
#define CHANGE 2
#define IRQ_GPIO6789 3
#define NVIC_SET_PRIORITY(irq, priority) setPriority(irq, priority)
namespace toern_audio {constexpr int kTimerPriority=224;}
constexpr int EEPROM_DATA_START=100;
struct Storage {uint8_t bytes[256]={}; uint8_t read(int i){return bytes[i];}} EEPROM;
void saveSingleModeToEEPROM(int i,int8_t v){EEPROM.bytes[EEPROM_DATA_START+i]=(uint8_t)v;}
struct Mode {unsigned int pos[4]={};} bpm;
Mode *currentMode=&bpm;
struct Knob {void writeCounter(int32_t){}} Encoder[4];
int clockMode=1,lastPos2=1,midiSendMode=2;
bool MIDI_CLOCK_SEND=true,MIDI_NOTE_SEND=true,analogExtClock=true,analogClockHigh=false;
bool analogInEnabled=true,analogInPolarityPositive=true;
bool analogClockInputActive(){return false;}
bool attached=false;int pin=-1,priority=-1;
int digitalPinToInterrupt(int p){return p;}
void detachInterrupt(int p){assert(p==38);attached=false;}
void pinMode(int p,int mode){assert(p==38&&mode==INPUT_PULLDOWN);pin=p;}
int digitalReadFast(int p){assert(p==38);return 0;}
void analogClockIsr(){}
void attachInterrupt(int p,void(*fn)(),int edge){assert(p==38&&fn==analogClockIsr&&edge==CHANGE);attached=true;}
void setPriority(int irq,int p){assert(irq==IRQ_GPIO6789);priority=p;}
#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))
LISTEN
void resetMidiClockState(){analogClockSyncListen();}
void syncBpmEncoderPositions(){}
void drawBPMScreen(){}
void load(){LOAD}
void select(){SELECT}
int main(){
 // Both EXT encodings must survive boot; malformed values retain the INT fallback.
 for(int raw: {0,255,1,23}){
  EEPROM.bytes[EEPROM_DATA_START+1]=raw;load();
  assert(clockMode==((raw==0||raw==255)?0:1));
  assert(EEPROM.read(EEPROM_DATA_START+1)==clockMode);
 }
 // Rotating to EXT persists 0, enables J26, and survives a reboot.
 clockMode=1;lastPos2=1;currentMode->pos[2]=0;select();
 assert(clockMode==0&&!MIDI_CLOCK_SEND&&attached&&priority==224);
 clockMode=1;load();analogClockSyncListen();assert(clockMode==0&&attached);
 // MIDI EXT and INT must both detach J26; returning to analog EXT reattaches it.
 analogExtClock=false;analogClockSyncListen();assert(!attached);
 analogExtClock=true;currentMode->pos[2]=1;select();assert(clockMode==1&&!attached);
 load();assert(clockMode==1);
}
'''.replace('LISTEN', listen).replace('LOAD', load).replace('SELECT', select)
with tempfile.TemporaryDirectory(prefix='toern-clock-selection-') as tmp:
    p=Path(tmp);(p/'clock.cpp').write_text(harness)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,str(p/'clock.cpp'),'-o',str(p/'clock')],check=True)
    subprocess.run([str(p/'clock')],check=True)
print('PASS: EXT save/reload, legacy EXT migration, INT fallback and J26 listener selection.')
