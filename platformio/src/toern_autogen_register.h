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
// motion matches the AGEN menu: 0 keeps the drawn pitch, 1 allows a second,
// 2 follows the chord and the occasional variation step.
static inline int monoPitch(int reference,int tonic,bool minor,unsigned phrase,unsigned channel,unsigned step,uint32_t seed,unsigned motion=2){
  if(!motion)return reference;
  int target=reference;
  const uint32_t h=seed+step*977u+channel*37u+phrase*101u;
  if(motion>=2&&toern_autogen_music::section(phrase)==toern_autogen_music::Section::Variation&&step%4&&h%4==0)
    target+=(h&4)?2:-2;
  const int fitted=nearSource(toern_autogen_music::fitPitch(target,tonic,minor,phrase,channel,step),reference);
  if(motion<2){const int delta=fitted-reference;if(delta>2||delta<-2)return reference;}
  return fitted;
}
}
