#pragma once
#include <stdint.h>

// Set to 0 to omit remote input, display capture, menus and telemetry.
#ifndef TOERN_REMOTE_CONTROL
#define TOERN_REMOTE_CONTROL 1
#endif

// Transport-independent remote state. Physical holds are never cleared by a
// remote release or lease expiry. No allocation and bounded work per poll.
struct ToernRemoteInput {
  static constexpr uint32_t leaseMs = 3000;
  bool physical[4] = {}, remote[4] = {}, touches[5] = {};
  int16_t turns[4] = {};
  uint32_t lastSeen = 0;
  bool live = false;

  void renew(uint32_t now) { lastSeen = now; live = true; }
  bool expired(uint32_t now) const {
    return live && uint32_t(now - lastSeen) >= leaseMs;
  }
  bool held(int i) const { return physical[i] || remote[i]; }
  bool pending() const {
    for (int i = 0; i < 4; ++i) if (turns[i]) return true;
    return false;
  }
  bool enqueue(int i, int delta) {
    if (i < 0 || i >= 4 || !delta || delta < -64 || delta > 64) return false;
    // One outstanding turn per encoder; don't silently combine/reorder gestures.
    if (turns[i]) return false;
    turns[i] = (int16_t)delta;
    return true;
  }
};
