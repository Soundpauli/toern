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

#ifndef TEENSY_AUDIO_SAMPLER_ST7735DISPLAY_H
#define TEENSY_AUDIO_SAMPLER_ST7735DISPLAY_H

#include <Arduino.h>
#include <ST7735_t3.h>
#include "sampleplaymidicontrollerenums.h"
#include "abstractdisplay.h"
#include "extracolors.h"

class ST7735Display : public AbstractDisplay {
public:
    ST7735Display(ST7735_t3 &tft) : _tft(tft) {

    }

    void switchMode(playcontrollerstate newstate) override {
        _tft.setCursor(0,0);

        switch (newstate) {
            case playcontrollerstate::playcontrollerstate_initialising: {
                clearDisplay(ST77XX_BLACK);
                clearTopArea(ST77XX_BLUE);
                _tft.setTextColor(ST77XX_WHITE);
                _tft.println("initializing...");
                break;
            }
            case playcontrollerstate::playcontrollerstate_performing: {
                clearDisplay(ST77XX_BLACK);
                clearTopArea(ST77XX_COLOR_BRITISHRACINGGREEN);
                _tft.setTextColor(ST77XX_WHITE);
                _tft.println("performing...");
                break;
            }
            case playcontrollerstate::playcontrollerstate_selecting_target: {
                clearDisplay(ST77XX_BLACK);
                clearTopArea(ST77XX_COLOR_VIOLET);
                _tft.setTextColor(ST77XX_WHITE);
                _tft.println("select a key...");
                break;
            }
            case playcontrollerstate::playcontrollerstate_editing_target: {
                clearDisplay(ST77XX_BLACK);
                clearTopArea(ST77XX_RED);
                _tft.setTextColor(ST77XX_WHITE);
                _tft.println("editing...");
                break;
            }
            default: {
                clearDisplay(ST77XX_BLACK);
                clearTopArea(ST77XX_BLACK);
                _tft.setTextColor(ST77XX_BLACK);
                _tft.println("not sure...");
                break;
            }
        }
    }
    
    void prompt(const char *text) override {
        clearPromptArea(ST77XX_BLACK);
        _tft.setCursor(0,10);
        _tft.setTextColor(ST77XX_WHITE);
        _tft.println(text);
    }

    void displayFileName(const char *text) override {
        clearFilenameArea(ST77XX_BLACK);
        _tft.setCursor(0,40);
        _tft.setTextColor(ST77XX_WHITE);
        _tft.setTextSize(2);
        _tft.println(text);
        _tft.setTextSize(1);    
    }

private:
    ST7735_t3 &_tft;

    void clearDisplay(uint16_t color) {
        _tft.fillRect(0, 0, 127, 127, color);
    }

    void clearTopArea(uint16_t color) {
        _tft.fillRect(0, 0, 127, 9, color);
    }

    void clearPromptArea(uint16_t color) {
        _tft.fillRect(0, 10, 127, 39, color);
    }

    void clearFilenameArea(uint16_t color) {
        _tft.fillRect(0, 40, 127, 64, color);
    }
};

#endif // TEENSY_AUDIO_SAMPLER_ST7735DISPLAY_H