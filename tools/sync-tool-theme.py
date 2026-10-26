"""Keep separately deployed TŒRN tools on the same visual theme."""
from pathlib import Path

root = Path(__file__).resolve().parent.parent
source = root / 'tools/tool-theme.css'
for folder in ('standalone-tools/teensyloader-standalone', 'standalone-tools/audio-converter-standalone',
               'standalone-tools/sd-tool-standalone'):
    (root / folder / 'tool-theme.css').write_bytes(source.read_bytes())
    print(f'Synced {folder}/tool-theme.css')
