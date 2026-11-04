#ifndef effect_delay_ext_h_
#define effect_delay_ext_h_

#include <Arduino.h>
#include <AudioStream.h>

enum AudioEffectDelayMemoryType_t {
  AUDIO_MEMORY_23LC1024 = 0,
  AUDIO_MEMORY_MEMORYBOARD = 1,
  AUDIO_MEMORY_CY15B104 = 2,
  AUDIO_MEMORY_EXTMEM = 3,
  AUDIO_MEMORY_UNDEFINED = 4
};

class AudioEffectDelayExternal : public AudioStream {
public:
  AudioEffectDelayExternal() : AudioStream(1, inputQueueArray) {
    memory_type = AUDIO_MEMORY_UNDEFINED;
    memory_begin = 0;
    memory_length = 0;
    head_offset = 0;
    extmem = nullptr;
    memset(delay_length, 0, sizeof(delay_length));
    memset(fade_length, 0, sizeof(fade_length));
    memset(fade_position, 0, sizeof(fade_position));
    memset(output_gain, 0, sizeof(output_gain));
    for (uint8_t channel = 0; channel < 8; ++channel) requested_length[channel] = 0;
    inputQueueArray[0] = nullptr;
  }

  AudioEffectDelayExternal(AudioEffectDelayMemoryType_t type, float milliseconds = 1e6)
      : AudioEffectDelayExternal() {
    uint32_t n = (uint32_t)(milliseconds * (AUDIO_SAMPLE_RATE_EXACT / 1000.0f) + 0.5f);
    initialize(type, n);
  }

  void begin(int16_t *storage, uint32_t samples) {
    extmem = storage;
    initialize(AUDIO_MEMORY_EXTMEM, samples);
  }

  void delay(uint8_t channel, float milliseconds) {
    if (channel >= 8 || memory_type >= AUDIO_MEMORY_UNDEFINED) return;
    if (milliseconds < 0.0f) milliseconds = 0.0f;
    uint32_t n = (uint32_t)(milliseconds * (AUDIO_SAMPLE_RATE_EXACT / 1000.0f) + 0.5f);
    n += AUDIO_BLOCK_SAMPLES;
    if (n > memory_length - AUDIO_BLOCK_SAMPLES)
      n = memory_length - AUDIO_BLOCK_SAMPLES;
    // Publish only the target. The audio interrupt owns the transition state.
    requested_length[channel] = n;
  }

  void disable(uint8_t channel) {
    if (channel >= 8) return;
    requested_length[channel] = 0;
  }

  virtual void update(void);

private:
  void initialize(AudioEffectDelayMemoryType_t type, uint32_t samples);
  void read(uint32_t address, uint32_t count, int16_t *data);
  void write(uint32_t address, uint32_t count, const int16_t *data);
  void zero(uint32_t address, uint32_t count) { write(address, count, NULL); }
  void readTap(uint32_t length, int16_t *data);

  // About 23 ms at 44.1 kHz. Complete each tap crossfade before following
  // the latest target, so fast encoder updates cannot restart it with a jump.
  static constexpr uint32_t transition_samples = 1024;

  uint32_t memory_begin;
  uint32_t memory_length;
  uint32_t head_offset;
  uint32_t delay_length[8];
  volatile uint32_t requested_length[8];
  uint32_t fade_length[8];
  uint32_t fade_position[8];
  uint16_t output_gain[8];
  uint8_t memory_type;
  int16_t *extmem;
  audio_block_t *inputQueueArray[1];
};

// Delay wet and feedback mixers use the same gain API as AudioMixer4, with
// sample-rate ramps. Gains here are deliberately limited to 0..1 (unity).
class AudioMixer4Ramp : public AudioStream {
public:
  AudioMixer4Ramp() : AudioStream(4, inputQueueArray) {
    for (uint8_t channel = 0; channel < 4; ++channel) {
      requested_gain[channel] = current_gain[channel] = target_gain[channel] = unity;
      gain_step[channel] = 0;
      ramp_remaining[channel] = 0;
      inputQueueArray[channel] = nullptr;
    }
  }

  void gain(uint8_t channel, float level) {
    if (channel >= 4) return;
    if (!(level > 0.0f)) level = 0.0f;
    else if (level > 1.0f) level = 1.0f;
    requested_gain[channel] = (int32_t)(level * (float)unity + 0.5f);
  }

  virtual void update(void);

private:
  static constexpr int32_t unity = 1 << 24;
  static constexpr uint32_t ramp_samples = 512;
  volatile int32_t requested_gain[4];
  int32_t current_gain[4];
  int32_t target_gain[4];
  int32_t gain_step[4];
  uint32_t ramp_remaining[4];
  audio_block_t *inputQueueArray[4];
};

#endif
