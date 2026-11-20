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

// MV <from> <to>. Both paths are absolute (start with '/'); names may contain
// spaces. Split on the last " /" so the destination is the final absolute path.
inline bool parseMvArgs(char *args, char *&from, char *&to) {
  if (!args) return false;
  while (isSpace(*args)) ++args;
  if (*args != '/') return false;
  char *split = nullptr;
  for (char *p = args + 1; *p; ++p) {
    if (isSpace(*p) && p[1] == '/') split = p;
  }
  if (!split) return false;
  *split = '\0';
  from = args;
  to = split + 1;
  while (isSpace(*to)) ++to;
  char *end = from + std::strlen(from);
  while (end > from && isSpace(end[-1])) *--end = '\0';
  end = to + std::strlen(to);
  while (end > to && isSpace(end[-1])) *--end = '\0';
  return *from == '/' && *to == '/' && std::strcmp(from, "/") != 0 && std::strcmp(to, "/") != 0;
}
}  // namespace toern_sd
