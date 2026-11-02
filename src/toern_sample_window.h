#pragma once
#include <stdint.h>

namespace toern_sample {
struct Window { uint32_t first, end; };

// Half-open PCM frame window. 0/100 (and legacy end=0) includes EVERY frame,
// including odd frame counts and quiet tails. Never hide an end trim in here.
inline Window window(uint32_t frames, unsigned startPercent, unsigned endPercent) {
  if (!frames) return {0, 0};
  if (startPercent > 99) startPercent = 99;
  if (!endPercent || endPercent > 100) endPercent = 100;
  if (endPercent <= startPercent) endPercent = startPercent + 1;
  const uint32_t first = (uint64_t(frames)*startPercent)/100;
  uint32_t end = (uint64_t(frames)*endPercent)/100;
  if (end <= first) end = first + 1;
  return {first, end};
}
}
