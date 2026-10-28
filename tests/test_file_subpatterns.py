"""Exercise real A–E switching/storage/UI with failure-injectable SD.

Includes the former single-file confirmation and browser-identity regressions:
NO/YES/cancel, selected targets, partial kits, WAV snapshots, cache reuse,
transaction failures, loaded ownership, and Save As rollback.
"""
from pathlib import Path
import subprocess, tempfile, sys
root=Path(__file__).resolve().parents[1]
main=(root/'toern.ino').read_text()
ops=(root/'toern_fileoperations.ino').read_text()
slots=(root/'toern_pattern_slots.ino').read_text()
loader=ops[ops.index('FLASHMEM void loadPattern'):ops.index('FLASHMEM void autoLoad')]
confirm_buttons=main[main.index('  if (currentMode == &patternConfirmMode) {',main.index('FLASHMEM void checkMode')):main.index('  //checkFastRec();')]
fn_delete=main[main.index('  if (currentMode == &loadSaveTrack && currentMode->pos[3] > 1 && fnEvents[1] == 4)'):main.index('  // Page row: FN + encoder 2 long')]
renderer=main[main.index('FLASHMEM void showLoadSave()'):main.index('FLASHMEM void showPresetLoadSave() {')]
sample=main[main.index('struct Sample {'):main.index('// Declare the device struct')]
device=main[main.index('struct Device {'):main.index('// Global variables struct')]
state=main[main.index('// File subpatterns are separate'):main.index('// End file subpattern declarations.')]
validator=ops[ops.index('FLASHMEM bool validPatternFile'):ops.index('FLASHMEM void saveSamplePack')]
# Includes the actual writer helpers and header, without unrelated sample loading.
helpers=ops[ops.index('static bool lastPatternSaveOk'):ops.index('FLASHMEM bool validPatternFile')]
harness=r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include "toern_pattern_slots.h"
#include "pattern_transaction.h"
#include "toern_grid_paint.h"
using std::min;
#define EXTMEM
#define DMAMEM
#define FLASHMEM
#define __DMB() ((void)0)
constexpr unsigned MAX_STEPS=256;
const unsigned maxlen=257,maxY=16,maxPages=16,maxFiles=9,maxFilters=15;
unsigned maxX=16;
const uint8_t defaultVelocity=100,NOTE_MIDI_PITCH_NONE=255;
struct Note {uint8_t channel,velocity,probability,condition,midiPitch;};
Note noteBanks[6][maxlen+1][maxY+1];
Note (*note)[maxY+1]=noteBanks[0];
SAMPLE
DEVICE
Device SMP;
bool globalMutes[maxY]={},pageMutes[maxPages][maxY]={};
struct Mode {unsigned pos[4]={};} draw,menu,singleMode,loadSaveTrack,patternConfirmMode,set_Wav,set_SamplePack;Mode *currentMode=&draw;
struct {unsigned x=1,page=1,edit=1;bool singleMode=false;} GLOB;
struct Encoder {uint32_t rgb=0;void writeCounter(int32_t){}void writeMax(int32_t){}void writeRGBCode(uint32_t color){rgb=color;}} Encoder[4];
const int NUM_ENCODERS=4;bool pressed[4]={};bool bypassModeSwitchDebounce=false;
bool fnHeld=false,fastRecordActive=false,isRecording=false,childLockEnabled=false,imageMode=false;
bool isNowPlaying=false,voiceMode=false,songModeActive=false,preventPaintUnpaint=false;
unsigned lastPage=1,beat=1,beatForUI=1,loopCount=1,pendingPage=0;
uint8_t voicePageLen[16]={},voiceModePages=1;
uint16_t voicePageMask[16]={},voiceRowMask[16]={},voiceLoopCount[16]={};
bool hasNotes[maxPages+1]={};int loopLength=0,editpage=1,ctrlMode=0;
unsigned long now=100;unsigned long millis(){return now;}
namespace toern_audio {struct StateGuard {};}
struct CRGB {uint8_t r=0,g=0,b=0;CRGB(int v=0):r(v){}operator int()const{return r;}};
const int UI_RED=1,UI_GREEN=2,UI_BLUE=3,UI_WHITE=4,UI_BRIGHT_GREEN=5,UI_DIM_RED=6,ICON_FOLDER_BIG=0;
CRGB getIndicatorColor(char){return {};}
CRGB getCurrentChannelColor(){return {};}
CRGB normalizeToMaxBrightness(CRGB c){return c;}
void getIndicatorXPositions(int enc,int &a,int &b,int &c){b=enc==2?6:10;a=b-1;c=b+1;}
struct PixelCall {int x,y,color;};std::vector<PixelCall> pixelCalls;
void light(int x,int y,CRGB c){pixelCalls.push_back({x,y,(int)c});}void showIconsAt(int,CRGB,int,int){}
std::vector<int> indicatorCalls;void drawIndicator(char,char,int enc,bool,bool){indicatorCalls.push_back(enc);}int numberY=0;void drawNumber(unsigned,CRGB,int y){numberY=y;}
struct TextCall {std::string text;int x,y,color;};std::vector<TextCall> textCalls;
void FastLEDclear(){}void FastLEDshow(){}void drawNoSD(){}
void drawText(const char *s,int x,int y,CRGB c){textCalls.push_back({s,x,y,(int)c});}
char overlayBuf[16]={};
void drawCenteredOverlayLabel(const char *s,int){snprintf(overlayBuf,sizeof(overlayBuf),"%s",s);}
int successReports=0,failureReports=0;
void showSaveSuccessAnimation(bool){successReports++;}
void showSaveFailureAnimation(bool){failureReports++;}
int cancellations=0;void showCanceledAnimation(bool back){cancellations++;if(back)currentMode=&draw;}
void switchMode(Mode *mode){currentMode=mode;}void sdIoYield(){}void sdIoBeginAudioSafe(){}void sdIoEndAudioSafe(){}
std::string capturedSample;
bool captureSelectedSampleForDelete(char *out,size_t size){if(capturedSample.empty())return false;snprintf(out,size,"%s",capturedSample.c_str());return true;}
void sampleBrowserInvalidate(){}
int savedKit=-1,kitSaveCalls=0;void saveSamplePack(int pack){savedKit=pack;++kitSaveCalls;switchMode(&draw);}
void restoreSampleBrowserSelection(unsigned selection){set_Wav.pos[3]=selection;}
void stopSdPreviewIfPlaying(){}void applyDrawSingleYEncoderLimits(){}void refreshCtrlEncoderConfig(){}
int effectivePageCount(){return 16;}int encoderPageMax(){return 16;}
bool SMP_LOAD_SETTINGS=false;
int clockMode=1,soundLoads=0;double playNoteInterval=0;Mode bpmMode;
struct Timer {void update(double){}} playTimer,fillTimer;
using std::isfinite;
void unmuteAllChannels(){}void applyMutesAfterPMODSwitch(){}void delay(int){}
void showNewFileScreen(){}void syncBpmEncoderPositions(){}
void updateLastPage(){
 memset(hasNotes,0,sizeof(hasNotes));
 for(unsigned x=1;x<maxlen;++x)for(unsigned y=1;y<=maxY;++y)
  if(note[x][y].channel)hasNotes[(x-1)/maxX+1]=true;
}
void loadSMPSettings(){soundLoads++;}
STATE
const int O_WRITE=1,O_CREAT=2,O_TRUNC=4,FILE_READ=0,FILE_WRITE_BEGIN=2;
extern bool enforceBudget;extern unsigned stepReads,stepWrites,stepEntries,maxRead,maxWrite;
unsigned sdWrites=0;
struct File {
 std::shared_ptr<std::vector<uint8_t>> data;size_t pos=0;bool fail=false;
 std::shared_ptr<std::vector<std::string>> entries;size_t dirpos=0;std::string filename;
 explicit operator bool()const{return !!data||!!entries;}
 bool isDirectory(){return !!entries;}const char *name(){return filename.c_str();}
 File openNextFile();
 size_t size(){return data?data->size():0;}
 int available(){return size()-pos;}
 bool seek(size_t n){pos=n;return n<=size();}
 int read(){return available()?(*data)[pos++]:-1;}
 int read(uint8_t *out,size_t n){if(enforceBudget){++stepReads;maxRead=std::max(maxRead,(unsigned)n);assert(n<=512);}if(!data)return -1;n=min(n,size()-pos);memcpy(out,data->data()+pos,n);pos+=n;return n;}
 size_t write(const uint8_t *in,size_t n){if(enforceBudget){++stepWrites;maxWrite=std::max(maxWrite,(unsigned)n);assert(n<=512);}sdWrites++;if(fail)return 0;if(pos+n>size())data->resize(pos+n);memcpy(data->data()+pos,in,n);pos+=n;return n;}
 bool truncate(uint64_t size=0){if(fail||!data)return false;data->resize(size);pos=std::min(pos,(size_t)size);return true;}
 void close(){data.reset();entries.reset();}
};
struct FS {
 std::map<std::string,std::shared_ptr<std::vector<uint8_t>>> files;bool failWrite=false;std::string failRemove,failWritePath,failRenameOnceTo;
 std::set<std::string> dirs;
 unsigned existsCalls=0,rootScans=0;
 bool exists(const char *p){existsCalls++;return files.count(p)||dirs.count(p);}
 bool remove(const char *p){if(failRemove==p)return false;return files.erase(p)>0;}
 bool rmdir(const char *p){std::string prefix=std::string(p)+"/";for(auto &v:files)if(v.first.find(prefix)==0)return false;return dirs.erase(p)>0;}
 bool rename(const char*a,const char*b){if(failRenameOnceTo==b){failRenameOnceTo.clear();return false;}if(!exists(a)||exists(b))return false;files[b]=files[a];files.erase(a);return true;}
 File open(const char*p,int mode=0){
  if(!strcmp(p,"/")){rootScans++;File f;f.entries=std::make_shared<std::vector<std::string>>();for(auto &pair:files)f.entries->push_back(pair.first);return f;}
  if(dirs.count(p)){File f;f.entries=std::make_shared<std::vector<std::string>>();return f;}
  if(mode&O_CREAT){if(!exists(p)||mode&O_TRUNC)files[p]=std::make_shared<std::vector<uint8_t>>();}
  if(!exists(p))return {};return {files[p],0,failWrite||failWritePath==p};}
} SD;
File File::openNextFile(){if(enforceBudget)++stepEntries;if(!entries||dirpos==entries->size())return {};auto n=(*entries)[dirpos++];File f;f.data=SD.files[n];f.filename=n;return f;}
HELPERS
VALIDATOR
SLOTS
LOADER
RENDERER
bool match_buttons(const uint8_t *b,int a,int c,int d,int e){return b[0]==a&&b[1]==c&&b[2]==d&&b[3]==e;}
void dispatchConfirmButtons(const uint8_t currentButtonStates[4]) {
CONFIRMBUTTONS
}
void dispatchFnDelete(const uint8_t fnEvents[4]) {
FNDELETE
}
void selectSlot(int delta) {fnHeld=true;subpatternTurn(delta);fnHeld=false;subpatternFnRelease();}
void drainSlots(){for(int i=0;i<8;++i)serviceSubpatterns();}


unsigned pollCount=0;bool enforceBudget=false;unsigned stepReads=0,stepWrites=0,stepEntries=0,maxRead=0,maxWrite=0;
void tick(){stepReads=stepWrites=stepEntries=0;enforceBudget=true;serviceSubpatterns();enforceBudget=false;
 assert(stepReads<=1&&stepWrites<=1&&stepEntries<=1);++pollCount;++now;}
void pump(unsigned n=1000){for(unsigned i=0;i<n;++i)tick();}
void ready(){for(unsigned i=0;i<10000&&(!subpatternGridReady||!subpatternViewOn)&&!subpatternErrorUntil;++i)tick();assert(subpatternGridReady);}
void family(){for(unsigned i=0;i<30000&&patternFamilyBusy();++i)tick();assert(!patternFamilyBusy());}
void flush(){for(unsigned i=0;i<30000&&(subpatternWriteJob.busy()||subpatternDirtyMask);++i)tick();assert(!subpatternWriteJob.busy()&&!subpatternDirtyMask);}
void choose(int delta){fnHeld=true;subpatternTurn(delta);ready();fnHeld=false;subpatternFnRelease();
 for(unsigned i=0;i<1000&&!subpatternReady&&!subpatternApplied;++i)tick();
 if(isNowPlaying){subpatternBoundary=true;assert(subpatternCommitAtStepStart());}else if(subpatternReady)assert(subpatternCommitAtStepStart());
 tick();}
void reload(unsigned file){currentMode=&draw;resetFileSubpattern();SMP.file=file;loadPattern(false,file);subpatternErrorUntil=0;}
int main(){
 static_assert(sizeof(Device)==4292,"Keep on-card settings layout compatible");
 for(auto &bank:noteBanks)memset(bank,0xA5,sizeof(bank));
 clearPatternNotes();SMP.file=1;SMP.bpm=120;GLOB.x=3;
 assert(!subpatternsEnabled);fnHeld=true;subpatternTurn(1);fnHeld=false;assert(!subpatternSelector);
 subpatternsEnabled=true;subpatternFile=1;subpatternCache[0]=note;note[1][2]={3,127,100,1,60};
 updateLastPage();savePattern(false);assert(lastPatternSaveOk&&validPatternFile("1.txt",true));
 SD.files["1-B.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 (*SD.files["1-B.txt"])[4]=7;
 // Cold preview/staging never finishes a whole file in a single loop pass.
 isNowPlaying=true;auto oldGrid=note;const unsigned writesBefore=sdWrites;
 fnHeld=true;subpatternTurn(1);tick();assert(!subpatternGridReady&&note==oldGrid);
 ready();assert(pollCount>30&&sdWrites==writesBefore&&note==oldGrid);
 assert(subpatternViewedNote(1,2)&&subpatternViewedNote(1,2)->channel==7);
 fnHeld=false;subpatternFnRelease();tick();assert(subpatternReady&&note==oldGrid);
 // Tick boundary publishes a pointer and small precomputed metadata only.
 stepReads=stepWrites=0;enforceBudget=true;subpatternBoundary=true;
 assert(subpatternCommitAtStepStart());enforceBudget=false;
 assert(!stepReads&&!stepWrites&&note!=oldGrid&&note[1][2].channel==7&&subpatternSlot==1);
 tick();note[1][2].velocity=88;
 choose(-1);assert(note==oldGrid&&note[1][2].channel==3);
 choose(1);assert(note[1][2].velocity==88&&sdWrites==writesBefore);
 // Stopped new-slot switching and all automatic writes remain cooperative.
 isNowPlaying=false;choose(1);assert(subpatternSlot==2&&note[1][2].channel==0);
 flush();assert(SD.exists("1-C.txt")&&validPatternFile("1-C.txt",true));
 // Missing primary must recover a valid backup, never install a new blank.
 SD.files["1-B.txt.bak"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1-B.txt"]);
 SD.files.erase("1-B.txt");reload(1);isNowPlaying=true;
 choose(1);assert(subpatternSlot==1&&note[1][2].channel==7&&note[1][2].velocity==88);
 assert(!(subpatternEmptyMask&2)&&!SD.exists("1-B.txt"));
 isNowPlaying=false;flush();assert(validPatternFile("1-B.txt",true));
 // A malformed marker is rejected by both the normal and staged loaders.
 SD.files["1-D.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1-B.txt"]);
 (*SD.files["1-D.txt"])[MAX_STEPS*maxY*4]=0;
 assert(!validPatternFile("1-D.txt",false));oldGrid=note;fnHeld=true;subpatternTurn(1);subpatternTurn(1);
 for(unsigned i=0;i<10000&&!subpatternErrorUntil;++i)tick();
 fnHeld=false;assert(subpatternErrorUntil&&note==oldGrid&&subpatternSlot==1&&!subpatternReady);
 SD.files.erase("1-D.txt");subpatternErrorUntil=0;
 // VMOD uses the actual selected bank, not live per-voice ownership mapping.
 voiceMode=true;isNowPlaying=true;fnHeld=true;subpatternTurn(1);ready();
 assert(subpatternPreviewVisible()&&subpatternViewedNote(1,2)->channel==0);
 fnHeld=false;subpatternFnRelease();tick();subpatternBoundary=true;assert(subpatternCommitAtStepStart());tick();
 voiceMode=false;isNowPlaying=false;flush();
 // A failed first save must remain retryable even if temporary cleanup also failed.
 SubpatternWriter freshWriter;Device freshSettings=SMP;freshSettings.file=99;
 SD.failWritePath="99.txt.tmp";SD.failRemove="99.txt.tmp";
 freshWriter.begin("99.txt",note,freshSettings,0,0,FILE_WRITE_BEGIN);
 for(unsigned i=0;i<10000&&freshWriter.busy();++i){stepReads=stepWrites=0;enforceBudget=true;freshWriter.tick(SD);enforceBudget=false;assert(stepReads<=1&&stepWrites<=1);}
 assert(freshWriter.result()==SubpatternIoResult::Failed&&!SD.exists("99.txt")&&SD.exists("99.txt.tmp"));
 SD.failWritePath.clear();SD.failRemove.clear();freshWriter.begin("99.txt",note,freshSettings,0,0,FILE_WRITE_BEGIN);
 for(unsigned i=0;i<10000&&freshWriter.busy();++i){stepReads=stepWrites=0;enforceBudget=true;freshWriter.tick(SD);enforceBudget=false;assert(stepReads<=1&&stepWrites<=1);}
 assert(freshWriter.result()==SubpatternIoResult::Valid&&validPatternFile("99.txt",true));
 // Existing temporary content is truncated rather than appended or left trailing.
 SD.files["99.txt.tmp"]=std::make_shared<std::vector<uint8_t>>(SubpatternWriter::totalBytes*2,42);
 freshWriter.begin("99.txt",note,freshSettings,0,0,FILE_WRITE_BEGIN);
 for(unsigned i=0;i<10000&&freshWriter.busy();++i){stepReads=stepWrites=0;enforceBudget=true;freshWriter.tick(SD);enforceBudget=false;assert(stepReads<=1&&stepWrites<=1);}
 assert(freshWriter.result()==SubpatternIoResult::Valid&&SD.files["99.txt"]->size()==SubpatternWriter::totalBytes);
 // Save As carries the latest resident grids and adopts identity only at commit.
 saveSelectedPatternFamily(2,false);assert(patternFamilyBusy());
 while(patternFamilyBusy()){assert(SMP.file==1);tick();}
 assert(lastPatternSaveOk&&SMP.file==2&&subpatternFile==2);
 assert(validPatternFile("2.txt",true)&&validPatternFile("2-B.txt",true)&&validPatternFile("2-C.txt",true));
 assert((*SD.files["2-B.txt"])[5]==88);
 // Rollback restores every target primary after a late rename failure.
 for(const char *path:{"3.txt","3-B.txt","3-C.txt"})SD.files[path]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 const auto oldA=*SD.files["3.txt"],oldB=*SD.files["3-B.txt"],oldC=*SD.files["3-C.txt"];
 SD.failRenameOnceTo="3-B.txt";saveSelectedPatternFamily(3,true);family();
 assert(!lastPatternSaveOk&&SMP.file==2&&*SD.files["3.txt"]==oldA&&*SD.files["3-B.txt"]==oldB&&*SD.files["3-C.txt"]==oldC);
 // Failed quiet writes retain RAM and retry without blocking input polling.
 subpatternDirtyMask=2;SD.failWritePath="2-B.txt.tmp";const unsigned pollsBefore=pollCount;
 for(unsigned i=0;i<10000&&subpatternWritingSlot!=1;++i)tick();
 while(subpatternWriteJob.busy())tick();
 assert(subpatternDirtyMask&2);assert(subpatternCache[1][1][2].velocity==88&&pollCount>pollsBefore);
 SD.failWritePath.clear();subpatternErrorUntil=0;now+=1501;flush();
 // Restarting transport abandons an incomplete automatic temp safely and unlocks switching.
 subpatternDirtyMask=2;const unsigned beforeResume=sdWrites;
 for(unsigned i=0;i<10000&&sdWrites==beforeResume;++i)tick();
 assert(subpatternWriteJob.busy());isNowPlaying=true;tick();
 assert(!subpatternWriteJob.busy()&&(subpatternDirtyMask&2));
 choose(-2);isNowPlaying=false;flush();
 // Family deletion preserves A if companion cleanup fails and preserves other live families.
 deleteSelectedPatternFamily(3);family();assert(!SD.exists("3.txt")&&SMP.file==2);
 SD.failRemove="2-B.txt";deleteSelectedPatternFamily(2);family();
 assert(!lastPatternSaveOk&&SD.exists("2.txt")&&SMP.file==2);
 SD.failRemove.clear();deleteSelectedPatternFamily(2);family();
 assert(lastPatternSaveOk&&!SD.exists("2.txt")&&!SD.exists("2-B.txt")&&deletedSubpatternSlots==31);
 for(unsigned x=1;x<maxlen;++x)for(unsigned y=1;y<=maxY;++y)assert(!note[x][y].channel);
 savePattern(true);SMP.file=0xA5A5A5A5;resetFileSubpattern();loadPattern(true);
 assert(SMP.file==2&&subpatternFile==2&&deletedSubpatternSlots==31);
 // NO/YES confirmation and browser selection remain independent from job execution.
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=2;openPatternConfirmation(1,false);
 const unsigned beforeNo=sdWrites;finishPatternConfirmation(false);
 assert(!patternFamilyBusy()&&sdWrites==beforeNo&&currentMode==&loadSaveTrack&&loadSaveTrack.pos[3]==2);
 // Snapshot-selected WAV deletion and partial-kit overwrite guards are preserved.
 SD.files["samples/a.wav"]=std::make_shared<std::vector<uint8_t>>(8,1);
 SD.files["samples/b.wav"]=std::make_shared<std::vector<uint8_t>>(8,2);
 currentMode=&set_Wav;set_Wav.pos[3]=7;capturedSample="samples/a.wav";requestBrowserDelete();
 capturedSample="samples/b.wav";patternConfirmMode.pos[3]=1;finishPatternConfirmation(true);
 assert(!SD.exists("samples/a.wav")&&SD.exists("samples/b.wav")&&set_Wav.pos[3]==7);
 SD.files["8/7.wav"]=std::make_shared<std::vector<uint8_t>>(8,3);currentMode=&set_SamplePack;set_SamplePack.pos[3]=8;
 requestKitSave(8);assert(currentMode==&patternConfirmMode&&kitSaveCalls==0);finishPatternConfirmation(false);
 assert(currentMode==&set_SamplePack&&set_SamplePack.pos[3]==8&&kitSaveCalls==0);
 requestKitSave(8);patternConfirmMode.pos[3]=1;finishPatternConfirmation(true);assert(savedKit==8&&kitSaveCalls==1);
 // Uncached siblings copy in bounded blocks as part of Save As.
 SD.files["1-E.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 reload(1);saveSelectedPatternFamily(4,false);family();assert(lastPatternSaveOk&&SMP.file==4);
 assert(*SD.files["4-E.txt"]==*SD.files["1-E.txt"]);
 // Save As also recovers an uncached sibling that only has a valid backup.
 SD.files["1-E.txt.bak"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1-E.txt"]);SD.files.erase("1-E.txt");
 reload(1);saveSelectedPatternFamily(5,false);family();assert(lastPatternSaveOk&&SMP.file==5);
 assert(*SD.files["5-E.txt"]==*SD.files["1-E.txt.bak"]);
 // Raw sibling copying truncates stale target temporaries too.
 SD.files["7-E.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1-E.txt.bak"]);
 SD.files["7-E.txt.tmp"]=std::make_shared<std::vector<uint8_t>>(60000,42);
 reload(1);saveSelectedPatternFamily(7,true);family();assert(lastPatternSaveOk&&*SD.files["7-E.txt"]==*SD.files["1-E.txt.bak"]);
 // A recovery-only destination still requires overwrite confirmation, before any write.
 SD.files["6-D.txt.bak"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=7;const unsigned recoveryPromptWrites=sdWrites;
 requestPatternSave(6);family();assert(currentMode==&patternConfirmMode&&patternConfirmTarget==6&&sdWrites==recoveryPromptWrites);
 finishPatternConfirmation(false);assert(currentMode==&loadSaveTrack&&loadSaveTrack.pos[3]==7);
 // Browser indexing is incremental; render never rescans the directory.
 currentMode=&loadSaveTrack;++patternBrowserRevision;const unsigned rootBefore=SD.rootScans;
 loadSaveTrack.pos[3]=5;showLoadSave();assert(SD.rootScans==rootBefore);tick();
 for(unsigned i=0;i<1000&&!subpatternIndexValid;++i)tick();
 assert(subpatternIndexValid&&savedSubpatternMask(4)==23&&savedSubpatternMask(6)==8);
 const unsigned scanned=SD.rootScans;for(unsigned i=0;i<20;++i){showLoadSave();tick();}assert(SD.rootScans==scanned);
 for(unsigned width:{16u,32u}){maxX=width;pixelCalls.clear();drawPatternSlotDots(0x05,UI_GREEN);assert(pixelCalls.size()==3);assert(pixelCalls[0].x==(width==32?10:5));}maxX=16;
 // Content-aware dots trim trailing empty files but retain internal empty letters.
 const auto blank=*SD.files["1-C.txt"],filled=*SD.files["1.txt"];
 for(unsigned familyNumber:{20u,21u})for(uint8_t slot=0;slot<5;++slot){
  char path[24];toern_pattern_slots::path(path,sizeof(path),familyNumber,slot);
  SD.files[path]=std::make_shared<std::vector<uint8_t>>(familyNumber==20&&slot==2?filled:blank);
 }
 // An old nonempty backup must not override a valid, intentionally emptied E.
 SD.files["20-E.txt.bak"]=std::make_shared<std::vector<uint8_t>>(filled);
 ++patternStorageRevision;currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=21;
 for(unsigned i=0;i<5000&&(subpatternContentRevision!=patternStorageRevision||subpatternContentKnown[20]!=31);++i)tick();
 assert(subpatternContentKnown[20]==31&&subpatternContentMask(20)==4);
 pixelCalls.clear();drawPatternSlotDots(subpatternContentMask(20),UI_GREEN);assert(pixelCalls.size()==3);
 assert(SD.exists("20-D.txt")&&SD.exists("20-E.txt"));
 loadSaveTrack.pos[3]=22;
 for(unsigned i=0;i<5000&&subpatternContentKnown[21]!=31;++i)tick();
 assert(subpatternContentKnown[21]==31&&subpatternContentMask(21)==0);
 pixelCalls.clear();drawPatternSlotDots(subpatternContentMask(21),UI_GREEN);assert(pixelCalls.size()==1);
 // The latest RAM contents override saved data, including an empty active E.
 loadSaveTrack.pos[3]=SMP.file+1;
 auto emptyBank=subpatternFreeBank();assert(emptyBank);memset(emptyBank,0,sizeof(noteBanks[0]));
 subpatternCache[4]=emptyBank;note=emptyBank;subpatternSlot=4;
 for(unsigned i=0;i<5000&&subpatternContentKnown[SMP.file]!=31;++i)tick();
 const uint8_t latestMask=subpatternContentMask(SMP.file);
 assert(!(latestMask&16)&&toern_pattern_slots::displayCount(latestMask)==2);
 // Rendering only consults RAM/cache; it never reads pattern files for counting.
 stepReads=stepWrites=0;enforceBudget=true;showLoadSave();enforceBudget=false;assert(!stepReads&&!stepWrites);
 puts("PASS: bounded 512-byte jobs, cooperative previews/index/save/delete/recovery, cached VMOD grids, constant-time bank switches, retained live edits, transactional rollback, retries and autosave identity.");
}
'''.replace('SAMPLE',sample).replace('DEVICE',device).replace('STATE',state).replace('HELPERS',helpers).replace('VALIDATOR',validator).replace('SLOTS',slots).replace('LOADER',loader).replace('RENDERER',renderer).replace('FNDELETE',fn_delete).replace('CONFIRMBUTTONS',confirm_buttons)
with tempfile.TemporaryDirectory(prefix='toern-subpatterns-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(harness)
 flags=['c++','-std=c++17','-O1','-Wno-deprecated-declarations','-fsanitize=address,undefined','-I',str(root/'src'),'-I',str(root)]
 if sys.platform=='darwin':flags+=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1']
 subprocess.run(flags+[str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
