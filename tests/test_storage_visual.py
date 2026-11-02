"""Render real storage UI pixels on both panels; verify bounds, icons and results."""
from pathlib import Path
import re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
icons=(root/'icons.h').read_text()
def icon(name):
    return re.search(r'const uint8_t '+name+r'\[ICON_BYTES\].*?=\s*\{([^}]+)\}',icons).group(1)
code=r'''
#include "src/toern_storage_visual.h"
#define PROGMEM
#include "font_3x5.h"
#include <cassert>
#include <string>
#include <vector>
using toern_storage_visual::Color;
const uint8_t folder[]={FOLDER},kitIcon[]={KIT};
struct Canvas {
 unsigned width;int textY=0;std::vector<uint8_t> rgb;std::vector<std::string> textLabels;
 Canvas(unsigned width):width(width),rgb(width*16*3){}
 void clear(){std::fill(rgb.begin(),rgb.end(),0);textLabels.clear();}
 void pixel(int x,int y,Color c){assert(x>=1&&x<=(int)width&&y>=1&&y<=16);const auto at=((16-y)*width+x-1)*3;rgb[at]=c.r;rgb[at+1]=c.g;rgb[at+2]=c.b;}
 void icon(bool kit,int x,int top,unsigned phase){
  const auto data=kit?kitIcon:folder;
  for(unsigned row=0;row<7;row++)for(unsigned col=0;col<7;col++){
   const auto bit=row*12+col;if((data[bit/8]>>(bit%8))&1)pixel(x+col,top-row,row==phase?Color{140,165,190}:Color{50,65,85});
  }
 }
 int textWidth(const char*s){int n=0;for(unsigned i=0;s[i];i++)n+=alphabet[s[i]-32][0]+1;return n-1;}
 void text(const char*s,int x,int y,Color c){
  textY=y;textLabels.push_back(s);
  for(unsigned i=0;s[i];i++){
   const auto *glyph=alphabet[s[i]-32];
   for(unsigned col=0;col<glyph[0];col++)for(unsigned row=0;row<5;row++)if(glyph[col+1]&(1<<(4-row)))pixel(x+col,y+row,c);
   x+=glyph[0]+1;
  }
 }
 void save(const char *dir,const char *name){char path[512];snprintf(path,sizeof(path),"%s/%s-%u.ppm",dir,name,width);auto f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u 16\n255\n",width);fwrite(rgb.data(),1,rgb.size(),f);fclose(f);}
};
int main(int argc,char**argv){assert(argc==2);
 for(unsigned width:{16u,32u}) {
  Canvas c(width);
  for(bool kit:{false,true})for(bool saving:{false,true})for(unsigned pct:{0u,1u,9u,37u,68u,99u,100u})for(unsigned t:{0u,150u,350u}) {
   toern_storage_visual::progress(c,width,kit,saving,pct,t);
   char label[6];snprintf(label,sizeof(label),"%02u%%",pct);
   assert(c.textLabels==std::vector<std::string>{label});assert(c.textY==4);
  }
  toern_storage_visual::progress(c,width,false,false,37,150);c.save(argv[1],"patt-load");
  const auto pattern=c.rgb;
  toern_storage_visual::progress(c,width,true,false,37,150);assert(c.rgb!=pattern);c.save(argv[1],"kit-load");
  toern_storage_visual::progress(c,width,true,true,68,150);c.save(argv[1],"kit-save");
  toern_storage_visual::progress(c,width,false,true,100,150);c.save(argv[1],"patt-save");
  for(bool success:{false,true})for(bool white:{false,true}) {
   unsigned previous=0;
   for(unsigned t:{0u,80u,200u,400u,600u,720u}) {
    toern_storage_visual::result(c,width,success,white,t);assert(c.textLabels.empty());
    unsigned pixels=0;
    for(size_t at=0;at<c.rgb.size();at+=3)if(c.rgb[at]||c.rgb[at+1]||c.rgb[at+2]) {
     pixels++;
     if(!success)assert(c.rgb[at]>0&&c.rgb[at+1]==0&&c.rgb[at+2]==0);
     else if(white)assert(c.rgb[at]==c.rgb[at+1]&&c.rgb[at+1]==c.rgb[at+2]);
     else assert(c.rgb[at]==0&&c.rgb[at+1]>0&&c.rgb[at+2]==0);
    }
    if(t<=400){assert(pixels>=previous);previous=pixels;}
   }
  }
  toern_storage_visual::result(c,width,true,false,450);
  const int8_t check[][2]={{-3,-1},{-2,-2},{-1,-3},{0,-2},{1,-1},{2,0},{3,1},{4,2}};
  unsigned lit=0;for(size_t at=0;at<c.rgb.size();at+=3)if(c.rgb[at+1])lit++;
  assert(lit==8);
  for(const auto &point:check){const auto at=((16-(8+point[1]))*width+width/2+point[0]-1)*3;assert(c.rgb[at+1]>0);}
  c.save(argv[1],"check-green");
  toern_storage_visual::result(c,width,true,true,450);c.save(argv[1],"check-white");
  toern_storage_visual::result(c,width,false,false,400);c.save(argv[1],"error");
 }
 puts("PASS: 16x16/32x16 pixel bounds, real menu icons, 00–100%, both arrows, green/white animated checks and red X without labels");
}
'''.replace('FOLDER',icon('icon_folder_big')).replace('KIT',icon('OLD_icon_samplepack'))
out=Path(tempfile.gettempdir())/'toern-storage-visual';out.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='toern-storage-ui-') as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(code)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe),str(out)],check=True)
sys.path.insert(0,str(root/'standalone-tools/toern-mcp'))
from remote import display_png
for ppm in out.glob('*.ppm'):
    _,size,_,rgb=ppm.read_bytes().split(b'\n',3);width,height=map(int,size.split())
    ppm.with_suffix('.png').write_bytes(display_png({'width':width,'height':height,'rgb':rgb.hex()},16))
