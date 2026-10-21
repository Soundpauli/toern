"""Exercise the shared firmware debounce and real TOUCHY pad dispatch."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'toern_touchy.ino').read_text()
function=source[source.index('FLASHMEM void checkTouchyTouches()'):source.index('\nFLASHMEM void handleTouchyButtons')].replace('FLASHMEM ','')
reader=(root/'toern_touch_input.ino').read_text()
harness=f'#include "{root / "toern_touch_input.h"}"\n'+r'''
#define assert(x) do {if(!(x)) __builtin_trap();}while(false)
static constexpr int SWITCH_1=0,SWITCH_2=1,SWITCH_3=2,SWITCH_4=3,SWITCH_5=4,touchThreshold=45;
bool exttouch=false,touchyTintActive=false,touchState[4]={},lastTouchState[4]={},touchyPrevious[5]={};
uint8_t touchyLastVoice=1;
int readings[5]={},triggers[6]={};
unsigned long clockMs=100;
unsigned long millis(){return clockMs;}
int fastTouchRead(int pin){return readings[pin];}
static constexpr int BUTTON_A=10,BUTTON_B=11,BUTTON_C=12,BUTTON_D=13,LOW=0;
int buttonReadings[16]={};
int digitalReadFast(int pin){
 if(pin>=BUTTON_A&&pin<=BUTTON_D) return buttonReadings[pin]>touchThreshold ? LOW : 1;
 return readings[pin]>touchThreshold ? LOW : 1;
}
bool readTouch1Pressed(){return readings[0]>touchThreshold;}
bool readTouch2Pressed(){return readings[1]>touchThreshold;}
void selectTouchyVoice(uint8_t){}
void noteUserActivity(){}
int basePitchRowForChannel(uint8_t voice){return voice;}
struct {int channelVol[6];} SMP;
int touchyVelocity[6]={100,100,100,100,100,100};
void applyVoiceAmpGainFromVol(int,int){}
void triggerCurrentVoiceAudition(int voice,int){triggers[voice]++;}
'''+reader+function+r'''
int main(){
 assert(TOUCH_PRESS_MS==10&&TOUCH_RELEASE_MS==30);
 // Uniform timing, including clock rollover.
 for(uint32_t start : {100u,0xfffffff0u}) {
  TouchDebouncer pad;
  assert(!pad.update(true,start));assert(!pad.update(true,start+9));assert(pad.update(true,start+10));
  assert(pad.update(false,start+11));assert(pad.update(false,start+40));assert(!pad.update(false,start+41));
 }
 for(int pad=0;pad<5;pad++) {
  readings[pad]=100;checkTouchyTouches();clockMs+=9;checkTouchyTouches();assert(triggers[pad+1]==0);
  clockMs+=1;checkTouchyTouches();assert(triggers[pad+1]==1);
  for(int i=0;i<100;i++) {
   readings[pad]=40;checkTouchyTouches();clockMs+=29;checkTouchyTouches();
   readings[pad]=100;checkTouchyTouches();clockMs+=10;checkTouchyTouches();
  }
  assert(triggers[pad+1]==1); // Held/noisy pad never retriggers.
  readings[pad]=45;checkTouchyTouches();clockMs+=30;checkTouchyTouches();
  readings[pad]=100;checkTouchyTouches();clockMs+=10;checkTouchyTouches();assert(triggers[pad+1]==2);
  readings[pad]=0;checkTouchyTouches();clockMs+=30;checkTouchyTouches();
 }
 // Repeated legitimate taps and independent state for every pad.
 for(int pad=0;pad<5;pad++) for(int hit=0;hit<20;hit++) {
  readings[pad]=100;checkTouchyTouches();clockMs+=10;checkTouchyTouches();assert(triggers[pad+1]==3+hit);
  readings[pad]=40;checkTouchyTouches();clockMs+=30;checkTouchyTouches();
 }
 // Cap pads must not fire under exttouch; A–D buttons map to pads 0–3.
 exttouch=true;
 for(int pad=0;pad<5;pad++){readings[pad]=100;checkTouchyTouches();clockMs+=10;checkTouchyTouches();assert(triggers[pad+1]==22);}
 buttonReadings[BUTTON_A]=100;checkTouchyTouches();clockMs+=10;checkTouchyTouches();assert(triggers[1]==23);
 buttonReadings[BUTTON_A]=0;checkTouchyTouches();clockMs+=30;checkTouchyTouches();
 buttonReadings[BUTTON_C]=100;checkTouchyTouches();clockMs+=10;checkTouchyTouches();assert(triggers[3]==23);
 buttonReadings[BUTTON_C]=0;checkTouchyTouches();clockMs+=30;checkTouchyTouches();
 buttonReadings[BUTTON_D]=100;checkTouchyTouches();clockMs+=10;checkTouchyTouches();assert(triggers[4]==23);
 buttonReadings[BUTTON_D]=0;checkTouchyTouches();clockMs+=30;checkTouchyTouches();
}
'''
harness='#include <initializer_list>\n'+harness
with tempfile.TemporaryDirectory(prefix='toern-touch-inputs-') as tmp:
 cpp=Path(tmp)/'inputs.cpp';exe=Path(tmp)/'inputs';cpp.write_text(harness)
 cmd=['c++','-std=c++17'];sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
 if sdk.exists():cmd+=['-isystem',str(sdk)]
 subprocess.run(cmd+[str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
print('PASS: all five pads share 10/30 ms timing; noise, held pads, repeated hits, external buttons and clock rollover.')
