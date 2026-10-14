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
    activemask = 0;
    extmem = nullptr;
    memset(delay_length, 0, sizeof(delay_length));
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
    delay_length[channel] = n;
    activemask = (uint8_t)(activemask | (1 << channel));
  }

  void disable(uint8_t channel) {
    if (channel >= 8) return;
    activemask = (uint8_t)(activemask & ~(1 << channel));
  }

  virtual void update(void);

private:
  void initialize(AudioEffectDelayMemoryType_t type, uint32_t samples);
  void read(uint32_t address, uint32_t count, int16_t *data);
  void write(uint32_t address, uint32_t count, const int16_t *data);
  void zero(uint32_t address, uint32_t count) { write(address, count, NULL); }

  uint32_t memory_begin;
  uint32_t memory_length;
  uint32_t head_offset;
  uint32_t delay_length[8];
  uint8_t activemask;
  uint8_t memory_type;
  int16_t *extmem;
  audio_block_t *inputQueueArray[1];
};

#endif
