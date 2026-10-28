"""Check boot volume decoding against the later hardware-settings path."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'toern.ino').read_text()
menu = (root / 'toern_menu.ino').read_text()
boot = main[main.index('  GLOB.vol = (int8_t)EEPROM.read'):main.index('  // Load micGain from EEPROM')]
init = main[main.index('  float vol =', main.index('FLASHMEM void initSoundChip()')):main.index('  // Initialize audio input amplifier', main.index('FLASHMEM void initSoundChip()'))]
apply_start = menu.index('  float vol;', menu.index('FLASHMEM void applyAudioSettingsFromGlobals()'))
apply = menu[apply_start:menu.index('  // Apply line output level', apply_start)]
harness = r'''
#include <cassert>
#include <cstdint>
#include <cmath>
#include <algorithm>
constexpr int EEPROM_DATA_START=43;
struct {int8_t vol;} GLOB;
struct {int value; int read(int){return value;}} EEPROM;
struct {float level; void volume(float value){level=value;}} sgtl5000_1;
template<class T>T constrain(T x,T lo,T hi){return std::clamp(x,lo,hi);}
void boot(){BOOT}
void init(){INIT}
void apply(){APPLY}
int main(){
 const int values[]={0,1,5,10,11,50,80,100,101,255};
 const float expected[]={.8,.1,.5,1,.11,.5,.8,1,.8,.8};
 for(int i=0;i<10;++i){
  EEPROM.value=values[i];boot();init();
  assert(std::abs(sgtl5000_1.level-expected[i])<.00001);
  apply();assert(std::abs(sgtl5000_1.level-expected[i])<.00001);
 }
}
'''.replace('BOOT', boot).replace('INIT', init).replace('APPLY', apply)
with tempfile.TemporaryDirectory(prefix='toern-volume-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: legacy/new boot levels match hardware reapplication; invalid/zero EEPROM defaults to 80%.')
