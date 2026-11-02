#pragma once
#include <stdint.h>

// Recent rendered PCM, before the envelope. Two completely zero blocks retire
// stale sustain even when a WAV reader is still walking a silent file tail.
// Do not use an amplitude threshold: quiet recorded detail remains active.
class ToernPcmActivity {
public:
  bool active() const { return silentBlocks_ < 2; }
  void clear() { silentBlocks_ = 2; }
  void observe(int16_t *const *data, unsigned channels, unsigned frames) {
    for (unsigned ch = 0; ch < channels; ++ch) {
      for (unsigned i = 0; i < frames; ++i) {
        if (data[ch][i] != 0) { silentBlocks_ = 0; return; }
      }
    }
    if (silentBlocks_ < 2) ++silentBlocks_;
  }
private:
  volatile uint8_t silentBlocks_ = 2;
};
