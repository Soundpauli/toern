#pragma once
#include <AudioStream.h>
#include <effect_envelope.h>
#include "toern_sample_envelope.h"
#include "toern_audio_runtime.h"

class AudioSampleEnvelope : public AudioStream {
public:
  AudioSampleEnvelope() : AudioStream(1, queue_) {}
  void attack(float ms) { toern_audio::StateGuard guard; core_.attack(ms); }
  void hold(float ms) { toern_audio::StateGuard guard; core_.hold(ms); }
  void decay(float ms) { toern_audio::StateGuard guard; core_.decay(ms); }
  void sustain(float value) { toern_audio::StateGuard guard; core_.sustain(value); }
  void release(float ms) { toern_audio::StateGuard guard; core_.release(ms); }
  void noteOn() { toern_audio::StateGuard guard; core_.noteOn(); }
  void noteOff() { toern_audio::StateGuard guard; core_.noteOff(); }
  bool isActive() const { toern_audio::StateGuard guard; return core_.isActive(); }
  void captureTail(const int16_t *data, unsigned n) { toern_audio::StateGuard guard; core_.captureTail(data,n); }
  void update() override {
    audio_block_t *input = receiveReadOnly();
    const bool tail = core_.hasTail();
    audio_block_t *output = input || tail ? allocate() : nullptr;
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
      const int16_t value = core_.render(input ? input->data[i] : 0);
      if (output) output->data[i] = value;
    }
    if (input) AudioStream::release(input);
    if (output) { transmit(output); AudioStream::release(output); }
  }
private:
  audio_block_t *queue_[1] = {};
  toern_audio::SampleEnvelopeState core_{AUDIO_SAMPLE_RATE_EXACT};
};

// Preserve the firmware's common controls; synth/preview envelopes keep their
// existing implementation. Sample readers register without a library envelope.
class EnvelopeControl {
public:
  EnvelopeControl() = default;
  EnvelopeControl(AudioEffectEnvelope &env) : standard_(&env) {}
  EnvelopeControl(AudioSampleEnvelope &env) : sample_(&env) {}
  void attack(float v) { if(sample_) sample_->attack(v); else if(standard_) standard_->attack(v); }
  void hold(float v) { if(sample_) sample_->hold(v); else if(standard_) standard_->hold(v); }
  void decay(float v) { if(sample_) sample_->decay(v); else if(standard_) standard_->decay(v); }
  void sustain(float v) { if(sample_) sample_->sustain(v); else if(standard_) standard_->sustain(v); }
  void release(float v) { if(sample_) sample_->release(v); else if(standard_) standard_->release(v); }
  void noteOn() { if(sample_) sample_->noteOn(); else if(standard_) standard_->noteOn(); }
  void noteOff() { if(sample_) sample_->noteOff(); else if(standard_) standard_->noteOff(); }
  bool isActive() { return sample_ ? sample_->isActive() : standard_ && standard_->isActive(); }
private:
  AudioEffectEnvelope *standard_ = nullptr;
  AudioSampleEnvelope *sample_ = nullptr;
};
