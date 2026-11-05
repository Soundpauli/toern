"""Verify sample-length attack, synth timing, note gates and simulator curves."""
from pathlib import Path
import subprocess,tempfile,json
root=(Path(__file__).resolve().parents[2] / 'platformio')
source=(root/'toern.ino').read_text()
start=source.index('float getNoteDuration(int channel) {');end=source.index('\n}',start)+2
method=source[start:end]
parameters=(root/'toern_parameters.ino').read_text()
start=parameters.index('float getAttackDuration(int channel) {');end=parameters.index('\n}',start)+2
attack=parameters[start:end]
assert 'mappedValue = getAttackDuration(index);' in parameters
assert 'if (ch >= 1 && envelopes[ch]) setParams(ATTACK, ch);' in source
code=r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <initializer_list>
constexpr float AUDIO_SAMPLE_RATE_EXACT=44100.0f;
uint32_t loadedSampleLen[15]={};
constexpr int DELAY=0,ATTACK=1,HOLD=2,DECAY=3;
constexpr int maxfilterResolution=32;
float maxParamVal[6]={1000,2000,1000,1000,1,1000};
struct {float param_settings[15][6]={};} SMP;
bool channelHasVoiceDelay(int){return true;}
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
'''+attack+'\n'+method+r'''
int main(){
 for(int ch=1;ch<=8;++ch){
  for(uint32_t frames : {0u, 2205u, 22050u, 44100u, 176400u}){
   loadedSampleLen[ch]=frames;
   for(int raw=0;raw<=32;++raw){
    SMP.param_settings[ch][ATTACK]=raw;
    const float expected=(1.0f-raw/32.0f)*frames*1000.0f/AUDIO_SAMPLE_RATE_EXACT;
    assert(std::abs(getAttackDuration(ch)-expected)<0.001f);
    assert(std::abs(getNoteDuration(ch)-(expected+5))<0.001f);
   }
  }
 }
 // The same setting must follow a replacement sample without a knob change.
 SMP.param_settings[1][ATTACK]=16;
 loadedSampleLen[1]=4410;assert(std::abs(getAttackDuration(1)-50)<0.001f);
 loadedSampleLen[1]=44100;assert(std::abs(getAttackDuration(1)-500)<0.001f);
 assert(mapf(32,0,32,2000,0)==0&&mapf(0,0,32,2000,0)==2000);
 assert(mapf(16,0,32,2000,0)==1000);
 float previous=2001;
 for(int raw=0;raw<=32;++raw){
  const float ms=mapf(raw,0,32,2000,0);assert(ms<previous||raw==32);previous=ms;
  SMP.param_settings[13][ATTACK]=raw;SMP.param_settings[14][ATTACK]=raw;
  assert(std::abs(getNoteDuration(13)-(ms+8))<.0001);
  assert(std::abs(getNoteDuration(14)-(ms+8))<.0001);
 }
}
'''
engine=(root/'../tools/sim/src/audio/engine.js').read_text()
att=next(line for line in engine.splitlines() if 'const att = (env,' in line)
for duration in (0.05, 0.5, 1, 4, 2):
 values=json.loads(subprocess.check_output(['node','--input-type=module','-e',att+f';console.log(JSON.stringify(Array.from({{length:33}},(_,attValue)=>att({{att:attValue}}, {duration}))))'],text=True))
 for raw,seconds in enumerate(values):
  expected=max(0.001, ((32-raw)/32)*duration)
  assert abs(seconds-expected)<1e-10
assert 'const a = att(env, span);' in engine
assert 'const a = att(env);' in engine  # Synths retain the default two-second range.
with tempfile.TemporaryDirectory() as directory:
    path=Path(directory);(path/'attack.cpp').write_text(code)
    flags=['c++','-std=c++17','-fsanitize=address,undefined','-I',str(root)]
    sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():flags+=['-isystem',str(sdk)]
    subprocess.run(flags+[str(path/'attack.cpp'),'-o',str(path/'attack')],check=True)
    subprocess.run([str(path/'attack')],check=True)
print('PASS: sample-length attack across all 33 settings, sample replacement, matching gates/simulator and fixed synth range')
