import { mountRamTest } from './ram-test.js';
import { mountSetupWizard } from './setup-wizard.js';
import {
  FirmwareFile,
  TeensyFlasher,
  SerialPortManager,
  TEENSY_DEVICE_FILTERS
} from './Teensy-Loader.js';

const FIRMWARE_URLS = {
  stable: new URL('./public/toern_firmware_3.0.ino.hex', import.meta.url).href,
  beta: new URL('./public/toern_firmware_beta.ino.hex', import.meta.url).href,
  'beta-modulr': new URL('./public/toern_firmware_beta_modulr.ino.hex', import.meta.url).href,
  ramtest: new URL('./public/toern_ram_test.ino.hex', import.meta.url).href
};
const FIRMWARE_NAMES = {
  stable: 'toern_firmware_3.0.ino.hex',
  beta: 'toern_firmware_beta.ino.hex',
  'beta-modulr': 'toern_firmware_beta_modulr.ino.hex',
  ramtest: 'toern_ram_test.ino.hex'
};
const BETA_VERSION_URL = new URL('./public/beta-version.json', import.meta.url).href;

let selectedDevice = null;
let firmwareData = null;
let firmwareName = null;
let firmwareLoadRequest = 0;
/** From #define VERSION in toern.ino (synced at loader build). */
let betaVersion = '';
let lastInstrumentFirmware = 'stable';
let ramTest;

function setLabelText(labelId, inputId, text) {
  const label = document.getElementById(labelId);
  const input = document.getElementById(inputId);
  if (!label || !input) return;
  label.replaceChildren(input, document.createTextNode(` ${text}`));
}

async function applyBetaVersionLabels() {
  try {
    const res = await fetch(BETA_VERSION_URL);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    betaVersion = typeof data.version === 'string' ? data.version.trim() : '';
  } catch (err) {
    console.warn('Beta version label unavailable:', err);
    betaVersion = '';
  }
  const suffix = betaVersion ? ` ${betaVersion}` : '';
  setLabelText('firmwareBetaLabel', 'firmwareBeta', `Beta${suffix}`);
  setLabelText('firmwareBetaModulrLabel', 'firmwareBetaModulr', `BETA-MODULR${suffix}`);
}

function firmwareDisplayName(selected, fileName) {
  if (selected === 'beta' && betaVersion) return `Beta ${betaVersion} (${fileName})`;
  if (selected === 'beta-modulr' && betaVersion) return `BETA-MODULR ${betaVersion} (${fileName})`;
  return fileName;
}

const flasher = new TeensyFlasher();
const serialManager = new SerialPortManager();
const setupWizard = mountSetupWizard(document.getElementById('setupWizard'), {
  beforeConnect: async () => {
    if (ramTest?.busy) throw new Error('Stop the RAM test before connecting guided setup.');
    await ramTest?.disconnect();
    if (serialManager.serialPort) await serialManager.closeSerialPort();
  },
  installFirmware: () => {
    const selected = document.querySelector('input[name="firmware"]:checked')?.value;
    if (selected === 'stable' || selected === 'ramtest') {
      document.getElementById('firmwareBeta').checked = true;
      document.getElementById('firmwareBeta').dispatchEvent(new Event('change'));
    }
    document.getElementById('uploadFirmwareBtn').scrollIntoView({ behavior: 'smooth', block: 'center' });
    setStatus('Install the latest Beta (or BETA-MODULR for an external panel), then reconnect in guided setup. Your answers are kept.');
  },
});

function chooseFirmware(value) {
  document.querySelector(`input[name="firmware"][value="${value}"]`).checked = true;
  document.querySelector(`input[name="firmware"][value="${value}"]`).dispatchEvent(new Event('change'));
  document.getElementById('uploadFirmwareBtn').scrollIntoView({ behavior: 'smooth', block: 'center' });
}
ramTest = mountRamTest(document.getElementById('ramTest'), {
  beforeConnect: async () => {
    if (setupWizard.busy || document.getElementById('uploadFirmwareBtn').disabled) throw new Error('Wait for the current device operation to finish.');
    await setupWizard.disconnect();
    if (serialManager.serialPort) await serialManager.closeSerialPort();
  },
  selectTest: () => chooseFirmware('ramtest'),
  restoreFirmware: () => {
    chooseFirmware(lastInstrumentFirmware);
    setStatus('Choose your usual instrument build, then Upload Firmware to restore TŒRN.');
  },
});

/**
 * Helper to update both a status box in the UI and the console.
 * If message is empty or null, hides the status box.
 * @param {string} message
 */
function setLoaded(message) {
  document.getElementById('firmwareLoaded').textContent = message || '';
}

function setStatus(message) {
  const statusElem = document.getElementById('status');
  if (!message) {
    statusElem.style.display = 'none';
    statusElem.textContent = '';
    return;
  }
  console.log(message);
  statusElem.style.display = 'block';
  statusElem.textContent = message;
}

serialManager.onData = (line) => {
  const logArea = document.getElementById('log');
  logArea.value += (logArea.value ? '\n' : '') + line;
  logArea.scrollTop = logArea.scrollHeight;
};

/** Load firmware from URL and set firmwareData/firmwareName */
async function loadFirmware() {
  const requestId = ++firmwareLoadRequest;
  firmwareData = null;
  firmwareName = null;
  const selected = document.querySelector('input[name="firmware"]:checked')?.value || 'stable';
  const url = FIRMWARE_URLS[selected] || FIRMWARE_URLS.stable;
  const name = FIRMWARE_NAMES[selected] || FIRMWARE_NAMES.stable;
  try {
    setLoaded('Loading firmware...');
    const res = await fetch(url);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const buf = await res.arrayBuffer();
    if (requestId !== firmwareLoadRequest) return;
    firmwareData = new Uint8Array(buf);
    firmwareName = name;
    setLoaded(`Firmware loaded: ${firmwareDisplayName(selected, name)}`);
  } catch (err) {
    if (requestId !== firmwareLoadRequest) return;
    setLoaded(`Failed to load firmware: ${err.message}`);
    firmwareData = null;
    firmwareName = null;
  }
}

document.querySelectorAll('input[name="firmware"]').forEach(input => {
  input.addEventListener('change', () => {
    const ram = input.value === 'ramtest';
    if (!ram) lastInstrumentFirmware = input.value;
    document.getElementById('betaWarning').classList.toggle('is-visible', input.value.startsWith('beta'));
    document.getElementById('ramTestWarning').hidden = !ram;
    document.getElementById('uploadFirmwareBtn').textContent = ram ? 'Upload RAM test' : 'Upload Firmware';
    loadFirmware();
  });
});

// Load firmware on page load (v3.0 default); Beta labels include sketch VERSION.
applyBetaVersionLabels().then(() => loadFirmware());

const uploadBtn = document.getElementById('uploadFirmwareBtn');

// Running TŒRN sketch is USB serial (vendor 0x16C0). Bootloader PIDs are TEENSY_DEVICE_FILTERS.
// Opening that port at 134 baud is the Teensy reboot-to-bootloader request.
const REBOOT_BAUD = 134;

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function permittedBootloader() {
  if (!navigator.hid?.getDevices) return null;
  const devices = await navigator.hid.getDevices();
  return devices.find((device) => TEENSY_DEVICE_FILTERS.some(
    (filter) => filter.vendorId === device.vendorId && filter.productId === device.productId
  )) || null;
}

let awaitingHid = false;
const selectBtn = document.getElementById('selectDeviceBtn');

function showHidChoice(message) {
  awaitingHid = true;
  selectBtn.hidden = false;
  setStatus(message);
}

async function chooseRunningPort() {
  // Always ask. A stored port can be a stale bootloader session, and the HID
  // list cannot see a sketch that is still running.
  setStatus('Choose the running Teensy serial port.');
  return navigator.serial.requestPort({ filters: [{ usbVendorId: 0x16c0 }] });
}

function waitForDisconnect(port, ms) {
  return new Promise((resolve) => {
    const timer = setTimeout(() => {
      port.removeEventListener('disconnect', onGone);
      resolve(false);
    }, ms);
    const onGone = () => {
      clearTimeout(timer);
      resolve(true);
    };
    port.addEventListener('disconnect', onGone, { once: true });
  });
}

async function rebootIntoBootloader() {
  if (serialManager.serialPort) {
    try { await serialManager.closeSerialPort(); } catch (_) {}
  }
  const port = await chooseRunningPort();
  if (port.readable || port.writable) {
    try { await port.close(); } catch (_) {}
  }
  setStatus('Rebooting into programming mode…');
  await port.open({ baudRate: REBOOT_BAUD });
  const gone = await waitForDisconnect(port, 4000);
  try { await port.close(); } catch (_) {}
  if (!gone) {
    throw new Error('The Teensy stayed on its serial port. Close any other serial program and try Upload again.');
  }
  await sleep(800);
}

async function waitForPermittedBootloader(ms) {
  const start = performance.now();
  while (performance.now() - start < ms) {
    const device = await permittedBootloader();
    if (device) return device;
    await sleep(200);
  }
  return null;
}

// Serial open at 134 baud reboots a running sketch. The HID bootloader exists
// only after that serial port disconnects and the red LED is on.
async function ensureBootloader() {
  const already = await permittedBootloader();
  if (already) return already;
  if (awaitingHid) return null;

  await rebootIntoBootloader();
  const appeared = await waitForPermittedBootloader(2500);
  if (appeared) {
    awaitingHid = false;
    selectBtn.hidden = false;
    return appeared;
  }
  showHidChoice('Teensy is in programming mode. Click Choose Teensy bootloader.');
  return null;
}

selectBtn.addEventListener('click', async () => {
  if (!navigator.hid) { setStatus('Use Chrome or Edge on a desktop computer.'); return; }
  if (setupWizard.busy || ramTest.busy || uploadBtn.disabled) { setStatus('Finish or stop the current device operation first.'); return; }
  try {
    const devices = await navigator.hid.requestDevice({ filters: TEENSY_DEVICE_FILTERS });
    if (devices && devices.length > 0) {
      selectedDevice = devices[0];
      awaitingHid = false;
      selectBtn.hidden = false;
      setStatus(`Teensy ready: ${selectedDevice.productName || 'bootloader'}. Press ${uploadBtn.textContent}.`);
    } else {
      setStatus('No device selected. Press the Teensy programming button so its red LED is on.');
    }
  } catch (error) {
    setStatus(`Device selection error: ${error}`);
    console.error('Device selection error:', error);
  }
});

document.getElementById('uploadFirmwareBtn').addEventListener('click', async () => {
  if (setupWizard.busy || ramTest.busy) { setStatus('Finish or stop the current device test/setup before flashing.'); return; }
  if (!firmwareData) {
    setStatus('Firmware is still loading.');
    return;
  }
  if (!navigator.hid || (!selectedDevice && !navigator.serial)) {
    setStatus('Use Chrome or Edge on a desktop computer.');
    return;
  }

  const isRamTest = document.getElementById('firmwareRamTest').checked;
  uploadBtn.disabled = true;
  document.querySelectorAll('input[name="firmware"]').forEach(input => { input.disabled = true; });
  setupWizard.setFlashing(true);
  ramTest.setFlashing(true);
  try {
    await ramTest.disconnect();
    await setupWizard.disconnect();
    const device = selectedDevice || await ensureBootloader();
    if (!device) {
      showHidChoice('Teensy is in programming mode. Click Choose Teensy bootloader, then Upload Firmware.');
      return;
    }
    selectedDevice = null;

    // HalfKay PID 0478 is shared across Teensy models, including Teensy 4.1.
    // The selected RAM-test image targets TŒRN/4.1; PID is not a model ID.
    if (isRamTest && ![0x0478, 0x0479].includes(device.productId)) throw new Error('Choose the Teensy HalfKay bootloader (16C0:0478) after pressing its programming button.');
    const fw = new FirmwareFile(firmwareData, firmwareName, device.productId);
    const blocks = await fw.buildBlocks();

    document.getElementById('flashProgress').value = 0;
    setStatus(isRamTest ? 'Flashing RAM-test firmware...' : 'Flashing firmware...');

    await flasher.flashFirmware(blocks, device, (progress) => {
      document.getElementById('flashProgress').value = progress;
    });

    setStatus(isRamTest ? 'RAM-test firmware uploaded. Wait for the restart, then use Connect RAM tester below.' : 'Flash complete! You can now configure your device in guided setup below.');
    ramTest.afterFlash(isRamTest);
    if (!isRamTest) setupWizard.afterFlash();
    if (isRamTest) document.getElementById('ramTest').scrollIntoView({ behavior: 'smooth', block: 'start' });
  } catch (err) {
    selectedDevice = null;
    const blocked = err && (err.name === 'SecurityError' || err.name === 'NotAllowedError' || err.name === 'NotFoundError');
    setStatus(blocked
      ? 'Factory firmware or no serial port? Press the Teensy programming button, then Choose Teensy bootloader and upload.'
      : `Flashing error: ${err}`);
    console.error('Flashing error:', err);
  } finally {
    uploadBtn.disabled = false;
    document.querySelectorAll('input[name="firmware"]').forEach(input => { input.disabled = false; });
    setupWizard.setFlashing(false);
    ramTest.setFlashing(false);
  }
});

document.getElementById('openSerialBtn').addEventListener('click', async () => {
  if (setupWizard.busy || ramTest.busy || uploadBtn.disabled) { setStatus('Wait for the current device operation to finish.'); return; }
  try {
    await setupWizard.disconnect();
    await ramTest.disconnect();
    await serialManager.openSerialPort({ baudRate: 115200 });
    setStatus('Serial opened.');
  } catch (err) {
    setStatus(`Serial open error: ${err}`);
    console.error('Serial open error:', err);
  }
});

document.getElementById('closeSerialBtn').addEventListener('click', async () => {
  try {
    await serialManager.closeSerialPort();
    setStatus('Serial closed.');
  } catch (err) {
    setStatus(`Serial close error: ${err}`);
    console.error('Serial close error:', err);
  }
});
