#pragma once
#include <stdint.h>

struct ToernI2cHealth {
  bool runtime=false,cooling=false;
  uint32_t retryAt=0,errors=0,skipped=0,maxUs=0;
  uint8_t lastError=0;
  bool ready(uint32_t now){
    if(cooling&&int32_t(now-retryAt)>=0)cooling=false;
    return !runtime||!cooling;
  }
  void record(uint32_t now,uint32_t duration,uint8_t error){
    if(duration>maxUs)maxUs=duration;
    if(!error)return;
    ++errors;lastError=error;
    // Optional devices may NACK during startup; never suppress those probes.
    if(runtime){cooling=true;retryAt=now+(duration>5000?100:8);}
  }
};
