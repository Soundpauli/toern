#pragma once
#include <stdint.h>
#include <math.h>

namespace toern_midi_pitch {
// The note table starts at C0 (16.35 Hz), MIDI note 12. MIDI A4 is 69.
constexpr int kTableMidiOffset = 12;
constexpr int kSampleRootMidi = 60;

inline int sampleNote(int root, int midi) {
  return root + midi - kSampleRootMidi;
}

inline int polyNote(int midi, int transpose, int polyOctave = 4) {
  // Ch11's octave control is an absolute grid register. Treat register 4
  // as neutral for MIDI, retaining preset/user octave and transpose tuning.
  return midi - kTableMidiOffset + 12 * (polyOctave - 4) + transpose;
}

inline float midiFrequency(int midi) {
  return 440.0f * powf(2.0f, (float)(midi - 69) / 12.0f);
}

// Invert each existing grid voice's local pitch calculation. This does not
// change how native grid notes sound, or include oscillator fine tuning.
inline int gridMidi(int channel, int row, int polyOctave, int polyTranspose) {
  if (channel >= 1 && channel <= 8) return kSampleRootMidi + row - (channel + 1);
  if (channel == 11) return 12 * polyOctave + polyTranspose + row - 1 + kTableMidiOffset;
  if (channel == 13 || channel == 14) {
    int index = row - channel + 47;
    if (index < 36) index = 36;
    if (index > 62) index = 62;
    return index + kTableMidiOffset;
  }
  return 48 + row - 1;
}

inline uint8_t gridRow(int channel, int midi, int polyOctave) {
  // The mono synths clamp lower table indices; avoid those duplicate-C rows
  // when folding so the incoming pitch class is retained.
  const int firstRow = channel == 13 ? 2 : (channel == 14 ? 3 : 1);
  // Transpose is patch tuning applied after the incoming note is mapped,
  // just as mono-synth CENT/OCTV and sample tuning are applied afterward.
  int row = firstRow + midi - gridMidi(channel, firstRow, polyOctave, 0);
  while (row < firstRow) row += 12;
  while (row > 15) row -= 12;
  return (uint8_t)row;
}
}
