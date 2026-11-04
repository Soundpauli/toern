#pragma once
#include <cmath>

namespace toern_audio {
// Sample SUST can sit at 0 so decay can close the hit. Synths keep a floor of 1
// so a held note cannot disappear.
inline int sustainSliderMin(int ch) {
  return (ch >= 1 && ch <= 8) ? 0 : 1;
}
}
