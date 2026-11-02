"""Render AGEN's real overlay and verify the harmonic/arrangement policy."""
from pathlib import Path
import subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'toern_ui.ino').read_text()
def function(signature):
 start=s.index(signature);end=s.index('{',start)+1;depth=1
 while depth:
  depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
ui='\n'.join(function(x) for x in ['FLASHMEM void drawPageNrOverlay(', 'bool autogenStatusVisible(){','FLASHMEM static void drawAutogenPageState(', 'FLASHMEM void drawAutogenPages(){','FLASHMEM void drawAutogenStatus(){'])
code=r'''
#include <cassert>
#include <cstdio>
#include <string>
#include <algorithm>
#include "src/toern_autogen_music.h"
#define FLASHMEM
#define PROGMEM
#include "font_3x5.h"
#define constrain(v,a,b) std::clamp((v),(a),(b))
using std::min;using std::max;
struct CRGB {uint8_t r,g,b;CRGB(int r=0,int g=0,int b=0):r(r),g(g),b(b){}static const CRGB Black;};
const CRGB CRGB::Black{0,0,0};
unsigned maxX=16,maxY=16,source=1,other=2,remaining=4;bool active=true,ready=false;
struct Mode {} draw,singleMode,menu;Mode *currentMode=&draw;
CRGB pixels[16][32];std::string label;int labelX=0,labelY=0;
bool autogenActive(){return active;}unsigned autogenPage(){return source;}unsigned autogenOtherPage(){return other;}
bool autogenNextReady(){return ready;}unsigned autogenRepeatsLeft(){return remaining;}
void autogenUiSnapshot(unsigned &p,unsigned &o,bool &r,unsigned &n){p=source;o=other;r=ready;n=remaining;}
void light(unsigned x,unsigned y,CRGB c){assert(x>=1&&x<=maxX&&y>=1&&y<=16);pixels[16-y][x-1]=c;}
void drawText(const char*s,int x,int y,CRGB c){
 label=s;labelX=x;labelY=y;
 for(unsigned i=0;s[i];++i){const auto *g=alphabet[s[i]-32];for(unsigned col=0;col<g[0];++col)for(unsigned row=0;row<5;++row)
  if(g[col+1]&(1<<(4-row)))light(x+col,y+row,c);x+=g[0]+1;}
}
UI
void save(const char *directory){
 char path[512];snprintf(path,sizeof(path),"%s/agen-%u.ppm",directory,maxX);auto f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u 16\n255\n",maxX);
 for(unsigned y=0;y<16;++y)for(unsigned x=0;x<maxX;++x){auto c=pixels[y][x];fputc(c.r,f);fputc(c.g,f);fputc(c.b,f);}fclose(f);
}
int main(int argc,char **argv){assert(argc==2);
 for(unsigned width:{16u,32u})for(unsigned page:{1u,256u/width})for(bool done:{false,true})for(unsigned count:{1u,4u,8u}){
  maxX=width;source=page;other=page==256/width?1:page+1;ready=done;remaining=count;
  for(auto &row:pixels)for(auto &c:row)c=CRGB(20,20,20);
  drawAutogenStatus();unsigned lit=0;
  for(unsigned x=1;x<=width;++x){auto c=pixels[0][x-1];if(c.r||c.g||c.b)++lit;
   if(x==source)assert(c.r==220&&c.g==220&&c.b==220);
   else if(x==other)assert(c.r>0&&c.b==0&&(done?c.g>0:c.g==0));
   else assert(!c.r&&!c.g&&!c.b);
  }
  assert(lit==2&&label==std::to_string(count)&&labelY==11&&labelX>=(int)width-4);
  if(page==1&&done&&count==8)save(argv[1]);
 }
 currentMode=&menu;assert(!autogenStatusVisible());currentMode=&singleMode;assert(autogenStatusVisible());active=false;assert(!autogenStatusVisible());
 using namespace toern_autogen_music;
 for(int tonic=0;tonic<12;++tonic)for(bool minor:{false,true})for(int reference:{12,24,36,48,60,72,84,96,108,120})
  for(unsigned phrase=0;phrase<16;++phrase)for(unsigned step=0;step<16;++step)for(unsigned ch:{6u,11u,13u,14u}){
   int pitch=fitPitch(reference,tonic,minor,phrase,ch,step);bool inScale=false,inChord=false;
   for(unsigned d=0;d<7;++d)inScale|=pitch%12==scalePc(tonic,minor,d);
   for(unsigned t=0;t<3;++t)inChord|=pitch%12==chordPc(tonic,minor,phrase,t);
   assert(inScale&&abs(pitch-reference)<=6);
   if(step%4==0)assert(inChord);
   if((ch==6||ch==14)&&step%4==0)assert(pitch%12==chordPc(tonic,minor,phrase,0));
  }
 for(int tonic=0;tonic<12;++tonic)for(bool minor:{false,true}){
  assert(chordPc(tonic,minor,6,0)==tonic);
  assert(degree(minor,3)==degree(minor,2)&&degree(minor,4)==degree(minor,5));
 }
 for(unsigned width:{16u,32u}){
  unsigned early=0,late=0,drop=0;
  for(unsigned step=0;step<width;++step){
   assert(drum(Section::Break,1,step,width,9)==0&&!bassAllowed(Section::Break,step,width));
   if(drum(Section::Build,2,step,width,9)>0){if(step<width/4)++early;if(step>=width*3/4)++late;}
   if(drum(Section::Drop,1,step,width,9)>0){++drop;assert(step%4==0);}
  }
  assert(late>early&&drop==width/4);
 }
 for(unsigned choice=0;choice<100;++choice){assert(repeats(Section::Fill,choice)==1);assert(repeats(Section::Drop,choice)>=4);}
 assert(section(3)==Section::Break&&section(4)==Section::Build&&section(5)==Section::Fill&&section(6)==Section::Drop);
 puts("PASS: two AGEN indicators and right countdown on 16/32 columns; all 12 major/minor keys; chord tones, bass roots, bounded registers, breaks/build/fill/drop dynamics");
}
'''.replace('UI',ui)
out=Path(tempfile.gettempdir())/'toern-agen-visual';out.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='toern-agen-visual-test-') as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-O1','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe),str(out)],check=True)
sys.path.insert(0,str(root/'standalone-tools/toern-mcp'))
from remote import display_png
for ppm in out.glob('*.ppm'):
 _,size,_,rgb=ppm.read_bytes().split(b'\n',3);width,height=map(int,size.split())
 ppm.with_suffix('.png').write_bytes(display_png({'width':width,'height':height,'rgb':rgb.hex()},16))
