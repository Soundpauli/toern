import test from 'node:test';
import assert from 'node:assert/strict';
import { DEFAULT_ANSWERS, QUESTIONS, makeSettings, visibleQuestions, migrateAnswers, toHex, fromHex, crc32 } from '../setup-settings.js';
import { SetupConnection } from '../setup-serial.js';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, writeFileSync, rmSync, readFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
const repoRoot = fileURLToPath(new URL('../../../', import.meta.url));

// Cover every option and every pair of preference choices without an
// exponential Cartesian product as the wizard gains independent settings.
const profilesByKey = new Map();
const addProfile = a => profilesByKey.set(JSON.stringify(a), a);
addProfile({ ...DEFAULT_ANSWERS });
for (let i = 0; i < QUESTIONS.length; ++i) {
  for (const a of QUESTIONS[i].options) addProfile({ ...DEFAULT_ANSWERS, [QUESTIONS[i].id]: a.value });
  for (let j = i + 1; j < QUESTIONS.length; ++j) {
    for (const a of QUESTIONS[i].options) for (const b of QUESTIONS[j].options)
      addProfile({ ...DEFAULT_ANSWERS, [QUESTIONS[i].id]: a.value, [QUESTIONS[j].id]: b.value });
  }
}
const profiles = [...profilesByKey.values()];
const current = new Uint8Array(60); current[13] = 4;
test('every option and pairwise preference combination forms a firmware-valid EEPROM block', () => {
  const directory = mkdtempSync(join(tmpdir(), 'toern-web-setup-'));
  try {
    // Validate browser-generated blocks with the actual firmware validator.
    const rows = profiles.map(a => {
      const b = makeSettings(a, current);
      assert.equal(b.length, 60); assert.equal(b[13], 4);
      assert.equal(b[27], a.speaker === 'yes' ? 1 : 0);
      assert.equal(b[11], a.controls === 'full' ? 2 : 1);
      assert.equal(b[14], a.ctrl === 'page' ? 0 : 1);
      assert.equal(b[18], a.subpatterns === 'on' ? 1 : 0);
      assert.equal(b[3], {off:255,on:1,song:2,next:3}[a.pageMode]);
      assert.equal(b[8], a.flow === 'on' ? 1 : 255);
      assert.equal(b[19], a.cursor === 'channel' ? 1 : 0);
      assert.equal(b[37], a.fold === 'yes' ? 1 : 0);
      assert.equal(b[57], a.usbPreview === 'yes' ? 1 : 0);
      assert.equal(b[49], {off:0,'1':1,'5':2,'15':3}[a.screensaver]);
      assert.equal(b[26],64);
      assert.equal(b[0], a.input === 'line' ? 255 : 1);
      assert.equal(b[5], a.input === 'none' ? 0 : 2);
      assert.equal(b[1], ['midi', 'pulse'].includes(a.sync) ? 0 : 1);
      assert.equal(b[2], a.sync === 'lead' ? (a.transportSend === 'yes' ? 2 : 255) : a.sync === 'solo' ? 255 : 1);
      assert.equal(b[24], a.sync === 'lead' ? 0 : 1);
      assert.equal(b[52], a.sync === 'pulse' ? 1 : 0);
      assert.equal(b[28], a.keys === 'yes' ? 1 : 0);
      assert.equal(b[56], 1); assert.equal(b[54], 0);
      for (const i of [38,39,40,41]) assert.equal(b[i], 10);
      assert.deepEqual(fromHex(toHex(b)), b);
      assert.equal(visibleQuestions(a).some(q => q.id === 'pulse'), a.sync === 'pulse');
      return `{${Array.from(b).join(',')}}`;
    });
    writeFileSync(join(directory, 'test.cpp'), `#include <cassert>\n#include "toern_settings_config.h"\nint main(){const uint8_t blocks[][60]={${rows.join(',')}};for(auto &b:blocks)assert(toern_config::valid(b));}\n`);
    const flags = process.platform === 'darwin' ? ['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] : [];
    execFileSync('c++', ['-std=c++17', ...flags, '-I', join(repoRoot, 'platformio/src'), join(directory,'test.cpp'), '-o',join(directory,'test')]);
    execFileSync(join(directory,'test'));
  } finally { rmSync(directory, {recursive:true,force:true}); }
  assert.throws(() => makeSettings({...DEFAULT_ANSWERS, sync:'bad'}, current));
  assert.throws(() => makeSettings(DEFAULT_ANSWERS, new Uint8Array(59)));
  assert.equal(crc32(new TextEncoder().encode('123456789')), 'CBF43926');
});
test('CTRL and subpatterns remain independent of Easy/Full; brightness is standard',()=>{
  for(const controls of ['easy','full'])for(const ctrl of ['vol','page'])for(const subpatterns of ['off','on']){
    const b=makeSettings({...DEFAULT_ANSWERS,controls,ctrl,subpatterns,brightness:'bright'},current);
    assert.equal(b[11],controls==='full'?2:1);assert.equal(b[14],ctrl==='page'?0:1);assert.equal(b[18],subpatterns==='on'?1:0);assert.equal(b[26],64);
  }
  assert(!QUESTIONS.some(q=>q.id==='brightness'));
  for(const sync of ['solo','lead','midi','pulse'])for(const transportSend of ['yes','no']){
    const a={...DEFAULT_ANSWERS,sync,transportSend},b=makeSettings(a,current);
    assert.equal(visibleQuestions(a).some(q=>q.id==='transportSend'),sync==='lead');
    assert.equal(b[2]===2,sync==='lead'&&transportSend==='yes');
    assert.equal(b[1]===1&&b[24]!==1,sync==='lead');
  }
  for(const keys of ['yes','no'])for(const fold of ['yes','no']){
    const a={...DEFAULT_ANSWERS,keys,fold};assert.equal(visibleQuestions(a).some(q=>q.id==='fold'),keys==='yes');assert.equal(makeSettings(a,current)[37],fold==='yes'?1:0);
  }
});
test('existing wizard drafts retain choices when new questions are added',()=>{
  const old={speaker:'no',controls:'full',input:'line',sync:'lead',pulse:'32',keys:'yes',preview:'press',brightness:'bright'};
  const a=migrateAnswers(old);assert.equal(a.ctrl,'page');assert.equal(a.subpatterns,'on');assert.equal(a.speaker,'no');assert.equal(a.screensaver,'1');assert(!Object.hasOwn(a,'brightness'));assert.equal(makeSettings(a,current)[26],64);
  const custom=migrateAnswers({...a,ctrl:'vol',subpatterns:'off'});assert.equal(custom.ctrl,'vol');assert.equal(custom.subpatterns,'off');
});
function fakeSerial({ unsupported = false, corrupt = false, backup = true, busy = 0 } = {}) {
  let bytes = makeSettings(DEFAULT_ANSWERS, current), controller, open = false;
  const commands = [], listeners = new Map();
  const reply = state => {
    const data = new TextEncoder().encode(`BOOT: device log\r\nCONFIG ${JSON.stringify(state)}\r\n`);
    controller.enqueue(data.slice(0, 11)); controller.enqueue(data.slice(11));
  };
  const port = {
    async open() {
      open = true;
      this.readable = new ReadableStream({start(c){controller=c;}});
      this.writable = new WritableStream({write(data){
        const command = new TextDecoder().decode(data).trim(); commands.push(command);
        if (unsupported) { controller.enqueue(new TextEncoder().encode('ERR CMD\n')); return; }
        if (command === 'CONFIG GET') reply({ok:true,protocol:1,firmware:'test',hex:toHex(bytes),crc:crc32(bytes)});
        else if (command === 'CONFIG STOP') reply({ok:true});
        else {
          const [, , expected, hex] = command.split(' ');
          assert.equal(expected,crc32(bytes));
          if (busy-- > 0) { reply({ok:false,error:'BUSY'}); return; }
          bytes = fromHex(hex);
          const crc = crc32(bytes);
          reply({ok:true,crc,backup});
          if (corrupt) bytes[27] ^= 1;
        }
      }});
    },
    async close(){open=false;},
    addEventListener(name, callback){listeners.set(name,callback);},
    removeEventListener(name){listeners.delete(name);},
  };
  return {port,commands,listeners,get open(){return open;}, async requestPort(){return port;}};
}
test('USB setup handles split replies/logs, BUSY retry, save and independent readback', async () => {
  const fake = fakeSerial({busy:1}); const device = new SetupConnection(fake);
  await device.connect(); assert(fake.open);
  const before = await device.get(); assert.equal(before.bytes.length,60);
  const result = await device.apply(b => makeSettings({...DEFAULT_ANSWERS,speaker:'no',sync:'pulse',pulse:'32'},b));
  assert.equal(result.backup,true);
  assert.equal(fake.commands.filter(c=>c.startsWith('CONFIG SET')).length,2);
  await device.close(); assert(!fake.open); assert.equal(device.port,null);
});
test('a changed EEPROM readback never reports success', async () => {
  const fake = fakeSerial({corrupt:true}); const device = new SetupConnection(fake);
  await device.connect(); await assert.rejects(device.apply(b=>makeSettings(DEFAULT_ANSWERS,b)),/do not match/); await device.close();
});
test('SD backup failure is distinct from verified EEPROM success', async () => {
  const fake = fakeSerial({backup:false}); const device = new SetupConnection(fake);
  await device.connect(); assert.equal((await device.apply(b=>makeSettings(DEFAULT_ANSWERS,b))).backup,false); await device.close();
});
test('old firmware times out without writing any settings', async () => {
  const fake = fakeSerial({unsupported:true}); const device = new SetupConnection(fake);
  await device.connect(); await assert.rejects(device.request('GET',20),/latest Beta/);
  assert.equal(fake.commands.length,1); await device.close();
});
test('disconnect rejects pending work and permits reconnect', async () => {
  const fake = fakeSerial({unsupported:true}); const device = new SetupConnection(fake);
  await device.connect();
  const pending = assert.rejects(device.request('GET'),/disconnected/);
  fake.listeners.get('disconnect')(); await pending;
  await new Promise(resolve=>setTimeout(resolve,10));
  assert.equal(device.port,null); await device.connect(); assert(fake.open); await device.close();
});
