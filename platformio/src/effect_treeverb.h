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

// Treeverb: full-rate, block-wise Schroeder reverb for Teensy/TŒRN.
// Original network by Jezar at Dreampoint; optimized scheduling/arithmetic here.
#pragma once
#include <Arduino.h>
#include <AudioStream.h>

// 0: light 4/2, 1: balanced 6/4, 2: original full 8/4 network.
// Keep legacy A/B builds working when they explicitly set LIGHT.
#ifndef TOERN_TREEVERB_QUALITY
#ifdef TOERN_TREEVERB_LIGHT
#define TOERN_TREEVERB_QUALITY (TOERN_TREEVERB_LIGHT ? 0 : 2)
#else
#define TOERN_TREEVERB_QUALITY 1
#endif
#endif

class AudioEffectTreeverb : public AudioStream {
public:
    FLASHMEM AudioEffectTreeverb();
    void update() override;
    FLASHMEM void roomsize(float amount);
    FLASHMEM void damping(float amount);
private:
    FLASHMEM void clearTanks();
    audio_block_t *inputQueue_[1];
#if TOERN_TREEVERB_QUALITY == 0
    static constexpr unsigned combCount = 4, allpassCount = 2;
    static constexpr unsigned tankSamples = 1116 + 1277 + 1422 + 1617 + 556 + 225;
#elif TOERN_TREEVERB_QUALITY == 1
    static constexpr unsigned combCount = 6, allpassCount = 4;
    static constexpr unsigned tankSamples = 1116 + 1188 + 1277 + 1422 + 1491 + 1617 + 556 + 441 + 341 + 225;
#elif TOERN_TREEVERB_QUALITY == 2
    static constexpr unsigned combCount = 8, allpassCount = 4;
    static constexpr unsigned tankSamples = 12587;
#else
#error Unsupported TOERN_TREEVERB_QUALITY
#endif
    int16_t tanks_[tankSamples];
    uint16_t positions_[combCount + allpassCount] = {};
    int16_t filters_[combCount] = {};
    // Unity Q15 is 32768, which must not wrap into a negative int16_t.
    uint16_t damp1_ = 6553, damp2_ = 26215;
    int16_t feedback_ = 27524;
    bool bypassed_ = true, pendingClear_ = false;
};
