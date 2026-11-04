"""Keep speed optimization local, and verify the ladder's hot helpers inline."""
from pathlib import Path
import os
import re
import subprocess

root=(Path(__file__).resolve().parents[2] / 'platformio')
class Env:
    flags=['-Wall','-Os','--specs=nano.specs','-mfpu=fpv5-d16']
    def AddBuildMiddleware(self,callback):self.callback=callback
    def get(self,key,default):return list(self.flags) if key=='CCFLAGS' else default
    def Object(self,node,**kwargs):return [dict(source=node,**kwargs)]
env=Env()
namespace={'Import':lambda name:None,'env':env}
script=root/'scripts/platformio_audio_dsp.py'
exec(compile(script.read_text(),str(script),'exec'),namespace)
other='src/toern.ino.cpp'
assert env.callback(env,other)==other
result=env.callback(env,'lib/Audio/filter_ladder.cpp')[0]
assert '-O2' in result['CCFLAGS'] and '-Os' not in result['CCFLAGS']
assert '--specs=nano.specs' in result['CCFLAGS']
assert '-Os' in env.flags, 'Middleware changed optimization for the whole sketch'
assert not any('fast-math' in flag for flag in result['CCFLAGS'])

core=Path(os.environ.get('PLATFORMIO_CORE_DIR',str(Path.home()/'.platformio')))
objdump=core/'packages/toolchain-gccarmnoneeabi-teensy/bin/arm-none-eabi-objdump'
objects=list((root/'.pio/build/teensy41').glob('lib*/Audio/filter_ladder.cpp.o'))
assert len(objects)==1,'Run pio run first'
assembly=subprocess.check_output([str(objdump),'-drC',str(objects[0])],text=True)
assert not re.search(r'R_ARM_THM_CALL\s+(?:AudioFilterLadder::LPF|fast_tanh)',assembly), 'Per-sample helpers were not inlined'
print('PASS: -O2 applies only to the ladder; no fast-math; LPF/tanh inner-loop call overhead removed')
