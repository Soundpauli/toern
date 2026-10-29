"""Exercise firmware sustain bounds and line-output codec/UI round trips."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'toern.ino').read_text()
params = (root / 'toern_parameters.ino').read_text()
ui = (root / 'toern_filterUI.ino').read_text()
menu = (root / 'toern_menu.ino').read_text()

def block(source, signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

parameter_enum = block(main, 'enum ParameterType') + ';'
preset_values = re.search(r'static const uint8_t kCh11SustainDefault\[13\] = \{[\s\S]*?\};', params)[0]
preset_assignment = next(line.strip() for line in params.splitlines() if 'SMP.param_settings[channel][SUSTAIN] = max' in line)
line_case = block(menu, 'case 28: { // LINE')
harness = r'''
#include <cassert>
#include <cstdint>
#include <cmath>
#include "src/toern_envelope_curve.h"
#include <algorithm>
using std::max;
#define constrain(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
constexpr int maxfilterResolution=32,PARAM_COUNT=6;
PARAM_ENUM
enum SettingArray {ARR_PARAM,ARR_NONE};
struct {float param_settings[15][8]{};} SMP;
float maxParamVal[12]{},sustainAmp[2][3]{};
uint16_t attackAmp[2][3]{},decayAmp[2][3]{},releaseAmp[2][3]{},releaseFilter[2][3]{};
struct Env {float level,attackMs;void attack(float v){attackMs=v;}void decay(float){}void release(float){}void sustain(float v){level=v;}} env[15];
Env *envelopes[15]{};
void updateVals(int){}bool channelHasVoiceDelay(int){return false;}void applyVoiceDelay(int){}
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
PARAMS
struct Mode {int pos[4]{},minValues[4]{};} mode;
Mode *currentMode=&mode;
struct Enc {int lo,hi,value;void writeMin(int v){lo=v;}void writeMax(int v){hi=v;}void writeCounter(int v){value=v;}} Encoder[4];
struct Spec {SettingArray arr=ARR_NONE;int idx=-1,maxValue=32;} sliderDef[15][1][4];
uint8_t filterPage[15]{};
void showFilterNames(uint8_t){}void showFilterPages(uint8_t){}
uint8_t readSetting(SettingArray,int8_t idx,uint8_t ch){return SMP.param_settings[ch][idx];}
SLIDERS
PRESETS
void preset(int channel,int instrumentIdx){PRESET_ASSIGNMENT}
constexpr int LINEOUT_MIN=13,LINEOUT_MAX=31;
uint8_t lineOutLevelSetting=30;
bool lineMenuFirstEnter=true;
struct {int value;void lineOutLevel(int v){value=v;}} sgtl5000_1;
int savedCodec=-1;
void saveSingleModeToEEPROM(int slot,int8_t v){assert(slot==15);savedCodec=v;}
void redrawMain(int){}
void lineMenu(){int setting=28;switch(setting){LINE_CASE}}
int main(){
 for(int ch:{1,8,11,13,14})for(int raw=0;raw<=32;++raw){
  envelopes[ch]=&env[ch];SMP.param_settings[ch][ATTACK]=raw;setParams(ATTACK,ch);
  const float expected=500.f*(32-raw)*(32-raw)/1024.f;
  if(ch==11)assert(attackAmp[0][0]==(uint16_t)expected);
  else assert(std::abs(env[ch].attackMs-expected)<.00001f);
 }
 maxParamVal[SUSTAIN]=1;
 for(int ch:{1,8,11,13,14})for(int raw:{0,1,16,32,40}){
  envelopes[ch]=&env[ch];SMP.param_settings[ch][SUSTAIN]=raw;
  setParams(SUSTAIN,ch);const int expected=constrain(raw,toern_audio::sustainSliderMin(ch),32);
  assert(SMP.param_settings[ch][SUSTAIN]==expected);
  assert(std::abs((ch==11?sustainAmp[0][0]:env[ch].level)-expected/32.f)<.00001);
  sliderDef[ch][0][0]={ARR_PARAM,SUSTAIN,32};
  sliderDef[ch][0][1]={ARR_PARAM,ATTACK,32};
  SMP.param_settings[ch][SUSTAIN]=0;initSliders(0,ch);
  const int sMin=toern_audio::sustainSliderMin(ch);
  assert(Encoder[0].lo==sMin&&Encoder[0].value==sMin&&mode.minValues[0]==sMin);
  assert(Encoder[1].lo==0&&mode.minValues[1]==0);
 }
 for(int instrument=0;instrument<13;++instrument){preset(11,instrument);assert(SMP.param_settings[11][SUSTAIN]>=1);}
 for(int codec=13;codec<=31;++codec){
  lineOutLevelSetting=codec;lineMenuFirstEnter=true;lineMenu();
  assert(mode.pos[2]==32-codec&&sgtl5000_1.value==codec);
  for(int display=1;display<=19;++display){
   mode.pos[2]=display;lineMenu();
   assert(sgtl5000_1.value==32-display&&lineOutLevelSetting==32-display);
  }
 }
 assert(savedCodec==13);
}
'''
for key, value in [('PARAM_ENUM', parameter_enum), ('PARAMS', block(params, 'void setParams(')),
                   ('SLIDERS', block(ui, 'void initSliders(')), ('PRESETS', preset_values),
                   ('PRESET_ASSIGNMENT', preset_assignment), ('LINE_CASE', line_case)]:
    harness = harness.replace(key, value)
with tempfile.TemporaryDirectory(prefix='toern-sustain-lout-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17','-I',str(root), *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: sustain parameter/preset/encoder bounds; all 19 LOUT levels round-trip to the correct codec value.')
