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
for name in ('embed.css', 'section.css', 'section.js'):
    files[Path(name)] = SOURCE / 'embed' / name

html = (SOURCE / 'index.html').read_text()
html = html.replace('<body>', '<body class="embedded">')
html = html.replace('<aside class="catalog">', '<aside class="catalog" hidden>')
html = re.sub(r' <header>.*?</header>\n', '', html)
html = html.replace('<link rel="stylesheet" href="brand.css">',
                    '<link rel="stylesheet" href="brand.css">\n <link rel="stylesheet" href="embed.css">')
html = html.replace('scroll to zoom', 'Ctrl/⌘ + scroll to zoom')
html = html.replace('use the component list for keyboard selection.',
                    'use the component selector for keyboard selection.')

differences = []
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
