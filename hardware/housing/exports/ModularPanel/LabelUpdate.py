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
  def audit():
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
   return {'document':app.activeDocument.name,'sketch':sk.name,'texts':texts,'curves':curves}
  before=audit()
  if mode=='inspect':
   json.dump(before,open(os.path.join(ROOT,'current_label_audit.json'),'w'),indent=2)
   ui.messageBox('Current modular sketch inspected; geometry preserved. Audit exported.');return
  spec=json.load(open(os.path.join(ROOT,'panel_spec.json')))
  manager=d.exportManager
  backup=os.path.join('/tmp','toern_v118_before_label_icons.f3d')
  if not manager.execute(manager.createFusionArchiveExportOptions(backup)):raise RuntimeError('Backup failed')
  def pt(x,y):return adsk.core.Point3D.create(x/10,y/10,0)
  def new_text(text,x,y,height):
   def add(cx,cy):
    inp=sk.sketchTexts.createInput3("'"+text+"'",adsk.core.ValueInput.createByReal(height/10))
    inp.fontName='Space Grotesk';inp.textStyle=adsk.fusion.TextStyles.TextStyleBold
    inp.setAsMultiLine(pt(cx-12,cy-2),pt(cx+12,cy+2),adsk.core.HorizontalAlignments.CenterHorizontalAlignment,adsk.core.VerticalAlignments.MiddleVerticalAlignment,0)
    return sk.sketchTexts.add(inp)
   tx=add(x,y);b=bounds(tx);dx=x-(b[0]+b[2])/2;dy=y-(b[1]+b[3])/2
   tx.deleteMe();tx=add(x+dx,y+dy);tx.isFixed=True;return tx
  rename={'MIDI':'MIDI A','CLOCK':'CLCK','SIDE':'SC'}
  for tx in list(sk.sketchTexts):
   if tx.text in rename:
    b=bounds(tx);text=rename[tx.text];h=tx.height*10;tx.deleteMe();new_text(text,(b[0]+b[2])/2,(b[1]+b[3])/2,h)
   elif tx.text in ('IN','OUT'):tx.deleteMe()
  # Only the newly requested glyphs are added. All existing sketch curves remain.
  if not sk.attributes.itemByName('TOERN','direction_icons_v1'):
   sk.isComputeDeferred=True
   for poly in spec['icon_polylines']:
    for a,b in zip(poly,poly[1:]):
     c=sk.sketchCurves.sketchLines.addByTwoPoints(pt(*a),pt(*b));c.isFixed=True
   sk.isComputeDeferred=False
   sk.attributes.add('TOERN','direction_icons_v1',json.dumps(spec['direction_icons']))
  d.computeAll();adsk.doEvents()
  after=audit()
  def cuts(data):return [c for c in data['curves'] if not c['construction'] and not c['fixed']]
  if cuts(before)!=cuts(after):raise RuntimeError('Unexpected change in cut geometry; backup available at '+backup)
  after['direction_icons']=spec['direction_icons'];after['midi_type']=spec['midi_type'];after['cut_geometry_preserved']=True
  after['original_curves_preserved']=all(c in after['curves'] for c in before['curves'] if not c['construction'])
  json.dump(after,open(os.path.join(ROOT,'label_update_result.json'),'w'),indent=2)
  # Keep all components and placements intact; only set a front camera for review.
  for s in root.sketches:s.isVisible=s.name in ('modular','modular_clearances')
  cam=app.activeViewport.camera;cam.isPerspective=False
  cam.eye=adsk.core.Point3D.create(9.125,6.425,60);cam.target=adsk.core.Point3D.create(9.125,6.425,0);cam.upVector=adsk.core.Vector3D.create(0,1,0);cam.isFitView=True
  app.activeViewport.camera=cam;adsk.doEvents();app.activeViewport.refresh();adsk.doEvents()
  app.activeViewport.saveAsImageFile(os.path.join(HOUSING,'modular_pcb.png'),1800,1300)
  if not manager.execute(manager.createFusionArchiveExportOptions(os.path.join(HOUSING,'toern_final_v3_modular.f3d'))):raise RuntimeError('Archive export failed')
  ui.messageBox('Updated current v118 modular sketch: MIDI A, CLCK, SC; IN/OUT replaced with engraved arrow/bar icons. Existing cut geometry and components preserved. Archive and preview exported.')
 except:
  try:sk.isComputeDeferred=False
  except:pass
  with open(os.path.join(ROOT,'label_error.txt'),'w') as f:f.write(traceback.format_exc())
  ui.messageBox(traceback.format_exc())
