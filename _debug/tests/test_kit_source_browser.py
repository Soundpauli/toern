"""KIT source restoration selects the original folder/file and loaded white mark."""
from pathlib import Path
import subprocess, sys, tempfile
root=Path(__file__).resolve().parents[2]/'platformio'
sample=(root/'toern_sample.ino').read_text()
helpers=(root/'toern_helpers.ino').read_text()
def function(text,signature):
    start=text.index(signature);end=text.index('{',start)+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
functions='\n'.join(function(sample,s) for s in ['const char *getVoiceLoadedPathRel(', 'void restoreVoiceSourceBrowsePath(', 'static void setVoiceLoadedPathRel(', 'void syncVoiceLoadedPathsFromStored('])
functions+='\n'+function(helpers,'FLASHMEM void sampleBrowserSyncBrowseFromStoredPath(')
marker=sample[sample.index('    bool selectionIsLoaded = false;'):sample.index('    renderLine(selectionLabel,')]
code=r'''
#include <cassert>
#include <cstring>
#include <strings.h>
#include <cstdint>
#include <algorithm>
#include <cstdio>
using std::max;
#define FLASHMEM
constexpr int maxFiles=9,SAMPLE_BROWSER_PATH_MAX=128,SAMPLE_BROWSER_ENTRY_FILE=2;
struct {char samplePathRel[9][128]={};bool sp0Active[9]={};} SMP;
char voiceLoadedPathRel[9][128]={},g_browseDir[9][128]={},g_wavPickName[4][65]={};
bool voiceSourceFromKit[9]={};
uint8_t g_wavPickType[4]={};uint16_t g_wavPickCount=0;
struct Mode {unsigned pos[4]={};} wav;Mode *currentMode=&wav;
struct Enc {int value=0,maximum=0;void writeCounter(int32_t n){value=n;}void writeMax(int32_t n){maximum=n;}} Encoder[4];
void sampleBrowserRefreshList(int ch) {
 g_wavPickCount=0;
 if(!strcmp(g_browseDir[ch],"Drums/808")) {
  g_wavPickCount=3;
  strcpy(g_wavPickName[0],"Hat.wav");strcpy(g_wavPickName[1],"Kick Long.wav");strcpy(g_wavPickName[2],"Snare.wav");
  for(auto &type:g_wavPickType)type=SAMPLE_BROWSER_ENTRY_FILE;
 }
}
FUNCTIONS
bool loadedMark(int ch) {
 bool selectionIsFileEntry=true;int ei=int(currentMode->pos[3])-1;
 MARKER
 return selectionIsLoaded;
}
int main() {
 // A KIT load has restored its embedded source. Pattern settings may contain an
 // old browser preview path, which must not replace the actual loaded identity.
 setVoiceLoadedPathRel(1,"Drums/808/Kick Long.wav");
 voiceSourceFromKit[1]=true;SMP.sp0Active[1]=true;
 strcpy(SMP.samplePathRel[1],"preview/wrong.wav");
 syncVoiceLoadedPathsFromStored();assert(!strcmp(SMP.samplePathRel[1],"Drums/808/Kick Long.wav"));
 sampleBrowserSyncBrowseFromStoredPath(1);
 assert(!strcmp(g_browseDir[1],"Drums/808")&&currentMode->pos[3]==2&&Encoder[3].value==2);
 assert(loadedMark(1));
 // Merely browsing/previewing a neighbour cannot change its source or white mark.
 currentMode->pos[3]=3;strcpy(SMP.samplePathRel[1],"Drums/808/Snare.wav");
 assert(!loadedMark(1)&&!strcmp(getVoiceLoadedPathRel(1),"Drums/808/Kick Long.wav"));
 restoreVoiceSourceBrowsePath(1);sampleBrowserSyncBrowseFromStoredPath(1);assert(loadedMark(1));
 // Legacy kits with no origin must not inherit a stale browser path/loaded mark.
 setVoiceLoadedPathRel(2,nullptr);strcpy(SMP.samplePathRel[2],"unrelated.wav");
 syncVoiceLoadedPathsFromStored();assert(!SMP.samplePathRel[2][0]);
 sampleBrowserSyncBrowseFromStoredPath(2);assert(!g_browseDir[2][0]&&!loadedMark(2));
 // Missing original file is a browsing fallback, not a different loaded source.
 setVoiceLoadedPathRel(3,"Missing/absent.wav");restoreVoiceSourceBrowsePath(3);
 sampleBrowserSyncBrowseFromStoredPath(3);assert(!loadedMark(3));
 // Boot restores scratch overrides from pattern state after loading KIT audio.
 SMP.sp0Active[4]=true;strcpy(SMP.samplePathRel[4],"Drums/808/Snare.wav");
 syncVoiceLoadedPathsFromStored();assert(!strcmp(getVoiceLoadedPathRel(4),"Drums/808/Snare.wav"));
 puts("PASS: KIT origins survive settings restore/preview, select the original directory and file, drive white loaded marks, and handle legacy/missing sources plus SP0 boot overrides");
}
'''.replace('FUNCTIONS',functions).replace('MARKER',marker)
with tempfile.TemporaryDirectory(prefix='toern-kit-source-browser-') as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(code)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
