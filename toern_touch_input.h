#pragma once
#include <stdint.h>

// One timing policy for REC, audition, FN and the five TOUCHY pads.
static constexpr uint32_t TOUCH_PRESS_MS = 10;
static constexpr uint32_t TOUCH_RELEASE_MS = 30;

struct TouchDebouncer {
  bool held = false;
  bool timing = false;
  uint32_t disagreeSince = 0;

  bool update(bool raw, uint32_t now) {
    if (raw == held) {
      timing = false;
      return held;
    }
    if (!timing) {
      timing = true;
      disagreeSince = now;
    }
    const uint32_t need = held ? TOUCH_RELEASE_MS : TOUCH_PRESS_MS;
    if (uint32_t(now - disagreeSince) >= need) {
      held = raw;
      timing = false;
    }
    return held;
  }
};
