// Forward declarations for ch11 polyphonic voice arrays (defined in toern_synths.ino).
extern uint16_t attackAmp[2][3];
extern uint16_t decayAmp[2][3];
extern float    sustainAmp[2][3];
extern uint16_t releaseAmp[2][3];
extern void     updateVals(int ch);

// Refactored: setParams combines processParameterAdjustment and updateParameterValue
void setParams(ParameterType paramType, int index) {

  float mappedValue;
  switch (paramType) {
    case ATTACK:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, maxParamVal[ATTACK], 0);
      if (index == 11) {
        for (int v = 0; v < 3; v++) attackAmp[0][v] = (uint16_t)mappedValue;
        updateVals(0);
      } else {
        envelopes[index]->attack(mappedValue);
      }
      break;
    case DECAY:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[DECAY]);
      if (index == 11) {
        for (int v = 0; v < 3; v++) decayAmp[0][v] = (uint16_t)mappedValue;
        updateVals(0);
      } else {
        envelopes[index]->decay(mappedValue);
      }
      break;
    /*case HOLD:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[HOLD]);
      envelopes[index]->hold(mappedValue);
      break;*/
    case SUSTAIN:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[SUSTAIN]);
      if (index == 11) {
        for (int v = 0; v < 3; v++) sustainAmp[0][v] = mappedValue;
        updateVals(0);
      } else {
        envelopes[index]->sustain(mappedValue);
      }
      break;
    case RELEASE:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[RELEASE]);
      if (index == 11) {
        for (int v = 0; v < 3; v++) releaseAmp[0][v] = (uint16_t)mappedValue;
        updateVals(0);
      } else {
        envelopes[index]->release(mappedValue);
      }
      break;
  }
}


// Handle waveform changes
void handleWaveformChange(int index, unsigned int waveformType) {
  // Mono synth presets keep oscillator 2 as their contrasting layer.
  if (synths[index][0] != nullptr) {
    switch (waveformType) {
      case 1:
        synths[index][0]->begin(WAVEFORM_SINE);
        break;
      case 2:
        synths[index][0]->begin(WAVEFORM_SQUARE);
        break;
      case 3:
        synths[index][0]->begin(WAVEFORM_SAWTOOTH);
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
  SMP.filter_settings[ch][HCUT] = 32;
  SMP.filter_settings[ch][LOWCUT] = 0;
  SMP.filter_settings[ch][REVERB] = 0;
  SMP.filter_settings[ch][BITCRUSHER] = 0;
  SMP.filter_settings[ch][DETUNE] = 16;
  SMP.filter_settings[ch][OCTAVE] = 16;
  // EFX setting: 0 = SAMPLE mode (default for all channels)
  SMP.filter_settings[ch][EFX] = 0;

  setFilters(HCUT, ch, true);
  setFilters(LOWCUT, ch, true);
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
  // Channel-specific ADSR defaults.
  // NOTE: These are "slider" values in the 0..maxfilterResolution range (typically 0..32),
  // not milliseconds directly. `setParams()` maps them to actual envelope times/levels.
  if (ch == 13 || ch == 14) {
    // Default synth envelope for channels 13/14:
    // A=32, D=9, S=20, R=9
    SMP.param_settings[ch][ATTACK] = 32;
    SMP.param_settings[ch][DECAY] = 9;
    SMP.param_settings[ch][SUSTAIN] = 20;
    SMP.param_settings[ch][RELEASE] = 9;
  } else {
    SMP.param_settings[ch][ATTACK] = 32;
    SMP.param_settings[ch][DECAY] = 32;
    SMP.param_settings[ch][SUSTAIN] = 10;
    SMP.param_settings[ch][RELEASE] = 5;
  }
  setParams(ATTACK, ch);
  setParams(DECAY, ch);
  setParams(SUSTAIN, ch);
  setParams(RELEASE, ch);
  initSliders(filterPage[GLOB.currentChannel],GLOB.currentChannel);
  updateSynthVoice(11);
}


// Set default synth values for a single channel
//SYNTH
void setSynthDefaultValues(int ch) {
  SMP.synth_settings[ch][CUTOFF] = 16;
  SMP.synth_settings[ch][RESONANCE] = 0;
  SMP.synth_settings[ch][FILTER] = 0;
  // Default pitch offsets:
  // - ch13: 2 octaves down
  // - ch14: 1 octave down
  // - ch11 pitch is handled via octave[0] (see initGlobalVars / resetAllToDefaults)
  if (ch == 13) {
    SMP.synth_settings[ch][CENT] = 0;   // maps to -24 semitones
  } else if (ch == 14) {
    SMP.synth_settings[ch][CENT] = 8;   // maps to -12 semitones
  } else {
    SMP.synth_settings[ch][CENT] = 16;  // neutral
  }
  SMP.synth_settings[ch][SEMI] = 0;
  SMP.synth_settings[ch][INSTRUMENT] = 0;
  SMP.synth_settings[ch][FORM] = 0;
  SMP.synth_settings[ch][LFO_RATE] = 0;
  SMP.synth_settings[ch][LFO_DEPTH] = 0;
  SMP.synth_settings[ch][LFO_PHASE] = 0; // spread index
  SMP.synth_settings[ch][ARP_STEP] = 0;   // note count index

  // Apply BASS-specific slider defaults (CUTOFF, RES, FILTER, CENT, FORM) so the
  // ch11 synth starts with the correct octave/timbre for instrument 0 (BASS).
  if (ch == 11) {
    applySynthInstrumentPreset(ch, 0);
  }

   initSliders(filterPage[GLOB.currentChannel],GLOB.currentChannel);
   // updateSynthVoice(11) is called at the end of resetAllToDefaults() instead
}

// Defaults for a single slider setting (mirrors setFilters/Envelope/SynthDefaultValues).
static int defaultSettingValue(int ch, SettingArray arr, int8_t idx) {
  switch (arr) {
    case ARR_FILTER:
      switch (idx) {
        case HCUT: return 32;
        case LOWCUT: return 0;
        case REVERB: return 0;
        case BITCRUSHER: return 0;
        case DETUNE: return 16;
        case OCTAVE: return 16;
        case RES: return 0;
        case EFX: return 0;
        case FILTER_WAVEFORM: return 8;
        default: return 0;
      }
    case ARR_PARAM:
      if (ch == 13 || ch == 14) {
        switch (idx) {
          case ATTACK: return 32;
          case DECAY: return 9;
          case SUSTAIN: return 20;
          case RELEASE: return 9;
          default: return 0;
        }
      }
      switch (idx) {
        case ATTACK: return 32;
        case DECAY: return 32;
        case SUSTAIN: return 10;
        case RELEASE: return 5;
        default: return 0;
      }
    case ARR_SYNTH:
      switch (idx) {
        case CUTOFF: return 16;
        case RESONANCE: return 0;
        case FILTER: return 0;
        case CENT:
          if (ch == 13) return 0;
          if (ch == 14) return 8;
          return 16;
        case SEMI: return 0;
        case INSTRUMENT: return 0;
        case FORM: return 0;
        case LFO_RATE: return 0;
        case LFO_DEPTH: return 0;
        case LFO_PHASE: return 0;
        case ARP_STEP: return 0;
        default: return 0;
      }
    default:
      return 0;
  }
}

// Reset only the sliders on the current filter page for one channel (FILTERMODE "2000").
void setCurrentFilterPageDefaultValues(int ch) {
  if (ch < 0 || ch >= NUM_CHANNELS) return;
  extern SliderDefEntry sliderDef[NUM_CHANNELS][4][4];
  extern uint8_t filterPage[NUM_CHANNELS];

  const uint8_t page = filterPage[ch];
  bool touchedSynth = false;

  for (uint8_t i = 0; i < 4; ++i) {
    const SliderDefEntry& d = sliderDef[ch][page][i];
    if (d.arr == ARR_NONE || d.idx < 0) continue;
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) continue;

    const int val = defaultSettingValue(ch, d.arr, d.idx);

    switch (d.arr) {
      case ARR_FILTER:
        SMP.filter_settings[ch][d.idx] = val;
        if (d.idx != EFX) {
          setFilters((FilterType)d.idx, ch, true);
        }
        break;
      case ARR_PARAM:
        SMP.param_settings[ch][d.idx] = val;
        setParams((ParameterType)d.idx, ch);
        break;
      case ARR_SYNTH:
        SMP.synth_settings[ch][d.idx] = val;
        if (ch == 11 && d.idx == INSTRUMENT) {
          applySynthInstrumentPreset(ch, val);
        }
        touchedSynth = true;
        break;
      default:
        break;
    }
  }

  initSliders(page, ch);
  if (touchedSynth && ch == 11) updateSynthVoice(11);
}

static uint8_t synthInstrumentFormDefault(int instrumentIdx) {
  // p6 chooses osc1's waveform column: saw, pulse, square, triangle, sine.
  static const uint8_t kFormDefault[10] = {
    0, 8, 4, 16, 12, 16, 12, 8, 4, 0,
  };
  return kFormDefault[constrain(instrumentIdx, 0, 9)];
}

uint8_t synthInstrumentWaveDefault(int instrumentIdx) {
  // Raw WAVE slider values: SIN=0, SQR=4, SAW=8, TRI=12.
  // Pulse-based presets use SQR as their displayed waveform family; returning
  // to that value restores their original pulse recipe.
  static const uint8_t kWaveDefault[10] = {
    8, 4, 4, 0, 12, 0, 12, 4, 4, 8,
  };
  return kWaveDefault[constrain(instrumentIdx, 0, 9)];
}

void applySynthInstrumentFormDefault(int channel, int instrumentIdx) {
  if (channel != 11) return;
  SMP.synth_settings[channel][FORM] =
      synthInstrumentFormDefault(instrumentIdx);
}

// When ch11 instrument (INST) changes, align synth and envelope sliders with
// that preset's intended defaults. Loaded user-editable project values remain
// authoritative until the user explicitly chooses a different instrument.
void applySynthInstrumentPreset(int channel, int instrumentIdx) {
  if (channel != 11) return;
  instrumentIdx = constrain(instrumentIdx, 0, 9);

  // The three synth controls have a few preset-specific meanings (for example,
  // PAD uses control 1 for decay and controls 2/3 for cutoff/resonance).
  static const uint8_t kControl1Default[10] = {
    8, 20, 12, 18, 5, 24, 20, 22, 26, 9,
  };
  static const uint8_t kControl2Default[10] = {
    5, 4, 2, 10, 24, 1, 22, 14, 6, 10,
  };
  static const uint8_t kControl3Default[10] = {
    12, 8, 2, 3, 20, 2, 2, 14, 4, 18,
  };

  // Slider values: attack runs backwards (32 = instant); decay/release run
  // forwards; sustain is 0..32. These preserve each preset's articulation
  // when updateSynthVoice() reapplies the editable ADSR controls.
  static const uint8_t kAttackDefault[10] = {
    32, 32, 32, 8, 26, 32, 28, 32, 32, 30,
  };
  static const uint8_t kDecayDefault[10] = {
    6, 14, 1, 32, 32, 2, 0, 5, 0, 28,
  };
  static const uint8_t kSustainDefault[10] = {
    0, 2, 28, 20, 26, 28, 31, 0, 31, 26,
  };
  static const uint8_t kReleaseDefault[10] = {
    2, 6, 2, 24, 20, 3, 10, 5, 1, 10,
  };

  // Base octave at start of each preset (before p5 override). Maps to CENT
  // slider via the 1..8 octave formula.
  static const uint8_t kCentSliderDefault[10] = {
    5,   // 0 BASS   — octave 2
    14,  // 1 KEYS   — octave 4
    14,  // 2 CHIPTUNE
    14,  // 3 PAD
    14,  // 4 WOW
    18,  // 5 ORGAN  — octave 5
    18,  // 6 FLUTE  — octave 5
    14,  // 7 LEAD
    18,  // 8 ARP    — octave 5
    14,  // 9 BRASS
  };

  SMP.synth_settings[channel][CUTOFF] = kControl1Default[instrumentIdx];
  SMP.synth_settings[channel][RESONANCE] = kControl2Default[instrumentIdx];
  SMP.synth_settings[channel][FILTER] = kControl3Default[instrumentIdx];
  SMP.synth_settings[channel][SEMI] = 16;  // mid → zero extra cent/semi offset from p4 mapping
  SMP.synth_settings[channel][CENT] = kCentSliderDefault[instrumentIdx];
  SMP.filter_settings[channel][FILTER_WAVEFORM] =
      synthInstrumentWaveDefault(instrumentIdx);
  applySynthInstrumentFormDefault(channel, instrumentIdx);
  SMP.param_settings[channel][ATTACK] = kAttackDefault[instrumentIdx];
  SMP.param_settings[channel][DECAY] = kDecayDefault[instrumentIdx];
  SMP.param_settings[channel][SUSTAIN] = kSustainDefault[instrumentIdx];
  SMP.param_settings[channel][RELEASE] = kReleaseDefault[instrumentIdx];
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
    
    // Reset synths (only for channels 11, 13-14)
    if (ch == 11 || ch == 13 || ch == 14) {
      setSynthDefaultValues(ch);
    }
  }

  // Make ch11 one octave deeper by default (used by playSound()).
  extern float octave[2];
  octave[0] = -1.0f;
  octave[1] = 0.0f;
  
  // Reset all audio effects/filters to clean defaults
  extern void resetAllAudioEffects();
  resetAllAudioEffects();

  extern void resetAllChannelVolumesToDefault();
  resetAllChannelVolumesToDefault();
  
  // Add delay before switching mode
  delay(300);
  
  // Switch to draw mode after reset
  switchMode(&draw);
}
