#ifndef TEENSYAUDIOLIBRARY_RESAMPLINGREADER_H
#define TEENSYAUDIOLIBRARY_RESAMPLINGREADER_H

#include <Arduino.h>
#include <cstdint>
#include <new> // Required for std::nothrow
#include "loop_type.h"
#include "interpolation.h"
#include "waveheaderparser.h"
#include "toern_pcm_interpolation.h"
#include "toern_wsola.h"

// Simple debug print helper (can be enabled/disabled)
// Keep this off in production builds: Serial I/O can cause timing issues/glitches.
#define RESAMPLING_READER_DEBUG 0 // Set to 1 to enable Serial prints from this file
#if RESAMPLING_READER_DEBUG
    #define RR_DEBUG_PRINT(...) if(Serial) Serial.printf(__VA_ARGS__)
#else
    #define RR_DEBUG_PRINT(...) 
#endif

namespace newdigate {

template<class TArray, class TFile>
class ResamplingReader {
public:
    ResamplingReader() :
        _playing(false),
        _file_size(0), // Will be set by play()
        _header_offset(0), // Will be set by play()
        _playbackRate(1.0),
        _remainder(0.0),
        _loopType(loop_type::looptype_none),
        _play_start(play_start::play_start_sample),
        _bufferPosition1(0), // Will be set by reset() or begin() based on _header_offset/_loop_start
        _bufferPosition2(0),
        _crossfade(0.0),
        _useDualPlaybackHead(false),
        _crossfadeDurationInSamples(256),
        _crossfadeState(0),
        _loop_start(0), // Will be set by play() or setLoopStart()
        _loop_finish(0), // Will be set by play() or setLoopFinish()
        _numChannels(-1), // Initialize to -1 (invalid/not set)
        _numInterpolationPointsChannels(0), // How many channels _interpolationPoints is allocated for
        _filename(nullptr),
        _sourceBuffer(nullptr),
        _interpolationType(ResampleInterpolationType::resampleinterpolation_none),
        _numInterpolationPoints(0), // How many valid points in history for current interpolation
        _interpolationPoints(nullptr) // Array of pointers to InterpolationData arrays
    {
        RR_DEBUG_PRINT("ResamplingReader(): Constructor\n");
    }

    virtual ~ResamplingReader() {    
        RR_DEBUG_PRINT("ResamplingReader(): Destructor (_filename: %s)\n", _filename ? _filename : "NULL");
        // Derived classes should handle their specific TFile and _sourceBuffer cleanup in their own destructors.
        // Base destructor cleans up what it directly allocated.
        deleteInterpolationPoints();
        if (_filename) {
            delete[] _filename;
            _filename = nullptr;
        }
    }

    virtual TFile open(char *filename) = 0;
    virtual TArray* createSourceBuffer() = 0;
    virtual int16_t getSourceBufferValue(long index) = 0;
    virtual void close(void) = 0; // Derived classes must implement this to free _sourceBuffer if owned, and TFile.

    void begin(void) 
    {
        RR_DEBUG_PRINT("begin(): _numChannels=%d, _interpolationType=%d\n", _numChannels, (int)_interpolationType);
        if (_numChannels > 0 && _interpolationType != ResampleInterpolationType::resampleinterpolation_none) {
            initializeInterpolationPoints(); 
        } else {
            deleteInterpolationPoints(); 
        }
        _playing = false;
        _crossfade = 0.0;
        if (_play_start == play_start::play_start_sample)
            _bufferPosition1 = _header_offset; // Should be valid after a play() call
        else
            _bufferPosition1 = _loop_start;    // Should be valid after play() or setLoopStart()
        // _file_size is set by play() calls
    }

    // This is your original playRaw
    bool playRaw(TArray *array, uint32_t length, uint16_t numChannels)
    {
        RR_DEBUG_PRINT("playRaw(array, length=%lu, numCh=%u)\n", length, numChannels);
        if (!array) { RR_DEBUG_PRINT("playRaw ERR: array is null\n"); return false; }
        if (numChannels == 0) { RR_DEBUG_PRINT("playRaw ERR: numChannels is 0\n"); return false; }
        // Assuming 'length' here is total number of int16_t samples (length * numChannels for frames)
        // Or if 'length' is sample frames, then file_size should be length * numChannels * sizeof(int16_t)
        // Original code: _file_size = length * 2;  _loop_finish = length;
        // This implies length was total int16_t samples, and _loop_finish also in total int16_t samples.

        _sourceBuffer = array;
        stop();

        _header_offset = 0;
        _file_size = length * sizeof(int16_t); // Assuming length is total int16_t values
        _loop_start = 0;
        _loop_finish = length; // loop_finish is also total int16_t values
        setNumChannels(numChannels);

        reset();
        _playing = true;
        return true;
    }

    // This overload seems to have a type mismatch in its call to the above playRaw
    // playRaw(array, false, numChannels) -> playRaw(array, 0, numChannels) if bool false is 0.
    // This will likely fail the length check in the above function or lead to issues.
    // It should probably take a length parameter.
    // For now, commenting it out to avoid ambiguity or making it call with a placeholder/error.
    /*
    bool playRaw(TArray *array, uint16_t numChannels) {
        RR_DEBUG_PRINT("playRaw(array, numCh=%u) - AMBIGUOUS, length unknown.\n", numChannels);
        // return playRaw(array, SOME_VALID_LENGTH_HERE, numChannels); // Needs length
        return false; // Safer to fail
    }
    */
   // Corrected overload based on what playWav does:
    bool playRaw(TArray *array, bool isWaveFormat /*unused in this context*/, uint16_t numChannels) {
        RR_DEBUG_PRINT("playRaw(TArray*, bool, uint16_t) called. Assuming TArray is raw, length unknown.\n");
        // This still needs a length. If TArray has a size/length method, use it.
        // Otherwise, this function is not safely usable without a length.
        // Forcing failure for now.
        // Example if TArray had a method: return playRaw(array, array->sizeInInt16Samples(), numChannels);
        return false;
    }


    // This overload also has issues. playRaw expects a length.
    // If TArray *array is a WAV in memory, it needs parsing.
    /*
    bool playWav(TArray *array, uint32_t length) {
        RR_DEBUG_PRINT("playWav(TArray*, length=%lu) - In-memory WAV parsing not implemented in base.\n", length);
        // return playRaw(array, true); // Original call was to a non-existent overload
        return false;
    }
    */

    // This is YOUR ORIGINAL play() method for files
    bool play(const char *filename, bool isWave, uint16_t numChannelsIfRaw = 0)
    {
        RR_DEBUG_PRINT("play(filename=%s, isWave=%d, numChRaw=%u)\n", filename ? filename : "NULL", isWave, numChannelsIfRaw);
        if (!filename || filename[0] == '\0') {
            RR_DEBUG_PRINT("play ERROR: filename is null or empty\n");
            return false;
        }
        close(); // Frees _filename, _sourceBuffer (if owned by derived class)

        size_t name_len = strlen(filename);
        _filename = new (std::nothrow) char[name_len + 1];
        if (!_filename) {
            RR_DEBUG_PRINT("play ERROR: Failed to new _filename\n");
            return false;
        }
        memcpy(_filename, filename, name_len);
        _filename[name_len] = '\0';

        TFile file = open(_filename);
        if (!file) { // Assumes TFile converts to bool for validity
            RR_DEBUG_PRINT("Not able to open file: %s\n", _filename);
            delete[] _filename; _filename = nullptr;
            return false;
        }

        _file_size = file.size();
        if (isWave) {
            wav_header wav_header_info; // Use a distinct name
            wav_data_header data_header_info; // Use a distinct name

            WaveHeaderParser wavHeaderParser; // From "waveheaderparser.h"
            char buffer[44]; // Increased buffer to potentially read more for robust parsing if needed
            size_t bytesRead;
            
            // This is the WAV parsing logic from your original file
            // It's brittle and assumes a specific structure.
            bytesRead = file.read(buffer, 36); // Read enough for basic fmt chunk info
            if (bytesRead < 36) {
                RR_DEBUG_PRINT("playWav ERR: Read < 36 bytes for header from %s\n", _filename);
                file.close(); delete[] _filename; _filename = nullptr; return false;
            }
            
            // This call reads the main WAV header fields like sample rate, channels, bit depth
            // It does NOT find the 'data' chunk offset or size. That's usually separate.
            wavHeaderParser.readWaveHeaderFromBuffer((const char *) buffer, wav_header_info); 
            
            if (wav_header_info.bit_depth != 16) {
                RR_DEBUG_PRINT("playWav ERR: Needs 16 bit audio! (got %d) for %s\n", wav_header_info.bit_depth, _filename);
                file.close(); delete[] _filename; _filename = nullptr; return false;
            }
            setNumChannels(wav_header_info.num_channels); // Critical: sets _numChannels and calls initializeInterpolationPoints
            
            // The original logic for infoTagsSize and finding 'data' chunk was:
            // bytesRead = file.read(buffer, 8); // Read next 8 bytes
            // unsigned infoTagsSize;
            // if (!wavHeaderParser.readInfoTags((unsigned char *)buffer, 0, infoTagsSize)) { ... }
            // file.seek(36 + infoTagsSize);
            // bytesRead = file.read(buffer, 8); // Read "data" + size
            // if (!wavHeaderParser.readDataHeader((unsigned char *)buffer, 0, data_header_info)) { ... }
            // _header_offset = (44 + infoTagsSize) / 2;
            // _loop_finish = ((data_header_info.data_bytes) / 2) + _header_offset;

            // More robust way to find 'data' chunk if WaveHeaderParser doesn't do it all:
            // (This is a simplified generic chunk reader)
            uint32_t current_pos = 12; // After "RIFFxxxxWAVE"
            bool dataFound = false;
            file.seek(current_pos); // Go to first chunk ID after "WAVE"
            
            char chunk_id[4];
            uint32_t chunk_size;

            while (current_pos < (uint32_t)_file_size - 8) {
                file.seek(current_pos);
                if (file.read(chunk_id, 4) != 4) { RR_DEBUG_PRINT("playWav ERR: Failed to read chunk ID at %lu\n", current_pos); break;}
                if (file.read((char*)&chunk_size, 4) != 4) { RR_DEBUG_PRINT("playWav ERR: Failed to read chunk size at %lu\n", current_pos + 4); break; }
                // Arduino is little-endian, WAV chunk sizes are little-endian.
                
                if (strncmp(chunk_id, "fmt ", 4) == 0) {
                    // Already processed essentials from initial read.
                    // Could re-verify wav_header_info.audio_format etc. here if needed.
                } else if (strncmp(chunk_id, "data", 4) == 0) {
                    data_header_info.data_bytes = chunk_size;
                    _header_offset = (current_pos + 8) / sizeof(int16_t); // Offset to actual sample data (in int16_t units)
                    _loop_finish = _header_offset + (data_header_info.data_bytes / sizeof(int16_t));
                    dataFound = true;
                    break; 
                }
                current_pos += (8 + chunk_size);
                if (chunk_size % 2 != 0) current_pos++; // RIFF chunks are padded to even size
            }

            if (!dataFound) {
                RR_DEBUG_PRINT("playWav ERR: 'data' chunk not found in %s\n", _filename);
                file.close(); delete[] _filename; _filename = nullptr; return false;
            }

        } else { // Raw file
             if (numChannelsIfRaw == 0) {
                RR_DEBUG_PRINT("playRaw (file) ERR: numChannelsIfRaw must be > 0 for %s\n", _filename);
                file.close(); delete[] _filename; _filename = nullptr; return false;
            }
            setNumChannels(numChannelsIfRaw);
            _header_offset = 0;
            _loop_finish = _file_size / sizeof(int16_t); // Total int16_t samples in the file
        }
        
        file.close();

        if (_file_size <= (uint32_t)_header_offset * sizeof(int16_t)) { // Check if any actual data exists past header
            _playing = false;
            RR_DEBUG_PRINT("File contains no playable samples after header: %s (filesize: %ld, header_offset_bytes: %ld)\n", 
                        _filename, (long)_file_size, (long)_header_offset * (long)sizeof(int16_t));
            if (_filename) { delete[] _filename; _filename = nullptr; }
            return false;
        }
        if (_numChannels <= 0) { // Should have been caught by setNumChannels or earlier logic
             RR_DEBUG_PRINT("play ERR: _numChannels invalid (%d) for %s\n", _numChannels, _filename);
             if (_filename) { delete[] _filename; _filename = nullptr; }
             return false;
        }

        _sourceBuffer = createSourceBuffer(); // Derived class implements this, should use _filename
        if (!_sourceBuffer) {
            RR_DEBUG_PRINT("play ERROR: createSourceBuffer failed for %s\n", _filename);
            if (_filename) { delete[] _filename; _filename = nullptr; }
            return false;
        }

        _loop_start = _header_offset; // Loop starts at the beginning of audio data

        reset(); // Resets positions and interpolation points based on new settings
        _playing = true;
        RR_DEBUG_PRINT("play() success: %s, size %ld, channels %d, header %ld, start %ld, finish %ld\n", _filename, (long)_file_size, _numChannels, _header_offset, _loop_start, _loop_finish);
        return true;
    }

    bool playRaw(const char *filename, uint16_t numChannelsIfRaw){
        return play(filename, false, numChannelsIfRaw);
    }
    
    bool playWav(const char *filename){
        return play(filename, true);
    }

    bool play() // Resume method
    {
        RR_DEBUG_PRINT("play() (resume) called.\n");
        if (!_sourceBuffer && !_filename) { // Need something to play
            RR_DEBUG_PRINT("play() (resume) ERR: No source loaded.\n");
            return false;
        }
        // If _playing is already true, maybe do nothing or restart?
        // Original just called stop then reset.
        stop(); // Set _playing = false
        reset(); // Recalculate positions, re-init interpolation points
        _playing = true;
        return true;
    }

    void stop(void)
    {
        if (_playing) {   
            RR_DEBUG_PRINT("stop()\n");
            _playing = false;
        }
        // Does not free _filename or _sourceBuffer, allowing for resume.
    }           

    bool isPlaying(void) { return _playing; }

    // Atomic request only: audio update owns all WSOLA state.
    void setTimeStretch(bool enabled) { _timeStretchSetting = enabled ? 12 : 11; }
    // 0..10 = -10%..-0%, 11 = OFF, 12..22 = +0%..+10% length.
    void setTimeStretchAmount(uint8_t value) {
        _timeStretchSetting = value <= 22 ? value : 11;
    }

    unsigned int read(void **buf, uint16_t nsamples) {
        if (!_playing || _numChannels <= 0) {
             // RR_DEBUG_PRINT("read() abort: playing=%d, numCh=%d\n", _playing, _numChannels);
             return 0;
        }
        const int MAX_LOCAL_BUF_CHANNELS = 8; // Static check for local array size
        if (_numChannels > MAX_LOCAL_BUF_CHANNELS) {
             RR_DEBUG_PRINT("read() ERR: _numChannels %d > MAX_LOCAL_BUF_CHANNELS %d\n", _numChannels, MAX_LOCAL_BUF_CHANNELS);
             return 0;
        }
        const float pitch = fabsf((float)_playbackRate);
        const uint8_t stretchSetting = _timeStretchSetting;
        const bool useStretch = stretchSetting != 11 && _numChannels == 1 &&
            _loopType == looptype_none && !_useDualPlaybackHead &&
            pitch >= 0.5f && pitch <= 2.0f;
        if (useStretch) return readTimeStretch(buf, nsamples, pitch, stretchSetting);
        _stretchActive = false;
        _wsola.reset();
        unsigned int count = 0;
        bool retried = false;
        while (count < nsamples) {
            bool frameOK = true;
            for (int channel = 0; channel < _numChannels; ++channel) {
                // Address by completed frame, not by successful individual channels.
                // A retry must not advance one channel past the output allocation.
                int16_t *out = static_cast<int16_t *>(buf[channel]) + count;
                if (!readNextValue(out, channel)) { frameOK = false; break; }
            }
            if (frameOK) { ++count; retried = false; continue; }

            if (_loopType == looptype_none || retried) {
                // One failed reposition is enough: never spin inside audio update.
                for (int channel = 0; channel < _numChannels; ++channel) {
                    int16_t *out = static_cast<int16_t *>(buf[channel]);
                    for (unsigned int frame = count; frame < nsamples; ++frame) out[frame] = 0;
                }
                stop();
                return nsamples;
            }
            retried = true;
            _crossfade = 0.0;
            _crossfadeState = 0;
            if (_loopType == looptype_pingpong) {
                _bufferPosition1 = _playbackRate >= 0.0 ? _loop_finish - _numChannels :
                    (_play_start == play_start::play_start_sample ? _header_offset : _loop_start);
                _playbackRate = -_playbackRate;
                _remainder = -_remainder;
            } else {
                _bufferPosition1 = _playbackRate >= 0.0 ? _loop_start : _loop_finish - _numChannels;
            }
            if (_bufferPosition1 < _header_offset) _bufferPosition1 = _header_offset;
            if (_bufferPosition1 >= _loop_finish && _loop_finish > _loop_start)
                _bufferPosition1 = _loop_finish - _numChannels;
            _numInterpolationPoints = 0;
        }
        return count;
    }

    bool readNextValue(int16_t *value, uint16_t channel) {
        if (_numChannels <= 0 || channel >= (uint16_t)_numChannels) {
             RR_DEBUG_PRINT("RNV ERR: ch %u OOB (numCh=%d)\n", channel, _numChannels);
             if(value) *value = 0; return false; 
        }
        // Ensure value pointer is not null before dereferencing
        if (!value) { RR_DEBUG_PRINT("RNV ERR: value pointer is NULL\n"); return false;}


        // Boundary checks - these define the valid range of int16_t indices for getSourceBufferValue
        int32_t current_play_start_boundary = (_play_start == play_start::play_start_sample) ? _header_offset : _loop_start;
        int32_t current_play_finish_boundary = _loop_finish; // One past the last valid index

        if (!_useDualPlaybackHead) {
            if (_playbackRate >= 0 ) {
                if (_bufferPosition1 + channel >= current_play_finish_boundary ) return false;
                if (_bufferPosition1 + channel < current_play_start_boundary) return false; // Should not happen if pos is managed
            } else { // _playbackRate < 0
                if (_bufferPosition1 + channel < current_play_start_boundary) return false;    
                if (_bufferPosition1 + channel >= current_play_finish_boundary) return false; // Should not happen
            }
        } else {
            // Complex crossfade boundary logic from original...
            // Simplified check: if primary head is out of overall bounds, might signal end soon
            if (_playbackRate >= 0.0) {
                if (_bufferPosition1 >= current_play_finish_boundary && _crossfade < 0.1) return false; 
            } else {
                if (_bufferPosition1 < current_play_start_boundary && _crossfade < 0.1) return false;
            }
            // ... [Your existing crossfade logic for updating _crossfade, _crossfadeState, _bufferPosition1/2] ...
            // This part is complex and needs to ensure _bufferPosition1 and _bufferPosition2 remain valid indices for getSourceBufferValue
            // Original crossfade logic:
            if (_playbackRate >= 0.0) {
                if (_crossfade == 0.0 && _bufferPosition1 > (current_play_finish_boundary - _numChannels) - _crossfadeDurationInSamples * _numChannels) { // crossfadeDurationInSamples is frames
                    _bufferPosition2 = current_play_start_boundary;
                    _crossfade = 1.0 - (double)((current_play_finish_boundary - _numChannels) - _bufferPosition1) / (_crossfadeDurationInSamples * _numChannels) ;
                    if (_crossfade < 0.0) _crossfade = 0.0; else if (_crossfade > 1.0) _crossfade = 1.0;
                    _crossfadeState = 1;
                } else if (_crossfade == 1.0 && _bufferPosition2 > (current_play_finish_boundary - _numChannels) - _crossfadeDurationInSamples * _numChannels) {
                    _bufferPosition1 = current_play_start_boundary;
                    _crossfade = (double)((current_play_finish_boundary - _numChannels) - _bufferPosition2) / (_crossfadeDurationInSamples * _numChannels);
                    if (_crossfade < 0.0) _crossfade = 0.0; else if (_crossfade > 1.0) _crossfade = 1.0;
                    _crossfadeState = 2;
                } else if (_crossfadeState == 1) {
                    _crossfade = 1.0 - (double)((current_play_finish_boundary - _numChannels) - _bufferPosition1) / (_crossfadeDurationInSamples * _numChannels);
                    if (_crossfade >= 1.0) { _crossfadeState = 0; _crossfade = 1.0; }
                     else if (_crossfade < 0.0) { _crossfadeState = 0; _crossfade = 0.0;} 
                } else if (_crossfadeState == 2) {
                    _crossfade = (double)((current_play_finish_boundary - _numChannels) - _bufferPosition2) / (_crossfadeDurationInSamples * _numChannels);
                    if (_crossfade <= 0.0) { _crossfadeState = 0; _crossfade = 0.0; }
                    else if (_crossfade > 1.0) { _crossfadeState = 0; _crossfade = 1.0;} 
                }
            } else { // Playback rate < 0
                if (_crossfade == 0.0 && _bufferPosition1 < current_play_start_boundary + _crossfadeDurationInSamples * _numChannels) {
                    _bufferPosition2 = current_play_finish_boundary - _numChannels;
                    _crossfade = 1.0 - (double)(_bufferPosition1 - current_play_start_boundary) / (_crossfadeDurationInSamples * _numChannels);
                     if (_crossfade < 0.0) _crossfade = 0.0; else if (_crossfade > 1.0) _crossfade = 1.0;
                    _crossfadeState = 1;
                } else if (_crossfade == 1.0 && _bufferPosition2 < current_play_start_boundary + _crossfadeDurationInSamples * _numChannels) {
                    _bufferPosition1 = current_play_finish_boundary - _numChannels;
                    _crossfade = (double)(_bufferPosition2 - current_play_start_boundary) / (_crossfadeDurationInSamples * _numChannels);
                     if (_crossfade < 0.0) _crossfade = 0.0; else if (_crossfade > 1.0) _crossfade = 1.0;
                    _crossfadeState = 2;
                } else if (_crossfadeState == 1) {
                    _crossfade = 1.0 - (double)(_bufferPosition1 - current_play_start_boundary) / (_crossfadeDurationInSamples * _numChannels);
                    if (_crossfade >= 1.0) { _crossfadeState = 0; _crossfade = 1.0; }
                    else if (_crossfade < 0.0) { _crossfadeState = 0; _crossfade = 0.0;}
                } else if (_crossfadeState == 2) {
                    _crossfade = (double)(_bufferPosition2 - current_play_start_boundary) / (_crossfadeDurationInSamples * _numChannels);
                    if (_crossfade <= 0.0) { _crossfadeState = 0; _crossfade = 0.0; }
                    else if (_crossfade > 1.0) { _crossfadeState = 0; _crossfade = 1.0;}
                }
            }
        }

        if (!_sourceBuffer) { *value = 0; return false; }

        // Interpolate each head at its actual fractional source position.
        // Output-history samples are not uniformly spaced when rate is fractional
        // or exceeds unity, so they cannot serve as source interpolation points.
        int16_t result;
        if (!_useDualPlaybackHead || _crossfade < 0.0001) {
            result = interpolateHead(_bufferPosition1, channel,
                                     current_play_start_boundary, current_play_finish_boundary);
        } else if (_crossfade > 0.9999) {
            result = interpolateHead(_bufferPosition2, channel,
                                     current_play_start_boundary, current_play_finish_boundary);
        } else {
            const int16_t r1 = interpolateHead(_bufferPosition1, channel,
                                              current_play_start_boundary, current_play_finish_boundary);
            const int16_t r2 = interpolateHead(_bufferPosition2, channel,
                                              current_play_start_boundary, current_play_finish_boundary);
            result = static_cast<int16_t>((1.0 - _crossfade) * r1 + _crossfade * r2);
        }

        if (channel == _numChannels - 1) { // Only advance position after all channels of a frame are processed
            _remainder += _playbackRate;
            double samples_to_advance_exact = _remainder;
            signed int whole_samples_to_advance = static_cast<signed int>(floor(samples_to_advance_exact));
            _remainder -= static_cast<double>(whole_samples_to_advance);
            
            int32_t int16_units_to_advance = whole_samples_to_advance * _numChannels;

            if (!_useDualPlaybackHead) {
                _bufferPosition1 += int16_units_to_advance;
            } else {
                if (_crossfade < 0.9999) _bufferPosition1 += int16_units_to_advance;
                if (_crossfade > 0.0001) _bufferPosition2 += int16_units_to_advance;
            }
        }

        *value = result;
        return true;
    }

    void setPlaybackRate(double f) {
        RR_DEBUG_PRINT("setPlaybackRate(%.2f), current rate: %.2f\n", f, _playbackRate);
        bool dirChanged = ( (f < 0.0 && _playbackRate >= 0.0) || (f >= 0.0 && _playbackRate < 0.0) );
        _playbackRate = f;

        if (dirChanged) {
            RR_DEBUG_PRINT("Playback direction changed. Resetting interpolation points and remainder.\n");
            if (_numChannels > 0 && _interpolationType != ResampleInterpolationType::resampleinterpolation_none) {
                 initializeInterpolationPoints(); 
            }
            _numInterpolationPoints = 0; 
            _remainder = 0.0; // Reset remainder on direction change
        }
        
        // Adjust buffer position to prevent getting stuck at boundaries on direction change
        int32_t eff_loop_start = (_play_start == play_start::play_start_sample) ? _header_offset : _loop_start;
        int32_t eff_loop_finish = _loop_finish; // This is one past last sample (total samples)

        if (_numChannels > 0) { // Only adjust if channels are set
            if (!_useDualPlaybackHead || _crossfade < 0.5) { // Adjust primary head _bufferPosition1
                if (_playbackRate < 0.0) { // Changed to reverse
                    if (_bufferPosition1 < eff_loop_start + _numChannels) { // If at or before start, jump to end
                        _bufferPosition1 = eff_loop_finish - _numChannels;
                    } else if (_bufferPosition1 >= eff_loop_finish) { // If past end, also jump to end
                        _bufferPosition1 = eff_loop_finish - _numChannels;
                    }
                } else { // Changed to forward (or still forward)
                    if (_bufferPosition1 >= eff_loop_finish - _numChannels) { // If at or past end, jump to start
                        _bufferPosition1 = eff_loop_start;
                    } else if (_bufferPosition1 < eff_loop_start) { // If before start, also jump to start
                         _bufferPosition1 = eff_loop_start;
                    }
                }
                 // Sanity check: ensure bufferPosition1 is a multiple of numChannels from header_offset IF that's how it's used
                 // _bufferPosition1 = eff_loop_start + (((_bufferPosition1 - eff_loop_start) / _numChannels) * _numChannels);
            }
        }
        RR_DEBUG_PRINT("setPlaybackRate done. New _bufferPosition1: %ld\n", _bufferPosition1);
    }

    float playbackRate() { // Return type should be double to match member
        return _playbackRate;
    }

    void loop(uint32_t numSamplesInFrames) {
        if (_numChannels <= 0) { RR_DEBUG_PRINT("loop() ERR: _numCh=%d\n", _numChannels); return; }
        
        // Current position is in int16_t units, convert to frames to make 'numSamplesInFrames' relative
        int32_t currentFramePos = (_crossfade < 0.5 ? _bufferPosition1 : _bufferPosition2) / _numChannels;
        
        // _loop_start and _loop_finish are int16_t indices
        _loop_start = (currentFramePos * _numChannels); 
        _loop_finish = _loop_start + (numSamplesInFrames * _numChannels);
        
        int32_t max_total_samples_in_file = _file_size / sizeof(int16_t);
        if (_loop_finish > max_total_samples_in_file) {
            _loop_finish = max_total_samples_in_file;
            RR_DEBUG_PRINT("loop() Warning: loop_finish adjusted to file end (%ld).\n", _loop_finish);
        }
        if (_loop_start >= _loop_finish && numSamplesInFrames > 0) { // Ensure loop has some valid length
             _loop_start = _loop_finish - _numChannels; 
             if (_loop_start < _header_offset) _loop_start = _header_offset;
             RR_DEBUG_PRINT("loop() Warning: loop_start adjusted (%ld) because start >= finish.\n", _loop_start);
        }
        _loopType = loop_type::looptype_repeat;
        RR_DEBUG_PRINT("loop() set: start_idx=%ld, finish_idx=%ld (frames=%lu)\n", _loop_start, _loop_finish, numSamplesInFrames);
    }

    void setLoopType(loop_type lt) { _loopType = lt; }
    loop_type getLoopType() { return _loopType; }

    int available(void) { return _playing ? AUDIO_BLOCK_SAMPLES : 0; }

    void reset(void) {
        _stretchActive = false;
        _wsola.reset();
        RR_DEBUG_PRINT("reset(): _numCh=%d, _pbRate=%.2f, _play_start=%d, _header=%ld, _loop_s=%ld, _loop_f=%ld\n", 
            _numChannels, _playbackRate, (int)_play_start, _header_offset, _loop_start, _loop_finish);

        if (_numChannels > 0 && _interpolationType != ResampleInterpolationType::resampleinterpolation_none) {
            initializeInterpolationPoints();
        }
        _numInterpolationPoints = 0;

        int32_t eff_loop_start = (_play_start == play_start::play_start_sample) ? _header_offset : _loop_start;
        int32_t eff_loop_finish = _loop_finish;

        if (_numChannels > 0) { // Only set if channels are valid
            if (_playbackRate >= 0.0) {
                _bufferPosition1 = eff_loop_start;
            } else { 
                _bufferPosition1 = eff_loop_finish - _numChannels; 
            }
            // Sanity checks
            if (_bufferPosition1 < _header_offset) _bufferPosition1 = _header_offset;
            if (_bufferPosition1 >= eff_loop_finish && eff_loop_finish > _header_offset) { // if finish is valid
                 _bufferPosition1 = eff_loop_finish - _numChannels;
                 if (_bufferPosition1 < _header_offset) _bufferPosition1 = _header_offset; // nested sanity
            }
        } else {
            _bufferPosition1 = 0; // Default if no channels
        }

        _remainder = 0.0;
        _crossfade = 0.0;
        _crossfadeState = 0;
        RR_DEBUG_PRINT("reset() done: _bufferPosition1=%ld\n", _bufferPosition1);
    }

    void setLoopStart(uint32_t loop_start_frames) {
        if (_numChannels <= 0) { RR_DEBUG_PRINT("setLoopStart ERR: _numCh=%d\n", _numChannels); return; }
        _loop_start = _header_offset + (loop_start_frames * _numChannels);
         RR_DEBUG_PRINT("setLoopStart: %lu frames -> idx %ld\n", loop_start_frames, _loop_start);
    }

    void setLoopFinish(uint32_t loop_finish_frames) {
        if (_numChannels <= 0) { RR_DEBUG_PRINT("setLoopFinish ERR: _numCh=%d\n", _numChannels); return; }
        _loop_finish = _header_offset + (loop_finish_frames * _numChannels);
        
        int32_t max_total_samples_in_file = _file_size / sizeof(int16_t);
        if (_loop_finish > max_total_samples_in_file) _loop_finish = max_total_samples_in_file;
        if (_loop_finish <= _loop_start && loop_finish_frames > 0 && _numChannels > 0) _loop_finish = _loop_start + _numChannels; 
        RR_DEBUG_PRINT("setLoopFinish: %lu frames -> idx %ld\n", loop_finish_frames, _loop_finish);
    }

    void setUseDualPlaybackHead(bool useDualHead) { _useDualPlaybackHead = useDualHead; }

    void setCrossfadeDurationInSamples(unsigned int cfDurationInSampleFrames) {
         _crossfadeDurationInSamples = cfDurationInSampleFrames; 
    }

    void setInterpolationType(ResampleInterpolationType interpType) {
        if (interpType != _interpolationType) {
            RR_DEBUG_PRINT("setInterpolationType: %d -> %d\n", (int)_interpolationType, (int)interpType);
            _interpolationType = interpType;
            if (_numChannels > 0) { 
                 initializeInterpolationPoints();
                 _numInterpolationPoints = 0; 
            }
        }
    }

    int16_t getNumChannels() { return _numChannels; }

    void setNumChannels(uint16_t numCh) {
        RR_DEBUG_PRINT("setNumChannels: %u (current: %d)\n", numCh, _numChannels);
        if (numCh == 0) {
            RR_DEBUG_PRINT("setNumChannels WARNING: Attempt to set 0 channels. Setting to -1 (uninit).\n");
            if (_numChannels != -1 && _numChannels != 0) { // if it was previously valid
                deleteInterpolationPoints();
            }
            _numChannels = -1; // Mark as uninitialized/error
            return;
        }
        if (numCh != (uint16_t)_numChannels) { // Cast _numChannels for comparison as it can be -1
            _numChannels = numCh; 
            initializeInterpolationPoints(); 
            _numInterpolationPoints = 0; 
        } else if (_numChannels > 0 && !_interpolationPoints) { // numChannels was set but points not alloc'd
            RR_DEBUG_PRINT("setNumChannels: _interpolationPoints was NULL for _numChannels=%d, re-initializing.\n", _numChannels);
            initializeInterpolationPoints();
            _numInterpolationPoints = 0;
        }
    }

    void setHeaderSizeInBytes(uint32_t headerSizeBytes) {
        _header_offset = headerSizeBytes / sizeof(int16_t);
        RR_DEBUG_PRINT("setHeaderSizeInBytes: %lu bytes -> idx %ld\n", headerSizeBytes, _header_offset);
    }
    
    void setPlayStart(play_start ps) { _play_start = ps; }

    // Using simpler millis calculation if AUDIO_SAMPLE_RATE_EXACT is frames/sec
    uint32_t positionMillis() {
        if (_numChannels <= 0 || _file_size <= 0 || AUDIO_SAMPLE_RATE_EXACT == 0) return 0;
        // _bufferPosition1 is an int16_t index. Convert to sample frame index.
        int32_t current_frame_abs = _bufferPosition1 / _numChannels; 
        int32_t header_frames_abs = _header_offset / _numChannels;
        int32_t current_frame_rel = current_frame_abs - header_frames_abs;

        if (current_frame_rel < 0) current_frame_rel = 0;
        return ((uint64_t)current_frame_rel * 1000) / AUDIO_SAMPLE_RATE_EXACT;
    }

    uint32_t lengthMillis() {
        if (_numChannels <= 0 || _file_size <= 0 || _loop_finish <= _header_offset || AUDIO_SAMPLE_RATE_EXACT == 0) return 0;
        int32_t total_data_samples = _loop_finish - _header_offset; // Total int16_t samples in data section
        int32_t total_frames = total_data_samples / _numChannels;
        if (total_frames < 0) total_frames = 0;
        return ((uint64_t)total_frames * 1000) / AUDIO_SAMPLE_RATE_EXACT;
    }
    
protected:
    volatile uint8_t _timeStretchSetting = 11;
    uint8_t _stretchLatchedSetting = 11;
    int32_t _stretchOutputLimit = 0;
    uint64_t _stretchProgressQ32 = 0;
    uint64_t _stretchStepQ32 = 1ULL << 32;
    float _stretchSourceStep = 1;
    bool _stretchNeedsWsola = false;
    int _stretchLengthPercent = 100;
    bool _stretchActive = false;
    int32_t _stretchStart = 0, _stretchFrames = 0, _stretchOutput = 0;
    int _stretchDirection = 1;
    float _stretchPitch = 1;
    ToernWsola _wsola;

    float stretchSource(float position, bool fast) {
        // One-shot boundary extension only, never read outside the trim.
        if (position < 0) position = 0;
        if (position > _stretchFrames - 1) position = _stretchFrames - 1;
        const int32_t frame = (int32_t)position;
        const float frac = position - frame;
        auto point = [&](int32_t f) -> int16_t {
            if (f < 0) f = 0;
            if (f >= _stretchFrames) f = _stretchFrames - 1;
            return getSourceBufferValue(_stretchStart + _stretchDirection * f);
        };
        const int16_t b = point(frame);
        if (frac == 0) return b;
        if (fast) return b + frac * ((int32_t)point(frame + 1) - b);
        return toern_audio::interpolatePcm16(point(frame - 1), b, point(frame + 1),
                                point(frame + 2), 1.0f + frac);
    }
    unsigned int readTimeStretch(void **buf, uint16_t nsamples, float pitch, uint8_t setting) {
        if (!_sourceBuffer) { stop(); return 0; }
        if (_stretchActive && (_stretchPitch != pitch ||
            _stretchDirection != (_playbackRate < 0 ? -1 : 1))) _stretchActive = false;
        if (!_stretchActive) {
            _stretchPitch = pitch;
            _stretchLatchedSetting = setting;
            _stretchLengthPercent = setting < 11 ? 90 + setting : 100 + setting - 12;
            _stretchStepQ32 = (100ULL << 32) / _stretchLengthPercent;
            _stretchSourceStep = 100.0f / _stretchLengthPercent;
            _remainder = 0;
            _stretchDirection = _playbackRate < 0 ? -1 : 1;
            _stretchStart = _bufferPosition1;
            const int32_t finish = _loop_finish < _file_size / 2 ?
                _loop_finish : _file_size / 2;
            const int32_t start = _play_start == play_start::play_start_sample ?
                _header_offset : _loop_start;
            _stretchFrames = _stretchDirection > 0 ? finish - _stretchStart :
                _stretchStart - start + 1;
            if (_stretchStart < start || _stretchStart >= finish || _stretchFrames <= 0) {
                stop(); return 0;
            }
            _stretchNeedsWsola = pitch != 1.0f || _stretchLengthPercent != 100;
            _stretchOutput = 0;
            _stretchProgressQ32 = 0;
            _stretchOutputLimit = (int32_t)(((int64_t)_stretchFrames * _stretchLengthPercent + 50) / 100);
            if (_stretchOutputLimit < 1) _stretchOutputLimit = 1;
            _wsola.reset();
            _stretchActive = true;
        }
        // Adjust remaining duration without restarting the running grain.
        if (_stretchLatchedSetting != setting) {
            _stretchLatchedSetting = setting;
            _stretchLengthPercent = setting < 11 ? 90 + setting : 100 + setting - 12;
            _stretchStepQ32 = (100ULL << 32) / _stretchLengthPercent;
            _stretchSourceStep = 100.0f / _stretchLengthPercent;
            if (_stretchLengthPercent != 100) _stretchNeedsWsola = true;
            const int32_t remaining = _stretchFrames - (int32_t)(_stretchProgressQ32 >> 32);
            int32_t extra = (int32_t)(((int64_t)remaining * _stretchLengthPercent + 50) / 100);
            if (extra < 1) extra = 1;
            _stretchOutputLimit = _stretchOutput + extra;
        }
        int16_t *out = static_cast<int16_t *>(buf[0]);
        unsigned int n = 0;
        for (; n < nsamples && _stretchOutput < _stretchOutputLimit; ++n) {
            // Unity is bit-exact and costs no correlation search.
            out[n] = !_stretchNeedsWsola ? getSourceBufferValue(_bufferPosition1) :
                _wsola.next([&](float pos, bool fast) { return stretchSource(pos, fast); }, pitch, _stretchSourceStep, (float)(_stretchProgressQ32 >> 32));
            ++_stretchOutput;
            _stretchProgressQ32 += _stretchStepQ32;
            int32_t sourceFrame = (int32_t)(_stretchProgressQ32 >> 32);
            if (sourceFrame >= _stretchFrames) sourceFrame = _stretchFrames - 1;
            _bufferPosition1 = _stretchStart + _stretchDirection * sourceFrame;
        }
        if (_stretchOutput == _stretchOutputLimit) stop();
        return n;
    }
    volatile bool _playing;

    int32_t _file_size;
    int32_t _header_offset; 

    double _playbackRate;
    double _remainder;
    loop_type _loopType;
    play_start _play_start;
    int32_t _bufferPosition1;
    int32_t _bufferPosition2; 
    
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
    
    // Boundary access is in complete interleaved frames, never across channels.
    int16_t interpolationPoint(int32_t head, int offset, uint16_t channel,
                               int32_t start, int32_t finish) {
        // A first pass can begin before loop_start; do not wrap that prefix.
        if (_loopType != looptype_none && head >= _loop_start && _loop_start > start)
            start = _loop_start;
        const int32_t frames = (finish - start) / _numChannels;
        if (frames <= 0) return 0;
        const int32_t index = head + offset * _numChannels + channel;
        const int32_t completeFinish = start + frames * _numChannels;
        if (index >= start && index < completeFinish)
            return getSourceBufferValue(index);

        int32_t frame = (head - start) / _numChannels + offset;
        if (_loopType == looptype_repeat) {
            frame %= frames;
            if (frame < 0) frame += frames;
        } else if (_loopType == looptype_pingpong && frames > 1) {
            const int32_t period = 2 * (frames - 1);
            frame %= period;
            if (frame < 0) frame += period;
            if (frame >= frames) frame = period - frame;
        } else {
            if (frame < 0) frame = 0;
            if (frame >= frames) frame = frames - 1;
        }
        return getSourceBufferValue(start + frame * _numChannels + channel);
    }

    int16_t interpolateHead(int32_t head, uint16_t channel,
                            int32_t start, int32_t finish) {
        if (_interpolationType == resampleinterpolation_none) {
            const int32_t index = head + channel;
            return index >= start && index < finish ? getSourceBufferValue(index) : 0;
        }
        // floor-based phase works in both directions. Ping-pong recovery can
        // temporarily negate the remainder, so normalize it before lookup.
        const int phaseFrames = static_cast<int>(floor(_remainder));
        head += phaseFrames * _numChannels;
        const float fraction = static_cast<float>(_remainder - phaseFrames);
        const int16_t b = interpolationPoint(head, 0, channel, start, finish);
        if (fraction == 0.0f) return b; // Exact/integer-rate playback is transparent.
        const int16_t c = interpolationPoint(head, 1, channel, start, finish);
        if (_interpolationType == resampleinterpolation_linear)
            return static_cast<int16_t>(b + fraction * (static_cast<int32_t>(c) - b));
        const int16_t a = interpolationPoint(head, -1, channel, start, finish);
        const int16_t d = interpolationPoint(head, 2, channel, start, finish);
        return toern_audio::interpolatePcm16(a, b, c, d, 1.0f + fraction);
    }

    void initializeInterpolationPoints(void) {
        RR_DEBUG_PRINT("initializeInterpolationPoints(): _numChannels=%d, _numIPC=%u, _ipPoints=%p\n", 
                       _numChannels, _numInterpolationPointsChannels, _interpolationPoints);
        
        if (_numChannels <= 0) { // Handles -1 (uninitialized) or 0 (error/explicitly set)
            RR_DEBUG_PRINT("InitIP: _numChannels (%d) is <=0. Deleting points.\n", _numChannels);
            deleteInterpolationPoints(); // Clears _interpolationPoints, sets _numInterpolationPointsChannels = 0
            return;
        }
            
        // If already allocated for the correct number of channels, just re-zero data
        // Check if _interpolationPoints is valid and _numInterpolationPointsChannels matches _numChannels
        if (_interpolationPoints != nullptr && _numInterpolationPointsChannels == (uint16_t)_numChannels) {
            RR_DEBUG_PRINT("InitIP: Re-zeroing existing interpolation points for %d channels.\n", _numChannels);
            for (int channel = 0; channel < _numChannels; channel++) {
                if (_interpolationPoints[channel] != nullptr) { 
                    for(int k=0; k<4; k++) _interpolationPoints[channel][k].y = 0.0;
                } else { 
                    RR_DEBUG_PRINT("InitIP CRITICAL WARNING: _interpolationPoints[%d] was NULL during re-zero. Attempting realloc.\n", channel);
                    _interpolationPoints[channel] = new (std::nothrow) InterpolationData[4]; // Try to fix it
                    if (_interpolationPoints[channel]) {
                         for(int k=0; k<4; k++) _interpolationPoints[channel][k].y = 0.0;
                    } else {
                        RR_DEBUG_PRINT("InitIP CRITICAL ERROR: Failed to re-allocate InterpolationData[4] for channel %d during re-zero.\n", channel);
                        // This is bad. Mark the whole thing as failed.
                        deleteInterpolationPoints(); // Clean up everything
                        _numChannels = -1; // Signal critical failure
                        return;
                    }
                }
            }
            _numInterpolationPoints = 0; 
            return;
        }

        // If different number of channels or not allocated, (re)allocate fully
        RR_DEBUG_PRINT("InitIP: Allocating new interpolation points for %d channels.\n", _numChannels);
        deleteInterpolationPoints(); // Clear any old allocations first

        _interpolationPoints = new (std::nothrow) InterpolationData*[_numChannels];
        if (!_interpolationPoints) {
            RR_DEBUG_PRINT("InitIP ERROR: Failed to new InterpolationData*[%d]\n", _numChannels);
            _numChannels = -1; 
            _numInterpolationPointsChannels = 0;
            return;
        }
        _numInterpolationPointsChannels = _numChannels; 

        for (int i = 0; i < _numChannels; ++i) { // Initialize pointers to null
            _interpolationPoints[i] = nullptr;
        }

        bool allocationErrorOccurred = false;
        for (int channel = 0; channel < _numChannels; channel++) {      
            _interpolationPoints[channel] = new (std::nothrow) InterpolationData[4]; 
            if (!_interpolationPoints[channel]) {
                RR_DEBUG_PRINT("InitIP ERROR: Failed to new InterpolationData[4] for channel %d\n", channel);
                allocationErrorOccurred = true;
                break; 
            }
            for(int k=0; k<4; k++) _interpolationPoints[channel][k].y = 0.0;    
        }

        if (allocationErrorOccurred) {
            RR_DEBUG_PRINT("InitIP: Cleaning up due to allocation error during sub-array new.\n");
            deleteInterpolationPoints(); 
            _numChannels = -1; 
            return;
        }
        
        _numInterpolationPoints = 0; 
        RR_DEBUG_PRINT("InitIP: Success for %d channels.\n", _numChannels);
    }

    void deleteInterpolationPoints(void) {
        // RR_DEBUG_PRINT("deleteInterpolationPoints(): _ipPoints=%p, _numIPC=%u\n", _interpolationPoints, _numInterpolationPointsChannels);
        if (_interpolationPoints) { // Only proceed if outer pointer is not null
            // _numInterpolationPointsChannels should hold the size of the _interpolationPoints array
            for (uint16_t i = 0; i < _numInterpolationPointsChannels; i++) { 
                if (_interpolationPoints[i] != nullptr) { 
                    delete[] _interpolationPoints[i];
                    _interpolationPoints[i] = nullptr; 
                }
            }
            delete[] _interpolationPoints;
            _interpolationPoints = nullptr;
        }
        _numInterpolationPointsChannels = 0; 
        _numInterpolationPoints = 0;       
        // RR_DEBUG_PRINT("deleteInterpolationPoints() done.\n");
    }
};

} // namespace newdigate

#endif //TEENSYAUDIOLIBRARY_RESAMPLINGREADER_H
