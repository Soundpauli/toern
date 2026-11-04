#pragma once
#include <cmath>
#include <cstdint>

namespace toern_audio {
// One window of lookahead. Sum before limiting, never saturate individual inputs.
// The previous window already ramps toward this window's safe gain, so gain
// changes cannot introduce a discontinuity at a window boundary.
template <unsigned N> class MixLimiter {
public:
  static_assert(N > 1, "Limiter interpolation needs at least two samples");
  static constexpr float ceiling = 32700.0f;
  void process(const float *incoming, int16_t *output) {
    float peak = 0.0f;
    for (unsigned i = 0; i < N; ++i) peak = fmaxf(peak, fabsf(incoming[i]));
    const float nextSafe = peak > ceiling ? ceiling / peak : 1.0f;
    const float allowed = fminf(pendingSafe_, nextSafe);
    // 50 ms recovery at 44.1 kHz, constant block-size-independent time scale.
    const float recovery = 1.0f - expf(-(float)N / (44100.0f * 0.050f));
    const float end = fminf(allowed, gain_ + (1.0f - gain_) * recovery);
    for (unsigned i = 0; i < N; ++i) {
      const float ramp = gain_ + (end - gain_) * ((float)i / (float)(N - 1));
      const float value = pending_[i] * ramp;
      output[i] = (int16_t)lrintf(fmaxf(-32768.0f, fminf(32767.0f, value)));
      pending_[i] = incoming[i];
    }
    // This is the gain at the start of the next, already buffered window.
    gain_ = fminf(end, nextSafe);
    pendingSafe_ = nextSafe;
  }
private:
  float pending_[N] = {};
  float pendingSafe_ = 1.0f;
  float gain_ = 1.0f;
};
}
