"""Render the real FILTER path with a rate-limited display, without DRAW mode."""
from pathlib import Path
import subprocess
import re
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'toern.ino').read_text()
ui = (root / 'toern_ui.ino').read_text()
filters = (root / 'toern_filterUI.ino').read_text()

def function(source, signature):
    source = re.sub(r'//[^\n]*', lambda m: ' ' * len(m[0]), source)
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

draw = function(ui, 'FLASHMEM void drawFilterResetOverlay()')
page = function(filters, 'void showFilterPages(uint8_t chan) {')
start = main.index('    setNewFilters();', main.index('// Set stateMashine'))
end = main.index('  } else if (currentMode->name == "BPM")', start)
render = main[start:end]
start = main.index('    setAllFilterPagesDefaultValues((int)GLOB.currentChannel);')
end = main.index('\n  }', start)
reset = main[start:end]

harness = r'''
#include <cassert>
#include <cstdint>
#include <algorithm>
#define FLASHMEM
struct CRGB { int r,g,b; CRGB(int r=0,int g=0,int b=0):r(r),g(g),b(b){} static const CRGB Black; };
const CRGB CRGB::Black{};
CRGB pixels[17][17];
const int maxX=16,maxY=16;
struct Mode{} filterMode;
Mode *currentMode=&filterMode;
struct {int currentChannel=1;} GLOB;
unsigned long nowMs=100,lastShow=0,filterResetOverlayUntil=0,lastInteraction=0;
unsigned long millis(){return nowMs;}
bool displayedBlue=false;int shows=0;
void light(int x,int y,CRGB c){assert(x>=1&&x<=16&&y>=1&&y<=16);pixels[x][y]=c;}
void FastLEDclear(){for(auto &col:pixels)for(auto &p:col)p=CRGB::Black;}
void FastLEDshow(){
 if(nowMs-lastShow<32)return;lastShow=nowMs;shows++;
 displayedBlue=pixels[5][7].b==255&&pixels[5][7].r==0;
}
void slider(int){for(int y=1;y<=16;y++)light(5,y,CRGB(120,0,0));}
uint8_t filterPage[16]{};
int lastChangedEncoder=-1;
const unsigned FILTER_INTERACTION_TIMEOUT=1000;
struct {int maxValue=32;} sliderDef[16][5][4];
struct {int pos[4];} fakeMode;
template<class T> T constrain(T x,int lo,int hi){return std::max((T)lo,std::min(x,(T)hi));}
void drawCornerValueCustom(int,int,...){ }
bool fnHeld=false;unsigned long sampleGateOverlayUntil=0;
bool sampleGateEnabled(int){return false;}
void drawText(const char*,int,int,CRGB){}
using std::max;
DRAW
PAGE
// Force the competing early presentation on every frame, as while editing.
void setNewFilters(){showFilterPages(GLOB.currentChannel);}
void renderFilter(){RENDER
 FastLEDshow();
}
int resetCount=0;
void setAllFilterPagesDefaultValues(int ch){assert(ch==1);resetCount++;}
void press0002(){RESET}
int main(){
 press0002();assert(resetCount==1);
 for(nowMs=100;nowMs<900;nowMs+=16){renderFilter();assert(displayedBlue);}
 assert(shows>10);
 int blue=0;for(int x=1;x<=16;x++)for(int y=1;y<=16;y++)blue+=pixels[x][y].b==255;
 assert(blue==8);
 nowMs=932;renderFilter();assert(!displayedBlue);assert(pixels[5][7].r==120);
}
'''.replace('DRAW', draw).replace('PAGE', page).replace('RENDER', render).replace('RESET', reset)
# The real page reads encoder positions through Mode.
harness = harness.replace('struct Mode{}', 'struct Mode{int pos[4]{};}')
with tempfile.TemporaryDirectory(prefix='toern-filter-feedback-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: 0002 presents the blue checkmark in FILTER despite early rate-limited redraws, then restores the filter page.')
