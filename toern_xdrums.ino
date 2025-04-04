
#define POLY_VOICES 3

#define AUTO_OFF_TIME 1800  // milliseconds
unsigned long voiceStartTime[2][POLY_VOICES] = {0, 0, 0};


int cents[2][3] = {0, 0, 0};
int semitones[2][3] = {0, 0, 0};
int pan[2][3] = {0, 0, 0};
int volume[2][3] = {100, 100, 100};

int attackAmp[2][3] = {100, 100, 100};
int decayAmp[2][3] = {100, 100, 100};
float sustainAmp[2][3] = {1, 1, 1};
int releaseAmp[2][3] = {100, 100, 100};

int attackFilter[2][3] = {100, 100, 100};
int decayFilter[2][3] = {100, 100, 100};
float sustainFilter[2][3] = {1, 1, 1};
int releaseFilter[2][3] = {100, 100, 100};

int cutoff[2][3] = {1000,1000,1000};
float resonance[2][3] = {2.0,2.0,2.0};
float filterAmount[2][3] = {2.0, 2.0, 2.0};

int waveforms[2][3] = {WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH};
int waveformsArray[2][5] = {WAVEFORM_SAWTOOTH, WAVEFORM_PULSE, WAVEFORM_SQUARE, WAVEFORM_TRIANGLE, WAVEFORM_SINE};
float pulseWidth[2][3] = {0.25, 0.25, 0.25};

int noteArray[2][8];
int notePlaying[2] = {0};


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

  Serial.print("BD ");
  Serial.print(A1);
  Serial.print(" ");
  Serial.print(A2);
  Serial.print(" ");
  Serial.print(A3);
  Serial.print(" ");
  Serial.print(type);
  Serial.println(" ");



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



/*
void keys_synth(float dummy1, float dummy2, float dummy3) {
  // Keys preset parameters (menuIndex==1)
  octave = 4.0;

  cents[0] = 2;
  cents[1] = -2;
  cents[2] = 0;
  semitones[0] = 0;
  semitones[1] = 0;
  semitones[2] = 12;
  pan[0] = -100;
  pan[1] = 100;
  pan[2] = 0;
  volume[0] = 100;
  volume[1] = 100;
  volume[2] = 100;

  attackAmp[0] = 0;
  attackAmp[1] = 0;
  attackAmp[2] = 0;
  decayAmp[0] = 2500;
  decayAmp[1] = 2500;
  decayAmp[2] = 2500;
  sustainAmp[0] = 0;
  sustainAmp[1] = 0;
  sustainAmp[2] = 0;
  releaseAmp[0] = 300;
  releaseAmp[1] = 300;
  releaseAmp[2] = 300;

  attackFilter[0] = 0;
  attackFilter[1] = 0;
  attackFilter[2] = 0;
  decayFilter[0] = 2500;
  decayFilter[1] = 2500;
  decayFilter[2] = 2500;
  sustainFilter[0] = 0;
  sustainFilter[1] = 0;
  sustainFilter[2] = 0;
  releaseFilter[0] = 300;
  releaseFilter[1] = 300;
  releaseFilter[2] = 300;

  cutoff[0] = 1000;
  cutoff[1] = 1000;
  cutoff[2] = 1000;
  resonance[0] = 0.2;
  resonance[1] = 0.2;
  resonance[2] = 0.2;
  filterAmount[0] = 3.0;
  filterAmount[1] = 3.0;
  filterAmount[2] = 3.0;

  waveforms[0] = 0;
  waveforms[1] = 0;
  waveforms[2] = 0;
  updateVals();
}

void pad_synth(float dummy1, float dummy2, float dummy3) {
  // Pad preset parameters (menuIndex==2)
  octave = 4.0;

  cents[0] = 15;
  cents[1] = -15;
  cents[2] = 0;
  semitones[0] = 0;
  semitones[1] = 0;
  semitones[2] = 0;
  pan[0] = -100;
  pan[1] = 100;
  pan[2] = 0;
  volume[0] = 100;
  volume[1] = 100;
  volume[2] = 0;

  attackAmp[0] = 1500;
  attackAmp[1] = 1500;
  attackAmp[2] = 1500;
  decayAmp[0] = 60;
  decayAmp[1] = 60;
  decayAmp[2] = 60;
  sustainAmp[0] = 0.6;
  sustainAmp[1] = 0.6;
  sustainAmp[2] = 0.6;
  releaseAmp[0] = 1500;
  releaseAmp[1] = 1500;
  releaseAmp[2] = 1500;

  attackFilter[0] = 1500;
  attackFilter[1] = 1500;
  attackFilter[2] = 1500;
  decayFilter[0] = 6000;
  decayFilter[1] = 6000;
  decayFilter[2] = 6000;
  sustainFilter[0] = 0.2;
  sustainFilter[1] = 0.2;
  sustainFilter[2] = 0.2;
  releaseFilter[0] = 1500;
  releaseFilter[1] = 1500;
  releaseFilter[2] = 1500;

  cutoff[0] = 1000;
  cutoff[1] = 1000;
  cutoff[2] = 1000;
  resonance[0] = 0.5;
  resonance[1] = 0.5;
  resonance[2] = 0.5;
  filterAmount[0] = 2.0;
  filterAmount[1] = 2.0;
  filterAmount[2] = 2.0;

  waveforms[0] = 4;
  waveforms[1] = 4;
  waveforms[2] = 4;

  updateVals();
}

void organ_synth(float dummy1, float dummy2, float dummy3) {
  // Organ preset parameters (menuIndex==6)
  octave = 5.0;

  cents[0] = 0;
  cents[1] = 0;
  cents[2] = 0;
  semitones[0] = 0;
  semitones[1] = -12;
  semitones[2] = 7;
  pan[0] = 0;
  pan[1] = 0;
  pan[2] = 0;
  volume[0] = 100;
  volume[1] = 100;
  volume[2] = 100;

  attackAmp[0] = 0;
  attackAmp[1] = 0;
  attackAmp[2] = 0;
  decayAmp[0] = 100;
  decayAmp[1] = 100;
  decayAmp[2] = 100;
  sustainAmp[0] = 0.1;
  sustainAmp[1] = 0.1;
  sustainAmp[2] = 0.1;
  releaseAmp[0] = 100;
  releaseAmp[1] = 100;
  releaseAmp[2] = 100;

  attackFilter[0] = 0;
  attackFilter[1] = 0;
  attackFilter[2] = 0;
  decayFilter[0] = 100;
  decayFilter[1] = 100;
  decayFilter[2] = 100;
  sustainFilter[0] = 0.6;
  sustainFilter[1] = 0.6;
  sustainFilter[2] = 0.6;
  releaseFilter[0] = 100;
  releaseFilter[1] = 100;
  releaseFilter[2] = 100;

  cutoff[0] = 1000;
  cutoff[1] = 1000;
  cutoff[2] = 1000;
  resonance[0] = 0.5;
  resonance[1] = 0.5;
  resonance[2] = 0.5;
  filterAmount[0] = 2.0;
  filterAmount[1] = 2.0;
  filterAmount[2] = 2.0;

  waveforms[0] = 4;
  waveforms[1] = 4;
  waveforms[2] = 4;
  updateVals();
}

void flute_synth(float dummy1, float dummy2, float dummy3) {
  // Flute preset parameters (menuIndex==7)
  octave = 5.0;

  cents[0] = -1;
  cents[1] = 1;
  cents[2] = 0;
  semitones[0] = 0;
  semitones[1] = 0;
  semitones[2] = 12;
  pan[0] = -100;
  pan[1] = 100;
  pan[2] = 0;
  volume[0] = 100;
  volume[1] = 100;
  volume[2] = 10;

  attackAmp[0] = 500;
  attackAmp[1] = 500;
  attackAmp[2] = 500;
  decayAmp[0] = 0;
  decayAmp[1] = 0;
  decayAmp[2] = 0;
  sustainAmp[0] = 1.0;
  sustainAmp[1] = 1.0;
  sustainAmp[2] = 1.0;
  releaseAmp[0] = 400;
  releaseAmp[1] = 400;
  releaseAmp[2] = 400;

  attackFilter[0] = 500;
  attackFilter[1] = 500;
  attackFilter[2] = 500;
  decayFilter[0] = 0;
  decayFilter[1] = 0;
  decayFilter[2] = 0;
  sustainFilter[0] = 1.0;
  sustainFilter[1] = 1.0;
  sustainFilter[2] = 1.0;
  releaseFilter[0] = 500;
  releaseFilter[1] = 500;
  releaseFilter[2] = 500;

  cutoff[0] = 1000;
  cutoff[1] = 1000;
  cutoff[2] = 1000;
  resonance[0] = 0.5;
  resonance[1] = 0.5;
  resonance[2] = 0.5;
  filterAmount[0] = 2.6;
  filterAmount[1] = 2.6;
  filterAmount[2] = 2.6;

  waveforms[0] = 3;
  waveforms[1] = 3;
  waveforms[2] = 4;
  updateVals();
}

void wow_synth(float dummy1, float dummy2, float dummy3) {
  // Wow preset parameters (menuIndex==9)
  octave = 4.0;

  cents[0] = 5;
  cents[1] = -5;
  cents[2] = 0;
  semitones[0] = 7;
  semitones[1] = 0;
  semitones[2] = -12;
  pan[0] = -50;
  pan[1] = 50;
  pan[2] = 0;
  volume[0] = 100;
  volume[1] = 100;
  volume[2] = 100;

  attackAmp[0] = 400;
  attackAmp[1] = 400;
  attackAmp[2] = 400;
  decayAmp[0] = 4000;
  decayAmp[1] = 4000;
  decayAmp[2] = 4000;
  sustainAmp[0] = 0.8;
  sustainAmp[1] = 0.8;
  sustainAmp[2] = 0.8;
  releaseAmp[0] = 10;
  releaseAmp[1] = 10;
  releaseAmp[2] = 10;

  attackFilter[0] = 800;
  attackFilter[1] = 800;
  attackFilter[2] = 800;
  decayFilter[0] = 6000;
  decayFilter[1] = 6000;
  decayFilter[2] = 6000;
  sustainFilter[0] = 0.2;
  sustainFilter[1] = 0.2;
  sustainFilter[2] = 0.2;
  releaseFilter[0] = 10;
  releaseFilter[1] = 10;
  releaseFilter[2] = 10;

  cutoff[0] = 1000;
  cutoff[1] = 1000;
  cutoff[2] = 1000;
  resonance[0] = 0.8;
  resonance[1] = 0.8;
  resonance[2] = 0.8;
  filterAmount[0] = 1.5;
  filterAmount[1] = 1.5;
  filterAmount[2] = 1.5;

  waveforms[0] = 0;
  waveforms[1] = 0;
  waveforms[2] = 0;
  updateVals();
}



*/

void bass_synth(float dummy1, float dummy2, float dummy3) {
  int ch=0;
   octave[ch] =  2.0;
    cents[ch][0] = 5;
        cents[ch][1] = -5;
        cents[ch][2] = 0;
        
        semitones[ch][0] = 0;
        semitones[ch][1] = 0;
        semitones[ch][2] = -12;
        
        pan[ch][0] = -100;
        pan[ch][1] = 100;
        pan[ch][2] = 0;
        
        volume[ch][0] = 100;
        volume[ch][1] = 100;
        volume[ch][2] = 100;
        
        attackAmp[ch][0] = 0;
        attackAmp[ch][1] = 0;
        attackAmp[ch][2] = 0;
        decayAmp[ch][0] = 400;
        decayAmp[ch][1] = 400;
        decayAmp[ch][2] = 400;
        sustainAmp[ch][0] = 0.0;
        sustainAmp[ch][1] = 0.0;
        sustainAmp[ch][2] = 0.0;
        releaseAmp[ch][0] = 10;
        releaseAmp[ch][1] = 10;
        releaseAmp[ch][2] = 10;
        
        attackFilter[ch][0] = 0;
        attackFilter[ch][1] = 0;
        attackFilter[ch][2] = 0;
        decayFilter[ch][0] = 250;
        decayFilter[ch][1] = 250;
        decayFilter[ch][2] = 250;
        sustainFilter[ch][0] = 0.0;
        sustainFilter[ch][1] = 0.0;
        sustainFilter[ch][2] = 0.0;
        releaseFilter[ch][0] = 10;
        releaseFilter[ch][1] = 10;
        releaseFilter[ch][2] = 10;
        
        cutoff[ch][0] = 1000;
        cutoff[ch][1] = 1000;
        cutoff[ch][2] = 1000;
        resonance[ch][0] = 0.2;
        resonance[ch][1] = 0.2;
        resonance[ch][2] = 0.2;
        filterAmount[ch][0] = 3.0;
        filterAmount[ch][1] = 3.0;
        filterAmount[ch][2] = 3.0;
        
        waveforms[ch][0] = 0;
        waveforms[ch][1] = 0;
        waveforms[ch][2] = 1;
  
  updateVals(ch);
}



void brass_synth(float dummy1, float dummy2, float dummy3) {
  int ch=1;
  // Brass preset parameters (menuIndex==5)
  octave[ch] = 4.0;

  cents[ch][0] = -4;
  cents[ch][1] = 4;
  cents[ch][2] = 0;
  semitones[ch][0] = 0;
  semitones[ch][1] = 0;
  semitones[ch][2] = 0;
  pan[ch][0] = -100;
  pan[ch][1] = 100;
  pan[ch][2] = 0;
  volume[ch][0] = 100;
  volume[ch][1] = 100;
  volume[ch][2] = 100;

  attackAmp[ch][0] = 0;
  attackAmp[ch][1] = 0;
  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 2500;
  decayAmp[ch][1] = 2500;
  decayAmp[ch][2] = 2500;
  sustainAmp[ch][0] = 0.8;
  sustainAmp[ch][1] = 0.8;
  sustainAmp[ch][2] = 0.8;
  releaseAmp[ch][0] = 250;
  releaseAmp[ch][1] = 250;
  releaseAmp[ch][2] = 250;

  attackFilter[ch][0] = 100;
  attackFilter[ch][1] = 100;
  attackFilter[ch][2] = 100;
  decayFilter[ch][0] = 2500;
  decayFilter[ch][1] = 2500;
  decayFilter[ch][2] = 2500;
  sustainFilter[ch][0] = 0.8;
  sustainFilter[ch][1] = 0.8;
  sustainFilter[ch][2] = 0.8;
  releaseFilter[ch][0] = 250;
  releaseFilter[ch][1] = 250;
  releaseFilter[ch][2] = 250;

  cutoff[ch][0] = 1000;
  cutoff[ch][1] = 1000;
  cutoff[ch][2] = 1000;
  resonance[ch][0] = 0.2;
  resonance[ch][1] = 0.2;
  resonance[ch][2] = 0.2;
  filterAmount[ch][0] = 2.0;
  filterAmount[ch][1] = 2.0;
  filterAmount[ch][2] = 2.0;

  waveforms[ch][0] = 0;
  waveforms[ch][1] = 0;
  waveforms[ch][2] = 0;
  updateVals(ch);
}



/*
void chiptune_synth(float dummy1, float dummy2, float dummy3) {
  // Chiptune preset parameters (menuIndex==8)
   octave =  4.0;
        
        cents[0] = 0;
        cents[1] = 0;
        cents[2] = 0;

        semitones[0] = 0;
        semitones[1] = 0;
        semitones[2] = 12;

        pan[0] = -100;
        pan[1] = 100;
        pan[2] = 0;

        volume[0] = 100;
        volume[1] = 100;
        volume[2] = 10;

        attackAmp[0] = 0;
        attackAmp[1] = 0;
        attackAmp[2] = 0;
        decayAmp[0] = 0;
        decayAmp[1] = 0;
        decayAmp[2] = 0;
        sustainAmp[0] = 1.0;
        sustainAmp[1] = 1.0;
        sustainAmp[2] = 1.0;
        releaseAmp[0] = 100;
        releaseAmp[1] = 100;
        releaseAmp[2] = 100;

        attackFilter[0] = 0;
        attackFilter[1] = 0;
        attackFilter[2] = 0;
        decayFilter[0] = 0;
        decayFilter[1] = 0;
        decayFilter[2] = 0;
        sustainFilter[0] = 1.0;
        sustainFilter[1] = 1.0;
        sustainFilter[2] = 1.0;
        releaseFilter[0] = 100;
        releaseFilter[1] = 100;
        releaseFilter[2] = 100;

        cutoff[0] = 2000;
        cutoff[1] = 2000;
        cutoff[2] = 2000;
        resonance[0] = 0.0;
        resonance[1] = 0.0;
        resonance[2] = 0.0;
        filterAmount[0] = 2.5;
        filterAmount[1] = 2.5;
        filterAmount[2] = 2.5;

        waveforms[0] = 1;
        waveforms[1] = 1;
        waveforms[2] = 1;

  updateVals();
}
*/

//–––––– updateVals() Function ––––––//
// This function updates all the audio objects based on the current global parameters.
void updateVals(int ch) {
  
  for (int i = 0; i < POLY_VOICES ; i++) {


    // Update amplitude envelopes
    Senvelope1[i].attack(attackAmp[ch][i]);
    Senvelope1[i].decay(decayAmp[ch][i]);
    Senvelope1[i].sustain(sustainAmp[ch][i]);
    Senvelope1[i].release(releaseAmp[ch][i]);
  
    Senvelope2[i].attack(attackAmp[ch][i]);
    Senvelope2[i].decay(decayAmp[ch][i]);
    Senvelope2[i].sustain(sustainAmp[ch][i]);
    Senvelope2[i].release(releaseAmp[ch][i]);

    // Update filter envelope
    SenvelopeFilter1[i].attack(attackFilter[ch][i]);
    SenvelopeFilter1[i].decay(decayFilter[ch][i]);
    SenvelopeFilter1[i].sustain(sustainFilter[ch][i]);
    SenvelopeFilter1[i].release(releaseFilter[ch][i]);

    // Set waveform for oscillator 1 (waveform1)
    if (waveforms[ch][0] == 0) {
      Swaveform1[i].begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][0] == 1) {
      Swaveform1[i].begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][0] == 2) {
      Swaveform1[i].begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][0] == 3) {
      Swaveform1[i].begin(waveformsArray[ch][3]);
    } else if (waveforms[ch][0] == 4) {
      Swaveform1[i].begin(waveformsArray[ch][4]);
    }
    Swaveform1[i].amplitude(1.0);
    Swaveform1[i].pulseWidth(pulseWidth[ch][0]);

    // Set waveform for oscillator 2 (waveform2)
    if (waveforms[ch][1] == 0) {
      Swaveform2[i].begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][1] == 1) {
      Swaveform2[i].begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][1] == 2) {
      Swaveform2[i].begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][1] == 3) {
      Swaveform2[i].begin(waveformsArray[ch][3]);
    } else if (waveforms[ch][1] == 4) {
      Swaveform2[i].begin(waveformsArray[ch][4]);
    }
    Swaveform2[i].amplitude(1.0);
    Swaveform2[i].pulseWidth(pulseWidth[ch][1]);

    // Set waveform for oscillator 3 (waveform3)
    if (waveforms[ch][2] == 0) {
      Swaveform3[i].begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][2] == 1) {
      Swaveform3[i].begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][2] == 2) {
      Swaveform3[i].begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][2] == 3) {
      Swaveform3[i].begin(waveformsArray[ch][3]);
    } else if (waveforms[ch][2] == 4) {
      Swaveform3[i].begin(waveformsArray[ch][4]);
    }

    Swaveform3[i].amplitude(1.0);
    Swaveform3[i].pulseWidth(pulseWidth[ch][2]);

    // Update filter settings for both ladders
    Sladder1[i].frequency(cutoff[ch][i]);
    Sladder1[i].octaveControl(filterAmount[ch][i]);
    Sladder1[i].resonance(resonance[ch][i]);

    Sladder2[i].frequency(cutoff[ch][i]);
    Sladder2[i].octaveControl(filterAmount[ch][i]);
    Sladder2[i].resonance(resonance[ch][i]);

    // Update mixer gains (example: using pan and volume to calculate gain)
    Smixer1[i].gain(0, max(0, (-pan[ch][0] + 100.0) * 0.005) * (volume[ch][0] / 100.0) * 0.33);
    Smixer1[i].gain(1, max(0, (-pan[ch][1] + 100.0) * 0.005) * (volume[ch][1] / 100.0) * 0.33);
    Smixer1[i].gain(2, max(0, (-pan[ch][2] + 100.0) * 0.005) * (volume[ch][2] / 100.0) * 0.33);

    Smixer2[i].gain(0, max(0, (pan[ch][0] + 100.0) * 0.005) * (volume[ch][0] / 100.0) * 0.33);
    Smixer2[i].gain(1, max(0, (pan[ch][1] + 100.0) * 0.005) * (volume[ch][1] / 100.0) * 0.33);
    Smixer2[i].gain(2, max(0, (pan[ch][2] + 100.0) * 0.005) * (volume[ch][2] / 100.0) * 0.33);

  }
}

// Modify playSound() so that when a note is played, you store its start time.
void playSound(int note, int ch) {
    stopSound(note, ch);
    notePlaying[ch] += 1;
    if (notePlaying[ch] >= POLY_VOICES) {
        notePlaying[ch] = 0;
    }
    noteArray[ch][notePlaying[ch]] = note;
    voiceStartTime[ch][notePlaying[ch]] = millis();  // Record the start time for this voice

    // Play note with frequency calculations, etc.
    Swaveform1[notePlaying[ch]].frequency(
      notesArray[note + semitones[ch][0]] * pow(2, cents[ch][0] / 1200.0) * pow(2, random(-2, 3) / 1200.0)
    );
    Swaveform2[notePlaying[ch]].frequency(
      notesArray[note + semitones[ch][1]] * pow(2, cents[ch][1] / 1200.0) * pow(2, random(-2, 3) / 1200.0)
    );
    Swaveform3[notePlaying[ch]].frequency(
      notesArray[note + semitones[ch][2]] * pow(2, cents[ch][2] / 1200.0) * pow(2, random(-2, 3) / 1200.0)
    );
    Senvelope1[notePlaying[ch]].noteOn();
    Senvelope2[notePlaying[ch]].noteOn();
    SenvelopeFilter1[notePlaying[ch]].noteOn();
}

// This function checks each voice and auto-offs active notes after AUTO_OFF_TIME.
void autoOffActiveNotes() {
    unsigned long currentTime = millis();
    for (int ch = 0; ch < 2; ch++) {
    for (int i = 0; i < POLY_VOICES; i++) {
        if (noteArray[ch][i] != 0) { // There is an active note on this voice.
            if (currentTime - voiceStartTime[ch][i] >= AUTO_OFF_TIME) {
                stopSound(noteArray[ch][i],ch);  // This will call noteOff on all envelopes for that note.
            }
        }
    }
  }
}

// Example stopSound() function (assuming noteArray[] holds the note numbers for active voices)
void stopSound(int note, int ch) {
    for (int num = 0; num < POLY_VOICES; num++) {
        if (noteArray[ch][num] == note) {
            Serial.print("NOTE OFF: ");
            Serial.println(num);
            Senvelope1[num].noteOff();
            Senvelope2[num].noteOff();
            SenvelopeFilter1[num].noteOff();
            noteArray[ch][num] = 0;
        }
    }
}