"""Verify the attack response and note gate duration use the same curve."""
from pathlib import Path
import subprocess,tempfile,json
root=Path(__file__).resolve().parents[1]
source=(root/'toern.ino').read_text()
start=source.index('float getNoteDuration(int channel) {');end=source.index('\n}',start)+2
method=source[start:end]
code=r'''
#include <cassert>
#include <cmath>
#include "src/toern_envelope_curve.h"
constexpr int DELAY=0,ATTACK=1,HOLD=2,DECAY=3;
constexpr int maxfilterResolution=32;
float maxParamVal[6]={1000,500,1000,1000,1,1000};
struct {float param_settings[15][6]={};} SMP;
bool channelHasVoiceDelay(int){return true;}
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
'''+method+r'''
int main(){
 using toern_audio::attackMilliseconds;
 assert(attackMilliseconds(32)==0&&attackMilliseconds(0)==500);
 assert(attackMilliseconds(28)==7.8125f&&attackMilliseconds(16)==125);
 assert(attackMilliseconds(-1)==500&&attackMilliseconds(33)==0);
 assert(attackMilliseconds(NAN)==0);
 float previous=501;
 for(int raw=0;raw<=32;++raw){
  const float ms=attackMilliseconds(raw);assert(ms<previous);previous=ms;
  SMP.param_settings[13][ATTACK]=raw;SMP.param_settings[14][ATTACK]=raw;
  assert(std::abs(getNoteDuration(13)-(ms+8))<.0001);
  assert(std::abs(getNoteDuration(14)-(ms+8))<.0001);
 }
}
'''
engine=(root/'standalone-tools/web-standalone/src/audio/engine.js').read_text()
att=next(line for line in engine.splitlines() if 'const att = (env)' in line)
values=json.loads(subprocess.check_output(['node','--input-type=module','-e',att+';console.log(JSON.stringify(Array.from({length:33},(_,attValue)=>att({att:attValue}))))'],text=True))
for raw,seconds in enumerate(values):assert abs(seconds-(.5*((32-raw)/32)**2))<1e-10
with tempfile.TemporaryDirectory() as directory:
    path=Path(directory);(path/'attack.cpp').write_text(code)
    flags=['c++','-std=c++17','-fsanitize=address,undefined','-I',str(root)]
    sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():flags+=['-isystem',str(sdk)]
    subprocess.run(flags+[str(path/'attack.cpp'),'-o',str(path/'attack')],check=True)
    subprocess.run([str(path/'attack')],check=True)
print('PASS: fine attack control near 32, 500ms maximum, monotonic 0–32 range, matching note gate and simulator curve')
