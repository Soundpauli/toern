"""Exercise actual firmware slot/save/validation code with an in-memory SD card."""
from pathlib import Path
import subprocess, tempfile, sys
root=Path(__file__).resolve().parents[1]
main=(root/'toern.ino').read_text()
ops=(root/'toern_fileoperations.ino').read_text()
slots=(root/'toern_pattern_slots.ino').read_text()
loader=ops[ops.index('FLASHMEM void loadPattern'):ops.index('FLASHMEM void autoLoad')]
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
struct Mode {unsigned pos[4]={};} draw,menu,singleMode;Mode *currentMode=&draw;
struct {unsigned x=1,page=1,edit=1;bool singleMode=false;} GLOB;
struct Encoder {void writeCounter(int32_t){}void writeMax(int32_t){}} Encoder[4];
bool fnHeld=false,fastRecordActive=false,isRecording=false,childLockEnabled=false,imageMode=false;
bool isNowPlaying=false,voiceMode=false,songModeActive=false,preventPaintUnpaint=false;
unsigned lastPage=1,beat=1,beatForUI=1,loopCount=1,pendingPage=0;
uint8_t voicePageLen[16]={},voiceModePages=1;
uint16_t voicePageMask[16]={},voiceRowMask[16]={},voiceLoopCount[16]={};
bool hasNotes[maxPages+1]={};int loopLength=0,editpage=1,ctrlMode=0;
unsigned long now=100;unsigned long millis(){return now;}
namespace toern_audio {struct StateGuard {};}
using CRGB=int;
const int UI_RED=1,UI_GREEN=2,UI_BLUE=3;
void FastLEDclear(){}void FastLEDshow(){}void drawNoSD(){}
void drawText(const char*,int,int,int){}
char overlayBuf[16]={};
void drawCenteredOverlayLabel(const char *s,int){snprintf(overlayBuf,sizeof(overlayBuf),"%s",s);}
void showSaveSuccessAnimation(bool){}void showSaveFailureAnimation(bool){}
void switchMode(Mode*){}void sdIoYield(){}void sdIoBeginAudioSafe(){}void sdIoEndAudioSafe(){}
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
struct File {
 std::shared_ptr<std::vector<uint8_t>> data;size_t pos=0;bool fail=false;
 explicit operator bool()const{return !!data;}
 size_t size(){return data?data->size():0;}
 int available(){return size()-pos;}
 bool seek(size_t n){pos=n;return n<=size();}
 int read(){return available()?(*data)[pos++]:-1;}
 int read(uint8_t *out,size_t n){if(!data)return -1;n=min(n,size()-pos);memcpy(out,data->data()+pos,n);pos+=n;return n;}
 size_t write(const uint8_t *in,size_t n){if(fail)return 0;if(pos+n>size())data->resize(pos+n);memcpy(data->data()+pos,in,n);pos+=n;return n;}
 void close(){}
};
struct FS {
 std::map<std::string,std::shared_ptr<std::vector<uint8_t>>> files;bool failWrite=false;
 bool exists(const char *p){return files.count(p);}
 bool remove(const char *p){return files.erase(p)>0;}
 bool rename(const char*a,const char*b){if(!exists(a)||exists(b))return false;files[b]=files[a];files.erase(a);return true;}
 File open(const char*p,int mode=0){if(mode&O_CREAT){if(!exists(p)||mode&O_TRUNC)files[p]=std::make_shared<std::vector<uint8_t>>();}
  if(!exists(p))return {};return {files[p],0,failWrite};}
} SD;
HELPERS
VALIDATOR
SLOTS
LOADER
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
 assert(SD.exists("1-A.txt")&&SD.exists("1-B.txt")&&!SD.exists("1.txt")&&!SD.exists("1.txt.bak"));
 note[32][5]={4,90,100,2,72};
 selectSlot(-1);drainSlots();
 assert(subpatternSlot==0&&note[1][2].channel==1&&note[1][2].midiPitch==61&&GLOB.x==9);
 assert(hasNotes[1]&&hasNotes[16]&&lastPage==16&&voicePageLen[2]==16&&voicePageMask[2]==0x8000);
 isNowPlaying=true;
 selectSlot(1);finishSwitch();
 assert(subpatternSlot==1&&note[32][5].channel==4&&note[32][5].midiPitch==72);
 // A failed outgoing save when paused keeps the new grid on screen and retries.
 isNowPlaying=false;
 SD.failWrite=true;selectSlot(-1);
 assert(subpatternSlot==0&&note[1][2].channel==1&&subpatternOldSavePending);
 drainSlots();assert(subpatternOldSavePending&&note[1][2].channel==1);
 now+=2000;SD.failWrite=false;drainSlots();assert(!subpatternOldSavePending);
 selectSlot(1);drainSlots();
 isNowPlaying=true;
 // Corrupt target never replaces the live grid, including its visible page.
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
 // FILE re-entry resolves the lettered A; sound settings OFF still works.
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
 // Load does not walk B-Z. With A gone and no in-memory family, the legacy name is used.
 SD.remove("1-A.txt");SD.remove("1-A.txt.bak");resetFileSubpattern();
 char destination[24];patternDestination(destination,sizeof(destination),1);
 assert(!strcmp(destination,"1.txt"));
 // Leftover 2-B.txt on a non-lettered pattern (2.txt only) must not show up as a subpattern.
 SMP.file=2;resetFileSubpattern();
 for(auto &row:note)for(auto &cell:row)cell={0,100,100,1,255};
 note[1][4]={3,100,100,1,255};
 savePattern(false);assert(SD.exists("2.txt")&&!SD.exists("2-A.txt"));
 SD.files["2-B.txt"]=SD.files["1-B.txt"];
 loadPattern(false);assert(subpatternFile==2&&!subpatternLettered&&subpatternSlot==0);
 isNowPlaying=true;
 fnHeld=true;subpatternTurn(1);serviceSubpatterns();
 assert(subpatternViewOn&&subpatternView[1][2].channel==0&&subpatternView[16][5].channel==0);
 fnHeld=false;subpatternFnRelease();finishSwitch();
 assert(subpatternSlot==1&&note[32][5].channel==0&&note[1][4].channel==0);
 // Loading an empty pattern slot resets subpatternFile so it cannot inherit the previous file's B-E.
 isNowPlaying=false;SMP.file=5;loadPattern(false);
 assert(subpatternFile==0&&subpatternSlot==0&&!subpatternLettered);
 puts("PASS: A-E families, no-save switches, deferred playback, visible preview, hasNotes, pitch/length metadata and SD failures.");
}
'''.replace('SAMPLE',sample).replace('DEVICE',device).replace('STATE',state).replace('HELPERS',helpers).replace('VALIDATOR',validator).replace('SLOTS',slots).replace('LOADER',loader)
with tempfile.TemporaryDirectory(prefix='toern-subpatterns-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(harness)
 flags=['c++','-std=c++17','-O1','-Wno-deprecated-declarations','-fsanitize=address,undefined','-I',str(root/'src')]
 if sys.platform=='darwin':flags+=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1']
 subprocess.run(flags+[str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
