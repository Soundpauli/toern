#!/usr/bin/env python3
"""Export rev H using KiCad's bundled Python (pcbnew and wx required).

macOS: /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3 tools/pcb/export_jlcpcb.py
Outputs are staged and validated before replacing the production files.
"""
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from collections import defaultdict
from datetime import datetime, timezone
from zipfile import ZipFile, ZIP_DEFLATED

import wx
import pcbnew as pcb

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / 'hardware/PCB/toern_revH'
NAME = 'toern_revH'
CLI = os.environ.get('KICAD_CLI', '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
# Assembly responsibilities: hardware/PCB/BOM/README.md.
HAND_SOLDER = {'J28', 'J29', 'J30', 'J31'}
# JLCPCB-friendly references, applied only to the manufacturing snapshot.
ALIASES = {'J11-5V': 'J11', 'J11-3.3V': 'J32',
           'J12 - BUTTON_A-C': 'J12', 'J13 - TOUCHYTOUCH': 'J13',
           'J14 + BUTTON_C': 'J14'}
# Use the part selections in the board, including R24 = 590R / C166635.
LCSC_OVERRIDES = {}
# Preserve the installed JLCPCB plugin's explicit rotation corrections.
ROTATION_OFFSETS = {'Conn_01x03': 180, 'Conn_01x02': 180,
                    'Microphone_Condenser': 180, 'PAM8403D': 270}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args):
    result = subprocess.run([CLI, *map(str, args)], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    print(result.stdout.strip(), flush=True)


def write_csv(path, header, rows):
    with path.open('w', newline='', encoding='utf-8') as stream:
        writer = csv.writer(stream)
        writer.writerow(header)
        writer.writerows(rows)


def archive(path, files, base):
    with ZipFile(path, 'w', ZIP_DEFLATED) as z:
        for f in sorted(files):
            z.write(f, f.relative_to(base))


def main():
    app = wx.App(False)
    source_files = [PROJECT / (NAME + ext) for ext in
                    ['.kicad_pcb', '.kicad_sch', '.kicad_pro']]
    source_hashes = {p.name: digest(p) for p in source_files}
    with tempfile.TemporaryDirectory(prefix='toern-jlcpcb-') as tmp:
        stage = Path(tmp)
        manufacturing = stage / 'source'
        manufacturing.mkdir()
        output = stage / 'release'
        gerber = output / 'gerber'
        production = output / 'production_files'
        review = output / 'review'
        for p in (gerber, production, review):
            p.mkdir(parents=True)
        for p in source_files:
            shutil.copy2(p, manufacturing / p.name)
        for name in ('fp-lib-table', 'sym-lib-table'):
            (manufacturing / name).write_text(
                (PROJECT / name).read_text().replace('${KIPRJMOD}', str(PROJECT)))
        board_path = manufacturing / (NAME + '.kicad_pcb')
        board = pcb.LoadBoard(str(board_path))
        pcb.ZONE_FILLER(board).Fill(board.Zones())
        pcb.SaveBoard(str(board_path), board)
        run('pcb', 'drc', '--schematic-parity', '--format', 'json',
            '-o', review / 'DRC.json', board_path)
        drc = json.loads((review / 'DRC.json').read_text())
        errors = [v for v in drc['violations'] if v['severity'] == 'error']
        assert not errors, errors
        assert not drc['unconnected_items'], drc['unconnected_items']
        assert not any('D2' in json.dumps(x) for x in drc['schematic_parity'])

        components, excluded, mapping = [], [], []
        groups = defaultdict(list)
        origin = board.GetDesignSettings().GetAuxOrigin()
        for fp in sorted(board.GetFootprints(), key=lambda f: f.GetReference()):
            ref, value = fp.GetReference(), fp.GetValue()
            package = str(fp.GetFPID().GetLibItemName())
            if fp.IsDNP() or fp.GetAttributes() & pcb.FP_EXCLUDE_FROM_BOM or ref in HAND_SOLDER:
                excluded.append([ref, value, package,
                                 'Hand solder' if ref in HAND_SOLDER else 'DNP / excluded from BOM'])
                continue
            fields = {f.GetName(): f.GetText().strip() for f in fp.GetFields()}
            lcsc = LCSC_OVERRIDES.get(ref, fields.get('lcsc#', fields.get('LCSC', '')))
            assert re.fullmatch(r'C\d+', lcsc), (ref, lcsc)
            alias = ALIASES.get(ref, ref)
            assert re.fullmatch(r'[A-Za-z]+\d+', alias), alias
            if alias != ref:
                mapping.append([alias, ref])
                fp.SetReference(alias)
            pads = list(fp.Pads())
            bbox = pads[0].GetBoundingBox()
            for pad in pads[1:]:
                bbox.Merge(pad.GetBoundingBox())
            center = bbox.GetCenter()
            angle = fp.GetOrientationDegrees()
            side = 'top' if fp.GetLayer() == pcb.F_Cu else 'bottom'
            if side == 'bottom':
                angle = 180 - angle
            correction = ROTATION_OFFSETS.get(value, 0)
            angle = (angle + correction) % 360
            row = [alias, value, package, round(pcb.ToMM(center.x-origin.x), 6),
                   round(-pcb.ToMM(center.y-origin.y), 6), round(angle, 6), side]
            components.append(row)
            groups[(value, package, lcsc)].append(alias)
        refs = [r[0] for r in components]
        assert len(refs) == len(set(refs))
        assert {'D2', 'CN1', 'CN2', 'J11', 'J12', 'J13', 'J14', 'J15', 'J32'} <= set(refs)
        assert not HAND_SOLDER.intersection(refs)
        d2 = next(f for f in board.GetFootprints() if f.GetReference() == 'D2')
        assert {p.GetNumber(): p.GetNetname() for p in d2.Pads()} == {
            '1': 'GND', '2': '+3.3V', '3': 'Net-(D2-common)'}
        pcb.SaveBoard(str(board_path), board)
        write_csv(production / ('BOM-' + NAME + '.csv'),
                  ['Comment', 'Designator', 'Footprint', 'LCSC', 'Quantity'],
                  [[v, ','.join(rs), f, lcsc, len(rs)]
                   for (v, f, lcsc), rs in sorted(groups.items())])
        write_csv(production / ('CPL-' + NAME + '.csv'),
                  ['Designator', 'Val', 'Package', 'Mid X', 'Mid Y', 'Rotation', 'Layer'], components)
        write_csv(review / 'reference-map.csv', ['Manufacturing reference', 'Source board reference'], mapping)
        write_csv(review / 'not-assembled.csv', ['Reference', 'Value', 'Footprint', 'Reason'], excluded)
        run('pcb', 'export', 'gerbers', '--layers',
            'F.Cu,In1.Cu,In2.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,F.Paste,B.Paste,Edge.Cuts',
            '--use-drill-file-origin', '--no-protel-ext', '--subtract-soldermask', '-o', gerber, board_path)
        run('pcb', 'export', 'drill', '--drill-origin', 'plot', '--excellon-separate-th',
            '--generate-map', '--map-format', 'svg', '-o', gerber, board_path)
        # Suppress long metadata fields and duplicate user labels in the drawing.
        # Keep the manufacturing snapshot itself unchanged.
        assembly_board = pcb.LoadBoard(str(board_path))
        for fp in assembly_board.GetFootprints():
            for field in fp.GetFields():
                if field.GetName() != 'Reference':
                    field.SetText('')
            fp.Reference().SetVisible(True)
            fp.Reference().SetLayer(pcb.F_Fab)
            fp.Reference().SetPosition(fp.GetPosition())
            fp.Reference().SetTextSize(pcb.VECTOR2I(pcb.FromMM(0.65), pcb.FromMM(0.65)))
            for item in fp.GraphicalItems():
                if isinstance(item, pcb.PCB_TEXT):
                    item.SetText('')
        assembly_path = manufacturing / 'assembly.kicad_pcb'
        pcb.SaveBoard(str(assembly_path), assembly_board)
        run('pcb', 'export', 'svg', '--layers', 'F.Fab,Edge.Cuts', '--mode-single',
            '--page-size-mode', '2', '--exclude-drawing-sheet', '--black-and-white',
            '--sketch-pads-on-fab-layers', '-o', review / 'assembly-top.svg', assembly_path)
        run('sch', 'export', 'netlist', '-o', review / 'schematic.net',
            PROJECT / (NAME + '.kicad_sch'))
        # Ensure the actual plotted copper has the corrected package pinning.
        top = next(p for p in gerber.glob('*.gbr') if 'Copper,L1,Top' in p.read_text())
        copper = top.read_text()
        for pin, net in [('1', 'GND'), ('2', '+3.3V'), ('3', 'Net-(D2-common)')]:
            assert re.search(r'%TO.P,D2,' + pin + r'(?:,[^*]*)?\*%\s*%TO.N,' + re.escape(net) + r'\*%', copper)
        assert len(list(gerber.glob('*.gbr'))) == 11
        assert len(list(gerber.glob('*.drl'))) == 2
        archive(production / ('GERBER-' + NAME + '.zip'),
                [p for p in gerber.iterdir() if p.suffix in ('.gbr', '.drl', '.gbrjob')], gerber)
        bom_refs = set()
        for row in csv.DictReader((production / ('BOM-' + NAME + '.csv')).open()):
            designators = row['Designator'].split(',')
            assert int(row['Quantity']) == len(designators)
            assert not bom_refs.intersection(designators)
            bom_refs.update(designators)
        assert bom_refs == set(refs)
        # Detect edits while exporting instead of mixing board revisions.
        assert source_hashes == {p.name: digest(p) for p in source_files}, 'Source changed during export; rerun'
        manifest = {
            'generated_utc': datetime.now(timezone.utc).isoformat(),
            'source_sha256': source_hashes,
            'manufacturing_board_sha256': digest(board_path),
            'copper_layers': board.GetCopperLayerCount(),
            'thickness_mm': pcb.ToMM(board.GetDesignSettings().GetBoardThickness()),
            'assembly_components': len(components), 'bom_rows': len(groups),
            'drc_errors': len(errors), 'drc_warnings': len(drc['violations']),
            'unconnected_items': 0, 'schematic_parity_issues': len(drc['schematic_parity']),
            'd2_verified_in_board_and_gerber': True,
            'reference_aliases': ALIASES, 'lcsc_overrides': LCSC_OVERRIDES,
            'rotation_corrections_by_value': ROTATION_OFFSETS,
        }
        (review / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        shutil.copy2(board_path, review / 'manufacturing.kicad_pcb')
        instructions = ROOT / 'tools/pcb/JLCPCB-INSTRUCTIONS.txt'
        shutil.copy2(instructions, output / 'JLCPCB-INSTRUCTIONS.txt')
        for name in ['preorder-parts.csv', 'hand-solder-parts.csv', 'additional-purchased-parts.csv']:
            shutil.copy2(ROOT / 'hardware/PCB/BOM' / name, review / name)
        checksum_files = [p for p in output.rglob('*') if p.is_file()]
        (output / 'SHA256SUMS.txt').write_text(''.join(
            digest(p) + '  ' + str(p.relative_to(output)) + '\n' for p in sorted(checksum_files)))
        target = PROJECT / 'jlcpcb'
        # Keep a complete prior output outside the release, including any local edits.
        backup = Path(tempfile.mkdtemp(prefix='toern-jlcpcb-previous-'))
        shutil.copytree(target, backup / 'jlcpcb')
        print('Previous exports backed up to', backup, flush=True)
        for folder in ['gerber', 'production_files', 'review']:
            if (target / folder).exists():
                shutil.rmtree(target / folder)
            shutil.copytree(output / folder, target / folder)
        for name in ['SHA256SUMS.txt', 'JLCPCB-INSTRUCTIONS.txt']:
            shutil.copy2(output / name, target / name)
        archive(PROJECT / 'jlcpcb.zip', [p for p in output.rglob('*') if p.is_file()], output)
        print(json.dumps(manifest, indent=2))
        print('Release:', PROJECT / 'jlcpcb.zip')


if __name__ == '__main__':
    main()
