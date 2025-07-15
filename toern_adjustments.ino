
      void drawDrums(char *txt, int activeDrum) {
        FastLED.clear();
        // Amplitude definitions:
        int baseAmplitude = 1;  // baseline (minimum amplitude)
        drawText(txt, 1, 12, filter_col[activeDrum]);


        if (SMP.currentChannel>3)   {
          drawLowResCircle(4,4,CRGB(100,20,0));
          drawText("!", 8, 5, CRGB(20,20,20));
          FastLEDshow();
          return;
          }

        float DrumValue = SMP.drum_settings[SMP.currentChannel][SMP.selectedDrum];  //mapf(SMP.drum_settings[SMP.currentChannel][SMP.selectedDrum], 0, maxfilterResolution, 0, 10);
        drawNumber(DrumValue, CRGB(100, 0, 100), 4);
      }



      /************/

      void drawSynths(char *txt, int activeSynth) {
        FastLED.clear();
        // Amplitude definitions:
        drawText(txt, 1, 12, filter_col[activeSynth]);
        float SynthValue = SMP.synth_settings[SMP.currentChannel][SMP.selectedSynth];  //mapf(SMP.drum_settings[SMP.currentChannel][SMP.selectedDrum], 0, maxfilterResolution, 0, 10);
        drawNumber(SynthValue, CRGB(100, 0, 100), 4);
        const int maxIndex = 9;
        int instrumentValue = mapf(SMP.synth_settings[SMP.currentChannel][INSTRUMENT], 0, maxIndex, 1, maxIndex + 1);
        int param1Value = SMP.synth_settings[SMP.currentChannel][PARAM1]; 
        int param2Value = SMP.synth_settings[SMP.currentChannel][PARAM2];
        int param3Value = SMP.synth_settings[SMP.currentChannel][PARAM3];
        int param4Value = SMP.synth_settings[SMP.currentChannel][PARAM4]; 
        int param5Value = SMP.synth_settings[SMP.currentChannel][PARAM5];
        int param6Value = SMP.synth_settings[SMP.currentChannel][PARAM6];
        
        if (SMP.currentChannel == 11) switchSynthVoice(instrumentValue, 0, param1Value, param2Value, param3Value, param4Value, param5Value, param6Value);

      }


      void drawInstrument(char *txt, int activeSynth) {
        FastLED.clear();

        const int maxIndex = 9;
        if (SMP.synth_settings[SMP.currentChannel][INSTRUMENT] > maxIndex) {
          SMP.synth_settings[SMP.currentChannel][INSTRUMENT] = maxIndex;
          currentMode->pos[3] = SMP.synth_settings[SMP.currentChannel][INSTRUMENT];
          Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
        }
        int instrumentValue = mapf(SMP.synth_settings[SMP.currentChannel][INSTRUMENT], 0, maxIndex, 1, maxIndex + 1);
        int param1Value = SMP.synth_settings[SMP.currentChannel][PARAM1];
        int param2Value = SMP.synth_settings[SMP.currentChannel][PARAM2];
        int param3Value = SMP.synth_settings[SMP.currentChannel][PARAM3];

        int param4Value = SMP.synth_settings[SMP.currentChannel][PARAM4]; 
        int param5Value = SMP.synth_settings[SMP.currentChannel][PARAM5];
        int param6Value = SMP.synth_settings[SMP.currentChannel][PARAM6];
        drawText(txt, 1, 12, filter_col[activeSynth]);

        drawText(SynthVoices[instrumentValue], 1, 6, filter_col[instrumentValue]);
        drawNumber(instrumentValue, CRGB(100, 100, 100), 1);
        if (SMP.currentChannel == 11) switchSynthVoice(instrumentValue,0, param1Value, param2Value, param3Value, param4Value, param5Value, param6Value);
      }


      /************/


      void drawFilters(char *txt, int activeFilter) {
        FastLED.clear();
        // Amplitude definitions:
        int baseAmplitude = 1;  // baseline (minimum amplitude)

        

        drawText(txt, 1, 12, filter_col[activeFilter]);
        if (SMP.currentChannel >= 15) return;
        
        float FilterValue = mapf(SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter], 0, maxfilterResolution, 0, 10);

        if (activeFilter == defaultFilter[SMP.currentChannel]) {
          for (int x = 1; x < 9; x++) {
            light(x, 10, CRGB(250, 200, 0));
          }
        }

        


        drawNumber(FilterValue, CRGB(100, 100, 100), 4);

        

      }

      void drawADSR(char *txt, int activeParameter) {
        FastLED.clear();
        // Amplitude definitions:
        int baseAmplitude = 1;
        int attackHeight = maxY - 6;
        int sustainHeight = mapf(SMP.param_settings[SMP.currentChannel][SUSTAIN], 0, maxfilterResolution, baseAmplitude, attackHeight);

        // Map widths (x durations) for each stage with min width of 1:
        int delayWidth = max(0, mapf(SMP.param_settings[SMP.currentChannel][DELAY], 0, maxfilterResolution, 0, 3));
        int attackWidth = max(1, mapf(SMP.param_settings[SMP.currentChannel][ATTACK], 0, maxfilterResolution, 0, 4));
        int holdWidth = max(1, mapf(SMP.param_settings[SMP.currentChannel][HOLD], 0, maxfilterResolution, 0, 3));
        int decayWidth = max(1, mapf(SMP.param_settings[SMP.currentChannel][DECAY], 0, maxfilterResolution, 0, 4));
        const int sustainFixedWidth = 2;
        int releaseWidth = mapf(SMP.param_settings[SMP.currentChannel][RELEASE], 0, maxfilterResolution, 0, 4);

        // Compute x-positions for each stage:
        int xDelayStart = 1;
        int xDelayEnd = xDelayStart + delayWidth;
        int xAttackStart = xDelayEnd;
        int xAttackEnd = xAttackStart + attackWidth;
        int xHoldEnd = xAttackEnd + holdWidth;
        int xDecayEnd = xHoldEnd + decayWidth;
        int xSustainEnd = xDecayEnd + sustainFixedWidth;
        int xReleaseEnd = xSustainEnd + releaseWidth;

        // Draw colored ADSR envelope:
        colorBelowCurve(xDelayStart, xDelayEnd, baseAmplitude, baseAmplitude,
                        activeParameter == 0 ? CRGB(100, 0, 0) : CRGB(4, 0, 0));
        colorBelowCurve(xAttackStart, xAttackEnd, baseAmplitude, attackHeight,
                        activeParameter == 1 ? CRGB(0, 100, 0) : CRGB(0, 4, 0));
        colorBelowCurve(xAttackEnd, xHoldEnd, attackHeight, attackHeight,
                        activeParameter == 2 ? CRGB(0, 0, 100) : CRGB(0, 0, 4));
        colorBelowCurve(xHoldEnd, xDecayEnd, attackHeight, sustainHeight,
                        activeParameter == 3 ? CRGB(100, 100, 0) : CRGB(4, 4, 0));
        colorBelowCurve(xDecayEnd, xSustainEnd, sustainHeight, sustainHeight,
                        activeParameter == 4 ? CRGB(100, 0, 100) : CRGB(4, 0, 4));
        colorBelowCurve(xSustainEnd, xReleaseEnd, sustainHeight, baseAmplitude,
                        activeParameter == 5 ? CRGB(0, 100, 100) : CRGB(0, 4, 4));

        // Overlay white envelope outline:
        drawLine(xDelayStart, baseAmplitude, xDelayEnd, baseAmplitude, CRGB::Red);
        drawLine(xAttackStart, baseAmplitude, xAttackEnd, attackHeight, CRGB(30, 255, 104));
        drawLine(xAttackEnd, attackHeight, xHoldEnd, attackHeight, CRGB::White);
        drawLine(xHoldEnd, attackHeight, xDecayEnd, sustainHeight, CRGB::White);
        drawLine(xDecayEnd, sustainHeight, xSustainEnd, sustainHeight, CRGB::White);
        drawLine(xSustainEnd, sustainHeight, xReleaseEnd, baseAmplitude, CRGB::White);
        drawText(txt, 1, 12, filter_col[activeParameter]);

        int paramValue = SMP.param_settings[SMP.currentChannel][SMP.selectedParameter];
        int width = 12;
        if (paramValue > 9) width = 8;
        // Draw black box background for the number
        for (int x = width; x <= 16; x++) {  // 6 pixels wide for the number area
          for (int y = 5; y <= 11; y++) {    // Box height (adjust as needed)
            light(x, y, CRGB(0, 0, 0));
          }
        }
        drawNumber(paramValue, CRGB(100, 100, 100), 6);
      }




      void drawType(char *txt, int activeParameter) {
        FastLED.clear();

        const int maxIndex = 3;
        if (SMP.param_settings[SMP.currentChannel][TYPE] > maxIndex) {
          SMP.param_settings[SMP.currentChannel][TYPE] = maxIndex;
          currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][TYPE];
          Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
        }
        int typeValue = mapf(SMP.param_settings[SMP.currentChannel][TYPE], 0, maxIndex, 1, maxIndex + 1);
        drawText(txt, 1, 12, filter_col[activeParameter]);

        drawText(channelType[typeValue], 1, 6, filter_col[typeValue]);
        drawNumber(typeValue, CRGB(100, 100, 100), 1);
      }


      void drawWaveforms(const char *txt, int activeParameter) {
        FastLED.clear();

        const int maxWaveformIndex = 3;
        int waveformSetting = SMP.param_settings[SMP.currentChannel][WAVEFORM];

        // Clamp waveform setting within valid bounds and update if needed
        if (waveformSetting > maxWaveformIndex) {
          waveformSetting = maxWaveformIndex;
          currentMode->pos[3] = waveformSetting;
          Encoder[3].writeCounter((int32_t)waveformSetting);
        }

        // Calculate displayed waveform value once, clearly mapping internal state to UI value
        int wavValue = mapf(waveformSetting, 0, maxWaveformIndex, 1, 4);

        // Draw waveform based on wavValue using light(x, y, color)
        switch (wavValue) {
          case 1:  // WAVEFORM_SINE
            {
              const char *pattern[6] = {
                "001100000000",
                "010010000001",
                "100001000010",
                "100001000010",
                "000000100100",
                "000000011000"
              };
              for (int y = 0; y < 6; y++) {
                for (int x = 0; x < 12; x++) {
                  if (pattern[y][x] == '1') {
                    light(x + 1, y + 2, CRGB::Red);
                  }
                }
              }
            }
            break;
          case 2:  // WAVEFORM_SAWTOOTH
            {
              const char *pattern[5] = {
                "0100010001",
                "1100110011",
                "0101010101",
                "0110011001",
                "0100010001"
              };
              for (int y = 0; y < 5; y++) {
                for (int x = 0; x < 10; x++) {
                  if (pattern[y][x] == '1') {
                    light(x + 1, y + 3, CRGB::Red);
                  }
                }
              }
            }
            break;
          case 3:  // WAVEFORM_SQUARE
            {
              const char *pattern[5] = {
                "01111000111",
                "010010001000",
                "01001000100",
                "01001000100",
                "11001111100"
              };
              for (int y = 0; y < 5; y++) {
                for (int x = 0; x < 11; x++) {
                  if (pattern[y][x] == '1') {
                    light(x + 1, y + 3, CRGB::Red);
                  }
                }
              }
            }
            break;
          case 4:  // WAVEFORM_TRIANGLE
            {
              const char *pattern[5] = {
                "00001000000",
                "00010100000",
                "00100010001",
                "01000001010",
                "10000000100"
              };
              for (int y = 0; y < 5; y++) {
                for (int x = 0; x < 11; x++) {
                  if (pattern[y][x] == '1') {
                    light(x + 1, y + 4, CRGB::Red);
                  }
                }
              }
            }
            break;
          default: return;
        }

        // Render UI elements
        drawText(txt, 1, 12, filter_col[activeParameter]);
        drawNumber(wavValue, CRGB(100, 100, 100), 4);
      }

      void drawLine(int x1, int y1, int x2, int y2, CRGB color) {
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true) {
          light(x1, y1, color);
          if (x1 == x2 && y1 == y2) break;
          int e2 = 2 * err;
          if (e2 > -dy) {
            err -= dy;
            x1 += sx;
          }
          if (e2 < dx) {
            err += dx;
            y1 += sy;
          }
        }
      }

      void colorBelowCurve(int xStart, int xEnd, int yStart, int yEnd, CRGB color) {
        for (int x = xStart; x <= xEnd; x++) {
          int yCurve = mapf(x, xStart, xEnd, yStart, yEnd);  // Compute envelope's y-value at this x
          for (int y = 1; y <= yCurve; y++) {
            light(x, y, color);  // Fill from y=1 up to the envelope
          }
        }
      }




      // Updates the display based on the current effect type
      void displayCurrentView() {
        switch (fxType) {
          case 0:
            if (SMP.selectedParameter == 7 && SMP.currentChannel<=3)
              drawType(currentParam, SMP.selectedParameter);
            else if (SMP.selectedParameter == 6 && (SMP.currentChannel<=3 || SMP.currentChannel>=13)){
    
              drawWaveforms(currentParam, SMP.selectedParameter);
            } else
              drawADSR(currentParam, SMP.selectedParameter);
            break;
          case 1:
            drawFilters(currentFilter, SMP.selectedFilter);
            break;
          case 2:
            drawDrums(currentDrum, SMP.selectedDrum);
            break;
          case 4:
            if (SMP.selectedSynth == 0)
              drawInstrument(currentSynth, SMP.selectedSynth);
            else
              drawSynths(currentSynth, SMP.selectedSynth);
            break;
          default:
            // Optionally handle unknown fxType
            break;
        }
        Encoder[0].writeRGBCode(0x0000FF);
      }

      // Handle updates from Encoder 1 for drum selection
      void handleDrumEncoder() {
        if (SMP.currentChannel <= 3 && currentMode->pos[1] != SMP.selectedDrum) {
          FastLED.clear();
          SMP.selectedDrum = currentMode->pos[1];
          currentDrum = activeDrumType[SMP.selectedDrum];
          fxType = 2;
          selectedFX = currentMode->pos[1];

          //Serial.print("DRUM: ");
          //Serial.println(currentDrum);
          //Serial.println(fxType);

          currentMode->pos[3] = SMP.drum_settings[SMP.currentChannel][SMP.selectedDrum];
          Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
          drawDrums(currentDrum, SMP.selectedDrum);
        }
      }

      // Handle updates from Encoder 1 for synth selection
      void handleSynthEncoder() {
        if (SMP.currentChannel == 11 && currentMode->pos[1] != SMP.selectedSynth) {
          FastLED.clear();
          SMP.selectedSynth = currentMode->pos[1];
          currentSynth = activeSynthVoice[SMP.selectedSynth];
          fxType = 4;
          selectedFX = currentMode->pos[1];

          //Serial.print("Synth: ");
          //Serial.println(currentSynth);
          //Serial.println(fxType);

          currentMode->pos[3] = SMP.synth_settings[SMP.currentChannel][SMP.selectedSynth];
          Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
          drawSynths(currentSynth, SMP.selectedSynth);
        }
      }

      // Handle updates from Encoder 2 for filter selection
      void handleFilterEncoder() {
        if (currentMode->pos[2] != SMP.selectedFilter) {

            /*//doesnt work good:
            if (SMP.currentChannel <10 && SMP.selectedFilter>6) {
              //stay at current pos
              SMP.selectedFilter=6;
              currentMode->pos[2] = SMP.selectedFilter;
              Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
              return;
              }
              */

          FastLED.clear();
          
          SMP.selectedFilter = currentMode->pos[2];
          currentFilter = activeFilterType[SMP.selectedFilter];
          fxType = 1;
          selectedFX = currentMode->pos[2];

          //Serial.print("FILTER: ");
          //Serial.println(currentFilter);
          //Serial.println(fxType);

          currentMode->pos[3] = SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter];
          Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
          drawFilters(currentFilter, SMP.selectedFilter);
        }
      }

      // Handle updates from Encoder 0 for parameter selection
      void handleParameterEncoder() {
        if (currentMode->pos[0] != SMP.selectedParameter) {
          FastLED.clear();
          SMP.selectedParameter = currentMode->pos[0];
          fxType = 0;
          selectedFX = currentMode->pos[0];
          currentParam = activeParameterType[SMP.selectedParameter];

          //Serial.print("PARAMS: ");
          //Serial.println(currentParam);

          currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][SMP.selectedParameter];
          Encoder[3].writeCounter((int32_t)currentMode->pos[3]);

          if (SMP.selectedParameter == 7) drawType(currentParam, SMP.selectedParameter);
          else if (SMP.selectedParameter == 6 ) drawWaveforms(currentParam, SMP.selectedParameter);
          else
            drawADSR(currentParam, SMP.selectedParameter);
        }
      }

      // Process the adjustments for the current effect type
      void processAdjustments() {
        switch (fxType) {
          case 2:  // Drum adjustments
            if (currentMode->pos[3] != SMP.drum_settings[SMP.currentChannel][selectedFX]) {
              float mappedValue = processDrumAdjustment(selectedFX, SMP.currentChannel, 3);
              updateDrumValue(selectedFX, SMP.currentChannel, mappedValue);
            }
            break;
          case 4:  // Synth adjustments
            if (currentMode->pos[3] != SMP.synth_settings[SMP.currentChannel][selectedFX]) {
              float mappedValue = processSynthAdjustment(selectedFX, SMP.currentChannel, 3);
              updateSynthValue(selectedFX, SMP.currentChannel, mappedValue);
            }
            break;
          case 1:  // Filter adjustments
            if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][selectedFX]) {
              float mappedValue = processFilterAdjustment(selectedFX, SMP.currentChannel, 3);
              updateFilterValue(selectedFX, SMP.currentChannel, mappedValue);
            }
            break;
          case 0:  // Parameter adjustments
            if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][selectedFX]) {
              float mappedValue = processParameterAdjustment(selectedFX, SMP.currentChannel);
              updateParameterValue(selectedFX, SMP.currentChannel, mappedValue);
            }
            break;
          default:
            // Optionally handle unknown fxType
            break;
        }
      }

      



      // Main refactored function that updates filters and handles encoder changes.
      void setFilters() {
        

        if (SMP.currentChannel>3 && SMP.currentChannel<13 ){
        } else {
          Encoder[0].writeMin((int32_t) 0);  //minval
        }

        // Update the display for the current effect type
        displayCurrentView();
        // Handle encoder changes for various selections
        handleDrumEncoder();
        handleSynthEncoder();
        handleFilterEncoder();
        handleParameterEncoder();
        // Process parameter, drum, synth, or filter adjustments
        processAdjustments();
        
      }