import {
  FirmwareFile,
  TeensyFlasher,
  SerialPortManager,
  TEENSY_DEVICE_FILTERS
} from './Teensy-Loader.js';

const FIRMWARE_URLS = {
  stable: new URL('./public/toern_firmware_3.0.ino.hex', import.meta.url).href,
  beta: new URL('./public/toern_firmware_beta.ino.hex', import.meta.url).href
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
  const isBeta = document.getElementById('firmwareBeta').checked;
  const url = isBeta ? FIRMWARE_URLS.beta : FIRMWARE_URLS.stable;
  const name = isBeta ? 'toern_firmware_beta.ino.hex' : 'toern_firmware_3.0.ino.hex';
  try {
    setStatus('Loading firmware...');
    const res = await fetch(url);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const buf = await res.arrayBuffer();
    firmwareData = new Uint8Array(buf);
    firmwareName = name;
    setStatus(`Firmware loaded: ${name}`);
  } catch (err) {
    setStatus(`Failed to load firmware: ${err.message}`);
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

async function chooseRunningPort() {
  const ports = await navigator.serial.getPorts();
  const infoOf = (port) => port.getInfo?.() || {};
  const exact = ports.find((port) => {
    const info = infoOf(port);
    return info.usbVendorId === 0x16c0 && info.usbProductId === 0x048a;
  });
  if (exact) return exact;
  const teensy = ports.find((port) => infoOf(port).usbVendorId === 0x16c0);
  if (teensy) return teensy;
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
  await port.open({ baudRate: REBOOT_BAUD });
  await waitForDisconnect(port, 2500);
  try { await port.close(); } catch (_) {}
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

// A running sketch reboots when the serial port opens at 134 baud. The red LED
// then blinks and the board reappears as the HID bootloader. No program button.
async function ensureBootloader() {
  const already = await permittedBootloader();
  if (already) return already;

  setStatus('Rebooting into programming mode…');
  try {
    await rebootIntoBootloader();
  } catch (err) {
    if (!(err && (err.name === 'NotFoundError' || err.name === 'AbortError'))) throw err;
  }

  const appeared = await waitForPermittedBootloader(4000);
  if (appeared) return appeared;

  setStatus('Red LED blinking — choose the Teensy in the list.');
  const devices = await navigator.hid.requestDevice({ filters: TEENSY_DEVICE_FILTERS });
  return devices && devices[0] ? devices[0] : null;
}

document.getElementById('selectDeviceBtn').addEventListener('click', async () => {
  try {
    const devices = await navigator.hid.requestDevice({ filters: TEENSY_DEVICE_FILTERS });
    if (devices && devices.length > 0) {
      selectedDevice = devices[0];
      setStatus(`Device selected: ${selectedDevice.productName || 'Unknown Teensy'}. Red LED should be blinking.`);
    } else {
      selectedDevice = null;
      setStatus('No device selected.');
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
      setStatus('No Teensy in programming mode. If the red LED is blinking, click Select Teensy Device.');
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
      ? 'The red LED should be blinking. Click Select Teensy Device, then Upload Firmware.'
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
