"""Exercise the real AGEN menu controls/rendering across both knob views."""
from pathlib import Path
import subprocess, sys, tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'platformio/toern_menu.ino').read_text()
def block(start):
 a=s.index('case 69:',s.index(start));b=s.index('{',a)+1;depth=1;e=b
 while depth:
  depth+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[b:e-1]
render=block('FLASHMEM void drawMainSettingStatus(')
controls=block('FLASHMEM bool handleAdditionalFeatureControls(')
code=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <algorithm>
#include "src/toern_autogen_roles.h"
using std::min;
#define constrain(v,a,b) std::clamp((v),(a),(b))
struct CRGB {int r=0;};
const CRGB UI_WHITE{},UI_ORANGE{},UI_DIM_RED{},UI_GREEN{},UI_CYAN{},UI_YELLOW{},UI_BLUE{};
CRGB col[16];
struct {unsigned pos[4]={};} state,*currentMode=&state;
struct Knob {int low=0,high=0,value=0;void writeMin(int n){low=n;}void writeMax(int n){high=n;}void writeCounter(int n){value=n;}} Encoder[4];
bool menuFirstEnter=true,autogenAssignmentView=false,active=false;
unsigned autogenAssignmentIndex=1;int autogenMenuFocus=2;
uint8_t scale=0,density=1,motion=1;
uint8_t autogenMotion(){return motion;}uint8_t autogenScaleChoice(){return scale;}uint8_t autogenDensity(){return density;}
void setAutogenMotion(uint8_t n){motion=n;}void setAutogenScaleChoice(uint8_t n){scale=n;}void setAutogenDensity(uint8_t n){density=n;}
toern_autogen_roles::Assignments assignments;
unsigned autogenAssignmentKind(unsigned ch){return assignments.kind(ch,ch);}
const char *autogenAssignmentLabel(unsigned ch){return toern_autogen_roles::names[autogenAssignmentKind(ch)];}
void setAutogenAssignment(unsigned ch,unsigned kind){assignments.set(ch,kind);}
bool autogenActive(){return active;}
void menuRequestFullRedraw(){}void redrawMain(int){}
std::string title,value;
CRGB currentMenuParentTextColor(){return {};}
void drawMenuText(const char *text,int,int,CRGB){title=text;}
void drawMenuValue(const char *text,int,int,CRGB){value=text;}
void drawMenuIndicator(char,char,int){}
void control(){int setting=69;switch(setting){case 69:{CONTROLS}}}
void render(){switch(69){case 69:{RENDER}}}
int main(){
 control();render();assert(title=="MOVE"&&value=="SOFT");
 currentMode->pos[0]=1;control();render();assert(title=="SCAL"&&value=="MAJ");
 currentMode->pos[1]=2;control();render();assert(title=="DENS"&&value=="HIGH");
 currentMode->pos[2]=4;control();render();assert(title=="MOVE"&&value=="DEEP");
 autogenAssignmentView=true;control();render();assert(title=="CH1"&&value=="KICK");
 assert(Encoder[0].high==11&&Encoder[1].high==9&&Encoder[2].high==0);
 currentMode->pos[1]=8;Encoder[1].value=8;control();render();assert(value=="VOIC");
 for(unsigned i=1;i<toern_autogen_roles::channelCount;++i){
  currentMode->pos[0]=i;control();render();
  assert(assignments.automatic(toern_autogen_roles::channels[i])==(i!=1));
  assert(Encoder[1].value==(int)autogenAssignmentKind(toern_autogen_roles::channels[i]));
 }
 assert(title=="CH14");
 currentMode->pos[0]=0;control();currentMode->pos[1]=2;control();
 for(unsigned ch:toern_autogen_roles::channels)if(ch)assert(autogenAssignmentKind(ch)==2);
 assert(assignments.role(0)==0); // ALL must never turn empty pattern cells into notes.
 autogenAssignmentView=false;control();render();
 assert(scale==1&&density==2&&motion==4&&title=="MOVE");
 assert(Encoder[0].high==6&&Encoder[1].high==3&&Encoder[2].high==4);
}
'''.replace('CONTROLS',controls).replace('RENDER',render)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,'-I',str(root/'platformio'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: actual AGEN labels follow all three knobs; assignment selection preserves roles; ALL edits all playable channels; view toggles restore settings/ranges.')
