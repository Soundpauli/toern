



void drawFilters(char *txt, int activeFilter) {
  FastLED.clear();
  // Amplitude definitions:
  int baseAmplitude = 1;        // baseline (minimum amplitude)
  drawText(txt, 1, 12, filter_col[activeFilter]);
  float FilterValue = mapf(SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter],0, maxfilterResolution, 0, 10);
  
  drawNumber(FilterValue, CRGB(100,100,100),5);
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
                  activeParameter == 2 ? CRGB(100, 0, 0) : CRGB(4, 0, 0));
  colorBelowCurve(xAttackStart, xAttackEnd, baseAmplitude, attackHeight,
                  activeParameter == 3 ? CRGB(0, 100, 0) : CRGB(0, 4, 0));
  colorBelowCurve(xAttackEnd, xHoldEnd, attackHeight, attackHeight,
                  activeParameter == 4 ? CRGB(0, 0, 100) : CRGB(0, 0, 4));
  colorBelowCurve(xHoldEnd, xDecayEnd, attackHeight, sustainHeight,
                  activeParameter == 5 ? CRGB(100, 100, 0) : CRGB(4, 4, 0));
  colorBelowCurve(xDecayEnd, xSustainEnd, sustainHeight, sustainHeight,
                  activeParameter == 6 ? CRGB(100, 0, 100) : CRGB(4, 0, 4));
  colorBelowCurve(xSustainEnd, xReleaseEnd, sustainHeight, baseAmplitude,
                  activeParameter == 7 ? CRGB(0, 100, 100) : CRGB(0, 4, 4));

  // Overlay white envelope outline:
  drawLine(xDelayStart, baseAmplitude, xDelayEnd, baseAmplitude, CRGB::Red);
  drawLine(xAttackStart, baseAmplitude, xAttackEnd, attackHeight, CRGB(30, 255, 104));
  drawLine(xAttackEnd, attackHeight, xHoldEnd, attackHeight, CRGB::White);
  drawLine(xHoldEnd, attackHeight, xDecayEnd, sustainHeight, CRGB::White);
  drawLine(xDecayEnd, sustainHeight, xSustainEnd, sustainHeight, CRGB::White);
  drawLine(xSustainEnd, sustainHeight, xReleaseEnd, baseAmplitude, CRGB::White);
  drawText(txt, 1, 12, filter_col[activeParameter]);
  
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


void setFilters() {
  // Display current view based on effect type
  if (fxType == 0) {
    drawADSR(currentFilter, SMP.selectedParameter);
  } else if (fxType == 1) {
    drawFilters(currentFilter, SMP.selectedFilter);
  }

  // Handle Encoder 3: Filter Selection
  if (currentMode->pos[2] != SMP.selectedFilter) {
    FastLED.clear();
    SMP.selectedFilter = currentMode->pos[2];
    currentFilter = activeFilterType[SMP.selectedFilter];
    fxType = 1;
    selectedFX = currentMode->pos[2];
    Serial.print("FILTER: ");
    Serial.println(currentFilter);
    Serial.println(fxType);
    currentMode->pos[3] = SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    drawFilters(currentFilter, SMP.selectedFilter);
  }

  // Handle Encoder 1: Parameter Selection
  if (currentMode->pos[0] != SMP.selectedParameter) {
    FastLED.clear();
    SMP.selectedParameter = currentMode->pos[0];
    fxType = 0;
    selectedFX = currentMode->pos[0];
    currentFilter = activeParameterType[SMP.selectedParameter];
    
    Serial.print("PARAMS: ");
    Serial.println(currentFilter);
    currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][SMP.selectedParameter];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    drawADSR(currentFilter, SMP.selectedParameter);
  }

  // Process filter adjustments
  if (fxType == 1 && currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][selectedFX]) {
    float mappedValue = processFilterAdjustment(selectedFX, SMP.currentChannel, 3);
    updateFilterValue(selectedFX, SMP.currentChannel, mappedValue);
  }

  // Process parameter adjustments
  if (fxType == 0 && currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][selectedFX]) {
    float mappedValue = processParameterAdjustment(selectedFX);
    updateParameterValue(selectedFX, SMP.currentChannel ,mappedValue);
  }
}

// Helper function to process filter adjustments
float processFilterAdjustment(int filterType, int index, int encoder) {
  SMP.filter_settings[SMP.currentChannel][filterType] = currentMode->pos[encoder];
  float mappedValue;
  if (filterType == HIGH) mappedValue = mapf(SMP.filter_settings[SMP.currentChannel][filterType], 0, maxfilterResolution, 5, 10000);
  if (filterType == FREQUENCY) mappedValue = mapf(SMP.filter_settings[SMP.currentChannel][filterType], 0, maxfilterResolution, 5, 10000);
  if (filterType == FREQUENCY) mappedValue = mapf(SMP.filter_settings[SMP.currentChannel][filterType], 0, maxfilterResolution, 5, 10000);
  if (filterType == ECHO) mappedValue = mapf(SMP.filter_settings[SMP.currentChannel][filterType], 0, maxfilterResolution, 0, 2000);
  if (filterType == REVERB) mappedValue = mapf(SMP.filter_settings[SMP.currentChannel][filterType], 0, maxfilterResolution, 0, 1);
  if (filterType == CHORUS) mappedValue = mapf(SMP.filter_settings[SMP.currentChannel][filterType], 0, maxfilterResolution, 0, 1);
  if (filterType == BITCRUSHER) mappedValue = mapf(SMP.filter_settings[SMP.currentChannel][filterType], 0, maxfilterResolution, 0, 16);
  if (filterType == FLANGER) mappedValue = mapf(SMP.filter_settings[SMP.currentChannel][filterType], 0, maxfilterResolution, 0, 1);
  //if (filterType == DETUNE) // find in main file.
  return mappedValue;
}

// Helper function to process parameter adjustments
float processParameterAdjustment(int paramType) {
  SMP.param_settings[SMP.currentChannel][paramType] = currentMode->pos[3];
  float mappedValue;
  switch (paramType) {
    case DELAY:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, 250); // max 0.25sec delay!
      break;
    case ATTACK:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, 2000);
      break;
    case HOLD:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, 1000);
      break;
    case DECAY:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, 1000);
      break;
    case SUSTAIN:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, 1.0);
      break;
    case RELEASE:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, 1000);
      break;
    case TYPE:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 1, maxfilterResolution, 0, 4.0);
      break;
    case WAVEFORM:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 1, 4);
      handleWaveformChange(SMP.currentChannel, (unsigned int)mappedValue);
      break;
    default:
    mappedValue = 0;
  }
  return mappedValue;
}

// Update filter values
void updateFilterValue(int filterType, int index, float value) {
  // Common function to update all filter types
      Serial.print(filterType);
      Serial.print(" :==>: ");
      Serial.println(value);

     
switch (filterType) {
    case FREQUENCY:
            filters[SMP.currentChannel]->frequency(value);
    break;

    case ECHO:
    break;

    case REVERB:
          freeverbs[index]->roomsize(value);
          freeverbs[index]->damping(0.8);
    break;

    case CHORUS:
    break;

    
    case FLANGER:
    if (value <= 0.01) {  // Bypass mode
        // Set to bypass mode once, without reinitializing continuously.
        if (!bypassSet) {
            flangers[index]->voices(FLANGE_DELAY_PASSTHRU, 0, 0);
            // Optionally, call begin() only if needed to set the initial bypass state.
            // flange13.begin(delayline, FLANGE_DELAY_LENGTH, 20, 40, 0.05);
            bypassSet = true;
        }
    } else {
        // Active mode: reset bypass flag
        static float lastValue = -1.0;
        bypassSet = false;
        // Smooth out abrupt jumps if the value has changed significantly
        if (fabs(value - lastValue) > 0.001) {
            int s_idx = mapf(value, 0.0, 1.0, -4, 4);
            int s_depth = mapf(value, 0.0, 1.0, 0, 100);
            float s_freq = mapf(value, 0.0, 1.0, 0.05, 0.25);
            // Update flanger with new parameters
            flangers[index]->voices(s_idx, s_depth, s_freq);
            // Consider calling begin() only on major changes or during initialization.
            flangers[index]->begin(delayline, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq);
            lastValue = value;
        }
    }
    break;
      
    case BITCRUSHER: 
        // Map the control value (0 to 10) to a bit depth:
        // At 0 → 1 (maximum crushing), at 10 → 16 (passthrough)
        int xbitDepth = constrain(value, 1, 16);
        // Map the control value (0 to 10) to a sample rate:
        // At 0, we choose a low sample rate (e.g., 1000Hz) for maximum effect,
        // and at 10 we want 44100Hz (passthrough).
        int xsampleRate = round(mapf(value, 0, 16, 1000, 44100));
        // Apply the parameters to the bitcrusher.
        bitcrushers[index]->bits(xbitDepth); 
        bitcrushers[index]->sampleRate(xsampleRate);
    break;
    }

  // Call the new update function to ensure changes are applied
  updateFiltersAndParameters();    
}

// Update parameter values
void updateParameterValue(int paramType, int index, float value) {
  switch (paramType) {
    case DELAY:
      envelopes[index]->delay(value);
      break;
    case ATTACK:
      envelopes[index]->attack(value);
      break;
    case HOLD:
      envelopes[index]->hold(value);
      break;
    case DECAY:
      envelopes[index]->decay(value);
      break;
    case SUSTAIN:
      envelopes[index]->sustain(value);
      break;
    case RELEASE:
      envelopes[index]->release(value);
      break;
    case TYPE:
      break;
  }
  
  // Call the new update function to ensure changes are applied
  updateFiltersAndParameters();
}

// Handle waveform changes
void handleWaveformChange(int index, unsigned int waveformType) {
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
  }
  // Call the new update function to ensure changes are applied
  updateFiltersAndParameters();
}

// New function to ensure updates are applied
void updateFiltersAndParameters() {
  // Add code here to ensure changes are properly applied
  // This might include refreshing displays, updating audio processing chain, etc.
}
void setFilterDefaults(int channel) {
  // Set gain values depending on channel
  switch (channel) {
    case 11:
      filtermixer11.gain(0, 1);
      filtermixer11.gain(1, 0);
      filtermixer11.gain(2, 0);
      filtermixer11.gain(3, 0);
      break;
    case 12:
      filtermixer12.gain(0, 1);
      filtermixer12.gain(1, 0);
      filtermixer12.gain(2, 0);
      filtermixer12.gain(3, 0);
      break;
    case 13:
      filtermixer13.gain(0, 1);
      filtermixer13.gain(1, 0);
      filtermixer13.gain(2, 0);
      filtermixer13.gain(3, 0);
      break;
    case 14:
      filtermixer14.gain(0, 1);
      filtermixer14.gain(1, 0);
      filtermixer14.gain(2, 0);
      filtermixer14.gain(3, 0);
      break;
    default:
      return;  // Invalid channel
  }

  SMP.filter_settings[channel][OCTAVE] = 32;  // middle = 0
  SMP.filter_settings[channel][DETUNE] = 32;  // middle = 0
  filters[channel]->resonance(0.0);           // default resonance off
  filters[channel]->frequency(10000);
  freeverbs[channel]->roomsize(0);
  freeverbs[channel]->damping(0);             // fixed damping
  bitcrushers[channel]->bits(16);
  bitcrushers[channel]->sampleRate(44100);

  int s_idx = mapf(0, 0.0, 1.0, -4, 4);
  int s_depth = mapf(0, 0.0, 1.0, 0, 100);
  float s_freq = mapf(0, 0.0, 1.0, 0.05, 0.25);

  flangers[channel]->voices(s_idx, s_depth, s_freq);
  flangers[channel]->begin(delayline, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq);

  updateFiltersAndParameters();
}

void setDahdsrDefaults(bool allChannels) {
  if (allChannels) {
    for (int ch = 0; ch < 16; ch++) {
      // First clear all 15 settings to zero
      for (int param = 0; param < 15; param++) {
        SMP.param_settings[ch][param] = 0;
      }
      // Now assign default envelope values for a DAHDSR shape:
      SMP.param_settings[ch][DELAY] = 0;    // No delay
      SMP.param_settings[ch][ATTACK] = 0;   // Quick attack duration
      SMP.param_settings[ch][HOLD] = 5;     // MIN hold time
      SMP.param_settings[ch][DECAY] = 5;    // MID decay period
      SMP.param_settings[ch][SUSTAIN] = 16;  // MID sustain level
      SMP.param_settings[ch][RELEASE] = 16;  // MID release
      
    }
  } else {
    int ch = SMP.currentChannel;
    for (int param = 0; param < 15; param++) {
      SMP.param_settings[ch][param] = 0;
    }
    // Now assign default envelope values for a DAHDSR shape:
    SMP.param_settings[ch][DELAY] = 0;    // No delay
    SMP.param_settings[ch][ATTACK] = 0;   // Quick attack duration
    SMP.param_settings[ch][HOLD] = 5;     // MIN hold time
    SMP.param_settings[ch][DECAY] = 5;    // MID decay period
    SMP.param_settings[ch][SUSTAIN] = 16;  // MID sustain level
    SMP.param_settings[ch][RELEASE] = 16;  // MID release
  }
}


void resetAllFilters() {
  for (unsigned int i = 0; i < maxFilters; i++) {
    filters[i]->frequency(0);
    filters[i]->resonance(0);
  }
}

