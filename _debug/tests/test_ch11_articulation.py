"""Check actual CH11 voice allocation and post-filter velocity behavior."""
from pathlib import Path
import ast
import subprocess
import sys
import tempfile

root = (Path(__file__).resolve().parents[2] / 'platformio')
source = (root/'toern_synths.ino').read_text()

def function(signature):
    start = source.index(signature)
    end = source.index('{',start)+1
    depth = 1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}')
        end += 1
    return source[start:end]

module=ast.parse((root/'../_debug/tests/test_poly_sleep.py').read_text())
base=next(ast.literal_eval(n.value) for n in module.body if isinstance(n,ast.Assign)
          and isinstance(n.targets[0],ast.Name) and n.targets[0].id=='harness'
          and isinstance(n.value,ast.Constant))
base=base.replace('void frequency(float){}','void frequency(float v){hz=v;}float hz=0;')
base=base.replace('void noteOn(){active=true;}',
                  'int ons=0,offs=0;void noteOn(){active=true;++ons;}void noteOff(){++offs;}')
code=base+r'''
constexpr int INSTRUMENT_CHANNELS=2,NOTE_EMPTY=-32768;
int notePlaying[2]={};
uint32_t sequencedSoundCurrentTick=0,sequencedSoundLastTick=0;
bool sequencedSoundLegato=false,sequencedSoundTouched[3]={};
void sidechainOnVoice(uint8_t){}
'''
for name in ('static void setSoundVoice(', 'static void releasePolyVoice(',
             'void playSound(', 'void playSequencedSound('):
    code+=function(name)+'\n'
code+=r'''
void clear(){
 for(auto &bank:env)for(auto &e:bank)e=Env{};
 for(auto &bank:noteArray)for(auto &n:bank)n=NOTE_EMPTY;
 for(auto &bank:voiceStartTime)for(auto &t:bank)t=0;
 sequencedSoundCurrentTick=sequencedSoundLastTick=0;
 for(auto &t:sequencedSoundTouched)t=false;
 now=0;
}
void off(int slot){releasePolyVoice(slot);noteArray[0][slot]=NOTE_EMPTY;}
void gains(int slot,int velocity){
 const float expected=CH11_POLY_SUM_GAIN*velocity/127.f;
 assert(std::abs(SmixerL4.gains[slot]-expected)<.00001f);
 assert(std::abs(SmixerR4.gains[slot]-expected)<.00001f);
 for(auto &bank:oscillators)assert(bank[slot].amp==1.f);
}
int main(){
 // Both entry paths use the same velocity VCA after the nonlinear filters.
 for(int velocity:{1,32,64,96,127}){
  clear();playSound(60,0,velocity);gains(0,velocity);
  const float frequency=oscillators[0][0].hz;
  clear();playSequencedSound(60,0,velocity,1,false);gains(0,velocity);
  assert(oscillators[0][0].hz==frequency);
 }
 clear();SMP.synth_settings[11][INSTRUMENT]=10;
 setSoundVoice(60,0,64,0,true);gains(0,64);
 assert(std::abs(dc[0].amp-(.6f+.4f*64/127.f))<.00001f); // Keep 303 accent.
 SMP.synth_settings[11][INSTRUMENT]=0;
 // Note-off frees identity, but a release tail must not monopolize slot 0.
 clear();now=10;playSound(60,0,80);off(0);
 now=20;playSound(64,0,90);assert(noteArray[0][1]==64);
 assert(env[0][0].ons==1&&env[0][0].active);off(1);
 now=30;playSound(67,0,100);assert(noteArray[0][2]==67);off(2);
 // All slots releasing: reclaim the oldest released slot, preserving others.
 now=40;playSound(72,0,110);assert(noteArray[0][0]==72);
 assert(env[0][1].ons==1&&env[0][2].ons==1);
 // A filter-only tail must not delay a fresh note when an idle slot exists.
 clear();env[2][0].active=true;playSound(60,0,100);
 assert(noteArray[0][1]==60&&env[2][0].ons==0);
 // Prefer a released slot over stealing an older key that is still held.
 clear();now=10;playSound(60,0,80);now=20;playSound(64,0,80);
 now=30;playSound(67,0,80);off(2);now=40;playSound(72,0,80);
 assert(noteArray[0][0]==60&&noteArray[0][1]==64&&noteArray[0][2]==72);
 // With three held keys, a fourth still steals the oldest deterministically.
 clear();now=10;playSound(60,0,80);now=20;playSound(64,0,80);
 now=30;playSound(67,0,80);now=40;playSound(72,0,80);
 assert(noteArray[0][0]==72&&noteArray[0][1]==64&&noteArray[0][2]==67);
}
'''
graph=(root/'audioinit.h').read_text()
for side,envname in [('L','Senvelope1'),('R','Senvelope2')]:
    for slot in range(3):
        assert f'({envname}_{slot}, 0, Smixer{side}4, {slot})' in graph
with tempfile.TemporaryDirectory(prefix='toern-ch11-articulation-') as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(code)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: MIDI/grid velocity parity, fixed ladder drive, velocity 1, 303 accent, idle-before-release allocation and held-note protection')
