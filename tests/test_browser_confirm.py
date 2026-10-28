"""Run real confirmation actions against a failure-injectable in-memory SD card."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_storage_confirm.ino').read_text()
def function(name):
    clean = re.sub(r'//[^\n]*', lambda m: ' ' * len(m[0]), source)
    start = re.search(r'FLASHMEM (?:void|bool) '+name+r'\([^;{]*\) \{', clean).start()
    end = clean.index('{', start) + 1
    depth = 1
    while depth:
        depth += (clean[end] == '{') - (clean[end] == '}')
        end += 1
    return source[start:end]

names = ['openPatternConfirmation','requestPatternSave','deleteSelectedPattern','requestKitSave',
         'requestBrowserDelete','deleteConfirmedBrowserResource','finishPatternConfirmation']
harness = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include "src/pattern_transaction.h"
#define FLASHMEM
constexpr int NUM_ENCODERS=4;
struct Mode {unsigned pos[4]{};} draw,loadSaveTrack,set_SamplePack,set_Wav,patternConfirmMode;
Mode *currentMode=&loadSaveTrack,*patternConfirmReturnMode=nullptr;
unsigned patternConfirmReturnPos[4]{},patternConfirmTarget=0,loadedPatternFileNumber=2;
uint8_t patternConfirmResource=0;
uint32_t patternConfirmRevision=0;
bool patternConfirmDelete=false,bypassModeSwitchDebounce=false,SMP_LOAD_SETTINGS=true;
bool pressed[4]{},preventPaintUnpaint=false,lastPatternSaveOk=false,fastRecordActive=false,isRecording=false;
char confirmedSampleDeletePath[160]{};
struct {unsigned file=0;} SMP;
struct Encoder {unsigned value=0;void writeCounter(unsigned n){value=n;}} Encoder[4];
struct File {bool valid=false,dir=false;operator bool()const{return valid;}bool isDirectory(){return dir;}void close(){}};
struct Card {
 std::set<std::string> files,dirs;std::string failRemove;
 bool exists(const char *p){return files.count(p)||dirs.count(p);}
 File open(const char *p){return {exists(p),bool(dirs.count(p))};}
 bool remove(const char *p){if(failRemove==p)return false;return files.erase(p);}
 bool rmdir(const char *p){for(auto &f:files)if(f.rfind(std::string(p)+"/",0)==0)return false;return dirs.erase(p);}
} SD;
int saves=0,kits=0,cancels=0,clears=0,invalidations=0,restored=-1;bool lastResult=false;
void sdIoBeginAudioSafe(){}void sdIoEndAudioSafe(){}void sdIoYield(){}void stopSdPreviewIfPlaying(){}
namespace toern_audio {struct StateGuard{};}
void clearPatternNotes(){++clears;}void updateLastPage(){}
void switchMode(Mode *m){currentMode=m;for(auto &p:m->pos)p=0;}
void savePattern(bool){++saves;lastPatternSaveOk=true;loadedPatternFileNumber=SMP.file;currentMode=&draw;}
void saveSamplePack(unsigned target){++kits;assert(target<=999);currentMode=&draw;}
void showCanceledAnimation(bool){++cancels;}void showPatternResult(bool ok){lastResult=ok;}
void sampleBrowserInvalidate(){++invalidations;}
void restoreSampleBrowserSelection(unsigned n){restored=n;}
const char *selectedWav="samples/drums/kick.wav";
bool captureSelectedSampleForDelete(char *p,size_t n){snprintf(p,n,"%s",selectedWav);return currentMode==&set_Wav;}
FUNCTIONS
int main(){
 SD.files={"2.txt","2.txt.tmp","2.txt.bak","3.txt","samples/drums/kick.wav","samples/drums/snare.wav","4/7.wav"};
 SD.dirs={"4","samples/drums"};
 loadSaveTrack.pos[3]=3;loadSaveTrack.pos[2]=1;
 requestPatternSave(2);assert(currentMode==&patternConfirmMode&&patternConfirmMode.pos[3]==0&&saves==0);
 finishPatternConfirmation(false);
 assert(cancels==1&&currentMode==&loadSaveTrack&&currentMode->pos[3]==3&&Encoder[3].value==3&&currentMode->pos[2]==0);
 assert(SD.exists("2.txt")&&saves==0);
 requestPatternSave(2);patternConfirmMode.pos[3]=1;finishPatternConfirmation(true);
 assert(saves==1&&SMP.file==2&&currentMode==&draw);
 currentMode=&loadSaveTrack;requestPatternSave(7);assert(saves==2&&SMP.file==7);
 // Recovery-only destinations still need an overwrite confirmation.
 SD.files.insert("8.txt.bak");currentMode=&loadSaveTrack;requestPatternSave(8);
 assert(currentMode==&patternConfirmMode&&saves==2);finishPatternConfirmation(false);
 currentMode=&set_SamplePack;set_SamplePack.pos[3]=4;
 requestKitSave(4);assert(currentMode==&patternConfirmMode&&kits==0&&patternConfirmTarget==4);
 finishPatternConfirmation(false);assert(currentMode==&set_SamplePack&&currentMode->pos[3]==4);
 requestKitSave(4);patternConfirmMode.pos[3]=1;finishPatternConfirmation(true);assert(kits==1);
 currentMode=&set_Wav;set_Wav.pos[3]=9;requestBrowserDelete();
 assert(!strcmp(confirmedSampleDeletePath,"samples/drums/kick.wav"));
 selectedWav="samples/drums/snare.wav";patternConfirmMode.pos[3]=1;finishPatternConfirmation(true);
 assert(!SD.exists("samples/drums/kick.wav")&&SD.exists("samples/drums/snare.wav")&&restored==9);
 assert(currentMode==&set_Wav&&lastResult);
 // Reject folder/traversal targets even after a malformed selection snapshot.
 patternConfirmResource=1;strcpy(confirmedSampleDeletePath,"samples/../secret.wav");
 SD.files.insert("samples/../secret.wav");assert(!deleteConfirmedBrowserResource()&&SD.exists("samples/../secret.wav"));
 strcpy(confirmedSampleDeletePath,"samples/drums");assert(!deleteConfirmedBrowserResource());
 // Deleting another pattern preserves the live notes; deleting the loaded one clears them.
 loadedPatternFileNumber=2;assert(deleteSelectedPattern(3)&&clears==0);
 SD.failRemove="2.txt.tmp";assert(!deleteSelectedPattern(2)&&SD.exists("2.txt")&&clears==0);
 SD.failRemove.clear();assert(deleteSelectedPattern(2)&&clears==1&&!SD.exists("2.txt.bak"));
 patternConfirmResource=2;patternConfirmTarget=4;assert(deleteConfirmedBrowserResource()&&!SD.exists("4/7.wav")&&!SD.exists("4"));
 currentMode=&loadSaveTrack;fastRecordActive=true;requestPatternSave(5);assert(currentMode==&loadSaveTrack);
 assert(!deleteSelectedPattern(5));fastRecordActive=false;
 for(bool p:pressed)assert(!p);assert(preventPaintUnpaint);
}
'''.replace('FUNCTIONS', '\n'.join(function(name) for name in names))
with tempfile.TemporaryDirectory(prefix='toern-confirm-') as directory:
    path=Path(directory)
    (path/'test.cpp').write_text(harness)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('PASS: overwrite/delete targets, NO/cancel restoration, partial kits, selected WAV snapshot, traversal/capture guards and failed SD cleanup.')
