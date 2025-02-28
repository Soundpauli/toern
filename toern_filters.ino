
void drawFilters(char *txt, int activeFilter) {
  FastLED.clear();
  // Amplitude definitions:
  int baseAmplitude = 1;        // baseline (minimum amplitude)
  CRGB activeFilterColor;
  switch (activeFilter) {
    case 0:
      activeFilterColor = CRGB(100, 0, 0);
      break;
    case 1:
      activeFilterColor = CRGB(0, 100, 0);
      break;
    case 2:
      activeFilterColor = CRGB(0, 0, 100);
      break;
    case 3:
      activeFilterColor = CRGB(100, 100, 0);
      break;
    case 4:
      activeFilterColor = CRGB(100, 0, 100);
      break;
    case 5:
      activeFilterColor = CRGB(0, 100, 100);
      break;
    case 6:
      activeFilterColor = CRGB(100, 100, 100);
      break;
    case 7:
      activeFilterColor = CRGB(100, 50, 100);
      break;
    case 8:
      activeFilterColor = CRGB(50, 10, 10);
      break;
    default:
      activeFilterColor = CRGB(0, 0, 0);  // default color (adjust as needed)
      break;
  }

  drawText(txt, 1, 12, activeFilterColor);
  int FilterValue = mapf(SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter],0,32,0,10);
  drawNumber(FilterValue, CRGB(100,100,100),5);
}





void setFilters() {

  if (fxType==0) drawADSR(currentFilter, SMP.selectedParameter);

  if (fxType==1) drawFilters(currentFilter,SMP.selectedFilter);

  // Encoder 3: Adjust Filters
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
    drawFilters(currentFilter,SMP.selectedFilter);
  }

  // Encoder 1: Adjust Params
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

  if (fxType == 1) {
    if (selectedFX == LOWPASS) {
      // Adjust delay
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][LOWPASS]) {
        SMP.filter_settings[SMP.currentChannel][LOWPASS] = currentMode->pos[3];
        float lowPassVal = mapf(SMP.filter_settings[SMP.currentChannel][LOWPASS], 0, 32, 0, 10);
        envelope13.delay(lowPassVal);
        Serial.println("lowpass: " + String(lowPassVal));
      }
    }

    if (selectedFX == HIGHPASS) {
      //Adjust highpass
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][HIGHPASS]) {
        SMP.filter_settings[SMP.currentChannel][HIGHPASS] = currentMode->pos[3];
        float highPassVal = mapf(SMP.filter_settings[SMP.currentChannel][HIGHPASS], 0, 32, 0, 10);
        envelope13.delay(highPassVal);
        Serial.println("highpass: " + String(highPassVal));
      }
    }

    if (selectedFX == FREQUENCY) {
      //Adjust frequency
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][FREQUENCY]) {
        SMP.filter_settings[SMP.currentChannel][FREQUENCY] = currentMode->pos[3];
        float freqVal = mapf(SMP.filter_settings[SMP.currentChannel][FREQUENCY], 0, 32, 0, 10);
        envelope13.delay(freqVal);
        Serial.println("frequency: " + String(freqVal));
      }
    }

    if (selectedFX == FLANGER) {
      //Adjust flanger
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][FLANGER]) {
        SMP.filter_settings[SMP.currentChannel][FLANGER] = currentMode->pos[3];
        float flangeVal = mapf(SMP.filter_settings[SMP.currentChannel][FLANGER], 0, 32, 0, 10);
        envelope13.delay(flangeVal);
        Serial.println("flangerVal: " + String(flangeVal));
      }
    }


    if (selectedFX == ECHO) {
      //Adjust echo
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][ECHO]) {
        SMP.filter_settings[SMP.currentChannel][ECHO] = currentMode->pos[3];
        float echoVal = mapf(SMP.filter_settings[SMP.currentChannel][ECHO], 0, 32, 0, 10);
        envelope13.delay(echoVal);
        Serial.println("echoVal: " + String(echoVal));
      }
    }


    if (selectedFX == DISTORTION) {
      //Adjust distortion
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][DISTORTION]) {
        SMP.filter_settings[SMP.currentChannel][DISTORTION] = currentMode->pos[3];
        float distortVal = mapf(SMP.filter_settings[SMP.currentChannel][DISTORTION], 0, 32, 0, 10);
        envelope13.delay(distortVal);
        Serial.println("distortVal: " + String(distortVal));
      }
    }

    if (selectedFX == RINGMOD) {
      //Adjust ringmod
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][RINGMOD]) {
        SMP.filter_settings[SMP.currentChannel][RINGMOD] = currentMode->pos[3];
        float ringModVal = mapf(SMP.filter_settings[SMP.currentChannel][RINGMOD], 0, 32, 0, 10);
        envelope13.delay(ringModVal);
        Serial.println("ringModVal: " + String(ringModVal));
      }
    }


    if (selectedFX == DETUNE) {
      //Adjust detune
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][DETUNE]) {
        SMP.filter_settings[SMP.currentChannel][DETUNE] = currentMode->pos[3];
        float detuneVal = mapf(SMP.filter_settings[SMP.currentChannel][DETUNE], 0, 32, 0, 10);
        envelope13.delay(detuneVal);
        Serial.println("detuneVal: " + String(detuneVal));
      }
    }



    if (selectedFX == NOISE) {
      //Adjust noise
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][NOISE]) {
        SMP.filter_settings[SMP.currentChannel][NOISE] = currentMode->pos[3];
        float noiseVal = mapf(SMP.filter_settings[SMP.currentChannel][NOISE], 0, 32, 0, 10);
        envelope13.delay(noiseVal);
        Serial.println("noiseVal: " + String(noiseVal));
      }
    }
  }

  if (fxType == 0) {
    if (selectedFX == DELAY) {
      // Adjust delay
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][DELAY]) {
        SMP.param_settings[SMP.currentChannel][DELAY] = currentMode->pos[3];
        float delayVal = mapf(SMP.param_settings[SMP.currentChannel][DELAY], 0, 32, 0, 10);
        envelope13.delay(delayVal);
        Serial.println("delay: " + String(delayVal));
      }
    }

    if (selectedFX == ATTACK) {
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][ATTACK]) {
        SMP.param_settings[SMP.currentChannel][ATTACK] = currentMode->pos[3];
        float attackVal = mapf(SMP.param_settings[SMP.currentChannel][ATTACK], 0, 32, 0, 11880);
        envelope13.attack(attackVal);
        Serial.println("attack: " + String(attackVal));
      }
    }


    if (selectedFX == HOLD) {
      //  Adjust hold
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][HOLD]) {
        SMP.param_settings[SMP.currentChannel][HOLD] = currentMode->pos[3];
        float holdVal = mapf(SMP.param_settings[SMP.currentChannel][HOLD], 0, 32, 0, 11880);
        envelope13.hold(holdVal);
        Serial.println("hold: " + String(holdVal));
      }
    }

    if (selectedFX == DECAY) {
      //  Adjust decay
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][DECAY]) {
        SMP.param_settings[SMP.currentChannel][DECAY] = currentMode->pos[3];
        float decayVal = mapf(SMP.param_settings[SMP.currentChannel][DECAY], 0, 32, 0, 11880);
        envelope13.decay(decayVal);
        Serial.println("decay: " + String(decayVal));
      }
    }

    if (selectedFX == SUSTAIN) {
      //  Adjust sustain
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][SUSTAIN]) {
        SMP.param_settings[SMP.currentChannel][SUSTAIN] = currentMode->pos[3];
        float sustainVal = mapf(SMP.param_settings[SMP.currentChannel][SUSTAIN], 0, 32, 0, 1.0);  // Sustain range: 0.1s to 2.0s
        envelope13.sustain(sustainVal);
        Serial.println("sustain: " + String(sustainVal) + "");
      }
    }

    if (selectedFX == RELEASE) {
      //  Adjust release
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][RELEASE]) {
        SMP.param_settings[SMP.currentChannel][RELEASE] = currentMode->pos[3];
        float releaseVal = mapf(SMP.param_settings[SMP.currentChannel][RELEASE], 0, 32, 0, 1000.0);  // Release range: 0ms to 1.0s
        envelope13.sustain(releaseVal);
        Serial.println("release: " + String(releaseVal) + "");
      }
    }





    if (selectedFX == TYPE) {
      // Adjust type
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][TYPE]) {
        SMP.param_settings[SMP.currentChannel][TYPE] = currentMode->pos[3];
        float typeVAL = mapf(SMP.param_settings[SMP.currentChannel][TYPE], 1, 100, 0, 4.0);  // Release range: 0ms to 1.0s
        envelope13.sustain(typeVAL);
        Serial.println("type: " + String(typeVAL) + "");
      }
    }





    if (selectedFX == WAVEFORM) {
      // adjust waveform
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][WAVEFORM]) {
        SMP.param_settings[SMP.currentChannel][WAVEFORM] = currentMode->pos[3];
        unsigned int wavformVal = mapf(SMP.param_settings[SMP.currentChannel][WAVEFORM], 0, 4, 1, 4);  // 0-4
        Serial.println("WaveForm: " + String(wavformVal));

        switch (wavformVal) {
          case 1:
            sound13.begin(WAVEFORM_SINE);
            break;
          case 2:
            sound13.begin(WAVEFORM_SAWTOOTH);
            break;
          case 3:
            sound13.begin(WAVEFORM_SQUARE);
            break;
          case 4:
            sound13.begin(WAVEFORM_TRIANGLE);
            break;
        }
      }
    }
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
      SMP.param_settings[ch][DELAY] = 11;    // No delay
      SMP.param_settings[ch][ATTACK] = 24;   // Quick attack duration
      SMP.param_settings[ch][HOLD] = 21;     // MIN hold time
      SMP.param_settings[ch][DECAY] = 27;    // MID decay period
      SMP.param_settings[ch][SUSTAIN] = 16;  // MID sustain level
      SMP.param_settings[ch][RELEASE] = 23;  // MID release
    }
  } else {
    int ch = SMP.currentChannel;
    for (int param = 0; param < 15; param++) {
      SMP.param_settings[ch][param] = 0;
    }
    // Now assign default envelope values for a DAHDSR shape:
    SMP.param_settings[ch][DELAY] = 11;    // No delay
    SMP.param_settings[ch][ATTACK] = 24;   // Quick attack duration
    SMP.param_settings[ch][HOLD] = 21;     // MIN hold time
    SMP.param_settings[ch][DECAY] = 27;    // MID decay period
    SMP.param_settings[ch][SUSTAIN] = 16;  // MID sustain level
    SMP.param_settings[ch][RELEASE] = 23;  // MID release
  }
}


void resetAllFilters() {
  for (unsigned int i = 0; i < maxFilters; i++) {
    filters[i]->frequency(0);
    filters[i]->resonance(0);
  }
}

