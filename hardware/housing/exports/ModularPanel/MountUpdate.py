import adsk.core,adsk.fusion,os,json,traceback,math
ROOT=os.path.dirname(__file__);HOUSING=os.path.abspath(os.path.join(ROOT,'..','..'))
def run(context):
 app=adsk.core.Application.get();ui=app.userInterface
 try:
  d=adsk.fusion.Design.cast(app.activeProduct);root=d.rootComponent
  sk=next(s for s in root.sketches if s.name.lower()=='modular')
  mode=json.load(open(os.path.join(ROOT,'label_mode.json')))['mode'] if os.path.exists(os.path.join(ROOT,'label_mode.json')) else 'update'
  def bounds(tx):
   pts=[]
   for c in tx.asCurves():
    ev=c.evaluator;ok,a,b=ev.getParameterExtents();ok,p=ev.getStrokes(a,b,.0001);pts.extend(p)
   return [min(p.x for p in pts)*10,min(p.y for p in pts)*10,max(p.x for p in pts)*10,max(p.y for p in pts)*10]
  def audit(target=None):
   nonlocal sk
   previous=sk
   if target:sk=target
   texts=[{'text':t.text,'height_mm':t.height*10,'font':t.fontName,'bounds':bounds(t)} for t in sk.sketchTexts]
   curves=[]
   for c in sk.sketchCurves:
    row={'type':c.objectType,'fixed':c.isFixed,'construction':c.isConstruction}
    ev=c.geometry.evaluator;ok,a,b=ev.getParameterExtents();ok,pts=ev.getStrokes(a,b,.0001)
    row['points_mm']=[[p.x*10,p.y*10] for p in pts]
    if isinstance(c,adsk.fusion.SketchCircle):
     p=c.centerSketchPoint.geometry;row['circle_mm']=[p.x*10,p.y*10,c.radius*10]
    elif isinstance(c,adsk.fusion.SketchArc):
     p=c.centerSketchPoint.geometry
     first,last=pts[0],pts[-1]
     a=math.degrees(math.atan2(first.y-p.y,first.x-p.x));b=math.degrees(math.atan2(last.y-p.y,last.x-p.x))
     if c.geometry.normal.z<0:a,b=b,a
     row['arc_mm']=[p.x*10,p.y*10,c.radius*10,a,b]
    curves.append(row)
   result={'document':app.activeDocument.name,'sketch':sk.name,'texts':texts,'curves':curves}
   sk=previous
   return result
  front=next(s for s in root.sketches if s.name=='modular_front')
  before=audit();before_front=audit(front)
  manager=d.exportManager
  backup='/tmp/toern_before_m25_mount_update.f3d'
  if not os.path.exists(backup) and not manager.execute(manager.createFusionArchiveExportOptions(backup)):raise RuntimeError('Backup failed')
  def pt(x,y):return adsk.core.Point3D.create(x/10,y/10,0)
  def circle(target,x,y,diam):return target.sketchCurves.sketchCircles.addByCenterRadius(pt(x,y),diam/20)
  def slot(target,x,y,w,h,construction=False):
   r=h/2;dx=(w-h)/2
   for yoff in (-r,r):
    c=target.sketchCurves.sketchLines.addByTwoPoints(pt(x-dx,y+yoff),pt(x+dx,y+yoff));c.isConstruction=construction
   for cx,sy in ((x+dx,y-r),(x-dx,y+r)):
    c=target.sketchCurves.sketchArcs.addByCenterStartSweep(pt(cx,y),pt(cx,sy),math.pi);c.isConstruction=construction
  def find(target,x,y,diam):
   return next(c for c in target.sketchCurves.sketchCircles if abs(c.centerSketchPoint.geometry.x*10-x)<.01 and abs(c.centerSketchPoint.geometry.y*10-y)<.01 and abs(c.radius*20-diam)<.01)
  clamps=[(x,y) for x in (8.75,173.75) for y in (39.5,118.5)]
  if not any(abs(c.radius*20-11.5)<.01 for c in sk.sketchCurves.sketchCircles):
   for x in (83,101,119):find(sk,x,18,11).deleteMe();circle(sk,x,18,11.5)
  for x,y in clamps:
   if any(abs(c.centerSketchPoint.geometry.x*10-x)<.01 and abs(c.centerSketchPoint.geometry.y*10-y)<.01 and abs(c.radius*20-2.8)<.01 for c in front.sketchCurves.sketchCircles):continue
   find(sk,x,y,2.2).deleteMe();slot(sk,x,y,5,2.8)
   find(front,x,y,2.2).deleteMe();circle(front,x,y,2.8)
  ref=next(s for s in root.sketches if s.name=='modular_clearances')
  for x in (83,101,119):
   try:c=find(ref,x,18,14)
   except StopIteration:continue
   c.deleteMe();circle(ref,x,18,14.5).isConstruction=True
  for x,y in clamps:
   try:c=find(ref,x,y,6)
   except StopIteration:continue
   c.deleteMe();slot(ref,x,y,8.2,6,True)
  # Extend the existing independent visualization bodies with matching cuts.
  for co in d.allComponents:
   if co.name.startswith('BASE_PANEL_REFERENCE_2mm') or co.name.startswith('FRONT_CLAMP_172x86_REFERENCE_2mm'):
    base=co.name.startswith('BASE_');cs=co.sketches.add(co.xYConstructionPlane);cs.name='M25_updated_plate_profile'
    source=sk if base else front
    oldbodies=list(co.bRepBodies)
    cs.isComputeDeferred=True
    for c in source.sketchCurves:
     if c.isConstruction or c.isFixed:continue
     if isinstance(c,adsk.fusion.SketchCircle):
      p=c.centerSketchPoint.geometry;circle(cs,p.x*10,p.y*10,c.radius*20)
     elif isinstance(c,adsk.fusion.SketchLine):cs.sketchCurves.sketchLines.addByTwoPoints(c.startSketchPoint.geometry,c.endSketchPoint.geometry)
     elif isinstance(c,adsk.fusion.SketchArc):
      ev=c.geometry.evaluator;ok,a,b=ev.getParameterExtents();ok,points=ev.getStrokes(a,b,.0001)
      cs.sketchCurves.sketchArcs.addByThreePoints(points[0],points[len(points)//2],points[-1])
     else:raise RuntimeError('Unexpected cut curve '+c.objectType)
    cs.isComputeDeferred=False
    profile=max(list(cs.profiles),key=lambda p:p.areaProperties().area)
    feature=co.features.extrudeFeatures.addSimple(profile,adsk.core.ValueInput.createByReal(-.2 if base else .2),adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
    body=feature.bodies.item(0);body.name=co.name;body.opacity=.16 if base else .18
    for old in oldbodies:old.deleteMe()
    cs.isVisible=False
   elif co.name=='M2_HEAD_ENVELOPES_6mm_REFERENCE':co.name='M25_HEAD_ENVELOPES_6mm_REFERENCE'
  d.computeAll();adsk.doEvents()
  after=audit();after['front']=audit(front)
  after['direction_icons']=json.load(open(os.path.join(ROOT,'panel_spec.json')))['direction_icons']
  after['mounts']={'front_screw':'M2.5','front_hole_diameter_mm':2.8,'base_slots_mm':[5,2.8],'audio_holes_diameter_mm':11.5,'pcb_screw':'M2','pcb_holes_diameter_mm':2.2}
  before_pcb=[c for c in before['curves'] if 'circle_mm' in c and abs(c['circle_mm'][2]-1.1)<.001 and not any(abs(c['circle_mm'][0]-x)<.01 and abs(c['circle_mm'][1]-y)<.01 for x,y in clamps)]
  assert len(before_pcb)==5 and all(c in after['curves'] for c in before_pcb)
  after['pcb_mounts_preserved']=True
  assert before['texts']==after['texts']
  assert [c for c in before['curves'] if c['fixed']]==[c for c in after['curves'] if c['fixed']]
  json.dump(after,open(os.path.join(ROOT,'mount_update_result.json'),'w'),indent=2)
  for s in root.sketches:s.isVisible=s.name in ('modular','modular_clearances')
  cam=app.activeViewport.camera;cam.isPerspective=False
  cam.eye=adsk.core.Point3D.create(9.125,6.425,60);cam.target=adsk.core.Point3D.create(9.125,6.425,0);cam.upVector=adsk.core.Vector3D.create(0,1,0);cam.isFitView=True
  app.activeViewport.camera=cam;adsk.doEvents();app.activeViewport.refresh();adsk.doEvents()
  app.activeViewport.saveAsImageFile(os.path.join(HOUSING,'modular_pcb.png'),1800,1300)
  if not manager.execute(manager.createFusionArchiveExportOptions(os.path.join(HOUSING,'toern_final_v3_modular.f3d'))):raise RuntimeError('Archive export failed')
  ui.messageBox('Updated M2.5 front holes diameter 2.8 mm, horizontal base slots 5 x 2.8 mm, three audio holes diameter 11.5 mm. Five PCB M2 holes and all engraving preserved. Reference bodies and archive updated.')
 except:
  with open(os.path.join(ROOT,'mount_error.txt'),'w') as f:f.write(traceback.format_exc())
  ui.messageBox(traceback.format_exc())
