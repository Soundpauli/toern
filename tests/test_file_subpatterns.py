"""Exercise actual firmware slot/save/validation code with an in-memory SD card."""
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
state=main[main.index('// File subpatterns are separate'):main.index('static unsigned long sampleGateOverlayUntil')]
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
using std::min;
#define EXTMEM
#define DMAMEM
#define FLASHMEM
#define __DMB() ((void)0)
const unsigned maxlen=257,maxY=16,maxPages=16,maxX=16,maxFiles=9,maxFilters=15;
const uint8_t defaultVelocity=100,NOTE_MIDI_PITCH_NONE=255;
struct Note {uint8_t channel,velocity,probability,condition,midiPitch;};
Note note[maxlen+1][maxY+1];
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
const int O_WRITE=1,O_CREAT=2,O_TRUNC=4,FILE_READ=0;
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
 int read(uint8_t *out,size_t n){if(!data)return -1;n=min(n,size()-pos);memcpy(out,data->data()+pos,n);pos+=n;return n;}
 size_t write(const uint8_t *in,size_t n){sdWrites++;if(fail)return 0;if(pos+n>size())data->resize(pos+n);memcpy(data->data()+pos,in,n);pos+=n;return n;}
 void close(){}
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
File File::openNextFile(){if(!entries||dirpos==entries->size())return {};auto n=(*entries)[dirpos++];File f;f.data=SD.files[n];f.filename=n;return f;}
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
void finishSwitch(){drainSlots();subpatternBoundary=true;assert(subpatternCommitAtStepStart());serviceSubpatterns();}
int main(){
 SMP.file=1;SMP.bpm=100;GLOB.x=9;draw.pos[3]=9;
 // SUBP defaults to OFF: FN+encoder 2 does not open the subpattern selector.
 assert(!subpatternsEnabled&&!subpatternOverlayVisible());
 fnHeld=true;subpatternTurn(1);assert(!subpatternSelector&&!subpatternOverlayVisible());fnHeld=false;
 subpatternsEnabled=true;
 for(auto &row:note)for(auto &cell:row)cell={0,100,100,1,255};
 note[1][2]={1,112,88,4,61};note[256][3]={2,100,100,1,255};
 savePattern(false);assert(lastPatternSaveOk&&SD.exists("1.txt"));
 savePattern(false);assert(SD.exists("1.txt.bak"));
 isNowPlaying=true;
 fnHeld=true;subpatternTurn(1);serviceSubpatterns();
 assert(subpatternOverlayVisible());drawSubpatternOverlay();assert(!strcmp(overlayBuf,"B1"));
 assert(subpatternViewOn&&subpatternShowingOtherSlot()&&subpatternSlot==0&&note[1][2].channel==1);
 fnHeld=false;subpatternFnRelease();
 assert(subpatternReady==false&&note[1][2].channel==1);
 finishSwitch();
 assert(subpatternSlot==1&&note[1][2].channel==0&&beat==1&&!hasNotes[1]&&GLOB.x==9);
 // Switching slots while playing never writes to SD.
 drainSlots();
 assert(SD.exists("1.txt")&&!SD.exists("1-A.txt")&&!SD.exists("1-B.txt"));
 // Switching slots while paused DOES save the outgoing slot and blank target.
 isNowPlaying=false;
 selectSlot(-1);drainSlots();
 assert(subpatternSlot==0&&note[1][2].channel==1&&note[1][2].midiPitch==61&&GLOB.x==9);
 assert(SD.exists("1-B.txt")&&SD.exists("1.txt")&&!SD.exists("1-A.txt"));
 selectSlot(1);drainSlots();
 assert(!SD.exists("1-A.txt")&&SD.exists("1-B.txt")&&SD.exists("1.txt")&&SD.exists("1.txt.bak"));
 note[32][5]={4,90,100,2,72};
 selectSlot(-1);drainSlots();
 assert(subpatternSlot==0&&note[1][2].channel==1&&note[1][2].midiPitch==61&&GLOB.x==9);
 assert(hasNotes[1]&&hasNotes[16]&&lastPage==16&&voicePageLen[2]==16&&voicePageMask[2]==0x8000);
 isNowPlaying=true;
 selectSlot(1);finishSwitch();
 assert(subpatternSlot==1&&note[32][5].channel==4&&note[32][5].midiPitch==72);
 // A failed outgoing save when paused keeps the new grid on screen and retries.
 isNowPlaying=false;
 note[23][4]={7,111,100,1,70};
 SD.failWrite=true;selectSlot(-1);
 assert(subpatternSlot==0&&note[1][2].channel==1&&subpatternOldSavePending);
 drainSlots();assert(subpatternOldSavePending&&note[1][2].channel==1);
 // Resuming must defer the retry and preserve the only copy of the outgoing edits.
 SD.failWrite=false;isNowPlaying=true;now+=2000;
 const unsigned writesBeforeResume=sdWrites;drainSlots();
 assert(subpatternOldSavePending&&subpatternGrid[23][4].channel==7);
 selectSlot(1);drainSlots();
 assert(subpatternSlot==0&&!subpatternSelector&&!subpatternReady);
 assert(subpatternOldSavePending&&subpatternGrid[23][4].midiPitch==70&&sdWrites==writesBeforeResume);
 isNowPlaying=false;drainSlots();assert(!subpatternOldSavePending&&sdWrites>writesBeforeResume);
 selectSlot(1);drainSlots();assert(note[23][4].channel==7&&note[23][4].midiPitch==70);
 isNowPlaying=true;
 // A truncated target never replaces the live grid, including its visible page.
 selectSlot(-1);finishSwitch();
 SD.files["1-C.txt"]=std::make_shared<std::vector<uint8_t>>(3,0);
 fnHeld=true;subpatternTurn(1);subpatternTurn(1);assert(subpatternSlot==0&&note[1][2].channel==1);
 fnHeld=false;subpatternFnRelease();drainSlots();
 assert(subpatternSlot==0&&note[1][2].channel==1&&!subpatternReady);
 SD.remove("1-C.txt");
 selectSlot(1);finishSwitch();
 // Page dots for the open family are known without scanning on the FN turn.
 note[1][2]={1,112,88,4,61};note[256][3]={2,100,100,1,255};hasNotes[1]=hasNotes[16]=true;
 subpatternNotePagesLoaded();fnHeld=true;subpatternTurn(0);
 uint16_t pageMask=0;assert(subpatternBrowsePageMask(&pageMask)&&(pageMask&1)&&(pageMask&(1u<<15)));
 subpatternSelector=false;
 // Existing base patterns are not renamed just by browsing or selecting A.
 resetFileSubpattern();SMP.file=7;subpatternFile=7;
 savePattern(false);assert(SD.exists("7.txt"));
 fnHeld=true;subpatternTurn(1);subpatternTurn(-1);fnHeld=false;subpatternFnRelease();
 assert(SD.exists("7.txt")&&!SD.exists("7-A.txt"));
 // FILE re-entry resolves the numbered A; sound settings OFF still works.
 resetFileSubpattern();SMP.file=1;SMP_LOAD_SETTINGS=false;loadPattern(false);
 assert(subpatternFile==1&&subpatternSlot==0&&note[1][2].midiPitch==61&&soundLoads==0);
 ctrlMode=1;draw.pos[1]=12;isNowPlaying=true;selectSlot(1);finishSwitch();drainSlots();assert(subpatternSlot==1&&draw.pos[1]==12);ctrlMode=0;
 savePattern(true);assert(lastPatternSaveOk);
 resetFileSubpattern();SMP.file=9;clearPatternNotes();loadPattern(true);
 assert(subpatternFile==1&&subpatternSlot==1&&SMP.file==1&&note[32][5].midiPitch==72&&soundLoads==1);
 // Switching between two slots that both have notes on the current page stays on that page.
 GLOB.edit=16;GLOB.page=16;editpage=16;GLOB.x=250;draw.pos[3]=250;
 note[256][5]={4,90,100,2,72};
 isNowPlaying=false;selectSlot(-1);drainSlots();
 assert(subpatternSlot==0&&GLOB.edit==16&&GLOB.page==16&&editpage==16&&GLOB.x==250&&beat==241);
 // Switching to a slot without notes on that page falls back to the first page with notes in that subpattern.
 GLOB.edit=4;GLOB.page=4;editpage=4;GLOB.x=57;draw.pos[3]=57;
 selectSlot(1);drainSlots();
 assert(subpatternSlot==1&&GLOB.edit==2&&GLOB.page==2&&editpage==2&&GLOB.x==25&&beat==17);
 // While FN+encoder 2 selector is open, encoder 3 steps only through pages with notes in that subpattern.
 isNowPlaying=false;GLOB.edit=3;GLOB.page=3;editpage=3;
 fnHeld=true;subpatternTurn(-1);serviceSubpatterns();
 drawSubpatternOverlay();assert(!strcmp(overlayBuf,"A1"));
 subpatternPageTurn(1);serviceSubpatterns();
 drawSubpatternOverlay();assert(!strcmp(overlayBuf,"A16"));
 fnHeld=false;subpatternFnRelease();drainSlots();
 assert(subpatternSlot==0&&GLOB.edit==16&&GLOB.page==16&&editpage==16&&GLOB.x==249);
 GLOB.edit=1;GLOB.page=1;editpage=1;GLOB.x=9;draw.pos[3]=9;
 // A always resolves directly to the numbered filename without scanning B-E.
 resetFileSubpattern();
 char destination[24];patternDestination(destination,sizeof(destination),1);
 assert(!strcmp(destination,"1.txt"));
 // Existing B is part of pattern 2 without a separate -A marker.
 SMP.file=2;resetFileSubpattern();
 for(auto &row:note)for(auto &cell:row)cell={0,100,100,1,255};
 note[1][4]={3,100,100,1,255};
 savePattern(false);assert(SD.exists("2.txt")&&!SD.exists("2-A.txt"));
 SD.files["2-B.txt"]=SD.files["1-B.txt"];
 loadPattern(false);assert(subpatternFile==2&&subpatternSlot==0);
 isNowPlaying=true;
 fnHeld=true;subpatternTurn(1);serviceSubpatterns();
 assert(subpatternViewOn&&subpatternView[16][5].channel==4);
 fnHeld=false;subpatternFnRelease();finishSwitch();
 assert(subpatternSlot==1&&note[32][5].channel==4&&note[1][4].channel==0);
 // Browsing / trying to load a missing number keeps the loaded identity until NEW is accepted.
 isNowPlaying=false;loadPattern(false,5);
 assert(subpatternFile==2&&subpatternSlot==1&&SMP.file==2&&pendingNewPatternFile==5);
 beginNewPattern();assert(subpatternFile==5&&SMP.file==5&&subpatternSlot==0&&pendingNewPatternFile==0);
 // Saving a different numbered A does not delete existing B-E companions.
 resetFileSubpattern();SMP.file=2;savePattern(false);
 assert(lastPatternSaveOk&&SD.exists("2.txt")&&SD.exists("2-B.txt")&&!SD.exists("2-A.txt"));
 // A suffixed -A file is ignored even when the canonical numbered file is absent.
 SD.files["3-A.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 SD.files["3-A.txt.bak"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 resetFileSubpattern();SMP.file=3;
 patternDestination(destination,sizeof(destination),3);assert(!strcmp(destination,"3.txt"));
 assert(!openSubpatternFile(3,0));loadPattern(false);
 assert(subpatternFile==0&&subpatternSlot==0&&!SD.exists("3.txt")&&SD.exists("3-A.txt"));
 // Canonical transaction recovery never consults the unrelated -A file.
 SD.files["4.txt.bak"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 SD.files["4-A.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 (*SD.files["4-A.txt"])[4]=7;
 resetFileSubpattern();SMP.file=4;loadPattern(false);
 assert(subpatternSlot==0&&note[1][2].channel==1&&SD.exists("4.txt"));
 savePattern(false);assert(lastPatternSaveOk&&SD.exists("4-A.txt"));
 // Old autosave family flags are ignored; the slot identity is still restored.
 isNowPlaying=true;selectSlot(1);finishSwitch();savePattern(true);
 (*SD.files["autosaved.txt"])[SD.files["autosaved.txt"]->size()-2]=1;
 resetFileSubpattern();loadPattern(true);assert(subpatternFile==4&&subpatternSlot==1);
 // Browsing another family must never redirect the active letter, save, or autosave identity.
 isNowPlaying=false;resetFileSubpattern();SMP.file=9;
 SD.files["9.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 SD.files["9-B.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1-B.txt"]);
 SD.files["10.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 SD.files["10-C.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 SD.files["10-D.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 loadPattern(false);selectSlot(1);drainSlots();assert(subpatternSlot==1);
 const auto otherA=*SD.files["10.txt"], otherC=*SD.files["10-C.txt"], otherD=*SD.files["10-D.txt"];
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=11;now+=100;
 textCalls.clear();showLoadSave();
 assert(SMP.file==9&&subpatternFile==9&&subpatternSlot==1&&numberY==11);
 std::string displayed;for(auto &c:textCalls)if(c.color==UI_WHITE)displayed+=c.text;
 assert(displayed.empty());
 unsigned greenDots=0;for(auto p:pixelCalls)if(p.y==7&&p.x>=5&&p.color==UI_GREEN)++greenDots;assert(greenDots==3);
 const unsigned existsBefore=SD.existsCalls;showLoadSave();assert(SD.existsCalls==existsBefore);
 // A detent performs just the target lookup, never a five-letter family rescan.
 const unsigned writesBeforeBrowse=sdWrites, scansBeforeBrowse=SD.rootScans;
 for(unsigned ui=12;ui<22;++ui) {
   const unsigned before=SD.existsCalls;
   loadSaveTrack.pos[3]=ui;now+=100;pixelCalls.clear();showLoadSave();
   if(ui==15){unsigned redDots=0;for(auto p:pixelCalls)if(p.y==7&&p.x>=5&&p.color==UI_RED)++redDots;assert(redDots==2);}
   assert(SD.rootScans==scansBeforeBrowse);
   assert(SD.existsCalls==before&&sdWrites==writesBeforeBrowse);
   const unsigned after=SD.existsCalls;now+=100;showLoadSave();assert(SD.existsCalls==after);
 }
 // Entry refreshes once; idle frames and FN redraws do not touch SD.
 ++patternBrowserRevision;const unsigned beforeEntry=SD.existsCalls;showLoadSave();
 assert(SD.existsCalls==beforeEntry&&SD.rootScans==scansBeforeBrowse+1);
 const unsigned afterEntry=SD.existsCalls;now+=1000;showLoadSave();assert(SD.existsCalls==afterEntry);
 loadSaveTrack.pos[3]=11;showLoadSave();
 clearPatternNotes();savePattern(false);assert(lastPatternSaveOk&&validPatternFile("9-B.txt",true));
 assert(*SD.files["10.txt"]==otherA&&*SD.files["10-C.txt"]==otherC&&*SD.files["10-D.txt"]==otherD);
 assert(!SD.exists("10-B.txt"));
 savePattern(true);resetFileSubpattern();loadPattern(true);
 assert(SMP.file==9&&subpatternFile==9&&subpatternSlot==1&&note[32][5].channel==0);
 currentMode=&draw;selectSlot(-1);drainSlots();assert(subpatternFile==9&&subpatternSlot==0&&note[1][2].channel==1);
 // Empty A persists as a valid blank file and preserves every sibling, including recovery files.
 SD.files["9-C.txt.tmp"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 SD.files["9-D.txt.bak"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 SD.files["9-A.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 const auto siblingB=*SD.files["9-B.txt"], siblingTmp=*SD.files["9-C.txt.tmp"], siblingBak=*SD.files["9-D.txt.bak"];
 clearPatternNotes();savePattern(false);savePattern(true);
 assert(lastPatternSaveOk&&validPatternFile("9.txt",true)&&subpatternFile==9&&subpatternSlot==0);
 assert(*SD.files["9-B.txt"]==siblingB&&*SD.files["9-C.txt.tmp"]==siblingTmp&&*SD.files["9-D.txt.bak"]==siblingBak&&SD.exists("9-A.txt"));
 // Only a successful explicit load adopts the highlighted number. Failed/cancelled loads do not.
 SD.files["11.txt"]=std::make_shared<std::vector<uint8_t>>(3,0);
 loadPattern(false,11);assert(subpatternFile==9&&SMP.file==9);
 loadPattern(false,12);assert(subpatternFile==9&&SMP.file==9&&pendingNewPatternFile==12);
 pendingNewPatternFile=0;savePattern(false);assert(!SD.exists("12.txt")&&subpatternFile==9);
 loadPattern(false,10);assert(subpatternFile==10&&SMP.file==10&&subpatternSlot==0);
 currentMode=&loadSaveTrack;textCalls.clear();now+=100;showLoadSave();displayed.clear();
 for(auto &c:textCalls)if(c.color==UI_WHITE)displayed+=c.text;
 assert(displayed.empty());
 currentMode=&draw;
 // A separate failed blank-destination save also survives a transport restart.
 resetFileSubpattern();SMP.file=20;
 SD.files["20.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1.txt"]);
 loadPattern(false);SD.failWritePath="20-B.txt.tmp";selectSlot(1);drainSlots();
 assert(subpatternSlot==1&&!subpatternOldSavePending&&subpatternSaveNew&&!SD.exists("20-B.txt"));
 SD.failWritePath.clear();isNowPlaying=true;now+=2000;
 const unsigned blankWrites=sdWrites;selectSlot(-1);drainSlots();
 assert(subpatternSlot==1&&subpatternSaveNew&&!subpatternSelector&&sdWrites==blankWrites);
 isNowPlaying=false;drainSlots();
 assert(!subpatternSaveNew&&validPatternFile("20-B.txt",true)&&sdWrites>blankWrites);
 // FN state invalidates the PATT render cache; delete X sits over encoder 2.
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=11;now+=100;
 fnHeld=false;loadSaveTrack.pos[2]=0;pixelCalls.clear();showLoadSave();
 for(auto p:pixelCalls)assert(p.color!=UI_WHITE);assert(SMP_LOAD_SETTINGS);
 loadSaveTrack.pos[2]=1;pixelCalls.clear();showLoadSave();assert(!SMP_LOAD_SETTINGS);
 unsigned trianglePixels=0;for(auto p:pixelCalls)if(p.color==UI_RED){assert(p.y==6||p.y==4||p.y==2);const int half=p.y==6?2:p.y==4?1:0;assert(p.x>=11-half&&p.x<=11+half);++trianglePixels;}
 assert(trianglePixels==9&&Encoder[2].rgb==0xFF0000);
 fnHeld=true;pixelCalls.clear();indicatorCalls.clear();showLoadSave();
 for(int enc:indicatorCalls)assert(enc!=2);
 unsigned deletePixels=0;for(auto p:pixelCalls)if(p.color==UI_RED&&p.x<=8){assert(p.x>=6&&p.x<=8&&p.y>=3&&p.y<=5);++deletePixels;}
 assert(deletePixels==5);
 fnHeld=false;pixelCalls.clear();showLoadSave();
 for(auto p:pixelCalls)if(p.color==UI_RED)assert(p.x>=9);
 loadSaveTrack.pos[2]=0;pixelCalls.clear();showLoadSave();
 for(auto p:pixelCalls)assert(p.color!=UI_WHITE);assert(SMP_LOAD_SETTINGS);
 loadSaveTrack.pos[2]=1;showLoadSave();assert(!SMP_LOAD_SETTINGS);
 loadSaveTrack.pos[3]=12;showLoadSave();assert(SMP_LOAD_SETTINGS&&loadSaveTrack.pos[2]==0);
 loadSaveTrack.pos[3]=11;showLoadSave();assert(SMP_LOAD_SETTINGS&&loadSaveTrack.pos[2]==0);
 // Autosave spells out its name in two rows on the 16-column display.
 loadSaveTrack.pos[3]=1;textCalls.clear();showLoadSave();
 assert(textCalls.size()==2&&textCalls[0].text=="AUTO"&&textCalls[1].text=="SAVE");
 assert(textCalls[0].x==1&&textCalls[0].y==12&&textCalls[1].x==1&&textCalls[1].y==7);
 assert(SMP_LOAD_SETTINGS&&Encoder[1].rgb==0);
 // Save As copies an empty A plus populated B, and includes unsaved active edits.
 currentMode=&draw;resetFileSubpattern();SMP.file=40;clearPatternNotes();
 savePattern(false);SD.files["40-B.txt"]=std::make_shared<std::vector<uint8_t>>(*SD.files["1-B.txt"]);
 ++patternStorageRevision;
 const auto sourceA=*SD.files["40.txt"],sourceB=*SD.files["40-B.txt"];
 saveSelectedPatternFamily(41);
 assert(subpatternFile==41&&SMP.file==41&&subpatternSlot==0);
 assert(validPatternFile("41.txt",true)&&*SD.files["41-B.txt"]==sourceB&&!SD.exists("41-C.txt"));
 assert(*SD.files["40.txt"]==sourceA&&*SD.files["40-B.txt"]==sourceB);
 selectSlot(1);drainSlots();note[1][2]={7,100,100,1,65};
 saveSelectedPatternFamily(42);
 assert(subpatternFile==42&&subpatternSlot==1&&SD.exists("42.txt")&&SD.exists("42-B.txt"));
 loadPattern(false);assert(note[1][2].channel==7);
 // Failed copy keeps live identity and leaves the source untouched.
 SD.failWritePath="43.txt.tmp";saveSelectedPatternFamily(43);SD.failWritePath.clear();
 assert(subpatternFile==42&&SMP.file==42&&note[1][2].channel==7&&!SD.exists("43.txt")&&!SD.exists("43-B.txt"));
 SD.failWritePath="45-B.txt.tmp";saveSelectedPatternFamily(45);SD.failWritePath.clear();
 assert(subpatternFile==42&&subpatternSlot==1&&note[1][2].channel==7&&!SD.exists("45.txt")&&!SD.exists("45-B.txt"));
 // A recoverable destination may not be silently overwritten by Save As.
 SD.files["44-C.txt.bak"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 saveSelectedPatternFamily(44);assert(subpatternFile==42&&SD.exists("44-C.txt.bak")&&!SD.exists("44.txt"));
 // FN+Save deletes the SELECTED complete family, leaving the loaded family and live notes alone.
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=42;
 SD.files["41-B.txt.tmp"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 SD.files["41-C.txt.bak"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 SD.files["41-A.txt"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 const uint8_t fnSave[4]={0,4,0,0};
 SD.failRemove="41-B.txt.tmp";const int failures=failureReports;dispatchFnDelete(fnSave);if(currentMode==&patternConfirmMode)finishPatternConfirmation(true);
 assert(failureReports==failures+1&&SD.exists("41.txt")&&SD.exists("41-B.txt"));
 assert(subpatternFile==42&&note[1][2].channel==7);
 SD.failRemove.clear();currentMode=&loadSaveTrack;dispatchFnDelete(fnSave);if(currentMode==&patternConfirmMode)finishPatternConfirmation(true);
 assert(!SD.exists("41.txt")&&!SD.exists("41-B.txt")&&!SD.exists("41-B.txt.tmp")&&!SD.exists("41-C.txt.bak"));
 assert(SD.exists("41-A.txt")&&SD.exists("42-B.txt")&&subpatternFile==42&&note[1][2].channel==7);
 // Deleting the loaded family clears the live notes and suppresses blank automatic recreation.
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=43;dispatchFnDelete(fnSave);if(currentMode==&patternConfirmMode)finishPatternConfirmation(true);
 assert(!SD.exists("42.txt")&&!SD.exists("42-B.txt")&&note[1][2].channel==0&&deletedSubpatternSlots==31);
 savePattern(true);resetFileSubpattern();loadPattern(true);
 assert(subpatternFile==42&&subpatternSlot==1&&deletedSubpatternSlots==31);
 currentMode=&draw;selectSlot(-1);drainSlots();assert(!SD.exists("42.txt")&&!SD.exists("42-B.txt"));
 savePattern(false);assert(SD.exists("42.txt"));
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=1;dispatchFnDelete(fnSave);if(currentMode==&patternConfirmMode)finishPatternConfirmation(true);assert(SD.exists("42.txt"));
 // DEL/SAV prompts do not mutate the card until encoder 2 accepts YES.
 currentMode=&draw;resetFileSubpattern();SMP.file=50;clearPatternNotes();savePattern(false);
 SD.files["50-B.txt"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 SD.files["60.txt"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 SD.files["60-E.txt"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 SD.files["60-E.txt.tmp"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 SD.files["60-E.txt.bak"]=std::make_shared<std::vector<uint8_t>>(sourceB);
 ++patternStorageRevision;currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=61;
 const unsigned beforePrompt=sdWrites;const auto old60=*SD.files["60.txt"];
 requestPatternSave(60);assert(currentMode==&patternConfirmMode&&!patternConfirmDelete&&patternConfirmMode.pos[3]==0&&patternConfirmTarget==60);
 textCalls.clear();showPatternConfirmation();assert(textCalls.front().text=="A"&&textCalls.back().text=="NO");assert(textCalls.back().color==UI_BLUE&&Encoder[1].rgb==0&&Encoder[3].rgb==0x0000FF);
 const uint8_t execute[4]={0,0,0,1},wrongButton[4]={0,1,0,0};
 dispatchConfirmButtons(execute);assert(cancellations==1&&currentMode==&loadSaveTrack&&loadSaveTrack.pos[3]==61&&sdWrites==beforePrompt&&*SD.files["60.txt"]==old60&&SD.exists("60-E.txt"));
 currentMode=&loadSaveTrack;requestPatternSave(60);patternConfirmMode.pos[3]=1;
 textCalls.clear();showPatternConfirmation();assert(textCalls.front().text=="A"&&textCalls.back().text=="YES");assert(textCalls.back().color==UI_GREEN&&Encoder[1].rgb==0&&Encoder[3].rgb==0x00FF00);
 dispatchConfirmButtons(wrongButton);assert(currentMode==&patternConfirmMode&&sdWrites==beforePrompt);
 dispatchConfirmButtons(execute);
 assert(currentMode==&draw&&subpatternFile==60&&validPatternFile("60.txt",true)&&SD.exists("60-B.txt"));
 assert(!SD.exists("60-E.txt")&&!SD.exists("60-E.txt.tmp")&&!SD.exists("60-E.txt.bak"));
 assert(*SD.files["50-B.txt"]==sourceB&&note[1][2].channel==0);
 // A later install failure restores every original target primary, including earlier commits.
 for(const char *name:{"61.txt","61-B.txt","61-C.txt"})SD.files[name]=std::make_shared<std::vector<uint8_t>>(sourceB);
 const auto before61A=*SD.files["61.txt"],before61B=*SD.files["61-B.txt"],before61C=*SD.files["61-C.txt"];
 SD.failRenameOnceTo="61-B.txt";saveSelectedPatternFamily(61,true);
 assert(!lastPatternSaveOk&&subpatternFile==60&&*SD.files["61.txt"]==before61A&&*SD.files["61-B.txt"]==before61B&&*SD.files["61-C.txt"]==before61C);
 SD.failWritePath="61-B.txt.tmp";saveSelectedPatternFamily(61,true);SD.failWritePath.clear();
 assert(!lastPatternSaveOk&&*SD.files["61.txt"]==before61A&&subpatternFile==60);
 // Deletion always begins at '?' and snapshots the selected number, not the live family.
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=62;dispatchFnDelete(fnSave);
 assert(patternConfirmDelete&&patternConfirmTarget==61&&patternConfirmMode.pos[3]==0&&SD.exists("61.txt"));
 textCalls.clear();indicatorCalls.clear();showPatternConfirmation();
 assert(textCalls.front().x==2&&textCalls.front().y==11&&textCalls.back().x==2&&textCalls.back().y==5);
 assert(indicatorCalls.size()==1&&indicatorCalls[0]==4);
 assert(textCalls.front().text=="DEL?"&&textCalls.back().text=="NO");assert(textCalls.back().color==UI_BLUE&&Encoder[1].rgb==0&&Encoder[3].rgb==0x0000FF);
 dispatchConfirmButtons(execute);assert(cancellations==2&&currentMode==&loadSaveTrack&&SD.exists("61.txt"));
 currentMode=&loadSaveTrack;loadSaveTrack.pos[3]=62;dispatchFnDelete(fnSave);patternConfirmMode.pos[3]=1;
 dispatchConfirmButtons(execute);assert(currentMode==&loadSaveTrack&&loadSaveTrack.pos[3]==62&&!SD.exists("61.txt")&&!SD.exists("61-B.txt")&&!SD.exists("61-C.txt")&&subpatternFile==60);
 // FN+encoder 3 captures selected sample/KIT for the same confirmation flow.
 const uint8_t fnThird[4]={0,0,4,0};
 SD.files["samples/drums/kick.wav"]=std::make_shared<std::vector<uint8_t>>(8,5);
 SD.files["samples/drums/snare.wav"]=std::make_shared<std::vector<uint8_t>>(8,6);
 currentMode=&set_Wav;set_Wav.pos[3]=7;capturedSample="samples/drums/kick.wav";dispatchFnDelete(fnThird);
 assert(currentMode==&patternConfirmMode&&patternConfirmResource==1&&patternConfirmMode.pos[3]==0);
 capturedSample="samples/drums/snare.wav";dispatchConfirmButtons(execute);
 assert(cancellations==3&&currentMode==&set_Wav&&set_Wav.pos[3]==7&&SD.exists("samples/drums/kick.wav"));
 currentMode=&set_Wav;capturedSample="samples/drums/kick.wav";dispatchFnDelete(fnThird);
 capturedSample="samples/drums/snare.wav";patternConfirmMode.pos[3]=1;dispatchConfirmButtons(execute);
 assert(currentMode==&set_Wav&&!SD.exists("samples/drums/kick.wav")&&SD.exists("samples/drums/snare.wav"));
 SD.files["samples/drums/mixed.Wav"]=std::make_shared<std::vector<uint8_t>>(8,5);
 currentMode=&set_Wav;capturedSample="samples/drums/mixed.Wav";dispatchFnDelete(fnThird);patternConfirmMode.pos[3]=1;dispatchConfirmButtons(execute);assert(!SD.exists("samples/drums/mixed.Wav"));
 // Folder rows cannot become sample deletion targets.
 currentMode=&set_Wav;capturedSample.clear();dispatchFnDelete(fnThird);assert(currentMode==&set_Wav);
 SD.dirs.insert("70");SD.dirs.insert("71");
 for(unsigned ch=1;ch<=8;ch++){SD.files["70/"+std::to_string(ch)+".wav"]=std::make_shared<std::vector<uint8_t>>(8,7);SD.files["71/"+std::to_string(ch)+".wav"]=std::make_shared<std::vector<uint8_t>>(8,7);}
 currentMode=&set_SamplePack;set_SamplePack.pos[3]=70;dispatchFnDelete(fnThird);assert(currentMode==&set_SamplePack);dispatchFnDelete(fnSave);
 assert(patternConfirmResource==2&&patternConfirmTarget==70&&SD.exists("70/1.wav"));
 set_SamplePack.pos[3]=71;patternConfirmMode.pos[3]=1;dispatchConfirmButtons(execute);
 assert(currentMode==&set_SamplePack&&set_SamplePack.pos[3]==70&&!SD.exists("70")&&SD.exists("71/1.wav"));
 // Removal errors show failure and leave the directory for retry.
 currentMode=&set_SamplePack;set_SamplePack.pos[3]=71;dispatchFnDelete(fnSave);
 SD.failRemove="71/3.wav";patternConfirmMode.pos[3]=1;const int mediaFailures=failureReports;
 dispatchConfirmButtons(execute);assert(failureReports==mediaFailures+1&&SD.exists("71/3.wav")&&SD.exists("71"));SD.failRemove.clear();
 // KIT overwrite defaults to NO and does not write until encoder 4 confirms YES.
 currentMode=&set_SamplePack;set_SamplePack.pos[3]=71;
 const unsigned kitWrites=sdWrites;requestKitSave(71);
 assert(currentMode==&patternConfirmMode&&!patternConfirmDelete&&patternConfirmResource==2&&patternConfirmMode.pos[3]==0);
 dispatchConfirmButtons(execute);
 assert(currentMode==&set_SamplePack&&set_SamplePack.pos[3]==71&&kitSaveCalls==0&&sdWrites==kitWrites);
 requestKitSave(71);set_SamplePack.pos[3]=72;
 dispatchConfirmButtons(wrongButton);assert(currentMode==&patternConfirmMode&&kitSaveCalls==0);
 patternConfirmMode.pos[3]=1;dispatchConfirmButtons(execute);
 assert(savedKit==71&&kitSaveCalls==1&&currentMode==&draw);
 // A later voice alone still protects an incomplete kit; an empty slot saves directly.
 SD.files["73/8.wav"]=std::make_shared<std::vector<uint8_t>>(8,7);
 currentMode=&set_SamplePack;set_SamplePack.pos[3]=73;requestKitSave(73);
 assert(currentMode==&patternConfirmMode&&kitSaveCalls==1);finishPatternConfirmation(false);
 set_SamplePack.pos[3]=74;requestKitSave(74);
 assert(currentMode==&draw&&savedKit==74&&kitSaveCalls==2);
 puts("PASS: canonical numbered A, B-E companions, strict no-fallback A filenames, transaction recovery, autosave identity, live switches, transport-safe save retries, loaded-family ownership, empty-save preservation, explicit deletion, recovery and PATT icons.");

}
'''.replace('SAMPLE',sample).replace('DEVICE',device).replace('STATE',state).replace('HELPERS',helpers).replace('VALIDATOR',validator).replace('SLOTS',slots).replace('LOADER',loader).replace('RENDERER',renderer).replace('FNDELETE',fn_delete).replace('CONFIRMBUTTONS',confirm_buttons)
with tempfile.TemporaryDirectory(prefix='toern-subpatterns-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(harness)
 flags=['c++','-std=c++17','-O1','-Wno-deprecated-declarations','-fsanitize=address,undefined','-I',str(root/'src')]
 if sys.platform=='darwin':flags+=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1']
 subprocess.run(flags+[str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
