"""Exercise actual firmware/SIM filename layout at viewport boundaries."""
from pathlib import Path
import json
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root/'platformio/toern_sample.ino').read_text()
start = source.index('  auto textPixelWidth =', source.index('static int scrollSelectionOffset'))
end = source.index('\n  {\n    int currentBrowsePos', start)
render = source[start:end]
font_uri = (root/'tools/sim/src/firmware/font.js').as_uri()
alphabet = json.loads(subprocess.check_output([
    'node', '--input-type=module', '-e',
    f'import {{ALPHABET}} from {json.dumps(font_uri)}; console.log(JSON.stringify(ALPHABET));'
], text=True))
font_cpp = ','.join('{'+','.join(map(str, glyph))+'}' for glyph in alphabet)
code = r'''
#include <algorithm>
#include <cassert>
#include <cstring>
using std::max;
struct CRGB {};
unsigned long now=0;
unsigned long millis(){return now;}
int drawnX=0;
void drawText(const char*,int x,int,CRGB){drawnX=x;}
int maxX=16;
int alphabet[95][4]={FONT};
int main(){
RENDER
 for(int columns : {16,32}) for(bool loaded : {false,true}) {
  maxX=columns;
  const int available=columns-(loaded?4:0);
  // i=1 pixel, M=3 pixels: cover variable widths, exact fits and short overflows.
  for(const char* name : {"i","iiiii","MMMM","iiiiiiii","MMMMMMMM","MMMMMMMMM","MMij","MMMij","MMMMMMij","MMMMMMMij"}) {
   int offset=0,dir=1;unsigned long next=300;
   const int width=textPixelWidth(name);
   bool moved=false;
   for(now=0;now<=6000;now+=50){
    renderLine(name,12,CRGB{},offset,dir,next,loaded);
    moved |= drawnX!=1;
    if(width<=available){assert(drawnX==1);assert(offset==0);}
    else assert(offset>=0&&offset<=width-available);
   }
   assert(moved==(width>available));
  }
  // A formerly scrolling label must reset immediately when the field grows.
  int offset=8,dir=-1;unsigned long next=0;
  renderLine("iiiii",12,CRGB{},offset,dir,next,loaded);
  assert(offset==0&&drawnX==1&&dir==1);
 }
}
'''.replace('FONT', font_cpp).replace('RENDER', render)
with tempfile.TemporaryDirectory(prefix='toern-marquee-') as directory:
    path=Path(directory)
    (path/'test.cpp').write_text(code)
    flags=['c++','-std=c++17','-fsanitize=address,undefined']
    sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists(): flags+=['-isystem',str(sdk)]
    subprocess.run(flags+[str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)

screens=(root/'tools/sim/src/firmware/screens.js').read_text()
marquee=screens[screens.index('function drawMarquee('):screens.index('\nfunction drawDepth')]
js=f'import {{textPixelWidth}} from {json.dumps(font_uri)};\n'+r'''
import assert from 'node:assert/strict';
for(const columns of [16,32]) for(const loaded of [false,true]){
 const draw=new Function('textPixelWidth','COLS',SOURCE+';return drawMarquee;')(textPixelWidth,columns);
 const available=columns-(loaded?4:0);
 for(const name of ['i','iiiii','MMMM','iiiiiiii','MMMMMMMM','MMMMMMMMM','MMij','MMMij','MMMMMMij','MMMMMMMij']){
  let x=0,moved=false;
  const matrix={drawText(_text,start){x=start;}};
  for(let now=0;now<=6000;now+=50){
   draw(matrix,name,[],now,loaded);moved ||= x!==1;
   if(textPixelWidth(name)-1<=available)assert.equal(x,1);
  }
  assert.equal(moved,textPixelWidth(name)-1>available);
 }
}
'''.replace('SOURCE',json.dumps(marquee))
subprocess.run(['node','--input-type=module','-e',js],check=True)
print('PASS: filename marquee scrolls only on pixel overflow, both layouts, loaded mark, exact fits and short names')
