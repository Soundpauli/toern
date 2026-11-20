"""Check the actual first setup actions without hardware or real delays."""
from pathlib import Path
import subprocess
import tempfile
import sys
root=Path(__file__).resolve().parents[2]
s=(root/'platformio/toern.ino').read_text()
a=s.index('FLASHMEM void setup() {')+len('FLASHMEM void setup() {')
b=s.index('  NVIC_SET_PRIORITY(',a)
startup=s[a:b]
code=r'''
#include <cassert>
#include <cstdint>
#include <vector>
constexpr int OUTPUT=1,HIGH=1,INPUT_PULLUP=2;
constexpr int BUTTON_A=1,BUTTON_B=2,BUTTON_C=3,BUTTON_D=4;
bool exttouch=false;
std::vector<int> events;
struct {void begin(int baud){assert(baud==115200);events.push_back(10);}} Serial;
void pinMode(int pin,int mode){
  if(pin==36){assert(mode==OUTPUT);events.push_back(36);}
  else {assert(exttouch&&pin>=1&&pin<=4&&mode==INPUT_PULLUP);events.push_back(pin);}
}
void digitalWrite(int pin,int value){assert(pin==36&&value==HIGH);events.push_back(100);}
void delay(int){assert(false && "Startup must not wait before initialization");}
void boot(){STARTUP}
int main(){
  boot();assert((events==std::vector<int>{10,36,100}));
  events.clear();exttouch=true;boot();
  assert((events==std::vector<int>{10,1,2,3,4,36,100}));
}
'''.replace('STARTUP',startup)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: Serial and LED power initialize immediately without startup blinks or delays on either panel.')
