"""Exercise firmware TOUCHY parameter bindings and recording button lifecycle."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_touchy.ino').read_text()
def function(name, next_marker):
    start = source.index(name)
    return source[start:source.index(next_marker, start)].replace('FLASHMEM ', '')

harness = r'''
#define assert(expression) do { if (!(expression)) __builtin_trap(); } while(false)
using uint8_t=unsigned char;
using int32_t=int;
const int NUM_ENCODERS=4;
enum FilterType {OCTAVE,REVERB,LOWCUT};
struct Mode {int pos[4],minValues[4],maxValues[4];};
Mode touchyMode={{24,100,0,0},{0,1,0,0},{48,127,32,32}},filterMode={},set_Wav={};
Mode *currentMode=&touchyMode;
struct {int currentChannel=1,y=2;} GLOB;
struct {int channelVol[6]; float filter_settings[6][3];} SMP;
struct EncoderMock {int count; void writeCounter(int value){count=value;}} Encoder[4];
uint8_t touchyLastVoice=5,touchyVelocity[6]={100,100,100,100,100,100},oldButtons[4]={};
unsigned int touchySavedVolume[6]={};
bool touchyRecordHeld=false,touchyEnabled=false,isNowPlaying=false,pendingStartOnBar=false,waitForFourBars=false;
bool isPressed[4]={},pendingStartFastRecord=false,fastRecordActive=false,fastRecWaitRelease=false,recordingBorderArmed=false;
int touchyEntryHeldEncoder=-1,recordingStartBeat=99,recordingBeatCount=99,fxType=9,stops=0,menus=0,applied[6][3]={},gain[6]={};
template<class T> T constrain(T v,T lo,T hi){return v<lo?lo:v>hi?hi:v;}
void applyVoiceAmpGainFromVol(int ch,int value){gain[ch]=value;}
void setFilters(FilterType type,int ch,bool){applied[ch][type]++;}
void pause(bool){isNowPlaying=false;}
void stopFastRecord(){stops++;fastRecordActive=false;}
void openTouchyMainMenu(){menus++;}
void switchMode(Mode *mode){currentMode=mode;}
'''
harness += function('void syncTouchyEncoders()', '\nstatic bool touchyTintActive')
harness += function('FLASHMEM void setTouchyEnabled', '\nstatic void selectTouchyVoice')
harness += function('static void selectTouchyVoice', '\nFLASHMEM void checkTouchyTouches')
harness += function('FLASHMEM void handleTouchyButtons', '\nFLASHMEM void showTouchyMode')
start = source.index('FLASHMEM void showTouchyMode()')
harness += source[start:source.index('  static elapsedMillis frame;', start)].replace('FLASHMEM ', '') + '}\n'
harness += r'''
int main(){
  for(int ch=1;ch<=5;ch++) {SMP.channelVol[ch]=ch;SMP.filter_settings[ch][OCTAVE]=24;}
  setTouchyEnabled(true);
  for(int ch=1;ch<=5;ch++) assert(SMP.channelVol[ch]==16 && gain[ch]==16);
  SMP.filter_settings[5][OCTAVE]=30; SMP.filter_settings[5][REVERB]=7; SMP.filter_settings[5][LOWCUT]=9;
  syncTouchyEncoders(); assert(GLOB.currentChannel==5 && Encoder[0].count==30 && Encoder[1].count==100 && Encoder[2].count==7 && Encoder[3].count==9);
  touchyMode.pos[0]=31; touchyMode.pos[1]=65; touchyMode.pos[2]=8; touchyMode.pos[3]=10;
  showTouchyMode();
  assert(SMP.filter_settings[5][OCTAVE]==31 && touchyVelocity[5]==65 && SMP.filter_settings[5][REVERB]==8 && SMP.filter_settings[5][LOWCUT]==10);
  assert(applied[5][OCTAVE]==1 && applied[5][REVERB]==1 && applied[5][LOWCUT]==1);
  touchyLastVoice=3; syncTouchyEncoders(); touchyMode.pos[2]=12; showTouchyMode();
  assert(SMP.filter_settings[3][REVERB]==0 && applied[3][REVERB]==0 && touchyVelocity[3]==100);
  touchyLastVoice=5; syncTouchyEncoders();
  uint8_t states[4]={2,0,0,0}; isPressed[0]=true; handleTouchyButtons(states);
  assert(touchyRecordHeld && pendingStartFastRecord && GLOB.currentChannel==5 && recordingStartBeat==0);
  pendingStartFastRecord=false; fastRecordActive=true; oldButtons[0]=2;
  handleTouchyButtons(states); assert(!pendingStartFastRecord && stops==0);
  states[0]=9; isPressed[0]=false; handleTouchyButtons(states);
  assert(stops==1 && !touchyRecordHeld && !pendingStartFastRecord && !fastRecWaitRelease && currentMode==&touchyMode);
  states[0]=1; handleTouchyButtons(states); assert(currentMode==&set_Wav && GLOB.currentChannel==1 && touchyLastVoice==5);
  currentMode=&touchyMode; syncTouchyEncoders(); assert(GLOB.currentChannel==5 && touchyMode.pos[1]==65);
  setTouchyEnabled(false);
  for(int ch=1;ch<=5;ch++) assert(SMP.channelVol[ch]==ch && gain[ch]==ch);
}
'''
with tempfile.TemporaryDirectory(prefix='toern-touchy-controls-') as directory:
    cpp=Path(directory)/'controls.cpp'; binary=Path(directory)/'controls'
    cpp.write_text(harness)
    subprocess.run(['c++','-std=c++17',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS: firmware TOUCHY targets the last voice, binds pitch/velocity/effects, restores volumes and stops Fastrec on release.')
