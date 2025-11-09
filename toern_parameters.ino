// Refactored: setParams combines processParameterAdjustment and updateParameterValue
void setParams(ParameterType paramType, int index) {

  float mappedValue;
  switch (paramType) {
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
  // EFX setting: 0 = SAMPLE mode (default for all channels), 1 = DRUM mode (only for channels 1-3)
  SMP.filter_settings[ch][EFX] = 0;

  setFilters(PASS, ch, true);
  setFilters(FREQUENCY, ch, true);
  setFilters(REVERB, ch, true);
  setFilters(BITCRUSHER, ch, true);
  setFilters(DETUNE, ch, true);
  setFilters(OCTAVE, ch, true);
  // EFX parameter doesn't need setFilters call - it's just a control parameter
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
  
  // Reset all audio effects/filters to clean defaults
  extern void resetAllAudioEffects();
  resetAllAudioEffects();
  
  // Add delay before switching mode
  delay(300);
  
  // Switch to draw mode after reset
  switchMode(&draw);
}
