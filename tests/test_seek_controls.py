"""Exercise actual SMP trim configuration and update logic, including zero."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root/'toern_sample.ino').read_text()

def block(signature):
    start = source.index(signature)
    end = source.index('{', start)+1
    depth = 1
    while depth:
        depth += (source[end] == '{')-(source[end] == '}')
        end += 1
    return source[start:end]

code = r'''
#include <cassert>
#include <cstdint>
#include <algorithm>
#define FLASHMEM
#define constrain(x,lo,hi) std::clamp((x),(lo),(hi))
struct Mode {unsigned pos[4]={};} set_Wav,draw;
Mode *currentMode=&set_Wav;
struct {unsigned seek=0,seekEnd=100;} GLOB;
struct Encoder {
 int lo=1,hi=16,step=1,value=0;
 void writeMin(int32_t n){lo=n;}void writeMax(int32_t n){hi=n;}
 void writeStep(int32_t n){step=n;}void writeCounter(int32_t n){value=n;}
 void turn(int n){value=std::clamp(value+n*step,lo,hi);}
} Encoder[4];
bool allowEncWrite=true,sampleIsLoaded=true;
struct {bool active=false;} g_peakScan;
int previews=0;
bool previewPlaysOnSelect(){return true;}
void yield(){}void previewSample(bool,bool){++previews;}
CONFIGURE
void update(){TRIM}
void turn(int encoder,int delta){
 Encoder[encoder].turn(delta);
 currentMode->pos[encoder==1?2:0]=(unsigned)Encoder[encoder].value;
 update();
}
int main(){
 configureSampleTrimEncoders(true);
 assert(Encoder[0].lo==0&&Encoder[0].hi==99&&Encoder[0].step==1);
 assert(Encoder[1].lo==1&&Encoder[1].hi==100&&Encoder[1].step==1);
 for(int n:{1,2,20,99}){
  turn(0,n);assert(GLOB.seek==(unsigned)n);
  turn(0,-n);assert(GLOB.seek==0&&set_Wav.pos[0]==0&&Encoder[0].value==0);
 }
 turn(1,-1);assert(GLOB.seekEnd==99);turn(1,1);assert(GLOB.seekEnd==100);
 turn(1,-99);assert(GLOB.seekEnd==1);turn(0,90);
 assert(GLOB.seek==0&&Encoder[0].value==0);
 turn(1,99);turn(0,50);turn(1,-90);
 assert(GLOB.seek==50&&GLOB.seekEnd==51&&Encoder[1].value==51);
 turn(0,-50);turn(1,100);assert(GLOB.seek==0&&GLOB.seekEnd==100);
 // A negative physical overshoot must return to zero, not unsigned-wrap high.
 set_Wav.pos[0]=(unsigned)-1;update();assert(GLOB.seek==0&&Encoder[0].value==0);
 GLOB.seek=30;GLOB.seekEnd=70;configureSampleTrimEncoders(true);
 assert(set_Wav.pos[0]==30&&set_Wav.pos[2]==70);
 GLOB.seek=0;GLOB.seekEnd=0;configureSampleTrimEncoders(true);
 assert(set_Wav.pos[0]==0&&set_Wav.pos[2]==100);
 GLOB.seek=1;set_Wav.pos[0]=0;Encoder[0].value=0;
 configureSampleTrimEncoders(false);assert(GLOB.seek==1&&set_Wav.pos[0]==0);
 update();assert(GLOB.seek==0&&Encoder[0].value==0);
 currentMode=&draw;Encoder[0].lo=1;configureSampleTrimEncoders(true);assert(Encoder[0].lo==1);
 assert(previews>0);
}
'''.replace('CONFIGURE', block('FLASHMEM void configureSampleTrimEncoders('))
code = code.replace('TRIM', block('if (!sampleChanged && !seekHwResetPending)').split('{',1)[1][:-1])
sketch = (root/'toern.ino').read_text()
assert 'configureSampleTrimEncoders(true);' in sketch
assert 'configureSampleTrimEncoders(false);' in source[source.index('if (allowEncWrite && !encRangeSynced)'):]
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(code)
    flags = ['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: seek returns to 0; seek-end returns to 100; crossing/overshoot, legacy full range and mode isolation')
