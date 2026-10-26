"""Exercise the firmware's actual settings-copy block and Device layout."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'toern.ino').read_text()
fileops = (root / 'toern_fileoperations.ino').read_text()
sample = main[main.index('struct Sample {'):main.index('// Declare the device struct')]
device = main[main.index('struct Device {'):main.index('// Global variables struct')]
start = fileops.index('      // OFF keeps both the live audio')
end = fileops.index('      if (!isfinite(SMP.bpm)', start)
copy_block = fileops[start:end]
harness = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#define EXTMEM
const unsigned maxFiles=9,maxY=16,maxFilters=15,maxPages=16;
SAMPLE
DEVICE
Device SMP;
void load(bool autoload,bool SMP_LOAD_SETTINGS,const Device &incoming) {
  const unsigned expectedNotes=0;
  uint8_t payload[2+sizeof(Device)]{};
  memcpy(payload+2,&incoming,sizeof(Device));
  COPY
}
int main() {
  Device current{},incoming{};
  current.pack=7;current.wav[1].fileID=507;
  strcpy(current.samplePathRel[1],"custom.wav");current.sp0Active[1]=true;
  current.param_settings[1][0]=11;current.filter_settings[1][0]=22;
  current.synth_settings[13][0]=3;current.channelVol[1]=9;
  incoming.pack=1;incoming.bpm=147;incoming.file=42;
  incoming.param_settings[1][0]=1;incoming.filter_settings[1][0]=2;
  incoming.synth_settings[13][0]=5;incoming.channelVol[1]=16;
  incoming.globalMutes[2]=true;incoming.pageMutes[4][3]=true;
  incoming.songArrangement[7]=6;
  for(bool autoload:{false,true})for(bool enabled:{false,true}) {
    SMP=current;load(autoload,enabled,incoming);
    const auto &sound=(autoload||enabled)?incoming:current;
    assert(SMP.pack==sound.pack);
    assert(memcmp(SMP.wav,sound.wav,sizeof(SMP.wav))==0);
    assert(memcmp(SMP.samplePathRel,sound.samplePathRel,sizeof(SMP.samplePathRel))==0);
    assert(memcmp(SMP.sp0Active,sound.sp0Active,sizeof(SMP.sp0Active))==0);
    assert(memcmp(SMP.param_settings,sound.param_settings,sizeof(SMP.param_settings))==0);
    assert(memcmp(SMP.filter_settings,sound.filter_settings,sizeof(SMP.filter_settings))==0);
    assert(memcmp(SMP.synth_settings,sound.synth_settings,sizeof(SMP.synth_settings))==0);
    assert(memcmp(SMP.channelVol,sound.channelVol,sizeof(SMP.channelVol))==0);
    assert(SMP.bpm==147&&SMP.file==42);
    assert(SMP.globalMutes[2]&&SMP.pageMutes[4][3]&&SMP.songArrangement[7]==6);
  }
}
'''.replace('SAMPLE', sample).replace('DEVICE', device).replace('COPY', copy_block)
# Range-for initializer lists require this header on the host compiler.
harness = '#include <initializer_list>\n' + harness
with tempfile.TemporaryDirectory(prefix='toern-pattern-sound-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: firmware manual settings OFF preserves sounds, ON restores sounds, and startup recovery always restores the full session.')
