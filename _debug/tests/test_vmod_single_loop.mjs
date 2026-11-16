/** VMOD + SINGLE uses LOOP length for the focused voice (mirrors refreshVoiceLens). */
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const source = readFileSync(new URL('../../tools/sim/src/firmware/device.js', import.meta.url), 'utf8');
assert.match(source, /SINGLE \+ VMOD \+ LOOP: focused voice cycles at LOOP length/);
assert.match(source, /s\.GLOB\.singleMode \|\| s\.mode === "single"/);

function applyVoiceLoopForce(len, longest, s) {
  if (s.voiceMode && s.loopLength > 0 && (s.GLOB.singleMode || s.mode === 'single')) {
    const ch = s.GLOB.currentChannel;
    if (ch >= 1 && ch <= 15) {
      const forced = Math.max(1, Math.min(s.loopLength, Math.min(8, 16)));
      len[ch] = forced;
      if (forced > longest) longest = forced;
    }
  }
  return longest;
}

const len = Array(16).fill(0);
let longest = 1;
const s = { voiceMode: true, loopLength: 4, mode: 'draw', GLOB: { singleMode: false, currentChannel: 1 } };
longest = applyVoiceLoopForce(len, longest, s);
assert.equal(len[1], 0);
assert.equal(longest, 1);

s.GLOB.singleMode = true;
s.mode = 'single';
longest = applyVoiceLoopForce(len, longest, s);
assert.equal(len[1], 4);
assert.equal(longest, 4);

s.loopLength = 0;
len[1] = 0;
longest = applyVoiceLoopForce(len, 1, s);
assert.equal(len[1], 0);

const firmware = readFileSync(new URL('../../platformio/toern.ino', import.meta.url), 'utf8');
assert.match(firmware, /SINGLE \+ VMOD \+ LOOP: the focused voice cycles at LOOP length/);
assert.match(firmware, /GLOB\.singleMode \|\| currentMode == &singleMode/);

console.log('PASS: VMOD SINGLE follows LOOP length for the focused voice');
