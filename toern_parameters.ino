
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

   /* case LENGTH:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[LENGTH]);  //1000
      break;

    case SECONDMIX:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[SECONDMIX]);  //1
      break;

    case PITCHMOD:
      mappedValue = mapf(SMP.param_settings[SMP.currentChannel][paramType], 0, maxfilterResolution, 0, maxParamVal[PITCHMOD]);  //1
      break;
*/
    default:
      mappedValue = 0;
  }
  return mappedValue;
}

// Update parameter values
void updateParameterValue(int paramType, int index, float value) {

  
  switch (paramType) {
    case DELAY:
        if (index > 9) envelopes[index]->delay(value);
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