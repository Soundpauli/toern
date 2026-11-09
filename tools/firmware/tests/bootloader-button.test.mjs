import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';

async function factoryFlash(hasSerialApi, productId = 0x0478) {
  const elements = new Map(), calls = {serial:0,hid:0,flashed:0,ramReady:0};
  const radios = ['stable','beta','beta-modulr','ramtest'].map(value=>({value,checked:value==='ramtest',disabled:false}));
  const ids = {firmwareStable:0,firmwareBeta:1,firmwareBetaModulr:2,firmwareRamTest:3};
  function element(id) {
    if (!elements.has(id)) elements.set(id,{
      ...(id in ids ? radios[ids[id]] : {}),id,hidden:false,disabled:false,textContent:id==='uploadFirmwareBtn'?'Upload RAM test':'',style:{},listeners:{},
      classList:{add(){},remove(){},toggle(){}},replaceChildren(){},scrollIntoView(){},
      addEventListener(name,fn){this.listeners[name]=fn;},dispatchEvent(event){this.listeners[event.type]?.(event);},
    });
    return elements.get(id);
  }
  const radioElements=Object.keys(ids).map(element);
  const document={getElementById:element,createTextNode:text=>({text}),querySelectorAll:()=>radioElements,querySelector:()=>radioElements.find(r=>r.checked)};
  const device={vendorId:0x16c0,productId,productName:''}; // Match the user's unnamed 16C0:0478 HalfKay device.
  const navigator={hid:{async requestDevice({filters}){assert(filters.some(f=>f.vendorId===device.vendorId&&f.productId===device.productId));++calls.hid;return[device];},async getDevices(){return[];}}};
  if(hasSerialApi)navigator.serial={async requestPort(){++calls.serial;throw new Error('Factory firmware has no serial port');}};
  const source=readFileSync(new URL('../main.js',import.meta.url),'utf8')
    .replace(/^import[\s\S]*?from\s+['"][^'"]+['"];\s*/gm,'')
    .replaceAll('import.meta.url',JSON.stringify(new URL('../main.js',import.meta.url).href));
  const context={document,navigator,URL,Uint8Array,Event,TextEncoder,TextDecoder,setTimeout,clearTimeout,performance,console:{log(){},warn(){},error(){}},TEENSY_DEVICE_FILTERS:[{vendorId:0x16c0,productId:0x0478},{vendorId:0x16c0,productId:0x0479}],
    async fetch(){return{ok:true,async json(){return{version:'test'};},async arrayBuffer(){return new Uint8Array([1,2,3]).buffer;}};},
    FirmwareFile:class {constructor(data,name,pid){assert.equal(name,'toern_ram_test.ino.hex');assert.equal(pid,productId);}async buildBlocks(){return{mainBlocks:[],loaderBlocks:[]};}},
    TeensyFlasher:class {async flashFirmware(blocks,selected,progress){assert.equal(selected,device);++calls.flashed;progress(1);}},
    SerialPortManager:class {constructor(){this.serialPort=null;}},
    mountSetupWizard(){return{busy:false,async disconnect(){},setFlashing(){},afterFlash(){}};},
    mountRamTest(){return{busy:false,async disconnect(){},setFlashing(){},afterFlash(ram){if(ram)++calls.ramReady;}};},
  };
  vm.runInNewContext(source,context);
  await new Promise(resolve=>setImmediate(resolve));
  await element('selectDeviceBtn').listeners.click();
  assert.equal(calls.hid,1);assert.equal(calls.serial,0);assert.equal(element('selectDeviceBtn').hidden,false);
  await element('uploadFirmwareBtn').listeners.click();
  assert.equal(calls.flashed,1);assert.equal(calls.serial,0);assert.equal(calls.ramReady,1);
  assert.match(element('status').textContent,/RAM-test firmware uploaded/);
}
test('programming-button factory flow flashes over HID without requesting a serial port',()=>factoryFlash(true));
test('a selected bootloader can flash even when Web Serial is unavailable',()=>factoryFlash(false));
test('legacy 0479 compatibility remains supported',()=>factoryFlash(true,0x0479));
test('the direct bootloader chooser is visible without first attempting serial reboot',()=>{
  const html=readFileSync(new URL('../index.html',import.meta.url),'utf8');
  const tag=html.match(/<button[^>]+id="selectDeviceBtn"[^>]*>/)[0];assert.doesNotMatch(tag,/\bhidden\b/);
});
