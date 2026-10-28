"""Run the real filter gain transitions between dry, lowpass and highpass."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_filter.ino').read_text()
start = source.index('#define NUM_MIXER_CHANNELS')
end = source.index('// ch13/14 CUT', start)
graph = (root / 'audioinit.h').read_text()
for ch in range(1, 9):
    assert f'patchSampleDry{ch}(amp{ch}, 0, filtermixer{ch}, 3);' in graph
harness = r'''
#include <cassert>
#include <cmath>
#include <algorithm>
constexpr int NUM_FILTERS=15,maxfilterResolution=32,HCUT=0,LOWCUT=1;
const int ALL_CHANNELS[]={1,2,3,4,5,6,7,8,11,13,14};
const int NUM_ALL_CHANNELS=11;
struct {float filter_settings[15][2]{};} SMP;
struct Mixer {float gains[4]{};void gain(int ch,float v){assert(ch>=0&&ch<4);gains[ch]=v;}} mixers[15];
struct Filter {float hz;void frequency(float v){hz=v;}} filter[15];
Mixer *filtermixers[15]{};Filter *filters[15]{};
template<class T>T constrain(T x,T a,T b){return std::clamp(x,a,b);}
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
FUNCTIONS
void settled(int ch,int path){
 for(int i=0;i<40;++i)updateAllMixerGains();
 for(int p=0;p<4;++p)assert(std::abs(mixers[ch].gains[p]-(p==path?1.f:0.f))<.0001);
}
int main(){
 for(int ch:ALL_CHANNELS){
  filtermixers[ch]=&mixers[ch];filters[ch]=&filter[ch];
  SMP.filter_settings[ch][HCUT]=32;
  initFilterTransition(ch);
  assert(mixers[ch].gains[3]==(ch<=8?1.f:0.f));
  applyVoiceCuts(ch);settled(ch,ch<=8?3:0);
  // Closing HCUT restores the lowpass path, including other voices' ramps.
  SMP.filter_settings[ch][HCUT]=16;applyVoiceCuts(ch);settled(ch,0);
  SMP.filter_settings[ch][HCUT]=32;applyVoiceCuts(ch);settled(ch,ch<=8?3:0);
  // LCUT takes precedence even with HCUT fully open.
  SMP.filter_settings[ch][LOWCUT]=8;applyVoiceCuts(ch);settled(ch,2);
  SMP.filter_settings[ch][LOWCUT]=0;applyVoiceCuts(ch);
  forceAllMixerGainsToTarget();settled(ch,ch<=8?3:0);
 }
}
'''.replace('FUNCTIONS', source[start:end])
with tempfile.TemporaryDirectory(prefix='toern-filter-bypass-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: eight dry connections; smooth dry/LP/HP transitions; synth filter routing unchanged.')
