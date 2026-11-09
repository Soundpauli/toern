import asyncio,json,sys
from pathlib import Path
import argparse
sys.path.insert(0,str(Path(__file__).resolve().parent))
from audit_device import Audit, ClientSession, StdioServerParameters, stdio_client
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description='Capture all device menu and filter pages through stdio MCP without file writes or resets.')
parser.add_argument('--port',required=True)
parser.add_argument('--report',type=Path,default=Path('/tmp/toern-device-pages.json'))
parser.add_argument('--focus',action='store_true',help='Temporarily turn and restore every non-INST slider on voices 1, 3, 11, 13, 14.')
args=parser.parse_args()
OUT=args.report
async def run():
 params=StdioServerParameters(command=sys.executable,args=[str(ROOT/'tools/toern-mcp/server.py')])
 async with stdio_client(params) as (r,w):
  async with ClientSession(r,w) as c:
   await c.initialize(); a=Audit(c,OUT); data={'menus':[],'filters':[],'browsers':[],'focus':{'filters':[]}}
   async def save(): OUT.write_text(json.dumps(data,indent=2))
   async def snap():
    await asyncio.sleep(.15)
    return {'state':await a.state(),'matrix':await a.call('get_matrix')}
   data['initial']=await a.call('connect',port=args.port)
   try:
    if data['initial']['mode']!='DRAW' or data['initial']['playing']:
     raise RuntimeError('Start this navigation capture in paused DRAW mode.')
    for group in ['ROOT','VOL','SETT','REC','MIDI','MODE','ETC','ADV']:
     menu=await (a.root() if group=='ROOT' else a.visit_group(group))
     for item in menu['items']:
      await a.position(4,item['index'])
      entry={'group':group,'item':item,**await snap()}
      data['menus'].append(entry);await save();print(group,item['name'],flush=True)
    for name in ['PATT','KIT','SMP','BPM']:
     root=await a.root(); item=next(i for i in root['items'] if i['name']==name)
     await a.position(4,item['index']);await a.press('button',4)
     data['browsers'].append({'name':name,**await snap()});await save()
    for voice in [1,2,3,4,5,6,7,8,11,13,14]:
     await a.draw();await a.position(1,voice+1);await a.press('button',3,500)
     meta=await a.call('get_filters',voice=voice)
     entry={'voice':voice,'metadata':meta,'pages':[]}
     for _ in meta['pages']:
      state=await a.state(); page=state['filter_page']
      await asyncio.sleep(1.05)
      entry['pages'].append({'page':page,**await snap()})
      if args.focus and voice in [1,3,11,13,14]:
       focus=data['focus']['filters']
       if not focus or focus[-1]['voice']!=voice:focus.append({'voice':voice,'original':meta,'changes':[]})
       for spec in meta['pages'][page-1]['sliders']:
        if spec['name']=='INST':continue
        raw=spec['value'];delta=-1 if raw>=spec['max'] else 1
        try:
         state=await a.turn(spec['encoder'],delta)
         focus[-1]['changes'].append({'page':page,'encoder':spec['encoder'],'before':raw,'delta':delta,'state':state,'metadata':await a.call('get_filters',voice=voice),'matrix':await a.call('get_matrix')})
        finally:await a.turn(spec['encoder'],-delta)
      await a.press('touch',2)
     restored=await a.call('get_filters',voice=voice)
     assert restored['pages']==meta['pages'],('Filter values changed during capture',voice)
     data['filters'].append(entry);await save();print('FILTER',voice,flush=True)
    await a.draw()
    # Restore initial cursor and return mode; no sample/kit/pattern or reset actions.
    await a.position(1,data['initial']['positions'][0]);await a.position(4,data['initial']['positions'][3])
    data['final']=await a.state();await save()
   finally: await a.call('disconnect')
if __name__=='__main__':
 asyncio.run(run())
