"""Render AGEN's real overlay and verify the harmonic/arrangement policy."""
from pathlib import Path
import subprocess,sys,tempfile
root=(Path(__file__).resolve().parents[2] / 'platformio')
s=(root/'toern_ui.ino').read_text()
def function(signature):
 start=s.index(signature);end=s.index('{',start)+1;depth=1
 while depth:
  depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
ui='\n'.join(function(x) for x in ['FLASHMEM void drawPageNrOverlay(', 'bool autogenStatusVisible(){','FLASHMEM static void drawAutogenPageState(', 'FLASHMEM void drawAutogenPages(){','FLASHMEM static void drawAutogenActivity(', 'FLASHMEM void drawAutogenStatus(){'])
code=r'''
#include <cassert>
#include <cstdio>
#include <string>
#include <algorithm>
#include "src/toern_autogen_music.h"
#include "src/toern_autogen.h"
#define FLASHMEM
#define PROGMEM
#include "font_3x5.h"
#define constrain(v,a,b) std::clamp((v),(a),(b))
using std::min;using std::max;
struct CRGB {uint8_t r,g,b;CRGB(int r=0,int g=0,int b=0):r(r),g(g),b(b){}static const CRGB Black;};
const CRGB CRGB::Black{0,0,0};
unsigned maxX=16,maxY=16,source=1,other=2,remaining=4,progress=0,motifSlot=1;bool active=true,ready=false;
struct Mode {} draw,singleMode,menu;Mode *currentMode=&draw;
CRGB pixels[16][32];std::string label;int labelX=0,labelY=0;CRGB labelColor;
uint32_t clockMs=0;uint32_t millis(){return clockMs;}
unsigned autogenMotifSlot(){return motifSlot;}
bool autogenViewingPerformance(){return true;}
bool autogenActive(){return active;}unsigned autogenPage(){return source;}unsigned autogenOtherPage(){return other;}
bool autogenNextReady(){return ready;}unsigned autogenRepeatsLeft(){return remaining;}
void autogenUiSnapshot(unsigned &p,unsigned &o,bool &r,unsigned &n){p=source;o=other;r=ready;n=progress;}
void light(unsigned x,unsigned y,CRGB c){assert(x>=1&&x<=maxX&&y>=1&&y<=16);pixels[16-y][x-1]=c;}
void drawText(const char*s,int x,int y,CRGB c){
 label=s;labelX=x;labelY=y;labelColor=c;
 for(unsigned i=0;s[i];++i){const auto *g=alphabet[s[i]-32];for(unsigned col=0;col<g[0];++col)for(unsigned row=0;row<5;++row)
  if(g[col+1]&(1<<(4-row)))light(x+col,y+row,c);x+=g[0]+1;}
}
UI
void save(const char *directory){
 char path[512];snprintf(path,sizeof(path),"%s/agen-%u.ppm",directory,maxX);auto f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u 16\n255\n",maxX);
 for(unsigned y=0;y<16;++y)for(unsigned x=0;x<maxX;++x){auto c=pixels[y][x];fputc(c.r,f);fputc(c.g,f);fputc(c.b,f);}fclose(f);
}
int main(int argc,char **argv){assert(argc==2);
 for(unsigned width:{16u,32u})for(unsigned page:{10u,11u})for(bool done:{false,true})for(unsigned fraction=0;fraction<=12;++fraction){
  maxX=width;source=page;other=page==10?11:10;motifSlot=width==16?9:1;ready=done;progress=fraction;
  for(auto &row:pixels)for(auto &c:row)c=CRGB(20,20,20);
  label.clear();clockMs=0;drawAutogenStatus();unsigned lit=0;
  for(unsigned x=1;x<=width;++x){auto c=pixels[0][x-1];if(c.r||c.g||c.b)++lit;
   if(x==source)assert(c.r==220&&c.g==220&&c.b==220);
   else if(x==other)assert(c.r>0&&c.b==0&&(done?c.g>0:c.g==0));
   else if(x==motifSlot)assert(c.r==210&&c.g==30&&c.b==130);
   else if(x<=8)assert(c.r==0&&c.g==0&&c.b==12);
   else if(x<(width>=32?16:width-4))assert(!c.r&&!c.g&&!c.b);
  }
  assert(lit>=10&&label.empty()); // the overlay never paints a numeral
  const unsigned first=width>=32?16:width-4;
  for(unsigned x=first;x<=width;++x){auto c=pixels[0][x-1];
    assert(c.b==(x-first<fraction?230:30));
  }
  for(unsigned y=1;y<16;++y)for(unsigned x=0;x<width;++x){auto c=pixels[y][x];assert(c.r==20&&c.g==20&&c.b==20);}
  if(page==10&&done&&fraction==8)save(argv[1]);
 }
 currentMode=&menu;assert(!autogenStatusVisible());currentMode=&singleMode;assert(autogenStatusVisible());active=false;assert(!autogenStatusVisible());
 for(unsigned width:{16u,32u})for(unsigned passes:{1u,2u,4u,8u}){
  ToernAutogen clock;clock.start(1,width,256,passes);unsigned previous=0,step=1;
  for(unsigned i=0;i<width*passes;++i){const unsigned value=clock.statusProgress(step,17);assert(value>=previous&&value<=17);previous=value;step=clock.advance(step);}
  assert(previous==17&&clock.statusProgress(step,17)==17); // late builder retains complete progress
  clock.publish(2);while(clock.generation==0)step=clock.advance(step);
  assert(clock.statusProgress(step,17)==0&&clock.passesTotal==2);
 }
 ToernAutogen held;held.start(1,32,256,2);unsigned cursor=1;
 for(unsigned i=0;i<32;++i)cursor=held.advance(cursor);
 assert(held.statusProgress(cursor,17)==8); // half filled after the first of two passes
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
  assert(chordPc(tonic,minor,10,0)==tonic);
  assert(degree(minor,6)==degree(minor,8)&&degree(minor,8)!=degree(minor,10));
 }
 for(unsigned width:{16u,32u}){
  unsigned early=0,late=0,drop=0;
  for(unsigned step=0;step<width;++step){
   assert(drum(Section::Break,1,step,width,9)==0&&!bassAllowed(Section::Break,step,width));
   assert(drum(Section::Build,1,step,width,9)==0&&!bassAllowed(Section::Build,step,width));
   if(step>=width-2)assert(!bassAllowed(Section::Fill,step,width));
   if(step==width-1)for(unsigned ch=1;ch<=5;++ch)assert(drum(Section::Fill,ch,step,width,9)==0);
   if(drum(Section::Build,2,step,width,9)>0){if(step<width/4)++early;if(step>=width*3/4)++late;}
   if(drum(Section::Drop,1,step,width,9)>0){++drop;assert(step%4==0);}
  }
  assert(late>early&&drop==width/4);
 }
 for(int tonic=0;tonic<12;++tonic)for(bool minor:{false,true})for(unsigned phrase=0;phrase<16;++phrase){
  const int a=voicingPitch(60,tonic,minor,phrase,0,113),b=voicingPitch(60,tonic,minor,phrase,1,113),c=voicingPitch(60,tonic,minor,phrase,2,113);
  assert(a<b&&b<c&&c-a<=12);
  const int next=voicingPitch(60,tonic,minor,phrase+1,0,113);
  assert(next!=a||voicingPitch(60,tonic,minor,phrase+1,1,113)!=b||voicingPitch(60,tonic,minor,phrase+1,2,113)!=c);
 }
 for(auto requested:{Section::Break,Section::Build,Section::Fill}){
  assert(supportedSection(requested,0)==Section::Groove);
  assert(supportedSection(requested,6)==requested);
 }
 assert(encodePitch(60,-24)==84&&encodePitch(36,-12)==48);
 for(unsigned p=0;p<256;++p){
  assert(volumeScale(Section::Break,1,false,p)==0&&volumeScale(Section::Break,14,true,p)==0);
  assert(volumeScale(Section::Drop,1,false,p)==16&&volumeScale(Section::Drop,14,true,p)==16);
  for(unsigned ch:{1u,2u,3u,5u,6u,7u,8u,11u,13u,14u})for(auto s:{Section::Groove,Section::Variation,Section::Break,Section::Build,Section::Fill,Section::Drop})
   assert(volumeScale(s,ch,ch==6||ch==14,p)<=16);
  if(p)assert(volumeScale(Section::Build,2,false,p)>=volumeScale(Section::Build,2,false,p-1));
 }
 for(unsigned width:{16u,32u})for(unsigned bpm:{40u,60u,90u,120u,180u,300u})
  for(unsigned g=0;g<24;++g){
   const auto s=section(g);
   const unsigned n=repeats(s,g,width,bpm);
   const unsigned cap=s==Section::Build||s==Section::Fill?32000:24000;
   assert(n>=1&&n<=8&&(n==1||2*n*width*15000u<=bpm*cap));
  }
 assert(repeats(Section::Build,4,16,120)==1&&repeats(Section::Build,4,32,120)==1);
 assert(repeats(Section::Fill,20,16,120)==1&&repeats(Section::Fill,20,32,120)==1);
 assert(repeats(Section::Break,10,16,120)==1); // two-bar dip, then the groove returns
 for(unsigned g=0;g<24;g+=2)assert(section(g)==section(g+1)&&degree(false,g)==degree(false,g+1));
 {
  const Section form[]={Section::Groove,Section::Variation,Section::Build,Section::Groove,
      Section::Variation,Section::Break,Section::Groove,Section::Groove,
      Section::Variation,Section::Build,Section::Fill,Section::Drop};
  unsigned bars=0;
  for(unsigned slot=0;slot<12;++slot){
   assert(section(slot*2)==form[slot]);
   // Two generations, each of `passes` pages; a 16-step page is one bar.
   bars+=2*repeats(form[slot],slot*2,16,120);
  }
  assert(bars==64); // the fill and drop close the cycle; nothing cadences at bar 16
  assert(form[2]==Section::Build&&form[3]==Section::Groove); // short rise, kick and bass return
  assert(form[5]==Section::Break&&form[6]==Section::Groove); // short dip, kick and bass return
  unsigned longBars=0;
  for(unsigned slot=0;slot<12;++slot)longBars+=2*repeats(form[slot],slot*2,16,120,true);
  assert(longBars==120); // LONG doubles groove, variation and drop; rolls stay short
  assert(repeats(Section::Drop,22,16,120,true)==4&&repeats(Section::Groove,0,16,120,true)==8);
  for(unsigned width:{16u,32u})for(unsigned bpm:{40u,60u,90u,120u,180u,300u})for(unsigned g=0;g<24;++g){
   const unsigned n=repeats(section(g),g,width,bpm,true);
   assert(n>=1&&n<=8&&(n==1||2*n*width*15000u<=bpm*48000u));
  }
 }
 for(int tonic=0;tonic<12;++tonic)for(bool minor:{false,true})for(unsigned phrase=0;phrase<24;++phrase)
  for(unsigned step=0;step<16;++step)for(unsigned ch:{6u,7u,8u,11u,13u,14u}){
   const int reference=ch==6||ch==14?36:60;
   assert(melodyPitch(reference,tonic,minor,phrase,ch,step,113,MotionLong)==fitPitch(reference,tonic,minor,phrase,ch,step));
   for(unsigned lane=0;lane<3;++lane){
    const int voiced=voicingPitch(reference,tonic,minor,phrase,lane,113,true);
    assert(voiced%12==chordPc(tonic,minor,phrase,lane)&&abs(voiced-reference)<=6);
   }
  }
 unsigned changed=0,large=0,total=0;
 for(int tonic=0;tonic<12;++tonic)for(bool minor:{false,true})for(unsigned phrase=0;phrase<32;phrase++)
  for(unsigned step=0;step<32;step++)for(unsigned ch:{6u,7u,8u,11u,13u,14u}){
   int reference=ch==6||ch==14?36:60;
   int n=melodyPitch(reference,tonic,minor,phrase,ch,step,113);
   int held=melodyPitch(reference,tonic,minor,phrase,ch,step,113,0);
   assert(held==fitPitch(reference,tonic,minor,phrase,ch,step));
   bool inScale=false,inChord=false;
   for(unsigned d=0;d<7;d++)inScale|=n%12==scalePc(tonic,minor,d);
   for(unsigned d=0;d<3;d++)inChord|=n%12==chordPc(tonic,minor,phrase,d);
   assert(n>=0&&n<=127&&abs(n-reference)<=12&&inScale);
   if(step%4==0)assert(inChord);
   if((ch==6||ch==14)&&step%4==0)assert(n%12==chordPc(tonic,minor,phrase,0));
   if(section(phrase)==Section::Variation&&ch==13){++total;changed+=n!=fitPitch(reference,tonic,minor,phrase,ch,step);large+=abs(n-reference)>=4;}
   for(unsigned lane=0;lane<3;lane++){
    int voiced=voicingPitch(reference,tonic,minor,phrase,lane,113);
    bool chord=false;for(unsigned d=0;d<3;d++)chord|=voiced%12==chordPc(tonic,minor,phrase,d);
    assert(chord&&voiced>=24&&voiced<=108);
   }
  }
 assert(changed>total/2&&large>total/3);
 puts("PASS: AGEN page-header progress line on 16/32 columns; all 12 major/minor keys; chord tones, bass roots, bounded registers, breaks/build/fill/drop dynamics");
}
'''.replace('UI',ui)
out=Path(tempfile.gettempdir())/'toern-agen-visual';out.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='toern-agen-visual-test-') as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-O1','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe),str(out)],check=True)
sys.path.insert(0,str(root/'../tools/toern-mcp'))
from remote import display_png
for ppm in out.glob('*.ppm'):
 _,size,_,rgb=ppm.read_bytes().split(b'\n',3);width,height=map(int,size.split())
 ppm.with_suffix('.png').write_bytes(display_png({'width':width,'height':height,'rgb':rgb.hex()},16))
