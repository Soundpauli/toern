"""Exercise actual chord arbitration and the F/F engine on page downbeats."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'toern.ino').read_text()
def function(signature, source=source):
    start=source.index(signature);end=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
harness=r'''
bool subpatternSelector=false,subpatternReady=false,fnReleaseLatch=false,fnReleaseWork=false;
void subpatternCancelPreview(){}

#define FLASHMEM
#include <cassert>
#include <cstdint>
#include <vector>
#include <initializer_list>
namespace toern_audio {struct StateGuard {};}
struct Mode {} draw,singleMode,menu,recordMode,touchyMode;
Mode *currentMode=&draw;
struct {int currentChannel=1;} GLOB;
bool isNowPlaying=false,isRecording=false,fastRecordActive=false,touchFillChordConsumed=false;
bool sensCountActive=false,touch3PressProcessed=false;
int sensCountSec=0,touch3PressStartTime=0,blockedChannel=-1;
bool pendingStartFastRecord=false,recordingBorderArmed=false,countInPending=false,countInActive=false,countInComplete=false;
int countInBeat=0;
bool isChildVoiceDisabled(int ch){return ch==blockedChannel;}
unsigned maxX=16,maxY=16,maxlen=1024;
const int defaultVelocity=100,NOTE_CONDITION_FILL=21,NOTE_MIDI_PITCH_NONE=255;
struct Note {int channel=0,condition=1;};
Note note[1025][17];
bool fillRunning=false,fillHasTriggered=false;
unsigned fillSubBeat=0,fillSubTick=0,fillStartSubTick=0,fillTriggerStep=0,fillTriggerRow=0;
int fillActiveChannel=0,fillActiveVelocity=0;
uint16_t touchFillPendingMask=0;
uint32_t fillStepsRemaining=0;
struct TouchFillState {bool running=false;uint32_t startSubTick=0,stepsRemaining=0;};
struct FillDropState {bool pending=false;uint8_t row=0,midiPitch=255;};
FillDropState fillDrops[15];
TouchFillState touchFills[15];
int fillRequestOverlayChannel=-1;uint32_t fillRequestOverlayUntil=0;
unsigned fillActiveRow=0;
uint8_t fillActiveMidiPitch=255;
bool muted[15]={};
bool getMuteState(int ch){return muted[ch];}
// Use the real shared debouncer and external C/D pin mapping as well.
#include "toern_touch_input.h"
const int SWITCH_1=0,SWITCH_2=1,SWITCH_3=2,SWITCH_4=3,SWITCH_5=4;
const int BUTTON_A=10,BUTTON_B=11,BUTTON_C=12,BUTTON_D=13,LOW=0,touchThreshold=45;
bool exttouch=false,rawCap[5]={},rawButton[4]={};
bool fnHeld=false;
void noteUserActivity() {}
unsigned clockMs=0;
unsigned millis(){return clockMs;}
int fastTouchRead(int pin){return rawCap[pin]?100:0;}
int digitalReadFast(int pin){return rawButton[pin-10]?LOW:1;}
std::vector<unsigned> hits;
std::vector<int> hitChannels,hitVelocities,hitPitches;
std::vector<unsigned> hitSteps;
unsigned sequencerStep=0;
void triggerFillHit(int ch,int velocity,unsigned int row,uint8_t midiPitch){
 assert(ch>0&&ch<15&&row>0);
 if(muted[ch]||isChildVoiceDisabled(ch))return;
 hits.push_back(fillSubTick);hitChannels.push_back(ch);hitVelocities.push_back(velocity==0?defaultVelocity:velocity);hitPitches.push_back(midiPitch);hitSteps.push_back(sequencerStep);
}
'''+(root/'toern_touch_input.ino').read_text()+'\n'+function('bool readRecTouchHeld() {')+'\n'+'\n'.join(function(sig) for sig in [
 'static void resetTouchFills() {',
 'static inline bool isSingleModeChannelAllowed(',
 'static inline int basePitchRowForChannel(',
 'static bool handleTouchFillChord(bool first, bool second) {',
 'static bool pollTouchFillChord() {',
 'static uint32_t currentFillStepStart() {',
 'static void queueFillDrop(',
 'static void serviceFillStepBoundaries() {',
 'static void triggerPendingFillDrops() {',
 'static void servicePendingTouchFill(',
 'FLASHMEM static void triggerActiveFillHit() {',
 'static bool fillHitDue(',
 'static void advanceTouchFills() {',
 'void playFillNote() {',
])+'\n'+function('static void updateFnState() {',(root/'toern_fn.ino').read_text())+'\n'+function('bool fillRequestOverlayVisible() {',(root/'toern_ui.ino').read_text())+r'''
void release(){handleTouchFillChord(false,false);}
void step(){
 for(int i=0;i<4;i++)playFillNote();
 ++sequencerStep;serviceFillStepBoundaries();triggerPendingFillDrops();
}
void resetFill(){fillRunning=fillHasTriggered=false;fillTriggerStep=fillTriggerRow=0;resetTouchFills();fillSubTick=100;hits.clear();hitChannels.clear();hitVelocities.clear();hitPitches.clear();hitSteps.clear();sequencerStep=0;}
int main(){
 // Playback-only; holding across Play cannot turn an old press into a new fill.
 assert(!handleTouchFillChord(true,true)&&touchFillPendingMask==0);
 isNowPlaying=true;assert(!handleTouchFillChord(true,true)&&touchFillPendingMask==0);
 release();sensCountActive=true;sensCountSec=3;touch3PressStartTime=15;
 pendingStartFastRecord=recordingBorderArmed=countInPending=countInActive=countInComplete=true;countInBeat=3;
 assert(handleTouchFillChord(true,true)&&touchFillPendingMask==(1u<<1));
 assert(fillRequestOverlayChannel==1&&fillRequestOverlayVisible());
 clockMs=999;assert(fillRequestOverlayVisible());
 clockMs=1000;assert(!fillRequestOverlayVisible());
 assert(!sensCountActive&&sensCountSec==0&&touch3PressStartTime==0);
 assert(!pendingStartFastRecord&&!recordingBorderArmed&&!countInPending&&!countInActive&&!countInComplete&&countInBeat==0);
 // Neither half of the released chord may fall through to REC or audition.
 assert(handleTouchFillChord(true,false));assert(handleTouchFillChord(false,true));
 release();assert(!touchFillChordConsumed);
 // Queue captures current voice and survives releasing the combination.
 GLOB.currentChannel=14;assert(fillRequestOverlayChannel==1);servicePendingTouchFill(8);assert(hits.empty());
 servicePendingTouchFill(16);assert(hits.empty());
 servicePendingTouchFill(17);
 assert(hits.size()==1&&hitChannels.back()==1&&touchFills[1].running);
 assert(fillTriggerStep==0&&fillTriggerRow==0);
 // Another voice may be queued while the first voice is already filling.
 handleTouchFillChord(true,true);assert(touchFillPendingMask==(1u<<14));release();
 servicePendingTouchFill(33);assert(touchFills[1].running&&touchFills[14].running);
 for(int width:{16,32}){
  maxX=width;
  for(int ch:{1,4,8,11,13,14}){
   resetFill();GLOB.currentChannel=ch;release();handleTouchFillChord(true,true);release();
   servicePendingTouchFill(width);assert(hits.empty());
   servicePendingTouchFill(width+1);assert(hits.size()==1&&hitChannels.back()==ch&&touchFills[ch].running);
   const unsigned row=basePitchRowForChannel(ch);
   for(int i=0;i<width*2;i++)step();
   assert(!touchFills[ch].running&&!fillRunning&&!fillHasTriggered&&touchFillPendingMask==0);
   auto touchHits=hits;
   assert(hitVelocities.back()==127&&hitSteps.back()==unsigned(width*2));
   assert(hitVelocities[hitVelocities.size()-2]==defaultVelocity);
   // Compare every event with a normal F/F armed at the same phase.
   resetFill();fillRunning=true;fillStartSubTick=100;fillActiveChannel=ch;fillActiveRow=row;fillActiveVelocity=defaultVelocity;fillActiveMidiPitch=255;fillStepsRemaining=width*2;
   fillTriggerStep=3;fillTriggerRow=row;note[3][row]={ch,NOTE_CONDITION_FILL};
   triggerActiveFillHit();
   for(int i=0;i<width*2;i++)step();
   assert(hits==touchHits&&note[3][row].condition==1&&hitVelocities.back()==127&&hitSteps.back()==unsigned(width*2));
  }
 }
 resetFill();release();GLOB.currentChannel=1;
 // Queue several channels in the preceding page; none may replace another.
 maxX=16;
 for(int ch:{1,2,3,4,5,6,7,8,11,13,14}){GLOB.currentChannel=ch;handleTouchFillChord(true,true);release();}
 GLOB.currentChannel=2;handleTouchFillChord(true,true);release(); // Deduplicate same voice.
 servicePendingTouchFill(16);assert(hits.empty());
 servicePendingTouchFill(17);assert(hits.size()==11&&touchFillPendingMask==0);
 for(int ch:{1,2,3,4,5,6,7,8,11,13,14})assert(touchFills[ch].running&&touchFills[ch].startSubTick==100);
 // A grid F/F for an already filling voice must not double-trigger it.
 fillActiveChannel=1;auto hitCount=hits.size();triggerActiveFillHit();assert(hits.size()==hitCount);
 for(int i=0;i<16;i++)step();
 GLOB.currentChannel=1;handleTouchFillChord(true,true);release();
 servicePendingTouchFill(33);assert(touchFills[1].startSubTick==164);
 muted[11]=true;
 for(int i=0;i<16;i++)step();
 assert(touchFills[1].running);
 for(int ch:{2,3,4,5,6,7,8,11,13,14})assert(!touchFills[ch].running);
 int together=0;
 for(unsigned i=0;i<hits.size();i++)if(hitSteps[i]==32&&hitVelocities[i]==127){
   assert(hitChannels[i]!=1&&hitChannels[i]!=11);++together;
 }
 assert(together==9); // all remaining audible voices share the drop downbeat
 for(int i=0;i<16;i++)step();assert(!touchFills[1].running);
 assert(hitChannels.back()==1&&hitVelocities.back()==127&&hitSteps.back()==48);
 muted[11]=false;resetFill();GLOB.currentChannel=1;
 // Clock callback ordering must not anchor the new fill one beat backward.
 resetFill();maxX=16;fillSubTick=103;GLOB.currentChannel=1;
 release();handleTouchFillChord(true,true);release();servicePendingTouchFill(17);
 assert(touchFills[1].startSubTick==104&&hits.size()==1);
 playFillNote();assert(fillSubTick==104&&hits.size()==1); // no duplicate first hit
 for(int i=0;i<28;i++)playFillNote();assert(fillSubTick==132&&hits.size()==1);
 for(int i=0;i<4;i++)playFillNote();assert(fillSubTick==136&&hits.size()==2);
 // Fill timer can reach the end before the sequencer downbeat: stay armed,
 // then strike once on that downbeat rather than clearing early or one step late.
 resetFill();GLOB.currentChannel=1;release();handleTouchFillChord(true,true);release();
 servicePendingTouchFill(1);
 for(int i=0;i<31;i++)step();
 for(int i=0;i<4;i++)playFillNote();
 assert(touchFills[1].running&&hitVelocities.back()!=127);
 ++sequencerStep;serviceFillStepBoundaries();triggerPendingFillDrops();
 assert(!touchFills[1].running&&hitVelocities.back()==127&&hitSteps.back()==32);
 auto total=hits.size();step();assert(hits.size()==total);
 // Opposite callback order: sequencer fires before the fill timer on the drop.
 resetFill();GLOB.currentChannel=14;release();handleTouchFillChord(true,true);release();
 servicePendingTouchFill(1);
 for(int i=0;i<31;i++)step();
 ++sequencerStep;serviceFillStepBoundaries();
 // A normal grid note on the same step must be followed by the full drop.
 triggerFillHit(14,42,basePitchRowForChannel(14),255);
 triggerPendingFillDrops();
 assert(hitVelocities[hitVelocities.size()-2]==42&&hitVelocities.back()==127&&hitSteps.back()==32);
 total=hits.size();for(int i=0;i<4;i++)playFillNote();assert(hits.size()==total);
 // A quiet grid F/F still gets a 127 drop with its preserved pitch.
 resetFill();fillRunning=true;fillStepsRemaining=32;fillStartSubTick=100;
 fillActiveChannel=11;fillActiveVelocity=42;fillActiveRow=12;fillActiveMidiPitch=72;
 fillTriggerStep=5;fillTriggerRow=12;note[5][12]={11,NOTE_CONDITION_FILL};
 triggerActiveFillHit();for(int i=0;i<32;i++)step();
 assert(hitVelocities.front()==42&&hitVelocities.back()==127&&hitPitches.back()==72&&hitSteps.back()==32);
 assert(note[5][12].condition==1&&!fillRunning);
 resetFill();GLOB.currentChannel=1;
 // Invalid voices and disabled child voices cannot queue audio.
 for(int ch:{0,9,10,12,15}){GLOB.currentChannel=ch;handleTouchFillChord(true,true);release();assert(touchFillPendingMask==0);}
 GLOB.currentChannel=1;blockedChannel=1;handleTouchFillChord(true,true);release();assert(touchFillPendingMask==0);blockedChannel=-1;
 // Recording and TOUCHY retain their existing pad assignments.
 for(auto mode:{&recordMode,&touchyMode}){currentMode=mode;assert(!handleTouchFillChord(true,true));release();assert(touchFillPendingMask==0);}
 currentMode=&draw;
 fastRecordActive=true;assert(!handleTouchFillChord(true,true));release();fastRecordActive=false;
 isRecording=true;assert(!handleTouchFillChord(true,true));release();isRecording=false;
 // Capacitive 4+5 and external buttons C+D enter the hardware-specific chord.
 for(bool external:{false,true}){
   resetFill();release();exttouch=external;
   rawCap[3]=rawCap[4]=true;rawButton[2]=rawButton[3]=true;
   pollTouchFillChord();
   clockMs+=10;
   assert(pollTouchFillChord());
   assert(touchFillPendingMask==(1u<<1)&&!readRecTouchHeld());
   updateFnState();assert(!fnHeld);
   rawCap[4]=false;rawButton[3]=false;
   pollTouchFillChord();clockMs+=30;
   assert(pollTouchFillChord());
   assert(!readRecTouchHeld());
   rawCap[3]=false;rawButton[2]=false;
   pollTouchFillChord();clockMs+=30;
   assert(!pollTouchFillChord());
 }
 // Old capacitive 3+4 must no longer queue a fill; touch5 alone remains FN.
 exttouch=false;resetFill();release();
 rawCap[2]=rawCap[3]=true;pollTouchFillChord();clockMs+=10;
 assert(!pollTouchFillChord()&&touchFillPendingMask==0);
 rawCap[2]=rawCap[3]=false;pollTouchFillChord();clockMs+=30;pollTouchFillChord();
 rawCap[4]=true;updateFnState();clockMs+=10;updateFnState();assert(fnHeld);
 assert(touchFillPendingMask==0);
 rawCap[4]=false;updateFnState();clockMs+=30;updateFnState();assert(!fnHeld);
 resetFill();
 // Pending fills cannot fire while transport is stopped.
 touchFillPendingMask=(1u<<1);isNowPlaying=false;servicePendingTouchFill(1);assert(hits.empty());
}
'''
with tempfile.TemporaryDirectory(prefix='toern-touch-fill-') as tmp:
    cpp=Path(tmp)/'fill.cpp';exe=Path(tmp)/'fill';cpp.write_text(harness)
    cmd=['c++','-std=c++17','-I',str(root)]
    sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():cmd+=['-isystem',str(sdk)]
    subprocess.run(cmd+[str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: playback-only touch 4+5/C+D queues multiple voices together on the next page, preserves independent timing/re-arm, consumes REC/audition, matches F/F cadence and strikes a single velocity-127 drop on the exact next downbeat in either timer order.')
