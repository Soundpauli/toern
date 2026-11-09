// GPL-3.0
import { DEFAULT_ANSWERS, visibleQuestions, validateAnswers, migrateAnswers, makeSettings, summaryRows } from './setup-settings.js';
import { SetupConnection } from './setup-serial.js';
import './setup-wizard.css';

const STORAGE_KEY = 'toern-guided-setup-v1';
const el = (tag, text, className) => { const node = document.createElement(tag); if (text) node.textContent = text; if (className) node.className = className; return node; };
export function mountSetupWizard(root, { beforeConnect, installFirmware } = {}) {
  let answers = { ...DEFAULT_ANSWERS };
  try { const saved = JSON.parse(sessionStorage.getItem(STORAGE_KEY)); if (saved) answers = migrateAnswers(saved); } catch {}
  let page = 'intro', index = 0, busy = false, flashing = false, connected = false, message = '', error = false, savedResult = null;
  const connection = new SetupConnection();
  connection.onDisconnect = () => { connected = false; message = 'TŒRN disconnected. Your answers are kept—reconnect when it is ready.'; error = true; render(); };
  const status = el('p', '', 'setup-status'); status.setAttribute('role', 'status'); status.setAttribute('aria-live', 'polite');
  const body = el('div'); root.append(body, status);
  function remember() { try { sessionStorage.setItem(STORAGE_KEY, JSON.stringify(answers)); } catch {} }
  function button(label, action, secondary = false) {
    const b = el('button', label, secondary ? 'teensy-btn setup-secondary' : 'teensy-btn setup-primary'); b.type = 'button'; b.disabled = busy || flashing; b.addEventListener('click', action); return b;
  }
  function move(nextPage, nextIndex = index) {
    page = nextPage; index = nextIndex; message = ''; error = false; render();
    const heading = body.querySelector('h3');
    heading?.focus({ preventScroll: true });
    heading?.scrollIntoView({ behavior: 'smooth', block: 'start' });
  }
  async function operation(action) {
    busy = true; error = false; message = ''; render();
    try { await action(); } catch (e) { error = true; message = e.name === 'NotFoundError' ? 'No device selected. Your answers are kept; connect when you are ready.' : e.message; }
    finally { busy = false; render(); }
  }
  async function connect() {
    await operation(async () => {
      await beforeConnect?.();
      message = 'Connecting to your running TŒRN…'; render();
      try {
        await connection.connect(); const state = await connection.get(); connected = true;
        message = `Connected · ${state.firmware}. Ready to apply your setup.`;
      } catch (e) { connected = false; await connection.close(); throw e; }
    });
  }
  async function apply() {
    await operation(async () => {
      message = 'Stopping playback, saving and checking your settings…'; render();
      savedResult = await connection.apply(current => makeSettings(answers, current));
      page = 'done'; message = ''; connected = false; await connection.close();
    });
  }
  function render() {
    body.replaceChildren();
    status.textContent = flashing ? 'Firmware upload is running. Your setup answers are kept.' : message;
    status.classList.toggle('is-error', error);
    root.setAttribute('aria-busy', String(busy || flashing));
    const heading = text => { const h = el('h3', text); h.tabIndex = -1; body.append(h); };
    if (page === 'intro') {
      body.append(el('p', '// Guided setup', 'setup-eyebrow'));
      heading('Your instrument. Your way.');
      body.append(el('p', 'A few questions about how you like to play. We’ll turn your answers into a complete, ready-to-use device setup.'));
      body.append(el('p', 'You can choose your preferences before connecting. Saving needs the latest Beta or BETA-MODULR firmware from above.', 'setup-note'));
      body.append(button('Set up my TŒRN', () => move('questions', 0)));
      return;
    }
    if (page === 'questions') {
      const questions = visibleQuestions(answers), q = questions[index];
      body.append(el('p', `Question ${index + 1} of ${questions.length}`, 'setup-eyebrow'));
      const progress = document.createElement('progress'); progress.max = questions.length; progress.value = index + 1; progress.setAttribute('aria-label', 'Setup progress'); body.append(progress);
      heading(q.title); body.append(el('p', q.description));
      const group = el('fieldset', '', 'setup-options');
      const legend = el('legend', q.title, 'setup-sr-only'); group.append(legend);
      q.options.forEach(o => {
        const label = el('label', '', 'setup-option');
        const radio = document.createElement('input'); radio.type = 'radio'; radio.name = `setup-${q.id}`; radio.value = o.value; radio.checked = answers[q.id] === o.value; radio.disabled = busy || flashing;
        const copy = el('span'); copy.append(el('strong', o.title), el('span', o.description, 'setup-option-description'));
        radio.addEventListener('change', () => { answers[q.id] = o.value; remember(); group.querySelectorAll('label').forEach(l => l.classList.toggle('is-selected', l.querySelector('input').checked)); });
        label.classList.toggle('is-selected', radio.checked); label.append(radio, copy); group.append(label);
      });
      body.append(group);
      const nav = el('div', '', 'setup-actions');
      nav.append(button('Back', () => index ? move('questions', index - 1) : move('intro'), true));
      nav.append(button(index === questions.length - 1 ? 'Review my setup' : 'Next', () => {
        const next = visibleQuestions(answers);
        index === next.length - 1 ? move('review') : move('questions', index + 1);
      })); body.append(nav);
      return;
    }
    if (page === 'done') {
      body.append(el('p', '// Setup complete', 'setup-eyebrow'));
      heading('Ready to make some noise.');
      body.append(el('p', 'Your preferences are saved to TŒRN and active now. You can unplug USB and start playing.'));
      body.append(el('p', savedResult.backup ? 'EEPROM verified. A settings backup was also saved on the SD card.' : 'EEPROM verified. The SD backup could not be written; check that your SD card is inserted and writable. The device will retry its backup.', 'setup-note'));
      const nav = el('div', '', 'setup-actions');
      nav.append(button('Adjust my setup', () => move('questions', 0), true));
      const handbook = el('a', 'Make your first beat ↗', 'teensy-btn setup-primary'); handbook.href = 'https://handbook.toern.live/first-beat.html'; nav.append(handbook); body.append(nav);
      return;
    }
    body.append(el('p', '// Your setup', 'setup-eyebrow')); heading('Looks like you?');
    const dl = el('dl', '', 'setup-summary');
    for (const [question, value] of summaryRows(answers)) { const row = el('div'); row.append(el('dt', question), el('dd', value)); dl.append(row); }
    body.append(dl);
    const defaults = el('details', '', 'setup-defaults'); defaults.append(el('summary', 'What else will be set?'));
    defaults.append(el('p', 'A moderate main volume, balanced preview level, unity mixer gains, recording high-pass on and input monitoring off. Brightness uses the standard level. Child lock, visual effects, stereo splitting, analog clock output and the extra LED strip start off. MIDI notes are sent when TŒRN is not the clock leader; leaders send clock, with start/stop determined by your Transport SEND choice. Existing recorded notes are kept when recording.'));
    body.append(defaults);
    body.append(el('p', 'Apply replaces your device preferences with this setup and sensible defaults. Your kit, samples, patterns and physical display layout stay in place.', 'setup-note'));
    const nav = el('div', '', 'setup-actions');
    nav.append(button('Back to answers', () => move('questions', visibleQuestions(answers).length - 1), true));
    nav.append(connected ? button(busy ? 'Applying…' : 'Apply to my TŒRN', apply) : button(busy ? 'Connecting…' : 'Connect TŒRN', connect)); body.append(nav);
    body.append(el('p', connected ? 'Applying stops playback. Finish any recording first.' : 'Connect the running device by USB, then select its Teensy serial port. You can use setup without flashing again if the current firmware supports it.', 'setup-note'));
    body.append(button('Install compatible firmware', async () => { if (busy || flashing) return; await connection.close(); connected = false; installFirmware?.(); render(); }, true));
  }
  render();
  return {
    get busy() { return busy; },
    async disconnect() { connected = false; await connection.close(); render(); },
    setFlashing(value) { flashing = value; render(); },
    afterFlash() { flashing = false; connected = false; if (page === 'review') message = 'Firmware uploaded. Wait for TŒRN to finish starting, then reconnect to apply your setup.'; render(); },
  };
}
