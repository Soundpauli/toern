#pragma once
#include <cmath>

namespace toern_audio {
// Sample SUST can sit at 0 so decay can close the hit. Synths keep a floor of 1
// so a held note cannot disappear.
inline int sustainSliderMin(int ch) {
  return (ch >= 1 && ch <= 8) ? 0 : 1;
}

// Keep the stored/UI range unchanged, with finer attack-time control near 32.
constexpr float kAttackMaxMs = 500.0f;
inline float attackMilliseconds(float slider) {
  if (!std::isfinite(slider)) slider = 32.0f;
  slider = fmaxf(0.0f, fminf(32.0f, slider));
  const float slow = (32.0f - slider) / 32.0f;
  return kAttackMaxMs * slow * slow;
}
}
