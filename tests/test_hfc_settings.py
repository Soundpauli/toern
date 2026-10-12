"""Exercise actual firmware HFC EEPROM decoding/migration and all 27 round trips."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
menu = (root / 'toern_menu.ino').read_text()
helpers = menu[menu.index('FLASHMEM static uint8_t hfcEncodeStep'):menu.index('FLASHMEM void loadMenuFromEEPROM')]
start = menu.index('    extern uint16_t codecHfCut;\n    uint8_t eqFmt')
end = menu.index('    codecHfCut = cut;', start) + len('    codecHfCut = cut;')
load = menu[start:end]
source = '''#include <cstdint>
#include <cstring>
#include <cassert>
#define FLASHMEM
const int EEPROM_DATA_START = 0;
uint16_t codecHfCut = 0;
struct Memory {
 uint8_t bytes[64] = {};
 uint8_t read(int i) {return bytes[i];}
 void write(int i, uint8_t v) {bytes[i]=v;}
 template<class T> void get(int i,T& v) {memcpy(&v,bytes+i,sizeof(T));}
} EEPROM;
void markSettingsBackupDirty() {}
''' + helpers + '\nvoid load() {\n' + load + '''
}
int main() {
 EEPROM.write(32,0); EEPROM.write(33,5); load(); assert(codecHfCut==0);
 for (uint16_t step=0; step<=26; ++step) {
  uint16_t cut=step==26 ? 256 : step*10;
  EEPROM.write(32,hfcEncodeStep(cut)); EEPROM.write(33,5);
  load(); assert(codecHfCut==cut); assert(EEPROM.read(33)==5);
 }
 for (uint16_t cut=10;cut<=250;cut+=10) {
  EEPROM.write(32,cut); EEPROM.write(33,4); load(); assert(codecHfCut==cut);
  assert(EEPROM.read(33)==5); load(); assert(codecHfCut==cut);
 }
 EEPROM.write(32,0); EEPROM.write(33,4); load(); assert(codecHfCut==256);
 EEPROM.write(32,255); EEPROM.write(33,5); load(); assert(codecHfCut==0);
 EEPROM.write(32,255); EEPROM.write(33,255); load(); assert(codecHfCut==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    src=Path(tmp)/'hfc.cpp'; exe=Path(tmp)/'hfc'; src.write_text(source)
    sdk='/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'
    command=['c++','-std=c++17']
    if Path(sdk).exists(): command+=['-isystem',sdk]
    subprocess.run(command+[str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: HFC defaults, 27 EEPROM round trips, legacy migration, corrupt storage.')
