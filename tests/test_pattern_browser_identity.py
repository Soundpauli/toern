"""Exercise real directory caching and save identity with SD failures injected."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_fileoperations.ino').read_text()
def function(name):
    clean = re.sub(r'//[^\n]*', lambda m: ' ' * len(m[0]), source)
    start = re.search(r'FLASHMEM (?:void|bool) '+name+r'\([^;{]*\) \{', clean).start()
    end = clean.index('{', start) + 1
    depth = 1
    while depth:
        depth += (clean[end] == '{') - (clean[end] == '}')
        end += 1
    return source[start:end]

load_body = function('loadPatternFrom')
identity_start = load_body.index('      SMP.file = autoload || requestedFile == 0')
identity_end = load_body.index('      loadedOk = true;', identity_start)
identity_block = load_body[identity_start:identity_end]
helpers = (root / 'toern_helpers.ino').read_text()
new_start = helpers.index('static void beginNewPattern() {')
new_end = helpers.index('\n}', new_start) + 2
new_function = helpers[new_start:new_end]

harness = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
using std::min;
#define FLASHMEM
#define EXTMEM
constexpr unsigned maxlen=3,maxY=2,maxPages=2;
constexpr int O_WRITE=1,O_CREAT=2,O_TRUNC=4,UI_GREEN=0;
const char *AUTOSAVE_PATH="autosaved.txt";
const uint8_t PITCH_EXTENSION_HEADER[8]={};
struct Device {unsigned file=2;bool globalMutes[maxY]{},pageMutes[maxPages][maxY]{};} SMP;
struct Note{uint8_t channel=1,velocity=127,probability=100,condition=1,midiPitch=60;}note[maxlen][maxY+1];
bool globalMutes[maxY]{},pageMutes[maxPages][maxY]{};
unsigned loadedPatternFileNumber=2,pendingNewPatternFile=0;
uint32_t patternBrowserRevision=1,patternStorageRevision=0;
bool lastPatternSaveOk=false,preventPaintUnpaint=false,failWrite=false,failCommit=false,failRoot=false;
std::vector<uint8_t> bytes;
std::vector<std::string> names={"2.txt","9.txt","999.txt","autosaved.txt","3.txt.tmp","1000.txt","4-B.txt","stuff","5.txt.bak"};
int scans=0,yields=0;
struct File {
 bool valid=false,dir=false;std::string filename;size_t index=0;
 operator bool()const{return valid;}
 bool isDirectory(){return dir;}
 const char* name(){return filename.c_str();}
 File openNextFile(){return index<names.size()?File{true,false,names[index++],0}:File{};}
 void close(){}
};
struct Card {
 File open(const char *path,int flags=0){
  if(std::string(path)=="/"){++scans;return {!failRoot,true,"/",0};}
  if(flags)bytes.clear();
  return {true,false,path,0};
 }
 void remove(const char*){}
}SD;
void sdIoBeginAudioSafe(){}void sdIoEndAudioSafe(){}void stopSdPreviewIfPlaying(){}
void sdIoYield(){++yields;}
void drawNoSD(){}void FastLEDclear(){}void FastLEDshow(){}
void drawText(const char*,int,int,int){}
int draw;void switchMode(int*){}void showPatternResult(bool){}
bool validPatternFile(const char*,bool){return true;}
template<class F>void recoverPatternTransaction(Card&,const char*,const char*,const char*,F){}
template<class F>bool commitPatternTransaction(Card&,const char*,const char*,const char*,F){return !failCommit;}
bool patternWriteAll(File&,const uint8_t* p,size_t n){
 assert(SMP.file==loadedPatternFileNumber); // Live identity stays stable throughout I/O.
 if(failWrite)return false;
 bytes.insert(bytes.end(),p,p+n);return true;
}
FUNCTIONS
void adoptLoadedIdentity(bool autoload,unsigned requestedFile){
IDENTITY
}
NEW_FUNCTION
unsigned storedIdentity(){Device d;memcpy(&d,bytes.data()+(maxlen-1)*maxY*4+2,sizeof(d));return d.file;}
int main(){
 assert(savedPatternExists(2)&&savedPatternExists(9)&&savedPatternExists(999)&&savedPatternExists(0));
 assert(!savedPatternExists(3)&&!savedPatternExists(4)&&!savedPatternExists(5)&&!savedPatternExists(1000));
 for(int frame=0;frame<200;frame++)assert(savedPatternExists(2));
 assert(scans==1); // No clock dependency: slow frames/slot turns cannot trigger another scan.
 names.push_back("6.txt");assert(!savedPatternExists(6));
 ++patternStorageRevision;assert(savedPatternExists(6)&&scans==2);
 ++patternBrowserRevision;assert(savedPatternExists(6)&&scans==3);
 ++patternBrowserRevision;failRoot=true;assert(!savedPatternExists(6));
 failRoot=false;assert(savedPatternExists(6)&&scans==5); // Failed opens remain retryable.
 failWrite=true;savePatternTo(false,7);
 assert(!lastPatternSaveOk&&SMP.file==2&&loadedPatternFileNumber==2);
 failWrite=false;failCommit=true;savePatternTo(false,7);
 assert(!lastPatternSaveOk&&SMP.file==2&&loadedPatternFileNumber==2&&storedIdentity()==7);
 failCommit=false;savePatternTo(true,2);
 assert(lastPatternSaveOk&&SMP.file==2&&storedIdentity()==2);
 savePatternTo(false,7);
 assert(lastPatternSaveOk&&SMP.file==7&&loadedPatternFileNumber==7&&storedIdentity()==7);
 savePatternTo(false,0);assert(!lastPatternSaveOk&&SMP.file==7);
 savePatternTo(false,1000);assert(!lastPatternSaveOk&&SMP.file==7);
}
'''.replace('FUNCTIONS', '\n'.join(function(n) for n in ['savedPatternExists','savePatternTo'])).replace('IDENTITY', identity_block).replace('NEW_FUNCTION', new_function)
with tempfile.TemporaryDirectory(prefix='toern-browser-identity-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', '-fsanitize=address,undefined', *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
main = (root / 'toern.ino').read_text()
browser = main.split('FLASHMEM void showLoadSave() {',1)[1].split('FLASHMEM void showPresetLoadSave()',1)[0]
assert not re.search(r'SMP\.file\s*=(?!=)',browser)
assert 'lastCallMs' not in browser
assert 'loadPatternFrom(false, currentMode->pos[3] - 1u)' in main
load = function('loadPatternFrom')
assert 'requestedFile' in load and 'pendingNewPatternFile = requestedFile' in load
print('PASS: directory caching, revision invalidation, retry, Save As/autosave identity, write/commit failure and explicit browser load targets.')
