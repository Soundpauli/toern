#define POLY_VOICES 3
#define INSTRUMENT_CHANNELS 2

#define AUTO_OFF_TIME 1800  // milliseconds
unsigned long voiceStartTime[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0, 0, 0 };

int cents[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0, 0, 0 };
int semitones[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0, 0, 0 };
int pan[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0, 0, 0 };
int volume[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };

int attackAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };
int decayAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };
float sustainAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = { 1, 1, 1 };
int releaseAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };

int attackFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };
int decayFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };
float sustainFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = { 1, 1, 1 };
int releaseFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };

int cutoff[INSTRUMENT_CHANNELS][POLY_VOICES] = { 1000, 1000, 1000 };
float resonance[INSTRUMENT_CHANNELS][POLY_VOICES] = { 2.0, 2.0, 2.0 };
float filterAmount[INSTRUMENT_CHANNELS][POLY_VOICES] = { 2.0, 2.0, 2.0 };

int waveforms[INSTRUMENT_CHANNELS][POLY_VOICES] = { WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH };
int waveformsArray[INSTRUMENT_CHANNELS][5] = { WAVEFORM_SAWTOOTH, WAVEFORM_PULSE, WAVEFORM_SQUARE, WAVEFORM_TRIANGLE, WAVEFORM_SINE };
float pulseWidth[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0.25, 0.25, 0.25 };

int noteArray[INSTRUMENT_CHANNELS][8];
int notePlaying[2] = { 0 };


/****INSTRUMENTS*****/
// arp_synth preset (menuIndex==?) with additional parameters
void arp_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  // Default tuning and sound parameters
  octave[ch] = 5.0;
  cents[ch][0] = 10;  cents[ch][1] = -10; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 0;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 0;   decayAmp[ch][1] = 0;   decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 2; releaseAmp[ch][1] = 2; releaseAmp[ch][2] = 2;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 0;   decayFilter[ch][1] = 0;   decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 2;  releaseFilter[ch][1] = 2;  releaseFilter[ch][2] = 2;

  // Extreme modulation mapping for filter parameters (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  // Default waveform settings (for voices 1–3 remain)
  waveforms[ch][0] = 0;
  waveforms[ch][1] = 0;
  waveforms[ch][2] = 0;

  // --- Additional modulation ---

  // p4 for pitch offset:
  // Map p4 (0-64) to an additional offset from -50 to +50 cents...
  // and -2 to +2 semitones.
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;  
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);  
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }

  // p5 for octave override: map from 1.0 to 8.0.
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;

  // p6 for waveform of first voice: map to an integer between 0 and 7.
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// lead_synth preset (menuIndex==?) with additional parameters
void lead_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 0;   cents[ch][1] = 0;   cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 0;
  pan[ch][0] = 0;     pan[ch][1] = 0;     pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 0;   volume[ch][2] = 0;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 250; decayAmp[ch][1] = 250; decayAmp[ch][2] = 250;
  sustainAmp[ch][0] = 0; sustainAmp[ch][1] = 0; sustainAmp[ch][2] = 0;
  releaseAmp[ch][0] = 250; releaseAmp[ch][1] = 250; releaseAmp[ch][2] = 250;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 250;  decayFilter[ch][1] = 250;  decayFilter[ch][2] = 250;
  sustainFilter[ch][0] = 0;  sustainFilter[ch][1] = 0;  sustainFilter[ch][2] = 0;
  releaseFilter[ch][0] = 250; releaseFilter[ch][1] = 250; releaseFilter[ch][2] = 250;

  // Extreme modulation mapping for filter parameters (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 2;
  waveforms[ch][1] = 2;
  waveforms[ch][2] = 2;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// keys_synth preset (menuIndex==1) with additional parameters
void keys_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 2;   cents[ch][1] = -2;   cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100;  pan[ch][1] = 100;  pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 2500; decayAmp[ch][1] = 2500; decayAmp[ch][2] = 2500;
  sustainAmp[ch][0] = 0;   sustainAmp[ch][1] = 0;   sustainAmp[ch][2] = 0;
  releaseAmp[ch][0] = 300; releaseAmp[ch][1] = 300; releaseAmp[ch][2] = 300;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 2500; decayFilter[ch][1] = 2500; decayFilter[ch][2] = 2500;
  sustainFilter[ch][0] = 0;  sustainFilter[ch][1] = 0;  sustainFilter[ch][2] = 0;
  releaseFilter[ch][0] = 300; releaseFilter[ch][1] = 300; releaseFilter[ch][2] = 300;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 0;
  waveforms[ch][1] = 0;
  waveforms[ch][2] = 0;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// pad_synth preset (menuIndex==2) with additional parameters
void pad_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 15;   cents[ch][1] = -15;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0;  semitones[ch][2] = 0;
  pan[ch][0] = -100;   pan[ch][1] = 100;   pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100;  volume[ch][2] = 0;
  
  attackAmp[ch][0] = 1500; attackAmp[ch][1] = 1500; attackAmp[ch][2] = 1500;
  sustainAmp[ch][0] = 0.6;  sustainAmp[ch][1] = 0.6;  sustainAmp[ch][2] = 0.6;
  releaseAmp[ch][0] = 1500; releaseAmp[ch][1] = 1500; releaseAmp[ch][2] = 1500;
  
  attackFilter[ch][0] = 1500; attackFilter[ch][1] = 1500; attackFilter[ch][2] = 1500;
  sustainFilter[ch][0] = 0.2;  sustainFilter[ch][1] = 0.2;  sustainFilter[ch][2] = 0.2;
  releaseFilter[ch][0] = 1500; releaseFilter[ch][1] = 1500; releaseFilter[ch][2] = 1500;
  
  // Map p1 to decay amplitude (p1 modulates decay from 60 to 3000)
  float minDecay = 60.0, maxDecay = 3000.0;
  float mappedDecay = minDecay + (p1 / 64.0) * (maxDecay - minDecay);
  decayAmp[ch][0] = decayAmp[ch][1] = decayAmp[ch][2] = mappedDecay;

  // p2 for filter cutoff; p3 for filter resonance:
  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p2 / 64.0) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p3 / 64.0) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 4;  waveforms[ch][1] = 4;  waveforms[ch][2] = 4;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// organ_synth preset (menuIndex==6) with additional parameters
void organ_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 5.0;
  cents[ch][0] = 0;  cents[ch][1] = 0;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = -12; semitones[ch][2] = 7;
  pan[ch][0] = 0;    pan[ch][1] = 0;    pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 100; decayAmp[ch][1] = 100; decayAmp[ch][2] = 100;
  sustainAmp[ch][0] = 0.1; sustainAmp[ch][1] = 0.1; sustainAmp[ch][2] = 0.1;
  releaseAmp[ch][0] = 100; releaseAmp[ch][1] = 100; releaseAmp[ch][2] = 100;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 100; decayFilter[ch][1] = 100; decayFilter[ch][2] = 100;
  sustainFilter[ch][0] = 0.6; sustainFilter[ch][1] = 0.6; sustainFilter[ch][2] = 0.6;
  releaseFilter[ch][0] = 100; releaseFilter[ch][1] = 100; releaseFilter[ch][2] = 100;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 4;  waveforms[ch][1] = 4;  waveforms[ch][2] = 4;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// flute_synth preset (menuIndex==7) with additional parameters
void flute_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 5.0;
  cents[ch][0] = -1;  cents[ch][1] = 1;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100;  pan[ch][1] = 100;  pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 10;

  decayAmp[ch][0] = 0; decayAmp[ch][1] = 0; decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 400; releaseAmp[ch][1] = 400; releaseAmp[ch][2] = 400;
  
  attackFilter[ch][0] = 500; attackFilter[ch][1] = 500; attackFilter[ch][2] = 500;
  decayFilter[ch][0] = 0;   decayFilter[ch][1] = 0;   decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 500; releaseFilter[ch][1] = 500; releaseFilter[ch][2] = 500;

  // p1 modulates attack amplitude (flute dynamics)
  float minAttack = 100.0, maxAttack = 2000.0;
  float mappedAttack = minAttack + (p1 / 64.0) * (maxAttack - minAttack);
  attackAmp[ch][0] = attackAmp[ch][1] = attackAmp[ch][2] = mappedAttack;

  // p2 for filter cutoff; p3 for filter resonance:
  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p2 / 64.0) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p3 / 64.0) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 3;  waveforms[ch][1] = 3;  waveforms[ch][2] = 4;
  
  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// wow_synth preset (menuIndex==9) with additional parameters
void wow_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 5;  cents[ch][1] = -5;  cents[ch][2] = 0;
  semitones[ch][0] = 7; semitones[ch][1] = 0; semitones[ch][2] = -12;
  pan[ch][0] = -50; pan[ch][1] = 50; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;
  attackAmp[ch][0] = 400; attackAmp[ch][1] = 400; attackAmp[ch][2] = 400;
  decayAmp[ch][0] = 4000; decayAmp[ch][1] = 4000; decayAmp[ch][2] = 4000;
  sustainAmp[ch][0] = 0.8; sustainAmp[ch][1] = 0.8; sustainAmp[ch][2] = 0.8;
  
  // p3 now is used to modulate release amp dramatically: from 10 to 1000.
  float minRelease = 10.0, maxRelease = 1000.0;
  float mappedRelease = minRelease + (p3 / 64.0) * (maxRelease - minRelease);
  releaseAmp[ch][0] = releaseAmp[ch][1] = releaseAmp[ch][2] = mappedRelease;

  attackFilter[ch][0] = 800; attackFilter[ch][1] = 800; attackFilter[ch][2] = 800;
  decayFilter[ch][0] = 6000; decayFilter[ch][1] = 6000; decayFilter[ch][2] = 6000;
  sustainFilter[ch][0] = 0.2; sustainFilter[ch][1] = 0.2; sustainFilter[ch][2] = 0.2;
  releaseFilter[ch][0] = 10;  releaseFilter[ch][1] = 10;  releaseFilter[ch][2] = 10;

  // p1 and p2 still control filter cutoff and resonance:
  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 0;  waveforms[ch][1] = 0;  waveforms[ch][2] = 0;
  
  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// bass_synth preset (with integrated parameters) with additional parameters
void bass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 2.0;
  cents[ch][0] = 5;  cents[ch][1] = -5;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = -12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 400; decayAmp[ch][1] = 400; decayAmp[ch][2] = 400;
  sustainAmp[ch][0] = 0.0; sustainAmp[ch][1] = 0.0; sustainAmp[ch][2] = 0.0;
  releaseAmp[ch][0] = 10;  releaseAmp[ch][1] = 10;  releaseAmp[ch][2] = 10;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 250;  decayFilter[ch][1] = 250;  decayFilter[ch][2] = 250;
  sustainFilter[ch][0] = 0.0; sustainFilter[ch][1] = 0.0; sustainFilter[ch][2] = 0.0;
  releaseFilter[ch][0] = 10;  releaseFilter[ch][1] = 10;  releaseFilter[ch][2] = 10;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  // Default waveform for voices
  waveforms[ch][0] = 0;  waveforms[ch][1] = 0;  waveforms[ch][2] = 1;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// brass_synth preset (menuIndex==5) with additional parameters
void brass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = -4;  cents[ch][1] = 4;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 0;
  pan[ch][0] = -100;  pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 2500; decayAmp[ch][1] = 2500; decayAmp[ch][2] = 2500;
  sustainAmp[ch][0] = 0.8; sustainAmp[ch][1] = 0.8; sustainAmp[ch][2] = 0.8;
  releaseAmp[ch][0] = 250; releaseAmp[ch][1] = 250; releaseAmp[ch][2] = 250;

  attackFilter[ch][0] = 100;  attackFilter[ch][1] = 100;  attackFilter[ch][2] = 100;
  decayFilter[ch][0] = 2500;  decayFilter[ch][1] = 2500;  decayFilter[ch][2] = 2500;
  sustainFilter[ch][0] = 0.8; sustainFilter[ch][1] = 0.8; sustainFilter[ch][2] = 0.8;
  releaseFilter[ch][0] = 250; releaseFilter[ch][1] = 250; releaseFilter[ch][2] = 250;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 0; waveforms[ch][1] = 0; waveforms[ch][2] = 0;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// chiptune_synth preset (menuIndex==8) with additional parameters
void chiptune_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 0;  cents[ch][1] = 0;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 10;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 0;  decayAmp[ch][1] = 0;  decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 100; releaseAmp[ch][1] = 100; releaseAmp[ch][2] = 100;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 0;  decayFilter[ch][1] = 0;  decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 100; releaseFilter[ch][1] = 100; releaseFilter[ch][2] = 100;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 1; waveforms[ch][1] = 1; waveforms[ch][2] = 1;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



//–––––– updateVals() Function ––––––//
// This function updates all the audio objects based on the current global parameters.
void updateVals(int ch) {

  for (int i = 0; i < POLY_VOICES; i++) {
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
      Swaveform1[i].begin(waveformsArray[ch][POLY_VOICES]);
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
      Swaveform2[i].begin(waveformsArray[ch][POLY_VOICES]);
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
      Swaveform3[i].begin(waveformsArray[ch][POLY_VOICES]);
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

    // Update mixer gains (example: using pan[ch] and volume[ch] to calculate gain)
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
    notesArray[note + semitones[ch][0]] * pow(2, cents[ch][0] / 1200.0) * pow(2, random(-2, 3) / 1200.0));
  Swaveform2[notePlaying[ch]].frequency(
    notesArray[note + semitones[ch][1]] * pow(2, cents[ch][1] / 1200.0) * pow(2, random(-2, 3) / 1200.0));
  Swaveform3[notePlaying[ch]].frequency(
    notesArray[note + semitones[ch][2]] * pow(2, cents[ch][2] / 1200.0) * pow(2, random(-2, 3) / 1200.0));
  Senvelope1[notePlaying[ch]].noteOn();
  Senvelope2[notePlaying[ch]].noteOn();
  SenvelopeFilter1[notePlaying[ch]].noteOn();
}

void autoOffActiveNotes2() {
  if (pressedKeyCount[11] >= 1) return;

  unsigned long currentTime = millis();
  for (int ch = 0; ch < 2; ch++) {
    for (int i = 0; i < POLY_VOICES; i++) {
      if (noteArray[ch][i] != 0) {  // There is an active note on this voice.
        // Calculate total note duration including delay
        int delay_ms = mapf(SMP.param_settings[ch][DELAY], 0, maxfilterResolution, 0, maxParamVal[DELAY]);
        int attack_ms = mapf(SMP.param_settings[ch][ATTACK], 0, maxfilterResolution, 0, maxParamVal[ATTACK]);
        int hold_ms = mapf(SMP.param_settings[ch][HOLD], 0, maxfilterResolution, 0, maxParamVal[HOLD]);
        int decay_ms = mapf(SMP.param_settings[ch][DECAY], 0, maxfilterResolution, 0, maxParamVal[DECAY]);
        int release_ms = mapf(SMP.param_settings[ch][RELEASE], 0, maxfilterResolution, 0, maxParamVal[RELEASE]);

        int noteDuration = delay_ms + attack_ms + hold_ms + decay_ms + release_ms;

        if (currentTime - voiceStartTime[ch][i] >= noteDuration) {
          stopSound(noteArray[ch][i], ch);  // Properly ends envelope
        }
      }
    }
  }
}


// This function checks each voice and auto-offs active notes after AUTO_OFF_TIME.
void autoOffActiveNotes() {
  //Serial.println(persistentNoteOn[11]);
if (pressedKeyCount[11]>=1) return;
  unsigned long currentTime = millis();
  for (int ch = 0; ch < 2; ch++) {
    for (int i = 0; i < POLY_VOICES; i++) {
      if (noteArray[ch][i] != 0) {  // There is an active note on this voice.
        if (currentTime - voiceStartTime[ch][i] >= AUTO_OFF_TIME) {
          stopSound(noteArray[ch][i], ch);  // This will call noteOff on all envelopes for that note.
        }
      }
    }
  }
}

// Example stopSound() function (assuming noteArray[] holds the note numbers for active voices)
void stopSound(int note, int ch) {
  for (int num = 0; num < POLY_VOICES; num++) {
    if (noteArray[ch][num] == note) {
      Senvelope1[num].noteOff();
      Senvelope2[num].noteOff();
      SenvelopeFilter1[num].noteOff();
      noteArray[ch][num] = 0;
    }
  }
}





// Helper function to process filter adjustments
float processSynthAdjustment(SynthTypes synthType, int index, int encoder) {
  SMP.synth_settings[index][synthType] = currentMode->pos[encoder];
  Serial.print(":::::");
  Serial.println(SMP.synth_settings[index][synthType]);
  
  int mappedValue;
  
    const int maxIndex = 64;
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

// Update filter values
void updateSynthValue(SynthTypes synthType, int index, float value) {
  // Common function to update all filter types
  Serial.print(synthType);
  Serial.print(" S==>: ");
  Serial.println(value);
  switch (synthType) {
    case INSTRUMENT:
      break;
    case PARAM1:
      break;

    case PARAM2:
      break;

    default: return;
  }  
}



void switchSynthVoice(int typeValue, int ch, int p1,int p2, int p3, int p4, int p5, int p6) {
    switch(typeValue) {
        case 1: bass_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 2: keys_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 3: chiptune_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 4: pad_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 5: wow_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 6: organ_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 7: flute_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 8: lead_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 9: arp_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 10: brass_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        default:
            // Optionally, handle any values of typeValue that are not supported.
            break;
    }
}