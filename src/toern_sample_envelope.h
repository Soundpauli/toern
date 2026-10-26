#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

namespace toern_audio {
class SampleEnvelopeState {
public:
  static constexpr unsigned tailSamples = 64; // 1.45 ms; only the old reader fades.
  explicit SampleEnvelopeState(float rate = 44100.0f) : rate_(rate) {}
  void attack(float ms) { a_ = count(ms); }
  void hold(float ms) { h_ = count(ms); }
  void decay(float ms) { d_ = count(ms); }
  void sustain(float level) {
    sustain_ = fmaxf(0.0f, fminf(1.0f, level));
    if (phase_ == Sustain && level_ != sustain_) segment(Decay, tailSamples, sustain_);
    else if (phase_ == Decay) {
      target_ = sustain_;
      step_ = (target_ - level_) / (float)remaining_;
    }
  }
  void release(float ms) { r_ = count(ms); }
  void noteOn() {
    level_ = a_ ? 0.0f : 1.0f;
    if (a_) segment(Attack, a_, 1.0f);
    else beginHold();
  }
  void noteOff() {
    if (phase_ == Idle || phase_ == Release) return;
    if (r_) segment(Release, r_, 0.0f);
    else { phase_ = Idle; level_ = 0.0f; }
  }
  bool isActive() const { return phase_ != Idle || hasTail(); }
  bool hasTail() const { return tailPos_ < tailSamples; }

  // Reader snapshot is already at the OLD note's playback rate and velocity.
  // Include any previous residual tail so fast repeated retriggers stay smooth.
  void captureTail(const int16_t *old, unsigned length) {
    float next[tailSamples]; bool audible = false;
    for (unsigned i = 0; i < tailSamples; ++i) {
      const float value = (i < length ? old[i] : 0) * gain() + tail();
      next[i] = value * (float)(tailSamples - 1 - i) / (float)(tailSamples - 1);
      audible |= fabsf(next[i]) >= 0.5f;
    }
    memcpy(tail_, next, sizeof(tail_));
    tailPos_ = audible ? 0 : tailSamples;
  }
  int16_t render(int16_t input) {
    const float fresh = (float)input * gain();
    // Preserve the new attack at unity; only the short OLD tail may give up
    // headroom if their sum would exceed the 16-bit per-voice connection.
    const float residual = fmaxf(-32768.0f - fresh, fminf(32767.0f - fresh, tail()));
    return (int16_t)lrintf(fmaxf(-32768.0f, fminf(32767.0f, fresh + residual)));
  }
private:
  enum Phase : uint8_t { Idle, Attack, Hold, Decay, Sustain, Release };
  float rate_, sustain_ = 1.0f, level_ = 0.0f, step_ = 0.0f, target_ = 0.0f;
  uint32_t a_ = 0, h_ = 0, d_ = 0, r_ = 0, remaining_ = 0;
  Phase phase_ = Idle;
  float tail_[tailSamples] = {};
  unsigned tailPos_ = tailSamples;
  uint32_t count(float ms) const {
    if (!(ms > 0.0f) || !std::isfinite(ms)) return 0;
    return (uint32_t)ceilf(fminf(ms, 12000.0f) * rate_ / 1000.0f);
  }
  void segment(Phase phase, uint32_t samples, float target) {
    phase_ = phase; remaining_ = samples; target_ = target;
    step_ = (target - level_) / (float)samples;
  }
  void beginHold() {
    if (h_) segment(Hold, h_, 1.0f);
    else beginDecay();
  }
  void beginDecay() {
    if (d_ && sustain_ != 1.0f) segment(Decay, d_, sustain_);
    else { level_ = sustain_; phase_ = Sustain; }
  }
  float gain() {
    const float result = level_;
    if (phase_ == Attack || phase_ == Hold || phase_ == Decay || phase_ == Release) {
      level_ += step_;
      if (!--remaining_) {
        level_ = target_;
        if (phase_ == Attack) beginHold();
        else if (phase_ == Hold) beginDecay();
        else if (phase_ == Decay) phase_ = Sustain;
        else phase_ = Idle;
      }
    }
    return result;
  }
  float tail() { return hasTail() ? tail_[tailPos_++] : 0.0f; }
};
}
