"""Exercise real MUTE controls, EEPROM decoding, mute setters and note gates."""
from pathlib import Path
import subprocess,sys,tempfile
root=Path(__file__).resolve().parents[2]/'platformio'
main=(root/'toern.ino').read_text();menu=(root/'toern_menu.ino').read_text()
def function(s,signature):
 a=s.index(signature);e=s.index('{',a)+1;depth=1
 while depth:
  depth+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
def case(section,number):
 a=menu.index(f'case {number}:',menu.index(section));e=menu.index('{',a)+1;b=e;depth=1
 while depth:
  depth+=(menu[e]=='{')-(menu[e]=='}');e+=1
 return menu[b:e-1]
loader=menu[menu.index('  // Byte 21:'):menu.index('  // Load stereoChannel',menu.index('  // Byte 21:'))]
parts='\n'.join(function(main,signature) for signature in ['bool getMuteState(int channel) {','static void stopMutedVoiceAudio(int ch) {','FLASHMEM void setMuteState(int channel, bool muted) {','static void silenceAllVoicesForFullMute() {','static void syncMuteArraysToSmp() {','FLASHMEM void applyMutesAfterPMODSwitch() {','FLASHMEM void fullMuteApplyView() {','void triggerGridNote(unsigned int globalX, unsigned int y, bool allowMuted) {'])
parts+='\n'+function(menu,'static inline int userChToInternalDrawR(')+'\n'+function(menu,'static inline int internalToUserChDrawR(')+'\n'+function(menu,'void toggleSelectedMuteMaskChannel(){')
code=r'''
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#define FLASHMEM
#define constrain(v,a,b) std::clamp((v),(a),(b))
using std::min;
constexpr int maxY=16,maxPages=2,maxlen=1,EEPROM_DATA_START=43;
bool muteNext=false,childLockEnabled=false,SMP_PATTERN_MODE=false,menuFirstEnter=true,muteMenuTimingFocus=false;
int patternMode=-1,drawMode=0;
struct {unsigned edit=1,page=1;} GLOB;
bool globalMutes[16]={},pageMutes[2][16]={};
struct {bool globalMutes[16]={},pageMutes[2][16]={};unsigned mute[16]={};} SMP;
struct {unsigned pos[4]={};} mode,*currentMode=&mode;
struct {uint8_t data[128]={};uint8_t read(int address){return data[address];}} EEPROM;
void saveSingleModeToEEPROM(int index,int8_t value){EEPROM.data[43+index]=value;}
uint16_t drawRFullMuteCustomUnmuteMask=6;
void saveSettingsU16ToEEPROM(int index,uint16_t value){assert(index==34&&value==drawRFullMuteCustomUnmuteMask);}
void menuRequestFullRedraw(){}void redrawMain(int){}
struct Knob {int low=0,high=0,value=0;void writeMin(int n){low=n;}void writeMax(int n){high=n;}void writeCounter(int n){value=n;}} Encoder[4];
struct CRGB {int r=0,g=0,b=0;};
const CRGB UI_WHITE{255,255,255},UI_GREEN{0,255,0},UI_RED{255,0,0};
int dots=0;CRGB valueColor;
CRGB currentMenuParentTextColor(){return {};}
std::string title,value;
void drawMenuText(const char *s,int,int,CRGB){title=s;}
void drawMenuValue(const char *s,int,int,CRGB c){value=s;valueColor=c;}
void drawMenuIndicator(char,char,int){}void light(int,int y,CRGB c){if(y==9&&(c.r||c.g||c.b))++dots;}
bool sounding[16]={};unsigned cuts[16]={},triggers=0;int pressedKeyCount[16]={};
void samplePolyStopAll(int ch){sounding[ch]=false;++cuts[ch];}
void stopSound(int,int){sounding[11]=false;++cuts[11];}
void stopSynthChannel(int ch){sounding[ch]=false;++cuts[ch];}
bool autogenActive(){return false;}bool autogenMuted(int){return false;}void autogenSetMute(int,bool){}
bool isChildVoiceMuted(int){return false;}bool isChildVoiceDisabled(int){return false;}
void unmuteAllChannels(){}
enum class FullMuteView {AllMuted,Ch12,Custom,UnmuteAll};FullMuteView fullMuteView=FullMuteView::AllMuted;
struct Note {int channel=0,velocity=100;uint8_t midiPitch=255;};Note note[2][17];
void onNoteTriggered(int){}bool shouldSkipVoiceTriggerForSyncPreview(int){return false;}
void triggerSetWavSyncPreview(uint8_t){}bool isChannelSampleReloadBusy(unsigned){return false;}
int samplePitchForNote(const Note&,int,int){return 60;}int sampleVoiceOctaveSemis(int){return 0;}
void triggerSamplerVoice(int,int,int,bool){++triggers;}
int ch11PitchForNote(const Note&,int){return 60;}
void playSound(int,int,int){++triggers;}bool noteHasMidiPitch(const Note &n){return n.midiPitch<=127;}
void playSynthMidi(int,uint8_t,int,bool){++triggers;}void playSynth(int,int,int,bool){++triggers;}
constexpr int defaultVelocity=100;
PARTS
void load(){LOADER}
void control(){int setting=47;switch(setting){case 47:{CONTROLS}}}
void render(){switch(47){case 47:{RENDER}}}
int main(){
 for(int stored=0;stored<4;++stored){EEPROM.data[64]=stored;load();assert(drawMode==(stored&1)&&muteNext==bool(stored&2));}
 EEPROM.data[64]=255;load();assert(drawMode==0&&!muteNext&&EEPROM.data[64]==0);
 control();render();assert(title=="CH1"&&value=="ON");
 assert(Encoder[0].low==1&&Encoder[0].high==16&&Encoder[1].high==0&&Encoder[2].high==1);
 assert(currentMode->pos[0]==16);
 currentMode->pos[0]=15;control();render();assert(title=="CH2");
 currentMode->pos[0]=1;control();toggleSelectedMuteMaskChannel();render();
 assert(title=="CH16"&&value=="ON"&&(drawRFullMuteCustomUnmuteMask&1));
 const unsigned mask=drawRFullMuteCustomUnmuteMask;drawMode=1;currentMode->pos[2]=1;control();dots=0;render();assert(dots==0);
 assert(title=="MUTE"&&value=="NEXT"&&drawRFullMuteCustomUnmuteMask==mask&&EEPROM.data[64]==3);
 toggleSelectedMuteMaskChannel();render();assert(title=="CH16"&&value=="MUTE"&&muteNext&&valueColor.r==255&&valueColor.g==0);
 for(bool next:{false,true})for(int ch:{1,8,11,13,14}){
  muteNext=next;SMP_PATTERN_MODE=false;globalMutes[ch]=false;sounding[ch]=true;cuts[ch]=0;
  note[1][1].channel=ch;setMuteState(ch,true);
  assert(getMuteState(ch)&&sounding[ch]==next&&(cuts[ch]==0)==next);
  const auto before=triggers;triggerGridNote(1,1,false);assert(triggers==before);
  setMuteState(ch,false);triggerGridNote(1,1,false);assert(triggers==before+1);
  // Editing a nonplaying NEXT page must never stop the playing page.
  SMP_PATTERN_MODE=true;patternMode=3;GLOB.edit=2;GLOB.page=1;
  pageMutes[0][ch]=pageMutes[1][ch]=false;sounding[ch]=true;cuts[ch]=0;
  setMuteState(ch,true);assert(!getMuteState(ch)&&sounding[ch]&&cuts[ch]==0);
  GLOB.edit=1;setMuteState(ch,true);assert(getMuteState(ch)&&sounding[ch]==next);
 }
 SMP_PATTERN_MODE=false;
 for(bool next:{false,true}){
  muteNext=next;for(int ch:{1,8,11,13,14})sounding[ch]=true;
  fullMuteView=FullMuteView::AllMuted;fullMuteApplyView();
  for(int ch:{1,8,11,13,14})assert(getMuteState(ch)&&sounding[ch]==next);
  fullMuteView=FullMuteView::Custom;drawRFullMuteCustomUnmuteMask=2;fullMuteApplyView();
  assert(!getMuteState(1)&&getMuteState(8));
 }
}
'''.replace('PARTS',parts).replace('LOADER',loader).replace('CONTROLS',case('FLASHMEM bool handleAdditionalFeatureControls(',47)).replace('RENDER',case('FLASHMEM void drawMainSettingStatus(',47))
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: MUTE selection/click, INST/NEXT persistence and legacy defaults; sample/synth tails, new-note gating, edit/play page separation, full/custom mutes.')
