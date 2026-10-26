/** Regression tests run the actual standalone SD tool code, without hardware. */
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { File } from 'node:buffer';
import vm from 'node:vm';

const base = new URL('../standalone-tools/sd-tool-standalone/', import.meta.url);
const html = readFileSync(new URL('index.html', base), 'utf8');
// Both entry points must carry the same fix, including all event handlers.
const scriptsOf = (page) => [...page.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/g)].map((m) => m[1]);
assert.deepEqual(scriptsOf(readFileSync(new URL('explorer.html', base), 'utf8')), scriptsOf(html));
for (const [, script] of html.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/g)) {
  new vm.Script(script);
}
const slice = (start, end) => html.slice(html.indexOf(start), html.indexOf(end, html.indexOf(start)));
const code = slice('const CRC_TABLE =', 'function concatBytes') +
  slice('function isHiddenName', '// --- Web Serial SD client') +
  slice('class ToernSdSerial', '// --- UI helpers') +
  slice('function normalizeRelPath', 'function showNeedSdModal');
const run = new Function('env', 'File', `
 const {state,requireSd,setBusy,showProgress,showBanner,refresh,playCache,
        prepareUploadFile,fmtXferBytes,joinPath,dbg,IO_TIMEOUT_MS,KEEPALIVE_MS=4000} = env;
 ${code}
 return {fileListToUploadItems,collectDroppedUploadItems,uploadFiles,validateUploadPath,ToernSdSerial,crc32Hex,unzipToUploadItems};
`);
const calls = [], banners = [];
const state = { connected: true, busy: false, path: '/target' };
const client = {
  _putSpaces: true,
  async mkdir(path) { calls.push(['mkdir', path]); },
  async listDir(path) { calls.push(['list', path]); return []; },
  async putBytes(path, bytes) { calls.push(['put', path, [...bytes]]); },
};
const env = {
  state, requireSd: () => client,
  setBusy: (v) => { state.busy = v; }, showProgress() {},
  showBanner: (text, ok) => banners.push([text, ok]), refresh: async () => {},
  playCache: new Map(), prepareUploadFile: async (file) => file,
  fmtXferBytes: String, joinPath: (parent, name) => parent.replace(/\/$/, '') + '/' + name,
  dbg() {}, IO_TIMEOUT_MS: 1000,
};
const tool = run(env, File);
const file = (name, text = 'data', relativePath = '') => {
  const f = new File([text], name);
  Object.defineProperty(f, 'webkitRelativePath', { value: relativePath });
  return f;
};
const fileEntry = (f) => ({ name: f.name, isFile: true, file(resolve) { resolve(f); } });
const dirEntry = (name, children) => ({
  name, isDirectory: true,
  createReader() {
    let offset = 0;
    return { readEntries(resolve) {
      const batch = children.slice(offset, offset + 100); offset += 100;
      queueMicrotask(() => resolve(batch));
    } };
  },
});

// These are the exact input shapes from Upload file(s) and Upload folder(s).
await tool.uploadFiles([file('one.bin')]);
assert.deepEqual(calls, [['put', '/target/one.bin', [...new TextEncoder().encode('data')]]]);
assert.equal(banners.at(-1)[1], true);
calls.length = 0;
await tool.uploadFiles([file('two.bin', 'x', 'Pack/Sub Folder/two.bin')]);
assert.deepEqual(calls.map((c) => c.slice(0, 2)), [
  ['mkdir', '/target/Pack'], ['mkdir', '/target/Pack/Sub Folder'],
  ['put', '/target/Pack/Sub Folder/two.bin'],
]);
assert.equal(state.busy, false);
// Already wrapped items, sparse arrays, null files, hidden folders and ancestors.
const normalized = tool.fileListToUploadItems([
  null, , { file: file('ok.bin'), relativePath: 'Top/ok.bin' },
  { type: 'directory', relativePath: 'Top/Empty' },
  file('.DS_Store'), file('secret.bin', 'x', '.hidden/secret.bin'),
]);
assert.deepEqual(normalized.map((i) => i.relativePath), ['Top', 'Top/Empty', 'Top/ok.bin']);
assert.throws(() => tool.fileListToUploadItems([{}]), /Could not read/);
assert.throws(() => tool.validateUploadPath('/' + 'é'.repeat(64)), /Path too long/);
assert.throws(() => tool.validateUploadPath('/bad\nPING'), /Invalid characters/);

// Whole folder drop: more than one readEntries batch, subfolders, empty folders.
const many = Array.from({ length: 105 }, (_, i) => fileEntry(file(`${i}.bin`)));
const root = dirEntry('Kit Folder', [
  ...many, dirEntry('Sub', [fileEntry(file('nested.bin'))]), dirEntry('Empty', []),
]);
const dropped = await tool.collectDroppedUploadItems({
  items: [{ kind: 'file', webkitGetAsEntry: () => root, getAsFile: () => null }], files: [],
});
assert.equal(dropped.filter((i) => i.type === 'file').length, 106);
assert(dropped.some((i) => i.type === 'directory' && i.relativePath === 'Kit Folder/Empty'));
assert(dropped.some((i) => i.relativePath === 'Kit Folder/Sub/nested.bin'));
calls.length = 0;
await tool.uploadFiles(dropped);
assert(calls.some((c) => c[0] === 'mkdir' && c[1] === '/target/Kit Folder/Empty'));
assert(calls.some((c) => c[0] === 'put' && c[1] === '/target/Kit Folder/Sub/nested.bin'));

// Modern API handles must all be requested before the drop event yields.
let active = true, requested = 0;
const handle = {
  kind: 'directory', name: 'Modern',
  async *values() { yield { kind: 'file', name: 'a.bin', getFile: async () => file('a.bin') }; },
};
const pending = tool.collectDroppedUploadItems({ items: [handle, handle].map((value) => ({
  kind: 'file', getAsFile() { return null; },
  getAsFileSystemHandle() { assert(active); requested++; return Promise.resolve(value); },
})), files: [] });
active = false;
const modern = await pending;
assert.equal(requested, 2);
assert.deepEqual(modern.map((i) => i.relativePath), ['Modern', 'Modern/a.bin']);

// Failed folder reads are shown in the UI and always release the busy state.
calls.length = 0;
await tool.uploadFiles({ items: [{ kind: 'file', webkitGetAsEntry: () => ({
  name: 'missing.bin', isFile: true, file(resolve, reject) { reject(new Error('Folder access denied')); },
}) }], files: [] });
assert.deepEqual(banners.at(-1), ['Folder access denied', false]);
assert.equal(state.busy, false); assert.equal(calls.length, 0);
// Existing folders can be merged, but an existing file must not masquerade as one.
const mkdir = client.mkdir;
client.mkdir = async () => { throw new Error('ERR EXISTS'); };
await tool.uploadFiles([file('a.bin', 'x', 'Existing/a.bin')]);
assert(calls.some((c) => c[0] === 'list' && c[1] === '/target/Existing'));
client.mkdir = mkdir;
// Refuse unsafe old-firmware paths before any remote mutation.
calls.length = 0; client._putSpaces = false;
await tool.uploadFiles([file('a.bin', 'x', 'Space Here/a.bin')]);
assert.match(banners.at(-1)[0], /updated TŒRN firmware/);
assert.equal(calls.length, 0); client._putSpaces = true;

// Capture the destination and reject a second upload while reading a folder.
calls.length = 0;
let releaseFile;
const reading = tool.uploadFiles({items:[{kind:'file',webkitGetAsEntry:()=>({
  name:'late.bin',isFile:true,file(resolve){releaseFile=resolve;},
})}],files:[]});
assert.equal(state.busy,true);
state.path='/elsewhere';
await tool.uploadFiles([file('ignored.bin')]);
releaseFile(file('late.bin'));
await reading;
assert.deepEqual(calls.map((c)=>c.slice(0,2)),[['put','/target/late.bin']]);
state.path='/target';

// Top-level ZIP expansion still preserves nested paths and empty directories.
function storedZip(entries) {
 const local=[],central=[]; let offset=0;
 for(const [path,bytes] of entries){
  const name=new TextEncoder().encode(path);
  const crc=parseInt(tool.crc32Hex(bytes),16);
  const a=new Uint8Array(30+name.length+bytes.length),av=new DataView(a.buffer);
  av.setUint32(0,0x04034b50,true);av.setUint32(14,crc,true);
  av.setUint32(18,bytes.length,true);av.setUint32(22,bytes.length,true);
  av.setUint16(26,name.length,true);a.set(name,30);a.set(bytes,30+name.length);
  const b=new Uint8Array(46+name.length),bv=new DataView(b.buffer);
  bv.setUint32(0,0x02014b50,true);bv.setUint32(16,crc,true);
  bv.setUint32(20,bytes.length,true);bv.setUint32(24,bytes.length,true);
  bv.setUint16(28,name.length,true);bv.setUint32(42,offset,true);b.set(name,46);
  local.push(a);central.push(b);offset+=a.length;
 }
 const end=new Uint8Array(22),ev=new DataView(end.buffer);
 ev.setUint32(0,0x06054b50,true);ev.setUint16(8,entries.length,true);
 ev.setUint16(10,entries.length,true);ev.setUint32(12,central.reduce((n,a)=>n+a.length,0),true);
 ev.setUint32(16,offset,true);
 return new File([...local,...central,end],'archive.zip');
}
calls.length=0;
await tool.uploadFiles([storedZip([
 ['Zip Folder/Empty/',new Uint8Array()],['Zip Folder/Sub/a.bin',new Uint8Array([1,2])],
])]);
assert(calls.some((c)=>c[0]==='mkdir'&&c[1]==='/target/Zip Folder/Empty'));
assert.deepEqual(calls.find((c)=>c[0]==='put'),['put','/target/Zip Folder/Sub/a.bin',[1,2]]);

// ZIP indexing reads bounded metadata, without eagerly extracting its files.
const zipFile=storedZip([['Large/data.bin',new Uint8Array(5*1024*1024)]]);
let bytesRead=0;
const archiveSource={name:zipFile.name,size:zipFile.size,slice(start,end){
 const blob=zipFile.slice(start,end);bytesRead+=blob.size;return blob;
}};
const zipItems=await tool.unzipToUploadItems(archiveSource);
assert(bytesRead<100000);assert.equal(zipItems[0].file.size,5*1024*1024);
const extracted=await zipItems[0].file.arrayBuffer();assert.equal(extracted.byteLength,5*1024*1024);
// Archive integrity is checked before file bytes reach PUT.
const damagedBytes=new Uint8Array(await zipFile.arrayBuffer());damagedBytes[30+'Large/data.bin'.length]^=1;
const badZip=await tool.unzipToUploadItems(new File([damagedBytes],'damaged.zip'));
await assert.rejects(badZip[0].file.arrayBuffer(),/Zip entry is damaged/);

// Run real CRC and ACK-framed PUT over a fake serial endpoint.
assert.equal(tool.crc32Hex(new TextEncoder().encode('123456789')), 'CBF43926');
assert.equal(tool.crc32Hex(new Uint8Array()), '00000000');
const serial = new tool.ToernSdSerial();
const writes = [], replies = ['READY', 'ACK', 'ACK', 'OK'];
serial._putSpaces = true;
serial._write = async (data) => writes.push(data);
serial._readLine = async () => { assert(replies.length); return replies.shift(); };
await serial.putBytes('/Kit Folder/Sub/1 - kick.bin', new Uint8Array(8193));
assert.match(writes[0], /^PUT \/Kit Folder\/Sub\/1 - kick.bin 8193 [0-9A-F]{8}\n$/);
assert.equal(writes[1].length, 8192); assert.equal(writes[2].length, 1);
assert.equal(replies.length, 0);
writes.length = 0; serial._putSpaces = false;
await assert.rejects(serial.putBytes('/Old Firmware/a.bin', new Uint8Array()), /updated TŒRN firmware/);
assert.equal(writes.length, 0);
serial._readLine = async () => 'OK TOERN SD v3.02 PUT_SPACES';
await serial.ping(); assert.equal(serial._putSpaces, true);
// Transfer the supplied 5.35 MB reproduction file through the real PUT framer.
const fixture=readFileSync(new URL('../standalone-tools/web-standalone/SD-CARD-CONTENT/samples/samples25/808s/roots 808.wav',import.meta.url));
const largeSerial=new tool.ToernSdSerial();largeSerial._putSpaces=true;
let outputOffset=0,stage='command',blocks=0;
largeSerial._write=async(data)=>{
 if(typeof data==='string'){
  assert.match(data,/^PUT \/samples\/roots 808.wav 5346474 028A5ADE\n$/);
  stage='ready';return;
 }
 assert.equal(stage,'data');
 assert.deepEqual(data,fixture.subarray(outputOffset,outputOffset+data.length));
 outputOffset+=data.length;blocks++;stage='ack';
};
largeSerial._readLine=async()=>{
 if(stage==='ready'){stage='data';return 'READY';}
 if(stage==='ack'){stage=outputOffset===fixture.length?'final':'data';return 'ACK';}
 assert.equal(stage,'final');stage='done';return 'OK';
};
await largeSerial.putBytes('/samples/roots 808.wav',fixture);
assert.equal(stage,'done');assert.equal(outputOffset,fixture.length);
assert.equal(blocks,Math.ceil(fixture.length/8192));

// A timeout mid-PUT retires the connection. Keepalive must not send command
// text into a device that may still be waiting for binary file data.
const stuck=new tool.ToernSdSerial();let reads=0,lost=0;
stuck._putSpaces=true;stuck._write=async()=>{};
stuck.onDisconnect=()=>lost++;
stuck._readLine=async()=>{if(reads++===0)return 'READY';throw new Error('timeout waiting for data from device');};
await assert.rejects(stuck.putBytes('/stalled.bin',new Uint8Array(3*1024*1024+1)),/0\/3145729 bytes acknowledged/);
assert.equal(lost,1);assert.equal(stuck._closed,true);
// Device-reported CRC failure is terminal/framed, rather than a binary timeout.
const badCrc=new tool.ToernSdSerial();let reply=0;
badCrc._write=async()=>{};badCrc._readLine=async()=>['READY','ACK','ERR CRC got=00000000 want=00000001'][reply++];
await assert.rejects(badCrc.putBytes('/crc.bin',new Uint8Array(1)),/ERR CRC/);
assert.equal(badCrc._closed,false);
// Browser writes themselves also have a deadline (not just ACK reads).
const fastTool=run({...env,IO_TIMEOUT_MS:5},File);
const blockedWrite=new fastTool.ToernSdSerial();
blockedWrite.writer={write:()=>new Promise(()=>{}),abort:async()=>{},releaseLock(){}};
await assert.rejects(blockedWrite._write('PING\n'),/timeout writing/);
assert.equal(blockedWrite._closed,true);
// Local conversion may keep the UI busy, while the protocol remains idle.
const savedInterval=globalThis.setInterval,savedClear=globalThis.clearInterval;
let tick, pings=0;
try{
 globalThis.setInterval=(fn)=>{tick=fn;return 1;};globalThis.clearInterval=()=>{};
 const idle=new tool.ToernSdSerial();idle.port={};idle.ping=async()=>{pings++;};
 state.busy=true;idle._startKeepalive();tick();await Promise.resolve();
 assert.equal(pings,1);
 idle._opBusy=1;tick();await Promise.resolve();assert.equal(pings,1);
 idle._stopKeepalive();state.busy=false;
}finally{globalThis.setInterval=savedInterval;globalThis.clearInterval=savedClear;}
console.log('PASS: picker/File arrays, nested and empty folder drops, >100 entries, modern handles, read failures, UTF-8 paths, firmware capability, 5.35 MB fixture, CRC/ACK framing and stalled-upload isolation.');
