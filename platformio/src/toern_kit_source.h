#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Optional RIFF chunk after PCM data. Unknown chunks remain WAV-compatible;
// old KITs without this chunk retain their existing audio-only behavior.
namespace toern_kit_io {
constexpr size_t sourcePathBytes=128;
constexpr size_t sourceChunkBytes=8+sourcePathBytes;
inline bool validSourcePath(const char *path) {
  if(!path||!path[0]||path[0]=='/'||path[0]=='*')return false;
  size_t segment=0;
  for(size_t i=0;i<sourcePathBytes;++i) {
    const unsigned char c=path[i];
    if(!c||c=='/') {
      const size_t n=i-segment;
      if(!n||(n==1&&path[segment]=='.')||(n==2&&path[segment]=='.'&&path[segment+1]=='.'))return false;
      if(!c)return true;
      segment=i+1;
    } else if(c<' '||c=='\\'||c==':')return false;
  }
  return false;
}
inline uint32_t sourceLE32(const uint8_t *p) {
  return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
template<class File> bool readSourcePath(File &file,char (&out)[sourcePathBytes]) {
  out[0]=0;
  uint8_t header[12];
  const uint64_t size=file.size();
  if(!file.seek(0)||file.read(header,sizeof(header))!=sizeof(header)
      ||memcmp(header,"RIFF",4)||memcmp(header+8,"WAVE",4))return false;
  const uint64_t end=uint64_t(sourceLE32(header+4))+8;
  if(end>size||end<12)return false;
  uint64_t pos=12;
  for(unsigned chunks=0;chunks<100&&pos+8<=end;++chunks) {
    if(!file.seek((uint32_t)pos)||file.read(header,8)!=8)return false;
    const uint32_t bytes=sourceLE32(header+4);
    const uint64_t next=pos+8+bytes+(bytes&1u);
    if(next>end)return false;
    if(!memcmp(header,"tsrc",4)) {
      if(bytes!=sourcePathBytes||file.read(reinterpret_cast<uint8_t*>(out),sourcePathBytes)!=sourcePathBytes
          ||!validSourcePath(out)){out[0]=0;return false;}
      return true;
    }
    pos=next;
  }
  return false;
}
} // namespace toern_kit_io
