from pathlib import Path
import json, math, collections
import ezdxf
from ezdxf.enums import TextEntityAlignment
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle, Arc
from matplotlib import font_manager
font_manager.fontManager.addfont('/Users/jank/Library/Fonts/SpaceGrotesk-Bold.ttf')
plt.rcParams['font.family']='Space Grotesk'
R=Path(__file__).parent;OUT=R.parent.parent
S={'name':'modular','width':182.5,'height':128.5,'led':[11.25,39,160,80],'pcb':[11.25,43.52,160,70.96],'cut_lines':[],'cut_circles':[],'slots':[],'engrave_lines':[],'labels':[],'clearance_rects':[],'clearance_circles':[],'reference_rects':[],'rack_head_slots':[]}
def rect(x,y,w,h,lines):
 pts=[(x,y),(x+w,y),(x+w,y+h),(x,y+h),(x,y)];lines.extend([[a,b] for a,b in zip(pts,pts[1:])])
def label(text,x,y,h=3.0,align='center',rotation=0):S['labels'].append({'text':text,'x':x,'y':y,'height':h,'align':align,'rotation':rotation})
rect(0,0,182.5,128.5,S['cut_lines']);rect(*S['led'],S['engrave_lines'])
for x in [7.5,175.14]:
 for y in [3,125.5]:
  S['slots'].append([x,y,5.5,3.2]);S['rack_head_slots'].append([x,y,8.3,6])
# Compact paired MIDI/CLOCK/SIDE columns, followed by one centred audio row.
cols=[36,49,62]
for x,tags in zip(cols,[['MIDI IN','MIDI OUT'],['CLOCK IN','CLOCK OUT'],['SIDECHAIN','']]):
 for y,tag in zip([28,13],tags):
  S['cut_circles'].append([x,y,7.8,tag]);S['clearance_circles'].append([x,y,10.8,'HOLE + 1.5 mm'])
for x,tag in [(83,'HEADPHONE OUT'),(101,'LINE OUT'),(119,'LINE IN')]:
 S['cut_circles'].append([x,18,11,tag]);S['clearance_circles'].append([x,18,14,'HOLE + 1.5 mm'])
S['cut_circles'].extend([[161.75,20.5,20,'DIN'],[149.75,11,3,'DIN SCREW'],[173.75,30,3,'DIN SCREW']])
S['reference_rects'].append([146.25,7.5,31,26,'DIN FLANGE 31 x 26'])
S['reference_rects'].append([*S['pcb'],'PCB 160 x 70.96'])
S['clearance_rects'].extend([[144.75,6,34,29,'DIN FLANGE + 1.5 mm'],[9.75,37.5,163,83,'LED + 1.5 mm']])
for text,x in [('MIDI',36),('CLOCK',49),('SIDE',62)]:label(text,x,20.5,align='middle_center')
for text,x in [('PHONES',83),('LINE-OUT',101),('LINE-IN',119)]:label(text,x,28.5,align='middle_center')
label('IN',25.5,28,3,align='middle_center')
label('OUT',25.5,13,3,align='middle_center')
S['slots'].append([136.5,18,11,12])
S['cutout_clearance_slots']=[[136.5,18,14,15]]
# Authoritative KiCad MountingHole footprints, mapped to the native-scale STEP placement.
import re
pcb_text=(OUT.parent/'PCB/toern_revH/toern_revH.kicad_pcb').read_text()
S['pcb_mount_holes']=[];S['pcb_mount_source']=str(OUT.parent/'PCB/toern_revH/toern_revH.kicad_pcb')
for m in re.finditer(r'\(footprint "MountingHole:MountingHole_2\.2mm_M2"',pcb_text):
 block=pcb_text[m.start():m.start()+1200]
 pos=re.search(r'\(at ([\d.-]+) ([\d.-]+)',block);name=re.search(r'\(property "Reference" "([^"]+)"',block)
 x,y=map(float,pos.groups());S['pcb_mount_holes'].append([round(x-19.73,5),round(y+18.48,5),2.2,'PCB '+name.group(1)])
S['cut_circles'].extend(S['pcb_mount_holes'])
S['PCB_orientation']={'rotation_axis':'X','rotation_degrees':180,'flat_underside_faces_panel':True,'components_face_case':True,'standoff_reference_length_mm':19.205}
S['pcb_mount_clearance_circles']=[[x,y,5,'M2 STANDOFF RESERVE'] for x,y,di,tag in S['pcb_mount_holes']]
S['cover']=[5.25,36,172,86]
S['clamp_holes']=[[x,y,2.2,'M2 CLAMP'] for x in [8.75,173.75] for y in [39.5,118.5]]
S['cut_circles'].extend(S['clamp_holes'])
S['reference_rects'].append([*S['cover'],'FRONT COVER 172 x 86'])
S['clamp_head_circles']=[[x,y,6,'M2 HEAD ENVELOPE'] for x,y,_,_ in S['clamp_holes']]
S['stack_mm']={'base_z':[-2,0],'LED_reference_z':[0,8],'front_cover_z':[8,10],'thicknesses_are_visualisation_only':True}
S['font']='Space Grotesk'
S['font_sizes_mm']=[2.4,3.0]
S['font_weight']='Bold'
S['logo_height_mm']=26
S['logo_rotation_degrees']=270
S['alignment']={'small_jack_pitch_mm':13,'small_jack_columns_mm':cols,'jack_row_y_mm':[28,13],'between_rows_label_y_mm':20.5,'audio_columns_mm':[83,101,119],'audio_row_y_mm':18,'audio_label_y_mm':28.5,'DIN_center_mm':[161.75,20.5],'shared_row_label_x_mm':25.5,'DIN_flange_right_mm':177.25,'LED_front_left_mm':5.25,'logo_left_mm':5.25}
# Preserve the existing Bebas Neue wordmark artwork; all functional labels stay editable text.
from svgpathtools import svg2paths
logo_paths,_=svg2paths('/Users/jank/git/toern/website/assets/img/logo.svg')
bounds=[p.bbox() for p in logo_paths];xmin=min(b[0] for b in bounds);xmax=max(b[1] for b in bounds);ymin=min(b[2] for b in bounds);ymax=max(b[3] for b in bounds)
scale=26/(xmax-xmin)
def lp(z):return [5.25+(ymax-z.imag)*scale,33.5-(z.real-xmin)*scale]
def flatten(seg,t0=0,t1=1,depth=0):
 a,b=lp(seg.point(t0)),lp(seg.point(t1));m=lp(seg.point((t0+t1)/2));q1=lp(seg.point(t0+(t1-t0)/4));q3=lp(seg.point(t0+3*(t1-t0)/4))
 from shapely.geometry import Point,LineString
 chord=LineString([a,b]);err=max(chord.distance(Point(q)) for q in [m,q1,q3])
 if err<.003 or depth>14:return [a,b]
 l=flatten(seg,t0,(t0+t1)/2,depth+1);r=flatten(seg,(t0+t1)/2,t1,depth+1);return l[:-1]+r
S['logo_polylines']=[]
for path in logo_paths:
 for sub in path.continuous_subpaths():
  pts=[]
  for seg in sub:
   part=flatten(seg);pts+=part if not pts else part[1:]
  S['logo_polylines'].append(pts)
S['logo_source']='/Users/jank/git/toern/website/assets/img/logo.svg'
S['requested_slot_pending']={'width_mm':13,'height_mm':70,'status':'Placement unresolved: a 70 mm slot intersects the LED stack at 36 HP; current 11 x 12 mm slot retained.'}
S['notes']=['Logo moved left, DIN right with no DIN text. Port groups shifted left to fit. The 70 x 13 mm slot placement remains unresolved; current slot retained.','MIDI, CLOCK and SIDE pairs use 13 mm column spacing. Audio PHONES, LINE-OUT and LINE-IN share one row at y=18 mm with centered labels above. The last cutout is a vertical 11 x 12 mm slot before the 270-degree rotated logo matching the DIN flange height (26 mm).','PCB flat underside faces panel; components face into case. Five M2 holes match current KiCad board. Use flush screw heads under the LED or an LED spacer gap.','DIN bore remains requested 20 mm. Clearance guides reserve 1.5 mm around cutouts and DIN flange; connector bodies and rear depths remain unverified.','Stack thicknesses and PCB standoff lengths are visualization references.']
(R/'panel_spec.json').write_text(json.dumps(S,indent=2,ensure_ascii=False))
def capsule(ms,x,y,w,h,layer):
 r=min(w,h)/2;dx=abs(w-h)/2;vertical=h>w;a={'layer':layer}
 def q(u,v):return (x-v,y+u) if vertical else (x+u,y+v)
 ms.add_line(q(-dx,r),q(dx,r),dxfattribs=a);ms.add_line(q(-dx,-r),q(dx,-r),dxfattribs=a)
 rot=90 if vertical else 0
 ms.add_arc(q(dx,0),r,(270+rot)%360,(90+rot)%360,dxfattribs=a);ms.add_arc(q(-dx,0),r,(90+rot)%360,(270+rot)%360,dxfattribs=a)
def capsule_shape(x,y,w,h):
 from shapely.geometry import LineString
 dx=abs(w-h)/2
 pts=[(x,y-dx),(x,y+dx)] if h>w else [(x-dx,y),(x+dx,y)]
 return LineString(pts).buffer(min(w,h)/2)
d=ezdxf.new('R2010');d.units=4;d.header['$MEASUREMENT']=1
for l,c in [('SCHNITT',7),('GRAVUR',3)]:d.layers.new(l,dxfattribs={'color':c})
d.styles.new('LABELS',dxfattribs={'font':'SpaceGrotesk-Bold.ttf'})
m=d.modelspace()
for a,b in S['cut_lines']:m.add_line(a,b,dxfattribs={'layer':'SCHNITT'})
for x,y,di,t in S['cut_circles']:m.add_circle((x,y),di/2,dxfattribs={'layer':'SCHNITT'})
for slot in S['slots']:capsule(m,*slot,'SCHNITT')
for a,b in S['engrave_lines']:m.add_line(a,b,dxfattribs={'layer':'GRAVUR'})
for t in S['labels']:
 align={'middle_left':TextEntityAlignment.MIDDLE_LEFT,'middle_center':TextEntityAlignment.MIDDLE_CENTER,'left':TextEntityAlignment.BOTTOM_LEFT,'center':TextEntityAlignment.BOTTOM_CENTER}[t['align']]
 m.add_text(t['text'],dxfattribs={'height':t['height'],'style':'LABELS','rotation':t['rotation'],'layer':'GRAVUR'}).set_placement((t['x'],t['y']),align=align)
for poly in S['logo_polylines']:m.add_lwpolyline(poly[:-1] if poly[0]==poly[-1] else poly,close=True,dxfattribs={'layer':'GRAVUR'})
d.set_modelspace_vport(height=140,center=(91.25,64.25));d.saveas(OUT/'modular.dxf')
front=ezdxf.new('R2010');front.units=4;front.layers.new('SCHNITT',dxfattribs={'color':7});fm=front.modelspace()
fp=[];rect(0,0,172,86,fp)
for a,b in fp:fm.add_line(a,b,dxfattribs={'layer':'SCHNITT'})
for x,y,di,t in S['clamp_holes']:fm.add_circle((x-5.25,y-36),di/2,dxfattribs={'layer':'SCHNITT'})
front.set_modelspace_vport(height=100,center=(86,43));front.saveas(OUT/'modular_front.dxf')
# Reference drawing is separate from the laser DXF, so clearance outlines cannot be cut accidentally.
for l,c in [('FREIHALTEN',30),('BAUTEIL_REFERENZ',4),('SCHRAUBENKOPF',1)]:d.layers.new(l,dxfattribs={'color':c,'plot':0})
for key,l in [('clearance_rects','FREIHALTEN'),('reference_rects','BAUTEIL_REFERENZ')]:
 for x,y,w,h,t in S[key]:
  lines=[];rect(x,y,w,h,lines)
  for a,b in lines:m.add_line(a,b,dxfattribs={'layer':l})
for x,y,di,t in S['clearance_circles']+S['pcb_mount_clearance_circles']:m.add_circle((x,y),di/2,dxfattribs={'layer':'FREIHALTEN'})
for slot in S['rack_head_slots']:capsule(m,*slot,'SCHRAUBENKOPF')
for slot in S['cutout_clearance_slots']:capsule(m,*slot,'FREIHALTEN')
for x,y,di,t in S['clamp_head_circles']:m.add_circle((x,y),di/2,dxfattribs={'layer':'SCHRAUBENKOPF'})
d.saveas(OUT/'modular_clearances.dxf')
fig,ax=plt.subplots(figsize=(14,10));ax.add_patch(Rectangle((0,0),182.5,128.5,fc='#faf9f5',ec='none'))
for key,col in [('clearance_rects','#dd8c23'),('reference_rects','#4b9898')]:
 for x,y,w,h,t in S[key]:ax.add_patch(Rectangle((x,y),w,h,fill=False,edgecolor=col,lw=.8,ls='--'))
for x,y,di,t in S['clearance_circles']+S['pcb_mount_clearance_circles']:ax.add_patch(Circle((x,y),di/2,fill=False,ec='#dd8c23',lw=.65,ls='--'))
for x,y,di,t in S['clamp_head_circles']:ax.add_patch(Circle((x,y),di/2,fill=False,ec='#bb7979',lw=.7,ls='--'))
for x,y,w,h in S['rack_head_slots']:
 from shapely.geometry import LineString
 p=capsule_shape(x,y,w,h)
 ax.plot(*p.exterior.xy,color='#bb7979',lw=.6,ls='--')
for x,y,w,h in S['cutout_clearance_slots']:
 p=capsule_shape(x,y,w,h)
 ax.plot(*p.exterior.xy,color='#dd8c23',lw=.7,ls='--')
for lines,c in [(S['cut_lines'],'#243c54'),(S['engrave_lines'],'#00894c')]:
 for a,b in lines:ax.plot([a[0],b[0]],[a[1],b[1]],color=c,lw=.8)
for x,y,di,t in S['cut_circles']:ax.add_patch(Circle((x,y),di/2,fill=False,ec='#243c54',lw=1))
for x,y,w,h in S['slots']:
 p=capsule_shape(x,y,w,h)
 ax.plot(*p.exterior.xy,color='#243c54',lw=1)
for t in S['labels']:ax.text(t['x'],t['y'],t['text'],color='#00894c',fontsize=t['height']*3.1,ha='left' if t['align'] in ['middle_left','left'] else 'center',va='center' if t['align'].startswith('middle') else 'bottom',rotation=t['rotation'],fontweight='bold')
for poly in S['logo_polylines']:ax.plot(*zip(*poly),color='#00894c',lw=.6)
ax.text(91.25,85,'LED · 160 × 80 mm\nengraved outline',ha='center',color='#6f967e',fontsize=12)
ax.text(91.25,65,'rev H PCB · 160 × 70.96 mm\nflat underside toward panel',ha='center',color='#4b9898',fontsize=10)
ax.text(161.75,1,'31 × 26 mm DIN flange',ha='center',color='#4b9898',fontsize=6)
ax.set_aspect('equal');ax.set_xlim(-5,187.5);ax.set_ylim(-5,134);ax.grid(alpha=.1);ax.set_xlabel('mm');ax.set_ylabel('mm')
ax.set_title('TŒRN modular · 36 HP · 182.5 × 128.5 mm\nBlue: cut · Green: editable engraving text / LED outline\nOrange: 1.5 mm clearance · Red: rack screw heads · Teal: component references',fontsize=12)
fig.text(.5,.025,'Jack body sizes and rear mounting depths remain to be checked; clearance markers are excluded from modular.dxf.',ha='center',fontsize=9,color='#945c2e')
fig.tight_layout(rect=(0,.04,1,1));fig.savefig(OUT/'modular.png',dpi=170)
print('Prepared 36HP revision with real TEXT, DIN screw holes and 5.5 x 3.2 mm mounting slots.')
