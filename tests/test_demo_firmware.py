"""Check the actual firmware DEMO timeline, rendering dispatch and exit latch."""
from pathlib import Path
import subprocess
import tempfile
import sys
root = Path(__file__).resolve().parents[1]
source = (root / 'toern_helpers.ino').read_text()
section = source[source.index('static bool demoAnimationActive'):source.index('// ----- Run the Animation Once')]
harness = r'''
#include <cassert>
#include <cstdint>
#include "src/toern_demo.h"
#define FLASHMEM
struct CRGB { int r,g,b; CRGB(int a=0,int b=0,int c=0):r(a),g(b),b(c){} static const CRGB Red,White; };
const CRGB CRGB::Red(255,0,0), CRGB::White(255,255,255);
CRGB CHSV(uint8_t,int,int){return CRGB(1,2,3);}
bool isNowPlaying=true,touchDown=false,mTop=false;
int buttons[4]={},buttonState[4]={},IDLE=0;
bool pressed[4]={},isPressed[4]={};
int maxX=16,alphabet[95][4],pauses=0,redraws=0,cats=0,logos=0,waves=0,letters=0,flushed=0;
uint32_t now=100;
uint32_t millis(){return now;}
void pause(bool skip){assert(skip);isNowPlaying=false;++pauses;}
void allOff(){} void stopAllSetWavPreviewAudio(){}
bool readTouch1Pressed(){return touchDown;}
void FastLEDclear(){} void FastLEDshow(){++flushed;}
void menuRequestFullRedraw(){++redraws;} void noteUserActivity(){}
void drawTouchyCatFrame(unsigned long,CRGB){++cats;}
void drawLogoAnimationFrame(unsigned long t,bool hold,bool wavesOnly){if(wavesOnly){assert(hold);++waves;}else{assert(t<5000&&!hold);++logos;}}
bool mPixels[5][5]={};
void light(int x,int y,CRGB c){assert(y>=4&&y<=13);++letters;if(x>=17&&x<27&&c.r==255)mPixels[4-(y-4)/2][(x-17)/2]=true;}
SECTION
int main(){
 for(auto &g:alphabet){g[0]=3;g[1]=g[2]=g[3]=31;}
 alphabet[0][1]=alphabet[0][2]=alphabet[0][3]=0;
 alphabet['M'-32][2]=6;
 const int width=112;
 for(int cols:{16,32}) {
  const uint32_t scroll=(cols+width+1)*65u, cycle=10000+3*scroll;
  using toern_demo::Phase;
  assert(toern_demo::frame(9999,cols,width).phase==Phase::Nyan);
  Phase phases[]={Phase::Red,Phase::White,Phase::Rainbow};
  for(int i=0;i<3;++i){
   auto f=toern_demo::frame(10000+i*scroll,cols,width);
   assert(f.phase==phases[i]&&f.x==cols+1);
   auto end=toern_demo::frame(10000+(i+1)*scroll-1,cols,width);
   assert(end.phase==phases[i]&&end.x<=-width+1);
  }
  assert(toern_demo::frame(10000+3*scroll,cols,width).phase==Phase::Nyan);
  assert(toern_demo::frame(cycle-1,cols,width).phase==Phase::Rainbow);
  for(uint32_t n:{1,2,100}){
   auto f=toern_demo::frame(n*cycle,cols,width);
   assert(f.phase==Phase::Nyan&&f.elapsed==0);
  }
 }
 startDemoAnimation();assert(pauses==1&&demoAnimationActive);
 assert(serviceDemoAnimation()&&cats==1);
 now=demoAnimationStarted+10000;serviceDemoAnimation();assert(letters==12*3*5*4-16&&waves==1);
 const char *mRows[]={"#...#","##.##","#.#.#","#...#","#...#"};
 for(int y=0;y<5;y++)for(int x=0;x<5;x++)assert(mPixels[y][x]==(mRows[y][x]=='#'));
 now=demoAnimationStarted+10000+3*(maxX+width+1)*65;
 serviceDemoAnimation();assert(cats==2&&logos==0);
 now=demoAnimationStarted+3*(10000+3*(maxX+width+1)*65);
 serviceDemoAnimation();assert(cats==3&&demoAnimationActive&&logos==0);
 touchDown=true;now+=5;serviceDemoAnimation();assert(demoAnimationExiting);
 int displayed=flushed;now+=100;serviceDemoAnimation();assert(flushed==displayed);
 touchDown=false;now+=5;assert(serviceDemoAnimation());
 assert(!demoAnimationActive&&redraws==1&&!serviceDemoAnimation());
}
'''.replace('SECTION', section)
with tempfile.TemporaryDirectory(prefix='toern-demo-') as tmp:
    tmp = Path(tmp)
    (tmp/'demo.cpp').write_text(harness)
    flags = ['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,'-include','initializer_list','-I',str(root),str(tmp/'demo.cpp'),'-o',str(tmp/'demo')],check=True)
    subprocess.run([str(tmp/'demo')],check=True)
print('PASS: firmware DEMO phases, off-grid passes, repeated cycles, shared renderers and Touch 1 press/release exit.')
