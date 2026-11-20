#pragma once
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// KIT WAVs live under kits/<pack>/<voice>.wav (pack 0–999, voice 1–8).
// Legacy root <pack>/<voice>.wav is still accepted when reading.
namespace toern_kit_paths {
inline bool dir(char *out, size_t size, unsigned pack) {
  if (pack > 999 || !out || !size) return false;
  const int n = snprintf(out, size, "kits/%u", pack);
  return n >= 0 && (size_t)n < size;
}

inline bool wav(char *out, size_t size, unsigned pack, unsigned voice) {
  if (pack > 999 || voice < 1 || voice > 8 || !out || !size) return false;
  const int n = snprintf(out, size, "kits/%u/%u.wav", pack, voice);
  return n >= 0 && (size_t)n < size;
}

inline bool legacyWav(char *out, size_t size, unsigned pack, unsigned voice) {
  if (pack > 999 || voice < 1 || voice > 8 || !out || !size) return false;
  const int n = snprintf(out, size, "%u/%u.wav", pack, voice);
  return n >= 0 && (size_t)n < size;
}

template <class Exists>
inline bool resolveWav(char *out, size_t size, unsigned pack, unsigned voice, Exists exists) {
  if (!wav(out, size, pack, voice)) return false;
  if (exists(out)) return true;
  char legacy[24];
  if (!legacyWav(legacy, sizeof(legacy), pack, voice) || !exists(legacy)) return false;
  if (strlen(legacy) + 1 > size) return false;
  memcpy(out, legacy, strlen(legacy) + 1);
  return true;
}
}  // namespace toern_kit_paths
