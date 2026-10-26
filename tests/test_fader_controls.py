"""Exercise the real fader processing/poller with four simulated seesaw ADCs."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern.ino').read_text()
ui_source = (root / 'toern_ui.ino').read_text()

def function(signature, source=source):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

constants = source[source.index('static const uint16_t FADER_ADC_MAX'):source.index('static void applyVoiceAmpGainFromVol')]
harness = r'''
#include <cassert>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#define FLASHMEM
using std::min;
template<class T>T constrain(T x,T lo,T hi){return x<lo?lo:x>hi?hi:x;}
unsigned long now = 1000;
unsigned long millis() { return now; }
struct elapsedMillis {
  unsigned long start;
  elapsedMillis(unsigned long value=0):start(now-value){}
  operator unsigned long() const {return now-start;}
  elapsedMillis& operator=(unsigned long value){start=now-value;return *this;}
};
const int NEOSLIDER_COUNT=4, maxY=16, maxPages=3, SS_NEOSLIDER_ADC_CH=18;
unsigned int neoSliderDisabledMask=0;
bool exttouch=true, neoSliderAnyPresent=true, fastRecordActive=false, isRecording=false;
struct Mode {} draw,menu,subpatternMode,set_Wav;
Mode *currentMode=&draw;
const int INT_PIN=27,LOW=0;
int digitalRead(int){return LOW;} // Continuous pending encoder activity.
bool preview=false;
struct {bool isPlaying(){return preview;}} playSdWav1;
int adc[4]={523,523,523,523}, reads[4]={}, activity=0, flushes=0;
bool fail[4]={};
struct Seesaw {
 int index;
 bool analogReadFast(int,uint16_t &out){reads[index]++; if(fail[index])return false;out=adc[index];return true;}
} neoSliderSS[4]={{0},{1},{2},{3}};
bool neoSliderPresent[4]={true,true,true,true};
int neoSliderLastVol[4]={-1,-1,-1,-1}, neoSliderLedVolWanted[4]={};
bool neoSliderInMuteDz[4]={},neoSliderInFullDz[4]={},neoSliderMuteLatched[4]={};
float neoSliderVelScale[4]={1,1,1,1},neoSliderLastVelNorm[4]={1,1,1,1},amp[5]={};
elapsedMillis neoSliderActiveUntil[4],g_faderMovedAgo;
const int neoSliderVoiceCh[4]={1,2,3,4};
bool neoSliderOverlayActive[4]={};
elapsedMillis neoSliderOverlayAge[4];
float neoSliderOverlayLevel[4]={};
bool childLockEnabled=false,tmpMuteActive=false;
struct {unsigned currentChannel=5;} GLOB;
bool prevMuteState[maxY+1]={};
bool muted[maxY+1]={};
bool getMuteState(int ch){return muted[ch];}
struct {unsigned int mute[maxY+1]={};bool globalMutes[maxY]={},pageMutes[maxPages][maxY]={};} SMP;
bool globalMutes[maxY]={},pageMutes[maxPages][maxY]={};
bool fullMuteSavedGlobal[maxY]={},fullMuteSavedPages[maxPages][maxY]={};
bool fullMuteActive=false;
enum class FullMuteView {AllMuted};
FullMuteView fullMuteView=FullMuteView::AllMuted;
void applyMutesAfterPMODSwitch(){}
bool getMuteStateForUI(int ch){return muted[ch];}
void setMuteState(int ch,bool value){assert(ch>=0&&ch<maxY);muted[ch]=value;}
void applyPlayAmplitude(int ch,float value){amp[ch]=value;}
void noteUserActivity(){activity++;}
void neoSliderFlushPendingLeds(){flushes++;}
''' + r'''
struct CRGB {
 int r,g,b;
 CRGB(int r=0,int g=0,int b=0):r(r),g(g),b(b){}
 void nscale8(uint8_t s){r=r*s/255;g=g*s/255;b=b*s/255;}
 static const CRGB White;
};
const CRGB CRGB::White={255,255,255};
CRGB blend(CRGB a,CRGB b,uint8_t f){return CRGB((a.r*(255-f)+b.r*f)/255,(a.g*(255-f)+b.g*f)/255,(a.b*(255-f)+b.b*f)/255);}
CRGB col[5]={{},{255,0,0},{0,255,0},{0,0,255},{255,255,0}};
const uint8_t sliderCols[4][2]={{2,3},{6,7},{10,11},{14,15}};
unsigned long neoSliderOverlayFrame=0;
int pixels[17]={};
void light(int x,int y,CRGB){assert(x>=1&&x<=16&&y>=1&&y<=16);pixels[x]++;}
''' + constants+'\n'+function('static inline bool isNeoSliderEnabled(')+'\n'+function('static inline bool isNeoSliderControlledChannel(')+'\n'+function('static bool restoredMuteWithFaderPriority(')+'\n'+function('static void syncMuteArraysToSmp() {')+'\n'+function('FLASHMEM void fullMuteExitRestore() {')+'\n'+function('void tmpMuteAll(bool pressed) {')+'\n'+function('static void processOneNeoSlider(')+'\n'+function('void updateNeoSliderVolume() {')+'\n'+function('bool neoSliderOverlayVisible() {',ui_source)+'\n'+function('FLASHMEM void drawNeoSliderOverlay() {',ui_source)+r'''
void tick(){now+=4;updateNeoSliderVolume();}
void scan(){for(int i=0;i<4;i++)tick();}
int main(){
 uint16_t raw[4]={0xFFFF,0xFFFF,0xFFFF,0xFFFF};
 // Boot picks up positions without flashing feedback or unmuting a manual mute.
 muted[1]=true;
 for(int i=0;i<4;i++)processOneNeoSlider(i,raw);
 assert(muted[1] && activity==0);
 for(bool active:neoSliderOverlayActive)assert(!active);
 float initial=neoSliderVelScale[0];
 // Each increment is below hysteresis; accepted movement must accumulate.
 for(int n=1;n<=120;n++){
   for(int i=0;i<4;i++) {adc[i]=523-n;processOneNeoSlider(i,raw);}
 }
 assert(!muted[1]);
 for(int i=0;i<4;i++){
   assert(neoSliderVelScale[i]>initial+0.1f);
   assert(neoSliderOverlayActive[i]);
   assert(std::fabs(amp[i+1]-neoSliderVelScale[i])<0.001f);
 }
 // 0200 -> move multiple faders -> 0000 must not restore a stale mute.
 muted[1]=muted[2]=true;
 tmpMuteAll(true);assert(muted[1]&&muted[2]);
 adc[0]-=100;adc[1]-=100;
 processOneNeoSlider(0,raw);processOneNeoSlider(1,raw);
 assert(!muted[1]&&!muted[2]);
 float soloScale0=neoSliderVelScale[0],soloScale1=neoSliderVelScale[1];
 tmpMuteAll(false);
 assert(!muted[1]&&!muted[2]);
 assert(neoSliderVelScale[0]==soloScale0&&neoSliderVelScale[1]==soloScale1);
 // Physical position also wins on repeated enter/exit without another move.
 muted[1]=true;tmpMuteAll(true);tmpMuteAll(false);assert(!muted[1]);
 // Bottom-stop moves during solo remain muted on release.
 tmpMuteAll(true);adc[0]=1023;processOneNeoSlider(0,raw);tmpMuteAll(false);
 assert(muted[1]&&amp[1]==0);
 adc[0]=100;processOneNeoSlider(0,raw);assert(!muted[1]);
 // Only read/present faders override snapshots. Other voices retain their state.
 muted[6]=true;muted[7]=false;tmpMuteAll(true);tmpMuteAll(false);
 assert(muted[6]&&!muted[7]);
 neoSliderPresent[1]=false;muted[2]=true;tmpMuteAll(true);tmpMuteAll(false);
 assert(muted[2]);neoSliderPresent[1]=true;
 int savedVol=neoSliderLastVol[1];neoSliderLastVol[1]=-1;
 muted[2]=true;tmpMuteAll(true);tmpMuteAll(false);assert(muted[2]);neoSliderLastVol[1]=savedVol;
 exttouch=false;muted[1]=true;tmpMuteAll(true);tmpMuteAll(false);
 assert(muted[1]);exttouch=true;
 // Full-mute restoration must apply the same rule to global and page mutes.
 adc[0]=1023;processOneNeoSlider(0,raw);
 for(int ch=0;ch<maxY;ch++){
   fullMuteSavedGlobal[ch]=ch%2==0;
   for(int page=0;page<maxPages;page++)fullMuteSavedPages[page][ch]=(page+ch)%2==0;
 }
 fullMuteActive=true;fullMuteExitRestore();assert(!fullMuteActive);
 for(int ch=0;ch<maxY;ch++){
   bool fader=ch>=1&&ch<=4;
   bool expected=fader?neoSliderInMuteDz[ch-1]:fullMuteSavedGlobal[ch];
   assert(globalMutes[ch]==expected&&SMP.globalMutes[ch]==expected&&SMP.mute[ch]==expected);
   for(int page=0;page<maxPages;page++){
     bool pageExpected=fader?neoSliderInMuteDz[ch-1]:fullMuteSavedPages[page][ch];
     assert(pageMutes[page][ch]==pageExpected&&SMP.pageMutes[page][ch]==pageExpected);
   }
 }
 // ADC jitter neither unmutes a manually muted voice nor resets feedback age.
 muted[2]=true;int before=activity;
 for(int n=0;n<100;n++){adc[1]=303+(n%3)-1;processOneNeoSlider(1,raw);}
 assert(muted[2] && activity==before);
 // Stop transitions override display-step hysteresis, including tiny crossings.
 adc[0]=1023-60;processOneNeoSlider(0,raw);
 adc[0]=1023-51;processOneNeoSlider(0,raw);
 assert(muted[1] && neoSliderLastVol[0]==0 && amp[1]==0);
 adc[0]=1023-74;processOneNeoSlider(0,raw);assert(muted[1]);
 adc[0]=1023-75;processOneNeoSlider(0,raw);
 assert(!muted[1] && neoSliderLastVol[0]>0 && amp[1]>0);
 adc[0]=1023-971;processOneNeoSlider(0,raw);
 adc[0]=1023-972;processOneNeoSlider(0,raw);
 assert(neoSliderLastVol[0]==16 && amp[1]==1);
 // Failed reads cannot turn into a full-volume jump or affect another fader.
 float last=amp[1];uint16_t accepted=raw[0];
 fail[0]=true;adc[0]=0;processOneNeoSlider(0,raw);
 assert(amp[1]==last && raw[0]==accepted);
 // Poller remains fair when a device fails; each tick reads only one ADC.
 updateNeoSliderVolume(); // Initialize the poll timer before advancing fake time.
 for(int i=0;i<4;i++)reads[i]=0;
 scan();for(int i=0;i<4;i++)assert(reads[i]==1);
 scan();for(int i=0;i<4;i++)assert(reads[i]==2);
 // Menu, mute/solo (SUBPATTERN), and sample selection all keep polling,
 // including while the encoder interrupt stays asserted.
 int scans=2;
 for(auto mode:{&menu,&subpatternMode,&set_Wav,&draw}) {
   currentMode=mode;scan();++scans;
   for(int i=0;i<4;i++)assert(reads[i]==scans);
 }
 // Missing devices don't waste the next present device's turn.
 neoSliderPresent[1]=false;
 for(int i=0;i<4;i++)reads[i]=0;
 for(int i=0;i<3;i++)tick();
 assert(reads[0]==1 && reads[1]==0 && reads[2]==1 && reads[3]==1);
 // Preview/capture must retain the existing hardware transport guards.
 before=flushes;
 preview=true;scan();preview=false;
 fastRecordActive=true;scan();fastRecordActive=false;
 isRecording=true;scan();isRecording=false;
 assert(flushes==before);
 scan();assert(flushes>before);
 // Render separate animated lanes; smooth height changes cannot overwrite peers.
 for(int i=0;i<4;i++)neoSliderOverlayActive[i]=false;
 assert(!neoSliderOverlayVisible());
 for(int i:{0,2}){
   neoSliderOverlayActive[i]=true;neoSliderOverlayAge[i]=0;
   neoSliderOverlayLevel[i]=0;neoSliderVelScale[i]=i==0?1.0f:0.5f;
 }
 neoSliderOverlayFrame=now-20;
 assert(neoSliderOverlayVisible());drawNeoSliderOverlay();
 assert(neoSliderOverlayLevel[0]>0 && neoSliderOverlayLevel[0]<16);
 assert(neoSliderOverlayLevel[2]>0 && neoSliderOverlayLevel[2]<8);
 for(int x=1;x<=16;x++)assert(pixels[x]==((x==2||x==3||x==10||x==11)?16:0));
 // Moving one fader extends only its own feedback window.
 now+=600;neoSliderOverlayAge[2]=0;now+=300;
 for(int &count:pixels)count=0;
 drawNeoSliderOverlay();
 assert(!neoSliderOverlayActive[0] && neoSliderOverlayActive[2]);
 assert(pixels[2]==0 && pixels[10]==16);
 now+=851;assert(!neoSliderOverlayVisible());
 drawNeoSliderOverlay();assert(!neoSliderOverlayActive[2]);
 // Broken fader 2 is ignored even if a stale present flag says it exists.
 neoSliderDisabledMask=1u<<1;neoSliderPresent[1]=true;
 assert(!isNeoSliderControlledChannel(2)&&isNeoSliderControlledChannel(1));
 for(int &r:reads)r=0;
 const float previousScale=neoSliderVelScale[1];
 const bool previousMute=muted[2];
 for(int v:{0,1023,0,1023}){adc[1]=v;processOneNeoSlider(1,raw);}
 assert(reads[1]==0&&neoSliderVelScale[1]==previousScale&&muted[2]==previousMute);
 for(int i=0;i<12;i++)tick();
 assert(reads[1]==0&&reads[0]>0&&reads[2]>0&&reads[3]>0);
 neoSliderInMuteDz[1]=true;muted[2]=false;
 tmpMuteAll(true);tmpMuteAll(false);assert(!muted[2]);
 for(int i=0;i<4;i++)neoSliderOverlayActive[i]=false;
 neoSliderOverlayActive[1]=true;neoSliderOverlayAge[1]=0;
 assert(!neoSliderOverlayVisible());
 for(int &n:pixels)n=0;drawNeoSliderOverlay();
 for(int n:pixels)assert(n==0);

}
'''
with tempfile.TemporaryDirectory(prefix='toern-faders-') as tmp:
    cpp = Path(tmp) / 'faders.cpp'
    exe = Path(tmp) / 'faders'
    cpp.write_text(harness)
    cmd = ['c++', '-std=c++17']
    sdk = Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():
        cmd += ['-isystem', str(sdk)]
    subprocess.run(cmd + [str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: four slow/simultaneous faders, jitter, mute/full stops, failed ADCs, fair polling, capture guards animated matrix bars and fader priority over solo/full-mute snapshots.')
