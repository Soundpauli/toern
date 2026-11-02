"""Check that small audio graph/control objects fit in DTCM, not sample PSRAM."""
from pathlib import Path
import os
import re
import subprocess

root=Path(__file__).resolve().parents[1]
core=Path(os.environ.get('PLATFORMIO_CORE_DIR',str(Path.home()/'.platformio')))
nm=core/'packages/toolchain-gccarmnoneeabi-teensy/bin/arm-none-eabi-nm'
elf=root/'.pio/build/teensy41/firmware.elf'
assert elf.exists(),'Run pio run first'
symbols={}
for line in subprocess.check_output([str(nm),'-S',str(elf)],text=True).splitlines():
    fields=line.split()
    if len(fields)==4:
        try:symbols[fields[3]]=(int(fields[0],16),int(fields[1],16))
        except ValueError:pass
source=(root/'audioinit.h').read_text()
mixers=re.findall(r'^(?:EXTMEM\s+)?AudioMixer4(?:Ramp|Limited)?\s+(\w+)',source,re.M)
routes=re.findall(r'^(?:EXTMEM\s+)?AudioConnection\s+(\w+)',source,re.M)
assert len(routes)>=269,'Audio graph inventory unexpectedly shrank'
hot=mixers+routes+['_samplers','ampPreview']
for name in hot:
    address,size=symbols[name]
    assert 0x20000000<=address<address+size<=0x20080000,(name,hex(address),size)
size=sum(symbols[name][1] for name in hot)
assert size<=16*1024, f'Small audio graph/control state outgrew its 16 KiB DTCM budget: {size}'
# The settings structure was not requested for relocation, nor were PCM/delays.
for name in ['SMP','sampled','voiceDelayStore']:
    address,size=symbols[name]
    assert 0x70000000<=address<address+size<=0x71000000,(name,hex(address),size)
print(f'PASS: {len(mixers)} mixer objects/arrays, {len(routes)} routes, sampler controllers and preview amp in DTCM; {sum(symbols[n][1] for n in hot)} bytes; settings and large buffers remain external')
