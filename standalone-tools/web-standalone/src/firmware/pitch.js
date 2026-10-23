// Grid folding mirrors src/toern_midi_pitch.h.
export function foldRow(midiNote, channel, polyOctave = 4) {
  const first = channel === 13 ? 2 : channel === 14 ? 3 : 1;
  const root = channel <= 8 ? 60 + first - (channel + 1)
    : channel === 11 ? 12 * polyOctave + first - 1 + 12
    : Math.max(36, first - channel + 47) + 12;
  let row = first + midiNote - root;
  while (row < first) row += 12;
  while (row > 15) row -= 12;
  return { pitch: midiNote, row };
}

