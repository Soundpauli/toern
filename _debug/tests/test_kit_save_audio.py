"""Run real cooperative KIT export while playback continues; inject SD failures."""
from pathlib import Path
import subprocess
import sys
import tempfile
root=(Path(__file__).resolve().parents[2] / 'platformio')
ops=(root/'toern_fileoperations.ino').read_text()
menu=(root/'toern_menu.ino').read_text()
controller=ops[ops.index('#include "src/toern_kit_writer.h"'):ops.index('// Scan both primaries')]
def function(source,signature):
    start=source.index(signature);end=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
code=r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#define DMAMEM
#define FLASHMEM
constexpr int FILE_WRITE_BEGIN=2;
bool isNowPlaying=true,fastRecordActive=false,isRecording=false,preventPaintUnpaint=true;
struct Mode {unsigned pos[4]={};} draw,set_SamplePack;Mode *currentMode=&set_SamplePack;
struct {unsigned pack=1;bool sp0Active[9]={};} SMP;
unsigned samplePackID=1,loadedSampleLen[9]={};uint8_t sampled[9][5000]={};
uint32_t clockUs=100000;uint32_t micros(){return clockUs;}uint32_t millis(){return clockUs/1000;}
void switchMode(Mode *m){currentMode=m;}
bool g_pendingKitEeprom=false,g_pendingSp0Eeprom=false;unsigned backupMarks=0;
void markSettingsBackupDirty(){backupMarks++;}
void saveSamplePack(int);
bool storageActive=false;
void storageTransferBegin(bool,bool){storageActive=true;}
void storageTransferProgress(unsigned){}
bool storageTransferIs(bool,bool){return storageActive;}
void storageTransferFinish(bool,bool){storageActive=false;}
PERSIST
unsigned operations=0,maxTransfer=0,mainLoopPasses=0;
bool failWrite=false,corruptRead=false;std::string failRenameTo;
struct Node {std::vector<uint8_t> bytes;};
struct File {
 std::shared_ptr<Node> node;size_t pos=0;std::string path;
 explicit operator bool()const{return bool(node);}
 size_t size()const{return node?node->bytes.size():0;}
 bool truncate(size_t n){operations++;node->bytes.resize(n);return true;}
 int read(uint8_t *out,size_t count){operations++;assert(count<=2048);maxTransfer=std::max(maxTransfer,(unsigned)count);
  count=std::min(count,size()-pos);memcpy(out,node->bytes.data()+pos,count);pos+=count;
  if(corruptRead&&path.find(".kit-tmp")!=std::string::npos&&count){out[0]^=1;corruptRead=false;}return count;}
 size_t write(const uint8_t *in,size_t count){operations++;assert(count<=2048);maxTransfer=std::max(maxTransfer,(unsigned)count);
  if(failWrite)return 0;if(pos+count>size())node->bytes.resize(pos+count);memcpy(node->bytes.data()+pos,in,count);pos+=count;return count;}
 void close(){if(node)operations++;node.reset();}
};
struct Card {
 std::map<std::string,std::shared_ptr<Node>> files;std::set<std::string> dirs;
 bool exists(const char *path){operations++;return files.count(path)||dirs.count(path);}
 bool mkdir(const char *path){operations++;dirs.insert(path);return true;}
 bool remove(const char *path){operations++;return files.erase(path);}
 bool rename(const char *from,const char *to){operations++;
  if(failRenameTo==to){failRenameTo.clear();return false;}
  if(!files.count(from)||files.count(to))return false;files[to]=files[from];files.erase(from);return true;}
 File open(const char *path,int flags=0){operations++;
  if(flags&&!files.count(path))files[path]=std::make_shared<Node>();
  return {files.count(path)?files[path]:nullptr,0,path};}
} SD;
CONTROLLER
REQUEST
unsigned confirmedTarget=9999;
static void confirmKitSave(unsigned target){confirmedTarget=target;}
void saveSamplePack(int pack){beginKitSave(pack);}
void poll(){
 clockUs+=1000;operations=0;serviceKitSave();assert(operations<=1);
 assert(isNowPlaying);++mainLoopPasses; // MIDI/controls keep getting normal loop passes.
}
void run(){for(unsigned n=0;n<10000&&kitSavePending();++n)poll();assert(!kitSavePending());}
void start(int target){currentMode=&set_SamplePack;set_SamplePack.pos[3]=target;beginKitSave(target);assert(kitSavePending());}
std::map<std::string,std::vector<uint8_t>> primary(unsigned kit){
 std::map<std::string,std::vector<uint8_t>> out;
 for(unsigned voice=1;voice<=8;++voice){const auto p=std::to_string(kit)+"/"+std::to_string(voice)+".wav";out[p]=SD.files.at(p)->bytes;}
 return out;
}
int main(){
 for(int ch=1;ch<=8;ch++){
  loadedSampleLen[ch]=ch==8?0:1600+ch;
  for(size_t i=0;i<loadedSampleLen[ch]*2;i++)sampled[ch][i]=(i*37+ch)%255;
 }
 start(4);unsigned progress=0;
 for(unsigned n=0;kitSavePending()&&n<10000;++n){poll();assert(kitSaveProgress()>=progress);progress=kitSaveProgress();}
 assert(!kitSavePending()&&kitSaveFeedback()==2&&samplePackID==4&&SMP.pack==4);
 assert(g_pendingKitEeprom&&g_pendingSp0Eeprom&&backupMarks==1);
 assert(mainLoopPasses>50&&maxTransfer==2048&&currentMode==&set_SamplePack);
 for(int ch=1;ch<=8;ch++){
  const auto &bytes=SD.files.at("4/"+std::to_string(ch)+".wav")->bytes;
  const size_t count=loadedSampleLen[ch]*2;
  if(!count){assert(bytes.empty());continue;}
  assert(bytes.size()==count+44&&!memcmp(bytes.data()+44,sampled[ch],count));
  uint32_t riff=0,body=0;memcpy(&riff,bytes.data()+4,4);memcpy(&body,bytes.data()+40,4);
  assert(riff==count+36&&body==count&&!memcmp(bytes.data(),"RIFF",4));
 }
 // Confirmation discovery also performs at most one lookup per loop.
 operations=0;requestKitSave(4);assert(operations==0&&kitSavePending());
 for(unsigned n=0;kitSavePending()&&n<30;++n)poll();
 assert(confirmedTarget==4&&!kitSavePending());
 // Same-target KIT 0 uses PCM rather than deleting its own source file.
 SD.dirs.insert("0");SD.files["0/1.wav"]=std::make_shared<Node>();SD.files["0/1.wav"]->bytes.assign(16,42);
 SMP.sp0Active[1]=true;start(0);run();assert(kitSaveFeedback()==2&&SD.files.at("0/1.wav")->bytes.size()==loadedSampleLen[1]*2+44);
 SMP.sp0Active[1]=true;const auto custom=SD.files.at("0/1.wav")->bytes;
 sampled[1][0]^=1; // SP0 still holds the previous sample until its deferred copy runs.
 start(5);run();assert(kitSaveFeedback()==2&&SD.files.at("5/1.wav")->bytes[44]==sampled[1][0]);
 assert(SD.files.at("5/1.wav")->bytes!=custom);
 const auto original=primary(4);const auto oldKit=samplePackID;
 // Changing sample buffers aborts staging, while preview channel 0 is harmless.
 start(4);poll();cancelKitSaveForSampleChange(0);assert(!kitSaveSourceChanged);
 cancelKitSaveForSampleChange(1);sampled[1][0]^=2;run();assert(primary(4)==original&&samplePackID==oldKit);
 // Staging failure preserves all eight originals and the loaded identity.
 SMP.sp0Active[1]=true;failWrite=true;start(4);run();failWrite=false;
 assert(kitSaveFeedback()==3&&primary(4)==original&&samplePackID==oldKit&&SMP.sp0Active[1]);
 // Read-back CRC rejects corruption before any primary is installed.
 corruptRead=true;start(4);run();assert(kitSaveFeedback()==3&&primary(4)==original);
 // A failure after earlier slots installed rolls the entire family back.
 failRenameTo="4/5.wav";start(4);run();assert(kitSaveFeedback()==3&&primary(4)==original);
 // Cancellation halfway through installation also restores the old family.
 start(4);
 for(unsigned n=0;n<10000&&!SD.files.count("4/3.wav.kit-bak");++n)poll();
 assert(SD.files.count("4/3.wav.kit-bak"));cancelKitSaveForSampleChange(1);run();assert(primary(4)==original&&samplePackID==oldKit);
 // Recover a previous interrupted rename before replacing a missing primary.
 SD.files["4/8.wav.kit-bak"]=SD.files["4/8.wav"];SD.files.erase("4/8.wav");
 start(4);currentMode=&draw;run();assert(kitSaveFeedback()==2&&samplePackID==4&&primary(4).size()==8);
 for(const auto &file:SD.files)assert(file.first.find(".kit-tmp")==std::string::npos);
 puts("PASS: KIT saves during playback, one <=2048-byte operation per loop, monotonic progress, exact WAVs/live custom PCM, deferred EEPROM flags, read-back CRC, failure/cancel rollback and interrupted-rename recovery");
}
'''.replace('PERSIST',function(menu,'void queueKitIdentityPersist()')).replace('CONTROLLER',controller).replace('REQUEST',function(ops,'FLASHMEM void requestKitSave(unsigned target)'))
with tempfile.TemporaryDirectory(prefix='toern-kit-save-') as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(code)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
