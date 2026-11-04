#pragma once

#include <cstdint>
#include <cstring>

namespace toern_sd {
inline bool parseUnsigned(const char *text, unsigned base, uint32_t &out) {
  if (!text || !*text) return false;
  uint32_t value = 0;
  for (const char *p = text; *p; ++p) {
    unsigned digit;
    if (*p >= '0' && *p <= '9') digit = *p - '0';
    else if (*p >= 'a' && *p <= 'f') digit = *p - 'a' + 10;
    else if (*p >= 'A' && *p <= 'F') digit = *p - 'A' + 10;
    else return false;
    if (digit >= base || value > (UINT32_MAX - digit) / base) return false;
    value = value * base + digit;
  }
  out = value;
  return true;
}

inline bool isSpace(char c) { return c == ' ' || c == '\t'; }

inline char *popLastToken(char *args) {
  char *end = args + std::strlen(args);
  while (end > args && isSpace(end[-1])) *--end = '\0';
  char *token = end;
  while (token > args && !isSpace(token[-1])) --token;
  if (token == args || token == end) return nullptr;
  *--token = '\0';
  return token + 1;
}

// PUT <path, possibly with spaces> <decimal size> <hex CRC32>.
// Parse the fixed numeric suffix from the right, and reject invalid numbers
// before the caller creates, truncates, or removes anything on the SD card.
inline bool parsePutArgs(char *args, char *&path, uint32_t &size, uint32_t &crc) {
  if (!args) return false;
  while (isSpace(*args)) ++args;
  char *crcText = popLastToken(args);
  char *sizeText = popLastToken(args);
  char *end = args + std::strlen(args);
  while (end > args && isSpace(end[-1])) *--end = '\0';
  if (!*args || !crcText || !sizeText || std::strlen(crcText) > 8 ||
      !parseUnsigned(sizeText, 10, size) || !parseUnsigned(crcText, 16, crc)) return false;
  path = args;
  return true;
}
}  // namespace toern_sd
