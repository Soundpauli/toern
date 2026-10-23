"""Check actual SYNC menu labels and inactive values against transport modes."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_menu.ino').read_text()
start = source.index('static bool syncDelayHasEffect(')
end = source.index('\n}', start) + 2
helper = source[start:end]
start = source.index('    case 45: // SYNC - Transport delay')
end = source.index('\n    case 50:', start)
render = source[start:end]
harness = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
using CRGB = int;
const int UI_GREEN=1, UI_RED=2;
struct Mode {int pos[4]={};} mode;
Mode *currentMode=&mode;
int8_t transportSendDelayMs=0,transportRcveDelayMs=0;
bool MIDI_CLOCK_SEND=false,MIDI_TRANSPORT_RECEIVE=false,analogExt=false;
bool getAnalogExtClock(){return analogExt;}
CRGB currentMenuParentTextColor(){return 0;}
std::string label,value;
CRGB valueColor;
void drawMenuText(const char *s,int,int,CRGB){label=s;}
void drawMenuValue(const char *s,int,int,CRGB c){value=s;valueColor=c;}
void drawMenuIndicator(char,char,int){}
''' + helper + '\nvoid render(){switch(45){\n' + render + r'''
}}
int main(){
 for(int clock=0;clock<2;++clock)
 for(int receive=0;receive<2;++receive)
 for(int analog=0;analog<2;++analog)
 for(int edit=0;edit<2;++edit)
 for(int offset : {-20,0,20}){
   MIDI_CLOCK_SEND=clock;MIDI_TRANSPORT_RECEIVE=receive;analogExt=analog;
   mode.pos[1]=edit;transportSendDelayMs=transportRcveDelayMs=offset;
   // Master Play uses the send delay. MIDI Start uses the receive delay;
   // local Play also uses it when waiting for MIDI clock. Analog EXT bypasses it.
   bool active;
   if((edit==0&&offset>=0)||(edit==1&&offset<0))active=clock;
   else active=clock?receive:!analog;
   render();
   assert(label==(edit==0?"SYC>":"SYC<"));
   assert(value==(active?std::to_string(offset):"--"));
   assert(valueColor==(!active||edit==1?UI_RED:UI_GREEN));
   assert(transportSendDelayMs==offset&&transportRcveDelayMs==offset);
 }
}
'''
with tempfile.TemporaryDirectory(prefix='toern-sync-menu-') as tmp:
    cpp = Path(tmp) / 'sync.cpp'
    exe = Path(tmp) / 'sync'
    cpp.write_text(harness)
    cmd = ['c++', '-std=c++17']
    sdk = Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():
        cmd += ['-isystem', str(sdk)]
    subprocess.run(cmd + [str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: SYC>/SYC< labels, signed offsets, red -- for inactive paths, and saved values across all clock/transport modes.')
