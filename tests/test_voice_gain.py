"""Exercise real sample gain, filter metadata, resets and saved Device bytes."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'toern.ino').read_text()
ui=(root/'toern_filterUI.ino').read_text()
params=(root/'toern_parameters.ino').read_text()
def function(text,signature):
    start=text.index(signature);end=text.index('{',start)+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
types=source[source.index('enum ValueDisplayMode'):source.index('struct Note {')]
sample=source[source.index('struct Sample {'):source.index('// Declare the device struct')]
device=source[source.index('struct Device {'):source.index('// Global variables struct')]
harness=r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <fstream>
#include <vector>
#include "src/toern_legacy_filter.h"
using std::isfinite;
#define FLASHMEM
#define DMAMEM
#define maxY 16
#define maxFiles 9
#define maxFilters 15
#define maxPages 16
#define NUM_CHANNELS 16
#define MAX_FILTER_RESOLUTION 32.0
#define PARAM_COUNT 6
#define DEFAULT_CH11_INSTRUMENT 12
#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))
'''+types+sample+device+r'''
Device SMP={};
struct Amp {float value=1;void gain(float v){value=v;}} sampleFilterGain[8];
constexpr int SAMPLE_VOICE_GAIN_SLOT=14;
constexpr float SAMPLE_VOICE_GAIN_TAG=1048576.0f;
SliderDefEntry sliderDef[NUM_CHANNELS][FILTER_PAGE_CAP][4];
const char *waveformNames[]={""},*instTypeNames[]={""};
uint8_t sampleTimeStretch[9]={};
uint8_t kCh11AttackDefault[13]={},kCh11DecayDefault[13]={},kCh11SustainDefault[13]={},kCh11ReleaseDefault[13]={};
uint8_t kCh11Control1Default[13]={},kCh11Control2Default[13]={},kCh11Control3Default[13]={},kCh11CentSliderDefault[13]={},kCh11WaveDefault[13]={},kCh11FormDefault[13]={};
void setFilters(FilterType,int,bool){}void setParams(ParameterType,int){}
void applySynthInstrumentPreset(int,int){}void midiArpResetVoice(int){}
void updateSynthVoice(int){}void applyMonoSynthCrf(int){}void applyDefaultAdsrSettings(int){}
void setSampleTimeStretch(int,int){}
'''+ '\n'.join([
function(source,'float& filterSetting('),
function(source,'int sampleVoiceGainSlider('),
function(source,'void applySampleVoiceGain('),
function(source,'void setSampleVoiceGain('),
function(source,'int getDefaultFastFilterValue('),
function(source,'void setDefaultFastFilterValue('),
function(source,'FLASHMEM void initSliderDefTemplates() {'),
function(source,'void setSliderDefForChannel('),
function(ui,'static inline bool isBipolarCenteredParam('),
function(ui,'uint8_t readSetting('),
function(params,'static int defaultSettingValue('),
function(params,'static void setFilterPageDefaultValues('),
])+r'''
int main(int argc,char **argv){
 static_assert(sizeof(Device)==4292,"Keep the saved Device layout unchanged");
 initSliderDefTemplates();
 for(int ch:{1,2,3,4,5,6,7,8,11,13,14})setSliderDefForChannel(ch);
 for(int ch=1;ch<=8;ch++){
   assert(sliderDef[ch][3][0].arr==ARR_PARAM&&sliderDef[ch][3][0].idx==DELAY);
   const auto &v=sliderDef[ch][3][1];
   assert(v.arr==ARR_GAIN&&v.name[0]=='V'&&v.maxValue==32&&isBipolarCenteredParam(v));
   assert(readSetting(v.arr,v.idx,ch)==16); // old zero-initialized pattern = 0 / unity
   applySampleVoiceGain(ch);assert(sampleFilterGain[ch-1].value==1);
 }
 for(int ch:{11,13,14})for(int page=0;page<5;page++)for(int slot=0;slot<4;slot++)assert(sliderDef[ch][page][slot].arr!=ARR_GAIN);
 for(int value=0;value<=32;value++){
   setSampleVoiceGain(4,value);
   assert(SMP.param_settings[4][14]==SAMPLE_VOICE_GAIN_TAG+value);
   assert(readSetting(ARR_GAIN,0,4)==value);
   assert(sampleFilterGain[3].value==(float)value/16);
   for(int ch=1;ch<=8;ch++)if(ch!=4)assert(sampleFilterGain[ch-1].value==1);
 }
 // Fast-filter edits use the same raw scale and apply only the chosen voice.
 setDefaultFastFilterValue(4,ARR_GAIN,0,20);
 assert(getDefaultFastFilterValue(4,ARR_GAIN,0)==20&&sampleFilterGain[3].value==1.25f);
 setSampleVoiceGain(4,-1);assert(sampleFilterGain[3].value==0);
 setSampleVoiceGain(4,33);assert(sampleFilterGain[3].value==2);
 SMP.param_settings[11][14]=7;
 setSampleVoiceGain(11,0);assert(SMP.param_settings[11][14]==7);
 // Serialized gain survives a Device round trip; no new fields/extension needed.
 setSampleVoiceGain(3,0);setSampleVoiceGain(4,24);setSampleVoiceGain(8,32);
 Device stored=SMP;
 std::memset(&SMP,0,sizeof SMP);std::memcpy(&SMP,&stored,sizeof SMP);
 for(int ch=1;ch<=8;ch++)applySampleVoiceGain(ch);
 assert(sampleFilterGain[2].value==0&&sampleFilterGain[3].value==1.5f&&sampleFilterGain[7].value==2);
 // Resetting D/V restores both DLAY=0 and neutral gain, preserving other voices.
 SMP.param_settings[4][DELAY]=32;setFilterPageDefaultValues(4,3);
 assert(SMP.param_settings[4][DELAY]==0&&SMP.param_settings[4][14]==SAMPLE_VOICE_GAIN_TAG+16&&sampleFilterGain[3].value==1);
 assert(sampleFilterGain[2].value==0&&sampleFilterGain[7].value==2);
 // Malformed or non-finite stored values return to neutral rather than amplifying.
 for(float invalid:{-17.0f,5.0f,33.0f,SAMPLE_VOICE_GAIN_TAG+0.5f,std::numeric_limits<float>::quiet_NaN()}){
   SMP.param_settings[4][14]=invalid;applySampleVoiceGain(4);assert(sampleFilterGain[3].value==1);
 }
 // Actual shipped legacy patterns contain spare bytes, sometimes nonzero.
 // Without a tagged gain value, every sample voice must still load at unity.
 for(int file=1;file<argc;file++){
   std::ifstream in(argv[file],std::ios::binary);assert(in);
   in.seekg(4096*4+2);in.read((char *)&SMP,sizeof SMP);assert(in.gcount()==sizeof SMP);
   for(int ch=1;ch<=8;ch++){applySampleVoiceGain(ch);assert(sampleFilterGain[ch-1].value==1);}
 }
}
'''
with tempfile.TemporaryDirectory(prefix='toern-voice-gain-') as tmp:
    cpp=Path(tmp)/'gain.cpp';exe=Path(tmp)/'gain';cpp.write_text(harness)
    cmd=['c++','-std=c++17','-I',str(root)]
    sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():cmd+=['-isystem',str(sdk)]
    subprocess.run(cmd+[str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe),*[str(root/'standalone-tools/web-standalone/SD-CARD-CONTENT'/name) for name in ['1.txt','2.txt','autosaved.txt']]],check=True)
print('PASS: V beside D for voices 1–8 only; -16/0/+16 = 0/1/2x, independent voices, legacy unity, Device round trip and page reset.')
