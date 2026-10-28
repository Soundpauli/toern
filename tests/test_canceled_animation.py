"""Check the actual generic blue cancellation animation on both matrix widths."""
from pathlib import Path
import subprocess, tempfile, sys
root=Path(__file__).resolve().parents[1]
source=(root/'toern_ui.ino').read_text()
start=source.index('FLASHMEM void showCanceledAnimation(')
source=source[start:source.index('FLASHMEM void drawRecordingBorder()',start)]
harness=r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <vector>
using std::min;
#define FLASHMEM
struct CRGB {int r,g,b;CRGB(int r,int g,int b):r(r),g(g),b(b){}};
unsigned maxX=16,maxY=16,now=0,yields=0;
struct Mode{} draw,other;Mode *currentMode=&other;
std::map<std::pair<int,int>,CRGB> frame;
std::vector<decltype(frame)> frames;
unsigned millis(){return now;}void delay(unsigned t){now+=t;}
long map(long x,long a,long b,long c,long d){return c+(x-a)*(d-c)/(b-a);}
void switchMode(Mode *m){currentMode=m;}
void FastLEDclear(){frame.clear();}void FastLEDshow(){frames.push_back(frame);}
void sdIoYield(){++yields;}
void light(int x,int y,CRGB c){assert(x>=1&&x<=(int)maxX&&y>=1&&y<=(int)maxY);frame.insert_or_assign({x,y},c);}
'''+source+r'''
int main(){
 for(unsigned width:{16u,32u}) {
  maxX=width;now=0;frames.clear();currentMode=&other;
  showCanceledAnimation(true);assert(currentMode==&draw&&now>=1000&&frames.size()>50);
  const int cx=(maxX+1)/2,cy=(maxY+1)/2;
  const auto &full=frames[30];
  for(int x=-1;x<=1;++x){auto p=full.at({cx+x,cy-1});assert(p.b==220&&p.r==0&&p.g==0);}
  assert(full.count({cx-1,cy+1})&&full.count({cx+1,cy+1}));
  assert(!full.count({cx,cy-2})); // no curved smile beneath the straight mouth
  for(auto &f:frames)for(auto &p:f)assert(p.second.b>0&&p.second.r==0&&p.second.g==0);
  assert(frames.back().at({cx,cy-1}).b<full.at({cx,cy-1}).b);
  currentMode=&other;showCanceledAnimation(false);assert(currentMode==&other);
 }
}
'''
with tempfile.TemporaryDirectory(prefix='toern-cancel-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(harness)
 flags=['c++','-std=c++17','-fsanitize=address,undefined']
 if sys.platform=='darwin':flags+=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1']
 subprocess.run(flags+[str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: blue neutral cancellation face, reveal/fade, audio yields and DRAW return on 16/32 columns.')
