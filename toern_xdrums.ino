
void BDnoteOn() {
  envelope1.noteOn();
  BDpitchEnv.noteOn();
}

void SNnoteOn() {
  envelope2.noteOn();
  SNtoneEnv.noteOn();
}

void HHnoteOn() {
  envelope3.noteOn();
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

void KD_drum(float A1, float A2, float A3, int type){

int BDvariation= 1;

////// BASE DRUM/KICK PARAMETERS ///////////////////////////////////////////////////////////////////////////////
  BDsine.frequency((float) A1 / 1023 * 100 + 10 * BDvariation);
  BDsaw.frequency((float) A1 / 1023 * 100 + 10 * (float)analogRead(A3) / 300 * BDvariation);
  envelope1.decay(analogRead(A2)* BDvariation);
  BDMix.gain(1, (float) A3 / 1024 / 3 * BDvariation);
  BDMix.gain(0, 1 - ((float)A3 / 1024 / 2)* BDvariation);

  //kick type//////////////////////
  if (type == 1) {
    BDsine.begin(WAVEFORM_SINE);
    BDsaw.begin(WAVEFORM_TRIANGLE);
    BDpitchEnv.decay(A2 / 6 * BDvariation);
    BDpitchAmt.amplitude((float) A3 / 1024 / 1.2 * BDvariation);
    BDchaosMix.gain(1, (float) A3 / 1300);
    BDsine.frequencyModulation(5);

  } else if (type == 2) {
    BDsine.begin(WAVEFORM_TRIANGLE);
    BDsaw.begin(WAVEFORM_TRIANGLE);
    BDpitchEnv.decay(A2 / 4 * BDvariation);
    BDpitchAmt.amplitude((float) A3 / 1024 / 1.5 * BDvariation);
    BDchaosMix.gain(1, (float) A3 / 2300);
    BDsine.frequencyModulation(2);

  } else if (type == 3) {
    BDsine.begin(WAVEFORM_SINE);
    BDsaw.begin(WAVEFORM_SINE);
    BDpitchEnv.decay(A2 / 10 * BDvariation);
    BDpitchAmt.amplitude((float) A3 / 1024 / 3 * BDvariation);
    BDchaosMix.gain(1, (float) A3 / 1000);
    BDsine.frequencyModulation(12);
  }

  
  BDnoteOn();
  
}

 void SN_drum(float A4, float A5, float A6, int type){
  int SNvariation = 1;
  
  ////// SNARE PARAMETERS ///////////////////////////////////////////////////////////////////////////////////////
  SNMix.gain(1, 1 - ((float)(A4) / 1024 * SNvariation)); //blend between noise and tone
  SNMix.gain(0, (float)(A4) / 1024 * SNvariation); //blend between noise and tone
  envelope2.decay((float)(A5)* SNvariation); // noise decay


  filter2.frequency((A6) * 3 + 1000 * SNvariation); // filter freq

  SNtoneMix.gain(0, 1 - (float)(A6) / 1850 * SNvariation); // reduce volume when fm
  SNtoneMix.gain(1, 1 - (float)(A6) / 1850 * SNvariation); // reduce volume when fm

  //Snare type////////////////
  if (type == 1) {
    SNtone.begin(WAVEFORM_SINE);
    SNtone2.begin(WAVEFORM_SINE);
    SNtone.amplitude(0.8);
    SNtone2.amplitude(0.8);
    SNchaosMix.gain(0, (float)(A4) / 1024 * SNvariation); // fm Amt
    SNtone.frequency((A6) / 2.2 + 50 * SNvariation); // tone 1 freq
    SNtone2.frequency((A6)  + 100 * SNvariation); // tone 2 freq
    SNtoneEnv.decay((float)(A5) / 3 * SNvariation); // shorter tone decay
    SNtone.frequencyModulation(0);

  } else if (type == 2) {
    SNtone.begin(WAVEFORM_TRIANGLE);
    SNtone2.begin(WAVEFORM_TRIANGLE);
    SNtone.amplitude(0.8);
    SNtone2.amplitude(0.8);
    SNchaosMix.gain(0, (float)(A4) / 2000 * SNvariation); // fm Amt
    SNtone.frequency((A6) / 1.5 + 50 * SNvariation); // tone 1 freq
    SNtone2.frequency((A6)  + 200 * SNvariation); // tone 2 freq
    SNtoneEnv.decay((float)(A5) / 2 * SNvariation); // shorter tone decay
    SNtone.frequencyModulation(1);

  } else if (type == 3) {
    SNtone.begin(WAVEFORM_SQUARE);
    SNtone2.begin(WAVEFORM_SAWTOOTH);
    SNtone.amplitude(0.7);
    SNtone2.amplitude(0.7);
    SNchaosMix.gain(0, (float)(A4) / 500 * SNvariation); // fm Amt
    SNtone.frequency((A6) / 1.5 + 50 * SNvariation); // tone 1 freq
    SNtone2.frequency((A6) / 2  + 100 * SNvariation); // tone 2 freq
    SNtoneEnv.decay((float)(A5) / 1 * SNvariation); // shorter tone decay
    SNtone.frequencyModulation(2);
  }
SNnoteOn();
}


 void HH_drum(float A7, float A8, float A9, int type){
  int HHvariation=1;

  ////////HIHIAT parameters /////////////////////////////////////////////////////////////////////////////////////
  HHMix.gain(1, 1 - ((float)(A9) / 1500 * HHvariation)); //blend between noise and tone
  HHMix.gain(0, (float)(A9) / 2000 * HHvariation); //blend between noise and tonea
  filter3.frequency((A7) * 22 + 2000 * HHvariation); // filter freq
  HHchaosMix.gain(0, (float)(A9) / 1024 * HHvariation); // fm Amt
  HHtoneMix.gain(0, 1 - (float)(A9) / 2300 * HHvariation); // reduce volume when fm
  HHtoneMix.gain(1, 1 - (float)(A9) / 2300 * HHvariation); // reduce volume when fm

  //Hihat type//////////////////////
  if (type == 1) {
    HHtone.begin(WAVEFORM_SQUARE);
    HHtone2.begin(WAVEFORM_SQUARE);
    HHtone.frequencyModulation(6);
    HHtone2.frequencyModulation(6);
    envelope3.attack(10);
    HHtone.frequency((A7) * 4 + 300 * HHvariation); // tone 1 freq
    HHtone2.frequency((A7) * 7 + 100 * HHvariation); // tone 2 freq
    envelope3.decay((float)(A8) / 2 * HHvariation); // noise decay
    HHtoneEnv.decay((float)(A8) / 1.5 * HHvariation); // shorter tone decay

  } else if (type == 2) {
    HHtone.begin(WAVEFORM_TRIANGLE);
    HHtone2.begin(WAVEFORM_TRIANGLE);
    HHtone.frequencyModulation(8);
    HHtone2.frequencyModulation(8);
    envelope3.attack((float)(A8) / 3);
    HHtone.frequency((A7) * 6 + 300 * HHvariation); // tone 1 freq
    HHtone2.frequency((A7) * 10 + 100 * HHvariation); // tone 2 freq
    envelope3.decay((float)(A8) / 2.5 * HHvariation); // noise decay
    HHtoneEnv.decay((float)(A8) / 2.5 * HHvariation); // tone decay


  } else if (type == 3) {
    HHtone.begin(WAVEFORM_TRIANGLE);
    HHtone2.begin(WAVEFORM_SQUARE);
    HHtone.frequencyModulation(3);
    HHtone2.frequencyModulation(2);
    envelope3.attack(10);
    HHtone.frequency((A7) * 8 + 300 * HHvariation); // tone 1 freq
    HHtone2.frequency((A7) * 12 + 100 * HHvariation); // tone 2 freq
    envelope3.decay((float)(A8) * 1.5 * HHvariation); // noise decay
    HHtoneEnv.decay((float)(A8) * 1.5 * HHvariation); // tone decay

  }
HHnoteOn();
  }
