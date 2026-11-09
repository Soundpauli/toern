// GPL-3.0 — configuration decisions belong to the firmware web tool.
export const DEFAULT_ANSWERS = Object.freeze({
  speaker: 'yes', controls: 'easy', ctrl: 'vol', subpatterns: 'off',
  pageMode: 'off', flow: 'on', cursor: 'normal', input: 'mic', sync: 'solo',
  transportSend: 'yes', pulse: '4', keys: 'no', fold: 'yes', preview: 'auto',
  usbPreview: 'no', screensaver: '1',
});
const option = (value, title, description) => ({ value, title, description });
export const QUESTIONS = [
  { id: 'speaker', title: 'Will you use the internal speaker?', description: 'Choose how you want to hear TŒRN. Headphones and line out remain available either way.', options: [option('yes', 'Yes, play anywhere', 'Enable the built-in speaker. Microphone monitoring stays off to avoid feedback.'), option('no', 'No, external audio only', 'Keep the speaker silent. Listen through headphones, a mixer or your audio interface.')] },
  { id: 'controls', title: 'Start easy, or show full note details?', description: 'Choose your note view. The following questions let you choose knob behavior and pattern banks independently.', options: [option('easy', 'Start easy', 'A simpler note view that is a good place to learn.'), option('full', 'Give me full control', 'The detailed note view for more involved editing. Choose the knob role and A–E banks separately below.')] },
  { id: 'ctrl', title: 'What should the CTRL knob control?', description: 'Choose what encoder 2 does while you are working on the grid.', options: [option('vol', 'VOL — voice volume', 'Adjust the selected voice’s volume directly.'), option('page', 'PAGE — page selection', 'Move between pages directly.')] },
  { id: 'subpatterns', title: 'Would you like A–E subpatterns?', description: 'Subpatterns give you five variations within one pattern file.', options: [option('off', 'OFF — keep one working bank', 'Start with the current pattern bank and simpler navigation.'), option('on', 'ON — use A–E banks', 'Switch between five variations, each with its own notes and settings.')] },
  { id: 'pageMode', title: 'How should your pages play?', description: 'Choose PMODE, the way the sequencer moves between pages.', options: [option('off', 'OFF — play the whole pattern', 'Play the pattern’s pages in sequence.'), option('on', 'ON — loop the selected page', 'Keep playing the selected page.'), option('next', 'NEXT — queue the next page', 'Choose a page during playback; switch at the page boundary.'), option('song', 'SONG — follow the arranger', 'Use your saved song arrangement. Set up the arrangement on the device before playing.')] },
  { id: 'flow', title: 'Should the display follow playback?', description: 'FLOW controls whether the editing view follows the playing page.', options: [option('on', 'ON — follow the music', 'Automatically show the page that is playing.'), option('off', 'OFF — keep my editing page', 'Stay on the page you chose while the sequence plays.')] },
  { id: 'cursor', title: 'How should the cursor identify voices?', description: 'Choose the CRSR display style.', options: [option('normal', 'NORM — standard cursor', 'Use the normal cursor display.'), option('channel', 'CHNR — show channel numbers', 'Show a channel-number overlay when you change the selected voice.')] },
  { id: 'input', title: 'What will you record?', description: 'Set up the recording input now, with sensible gain and no live input monitoring.', options: [option('mic', 'Sounds around me', 'Use the microphone. Hold the recording pad through its countdown to start capturing.'), option('line', 'Another instrument', 'Use line input for a synth, phone or mixer. Hold to arm recording; existing notes are kept.'), option('none', 'No recording for now', 'Disable the quick-record gesture so you can explore without accidentally recording.')] },
  { id: 'sync', title: 'Who should set the tempo?', description: 'Set the clock source. When TŒRN leads, you can choose whether it also sends start/stop.', options: [option('solo', 'TŒRN, on its own', 'Use the internal tempo. No MIDI clock or start/stop messages are sent.'), option('lead', 'TŒRN leads my other gear', 'Send MIDI clock. Connect MIDI out to the device you want to follow; choose start/stop behavior next.'), option('midi', 'My MIDI gear leads', 'Follow incoming MIDI clock and start/stop. Connect your clock source to MIDI in.'), option('pulse', 'An analog clock leads', 'Follow positive clock pulses at the clock input. Press Play on TŒRN to arm playback.')] },
  { id: 'transportSend', title: 'Should TŒRN send MIDI start and stop?', description: 'Transport SEND lets Play and Pause control your other gear. MIDI clock keeps running with either choice.', when: a => a.sync === 'lead', options: [option('yes', 'SEND — start and stop together', 'Send MIDI start/stop when you use TŒRN’s transport controls.'), option('no', 'OFF — send clock only', 'Let the other device use its own start/stop controls.')] },
  { id: 'pulse', title: 'How many pulses per quarter note?', description: 'Match your clock source. Four pulses means one pulse per sixteenth note.', when: a => a.sync === 'pulse', options: [1, 2, 4, 8, 12, 16, 24, 32].map(n => option(String(n), `${n} PPQN`, n === 4 ? 'A common starting point for a sixteenth-note clock.' : `Choose this if your clock source sends ${n} ${n === 1 ? 'pulse' : 'pulses'} per quarter note.`)) },
  { id: 'keys', title: 'Will you play TŒRN with MIDI keys?', description: 'You can use a keyboard even when TŒRN runs on its own tempo.', options: [option('no', 'No, use the device controls', 'Ignore incoming MIDI notes. Your clock and transport choices still work.'), option('yes', 'Yes, use a MIDI keyboard', 'The incoming MIDI channel selects the voice. Choose pitch folding next; occupied notes are preserved.')] },
  { id: 'fold', title: 'Should MIDI notes fold into the visible grid?', description: 'MIDI FOLD controls incoming pitches that fall outside the visible note range.', when: a => a.keys === 'yes', options: [option('yes', 'FOLD — keep pitches in the grid', 'Move out-of-range incoming notes by octaves into the visible grid.'), option('no', 'No folding — keep incoming pitches', 'Disable pitch folding so incoming MIDI keeps its original pitch.')] },
  { id: 'preview', title: 'How do you like to browse samples?', description: 'Choose when the sample browser plays an audition.', options: [option('auto', 'Let me hear as I browse', 'Audition samples automatically at a moderate preview volume.'), option('press', 'Only when I ask', 'Browse quietly. Press the preview knob when you want to hear a sample.')] },
  { id: 'usbPreview', title: 'Should sample previews play over USB?', description: 'USBP adds sample-browser auditions to the USB audio output.', options: [option('no', 'USBP OFF — keep USB previews silent', 'Leave sample auditions out of the computer’s USB audio feed.'), option('yes', 'USBP ON — hear previews on the computer', 'Include sample-browser auditions in USB audio.')] },
  { id: 'screensaver', title: 'When should the screensaver appear?', description: 'EYES starts the screensaver after the device has been idle for your chosen time.', options: [option('off', 'OFF — leave the display awake', 'Disable the idle screensaver.'), option('1', 'After 1 minute', 'Use the standard one-minute idle timeout.'), option('5', 'After 5 minutes', 'Keep the working display visible for longer.'), option('15', 'After 15 minutes', 'Wait a quarter of an hour before the screensaver starts.')] },
];
export function visibleQuestions(answers) { return QUESTIONS.filter(q => !q.when || q.when(answers)); }
export function validateAnswers(a) {
  for (const q of QUESTIONS) if (!q.options.some(o => o.value === a[q.id])) throw new Error(`Choose an answer for: ${q.title}`);
}
export function makeSettings(answers, current) {
  validateAnswers(answers);
  if (!(current instanceof Uint8Array) || current.length !== 60 || current[13] < 1 || current[13] > 4) throw new Error('The device settings format is not supported. Install the latest Beta firmware first.');
  // Complete current EEPROM schema, including migration markers. Preserve the
  // physical panel layout; kit identity and SP0 voices live outside this block.
  const b = new Uint8Array([
    1, 1, 2, 255, 255, 2, 0, 20, 1, 10,
    24, 1, 0, 4, 1, 30, 8, 60, 0, 0,
    0, 0, 0, 0, 1, 0, 64, 1, 0, 0,
    12, 0, 0, 5, 6, 0, 0, 1, 10, 10,
    10, 10, 0, 0, 8, 1, 0, 8, 0, 1,
    0xA5, 1, 0, 2, 0, 0xA5, 1, 0, 0xA5, 0,
  ]);
  b[13] = current[13];
  b[27] = answers.speaker === 'yes' ? 1 : 0;
  const full = answers.controls === 'full';
  b[11] = full ? 2 : 1;
  b[14] = answers.ctrl === 'page' ? 0 : 1;
  b[18] = answers.subpatterns === 'on' ? 1 : 0;
  b[3] = { off: 255, on: 1, song: 2, next: 3 }[answers.pageMode];
  b[8] = answers.flow === 'on' ? 1 : 255;
  b[19] = answers.cursor === 'channel' ? 1 : 0;
  b[0] = answers.input === 'line' ? 255 : 1;
  b[5] = answers.input === 'none' ? 0 : 2;
  b[1] = ['midi', 'pulse'].includes(answers.sync) ? 0 : 1;
  b[2] = answers.sync === 'lead' ? (answers.transportSend === 'yes' ? 2 : 255) : answers.sync === 'solo' ? 255 : 1;
  b[24] = answers.sync === 'lead' ? 0 : 1;
  b[52] = answers.sync === 'pulse' ? 1 : 0;
  b[53] = [1, 2, 4, 8, 12, 16, 24, 32].indexOf(Number(answers.pulse));
  b[28] = answers.keys === 'yes' ? 1 : 0;
  b[4] = b[28] ? 1 : 255;
  b[20] = answers.preview === 'press' ? 1 : 0;
  b[37] = answers.fold === 'yes' ? 1 : 0;
  b[57] = answers.usbPreview === 'yes' ? 1 : 0;
  b[49] = { off: 0, '1': 1, '5': 2, '15': 3 }[answers.screensaver];
  b[26] = 64; // Standard brightness is always used; no brightness question.
  return b;
}
// Keep in-progress answers when the web wizard gains fields. Old Easy/Full
// presets implied CTRL/subpatterns; preserve those choices during migration.
export function migrateAnswers(saved) {
  const a = { ...DEFAULT_ANSWERS, ...saved };
  if (saved && !Object.hasOwn(saved, 'ctrl')) a.ctrl = saved.controls === 'full' ? 'page' : 'vol';
  if (saved && !Object.hasOwn(saved, 'subpatterns')) a.subpatterns = saved.controls === 'full' ? 'on' : 'off';
  delete a.brightness;
  validateAnswers(a);
  return a;
}
export function summaryRows(a) {
  return visibleQuestions(a).map(q => [q.title, q.options.find(o => o.value === a[q.id]).title]);
}
export const toHex = b => Array.from(b, v => v.toString(16).padStart(2, '0')).join('').toUpperCase();
export function fromHex(hex) {
  if (!/^[0-9A-F]{120}$/i.test(hex)) throw new Error('The device returned incomplete settings. Reconnect and try again.');
  return Uint8Array.from(hex.match(/../g), v => parseInt(v, 16));
}
export function crc32(bytes) {
  let crc = 0xFFFFFFFF;
  for (const byte of bytes) { crc ^= byte; for (let i = 0; i < 8; ++i) crc = (crc >>> 1) ^ ((crc & 1) ? 0xEDB88320 : 0); }
  return ((~crc) >>> 0).toString(16).padStart(8, '0').toUpperCase();
}
