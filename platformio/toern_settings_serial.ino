#include "src/toern_settings_config.h"

// CONFIG is independent of REMOTE and works in builds with MCP disabled too.
// GET -> current settings/CRC; STOP -> quiet audio; SET <CRC> <120 hex digits>
// -> validate entire block, persist, apply, read back. No wizard UI on device.
FLASHMEM static void settingsConfigError(const char *error) {
  Serial.printf("CONFIG {\"ok\":false,\"error\":\"%s\"}\n", error);
}

FLASHMEM void handleSettingsConfigCommand(char *args) {
  static_assert(SETTINGS_EEPROM_BLOCK_LEN == toern_config::count, "Update USB settings schema when EEPROM changes");
  if (!strcmp(args, "STOP")) {
    if (isRecording || fastRecordActive) { settingsConfigError("RECORDING"); return; }
    pause(true);
    allOff();
    stopAllSetWavPreviewAudio();
    Serial.println("CONFIG {\"ok\":true}");
    return;
  }
  uint8_t block[toern_config::count];
  if (!isNowPlaying && sdIoAudioSettled()) serviceSettingsBackup();
  for (size_t i = 0; i < toern_config::count; ++i) block[i] = EEPROM.read(EEPROM_DATA_START + i);
  const uint32_t previousCrc = crc32_compute(block, sizeof(block));
  if (!strcmp(args, "GET")) {
    char hex[toern_config::count * 2 + 1];
    encodeHexBytes(block, sizeof(block), hex, sizeof(hex));
    Serial.printf("CONFIG {\"ok\":true,\"protocol\":1,\"firmware\":\"%s\",\"hex\":\"%s\",\"crc\":\"%08lX\",\"playing\":%s}\n",
      VERSION, hex, (unsigned long)previousCrc, isNowPlaying ? "true" : "false");
    return;
  }
  if (strncmp(args, "SET ", 4)) { settingsConfigError("COMMAND"); return; }
  if (isRecording || fastRecordActive || isNowPlaying || !sdIoAudioSettled()) {
    settingsConfigError("BUSY"); return;
  }
  const char *crc = args + 4;
  const char *hex = strchr(crc, ' ');
  if (!hex || hex - crc != 8 || strlen(hex + 1) != sizeof(block) * 2) { settingsConfigError("FORMAT"); return; }
  for (int i = 0; i < 8; ++i) if (hexNibble(crc[i]) < 0) { settingsConfigError("FORMAT"); return; }
  if ((uint32_t)strtoul(crc, nullptr, 16) != previousCrc) { settingsConfigError("CHANGED"); return; }
  if (!decodeHexBytes(hex + 1, block, sizeof(block)) || !toern_config::valid(block)) {
    settingsConfigError("VALUES"); return;
  }
  // Physical layout is independent of user preferences and must stay intact.
  if (block[13] != EEPROM.read(EEPROM_DATA_START + 13)) { settingsConfigError("LAYOUT"); return; }
  for (size_t i = 0; i < sizeof(block); ++i) EEPROM.write(EEPROM_DATA_START + i, block[i]);
  setTouchyEnabled(false);
  loadMenuFromEEPROM();
  // A new PMODE/FLOW selection must not inherit a queued page or old mute view.
  pendingPage = 0;
  lastFlowPage = 0;
  unmuteAllChannels();
  applyMutesAfterPMODSwitch();
  // Boot normally loads these before loadMenuFromEEPROM().
  GLOB.vol = block[17];
  micGain = block[9];
  previewVol = block[7];
  previewTriggerMode = block[20];
  lineOutLevelSetting = block[15];
  sgtl5000_1.inputSelect(recInput);
  loadColorSchemeAndBrightness();
  applyAudioSettingsFromGlobals();
  resetMidiClockState();
  markSettingsBackupDirty();
  flushSettingsBackupNow();
  for (size_t i = 0; i < sizeof(block); ++i) {
    if (EEPROM.read(EEPROM_DATA_START + i) != block[i]) { settingsConfigError("VERIFY"); return; }
  }
  bypassModeSwitchDebounce = true;
  switchMode(&draw);
  // switchMode(draw) can be a same-mode refresh: apply CTRL's new physical
  // encoder range explicitly, and start the screensaver timeout from now.
  refreshCtrlEncoderConfig();
  noteUserActivity();
  bypassModeSwitchDebounce = false;
  Serial.printf("CONFIG {\"ok\":true,\"crc\":\"%08lX\",\"backup\":%s}\n",
    (unsigned long)crc32_compute(block, sizeof(block)), settingsBackupDirty ? "false" : "true");
}
