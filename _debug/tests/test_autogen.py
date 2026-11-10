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
constexpr uint8_t NOTE_MIDI_PITCH_NONE=255,NOTE_CONDITION_GLIDE=22;
int ctrlMode=0;bool hasNotes[17]={};
struct Note {uint8_t channel=0,velocity=127,probability=100,condition=1,midiPitch=255;};
Note original[maxlen+1][maxY+1];Note (*note)[maxY+1]=original;
struct {unsigned edit=1,page=1,x=1;} GLOB;
struct Mode {unsigned pos[4]={};} draw,singleMode,menu;Mode *currentMode=&draw;
struct EncoderStub {unsigned writes=0;void writeCounter(int32_t){++writes;}} Encoder[4];
bool childLockEnabled=false,imageMode=false,touchyEnabled=false,fastRecordActive=false,isRecording=false,isNowPlaying=true;
bool paintMode=false,unpaintMode=false;
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
Note observedSource[16][33][17];unsigned sourceCalls[16]={};
void evolveCompanionChannelFromBase(int src,int target,uint8_t ch,int,bool,const CompanionContext&,const CompanionHarmony&,int){
 assert(!guardDepth&&src!=target);++generations;++sourceCalls[ch];
 for(auto &col:observedSource[ch])for(auto &n:col)n=Note{};
 for(unsigned col=0;col<maxX;++col)for(unsigned y=1;y<=maxY;++y)if(note[(src-1)*maxX+1+col][y].channel==ch){
  auto n=note[(src-1)*maxX+1+col][y];observedSource[ch][col][y]=n;n.velocity=40+(n.velocity+1)%80;note[(target-1)*maxX+1+col][y]=n;
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
  assert(toggleAutogen()&&allocations==1&&autogenActive()&&note!=original&&beat==5);
  assert(agen.source()==1&&agen.target()==2&&autogenMotifPage()==base&&autogenMuted(2));
  {
   auto editable=note;
   ToernNoteScope<decltype(note)> builder(note,agenWork->performance);
   {ToernNoteScope<decltype(note)> timer(note,autogenPlaybackGrid());assert(note==agenWork->performance);}
   assert(note==agenWork->performance);assert(autogenEditableGrid()==editable);
  }
  assert(note==agenWork->notes);
  {unsigned writes=0;for(auto &enc:Encoder)writes+=enc.writes;
   const auto x=GLOB.x,edit=GLOB.edit;
   for(unsigned i=0;i<20;++i)autogenToggleView();
   unsigned after=0;for(auto &enc:Encoder)after+=enc.writes;
   assert(after==writes&&GLOB.x==x&&GLOB.edit==edit);
  }
  Note anchor[33][17];memcpy(anchor,note+(agenWork->anchor-1)*width+1,width*sizeof(note[0]));
  const auto tonic=agenWork->harmony.rootPc;const bool minor=agenWork->harmony.isMinor;
  autogenSetMute(2,false);assert(!autogenMuted(2));
  // Starve generation for more than a phrase: never switch to unfinished data.
  for(unsigned i=0;i<width*10;++i){autogenPrepareStep();autogenAdvanceStep();assert(agen.source()==1);}
  std::set<unsigned> repeats,playedPages;
  for(unsigned i=0;i<width*160;++i){
   playedPages.insert(agen.source());
   const unsigned start=(agen.source()-1)*width+1;
   Note playing[33][17];memcpy(playing,agenWork->performance+start,width*sizeof(note[0]));
   clockMs+=50;
   const auto count=generations;serviceAutogen();assert(generations-count<=1);
   if(toern_autogen_music::section(agen.generation)==toern_autogen_music::Section::Drop)
    for(uint8_t ch:agenOrder)assert(agenWork->volumeNow[ch]==agenWork->volumeBase[ch]);
   assert(memcmp(playing,agenWork->performance+start,width*sizeof(note[0]))==0);
   assert(memcmp(saved,original,sizeof(saved))==0);
   assert(memcmp(patch.filter_settings,SMP.filter_settings,sizeof(patch.filter_settings))==0);
   assert(memcmp(patch.attack,SMP.attack,sizeof(patch.attack))==0);
   assert(agenWork->harmony.rootPc==tonic&&agenWork->harmony.isMinor==minor);
   assert(memcmp(anchor,note+(agenWork->anchor-1)*width+1,width*sizeof(note[0]))==0);
   if(agen.ready){
    const unsigned dst=(agen.target()-1)*width+1;
    for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=maxY;++row){
     const Note &n=agenWork->performance[dst+col][row];
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
  assert(playedPages==std::set<unsigned>({1,2}));
  assert(requestedFills==0); // generated fill must not stack a live-fill overlay
  for(uint8_t ch:agenOrder)assert(autogenControlVolume(ch)<=agenWork->volumeBase[ch]&&SMP.channelVol[ch]<=12);
  applyChannelControlVolume(1,5,autogenControlVolume(1));
  autogenManualVolume(1,5);assert(SMP.channelVol[1]==5);const auto manual=autogenControlVolume(1);clockMs+=100;serviceAutogen();assert(autogenControlVolume(1)==manual);
  assert(!toggleAutogen()&&!autogenActive()&&note==original&&allocations==0&&agenWork==nullptr);
  for(unsigned ch:{1u,2u,3u,4u,5u,6u,7u,8u,11u,13u,14u})assert(SMP.channelVol[ch]==12&&lastChannelVolBeforeMute[ch]==12&&!sampleVolOnReader[ch<=8?ch:0]);
  assert(GLOB.edit==base&&memcmp(saved,original,sizeof(saved))==0);
  serviceAutogen();assert(currentMode->pos[3]==GLOB.x&&currentMode->pos[1]==GLOB.edit);
  // A middle motif page requires non-contiguous work buffers; neither may be the motif.
  GLOB.edit=GLOB.page=2;GLOB.x=width+3;assert(toggleAutogen());
  assert(agen.source()==1&&agen.target()==2&&autogenMotifPage()==2&&autogenMotifSlot()==2);
  unsigned slot,otherSlot,arc;bool prepared;autogenUiSnapshot(slot,otherSlot,prepared,arc);
  assert(slot==10&&otherSlot==11);stopAutogen();
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
  for(unsigned row=1;row<=maxY;++row){third|=agenWork->performance[chordStep][row].channel==13;fifth|=agenWork->performance[chordStep][row].channel==14;}
  assert(third&&fifth);stopAutogen();assert(original[1][11].midiPitch==36);
  // Keep raw motif pitches; playback applies the same patch to motif and generated notes.
  for(auto &col:original)for(auto &n:col)n=Note{};
  original[1][11]={11,90,100,1,36};original[1][12]={11,90,100,1,40};original[1][13]={11,90,100,1,43};
  original[1][14]={13,90,100,1,60};original[1][15]={14,90,100,1,36};
  octave[0]=2;SMP.synth_settings[13][CENT]=0;SMP.synth_settings[14][CENT]=8;
  GLOB.edit=GLOB.page=1;assert(toggleAutogen());
  assert(!agenWork->ch11Bass&&agenWork->registerShift[11]==0&&agenWork->registerShift[13]==0&&agenWork->registerShift[14]==0);
  for(unsigned i=0;i<15;++i)serviceAutogen();assert(agen.ready);
  std::set<int> chord;
  const unsigned onset=(agen.target()-1)*width+1;
  for(unsigned row=1;row<=16;++row){const auto &n=agenWork->performance[onset][row];
   if(n.channel==11){const int sounding=n.midiPitch+agenWork->registerShift[11];assert(sounding>=24&&sounding<=60);chord.insert(sounding);}
   if(n.channel==13)assert(n.midiPitch+agenWork->registerShift[13]>=48);
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
    for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)assert(!agenWork->performance[dst+col][row].channel||agenWork->performance[dst+col][row].channel==3);
   }
   autogenPrepareStep();autogenAdvanceStep();
  }
  stopAutogen();for(unsigned ch=0;ch<16;++ch)testMutes[ch]=ch==2;
  // A short rise keeps the bass, then the groove (kick and bass) returns.
  // The fill and drop come once per 64-bar cycle, not at the end of every 16.
  for(auto &col:original)for(auto &n:col)n=Note{};
  for(unsigned col=1;col<=width;col+=4)original[col][7]={6,90,100,1,36};
  original[1][4]={3,90,100,1,255};GLOB.edit=GLOB.page=1;assert(toggleAutogen());
  unsigned buildTicks=0;bool sawReturn=false,sawDrop=false;
  auto previous=toern_autogen_music::section(0);
  for(unsigned tick=0;tick<width*80;++tick){serviceAutogen();
   const auto form=toern_autogen_music::section(agen.generation);
   if(form==toern_autogen_music::Section::Build){++buildTicks;assert(agen.repeatsLeft==1);assert(agenWork->volumeNow[6]>=agenWork->volumeBase[6]*12/16);}
   if(form!=previous){
    if(previous==toern_autogen_music::Section::Build&&form==toern_autogen_music::Section::Groove){assert(buildTicks==2*width);sawReturn=true;}
    if(previous==toern_autogen_music::Section::Build)buildTicks=0;
    previous=form;
   }
   if(form==toern_autogen_music::Section::Drop){sawDrop=true;break;}
   if(agen.ready&&agenWork->section==toern_autogen_music::Section::Build){bool bassHit=false;
    const unsigned dst=(agen.target()-1)*width+1;
    for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)bassHit|=agenWork->performance[dst+col][row].channel==6;
    assert(bassHit);
   }
   autogenPrepareStep();autogenAdvanceStep();
  }
  assert(sawReturn&&sawDrop);stopAutogen();
  // Pink motif edits must replace a prebuilt next page without touching playback.
  for(auto &col:original)for(auto &n:col)n=Note{};
  for(unsigned ch=0;ch<16;++ch)testMutes[ch]=false;
  original[1][2]={1,90,100,1,255};original[1][14]={13,90,100,1,60};original[1][4]={3,90,100,1,255};
  GLOB.edit=GLOB.page=1;GLOB.x=1;assert(toggleAutogen());
  assert(autogenViewingPerformance());autogenToggleView();assert(!autogenViewingPerformance());
  assert(autogenMotifPage()==1&&agen.source()==1&&agen.target()==2);
  assert(autogenRecordStep(agen.firstStep()+4)==5);
  for(unsigned i=0;i<20;++i)serviceAutogen();assert(agen.ready);
  Note live[33][17];memcpy(live,agenWork->performance+agen.firstStep(),width*sizeof(note[0]));
  note[1][14]=Note{};note[3][14]={13,79,83,3,67};
  serviceAutogen();assert(!agen.ready&&agenWork->motifRevision==1);
  assert(memcmp(live,agenWork->performance+agen.firstStep(),width*sizeof(note[0]))==0);
  for(unsigned i=0;i<20;++i)serviceAutogen();assert(agen.ready);
  std::set<uint32_t> developed;unsigned editedRolls[4]={},editedBuilds=0,editedDropRoll=0;bool editedDrop=false;
  for(unsigned response=0;response<24;++response){
    const unsigned target=(agen.target()-1)*width+1;
    const Note &input=agenWork->performance[2*width+3][14];
    assert(sourceCalls[13]==0&&input.channel==13&&input.midiPitch==67&&input.velocity==79&&input.probability==83&&input.condition==3);
    assert(agenWork->performance[2*width+1][14].channel!=13); // no fallback to the old onset/pitch
    uint32_t hash=2166136261u;
    for(unsigned col=0;col<width;++col)for(const auto &n:agenWork->performance[target+col])
      for(unsigned v:{(unsigned)n.channel,(unsigned)n.velocity,(unsigned)n.midiPitch})hash=(hash^v)*16777619u;
    developed.insert(hash);
    unsigned snare=0;for(unsigned col=0;col<width;++col){bool hit=false;for(const auto &n:agenWork->performance[target+col])hit|=n.channel==2;snare+=hit;}
    if(agenWork->section==toern_autogen_music::Section::Build){assert(editedBuilds<4);editedRolls[editedBuilds++]=snare;}
    if(agenWork->section==toern_autogen_music::Section::Drop){editedDrop=true;editedDropRoll=snare;}

    assert(agenWork->pageRevision[agen.playing^1u]==1);
    agen.repeatsLeft=1;beat=agen.firstStep()+width-1;autogenAdvanceStep();
    for(unsigned i=0;i<20;++i)serviceAutogen();assert(agen.ready);
    assert(GLOB.edit==1&&note[3][14].midiPitch==67);
  }
  assert(editedBuilds==4&&editedRolls[1]>editedRolls[0]&&editedRolls[0]>0&&editedDrop&&editedDropRoll<editedRolls[3]);
  assert(developed.size()>=3); // edited input must still pass through development
  note[width+5][14]={13,88,100,1,72};
  GLOB.edit=2;GLOB.x=width+5;autogenFocusMotif();
  for(unsigned i=0;i<20;++i)serviceAutogen();
  assert(autogenMotifPage()==2&&GLOB.edit==2&&GLOB.x==width+5);
  assert(agenWork->performance[2*width+5][14].midiPitch==72);
  assert(!(agenWork->removedVoices&(1u<<11))&&(agenWork->channels&(1u<<11))); // absent companions remain eligible after navigation
  GLOB.edit=1;GLOB.x=3;autogenFocusMotif();
  for(unsigned i=0;i<20;++i)serviceAutogen();
  assert(note[3][14].midiPitch==67);
  // Muting withdraws a stale ready page; unmuting restores initially silent levels.
  autogenSetMute(13,true);assert(!agen.ready);
  for(unsigned i=0;i<20;++i)serviceAutogen();
  unsigned target=(agen.target()-1)*width+1;
  for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)assert(agenWork->performance[target+col][row].channel!=13);
  agenWork->volumeBase[13]=0;autogenSetMute(13,false);assert(agenWork->volumeBase[13]>0&&!agen.ready);
  for(unsigned i=0;i<20;++i)serviceAutogen();
  target=(agen.target()-1)*width+1;bool revived=false;
  for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)revived|=agenWork->performance[target+col][row].channel==13;
  assert(revived);
  // Explicitly deleting the hat prevents AGEN's companion injection from re-adding it.
  note[1][4]=Note{};for(unsigned i=0;i<20;++i)serviceAutogen();
  assert(agenWork->removedVoices&(1u<<3));target=(agen.target()-1)*width+1;
  for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)assert(agenWork->performance[target+col][row].channel!=3);
  // Clearing the motif stays silent; it must not be mistaken for an initial empty starter.
  for(unsigned col=1;col<=width;++col)for(auto &n:note[col])n=Note{};
  for(unsigned i=0;i<20;++i)serviceAutogen();target=(agen.target()-1)*width+1;
  for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)assert(!agenWork->performance[target+col][row].channel);
  assert(original[1][14].midiPitch==60&&original[1][4].channel==3);
  stopAutogen();assert(note==original&&note[1][14].midiPitch==60);
  for(unsigned ch=0;ch<16;++ch)testMutes[ch]=ch==2;
  // CH11 must never expand a single note or dyad into a mandatory triad.
  for(unsigned poly:{1u,2u,3u}){
    for(auto &col:original)for(auto &n:col)n=Note{};
    for(unsigned i=0;i<poly;++i)original[1][11+i]={11,90,100,1,(uint8_t)(60+i*4)};
    original[1][2]={1,90,100,1,255};GLOB.edit=GLOB.page=1;GLOB.x=1;
    assert(toggleAutogen());
    for(unsigned tick=0;tick<width*40;++tick){serviceAutogen();
      const unsigned start=agen.firstStep();
      for(unsigned col=0;col<width;++col){unsigned count=0;
        for(unsigned row=1;row<=16;++row)count+=agenWork->performance[start+col][row].channel==11;
        assert(count<=poly);
      }
      autogenPrepareStep();autogenAdvanceStep();
    }
    stopAutogen();
  }
  // Widely spaced CH6 motif notes keep their individual source octaves.
  for(auto &col:original)for(auto &n:col)n=Note{};
  original[1][7]={6,90,100,1,48};original[width/2+1][7]={6,90,100,1,72};
  GLOB.edit=GLOB.page=1;GLOB.x=1;assert(toggleAutogen());
  for(unsigned i=0;i<20;++i)serviceAutogen();assert(agen.ready);
  for(unsigned col:{0u,width/2}){bool found=false;
    for(unsigned row=1;row<=16;++row){const auto &cell=agenWork->performance[(agen.target()-1)*width+1+col][row];
      if(cell.channel==6){found=true;assert(abs((int)cell.midiPitch-(col?72:48))<=6);}
    }
    assert(found);
  }
  stopAutogen();
  // Three held CH11 chords across the page, a held lead and a syncopated beat,
  // followed through a whole cycle: the chords reach the end of the page, stay
  // held, keep the drawn progression; solos are complete; the drop keeps the groove.
  // Vocals on voice 8, drawn or not, sing at the climaxes only and exactly as drawn.
  for(unsigned ch=0;ch<16;++ch)testMutes[ch]=false;
  for(bool drawnVocal:{false,true}){
   for(auto &col:original)for(auto &n:col)n=Note{};
   const Note vocalA={8,93,77,2,255},vocalB={8,64,100,1,255};
   if(drawnVocal){original[1][9]=vocalA;original[1+width/2][7]=vocalB;}
   const uint8_t chords[3][3]={{48,51,55},{44,48,51},{46,50,53}}; // Cm, Ab, Bb
   auto chordAt=[&](unsigned col){return min(2u,col*3/width);};
   auto headAt=[&](unsigned col){return !col||chordAt(col-1)!=chordAt(col);};
   for(unsigned col=0;col<width;++col)for(unsigned t=0;t<3;++t)
    original[1+col][11+t]={11,100,100,(uint8_t)(headAt(col)?1:NOTE_CONDITION_GLIDE),chords[chordAt(col)][t]};
   for(unsigned col=2;col<10;++col)original[1+col][15]={13,100,100,(uint8_t)(col==2?1:NOTE_CONDITION_GLIDE),62};
   uint32_t drawnKicks=0;
   for(unsigned col:{0u,3u,6u,10u}){original[1+col][2]={1,110,100,1,255};drawnKicks|=1u<<col;}
   original[5][3]={2,100,100,1,255};original[13][3]={2,100,100,1,255};
   GLOB.edit=GLOB.page=1;GLOB.x=1;for(auto &calls:sourceCalls)calls=0;
   assert(toggleAutogen());
   bool sawBreak=false,sawSolo=false,sawDrop=false;unsigned sung=0;
   for(unsigned response=0;response<26;++response){
    for(unsigned i=0;i<20;++i)serviceAutogen();assert(agen.ready);
    const unsigned target=(agen.target()-1)*width+1;const auto built=agenWork->section;
    const bool sings=toern_autogen_music::vocalWanted(agen.generation+1,built);
    uint32_t kicks=0;unsigned drumCells=0,tonalCells=0,vocalCells=0;std::set<int> previous;int lead=-1;
    for(unsigned col=0;col<width;++col){
     std::set<int> pitches;unsigned drawnPcs=0,pcs=0;
     for(unsigned t=0;t<3;++t)drawnPcs|=1u<<(chords[chordAt(col)][t]%12);
     for(unsigned row=1;row<=16;++row){
      const Note &n=agenWork->performance[target+col][row];
      if(n.channel==8){
       ++vocalCells;
       const Note expected=!drawnVocal?Note{8,100,100,20,255}:col?vocalB:vocalA; // 20 = first pass only
       assert((col==0&&row==9)||(drawnVocal&&col==width/2&&row==7));
       const bool ducked=toern_autogen_music::guestWanted(7,agen.generation+1,built)
         &&built!=toern_autogen_music::Section::Break&&built!=toern_autogen_music::Section::Build
         &&(col==0||col==3||col==6||col==10);
       const unsigned vel=ducked?expected.velocity*78/100:expected.velocity;
       assert(n.velocity==vel&&n.probability==expected.probability&&n.condition==expected.condition&&n.midiPitch==expected.midiPitch);
      }
      if(n.channel>=1&&n.channel<=5)++drumCells;
      if(n.channel==11||n.channel==13||n.channel==14)++tonalCells;
      if(n.channel==1)kicks|=1u<<col;
      if(n.channel==11){
       pitches.insert(n.midiPitch);pcs|=1u<<(n.midiPitch%12);
       assert(n.probability==100&&n.condition==(headAt(col)?1:NOTE_CONDITION_GLIDE)); // never thinned, never re-attacked
      }
      if(n.channel==13&&built!=toern_autogen_music::Section::Solo){
       assert(col>=2&&col<10&&n.probability==100&&n.condition==(col==2?1:NOTE_CONDITION_GLIDE));
       if(col==2)lead=n.midiPitch;else assert(n.midiPitch==lead); // a held note keeps one pitch
      }
     }
     if(built==toern_autogen_music::Section::Solo)continue;
     assert(pitches.size()==3&&pcs==drawnPcs);          // drawn chord on every step, both halves
     if(!headAt(col))assert(pitches==previous);          // identical tones: the chord sustains
     previous=pitches;
    }
    if(built!=toern_autogen_music::Section::Solo)assert(lead>=0);
    assert(vocalCells==(sings?drawnVocal?2u:1u:0u));sung+=sings;
    if(built==toern_autogen_music::Section::Solo){sawSolo=true;assert(!tonalCells&&drumCells);}
    if(built==toern_autogen_music::Section::Break){sawBreak=true;assert(!drumCells&&tonalCells);}
    if(built==toern_autogen_music::Section::Drop){sawDrop=true;assert(kicks==drawnKicks);} // no stock four-on-the-floor
    if(built==toern_autogen_music::Section::Groove)assert(kicks==drawnKicks);
    const unsigned builtGen=agen.generation+1;
    const bool wantBass=toern_autogen_music::guestWanted(6,builtGen,built);
    const bool wantKick=toern_autogen_music::guestWanted(7,builtGen,built);
    const bool bassRests=built==toern_autogen_music::Section::Break||built==toern_autogen_music::Section::Solo||built==toern_autogen_music::Section::Build;
    const bool drumsRest=built==toern_autogen_music::Section::Break||built==toern_autogen_music::Section::Build;
    unsigned bassNotes=0,kick2=0;
    for(unsigned col=0;col<width;++col){
     bool has1=false,has7=false,has11=false;unsigned chordVel=0;
     for(unsigned row=1;row<=16;++row){
      const Note &n=agenWork->performance[target+col][row];
      if(n.channel==1)has1=true;
      if(n.channel==6){++bassNotes;assert(n.midiPitch>=36&&n.midiPitch<60);assert(n.midiPitch%12==(chords[chordAt(col)][0]%12));}
      if(n.channel==7){has7=true;++kick2;assert(n.midiPitch==60);}
      if(n.channel==11){has11=true;chordVel=n.velocity;}
     }
     if(wantKick&&!drumsRest)assert(has7==has1);
     if(has7&&has11)assert(chordVel<=70); // the second kick ducks the chord
    }
    if(wantBass&&!bassRests)assert(bassNotes>=width/4);
    if(!wantBass)assert(bassNotes==0);
    if(!wantKick||drumsRest)assert(kick2==0);
    agen.repeatsLeft=1;beat=agen.firstStep()+width-1;autogenAdvanceStep();
   }
   assert(sawBreak&&sawSolo&&sawDrop&&sourceCalls[11]==0&&sourceCalls[1]>0);
   assert(sung==3&&sourceCalls[8]==0); // half the melodic solo and both drop pages, never evolved
   stopAutogen();
  }
  for(unsigned ch=0;ch<16;++ch)testMutes[ch]=ch==2;
  // Empty input starts a small, coherent set rather than all voices at once.
  for(auto &col:original)for(auto &n:col)n=Note{};
  GLOB.edit=GLOB.page=1;assert(toggleAutogen());
  for(unsigned i=0;i<15;++i)serviceAutogen();
  assert(agen.ready);unsigned channels=0;for(unsigned ch=1;ch<15;++ch)if(agenWork->channels&(1u<<ch))channels++;
  assert(channels==5); // selected set includes CH2, but its muted notes are never built
  autogenClearMotif(0);for(unsigned i=0;i<20;++i)serviceAutogen();
  const unsigned silent=(agen.target()-1)*width+1;
  for(unsigned col=0;col<width;++col)for(unsigned row=1;row<=16;++row)assert(!agenWork->performance[silent+col][row].channel);
  stopAutogen();for(auto &col:original)for(auto &n:col)assert(n.channel==0);
 }
 puts("PASS: AGEN 16/32-step boundaries, tempo/page-aware repeats, late-builder safety, one-voice jobs, untouched live/source grids, editable motif, pending-page invalidation, developed edited sources, mute/unmute, removed voices, held CH11 progressions, held leads, full solos, kept drop groove, climax-only vocals on voice 8, exact restoration, empty starters and incompatible-mode guards");
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
