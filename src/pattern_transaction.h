#pragma once

// Explicit slot deletion removes its recovery files too. Empty saves keep files.
// Remove the destination last so a failed cleanup keeps the current pattern.
template <class FS>
bool deletePatternTransaction(FS &fs, const char *dest, const char *tmp,
                              const char *backup) {
  if (fs.exists(tmp) && !fs.remove(tmp)) return false;
  if (fs.exists(backup) && !fs.remove(backup)) return false;
  return !fs.exists(dest) || fs.remove(dest);
}

// Destination-specific recovery; filesystem operations can fail independently.
// A valid destination always wins over an uncommitted temporary file.
template <class FS, class Validate>
bool recoverPatternTransaction(FS &fs, const char *dest, const char *tmp,
                               const char *backup, Validate valid) {
  if (valid(dest, false)) return true;
  const char *candidate = valid(tmp, true) ? tmp :
                          valid(backup, false) ? backup : nullptr;
  if (!candidate) return false;
  if (fs.exists(dest) && !fs.remove(dest)) return false;
  return fs.rename(candidate, dest);
}

template <class FS, class Validate>
bool commitPatternTransaction(FS &fs, const char *dest, const char *tmp,
                              const char *backup, Validate valid) {
  if (!valid(tmp, true)) return false;
  if (fs.exists(dest)) {
    // Never delete the destination to make room. Keep the previous generation
    // until the new generation is installed and checked.
    if (fs.exists(backup) && !fs.remove(backup)) return false;
    if (!fs.rename(dest, backup)) return false;
  }
  if (!fs.rename(tmp, dest)) {
    if (!fs.exists(dest) && fs.exists(backup)) fs.rename(backup, dest);
    return false;
  }
  if (!valid(dest, true)) return false;
  // Retain the previous generation for recovery from subsequent card errors.
  return true;
}
