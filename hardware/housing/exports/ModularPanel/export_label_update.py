"""Export the audited current Fusion sketch without rebuilding its geometry."""
from pathlib import Path
import json,zipfile
import ezdxf
from ezdxf.enums import TextEntityAlignment
from ezdxf.path import make_path
from shapely.geometry import LineString,Polygon,Point
from shapely.ops import unary_union,polygonize_full
ROOT=Path(__file__).parent;OUT=ROOT.parent.parent
r=json.loads((ROOT/'label_update_result.json').read_text())
assert r['cut_geometry_preserved'] and r['original_curves_preserved']
d=ezdxf.new('R2010');d.units=4;d.header['$MEASUREMENT']=1
for name,color in [('SCHNITT',7),('GRAVUR',3)]:d.layers.new(name,dxfattribs={'color':color})
d.styles.new('LABELS',dxfattribs={'font':'SpaceGrotesk-Bold.ttf'})
m=d.modelspace()
for c in r['curves']:
 if c['construction']:continue
 attrs={'layer':'GRAVUR' if c['fixed'] else 'SCHNITT'}
 if 'circle_mm' in c:
  x,y,rad=c['circle_mm'];m.add_circle((x,y),rad,dxfattribs=attrs)
 elif 'arc_mm' in c:
  x,y,rad,a,b=c['arc_mm'];m.add_arc((x,y),rad,a,b,dxfattribs=attrs)
 elif c['type'].endswith('SketchLine'):
  m.add_line(c['points_mm'][0],c['points_mm'][-1],dxfattribs=attrs)
 else:m.add_lwpolyline(c['points_mm'],dxfattribs=attrs)
for t in r['texts']:
 b=t['bounds'];x=(b[0]+b[2])/2;y=(b[1]+b[3])/2
 m.add_text(t['text'],dxfattribs={'height':t['height_mm'],'style':'LABELS','layer':'GRAVUR'}).set_placement((x,y),align=TextEntityAlignment.MIDDLE_CENTER)
d.set_modelspace_vport(height=140,center=(91.25,64.25));d.saveas(OUT/'modular.dxf')
assert not d.audit().has_errors
counts={}
for name,count in [('modular.dxf',27),('modular_front.dxf',5)]:
 doc=ezdxf.readfile(OUT/name);assert not doc.audit().has_errors
 lines=[LineString([(round(p.x,6),round(p.y,6)) for p in make_path(e).flattening(distance=.001)]) for e in doc.modelspace().query('*[layer=="SCHNITT"]')]
 polys,cuts,dangles,invalid=polygonize_full(unary_union(lines))
 assert len(polys.geoms)==count and cuts.is_empty and dangles.is_empty and invalid.is_empty,(name,len(polys.geoms))
 counts[name]=count
assert sorted(t['text'] for t in r['texts'])==sorted(['MIDI A','CLCK','SC','PHONES','LINE-OUT','LINE-IN'])
assert [x['kind'] for x in r['direction_icons']]==['IN','OUT']
cut_shapes=[Point(*c['circle_mm'][:2]).buffer(c['circle_mm'][2]) for c in r['curves'] if 'circle_mm' in c and not c['fixed']]
for icon in r['direction_icons']:
 shape=unary_union([Polygon(poly) for poly in icon['polylines']])
 assert shape.is_valid and all(shape.distance(c)>1.5 for c in cut_shapes)
assert zipfile.ZipFile(OUT/'toern_final_v3_modular.f3d').namelist()
v=json.loads((ROOT/'validation.json').read_text());v.update({'source_document':r['document'],'current_cut_geometry_preserved':True,'existing_engraving_preserved':True,'label_contents':[t['text'] for t in r['texts']],'IN_OUT_text_removed':True,'direction_icons':['right arrow toward bar (IN)','left arrow away from bar (OUT)'],'direction_icon_stroke_mm':.45,'direction_icon_clearance_verified':True,'midi_type':'A','base_closed_cut_regions':27,'cover_closed_cut_regions':5})
(ROOT/'validation.json').write_text(json.dumps(v,indent=2)+'\n')
p=ROOT/'LASER_README.txt';t=p.read_text();start=t.index('PORT LAYOUT');end=t.index('PCB MOUNTING',start)
t=t[:start]+'''PORT LAYOUT AND MARKINGS
Current Fusion source: '''+r['document']+'''. Existing cut geometry and engraving preserved.
DIN right, logo left; no DIN text.
MIDI A = TRS MIDI Type A (tip DIN5 / current sink; ring DIN4 / current source).
CLCK and SC are the abbreviated clock/sidechain headings.
Functional labels remain editable Space Grotesk Bold TEXT.
IN/OUT words replaced by horizontal arrow/bar icons, 0.45 mm rounded strokes:
upper row points right toward the bar (IN); lower row points left away from it (OUT).
Icons are fixed engraving outlines on GRAVUR, excluded from SCHNITT.
The existing cutouts, screw holes and slot are preserved.
The earlier 70 x 13 mm slot request remains pending placement; it is not applied here.

'''+t[end:];p.write_text(t)
(ROOT/'MIDI_TYPE_A_VERIFICATION.txt').write_text('''TRS MIDI Type A verification - rev H
SJ1-3533 terminal 1=sleeve, 2=tip, 3=ring.
J17 OUT: pad2/tip through R7 (33R) to U5 data buffer; pad3/ring through R8 (33R) to +3.3 V.
J3 IN: pad2/tip to optocoupler U4 cathode; pad3/ring through R6 (220R) to optocoupler anode.
This matches Type A: tip = DIN5/current sink, ring = DIN4/current source.
PCB source: hardware/PCB/toern_revH/toern_revH.kicad_pcb
Jack manufacturer: https://www.sameskydevices.com/product/resource/masterpdf/sj1-353x.pdf
MIDI Association: https://midi.org/updated-how-to-make-your-own-3-5mm-mini-stereo-trs-to-midi-5-pin-din-cables
''')
with zipfile.ZipFile(OUT/'modular_laser_files.zip','w',zipfile.ZIP_DEFLATED) as z:
 for name in ['modular.dxf','modular_front.dxf']:z.write(OUT/name,name)
 for name in ['SpaceGrotesk-Bold.ttf','LASER_README.txt','FONT-LICENSE.txt']:z.write(ROOT/name,name)
 for name in ['PCB_MOUNTING_HOLES.csv','validation.json','MIDI_TYPE_A_VERIFICATION.txt']:z.write(ROOT/name,'reference/'+name)
 z.write(OUT/'modular_clearances.dxf','reference/modular_clearances.dxf')
assert zipfile.ZipFile(OUT/'modular_laser_files.zip').testzip() is None
print('Validated current v118: 27 base/5 cover closed cut regions; 6 editable labels; 2 engraved direction icons; original geometry preserved. Laser package refreshed.')
