// Grid folding mirrors src/toern_midi_pitch.h.
export function foldRow(midiNote, channel, polyOctave = 4) {
  const first = channel === 13 ? 2 : channel === 14 ? 3 : 1;
  const root = channel <= 8 ? 60 + first - (channel + 1)
    : channel === 11 ? 60 + 12 * (polyOctave - 4) + first - (channel + 1)
    : Math.max(36, Math.min(62, 48 + first - (channel + 1))) + 12;
  let row = first + midiNote - root;
  while (row < first) row += 12;
  while (row > 15) row -= 12;
  return { pitch: midiNote, row };
}


// Neutral grid register, matching toern_midi_pitch::gridMidi(channel, row, 4, 0).
export function neutralGridMidi(channel, row) {
  if (channel >= 1 && channel <= 8) return 60 + row - (channel + 1);
  if (channel === 11) return 60 + row - (channel + 1);
  if (channel === 13 || channel === 14) return Math.max(36, Math.min(62, 48 + row - (channel + 1))) + 12;
  return 48 + row - 1;
}
