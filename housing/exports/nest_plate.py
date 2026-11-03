import json, math, collections
from pathlib import Path
import numpy as np
from shapely.geometry import LineString, Polygon, Point, box
from shapely.ops import polygonize, unary_union
import ezdxf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
ROOT=Path(__file__).resolve().parent
DATA=json.load(open(ROOT/'fusion_geometry.json'))
def points(c):
 t=c['type']
 if t=='Line3D': return [c['start'][:2],c['end'][:2]]
 if t in ('Arc3D','Circle3D'):
  x,y=c['center'][:2];r=c['radius']
  if t=='Circle3D': a,b=0,2*math.pi
  else:
   a=math.atan2(c['start'][1]-y,c['start'][0]-x); b=math.atan2(c['end'][1]-y,c['end'][0]-x)
   if c['normal'][2]>0: b=a+(b-a)%(2*math.pi)
   else: b=a-((a-b)%(2*math.pi))
  n=max(8,math.ceil(abs(b-a)/0.025))
  return [(x+r*math.cos(v),y+r*math.sin(v)) for v in np.linspace(a,b,n+1)]
 return [p[:2] for p in c['points']]
def line(c): return LineString([(round(x,6),round(y,6)) for x,y in points(c)])
def prepare():
 parts=[]
 fig,axs=plt.subplots(1,2,figsize=(15,9))
 for ax,s in zip(axs,DATA['sketches']):
  cuts=[c for c in s['curves'] if not c['construction'] and not c['fixed']]
  engr=[c for c in s['curves'] if not c['construction'] and c['fixed']]+[c for t in s['texts'] for c in t['curves']]
  polys=list(polygonize(unary_union([line(c) for c in cuts])))
  # Each outside boundary is a part; internal cutouts belong to that part.
  shells=[Polygon(p.exterior) for p in polys]
  outer=[p for p in shells if not any(q.area>p.area+0.01 and q.buffer(0.0001).covers(p) for q in shells)]
  print(s['name'],'polygons',len(polys),'parts',len(outer),'area',sum(p.area for p in outer))
  assigned=[]
  for i,p in enumerate(sorted(outer,key=lambda p:(p.bounds[1],p.bounds[0]))):
   cs=[c for c in cuts if p.buffer(0.001).covers(line(c))]
   es=[c for c in engr if p.buffer(0.001).covers(line(c))]
   parts.append({'name':f'{s["name"]}_{i+1}','poly':p,'cuts':cs,'engr':es})
   assigned+=cs+es
   x,y=p.representative_point().coords[0];ax.text(x,y,f'{s["name"]}_{i+1}',fontsize=8)
   print(parts[-1]['name'],tuple(round(v,3) for v in p.bounds),'cuts',len(cs),'engr',len(es))
  missing=[c for c in cuts+engr if not any(c is a for a in assigned)]
  print('UNASSIGNED',len(missing),collections.Counter(c['type'] for c in missing))
  if missing: (ROOT/(s['name']+'_unassigned.json')).write_text(json.dumps(missing,indent=2))
  for c in cuts+engr:
   ps=points(c);ax.plot(*zip(*ps),color='#10934f' if any(c is e for e in engr) else '#263851',lw=.6)
  ax.set_aspect('equal');ax.set_title(s['name']);ax.grid(alpha=.2)
 fig.tight_layout();fig.savefig(ROOT/'source_geometry.png',dpi=150)
 return parts
if __name__=='__main__': prepare()

def pack_rect(parts,count,gap=4):
 import rectpack,random
 best=None
 for algo in [rectpack.MaxRectsBssf,rectpack.MaxRectsBaf,rectpack.MaxRectsBlsf,rectpack.MaxRectsBl,rectpack.SkylineBl,rectpack.SkylineMwf,rectpack.GuillotineBssfSas,rectpack.GuillotineBafLas]:
  for sort in [rectpack.SORT_AREA,rectpack.SORT_PERI,rectpack.SORT_DIFF,rectpack.SORT_SSIDE,rectpack.SORT_LSIDE,rectpack.SORT_RATIO]:
   packer=rectpack.newPacker(pack_algo=algo,sort_algo=sort,rotation=True)
   for k in range(count):
    for i,p in enumerate(parts):
     a,b,c,d=p['poly'].bounds; packer.add_rect(c-a+gap,d-b+gap,(k,i))
   packer.add_bin(750-2*4+gap,500-2*4+gap);packer.pack()
   rects=packer.rect_list()
   if len(rects)==count*len(parts):
    score=max(r[1]+r[3] for r in rects)*max(r[2]+r[4] for r in rects)
    if best is None or score<best[0]: best=(score,rects)
 return best

def generate(parts,count=4,gap=4):
 from shapely import affinity
 packed=pack_rect(parts,count,gap)
 assert packed
 doc=ezdxf.new('R2010'); doc.units=4
 doc.header['$MEASUREMENT']=1;doc.header['$LIMMIN']=(0,0);doc.header['$LIMMAX']=(750,500)
 doc.layers.new('SCHNITT',dxfattribs={'color':7})
 doc.layers.new('GRAVUR',dxfattribs={'color':3})
 msp=doc.modelspace(); placed=[];stats=collections.Counter(); removed=0
 fig,ax=plt.subplots(figsize=(15,10))
 ax.add_patch(plt.Rectangle((0,0),750,500,facecolor='#f7f3e9',edgecolor='#666',lw=1))
 ax.add_patch(plt.Rectangle((4,4),742,492,fill=False,edgecolor='#bbb',lw=.6,linestyle='--'))
 transforms=[]
 for _,x,y,w,h,(k,i) in packed[1]:
  p=parts[i];a,b,c,d=p['poly'].bounds
  rotated=abs(w-(c-a+gap))>0.0001
  shape=affinity.rotate(p['poly'],90 if rotated else 0,origin=(0,0))
  aa,bb,cc,dd=shape.bounds;tx=x+4-aa;ty=y+4-bb
  shape=affinity.translate(shape,tx,ty);placed.append((p['name'],k,shape))
  def tr(q):
   u,v=q[:2]
   if rotated:u,v=-v,u
   return (u+tx,v+ty)
  seen=set()
  for layer,curves in [('SCHNITT',p['cuts']),('GRAVUR',p['engr'])]:
   for cv in curves:
    t=cv['type'];at={'layer':layer}
    if t=='Line3D':
     key=(layer,t,tuple(sorted([tuple(round(z,6) for z in cv['start'][:2]),tuple(round(z,6) for z in cv['end'][:2])])))
     if key in seen:removed+=1;continue
     seen.add(key);msp.add_line(tr(cv['start']),tr(cv['end']),dxfattribs=at)
    elif t=='Circle3D':msp.add_circle(tr(cv['center']),cv['radius'],dxfattribs=at)
    elif t=='Arc3D':
     ce=tr(cv['center']);st=tr(cv['start']);en=tr(cv['end'])
     sa=math.degrees(math.atan2(st[1]-ce[1],st[0]-ce[0]))%360
     ea=math.degrees(math.atan2(en[1]-ce[1],en[0]-ce[0]))%360
     if cv['normal'][2]<0:sa,ea=ea,sa
     msp.add_arc(ce,cv['radius'],sa,ea,dxfattribs=at)
    else:
     pts=[tr(q) for q in cv['points']]
     closed=math.dist(pts[0],pts[-1])<1e-7
     msp.add_lwpolyline(pts[:-1] if closed else pts,close=closed,dxfattribs=at)
    stats[layer]+=1
    xy=[tr(q) for q in points(cv)];ax.plot(*zip(*xy),c='#188849' if layer=='GRAVUR' else '#243b53',lw=.55)
  rp=shape.representative_point();ax.text(rp.x,rp.y,f'{k+1} · {p["name"]}',ha='center',va='center',fontsize=5.5,color='#7c8792')
  transforms.append({'set':k+1,'part':p['name'],'rotation':90 if rotated else 0,'translation_mm':[tx,ty],'bounds_mm':shape.bounds})
 min_gap=min(a[2].distance(b[2]) for i,a in enumerate(placed) for b in placed[i+1:])
 min_margin=min(min(p.bounds[0],p.bounds[1],750-p.bounds[2],500-p.bounds[3]) for _,_,p in placed)
 assert min_gap>=gap-1e-5 and min_margin>=4-1e-5,(min_gap,min_margin)
 assert all(box(0,0,750,500).covers(p) for _,_,p in placed)
 out=ROOT/f'TOERN_COMMON_32_16_750x500_{count}sets.dxf'
 doc.set_modelspace_vport(height=530,center=(375,250));doc.saveas(out)
 audit=ezdxf.readfile(out).audit();assert not audit.has_errors
 ax.set_xlim(-7,757);ax.set_ylim(-7,507);ax.set_aspect('equal');ax.set_xlabel('mm');ax.set_ylabel('mm')
 ax.set_title(f'TŒRN · COMMON + 32_16 · {count} vollständige Sätze / {8*count} Bauteile\n750 × 500 mm · Abstand ≥ {gap} mm · Blau: SCHNITT · Grün: GRAVUR',pad=12)
 fig.tight_layout();fig.savefig(out.with_suffix('.png'),dpi=180);plt.close(fig)
 report={'source':DATA['document'],'plate_mm':[750,500],'sets':count,'parts':len(placed),'min_part_clearance_mm':min_gap,'min_plate_clearance_mm':min_margin,'entities':dict(stats),'duplicate_cut_lines_removed':removed,'text_outline_sampling_tolerance_mm':0.001,'layers':{'SCHNITT':'Schnittkonturen und Ausschnitte','GRAVUR':'Fixierte Kurven und TŒRN-Schrift als Konturen'},'placement':transforms,'dxf_audit_errors':len(audit.errors)}
 out.with_suffix('.json').write_text(json.dumps(report,indent=2,ensure_ascii=False))
 print('OUTPUT',out,'VALIDATED',min_gap,min_margin,dict(stats),'duplicates removed',removed)
 return report
