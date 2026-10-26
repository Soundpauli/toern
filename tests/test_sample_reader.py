"""Exercise the repo reader snapshot with mono/stereo data and old velocity."""
from pathlib import Path
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
stub=r'''
#pragma once
#include <cstdint>
#define AUDIO_BLOCK_SAMPLES 128
struct Reader {
 bool playing=true,hasData=true;int channels=1,position=0,rate=1;
 bool isPlaying(){return playing;}bool available(){return hasData;}
 int getNumChannels(){return channels;}
 uint32_t playbackFramesRemaining(){return playing&&hasData?128:0;}
 unsigned read(void **out,unsigned count){for(int ch=0;ch<channels;++ch)for(unsigned i=0;i<count;++i)((int16_t*)out[ch])[i]=(ch?12000:1000)*rate;position+=count;return count;}
};
class AudioPlayArrayResmp {
protected:
 Reader source;Reader *reader=&source;int32_t ampMult=65536,ampTarget=65536;
public:
 Reader &fake(){return source;}
 void amplitude(int32_t value){ampMult=ampTarget=value;}
};
'''
code=r'''
#include <cassert>
#include "src/sample_reader.h"
#include "src/toern_sample_envelope.h"
int main(){
 AudioSampleReader reader;int16_t tail[64]={};
 assert(reader.snapshotTail(tail,64)==64);for(auto x:tail)assert(x==1000);assert(reader.fake().position==64);
 reader.fake().channels=2;reader.amplitude(32768);reader.fake().rate=2;
 assert(reader.snapshotTail(tail,64)==64);for(auto x:tail)assert(x==1000); // old stereo-left rate 2 x old velocity 0.5
 toern_audio::SampleEnvelopeState envelope;envelope.noteOn();envelope.captureTail(tail,64);envelope.noteOn();
 assert(envelope.render(5000)==6000);for(int i=1;i<64;++i)envelope.render(5000);assert(envelope.render(5000)==5000);
 reader.fake().playing=false;assert(reader.snapshotTail(tail,64)==0);
 reader.fake().playing=true;reader.fake().hasData=false;assert(reader.snapshotTail(tail,64)==0);
 reader.fake().hasData=true;assert(reader.snapshotTail(tail,65)==0);
 reader.fake().channels=3;assert(reader.snapshotTail(tail,64)==0);
}
'''
with tempfile.TemporaryDirectory(prefix='toern-reader-tail-') as directory:
 p=Path(directory);(p/'playarrayresmp.h').write_text(stub);(p/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,'-I',str(p),'-I',str(root),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: actual old-reader snapshot uses old rate/velocity, keeps the mono channel, drains its tail, and handles ended/invalid readers.')
