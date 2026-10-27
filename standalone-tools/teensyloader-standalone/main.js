import {
  FirmwareFile,
  TeensyFlasher,
  SerialPortManager,
  TEENSY_DEVICE_FILTERS
} from './Teensy-Loader.js';

const FIRMWARE_URLS = {
  stable: new URL('./public/toern_firmware_3.0.ino.hex', import.meta.url).href,
  beta: new URL('./public/toern_firmware_beta.ino.hex', import.meta.url).href,
  'beta-eurorck': new URL('./public/toern_firmware_beta_eurorck.ino.hex', import.meta.url).href
};
const FIRMWARE_NAMES = {
  stable: 'toern_firmware_3.0.ino.hex',
  beta: 'toern_firmware_beta.ino.hex',
  'beta-eurorck': 'toern_firmware_beta_eurorck.ino.hex'
};

let selectedDevice = null;
let firmwareData = null;
let firmwareName = null;

const flasher = new TeensyFlasher();
const serialManager = new SerialPortManager();

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
  const selected = document.querySelector('input[name="firmware"]:checked')?.value || 'stable';
  const url = FIRMWARE_URLS[selected] || FIRMWARE_URLS.stable;
  const name = FIRMWARE_NAMES[selected] || FIRMWARE_NAMES.stable;
  try {
    setLoaded('Loading firmware...');
    const res = await fetch(url);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const buf = await res.arrayBuffer();
    firmwareData = new Uint8Array(buf);
    firmwareName = name;
    setLoaded(`Firmware loaded: ${name}`);
  } catch (err) {
    setLoaded(`Failed to load firmware: ${err.message}`);
    firmwareData = null;
    firmwareName = null;
  }
}

document.getElementById('firmwareStable').addEventListener('change', () => {
  document.getElementById('betaWarning').classList.remove('is-visible');
  loadFirmware();
});

document.getElementById('firmwareBeta').addEventListener('change', () => {
  document.getElementById('betaWarning').classList.add('is-visible');
  loadFirmware();
});

document.getElementById('firmwareBetaEurorck').addEventListener('change', () => {
  document.getElementById('betaWarning').classList.add('is-visible');
  loadFirmware();
});

// Load firmware on page load (v3.0 default)
loadFirmware();

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
// only after that serial port disconnects and the red LED is blinking.
async function ensureBootloader() {
  const already = await permittedBootloader();
  if (already) return already;
  if (awaitingHid) return null;

  await rebootIntoBootloader();
  const appeared = await waitForPermittedBootloader(2500);
  if (appeared) {
    awaitingHid = false;
    selectBtn.hidden = true;
    return appeared;
  }
  showHidChoice('Red LED is blinking. Click Choose blinking Teensy.');
  return null;
}

selectBtn.addEventListener('click', async () => {
  if (!awaitingHid && !(await permittedBootloader())) {
    setStatus('Press Upload Firmware first. The HID list cannot see the Teensy until the red LED blinks.');
    return;
  }
  try {
    const devices = await navigator.hid.requestDevice({ filters: TEENSY_DEVICE_FILTERS });
    if (devices && devices.length > 0) {
      selectedDevice = devices[0];
      awaitingHid = false;
      selectBtn.hidden = true;
      setStatus(`Teensy ready: ${selectedDevice.productName || 'bootloader'}. Press Upload Firmware.`);
    } else {
      setStatus('No device selected. The red LED must still be blinking.');
    }
  } catch (error) {
    setStatus(`Device selection error: ${error}`);
    console.error('Device selection error:', error);
  }
});

document.getElementById('uploadFirmwareBtn').addEventListener('click', async () => {
  if (!firmwareData) {
    setStatus('Firmware is still loading.');
    return;
  }
  if (!navigator.serial || !navigator.hid) {
    setStatus('Use Chrome or Edge on a desktop computer.');
    return;
  }

  uploadBtn.disabled = true;
  try {
    const device = selectedDevice || await ensureBootloader();
    if (!device) {
      showHidChoice('Red LED is blinking. Click Choose blinking Teensy, then Upload Firmware.');
      return;
    }
    selectedDevice = null;

    const fw = new FirmwareFile(firmwareData, firmwareName, device.productId);
    const blocks = await fw.buildBlocks();

    document.getElementById('flashProgress').value = 0;
    setStatus('Flashing firmware...');

    await flasher.flashFirmware(blocks, device, (progress) => {
      document.getElementById('flashProgress').value = progress;
    });

    setStatus('Flash complete!');
  } catch (err) {
    selectedDevice = null;
    const blocked = err && (err.name === 'SecurityError' || err.name === 'NotAllowedError');
    setStatus(blocked
      ? 'Red LED is blinking. Click Choose blinking Teensy, then Upload Firmware.'
      : `Flashing error: ${err}`);
    console.error('Flashing error:', err);
  } finally {
    uploadBtn.disabled = false;
  }
});

document.getElementById('openSerialBtn').addEventListener('click', async () => {
  try {
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
