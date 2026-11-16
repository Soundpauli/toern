"""Still-playing playRaw must arm the 64-sample join ramp."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2] / 'platformio'
reader = (root / 'src' / 'resamplerReader.h').read_text()
play = (root / '.pio/libdeps/teensy41/TeensyVariablePlayback/src/playresmp.h').read_text()

assert 'Keep _playing true into the reader so restart declick can arm' in play
assert 'stop();\n            return reader->playRaw(data, numSamples, numChannels);' not in play
assert '_declickRestart = restarting;' in reader
assert 'consumeDeclickRestart()' in reader

# MIDI and grid share triggerSamplerVoice and restart ADSR on every hit.
main = (root / 'toern.ino').read_text()
assert 'env->retrigger();' in main
assert 'if (!(active && env->isActive())) env->noteOn();' not in main
assert 'Keep _playing true into the reader so restart declick can arm' in play

code = r'''
#include <cassert>
#include <cstdint>
#include <cstring>

struct FakeReader {
  bool playing = false;
  bool declick = false;
  int16_t *data = nullptr;
  uint32_t frames = 0;
  uint16_t channels = 0;
  bool playRaw(int16_t *d, uint32_t n, uint16_t c) {
    const bool restarting = playing;
    playing = false;
    data = d; frames = n; channels = c;
    declick = restarting;
    playing = true;
    return true;
  }
  void stop() { playing = false; }
  bool isPlaying() const { return playing; }
  bool consumeDeclickRestart() { bool v = declick; declick = false; return v; }
};

// Mirrors the fixed AudioPlayResmp::playRaw contract.
struct Voice {
  FakeReader *reader;
  bool playRaw(int16_t *d, uint32_t n, uint16_t c) {
    return reader->playRaw(d, n, c);
  }
};

int main() {
  int16_t pcm[8] = {1000, -1000, 2000, -2000, 3000, -3000, 4000, -4000};
  FakeReader reader;
  Voice voice{&reader};
  assert(voice.playRaw(pcm, 8, 1));
  assert(reader.isPlaying());
  assert(!reader.consumeDeclickRestart());  // cold start
  assert(voice.playRaw(pcm, 8, 1));
  assert(reader.consumeDeclickRestart());   // still-playing retrigger
  assert(!reader.consumeDeclickRestart());  // one-shot flag
}
'''

with tempfile.TemporaryDirectory(prefix='toern-declick-') as tmp:
    path = Path(tmp)
    (path / 'test.cpp').write_text(code)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: still-playing playRaw arms declick; every sample hit restarts ADSR from the current gain')
