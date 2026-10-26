#pragma once
#include <stdint.h>
namespace toern_demo {
// Five-column M: upright stems and two diagonals meeting at the center.
constexpr uint8_t mColumns[] = {0x1f, 0x02, 0x04, 0x02, 0x1f};
inline int glyphWidth(char c, int fallback) { return c == 'M' ? 5 : fallback; }
enum class Phase { Nyan, Red, White, Rainbow };
struct Frame { Phase phase; uint32_t elapsed; int x; };
inline Frame frame(uint32_t elapsed, int columns, int textWidth) {
  const uint32_t scroll = (columns + textWidth + 1) * 65u;
  uint32_t t = elapsed % (10000u + 3u * scroll);
  if (t < 10000u) return {Phase::Nyan, t, 0};
  t -= 10000u;
  const Phase passes[] = {Phase::Red, Phase::White, Phase::Rainbow};
  for (Phase phase : passes) {
    if (t < scroll) return {phase, t, columns + 1 - int(t / 65u)};
    t -= scroll;
  }
  return {Phase::Nyan, 0, 0};
}
}
