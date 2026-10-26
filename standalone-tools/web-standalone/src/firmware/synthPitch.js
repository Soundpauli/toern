// Pitch rules from toern.ino playSynthInternal and toern_synths.ino.
const notes = [16.35,17.32,18.35,19.45,20.60,21.83,23.12,24.50,25.96,27.50,29.14,30.87,32.70,34.65,36.71,38.89,41.20,43.65,46.25,49.00,51.91,55.00,58.27,61.74,65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,130.81,138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,261.63,277.18,293.66,311.13,329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,523.25,554.37,587.33,622.25,659.25,698.46,739.99,783.99,830.61,880.00,932.33,987.77,1046.50,1108.73,1174.66,1244.51,1318.51,1396.91,1479.98,1567.98,1661.22,1760.00,1864.66,1975.53,2093.00,2217.46,2349.32,2489.02,2637.02,2793.83,2959.96,3135.96,3322.44,3520.00,3729.31,3951.07,4186.01,4434.92,4698.63,4978.03,5274.04,5587.65,5919.91,6271.93,6644.88,7040.00,7458.62,7902.13]; // notes.h notesArray
const presetCents = [7,0,2,15,0,-1,12,5,-7,0,0,-6,0];
const tableFrequency = index => index >= 0 && index < notes.length ? notes[index] : 16.35 * 2 ** (index / 12);
export const midiFrequency = midi => 440 * 2 ** ((midi - 69) / 12);
const roundAway = value => Math.sign(value) * Math.floor(Math.abs(value) + .5);
export function synthFrequencies(ch, row, wav = {}) {
  const absolute = Number.isFinite(wav.midiPitch) && wav.midiPitch >= 0 && wav.midiPitch <= 127;
  if (ch === 11) {
    const register = Math.trunc(1 + (wav.cent ?? 5) * 7 / 32);
    const midi = absolute ? wav.midiPitch + 12 * (register - 4) : 12 * register + row - 1 + 12;
    const semi = wav.semi ?? 16;
    const semitones = roundAway(-2 + semi * 4 / 32);
    const cents = (presetCents[wav.inst ?? 12] ?? 0) + (-50 + semi * 100 / 32);
    return [tableFrequency(midi - 12 + semitones) * 2 ** (cents / 1200)];
  }
  const midi = absolute ? wav.midiPitch : Math.min(62, Math.max(36, row - ch + 47)) + 12;
  const cent = wav.cent ?? 16;
  const frequency = absolute ? midiFrequency(midi) : tableFrequency(midi - 12);
  const base = frequency * 2 ** ((cent * 48 / 32 - 24) / 12);
  // OCTV is a signed integer register for oscillator 2 only. C++ casts
  // truncate towards zero; DTNE is ±1 semitone on that oscillator only.
  const octave = Math.trunc((wav.octaveRaw ?? 16) * 6 / 32 - 3);
  return [base, base * 2 ** (octave + (wav.detune ?? 0) / 12)];
}
