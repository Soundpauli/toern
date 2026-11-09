"""Check the actual first setup actions without hardware or real delays."""
from pathlib import Path
import subprocess
import tempfile
import sys
root=Path(__file__).resolve().parents[2]
s=(root/'platformio/toern.ino').read_text()
a=s.index('FLASHMEM void setup() {')+len('FLASHMEM void setup() {')
b=s.index('  Serial.begin(115200);',a)
startup=s[a:b]
code=r'''
#include <cassert>
#include <cstdint>
#include <vector>
constexpr int LED_BUILTIN=13,OUTPUT=1,HIGH=1,LOW=0;
std::vector<int> events;
void pinMode(int pin,int mode){assert(pin==13&&mode==OUTPUT);events.push_back(99);}
void digitalWrite(int pin,int value){assert(pin==13);events.push_back(value);}
void delay(int ms){events.push_back(ms);}
void boot(){STARTUP}
int main(){boot();std::vector<int> expected={99};for(int i=0;i<4;++i){expected.push_back(1);expected.push_back(500);expected.push_back(0);expected.push_back(500);}expected.push_back(1000);assert(events==expected);}
'''.replace('STARTUP',startup)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: four 500 ms on/off blinks and a 1 s pause precede Serial.begin and the remaining setup.')
