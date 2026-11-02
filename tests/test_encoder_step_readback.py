"""Exercise the dependency patch and real DuPPa step accessor with 32-bit steps."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
patch = root / 'scripts/platformio_patch_dependencies.py'
source = (root / '.pio/libdeps/teensy41/DuPPa Library/src/i2cEncoderLibV2.cpp').read_text()
start = source.index('int32_t i2cEncoderLibV2::readStep(void) {')
end = source.index('}', start) + 1
# Reproduce the pinned upstream bug regardless of whether the local cache has
# already been patched. Execute the actual build script twice for idempotence.
accessor = source[start:end].replace('readEncoderLong(REG_ISTEPB4)', 'readEncoderInt(REG_ISTEPB4)')
with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    library = work / 'test/DuPPa Library/src/i2cEncoderLibV2.cpp'
    library.parent.mkdir(parents=True)
    library.write_text(accessor)
    class Env:
        def subst(self, key):
            return {'$PROJECT_DIR': str(root), '$PROJECT_LIBDEPS_DIR': str(work), '$PIOENV': 'test'}[key]
        def AddBuildMiddleware(self, callback):
            pass
    namespace = {'Import': lambda name: None, 'env': Env()}
    exec(compile(patch.read_text(), str(patch), 'exec'), namespace)
    first = library.read_text()
    exec(compile(patch.read_text(), str(patch), 'exec'), namespace)
    assert library.read_text() == first
    cpp = work / 'step.cpp'
    cpp.write_text('''
#include <cstdint>
#include <cassert>
class i2cEncoderLibV2 {
public:
 int32_t value=0;
 int32_t readStep(void);
 enum {REG_ISTEPB4=0x14};
 int16_t readEncoderInt(uint8_t){return (int16_t)(value>>16);}
 int32_t readEncoderLong(uint8_t){return value;}
};
''' + first + '''
int main(){
 i2cEncoderLibV2 encoder;
 for(int32_t step : {1,16,65536,-1}){encoder.value=step;assert(encoder.readStep()==step);}
}
'''.replace('for(int32_t step : {1,16,65536,-1})', 'int32_t values[]={1,16,65536,-1};for(int32_t step : values)'))
    flags = ['c++', '-std=c++17']
    sdk = Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():
        flags += ['-isystem', str(sdk)]
    subprocess.run(flags+[str(cpp), '-o', str(work/'step')], check=True)
    subprocess.run([str(work/'step')], check=True)
print('PASS: DuPPa step readback uses all 32 bits; dependency patch is idempotent')
