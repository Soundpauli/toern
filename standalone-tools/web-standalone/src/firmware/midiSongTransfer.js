import { encodePatternRam } from "./patternFile.js";

/** Prepare every image before sending; leave A selected after the last ACK. */
export async function transferMidiSubpatterns(client, banks, bpm, onProgress) {
  if (!Array.isArray(banks) || banks.length < 2 || banks.length > 5)
    throw new Error("A song transfer needs 2–5 subpatterns.");
  const images = Array.from({ length: 5 }, (_, slot) => {
    const flat = Array(256 * 16).fill(null);
    for (const cell of banks[slot] || []) {
      if (!Number.isInteger(cell.step) || cell.step < 1 || cell.step > 256 ||
          !Number.isInteger(cell.row) || cell.row < 1 || cell.row > 15)
        throw new Error("Invalid MIDI song cell.");
      flat[(cell.step - 1) * 16 + cell.row - 1] = cell;
    }
    return encodePatternRam(flat, bpm);
  });
  const completed = [];
  try {
    // Clear unused trailing letters too, so an older song cannot reappear there.
    for (const slot of [1, 2, 3, 4, 0]) {
      const letter = String.fromCharCode(65 + slot);
      await client.putSubpatternRam(letter, images[slot], (done, total) => {
        onProgress?.(letter, (completed.length + done / total) / 5);
      });
      completed.push(letter);
    }
  } catch (error) {
    error.completedSubpatterns = completed;
    throw error;
  }
  return completed;
}
