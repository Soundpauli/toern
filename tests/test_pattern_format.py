"""Test the firmware's actual pattern validator, including shipped legacy files."""
from pathlib import Path
import subprocess, tempfile, sys
source = Path('toern_fileoperations.ino').read_text()
a = source.index('FLASHMEM bool validPatternFile')
b = source.index('FLASHMEM void showPatternResult', a)
harness = r'''
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <iostream>
using std::min;
#define FLASHMEM
const unsigned maxlen=257, maxY=16;
// Confirmed by the compiled Teensy ELF: SMP is 0x10c4 bytes.
struct { uint8_t bytes[4292]; } SMP;
const uint8_t PITCH_EXTENSION_HEADER[8]={'T','P','I','T',1,0x10,0,0};
void sdIoYield(){}
struct File {
 std::vector<uint8_t> bytes; size_t pos=0; bool opened=false, fail=false;
 explicit operator bool()const{return opened;}
 size_t size(){return bytes.size();}
 bool seek(size_t p){pos=p;return p<=size();}
 int read(){return pos<size() ? bytes[pos++] : -1;}
 int read(uint8_t *out,size_t n){if(fail)return -1; n=min(n,size()-pos);memcpy(out,bytes.data()+pos,n);pos+=n;return n;}
 void close(){}
};
struct {std::map<std::string,File> files;File open(const char *p){return files[p];}} SD;
VALIDATOR
int main(){
 const size_t base=4096*4+2+sizeof(SMP);
 File f;f.opened=true;f.bytes.resize(base);f.bytes[16384]=255;f.bytes[16385]=254;
 SD.files["legacy"]=f;assert(validPatternFile("legacy",false));assert(!validPatternFile("legacy",true));
 f.bytes.insert(f.bytes.end(),PITCH_EXTENSION_HEADER,PITCH_EXTENSION_HEADER+8);f.bytes.resize(base+8+4096,255);
 SD.files["new"]=f;assert(validPatternFile("new",true));
 f.bytes.pop_back();SD.files["torn"]=f;assert(!validPatternFile("torn",true));
 f.bytes.resize(base-1);SD.files["short-settings"]=f;assert(!validPatternFile("short-settings",false));
 f=SD.files["legacy"];f.bytes[16385]=0;SD.files["bad-marker"]=f;assert(!validPatternFile("bad-marker",false));
 f=SD.files["new"];f.fail=true;SD.files["read-error"]=f;assert(!validPatternFile("read-error",true));
 assert(!validPatternFile("absent",false));
 for(const char *path:{"standalone-tools/web-standalone/SD-CARD-CONTENT/1.txt",
 "standalone-tools/web-standalone/SD-CARD-CONTENT/2.txt",
 "standalone-tools/web-standalone/SD-CARD-CONTENT/autosaved.txt"}) {
   std::ifstream in(path,std::ios::binary);assert(in);
   File shipped;shipped.opened=true;shipped.bytes.assign(std::istreambuf_iterator<char>(in),{});
   SD.files[path]=shipped;assert(validPatternFile(path,false));
 }
 std::cout<<"Complete, truncated, unreadable and shipped legacy patterns validated\n";
}
'''.replace('VALIDATOR',source[a:b])
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'format.cpp').write_text(harness)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,str(p/'format.cpp'),'-o',str(p/'format')],check=True)
 subprocess.run([str(p/'format')],check=True)
