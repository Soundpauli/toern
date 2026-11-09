#pragma once
#include <stdint.h>
#include <stdio.h>
#include <algorithm>

// Pixel layouts shared by boot/runtime PATT/KIT operations. No I/O, waits,
// allocations or display-driver calls: animation advances from the supplied time.
namespace toern_storage_visual {
struct Color {uint8_t r,g,b;};
constexpr unsigned resultDuration=720;
template<class Canvas> void progress(Canvas &c,unsigned width,bool kit,bool saving,unsigned percent,uint32_t now) {
  percent=std::min(percent,100u);
  const Color accent=saving?Color{190,0,0}:Color{0,170,0};
  const int left=(int(width)-16)/2+1;
  c.clear();
  c.icon(kit,left+(kit?1:0),16,(now/100)%7);
  // Out of the icon to load; into the icon to save. A travelling highlight
  // makes direction readable without requiring a LOAD/SAVE label.
  const int x0=left+8,x1=left+14,head=saving?x0:x1,sign=saving?1:-1;
  for(int x=x0;x<=x1;++x)c.pixel(x,13,{uint8_t(accent.r/3),uint8_t(accent.g/3),0});
  for(int arm=0;arm<=2;++arm){c.pixel(head+sign*arm,13+arm,accent);c.pixel(head+sign*arm,13-arm,accent);}
  const int travel=(now/75)%5;
  c.pixel(saving?x1-travel:x0+travel,13,{180,180,180});
  char label[6];snprintf(label,sizeof(label),"%02u%%",percent);
  c.text(label,1+(int(width)-c.textWidth(label))/2,4,{160,175,195});
  const unsigned span=width-2,filled=percent*span/100;
  for(unsigned i=0;i<span;++i) {
    const bool on=i<filled;
    c.pixel(i+2,2,on?accent:Color{8,12,20});
    c.pixel(i+2,1,on?Color{uint8_t(accent.r/5),uint8_t(accent.g/5),0}:Color{3,5,8});
  }
  if(filled)c.pixel(filled+1,2,{190,205,220});
}
template<class Canvas> void result(Canvas &c,unsigned width,bool success,bool white,uint32_t elapsed,bool overlay=false) {
  if(!overlay)c.clear();
  elapsed=std::min<uint32_t>(elapsed,resultDuration);
  unsigned brightness=elapsed<100?50+elapsed*130/100:elapsed>540?(resultDuration-elapsed)*180/180:180;
  const Color color=white&&success?Color{uint8_t(brightness),uint8_t(brightness),uint8_t(brightness)}:
      success?Color{0,uint8_t(brightness),0}:Color{uint8_t(brightness),0,0};
  const int cx=width/2,cy=8,sx=width>=24?2:1;
  if(success) {
    // Same single-pixel check as filter reset and the original save animation.
    static const int8_t points[][2]={{-3,-1},{-2,-2},{-1,-3},{0,-2},{1,-1},{2,0},{3,1},{4,2}};
    const unsigned reveal=std::min<unsigned>(8u,elapsed*8/420);
    for(unsigned i=0;i<reveal;++i) {
      c.pixel(cx+points[i][0],cy+points[i][1],color);
    }
  } else {
    const int radius=std::min<unsigned>(3u,elapsed/60);
    for(int i=-radius;i<=radius;++i)for(int dx=0;dx<sx;++dx) {
      c.pixel(cx+i*sx+dx,cy+i,color);c.pixel(cx+i*sx+dx,cy-i,color);
    }
  }
}
} // namespace toern_storage_visual
