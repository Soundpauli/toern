import {
  FirmwareFile,
  TeensyFlasher,
  SerialPortManager,
  TEENSY_DEVICE_FILTERS
} from './Teensy-Loader.js';

const FIRMWARE_URLS = {
  stable: './public/toern_firmware_2.2.ino.hex',
  beta: './public/toern_firmware_beta.ino.hex'
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
  const name = isBeta ? 'toern_firmware_beta.ino.hex' : 'toern_firmware_2.2.ino.hex';
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

// Load firmware on page load (v2.2 default)
loadFirmware();

const uploadBtn = document.getElementById('uploadFirmwareBtn');
uploadBtn.disabled = true;

document.getElementById('selectDeviceBtn').addEventListener('click', async () => {
  try {
    const devices = await navigator.hid.requestDevice({ filters: TEENSY_DEVICE_FILTERS });
    if (devices && devices.length > 0) {
      selectedDevice = devices[0];
      uploadBtn.disabled = false;
      setStatus(`Device selected: ${selectedDevice.productName || 'Unknown Teensy'}`);
    } else {
      selectedDevice = null;
      uploadBtn.disabled = true;
      setStatus('No device selected.');
    }
  } catch (error) {
    selectedDevice = null;
    uploadBtn.disabled = true;
    setStatus(`Device selection error: ${error}`);
    console.error('Device selection error:', error);
  }
});

document.getElementById('uploadFirmwareBtn').addEventListener('click', async () => {
  if (!selectedDevice || !firmwareData) {
    setStatus('No device or firmware selected.');
    return;
  }

  try {
    const fw = new FirmwareFile(firmwareData, firmwareName, selectedDevice.productId);
    const blocks = await fw.buildBlocks();

    document.getElementById('flashProgress').value = 0;
    setStatus('Flashing firmware...');

    await flasher.flashFirmware(blocks, selectedDevice, (progress) => {
      document.getElementById('flashProgress').value = progress;
    });

    setStatus('Flash complete!');
  } catch (err) {
    setStatus(`Flashing error: ${err}`);
    console.error('Flashing error:', err);
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
