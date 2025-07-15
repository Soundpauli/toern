


// Update filter values
void updateFilterValue(FilterType filterType, int index, float value) {
  // Common function to update all filter types
  //Serial.print(filterType);
  //Serial.print(" :==>: ");
  //Serial.println(value);


  switch (filterType) {
    case FREQUENCY:{
      filters[index]->frequency(value);
      filtermixers[index]->gain(0, 0.0);
      filtermixers[index]->gain(1, 1.0);
      filtermixers[index]->gain(2, 0.0);
      break;}

    case HIGHPASS:{
      filters[index]->frequency(value);
      filtermixers[index]->gain(0, 0.0);
      filtermixers[index]->gain(1, 0.0);
      filtermixers[index]->gain(2, 1.0);
      break;}

    case LOWPASS:{
      filters[index]->frequency(value);
      filtermixers[index]->gain(0, 1.0);
      filtermixers[index]->gain(1, 0.0);
      filtermixers[index]->gain(2, 0.0);
      break;}


    case REVERB:{
      if (freeverbs[index] != nullptr && freeverbs[index] != 0) {

        if (freeverbmixers[index] != 0 && value < 0.1) {
          // Effectively bypass the reverb
          freeverbmixers[index]->gain(0, 0);  // Mute wet
          freeverbmixers[index]->gain(3, 1);  // Enable dry
        } else {
          // Apply reverb settings
          
          freeverbs[index]->roomsize(value);
          //freeverbs[index]->damping(0);
          if (freeverbmixers[index] != 0) {
            // Enable wet, disable dry
            freeverbmixers[index]->gain(0, 1);  // Enable wet
            freeverbmixers[index]->gain(3, 1);  // dont Mute dry
          }
        }
      }
      break;}

  

    case BITCRUSHER:{
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
      //Serial.print("BitDepth: ");
      //Serial.print(16 - xbitDepth);
      //Serial.print(" | SampleRate: ");
      //Serial.print(xsampleRate);
      //Serial.print(" | CompGain: ");
      //Serial.println(crushCompGain);
      break;
  }
  }

  
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
  
  //if (filterType == DETUNE) // find in main file.
  //if (filterType == OCTAVE) // find in main file.
  

  return mappedValue;
}



void resetAllFilters() {
  for (unsigned int i = 0; i < 15; i++) {
    filters[i]->frequency(0);
    filters[i]->resonance(0);
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

}
