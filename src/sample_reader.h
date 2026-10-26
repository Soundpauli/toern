#pragma once
#include <playarrayresmp.h>

// The existing reader stays the sampler's AudioPlayArrayResmp interface.
// Capture only the old note's short continuation before a retrigger rewinds it.
class AudioSampleReader : public AudioPlayArrayResmp {
public:
  uint32_t playbackFramesRemaining() { return reader ? reader->playbackFramesRemaining() : 0; }
  unsigned snapshotTail(int16_t *mono, unsigned count) {
    if (!reader || !reader->isPlaying() || !reader->available() || count > 64) return 0;
    const int channels = reader->getNumChannels();
    if (channels < 1 || channels > 2) return 0;
    int16_t right[64];
    void *data[2] = {mono, right};
    const unsigned n = reader->read(data, count);
    int32_t current = ampMult;
    const int32_t target = ampTarget;
    const int32_t step = (target - current) / AUDIO_BLOCK_SAMPLES;
    for (unsigned i = 0; i < n; ++i) {
      current += step;
      mono[i] = (int16_t)((int32_t)mono[i] * current / 65536);
    }
    return n;
  }
};
