import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, writeFileSync, readFileSync, rmSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { RAM_BYTES, RAM_STAGES, validateRamStatus, ramResultText } from '../ram-test.js';
import { SetupConnection } from '../setup-serial.js';
import { FirmwareFile } from '../Teensy-Loader.js';
const here = fileURLToPath(new URL('../', import.meta.url));
const status = (overrides={}) => ({ok:true,protocol:1,device:'toern-psram-test',state:'idle',run:0,detectedBytes:RAM_BYTES,targetBytes:RAM_BYTES,testedBytes:0,stage:0,stages:RAM_STAGES,progress:0,elapsedMs:0,offset:0,expected:0,actual:0,...overrides});
const pass = status({state:'pass',run:1,testedBytes:RAM_BYTES,stage:RAM_STAGES,progress:100});
test('PASS requires 16 MB detected and tested and all 26 stages complete',()=>{
  assert.equal(validateRamStatus(pass),pass);assert.match(ramResultText(pass),/^PASS/);
  for (const patch of [{detectedBytes:8*1048576},{testedBytes:8*1048576},{stage:25},{progress:99},{run:0},{stages:25},{device:'instrument'},{state:'success'}]) assert.throws(()=>validateRamStatus({...pass,...patch}));
  for(const state of ['idle','running','missing','fail','aborted']) assert.doesNotMatch(ramResultText(validateRamStatus(status({state}))),/^PASS/);
  assert.match(ramResultText(status({state:'missing',detectedBytes:8*1048576})),/8 of 16 MB detected/);
  assert.match(ramResultText(status({state:'idle',detectedBytes:0})),/0 of 16 MB detected/);
  assert.match(ramResultText(status({state:'fail',offset:RAM_BYTES-4,expected:1,actual:0})),/0x70FFFFFC/);
});
test('actual memory engine covers full 16 MB, evicts cache blocks and catches faults',()=>{
  const directory=mkdtempSync(join(tmpdir(),'toern-ram-test-'));
  try {
    const src=String.raw`
#include <cassert>
#include <vector>
#include <algorithm>
#include "memory_test.h"
using namespace toern_ramtest;
constexpr uint32_t words=targetBytes/4;
struct Memory {
 std::vector<uint32_t> data=std::vector<uint32_t>(words,0x12345678);
 uint32_t first=0,count=0,mode=0,fault=words-1,phase=0;
 uint64_t readCount[totalStages]{},writeCount[totalStages]{};
 bool heads[totalStages]{},tails[totalStages]{};uint32_t prepares=0,finishes=0;
 void prepare(uint32_t a,uint32_t n){assert(!count&&a%1024==0&&n==1024&&a+n<=words);first=a;count=n;++prepares;}
 void finish(uint32_t a,uint32_t n){assert(a==first&&n==count);count=0;++finishes;}
 void check(uint32_t i){assert(count&&i>=first&&i<first+count&&phase<totalStages);if(i==0)heads[phase]=true;if(i==words-1)tails[phase]=true;}
 uint32_t read(uint32_t i){check(i);++readCount[phase];uint32_t v=data[mode==1?i%(words/2):i];return mode==2&&i==fault ? v^1u:v;}
 void write(uint32_t i,uint32_t v){check(i);++writeCount[phase];uint32_t &cell=data[mode==1?i%(words/2):i];if(mode==3&&i==fault)v&=~1u;cell=v;}
};
void run(Test<Memory> &t){uint32_t previous=0;while(t.state==State::Running){t.memory.phase=t.stage;t.tick();assert(t.progress()>=previous&&t.progress()<=100);previous=t.progress();}assert(t.memory.prepares==t.memory.finishes);}
int main(){
 Memory memory;Test<Memory> good(memory);good.start(targetBytes);run(good);assert(good.state==State::Pass&&good.stage==totalStages&&good.progress()==100);
 for(uint32_t s=0;s<totalStages;++s){assert(memory.heads[s]&&memory.tails[s]);bool reads=s!=0&&(s<6||(s&1));bool writes=s<5||(s>=6&&!(s&1));assert(memory.readCount[s]==(reads?words:0));assert(memory.writeCount[s]==(writes?words:0));}
 for(uint32_t bytes:{0u,8u*1048576u,32u*1048576u}){Memory m;Test<Memory> t(m);t.start(bytes);t.tick();assert(t.state==State::Missing&&m.prepares==0);}
 // Aliased 8 MB that falsely identifies as 16 MB must fail.
 {Memory m;m.mode=1;Test<Memory> t(m);t.start(targetBytes);run(t);assert(t.state==State::Fail);}
 // Tail word and second-chip faults prove complete coverage, not a sample test.
 for(uint32_t i:{words-1,words/2+1234}){Memory m;m.mode=2;m.fault=i;Test<Memory> t(m);t.start(targetBytes);run(t);assert(t.state==State::Fail&&t.failure.offset==i*4&&t.failure.expected!=t.failure.actual);}
 {Memory m;m.mode=3;m.fault=words/2;Test<Memory> t(m);t.start(targetBytes);run(t);assert(t.state==State::Fail);}
 {Memory m;Test<Memory> t(m);t.start(targetBytes);t.tick();t.abort();assert(t.state==State::Aborted&&t.progress()<100);t.start(targetBytes);assert(t.run==2&&t.stage==0&&t.cursor==0);}
}
`;
    writeFileSync(join(directory,'test.cpp'),src);
    const flags=process.platform==='darwin'?['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1']:[];
    execFileSync('c++',['-std=c++17','-O2',...flags,'-I',join(here,'ram-test/src'),join(directory,'test.cpp'),'-o',join(directory,'test')]);
    execFileSync(join(directory,'test'));
  } finally {rmSync(directory,{recursive:true,force:true});}
});
test('compiled diagnostic HEX loads through the same flasher without EEPROM data',async()=>{
  const data=readFileSync(join(here,'public/toern_ram_test.ino.hex'));
  const blocks=await new FirmwareFile(data,'toern_ram_test.ino.hex',0x0478).buildBlocks();
  assert(blocks.mainBlocks.length>0);assert.equal(blocks.loaderBlocks.length,0);
  for(const block of blocks.mainBlocks)assert(block.address>=0x60000000&&block.address<0x60040000);
  const firmware=Buffer.concat(blocks.mainBlocks.map(b=>Buffer.from(b.data)));
  assert(firmware.includes(Buffer.from('RAMTEST ')));
  // This standalone project has no EEPROM/SD libraries or EXTMEM allocations.
  assert.doesNotMatch(readFileSync(join(here,'ram-test/src/main.cpp'),'utf8'),/#include\s*[<"](?:EEPROM|SD)/);
});
test('RAM USB prefix receives unsolicited progress as well as command replies',async()=>{
  let controller;const messages=[],commands=[];
  const emit=value=>controller.enqueue(new TextEncoder().encode(`RAMTEST ${JSON.stringify(value)}\n`));
  const port={async open(){this.readable=new ReadableStream({start(c){controller=c;}});this.writable=new WritableStream({write(bytes){const command=new TextDecoder().decode(bytes);commands.push(command);if(command.includes('START')){emit({...pass,reply:false});emit(status({reply:true,state:'running',run:2}));}else emit(status({reply:true}));}});},async close(){},addEventListener(){},removeEventListener(){}};
  const serial=new SetupConnection({async requestPort(){return port;}},{prefix:'RAMTEST',replyFilter:s=>s.reply===true});serial.onMessage=s=>messages.push(validateRamStatus(s));
  await serial.connect();await serial.request('STATUS');
  emit(status({state:'running',run:1,stage:4,progress:16}));await new Promise(resolve=>setTimeout(resolve,5));
  assert.equal(commands[0],'RAMTEST STATUS\n');assert.equal(messages.at(-1).state,'running');
  const started=await serial.request('START');assert.equal(started.state,'running');assert.equal(started.run,2); // An old PASS broadcast cannot fulfill START.
  await serial.close();
});
