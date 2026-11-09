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
// One 64-bar cycle. A short rise (Build) and a short dip (Break) each resolve by
// the kick and bass coming back in the following groove. The fill and drop
// happen once, at the end of the cycle, rather than every 16 bars.
AGEN_MUSIC_CODE static Section section(unsigned generation){
  static const Section form[] AGEN_MUSIC_DATA={
      Section::Groove,Section::Variation,Section::Build,Section::Groove,
      Section::Variation,Section::Break,Section::Groove,Section::Groove,
      Section::Variation,Section::Build,Section::Fill,Section::Drop};
  return form[(generation/2)%12];
}
AGEN_MUSIC_CODE static unsigned phraseBars(unsigned generation){
  static const uint8_t bars[] AGEN_MUSIC_DATA={8,8,2,8,8,2,8,8,4,2,2,4};
  return bars[(generation/2)%12];
}
// `generation` selects the slot length. A roll is never repeated: Build and Fill
// stay one pass of the call and one of the answer. Lengths are musical bars for
// the whole pair; slow tempos halve them so a slot cannot run on past half a minute.
AGEN_MUSIC_CODE static unsigned repeats(Section s,unsigned generation,unsigned width=16,unsigned bpm=120){
  if(s==Section::Build||s==Section::Fill)return 1;
  unsigned bars=phraseBars(generation);
  const unsigned limit=s==Section::Build?32000u:24000u;
  if(bpm<40)bpm=40;
  while(bars>1&&bars*240000u>bpm*limit)bars/=2;
  const unsigned passes=(bars*16u)/(2u*((width==32)?32u:16u));
  return passes?passes:1;
}
AGEN_MUSIC_CODE static unsigned degree(bool minor,unsigned phrase){
  static const uint8_t major[] AGEN_MUSIC_DATA={0,5,3,4};
  static const uint8_t naturalMinor[] AGEN_MUSIC_DATA={0,5,3,6};
  // Hold harmony through break/build/fill, then resolve the drop to the tonic.
  static const uint8_t harmonicSlots[] AGEN_MUSIC_DATA={0,1,2,3,3,0,1,2};
  return (minor?naturalMinor:major)[harmonicSlots[(phrase/2)%8]];
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
// Move actual in-key notes, rather than only quantizing unchanged notes. Both
// pages share a contour seed; the answer reverses its direction. Bass downbeats
// and chord-tone landings stay structural. Everything derives from the anchor.
// motion: 0 hold the drawn pitch, 1 soft steps of a second, 2 the full contour.
AGEN_MUSIC_CODE static int melodyPitch(int reference,int tonic,bool minor,unsigned phrase,
                                      unsigned channel,unsigned step,uint32_t seed,unsigned motion=2){
  const Section s=section(phrase);
  const bool bass=channel==6||channel==14;
  const int baseline=fitPitch(reference,tonic,minor,phrase,channel,step);
  if(!motion||(bass&&step%4==0))return baseline;
  uint32_t h=seed^(phrase/2*374761393u)^(channel*668265263u)^(step*2246822519u);
  h=(h^(h>>13))*1274126177u;h^=h>>16;
  unsigned chance=bass?50:s==Section::Variation||s==Section::Fill?90:
      s==Section::Build?80:s==Section::Break?65:s==Section::Drop?45:40;
  if(motion<2&&chance>35)chance=35;
  if(h%100>=chance)return baseline;
  static const int8_t contour[] AGEN_MUSIC_DATA={2,4,3,7,5,-2,4,-5};
  int delta=contour[((step/2)+channel+(phrase/2))%8];
  if(phrase&1)delta=-delta;
  if(bass||motion<2)delta=delta>0?2:-2;
  const int target=reference+delta;
  int best=baseline,distance=999;
  for(int n=reference-12;n<=reference+12;++n){
    if(n<0||n>127)continue;
    bool allowed=false;
    for(unsigned d=0;d<(step%4==0?3u:7u);++d)
      allowed|=n%12==(step%4==0?chordPc(tonic,minor,phrase,d):scalePc(tonic,minor,d));
    int gap=n-target;if(gap<0)gap=-gap;
    if(allowed&&gap<distance){best=n;distance=gap;}
  }
  return best;
}
AGEN_MUSIC_CODE static int voicingPitch(int reference,int tonic,bool minor,unsigned phrase,unsigned lane,uint32_t seed){
  const unsigned inversion=(phrase+seed)%3;
  const int root=nearestPc(reference,chordPc(tonic,minor,phrase,0));
  int notes[3];
  for(unsigned i=0;i<3;++i){
    const unsigned tone=(i+inversion)%3;
    notes[i]=root+(chordPc(tonic,minor,phrase,tone)-chordPc(tonic,minor,phrase,0)+12)%12+(tone<inversion?12:0);
  }
  int center=(notes[0]+notes[1]+notes[2])/3,shift=0;
  while(center+shift>reference+6)shift-=12;
  while(center+shift<reference-6)shift+=12;
  while(notes[0]+shift<24)shift+=12;
  while(notes[2]+shift>108)shift-=12;
  int pitch=notes[lane%3]+shift;
  return pitch;
}
// AGEN notes use a musical register, then undo only the patch's octave offset.
// Fine tuning and the user's patch stay intact; never apply the register twice.
AGEN_MUSIC_CODE static int encodePitch(int pitch,int octaveShift){
  pitch-=octaveShift;
  while(pitch<0)pitch+=12;
  while(pitch>127)pitch-=12;
  return pitch;
}
AGEN_MUSIC_CODE static Section supportedSection(Section requested,unsigned foundation){
  if(!foundation&&(requested==Section::Build||requested==Section::Break||requested==Section::Fill))return Section::Groove;
  return requested;
}
AGEN_MUSIC_CODE static bool bassAllowed(Section s,unsigned step,unsigned width,bool finale=true){
  if(s==Section::Break)return false;
  if(s==Section::Build)return false;
  if(s==Section::Fill)return !finale||step<width-2;
  return true;
}
// 0..16 relative to the user's starting level; never boost beyond that ceiling.
AGEN_MUSIC_CODE static unsigned volumeScale(Section s,unsigned ch,bool bass,unsigned progress){
  if(progress>255)progress=255;
  if(s==Section::Break){if(ch==1||bass)return 0;return ch<=5?7:12;}
  if(s==Section::Build){
    if(ch==1||bass)return 0;
    if(ch==2)return 5+progress*11/255;
    if(ch<=5)return 6+progress*9/255;
    return 10+progress*6/255;
  }
  if(s==Section::Fill){
    if(ch==1||bass)return 0;
    return ch==2||ch==5?12+progress*4/255:12;
  }
  if(s==Section::Drop)return 16;
  const unsigned triangle=progress<128?progress*2:(255-progress)*2;
  if(ch==3||ch==5)return 11+triangle*5/255;
  if(ch==7||ch==8||(!bass&&(ch==11||ch==13||ch==14)))return 12+triangle*4/255;
  return 16;
}

// Return -1 to keep the existing hit, 0 to clear, or a velocity for a new/accented hit.
AGEN_MUSIC_CODE static int drum(Section s,unsigned channel,unsigned step,unsigned width,uint32_t seed,bool finale=true){
  const unsigned phase=step%16;const bool tail=step>=width*3/4;
  const uint32_t h=(seed*1664525u+step*1013904223u+channel*977u)>>8;
  if(s==Section::Break){
    if(channel==1||channel==5||channel==4)return 0;
    if(channel==2)return step==width*3/4?65:0;
    if(channel==3)return phase%8==2?56:0;
  }
  if(s==Section::Build){
    if(finale&&step==width-1)return 0; // one breath immediately before the drop
    if(channel==1)return 0;
    if(channel==2){const unsigned position=(finale?width:0)+step,total=width*2;unsigned spacing=position>=total*3/4?1:position>=total/2?2:4;return step%spacing==0?58+position*56/total:0;}
    if(channel==3)return phase%4==2?64:0; // steady support, not a separate first roll
    if(channel==4||channel==5)return tail&&phase%2==0?78:0;
  }
  if(s==Section::Fill){
    if(finale&&step==width-1)return 0; // one clean breath at the end of the pair
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
