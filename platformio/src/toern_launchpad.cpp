// Novation Launchpad Mini MK3 mixer over Teensy 4.1 USB host.
#include "toern_launchpad.h"

#ifndef TOERN_LAUNCHPAD_HOST
#define TOERN_LAUNCHPAD_HOST 1
#endif

#if TOERN_LAUNCHPAD_HOST

#include <Arduino.h>
#include <USBHost_t36.h>
#include <new>
#include <string.h>

static constexpr uint16_t NOVATION_VID = 0x1235;
static constexpr uint16_t LP_MINI_MK3_PID = 0x0113;
// One USB MIDIStreaming interface, two embedded jack pairs (virtual cables).
// Cable 0 = DAW; cable 1 = MIDI / programmer pads and LEDs.
static constexpr uint8_t LP_MIDI_CABLE = 1;
static constexpr uint8_t LP_VOL_LEVELS[5] = {0, 4, 8, 12, 16};
// Wait until the instrument is fully up before touching EHCI.
static constexpr uint32_t LP_HOST_DELAY_MS = 8000;
static constexpr uint32_t LP_TASK_IDLE_MS = 5;

static constexpr uint8_t LP_COL_OFF = 0;
static constexpr uint8_t LP_COL_DIM = 1;
static constexpr uint8_t LP_COL_YELLOW = 13;
static constexpr uint8_t LP_COL_GREEN = 21;
static constexpr uint8_t LP_COL_GREEN_DIM = 23;

// The hub contributes the Device_t pool, including for direct attachment.
// Keep these zero-initialized stores in RAM1 (.bss), just like normal static
// USBHost_t36 objects. The drivers contribute embedded Pipe_t / Transfer_t
// descriptors to EHCI; this library does not maintain the RAM2 data cache for
// them. Cached, non-zeroed DMAMEM in RAM2 can therefore stall enumeration.
alignas(USBHost) static uint8_t lpUsbStore[sizeof(USBHost)];
alignas(USBHub) static uint8_t lpHubStore[sizeof(USBHub)];
alignas(MIDIDevice_BigBuffer) static uint8_t lpMidiStoreA[sizeof(MIDIDevice_BigBuffer)];

static USBHost *lpUsb = nullptr;
static USBHub *lpHub = nullptr;
static MIDIDevice_BigBuffer *lpMidiA = nullptr;
static MIDIDeviceBase *lpMidi = nullptr;
static uint8_t lpInitStep = 0;  // 0=construct, 1=begin, 2=enum wait, 3=ready
static uint16_t lpEnumTicks = 0;  // must be >8-bit — enum wait runs to 1000
static uint8_t lpConnectStep = 0;
static elapsedMillis lpTaskTimer;
static elapsedMillis lpConnectTimer;

static bool lpArmed = false;
static bool lpReady = false;
static bool lpWasConnected = false;
static bool lpSoloActive = false;
static bool lpSoloSaved[16];
static uint8_t lpSoloChannel = 0;
static bool lpPaintDirty = true;
static elapsedMillis lpPaintTimer;
static elapsedMillis lpDiagTimer;
static uint8_t lpLastVoice = 255;
static uint8_t lpLastVol[9];
static bool lpLastMute[9];
static uint32_t lpLastColor[9];
static bool lpLastSolo = false;

static bool lpNoteToCell(uint8_t note, uint8_t &row, uint8_t &col) {
  if (note < 11 || note > 88) return false;
  row = note / 10;
  col = note % 10;
  return row >= 1 && row <= 8 && col >= 1 && col <= 8;
}

static uint8_t lpCellToNote(uint8_t row, uint8_t col) {
  return (uint8_t)(row * 10 + col);
}

static int lpVolumeForRow(uint8_t row) {
  if (row < 2 || row > 6) return -1;
  return LP_VOL_LEVELS[row - 2];
}

static uint8_t lpVolumeRowFor(int vol) {
  if (vol <= 0) return 0;
  if (vol > 16) vol = 16;
  uint8_t best = 2;
  int bestDist = 999;
  for (uint8_t i = 0; i < 5; i++) {
    int d = abs((int)LP_VOL_LEVELS[i] - vol);
    if (d < bestDist) {
      bestDist = d;
      best = (uint8_t)(i + 2);
    }
  }
  return best;
}

static bool lpMidiLooksLikeLaunchpad(MIDIDeviceBase *m) {
  return m && (*m) && m->idVendor() == NOVATION_VID
      && m->idProduct() == LP_MINI_MK3_PID;
}

static MIDIDeviceBase *lpPickPort() {
  if (lpMidiLooksLikeLaunchpad(lpMidiA)) return lpMidiA;
  return nullptr;
}

static uint8_t lpModelFor(MIDIDeviceBase *m) {
  if (!m) return 0x0d;
  uint16_t pid = m->idProduct();
  if (pid == 0x0103) return 0x0c;  // Launchpad X
  if (pid == 0x0123) return 0x0e;  // Pro MK3
  return 0x0d;                    // Mini MK3 (0x0113) and default
}

static void lpSendSysEx(MIDIDeviceBase *m, const uint8_t *data, uint32_t len) {
  if (!m) return;
  m->sendSysEx(len, data, true, LP_MIDI_CABLE);
}

// Mini MK3 programmers reference:
//  - Standalone/DAW: F0 00 20 29 02 0D 10 <0|1> F7
//  - Prog/Live mode:  F0 00 20 29 02 0D 0E <0|1> F7
//  - Layout:          F0 00 20 29 02 0D 00 <layout> F7  (7F = Programmer)
//  - LED SysEx:       F0 00 20 29 02 0D 03 <type> <led> <palette> F7
static void lpSendStandalone(MIDIDeviceBase *m) {
  if (!m) return;
  const uint8_t model = lpModelFor(m);
  const uint8_t msg[] = {0xf0, 0x00, 0x20, 0x29, 0x02, model, 0x10, 0x00, 0xf7};
  lpSendSysEx(m, msg, sizeof(msg));
}

static void lpSendProgrammer(MIDIDeviceBase *m) {
  if (!m) return;
  const uint8_t model = lpModelFor(m);
  const uint8_t msg[] = {0xf0, 0x00, 0x20, 0x29, 0x02, model, 0x0e, 0x01, 0xf7};
  lpSendSysEx(m, msg, sizeof(msg));
}

static void lpSendProgLayout(MIDIDeviceBase *m) {
  if (!m) return;
  const uint8_t model = lpModelFor(m);
  const uint8_t msg[] = {0xf0, 0x00, 0x20, 0x29, 0x02, model, 0x00, 0x7f, 0xf7};
  lpSendSysEx(m, msg, sizeof(msg));
}

static void lpLight(uint8_t note, uint8_t color) {
  if (!lpMidi) return;
  // Programmer path: LED lighting SysEx command 0x03.
  const uint8_t model = lpModelFor(lpMidi);
  const uint8_t sysexLed[] = {
    0xf0, 0x00, 0x20, 0x29, 0x02, model, 0x03, 0x00, note, color, 0xf7
  };
  lpSendSysEx(lpMidi, sysexLed, sizeof(sysexLed));
}

static void lpLightVoice(uint8_t note, uint32_t rgb, uint8_t brightness) {
  if (!lpMidi) return;
  // LED command 0x03, lighting type 3 = RGB (each component is 0..127).
  const uint8_t msg[] = {
    0xf0, 0x00, 0x20, 0x29, 0x02, lpModelFor(lpMidi), 0x03, 0x03, note,
    (uint8_t)(((rgb >> 16) & 255) * brightness / 255),
    (uint8_t)(((rgb >> 8) & 255) * brightness / 255),
    (uint8_t)((rgb & 255) * brightness / 255), 0xf7
  };
  lpSendSysEx(lpMidi, msg, sizeof(msg));
}

static void lpSelectVoice(uint8_t ch) {
  launchpadSelectVoice(ch);
  lpPaintDirty = true;
}

static void lpToggleMute(uint8_t ch) {
  if (launchpadGetChildLock() || lpSoloActive) return;
  if (ch < 1 || ch > 8) return;
  launchpadSetMuted(ch, !launchpadGetMuted(ch));
  lpPaintDirty = true;
}

static void lpSetVolume(uint8_t ch, int vol) {
  if (ch < 1 || ch > 8) return;
  launchpadSetChannelVolume(ch, vol);
  lpPaintDirty = true;
}

static void lpSoloToggle(uint8_t ch) {
  if (launchpadGetChildLock()) return;
  if (ch < 1 || ch > 8) return;
  if (lpSoloActive && lpSoloChannel == ch) {
    for (unsigned i = 1; i < 16; i++) launchpadSetMuted(i, lpSoloSaved[i]);
    lpSoloActive = false;
    lpSoloChannel = 0;
  } else {
    if (!lpSoloActive) {
      memset(lpSoloSaved, 0, sizeof(lpSoloSaved));
      for (unsigned i = 1; i < 16; i++) lpSoloSaved[i] = launchpadGetMuted(i);
    }
    for (unsigned i = 1; i < 16; i++) launchpadSetMuted(i, i != ch);
    lpSoloActive = true;
    lpSoloChannel = ch;
    lpSelectVoice(ch);
  }
  lpPaintDirty = true;
}

static void lpHandlePad(uint8_t row, uint8_t col) {
  if (row == 1) {
    lpToggleMute(col);
    return;
  }
  if (row >= 2 && row <= 6) {
    int vol = lpVolumeForRow(row);
    if (vol >= 0) lpSetVolume(col, vol);
    return;
  }
  if (row == 7) {
    lpSoloToggle(col);
    return;
  }
  if (row == 8) lpSelectVoice(col);
}

static void lpPaint() {
  if (!lpMidi) return;
  const uint8_t selected = (uint8_t)constrain((int)launchpadGetCurrentChannel(), 1, 8);
  for (uint8_t col = 1; col <= 8; col++) {
    const bool muted = launchpadGetMuted(col);
    const int vol = (int)launchpadGetChannelVolume(col);
    const uint32_t voiceColor = launchpadGetVoiceColor(col);
    lpLightVoice(lpCellToNote(1, col), voiceColor, muted ? 10 : 127);
    const uint8_t litRow = lpVolumeRowFor(vol);
    for (uint8_t row = 2; row <= 6; row++) {
      uint8_t color = LP_COL_OFF;
      if (litRow && row <= litRow) {
        color = muted ? LP_COL_DIM : (row == litRow ? LP_COL_GREEN : LP_COL_GREEN_DIM);
      }
      lpLight(lpCellToNote(row, col), color);
    }
    const bool thisSolo = lpSoloActive && lpSoloChannel == col;
    lpLight(lpCellToNote(7, col), thisSolo ? LP_COL_YELLOW : LP_COL_DIM);
    lpLightVoice(lpCellToNote(8, col), voiceColor, selected == col ? 127 : 10);
    lpLastColor[col] = voiceColor;
  }
  lpLastVoice = selected;
  lpLastSolo = lpSoloActive;
  for (uint8_t ch = 1; ch <= 8; ch++) {
    lpLastVol[ch] = (uint8_t)launchpadGetChannelVolume(ch);
    lpLastMute[ch] = launchpadGetMuted(ch);
  }
  lpPaintDirty = false;
  lpPaintTimer = 0;
}

static bool lpStateChanged() {
  uint8_t selected = (uint8_t)constrain((int)launchpadGetCurrentChannel(), 1, 8);
  if (selected != lpLastVoice || lpSoloActive != lpLastSolo) return true;
  for (uint8_t ch = 1; ch <= 8; ch++) {
    if (lpLastMute[ch] != launchpadGetMuted(ch)) return true;
    if (lpLastColor[ch] != launchpadGetVoiceColor(ch)) return true;
    if (lpLastVol[ch] != (uint8_t)launchpadGetChannelVolume(ch)) return true;
  }
  return false;
}

static void lpPrintPort(const char *tag, MIDIDeviceBase *m) {
  if (!m) {
    Serial.printf("LP %s: null\n", tag);
    return;
  }
  if (!(*m)) {
    Serial.printf("LP %s: not claimed\n", tag);
    return;
  }
  Serial.printf("LP %s: vid=%04X pid=%04X\n", tag, m->idVendor(), m->idProduct());
}

static void lpPrintPortsc() {
  // IMXRT USBHS PORTSC1: CCS=bit0, PE=bit2, PP=bit12, LSP/HSP speed bits
  const uint32_t portsc = USBHS_PORTSC1;
  const bool ccs = (portsc & USBHS_PORTSC_CCS) != 0;
  const bool pe = (portsc & USBHS_PORTSC_PE) != 0;
  const bool pp = (portsc & USBHS_PORTSC_PP) != 0;
  Serial.printf("LP PORTSC=%08lX CCS=%u PE=%u PP=%u\n",
                (unsigned long)portsc, ccs ? 1u : 0u, pe ? 1u : 0u, pp ? 1u : 0u);
}

static bool lpEnsureHost() {
  if (lpReady) return true;
  if (!lpArmed) return false;
  if (millis() < LP_HOST_DELAY_MS) return false;

  if (lpInitStep == 0) {
    Serial.println(F("LP construct"));
    Serial.flush();
    lpUsb = new (lpUsbStore) USBHost();
    lpHub = new (lpHubStore) USBHub(*lpUsb);
    lpMidiA = new (lpMidiStoreA) MIDIDevice_BigBuffer(*lpUsb);
    Serial.printf("LP storage hub=%p MIDI=%p (RAM1), programmer cable=%u\n",
                  (void *)lpHub, (void *)lpMidiA, LP_MIDI_CABLE);
    lpInitStep = 1;
    return false;
  }

  if (lpInitStep == 1) {
    Serial.println(F("LP begin"));
    Serial.flush();
    lpUsb->begin();
    // Let USBHost_t36 own port reset and connect/disconnect handling. Direct
    // PORTSC read-modify-writes also acknowledge write-one-to-clear change
    // bits and can hide events from the host ISR.
    lpEnumTicks = 0;
    lpInitStep = 2;
    lpTaskTimer = 0;
    return false;
  }

  if (lpInitStep == 2) {
    // Enumeration/control completion runs in the host ISR. Task services
    // active drivers; calling it more often cannot repair stalled descriptors.
    lpUsb->Task();
    lpEnumTicks++;
    if (lpEnumTicks == 1 || (lpEnumTicks % 200) == 0) {
      Serial.printf("LP enum tick %u\n", lpEnumTicks);
      lpPrintPortsc();
      Serial.flush();
    }
    // Wait up to ~5s of loop ticks or until a MIDI interface claims.
    if (lpEnumTicks < 1000 && !(lpMidiA && (*lpMidiA))) {
      return false;
    }
    lpReady = true;
    lpInitStep = 3;
    lpDiagTimer = 0;
    lpTaskTimer = 0;
    Serial.println(F("Launchpad USB host running (claims below)"));
    Serial.flush();
    lpPrintPortsc();
    lpPrintPort("MIDI interface", lpMidiA);
    return true;
  }

  return lpReady;
}

void launchpadHostBegin() {
  for (uint8_t i = 0; i < 9; i++) {
    lpLastVol[i] = 16;
    lpLastMute[i] = false;
  }
  lpArmed = true;
  lpReady = false;
  lpInitStep = 0;
  lpEnumTicks = 0;
  lpConnectStep = 0;
  lpUsb = nullptr;
  lpHub = nullptr;
  lpMidiA = nullptr;
  lpMidi = nullptr;
  Serial.println(F("BOOT launchpad USB host armed"));
  Serial.flush();
}

void launchpadHostService() {
  if (!lpEnsureHost()) return;

  // Keep Task hot while a device is present but unclaimed (enum in progress).
  const bool anyClaimed = lpMidiA && (*lpMidiA);
  if (!anyClaimed || lpTaskTimer >= LP_TASK_IDLE_MS) {
    lpTaskTimer = 0;
    lpUsb->Task();
  }

  MIDIDeviceBase *port = lpPickPort();
  const bool connected = port != nullptr;

  if (connected && !lpWasConnected) {
    lpMidi = port;
    lpConnectStep = 1;
    lpConnectTimer = 0;
    Serial.printf("Launchpad seen on MIDI cable 1 vid=%04X pid=%04X — staging programmer\n",
                  lpMidi->idVendor(), lpMidi->idProduct());
    lpPrintPort("MIDI interface", lpMidiA);
  } else if (!connected && lpWasConnected) {
    if (lpSoloActive) {
      for (unsigned i = 1; i < 16; i++) launchpadSetMuted(i, lpSoloSaved[i]);
      lpSoloActive = false;
      lpSoloChannel = 0;
    }
    lpMidi = nullptr;
    lpConnectStep = 0;
    Serial.println(F("Launchpad disconnected"));
  }
  lpWasConnected = connected;

  if (!connected) {
    if (lpDiagTimer >= 2000) {
      lpDiagTimer = 0;
      lpPrintPortsc();
      lpPrintPort("MIDI interface", lpMidiA);
    }
    return;
  }

  // Staged mode switch — one SysEx every 80ms, no blocking delay storms.
  if (lpConnectStep > 0 && lpConnectStep < 10 && lpConnectTimer >= 80) {
    lpConnectTimer = 0;
    MIDIDeviceBase *target = lpMidi;
    if (lpConnectStep == 1) {
      lpSendStandalone(target);
      lpConnectStep = 2;
    } else if (lpConnectStep == 2) {
      lpSendProgrammer(target);
      lpConnectStep = 3;
    } else if (lpConnectStep == 3) {
      lpSendProgLayout(target);
      lpConnectStep = 4;
    } else if (lpConnectStep == 4) {
      // Clear a few pads gently (not full 64-pad blast).
      for (uint8_t n = 11; n <= 18; n++) lpLight(n, LP_COL_OFF);
      lpConnectStep = 5;
    } else {
      lpPaintDirty = true;
      lpConnectStep = 10;
      Serial.println(F("Launchpad programmer staged"));
    }
  }

  auto drain = [](MIDIDeviceBase *m) {
    if (!m || !(*m)) return;
    while (m->read()) {
      if (m != lpMidi || m->getCable() != LP_MIDI_CABLE) continue;
      uint8_t type = m->getType();
      uint8_t data1 = m->getData1();
      uint8_t data2 = m->getData2();
      if (type == MIDIDeviceBase::NoteOn && data2 > 0) {
        uint8_t row, col;
        if (lpNoteToCell(data1, row, col)) lpHandlePad(row, col);
      }
    }
  };
  drain(lpMidiA);

  if (lpConnectStep >= 10 && lpPaintTimer >= 80 && (lpPaintDirty || lpStateChanged())) {
    lpPaint();
  }
}

#else

void launchpadHostBegin() {}
void launchpadHostService() {}

#endif
