"""Exercise the actual firmware pad latch with mocked touch readings."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_touchy.ino').read_text()
start = source.index('FLASHMEM void checkTouchyTouches()')
end = source.index('\nFLASHMEM void handleTouchyButtons', start)
function = source[start:end].replace('FLASHMEM ', '')
harness = r'''
#define assert(expression) do { if (!(expression)) __builtin_trap(); } while (false)
using uint8_t = unsigned char;
static constexpr int SWITCH_1=0, SWITCH_2=1, SWITCH_3=2, SWITCH_4=3, SWITCH_5=4;
static constexpr int touchThreshold=45;
bool exttouch=false, touchyTintActive=false, touchState[4]={}, lastTouchState[4]={};
uint8_t touchyLastVoice=1;
int readings[5]={}, triggers[6]={};
unsigned long clockMs=100;
unsigned long millis(){return clockMs;}
int fastTouchRead(int pin){return readings[pin];}
bool readTouch1Pressed(){return readings[0]>touchThreshold;}
bool readTouch2Pressed(){return readings[1]>touchThreshold;}
void selectTouchyVoice(uint8_t){}
void noteUserActivity(){}
int basePitchRowForChannel(uint8_t voice){return voice;}
struct {int channelVol[6];} SMP;
int touchyVelocity[6]={100,100,100,100,100,100};
void applyVoiceAmpGainFromVol(int,int){}
void triggerCurrentVoiceAudition(int voice,int){triggers[voice]++;}
'''
harness += function
harness += r'''
int main(){
  for(int pad=0;pad<5;pad++) {
    readings[pad]=100; checkTouchyTouches();
    assert(triggers[pad+1]==1);
    // Threshold chatter and short complete dips while held cannot rearm.
    for(int i=0;i<100;i++) {
      clockMs+=10; readings[pad]=(i%2)?35:55; checkTouchyTouches();
      clockMs+=10; readings[pad]=0; checkTouchyTouches();
      clockMs+=20; readings[pad]=100; checkTouchyTouches();
    }
    assert(triggers[pad+1]==1);
    readings[pad]=0; checkTouchyTouches();
    clockMs+=59; checkTouchyTouches();
    readings[pad]=100; checkTouchyTouches();
    assert(triggers[pad+1]==1);
    readings[pad]=0; checkTouchyTouches();
    clockMs+=60; checkTouchyTouches();
    readings[pad]=100; checkTouchyTouches();
    assert(triggers[pad+1]==2);
    readings[pad]=0; checkTouchyTouches();
    clockMs+=60; checkTouchyTouches();
  }
  // The external-button variant uses the same stable-release latch.
  exttouch=true;
  readings[0]=100; checkTouchyTouches(); assert(triggers[1]==3);
  readings[0]=0; checkTouchyTouches(); clockMs+=20;
  readings[0]=100; checkTouchyTouches(); assert(triggers[1]==3);
}
'''
with tempfile.TemporaryDirectory(prefix='toern-touchy-inputs-') as directory:
    cpp = Path(directory) / 'inputs.cpp'
    binary = Path(directory) / 'inputs'
    cpp.write_text(harness)
    subprocess.run(['c++', '-std=c++17', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: five firmware pads require a stable release; held/noisy touches trigger once.')
