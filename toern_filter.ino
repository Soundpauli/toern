


// --- Smooth Gain Transition State ---
#define NUM_FILTERS 15
#define NUM_MIXER_CHANNELS 3

struct GainTransition {
  float currentGain[NUM_MIXER_CHANNELS];
  float targetGain[NUM_MIXER_CHANNELS];
  float step[NUM_MIXER_CHANNELS];
  bool active[NUM_MIXER_CHANNELS];
};

GainTransition filterMixerTransitions[NUM_FILTERS];


// Call this regularly in your main loop to update gains smoothly
void updateMixerGains(int i) {
  // Only update the current channel for better performance
  
  //int i = GLOB.currentChannel;
  if (i >= 0 && i < NUM_FILTERS && filtermixers[i] != nullptr) {
    for (int ch = 0; ch < NUM_MIXER_CHANNELS; ++ch) {
      if (filterMixerTransitions[i].active[ch]) {
        float& cur = filterMixerTransitions[i].currentGain[ch];
        float tgt = filterMixerTransitions[i].targetGain[ch];
        float stp = filterMixerTransitions[i].step[ch];
        if (fabs(cur - tgt) <= fabs(stp)) {
          cur = tgt;
          filterMixerTransitions[i].active[ch] = false;
        } else {
          cur += stp;
        }
        filtermixers[i]->gain(ch, cur);
      }
    }
  }
}



// Set a target gain for smooth transition
void setMixerGainSmooth(int index, int channel, float target, int steps = 8) {
  Serial.print("*******");
  Serial.println(index);
  if (filtermixers[index] == nullptr) return;
  float cur = filterMixerTransitions[index].currentGain[channel];
  filterMixerTransitions[index].targetGain[channel] = target;
  filterMixerTransitions[index].step[channel] = (target - cur) / steps;
  filterMixerTransitions[index].active[channel] = true;
}

// Initialize transition state for a filter
void initFilterTransition(int index) {
  if (filtermixers[index] == nullptr) return;

  Serial.print("######");
  Serial.println(index);

  // Initialize with default values (channel 0 = 1.0, others = 0.0)
  filterMixerTransitions[index].currentGain[0] = 1.0;
  filterMixerTransitions[index].currentGain[1] = 0.0;
  filterMixerTransitions[index].currentGain[2] = 0.0;

  filterMixerTransitions[index].targetGain[0] = 1.0;
  filterMixerTransitions[index].targetGain[1] = 0.0;
  filterMixerTransitions[index].targetGain[2] = 0.0;

  for (int ch = 0; ch < NUM_MIXER_CHANNELS; ++ch) {
    filterMixerTransitions[index].step[ch] = 0.0;
    filterMixerTransitions[index].active[ch] = false;
  }
}


void forceAllMixerGainsToTarget() {
    extern const int ALL_CHANNELS[];
    extern const int NUM_ALL_CHANNELS;
    for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
        int idx = ALL_CHANNELS[i];
        if (filtermixers[idx] == nullptr) return;
  
        Serial.print("--------");
  Serial.println(idx);

        for (int ch = 0; ch < NUM_MIXER_CHANNELS; ++ch) {
            filterMixerTransitions[idx].currentGain[ch] = filterMixerTransitions[idx].targetGain[ch];
            filterMixerTransitions[idx].active[ch] = false;
            filtermixers[idx]->gain(ch, filterMixerTransitions[idx].currentGain[ch]);
        }
    }
}

// Refactored: setFilters combines processFilterAdjustment and updateFilterValue
void setFilters(FilterType filterType, int index, bool initial) {
  // Map encoder value to filter setting
Serial.print(filterType);
Serial.print(":");
Serial.print(index);
Serial.print(">");
Serial.print(initial);


  float mappedValue = 0.0;
  if (filterType == PASS) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 9000.0);

  //if (filterType == LOWPASS) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0, 9000);
  //if (filterType == HIGHPASS) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0, 9000);

  if (filterType == SPEED) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.125, 8.0);
  if (filterType == PITCH) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 3000.0); 
  if (filterType == OFFSET) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 3000.0);

  if (filterType == RES) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.7, 5.0);
  if (filterType == OCTAVE) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 7.0);
  if (filterType == FREQUENCY) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 10000.0);
  if (filterType == REVERB) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 0.79);

  if (filterType == BITCRUSHER) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 1.0, 16.0);

  if (filterType == ACTIVE) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0, 1);


  // Now update the filter value (was updateFilterValue)
  Serial.println("-------------------------");
  Serial.println(filterType);
  Serial.println(mappedValue);
  Serial.println("-------------------------");
  switch (filterType) {


case ACTIVE:{
  //float pitch = mapf(SMP.filter_settings[index][PITCH], 0, maxfilterResolution, 0.0, 3000.0);
 //if (mappedValue>0){ granular1.beginFreeze(mappedValue);}
 // else{granular1.stop();}
}
break;

  case OFFSET:{
  //float mappedValue2 = mapf(SMP.filter_settings[index][OFFSET], 0, maxfilterResolution, 0.0, 350.0);
    //  granular1.beginFreeze(mappedValue);
    //granular1.beginPitchShift(mappedValue2);
  }
  break;

    case PITCH:{
     // float mappedValue1 = mapf(SMP.filter_settings[index][OFFSET], 0, maxfilterResolution, 0.0, 350.0);
      //granular1.beginFreeze(mappedValue1);
    //  granular1.beginPitchShift(mappedValue);
    }
    break;

    case SPEED:
      //granular1.setSpeed(mappedValue);
    break;
    case PASS:
      {
        Serial.println("SETTING LOWPASS+HIGHPASS");

        filters[index]->frequency(mappedValue);
        if (SMP.filter_settings[index][filterType] >= maxfilterResolution / 2) {  //16
          // Smooth transition to frequency configuration
          Serial.print("HIGHPASS:");
          Serial.println(mappedValue);
          setMixerGainSmooth(index, 0, 0.0, 32);  // low pass off
          setMixerGainSmooth(index, 1, 0.0, 32);  // bandpass off
          setMixerGainSmooth(index, 2, 1.0, 32);  // Highpass ON
        } else {

          Serial.print("LOWPASS:");
          Serial.println(mappedValue);

          setMixerGainSmooth(index, 0, 1.0, 32);  // LOWPASS ON
          setMixerGainSmooth(index, 1, 0.0, 32);  // bandpass off
          setMixerGainSmooth(index, 2, 0.0, 32);  // Highpass off
        }
        break;
      }
    case FREQUENCY:
      {
        if (initial) break;
        filters[index]->frequency(mappedValue);
        setMixerGainSmooth(index, 0, 0.0, 64);  // Dry signal off
        setMixerGainSmooth(index, 1, 1.0, 64);  // Lowpass on
        setMixerGainSmooth(index, 2, 0.0, 64);  // Highpass off
        break;
      }

       case RES:
      {
        filters[index]->resonance(mappedValue);
        break;
      }

        case OCTAVE:
      {
        filters[index]->octaveControl(mappedValue);
        break;
      }


    case REVERB:
      {
        if (freeverbs[index] != nullptr && freeverbs[index] != 0) {
          if (freeverbmixers[index] != 0 && mappedValue < 0.1) {
            freeverbmixers[index]->gain(0, 0);  // Mute wet
            freeverbmixers[index]->gain(3, 1);  // Enable dry
          } else {
            freeverbs[index]->damping(0.01);
            freeverbs[index]->roomsize(mappedValue);
            if (freeverbmixers[index] != 0) {
              freeverbmixers[index]->gain(0, 1);  // Enable wet
              freeverbmixers[index]->gain(3, 1);  // dont Mute dry
            }
          }
        }
        break;
      }
    case BITCRUSHER:
      {
        int xbitDepth = constrain(mappedValue, 1, 16);
        int xsampleRate = round(mapf(mappedValue, 1, 16, 44100, 1000));
        bitcrushers[index]->bits(16 - xbitDepth);
        bitcrushers[index]->sampleRate(xsampleRate);
        float channelvolume = mapf(SMP.channelVol[index], 1, 16, 1, 0.2);
        float crushCompGain = mapf(mappedValue, 1, 16, max(channelvolume, 1), 0.2);
        amps[index]->gain(crushCompGain);
        break;
      }
    default:
      break;
  }
}





void setFilterDefaults(int channel) {
  // Set gain values depending on channel
  AudioMixer4* mixer = nullptr;
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

  SMP.filter_settings[channel][OCTAVE] = 16;  // middle = 0
  SMP.filter_settings[channel][DETUNE] = 16;  // middle = 0
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

  // Initialize transition state for this filter
  initFilterTransition(channel);
}
