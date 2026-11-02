#pragma once
#include <stdint.h>
#include <string.h>

// Cache availability during one browser visit. Unknown slots are probed only
// after the dial settles, with at most one SD lookup per loop (never eight).
class ToernKitBrowser {
  uint8_t known[125]={},present[125]={};
  unsigned slot=1000,voice=1;
  uint32_t selectedAt=0;
public:
  void reset(){memset(known,0,sizeof(known));memset(present,0,sizeof(present));slot=1000;}
  bool select(unsigned value,uint32_t now){
    if(value==slot)return false;
    slot=value;voice=1;selectedAt=now;return true;
  }
  bool available()const{return slot<1000&&(present[slot/8]&(1u<<(slot%8)));}
  bool pending()const{return slot<1000&&!(known[slot/8]&(1u<<(slot%8)));}
  template<class Exists> bool tick(uint32_t now,Exists exists){
    if(!pending()||uint32_t(now-selectedAt)<35)return false;
    const bool found=exists(slot,voice++);
    if(found||voice>8){
      known[slot/8]|=1u<<(slot%8);
      if(found)present[slot/8]|=1u<<(slot%8);
      return true;
    }
    return false;
  }
};
