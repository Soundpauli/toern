"""Copy shared CSS into independently deployable tools."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
shared = root / 'tools/_shared'
for folder in ['tools/firmware','tools/audioconvert','tools/sdtool','tools/case','tools/colorsheme','_private/helpers/annotate']:
    for name in ['tool-theme.css','tool-shell.css']:
        (root / folder / name).write_bytes((shared / name).read_bytes())
    print(f'Synced {folder}')
