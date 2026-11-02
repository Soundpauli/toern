"""Compile actual remote firmware handlers against deterministic hardware stubs."""
from pathlib import Path
import json
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
harness = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <algorithm>
#include <initializer_list>
#include <iostream>
#include "src/toern_remote_input.h"
using std::min; using std::max;
#define FLASHMEM
#define DMAMEM
#define NUM_LEDS 256
#define LED_MODULES 1
#define MATRIX_WIDTH 16
#define VERSION "test"
constexpr unsigned maxX=16,maxY=16;
unsigned long clockMs=100;
unsigned long millis(){return clockMs;}
struct FakeSerial {
 std::string out,input; bool connected=true;
 int available(){return input.size();} int peek(){return input.empty()?-1:(unsigned char)input[0];}
 int read(){if(input.empty())return -1;int c=(unsigned char)input[0];input.erase(0,1);return c;}
 operator bool()const{return connected;}
 template<class... T> void printf(const char *fmt,T... values){assert(!strstr(fmt,"%.3f"));char b[2048];snprintf(b,sizeof b,fmt,values...);out+=b;}
 void print(const char *s){out+=s;}
 void println(const char *s){out+=s;out+='\n';}
} Serial;
struct CRGB {uint8_t r=0,g=0,b=0;} leds[NUM_LEDS];
bool ledModulesRotated=false;
struct Mode {std::string name="DRAW";unsigned pos[4]={1,2,3,4};} draw,menu,touchyMode;
Mode *currentMode=&draw;
struct {unsigned x=1,y=2,page=1,currentChannel=1;} GLOB;
struct {float bpm=100;unsigned pack=1;} SMP;
unsigned samplePackID=1;
bool fnHeld=false,isNowPlaying=false,fastRecordActive=false,isRecording=false,pressed[4]={};
bool isPressed[4]={};
enum {IDLE,RELEASED};int buttonState[4]={};
int pushes[4]={},releases[4]={},activities=0,fnDelta=0;
struct EncoderStub {
 int value=0,lo=1,hi=16,step=1;
 int readMin(){return lo;} int readMax(){return hi;} int readStep(){return step;}
 void writeCounter(int x){value=x;}
} Encoder[4];
void encoder_button_pushed(EncoderStub*,int i){++pushes[i];isPressed[i]=true;}
void encoder_button_released(EncoderStub*,int i){++releases[i];isPressed[i]=false;buttonState[i]=IDLE;}
void noteUserActivity(){++activities;}
int menuSetting=2;int getCurrentMenuMainSetting(){return menuSetting;}
struct EncoderTurnEvent {int encoder,delta;bool fn;};
bool dispatchFnTurn(const EncoderTurnEvent &event){fnDelta+=event.delta;return true;}
void remotePrintUiState(){Serial.print("\"test_ui\":null");}
void remoteReplyFilters(unsigned long,int){}
void remoteReplyMenus(unsigned long){}
void remoteReplyPerformance(unsigned long){}
#include "toern_remote.ino"
static char sdSerLineBuf[192];static size_t sdSerLineLen=0;static bool sdSerLineOverflow=false;
bool sdSerialLineInProgress(){return sdSerLineLen || sdSerLineOverflow;}
bool inEtcSubmenu=false;
void sdSerExpireClient(){}
void sdSerReply(const char *s){Serial.println(s);}
void sdSerHandleOffPage(char *s){
 if(!strncmp(s,"REMOTE ",7))remoteHandleCommand(s+7);
 else if(!strcmp(s,"PING"))Serial.println("OK TOERN SD");
}
POLL_FUNCTION
std::string cmd(const char *text){char b[192];strcpy(b,text);Serial.out.clear();remoteHandleCommand(b);return Serial.out;}
void good(const char *s){assert(cmd(s).find("\"ok\":true")!=std::string::npos);}
void bad(const char *s){assert(cmd(s).find("\"ok\":false")!=std::string::npos);}
int main(){
 // Inactive at boot: serial activity cannot enable capture or remote controls.
 leds[0]={9,8,7};remoteCaptureDisplay();assert(!remoteDisplayValid&&remoteDisplayFrame[0].r==0);
 bad("40 STATE");bad("41 TURN 1 1");assert(!remoteInput.live&&!remoteInput.pending());
 Serial.input="PING\n";sdSerialServerPollNeedSdHint();assert(!remoteInput.live);
 good("42 HELLO");bad("43 DISPLAY");
 // Real off-page poller: fragmented REMOTE while COLR is open, bounded work,
 // overflow tail cannot become a second command, and old PING still dispatches.
 menuSetting=41;inEtcSubmenu=true;
 Serial.input="REMOTE 30 TO";sdSerialServerPollNeedSdHint();assert(sdSerLineLen==12);
 Serial.input="UCH 5 1\nREMOTE 31 TOUCH 5 0\n";sdSerialServerPollNeedSdHint();assert(remoteTouchHeld(4));
 assert(Serial.available()>0);sdSerialServerPollNeedSdHint();assert(!remoteTouchHeld(4));
 Serial.input=std::string(200,'x')+"REMOTE 32 TOUCH 5 1\nPING\n";
 sdSerialServerPollNeedSdHint();assert(!remoteTouchHeld(4));
 assert(Serial.out.find("OK TOERN SD")!=std::string::npos);
 menuSetting=2;inEtcSubmenu=false;
 good("1 HELLO"); bad("0 STATE");bad("2 TURN 0 2");bad("3 TURN 1 65");
 bad("4 BUTTON 1 2");bad("5 TOUCH 6 1");bad("6 STATE junk");bad("7 TURN 1 2 trailing");
 bad("99999999999999999999999999999999 STATE");bad("8 TURN 1 2garbage");
 good("9 TURN 1 3");bad("10 TURN 1 4");assert(remoteInput.pending());
 int raw=5;remoteApplyEncoderTurn(0,raw,false,false);assert(raw==5&&remoteInput.pending());
 remoteApplyEncoderTurn(0,raw,true,false);assert(raw==8&&Encoder[0].value==8&&!remoteInput.pending());
 good("11 TURN 1 64");remoteApplyEncoderTurn(0,raw,true,false);assert(raw==16);
 good("12 TURN 1 -64");remoteApplyEncoderTurn(0,raw,true,false);assert(raw==1);
 Encoder[0].step=0;good("25 TURN 1 1");remoteApplyEncoderTurn(0,raw,true,false);
 assert(remoteTurnFault&&!remoteInput.pending());Encoder[0].step=1;
 fnHeld=true;good("13 TURN 1 -2");remoteApplyEncoderTurn(0,raw,true,true);assert(raw==1&&fnDelta==-2);fnHeld=false;
 good("14 BUTTON 1 1");good("15 BUTTON 1 1");assert(pushes[0]==1);
 remotePhysicalButton(0,true);good("16 BUTTON 1 0");assert(releases[0]==0);
 good("17 RELEASE");assert(remoteInput.live);assert(remoteInput.held(0));remotePhysicalButton(0,false);assert(releases[0]==1);
 good("18 TOUCH 5 1");assert(remoteTouchHeld(4));
 clockMs+=3000;remoteService();assert(!remoteTouchHeld(4));
 assert(!remoteInput.live&&!remoteDisplayValid);bad("44 KEEPALIVE");
 good("45 HELLO");clockMs=0xfffffff0u;remoteInput.renew(clockMs);good("19 BUTTON 2 1");clockMs=uint32_t(clockMs+3000);remoteService();assert(!remoteInput.held(1));
 good("46 HELLO");good("20 BUTTON 3 1");Serial.connected=false;remoteService();assert(!remoteInput.held(2));Serial.connected=true;
 good("47 HELLO");
 // An existing FSM recovery must also release the physical shadow, so
 // the next real push and managed remote gestures are not suppressed.
 remotePhysicalButton(3,true);const int firstPush=pushes[3];
 remotePhysicalButton(3,true);assert(pushes[3]==firstPush+1); // Lost hardware release.
 const int beforeRecovery=pushes[3];
 isPressed[3]=false;buttonState[3]=IDLE;remoteService();
 assert(!remoteInput.held(3)&&remoteInputRecoveries==1);
 remotePhysicalButton(3,true);assert(pushes[3]==beforeRecovery+1);
 remotePhysicalButton(3,false);
 isRecording=true;bad("21 TURN 1 1");isRecording=false;
 // Capture includes temporary overlays and survives subsequent drawing.
 leds[0]={255,16,32};remoteCaptureDisplay();leds[0]={};
 std::cout<<cmd("22 DISPLAY");
 ledModulesRotated=true;leds[15]={1,2,3};remoteCaptureDisplay();std::cout<<cmd("23 DISPLAY");
 std::cout<<cmd("24 STATE");
 good("48 TOUCH 5 1");good("49 TURN 1 3");remotePhysicalButton(0,true);
 good("50 BYE");assert(!remoteInput.live&&!remoteInput.pending()&&!remoteTouchHeld(4));
 assert(remoteInput.held(0)&&!remoteDisplayValid);bad("51 STATE");
 leds[0]={99,99,99};const auto old=remoteDisplayFrame[0];remoteCaptureDisplay();
 assert(remoteDisplayFrame[0].r==old.r); // No copy while disconnected.
 good("52 HELLO");assert(remoteInput.held(0));bad("53 DISPLAY");
 remotePhysicalButton(0,false);
 good("54 TOUCH 5 1");clockMs+=3000;
 bad("55 TOUCH 5 1");assert(!remoteInput.live&&!remoteTouchHeld(4)); // Timeout enforced at dispatch too.

}
'''
# Include the actual line-framing poller, not a reimplementation.
serial_source = (root / 'toern_sd_serial.ino').read_text()
start = serial_source.index('void sdSerialServerPollNeedSdHint() {')
end = serial_source.index('\n#endif', start)
harness = harness.replace('POLL_FUNCTION', serial_source[start:end])

with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'remote.cpp'
    binary = Path(directory) / 'remote'
    source.write_text(harness)
    flags = ['c++', '-std=c++17', '-fsanitize=address,undefined', '-I', str(root)]
    sdk = Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():
        flags += ['-isystem', str(sdk)]
    subprocess.run(flags + [str(source), '-o', str(binary)], check=True)
    output = subprocess.check_output([str(binary)], text=True)
    disabled_main = r'''int main(){
      assert(cmd("60 HELLO").find("DISABLED")!=std::string::npos);
      Serial.input="REMOTE 61 HELLO\nPING\n";
      sdSerialServerPollNeedSdHint();sdSerialServerPollNeedSdHint();
      assert(Serial.out.find("OK TOERN SD")!=std::string::npos);
    }'''
    disabled = harness[:harness.index('int main(){')] + disabled_main
    source.write_text(disabled)
    subprocess.run(flags + ['-DTOERN_REMOTE_CONTROL=0', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

    frames = [json.loads(line.removeprefix('REMOTE ')) for line in output.splitlines()]
    assert len(bytes.fromhex(frames[0]['rgb'])) == 16*16*3
    # Row-major image is top first: device pixel (1,1) is bottom left.
    assert frames[0]['rgb'][15*16*6:15*16*6+6] == 'ff1020'
    # With rotation, physical index 15 corresponds to logical (1,1) as well.
    assert frames[1]['rgb'][15*16*6:15*16*6+6] == '010203'
    assert frames[2]['mode'] == 'DRAW' and not frames[2]['pending_turns']
    assert frames[2]['bpm'] == 100.0
print('PASS: remote parsing, ranges, backpressure, normal/FN turns, independent holds, expiry/wrap, recording guard, captured/rotated display')
