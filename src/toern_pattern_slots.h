#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace toern_pattern_slots {
constexpr uint8_t count = 5;

// A always uses the original numbered file; B-E use letter suffixes.
inline bool path(char *out, size_t size, unsigned file, uint8_t slot) {
  if (file < 1 || file > 999 || slot >= count || !size) return false;
  const int length = (slot != 0)
      ? snprintf(out, size, "%u-%c.txt", file, 'A' + slot)
      : snprintf(out, size, "%u.txt", file);
  return length >= 0 && (size_t)length < size;
}

// Accept canonical numbered files only, never -A, temps, backups or leading zeros.
inline bool parse(const char *name, unsigned &file, uint8_t &slot) {
  const char *p = name;
  file = 0; slot = 0;
  if (*p < '1' || *p > '9') return false;
  while (*p >= '0' && *p <= '9') {
    file = file * 10 + (*p++ - '0');
    if (file > 999) return false;
  }
  if (*p == '-') {
    ++p;
    const char letter = *p >= 'b' && *p <= 'e' ? *p - ('a' - 'A') : *p;
    if (letter < 'B' || letter > 'E') return false;
    slot = letter - 'A'; ++p;
  }
  return strcmp(p, ".txt") == 0 || strcmp(p, ".TXT") == 0;
}

inline unsigned countSlots(uint8_t mask) {
  unsigned n = 0;
  for (uint8_t slot = 0; slot < count; ++slot) if (mask & (1u << slot)) ++n;
  return n;
}

// Display the extent through the last nonempty letter, retaining A when all are empty.
inline unsigned displayCount(uint8_t nonemptyMask) {
  for(int slot=count-1;slot>=0;--slot)if(nonemptyMask&(1u<<slot))return (unsigned)slot+1;
  return 1;
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
