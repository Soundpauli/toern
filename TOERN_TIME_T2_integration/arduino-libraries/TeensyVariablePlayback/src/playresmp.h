#ifndef TEENSY_RESAMPLING_SDREADER_PLAYRESMP_H
#define TEENSY_RESAMPLING_SDREADER_PLAYRESMP_H

#include "Arduino.h"
#include "Audio.h"
#include "loop_type.h"

template <class TResamplingReader>
class AudioPlayResmp : public AudioStream
{
    public:
        AudioPlayResmp(): AudioStream(0, NULL), reader(nullptr), ampMult(65536), ampTarget(65536)
        {
        }

        virtual ~AudioPlayResmp() {
        }

        void begin(void)
        {
            reader->begin();
        }

        bool playRaw(const char *filename, uint16_t numChannels)
        {
            stop();
            return reader->play(filename, false, numChannels);
        }

        bool playWav(const char *filename)
        {
            stop();
            return reader->play(filename, true, 0);
        }
        
        bool playRaw(int16_t *data, uint32_t numSamples, uint16_t numChannels)
        {
            stop();
            return reader->playRaw(data, numSamples, numChannels);
        }

        bool playRaw(const unsigned int *data, uint32_t numSamples, uint16_t numChannels) 
        {
            return playRaw((int16_t *) data, numSamples, numChannels);
        }

        bool playWav(int16_t *data, uint32_t fileSize)
        {
            stop();
            return reader->playWav(data, fileSize);
        }

        bool playWav(const unsigned int *data, uint32_t fileSize) {
            return playWav((int16_t *) data, fileSize);
        }

        void setTimeStretch(bool enabled) { reader->setTimeStretch(enabled); }

        void setTimeStretchAmount(uint8_t value) { reader->setTimeStretchAmount(value); }

        void setPlaybackRate(float f) {
            reader->setPlaybackRate(f);
        }

        void setLoopType(loop_type t) {
            reader->setLoopType(t);
        }

        void setLoopStart(uint32_t loop_start) {
            reader->setLoopStart(loop_start);
        }

        void setLoopFinish(uint32_t loop_finish) {
            reader->setLoopFinish(loop_finish);
        }

        void setUseDualPlaybackHead(bool useDualPlaybackHead) {
            reader->setUseDualPlaybackHead(useDualPlaybackHead);
        }

        void setCrossfadeDurationInSamples(unsigned int crossfadeDurationInSamples) {
            reader->setCrossfadeDurationInSamples(crossfadeDurationInSamples);
        }

        void setPlayStart(play_start start) {
            reader->setPlayStart(start);
        }

        void enableInterpolation(bool enable) {
            if (enable)
                reader->setInterpolationType(ResampleInterpolationType::resampleinterpolation_quadratic);
            else 
                reader->setInterpolationType(ResampleInterpolationType::resampleinterpolation_none);
        }

        bool isPlaying(void) {
            return reader->isPlaying();
        };

        void stop() {
            reader->stop();
        }

        // 0..1 playback level (note velocity x fader), ramped per sample.
        void setAmplitude(float n) {
            if (n < 0.0f) n = 0.0f;
            else if (n > 1.0f) n = 1.0f;
            int32_t m = (int32_t)(n * 65536.0f + 0.5f);
            ampTarget = m;
            if (reader == nullptr || !reader->isPlaying()) ampMult = m;
        }

        void update()
        {
            int _numChannels = reader->getNumChannels();
            if (_numChannels == -1)
                return;

            unsigned int n;
            audio_block_t *blocks[_numChannels];
            int16_t *data[_numChannels];
            if (!reader->isPlaying()) {
                ampMult = ampTarget;
                return;
            }
            int32_t cur = ampMult;
            const int32_t tgt = ampTarget;

            // allocate the audio blocks to transmit
            for (int i=0; i < _numChannels; i++) {
                blocks[i] = allocate();
                if (blocks[i] == nullptr) return;
                data[i] = blocks[i]->data;
            }

            if (reader->available()) {
                // we can read more data from the file...
                n = reader->read((void**)data, AUDIO_BLOCK_SAMPLES);
                const int32_t delta = tgt - cur;
                int32_t step = delta / (int32_t)AUDIO_BLOCK_SAMPLES;
                if (delta != 0 && step == 0) step = (delta > 0) ? 1 : -1;
                if (cur != 65536 || tgt != 65536 || step != 0) {
                    for (unsigned int s = 0; s < n; s++) {
                        if (step != 0) {
                            cur += step;
                            if ((step > 0 && cur > tgt) || (step < 0 && cur < tgt)) {
                                cur = tgt;
                                step = 0;
                            }
                        }
                        for (int channel=0; channel < _numChannels; channel++)
                            data[channel][s] = (int16_t)(((int32_t)data[channel][s] * cur) >> 16);
                    }
                }
                ampMult = cur;
                for (int channel=0; channel < _numChannels; channel++) {
                    memset( &blocks[channel]->data[n], 0, (AUDIO_BLOCK_SAMPLES - n) * 2);
                    transmit(blocks[channel], channel);
                }

                if(_numChannels == 1) {
                    transmit(blocks[0], 1);
                }
            } else {
                reader->close();
                ampMult = ampTarget;
            }
            for (int channel=0; channel < _numChannels; channel++) {
                release(blocks[channel]);
            }
        }
        uint32_t positionMillis()
        {
            return reader->positionMillis();
        }

        uint32_t lengthMillis()
        {
            return reader->lengthMillis();
        }

    protected:
        TResamplingReader *reader;
        volatile int32_t ampMult;
        volatile int32_t ampTarget;
};

#endif // TEENSY_RESAMPLING_SDREADER_PLAYRESMP_H