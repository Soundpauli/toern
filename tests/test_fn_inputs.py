"""Verify FN is a separate input namespace, leaving normal encoders unchanged."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'toern_fn.ino').read_text()
harness=r'''
#define assert(x) do {if(!(x)) __builtin_trap();}while(false)
using uint8_t=unsigned char;using int8_t=signed char;using int32_t=int;
const int NUM_ENCODERS=4;
struct Mode {int pos[4];};
Mode draw={{2,5,6,7}},menu={{12,13,14,15}},touchyMode={{24,100,0,0}},subpatternMode={},filterMode={},recordMode={};
Mode *currentMode=&draw;
struct EncoderTurnEvent {int encoder;int delta;bool fn;};
struct {int currentChannel=1;} GLOB;
struct i2cEncoderLibV2 {enum {RINC=8,RDEC=16,RMAX=32,RMIN=64};};
struct Enc {
 int lo=1,hi=16,step=1,counter=2,status=0,writes=0;
 int readStatus(){return status;}
 bool readStatus(int flag){return (status&flag)!=0;}
 void writeCounter(int v){counter=v;writes++;}
} Encoder[4];
const int SWITCH_5=39,touchThreshold=45;
int touched=0,activity=0,calls=0,lastDir=0;
bool fastRecordActive=false,isRecording=false,muteModeActive=false,fnHeld=false,subpatternSelector=false;
bool fnButtonHeld[NUM_ENCODERS]={};
unsigned int loadedSampleLen[16]={0,256,256};
unsigned long clockMs=100,sampleGateOverlayUntil=0;
int filterPage[16]={},lastChangedEncoder=0,slotCalls=0,releases=0;
bool pollTouchFillChord(){return false;}
bool sampleGateEnabled(int){return false;}
void setSampleGate(int,bool){}void showFilterPages(int){}
bool subpatternTurn(int){slotCalls++;return true;}
bool subpatternPageTurn(int){return true;}
bool subpatternFnRelease(){releases++;return false;}
int fastTouchRead(int){return touched;}
bool readDebouncedTouch(int){return touched>touchThreshold;}
void noteUserActivity(){activity++;}
unsigned long millis(){return clockMs;}
void applyChannelDirection(uint8_t ch,int8_t dir){assert(ch==GLOB.currentChannel);calls++;lastDir=dir;}
'''+source+r'''
int main(){
 // With FN released every raw counter and button event is untouched, in any mode.
 for(auto mode:{&draw,&menu,&subpatternMode}) {
  currentMode=mode; touched=0;updateFnState();
  for(int enc=0;enc<4;enc++) for(int raw=-10;raw<=100;raw++) {
   int value=raw;assert(!handleFnEncoder(enc,value,true));assert(value==raw&&Encoder[enc].writes==0);
  }
  for(int enc=0;enc<4;enc++) for(int physical:{0,1,2,9}) assert(fnButtonEvent(enc,physical)==physical);
 }
 currentMode=&draw;touched=100;updateFnState();
 int value=1;assert(handleFnEncoder(0,value,true));assert(value==2&&calls==0);
 Encoder[0].status=i2cEncoderLibV2::RDEC;value=1;assert(handleFnEncoder(0,value,true));assert(lastDir==-1&&value==2&&draw.pos[0]==2);
 Encoder[0].status=i2cEncoderLibV2::RINC;value=3;assert(handleFnEncoder(0,value,true));assert(lastDir==1&&value==2);
 // Even at the existing limit a rotation status can dispatch FN direction.
 Encoder[0].status=i2cEncoderLibV2::RDEC;value=2;assert(handleFnEncoder(0,value,true));assert(lastDir==-1);
 Encoder[1].status=i2cEncoderLibV2::RINC;value=6;assert(handleFnEncoder(1,value,true));assert(slotCalls==1&&value==5);
 assert(Encoder[0].lo==1&&Encoder[0].hi==16&&Encoder[0].step==1);
 for(int enc=1;enc<4;enc++){value=42;assert(handleFnEncoder(enc,value,true));assert(value==draw.pos[enc]&&Encoder[enc].counter==draw.pos[enc]);}
 // All four buttons get FN-long=3; releasing FN during the hold never produces 2.
 for(int enc=0;enc<4;enc++) {
  fnHeld=true;assert(fnButtonEvent(enc,2)==3);
  fnHeld=false;assert(fnButtonEvent(enc,2)==3);assert(fnButtonEvent(enc,9)==8);
  assert(fnButtonEvent(enc,2)==2);
  fnHeld=true;assert(fnButtonEvent(enc,1)==4);
 }
 fnHeld=true;int before=calls;
 GLOB.currentChannel=11;value=1;assert(handleFnEncoder(0,value,true));assert(value==2&&calls==before);
 GLOB.currentChannel=3;value=1;assert(handleFnEncoder(0,value,true));assert(value==2&&calls==before);
 GLOB.currentChannel=1;fastRecordActive=true;value=1;assert(handleFnEncoder(0,value,true));assert(value==2&&calls==before);
 fastRecordActive=false;isRecording=true;value=1;assert(handleFnEncoder(0,value,true));assert(value==2&&calls==before);
 isRecording=false;value=1;assert(handleFnEncoder(0,value,false));assert(value==2);
 // Deferred counter writes cannot leak FN movement after the modifier is released.
 fnHeld=false;value=1;assert(handleFnEncoder(0,value,false));assert(value==2);
 value=1;assert(handleFnEncoder(0,value,true));assert(value==2&&Encoder[0].counter==2);
 value=3;assert(!handleFnEncoder(0,value,true));assert(value==3);
 fnHeld=true;
 for(auto mode:{&draw,&menu,&subpatternMode}) {
  currentMode=mode;
  for(int enc=0;enc<4;enc++) {
   Encoder[enc].status=0;value=100;
   assert(handleFnEncoder(enc,value,true));assert(value==mode->pos[enc]);
  }
 }
 currentMode=&draw;touched=100;updateFnState();assert(fnHeld);
 Encoder[1].counter=9;Encoder[1].status=0;draw.pos[1]=5;
 touched=0;updateFnState();
 int leaked=9;
 assert(!fnHeld&&handleFnEncoder(1,leaked,true));
 assert(leaked==5&&Encoder[1].counter==5);
 assert(!finishFnSubpatternRelease(true)&&Encoder[1].counter==5);
 leaked=8;assert(!handleFnEncoder(1,leaked,true)&&leaked==8);
 currentMode=&touchyMode;updateFnState();value=25;
 assert(!fnHeld&&!handleFnEncoder(0,value,true)&&fnButtonEvent(0,2)==2);
}
'''
# Range-based initializer lists need this header on host compilers.
harness='#include <initializer_list>\n'+harness
with tempfile.TemporaryDirectory(prefix='toern-fn-') as tmp:
 p=Path(tmp)/'fn.cpp';exe=Path(tmp)/'fn';p.write_text(harness)
 cmd=['c++','-std=c++17'];sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
 if sdk.exists():cmd+=['-isystem',str(sdk)]
 subprocess.run(cmd+[str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS: normal counters/buttons unchanged, FN 3000 namespace, direction/status dispatch, capture guards and TOUCHY exclusion.')
