"""Copy the standalone explorer's shared files into its website embed."""
from pathlib import Path
import argparse
import re
import shutil

SOURCE = Path(__file__).resolve().parents[1]
DESTINATION = SOURCE.parents[1] / 'website' / 'board-explorer'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--check', action='store_true', help='Report drift without writing files')
args = parser.parse_args()

files = {}
for name in ('app.js', 'encoder-assemblies.js', 'style.css', 'brand.css'):
    files[Path(name)] = SOURCE / name
for folder in ('assets', 'vendor'):
    for path in (SOURCE / folder).rglob('*'):
        if path.is_file():
            files[path.relative_to(SOURCE)] = path
for name in ('embed.css', 'embed.js', 'section.css', 'section.js'):
    files[Path(name)] = SOURCE / 'embed' / name

html = (SOURCE / 'index.html').read_text()
html = html.replace('<body>', '<body class="embedded">')
html = html.replace('<aside class="catalog">', '<aside class="catalog" hidden>')
html = html.replace('<div class="scene-bottom">', '<div class="scene-bottom" hidden>')
html = html.replace('<div class="selection-strip">', '<div class="selection-strip" hidden>')
html = html.replace('  <aside class="inspector" id="inspector"',
                    '  <button id="inspector-toggle" class="sidebar-toggle" type="button" '
                    'aria-label="Open component details" aria-expanded="false" aria-controls="inspector">'
                    '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" '
                    'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">'
                    '<path d="m14 6-6 6 6 6"/></svg></button>\n'
                    '  <aside class="inspector" id="inspector" inert aria-hidden="true"')
html = re.sub(r' <header>.*?</header>\n', '', html)
html = html.replace('<link rel="stylesheet" href="brand.css">',
                    '<link rel="stylesheet" href="brand.css">\n <link rel="stylesheet" href="embed.css">')
html = html.replace('scroll to zoom', 'Ctrl/⌘ + scroll to zoom')
html = html.replace(' <script type="module" src="app.js"></script>',
                    ' <script type="module" src="app.js"></script>\n <script src="embed.js" defer></script>')
html = html.replace('use the component list for keyboard selection.',
                    'use the component selector for keyboard selection.')

differences = []
# Retire the old runtime location: the live upload omitted files below build/.
# Keep runtime files directly under vendor/three so deployment filters for
# build output directories cannot silently drop this required dependency.
for name in ('three.module.js', 'three.core.js'):
    legacy = DESTINATION / 'vendor' / 'three' / 'build' / name
    if legacy.exists():
        differences.append(str(legacy.relative_to(DESTINATION)) + ' (obsolete)')
        if not args.check:
            legacy.unlink()
for relative, source in files.items():
    destination = DESTINATION / relative
    if not destination.exists() or source.read_bytes() != destination.read_bytes():
        differences.append(str(relative))
        if not args.check:
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
index = DESTINATION / 'index.html'
if not index.exists() or index.read_text() != html:
    differences.append('index.html')
    if not args.check:
        DESTINATION.mkdir(parents=True, exist_ok=True)
        index.write_text(html)
if args.check and differences:
    print('Website embed differs: ' + ', '.join(differences))
    raise SystemExit(1)
print(f'{"Verified" if args.check else "Synchronized"} {len(files) + 1} website embed files.')
