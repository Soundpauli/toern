import assert from "node:assert/strict";
import { transferMidiSubpatterns } from "../standalone-tools/web-standalone/src/firmware/midiSongTransfer.js";
import { decodePatternFile } from "../standalone-tools/web-standalone/src/firmware/patternFile.js";
import { ToernSdSerial, crc32Hex } from "../standalone-tools/web-standalone/src/firmware/sdSerial.js";

const cell = (step, pitch) => ({step, row:12, channel:11, velocity:89, midiPitch:pitch, probability:100});
const banks = [[cell(256,60)],[],[cell(1,72)]];
const sent = [], progress = [];
const client = {async putSubpatternRam(letter, bytes, update) {
  sent.push([letter, decodePatternFile(bytes)]);
  update(bytes.length, bytes.length);
}};
assert.deepEqual(await transferMidiSubpatterns(client,banks,123.5,(_,fraction)=>progress.push(fraction)),["B","C","D","E","A"]);
assert.deepEqual(progress,[.2,.4,.6,.8,1]);
for (const [letter, image] of sent) {
  assert.equal(image.bpm,123.5);
  const nonempty = image.cells.filter(c=>c.channel);
  assert.equal(nonempty.length,letter==="A"||letter==="C"?1:0);
  if(letter==="A") assert.equal(image.cells[255*16+11].midiPitch,60);
  if(letter==="C") assert.equal(image.cells[11].midiPitch,72);
}
let attempts=0;
await assert.rejects(transferMidiSubpatterns({async putSubpatternRam(){
  if(++attempts===3)throw new Error("unplugged");
}},banks,120),error=>error.message==="unplugged"&&error.completedSubpatterns.join()==="B,C");
assert.equal(attempts,3,'do not retry an uncertain transfer or overwrite A after failure');
await assert.rejects(transferMidiSubpatterns(client,[[cell(999,60)],[]],120),/Invalid MIDI song cell/);
assert.equal(sent.length,5,'validate all banks before sending any bytes');

// Exercise the actual Web Serial framing and old-firmware rejection path.
const serial = new ToernSdSerial();
const writes=[], responses=[];
serial._write=async data=>{
  writes.push(data);
  if(typeof data==='string') responses.push('READY','ACK','ACK','ACK','OK');
};
serial._readLine=async()=>responses.shift();
let wireProgress=0;
const bytes=new Uint8Array(20494);
await serial.putSubpatternRam('E',bytes,done=>{wireProgress=done;});
assert.equal(writes[0],`PUTSUB E 20494 ${crc32Hex(bytes)}\n`);
assert.deepEqual(writes.slice(1).map(data=>data.length),[8192,8192,4110]);
assert.equal(wireProgress,20494);
writes.length=0;
await serial.putPatternRam(bytes);
assert.equal(writes[0],`PUTPAT 20494 ${crc32Hex(bytes)}\n`);
writes.length=0;
serial._write=async data=>{writes.push(data);};
serial._readLine=async()=>"ERR UNKNOWN";
await assert.rejects(serial.putSubpatternRam('A',bytes),/ERR UNKNOWN/);
assert.equal(writes.length,1,'unsupported firmware receives no binary payload');
await assert.rejects(serial.putSubpatternRam('F',bytes),/Invalid subpattern/);
console.log('PASS: song bank order, silent/cleared banks, pitch/BPM, failure progress, legacy framing and unsupported-firmware rejection');
