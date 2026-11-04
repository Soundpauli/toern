"""Check that small audio graph/control objects fit in DTCM, not sample PSRAM."""
from pathlib import Path
import os
import re
import subprocess

root=(Path(__file__).resolve().parents[2] / 'platformio')
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
# Teensy .dmabuffers is NOLOAD: constexpr File objects there do not receive
# their initial pointers/vtables/state. Only raw I/O bytes belong in that section.
demangled = subprocess.check_output([str(nm),'-S','-C',str(elf)],text=True)
kit_symbols = {}
for line in demangled.splitlines():
    fields=line.split()
    if len(fields)==4 and fields[3] in ('kitSaveJob','kitSaveSector'):
        kit_symbols[fields[3]]=(int(fields[0],16),int(fields[1],16))
address,size=kit_symbols['kitSaveJob']
assert 0x20000000<=address<address+size<=0x20080000,('KIT state/File handles must be initialized',hex(address))
address,size=kit_symbols['kitSaveSector']
assert 0x20200000<=address<address+size<=0x20280000 and size==2048 and address%32==0,('KIT DMA sector',hex(address),size)
print('PASS: KIT writer/File state in initialized DTCM; only its aligned 2 KB sector lives in DMA RAM')

# AGEN is rarely enabled: its note/context workspace must not reserve static RAM.
agen_symbols={}
for line in demangled.splitlines():
    fields=line.split(maxsplit=3)
    if len(fields)==4:
        try:agen_symbols[fields[3]]=(int(fields[0],16),int(fields[1],16))
        except ValueError:pass
assert 'agenWork' in agen_symbols,'Build current AGEN firmware first'
assert agen_symbols['agenWork'][1]==4,'Only a workspace pointer should stay resident'
for name in ['agenNotes','agenContext','agenFoundation','agenHarmony','agenGroove','agenRegister']:
    assert name not in agen_symbols,('AGEN buffers must be allocated on entry',name)
knowledge=[(name,location) for name,location in agen_symbols.items()
    if name.startswith('toern_autogen_music::') and '::' in name and name.endswith(('::form','::lengths','::major','::naturalMinor','::harmonicSlots'))]
assert len(knowledge)>=6,'AGEN theory/arrangement tables missing from compiled image'
for name,(address,size) in knowledge:
    assert 0x60000000<=address<address+size<=0x70000000,('AGEN theory belongs in flash',name,hex(address))
shared=[(name,location) for name,location in agen_symbols.items()
    if name.endswith(('::majorScale','::minorScale','::majorIntervals','::minorIntervals','::degreeWeights','::majorProg','::minorProg','::motifContours'))]
assert len(shared)>=8,'Shared generation knowledge tables missing from compiled image'
for name,(address,size) in shared:
    assert 0x60000000<=address<address+size<=0x70000000,('Shared musical knowledge belongs in flash',name,hex(address))
print('PASS: AGEN keeps only its workspace pointer resident; AGEN/shared scale, chord and motif tables live in flash')
