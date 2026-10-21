"""Exercise actual PQ-I settings, GPIO edge handling and OFF with a fake clock."""
from pathlib import Path
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
s=(root/'toern_midi.ino').read_text()
def function(name):
    import re
    m=re.search(r'^(?:FASTRUN )?(?:static )?(?:inline )?(?:bool|void|uint8_t) '+name+r'\([^\n]*\) \{',s,re.M)
    assert m,name
    start=m.start();brace=s.index('{',start);depth=1;end=brace+1
    while depth:
        if s[end]=='{':depth+=1
        if s[end]=='}':depth-=1
        end+=1
    return s[start:end]
names=['analogClockInputActive','analogClockInPpqn','loadAnalogExtClockFromEEPROM',
       'saveAnalogInSettings','saveAnalogInFlags','resetAnalogInTiming',
       'getAnalogInRateSel','getAnalogInRateSelCount','setAnalogInRateSel',
       'toggleAnalogInPolarity','getAnalogInHoldMs','setAnalogInHoldMs',
       'analogClockCountRise','analogClockIsr','analogClockSyncListen','analogClockCatchIfHigh']
harness=r'''
#include <cstdint>
#include <cassert>
#include <cmath>
#define FASTRUN
#define ANALOG_CLOCK_IN_PIN 38
#define INPUT_PULLDOWN 1
#define INPUT_PULLUP 2
#define CHANGE 3
#define IRQ_GPIO6789 4
#define ANALOG_IN_HOLD_MS_MIN 1
#define ANALOG_IN_HOLD_MS_MAX 32
#define ANALOG_CLOCK_REARM_US 500u
#define NO_CLOCK_TIMEOUT_US 2000000u
#define BPM_MIN 1
#define BPM_MAX 100000
#define EEPROM_DATA_START 43
namespace toern_audio {constexpr int kTimerPriority=224;}
struct PulseOutputGuard {};
struct IntervalTimer {int ended=0;void end(){++ended;}} playTimer;
struct {float bpm=100;} SMP;
struct Storage {uint8_t data[200]={};uint8_t read(int i){return data[i];}} EEPROM;
void saveSingleModeToEEPROM(int i,int8_t v){EEPROM.data[EEPROM_DATA_START+i]=(uint8_t)v;}
const uint8_t ANALOG_IN_PPQN_TABLE[]={1,2,4,8,12,16,24,32};
const uint8_t ANALOG_IN_PPQN_COUNT=8;
bool analogExtClock=true,analogInEnabled=true,analogInPolarityPositive=true;
bool analogClockHigh=false,pendingStartOnBar=false;
uint8_t analogInPpqnIndex=2,analogInHoldMs=1;
uint8_t analogStepPulse=0,analogFillSubs=0,analogSubstepsLeft=0;
uint32_t analogClockLastUs=0,analogClockPeriodUs=0,analogClockFallUs=0,analogInHoldUs=1000;
uint32_t midiClockTickCounter=0,now=0;
double playNoteInterval=0;
int clockMode=0,raw=0,pull=0,directCalls=0;
bool attached=false;
uint32_t micros(){return now;}
int digitalReadFast(int pin){assert(pin==38);return raw;}
int digitalPinToInterrupt(int pin){return pin;}
void detachInterrupt(int pin){assert(pin==38);attached=false;}
void pinMode(int pin,int mode){assert(pin==38);pull=mode;}
static void analogClockIsr();
void attachInterrupt(int pin,void(*fn)(),int mode){assert(pin==38&&fn==analogClockIsr&&mode==CHANGE);attached=true;}
#define NVIC_SET_PRIORITY(irq, priority) assert((irq)==IRQ_GPIO6789&&(priority)==224)
static void analogClockSyncListen();
void analogDirectPulse(){++directCalls;}
void noInterrupts(){} void interrupts(){}
FUNCTIONS
void edge(int level,uint32_t at){now=at;bool changed=raw!=level;raw=level;if(changed&&attached)analogClockIsr();}
int main(){
 // Legacy settings retain rate/hold-off and default to enabled, positive polarity.
 EEPROM.data[EEPROM_DATA_START+52]=1;
 EEPROM.data[EEPROM_DATA_START+53]=2;
 EEPROM.data[EEPROM_DATA_START+59]=255;
 loadAnalogExtClockFromEEPROM();assert(attached&&getAnalogInRateSel()==3&&pull==INPUT_PULLDOWN);
 edge(1,10000);edge(0,12000);assert(midiClockTickCounter==1&&directCalls==1);
 edge(1,14000);edge(0,16000);assert(midiClockTickCounter==2&&analogClockPeriodUs==4000);
 // OFF really detaches, cancels substeps and prevents Play catch from firing.
 analogSubstepsLeft=3;setAnalogInRateSel(0);
 assert(!attached&&getAnalogInRateSel()==0&&analogSubstepsLeft==0&&playTimer.ended>0);
 pendingStartOnBar=true;edge(1,20000);analogClockCatchIfHigh();assert(directCalls==2);
 // Every rate selector maps correctly, while OFF preserves the last rate.
 for(uint8_t i=1;i<=8;++i){setAnalogInRateSel(i);assert(analogInPpqnIndex==i-1&&getAnalogInRateSel()==i);}
 setAnalogInRateSel(0);assert(analogInPpqnIndex==7);setAnalogInRateSel(3);
 // Negative polarity counts falling edges only; returning HIGH is not another pulse.
 raw=1;now=25000;toggleAnalogInPolarity();assert(attached&&pull==INPUT_PULLUP);
 edge(0,30000);edge(1,32000);assert(midiClockTickCounter==3&&directCalls==3);
 edge(0,34000);edge(1,36000);assert(midiClockTickCounter==4&&analogClockPeriodUs==4000);
 setAnalogInHoldMs(7);assert(analogInHoldUs==7000);
 edge(0,38000);edge(1,39000);assert(midiClockTickCounter==4);
 // All four OFF/polarity combinations survive EEPROM reload.
 for(int flags=0;flags<4;++flags){
  EEPROM.data[EEPROM_DATA_START+59]=flags;loadAnalogExtClockFromEEPROM();
  assert(analogInEnabled==!(flags&1)&&analogInPolarityPositive==!(flags&2));
  assert(attached==!(flags&1));
 }
 clockMode=1;loadAnalogExtClockFromEEPROM();assert(!attached);
}
'''.replace('FUNCTIONS','\n'.join(function(n) for n in names))
with tempfile.TemporaryDirectory(prefix='toern-pq-input-') as tmp:
    p=Path(tmp);(p/'input.cpp').write_text(harness)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,str(p/'input.cpp'),'-o',str(p/'input')],check=True)
    subprocess.run([str(p/'input')],check=True)
print('PASS: PQ-I rate mapping, OFF cancels listening/substeps, +/- edges, hold-off and legacy/settings reload.')
