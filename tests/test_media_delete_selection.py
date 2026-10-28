"""Compile the actual sample-browser path snapshot used by confirmed deletion."""
from pathlib import Path
import subprocess, tempfile, sys
root=Path(__file__).resolve().parents[1]
s=(root/'toern_sample.ino').read_text()
a=s.index('static bool buildCurrentBrowseSamplePath(')
b=s.index('// PREV=SYNC while playing',a)
source=s[a:s.index("void restoreSampleBrowserSelection",a)]
restore=s[s.index("void restoreSampleBrowserSelection",a):b]
harness=r'''
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
int constrain(int n,int a,int b){return std::clamp(n,a,b);}
struct Mode{unsigned pos[4]={};} set_Wav,draw;Mode *currentMode=&set_Wav;
struct {int currentChannel=1;} GLOB;
bool fastRecordActive=false,isRecording=false,g_browseListDirty=false;
char g_browseDir[9][128]={},g_wavPickName[3][65]={};
unsigned g_wavPickCount=3;uint8_t types[3]={1,2,2};
uint8_t sampleBrowserEntryTypeAt(int idx){return types[idx];}
struct {struct {unsigned fileID=1;} wav[9];} SMP;
struct {int value=0;void writeCounter(int32_t v){value=v;}} Encoder[4];
void sampleBrowserClampBrowseIndex(int){set_Wav.pos[3]=std::clamp(set_Wav.pos[3],1u,std::max(1u,g_wavPickCount));}
int refreshes=0;void sampleBrowserRefreshList(int){refreshes++;g_browseListDirty=false;}
'''+source+restore+r'''
int main(){
 char path[160];strcpy(g_browseDir[1],"drums");strcpy(g_wavPickName[1],"kick.wav");strcpy(g_wavPickName[2],"snare.wav");
 set_Wav.pos[3]=1;assert(!captureSelectedSampleForDelete(path,sizeof(path)));
 set_Wav.pos[3]=2;assert(captureSelectedSampleForDelete(path,sizeof(path))&&!strcmp(path,"samples/drums/kick.wav"));
 char tiny[8];assert(!captureSelectedSampleForDelete(tiny,sizeof(tiny)));
 set_Wav.pos[3]=3;g_browseListDirty=true;assert(captureSelectedSampleForDelete(path,sizeof(path))&&refreshes==1&&!strcmp(path,"samples/drums/snare.wav"));
 currentMode=&draw;assert(!captureSelectedSampleForDelete(path,sizeof(path)));
 currentMode=&set_Wav;fastRecordActive=true;assert(!captureSelectedSampleForDelete(path,sizeof(path)));
 fastRecordActive=false;isRecording=true;assert(!captureSelectedSampleForDelete(path,sizeof(path)));
 isRecording=false;set_Wav.pos[0]=10;set_Wav.pos[2]=80;restoreSampleBrowserSelection(100);
 assert(Encoder[0].value==10&&Encoder[1].value==80&&Encoder[2].value==0);assert(set_Wav.pos[3]==3&&SMP.wav[1].fileID==3&&Encoder[3].value==3);
 g_wavPickCount=0;restoreSampleBrowserSelection(3);assert(set_Wav.pos[3]==1&&SMP.wav[1].fileID==1);
}
'''
with tempfile.TemporaryDirectory(prefix='toern-media-delete-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(harness)
 flags=['c++','-std=c++17','-fsanitize=address,undefined']
 if sys.platform=='darwin':flags+=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1']
 subprocess.run(flags+[str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: deletion snapshots selected WAV paths; folders, truncated paths and capture are rejected.')
