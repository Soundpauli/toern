"""Silence detection must preserve quiet PCM and both stereo channels."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
code = r'''
#include "src/toern_pcm_activity.h"
#include <cassert>
int main(){
 ToernPcmActivity activity;
 int16_t left[128]={},right[128]={};int16_t *pcm[]={left,right};
 assert(!activity.active());
 right[127]=1;activity.observe(pcm,2,128);assert(activity.active());
 right[127]=0;activity.observe(pcm,2,128);assert(activity.active());
 activity.observe(pcm,2,128);assert(!activity.active());
 left[0]=-1;activity.observe(pcm,2,128);assert(activity.active());
 left[0]=0;activity.observe(pcm,1,128);assert(activity.active());
 left[127]=1;activity.observe(pcm,1,128);assert(activity.active());
 activity.clear();assert(!activity.active());
 // Only the frames actually read count, not stale/unwritten buffer contents.
 activity.observe(pcm,1,64);assert(!activity.active());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    source, binary = Path(tmp)/'test.cpp', Path(tmp)/'test'
    source.write_text(code)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', '-fsanitize=address,undefined', *flags,
                    '-I', str(root), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: two zero blocks, +/-1 LSB, stereo, clear and partial-block PCM activity')

# Exercise the actual dependency patch twice. An activity hook between close()
# and amplitude reset must not make the earlier patch try to apply again.
patch = root/'scripts/platformio_patch_dependencies.py'
with tempfile.TemporaryDirectory() as tmp:
    work=Path(tmp)
    playback=work/'test/TeensyVariablePlayback/src/playresmp.h'
    playback.parent.mkdir(parents=True)
    playback.write_text((root/'.pio/libdeps/teensy41/TeensyVariablePlayback/src/playresmp.h').read_text())
    class Env:
        def subst(self,key):
            return {'$PROJECT_DIR':str(root),'$PROJECT_LIBDEPS_DIR':str(work),'$PIOENV':'test'}[key]
        def AddBuildMiddleware(self,callback):
            pass
    namespace={'Import':lambda name:None,'env':Env()}
    exec(compile(patch.read_text(),str(patch),'exec'),namespace)
    first=playback.read_text()
    exec(compile(patch.read_text(),str(patch),'exec'),namespace)
    assert playback.read_text()==first
    assert first.count('pcmActivity.observe(')==1
    assert first.count('bool hasRecentOutput()')==1
    assert 'pcmActivity.observe(data, _numChannels, n);' in first
print('PASS: PCM activity dependency patch is idempotent and observes rendered PCM once')
