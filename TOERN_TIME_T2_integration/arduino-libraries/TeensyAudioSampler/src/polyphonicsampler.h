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

#ifndef TEENSYPOLYPHONY_POLYPHONICSAMPLER_H
#define TEENSYPOLYPHONY_POLYPHONICSAMPLER_H

#include <cstdint>
#include <Arduino.h>
#include <functional>
#define MAX_VOICES 16

//typedef void (*noteEventCallback)();

class polyphonicsampler {
public:
    polyphonicsampler() : _numVoices(0) {
        for (int i=0; i < 128; i++)
          activeNotes[i] = 255;
    }

    void setNumVoices(uint8_t numVoices) {
        if (numVoices > _numVoices)
            for (int i=_numVoices; i < numVoices; i++) {
              activeVoices[i] = 255;
              voice_noteOff[i] = 0;
            }
        else if (numVoices < _numVoices) {
            for (int i=_numVoices; i > numVoices; i--) {
              activeVoices[i] = 255;
              voice_noteOff[i] = 0;
            }
        }
        _numVoices = numVoices;
    }

    void noteOn(uint8_t noteNumber, uint8_t velocity) {
        int indexOfVoice = 255;
        bool isretrigger = false;
        if (activeNotes[noteNumber] == 255) {
            // note is not active, allocate a voice if possible
            indexOfVoice = getFirstFreeVoice();
            if (indexOfVoice < _numVoices) {
                activeVoices[indexOfVoice] = noteNumber;
                activeNotes[noteNumber] = indexOfVoice;                
            } else
            {
                // note dropped: insufficient polyphony to play this note
                indexOfVoice = 255;
                // Serial.printf("Note dropped: %i \n", noteNumber);
            }
        } else {
            // note is already active, just re-trigger it...
            indexOfVoice = activeNotes[noteNumber];
            isretrigger = true;
        }
        if (indexOfVoice != 255) {            
            _noteEventFunction(indexOfVoice, noteNumber, velocity, true, isretrigger);
            voice_noteOn[indexOfVoice] = millis();
            // Serial.printf("Voice %i plays note %i (%i, %i)\n", indexOfVoice, noteNumber, velocity, isretrigger);
        }
    }

    void noteOff(uint8_t noteNumber) {
        uint8_t index = activeNotes[noteNumber];
        if (index == 255) {
            // note is not active, ignore
            return;
        }

        _noteEventFunction(index, noteNumber, 0, false, false);
        activeNotes[noteNumber] = 255;
        activeVoices[index] = 255; // free the voice
        voice_noteOff[index] = millis();
    }

    void setNoteEventCallback (std::function<void(uint8_t voice, uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger)> noteEventFunction) {
        _noteEventFunction =  noteEventFunction;
    }

    void turnOffAllNotesStillPlaying() {
        // Serial.printf("turn off all notes: (%d)\n", _numVoices);
        for (int i=0; i<_numVoices; i++) {
            if (activeVoices[i] != 255) {
                // Serial.printf("turn off %d\n",activeVoices[i]);
                noteOff(activeVoices[i]);
            }
        }
    }

private:
    std::function<void(uint8_t voice, uint8_t noteNumber, uint8_t velocity, bool isNoteOn, bool retrigger)>  _noteEventFunction;

    uint8_t activeNotes[128];
    uint8_t activeVoices[MAX_VOICES];
    uint8_t _numVoices;
    unsigned long voice_noteOff[MAX_VOICES] {0};
    unsigned long voice_noteOn[MAX_VOICES] {0};

    uint8_t getFirstFreeVoice() {
        unsigned long leastRecentNoteOffEvent = UINT32_MAX;
        uint8_t indexOfVoiceWithLeastRecentNoteOff = 255;
        for (int i=0; i < _numVoices; i++) {
            if (activeVoices[i] == 255) {
                if (voice_noteOff[i] < leastRecentNoteOffEvent) {
                    leastRecentNoteOffEvent = voice_noteOff[i];
                    indexOfVoiceWithLeastRecentNoteOff = i;
                }
            }
        }
        if (indexOfVoiceWithLeastRecentNoteOff == 0xff) {
            // all voices are in use... return the voice with least recent note off
            unsigned long leastRecentNoteOnEvent = UINT32_MAX;
            for (int i=0; i < _numVoices; i++) {
                if (voice_noteOn[i] < leastRecentNoteOffEvent) {
                    leastRecentNoteOffEvent = voice_noteOn[i];
                    indexOfVoiceWithLeastRecentNoteOff = i;
                }
            }
        }
        return indexOfVoiceWithLeastRecentNoteOff;
    }
};


#endif //TEENSYSEQUENCER_POLYPHONICSAMPLER_H
