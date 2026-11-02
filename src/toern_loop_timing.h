#pragma once
#include <stdint.h>

// Passive timing only: no I/O, allocation or interrupt masking. Read via PERF.
struct ToernLoopTiming {
  enum Stage {Serial, Background, Render, Storage, Inputs, Other, Count};
  static constexpr uint32_t stallThresholdUs=50000;
  uint32_t maximum[Count]={},loopMaximum=0,stalls=0,lastStallUs=0;
  Stage stage=Serial,lastStallStage=Serial;
  uint32_t started=0,stageStarted=0;
  void begin(uint32_t now){started=stageStarted=now;stage=Serial;}
  void mark(Stage next,uint32_t now){
    const uint32_t duration=now-stageStarted;
    if(duration>maximum[stage])maximum[stage]=duration;
    if(duration>stallThresholdUs){++stalls;lastStallUs=duration;lastStallStage=stage;}
    stage=next;stageStarted=now;
  }
  void end(uint32_t now){mark(Other,now);const uint32_t duration=now-started;if(duration>loopMaximum)loopMaximum=duration;}
  static const char *name(Stage stage){
    switch(stage){case Serial:return "serial";case Background:return "background";
      case Render:return "render";case Storage:return "storage";case Inputs:return "inputs";default:return "other";}
  }
};
