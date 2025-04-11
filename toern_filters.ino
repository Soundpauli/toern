
void drawDrums(char *txt, int activeDrum) {
  FastLED.clear();
  // Amplitude definitions:
  int baseAmplitude = 1;  // baseline (minimum amplitude)
  drawText(txt, 1, 12, filter_col[activeDrum]);
  float DrumValue = SMP.drum_settings[SMP.currentChannel][SMP.selectedDrum];  //mapf(SMP.drum_settings[SMP.currentChannel][SMP.selectedDrum], 0, maxfilterResolution, 0, 10);
  drawNumber(DrumValue, CRGB(100, 0, 100), 4);
}

void drawSynths(char *txt, int activeSynth) {
  FastLED.clear();
  // Amplitude definitions:
  drawText(txt, 1, 12, filter_col[activeSynth]);
  float SynthValue = SMP.synth_settings[SMP.currentChannel][SMP.selectedSynth];  //mapf(SMP.drum_settings[SMP.currentChannel][SMP.selectedDrum], 0, maxfilterResolution, 0, 10);
  drawNumber(SynthValue, CRGB(100, 0, 100), 4);
}


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


void drawInstrument(char *txt, int activeSynth) {
  FastLED.clear();
  
  const int maxIndex =  10;
  if (SMP.synth_settings[SMP.currentChannel][INSTRUMENT] > maxIndex) {
    SMP.synth_settings[SMP.currentChannel][INSTRUMENT] = maxIndex;
    currentMode->pos[3] = SMP.synth_settings[SMP.currentChannel][INSTRUMENT];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
  }
  int instrumentValue = mapf(SMP.synth_settings[SMP.currentChannel][INSTRUMENT], 0, maxIndex, 1, maxIndex + 1);
  drawText(txt, 1, 12, filter_col[activeSynth]);
  
  drawText(SynthVoices[instrumentValue], 1, 6, filter_col[instrumentValue]);
  drawNumber(instrumentValue, CRGB(100, 100, 100), 1);
  if (SMP.currentChannel == 11) switchSynthVoice(instrumentValue, 0);
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


void switchSynthVoice(int typeValue, int ch) {
    switch(typeValue) {
        case 1: bass_synth(ch);
            break;
        case 2: keys_synth(ch);
            break;
        case 3: chiptune_synth(ch);
            break;
        case 4: pad_synth(ch);
            break;
        case 5: wow_synth(ch);
            break;
        case 6: organ_synth(ch);
            break;
        case 7: flute_synth(ch);
            break;
        case 8: lead_synth(ch);
            break;
        case 9: arp_synth(ch);
            break;
        case 10: brass_synth(ch);
            break;
        default:
            // Optionally, handle any values of typeValue that are not supported.
            break;
    }
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


void setFilters() {
  // Display current view based on effect type
  if (fxType == 0) {
    if (SMP.selectedParameter == 0) drawType(currentFilter, SMP.selectedParameter);
    if (SMP.selectedParameter == 1) drawWaveforms(currentFilter, SMP.selectedParameter);
    if (SMP.selectedParameter > 1) drawADSR(currentFilter, SMP.selectedParameter);
  } else if (fxType == 1) {
    drawFilters(currentFilter, SMP.selectedFilter);
  } else if (fxType == 2) { drawDrums(currentDrum, SMP.selectedDrum);
  } else if (fxType ==4){
    if (SMP.selectedSynth == 0) drawInstrument(currentSynth, SMP.selectedSynth);
    if (SMP.selectedSynth > 0) drawSynths(currentSynth, SMP.selectedSynth);
  }


if (SMP.currentChannel<=3){
  // Handle Encoder 1: Filter Selection
  if (currentMode->pos[1] != SMP.selectedDrum) {
    FastLED.clear();
    SMP.selectedDrum = currentMode->pos[1];
    currentDrum = activeDrumType[SMP.selectedDrum];
    fxType = 2;
    selectedFX = currentMode->pos[1];
    Serial.print("DRUM: ");
    Serial.println(currentDrum);
    Serial.println(fxType);
    currentMode->pos[3] = SMP.drum_settings[SMP.currentChannel][SMP.selectedDrum];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    drawDrums(currentDrum, SMP.selectedDrum);
  }
}

if (SMP.currentChannel==11){
  // Handle Encoder 1: Filter Selection
  if (currentMode->pos[1] != SMP.selectedSynth) {
    FastLED.clear();
    SMP.selectedSynth = currentMode->pos[1];
    currentSynth = activeSynthVoice[SMP.selectedSynth];
    fxType = 4;
    selectedFX = currentMode->pos[1];
    Serial.print("Synth: ");
    Serial.println(currentSynth);
    Serial.println(fxType);
    currentMode->pos[3] = SMP.synth_settings[SMP.currentChannel][SMP.selectedSynth];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    drawSynths(currentSynth, SMP.selectedSynth);
  }
}

  // Handle Encoder 2: Filter Selection
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

  // Handle Encoder 0: Parameter Selection
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

    if (SMP.selectedParameter == 0) drawType(currentFilter, SMP.selectedParameter);
    if (SMP.selectedParameter == 1) drawWaveforms(currentFilter, SMP.selectedParameter);
    if (SMP.selectedParameter > 1) drawADSR(currentFilter, SMP.selectedParameter);
  }

  // Process drum adjustments
  if (fxType == 2 && currentMode->pos[3] != SMP.drum_settings[SMP.currentChannel][selectedFX]) {
    float mappedValue = processDrumAdjustment(selectedFX, SMP.currentChannel, 3);
    updateDrumValue(selectedFX, SMP.currentChannel, mappedValue);
  }

  // Process Synth adjustments
  if (fxType == 4 && currentMode->pos[3] != SMP.synth_settings[SMP.currentChannel][selectedFX]) {
    float mappedValue = processSynthAdjustment(selectedFX, SMP.currentChannel, 3);
    updateSynthValue(selectedFX, SMP.currentChannel, mappedValue);
  }

  // Process filter adjustments
  if (fxType == 1 && currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][selectedFX]) {
    float mappedValue = processFilterAdjustment(selectedFX, SMP.currentChannel, 3);
    updateFilterValue(selectedFX, SMP.currentChannel, mappedValue);
  }

  // Process parameter adjustments
  if (fxType == 0 && currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][selectedFX]) {
    float mappedValue = processParameterAdjustment(selectedFX, SMP.currentChannel);
    updateParameterValue(selectedFX, SMP.currentChannel, mappedValue);
  }
}


// Helper function to process filter adjustments
float processSynthAdjustment(SynthTypes synthType, int index, int encoder) {
  SMP.synth_settings[index][synthType] = currentMode->pos[encoder];
  Serial.print(":::::");
  Serial.println(SMP.drum_settings[index][synthType]);
  int mappedValue;
  
    const int maxIndex = 10;
    if (SMP.synth_settings[SMP.currentChannel][synthType] > maxIndex) {
      SMP.synth_settings[SMP.currentChannel][synthType] = maxIndex;
      currentMode->pos[3] = SMP.synth_settings[SMP.currentChannel][synthType];
      Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    }
    mappedValue = mapf(SMP.synth_settings[SMP.currentChannel][synthType], 0, maxIndex, 1, maxIndex + 1); 
  
  Serial.print(synthType);
  Serial.print(" #####---->");
  Serial.println(mappedValue);
  return mappedValue;
}




// Helper function to process filter adjustments
float processDrumAdjustment(DrumTypes drumType, int index, int encoder) {
  SMP.drum_settings[index][drumType] = currentMode->pos[encoder];
  Serial.print(":::::");
  Serial.println(SMP.drum_settings[index][drumType]);
  float mappedValue;
  if (drumType == DRUMDECAY) mappedValue = mapf(SMP.drum_settings[index][drumType], 0, maxfilterResolution, 0, 1023);
  if (drumType == DRUMPITCH) mappedValue = mapf(SMP.drum_settings[index][drumType], 0, maxfilterResolution, 0, 1023);
  if (drumType == DRUMTYPE) {

    const int maxIndex = 3;
    if (SMP.drum_settings[SMP.currentChannel][drumType] > maxIndex) {
      SMP.drum_settings[SMP.currentChannel][drumType] = maxIndex;
      currentMode->pos[3] = SMP.drum_settings[SMP.currentChannel][drumType];
      Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    }
    int mappedValue = mapf(SMP.drum_settings[SMP.currentChannel][drumType], 0, maxIndex, 1, maxIndex + 1);  // = mapf(SMP.drum_settings[index][drumType], 0, maxfilterResolution, 1, 3);
  }
  Serial.print(drumType);
  Serial.print(" #####---->");
  Serial.println(mappedValue);
  return mappedValue;
}

// Helper function to process filter adjustments
float processFilterAdjustment(FilterType filterType, int index, int encoder) {

  SMP.filter_settings[index][filterType] = currentMode->pos[encoder];
  float mappedValue;
  if (filterType == LOWPASS) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 5, 10000);
  if (filterType == HIGHPASS) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 5, 10000);
  if (filterType == FREQUENCY) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 5, 10000);
  if (filterType == REVERB) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0, 1);
  if (filterType == BITCRUSHER) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 1, 16);
  if (filterType == FLANGER) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0, 1);
  //if (filterType == DETUNE) // find in main file.

  return mappedValue;
}

// Helper function to process parameter adjustments
float processParameterAdjustment(int paramType, int index) {
  SMP.param_settings[index][paramType] = currentMode->pos[3];
  float mappedValue;
  switch (paramType) {
    case TYPE:
      {
        int maxIndex =  3;
        if (SMP.param_settings[SMP.currentChannel][TYPE] > maxIndex) {
          SMP.param_settings[SMP.currentChannel][TYPE] = maxIndex;
          currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][TYPE];
          Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
        }
        mappedValue = mapf(SMP.param_settings[SMP.currentChannel][TYPE], 0, maxIndex, 1, maxIndex + 1);  
        break;
      }

    case WAVEFORM:
      {
        int maxIndex = 3;
        if (SMP.param_settings[SMP.currentChannel][WAVEFORM] > maxIndex) {
          SMP.param_settings[SMP.currentChannel][WAVEFORM] = maxIndex;
          currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][WAVEFORM];
          Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
        }
        mappedValue = mapf(SMP.param_settings[SMP.currentChannel][WAVEFORM], 0, maxIndex, 1, maxIndex + 1);  
        handleWaveformChange(index, (unsigned int)mappedValue);
        break;
      }
    case DELAY:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[DELAY]);  //250
      break;
    case ATTACK:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[ATTACK]);  //2000
      break;
    case HOLD:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[HOLD]);  //1000
      break;
    case DECAY:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[DECAY]);  //1000
      break;
    case SUSTAIN:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[SUSTAIN]);  //1.0
      break;
    case RELEASE:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[RELEASE]);  //1000
      break;

    case LENGTH:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[LENGTH]);  //1000
      break;

    case SECONDMIX:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[SECONDMIX]);  //1
      break;

    case PITCHMOD:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[PITCHMOD]);  //1
      break;

    default:
      mappedValue = 0;
  }
  return mappedValue;
}



// Update filter values
void updateDrumValue(DrumTypes drumType, int index, float value) {
  // Common function to update all filter types
  Serial.print(drumType);
  Serial.print(" d==>: ");
  Serial.println(value);

  switch (drumType) {
    case DRUMDECAY:
      break;

    case DRUMPITCH:
      break;

    case DRUMTYPE:
      break;

    default: return;
  }
  float tone = mapf(SMP.drum_settings[SMP.currentChannel][DRUMTONE], 0, 64, 0, 1023);
  float dec = mapf(SMP.drum_settings[SMP.currentChannel][DRUMDECAY], 0, 64, 0, 1023);
  float pit = mapf(SMP.drum_settings[SMP.currentChannel][DRUMPITCH], 0, 64, 0, 1023);
  float typ = mapf(SMP.drum_settings[SMP.currentChannel][DRUMTYPE], 0, 64, 1, 3);


}




// Update filter values
void updateSynthValue(SynthTypes synthType, int index, float value) {
  // Common function to update all filter types
  Serial.print(synthType);
  Serial.print(" S==>: ");
  Serial.println(value);
  switch (synthType) {
    case INSTRUMENT:
      break;

    case BASS:
      break;

    case WOW:
      break;

    default: return;
  }  
}






// Update filter values
void updateFilterValue(FilterType filterType, int index, float value) {
  // Common function to update all filter types
  Serial.print(filterType);
  Serial.print(" :==>: ");
  Serial.println(value);


  switch (filterType) {
    case FREQUENCY:
      filters[index]->frequency(value);
      filtermixers[index]->gain(0, 0.0);
      filtermixers[index]->gain(1, 1.0);
      filtermixers[index]->gain(2, 0.0);
      break;

    case HIGHPASS:
      filters[index]->frequency(value);
      filtermixers[index]->gain(0, 0.0);
      filtermixers[index]->gain(1, 0.0);
      filtermixers[index]->gain(2, 1.0);
      break;

    case LOWPASS:
      filters[index]->frequency(value);
      filtermixers[index]->gain(0, 1.0);
      filtermixers[index]->gain(1, 0.0);
      filtermixers[index]->gain(2, 0.0);
      break;


    case REVERB:
      if (freeverbs[index] != nullptr && freeverbs[index] != 0) {

        if (freeverbmixers[index] != 0 && value < 0.1) {
          // Effectively bypass the reverb
          freeverbmixers[index]->gain(0, 0);  // Mute wet
          freeverbmixers[index]->gain(3, 1);  // Enable dry
        } else {
          // Apply reverb settings
          freeverbs[index]->roomsize(value);
          freeverbs[index]->damping(0.5);
          if (freeverbmixers[index] != 0) {
            // Enable wet, disable dry
            freeverbmixers[index]->gain(0, 1);  // Enable wet
            freeverbmixers[index]->gain(3, 1);  // dont Mute dry
          }
        }
      }
      break;

  case FLANGER:
  if (flangers[index] != nullptr) {
    if (value <= 0.01) {  // Bypass mode
      if (!bypassSet) {
        flangers[index]->voices(FLANGE_DELAY_PASSTHRU, 0, 0);
        bypassSet = true;
      }
    } else {
      // Active mode - full frequency flanger
      bypassSet = false;
      
      static float lastValue = -1.0;
      if (fabs(value - lastValue) > 0.001) {
        // Custom mapping curve (gentle at first, then aggressive)
        float normalized = value < 0.3 ? value * 0.7 : 0.21 + (value - 0.3) * 1.7;
        
        // Primary flanger voice (bass emphasis)
        int offset = (int)mapf(normalized, 0.0, 1.0, 30, 300);  // Samples (1.5ms-15ms @44.1kHz)
        int depth = (int)mapf(normalized, 0.0, 1.0, 60, 150);   // 60-150% depth
        float delayRate = mapf(normalized, 0.0, 1.0, 0.08, 0.6); // Hz
        
        // Secondary modulation for richer sweeps
        float harmonicRate = delayRate * 1.3; // Slightly detuned
        
        // Update flanger with modulated parameters
        flangers[index]->voices(
          offset + (int)(10 * sin(millis() * 0.001 * harmonicRate)), // Dynamic offset
          depth,
          delayRate + (0.02 * sin(millis() * 0.0007)) // Slight rate modulation
        );
        
        lastValue = value;
      }
    }
  }
  break;
    case BITCRUSHER:
      // Map value (1 = clean, 16 = max crush) to bit depth
      int xbitDepth = constrain(value, 1, 16);
      // Optional volume influence based on bit depth (not needed anymore)
      // float xbitDepthVol = mapf(value, 1, 16, 1, 0); // Remove this

      // Sample rate mapping (cleaner has full rate, crushed has low rate)
      int xsampleRate = round(mapf(value, 1, 16, 44100, 1000));

      // Apply settings
      bitcrushers[index]->bits(16 - xbitDepth);  // invert bit depth if needed
      bitcrushers[index]->sampleRate(xsampleRate);

      // Calculate channel volume separately (assuming SMP.channelVol is 1..16)
      float channelvolume = mapf(SMP.channelVol[SMP.currentChannel], 1, 16, 1, 0.2);

      // Auto-gain: 1 (clean) -> channelVol | 16 (max crush) -> 0.2
      //float crushCompGain = mapf(value, 1, 16, channelvolume, 0.2);
      float crushCompGain = mapf(value, 1, 16, max(channelvolume, 1), 0.2);

      // Apply gain
      amps[index]->gain(crushCompGain);

      // Debugging
      Serial.print("BitDepth: ");
      Serial.print(16 - xbitDepth);
      Serial.print(" | SampleRate: ");
      Serial.print(xsampleRate);
      Serial.print(" | CompGain: ");
      Serial.println(crushCompGain);
      break;
  }

  
}

// Update parameter values
void updateParameterValue(int paramType, int index, float value) {

  Serial.println(index);
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
      {
        int synthType = value;
        Serial.println(synthType);

        if (waveformmixers[index] != nullptr && waveformmixers[index] != 0) {

          switch (synthType) {
            case 1:
              Serial.print("enable drum at Input0 at channel ");
              Serial.println(index);
              waveformmixers[index]->gain(0, 0);  // Disable SAMPLE
              waveformmixers[index]->gain(1, 1);  // Enable drum channel

              break;

            case 2:
              Serial.print("enable synth at Input1 for channel ");
              Serial.println(index);
              waveformmixers[index]->gain(0, 1);  // Enable SAMPLE channel
              waveformmixers[index]->gain(1, 0);  // disable drum channel
            break;

            default: return;
          }
          break;
        }
        break;
      }
  }

  
}

// Handle waveform changes
void handleWaveformChange(int index, unsigned int waveformType) {
  Serial.println(index);

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

void setFilterDefaults(int channel) {
  // Set gain values depending on channel

  AudioMixer4 *mixer = nullptr;

  switch (channel) {
    case 1: mixer = &filtermixer1; break;
    case 2: mixer = &filtermixer2; break;
    case 3: mixer = &filtermixer3; break;
    case 4: mixer = &filtermixer4; break;
    case 5: mixer = &filtermixer5; break;
    case 6: mixer = &filtermixer6; break;
    case 7: mixer = &filtermixer7; break;
    case 8: mixer = &filtermixer8; break;

    case 11: mixer = &filtermixer11; break;
    case 12: mixer = &filtermixer12; break;
    case 13: mixer = &filtermixer13; break;
    case 14: mixer = &filtermixer14; break;
    default: return;  // Invalid channel
  }

  if (mixer) {
    mixer->gain(0, 1);
    for (int i = 1; i < 4; ++i) {
      mixer->gain(i, 0);
    }
  }

  SMP.filter_settings[channel][OCTAVE] = 32;  // middle = 0
  SMP.filter_settings[channel][DETUNE] = 32;  // middle = 0
  filters[channel]->resonance(0.0);           // default resonance off
  filters[channel]->frequency(10000);

  if (freeverbs[channel] != 0 && freeverbs[channel] != nullptr) {
    freeverbs[channel]->roomsize(0);
    freeverbs[channel]->damping(0);
  }

  if (freeverbmixers[channel] != 0 && freeverbmixers[channel] != nullptr) {
    freeverbmixers[channel]->gain(0, 0);
    freeverbmixers[channel]->gain(3, 1);
  }


  bitcrushers[channel]->bits(16);
  bitcrushers[channel]->sampleRate(44100);

  int s_idx = mapf(0, 0.0, 1.0, -4, 4);
  int s_depth = mapf(0, 0.0, 1.0, 0, 100);
  float s_freq = mapf(0, 0.0, 1.0, 0.05, 0.25);

  if (flangers[channel] != 0 && flangers[channel] != nullptr) {
    flangers[channel]->voices(s_idx, s_depth, s_freq);
    flangers[channel]->begin(delayline, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq);
  }

  
}

void setDahdsrDefaults(bool allChannels) {
  if (allChannels) {
    for (int ch = 0; ch < 16; ch++) {
      // First clear all 15 settings to zero
      for (int param = 0; param < 15; param++) {
        SMP.param_settings[ch][param] = 0;
      }
      // Now assign default envelope values for a DAHDSR shape:
      SMP.param_settings[ch][DELAY] = 0;     // No delay
      SMP.param_settings[ch][ATTACK] = 0;    // Quick attack duration
      SMP.param_settings[ch][HOLD] = 5;      // MIN hold time
      SMP.param_settings[ch][DECAY] = 5;     // MID decay period
      SMP.param_settings[ch][SUSTAIN] = 16;  // MID sustain level
      SMP.param_settings[ch][RELEASE] = 16;  // MID release
    }
  } else {
    int ch = SMP.currentChannel;
    for (int param = 0; param < 15; param++) {
      SMP.param_settings[ch][param] = 0;
    }
    // Now assign default envelope values for a DAHDSR shape:
    SMP.param_settings[ch][DELAY] = 0;     // No delay
    SMP.param_settings[ch][ATTACK] = 0;    // Quick attack duration
    SMP.param_settings[ch][HOLD] = 5;      // MIN hold time
    SMP.param_settings[ch][DECAY] = 5;     // MID decay period
    SMP.param_settings[ch][SUSTAIN] = 16;  // MID sustain level
    SMP.param_settings[ch][RELEASE] = 16;  // MID release
  }
}



void setDrumDefaults(bool allChannels) {


  //BD parameters
  BDsine.begin(0.7, 100, WAVEFORM_SINE);
  BDsaw.begin(0.4, 100, WAVEFORM_TRIANGLE);
  BDenv.sustain(0);
  BDpitchEnv.sustain(0);

  //SN parameters
  SNenv.sustain(0);
  SNfilt.frequency(1000);
  SNnoise.amplitude(0.5);
  SNtone.begin(0.8, 700, WAVEFORM_SINE);
  SNtone2.begin(0.8, 700, WAVEFORM_SINE);
  SNtoneEnv.sustain(0);
  SNfilt.resonance(2);
  SNchaosMix.gain(1, 0);
  SNtone.frequencyModulation(0);

  //HH parameters
  HHenv.sustain(0);
  HHfilt.frequency(6000);
  HHnoise.amplitude(0.6);
  HHtone.begin(0.8, 700, WAVEFORM_SQUARE);
  HHtone2.begin(0.8, 700, WAVEFORM_SQUARE);
  HHtoneEnv.sustain(0);
  HHfilt.resonance(2);
  HHchaosMix.gain(1, 0);
  HHtone.frequencyModulation(6);
  HHtone2.frequencyModulation(6);


  if (allChannels) {
    for (int ch = 1; ch < 4; ch++) {
      // First clear all 3 Drums

      SMP.drum_settings[ch][DRUMTONE] = 0;    //0
      SMP.drum_settings[ch][DRUMDECAY] = 32;  //half
      SMP.drum_settings[ch][DRUMPITCH] = 32;  //half
      SMP.drum_settings[ch][DRUMTYPE] = 1;    // MID decay period
      SMP.param_settings[ch][TYPE] = 1;       // SMP
    }
  }
}

void resetAllFilters() {
  for (unsigned int i = 0; i < 15; i++) {
    filters[i]->frequency(0);
    filters[i]->resonance(0);
  }
}
