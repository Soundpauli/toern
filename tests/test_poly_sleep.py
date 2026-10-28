"""Exercise firmware oscillator sleep/wake without truncating release tails."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_synths.ino').read_text()

def function(signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

harness = r'''
#include <cassert>
#include <cmath>
#include <algorithm>
unsigned now = 0;
unsigned millis(){return now;}
struct elapsedMillis {
 unsigned start = now;
 operator unsigned() const {return now-start;}
 elapsedMillis &operator=(unsigned value){start=now-value;return *this;}
};
namespace toern_audio {struct StateGuard {};}
constexpr int POLY_VOICES=3, INSTRUMENT=0, defaultVelocity=127;
bool pendingPauseAudioCleanup=true;
int semitones[2][3]={}, cents[2][3]={}, noteArray[2][8]={};
unsigned voiceStartTime[2][3]={};
struct {int synth_settings[15][1]={};} SMP;
float synthNoteFrequency(int n){return 440*std::pow(2.f,(n-69)/12.f);}
int random(int,int){return 0;}
int constrain(int x,int low,int high){return std::clamp(x,low,high);}
float mapf(float x,float a,float b,float c,float d){return c+(x-a)*(d-c)/(b-a);}
struct Osc {float amp=1;void amplitude(float a){amp=a;}void frequency(float){}};
struct Env {bool active=false;bool isActive(){return active;}void noteOn(){active=true;}};
Osc oscillators[3][3], dc[3];
Osc *Swaveform1[]={&oscillators[0][0],&oscillators[0][1],&oscillators[0][2]};
Osc *Swaveform2[]={&oscillators[1][0],&oscillators[1][1],&oscillators[1][2]};
Osc *Swaveform3[]={&oscillators[2][0],&oscillators[2][1],&oscillators[2][2]};
Osc *Sdc1[]={&dc[0],&dc[1],&dc[2]};
Env env[3][3];
Env *Senvelope1[]={&env[0][0],&env[0][1],&env[0][2]};
Env *Senvelope2[]={&env[1][0],&env[1][1],&env[1][2]};
Env *SenvelopeFilter1[]={&env[2][0],&env[2][1],&env[2][2]};
'''
harness += function('static void setSoundVoice(')
harness += function('void serviceIdlePolySynthVoices()')
# Execute the preset's actual amplitude assignments as well.
preset = source[source.index('const float oscillatorAmp ='):source.index('// Update filter settings for both ladders')]
harness += '\nvoid presetAmplitude(int i){\n' + '\n'.join(
    line for line in preset.splitlines() if 'oscillatorAmp' in line
) + '\n}\n'
harness += r'''
void check(int voice,float amp){for(auto &bank:oscillators)assert(bank[voice].amp==amp);}
void poll(){now+=4;serviceIdlePolySynthVoices();}
int main(){
 serviceIdlePolySynthVoices(); // Initialize throttle.
 poll();for(int i=0;i<3;++i)check(i,0); // Cold idle.
 setSoundVoice(60,0,100,0,true);
 // Firmware velocity mapping is (velocity-1)/126.
 float amplitude=99.f/126.f;check(0,amplitude);assert(!pendingPauseAudioCleanup);
 poll();check(0,amplitude); // Sustained note.
 env[0][0].active=false;poll();check(0,amplitude); // Right release still active.
 env[1][0].active=false;poll();check(0,0); // Both releases finished.
 presetAmplitude(0);check(0,0); // Editing a silent preset cannot wake it.
 setSoundVoice(64,0,127,0,true);poll();check(0,1); // Retrigger wakes it.
 setSoundVoice(67,0,80,0,false);poll();check(0,79.f/126.f); // Legato retune.
 setSoundVoice(72,0,127,1,true);poll();check(1,1); // Independent voice.
 env[0][0].active=env[1][0].active=false;poll();check(0,0);check(1,1);
 env[0][1].active=env[1][1].active=false;poll();check(1,0);
}
'''
assert 'serviceIdlePolySynthVoices();' in (root/'toern.ino').read_text()
with tempfile.TemporaryDirectory() as directory:
    cpp=Path(directory)/'sleep.cpp'
    binary=Path(directory)/'sleep'
    cpp.write_text(harness)
    flags=['c++','-std=c++17','-fsanitize=address,undefined']
    sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():
        flags += ['-isystem',str(sdk)]
    subprocess.run(flags+[str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS: poly idle, independent release tails, preset edits, retrigger and legato wake')
