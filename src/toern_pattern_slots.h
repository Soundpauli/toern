#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace toern_pattern_slots {
constexpr uint8_t count = 5;

// Slot A aliases the legacy unsuffixed pattern until a family is created.
inline bool path(char *out, size_t size, unsigned file, uint8_t slot,
                 bool lettered) {
  if (file < 1 || file > 999 || slot >= count || !size) return false;
  const int length = (lettered || slot != 0)
      ? snprintf(out, size, "%u-%c.txt", file, 'A' + slot)
      : snprintf(out, size, "%u.txt", file);
  return length >= 0 && (size_t)length < size;
}

inline bool atBoundary(bool wholeCycle, unsigned nextBeat, unsigned pageWidth) {
  return wholeCycle ? nextBeat == 1
      : pageWidth && nextBeat >= 1 && (nextBeat - 1) % pageWidth == 0;
}

inline uint8_t select(uint8_t current, int delta) {
  const int next = (int)current + delta;
  return (uint8_t)(next < 0 ? 0 : next >= count ? count - 1 : next);
}
} // namespace toern_pattern_slots
