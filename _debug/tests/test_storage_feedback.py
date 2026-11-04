"""Exercise actual nonblocking storage UI state and pause-autosave acknowledgement."""
from pathlib import Path
import subprocess,sys,tempfile
root=(Path(__file__).resolve().parents[2] / 'platformio')
ui=(root/'toern_ui.ino').read_text();ops=(root/'toern_fileoperations.ino').read_text()
state=ui[ui.index('static bool storageVisualActive'):ui.index('FLASHMEM void showSaveSuccessAnimation')]
autosave=ops[ops.index('static unsigned autoSaveWriteSerial'):ops.index('// ---- Voice filter-page presets')]
code=r'''
#include "src/toern_storage_visual.h"
#include <cassert>
#include <string>
#include <vector>
#define FLASHMEM
unsigned maxX=32,now=10000;unsigned millis(){return now;}
struct Mode {} draw,set_SamplePack;Mode *currentMode=&set_SamplePack;
unsigned presentations=0,activity=0,writes=0;bool lastPatternSaveOk=false,saveOk=true,preventPaintUnpaint=true;
void noteUserActivity(){activity++;}void FastLEDshow(){presentations++;}
void savePattern(bool autosave){assert(autosave);writes++;lastPatternSaveOk=saveOk;}
struct StorageVisualCanvas {
 static std::vector<std::string> labels;
 void clear(){labels.clear();}void pixel(int,int,toern_storage_visual::Color){}
 void icon(bool,int,int,unsigned){}int textWidth(const char*s){return std::string(s).size()*4-1;}
 void text(const char*s,int,int,toern_storage_visual::Color){labels.push_back(s);}
};
std::vector<std::string> StorageVisualCanvas::labels;
STATE
AUTOSAVE
int main(){
 storageTransferBegin(true,true);storageTransferProgress(40);storageTransferProgress(20);
 assert(storageTransferVisible()&&storageTransferIs(true,true)&&storageVisualPercent==40);
 currentMode=&draw;assert(!storageTransferVisible()&&storageTransferIs(true,true));
 currentMode=&set_SamplePack;assert(storageTransferVisible());
 storageTransferFinish(true,false);drawStorageTransferOverlay();
 assert(StorageVisualCanvas::labels==std::vector<std::string>{"100%"});
 now+=200;drawStorageTransferOverlay();assert(StorageVisualCanvas::labels.empty());
 now+=900;assert(!storageTransferVisible());
 storageTransferBegin(true,false);storageTransferPart(3,8);storageTransferBytes(1,2);
 assert(storageVisualPercent==43);storageTransferPart(4,8);assert(storageVisualPercent==50);
 storageTransferFinish(true,false);now+=1000;
 // Only an actual completed autosave acknowledges durable storage; no false
 // white check when the existing five-second cooldown suppresses the write.
 autoSaveWithFeedback();assert(writes==1&&storageVisualWhite&&storageVisualSuccess&&!storageVisualHadProgress);
 const auto first=storageVisualResultAt;now++;autoSaveWithFeedback();assert(writes==1&&storageVisualResultAt==first);
 now+=1000;assert(!storageTransferVisible());
 // Pause acknowledgement temporarily covers a concurrent background KIT save,
 // then its progress resumes, including when the autosave itself fails.
 storageTransferBegin(true,true);storageTransferProgress(60);now+=5000;
 autoSaveWithFeedback();assert(writes==2&&storageTransferIs(true,true)&&storageVisualWhite);
 drawStorageTransferOverlay();assert(StorageVisualCanvas::labels.empty());
 now+=721;drawStorageTransferOverlay();assert(StorageVisualCanvas::labels==std::vector<std::string>{"60%"});
 saveOk=false;now+=5000;autoSaveWithFeedback();assert(writes==3&&!storageVisualSuccess&&storageTransferIs(true,true));
 now+=721;drawStorageTransferOverlay();assert(StorageVisualCanvas::labels==std::vector<std::string>{"60%"});
 storageTransferDismiss();assert(!storageTransferVisible());
 storageTransferBegin(true,false);storageTransferFailure();storageTransferFinish(true,false);
 assert(!storageVisualSuccess&&storageTransferVisible());
 puts("PASS: monotonic transfer progress, 100% completion, nonblocking results, white check only after real pause-autosave, cooldown and concurrent KIT progress");
}
'''.replace('STATE',state).replace('AUTOSAVE',autosave)
with tempfile.TemporaryDirectory(prefix='toern-storage-feedback-') as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(code)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
