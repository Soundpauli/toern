import assert from "node:assert/strict";
import {
  isLaunchpadName,
  noteToCell,
  cellToNote,
  volumeForRow,
  volumeRowFor,
} from "../../tools/sim/src/launchpad.js";

assert.equal(isLaunchpadName("Launchpad Mini MK3 LPMiniMK3 MIDI"), true);
assert.equal(isLaunchpadName("LPX MIDI Out"), true);
assert.equal(isLaunchpadName("Launchpad X LPX MIDI"), true);
assert.equal(isLaunchpadName("MIDIIN2 (LPMiniMK3 MIDI)"), true);
assert.equal(isLaunchpadName("Arduino Leonardo"), false);

assert.deepEqual(noteToCell(11), { row: 1, col: 1, voice: 1 });
assert.deepEqual(noteToCell(18), { row: 1, col: 8, voice: 8 });
assert.deepEqual(noteToCell(81), { row: 8, col: 1, voice: 1 });
assert.deepEqual(noteToCell(88), { row: 8, col: 8, voice: 8 });
assert.equal(noteToCell(19), null);
assert.equal(noteToCell(10), null);
assert.equal(noteToCell(99), null);

assert.equal(cellToNote(1, 1), 11);
assert.equal(cellToNote(5, 3), 53);
assert.equal(cellToNote(8, 8), 88);
assert.equal(cellToNote(0, 1), null);

assert.equal(volumeForRow(1), null);
assert.deepEqual(
  [2, 3, 4, 5, 6].map(volumeForRow),
  [0, 4, 8, 12, 16],
);
assert.equal(volumeForRow(7), null);

assert.equal(volumeRowFor(0), 0);
assert.equal(volumeRowFor(4), 3);
assert.equal(volumeRowFor(8), 4);
assert.equal(volumeRowFor(12), 5);
assert.equal(volumeRowFor(16), 6);
assert.equal(volumeRowFor(15), 6);
assert.equal(volumeRowFor(1), 2);

console.log("test_sim_launchpad: ok");
