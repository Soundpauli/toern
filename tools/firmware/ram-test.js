// GPL-3.0 — standalone diagnostic firmware, shared USB flasher and live readback.
import { SetupConnection } from './setup-serial.js';
export const RAM_BYTES = 16 * 1024 * 1024;
export const RAM_STAGES = 26;
const states = new Set(['idle', 'running', 'pass', 'fail', 'missing', 'aborted']);
export function validateRamStatus(s) {
  if (!s?.ok || s.protocol !== 1 || s.device !== 'toern-psram-test' || !states.has(s.state)) throw new Error('This is not the RAM-test firmware. Select RAM test above and upload it first.');
  const uint = (v, max = 0xFFFFFFFF) => Number.isInteger(v) && v >= 0 && v <= max;
  if (s.targetBytes !== RAM_BYTES || s.stages !== RAM_STAGES || !uint(s.run) || !uint(s.stage, RAM_STAGES) || !uint(s.progress, 100) || !uint(s.detectedBytes) || !uint(s.testedBytes, RAM_BYTES) || !uint(s.elapsedMs) || !uint(s.offset, RAM_BYTES - 1) || !uint(s.expected) || !uint(s.actual)) throw new Error('The RAM test returned an incomplete result. Full 16 MB has not been verified.');
  if (s.state === 'pass' && (s.run < 1 || s.detectedBytes !== RAM_BYTES || s.testedBytes !== RAM_BYTES || s.stage !== RAM_STAGES || s.progress !== 100)) throw new Error('Incomplete RAM test: a full 16 MB pass could not be verified.');
  return s;
}
export function ramResultText(s) {
  if (!s) return 'Flash the RAM-test firmware, then connect to run the test.';
  if (s.state === 'pass') return 'PASS — all 16 MB passed all 26 test stages.';
  if (s.state === 'missing' || (s.state === 'idle' && s.detectedBytes !== RAM_BYTES)) return `${s.detectedBytes / 1048576} of 16 MB detected — the full 16 MB test cannot pass. Check the PSRAM chips and their solder joints.`;
  if (s.state === 'fail') return `FAIL — RAM error at 0x${(0x70000000 + s.offset).toString(16).toUpperCase()}. Expected 0x${s.expected.toString(16).padStart(8,'0').toUpperCase()}, read 0x${s.actual.toString(16).padStart(8,'0').toUpperCase()}.`;
  if (s.state === 'running') return `Testing 16 MB · ${s.progress}% · stage ${Math.min(s.stage + 1, RAM_STAGES)} of ${RAM_STAGES}`;
  if (s.state === 'aborted' || s.state === 'interrupted') return 'Test incomplete — the full 16 MB has not been verified. Reconnect or start again.';
  return `${s.detectedBytes / 1048576} MB detected. Ready to test the full 16 MB.`;
}
const el = (tag,text,className) => { const node=document.createElement(tag); if(text)node.textContent=text;if(className)node.className=className;return node; };
export function mountRamTest(root, { beforeConnect, selectTest, restoreFirmware } = {}) {
  const connection = new SetupConnection(undefined, { prefix: 'RAMTEST', replyFilter: s => s.reply === true, timeoutMessage: 'No RAM-test reply. Upload the RAM-test firmware first, wait for it to restart, then reconnect.' });
  let connected = false, operation = false, running = false, flashing = false, status = null, message = '', minimumRun = 0;
  let timer, renderKey, resultNode, progressNode, timingNode, messageNode;
  const content = el('div'); root.append(content);
  function accept(raw) {
    try {
      const next = validateRamStatus(raw);
      if (next.run < minimumRun) return;
      status = next; running = next.state === 'running'; message = ''; render();
    } catch (error) { status = {state:'interrupted'}; running = false; message = error.message; render(); }
  }
  connection.onMessage = accept;
  connection.onDisconnect = () => {
    connected = false; running = false; clearInterval(timer);
    if (status?.state === 'running') status = {state:'interrupted'};
    message = 'USB disconnected. Reconnect to read the diagnostic result.'; render();
  };
  async function action(fn) {
    operation = true; message = ''; render();
    try { await fn(); } catch (error) {
      message = error.name === 'NotFoundError' ? 'No device selected. Connect when you are ready.' : error.message;
      if (running) status = {state:'interrupted'};
      running = false;
    } finally { operation = false; render(); }
  }
  async function connect() {
    await action(async () => {
      await beforeConnect?.();
      try {
        minimumRun = 0; await connection.connect();
        const response = validateRamStatus(await connection.request('STATUS'));
        connected = true; accept(response);
        clearInterval(timer);
        timer = setInterval(async () => {
          if (!connected || operation || connection.pending) return;
          try { accept(await connection.request('STATUS')); }
          catch (error) {
            if (running) status = {state:'interrupted'};
            running = false; message = error.message; clearInterval(timer); render();
          }
        }, 1200);
      } catch (error) { connected = false; await connection.close(); throw error; }
    });
  }
  async function start() {
    await action(async () => {
      minimumRun = (status?.run || 0) + 1; status = null; running = true; render();
      await connection.request('START');
      // Broadcasts can arrive while START is in flight. Only the newer run may
      // update the result, so an old PASS cannot make a retest pass instantly.
    });
  }
  async function stop() { await action(async () => { accept(await connection.request('ABORT')); }); }
  function button(text,fn,disabled=false,secondary=false) {
    const b=el('button',text,secondary?'teensy-btn setup-secondary':'teensy-btn setup-primary');b.type='button';b.disabled=disabled;b.addEventListener('click',fn);return b;
  }
  function updateLive() {
    const text = running && !status ? 'Starting the full 16 MB test…' : ramResultText(status);
    if (resultNode.textContent !== text) resultNode.textContent = text;
    progressNode.value = status?.progress || 0;
    timingNode.hidden = !status?.elapsedMs;
    const timing = status?.elapsedMs ? `${(status.elapsedMs/1000).toFixed(1)} seconds · ${status.detectedBytes/1048576} MB detected` : '';
    if (timingNode.textContent !== timing) timingNode.textContent = timing;
    messageNode.hidden = !message;
    if (messageNode.textContent !== message) messageNode.textContent = message;
  }
  function render() {
    // Progress packets must not replace focused controls every 250 ms.
    const key = `${connected}/${operation}/${running}/${flashing}/${status?.state}`;
    if (key === renderKey && resultNode) { updateLive(); return; }
    renderKey = key;
    content.replaceChildren(); root.setAttribute('aria-busy',String(operation||running||flashing));
    content.append(el('p','Check every address in the full 16 MB of external PSRAM, including both halves of memory.'));
    content.append(el('p','This temporarily replaces the instrument firmware. EEPROM settings and SD files are not changed. After testing, upload your usual instrument firmware again.','setup-note'));
    const instructions=el('ol');for(const text of ['Select RAM test. For factory firmware, press the Teensy programming button and choose the Teensy bootloader above; then upload.','After the device restarts, connect here and start the full test. Keep USB connected.','Read the result, then restore your usual firmware.'])instructions.append(el('li',text));content.append(instructions);
    const result = el('p',running && !status ? 'Starting the full 16 MB test…' : ramResultText(status),'ram-result');result.setAttribute('role','status');result.setAttribute('aria-live','polite');
    resultNode = result;
    result.classList.toggle('ram-pass',status?.state==='pass');result.classList.toggle('ram-fail',['fail','missing','aborted','interrupted'].includes(status?.state) || (status?.state==='idle' && status.detectedBytes!==RAM_BYTES));content.append(result);
    const progress=document.createElement('progress');progress.max=100;progress.value=status?.progress||0;progress.setAttribute('aria-label','16 MB RAM test progress');content.append(progress);progressNode=progress;
    timingNode=el('p','','setup-note');content.append(timingNode);
    if(flashing)content.append(el('p','Firmware upload is running. Connect here after TŒRN restarts.','setup-note'));
    messageNode=el('p','','setup-status');messageNode.setAttribute('role','alert');content.append(messageNode);
    const buttons=el('div','','setup-actions');
    buttons.append(button('Select RAM test',selectTest,operation||running||flashing,true));
    if(!connected)buttons.append(button('Connect RAM tester',connect,operation||flashing));
    else buttons.append(running?button('Stop test',stop,operation||flashing,true):button(status?.state==='pass'?'Test again':'Start full 16 MB test',start,operation||flashing));
    content.append(buttons);
    content.append(button('Restore instrument firmware',async()=>{await api.disconnect();restoreFirmware?.();},operation||running||flashing,true));
    const note=el('p','Only a completed, error-free test of all 16 MB is a PASS. Missing memory or an interrupted test is never a pass.','setup-note');content.append(note);
    const method=el('details','','setup-defaults');method.append(el('summary','How is the RAM checked?'));method.append(el('p','All memory is written and read across 26 stages: March C−, alternating bits, address checks and pseudorandom patterns. CPU cache is cleared between checks so reads reach the physical RAM chips. Testing uses the instrument’s 816 MHz CPU setting. A failure shows its address and the expected and actual values.'));content.append(method);
    updateLive();
  }
  const api = {
    get busy(){return operation||running;},
    async disconnect(){clearInterval(timer);connected=false;await connection.close();render();},
    setFlashing(value){flashing=value;render();},
    afterFlash(isTest){if(isTest){status=null;minimumRun=0;message='RAM-test firmware uploaded. Wait for the restart, then connect here.';}render();},
  };
  render(); return api;
}
