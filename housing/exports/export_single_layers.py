from pathlib import Path
import json,math,collections,os
import ezdxf
ROOT=Path(__file__).resolve().parent
SOURCE=ROOT/'v108_single_source'
DEST=ROOT.parent

def main():
 data=json.loads((SOURCE/'fusion_geometry.json').read_text())
 assert data['document']=='toern_final_v3 v108'
 assert {s['name'].upper() for s in data['sketches']}=={'COMMON','16_16','32_16'}
 staged=[]
 for s in data['sketches']:
  doc=ezdxf.new('R2010');doc.units=4;doc.header['$MEASUREMENT']=1
  doc.layers.new('SCHNITT',dxfattribs={'color':7})
  doc.layers.new('GRAVUR',dxfattribs={'color':3})
  ms=doc.modelspace();seen=set();removed=0
  curves=[(c,'GRAVUR' if c['fixed'] else 'SCHNITT') for c in s['curves'] if not c['construction']]
  curves.extend((c,'GRAVUR') for text in s['texts'] for c in text['curves'])
  for c,layer in curves:
   t=c['type'];a={'layer':layer}
   if t=='Line3D':
    key=(layer,t,tuple(sorted(tuple(round(v,6) for v in c[k][:2]) for k in ['start','end'])))
    if key in seen:removed+=1;continue
    seen.add(key);ms.add_line(c['start'][:2],c['end'][:2],dxfattribs=a)
   elif t=='Circle3D': ms.add_circle(c['center'][:2],c['radius'],dxfattribs=a)
   elif t=='Arc3D':
    x,y=c['center'][:2]
    st=math.degrees(math.atan2(c['start'][1]-y,c['start'][0]-x))%360
    en=math.degrees(math.atan2(c['end'][1]-y,c['end'][0]-x))%360
    if c['normal'][2]<0: st,en=en,st
    ms.add_arc((x,y),c['radius'],st,en,dxfattribs=a)
   else:
    pts=[p[:2] for p in c['points']];closed=math.dist(pts[0],pts[-1])<1e-7
    ms.add_lwpolyline(pts[:-1] if closed else pts,close=closed,dxfattribs=a)
  name=s['name'].upper() if s['name'].upper()=='COMMON' else s['name']
  file=SOURCE/(name+'.dxf');doc.saveas(file)
  verify=ezdxf.readfile(file);assert not verify.audit().has_errors
  assert len(verify.modelspace())==len(curves)-removed
  print(name,dict(collections.Counter(e.dxf.layer for e in verify.modelspace())),'duplicate lines removed:',removed)
  staged.append((file,DEST/file.name))
 for file,dest in staged:os.replace(file,dest)
 print('Verified and replaced all three DXFs.')
if __name__=='__main__':main()
