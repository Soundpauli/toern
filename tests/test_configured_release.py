"""Run the dependency patch against clean and previously patched samplers."""
from pathlib import Path
import tempfile

root = Path(__file__).resolve().parents[1]
script = (root / 'scripts/platformio_patch_dependencies.py').read_text()

with tempfile.TemporaryDirectory(prefix='toern-release-') as directory:
    base = Path(directory)
    library = base / 'test' / 'TeensyAudioSampler' / 'src'
    library.mkdir(parents=True)

    class Env:
        def subst(self, key):
            return {'$PROJECT_DIR': str(root), '$PROJECT_LIBDEPS_DIR': str(base),
                    '$PIOENV': 'test'}[key]

        def AddBuildMiddleware(self, callback):
            pass

    for old_override in (False, True):
        sampler = library / 'sampler.h'
        fixture = 'void noteEvent(uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger) {}\n'
        for voice in ('_audioenvelop', '_audioenvelop2'):
            if old_override:
                fixture += f'audio_voice->{voice}->release(2);\n'
            fixture += f'audio_voice->{voice}->noteOff();\n'
        sampler.write_text(fixture)
        (library / 'polyphonicsampler.h').write_text('')
        for _ in range(2):
            exec(compile(script, str(root / 'scripts/platformio_patch_dependencies.py'), 'exec'),
                 {'Import': lambda name: None, 'env': Env()})
            result = sampler.read_text()
            assert 'release(2)' not in result
            assert result.count('->noteOff();') == 2

print('PASS: clean and previously patched samplers preserve configured release; patch is repeatable.')
