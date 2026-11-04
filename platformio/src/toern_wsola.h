#pragma once
#include <stdint.h>
#include <math.h>

// RAM-source WSOLA followed by resampling, fused into two read heads.
// 1024 output-sample grains, 512-sample overlap. No heap, FFT or audio queue.
// Search work is spread across the hop, never a full search in one IRQ.
class ToernWsola {
public:
    enum { Hop = 512, Radius = 256, Compare = 64, Stride = 8 };
    void reset() { ready = false; }
    template<class Source> int16_t next(Source source, float rate, float sourceStep = 1.0f, float sourceStart = 0.0f) {
        if (!ready) {
            analysisHop = Hop * sourceStep;
            current = previous = nominal = sourceStart;
            phase = 0;
            first = true;
            ready = true;
            prepare(source, rate);
        }
        if (phase == Hop) {
            previous = current;
            current = best;
            nominal += analysisHop;
            analysisHop = Hop * sourceStep;
            phase = 0;
            first = false;
            prepare(source, rate);
        }
        if (phase % 10 == 0) searchOne(source, rate);
        float out = source(current + phase * rate, false);
        if (!first) {
            const float old = source(previous + (Hop + phase) * rate, false);
            // Complementary raised-cosine windows: constant signals stay constant.
            const float weight = weights[phase];
            out = old + weight * (out - old);
        }
        ++phase;
        if (out > 32767.f) out = 32767.f;
        if (out < -32768.f) out = -32768.f;
        return (int16_t)out;
    }
    // Called once at boot, not from the audio interrupt.
    static void init() {
        for (int i = 0; i < Hop; ++i)
            weights[i] = 0.5f - 0.5f * cosf(3.14159265358979323846f * i / Hop);
    }
private:
    bool ready = false, first = true;
    int phase = 0, candidate = 0, fine = 0;
    float current = 0, previous = 0, nominal = 0, best = 0, coarseBest = 0;
    float analysisHop = Hop;
    float bestScore = -1;
    float reference[Compare];
    static float weights[Hop];
    template<class Source> void prepare(Source source, float rate) {
        best = nominal + analysisHop;
        bestScore = -1;
        candidate = -Radius;
        fine = -8;
        for (int i = 0; i < Compare; ++i)
            reference[i] = source(current + (Hop + i * Stride) * rate, true);
    }
    template<class Source> void evaluate(Source source, float rate, float start) {
        float correlation = 0, energy = 1;
        for (int i = 0; i < Compare; ++i) {
            const float x = source(start + (i * Stride) * rate, true);
            correlation += x * reference[i];
            energy += x * x;
        }
        // Reference energy is constant across candidates. Avoid sqrt/divisions
        // in the inner loop; negative correlation must never win.
        const float score = correlation > 0 ? correlation * correlation / energy : 0;
        if (score > bestScore || (score == bestScore &&
            fabsf(start - (nominal + analysisHop)) < fabsf(best - (nominal + analysisHop)))) {
            bestScore = score;
            best = start;
        }
    }
    template<class Source> void searchOne(Source source, float rate) {
        if (candidate <= Radius) {
            evaluate(source, rate, nominal + analysisHop + candidate);
            candidate += 16;
            if (candidate > Radius) coarseBest = best;
        } else if (fine <= 8) {
            const float start = coarseBest + fine++;
            if (fabsf(start - (nominal + analysisHop)) <= Radius)
                evaluate(source, rate, start);
        }
    }
};
