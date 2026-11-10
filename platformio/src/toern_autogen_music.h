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
enum class Section {Groove,Variation,Break,Build,Fill,Drop,Solo};
// AGEN menu order. LONG keeps the drawn motif (fitted to the chord only) and
// doubles the groove, variation and drop slots so the theme can settle in.
// DEEP is minimal deep house: the same long grooves, with a pulse bass,
// offbeat hats and a few minor-seven stabs instead of a busy arrangement.
enum Motion : uint8_t {MotionHold=0,MotionSoft=1,MotionLong=2,MotionLive=3,MotionDeep=4};
// Menu 0 is AUTO (motif major/minor). Explicit picks follow.
enum Scale : uint8_t {ScaleMajor=0,ScaleMinor,ScaleHarmonic,ScaleDorian,ScalePenta,ScaleBlues,ScaleCount};
AGEN_MUSIC_CODE static Scale defaultScale(bool minor){return minor?ScaleMinor:ScaleMajor;}
AGEN_MUSIC_CODE static unsigned scaleCount(Scale sc){
  static const uint8_t n[] AGEN_MUSIC_DATA={7,7,7,7,5,6};
  return n[(unsigned)sc%6];
}
AGEN_MUSIC_CODE static int scalePc(int tonic,Scale sc,unsigned d){
  static const uint8_t major[] AGEN_MUSIC_DATA={0,2,4,5,7,9,11};
  static const uint8_t naturalMinor[] AGEN_MUSIC_DATA={0,2,3,5,7,8,10};
  static const uint8_t harmonic[] AGEN_MUSIC_DATA={0,2,3,5,7,8,11};
  static const uint8_t dorian[] AGEN_MUSIC_DATA={0,2,3,5,7,9,10};
  static const uint8_t penta[] AGEN_MUSIC_DATA={0,3,5,7,10};
  static const uint8_t blues[] AGEN_MUSIC_DATA={0,3,5,6,7,10};
  const uint8_t *table=major;
  if(sc==ScaleMinor)table=naturalMinor;
  else if(sc==ScaleHarmonic)table=harmonic;
  else if(sc==ScaleDorian)table=dorian;
  else if(sc==ScalePenta)table=penta;
  else if(sc==ScaleBlues)table=blues;
  const unsigned n=scaleCount(sc);
  return (tonic+table[d%n])%12;
}
// One 64-bar cycle. A short rise (Build) resolves by the kick and bass coming
// back in the following groove. The Break is a melodic solo: every drum rests
// for eight bars. The groove then returns for four bars only and moves straight
// on into a variation, so the stretch after the solo does not mark time. The
// Solo is a drum solo: every tonal voice rests. The fill and drop happen once,
// at the end of the cycle, rather than every 16 bars.
AGEN_MUSIC_CODE static Section section(unsigned generation){
  static const Section form[] AGEN_MUSIC_DATA={
      Section::Groove,Section::Variation,Section::Build,Section::Groove,
      Section::Variation,Section::Break,Section::Groove,Section::Variation,
      Section::Solo,Section::Build,Section::Fill,Section::Drop};
  return form[(generation/2)%12];
}
AGEN_MUSIC_CODE static unsigned phraseBars(unsigned generation){
  static const uint8_t bars[] AGEN_MUSIC_DATA={8,8,2,8,8,8,4,8,2,2,2,4};
  return bars[(generation/2)%12];
}
// Note condition (1/N) that sounds only on the last of `passes` plays of a page.
AGEN_MUSIC_CODE static unsigned lastPassCondition(unsigned passes){
  return passes==2||passes==4||passes==8||passes==16?passes:1;
}
// `generation` selects the slot length. A roll is never repeated: Build and Fill
// stay one pass of the call and one of the answer. Lengths are musical bars for
// the whole pair; slow tempos halve them so a slot cannot run on past half a minute.
AGEN_MUSIC_CODE static unsigned repeats(Section s,unsigned generation,unsigned width=16,unsigned bpm=120,bool longForm=false){
  if(s==Section::Build||s==Section::Fill)return 1;
  unsigned bars=phraseBars(generation);
  if(longForm&&(s==Section::Groove||s==Section::Variation||s==Section::Drop))bars*=2;
  const unsigned limit=longForm?48000u:24000u;
  if(bpm<40)bpm=40;
  while(bars>1&&bars*240000u>bpm*limit)bars/=2;
  const unsigned passes=(bars*16u)/(2u*((width==32)?32u:16u));
  return passes?passes:1;
}
// Eight-pixel cue for the top row. Red fills toward the next Break. Once that
// break has started, orange fills toward the Fill, then red resumes toward the
// break of the next cycle. `step` is the step inside `generation`; `generationSteps`
// is that generation's real length in steps (already scheduled passes).
struct FormCue { unsigned pixels; bool fill; };
AGEN_MUSIC_CODE static unsigned scheduledSteps(unsigned generation,unsigned width,unsigned bpm,bool longForm){
  const unsigned columns=width==32?32u:16u;
  return repeats(section(generation),generation,columns,bpm,longForm)*columns;
}
AGEN_MUSIC_CODE static FormCue formCue(unsigned generation,unsigned step,unsigned generationSteps,unsigned width,unsigned bpm,bool longForm){
  const unsigned cycle=generation/24u*24u;
  const unsigned index=generation-cycle;
  unsigned origin[25]={};unsigned at=0;
  for(unsigned i=0;i<24;++i){
    origin[i]=at;
    unsigned len=i==index?(generationSteps?generationSteps:1u):scheduledSteps(cycle+i,width,bpm,longForm);
    if(!len)len=1;
    at+=len;
  }
  origin[24]=at;
  const unsigned cycleLen=at?at:1;
  unsigned pos=origin[index]+(generationSteps&&step<generationSteps?step:generationSteps?generationSteps-1:0);
  if(pos>=cycleLen)pos=cycleLen-1;
  unsigned breakAt=0,fillAt=0;bool sawBreak=false,sawFill=false;
  for(unsigned i=0;i<24;++i){
    const Section part=section(cycle+i);
    if(part==Section::Break&&!sawBreak){breakAt=origin[i];sawBreak=true;}
    if(part==Section::Fill&&!sawFill){fillAt=origin[i];sawFill=true;}
  }
  unsigned start=0,end=breakAt?breakAt:cycleLen;bool fill=false;
  if(pos>=fillAt){start=fillAt;end=cycleLen+breakAt;}
  else if(pos>=breakAt){start=breakAt;end=fillAt;fill=true;}
  const unsigned span=end>start?end-start:1;
  const unsigned elapsed=pos>=start?pos-start:0;
  unsigned pixels=elapsed*8u/(span>1?span-1:1);
  if(pixels>8)pixels=8;
  return {pixels,fill};
}
AGEN_MUSIC_CODE static unsigned degree(Scale sc,unsigned phrase){
  static const uint8_t major[] AGEN_MUSIC_DATA={0,5,3,4};
  static const uint8_t naturalMinor[] AGEN_MUSIC_DATA={0,5,3,6};
  static const uint8_t compact[] AGEN_MUSIC_DATA={0,3,2,4};
  // Hold harmony through break/build/fill, then resolve the drop to the tonic.
  static const uint8_t harmonicSlots[] AGEN_MUSIC_DATA={0,1,2,3,3,0,1,2};
  const unsigned slot=harmonicSlots[(phrase/2)%8];
  if(sc==ScalePenta||sc==ScaleBlues)return compact[slot];
  return ((sc==ScaleMinor||sc==ScaleHarmonic)?naturalMinor:major)[slot];
}
AGEN_MUSIC_CODE static unsigned degree(bool minor,unsigned phrase){return degree(defaultScale(minor),phrase);}
AGEN_MUSIC_CODE static int scalePc(int tonic,bool minor,unsigned d){return scalePc(tonic,defaultScale(minor),d);}
// `lock` >= 0 pins the chord to a scale degree read from the drawn progression;
// -1 follows the phrase progression above.
AGEN_MUSIC_CODE static int chordPc(int tonic,Scale sc,unsigned phrase,unsigned tone,int lock=-1){
  return scalePc(tonic,sc,(lock>=0?(unsigned)lock:degree(sc,phrase))+tone*2);
}
// Scale degree whose stacked chord best explains drawn pitch classes (bit n = pc n).
// The lowest drawn note breaks ties as the root, so inversions stay recognisable.
AGEN_MUSIC_CODE static int drawnDegree(int tonic,Scale sc,unsigned pcs,int lowestPc){
  static const int8_t weight[] AGEN_MUSIC_DATA={5,3,3,1};
  int best=0,bestScore=-999;
  for(unsigned d=0;d<scaleCount(sc);++d){
    unsigned tones=0;int score=0;
    for(unsigned t=0;t<4;++t){
      const unsigned bit=1u<<scalePc(tonic,sc,d+t*2);
      if(tones&bit)continue;
      tones|=bit;if(pcs&bit)score+=weight[t];
    }
    for(unsigned pc=0;pc<12;++pc)if((pcs&(1u<<pc))&&!(tones&(1u<<pc)))score-=3;
    if(scalePc(tonic,sc,d)==lowestPc)score+=2;
    if(score>bestScore){bestScore=score;best=(int)d;}
  }
  return best;
}
AGEN_MUSIC_CODE static int chordPc(int tonic,bool minor,unsigned phrase,unsigned tone){return chordPc(tonic,defaultScale(minor),phrase,tone);}
AGEN_MUSIC_CODE static int nearestPc(int reference,int pc){
  int best=reference,distance=999;
  for(int n=0;n<=127;++n){int delta=n-reference;if(delta<0)delta=-delta;
    if(n%12==pc&&delta<distance){best=n;distance=delta;}}
  return best;
}
AGEN_MUSIC_CODE static int fitPitch(int reference,int tonic,Scale sc,unsigned phrase,unsigned channel,unsigned step,int lock=-1){
  const unsigned tones=step%4==0?3:scaleCount(sc);
  if((channel==6||channel==14)&&step%4==0)return nearestPc(reference,chordPc(tonic,sc,phrase,0,lock));
  int best=reference,distance=999;
  for(unsigned i=0;i<tones;++i){
    int pc=step%4==0?chordPc(tonic,sc,phrase,i,lock):scalePc(tonic,sc,i);
    int n=nearestPc(reference,pc),delta=n-reference;if(delta<0)delta=-delta;
    if(delta<distance){distance=delta;best=n;}
  }
  return best;
}
AGEN_MUSIC_CODE static int fitPitch(int reference,int tonic,bool minor,unsigned phrase,unsigned channel,unsigned step){
  return fitPitch(reference,tonic,defaultScale(minor),phrase,channel,step);
}
// Bass stays a root groove: downbeats lock to the chord, offbeats only octave-jump.
AGEN_MUSIC_CODE static bool bassHit(unsigned step,unsigned density,uint32_t seed,bool deep=false){
  if(deep)return step%4==0; // one note per beat: the kick and the bass share the pulse
  if(step%4==0)return true;
  if(!density)return false;
  const uint32_t h=(seed*1664525u+step*1013904223u)>>8;
  unsigned chance=density==1?22:density==2?48:72;
  if(step%2)chance=chance*2/3;
  return h%100<chance;
}
AGEN_MUSIC_CODE static int bassPitch(int reference,int tonic,Scale sc,unsigned phrase,unsigned step,uint32_t seed,int lock=-1,bool deep=false){
  const int root=nearestPc(reference,chordPc(tonic,sc,phrase,0,lock));
  if(deep||step%4==0)return root; // deep house stays on the low root; no octave jumps
  const uint32_t h=(seed+step*977u)>>8;
  if(h%4==0)return nearestPc(reference+12,chordPc(tonic,sc,phrase,0,lock));
  return root;
}
// Voice 6 is a bass sample tuned at C2, with lifts into C3. Fold a concert pitch
// into that octave: `upper` selects C3, otherwise C2.
AGEN_MUSIC_CODE static int bassSamplePitch(int pitch,bool upper){
  const int low=upper?48:36;
  while(pitch<low)pitch+=12;
  while(pitch>=low+12)pitch-=12;
  return pitch;
}
// Lead only: HOLD/LONG keep the fitted motif, SOFT moves by a second, LIVE uses the contour.
// density 0..3 further caps how often the contour fires (0 sparse, 3 full).
AGEN_MUSIC_CODE static int melodyPitch(int reference,int tonic,Scale sc,unsigned phrase,
                                      unsigned channel,unsigned step,uint32_t seed,unsigned motion=MotionLive,unsigned density=2,int lock=-1){
  const Section s=section(phrase);
  const int baseline=fitPitch(reference,tonic,sc,phrase,channel,step,lock);
  if(motion==MotionHold||motion==MotionLong||motion==MotionDeep||((channel==6||channel==14)&&step%4==0))return baseline;
  uint32_t h=seed^(phrase/2*374761393u)^(channel*668265263u)^(step*2246822519u);
  h=(h^(h>>13))*1274126177u;h^=h>>16;
  unsigned chance=s==Section::Variation||s==Section::Fill?90:
      s==Section::Build?80:s==Section::Break?65:s==Section::Drop?45:40;
  if(motion==MotionSoft&&chance>35)chance=35;
  if(density==0&&chance>18)chance=18;
  else if(density==1&&chance>32)chance=32;
  if(h%100>=chance)return baseline;
  static const int8_t contour[] AGEN_MUSIC_DATA={2,4,3,7,5,-2,4,-5};
  int delta=contour[((step/2)+channel+(phrase/2))%8];
  if(phrase&1)delta=-delta;
  if(motion==MotionSoft)delta=delta>0?2:-2;
  const int target=reference+delta;
  int best=baseline,distance=999;
  const unsigned allowedTones=step%4==0?3u:scaleCount(sc);
  for(int n=reference-12;n<=reference+12;++n){
    if(n<0||n>127)continue;
    bool allowed=false;
    for(unsigned d=0;d<allowedTones;++d)
      allowed|=n%12==(step%4==0?chordPc(tonic,sc,phrase,d,lock):scalePc(tonic,sc,d));
    int gap=n-target;if(gap<0)gap=-gap;
    if(allowed&&gap<distance){best=n;distance=gap;}
  }
  return best;
}
AGEN_MUSIC_CODE static int melodyPitch(int reference,int tonic,bool minor,unsigned phrase,
                                      unsigned channel,unsigned step,uint32_t seed,unsigned motion=MotionLive,unsigned density=2){
  return melodyPitch(reference,tonic,defaultScale(minor),phrase,channel,step,seed,motion,density);
}
// tones 3 = triad, 4 = seventh. A stable voicing takes each chord tone nearest the drawn register.
AGEN_MUSIC_CODE static int voicingPitch(int reference,int tonic,Scale sc,unsigned phrase,unsigned lane,uint32_t seed,bool stable=false,unsigned tones=3,int lock=-1){
  if(tones<3)tones=3;if(tones>4)tones=4;
  if(stable)return nearestPc(reference,chordPc(tonic,sc,phrase,lane%tones,lock));
  const unsigned inversion=(phrase+seed)%tones;
  const int root=nearestPc(reference,chordPc(tonic,sc,phrase,0,lock));
  int notes[4];
  for(unsigned i=0;i<tones;++i){
    const unsigned tone=(i+inversion)%tones;
    notes[i]=root+(chordPc(tonic,sc,phrase,tone,lock)-chordPc(tonic,sc,phrase,0,lock)+12)%12+(tone<inversion?12:0);
  }
  int center=0;for(unsigned i=0;i<tones;++i)center+=notes[i];center/=(int)tones;
  int shift=0;
  while(center+shift>reference+6)shift-=12;
  while(center+shift<reference-6)shift+=12;
  while(notes[0]+shift<24)shift+=12;
  while(notes[tones-1]+shift>108)shift-=12;
  return notes[lane%tones]+shift;
}
AGEN_MUSIC_CODE static int voicingPitch(int reference,int tonic,bool minor,unsigned phrase,unsigned lane,uint32_t seed,bool stable=false,unsigned tones=3){
  return voicingPitch(reference,tonic,defaultScale(minor),phrase,lane,seed,stable,tones);
}
// AGEN notes use a musical register, then undo only the patch's octave offset.
// Fine tuning and the user's patch stay intact; never apply the register twice.
AGEN_MUSIC_CODE static int encodePitch(int pitch,int octaveShift){
  pitch-=octaveShift;
  while(pitch<0)pitch+=12;
  while(pitch>127)pitch-=12;
  return pitch;
}
// A melodic break needs a melodic voice to carry it and a drum solo needs drums;
// without them the form keeps playing instead of going silent.
AGEN_MUSIC_CODE static Section supportedSection(Section requested,unsigned foundation,bool drums=true){
  if(requested==Section::Solo)return drums?requested:Section::Variation;
  if(!foundation&&(requested==Section::Build||requested==Section::Break||requested==Section::Fill))return Section::Groove;
  if(requested==Section::Break&&foundation<=5)return Section::Groove;
  return requested;
}
// Voices AGEN may add when the motif leaves them empty. They enter in stages so
// the arrangement grows: hat and chords first, the beat (both kicks and the
// sample bass) after the first rise, colour later, everything on the drop.
// From the second cycle on the beat is already established. Voice 8 is not a
// guest here: vocals have their own climax schedule.
AGEN_MUSIC_CODE static bool guestWanted(unsigned channel,unsigned generation,Section s,bool deep=false){
  const unsigned slot=(generation/2)%12;const bool established=generation>=24;
  // Deep house leaves the tom, the second kick, the clap and the keys line out.
  // The backbeat arrives with the drop, not as a fill.
  if(deep&&(channel==4||channel==5||channel==7||channel==13))return false;
  if(deep&&channel==2)return s==Section::Drop;
  if(channel==3||channel==11)return true;
  const bool guest=channel==1||channel==2||channel==4||channel==5||channel==6||channel==7||channel==13||channel==14;
  if(s==Section::Drop)return guest;
  if(channel==2)return established||slot>=3||s==Section::Build||s==Section::Fill;
  // Voice 7 is the second kick and enters with voice 1. Voice 6 is the sample bass.
  if(channel==1||channel==7||channel==6||channel==14)return established||slot>=3;
  if(channel==4||channel==13)return slot>=6;
  if(channel==5)return s==Section::Solo||s==Section::Fill;
  return false;
}
// Voice 8 carries vocals. They are a feature, not a bed: they sing over the drop
// and answer the second half of the melodic solo, and rest everywhere else.
AGEN_MUSIC_CODE static bool vocalWanted(unsigned generation,Section s,bool deep=false){
  if(deep)return s==Section::Drop&&generation%2==0; // one phrase on the drop, then it goes
  return s==Section::Drop||(s==Section::Break&&generation%2==1);
}
// Velocity for a kept hit. A voice drawn at one level gets a metric groove; a
// voice the user shaped keeps its dynamics. The drop leans on the beat.
AGEN_MUSIC_CODE static unsigned accent(Section s,unsigned channel,unsigned step,unsigned velocity,bool flat){
  static const uint8_t drums[] AGEN_MUSIC_DATA={100,78,90,78};
  static const uint8_t tonal[] AGEN_MUSIC_DATA={100,88,94,88};
  const unsigned beat=step%4;
  unsigned level=velocity;
  if(flat){
    unsigned percent=(channel<=5?drums:tonal)[beat];
    if((channel==1||channel==7)&&beat)percent=92; // both kicks stay solid
    if(channel==3&&beat==2)percent=100;   // hats may lean on the offbeat
    level=velocity*percent/100;
  }
  if(s==Section::Drop&&beat==0)level+=level/10;
  return level<1?1:level>127?127:level;
}
AGEN_MUSIC_CODE static bool bassAllowed(Section s,unsigned step,unsigned width,bool finale=true){
  if(s==Section::Break||s==Section::Solo)return false;
  if(s==Section::Build)return false;
  if(s==Section::Fill)return !finale||step<width-2;
  return true;
}
// 0..16 relative to the user's starting level; never boost beyond that ceiling.
AGEN_MUSIC_CODE static unsigned volumeScale(Section s,unsigned ch,bool bass,unsigned progress){
  if(progress>255)progress=255;
  if(s==Section::Solo)return 16; // resting tonal voices ring out; nothing is cut
  if(s==Section::Break){if(ch==1||ch==7||bass)return 0;return ch<=5?7:16;}
  if(s==Section::Build){
    if(ch==1||ch==7||bass)return 0;
    if(ch==2)return 5+progress*11/255;
    if(ch<=5)return 6+progress*9/255;
    return 10+progress*6/255;
  }
  if(s==Section::Fill){
    if(ch==1||ch==7||bass)return 0;
    return ch==2||ch==5?12+progress*4/255:12;
  }
  if(s==Section::Drop)return 16;
  const unsigned triangle=progress<128?progress*2:(255-progress)*2;
  if(ch==3||ch==5)return 11+triangle*5/255;
  if(ch==8||(!bass&&(ch==11||ch==13||ch==14)))return 12+triangle*4/255;
  return 16;
}

// Return -1 to keep what is on the page, 0 to clear, or a velocity to place a hit.
// `hit` says a drawn or generated hit already sits on this step. The groove on
// the page is the one to play: sections thin or decorate it and never swap in a
// stock four-on-the-floor or backbeat.
AGEN_MUSIC_CODE static int drum(Section s,unsigned channel,unsigned step,unsigned width,uint32_t seed,bool finale=true,bool hit=false,bool deep=false){
  if(channel==7)channel=1; // voice 7 is a second kick: same part as voice 1
  if(deep){
    // Kick and drawn hats stay. Everything else is an offbeat hat, plus a soft
    // backbeat only when the drop arrives. No rolls, ghosts or pushed kicks.
    if(s==Section::Break)return 0;
    if(finale&&(s==Section::Build||s==Section::Fill)&&step==width-1)return 0;
    if((s==Section::Build||s==Section::Fill)&&channel==1)return 0;
    if(hit)return -1;
    if(channel==3&&step%4==2)return 54;
    if(channel==2&&s==Section::Drop&&step%8==4)return 72;
    return 0;
  }
  const unsigned phase=step%16;const bool tail=step>=width*3/4;
  const uint32_t h=(seed*1664525u+step*1013904223u+channel*977u)>>8;
  if(s==Section::Break)return 0; // melodic solo: every drum rests
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
    if(tail&&(channel==2||channel==5)){
      // Three turnarounds: a straight roll, hand-to-hand with the tom, or a 3-3-2 push.
      const unsigned offset=step-width*3/4,style=(seed>>3)%3;
      if(!style)return channel==2?76+(offset%4)*10:phase%2==1?86:0;
      const bool snare=style==1?offset%2==0:offset%3==0;
      if(channel==2)return snare?76+(offset%4)*10:0;
      return snare?0:80+(offset%4)*8;
    }
    if(channel==3&&tail)return 68+(step%2)*12;
  }
  if(hit)return -1;
  if(s==Section::Variation){
    if(channel==1&&step==width-2)return 88;
    if(channel==3&&phase%2==1&&h%3==0)return 58;
    if(channel==2&&(phase==7||phase==15)&&h%2==0)return 56;
    if(channel==5&&tail&&phase%2==1)return 78;
  }
  if(s==Section::Solo){
    // The drawn groove plus two pushed kicks, ghosts and toms: off the grid, not on it.
    static const uint8_t push[] AGEN_MUSIC_DATA={3,6,7,10,11,14};
    const unsigned first=seed%6,second=(first+2+(seed/6)%3)%6;
    if(channel==1&&(phase==push[first]||phase==push[second]))return 100;
    if(channel==2&&phase%2==1&&h%3==0)return 54;
    if(channel==3&&phase%2==1&&h%2==0)return 60;
    if(channel==5&&((tail&&phase%2==1)||(phase%4==3&&h%2==0)))return 84;
  }
  return -1;
}
}

#undef AGEN_MUSIC_DATA
#undef AGEN_MUSIC_CODE
