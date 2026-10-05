#ifndef TOERN_PCM_INTERPOLATION_H
#define TOERN_PCM_INTERPOLATION_H
#include <cstdint>

// Cubic interpolation may exceed PCM16 even when all source samples are valid.
// Saturate in floating point BEFORE conversion; converting the oversized result
// directly to int16_t is undefined and can turn a positive peak into negative noise.
namespace toern_audio {

inline int16_t interpolatePcm16(int16_t a, int16_t b, int16_t c, int16_t d, float x) {
    const float result =
        a * ((x - 1.0f) * (x - 2.0f) * (x - 3.0f) / -6.0f) +
        b * (x * (x - 2.0f) * (x - 3.0f) / 2.0f) +
        c * (x * (x - 1.0f) * (x - 3.0f) / -2.0f) +
        d * (x * (x - 1.0f) * (x - 2.0f) / 6.0f);
    if (result > 32767.0f) { return 32767; }
    if (result < -32768.0f) { return -32768; }
    return static_cast<int16_t>(result);
}
}
#endif // TOERN_PCM_INTERPOLATION_H
