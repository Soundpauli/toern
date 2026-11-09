"""Run AGEN's real lifecycle/builder against a sequencer, including late jobs."""
from pathlib import Path
import subprocess,sys,tempfile
root=(Path(__file__).resolve().parents[2] / 'platformio')
helpers=(root/'toern_helpers.ino').read_text()
def function(signature):
 start=helpers.index(signature);end=helpers.index('{',start)+1;depth=1
 while depth:
  depth+=(helpers[end]=='{')-(helpers[end]=='}');end+=1
 return helpers[start:end]
control_source=(root/'toern.ino').read_text()
def control_function(signature):
 start=control_source.index(signature);end=control_source.index('{',start)+1;depth=1
 while depth:
  depth+=(control_source[end]=='{')-(control_source[end]=='}');end+=1
 return control_source[start:end]
volume_control=control_function('FLASHMEM void applyChannelControlVolume(')
placement='\n'.join(function(s) for s in ['static inline uint8_t clampVel(', 'static inline void placeGenNote(', 'FLASHMEM static int companionFreeRow(unsigned int step, int preferred) {', 'FLASHMEM static int companionChannelStepLimit(', 'FLASHMEM static bool companionPlaceCellLimited('])
implementation=helpers[helpers.index('// AGEN: RAM-only alternating performance pages.'):]
code=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <set>
#include <cmath>
#include <cstdlib>
#include "src/toern_build_types.h"
using std::min;using std::max;
#define constrain(v,a,b) std::clamp((v),(a),(b))
#define FLASHMEM
#define EXTMEM
#define PROGMEM
unsigned allocations=0;bool allocationAllowed=true;
void *extmem_malloc(size_t size){if(!allocationAllowed)return nullptr;allocations++;return malloc(size);}
void extmem_free(void *p){--allocations;free(p);}
constexpr unsigned maxlen=257,maxY=16,MAX_STEPS=256;
unsigned maxX=16,beat=1,beatForUI=1,loopCount=1,lastPage=1,editpage=1;
int octave[2]={4,4},transpose=0,SampleRate[16]={};float detune[16]={},channelOctave[16]={};
constexpr uint8_t NOTE_MIDI_PITCH_NONE=255;
int ctrlMode=0;bool hasNotes[17]={};
struct Note {uint8_t channel=0,velocity=127,probability=100,condition=1,midiPitch=255;};
Note original[maxlen+1][maxY+1];Note (*note)[maxY+1]=original;
struct {unsigned edit=1,page=1,x=1;} GLOB;
struct Mode {unsigned pos[4]={};} draw,singleMode,menu;Mode *currentMode=&draw;
struct EncoderStub {void writeCounter(int32_t){}} Encoder[4];
bool childLockEnabled=false,imageMode=false,touchyEnabled=false,fastRecordActive=false,isRecording=false,isNowPlaying=true;
bool generationUndoValid=true,storageReady=true;int failures=0,generations=0,guardDepth=0;
namespace toern_audio {struct StateGuard {StateGuard(){++guardDepth;}~StateGuard(){--guardDepth;}};}
unsigned clockMs=100;unsigned millis(){return clockMs;}
constexpr int HCUT=0,LOWCUT=1,REVERB=2,CENT=0,maxfilterResolution=32;
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
constexpr float CH11_SYNTHMIX_GAIN=.30f;
struct {unsigned channelVol[16]={},mute[16]={};float bpm=120;float filter_settings[16][3]={},attack[16]={},synth_settings[16][1]={};} SMP;
uint8_t lastChannelVolBeforeMute[16]={};bool sampleVolOnReader[9]={},neoSliderInMuteDz[8]={};
float sampleTriggerAmp[9]={1,1,1,1,1,1,1,1,1},ampLevel[16]={},readerLevel[9]={};
struct Amp {} testAmps[15];Amp *amps[15]={};
struct Voice {} testVoices[9];Voice *voices[9]={};
bool autogenActive();bool autogenMuted(int);void autogenSetMute(int,bool);
bool getMuteState(int ch){return autogenActive()?autogenMuted(ch):ch==2;}
void setMuteState(int ch,bool muted){autogenSetMute(ch,muted);}
void stopMutedVoiceAudio(int){}
bool isNeoSliderControlledChannel(int){return false;}
float playAmpForVoice(int){return 1.0f;}
float sampleChannelVolScale(int ch){return sampleVolOnReader[ch]?SMP.channelVol[ch]/16.0f:1.0f;}
void applyVoiceAmpGainFromVol(int ch,int vol){ampLevel[ch]=ch<=8&&sampleVolOnReader[ch]?1.0f:vol/16.0f;}
void applyPlayAmplitude(int ch,float level){if(ch<=8)readerLevel[ch]=level;}
unsigned requestedFills=0;bool queueAutogenFill(int ch){assert(ch>=2&&ch<=5);++requestedFills;return true;}
void cancelAutogenFills(){}
VOLUME_CONTROL
void showSaveFailureAnimation(bool){++failures;}
void menuRequestFullRedraw(){}void noteUserActivity(){}
void play(bool){isNowPlaying=true;}
void updateLastPage(){assert(note==original);}
int companionEffectivePages(){return MAX_STEPS/maxX;}
bool testMutes[16]={false,false,true};
bool getMuteStateForUI(int ch){return testMutes[ch];}
bool autogenStorageReady(){return storageReady;}
bool isChildVoiceDisabled(int){return false;}
bool companionChannelSupported(unsigned ch){return ch>=1&&ch<=14&&ch!=9&&ch!=10&&ch!=12;}
uint32_t rng=7;
unsigned random(unsigned high){rng=rng*1664525u+1013904223u;return (rng>>8)%high;}
unsigned random(unsigned low,unsigned high){return low+random(high-low);}
CompanionContext analyzeCompanionContext(int first,int last,int page,int){
 assert(guardDepth==0&&first==last&&last==page);CompanionContext c;
 for(unsigned x=(page-1)*maxX+1;x<=page*maxX;++x)for(unsigned y=1;y<=maxY;++y){const unsigned ch=note[x][y].channel;if(ch<16&&ch)c.channelHitCount[ch]++;}
 return c;
}
CompanionHarmony companionMakeHarmony(const CompanionContext&,int){return {};}
CompanionGroove companionMakeGroove(const CompanionContext&,int,int){return {};}
void clearCompanionPage(int page){assert(!guardDepth);for(unsigned x=(page-1)*maxX+1;x<=page*maxX;++x)for(auto &n:note[x])n=Note{};}
void evolveCompanionChannelFromBase(int src,int target,uint8_t ch,int,bool,const CompanionContext&,const CompanionHarmony&,int){
 assert(!guardDepth&&src!=target);++generations;
 for(unsigned col=0;col<maxX;++col)for(unsigned y=1;y<=maxY;++y)if(note[(src-1)*maxX+1+col][y].channel==ch){
  auto n=note[(src-1)*maxX+1+col][y];n.velocity=40+(n.velocity+1)%80;note[(target-1)*maxX+1+col][y]=n;
 }
}
void generateCompanionChannelPage(int target,uint8_t ch,const CompanionContext&,const CompanionHarmony&,const CompanionGroove&,int){
 assert(!guardDepth);++generations;note[(target-1)*maxX+1][ch].channel=ch;
}
PLACEMENT
int companionPitchClassForRow(unsigned ch,int row){return (48+row-(int)ch-1+120)%12;}
IMPLEMENTATION
int main(){
 for(unsigned width:{16u,32u}){
  maxX=width;
  for(unsigned ch=0;ch<16;++ch){SMP.channelVol[ch]=12;SMP.mute[ch]=ch==2;lastChannelVolBeforeMute[ch]=12;if(ch<15)amps[ch]=&testAmps[ch];if(ch<=8)voices[ch]=&testVoices[ch];}
  for(unsigned ch=1;ch<15;++ch){SMP.filter_settings[ch][HCUT]=24;SMP.filter_settings[ch][LOWCUT]=0;SMP.filter_settings[ch][REVERB]=8;SMP.attack[ch]=2;}
  SMP.filter_settings[11][REVERB]=0;
  const auto patch=SMP;
  allocationAllowed=false;assert(!toggleAutogen()&&allocations==0&&note==original);allocationAllowed=true;
  for(unsigned ch:{1u,2u,3u,4u,5u,6u,11u,13u,14u})for(unsigned col=0;col<width;++col)for(unsigned seed=0;seed<12;++seed){
   const auto chance=ToernAutogen::noteChance(ch,col,seed);assert(chance>=60&&chance<=100);
   if(ch==1&&col%4==0)assert(chance==100);
  }
  ToernAutogen schedule;schedule.start(1,width,MAX_STEPS,4);schedule.publish(8);
  unsigned step=1;
  for(unsigned i=1;i<4*width;++i){step=schedule.advance(step);assert(schedule.source()==1);}
  step=schedule.advance(step);assert(schedule.source()==2&&step==width+1&&schedule.repeatsLeft==8);
  schedule.publish(1);
  for(unsigned i=1;i<8*width;++i){step=schedule.advance(step);assert(schedule.source()==2);}
  step=schedule.advance(step);assert(schedule.source()==1&&step==1&&schedule.repeatsLeft==1);
  // Last physical page wraps its extra page to 1 without touching stored data.
  for(auto &col:original)for(auto &n:col)n=Note{};
  const unsigned base=MAX_STEPS/width;
  for(unsigned x=(base-1)*width+1;x<=base*width;++x){original[x][1]={1,93,87,2,61};original[x][11]={11,81,100,1,67};}
  original[1][7]={7,99,100,1,71};
  Note saved[maxlen+1][maxY+1];memcpy(saved,original,sizeof(saved));
  GLOB.edit=GLOB.page=base;GLOB.x=beat=(base-1)*width+5;
  assert(toggleAutogen()&&allocations==1&&autogenActive()&&note!=original&&beat==(base-1)*width+5);
  assert(agen.source()==base&&agen.target()==1&&autogenMuted(2));
  Note anchor[33][17];memcpy(anchor,note+(agenWork->anchor-1)*width+1,width*sizeof(note[0]));
  const auto tonic=agenWork->harmony.rootPc;const bool minor=agenWork->harmony.isMinor;
  autogenSetMute(2,false);assert(!autogenMuted(2));
  // Starve generation for more than a phrase: never switch to unfinished data.
  for(unsigned i=0;i<width*10;++i){autogenPrepareStep();autogenAdvanceStep();assert(agen.source()==base);}
  std::set<unsigned> repeats,playedPages;
  for(unsigned i=0;i<width*160;++i){
   playedPages.insert(agen.source());
   const unsigned start=(agen.source()-1)*width+1;
   Note playing[33][17];memcpy(playing,note+start,width*sizeof(note[0]));
   clockMs+=50;
   const auto count=generations;serviceAutogen();assert(generations-count<=1);
   if(toern_autogen_music::section(agen.generation)==toern_autogen_music::Section::Drop)
    for(uint8_t ch:agenOrder)assert(agenWork->volumeNow[ch]==agenWork->volumeBase[ch]);
   assert(memcmp(playing,note+start,width*sizeof(note[0]))==0);
   assert(memcmp(saved,original,sizeof(saved))==0);
   assert(memcmp(patch.filter_settings,SMP.filter_settings,sizeof(patch.filter_settings))==0);
   assert(memcmp(patch.attack,SMP.attack,sizeof(patch.attack))==0);
   assert(agenWork->harmony.rootPc==tonic&&agenWork->harmony.isMinor==minor);
   assert(memcmp(anchor,note+(agenWork->anchor-1)*width+1,width*sizeof(note[0]))==0);
   if(agen.ready){
    const unsigned dst=(agen.target()-1)*width+1;
    for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=maxY;++row){
     const Note &n=note[dst+col][row];
     if(n.channel==11||n.channel==13||n.channel==14){
      assert(n.midiPitch<=127);bool inKey=false;
      for(unsigned degree=0;degree<7;++degree)inKey|=n.midiPitch%12==toern_autogen_music::scalePc(tonic,minor,degree);
      assert(inKey);
     }
    }
   }
   const unsigned generation=agen.generation;autogenPrepareStep();autogenAdvanceStep();
   if(agen.generation!=generation)repeats.insert((unsigned)agen.repeatsLeft);
  }
  assert(agen.generation>16&&repeats==(width==16?std::set<unsigned>({1,2,4}):std::set<unsigned>({1,2})));
  assert(playedPages==std::set<unsigned>({base,1}));
  assert(requestedFills==0); // generated fill must not stack a live-fill overlay
  for(uint8_t ch:agenOrder)assert(autogenControlVolume(ch)<=agenWork->volumeBase[ch]&&SMP.channelVol[ch]<=12);
  applyChannelControlVolume(1,5,autogenControlVolume(1));
  autogenManualVolume(1,5);assert(SMP.channelVol[1]==5);const auto manual=autogenControlVolume(1);clockMs+=100;serviceAutogen();assert(autogenControlVolume(1)==manual);
  assert(!toggleAutogen()&&!autogenActive()&&note==original&&allocations==0&&agenWork==nullptr);
  for(unsigned ch:{1u,2u,3u,4u,5u,6u,7u,8u,11u,13u,14u})assert(SMP.channelVol[ch]==12&&lastChannelVolBeforeMute[ch]==12&&!sampleVolOnReader[ch<=8?ch:0]);
  assert(GLOB.edit==base&&memcmp(saved,original,sizeof(saved))==0);
  serviceAutogen();assert(currentMode->pos[3]==GLOB.x&&currentMode->pos[1]==GLOB.edit);
  // A paused / incompatible / recording performance restores its original bank.
  isNowPlaying=false;assert(toggleAutogen()&&isNowPlaying);isNowPlaying=false;serviceAutogen();assert(note==original&&!autogenActive());
  isNowPlaying=true;assert(toggleAutogen());maxX=width==16?32:16;serviceAutogen();assert(note==original&&!autogenActive());maxX=width;
  fastRecordActive=true;assert(!toggleAutogen());fastRecordActive=false;
  storageReady=false;assert(!toggleAutogen());storageReady=true;
  // A low CH11 bass keeps its register; spare monos supply the chord third/fifth.
  for(auto &col:original)for(auto &n:col)n=Note{};
  original[1][11]={11,90,100,1,36};GLOB.edit=GLOB.page=1;
  assert(toggleAutogen());for(unsigned i=0;i<15;++i)serviceAutogen();
  assert(agenWork->chordPair&&agen.ready);
  const unsigned chordStep=(agen.target()-1)*width+1;bool third=false,fifth=false;
  for(unsigned row=1;row<=maxY;++row){third|=note[chordStep][row].channel==13;fifth|=note[chordStep][row].channel==14;}
  assert(third&&fifth);stopAutogen();assert(original[1][11].midiPitch==36);
  // Low-register source chords stay polyphonic; compensate the patch once.
  for(auto &col:original)for(auto &n:col)n=Note{};
  original[1][11]={11,90,100,1,36};original[1][12]={11,90,100,1,40};original[1][13]={11,90,100,1,43};
  original[1][14]={13,90,100,1,60};original[1][15]={14,90,100,1,36};
  octave[0]=2;SMP.synth_settings[13][CENT]=0;SMP.synth_settings[14][CENT]=8;
  GLOB.edit=GLOB.page=1;assert(toggleAutogen());
  assert(!agenWork->ch11Bass&&agenWork->registerShift[11]==-24&&agenWork->registerShift[13]==-24&&agenWork->registerShift[14]==-12);
  for(unsigned i=0;i<15;++i)serviceAutogen();assert(agen.ready);
  std::set<int> chord;
  const unsigned onset=(agen.target()-1)*width+1;
  for(unsigned row=1;row<=16;++row){const auto &n=note[onset][row];
   if(n.channel==11){const int sounding=n.midiPitch+agenWork->registerShift[11];assert(sounding>=48&&sounding<=84);chord.insert(sounding);}
   if(n.channel==13)assert(n.midiPitch+agenWork->registerShift[13]>=55);
   if(n.channel==14)assert(n.midiPitch+agenWork->registerShift[14]>=36);
  }
  assert(chord.size()==3&&*chord.rbegin()-*chord.begin()<=12);
  assert(autogenReferenceMidi(Note{},13,14)==60); // grid home C4, not C3
  stopAutogen();octave[0]=4;
  // A manually muted arrangement must never manufacture a hat/snare-only build.
  for(auto &col:original)for(auto &n:col)n=Note{};
  for(unsigned ch=1;ch<15;++ch)testMutes[ch]=ch!=3;
  original[1][4]={3,90,100,1,255};GLOB.edit=GLOB.page=1;
  assert(toggleAutogen());
  for(unsigned tick=0;tick<width*100;++tick){serviceAutogen();
   assert(agenWork->foundationVoice==0);
   if(agen.ready){assert(agenWork->section!=toern_autogen_music::Section::Build&&agenWork->section!=toern_autogen_music::Section::Fill);
    const unsigned dst=(agen.target()-1)*width+1;
    for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)assert(!note[dst+col][row].channel||note[dst+col][row].channel==3);
   }
   autogenPrepareStep();autogenAdvanceStep();
  }
  stopAutogen();for(unsigned ch=0;ch<16;++ch)testMutes[ch]=ch==2;
  // Keep a real bass under the build; the next generation is the drop, with no Fill wait.
  for(auto &col:original)for(auto &n:col)n=Note{};
  for(unsigned col=1;col<=width;col+=4)original[col][7]={6,90,100,1,36};
  original[1][4]={3,90,100,1,255};GLOB.edit=GLOB.page=1;assert(toggleAutogen());
  unsigned buildTicks=0;bool sawDrop=false;
  for(unsigned tick=0;tick<width*100;++tick){serviceAutogen();
   const auto form=toern_autogen_music::section(agen.generation);
   if(form==toern_autogen_music::Section::Build){++buildTicks;assert(agen.repeatsLeft==1);assert(agenWork->volumeNow[6]>=agenWork->volumeBase[6]*12/16);}
   if(buildTicks&&form==toern_autogen_music::Section::Drop){assert(buildTicks==2*width);sawDrop=true;break;}
   if(agen.ready&&agenWork->section==toern_autogen_music::Section::Build){bool bassHit=false;
    const unsigned dst=(agen.target()-1)*width+1;
    for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)bassHit|=note[dst+col][row].channel==6;
    assert(bassHit);
   }
   autogenPrepareStep();autogenAdvanceStep();
  }
  assert(sawDrop);stopAutogen();
  // Empty input starts a small, coherent set rather than all voices at once.
  for(auto &col:original)for(auto &n:col)n=Note{};
  GLOB.edit=GLOB.page=1;assert(toggleAutogen());
  for(unsigned i=0;i<15;++i)serviceAutogen();
  assert(agen.ready);unsigned channels=0;for(unsigned ch=1;ch<15;++ch)if(agenWork->channels&(1u<<ch))channels++;
  assert(channels==5); // selected set includes CH2, but its muted notes are never builtstopAutogen();for(auto &col:original)for(auto &n:col)assert(n.channel==0);
 }
 puts("PASS: AGEN 16/32-step boundaries, tempo/page-aware repeats, late-builder safety, one-voice jobs, untouched live/source grids, exact restoration, empty starters and incompatible-mode guards");
}
'''.replace('IMPLEMENTATION',implementation).replace('PLACEMENT',placement).replace('VOLUME_CONTROL',volume_control)
with tempfile.TemporaryDirectory(prefix='toern-autogen-') as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
main=(root/'toern.ino').read_text();slots=(root/'toern_pattern_slots.ino').read_text();ops=(root/'toern_fileoperations.ino').read_text()
assert 'if(mainSetting==69){toggleAutogen();' in main
assert 'void pause(bool skipSave) {\n  stopAutogen();' in main
assert 'if(autogenActive())return;' in slots
assert 'FLASHMEM void savePattern(bool autosave) {\n  stopAutogen();' in ops
