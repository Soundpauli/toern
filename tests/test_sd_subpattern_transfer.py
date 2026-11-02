"""Run the real PUTPAT/PUTSUB handlers and RAM decoder with serial fault injection."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'toern_sd_serial.ino').read_text()

def function(name):
    match = re.search(r'^static (?:bool|void|uint32_t|char \*)\s*' + name + r'\([^\n]*\) \{', source, re.M)
    assert match, name
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

constants = source[source.index('static const size_t SD_PAT_NOTES'):source.index('static void sdPatPack')]
code = r'''
#include "src/toern_sd_protocol.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#define EXTMEM
const uint8_t NOTE_MIDI_PITCH_NONE=255;
struct Note {uint8_t channel=0,velocity=0,probability=100,condition=1,midiPitch=255;};
Note banks[5][257][17];Note (*note)[17]=banks[0];
struct {float bpm=100;} SMP;
double playNoteInterval=0;
struct Timer {void update(double){}} playTimer,fillTimer;
void syncBpmEncoderPositions(){}
unsigned activated=0,finished=0,stopped=0;int selected=0;
const char *failure=nullptr;
const char *subpatternTransferError(){return failure;}
bool beginTransferredSubpattern(uint8_t slot){assert(slot<5);activated++;selected=slot;note=banks[slot];return !failure;}
void finishTransferredSubpattern(){finished++;}
void showDrawAfterPatternTransfer(){}
std::vector<std::string> replies;
void sdSerReply(const char *s){replies.push_back(s);}
void sdSerReplyFlush(const char *s){sdSerReply(s);}
void sdSerAudioStopForSd(){stopped++;}
void sdSerDrainRx(){}
uint32_t sdSerLastClientMs=0;uint32_t millis(){return 100;}
constexpr size_t SD_SER_PUT_BLOCK=8192,SD_SER_CHUNK=8192;
constexpr uint32_t SD_SER_IO_TIMEOUT_MS=60000;
std::vector<uint8_t> incoming;size_t cursor=0;
bool sdSerReadExact(uint8_t *out,size_t count,uint32_t){
 if(count>incoming.size()-cursor)return false;
 memcpy(out,incoming.data()+cursor,count);cursor+=count;return true;
}
CONSTANTS
FUNCTIONS
std::vector<uint8_t> image(){
 memset(sdPatBuf,0,sizeof(sdPatBuf));
 sdPatBuf[0]=11;sdPatBuf[1]=88;sdPatBuf[2]=100;sdPatBuf[3]=1;
 // Also exercise the very last cell of the 256-step bank.
 sdPatBuf[SD_PAT_NOTE_BYTES-4]=14;sdPatBuf[SD_PAT_NOTE_BYTES-3]=127;
 sdPatBuf[SD_PAT_NOTE_BYTES]=255;sdPatBuf[SD_PAT_NOTE_BYTES+1]=254;
 float bpm=123.5f;memcpy(sdPatBuf+SD_PAT_NOTE_BYTES+2,&bpm,4);
 memcpy(sdPatBuf+SD_PAT_NOTE_BYTES+6,SD_PAT_PITCH_HDR,8);
 memset(sdPatBuf+SD_PAT_NOTE_BYTES+14,255,SD_PAT_NOTES);
 sdPatBuf[SD_PAT_NOTE_BYTES+14]=72;sdPatBuf[SD_PAT_BYTES-1]=36;
 return {sdPatBuf,sdPatBuf+SD_PAT_BYTES};
}
void send(std::string args,const std::vector<uint8_t>&data,bool sub=true){
 incoming=data;cursor=0;replies.clear();
 std::vector<char> line(args.begin(),args.end());line.push_back(0);
 if(sub)sdSerCmdPutSub(line.data());else sdSerCmdPutPat(line.data(),-1);
}
std::string suffix(const std::vector<uint8_t>&data){
 char out[64];snprintf(out,sizeof(out),"%zu %08X",data.size(),sdSerCrc32Update(0,data.data(),data.size()));return out;
}
int main(){
 auto bytes=image();
 for(char letter='A';letter<='E';letter++){
  send(std::string(1,letter)+" "+suffix(bytes),bytes);
  assert(replies==std::vector<std::string>({"READY","ACK","ACK","ACK","OK"}));
  assert(selected==letter-'A'&&note[1][1].channel==11&&note[1][1].midiPitch==72);
  assert(note[256][16].channel==14&&note[256][16].midiPitch==36&&SMP.bpm==123.5f);
 }
 assert(activated==5&&finished==5);
 const auto old=note[1][1];const unsigned before=activated;
 for(auto bad:{std::string("F ")+suffix(bytes),std::string("AA ")+suffix(bytes),std::string("A nope 0"),std::string("A 20494 FFFFFFFF extra"),std::string("A -1 0")}){
  send(bad,bytes);assert(replies==std::vector<std::string>({"ERR USAGE"}));assert(activated==before);
 }
 send("A 1 0",{});assert(replies==std::vector<std::string>({"ERR SIZE"}));
 failure="ERR SUBP_OFF";send("A "+suffix(bytes),bytes);
 assert(replies==std::vector<std::string>({"ERR SUBP_OFF"})&&cursor==0&&activated==before);failure=nullptr;
 send("A "+suffix(bytes),{});assert(replies.back()=="ERR IO"&&activated==before);
 send("A 20494 00000000",bytes);assert(replies.back().find("ERR CRC")==0&&activated==before);
 for(size_t at:{size_t(SD_PAT_NOTE_BYTES),size_t(SD_PAT_NOTE_BYTES+6),size_t(0),size_t(1),size_t(2)}){
  auto bad=bytes;bad[at]=253;send("A "+suffix(bad),bad);
  assert(replies.back()=="ERR PATTERN"&&activated==before&&memcmp(&old,&note[1][1],sizeof(old))==0);
 }
 // Legacy PUTPAT continues to target the active bank without selecting a letter.
 send(suffix(bytes),bytes,false);assert(replies.back()=="OK"&&activated==before&&selected==4);
 puts("PASS: A–E USB RAM images, all ACK blocks, preserved pitch/BPM, legacy PUTPAT, disabled mode, malformed headers/data, short reads and CRC rejection before bank mutation");
}
'''.replace('CONSTANTS', constants).replace('FUNCTIONS', '\n'.join(function(name) for name in (
    'sdSerCrc32Update', 'sdSerSkipSpaces', 'sdPatApply', 'sdSerCmdPutPat', 'sdSerCmdPutSub')))
with tempfile.TemporaryDirectory(prefix='toern-putsub-') as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(code)
    flags = ['-isystem', '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform == 'darwin' else []
    subprocess.run(['c++', '-std=c++17', '-fsanitize=address,undefined', *flags, '-I', str(root), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
