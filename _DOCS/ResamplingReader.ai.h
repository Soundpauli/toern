#ifndef TEENSYAUDIOLIBRARY_RESAMPLINGREADER_H
#define TEENSYAUDIOLIBRARY_RESAMPLINGREADER_H

#include <Arduino.h>
#include <cstdint>
#include <cstring>
#include "loop_type.h"
#include "interpolation.h"
#include "waveheaderparser.h"

namespace newdigate {

template<class TArray, class TFile>
class ResamplingReader {
public:
    ResamplingReader() : _playing(false),
                           _file_size(0),
                           _header_offset(0),
                           _playbackRate(1.0),
                           _remainder(0.0),
                           _loopType(loop_type::looptype_none),
                           _play_start(play_start::play_start_sample),
                           _bufferPosition1(0),
                           _bufferPosition2(0),
                           _crossfade(0.0),
                           _useDualPlaybackHead(false),
                           _crossfadeDurationInSamples(256),
                           _crossfadeState(0),
                           _loop_start(0),
                           _loop_finish(0),
                           _numChannels(-1),
                           _numInterpolationPointsChannels(0),
                           _filename(nullptr),
                           _sourceBuffer(nullptr),
                           _interpolationType(ResampleInterpolationType::resampleinterpolation_none),
                           _numInterpolationPoints(0),
                           _interpolationPoints(nullptr)
    {
    }
    
    virtual ~ResamplingReader() {       
        // Clean up allocated memory to avoid nullptr issues.
        deleteInterpolationPoints();
        if (_filename) {
            delete [] _filename;
            _filename = nullptr;
        }
    }

    // These must be defined by the concrete implementation.
    virtual TFile open(char *filename) = 0;
    virtual TArray* createSourceBuffer() = 0;
    virtual int16_t getSourceBufferValue(long index) = 0;
    virtual void close(void) = 0;

    void begin(void) 
    {
        if (_interpolationType != ResampleInterpolationType::resampleinterpolation_none) {
            initializeInterpolationPoints();
        }
        _playing = false;
        _crossfade = 0.0;
        if (_play_start == play_start::play_start_sample)
            _bufferPosition1 = _header_offset;
        else
            _bufferPosition1 = _loop_start;
        _file_size = 0;
    }

    bool playRaw(TArray *array, uint32_t length, uint16_t numChannels)
    {
        _sourceBuffer = array;
        stop();

        _header_offset = 0;
        _file_size = length * 2; // length is given in samples; 16-bit samples = 2 bytes each.
        _loop_start = 0;
        _loop_finish = length;
        setNumChannels(numChannels);

        reset();
        _playing = true;
        return true;
    }

    bool playRaw(TArray *array, uint16_t numChannels) {
        return playRaw(array, false, numChannels); 
    }

    bool playWav(TArray *array, uint32_t length) {
        return playRaw(array, true); 
    }

    bool play(const char *filename, bool isWave, uint16_t numChannelsIfRaw = 0)
    {
        close();  // Close any existing file first.

        if (!isWave) // For raw files, set number of channels from the parameter.
            setNumChannels(numChannelsIfRaw);

        // Free any previously allocated filename.
        if (_filename) {
            delete [] _filename;
            _filename = nullptr;
        }
        _filename = new char[strlen(filename)+1];
        strcpy(_filename, filename);

        TFile file = open(_filename);
        if (!file) {
            Serial.printf("Not able to open file: %s\n", _filename);
            delete [] _filename;
            _filename = nullptr;
            return false;
        }

        _file_size = file.size();
        if (isWave) {
            wav_header wav_header;
            wav_data_header data_header;
            WaveHeaderParser wavHeaderParser;
            char buffer[36];
            size_t bytesRead = file.read(buffer, 36);
            
            wavHeaderParser.readWaveHeaderFromBuffer(buffer, wav_header);
            if (wav_header.bit_depth != 16) {
                Serial.printf("Needs 16 bit audio! Aborting.... (got %d)\n", wav_header.bit_depth);
                file.close();
                return false;
            }
            setNumChannels(wav_header.num_channels);
            
            bytesRead = file.read(buffer, 8);
            unsigned infoTagsSize;
            if (!wavHeaderParser.readInfoTags((unsigned char *)buffer, 0, infoTagsSize))
            {
                Serial.println("Not able to read header! Aborting...");
                file.close();
                return false;
            }

            file.seek(36 + infoTagsSize);
            bytesRead = file.read(buffer, 8);

            if (!wavHeaderParser.readDataHeader((unsigned char *)buffer, 0, data_header)) {
                Serial.println("Not able to read header! Aborting...");
                file.close();
                return false;
            }

            // Compute header offset and loop finish in samples.
            _header_offset = (44 + infoTagsSize) / 2;
            _loop_finish = ((data_header.data_bytes) / 2) + _header_offset; 
        } else {
            _loop_finish = _file_size / 2;
        }
        
        file.close();

        if (_file_size <= _header_offset * sizeof(int16_t)) {
            _playing = false;
            if (_filename) {
                delete [] _filename;
                _filename = nullptr;
            }
            Serial.printf("Wave file contains no samples: %s\n", filename);
            return false;
        }

        _sourceBuffer = createSourceBuffer();
        _loop_start = _header_offset;

        reset();
        _playing = true;
        return true;
    }

    bool playRaw(const char *filename, uint16_t numChannelsIfRaw){
        return play(filename, false, numChannelsIfRaw);
    }
    
    bool playWav(const char *filename){
        return play(filename, true);
    }

    bool play()
    {
        stop();
        reset();
        _playing = true;
        return true;
    }

    void stop(void)
    {
        if (_playing) {   
            _playing = false;
        }
    }           

    bool isPlaying(void) { return _playing; }

    unsigned int read(void **buf, uint16_t nsamples) {
        if (!_playing) return 0;
        if (!_sourceBuffer) return 0; // safeguard against bad pointers

        int16_t *index[_numChannels];
        unsigned int count = 0;
        for (int channel = 0; channel < _numChannels; channel++) {
            index[channel] = reinterpret_cast<int16_t*>(buf[channel]);
        }

        while (count < nsamples) {
            for (int channel = 0; channel < _numChannels; channel++) {
                if (readNextValue(index[channel], channel)) {
                    if (channel == _numChannels - 1)
                        count++;
                    index[channel]++;
                }
                else {
                    // End of file reached; handle looping if enabled.
                    switch (_loopType) {
                        case looptype_repeat:
                        {
                            _crossfade = 0.0;
                            if (_playbackRate >= 0.0) 
                                _bufferPosition1 = _loop_start;
                            else
                                _bufferPosition1 = _loop_finish - _numChannels;
                            break;
                        }

                        case looptype_pingpong:
                        {
                            if (_playbackRate >= 0.0) {
                                _bufferPosition1 = _loop_finish - _numChannels;
                            }
                            else {
                                if (_play_start == play_start::play_start_sample)
                                    _bufferPosition1 = _header_offset;
                                else
                                    _bufferPosition1 = _loop_start;
                            }
                            _playbackRate = -_playbackRate;
                            break;
                        }            

                        case looptype_none:            
                        default:
                        {
                            // No looping – close and return the count so far.
                            close();
                            return count;
                        }
                    }   
                }
            }
        }
        return count;
    }

    // Read the sample value for a given channel and store it in *value.
    bool readNextValue(int16_t *value, uint16_t channel) {
        // Check boundaries for non-dual playback.
        if (!_useDualPlaybackHead) {
            if (_playbackRate >= 0 ) {
                // Forward playback.
                if (_bufferPosition1 >= _loop_finish) {
                    // Uncomment for debugging:
                    // Serial.printf("Forward playback end reached: _bufferPosition1=%d, _loop_finish=%d\n", _bufferPosition1, _loop_finish);
                    return false;
                }
            } else if (_playbackRate < 0) {
                // Reverse playback.
                if (_play_start == play_start::play_start_sample) {
                    if (_bufferPosition1 < _header_offset)
                        return false;
                } else {
                    if (_bufferPosition1 < _loop_start)
                        return false;    
                }
            }
        } else {
            // Dual playback head (crossfading) logic.
            if (_playbackRate >= 0.0) {
                if (_crossfade == 0.0 && _bufferPosition1 > (_loop_finish - _numChannels) - _crossfadeDurationInSamples) {
                    _bufferPosition2 = _loop_start;
                    _crossfade = 1.0 - (((_loop_finish - _numChannels) - _bufferPosition1) / static_cast<double>(_crossfadeDurationInSamples));
                    _crossfadeState = 1;
                } else if (_crossfade == 1.0 && _bufferPosition2 > (_loop_finish - _numChannels) - _crossfadeDurationInSamples) {
                    _bufferPosition1 = _loop_start;
                    _crossfade = (((_loop_finish - _numChannels) - _bufferPosition2) / static_cast<double>(_crossfadeDurationInSamples));
                    _crossfadeState = 2;
                } else if (_crossfadeState == 1) {
                    _crossfade = 1.0 - (((_loop_finish - _numChannels) - _bufferPosition1) / static_cast<double>(_crossfadeDurationInSamples));
                    if (_crossfade >= 1.0) {
                        _crossfadeState = 0;
                        _crossfade = 1.0;
                    }
                } else if (_crossfadeState == 2) {
                    _crossfade = (((_loop_finish - _numChannels) - _bufferPosition2) / static_cast<double>(_crossfadeDurationInSamples));
                    if (_crossfade <= 0.0) {
                        _crossfadeState = 0;
                        _crossfade = 0.0;
                    }
                }
            } else {
                if (_crossfade == 0.0 && _bufferPosition1 < _crossfadeDurationInSamples + _header_offset) {
                    _bufferPosition2 = _loop_finish - _numChannels;
                    _crossfade = 1.0 - ((_bufferPosition1 - _header_offset) / static_cast<double>(_crossfadeDurationInSamples));
                    _crossfadeState = 1;
                } else if (_crossfade == 1.0 && _bufferPosition2 < _crossfadeDurationInSamples + _header_offset) {
                    _bufferPosition1 = _loop_finish - _numChannels;
                    _crossfade = ((_bufferPosition2 - _header_offset) / static_cast<double>(_crossfadeDurationInSamples));
                    _crossfadeState = 2;
                } else if (_crossfadeState == 1) {
                    _crossfade = 1.0 - ((_bufferPosition1 - _header_offset) / static_cast<double>(_crossfadeDurationInSamples));
                    if (_crossfade >= 1.0) {
                        _crossfadeState = 0;
                        _crossfade = 1.0;
                    }
                } else if (_crossfadeState == 2) {
                    _crossfade = ((_bufferPosition2 - _header_offset) / static_cast<double>(_crossfadeDurationInSamples));
                    if (_crossfade <= 0.0) {
                        _crossfadeState = 0;
                        _crossfade = 0.0;
                    }
                }
            }
        }

        int16_t result = 0;
        if (!_useDualPlaybackHead || _crossfade == 0.0) {
            result = getSourceBufferValue(_bufferPosition1 + channel);
        } else if (_crossfade == 1.0) {
            result = getSourceBufferValue(_bufferPosition2 + channel);
        } else {
            int result1 = getSourceBufferValue(_bufferPosition1 + channel);
            int result2 = getSourceBufferValue(_bufferPosition2 + channel);
            result = static_cast<int16_t>(((1.0 - _crossfade) * result1) + (_crossfade * result2));
        }

        // Apply interpolation if selected.
        if (_interpolationType == ResampleInterpolationType::resampleinterpolation_linear) {
            double abs_remainder = abs(_remainder);
            if (abs_remainder > 0.0) {

                if (_playbackRate > 0) {
                    if (_remainder - _playbackRate < 0.0){
                        if (!_useDualPlaybackHead) {
                            if (_numInterpolationPoints < 2 && _playbackRate > 1.0 && _bufferPosition1 - _numChannels > _header_offset * 2 ) {
                                _interpolationPoints[channel][1].y = getSourceBufferValue(_bufferPosition1 - _numChannels);
                            }
                        }
                        _interpolationPoints[channel][0].y = _interpolationPoints[channel][1].y;
                        _interpolationPoints[channel][1].y = result;
                        if (_numInterpolationPoints < 2)
                            _numInterpolationPoints++;
                    }
                } 
                else if (_playbackRate < 0) {
                    if (_remainder - _playbackRate > 0.0){
                        if (!_useDualPlaybackHead) {
                            if (_numInterpolationPoints < 2 && _playbackRate < -1.0) {
                                _interpolationPoints[channel][1].y = getSourceBufferValue(_bufferPosition1 + _numChannels);
                            }
                        }
                        _interpolationPoints[channel][0].y = _interpolationPoints[channel][1].y;
                        _interpolationPoints[channel][1].y = result;
                        if (_numInterpolationPoints < 2)
                            _numInterpolationPoints++;
                    }
                }

                if (_numInterpolationPoints > 1) {
                    result = static_cast<int16_t>(abs_remainder * _interpolationPoints[channel][1].y +
                                                    (1.0 - abs_remainder) * _interpolationPoints[channel][0].y);
                }
            } else {
                _interpolationPoints[channel][0].y = _interpolationPoints[channel][1].y;
                _interpolationPoints[channel][1].y = result;
                if (_numInterpolationPoints < 2)
                    _numInterpolationPoints++;

                result = _interpolationPoints[channel][0].y;
            }
        } 
        else if (_interpolationType == ResampleInterpolationType::resampleinterpolation_quadratic) {
            double abs_remainder = abs(_remainder);
            if (abs_remainder > 0.0) {
                if (_playbackRate > 0) {                
                    if (_remainder - _playbackRate < 0.0){
                        int numberOfSamplesToUpdate = - static_cast<int>(floor(_remainder - _playbackRate));
                        if (numberOfSamplesToUpdate > 4) 
                            numberOfSamplesToUpdate = 4;
                        for (int i = numberOfSamplesToUpdate; i > 0; i--) {
                            _interpolationPoints[channel][0].y = _interpolationPoints[channel][1].y;
                            _interpolationPoints[channel][1].y = _interpolationPoints[channel][2].y;
                            _interpolationPoints[channel][2].y = _interpolationPoints[channel][3].y;
                            if (!_useDualPlaybackHead) {
                                _interpolationPoints[channel][3].y = getSourceBufferValue(_bufferPosition1 - (i * _numChannels) + 1 + channel);
                            } else {
                                _interpolationPoints[channel][3].y = result;
                            }
                            if (_numInterpolationPoints < 4) _numInterpolationPoints++;
                        }
                    }
                } 
                else if (_playbackRate < 0) {                
                    if (_remainder - _playbackRate > 0.0){
                        int numberOfSamplesToUpdate = static_cast<int>(ceil(_remainder - _playbackRate));
                        if (numberOfSamplesToUpdate > 4) 
                            numberOfSamplesToUpdate = 4;
                        for (int i = numberOfSamplesToUpdate; i > 0; i--) {
                            _interpolationPoints[channel][0].y = _interpolationPoints[channel][1].y;
                            _interpolationPoints[channel][1].y = _interpolationPoints[channel][2].y;
                            _interpolationPoints[channel][2].y = _interpolationPoints[channel][3].y;
                            if (!_useDualPlaybackHead) {
                                _interpolationPoints[channel][3].y = getSourceBufferValue(_bufferPosition1 + (i * _numChannels) - 1 + channel);
                            } else {
                                _interpolationPoints[channel][3].y = result;
                            }
                            if (_numInterpolationPoints < 4) _numInterpolationPoints++;
                        }
                    }
                }
                
                if (_numInterpolationPoints >= 4) {
                    int16_t interpolation = fastinterpolate(
                        _interpolationPoints[channel][0].y, 
                        _interpolationPoints[channel][1].y, 
                        _interpolationPoints[channel][2].y, 
                        _interpolationPoints[channel][3].y, 
                        1.0 + abs_remainder); 
                    result = interpolation;
                } else 
                    result = 0;
            } else {
                _interpolationPoints[channel][0].y = _interpolationPoints[channel][1].y;
                _interpolationPoints[channel][1].y = _interpolationPoints[channel][2].y;
                _interpolationPoints[channel][2].y = _interpolationPoints[channel][3].y;
                _interpolationPoints[channel][3].y = result;
                if (_numInterpolationPoints < 4) {
                    _numInterpolationPoints++;
                    result = 0;
                } else 
                    result = _interpolationPoints[channel][1].y;
            }
        }
  
        // Advance the sample pointer only once per full sample (i.e. after processing all channels)
        if (channel == _numChannels - 1) {
            _remainder += _playbackRate;
            int delta = static_cast<int>(_remainder);
            _remainder -= static_cast<double>(delta);
            if (!_useDualPlaybackHead) {
                _bufferPosition1 += (delta * _numChannels);
            } else {
                if (_crossfade < 1.0)
                    _bufferPosition1 += (delta * _numChannels);
                if (_crossfade > 0.0)
                    _bufferPosition2 += (delta * _numChannels);
            }
        }

        *value = result;
        return true;
    }

    void setPlaybackRate(double f) {
        _playbackRate = f;
        if (!_useDualPlaybackHead) {
            if (f < 0.0) {
                if (_bufferPosition1 <= _header_offset) {
                    if (_play_start == play_start::play_start_sample)
                        _bufferPosition1 = _file_size/2 - _numChannels;
                    else
                        _bufferPosition1 = _loop_finish - _numChannels;
                } 
            } else {
                if (f >= 0.0 && _bufferPosition1 < _header_offset) {
                    if (_play_start == play_start::play_start_sample) 
                        _bufferPosition1 = _header_offset;
                    else
                        _bufferPosition1 = _loop_start;
                }
            }
        } else { 
            // When using dual playback head.
            if (_crossfade == 0.0) {
                if (f < 0.0) { 
                    if (_bufferPosition1 <= _header_offset) {
                        if (_play_start == play_start::play_start_sample)
                            _bufferPosition1 = _file_size/2 - _numChannels;
                        else
                            _bufferPosition1 = _loop_finish - _numChannels;
                    }
                } else {
                    if (f >= 0.0 && _bufferPosition1 < _header_offset) {
                        _bufferPosition1 = _header_offset;
                    }
                }
            }
        }
    }

    float playbackRate() {
        return _playbackRate;
    }

    void loop(uint32_t numSamples) {
        int bufferPosition = (_crossfade < 1.0) ? _bufferPosition1 : _bufferPosition2;
        _loop_start = bufferPosition;
        _loop_finish = bufferPosition + numSamples * _numChannels;
        _loopType = loop_type::looptype_repeat;
    }

    void setLoopType(loop_type loopType)
    {
        _loopType = loopType;
    }

    loop_type getLoopType() {
        return _loopType;    
    }

    int available(void) {
        return _playing;
    }

    void reset(void) {
        if (_interpolationType != ResampleInterpolationType::resampleinterpolation_none) {
            initializeInterpolationPoints();
        }
        _numInterpolationPoints = 0;
        if (_playbackRate > 0.0) {
            if (_play_start == play_start::play_start_sample)
                _bufferPosition1 = _header_offset;
            else
                _bufferPosition1 = _loop_start;
        } else {
            if (_play_start == play_start::play_start_sample)
                _bufferPosition1 = _file_size/2 - _numChannels;
            else
                _bufferPosition1 = _loop_finish - _numChannels;
        }
        _crossfade = 0.0;
    }

    void setLoopStart(uint32_t loop_start) {
        _loop_start = _header_offset + (loop_start * _numChannels);
    }

    void setLoopFinish(uint32_t loop_finish) {
        _loop_finish = _header_offset + (loop_finish * _numChannels);
    }

    void setUseDualPlaybackHead(bool useDualPlaybackHead) {
        _useDualPlaybackHead = useDualPlaybackHead;
    }

    void setCrossfadeDurationInSamples(unsigned int crossfadeDurationInSamples) {
        _crossfadeDurationInSamples = crossfadeDurationInSamples;
    }

    void setInterpolationType(ResampleInterpolationType interpolationType) {
        if (interpolationType != _interpolationType) {
            _interpolationType = interpolationType;
            initializeInterpolationPoints();
        }
    }

    int16_t getNumChannels() {
        return _numChannels;
    }

    void setNumChannels(uint16_t numChannels) {
        if (numChannels != _numChannels) {
            _numChannels = numChannels;
            initializeInterpolationPoints();
        }
    }

    void setHeaderSizeInBytes(uint32_t headerSizeInBytes) {
        _header_offset = headerSizeInBytes / 2;
    }
    
    void setPlayStart(play_start start) {
        _play_start = start;
    }

    // Convert file size (in bytes) to milliseconds.
    #define B2M (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT / 2.0)
    uint32_t positionMillis()
    {
        return ((uint64_t)_file_size * B2M) >> 32;
    }

    uint32_t lengthMillis()
    {
        return ((uint64_t)_file_size * B2M) >> 32;
    }
    
protected:
    volatile bool _playing;
    int32_t _file_size;
    int32_t _header_offset;
    double _playbackRate;
    double _remainder;
    loop_type _loopType;
    play_start _play_start;
    int _bufferPosition1;
    int _bufferPosition2;
    double _crossfade;
    bool _useDualPlaybackHead;
    unsigned int _crossfadeDurationInSamples;
    int _crossfadeState;
    int32_t _loop_start;
    int32_t _loop_finish;
    int16_t _numChannels;
    uint16_t _numInterpolationPointsChannels;
    char *_filename;
    TArray *_sourceBuffer;

    ResampleInterpolationType _interpolationType;
    unsigned int _numInterpolationPoints;
    InterpolationData **_interpolationPoints;
    
    void initializeInterpolationPoints(void) {
        if (_numChannels < 0)
            return;
            
        deleteInterpolationPoints();
        _interpolationPoints = new InterpolationData*[_numChannels];
        for (int channel = 0; channel < _numChannels; channel++) {        
            InterpolationData *interpolation = new InterpolationData[4];
            interpolation[0].y = 0.0;
            interpolation[1].y = 0.0;    
            interpolation[2].y = 0.0;    
            interpolation[3].y = 0.0;
            _interpolationPoints[channel] = interpolation;
        }
        _numInterpolationPointsChannels = _numChannels;
    }

    void deleteInterpolationPoints(void)
    {
        if (!_interpolationPoints) return;
        for (int i = 0; i < _numInterpolationPointsChannels; i++) {
            delete [] _interpolationPoints[i];
        }
        delete [] _interpolationPoints;
        _interpolationPoints = nullptr;
        _numInterpolationPointsChannels = 0;
    }

};

} // namespace newdigate

#endif // TEENSYAUDIOLIBRARY_RESAMPLINGREADER_H
