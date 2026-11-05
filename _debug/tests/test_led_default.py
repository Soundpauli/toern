"""Check the actual early LED setting loader on fresh and configured units."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'platformio/toern.ino').read_text()
start = source.index('FLASHMEM void loadLedModeEarlyFromEEPROM()')
loader = source[start:source.index('\nFLASHMEM void setup()', start)]
globals_ = '\n'.join(
    re.search(r'^' + prefix + r'.*$', source, re.M)[0]
    for prefix in [
        'static constexpr uint8_t DEFAULT_LED_MODE',
        'int ledModules =',
        'bool ledModulesRotated =',
        'unsigned int maxX =',
    ]
)
harness = r'''
#include <cassert>
#include <cstdint>
#define FLASHMEM
#define MATRIX_WIDTH 16
constexpr int EEPROM_DATA_START = 100;
struct {
  uint8_t value = 255;
  int writes = 0;
  uint8_t read(int address) { assert(address == EEPROM_DATA_START + 13); return value; }
  void write(int address, uint8_t v) { assert(address == EEPROM_DATA_START + 13); value = v; ++writes; }
} EEPROM;
int dirty = 0;
void markSettingsBackupDirty() { ++dirty; }
GLOBALS
LOADER
int main() {
  assert(ledModules == 2 && maxX == 32 && !ledModulesRotated);
  for (int saved = 0; saved < 256; ++saved) {
    EEPROM.value = saved; EEPROM.writes = 0; dirty = 0;
    loadLedModeEarlyFromEEPROM();
    const bool valid = saved >= 1 && saved <= 4;
    const int expected = valid ? saved : 2;
    assert(EEPROM.value == expected);
    assert(EEPROM.writes == !valid && dirty == !valid);
    assert(ledModules == ((expected == 1 || expected == 3) ? 1 : 2));
    assert(maxX == 16u * ledModules);
    assert(ledModulesRotated == (expected >= 3));
  }
}
'''.replace('GLOBALS', globals_).replace('LOADER', loader)
with tempfile.TemporaryDirectory(prefix='toern-led-default-') as tmp:
    cpp = Path(tmp) / 'led.cpp'
    exe = Path(tmp) / 'led'
    cpp.write_text(harness)
    cmd = ['c++', '-std=c++17']
    sdk = Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():
        cmd += ['-isystem', str(sdk)]
    subprocess.run(cmd + [str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: 32x16 startup default, all invalid EEPROM bytes, and preserved saved 16x16/32x16 orientations.')
