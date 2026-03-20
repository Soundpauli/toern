import re

with open("/Users/jank/git/toern/toern_synths.ino", "r") as f:
    code = f.read()

old_updateVals = """    // Set waveform for oscillator 1 (waveform1)
    if (waveforms[ch][0] == 0) {
      Swaveform1[i]->begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][0] == 1) {
      Swaveform1[i]->begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][0] == 2) {
      Swaveform1[i]->begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][0] == 3) {
      Swaveform1[i]->begin(waveformsArray[ch][3]);
    } else if (waveforms[ch][0] == 4) {
      Swaveform1[i]->begin(waveformsArray[ch][4]);
    }
    Swaveform1[i]->amplitude(1.0);
    Swaveform1[i]->pulseWidth(pulseWidth[ch][0]);

    // Set waveform for oscillator 2 (waveform2)
    if (waveforms[ch][1] == 0) {
      Swaveform2[i]->begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][1] == 1) {
      Swaveform2[i]->begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][1] == 2) {
      Swaveform2[i]->begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][1] == 3) {
      Swaveform2[i]->begin(waveformsArray[ch][3]);
    } else if (waveforms[ch][1] == 4) {
      Swaveform2[i]->begin(waveformsArray[ch][4]);
    }
    Swaveform2[i]->amplitude(1.0);
    Swaveform2[i]->pulseWidth(pulseWidth[ch][1]);

    // Set waveform for oscillator 3 (waveform3)
    if (waveforms[ch][2] == 0) {
      Swaveform3[i]->begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][2] == 1) {
      Swaveform3[i]->begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][2] == 2) {
      Swaveform3[i]->begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][2] == 3) {
      Swaveform3[i]->begin(waveformsArray[ch][3]);
    } else if (waveforms[ch][2] == 4) {
      Swaveform3[i]->begin(waveformsArray[ch][4]);
    }

    Swaveform3[i]->amplitude(1.0);
    Swaveform3[i]->pulseWidth(pulseWidth[ch][2]);"""

new_updateVals = """    // Set waveform for oscillator 1 (waveform1)
    uint8_t type1 = waveformsArray[ch][waveforms[ch][0]];
    if (currentWaveforms[ch][i][0] != type1) {
      Swaveform1[i]->begin(type1);
      currentWaveforms[ch][i][0] = type1;
    }
    Swaveform1[i]->amplitude(1.0);
    Swaveform1[i]->pulseWidth(pulseWidth[ch][0]);

    // Set waveform for oscillator 2 (waveform2)
    uint8_t type2 = waveformsArray[ch][waveforms[ch][1]];
    if (currentWaveforms[ch][i][1] != type2) {
      Swaveform2[i]->begin(type2);
      currentWaveforms[ch][i][1] = type2;
    }
    Swaveform2[i]->amplitude(1.0);
    Swaveform2[i]->pulseWidth(pulseWidth[ch][1]);

    // Set waveform for oscillator 3 (waveform3)
    uint8_t type3 = waveformsArray[ch][waveforms[ch][2]];
    if (currentWaveforms[ch][i][2] != type3) {
      Swaveform3[i]->begin(type3);
      currentWaveforms[ch][i][2] = type3;
    }
    Swaveform3[i]->amplitude(1.0);
    Swaveform3[i]->pulseWidth(pulseWidth[ch][2]);"""

if old_updateVals in code:
    code = code.replace(old_updateVals, new_updateVals)
else:
    print("WARNING: Could not find old_updateVals!")

with open("/Users/jank/git/toern/toern_synths.ino", "w") as f:
    f.write(code)

print("Done patching updateVals.")
