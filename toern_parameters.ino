// Forward declarations for ch11 polyphonic voice arrays (defined in toern_synths.ino).
extern uint16_t attackAmp[2][3];
extern uint16_t decayAmp[2][3];
extern float    sustainAmp[2][3];
extern uint16_t releaseAmp[2][3];
extern uint16_t releaseFilter[2][3];
extern void     updateVals(int ch);

// Refactored: setParams combines processParameterAdjustment and updateParameterValue
void setParams(ParameterType paramType, int index) {

  float mappedValue;
  switch (paramType) {
    case DELAY:
      if (channelHasVoiceDelay(index)) {
        extern void applyVoiceDelay(int ch);
        applyVoiceDelay(index);
      }
      break;
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
      SMP.param_settings[index][SUSTAIN] = constrain(SMP.param_settings[index][SUSTAIN], 1.0f, (float)maxfilterResolution);
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
        uint16_t relMs = (uint16_t)mappedValue;
        uint16_t fltRelMs = relMs < 180 ? 180 : relMs;
        for (int v = 0; v < 3; v++) {
          releaseAmp[0][v] = relMs;
          releaseFilter[0][v] = fltRelMs;
        }
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

#define DEFAULT_CH11_INSTRUMENT 12  // 12 = DXBS (DX7 FM E-Bass)

static const uint8_t kCh11Control1Default[13] = {
  8, 20, 12, 18, 5, 24, 20, 22, 26, 9,
  14, 20, 14,   // 303, DXEP, DXBS — open, present CUT default
};
static const uint8_t kCh11Control2Default[13] = {
  5, 4, 2, 10, 24, 1, 22, 14, 6, 10,
  12, 6, 10,    // musical RES defaults (maxResonance is capped < 0.88 so 0..32 never self-oscillates)
};
static const uint8_t kCh11Control3Default[13] = {
  12, 8, 2, 3, 20, 2, 2, 14, 4, 18,
  12, 10, 12,   // balanced FLT envelope sweep (maxFilterAmount is capped at 2.2..3.2 octaves)
};
static const uint8_t kCh11AttackDefault[13] = {
  32, 32, 32, 8, 26, 32, 28, 32, 32, 30,
  32, 32, 32,
};
static const uint8_t kCh11DecayDefault[13] = {
  6, 14, 1, 32, 32, 2, 0, 5, 0, 28,
  18, 28, 16,   // 303=560ms, DXEP=875ms, DXBS=500ms
};
static const uint8_t kCh11SustainDefault[13] = {
  0, 2, 28, 20, 26, 28, 31, 0, 31, 26,
  18, 20, 16,   // 303=56%, DXEP=62.5% Rhodes body, DXBS=50% solid bass body
};
static const uint8_t kCh11ReleaseDefault[13] = {
  2, 6, 2, 24, 20, 3, 10, 5, 1, 10,
  12, 16, 12,   // 303=375ms, DXEP=500ms damper ring, DXBS=375ms natural bass release
};
static const uint8_t kCh11CentSliderDefault[13] = {
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
  5,   // 10 303   — octave 2
  14,  // 11 DXEP  — octave 4
  5,   // 12 DXBS  — octave 2
};
static const uint8_t kCh11FormDefault[13] = {
  0, 8, 4, 16, 12, 16, 12, 8, 4, 0,
  0, 12, 4,
};
static const uint8_t kCh11WaveDefault[13] = {
  8, 4, 4, 0, 12, 0, 12, 4, 4, 8,
  8, 12, 4,
};

// Defaults used by filter-page and full-voice resets.
static int defaultSettingValue(int ch, SettingArray arr, int8_t idx) {
  int ch11Inst = constrain((int)SMP.synth_settings[11][INSTRUMENT], 0, 12);
  switch (arr) {
    case ARR_FILTER:
      switch (idx) {
        case HCUT: return (ch == 14) ? 14 : 32;
        case LOWCUT: return 0;
        case REVERB: return 0;
        case BITCRUSHER: return 0;
        case DETUNE: return 16;
        case OCTAVE: return (ch >= 1 && ch <= 8) ? 24 : 16;
        case RES: return (ch == 14) ? 5 : 0;
        case EFX: return 0;
        case FILTER_WAVEFORM:
          return (ch == 11) ? kCh11WaveDefault[ch11Inst] : 8;
        default: return 0;
      }
    case ARR_GAIN: return 16;
    case ARR_PARAM:
      // Default ADSR: ch11 uses active instrument preset; ch14 303 uses D=20, S=20, R=16; others A=32, D=32, S=32
      switch (idx) {
        case DELAY: return 0;
        case ATTACK:
          return (ch == 11) ? kCh11AttackDefault[ch11Inst] : 32;
        case DECAY:
          if (ch == 11) return kCh11DecayDefault[ch11Inst];
          return (ch == 14) ? 20 : 32;
        case SUSTAIN:
          if (ch == 11) return max(1, (int)kCh11SustainDefault[ch11Inst]);
          return (ch == 14) ? 20 : 32;
        case RELEASE:
          if (ch == 11) return kCh11ReleaseDefault[ch11Inst];
          return (ch == 13 || ch == 14) ? 16 : 0;
        default: return 0;
      }
    case ARR_SYNTH:
      switch (idx) {
        case CUTOFF:
          if (ch == 11) return kCh11Control1Default[ch11Inst];
          return (ch == 13) ? 32 : ((ch == 14) ? 14 : 16);
        case RESONANCE:
          if (ch == 11) return kCh11Control2Default[ch11Inst];
          return (ch == 14) ? 5 : 0;
        case FILTER:
          if (ch == 11) return kCh11Control3Default[ch11Inst];
          return (ch == 14) ? 8 : 0;
        case CENT:
          if (ch == 11) return kCh11CentSliderDefault[ch11Inst];
          if (ch == 13) return 0;
          if (ch == 14) return 8;
          return 16;
        case SEMI: return 16;
        case INSTRUMENT: return DEFAULT_CH11_INSTRUMENT;
        case FORM:
          return (ch == 11) ? kCh11FormDefault[ch11Inst] : 0;
        case LFO_RATE: return 0;
        case LFO_DEPTH: return 0;
        case LFO_PHASE: return 0;
        case ARP_STEP: return 16;
        default: return 0;
      }
    case ARR_STRETCH:
      return 11;
    default:
      return 0;
  }
}

// Unified ADSR slider defaults (0..maxfilterResolution): A=32, D=32, S=32, R=16 on 11/13/14 else R=0
static void applyDefaultAdsrSettings(int ch) {
  setSampleGate(ch, false);
  SMP.param_settings[ch][ATTACK] = defaultSettingValue(ch, ARR_PARAM, ATTACK);
  SMP.param_settings[ch][DECAY] = defaultSettingValue(ch, ARR_PARAM, DECAY);
  SMP.param_settings[ch][SUSTAIN] = defaultSettingValue(ch, ARR_PARAM, SUSTAIN);
  SMP.param_settings[ch][RELEASE] = defaultSettingValue(ch, ARR_PARAM, RELEASE);
  setParams(ATTACK, ch);
  setParams(DECAY, ch);
  setParams(SUSTAIN, ch);
  setParams(RELEASE, ch);
}

// Reset sliders on one filter page for one channel (FILTERMODE touch3 / "2000").
static void setFilterPageDefaultValues(int ch, uint8_t page) {
  if (ch < 0 || ch >= NUM_CHANNELS) return;
  extern SliderDefEntry sliderDef[NUM_CHANNELS][5][4];

  bool touchedSynth = false;
  bool touchedAdsr = false;

  for (uint8_t i = 0; i < 4; ++i) {
    const SliderDefEntry& d = sliderDef[ch][page][i];
    if (d.arr == ARR_NONE || d.idx < 0) continue;
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) continue;

    const int val = defaultSettingValue(ch, d.arr, d.idx);

    switch (d.arr) {
      case ARR_FILTER:
        filterSetting(ch, d.idx) = val;
        if (d.idx != EFX) {
          setFilters((FilterType)d.idx, ch, true);
        }
        break;
      case ARR_PARAM:
        SMP.param_settings[ch][d.idx] = val;
        setParams((ParameterType)d.idx, ch);
        if (d.idx == ATTACK || d.idx == DECAY || d.idx == SUSTAIN || d.idx == RELEASE) {
          touchedAdsr = true;
        }
        break;
      case ARR_SYNTH:
        SMP.synth_settings[ch][d.idx] = val;
        if (ch == 11 && d.idx == INSTRUMENT) {
          applySynthInstrumentPreset(ch, val);
        }
        if ((ch == 11 || ch == 13 || ch == 14) && d.idx == ARP_STEP) {
          extern void midiArpResetVoice(int ch);
          midiArpResetVoice(ch);
        }
        touchedSynth = true;
        break;
      case ARR_STRETCH:
        setSampleTimeStretch(ch, val);
        break;
      case ARR_GAIN:
        setSampleVoiceGain(ch, val);
        break;
      default:
        break;
    }
  }

  // Force unified ADSR after any page that owns those sliders (also wins over INST preset).
  if (touchedAdsr) {
    applyDefaultAdsrSettings(ch);
  }

  if (touchedSynth) {
    if (ch == 11) updateSynthVoice(11);
    if (ch == 13 || ch == 14) {
      extern void applyMonoSynthCrf(int ch);
      applyMonoSynthCrf(ch);
    }
  }
}

void setCurrentFilterPageDefaultValues(int ch) {
  if (ch < 0 || ch >= NUM_CHANNELS) return;
  extern uint8_t filterPage[NUM_CHANNELS];
  const uint8_t page = filterPage[ch];
  setFilterPageDefaultValues(ch, page);
  initSliders(page, ch);
}

// Reset every filter page for one voice (FILTERMODE "0002").
// Walks sliderDef so sample RES, ch11 INST/WAVE pages, and ch13/14 LFO/ARP are included —
// Includes every visible parameter through the channel slider definitions.
void setAllFilterPagesDefaultValues(int ch) {
  if (ch < 0 || ch >= NUM_CHANNELS) return;
  extern uint8_t filterPage[NUM_CHANNELS];
  extern const uint8_t filterPageCount[NUM_CHANNELS];

  int pages = filterPageCount[ch];
  if (pages <= 0) pages = 4;
  if (pages > 5) pages = 5;
  for (int p = 0; p < pages; ++p) {
    setFilterPageDefaultValues(ch, (uint8_t)p);
  }
  // EFX is not on every voice's slider pages but still owns sample routing.
  SMP.filter_settings[ch][EFX] = 0;

  // Explicitly ensure DELAY=0 and synth LFO/ARP defaults on all voices.
  SMP.param_settings[ch][DELAY] = 0;
  setParams(DELAY, ch);
  if (ch >= 1 && ch <= 8) {
    setSampleTimeStretch(ch, 11);
  }
  if (ch == 11) {
    SMP.synth_settings[11][INSTRUMENT] = DEFAULT_CH11_INSTRUMENT;
    applySynthInstrumentPreset(11, DEFAULT_CH11_INSTRUMENT);
  }
  if (ch == 11 || ch == 13 || ch == 14) {
    SMP.synth_settings[ch][LFO_RATE] = 0;
    SMP.synth_settings[ch][LFO_DEPTH] = 0;
    SMP.synth_settings[ch][LFO_PHASE] = 0;
    SMP.synth_settings[ch][ARP_STEP] = 16;
    extern void midiArpResetVoice(int ch);
    midiArpResetVoice(ch);
  }

  // Always end on default ADSR (for ch11 this matches the active instrument preset).
  applyDefaultAdsrSettings(ch);

  if (ch == 11) updateSynthVoice(11);
  if (ch == 13 || ch == 14) {
    extern void applyMonoSynthCrf(int ch);
    applyMonoSynthCrf(ch);
  }
  if (currentMode == &filterMode && ch == (int)GLOB.currentChannel) {
    initSliders(filterPage[ch], ch);
  }
}

static uint8_t synthInstrumentFormDefault(int instrumentIdx) {
  return kCh11FormDefault[constrain(instrumentIdx, 0, 12)];
}

uint8_t synthInstrumentWaveDefault(int instrumentIdx) {
  return kCh11WaveDefault[constrain(instrumentIdx, 0, 12)];
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
  instrumentIdx = constrain(instrumentIdx, 0, 12);

  SMP.synth_settings[channel][INSTRUMENT] = instrumentIdx;
  SMP.synth_settings[channel][CUTOFF] = kCh11Control1Default[instrumentIdx];
  SMP.synth_settings[channel][RESONANCE] = kCh11Control2Default[instrumentIdx];
  SMP.synth_settings[channel][FILTER] = kCh11Control3Default[instrumentIdx];
  SMP.synth_settings[channel][SEMI] = 16;  // mid → zero extra cent/semi offset from p4 mapping
  SMP.synth_settings[channel][CENT] = kCh11CentSliderDefault[instrumentIdx];
  filterSetting(channel, FILTER_WAVEFORM) =
      synthInstrumentWaveDefault(instrumentIdx);
  applySynthInstrumentFormDefault(channel, instrumentIdx);
  SMP.param_settings[channel][ATTACK] = kCh11AttackDefault[instrumentIdx];
  SMP.param_settings[channel][DECAY] = kCh11DecayDefault[instrumentIdx];
  SMP.param_settings[channel][SUSTAIN] = max(1, (int)kCh11SustainDefault[instrumentIdx]);
  SMP.param_settings[channel][RELEASE] = kCh11ReleaseDefault[instrumentIdx];
}

// Reset ALL filters, envelopes, drums and synths to default values
void resetAllToDefaults() {
  // Reset all channels (1-8, 11, 13-14)
  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);
  
  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    setAllFilterPagesDefaultValues(ch);
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

  showSaveSuccessAnimation();
}
