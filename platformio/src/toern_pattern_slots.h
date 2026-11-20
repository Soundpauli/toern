#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace toern_pattern_slots {
constexpr uint8_t count = 8;
constexpr const char *dir = "patterns";

// A = patterns/<n>/<n>.txt; B-H = patterns/<n>/<n>-B.txt … patterns/<n>/<n>-H.txt.
inline bool familyDir(char *out, size_t size, unsigned file) {
  if (file < 1 || file > 999 || !out || !size) return false;
  const int length = snprintf(out, size, "%s/%u", dir, file);
  return length >= 0 && (size_t)length < size;
}

inline bool path(char *out, size_t size, unsigned file, uint8_t slot) {
  if (file < 1 || file > 999 || slot >= count || !size) return false;
  const int length = (slot != 0)
      ? snprintf(out, size, "%s/%u/%u-%c.txt", dir, file, file, 'A' + slot)
      : snprintf(out, size, "%s/%u/%u.txt", dir, file, file);
  return length >= 0 && (size_t)length < size;
}

// Flat patterns/<n>.txt layout (pre per-number folders).
inline bool flatPath(char *out, size_t size, unsigned file, uint8_t slot) {
  if (file < 1 || file > 999 || slot >= count || !size) return false;
  const int length = (slot != 0)
      ? snprintf(out, size, "%s/%u-%c.txt", dir, file, 'A' + slot)
      : snprintf(out, size, "%s/%u.txt", dir, file);
  return length >= 0 && (size_t)length < size;
}

// Legacy root paths (pre-patterns/ folder) for migration and read fallback.
inline bool legacyPath(char *out, size_t size, unsigned file, uint8_t slot) {
  if (file < 1 || file > 999 || slot >= count || !size) return false;
  const int length = (slot != 0)
      ? snprintf(out, size, "%u-%c.txt", file, 'A' + slot)
      : snprintf(out, size, "%u.txt", file);
  return length >= 0 && (size_t)length < size;
}

// Resolve nested → flat patterns/ → root. Copies the first existing path into out.
template <class Exists>
inline bool resolve(char *out, size_t size, unsigned file, uint8_t slot, Exists exists) {
  if (!path(out, size, file, slot)) return false;
  if (exists(out)) return true;
  char alt[40];
  if (flatPath(alt, sizeof(alt), file, slot) && exists(alt)) {
    if (strlen(alt) + 1 > size) return false;
    memcpy(out, alt, strlen(alt) + 1);
    return true;
  }
  if (legacyPath(alt, sizeof(alt), file, slot) && exists(alt)) {
    if (strlen(alt) + 1 > size) return false;
    memcpy(out, alt, strlen(alt) + 1);
    return true;
  }
  return false;
}

// Accept nested, flat, or bare names. Never -A, temps, backups or leading zeros.
inline bool parse(const char *name, unsigned &file, uint8_t &slot) {
  const char *p = name;
  // Allow "patterns/1/1-B.txt", "patterns/1-B.txt", or bare "1-B.txt" / "1/1.txt".
  if (!strncmp(p, "patterns/", 9)) p += 9;
  file = 0; slot = 0;
  if (*p < '1' || *p > '9') return false;
  while (*p >= '0' && *p <= '9') {
    file = file * 10 + (*p++ - '0');
    if (file > 999) return false;
  }
  if (*p == '/') {
    ++p;
    unsigned again = 0;
    if (*p < '1' || *p > '9') return false;
    while (*p >= '0' && *p <= '9') {
      again = again * 10 + (*p++ - '0');
      if (again > 999) return false;
    }
    if (again != file) return false;
  }
  if (*p == '-') {
    ++p;
    const char letter = *p >= 'b' && *p <= 'h' ? *p - ('a' - 'A') : *p;
    if (letter < 'B' || letter > 'H') return false;
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
