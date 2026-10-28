const fs = require('fs'), vm = require('vm'), assert = require('assert'), path = require('path');
const code = fs.readFileSync(path.join(__dirname, '../handbook/handbook-led.js'), 'utf8')
  .replace(/\}\)\(\);\s*$/, 'window.qa = { drawLoadSaveLike, Mtx, scenes };})();');
const window = {HBLayout:{width:32},addEventListener(){},matchMedia(){return {matches:true}}};
const document = {readyState:'loading',addEventListener(){},querySelector(){return null}};
vm.runInNewContext(code,{window,document,Uint8ClampedArray,Math,performance:{now(){return 0}}});
const {drawLoadSaveLike,Mtx,scenes} = window.qa;
for (const width of [16,32]) {
 window.HBLayout.width=width;
 for(const name of ['pattern_del_question','pattern_del_confirm','pattern_save_question','pattern_save_confirm']) {
  const m=new Mtx({getContext(){return {}}});scenes[name](m);
  let command=0,choice=0;
  for(let y=1;y<=16;y++)for(let x=1;x<=width;x++) {
   const at=((y-1)*width+x-1)*4;
   if(y>=11&&y<=15&&m.data[at]){assert(m.data[at+1]===0&&m.data[at+2]===0);command++;}
   if(y>=5&&y<=9&&(m.data[at+1]||m.data[at+2])){assert(m.data[at]===0);if(name.endsWith('_confirm'))assert(m.data[at+1]>0&&m.data[at+2]===0);else assert(m.data[at+2]>0&&m.data[at+1]===0);choice++;}
  }
  assert(command>0&&choice>0);
 }
 const neutral=new Mtx({getContext(){return {}}});scenes.canceled(neutral);
 const cx=Math.floor((width+1)/2);
 for(let x=cx-1;x<=cx+1;x++)assert(neutral.data[((7-1)*width+x-1)*4+2]===220);
 for (const exists of [true,false]) for (const letters of ['', 'A', 'CD', 'ABCDE']) {
  const m=new Mtx({getContext(){return {}}});drawLoadSaveLike(m,42,exists,true,letters,true);
  const rgb=(x,y)=>Array.from(m.data.slice(((y-1)*width+x-1)*4,((y-1)*width+x-1)*4+3));
  const start=width===32?10:5;
  for(let i=0;i<5;i++)assert.deepStrictEqual(rgb(start+2*i,7),i<letters.length?(exists?[0,120,0]:[120,0,0]):[0,0,0]);
  assert.deepStrictEqual(rgb(width===32?2:1,8),exists?[0,120,0]:[40,0,0]);
  if(exists){
   const center=width===32?20:11;
   for(let y=2;y<=6;y++)for(let x=center-2;x<=center+2;x++){
    const half=y===6?2:y===4?1:y===2?0:-1;
    const on=half>=0&&Math.abs(x-center)<=half;
    assert.deepStrictEqual(rgb(x,y),on?[120,0,0]:[0,0,0],'red 5–3–1 triangle with blank rows');
   }
  }
 }
}
console.log('PASS: PATT folders and 0–5 selected/save-source dots on both matrix widths.');
