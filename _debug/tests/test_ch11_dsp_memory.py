"""Check the linked memory budget for CH11's per-block DSP working set.

Run `pio run` first. This checks actual ELF placement, rather than trusting an
EXTMEM spelling: the polyphonic hot state must not migrate back to PSRAM.
"""
from pathlib import Path
import os
import subprocess

root = (Path(__file__).resolve().parents[2] / 'platformio')
tool = Path(os.environ.get('PLATFORMIO_CORE_DIR', str(Path.home()/'.platformio'))) / 'packages/toolchain-gccarmnoneeabi-teensy/bin/arm-none-eabi-nm'
elf = root / '.pio/build/teensy41/firmware.elf'
assert elf.exists(), 'Run pio run before the CH11 memory-layout check'
symbols = {}
for line in subprocess.check_output([str(tool), '-S', str(elf)], text=True).splitlines():
    parts = line.split()
    if len(parts)==4:
        try:
            symbols[parts[3]]=(int(parts[0],16),int(parts[1],16))
        except ValueError:
            pass
hot = [f'Swaveform{bank}_{voice}' for bank in (1,2,3) for voice in range(3)]
hot += [f'Sladder{side}_{voice}' for side in (1,2) for voice in range(3)]
hot += [f'Senvelope{side}_{voice}' for side in (1,2) for voice in range(3)]
hot += [f'Smixer{side}_{voice}' for side in (1,2) for voice in range(3)]
hot += ['SmixerL4','SmixerR4']
for name in hot:
    address,size=symbols[name]
    assert 0x20000000 <= address < address+size <= 0x20080000,(name,hex(address),size)
size=sum(symbols[name][1] for name in hot)
assert size <= 24*1024, f'CH11 DSP state outgrew its DTCM budget: {size} bytes'
for name in ('sampled','voiceDelayStore'):
    address,size=symbols[name]
    assert 0x70000000 <= address < address+size <= 0x71000000,(name,hex(address),size)
print(f'PASS: {len(hot)} CH11 DSP nodes in DTCM; {sum(symbols[n][1] for n in hot)} bytes; large audio buffers remain in PSRAM')
