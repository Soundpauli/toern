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

#ifndef TEENSY_AUDIO_STRINGSAMPLER_H
#define TEENSY_AUDIO_STRINGSAMPLER_H

#include <vector>
#include "polyphonicsampler.h"
#include "synth_karplusstrong.h"
#include "effect_envelope.h"

class stringvoice {
public:
    stringvoice(AudioSynthKarplusStrong *strings) :
        _envelop(nullptr),
        _strings(strings) {
    }

    stringvoice(AudioSynthKarplusStrong *strings, AudioEffectEnvelope *envelop) :
        _envelop(envelop), 
        _strings(strings) {
    }

    AudioEffectEnvelope *_envelop = nullptr;
    AudioSynthKarplusStrong *_strings = nullptr;
};

class stringsampler {
public:
    stringsampler() : _polysampler() {
        _polysampler.setNoteEventCallback( [&] (uint8_t voice, uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger) {
            noteEventCallback(voice, noteNumber, velocity, isNoteOn, retrigger);
        });
    }

    void noteEvent(uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger) {
        if (isNoteOn && velocity > 0)
            _polysampler.noteOn(noteNumber, velocity);
        else 
            _polysampler.noteOff(noteNumber);
    }
    
    void addVoice(AudioSynthKarplusStrong  &strings) {
        stringvoice *voice = new stringvoice(&strings);
        addVoice(voice) ;
    }
    
    void addVoice(AudioSynthKarplusStrong  &strings, AudioEffectEnvelope &envelope) {
        stringvoice *voice = new stringvoice(&strings, &envelope);
        addVoice(voice) ;
    }

    void turnOffAllNotesStillPlaying() {
        _polysampler.turnOffAllNotesStillPlaying();
    }

private:
    uint8_t _numVoices = 0;
    std::vector<stringvoice*> _voices;
    polyphonicsampler _polysampler;
    
    static float calcFrequency(uint8_t note) {
        float result = 440.0 * powf(2.0, (note-69) / 12.0);
        return result;
    }

    void noteEventCallback(uint8_t voice, uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger)
    {
        if (voice < _numVoices) {
            if (isNoteOn) {
                float freq = calcFrequency(noteNumber);

                if (_voices[voice]->_envelop != nullptr) {
                    _voices[voice]->_envelop->noteOn();
                }
                _voices[voice]->_strings->noteOn(freq, velocity/127.0);
            } else {
                // Note off event
                if (_voices[voice]->_envelop != nullptr) {
                    _voices[voice]->_envelop->noteOff();
                }
            }
        }
    }

    void addVoice(stringvoice *voice){
        _voices.push_back(voice);
        //_voices.begin();
        _numVoices++;
        _polysampler.setNumVoices(_numVoices);
    }
};

#endif