"""Check the actual reader's duration calculation in output frames."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/resamplerReader.h').read_text()
start = source.index('    uint32_t playbackFramesRemaining() {')
end = source.index('    uint32_t lengthMillis()', start)
method = source[start:end]
harness = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <algorithm>
using std::max;using std::isfinite;
enum {looptype_none,looptype_repeat};
enum class play_start {play_start_sample,play_start_loop};
struct Reader {
 bool _playing=true,_useDualPlaybackHead=false,_stretchActive=false;
 int _numChannels=1,_loopType=looptype_none,_header_offset=0,_loop_start=0,_loop_finish=1000,_bufferPosition1=0;
 int _stretchOutputLimit=0,_stretchOutput=0;
 float _playbackRate=1;
 uint8_t _timeStretchSetting=11;
 play_start _play_start=play_start::play_start_sample;
 METHOD
};
int main(){
 Reader r;assert(r.playbackFramesRemaining()==1000);
 r._playbackRate=2;assert(r.playbackFramesRemaining()==500);
 r._playbackRate=.5;assert(r.playbackFramesRemaining()==2000);
 r._playbackRate=-2;r._bufferPosition1=999;assert(r.playbackFramesRemaining()==500);
 r._loop_start=200;r._loop_finish=800;r._play_start=play_start::play_start_loop;
 r._bufferPosition1=799;r._playbackRate=-1;assert(r.playbackFramesRemaining()==600);
 r._bufferPosition1=200;r._playbackRate=1;assert(r.playbackFramesRemaining()==600);
 r._loop_start=0;r._loop_finish=10;r._bufferPosition1=0;assert(r.playbackFramesRemaining()==10);
 r._loop_finish=1000;
 for(int setting:{0,10,12,22}) {
   r._timeStretchSetting=setting;
   const int percent=setting<11?90+setting:100+setting-12;
   for(float rate:{.5f,1.f,2.f}){r._playbackRate=rate;assert(r.playbackFramesRemaining()==10u*percent);}
 }
 r._playbackRate=4;assert(r.playbackFramesRemaining()==250);
 r._playbackRate=1;r._stretchActive=true;r._stretchOutputLimit=1100;r._stretchOutput=123;
 assert(r.playbackFramesRemaining()==977);
 r._stretchActive=false;r._numChannels=2;r._loop_finish=2000;r._playbackRate=2;
 assert(r.playbackFramesRemaining()==500);
 r._loopType=looptype_repeat;assert(r.playbackFramesRemaining()==0);
 r._loopType=looptype_none;r._playing=false;assert(r.playbackFramesRemaining()==0);
}
'''.replace('METHOD', method)
with tempfile.TemporaryDirectory(prefix='toern-duration-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: reader duration includes pitch, trim, reverse, stereo and actual TIME/WSOLA eligibility, including sub-ms samples.')
