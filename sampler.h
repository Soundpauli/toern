/* Audio Library for Teensy
 * Copyright (c) 2021, Nic Newdigate
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
#ifndef TEENSY_AUDIO_SAMPLER_H
#define TEENSY_AUDIO_SAMPLER_H

#include <Audio.h>
#include <TeensyVariablePlayback.h>
#include "polyphonicsampler.h"
#include <vector>
#include "effect_envelope.h"
#include "mixer.h"
//template <unsigned MAX_NUM_POLYPHONY>
class audiosample {
public:
    audiosample(uint8_t noteNumber, int16_t *data, uint32_t sampleLength, uint16_t numChannels) : 
        _noteNumber(noteNumber), 
        _data(data), 
        _sampleLength(sampleLength), 
        _numChannels(numChannels) {
    }

    audiosample(uint8_t noteNumber, const char *filename, uint16_t numChannels) : 
        _noteNumber(noteNumber), 
        _data(nullptr), 
        _sampleLength(0), 
        _numChannels(numChannels),
        _filename(filename),
        _isSdFile(true),
        _isWavFile(false) {
    }

    audiosample(uint8_t noteNumber, const char *filename) : 
        _noteNumber(noteNumber), 
        _data(nullptr), 
        _sampleLength(0), 
        _numChannels(0),
        _filename(filename),
        _isSdFile(true),
        _isWavFile(true) {
    }

    uint8_t _noteNumber;    
    int16_t *_data; 
    uint32_t _sampleLength;
    uint16_t _numChannels;

    bool _isSdFile = false; // is this sample a file or an array
    bool _isWavFile = false; // is the file wav or raw format
    const char *_filename;
private:
};

template <class TAudioPlay>
class audiovoice {
public:
    audiovoice(TAudioPlay *audioplayarray, AudioEffectEnvelope *audioenvelop,  AudioEffectEnvelope *audioenvelop2, AudioMixer4 *audiomixer, AudioMixer4 *audiomixer2, uint8_t mixerChannel, uint8_t mixerChannel2) :
        _audioplayarray(audioplayarray), 
        _audioenvelop(audioenvelop),
        _audioenvelop2(audioenvelop2),
        _audiomixer(audiomixer),
        _audiomixer2(audiomixer2),
        _mixerChannel(mixerChannel),
        _mixerChannel2(mixerChannel2),
        _isStereo(true) {
    }

    audiovoice(TAudioPlay *audioplayarray, AudioEffectEnvelope *audioenvelop, AudioMixer4 *audiomixer, uint8_t mixerChannel) :
        _audioplayarray(audioplayarray), 
        _audioenvelop(audioenvelop),
        _audiomixer(audiomixer),
        _mixerChannel(mixerChannel)
    {
    }

    audiovoice(TAudioPlay *audioplayarray, AudioEffectEnvelope *audioenvelop) :
        _audioplayarray(audioplayarray), 
        _audioenvelop(audioenvelop),
        _audiomixer(nullptr),
        _mixerChannel(0)
    {
    }

    audiovoice(TAudioPlay *audioplayarray, AudioEffectEnvelope *audioenvelop1, AudioEffectEnvelope *audioenvelop2) :
        _audioplayarray(audioplayarray), 
        _audioenvelop(audioenvelop1),
        _audioenvelop2(audioenvelop2),
        _audiomixer(nullptr),
        _mixerChannel(0)
    {
    }

    audiovoice(TAudioPlay *audioplayarray) : 
        _audioplayarray(audioplayarray) {
    }

    audiovoice(TAudioPlay *audioplayarray, AudioMixer4 *audiomixer, uint8_t mixerChannel) :
        _audioplayarray(audioplayarray), 
        _audiomixer(audiomixer),
        _mixerChannel(mixerChannel) {
    }
    audiovoice(TAudioPlay *audioplayarray, AudioMixer4 *audiomixer, uint8_t mixerChannel, AudioMixer4 *audiomixer2, uint8_t mixerChannel2) :
        _audioplayarray(audioplayarray), 
        _audiomixer(audiomixer),
        _audiomixer2(audiomixer2),
        _mixerChannel(mixerChannel),
        _mixerChannel2(mixerChannel2) {
    }
    TAudioPlay *_audioplayarray = nullptr;

    AudioEffectEnvelope *_audioenvelop = nullptr;
    AudioEffectEnvelope *_audioenvelop2 = nullptr;
    AudioMixer4 *_audiomixer = nullptr;
    AudioMixer4 *_audiomixer2 = nullptr;

    uint8_t _mixerChannel;
    uint8_t _mixerChannel2;
    bool _isStereo = false;
};

class relativepitchcalculator {
public:
    static float calcPitchFactor(uint8_t note, uint8_t rootNoteNumber) {
        float result = powf(2.0, (note-rootNoteNumber) / 12.0);
        return result;
    }
};

// TAudioPlay should be an object extending AudioStream which will play the audio
// TSampleType stores information about a sample 
template<class TAudioPlay, class TSampleType>
class basesampler {
public:
    basesampler() : _polysampler() {
    }

    void noteEvent(uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger) {
        if (isNoteOn && velocity > 0)
            _polysampler.noteOn(noteNumber, velocity);
        else 
            _polysampler.noteOff(noteNumber);
    }

    void addSample(TSampleType *sample) {
        _audiosamples.push_back(sample);
    }

    void removeAllSamples() {
        for (auto && sample : _audiosamples) {
            //if (sample->_filename)
            //    delete [] sample->_filename;
            delete sample;
        }
        _audiosamples.clear();
    }

    void addVoice(TAudioPlay &audioplayarrayresmp, AudioMixer4 &mixer, uint8_t mixerChannel, AudioEffectEnvelope &envelope, AudioMixer4 &mixer2, uint8_t mixerChannel2, AudioEffectEnvelope &envelope2) {
        audiovoice<TAudioPlay> *voice = new audiovoice<TAudioPlay>(&audioplayarrayresmp, &envelope, &envelope2, &mixer, &mixer2, mixerChannel, mixerChannel2);
        addVoice(voice) ;
    }
    void addVoice(TAudioPlay &audioplayarrayresmp, AudioMixer4 &mixer, uint8_t mixerChannel, AudioEffectEnvelope &envelope) {

        audiovoice<TAudioPlay> *voice = new audiovoice<TAudioPlay>(&audioplayarrayresmp, &envelope, &mixer, mixerChannel);
        addVoice(voice) ;
    }
    void addVoice(TAudioPlay &audioplayarrayresmp, AudioMixer4 &mixer, uint8_t mixerChannel) {

        audiovoice<TAudioPlay> *voice = new audiovoice<TAudioPlay>(&audioplayarrayresmp, nullptr, &mixer, mixerChannel);
        addVoice(voice) ;
    }
    void addVoice(TAudioPlay &audioplayarrayresmp) {
        audiovoice<TAudioPlay> *voice = new audiovoice<TAudioPlay>(&audioplayarrayresmp, nullptr, nullptr, 0);
        addVoice(voice) ;
    }

    void addVoice(TAudioPlay &audioplayarrayresmp, AudioEffectEnvelope &envelope) {
        audiovoice<TAudioPlay> *voice = new audiovoice<TAudioPlay>(&audioplayarrayresmp, &envelope);
        addVoice(voice) ;
    }

    void addVoice(TAudioPlay &audioplayarrayresmp, AudioEffectEnvelope &envelope1, AudioEffectEnvelope &envelope2) {
        audiovoice<TAudioPlay> *voice = new audiovoice<TAudioPlay>(&audioplayarrayresmp, &envelope1, &envelope2);
        addVoice(voice) ;
    }

    void addVoices(TAudioPlay **voices, uint8_t numOfVoicesToAdd){
        for (int i = 0; i < numOfVoicesToAdd; i++){
            addVoice(*voices[i]);
        }
    }
    void addVoices(TAudioPlay *voices, uint8_t numOfVoicesToAdd){
        for (int i = 0; i < numOfVoicesToAdd; i++){
            addVoice(voices[i]);
        }
    }

protected:
    uint8_t _numVoices = 0;
    std::vector<audiovoice<TAudioPlay>*> _voices;
    polyphonicsampler _polysampler;
    
    std::vector<TSampleType*> _audiosamples;

    // _polysampler will call noteEventCallback with a voice number
    //virtual void noteEventCallback(uint8_t voice, uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger);

    //virtual TSampleType* findNearestSampleForKey(uint8_t noteNumber);
    
    void addVoice(audiovoice<TAudioPlay> *voice){
        _voices.push_back(voice);
        _numVoices++;
        _polysampler.setNumVoices(_numVoices);
    }
};

class PitchedArraySamplePlay {
public:
    static void play(uint8_t noteNumber, audiovoice<AudioPlayArrayResmp> *voice, audiosample *sample) {
        float factor = relativepitchcalculator::calcPitchFactor(noteNumber, sample->_noteNumber);
        voice->_audioplayarray->setPlaybackRate(factor);
        voice->_audioplayarray->playRaw(sample->_data, sample->_sampleLength, sample->_numChannels);
    }
};

class PitchedSdWavSamplePlay {
public:
    static void play(uint8_t noteNumber, audiovoice<AudioPlaySdResmp> *voice, audiosample *sample) {
        float factor = relativepitchcalculator::calcPitchFactor(noteNumber, sample->_noteNumber);
        voice->_audioplayarray->setPlaybackRate(factor);
        voice->_audioplayarray->playWav(sample->_filename);
    }
};

class PitchedSdRawSamplePlay {
public:
    static void play(uint8_t noteNumber, audiovoice<AudioPlaySdResmp> *voice, audiosample *sample) {
        float factor = relativepitchcalculator::calcPitchFactor(noteNumber, sample->_noteNumber);
        voice->_audioplayarray->setPlaybackRate(factor);
        voice->_audioplayarray->playWav(sample->_filename);
    }
};

template<class TAudioPlay, class TSamplePlay>
class audiosampler : public basesampler<TAudioPlay, audiosample> {
public:
    using __base = basesampler<TAudioPlay, audiosample>;

    audiosampler() : __base() {
        __base::_polysampler.setNoteEventCallback( [&] (uint8_t voice, uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger) {
            noteEventCallback(voice, noteNumber, velocity, isNoteOn, retrigger);
        });
    }

    void addSample(uint8_t noteNumber, int16_t *data, uint32_t sampleLength, uint16_t numChannels) {
        audiosample *newSample = new audiosample(noteNumber, data, sampleLength, numChannels);
        __base::addSample(newSample);
    }

    void addSample(uint8_t noteNumber, const char* filename) {
        audiosample *newSample = new audiosample(noteNumber, filename);
        __base::addSample(newSample);
    }

 protected:
    void noteEventCallback(uint8_t voice, uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger)    
    {
        uint8_t numVoices = __base::_numVoices;
        if (voice < numVoices) {
            audiovoice<TAudioPlay> *audio_voice = __base::_voices[voice];
            if (isNoteOn) {
                audiosample *nearestSample = findNearestSampleForKey(noteNumber);
                if (nearestSample != nullptr) {
                    
                    if (audio_voice->_audiomixer != nullptr) {                        
                        //audio_voice->_audiomixer->gain( audio_voice->_mixerChannel, velocity / 255.0);
                    }
                    if (audio_voice->_audiomixer2 != nullptr) {                        
                       // audio_voice->_audiomixer2->gain(audio_voice->_mixerChannel, velocity / 255.0);
                    }
                    if (audio_voice->_audioenvelop != nullptr) {
                       audio_voice->_audioenvelop->noteOn();
                    }
                    if (audio_voice->_audioenvelop2 != nullptr) {
                        audio_voice->_audioenvelop2->noteOn();
                    }
                    TSamplePlay::play(noteNumber, audio_voice, nearestSample);
                }
            } else {
                // Note off event
                if (audio_voice->_audioenvelop != nullptr) {
                    audio_voice->_audioenvelop->release(2); 
                    audio_voice->_audioenvelop->noteOff();
                }
                
                if (audio_voice->_audioenvelop2 != nullptr) {
                    audio_voice->_audioenvelop->release(2); 
                    audio_voice->_audioenvelop2->noteOff();
                }

            }
        }
    }

    audiosample* findNearestSampleForKey(uint8_t noteNumber) {
        uint8_t smallestDiff = 255;
        audiosample *candidate = nullptr;
        for (auto &&x : __base::_audiosamples) {
            uint8_t diff = abs(x->_noteNumber - noteNumber);
            if (diff < smallestDiff) {
                smallestDiff = diff;
                candidate = x;
            }
        }
        return candidate;
    }
};

class arraysampler : public audiosampler<AudioPlayArrayResmp, PitchedArraySamplePlay> {
};

class sdwavsampler : public audiosampler<AudioPlaySdResmp, PitchedSdWavSamplePlay> {
};

class sdrawsampler : public audiosampler<AudioPlaySdResmp, PitchedSdRawSamplePlay> {
};

class UnpitchedSdWavSamplePlay {
public:
    static void play(uint8_t noteNumber, audiovoice<AudioPlaySdWav> *voice, audiosample *sample) {
        voice->_audioplayarray->play(sample->_filename);
    }
};

class UnpitchedSdRawSamplePlay {
public:
    static void play(uint8_t noteNumber, audiovoice<AudioPlaySdRaw> *voice, audiosample *sample) {
        voice->_audioplayarray->play(sample->_filename);
    }
};

class unpitchedsdwavsampler : public audiosampler<AudioPlaySdWav, UnpitchedSdWavSamplePlay> {

};

class unpitchedsdrawsampler : public audiosampler<AudioPlaySdRaw, UnpitchedSdRawSamplePlay> {

};

class UnpitchedMemorySamplePlay {
public:
    static void play(uint8_t noteNumber, audiovoice<AudioPlayMemory> *voice, audiosample *sample) {
        voice->_audioplayarray->play((const unsigned int*)sample->_data);
    }
};

class unpitchedmemorysampler : public audiosampler<AudioPlayMemory, UnpitchedMemorySamplePlay> {

};

#endif