import assert from "node:assert/strict";
import { mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { createRequire } from "node:module";
import { build } from "../standalone-tools/web-standalone/node_modules/esbuild/lib/main.js";
const require = createRequire(new URL("../standalone-tools/web-standalone/package.json", import.meta.url));
const { Midi } = require("@tonejs/midi");
const dir = await mkdtemp(join(tmpdir(), "toern-midi-import-"));
try {
  const bundle = join(dir, "midi-import.mjs");
  await build({ entryPoints: [fileURLToPath(new URL("../standalone-tools/web-standalone/src/firmware/midiImport.js", import.meta.url))], outfile: bundle, bundle: true, platform: "node", format: "esm" });
  const { parseMidiFile, mapNotesToGrid, buildImportCells, filterOverlappingNotes, suggestMidiSlots, selectMidiPageWindow } = await import(pathToFileURL(bundle));
  const midi = new Midi();
  midi.header.setTempo(120);
  const track = midi.addTrack();
  track.name = "Import regression";
  for (const [i, pitch] of [0, 60, 127].entries()) track.addNote({ midi: pitch, time: i * .125, duration: .1, velocity: .8 });
  const bytes = midi.toArray();
  const parsed = await parseMidiFile({ arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength) });
  assert.equal(parsed.notes.length, 3);
  assert.equal(parsed.tracks[0].name, track.name);
  assert.equal(parsed.bpm, 120);
  for (const transpose of [true, false]) {
    const grid = mapNotesToGrid(parsed.notes, parsed.bpm, 16, 0, transpose);
    assert.equal(grid.length, 3);
    assert.deepEqual(grid.map(n => n.x), [0, 1, 2]);
    assert(grid.every(n => n.y >= 0 && n.y <= 14));
    const { cells, pages } = buildImportCells(filterOverlappingNotes(grid), new Map([[0, 8]]), transpose);
    assert.equal(cells.length, 3);
    assert.deepEqual(pages, [1]);
    assert(cells.every(n => n.channel === 8 && n.row >= 1 && n.row <= 15));
    assert.deepEqual(cells.map(n => n.midiPitch), transpose ? [255, 255, 255] : [0, 60, 127]);
  }
  // One GM drum track must split by instrument, including simultaneous hits.
  const arrangement = new Midi();
  for (const [name, program, pitch] of [["Bass",32,36], ["Piano",0,60], ["Strings",48,67], ["Distortion Guitar",42,72]]) {
    const t=arrangement.addTrack();t.name=name;t.instrument.number=program;
    t.addNote({midi:pitch,time:0,duration:.1,velocity:.8});
  }
  const drums=arrangement.addTrack();drums.channel=9;
  for (const pitch of [36,38,42,39,45,51,54]) drums.addNote({midi:pitch,time:0,duration:.1,velocity:.8});
  const raw=arrangement.toArray();
  const parts=await parseMidiFile({arrayBuffer:async()=>raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength)});
  const slots=suggestMidiSlots(parts.tracks);
  const assignments=new Map(Object.entries(slots).filter(([,id])=>id!=null).map(([ch,id])=>[id,Number(ch)]));
  for (const t of parts.tracks) {
    const ch=assignments.get(t.id);
    if(t.percussion) assert(ch>=1&&ch<=5);
    else assert(ch>=6, `melodic ${t.name} must never use a drum voice`);
    if(t.name==='Bass') assert.equal(ch,14);
    if(t.name==='Piano') assert.equal(ch,13);
    if(t.name==='Strings') assert.equal(ch,8);
    if(t.name==='Distortion Guitar') { assert.equal(t.role,'lead', 'explicit name overrides stale cello program'); assert.equal(ch,11); }
  }
  const all=mapNotesToGrid(parts.notes,120);
  assert(all.filter(n=>!n.percussion).every(n=>n.midiPitch===n.note), 'transpose defaults off');
  const imported=buildImportCells(filterOverlappingNotes(all),assignments).cells;
  assert.equal(imported.length,9, 'five simultaneous drum roles and four melodic parts survive mapping');
  assert.equal(new Set(imported.map(n=>`${n.step}:${n.row}`)).size,imported.length);
  assert(imported.filter(n=>n.channel<=5).every(n=>n.row===n.channel+1&&n.midiPitch===255), 'drum keys select sounds without detuning samples');
  const channelMap=new Map();
  const simultaneous=[];
  for (const ch of [1,2,3,4,5,6,7,8,11,13,14]) {
    channelMap.set(ch,ch);
    for (const original of [60,72]) simultaneous.push({track:ch,page:0,x:0,y:0,original,note:original,velocity:.8});
  }
  // The same drum/sample voice can receive multiple tracks; filter by voice.
  channelMap.set(99,7);
  simultaneous.push({track:99,page:0,x:0,y:0,original:80,note:80,velocity:.8});
  for(const checked of [false,true]) {
    const filtered=filterOverlappingNotes(simultaneous,'highest',channelMap,checked);
    const count=ch=>filtered.filter(n=>channelMap.get(n.track)===ch).length;
    for(let ch=1;ch<=8;ch++)assert.equal(count(ch),1,`CH${ch} must always filter`);
    assert.equal(count(13),checked?1:2);
    assert.equal(count(11),2);assert.equal(count(14),2);
    assert.equal(filtered.find(n=>channelMap.get(n.track)===7).original,80);
  }
  const octaves=mapNotesToGrid([{track:0,note:60,time:0,duration:.1,velocity:.8},{track:0,note:72,time:0,duration:.1,velocity:.8}],120,16,0,false);
  assert.equal(octaves.length,2,'absolute-pitch octaves must survive sharing a folded row');
  assert.equal(buildImportCells(filterOverlappingNotes(octaves,'highest',new Map([[0,11]]),true),new Map([[0,11]])).cells.length,2);
  const songGrid=mapNotesToGrid(Array.from({length:600},(_,i)=>({track:0,note:60,velocity:.8,time:i*.125,duration:.1})),120);
  for (const cols of [16,32]) {
    const selected=selectMidiPageWindow(songGrid,3,cols);
    assert.equal(selected.notes.length,256);
    assert.equal(selected.notes[0].time,2*cols*.125);
    assert.equal(selected.notes[0].page,0);assert.equal(selected.notes[0].x,0);
    const payload=buildImportCells(selected.notes,new Map([[0,7]])).cells;
    assert.equal(payload[0].step,1);assert.equal(payload.at(-1).step,256);
    const last=selectMidiPageWindow(songGrid,9999,cols);
    assert.equal(last.endPage,last.totalPages);
    assert.equal(last.startPage,last.maxStartPage);
    assert.equal(last.notes.at(-1).time,599*.125,'last song note is reachable');
    assert.equal(selectMidiPageWindow(songGrid,-1,cols).startPage,1);
  }
  assert.equal(selectMidiPageWindow([]).maxStartPage,1);
  assert.deepEqual(mapNotesToGrid([]), []);
  console.log("PASS: real MIDI file parses, quantizes and imports low/high pitches with transpose on/off.");
} finally {
  await rm(dir, { recursive: true, force: true });
}
