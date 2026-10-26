"""Compile and exercise firmware clock math, UART transport, and actual handlers."""
from pathlib import Path
import subprocess
import sys
import tempfile
import re

root = Path(__file__).resolve().parents[1]
midi = (root / 'toern_midi.ino').read_text()
sketch = (root / 'toern.ino').read_text()

def function(source, name):
    match = re.search(r'^(?:(?:static|inline|FLASHMEM) )*(?:void|bool) ' + name + r'\([^\n]*\) \{', source, re.M)
    assert match, name
    start = match.start()
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
def run(tmp, name, source):
    path = tmp / (name + '.cpp')
    path.write_text(source)
    subprocess.run(['c++', '-std=c++17', *flags, '-I', str(root), '-I', str(tmp), str(path), '-o', str(tmp / name)], check=True)
    subprocess.run([str(tmp / name)], check=True)

with tempfile.TemporaryDirectory(prefix='toern-midi-clock-') as tmp:
    tmp = Path(tmp)
    run(tmp, 'clock', r'''
#include "src/toern_midi_clock.h"
#include <cassert>
using toern_midi::ClockFollower;
int main() {
 for (double bpm : {40.0, 110.10, 127.65, 300.0}) {
  ClockFollower f;
  const double clock = 60000000.0 / (bpm * 24.0);
  const uint32_t start = 0xffff0000u; // micros wrap in acquisition window
  assert(!f.tick(start));
  for (unsigned i=1; i<=5; ++i) assert(!f.tick(start + uint32_t(std::round(i * clock))));
  assert(f.tick(start + uint32_t(std::round(6 * clock))));
  assert(f.ready() && std::fabs(f.bpm() - bpm) < 0.002);
  for (unsigned i=7; i<=240; ++i) f.tick(start + uint32_t(std::round(i * clock)));
  const uint32_t last = start + uint32_t(std::round(240 * clock));
  assert(f.stable(last) && std::fabs(f.bpm() - bpm) < 0.002);
  assert(!f.stable(last + 500000u));
  assert(!f.tick(last + 2100000u) && !f.ready());
 }
 ClockFollower f;
 double us=1000000, clock=60000000.0/(110.10*24);
 // Endpoint jitter is smoothed without throwing away decimal tempo.
 for (int i=0; i<500; ++i) {
  us += clock;
  f.tick(uint32_t(std::round(us)) + (i%2 ? 500 : 0));
 }
 assert(std::fabs(f.bpm()-110.10) < 0.03);
 // A deliberate tempo change is followed in under two quarter notes.
 clock=60000000.0/(135.25*24);
 for (int i=0; i<48; ++i) {us+=clock; f.tick(uint32_t(std::round(us)));}
 assert(std::fabs(f.bpm()-135.25) < 0.1);
 const double base=f.stepUs();
 const uint32_t boundary=10000000, step=boundary-10000;
 assert(f.correctedStepUs(boundary, step, 0)>base); // early: lengthen
 assert(f.correctedStepUs(boundary, boundary+10000, 0)<base); // late: shorten
 assert(std::fabs(f.correctedStepUs(boundary, boundary+17000, 17000)-base)<1);
 assert(std::fabs(f.correctedStepUs(0x100u, 0xfffff000u, 0)-base-1088)<1);
 assert(std::fabs(f.correctedStepUs(boundary, boundary-uint32_t(base*.45), 0)-base)<=base*.05001);
 // Repeated phase corrections converge rather than free-running at an offset.
 double phase=20000;
 for (int i=0;i<48;++i) phase+=f.correctedStepUs(boundary, boundary+uint32_t(phase),0)-base;
 assert(std::fabs(phase)<2);
}
'''.replace('#include <cassert>', '#include <cassert>\n#include <initializer_list>'))

    (tmp / 'Arduino.h').write_text(r'''
#pragma once
#include <cstdint>
#include <cstddef>
#include <deque>
#define DMAMEM
#define IRQ_LPUART5 5
inline void (*_VectorsRam[32])() = {};
inline uint32_t now=0;
inline uint32_t micros(){return now;}
inline void attachInterruptVector(int irq,void(*fn)()){_VectorsRam[irq+16]=fn;}
inline int priority=0;
#define NVIC_SET_PRIORITY(irq,p) (priority=(p))
struct SerialMock {
 std::deque<int> rx;
 void begin(long);
 int available(){return rx.size();}
 int read(){int v=rx.front();rx.pop_front();return v;}
 size_t write(uint8_t){return 1;}
};
inline SerialMock Serial8;
inline int coreCalls=0;
inline void coreHandler(){++coreCalls;}
inline void SerialMock::begin(long baud){if(baud!=31250) __builtin_abort();_VectorsRam[21]=coreHandler;}
''')
    run(tmp, 'serial', r'''
#include "src/toern_midi_serial.h"
#include <cassert>
int main(){
 toern_midi::TimestampedSerial port;
 port.begin(31250); assert(priority==32);
 Serial8.rx={0x90,60,100,0xf8}; now=20000; _VectorsRam[21]();
 now=40000; Serial8.rx={0xf8}; _VectorsRam[21]();
 now=900000; // parser stalls do not alter captured receive times
 assert(coreCalls==2 && port.available()==5);
 for(int v:{0x90,60,100,0xf8}){assert(port.read()==v);assert(toern_midi::parsedByteUs==20000);}
 assert(port.read()==0xf8 && toern_midi::parsedByteUs==40000);
 assert(port.read()==-1);
 // Queue wrap, full detection and recovery preserve unread bytes in order.
 for(int i=0;i<600;++i)Serial8.rx.push_back(i&127);
 _VectorsRam[21](); assert(port.available()==511 && toern_midi::rxOverruns==89);
 for(int i=0;i<511;++i)assert(port.read()==(i&127));
 port.begin(31250); Serial8.rx={0xf8}; _VectorsRam[21]();
 assert(port.read()==0xf8 && coreCalls==4);
}
''')

    handlers = '\n'.join(function(midi, n) for n in [
        'markExternalOne', 'serviceMidiTransportStart', 'myClock',
        'cancelMidiTransportStart', 'handleStop', 'handleStart',
        'configureMidiClockSend', 'updateMidiClockOutput'])
    ui = '\n'.join(function(sketch, n) for n in ['syncBpmEncoderPositions','updateBPM'])
    run(tmp, 'handlers', r'''
#include "src/toern_midi_clock.h"
#include <cassert>
#include <cmath>
#include <algorithm>
#include <cstdint>
using std::isfinite;
#define FLASHMEM
#define BPM_MIN 40
#define BPM_MAX 300
namespace toern_audio {constexpr int kTimerPriority=224;}
namespace midi {constexpr int Start=0xfa;}
template<class T> T constrain(T x,T lo,T hi){return std::max(lo,std::min(x,hi));}
struct Knob{void writeCounter(int32_t){}} Encoder[4];
struct Mode{unsigned pos[4]={};} bpmMode,draw,singleMode;
Mode *currentMode=&bpmMode;
struct {float bpm=100;} SMP;
struct {unsigned page=1,edit=1;} GLOB;
uint32_t now=1000000;
uint32_t micros(){return now;} uint32_t millis(){return now/1000;}
struct IntervalTimer{
 double interval=0; int starts=0,updates=0;
 void begin(void(*)(),double us){interval=us;++starts;}
 void end(){} void update(double us){interval=us;++updates;} void priority(int){}
} playTimer,fillTimer,midiClockTimer;
struct{void write(uint8_t){} void sendRealTime(int){}}Serial8,MIDI;
bool isNowPlaying=false,pendingStartOnBar=false,touchyEnabled=false;
bool MIDI_CLOCK_SEND=false,MIDI_TRANSPORT_RECEIVE=true,MIDI_TRANSPORT_SEND=false;
bool SMP_PATTERN_MODE=false,analogExtClock=false,pulseClockEnabled=false;
bool midiStartOnClock=false,midiFollowerLockNotified=false,menuEnteredFromSingleMode=false;
bool fillHasTriggered=false,fillRunning=false;
int patternMode=0,pendingPage=0,currentMenuPage=0,clockMode=0;
unsigned beat=1,maxX=16,pulseCount=0,playStartTime=0;
unsigned fillSubTick=0,fillStartSubTick=0,fillTriggerStep=0,fillTriggerRow=0;
unsigned fillActiveChannel=0,fillActiveVelocity=0,fillActiveRow=0;
constexpr uint8_t clocksPerStep=6;
uint8_t midiClockTicks=0,externalStepWithinPage=0;
uint16_t midiClockTickCounter=0;
uint32_t lastMidiClockUs=0,transportStartDelayUntil=0,midiNextClockMicros=0;
const uint32_t NO_CLOCK_TIMEOUT_US=2000000;
float midiClockSendBPM=0;
double midiClockIntervalUs=0,playNoteInterval=0;
volatile uint32_t sequencerRecordStepStartedUs=0;
toern_midi::ClockFollower midiFollower;
unsigned recvDelay=17000,notes=0;
unsigned long effectiveTransportRecvDelayMs(){return recvDelay/1000;}
void triggerExternalOneBlink(){} void pulseClockMidiTick(){} void pulseClockOnTransportStart(){}
void resetTouchFills(){} void deleteActiveCopy(){} void logMidiRxSimple(const char*){}
void analogClockCatchIfHigh(){} void midiClockTick(){}
void switchMode(Mode*m){currentMode=m;} void drawBPMScreen(){}
void playNote(){++notes;sequencerRecordStepStartedUs=now;} void playFillNote(){}
void cancelMidiTransportStart();
void updateMidiClockOutput();
void pause(bool){cancelMidiTransportStart();isNowPlaying=false;}
HANDLERS
UI
int main(){
 // Start arms; first subsequent clock establishes the delayed first step.
 handleStart();assert(midiStartOnClock&&!isNowPlaying);
 now+=9000;myClock(now);assert(!midiStartOnClock&&!isNowPlaying);
 serviceMidiTransportStart();assert(notes==0);
 now+=recvDelay;serviceMidiTransportStart();assert(notes==1&&isNowPlaying);
 assert(std::fabs(playTimer.interval-fillTimer.interval*4)<1);
 // Stop is honored immediately, including before a pending delayed start.
 handleStop();assert(!isNowPlaying);
 handleStart();now+=20000;myClock(now);handleStop();now+=recvDelay;
 serviceMidiTransportStart();assert(notes==1&&!midiStartOnClock&&!pendingStartOnBar);
 // Every clock can adjust playback while display BPM remains fractional.
 midiFollower.reset();midiClockTicks=0;
 const double period=60000000.0/(110.10*24.0);
 for(int i=0;i<49;++i){now=2000000+uint32_t(std::round(i*period));myClock(now);}
 assert(std::fabs(SMP.bpm-110.10)<0.002);
 assert(std::fabs(playNoteInterval-60000000.0/(110.10*4))<2);
 assert(std::fabs(playTimer.interval-fillTimer.interval*4)<1);
 // INT NOTE-only must ignore external clock despite CLOCK_SEND being false.
 clockMode=1;SMP.bpm=123.45f;now+=20000;myClock(now);assert(SMP.bpm==123.45f);
 // Knobs edit tenths; clock following and storage remain fractional.
 syncBpmEncoderPositions();assert(bpmMode.pos[3]==123&&bpmMode.pos[0]==5);
 currentMode=&bpmMode;bpmMode.pos[3]=110;bpmMode.pos[0]=1;MIDI_CLOCK_SEND=true;
 updateBPM();assert(std::fabs(SMP.bpm-110.10)<0.0001);
 assert(std::fabs(midiClockIntervalUs-60000000.0/(110.10*24))<0.01);
 assert(std::fabs(playTimer.interval-fillTimer.interval*4)<1);
 // Fine editing at the upper bound clamps both stored and logical values.
 bpmMode.pos[3]=300;bpmMode.pos[0]=9;updateBPM();
 assert(SMP.bpm==300&&bpmMode.pos[0]==0);
 // Respect GET=off for both currently playing and armed transport.
 MIDI_TRANSPORT_RECEIVE=false;isNowPlaying=true;handleStop();assert(isNowPlaying);
}
'''.replace('HANDLERS', handlers).replace('UI', ui))
print('PASS: acquisition, fractional tempo, jitter, phase correction, rollover, UART queue, transport and timer integration.')
