"""Refresh the viewer's PCB snapshot from the live revJ source."""
from pathlib import Path
import json
import re

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[1] / 'hardware/PCB/toern_revJ/toern_revJ.kicad_pcb'
tokens = re.findall(r'"(?:\\.|[^"\\])*"|[()]|[^\s()]+', SOURCE.read_text())
root = []
stack = [root]
for token in tokens:
    if token == '(':
        item = []
        stack[-1].append(item)
        stack.append(item)
    elif token == ')':
        stack.pop()
    else:
        stack[-1].append(json.loads(token) if token.startswith('"') else token)
board = root[0]

def children(item, key):
    return [v for v in item if isinstance(v, list) and v and v[0] == key]

def one(item, key, default=None):
    return next(iter(children(item, key)), [key] + (default or []))[1:]

parts = []
for footprint in children(board, 'footprint'):
    props = {v[1]: v[2] for v in children(footprint, 'property')}
    pads = []
    for pad in children(footprint, 'pad'):
        pads.append(dict(number=pad[1], at=one(pad, 'at'), size=one(pad, 'size'),
                         net=one(pad, 'net'), type=pad[2], shape=pad[3], drill=one(pad, 'drill')))
    parts.append(dict(ref=props.get('Reference'), value=props.get('Value'), footprint=footprint[1],
                      at=one(footprint, 'at'), layer=one(footprint, 'layer'), properties=props,
                      pads=pads, models=[x[1] for x in children(footprint, 'model')]))
parts.sort(key=lambda p: (re.sub(r'\d', '', p['ref'] or ''),
                          int('0' + ''.join(re.findall(r'\d+', p['ref'] or '')))))
snapshot = dict(parts=parts, lines=children(board, 'gr_line'), arcs=children(board, 'gr_arc'),
                thickness=one(children(board, 'general')[0], 'thickness'), texts=children(board, 'gr_text'))
(HERE / 'source-data.json').write_text(json.dumps(snapshot, indent=2))
print(f'Refreshed PCB snapshot: {len(parts)} footprints')
