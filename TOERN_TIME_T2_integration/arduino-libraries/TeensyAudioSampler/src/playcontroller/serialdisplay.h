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

#ifndef TEENSY_AUDIO_SAMPLER_SERIALDISPLAY_H
#define TEENSY_AUDIO_SAMPLER_SERIALDISPLAY_H

#include <Arduino.h>
#include "sampleplaymidicontrollerenums.h"
#include "abstractdisplay.h"

class SerialDisplay : public AbstractDisplay {
public:
    SerialDisplay(HardwareSerial &serialPort) : _serialPort(serialPort) {

    }

    void switchMode(playcontrollerstate newstate) override {
        _serialPort.print("Controller switched to ");
        switch (newstate) {
            case playcontrollerstate::playcontrollerstate_initialising: {
                _serialPort.println("initialization");
                break;
            }
            case playcontrollerstate::playcontrollerstate_performing: {
                _serialPort.println("performing");
                break;
            }
            case playcontrollerstate::playcontrollerstate_selecting_target: {
                _serialPort.println("select note");
                break;
            }
            case playcontrollerstate::playcontrollerstate_editing_target: {
                _serialPort.println("editing note");
                break;
            }
            default: {
                _serialPort.print("(unknown)");
                break;
            }

        }
    }
    
    void prompt(const char *text) override {
        _serialPort.printf("prompt: %s\n", text);
    }

    void displayFileName(const char *text) override {
        _serialPort.printf("filename: %s\n", text);
    }


private:
    HardwareSerial &_serialPort;
};

#endif // TEENSY_AUDIO_SAMPLER_SERIALDISPLAY_H