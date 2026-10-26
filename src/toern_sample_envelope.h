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
  void release(float ms) { r_ = count(ms); runR_ = r_; }
  void noteOn(bool oneShot = false, uint32_t availableFrames = 0) {
    // Neutral hits play the complete sample. Shaped one-shots must generate
    // their own Note Off; grid/audition triggers have no key-release event.
    runA_ = a_; runH_ = h_; runD_ = sustain_ != 1.0f ? d_ : 0; runR_ = r_;
    framesLeft_ = availableFrames; bounded_ = availableFrames != 0;
    const bool shaped = a_ || h_ || sustain_ != 1.0f || r_;
    uint32_t gate = (uint32_t)fmaxf((float)(runA_ + runH_ + runD_ + 1), (float)count(5.0f));
    const uint32_t requested = gate + runR_;
    // Reserve the last PCM frame for zero. Keep stage ratios and configured
    // controls; only this note's effective times shrink to fit the reader.
    const uint32_t budget = availableFrames > 1 ? availableFrames - 1 : 1;
    if (shaped && bounded_ && requested > budget) {
      const float scale = (float)budget / requested;
      runA_ = (uint32_t)(runA_ * scale);
      runH_ = (uint32_t)(runH_ * scale);
      runD_ = (uint32_t)(runD_ * scale);
      gate = (uint32_t)fmaxf(1.0f, floorf(gate * scale));
      runR_ = r_ ? budget - gate : 0;
    }
    // A shaped one-shot releases from the level reached after A/H/D.
    // Do not anchor its end to EOF: more R must lengthen the fade rather
    // than bring its onset forward when the requested stages already fit.
    autoOff_ = oneShot && shaped ? gate : 0;
    level_ = runA_ ? 0.0f : 1.0f;
    if (runA_) segment(Attack, runA_, 1.0f);
    else beginHold();
  }
  void noteOff() {
    autoOff_ = 0;
    if (phase_ == Idle || phase_ == Release) return;
    const uint32_t releaseFrames = bounded_ ? (runR_ < framesLeft_ ? runR_ : framesLeft_) : runR_;
    if (releaseFrames) segment(Release, releaseFrames, 0.0f);
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
  uint32_t a_ = 0, h_ = 0, d_ = 0, r_ = 0, remaining_ = 0, autoOff_ = 0;
  uint32_t runA_ = 0, runH_ = 0, runD_ = 0, runR_ = 0, framesLeft_ = 0;
  bool bounded_ = false;
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
    if (runH_) segment(Hold, runH_, 1.0f);
    else beginDecay();
  }
  void beginDecay() {
    if (runD_ && sustain_ != 1.0f) segment(Decay, runD_, sustain_);
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
    if (bounded_ && framesLeft_) --framesLeft_;
    if (autoOff_ && !--autoOff_) noteOff();
    return result;
  }
  float tail() { return hasTail() ? tail_[tailPos_++] : 0.0f; }
};
}
