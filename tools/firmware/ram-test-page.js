import { mountRamTest } from './ram-test.js';
import './setup-wizard.css';
import {
  FirmwareFile,
  TeensyFlasher,
  SerialPortManager,
  TEENSY_DEVICE_FILTERS
} from './Teensy-Loader.js';

const FIRMWARE_URL = new URL('./public/toern_ram_test.ino.hex', import.meta.url).href;
const FIRMWARE_NAME = 'toern_ram_test.ino.hex';

let selectedDevice = null;
let firmwareData = null;
let firmwareName = null;
let firmwareLoadRequest = 0;
let awaitingHid = false;

const flasher = new TeensyFlasher();
const serialManager = new SerialPortManager();
const uploadBtn = document.getElementById('uploadFirmwareBtn');
const selectBtn = document.getElementById('selectDeviceBtn');
const REBOOT_BAUD = 134;

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

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function showHidChoice(message) {
  awaitingHid = true;
  selectBtn.hidden = false;
  setStatus(message);
}

async function permittedBootloader() {
  if (!navigator.hid?.getDevices) return null;
  const devices = await navigator.hid.getDevices();
  return devices.find((device) => TEENSY_DEVICE_FILTERS.some(
    (filter) => filter.vendorId === device.vendorId && filter.productId === device.productId
  )) || null;
}

async function chooseRunningPort() {
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

async function loadFirmware() {
  const requestId = ++firmwareLoadRequest;
  firmwareData = null;
  firmwareName = null;
  try {
    setLoaded('Loading firmware...');
    const res = await fetch(FIRMWARE_URL);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const buf = await res.arrayBuffer();
    if (requestId !== firmwareLoadRequest) return;
    firmwareData = new Uint8Array(buf);
    firmwareName = FIRMWARE_NAME;
    setLoaded(`Firmware loaded: ${FIRMWARE_NAME}`);
  } catch (err) {
    if (requestId !== firmwareLoadRequest) return;
    setLoaded(`Failed to load firmware: ${err.message}`);
    firmwareData = null;
    firmwareName = null;
  }
}

const ramTest = mountRamTest(document.getElementById('ramTest'), {
  beforeConnect: async () => {
    if (uploadBtn.disabled) throw new Error('Wait for the current device operation to finish.');
    if (serialManager.serialPort) await serialManager.closeSerialPort();
  },
  selectTest: () => {
    document.getElementById('firmwareRamTest').checked = true;
    uploadBtn.scrollIntoView({ behavior: 'smooth', block: 'center' });
    setStatus('Press Upload RAM test, then connect here after the device restarts.');
  },
  restoreFirmware: () => {
    window.location.href = './';
  },
});

loadFirmware();

selectBtn.addEventListener('click', async () => {
  if (!navigator.hid) { setStatus('Use Chrome or Edge on a desktop computer.'); return; }
  if (ramTest.busy || uploadBtn.disabled) { setStatus('Finish or stop the current device operation first.'); return; }
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

uploadBtn.addEventListener('click', async () => {
  if (ramTest.busy) { setStatus('Finish or stop the current device test before flashing.'); return; }
  if (!firmwareData) {
    setStatus('Firmware is still loading.');
    return;
  }
  if (!navigator.hid || (!selectedDevice && !navigator.serial)) {
    setStatus('Use Chrome or Edge on a desktop computer.');
    return;
  }

  uploadBtn.disabled = true;
  document.querySelectorAll('input[name="firmware"]').forEach((input) => { input.disabled = true; });
  ramTest.setFlashing(true);
  try {
    await ramTest.disconnect();
    const device = selectedDevice || await ensureBootloader();
    if (!device) {
      showHidChoice('Teensy is in programming mode. Click Choose Teensy bootloader, then Upload RAM test.');
      return;
    }
    selectedDevice = null;

    if (![0x0478, 0x0479].includes(device.productId)) {
      throw new Error('Choose the Teensy HalfKay bootloader (16C0:0478) after pressing its programming button.');
    }
    const fw = new FirmwareFile(firmwareData, firmwareName, device.productId);
    const blocks = await fw.buildBlocks();

    document.getElementById('flashProgress').value = 0;
    setStatus('Flashing RAM-test firmware...');

    await flasher.flashFirmware(blocks, device, (progress) => {
      document.getElementById('flashProgress').value = progress;
    });

    setStatus('RAM-test firmware uploaded. Wait for the restart, then use Connect RAM tester below.');
    ramTest.afterFlash(true);
    document.getElementById('ramTest').scrollIntoView({ behavior: 'smooth', block: 'start' });
  } catch (err) {
    selectedDevice = null;
    const blocked = err && (err.name === 'SecurityError' || err.name === 'NotAllowedError' || err.name === 'NotFoundError');
    setStatus(blocked
      ? 'Factory firmware or no serial port? Press the Teensy programming button, then Choose Teensy bootloader and upload.'
      : `Flashing error: ${err}`);
    console.error('Flashing error:', err);
  } finally {
    uploadBtn.disabled = false;
    document.querySelectorAll('input[name="firmware"]').forEach((input) => { input.disabled = false; });
    ramTest.setFlashing(false);
  }
});
