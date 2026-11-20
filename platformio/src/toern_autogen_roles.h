#pragma once
#include <stdint.h>

namespace toern_autogen_roles {
// Channel zero is the ALL selector; pattern cells use zero for silence.
constexpr uint8_t channels[]={0,1,2,3,4,5,6,7,8,11,13,14};
constexpr uint8_t roleChannels[]={0,1,2,3,4,5,6,11,8,13};
constexpr const char *names[]={"OFF","KICK","SNAR","HAT","CLAP","TOM","BASS","KEYS","VOIC","LEAD"};
constexpr unsigned kindCount=sizeof(roleChannels);
constexpr unsigned channelCount=sizeof(channels);
struct Assignments {
  // 255 keeps the original automatic behavior, including DEEP and low CH11.
  uint8_t choices[16];
  Assignments(){for(auto &choice:choices)choice=255;}
  bool automatic(unsigned ch)const{return ch<16&&choices[ch]==255;}
  unsigned role(unsigned ch)const{
    return ch>0&&ch<16?(automatic(ch)?ch:roleChannels[choices[ch]]):0;
  }
  unsigned kind(unsigned ch,unsigned automaticRole)const{
    if(ch>=16)return 0;
    if(!automatic(ch))return choices[ch];
    if(automaticRole==7)automaticRole=1;
    if(automaticRole==14)automaticRole=6;
    for(unsigned i=0;i<kindCount;++i)if(roleChannels[i]==automaticRole)return i;
    return 0;
  }
  void set(unsigned ch,unsigned kind){
    if(ch>=16||kind>=kindCount)return;
    if(ch)choices[ch]=kind;
    else {choices[0]=kind;for(unsigned i=1;i<channelCount;++i)choices[channels[i]]=kind;}
  }
};
}
