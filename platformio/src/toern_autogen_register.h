#pragma once
#include "toern_autogen_music.h"
namespace toern_autogen_register {
// Sample MIDI is relative to root C4, not the sampler's playback-table index.
static inline int sampleMidi(unsigned channel,unsigned row){return 60+(int)row-(int)channel-1;}
static inline int nearSource(int pitch,int reference){
  while(pitch>reference+6)pitch-=12;
  while(pitch<reference-6)pitch+=12;
  return pitch<0?pitch+12:pitch>127?pitch-12:pitch;
}
static inline unsigned phrasePages(unsigned width){return (64+width-1)/width;}
static inline int monoPitch(int reference,int tonic,bool minor,unsigned phrase,unsigned channel,unsigned step,uint32_t seed){
  // Preserve the motif's register. Only occasional small passing-note movement.
  int target=reference;
  const uint32_t h=seed+step*977u+channel*37u+phrase*101u;
  if(toern_autogen_music::section(phrase)==toern_autogen_music::Section::Variation&&step%4&&h%4==0)
    target+=(h&4)?2:-2;
  return nearSource(toern_autogen_music::fitPitch(target,tonic,minor,phrase,channel,step),reference);
}
}
