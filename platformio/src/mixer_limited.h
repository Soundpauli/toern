#pragma once
#include <AudioStream.h>
#include "toern_mix_limiter.h"

// 16 samples of lookahead per stage (0.36 ms), independent of Audio block size.
// Compatible gain()/AudioConnection surface with AudioMixer4. Float summing
// keeps all four signed inputs intact before the mono peak limiter.
class AudioMixer4Limited : public AudioStream {
public:
  AudioMixer4Limited() : AudioStream(4, inputQueue_) {}
  void gain(unsigned channel, float value) {
    if (channel < 4) gains_[channel] = std::isfinite(value) ? fmaxf(-16.0f, fminf(16.0f, value)) : 0.0f;
  }
  void update() override {
    float sum[AUDIO_BLOCK_SAMPLES] = {};
    bool hasInput = false;
    for (unsigned ch = 0; ch < 4; ++ch) {
      audio_block_t *block = receiveReadOnly(ch);
      if (!block) continue;
      hasInput = true;
      const float scale = gains_[ch];
      for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) sum[i] += (float)block->data[i] * scale;
      release(block);
    }
    const bool produceOutput = hasInput || pendingInput_;
    pendingInput_ = hasInput;
    audio_block_t *out = produceOutput ? allocate() : nullptr;
    if (!out) {
      // Advance recovery even in silence / pool exhaustion, without keeping
      // downstream effect graphs awake by transmitting endless silent blocks.
      int16_t discard[AUDIO_BLOCK_SAMPLES];
      process(sum, discard);
      return;
    }
    process(sum, out->data);
    transmit(out);
    release(out);
  }
private:
  audio_block_t *inputQueue_[4] = {};
  bool pendingInput_ = false;
  volatile float gains_[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  static constexpr unsigned lookahead = 16;
  static_assert(AUDIO_BLOCK_SAMPLES % lookahead == 0, "Audio block must contain complete limiter windows");
  toern_audio::MixLimiter<lookahead> limiter_;
  void process(const float *input, int16_t *output) {
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; i += lookahead) limiter_.process(input + i, output + i);
  }
};
