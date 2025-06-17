

void BDnoteOn() {
  BDenv.noteOn();
  BDpitchEnv.noteOn();
}

void SNnoteOn() {
  SNenv.noteOn();
  SNtoneEnv.noteOn();
}

void HHnoteOn() {
  HHenv.noteOn();
  HHtoneEnv.noteOn();
}

// ------------------- Drum Functions Using Parameters -------------------
//
// The parameters tone, decay, and pitch are assumed to be normalized (0.0 to 1.0).
// The mapping below is recommended:
//
// For Kick (drum_KD):
//   • tone → base frequency from about 40 Hz to 120 Hz
//   • decay → envelope decay from about 50 ms to 300 ms
//   • pitch → extra modulation (adds up to ~34 Hz to saw frequency)
// For Snare (drum_SN):
//   • tone → used as a blend (0 = more noise, 1 = more tone)
//   • decay → envelope decay from about 30 ms to 150 ms
//   • pitch → mapped to filter cutoff (1000–3000 Hz) and tone frequency
// For HiHat (drum_HH):
//   • tone → blend (controls balance between noise and tone)
//   • decay → envelope decay from about 10 ms to 50 ms
//   • pitch → mapped to filter frequency (2000–7000 Hz)
// The int type (1, 2, or 3) selects one of three character settings.

void KD_drum(float A1, float A2, float A3, int type) {

  //Serial.print("BD ");
  //Serial.print(A1);
  //Serial.print(" ");
  //Serial.print(A2);
  //Serial.print(" ");
  //Serial.print(A3);
  //Serial.print(" ");
  //Serial.print(type);
  //Serial.println(" ");



  int BDvariation = 1;

  ////// BASE DRUM/KICK PARAMETERS ///////////////////////////////////////////////////////////////////////////////
  BDsine.frequency((float)A1 / 1023 * 100 + 10 * BDvariation);
  BDsaw.frequency((float)A1 / 1023 * 100 + 10 * (float)A3 / 300 * BDvariation);
  BDenv.decay((float)A2 * BDvariation);
  BDMix.gain(1, (float)A3 / 1024 / 3 * BDvariation);
  BDMix.gain(0, 1 - ((float)A3 / 1024 / 2) * BDvariation);

  //kick type//////////////////////
  if (type == 1) {
    BDsine.begin(WAVEFORM_SINE);
    BDsaw.begin(WAVEFORM_TRIANGLE);
    BDpitchEnv.decay(A2 / 6 * BDvariation);
    BDpitchAmt.amplitude((float)A3 / 1024 / 1.2 * BDvariation);
    BDchaosMix.gain(1, (float)A3 / 1300);
    BDsine.frequencyModulation(5);

  } else if (type == 2) {
    BDsine.begin(WAVEFORM_TRIANGLE);
    BDsaw.begin(WAVEFORM_TRIANGLE);
    BDpitchEnv.decay(A2 / 4 * BDvariation);
    BDpitchAmt.amplitude((float)A3 / 1024 / 1.5 * BDvariation);
    BDchaosMix.gain(1, (float)A3 / 2300);
    BDsine.frequencyModulation(2);

  } else if (type == 3) {
    BDsine.begin(WAVEFORM_SINE);
    BDsaw.begin(WAVEFORM_SINE);
    BDpitchEnv.decay(A2 / 10 * BDvariation);
    BDpitchAmt.amplitude((float)A3 / 1024 / 3 * BDvariation);
    BDchaosMix.gain(1, (float)A3 / 1000);
    BDsine.frequencyModulation(12);
  }


  BDnoteOn();
}

void SN_drum(float A4, float A5, float A6, int type) {
  int SNvariation = 1;

  ////// SNARE PARAMETERS ///////////////////////////////////////////////////////////////////////////////////////
  SNMix.gain(1, 1 - ((float)(A4) / 1024 * SNvariation));  //blend between noise and tone
  SNMix.gain(0, (float)(A4) / 1024 * SNvariation);        //blend between noise and tone
  SNenv.decay((float)(A5)*SNvariation);                   // noise decay


  SNfilt.frequency((A6)*3 + 1000 * SNvariation);  // filter freq

  SNtoneMix.gain(0, 1 - (float)(A6) / 1850 * SNvariation);  // reduce volume when fm
  SNtoneMix.gain(1, 1 - (float)(A6) / 1850 * SNvariation);  // reduce volume when fm

  //Snare type////////////////
  if (type == 1) {
    SNtone.begin(WAVEFORM_SINE);
    SNtone2.begin(WAVEFORM_SINE);
    SNtone.amplitude(0.8);
    SNtone2.amplitude(0.8);
    SNchaosMix.gain(0, (float)(A4) / 1024 * SNvariation);  // fm Amt
    SNtone.frequency((A6) / 2.2 + 50 * SNvariation);       // tone 1 freq
    SNtone2.frequency((A6) + 100 * SNvariation);           // tone 2 freq
    SNtoneEnv.decay((float)(A5) / 3 * SNvariation);        // shorter tone decay
    SNtone.frequencyModulation(0);

  } else if (type == 2) {
    SNtone.begin(WAVEFORM_TRIANGLE);
    SNtone2.begin(WAVEFORM_TRIANGLE);
    SNtone.amplitude(0.8);
    SNtone2.amplitude(0.8);
    SNchaosMix.gain(0, (float)(A4) / 2000 * SNvariation);  // fm Amt
    SNtone.frequency((A6) / 1.5 + 50 * SNvariation);       // tone 1 freq
    SNtone2.frequency((A6) + 200 * SNvariation);           // tone 2 freq
    SNtoneEnv.decay((float)(A5) / 2 * SNvariation);        // shorter tone decay
    SNtone.frequencyModulation(1);

  } else if (type == 3) {
    SNtone.begin(WAVEFORM_SQUARE);
    SNtone2.begin(WAVEFORM_SAWTOOTH);
    SNtone.amplitude(0.7);
    SNtone2.amplitude(0.7);
    SNchaosMix.gain(0, (float)(A4) / 500 * SNvariation);  // fm Amt
    SNtone.frequency((A6) / 1.5 + 50 * SNvariation);      // tone 1 freq
    SNtone2.frequency((A6) / 2 + 100 * SNvariation);      // tone 2 freq
    SNtoneEnv.decay((float)(A5) / 1 * SNvariation);       // shorter tone decay
    SNtone.frequencyModulation(2);
  }
  SNnoteOn();
}


void HH_drum(float A7, float A8, float A9, int type) {
  int HHvariation = 1;

  ////////HIHIAT parameters /////////////////////////////////////////////////////////////////////////////////////
  HHMix.gain(1, 1 - ((float)(A9) / 1500 * HHvariation));    //blend between noise and tone
  HHMix.gain(0, (float)(A9) / 2000 * HHvariation);          //blend between noise and tonea
  HHfilt.frequency((A7)*22 + 2000 * HHvariation);           // filter freq
  HHchaosMix.gain(0, (float)(A9) / 1024 * HHvariation);     // fm Amt
  HHtoneMix.gain(0, 1 - (float)(A9) / 2300 * HHvariation);  // reduce volume when fm
  HHtoneMix.gain(1, 1 - (float)(A9) / 2300 * HHvariation);  // reduce volume when fm

  //Hihat type//////////////////////
  if (type == 1) {
    HHtone.begin(WAVEFORM_SQUARE);
    HHtone2.begin(WAVEFORM_SQUARE);
    HHtone.frequencyModulation(6);
    HHtone2.frequencyModulation(6);
    HHenv.attack(10);
    HHtone.frequency((A7)*4 + 300 * HHvariation);      // tone 1 freq
    HHtone2.frequency((A7)*7 + 100 * HHvariation);     // tone 2 freq
    HHenv.decay((float)(A8) / 2 * HHvariation);        // noise decay
    HHtoneEnv.decay((float)(A8) / 1.5 * HHvariation);  // shorter tone decay

  } else if (type == 2) {
    HHtone.begin(WAVEFORM_TRIANGLE);
    HHtone2.begin(WAVEFORM_TRIANGLE);
    HHtone.frequencyModulation(8);
    HHtone2.frequencyModulation(8);
    HHenv.attack((float)(A8) / 3);
    HHtone.frequency((A7)*6 + 300 * HHvariation);      // tone 1 freq
    HHtone2.frequency((A7)*10 + 100 * HHvariation);    // tone 2 freq
    HHenv.decay((float)(A8) / 2.5 * HHvariation);      // noise decay
    HHtoneEnv.decay((float)(A8) / 2.5 * HHvariation);  // tone decay


  } else if (type == 3) {
    HHtone.begin(WAVEFORM_TRIANGLE);
    HHtone2.begin(WAVEFORM_SQUARE);
    HHtone.frequencyModulation(3);
    HHtone2.frequencyModulation(2);
    HHenv.attack(10);
    HHtone.frequency((A7)*8 + 300 * HHvariation);    // tone 1 freq
    HHtone2.frequency((A7)*12 + 100 * HHvariation);  // tone 2 freq
    HHenv.decay((float)(A8)*1.5 * HHvariation);      // noise decay
    HHtoneEnv.decay((float)(A8)*1.5 * HHvariation);  // tone decay
  }
  HHnoteOn();
}



// Helper function to process filter adjustments
float processDrumAdjustment(DrumTypes drumType, int index, int encoder) {
  SMP.drum_settings[index][drumType] = currentMode->pos[encoder];
  //Serial.print(":::::");
  //Serial.println(SMP.drum_settings[index][drumType]);
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
  //Serial.print(drumType);
  //Serial.print(" #####---->");
  //Serial.println(mappedValue);
  return mappedValue;
}




// Update filter values
void updateDrumValue(DrumTypes drumType, int index, float value) {
  // Common function to update all filter types
  //Serial.print(drumType);
  //Serial.print(" d==>: ");
  //Serial.println(value);

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