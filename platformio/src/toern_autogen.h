#pragma once
#include <stdint.h>

// Loop owns generation; sequencer owns page-boundary commits. A late builder
// repeats the current complete page and never exposes a partial variation.
struct ToernAutogen {
  volatile bool active=false,ready=false,needsBuild=false;
  volatile unsigned playing=0,generation=0,repeatsLeft=4;
  unsigned pages[2]={1,2},width=16,nextRepeats=4;
  volatile unsigned passesTotal=4;
  // Musical progress to the next page boundary, including every scheduled pass.
  // A late builder holds the full ring while the current page safely repeats.
  unsigned statusProgress(unsigned step)const{
    if(!repeatsLeft)return 12;
    const unsigned total=passesTotal?passesTotal:1;
    const unsigned left=repeatsLeft>total?total:(unsigned)repeatsLeft;
    const unsigned elapsed=(total-left)*width+(step?(step-1)%width:0);
    const unsigned span=total*width>1?total*width-1:1;
    const unsigned progress=elapsed*12/span;
    return progress>12?12:progress;
  }
  // 5x5 clock: persistent rim + playback arc; amber preparation spinner or green check.
  // Ink: 0 black, 1 dim rim, 2 elapsed cyan, 3 busy/late amber, 4 ready green.
  static unsigned statusInk(unsigned x,unsigned y,unsigned progress,bool ready,uint32_t now){
    static const uint8_t ring[12][2]={{2,0},{3,0},{4,1},{4,2},{4,3},{3,4},
      {2,4},{1,4},{0,3},{0,2},{0,1},{1,0}};
    for(unsigned i=0;i<12;++i)if(x==ring[i][0]&&y==ring[i][1])
      return i<progress?(!ready&&progress>=12?3:2):1;
    if(ready)return ((x==1&&y==2)||(x==2&&y==3)||(x==3&&(y==1||y==2)))?4:0;
    static const uint8_t spinner[8][2]={{2,1},{3,1},{3,2},{3,3},{2,3},{1,3},{1,2},{1,1}};
    const unsigned frame=(now/240)%8;
    for(unsigned i=0;i<8;++i)if(x==spinner[i][0]&&y==spinner[i][1])return (frame+8-i)%8<3?3:0;
    return 0;
  }
  void start(unsigned page,unsigned columns,unsigned totalSteps,unsigned repeats){
    width=columns;for(unsigned i=0;i<2;++i)pages[i]=(page-1+i)%(totalSteps/columns)+1;
    playing=0;generation=0;repeatsLeft=repeats;passesTotal=repeats;ready=false;needsBuild=true;active=true;
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
        playing=playing^1u;++generation;repeatsLeft=nextRepeats;passesTotal=nextRepeats;
        ready=false;needsBuild=true;
      }
      return firstStep();
    }
    return step+1;
  }
  void publish(unsigned repeats){nextRepeats=repeats;needsBuild=false;ready=true;}
  void stop(){active=false;ready=false;needsBuild=false;}
};
