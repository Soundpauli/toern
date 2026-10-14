#include "effect_delay_ext.h"

void AudioEffectDelayExternal::initialize(AudioEffectDelayMemoryType_t type, uint32_t samples) {
  activemask = 0;
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
  uint32_t n, channel, read_offset;

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
    if (!(activemask & (1 << channel))) continue;
    block = allocate();
    if (!block) continue;
    if (delay_length[channel] <= head_offset) {
      read_offset = head_offset - delay_length[channel];
    } else {
      read_offset = memory_length + head_offset - delay_length[channel];
    }
    if (read_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
      read(read_offset, AUDIO_BLOCK_SAMPLES, block->data);
    } else {
      n = memory_length - read_offset;
      read(read_offset, n, block->data);
      read(0, AUDIO_BLOCK_SAMPLES - n, block->data + n);
    }
    transmit(block, channel);
    release(block);
  }
}
