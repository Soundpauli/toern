import adsk.core,adsk.fusion,json,os,traceback,math
ROOT=os.path.dirname(__file__);HOUSING=os.path.abspath(os.path.join(ROOT,'..','..'))
STEP=os.path.join(ROOT,'PCB_current.step')
def run(context):
 app=adsk.core.Application.get();ui=app.userInterface;made=[]
 try:
  d=adsk.fusion.Design.cast(app.activeProduct)
  if not d:raise RuntimeError('No active design')
  root=d.rootComponent
  if not any(s.name in ('COMMON','32_16') for s in root.sketches):raise RuntimeError('Expected TŒRN housing design')
  if 'Space Grotesk' not in app.fontNames:raise RuntimeError('Space Grotesk not available')
  data=json.load(open(os.path.join(ROOT,'panel_spec.json')))
  def sync():
   d.computeAll();adsk.doEvents();app.activeViewport.refresh();adsk.doEvents()
  def pt(p):return adsk.core.Point3D.create(p[0]/10,p[1]/10,0)
  def line(target,a,b,fixed=False,construction=False):
   if abs(a[0]-b[0])+abs(a[1]-b[1])<1e-8:return
   c=target.sketchCurves.sketchLines.addByTwoPoints(pt(a),pt(b));c.isConstruction=construction
   if fixed:c.isFixed=True
   return c
  def rect(target,x,y,w,h,construction=False,fixed=False):
   ps=[(x,y),(x+w,y),(x+w,y+h),(x,y+h),(x,y)]
   for a,b in zip(ps,ps[1:]):line(target,a,b,fixed,construction)
  def circle(target,x,y,di,construction=False):
   c=target.sketchCurves.sketchCircles.addByCenterRadius(pt((x,y)),di/20);c.isConstruction=construction;return c
  def slot(target,x,y,w,h,construction=False):
   rad=min(w,h)/2;dx=abs(w-h)/2;vertical=h>w
   def q(u,v):return (x-v,y+u) if vertical else (x+u,y+v)
   for a,b in [(q(-dx,rad),q(dx,rad)),(q(-dx,-rad),q(dx,-rad))]:line(target,a,b,construction=construction)
   for center,start in [(q(dx,0),q(dx,-rad)),(q(-dx,0),q(-dx,rad))]:
    a=target.sketchCurves.sketchArcs.addByCenterStartSweep(pt(center),pt(start),math.pi);a.isConstruction=construction
  old=[s for s in root.sketches if s.name in ('modular','modular_clearances','modular_front')]
  sk=root.sketches.add(root.xYConstructionPlane);sk.name='modular_update';made.append(sk)
  ref=root.sketches.add(root.xYConstructionPlane);ref.name='modular_clearances_update';made.append(ref)
  front=root.sketches.add(root.xYConstructionPlane);front.name='modular_front_update';made.append(front)
  for s in made:s.isComputeDeferred=True
  for a,b in data['cut_lines']:line(sk,a,b)
  for x,y,diam,tag in data['cut_circles']:circle(sk,x,y,diam).attributes.add('TOERN','function',tag)
  for sl in data['slots']:slot(sk,*sl)
  for a,b in data['engrave_lines']:line(sk,a,b,True)
  for poly in data['logo_polylines']:
   for a,b in zip(poly,poly[1:]):line(sk,a,b,True)
   if poly[0]!=poly[-1]:line(sk,poly[-1],poly[0],True)
  sk.isComputeDeferred=False
  text_checks=[]
  def glyph_bounds(tx):
   pts=[]
   for c in tx.asCurves():
    ev=c.evaluator;ok,lo,hi=ev.getParameterExtents()
    if not ok:raise RuntimeError('Cannot measure text outline')
    ok,stroke=ev.getStrokes(lo,hi,.0001)
    if not ok:raise RuntimeError('Cannot measure text outline strokes')
    pts.extend(stroke)
   return [min(p.x for p in pts),min(p.y for p in pts),max(p.x for p in pts),max(p.y for p in pts)]
  for t in data['labels']:
   def add_label(cx,cy):
    inp=sk.sketchTexts.createInput3("'"+t['text']+"'",adsk.core.ValueInput.createByReal(t['height']/10))
    inp.fontName='Space Grotesk';inp.textStyle=adsk.fusion.TextStyles.TextStyleBold
    inp.setAsMultiLine(pt((cx-12,cy-2)),pt((cx+12,cy+2)),adsk.core.HorizontalAlignments.CenterHorizontalAlignment,adsk.core.VerticalAlignments.MiddleVerticalAlignment,0)
    return sk.sketchTexts.add(inp)
   tx=add_label(t['x'],t['y']);bounds=glyph_bounds(tx)
   shift_x=t['x']-(bounds[0]+bounds[2])*5;shift_y=t['y']-(bounds[1]+bounds[3])*5
   tx.deleteMe();tx=add_label(t['x']+shift_x,t['y']+shift_y);tx.isFixed=True
   bounds=glyph_bounds(tx)
   text_checks.append({'text':t['text'],'glyph_center_mm':[(bounds[0]+bounds[2])*5,(bounds[1]+bounds[3])*5],'target_center_mm':[t['x'],t['y']]})
  for key in ['clearance_rects','reference_rects']:
   for x,y,w,h,tag in data[key]:rect(ref,x,y,w,h,True)
  for key in ['clearance_circles','clamp_head_circles','pcb_mount_clearance_circles']:
   for x,y,di,tag in data[key]:circle(ref,x,y,di,True)
  for sl in data['rack_head_slots']:slot(ref,*sl,construction=True)
  for sl in data['cutout_clearance_slots']:slot(ref,*sl,construction=True)
  rect(front,*data['cover'])
  for x,y,di,tag in data['clamp_holes']:circle(front,x,y,di)
  sk.attributes.add('TOERN','specification',json.dumps(data,ensure_ascii=False))
  for s in made:
   s.isComputeDeferred=False;s.arePointsShown=False;s.areConstraintsShown=False;s.areDimensionsShown=False;s.areProfilesShown=False
  sk.isConstructionGeometryShown=False
  for s in old:s.deleteMe()
  sk.name='modular';ref.name='modular_clearances';front.name='modular_front'
  # Import PCB once, then measure its local geometry with the wrapper at identity.
  for o in list(root.occurrences):
   if o.component.name=='PCB_REVH_REFERENCE':o.deleteMe()
  pcbocc=root.occurrences.addNewComponent(adsk.core.Matrix3D.create());pcbocc.component.name='PCB_REVH_REFERENCE'
  if not app.importManager.importToTarget2(app.importManager.createSTEPImportOptions(STEP),pcbocc.component):raise RuntimeError('STEP import failed')
  pcbocc.transform2=adsk.core.Matrix3D.create();sync()
  bodies=list(pcbocc.component.bRepBodies)
  for occ in pcbocc.component.allOccurrences:bodies.extend(list(occ.bRepBodies))
  def dims(b):
   bb=b.boundingBox;return [bb.maxPoint.x-bb.minPoint.x,bb.maxPoint.y-bb.minPoint.y,bb.maxPoint.z-bb.minPoint.z]
  board=next((b for b in bodies if abs(dims(b)[0]-16)<.05 and abs(dims(b)[1]-7.096)<.05),None)
  if not board:raise RuntimeError('PCB body not identified')
  bb=board.boundingBox
  source_holes=[]
  for edge in board.edges:
   geom=edge.geometry
   if geom.objectType.endswith('Circle3D') and abs(geom.radius-.11)<.0001:
    source_holes.append([geom.center.x*10,geom.center.y*10])
  mount_checks=[]
  for x,y,di,tag in data['pcb_mount_holes']:
   expected=[x+19.73,18.48-y]
   err=min([math.hypot(cx-expected[0],cy-expected[1]) for cx,cy in source_holes] or [999])
   mount_checks.append({'name':tag,'position_mm':[x,y],'STEP_alignment_error_mm':err})
  if any(m['STEP_alignment_error_mm']>.02 for m in mount_checks):raise RuntimeError('KiCad and STEP mounting patterns differ: '+repr(mount_checks))
  maxz=max(b.boundingBox.maxPoint.z for b in bodies)
  # Flip about the long board axis: native component-side +Z points into the case (-Z).
  dx=9.125-(bb.minPoint.x+bb.maxPoint.x)/2
  dy=7.9+(bb.minPoint.y+bb.maxPoint.y)/2
  stand_plane_z=(-2-data['PCB_orientation']['standoff_reference_length_mm'])/10
  dz=stand_plane_z+bb.minPoint.z
  tr=adsk.core.Matrix3D.create();tr.setToRotation(math.pi,adsk.core.Vector3D.create(1,0,0),adsk.core.Point3D.create(0,0,0))
  tr.translation=adsk.core.Vector3D.create(dx,dy,dz);pcbocc.transform2=tr;pcbocc.isLightBulbOn=True;sync()
  if abs(tr.getCell(2,2)+1)>1e-8:raise RuntimeError('PCB must face component side into the case')
  try:
   if d.snapshots.hasPendingSnapshot:d.snapshots.add()
  except RuntimeError:pass
  # Layer stack: dimensions in Z are illustrative until sheet and LED thicknesses are specified.
  oldstacks=[o for o in root.occurrences if o.component.name=='MODULAR_STACK_REFERENCE']
  stack=root.occurrences.addNewComponent(adsk.core.Matrix3D.create());stack.component.name='MODULAR_STACK_UPDATE'
  def part(name,z,outline,holes,distance,opacity,slots=False):
   tr=adsk.core.Matrix3D.create();tr.translation=adsk.core.Vector3D.create(0,0,z/10)
   occ=stack.component.occurrences.addNewComponent(tr);co=occ.component;co.name=name
   s=co.sketches.add(co.xYConstructionPlane);s.name=name+'_profile';s.isComputeDeferred=True
   rect(s,*outline)
   for x,y,di,tag in holes:circle(s,x,y,di)
   if slots:
    for sl in data['slots']:slot(s,*sl)
   s.isComputeDeferred=False
   profile=max(list(s.profiles),key=lambda p:p.areaProperties().area)
   feature=co.features.extrudeFeatures.addSimple(profile,adsk.core.ValueInput.createByReal(distance/10),adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
   body=feature.bodies.item(0);body.name=name;body.opacity=opacity;s.isVisible=False
   return occ
  base=part('BASE_PANEL_REFERENCE_2mm',0,[0,0,182.5,128.5],data['cut_circles'],-2,.16,True)
  # Five bored rear standoffs align to the actual PCB mounting holes.
  stand_z=(dz-bb.minPoint.z)*10
  stand_height=-2-stand_z
  st=adsk.core.Matrix3D.create();st.translation=adsk.core.Vector3D.create(0,0,stand_z/10)
  posts=stack.component.occurrences.addNewComponent(st);posts.component.name='PCB_M2_STANDOFFS_REFERENCE_LENGTH_ASSUMED'
  ps=posts.component.sketches.add(posts.component.xYConstructionPlane)
  for x,y,di,tag in data['pcb_mount_holes']:
   circle(ps,x,y,4);circle(ps,x,y,2.2)
  rings=[p for p in ps.profiles if p.profileLoops.count==2]
  if len(rings)!=5:raise RuntimeError('Expected five rear standoff rings')
  for p in rings:
   f=posts.component.features.extrudeFeatures.addSimple(p,adsk.core.ValueInput.createByReal(stand_height/10),adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
   f.bodies.item(0).opacity=.55
  ps.isVisible=False
  led=part('LED_PANEL_REFERENCE_160x80_THICKNESS_ASSUMED_8mm',0,data['led'],[],8,.12)
  cap=part('FRONT_CLAMP_172x86_REFERENCE_2mm',8,data['cover'],data['clamp_holes'],2,.18)
  heads=part('M2_HEAD_ENVELOPES_DIAMETER_6mm',10,[0,0,.01,.01],[],.01,.1) if False else None
  # Four head-envelope cylinders show the selected generous M2 head allowance.
  tr=adsk.core.Matrix3D.create();tr.translation=adsk.core.Vector3D.create(0,0,1.0)
  headocc=stack.component.occurrences.addNewComponent(tr);headocc.component.name='M2_HEAD_ENVELOPES_6mm_REFERENCE'
  hs=headocc.component.sketches.add(headocc.component.xYConstructionPlane)
  for x,y,di,t in data['clamp_head_circles']:circle(hs,x,y,di)
  for p in hs.profiles:
   f=headocc.component.features.extrudeFeatures.addSimple(p,adsk.core.ValueInput.createByReal(.15),adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
   f.bodies.item(0).opacity=.5
  hs.isVisible=False
  for o in oldstacks:o.deleteMe()
  stack.component.name='MODULAR_STACK_REFERENCE';stack.attributes.add('TOERN','Z_dimensions','Illustrative only: base 2 mm, LED 8 mm, front cover 2 mm. Set actual thickness before fabrication hardware selection.')
  for other in root.sketches:other.isVisible=other in (sk,ref)
  sync()
  def camera(iso=False):
   cam=app.activeViewport.camera;cam.isPerspective=False
   cam.eye=adsk.core.Point3D.create(29,-25,34) if iso else adsk.core.Point3D.create(9.125,6.425,60)
   cam.target=adsk.core.Point3D.create(9.125,6.425,0);cam.upVector=adsk.core.Vector3D.create(0,0,1) if iso else adsk.core.Vector3D.create(0,1,0)
   cam.isFitView=True;app.activeViewport.camera=cam;adsk.doEvents();app.activeViewport.refresh();adsk.doEvents()
  camera();app.activeViewport.saveAsImageFile(os.path.join(HOUSING,'modular_pcb.png'),1800,1300)
  # Exploded preview, then restore the assembled stack before exporting.
  def zpose(occ,z):
   t=adsk.core.Matrix3D.create();t.translation=adsk.core.Vector3D.create(0,0,z/10);occ.createForAssemblyContext(stack).transform2=t
  zpose(led,10);zpose(cap,30);zpose(headocc,32);sync();camera(True)
  app.activeViewport.saveAsImageFile(os.path.join(HOUSING,'modular_stack.png'),1800,1300)
  zpose(led,0);zpose(cap,8);zpose(headocc,10);sync()
  try:
   if d.snapshots.hasPendingSnapshot:d.snapshots.add()
  except RuntimeError:pass
  # Rear view makes the component-side orientation visible.
  cam=app.activeViewport.camera;cam.isPerspective=False;cam.eye=adsk.core.Point3D.create(29,-25,-34);cam.target=adsk.core.Point3D.create(9.125,6.425,-2);cam.upVector=adsk.core.Vector3D.create(0,1,0);cam.isFitView=True;app.activeViewport.camera=cam;adsk.doEvents();app.activeViewport.refresh();adsk.doEvents()
  app.activeViewport.saveAsImageFile(os.path.join(HOUSING,'modular_pcb_rear.png'),1800,1300)
  camera()
  box=pcbocc.boundingBox
  result={'document':app.activeDocument.name,'sketch':sk.name,'curves':sk.sketchCurves.count,'circles':sk.sketchCurves.sketchCircles.count,'texts':sk.sketchTexts.count,'slots':len(data['slots']),'rack_slots':4,'long_slot_mm':data['slots'][-1][2:],'slot_center_mm':data['slots'][-1][:2],'PCB_mount_holes':mount_checks,'PCB_standoff_length_mm_reference':stand_height,'clearance_curves':ref.sketchCurves.count,'pcb_source':STEP,'pcb_body_dimensions_mm':[v*10 for v in dims(board)],'assembly_bounds_mm':{'min_mm':[box.minPoint.x*10,box.minPoint.y*10,box.minPoint.z*10],'max_mm':[box.maxPoint.x*10,box.maxPoint.y*10,box.maxPoint.z*10]},'assembly_fits_panel_xy':box.minPoint.x>=0 and box.maxPoint.x<=18.25 and box.minPoint.y>=0 and box.maxPoint.y<=12.85,'pcb_local_board_bbox_mm':[[bb.minPoint.x*10,bb.minPoint.y*10,bb.minPoint.z*10],[bb.maxPoint.x*10,bb.maxPoint.y*10,bb.maxPoint.z*10]],'pcb_transform_mm':[dx*10,dy*10,dz*10],'depth_reference_only':True,'PCB_orientation':data['PCB_orientation'],'PCB_component_side_normal_world':[pcbocc.transform2.getCell(0,2),pcbocc.transform2.getCell(1,2),pcbocc.transform2.getCell(2,2)],'fonts':list(set(t.fontName for t in sk.sketchTexts)),'label_contents':[t.text for t in sk.sketchTexts],'text_center_checks':text_checks,'font_sizes_mm':data['font_sizes_mm'],'font_weight':'Bold','text_styles':[int(t.textStyle) for t in sk.sketchTexts],'alignment':data['alignment'],'cover_mm':[172,86],'clamp_holes_mm':data['clamp_holes'],'clamp_head_allowance_mm':6,'stack_mm':data['stack_mm'],'logo_source':data['logo_source']}
  with open(os.path.join(ROOT,'result.json'),'w') as f:json.dump(result,f,indent=2)
  manager=d.exportManager
  if not manager.execute(manager.createFusionArchiveExportOptions(os.path.join(HOUSING,'toern_final_v3_modular.f3d'))):raise RuntimeError('Archive export failed')
  ui.messageBox('modular updated: compact MIDI/CLOCK/SIDE pairs, one HEAD / LINE-OUT / LINE-IN audio row with labels above, final vertical 11 x 12 mm slot and rotated logo matching DIN height; glyph centers measured geometrically. Archive and previews exported.')
 except:
  for s in made:
   try:s.isComputeDeferred=False
   except:pass
  with open(os.path.join(ROOT,'error.txt'),'w') as f:f.write(traceback.format_exc())
  ui.messageBox(traceback.format_exc())
