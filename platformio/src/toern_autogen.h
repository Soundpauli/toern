#pragma once
#include <stdint.h>

// Loop owns generation; sequencer owns page-boundary commits. A late builder
// repeats the current complete page and never exposes a partial variation.
struct ToernAutogen {
  volatile bool active=false,ready=false,needsBuild=false;
  volatile unsigned playing=0,generation=0,repeatsLeft=4;
  unsigned pages[2]={1,2},width=16,nextRepeats=4;
  static unsigned phraseRepeats(unsigned choice){
    choice%=6;return choice<4?(choice%2?8:4):1;
  }
  void start(unsigned page,unsigned columns,unsigned totalSteps,unsigned repeats){
    width=columns;pages[0]=page;pages[1]=page<totalSteps/columns?page+1:1;
    playing=0;generation=0;repeatsLeft=repeats;ready=false;needsBuild=true;active=true;
  }
  // Keep structural hits reliable; stochastic detail supplies motion on repeats.
  static uint8_t noteChance(unsigned channel,unsigned step,uint32_t seed){
    const unsigned phase=step%16;
    if((channel==1&&phase%4==0)||((channel==2||channel==4)&&phase%8==4)
        ||((channel==6||channel==11||channel==13||channel==14)&&phase%8==0))return 100;
    uint32_t h=seed^(step*374761393u)^(channel*668265263u);
    h=(h^(h>>13))*1274126177u;h^=h>>16;
    const unsigned floor=channel==3||channel==5?60:channel==2||channel==4?70:80;
    return floor+h%(96-floor);
  }
  unsigned source()const{return pages[playing];}
  unsigned target()const{return pages[playing^1u];}
  unsigned firstStep()const{return (source()-1)*width+1;}
  unsigned advance(unsigned step){
    if(step<firstStep()||step>=firstStep()+width-1){
      if(repeatsLeft) --repeatsLeft;
      if(!repeatsLeft&&ready){
        playing^=1u;++generation;repeatsLeft=nextRepeats;
        ready=false;needsBuild=true;
      }
      return firstStep();
    }
    return step+1;
  }
  void publish(unsigned repeats){nextRepeats=repeats;needsBuild=false;ready=true;}
  void stop(){active=false;ready=false;needsBuild=false;}
};
