"""Export current audited Fusion geometry, preserving editable engraving text."""
from pathlib import Path
import json,zipfile
import ezdxf
from ezdxf.enums import TextEntityAlignment
from ezdxf.addons import Importer
from ezdxf.path import make_path
from shapely.geometry import LineString
from shapely.ops import unary_union,polygonize_full
ROOT=Path(__file__).parent;OUT=ROOT.parent.parent
r=json.loads((ROOT/'mount_update_result.json').read_text())
assert r['pcb_mounts_preserved']
def export(data,name,offset=(0,0)):
 d=ezdxf.new('R2010');d.units=4;d.header['$MEASUREMENT']=1
 for layer,color in [('SCHNITT',7),('GRAVUR',3)]:d.layers.new(layer,dxfattribs={'color':color})
 d.styles.new('LABELS',dxfattribs={'font':'SpaceGrotesk-Bold.ttf'});m=d.modelspace()
 for c in data['curves']:
  if c['construction']:continue
  attrs={'layer':'GRAVUR' if c['fixed'] else 'SCHNITT'}
  if 'circle_mm' in c:
   x,y,rad=c['circle_mm'];m.add_circle((x,y),rad,dxfattribs=attrs)
  elif 'arc_mm' in c:
   x,y,rad,a,b=c['arc_mm'];m.add_arc((x,y),rad,a,b,dxfattribs=attrs)
  elif c['type'].endswith('SketchLine'):m.add_line(c['points_mm'][0],c['points_mm'][-1],dxfattribs=attrs)
  else:raise ValueError(c['type'])
 for t in data['texts']:
  b=t['bounds'];m.add_text(t['text'],dxfattribs={'height':t['height_mm'],'style':'LABELS','layer':'GRAVUR'}).set_placement(((b[0]+b[2])/2,(b[1]+b[3])/2),align=TextEntityAlignment.MIDDLE_CENTER)
 for e in m:e.translate(*offset,0)
 d.set_modelspace_vport(height=140,center=(91.25,64.25));d.saveas(OUT/name)
 assert not d.audit().has_errors
 return d
base=export(r,'modular.dxf');front=export(r['front'],'modular_front.dxf',(-5.25,-36))
# Refresh references around the exact same native cut and engraving geometry.
ref=ezdxf.readfile(OUT/'modular_clearances.dxf')
for e in list(ref.modelspace()):
 if e.dxf.layer in ('SCHNITT','GRAVUR'):ref.modelspace().delete_entity(e)
imp=Importer(base,ref);imp.import_modelspace();imp.finalize();ref.saveas(OUT/'modular_clearances.dxf')
combined=ezdxf.new('R2010');combined.units=4;combined.header['$MEASUREMENT']=1
for source,offset,name in [(base,0,'BASE_PANEL'),(front,186.5,'FRONT_CLAMP')]:
 previous={e.dxf.handle for e in combined.modelspace()}
 imp=Importer(source,combined);imp.import_modelspace();imp.finalize()
 entities=[e for e in combined.modelspace() if e.dxf.handle not in previous]
 for e in entities:e.translate(offset,0,0)
 combined.groups.new(name=name).set_data(entities)
combined.set_modelspace_vport(height=155,center=(179.25,64.25));combined.saveas(OUT/'modular_2parts.dxf')
def validate(doc,count):
 assert not doc.audit().has_errors and doc.units==4
 lines=[LineString([(round(p.x,6),round(p.y,6)) for p in make_path(e).flattening(distance=.001)]) for e in doc.modelspace().query('*[layer=="SCHNITT"]')]
 polys,cuts,dangles,invalid=polygonize_full(unary_union(lines))
 assert len(polys.geoms)==count and cuts.is_empty and dangles.is_empty and invalid.is_empty,(count,len(polys.geoms))
 return sorted(polys.geoms,key=lambda p:p.area,reverse=True)
validate(base,27);validate(front,5);panels=validate(combined,32)[:2]
assert abs(panels[0].distance(panels[1])-4)<1e-5
assert sorted(len(p.interiors) for p in panels)==[4,26]
circles=list(base.modelspace().query('CIRCLE[layer=="SCHNITT"]'))
assert len(circles)==17
for x in [83,101,119]:assert any(abs(c.dxf.center.x-x)<.001 and abs(c.dxf.radius-5.75)<.001 for c in circles)
assert sum(abs(c.dxf.radius-1.1)<.001 for c in circles)==5
assert all(abs(c.dxf.radius-1.4)<.001 for c in front.modelspace().query('CIRCLE'))
assert len(front.modelspace().query('CIRCLE'))==4
# Slot endpoints: 5 mm overall length, 2.8 mm width; exact native arc radii.
for x in [8.75,173.75]:
 for y in [39.5,118.5]:
  arcs=[e for e in base.modelspace().query('ARC[layer=="SCHNITT"]') if abs(e.dxf.center.y-y)<.001 and abs(abs(e.dxf.center.x-x)-1.1)<.001 and abs(e.dxf.radius-1.4)<.001]
  assert len(arcs)==2
v=json.loads((ROOT/'validation.json').read_text());v.pop('current_cut_geometry_preserved',None)
v.update(r['mounts']);v.update({'source_document':r['document'],'pcb_mounts_preserved':True,'existing_engraving_preserved':True,'base_closed_cut_regions':27,'cover_closed_cut_regions':5,'combined_closed_cut_regions':32,'combined_panel_gap_mm':4,'base_slot_horizontal':True,'front_hole_edge_material_mm':2.1,'base_slot_min_panel_edge_material_mm':6.25})
(ROOT/'validation.json').write_text(json.dumps(v,indent=2)+'\n')
v=json.loads((ROOT/'result.json').read_text());v.update(r['mounts']);v['clamp_holes_mm']=[[x,y,2.8,'M2.5 CLAMP'] for x in [8.75,173.75] for y in [39.5,118.5]];v['base_clamp_slots_mm']=[[x,y,5,2.8] for x in [8.75,173.75] for y in [39.5,118.5]];v.update({'circles':17,'slots':9,'pcb_mounts_preserved':True});(ROOT/'result.json').write_text(json.dumps(v,indent=2)+'\n')
p=ROOT/'LASER_README.txt';t=p.read_text().replace('Existing cut geometry and engraving preserved.','Engraving preserved; screw and audio clearances updated.').replace('The existing cutouts, screw holes and slot are preserved.','Three audio socket cutouts enlarged from diameter 11 to 11.5 mm. Other ports unchanged.')
t=t.replace('PCB MOUNTING - FLIPPED ORIENTATION','''FRONT CLAMP MOUNTING - M2.5
Front sheet: four diameter 2.8 mm clearance holes for M2.5 screws.
Base sheet: matching horizontal 5 x 2.8 mm rounded slots, allowing lateral compensation.
Hole/slot centers: x=8.75 or 173.75; y=39.5 or 118.5 mm in base coordinates.
Front holes leave 2.1 mm material to each nearest cover edge.
Reference screw-head envelopes: diameter 6 mm at cover; 8.2 x 6 mm swept area at base.
PCB mounts remain M2; do not use the larger clamp screws in the PCB holes.

PCB MOUNTING - FLIPPED ORIENTATION''')
t=t.replace('Millimetres, 1:1.','modular_2parts.dxf: both sheets in one file, separate groups, 4 mm gap.\nMillimetres, 1:1.')
p.write_text(t)
with zipfile.ZipFile(OUT/'modular_laser_files.zip','w',zipfile.ZIP_DEFLATED) as z:
 for name in ['modular.dxf','modular_front.dxf','modular_2parts.dxf']:z.write(OUT/name,name)
 for name in ['SpaceGrotesk-Bold.ttf','LASER_README.txt','FONT-LICENSE.txt']:z.write(ROOT/name,name)
 for name in ['PCB_MOUNTING_HOLES.csv','validation.json','MIDI_TYPE_A_VERIFICATION.txt']:z.write(ROOT/name,'reference/'+name)
 z.write(OUT/'modular_clearances.dxf','reference/modular_clearances.dxf')
assert zipfile.ZipFile(OUT/'modular_laser_files.zip').testzip() is None
assert zipfile.ZipFile(OUT/'toern_final_v3_modular.f3d').namelist()
print('Validated: 27 base + 5 front closed cut regions, 4 mm separation, M2.5 clearances/slots, three diameter 11.5 mm audio holes; five M2 PCB holes and engraving preserved.')
