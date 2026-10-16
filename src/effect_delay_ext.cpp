#include "effect_delay_ext.h"

void AudioEffectDelayExternal::initialize(AudioEffectDelayMemoryType_t type, uint32_t samples) {
  memset(delay_length, 0, sizeof(delay_length));
  memset(fade_length, 0, sizeof(fade_length));
  memset(fade_position, 0, sizeof(fade_position));
  memset(output_gain, 0, sizeof(output_gain));
  for (uint8_t channel = 0; channel < 8; ++channel) requested_length[channel] = 0;
  head_offset = 0;
  memory_type = AUDIO_MEMORY_UNDEFINED;
  memory_begin = 0;
  memory_length = 0;
  if (type != AUDIO_MEMORY_EXTMEM || !extmem || samples < AUDIO_BLOCK_SAMPLES * 2u + 1u) return;
  memory_type = AUDIO_MEMORY_EXTMEM;
  memory_length = samples;
  memset(extmem, 0, samples * sizeof(int16_t));
}

void AudioEffectDelayExternal::read(uint32_t offset, uint32_t count, int16_t *data) {
  if (!extmem || !data) return;
  memcpy(data, extmem + memory_begin + offset, count * sizeof(int16_t));
}

void AudioEffectDelayExternal::write(uint32_t offset, uint32_t count, const int16_t *data) {
  if (!extmem) return;
  int16_t *dst = extmem + memory_begin + offset;
  if (data) memcpy(dst, data, count * sizeof(int16_t));
  else memset(dst, 0, count * sizeof(int16_t));
}

void AudioEffectDelayExternal::update(void) {
  audio_block_t *block;
  uint32_t n, channel;

  block = receiveReadOnly();
  if (memory_type >= AUDIO_MEMORY_UNDEFINED) {
    if (block) release(block);
    return;
  }
  if (block) {
    if (head_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
      write(head_offset, AUDIO_BLOCK_SAMPLES, block->data);
      head_offset += AUDIO_BLOCK_SAMPLES;
    } else {
      n = memory_length - head_offset;
      write(head_offset, n, block->data);
      head_offset = AUDIO_BLOCK_SAMPLES - n;
      write(0, head_offset, block->data + n);
    }
    release(block);
  } else {
    if (head_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
      zero(head_offset, AUDIO_BLOCK_SAMPLES);
      head_offset += AUDIO_BLOCK_SAMPLES;
    } else {
      n = memory_length - head_offset;
      zero(head_offset, n);
      head_offset = AUDIO_BLOCK_SAMPLES - n;
      zero(0, head_offset);
    }
  }

  for (channel = 0; channel < 8; channel++) {
    const uint32_t requested = requested_length[channel];
    if (!requested && !output_gain[channel]) {
      delay_length[channel] = fade_length[channel] = fade_position[channel] = 0;
      continue;
    }
    if (!delay_length[channel]) delay_length[channel] = requested;
    if (requested && requested != delay_length[channel] && !fade_length[channel]) {
      fade_length[channel] = requested;
      fade_position[channel] = 0;
    }
    block = allocate();
    if (!block) continue;
    readTap(delay_length[channel], block->data);
    int16_t next[AUDIO_BLOCK_SAMPLES];
    if (fade_length[channel]) readTap(fade_length[channel], next);
    for (uint32_t i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
      int32_t value = block->data[i];
      if (fade_length[channel]) {
        const uint32_t phase = ++fade_position[channel];
        value += ((int32_t)next[i] - value) * (int32_t)phase / (int32_t)transition_samples;
        if (phase == transition_samples) {
          delay_length[channel] = fade_length[channel];
          // Finish this block from the new tap, including a mid-block completion.
          for (uint32_t j = i + 1; j < AUDIO_BLOCK_SAMPLES; ++j) block->data[j] = next[j];
          fade_length[channel] = fade_position[channel] = 0;
        }
      }
      const uint16_t gain_step = 32768u / transition_samples;
      if (requested) {
        if (output_gain[channel] < 32768u) output_gain[channel] += gain_step;
      } else if (output_gain[channel]) {
        output_gain[channel] -= gain_step;
      }
      block->data[i] = (int16_t)(value * output_gain[channel] / 32768);
    }
    transmit(block, channel);
    release(block);
  }
}

void AudioEffectDelayExternal::readTap(uint32_t length, int16_t *data) {
  const uint32_t offset = length <= head_offset ? head_offset - length
                                               : memory_length + head_offset - length;
  const uint32_t first = memory_length - offset;
  if (first >= AUDIO_BLOCK_SAMPLES) {
    read(offset, AUDIO_BLOCK_SAMPLES, data);
  } else {
    read(offset, first, data);
    read(0, AUDIO_BLOCK_SAMPLES - first, data + first);
  }
}

void AudioMixer4Ramp::update(void) {
  audio_block_t *inputs[4];
  bool has_input = false;
  for (uint8_t channel = 0; channel < 4; ++channel) {
    inputs[channel] = receiveReadOnly(channel);
    has_input |= inputs[channel] != nullptr;
    const int32_t requested = requested_gain[channel];
    if (requested != target_gain[channel]) {
      target_gain[channel] = requested;
      gain_step[channel] = (requested - current_gain[channel]) / (int32_t)ramp_samples;
      ramp_remaining[channel] = ramp_samples;
    }
  }
  audio_block_t *output = has_input ? allocate() : nullptr;
  for (uint32_t i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
    int32_t sum = 0;
    for (uint8_t channel = 0; channel < 4; ++channel) {
      if (ramp_remaining[channel]) {
        current_gain[channel] += gain_step[channel];
        if (!--ramp_remaining[channel]) current_gain[channel] = target_gain[channel];
      }
      if (output && inputs[channel]) {
        const int32_t gain = current_gain[channel] >> 8;
        sum += ((int32_t)inputs[channel]->data[i] * gain) >> 16;
      }
    }
    if (output) {
      if (sum > 32767) sum = 32767;
      else if (sum < -32768) sum = -32768;
      output->data[i] = (int16_t)sum;
    }
  }
  for (uint8_t channel = 0; channel < 4; ++channel) {
    if (inputs[channel]) release(inputs[channel]);
  }
  if (output) {
    transmit(output);
    release(output);
  }
}
