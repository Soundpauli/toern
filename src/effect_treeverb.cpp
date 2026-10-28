/* Audio Library for Teensy 3.X
 * Copyright (c) 2018, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "effect_treeverb.h"
#include <arm_math.h> // CMSIS barriers used by StateGuard
#include "toern_audio_runtime.h"

namespace {
#if TOERN_TREEVERB_LIGHT
// Spread the four combs across the original 25–37 ms range. The two
// diffusers retain the longest/shortest stages; tail density will differ.
constexpr uint16_t combLengths[] = {1116, 1277, 1422, 1617};
constexpr uint16_t allpassLengths[] = {556, 225};
#else
constexpr uint16_t combLengths[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
constexpr uint16_t allpassLengths[] = {556, 441, 341, 225};
#endif

// Preserve truncation toward zero, including tiny negative feedback values.
// Shifts are compile-time constants; none of this calls generic sat16().
template<unsigned Shift>
__attribute__((always_inline)) inline int32_t truncateShift(int32_t value) {
    return (value + (value < 0 ? (int32_t)((1u << Shift) - 1u) : 0)) >> Shift;
}

__attribute__((always_inline)) inline int16_t saturate16(int32_t value) {
#if defined(__arm__) && defined(__ARM_ARCH_7EM__)
    int32_t result;
    asm("ssat %0, #16, %1" : "=r"(result) : "r"(value));
    return (int16_t)result;
#else
    return value > 32767 ? 32767 : value < -32768 ? -32768 : (int16_t)value;
#endif
}

// Work on one delay line for a whole block. Index, filter memory, coefficients,
// and buffer pointers stay local instead of being spilled around 35 calls/sample.
// No generic saturation function calls occur in the per-sample loops.
void combBlock(int16_t *tank, unsigned length, uint16_t &position,
               int16_t &filter, const int16_t *input, int32_t *sum,
               int32_t damp1, int32_t damp2, int32_t feedback) {
    unsigned index = position;
    int32_t memory = filter;
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
        const int32_t delayed = tank[index];
        sum[i] += delayed;
        // A convex Q15 combination stays within int16; no saturation needed.
        memory = truncateShift<15>(delayed * damp2 + memory * damp1);
        // feedback < 1, so this multiplication also cannot exceed int16.
        tank[index] = saturate16(input[i] + truncateShift<15>(memory * feedback));
        if (++index == length) index = 0;
    }
    position = index;
    filter = (int16_t)memory;
}

void allpassBlock(int16_t *tank, unsigned length, uint16_t &position, int16_t *data) {
    unsigned index = position;
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
        const int32_t delayed = tank[index];
        const int32_t input = data[i];
        tank[index] = saturate16(input + (delayed >> 1));
        // Difference of two int16 samples divided by two always fits int16.
        data[i] = (int16_t)truncateShift<1>(delayed - input);
        if (++index == length) index = 0;
    }
    position = index;
}
} // namespace

FLASHMEM AudioEffectTreeverb::AudioEffectTreeverb() : AudioStream(1, inputQueue_) {
    clearTanks();
}

FLASHMEM void AudioEffectTreeverb::clearTanks() {
    memset(tanks_, 0, sizeof(tanks_));
    memset(positions_, 0, sizeof(positions_));
    memset(filters_, 0, sizeof(filters_));
}

FLASHMEM void AudioEffectTreeverb::roomsize(float amount) {
    if (!(amount > 0.0f)) amount = 0.0f;
    else if (amount > 1.0f) amount = 1.0f;
    const bool bypass = amount <= 0.001f;
    toern_audio::StateGuard guard;
    if (bypass != bypassed_) {
        bypassed_ = bypass;
        pendingClear_ = true;
    }
    if (!bypass) feedback_ = (int)(amount * 9175.04f) + 22937;
}

FLASHMEM void AudioEffectTreeverb::damping(float amount) {
    if (!(amount > 0.0f)) amount = 0.0f;
    else if (amount > 1.0f) amount = 1.0f;
    const int16_t damp = (int)(amount * 13107.2f);
    toern_audio::StateGuard guard;
    damp1_ = damp;
    damp2_ = 32768 - damp;
}

void AudioEffectTreeverb::update() {
    if (pendingClear_) {
        clearTanks();
        pendingClear_ = false;
    }
    audio_block_t *input = receiveReadOnly();
    if (bypassed_) {
        if (input) release(input);
        return;
    }
    audio_block_t *output = allocate();
    if (!output) {
        if (input) release(input);
        return;
    }

    int16_t scaled[AUDIO_BLOCK_SAMPLES];
    int32_t sum[AUDIO_BLOCK_SAMPLES] = {};
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
        scaled[i] = input ? (int16_t)truncateShift<17>((int32_t)input->data[i] * 8738) : 0;
    if (input) release(input);

    unsigned offset = 0;
    for (unsigned line = 0; line < combCount; ++line) {
        combBlock(tanks_ + offset, combLengths[line], positions_[line],
                  filters_[line], scaled, sum, damp1_, damp2_, feedback_);
        offset += combLengths[line];
    }
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
#if TOERN_TREEVERB_LIGHT
        // Normalize the reduced network against the full version: removing
        // two scaled diffusers also removes their attenuation. Impulse tests
        // cover the resulting wet energy across room/damping settings.
        output->data[i] = saturate16(truncateShift<14>((sum[i] >> 2) * 12135));
#else
        output->data[i] = saturate16(truncateShift<14>((sum[i] >> 3) * 31457));
#endif
    }
    for (unsigned line = 0; line < allpassCount; ++line) {
        allpassBlock(tanks_ + offset, allpassLengths[line], positions_[combCount + line], output->data);
        offset += allpassLengths[line];
    }
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
        output->data[i] = saturate16((int32_t)output->data[i] * 8);
    transmit(output);
    release(output);
}
