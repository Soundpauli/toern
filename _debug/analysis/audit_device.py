"""Hardware audit through the real stdio MCP server.

Mutates parameters temporarily and resets all 11 playable voices to defaults.
Does not back up, delete files, load kits, or execute global reset/generation.
Run with the MCP requirements installed: python audit_device.py --port PORT.
"""
from __future__ import annotations
import argparse
import asyncio
import json
from pathlib import Path
import sys
import time
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client


class Audit:
    def __init__(self, client, report):
        self.client = client
        self.report = report
        self.results = []

    async def call(self, name, **args):
        result = await self.client.call_tool(name, args)
        if result.isError:
            raise RuntimeError(f'{name}({args}): {result.content}')
        return result.structuredContent or json.loads(result.content[0].text)

    async def state(self):
        return await self.call('get_state')

    async def press(self, kind, index, hold=120):
        return await self.call('press_control', kind=kind, index=index, hold_ms=hold)

    async def turn(self, index, delta):
        state = await self.state()
        while delta:
            step = max(-64, min(64, delta))
            state = await self.call('turn_encoder', encoder=index, steps=step)
            delta -= step
        return state

    async def position(self, index, target):
        state = await self.state()
        return await self.turn(index, target-state['positions'][index-1])

    async def record(self, name, action):
        started = time.monotonic()
        try:
            detail = await action()
            result = dict(name=name, status='pass', detail=detail)
        except Exception as error:
            result = dict(name=name, status='fail', error=str(error))
        result['seconds'] = round(time.monotonic()-started, 3)
        self.results.append(result)
        self.report.write_text(json.dumps({'cases': self.results}, indent=2))
        print(result['status'].upper(), name, result.get('error', ''), flush=True)
        return result

    async def draw(self):
        state = await self.state()
        for _ in range(4):
            if state['mode']=='DRAW':
                return state
            # Touch 1 exits FILTERMODE and toggles SINGLE; touch 2 exits menus,
            # browsers and SONG without firing encoder action buttons.
            state = await self.press('touch', 1 if state['mode'] in ('FILTERMODE','SINGLE') else 2)
        raise AssertionError(f'Cannot return to DRAW: {state}')

    async def single_test(self):
        await self.draw()
        await self.position(1, 2)
        await self.position(4, 256)
        state = await self.press('touch', 1)
        assert state['mode']=='SINGLE' and state['voice']==1, state
        await self.position(1, 14)
        state = await self.state()
        assert state['mode']=='SINGLE' and state['voice']==1 and state['cursor']['y']==14, state
        assert state['cell']['voice']==0, 'Test cell is occupied; refusing to replace it'
        state = await self.press('button', 4)
        assert state['cell']['voice']==1, state
        state = await self.press('button', 1)
        assert state['cell']['voice']==0, state
        # Menu exit must retain SINGLE, not silently switch to DRAW.
        state = await self.press('touch', 2)
        assert state['mode']=='MENU', state
        state = await self.press('touch', 2)
        assert state['mode']=='SINGLE' and state['voice']==1, state
        await self.draw()
        return {'pitch_row':14,'voice':1,'step':256,'paint_erase':True,'menu_return':'SINGLE'}

    async def filter_voice(self, voice):
        await self.draw()
        await self.position(1, voice+1)
        state = await self.press('button', 3, 500)
        assert state['mode']=='FILTERMODE' and state['voice']==voice, state
        data = await self.call('get_filters', voice=voice)
        count = len(data['pages'])
        for _ in range(count):
            if data['active_page']==1:
                break
            await self.press('touch', 2)
            data = await self.call('get_filters', voice=voice)
        assert data['active_page']==1, data
        pages = []
        for page in range(1, count+1):
            data = await self.call('get_filters', voice=voice)
            assert data['active_page']==page, (voice,page,data)
            sliders = data['pages'][page-1]['sliders']
            checked = []
            for slider in sliders:
                if slider['max']==0:
                    continue
                enc = slider['encoder']
                # Changing/restoring INST applies a preset to its sibling sliders.
                # Always read a fresh value before testing the next physical knob.
                fresh = await self.call('get_filters', voice=voice)
                slider = next(s for s in fresh['pages'][page-1]['sliders'] if s['encoder']==enc)
                before = slider['value']
                delta = -1 if before>=slider['max'] else 1
                await self.turn(enc, delta)
                changed = await self.call('get_filters', voice=voice)
                value = next(s['value'] for s in changed['pages'][page-1]['sliders'] if s['encoder']==enc)
                assert value==before+delta, (voice,page,slider,value)
                await self.turn(enc, before-value)
                restored = await self.call('get_filters', voice=voice)
                value = next(s['value'] for s in restored['pages'][page-1]['sliders'] if s['encoder']==enc)
                assert value==before, (voice,page,slider,value)
                checked.append(slider['name'])
            reset = await self.call('reset_filters', scope='page')
            mismatches = [s for s in reset['pages'][page-1]['sliders'] if s['value']!=s['default']]
            assert not mismatches, ('page_reset',voice,page,mismatches)
            pages.append({'page':page,'sliders':checked,'page_reset':True})
            await self.press('touch', 2)
        data = await self.call('get_filters', voice=voice)
        assert data['active_page']==1, ('wrap',voice,data)
        # Perturb a page-1 parameter before whole-voice reset so it proves change.
        first = data['pages'][0]['sliders'][0]
        await self.turn(first['encoder'], -1 if first['value']>=first['max'] else 1)
        reset = await self.call('reset_filters', scope='voice')
        mismatches = [(p['page'],s) for p in reset['pages'] for s in p['sliders'] if s['value']!=s['default']]
        assert not mismatches, ('voice_reset',voice,mismatches)
        await self.draw()
        return {'voice':voice,'pages':pages,'voice_reset':True}

    async def root(self):
        await self.draw()
        state = await self.state()
        # Avoid y=1 transport behavior when using touch 1 elsewhere.
        await self.position(1, 2)
        await self.press('touch', 2)
        menu = await self.call('get_menu')
        assert menu['active'] and menu['group']=='ROOT', menu
        return menu

    async def visit_group(self, group):
        root = await self.root()
        parent = 'ETC' if group=='ADV' else group
        target = next(item for item in root['items'] if item['name']==parent)
        await self.position(4, target['index'])
        await self.press('button', 4)
        menu = await self.call('get_menu')
        assert menu['group']==parent, menu
        if group=='ADV':
            item = next(item for item in menu['items'] if item['name']=='ADV')
            await self.position(4, item['index'])
            await self.press('button', 4)
            menu = await self.call('get_menu')
        assert menu['group']==group, menu
        return menu

    async def menu_item(self, group, item):
        await self.position(4,item['index'])
        state = await self.state()
        menu = await self.call('get_menu')
        assert state['mode']=='MENU' and state['menu_setting']==item['setting'], (item,state)
        assert menu['active'] and menu['group']==group and menu['name']==item['name'], (item,menu)
        matrix = await self.call('get_matrix')
        assert matrix['pixels'], (group,item,'empty display')
        return {'setting':item['setting'],'index':menu['index'],'lit_pixels':len(matrix['pixels']),
                'scope':'navigation_and_display'}

    async def setting_item(self, group, item):
        navigation = await self.menu_item(group, item)
        # Read-only screens and navigation/action launchers have no editable
        # scalar here. CHLD intentionally rewrites every mute mask when enabled.
        if item['setting'] in [39,49,42,62,66,48]:
            return {**navigation,'scope':'navigation_only',
                    'reason':'read-only/action launcher' if item['setting']!=48 else 'enabling child mode rewrites mute masks'}
        controls = [1,2,3] if item['setting'] in [45,50,55,59,67,47,15,53] else [3]
        changed = []
        fixed = []
        for encoder in controls:
            before = await self.state()
            initial = before['positions'][encoder-1]
            after = await self.turn(encoder, 1)
            value = after['positions'][encoder-1]
            if value==initial:
                after = await self.turn(encoder, -1)
                value = after['positions'][encoder-1]
            if value==initial:
                fixed.append(encoder)
                continue
            assert after['mode']=='MENU' and after['menu_setting']==item['setting'], (item,after)
            await self.turn(encoder, initial-value)
            restored = await self.state()
            assert restored['positions'][encoder-1]==initial, (group,item,encoder,initial,restored)
            changed.append({'encoder':encoder,'from':initial,'to':value,'restored':True})
        return {**navigation,'scope':'reversible_encoder_adjustments','controls':changed,'fixed':fixed}

    async def demo_test(self):
        menu = await self.visit_group('MODE')
        item = next(i for i in menu['items'] if i['name']=='DEMO')
        await self.position(4,item['index'])
        state = await self.press('button',4)
        assert state['demo_active'], state
        matrix = await self.call('get_matrix')
        assert matrix['pixels'], matrix
        state = await self.press('touch',1)
        assert not state['demo_active'] and state['mode']=='MENU', state
        await self.draw()
        return {'launched':True,'serial_alive':True,'touch_exit':True}

    async def browser(self, rootname, mode):
        root = await self.root()
        item = next(i for i in root['items'] if i['name']==rootname)
        await self.position(4,item['index'])
        state = await self.press('button',4)
        assert state['mode']==mode, (rootname,state)
        matrix = await self.call('get_matrix')
        assert matrix['pixels'], (rootname,'empty screen')
        state = await self.press('touch',2)
        assert state['mode']=='DRAW', state
        return {'entered':mode,'exit':'DRAW','scope':'open_and_close_no_file_actions'}

    async def preset_test(self, voice):
        await self.draw()
        await self.position(1,voice+1)
        await self.press('touch',2)
        await self.position(4,2)  # Root SMP opens PSET on synth voices.
        state = await self.press('button',4)
        assert state['mode']=='LOADSAVE_PRESET' and state['voice']==voice, state
        initial = state['positions'][3]
        state = await self.turn(4,1 if initial<99 else -1)
        assert state['positions'][3]!=initial, state
        await self.position(4,initial)
        state = await self.press('touch',2)
        assert state['mode']=='DRAW', state
        return {'voice':voice,'browser':'LOADSAVE_PRESET','slot_browse':True,'file_actions':False}

    async def song_test(self):
        menu = await self.visit_group('MODE')
        item = next(i for i in menu['items'] if i['name']=='SONG')
        await self.position(4,item['index'])
        state = await self.press('button',4)
        assert state['mode']=='SONGMODE',state
        initial = state['positions'][3]
        state = await self.turn(4,1 if initial<64 else -1)
        assert state['positions'][3]!=initial,state
        await self.position(4,initial)
        state = await self.press('touch',2)
        assert state['mode']=='DRAW',state
        return {'browser':'SONGMODE','position_browse':True,'playback_not_started':True}

    async def reset_options_test(self):
        menu = await self.visit_group('ETC')
        item = next(i for i in menu['items'] if i['name']=='RSET')
        await self.position(4,item['index'])
        initial = (await self.state())['positions'][2]
        for target in range(8):
            state = await self.position(3,target)
            assert state['mode']=='MENU' and state['menu_setting']==16 and state['positions'][2]==target,state
        await self.position(3,initial)
        await self.draw()
        return {'options':['SD','EFX','FULL','PATT','KIT','ASAV','RAM','RVRB'],'scope':'selector_only_no_apply'}

    async def single_filter_test(self):
        await self.draw()
        await self.position(1,3)
        state = await self.press('touch',1)
        assert state['mode']=='SINGLE' and state['voice']==2,state
        await self.position(1,7)
        state = await self.press('button',3,500)
        assert state['mode']=='FILTERMODE' and state['voice']==2,state
        before = await self.call('get_filters')
        await self.press('touch',2)
        after = await self.call('get_filters')
        assert after['active_page'] == before['active_page']%len(before['pages'])+1,after
        await self.call('reset_filters',scope='page')
        state = await self.press('touch',1)
        assert state['mode']=='DRAW',state
        return {'voice':2,'pitched_row':7,'filter_voice_preserved':True,'page_cycle_reset':True,'exit':'DRAW (panel behavior)'}

    async def reset_voice(self, voice):
        await self.draw()
        await self.position(1,voice+1)
        state = await self.press('button',3,500)
        assert state['mode']=='FILTERMODE' and state['voice']==voice,state
        reset = await self.call('reset_filters',scope='voice')
        diff = [(p['page'],s) for p in reset['pages'] for s in p['sliders'] if s['value']!=s['default']]
        assert not diff,diff
        await self.draw()
        return {'voice':voice,'all_defaults_verified':True}

    async def run(self, port, section="all", voice=None):
        await self.call('connect',port=port)
        if section=='reset':
            for ch in [1,2,3,4,5,6,7,8,11,13,14]:
                await self.record(f'Final defaults voice {ch}',lambda ch=ch:self.reset_voice(ch))
            await self.position(1,2)
            await self.position(4,1)
            self.report.write_text(json.dumps({'cases':self.results,'final_state':await self.state()},indent=2))
            await self.call('disconnect')
            return
        if section=='special':
            await self.record('SINGLE: selected voice through filter entry/page/reset/exit',self.single_filter_test)
            for ch in [11,13,14]:
                await self.record(f'PSET voice {ch}: open, browse, close',lambda ch=ch:self.preset_test(ch))
            await self.record('SONG: open, position, close',self.song_test)
            await self.record('RSET: all eight selectors without applying',self.reset_options_test)
            await self.draw()
            await self.call('disconnect')
            return
        if section=='all':
            await self.record('SINGLE: enter, pitch, paint, erase, menu return',self.single_test)
        if section in ('all','filters'):
            for ch in ([voice] if voice else [1,2,3,4,5,6,7,8,11,13,14]):
                await self.record(f'FILTER voice {ch}: all sliders, pages, page/voice resets', lambda ch=ch:self.filter_voice(ch))
        if section=='filters':
            await self.draw()
            await self.call('disconnect')
            return
        root = await self.root()
        for item in (root['items'] if section!='settings' else []):
            await self.record(f'ROOT/{item["name"]}', lambda item=item:self.menu_item('ROOT',item))
        for name,mode in ([('PATT','LOADSAVE_TRACK'),('KIT','SET_SAMPLEPACK'),('SMP','SET_WAV'),('BPM','BPM')] if section!='settings' else []):
            await self.record(f'OPEN/{name}',lambda name=name,mode=mode:self.browser(name,mode))
        for group in ['VOL','SETT','REC','MIDI','MODE','ETC','ADV']:
            try:
                menu = await self.visit_group(group)
            except Exception as error:
                async def fail(error=error): raise error
                await self.record(f'{group}/enter',fail)
                continue
            for item in menu['items']:
                await self.record(f'{group}/{item["name"]}',lambda item=item,group=group:(self.setting_item(group,item) if section in ('all','settings') else self.menu_item(group,item)))
        if section in ('all','settings'):
            await self.record('MODE/DEMO launch and remote exit',self.demo_test)
        if section=='all':
            await self.record('SINGLE: selected voice through filter entry/page/reset/exit',self.single_filter_test)
            for ch in [11,13,14]:
                await self.record(f'PSET voice {ch}: open, browse, close',lambda ch=ch:self.preset_test(ch))
            await self.record('SONG: open, position, close',self.song_test)
            await self.record('RSET: all eight selectors without applying',self.reset_options_test)
            for ch in [1,2,3,4,5,6,7,8,11,13,14]:
                await self.record(f'Final defaults voice {ch}',lambda ch=ch:self.reset_voice(ch))
        await self.draw()
        await self.position(1,2)
        await self.position(4,1)
        self.results.append({'name':'Global wipes, file operations, recording and generation','status':'not_executed',
                             'detail':'Menus inspected; destructive/irreversible actions deliberately not fired.'})
        self.report.write_text(json.dumps({'cases':self.results,'final_state':await self.state()},indent=2))
        await self.call('disconnect')
        counts = {status:sum(r['status']==status for r in self.results) for status in ['pass','fail','not_executed']}
        print('SUMMARY',json.dumps(counts),flush=True)


async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',required=True)
    parser.add_argument('--report',type=Path,default=Path('/tmp/toern-menu-audit.json'))
    parser.add_argument('--section',choices=['all','menus','filters','settings','special','reset'],default='all')
    parser.add_argument('--voice',type=int,choices=[1,2,3,4,5,6,7,8,11,13,14])
    args = parser.parse_args()
    params = StdioServerParameters(command=sys.executable,args=[str(Path(__file__).resolve().parents[2] / 'tools/toern-mcp/server.py')])
    async with stdio_client(params) as (read,write):
        async with ClientSession(read,write) as client:
            await client.initialize()
            await Audit(client,args.report).run(args.port,args.section,args.voice)


if __name__=='__main__':
    asyncio.run(main())
