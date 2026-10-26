"""Exercise the real SD upload parser/handler with paths containing spaces."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_sd_serial.ino').read_text()
def function(name):
    m = re.search(r'^static (?:bool|void|uint32_t) ' + name + r'\([^\n]*\) \{', source, re.M)
    assert m, name
    end = source.index('{', m.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end]

harness = r'''
#include "src/toern_sd_protocol.h"
#include <cassert>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
extern size_t consumed;
extern std::vector<uint8_t> input;
struct Node {bool dir=false;std::vector<uint8_t> bytes;};
struct File {
 std::shared_ptr<Node> n;
 explicit operator bool()const{return bool(n);}
 bool isDirectory(){return n&&n->dir;}
 void close(){} void flush(){}
 // SD must not stall reception while the host is still sending this ACK block.
 size_t write(const uint8_t*b,size_t count){assert(consumed==input.size()||consumed%8192==0);n->bytes.insert(n->bytes.end(),b,b+count);return count;}
};
constexpr int O_WRITE=1,O_CREAT=2,O_TRUNC=4;
struct Card {
 std::map<std::string,std::shared_ptr<Node>> nodes;
 bool exists(const char*p){return nodes.count(p);}
 File open(const char*p,int mode=0){
  if(mode){auto n=std::make_shared<Node>();nodes[p]=n;return {n};}
  return {exists(p)?nodes[p]:nullptr};
 }
 bool remove(const char*p){return nodes.erase(p);}
 bool mkdir(const char*p){auto n=std::make_shared<Node>();n->dir=true;nodes[p]=n;return true;}
} SD;
std::vector<std::string> replies;
std::vector<uint8_t> input;
size_t consumed=0;
const size_t SD_SER_CHUNK=CHUNK_VALUE,SD_SER_PUT_BLOCK=8192;
const uint32_t SD_SER_IO_TIMEOUT_MS=60000;
uint32_t sdSerLastClientMs=0;
uint32_t clockMs=1000;
uint32_t millis(){return clockMs++;}
void yield(){}
bool sdSerPutTrace=false;
void sdSerReplyf(const char*,...){}
struct SerialInput {
 int available(){return int(std::min(size_t(384),input.size()-consumed));}
 int read(){return consumed<input.size()?input[consumed++]:-1;}
 void flush(){}
} Serial;
void sdSerReply(const char*s){replies.push_back(s);}
void sdSerReplyFlush(const char*s){sdSerReply(s);}
void sdSerPutCheckpoint(const char*,uint32_t){}
void sdSerDrainRx(){} void sdSerAudioStopForSd(){}
void AudioNoInterrupts(){} void AudioInterrupts(){}
FUNCTIONS
void put(std::string args,std::vector<uint8_t> bytes){
 input=bytes;consumed=0;replies.clear();
 std::vector<char> line(args.begin(),args.end());line.push_back(0);sdSerCmdPut(line.data());
}
int main(){
 uint8_t shortRead[4]={};input={12,34};consumed=0;
 assert(!sdSerReadExact(shortRead,4,5)&&shortRead[0]==12&&shortRead[1]==34);
 // Standard CRC32 vector and empty-file behavior remain compatible.
 const std::string digits="123456789";
 assert(sdSerCrc32Update(0,(const uint8_t*)digits.data(),digits.size())==0xCBF43926u);
 std::vector<uint8_t> payload(8193);
 for(size_t i=0;i<payload.size();++i)payload[i]=i&255;
 const uint32_t crc=sdSerCrc32Update(0,payload.data(),payload.size());
 char suffix[64];snprintf(suffix,sizeof(suffix)," %zu %08X",payload.size(),crc);
 const std::string path="Sample Packs/Drum Kit 01/1 - Kick.bin";
 put("/"+path+suffix,payload);
 assert(replies==std::vector<std::string>({"READY","ACK","ACK","OK"}));
 assert(SD.nodes.at(path)->bytes==payload&&consumed==payload.size());
 assert(SD.nodes.at("Sample Packs")->dir&&SD.nodes.at("Sample Packs/Drum Kit 01")->dir);
 // Multi-megabyte files exercise hundreds of ACK blocks, with exact bytes/CRC.
 for(size_t length : {size_t(3*1024*1024+17),size_t(5*1024*1024+1)}){
  std::vector<uint8_t> big(length);
  for(size_t i=0;i<length;++i)big[i]=(i*17+31)&255;
  const uint32_t bigCrc=sdSerCrc32Update(0,big.data(),big.size());
  snprintf(suffix,sizeof(suffix)," %zu %08X",length,bigCrc);
  put("/large.bin"+std::string(suffix),big);
  assert(replies.front()=="READY"&&replies.back()=="OK");
  assert(std::count(replies.begin(),replies.end(),"ACK")==int((length+8191)/8192));
  assert(SD.nodes.at("large.bin")->bytes==big&&consumed==length);
 }
 // With the old tokenizer this exact command became size=0, CRC=1.
 put("/New Folder/test.bin 1 D202EF8D",{0});
 assert(replies.back()=="OK"&&SD.nodes.at("New Folder/test.bin")->bytes==std::vector<uint8_t>({0}));
 // Invalid numbers are rejected before opening/truncating an existing file.
 auto original=SD.nodes.at(path);
 for(const std::string tail:{" nope 00000000"," -1 00000000"," 4294967296 00000000", " 0 000000001", " 0 G1234567", " 0 12garbage"}){
  put("/"+path+tail,{});assert(replies==std::vector<std::string>({"ERR USAGE"}));
  assert(SD.nodes.at(path)==original);
 }
 put("/empty.bin 0 00000000",{});assert(replies.back()=="OK"&&SD.nodes.at("empty.bin")->bytes.empty());
 put("/bad-crc.bin 1 00000001",{0});assert(replies.back().find("ERR CRC got=")==0&&!SD.exists("bad-crc.bin"));
 put("/plain.bin 1 D202EF8D",{0});assert(replies.back()=="OK");
 // No truncation, segment splitting, or newline injection in long/nested paths.
 char out[128];
 assert(sdSerNormalizePath("/space here/sub dir/a.bin",out,sizeof(out))&&std::string(out)=="space here/sub dir/a.bin");
 assert(sdSerNormalizePath(std::string(100,'a').c_str(),out,sizeof(out))&&strlen(out)==100);
 assert(!sdSerNormalizePath(std::string(128,'a').c_str(),out,sizeof(out)));
 assert(!sdSerNormalizePath("/ok/../escape",out,sizeof(out)));
 assert(!sdSerNormalizePath("/bad\nPING",out,sizeof(out)));
 assert(sdSerNormalizePath("/",out,sizeof(out))&&std::string(out)=="/");
}
'''.replace('FUNCTIONS','\n'.join(function(n) for n in ['sdSerReadExact','sdSerCrc32Update','sdSerNormalizePath','sdSerCmdPut']))
harness = harness.replace('CHUNK_VALUE', re.search(r'SD_SER_CHUNK = (\d+);', source).group(1))
with tempfile.TemporaryDirectory(prefix='toern-sd-protocol-') as tmp:
    tmp = Path(tmp)
    cpp = tmp / 'sd.cpp'
    cpp.write_text(harness)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,'-I',str(root),str(cpp),'-o',str(tmp/'sd')],check=True)
    subprocess.run([str(tmp/'sd')],check=True)
print('PASS: spaced/nested paths, exact CRC reproduction, complete blocks before SD writes, strict numbers, empty files, path bounds and CRC rejection.')
