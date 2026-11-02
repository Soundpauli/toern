#pragma once
#include <stdint.h>
#if defined(ARDUINO)
#include <avr/pgmspace.h>
#define AGEN_MUSIC_DATA PROGMEM
#define AGEN_MUSIC_CODE FLASHMEM
#else
#define AGEN_MUSIC_DATA
#define AGEN_MUSIC_CODE
#endif

namespace toern_autogen_music {
enum class Section {Groove,Variation,Break,Build,Fill,Drop};
AGEN_MUSIC_CODE static Section section(unsigned generation){
  static const Section form[] AGEN_MUSIC_DATA={Section::Groove,Section::Variation,Section::Groove,Section::Break,
      Section::Build,Section::Fill,Section::Drop,Section::Variation};
  return form[generation%8];
}
AGEN_MUSIC_CODE static unsigned repeats(Section s,unsigned choice){
  if(s==Section::Fill)return 1;
  if(s==Section::Break||s==Section::Build)return choice%3?1:4;
  if(s==Section::Drop)return choice%2?8:4;
  static const uint8_t lengths[] AGEN_MUSIC_DATA={4,8,4,8,1,1};return lengths[choice%6];
}
AGEN_MUSIC_CODE static unsigned degree(bool minor,unsigned phrase){
  static const uint8_t major[] AGEN_MUSIC_DATA={0,5,3,4};
  static const uint8_t naturalMinor[] AGEN_MUSIC_DATA={0,5,3,6};
  // Hold harmony through break/build/fill, then resolve the drop to the tonic.
  static const uint8_t harmonicSlots[] AGEN_MUSIC_DATA={0,1,2,2,3,3,0,1};
  return (minor?naturalMinor:major)[harmonicSlots[phrase%8]];
}
AGEN_MUSIC_CODE static int scalePc(int tonic,bool minor,unsigned d){
  static const uint8_t major[] AGEN_MUSIC_DATA={0,2,4,5,7,9,11};
  static const uint8_t naturalMinor[] AGEN_MUSIC_DATA={0,2,3,5,7,8,10};
  return (tonic+(minor?naturalMinor:major)[d%7])%12;
}
AGEN_MUSIC_CODE static int chordPc(int tonic,bool minor,unsigned phrase,unsigned tone){return scalePc(tonic,minor,degree(minor,phrase)+tone*2);}
AGEN_MUSIC_CODE static int nearestPc(int reference,int pc){
  int best=reference,distance=999;
  for(int n=0;n<=127;++n){int delta=n-reference;if(delta<0)delta=-delta;
    if(n%12==pc&&delta<distance){best=n;distance=delta;}}
  return best;
}
AGEN_MUSIC_CODE static int fitPitch(int reference,int tonic,bool minor,unsigned phrase,unsigned channel,unsigned step){
  if((channel==6||channel==14)&&step%4==0)return nearestPc(reference,chordPc(tonic,minor,phrase,0));
  int best=reference,distance=999;
  const unsigned choices=step%4==0?3:7;
  for(unsigned i=0;i<choices;++i){
    int pc=choices==3?chordPc(tonic,minor,phrase,i):scalePc(tonic,minor,i);
    int n=nearestPc(reference,pc),delta=n-reference;if(delta<0)delta=-delta;
    if(delta<distance){distance=delta;best=n;}
  }
  return best;
}
AGEN_MUSIC_CODE static bool bassAllowed(Section s,unsigned step,unsigned width){
  if(s==Section::Break)return false;
  if(s==Section::Build)return step==0||step==width/2;
  return true;
}
// Return -1 to keep the existing hit, 0 to clear, or a velocity for a new/accented hit.
AGEN_MUSIC_CODE static int drum(Section s,unsigned channel,unsigned step,unsigned width,uint32_t seed){
  const unsigned phase=step%16;const bool tail=step>=width*3/4;
  const uint32_t h=(seed*1664525u+step*1013904223u+channel*977u)>>8;
  if(s==Section::Break){
    if(channel==1||channel==5||channel==4)return 0;
    if(channel==2)return step==width*3/4?65:0;
    if(channel==3)return phase%8==2?56:0;
  }
  if(s==Section::Build){
    if(channel==1)return step>=width/2&&phase%8==0?94:0;
    if(channel==2){unsigned spacing=tail?1:step>=width/2?2:4;return step%spacing==0?58+step*56/width:0;}
    if(channel==3)return step>=width/2&&step%2==1?68:-1;
    if(channel==4||channel==5)return tail&&phase%2==0?78:0;
  }
  if(s==Section::Fill){
    if(channel==1&&tail)return 0;
    if(channel==2&&tail)return 76+(step%4)*10;
    if(channel==5&&tail&&phase%2==1)return 86;
    if(channel==3&&tail)return 68+(step%2)*12;
  }
  if(s==Section::Drop){
    if(channel==1)return phase%4==0?116:0;
    if(channel==2)return phase%8==4?110:0;
    if(channel==3)return phase%4==2?90:phase%2==0?72:0;
    if(channel==4)return phase==12?96:0;
  }
  if(s==Section::Variation){
    if(channel==1&&step==width-2)return 88;
    if(channel==3)return phase%4==2?88:phase%2==1&&h%3==0?58:0;
    if(channel==2&&(phase==7||phase==15)&&h%2==0)return 56;
    if(channel==5&&tail&&phase%2==1)return 78;
  }
  return -1;
}
}

#undef AGEN_MUSIC_DATA
#undef AGEN_MUSIC_CODE
