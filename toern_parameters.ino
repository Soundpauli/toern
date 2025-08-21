

// Refactored: setParams combines processParameterAdjustment and updateParameterValue
void setParams(ParameterType paramType, int index) {

  float mappedValue;
  switch (paramType) {
    /*case WAVEFORM: {
      mappedValue = mapf(SMP.param_settings[index][WAVEFORM], 0, maxIndex, 1, maxIndex + 1);
      handleWaveformChange(index, (unsigned int)mappedValue);
      break;
    }*/
    /*case DELAY:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[DELAY]);
      if (index > 9) envelopes[index]->delay(mappedValue);
      break;
    */
    case ATTACK:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, maxParamVal[ATTACK], 0);
      envelopes[index]->attack(mappedValue);
      break;
    case DECAY:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[DECAY]);
      envelopes[index]->decay(mappedValue);
      break;
    /*case HOLD:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[HOLD]);
      envelopes[index]->hold(mappedValue);
      break;*/
    case SUSTAIN:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[SUSTAIN]);
      envelopes[index]->sustain(mappedValue);
      break;
    case RELEASE:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[RELEASE]);
      envelopes[index]->release(mappedValue);
      break;
  }
}


      void drawWaveforms(const char *txt, int activeParameter) {
        FastLED.clear();

        const int maxWaveformIndex = 3;
        int waveformSetting = SMP.param_settings[GLOB.currentChannel][WAVEFORM];

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


// Handle waveform changes
void handleWaveformChange(int index, unsigned int waveformType) {
  //Serial.println(index);

  if (synths[index][0] != nullptr && synths[index][0] != 0) {
    switch (waveformType) {
      case 1:
        synths[index][0]->begin(WAVEFORM_SINE);
        break;
      case 2:
        synths[index][0]->begin(WAVEFORM_SAWTOOTH);
        break;
      case 3:
        synths[index][0]->begin(WAVEFORM_SQUARE);
        break;
      case 4:
        synths[index][0]->begin(WAVEFORM_TRIANGLE);
        break;
      default: return;
    }
  }
}

// Set default filter values for a single channel
void setFiltersDefaultValues(int ch) {
  SMP.filter_settings[ch][PASS] = 15;
  SMP.filter_settings[ch][FREQUENCY] = 0;
  SMP.filter_settings[ch][REVERB] = 0;
  SMP.filter_settings[ch][BITCRUSHER] = 0;
  SMP.filter_settings[ch][DETUNE] = 16;
  SMP.filter_settings[ch][OCTAVE] = 16;


  //SND?
  setFilters(PASS, ch, true);
  setFilters(FREQUENCY, ch, true);
  setFilters(REVERB, ch, true);
  setFilters(BITCRUSHER, ch, true);
  setFilters(DETUNE, ch, true);
  setFilters(OCTAVE, ch, true);
  initSliders(filterPage[GLOB.currentChannel],GLOB.currentChannel);
  updateSynthVoice(11);
}


// Set default envelope values for a single channel
//ASDR
void setEnvelopeDefaultValues(int ch) {
  SMP.param_settings[ch][ATTACK] = 32;
  SMP.param_settings[ch][DECAY] = 32;
  SMP.param_settings[ch][SUSTAIN] = 10;
  SMP.param_settings[ch][RELEASE] = 5;
  setParams(ATTACK, ch);
  setParams(DECAY, ch);
  setParams(SUSTAIN, ch);
  setParams(RELEASE, ch);
  initSliders(filterPage[GLOB.currentChannel],GLOB.currentChannel);
  updateSynthVoice(11);
}


// Set default drum values for a single channel
//DRUM
void setDrumDefaultValues(int ch) {
  SMP.drum_settings[ch][DRUMTONE] = 0;
  SMP.drum_settings[ch][DRUMDECAY] = 16;
  SMP.drum_settings[ch][DRUMPITCH] = 16;
  SMP.drum_settings[ch][DRUMTYPE] = 1;
  setDrums(DRUMTONE, ch);
  setDrums(DRUMDECAY, ch);
  setDrums(DRUMPITCH, ch);
  setDrums(DRUMTYPE, ch);
  initSliders(filterPage[GLOB.currentChannel],GLOB.currentChannel);
  updateSynthVoice(11);
}


// Set default synth values for a single channel
//SYNTH
void setSynthDefaultValues(int ch) {
  SMP.synth_settings[ch][CUTOFF] = 0;
  SMP.synth_settings[ch][RESONANCE] = 0;
  SMP.synth_settings[ch][FILTER] = 0;
  SMP.synth_settings[ch][CENT] = 16;
  SMP.synth_settings[ch][SEMI] = 0;
  SMP.synth_settings[ch][INSTRUMENT] = 0;
  SMP.synth_settings[ch][FORM] = 0;

   initSliders(filterPage[GLOB.currentChannel],GLOB.currentChannel);
   // updateSynthVoice(11) is called at the end of resetAllToDefaults() instead
}





// Reset ALL filters, envelopes, drums and synths to default values
void resetAllToDefaults() {
  // Visual feedback - clear display and show reset message
  FastLEDclear();
  drawText("RESET", 6, 8, CRGB(255, 100, 0));
  FastLEDshow();
 // delay(500);
  
  // Reset all channels (1-8, 11, 13-14)
  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);
  
  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    
    // Reset filters
    setFiltersDefaultValues(ch);
    
    // Reset envelopes
    setEnvelopeDefaultValues(ch);
    
    // Reset drums (only for channels 1-3)
    if (ch >= 1 && ch <= 3) {
      setDrumDefaultValues(ch);
    }
    
    // Reset synths (only for channels 11, 13-14)
    if (ch == 11 || ch == 13 || ch == 14) {
      setSynthDefaultValues(ch);
    }
  }
  
  // Reset drum engine defaults
  setDrumDefaults(true);
  
  // Reset all filter mixer gains to default
  extern const int ALL_CHANNELS[];
  extern const int NUM_ALL_CHANNELS;
  for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
    int idx = ALL_CHANNELS[i];
    setFilterDefaults(idx);
  }
  
  // Force all mixer gains to target
  extern void forceAllMixerGainsToTarget();
  forceAllMixerGainsToTarget();
  
  // Update synth voice for channel 11
  updateSynthVoice(11);
  
  // Initialize sliders for current channel
  initSliders(filterPage[GLOB.currentChannel], GLOB.currentChannel);
  
  // Switch to draw mode after reset
  switchMode(&draw);
}
