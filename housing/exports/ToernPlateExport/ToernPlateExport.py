import adsk.core, adsk.fusion, json, traceback, os
OUT='/Users/jank/git/toern/housing/exports/v108_single_source'
def pt(p): return [p.x*10,p.y*10,p.z*10]
def geom(g):
    t=g.objectType.split('::')[-1]
    d={'type':t}
    if t=='Line3D': d.update(start=pt(g.startPoint),end=pt(g.endPoint))
    elif t=='Circle3D': d.update(center=pt(g.center),radius=g.radius*10,normal=[g.normal.x,g.normal.y,g.normal.z])
    elif t=='Arc3D':
        d.update(center=pt(g.center),radius=g.radius*10,start=pt(g.startPoint),end=pt(g.endPoint),normal=[g.normal.x,g.normal.y,g.normal.z])
        ev=g.evaluator; ok,a,b=ev.getParameterExtents(); ok,mid=ev.getPointAtParameter((a+b)/2); d['mid']=pt(mid)
    else:
        ev=g.evaluator; ok,a,b=ev.getParameterExtents(); ok,pts=ev.getStrokes(a,b,0.0001)
        if not ok: raise RuntimeError('Curve sampling failed')
        d['points']=[pt(p) for p in pts]
    return d
def run(context):
    app=adsk.core.Application.get(); ui=app.userInterface
    try:
        os.makedirs(OUT,exist_ok=True)
        design=adsk.fusion.Design.cast(app.activeProduct)
        result={'document':app.activeDocument.name,'sketches':[]}
        for sk in design.rootComponent.sketches:
            if sk.name.upper() not in ('COMMON','16_16','32_16'): continue
            rec={'name':sk.name,'curves':[],'texts':[]}
            for c in sk.sketchCurves:
                d=geom(c.geometry); d.update(fixed=c.isFixed,construction=c.isConstruction); rec['curves'].append(d)
            for tx in sk.sketchTexts:
                rec['texts'].append({'text':tx.text,'curves':[geom(g) for g in tx.asCurves()]})
            sk.saveAsDXF(os.path.join(OUT,sk.name+'_v108_raw.dxf'))
            result['sketches'].append(rec)
        with open(os.path.join(OUT,'fusion_geometry.json'),'w') as f: json.dump(result,f,indent=2)
        ui.messageBox('COMMON, 16_16 und 32_16 exportiert. Original unverändert.')
    except:
        with open(os.path.join(OUT,'export_error.txt'),'w') as f: f.write(traceback.format_exc())
        ui.messageBox(traceback.format_exc())
