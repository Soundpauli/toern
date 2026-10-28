"""Run the real loader from garbage PSRAM through boot and manual-load paths."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_fileoperations.ino').read_text()
main = (root / 'toern.ino').read_text()
def function(name):
    clean = re.sub(r'//[^\n]*', lambda m: ' ' * len(m[0]), source)
    start = re.search(r'FLASHMEM void '+name+r'\([^;{]*\) \{',clean).start()
    end = clean.index('{',start)+1
    depth=1
    while depth:
        depth += (clean[end]=='{')-(clean[end]=='}')
        end += 1
    return source[start:end]
sample = main[main.index('struct Sample {'):main.index('// Declare the device struct')]
device = main[main.index('struct Device {'):main.index('// Global variables struct')]
harness = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <climits>
#include "toern_pattern_slots.h"
using std::min;
using std::isfinite;
#define FLASHMEM
#define EXTMEM
constexpr unsigned maxFiles=9,maxY=16,maxFilters=15,maxPages=16,maxlen=3;
SAMPLE
DEVICE
Device SMP;
struct Note {uint8_t channel,velocity,probability,condition,midiPitch;}note[maxlen+1][maxY+1];
constexpr uint8_t NOTE_MIDI_PITCH_NONE=255;
const uint8_t PITCH_EXTENSION_HEADER[8]={'T','P','I','T',1,16,0,0};
const char *AUTOSAVE_PATH="autosaved.txt";
std::map<std::string,std::vector<uint8_t>> files;
std::vector<std::string> opens;
struct File {
 std::vector<uint8_t>* data=nullptr;size_t pos=0;
 operator bool()const{return data!=nullptr;}
 size_t size(){return data?data->size():0;}
 int available(){return size()-pos;}
 int read(uint8_t *dst,size_t n){n=min(n,size()-pos);memcpy(dst,data->data()+pos,n);pos+=n;return n;}
 void close(){}
};
struct Card {
 File open(const char *path){opens.push_back(path);auto it=files.find(path);return {it==files.end()?nullptr:&it->second,0};}
 bool exists(const char *path){return files.count(path);}
}SD;
int recovered=0,cleared=0,soundLoads=0,newScreens=0,failures=0;
unsigned pendingNewPatternFile=0,subpatternFile=0;
uint8_t subpatternSlot=0,deletedSubpatternSlots=0;
bool subpatternOldSavePending=false,subpatternSaveNew=false,subpatternReady=false,subpatternApplied=false,subpatternSelector=false;
unsigned loadedPatternFile(){return subpatternFile>=1&&subpatternFile<=999?subpatternFile:SMP.file;}
void subpatternFailure(){++failures;}
void subpatternCancelPreview(){}
void resetFileSubpattern(){subpatternFile=0;subpatternSlot=0;deletedSubpatternSlots=0;pendingNewPatternFile=0;}
void subpatternNotePagesLoaded(){}
bool patternFamilyBusy(){return false;}
void refreshCtrlEncoderConfig(){}
bool isNowPlaying=false,SMP_LOAD_SETTINGS=true,preventPaintUnpaint=false,g_enterNewFileAfterCorruptAutoload=false;
bool globalMutes[maxY]{},pageMutes[maxPages][maxY]{};
struct {bool singleMode=true;}GLOB;
struct {unsigned pos[4]{};}bpmMode;
struct Timer {void update(double){}}playTimer,fillTimer;
double playNoteInterval=0;int clockMode=1,draw=0,UI_GREEN=0;
struct CRGB{};
void drawText(const char*,int,int,CRGB){}
void drawText(const char*,int,int,int){}
void drawNoSD(){}void stopSdPreviewIfPlaying(){}void sdIoYield(){}
void sdIoBeginAudioSafe(){}void sdIoEndAudioSafe(){}
void FastLEDclear(){}void FastLEDshow(){}
void clearPatternNotes(){++cleared;memset(note,0,sizeof(note));}
void unmuteAllChannels(){}void applyMutesAfterPMODSwitch(){}
void showPatternResult(bool ok){if(!ok)++failures;}
void showNewFileScreen(){++newScreens;}
void syncBpmEncoderPositions(){}void updateLastPage(){}
void loadSMPSettings(){++soundLoads;}
void delay(int){}void switchMode(int*){}void applyDrawSingleYEncoderLimits(){}
bool validPatternFile(const char*,bool){return true;}
template<class F>void recoverPatternTransaction(Card&,const char*,const char*,const char*,F){++recovered;}
void loadPattern(bool,unsigned=1000);
FUNCTIONS
std::vector<uint8_t> fixture(unsigned embeddedFile,bool corrupt=false){
 Device saved{};saved.file=embeddedFile;saved.pack=2;saved.bpm=137;
 const size_t n=(maxlen-1)*maxY;
 std::vector<uint8_t> payload(n*4+2+sizeof(saved));
 payload[0]=3;payload[1]=127;payload[2]=100;payload[3]=1;
 payload[n*4]=corrupt?0:0xFF;payload[n*4+1]=0xFE;
 memcpy(payload.data()+n*4+2,&saved,sizeof(saved));
 return payload;
}
void garbage(unsigned file){
 memset(&SMP,0xA5,sizeof(SMP));SMP.file=file;
 subpatternFile=0;subpatternSlot=0;deletedSubpatternSlots=0;
 memset(note,0x7F,sizeof(note));opens.clear();
 recovered=cleared=soundLoads=failures=newScreens=0;
 g_enterNewFileAfterCorruptAutoload=false;
}
int main(){
 for(unsigned initial:{0u,1u,999u,1000u,0xA5A5A5A5u,UINT_MAX}){
  files.clear();files["autosaved.txt"]=fixture(42);garbage(initial);
  loadPattern(true);
  assert(recovered==1&&!opens.empty()&&opens[0]=="autosaved.txt");
  assert(note[1][1].channel==3&&note[1][1].velocity==127);
  assert(note[2][16].channel==0&&SMP.file==42&&subpatternFile==42);
  assert(SMP.bpm==137&&soundLoads==1&&cleared==1&&!failures);
 }
 // Direct boot calls also ignore the incoming target identity.
 garbage(UINT_MAX);loadPattern(true,UINT_MAX);
 assert(SMP.file==42&&soundLoads==1);
 // Missing/corrupt autosaves clear the entire grid and leave a valid browser identity.
 for(bool corrupt:{false,true}){
  files.clear();if(corrupt)files["autosaved.txt"]=fixture(42,true);
  garbage(UINT_MAX);loadPattern(true);
  assert(cleared==1&&SMP.file==1&&subpatternFile==0&&soundLoads==0);
  for(auto &column:note)for(auto &cell:column)assert(cell.channel==0);
  assert(g_enterNewFileAfterCorruptAutoload==corrupt);
 }
 // Explicit numbered loads override stale identities embedded in copied patterns.
 files.clear();files["7.txt"]=fixture(2);garbage(UINT_MAX);
 loadPattern(false,7);
 assert(SMP.file==7&&subpatternFile==7&&note[1][1].channel==3);
 // Invalid manual targets are rejected without touching the live session.
 opens.clear();loadPattern(false,1001);assert(opens.empty()&&SMP.file==7);
}
'''.replace('SAMPLE',sample).replace('DEVICE',device).replace('FUNCTIONS',function('loadPattern'))
with tempfile.TemporaryDirectory(prefix='toern-boot-recovery-') as directory:
    path=Path(directory)
    (path/'test.cpp').write_text(harness)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-I',str(root/'src'),*flags,str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('PASS: boot recovery with garbage PSRAM identities, missing/corrupt autosaves, manual load identity and invalid manual targets.')
