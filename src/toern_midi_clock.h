#pragma once

#include <cmath>
#include <cstdint>

namespace toern_midi {
// One timestamp per F8. Rolling windows count elapsed intervals, not endpoints.
// Short acquisition, then a quarter-note window to reject UART/parser jitter.
class ClockFollower {
 public:
  void reset() { *this = ClockFollower(); }
  bool tick(uint32_t now) {
    if (count_ && uint32_t(now - last_) > 2000000u) reset();
    last_ = now;
    const unsigned intervals = count_ < 24 ? count_ : 24;
    const uint32_t first = times_[(index_ + 25 - intervals) % 25];
    times_[index_] = now;
    index_ = (index_ + 1) % 25;
    if (count_ < 24) ++count_;
    if (intervals < 6 || (period_ && ++updateTicks_ < 6)) return false;
    updateTicks_ = 0;
    double measured = double(uint32_t(now - first)) / intervals;
    // Reject implausible windows without substituting an arbitrary tempo.
    if (measured < 0.98 * 60000000.0 / (300.0 * 24.0) ||
        measured > 1.02 * 60000000.0 / (40.0 * 24.0)) {
      stable_ = false;
      return false;
    }
    measured = std::fmax(60000000.0 / (300.0 * 24.0),
                         std::fmin(60000000.0 / (40.0 * 24.0), measured));
    if (!period_) period_ = measured;
    const double relative = std::fabs(measured - period_) / period_;
    // Follow deliberate tempo changes quickly, smooth small timing noise.
    period_ += (measured - period_) * (relative > 0.005 ? 0.75 : 0.5);
    stable_ = intervals == 24 && relative < 0.005;
    return true;
  }
  bool ready() const { return period_ > 0.0; }
  bool stable(uint32_t now) const {
    return stable_ && uint32_t(now - last_) < 500000u;
  }
  double stepUs() const { return period_ * 6.0; }
  float bpm() const { return float(60000000.0 / (period_ * 24.0)); }

  // Slew the timer toward the nearest external sixteenth boundary. Modulo
  // handles callbacks immediately before/after the corresponding timer ISR.
  // Keep display tempo independent of this temporary phase correction.
  double correctedStepUs(uint32_t clockUs, uint32_t stepUs,
                         uint32_t delayUs) const {
    const double base = this->stepUs();
    double error = double(int32_t(clockUs + delayUs - stepUs));
    error -= std::round(error / base) * base;
    const double limit = base * 0.05;
    const double correction = std::fmax(-limit, std::fmin(limit, error * 0.25));
    return base + correction;
  }

 private:
  uint32_t times_[25] = {};
  uint32_t last_ = 0;
  unsigned index_ = 0, count_ = 0, updateTicks_ = 0;
  double period_ = 0.0;
  bool stable_ = false;
};
}  // namespace toern_midi
