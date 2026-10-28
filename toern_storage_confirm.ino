// Browser confirmations are independent of file subpatterns.
FLASHMEM void requestPatternSave(unsigned target) {
  if (currentMode != &loadSaveTrack || target < 1 || target > 999 || fastRecordActive || isRecording) return;
  char path[24], tmp[32], backup[32];
  snprintf(path, sizeof(path), "%u.txt", target);
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  snprintf(backup, sizeof(backup), "%s.bak", path);
  sdIoBeginAudioSafe();
  const bool occupied = SD.exists(path) || SD.exists(tmp) || SD.exists(backup);
  sdIoEndAudioSafe();
  if (occupied) openPatternConfirmation(target, false);
  else savePatternTo(false, target);
}

FLASHMEM bool deleteSelectedPattern(unsigned target) {
  ++patternStorageRevision;
  if (target < 1 || target > 999 || fastRecordActive || isRecording) return false;
  char path[24], tmp[32], backup[32];
  snprintf(path, sizeof(path), "%u.txt", target);
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  snprintf(backup, sizeof(backup), "%s.bak", path);
  sdIoBeginAudioSafe(); stopSdPreviewIfPlaying();
  const bool ok = deletePatternTransaction(SD, path, tmp, backup);
  sdIoEndAudioSafe();
  if (ok && target == loadedPatternFileNumber) {
    toern_audio::StateGuard guard;
    clearPatternNotes(); updateLastPage(); loadedPatternFileNumber = 0;
  }
  return ok;
}

// Check every voice so a partially populated kit also requires confirmation.
FLASHMEM void requestKitSave(unsigned target) {
  if (currentMode != &set_SamplePack || target > 999 || fastRecordActive || isRecording) return;
  bool occupied = false;
  sdIoBeginAudioSafe();
  for (unsigned voice = 1; voice <= 8 && !occupied; ++voice) {
    char path[24]; snprintf(path, sizeof(path), "%u/%u.wav", target, voice);
    occupied = SD.exists(path);
  }
  sdIoEndAudioSafe();
  if (!occupied) { saveSamplePack(target); return; }
  patternConfirmReturnMode = currentMode;
  memcpy(patternConfirmReturnPos, currentMode->pos, sizeof(patternConfirmReturnPos));
  patternConfirmTarget = target;
  patternConfirmResource = 2;
  patternConfirmDelete = false;
  patternConfirmMode.pos[3] = 0;
  ++patternConfirmRevision;
  const bool wasBypassed = bypassModeSwitchDebounce;
  bypassModeSwitchDebounce = true; switchMode(&patternConfirmMode);
  bypassModeSwitchDebounce = wasBypassed;
}

// Snapshot the browser selection before encoder 4 becomes the NO/YES selector.
FLASHMEM void requestBrowserDelete() {
  if (fastRecordActive || isRecording) return;
  const bool kit = currentMode == &set_SamplePack;
  if (!kit && !captureSelectedSampleForDelete(confirmedSampleDeletePath, sizeof(confirmedSampleDeletePath))) return;
  if (kit && currentMode->pos[3] > 999) return;
  patternConfirmReturnMode = currentMode;
  memcpy(patternConfirmReturnPos, currentMode->pos, sizeof(patternConfirmReturnPos));
  patternConfirmTarget = currentMode->pos[3];
  patternConfirmResource = kit ? 2 : 1;
  patternConfirmDelete = true;
  patternConfirmMode.pos[3] = 0;
  ++patternConfirmRevision;
  const bool wasBypassed = bypassModeSwitchDebounce;
  bypassModeSwitchDebounce = true; switchMode(&patternConfirmMode);
  bypassModeSwitchDebounce = wasBypassed;
}

FLASHMEM bool deleteConfirmedBrowserResource() {
  if (fastRecordActive || isRecording) return false;
  sdIoBeginAudioSafe(); stopSdPreviewIfPlaying();
  bool ok = false;
  if (patternConfirmResource == 1) {
    const char *path = confirmedSampleDeletePath;
    // Only the snapshotted WAV is eligible; folders and '..' are never recursive targets.
    const size_t len = strlen(path);
    if (strncmp(path, "samples/", 8) == 0 && !strstr(path, "/../") && len > 12
        && (path[len-4] == '.' && (path[len-3] | 32) == 'w' && (path[len-2] | 32) == 'a' && (path[len-1] | 32) == 'v')) {
      File file = SD.open(path);
      const bool regular = file && !file.isDirectory();
      file.close();
      ok = regular && SD.remove(path);
    }
  } else if (patternConfirmResource == 2 && patternConfirmTarget <= 999) {
    char folder[12]; snprintf(folder, sizeof(folder), "%u", patternConfirmTarget);
    File dir = SD.open(folder);
    ok = dir && dir.isDirectory(); dir.close();
    if (ok) for (unsigned voice = 1; voice <= 8; ++voice) {
      char path[24]; snprintf(path, sizeof(path), "%u/%u.wav", patternConfirmTarget, voice);
      const bool removed = !SD.exists(path) || SD.remove(path);
      ok = removed && ok; sdIoYield();
    }
    if (ok) ok = SD.rmdir(folder);
  }
  sampleBrowserInvalidate();
  sdIoEndAudioSafe(); return ok;
}


FLASHMEM void drawBrowserDeleteCross(int encoder) {
  int x1, center, x3;
  getIndicatorXPositions(encoder, x1, center, x3);
  ++x1; ++center; ++x3;
  light(x1, 3, UI_RED); light(x3, 3, UI_RED);
  light(center, 4, UI_RED);
  light(x1, 5, UI_RED); light(x3, 5, UI_RED);
}

FLASHMEM void openPatternConfirmation(unsigned target, bool deleting) {
  if (target < 1 || target > 999 || fastRecordActive || isRecording) return;
  patternConfirmReturnMode = currentMode;
  memcpy(patternConfirmReturnPos, currentMode->pos, sizeof(patternConfirmReturnPos));
  ++patternConfirmRevision;
  patternConfirmResource = 0;
  patternConfirmTarget = target;
  patternConfirmDelete = deleting;
  patternConfirmMode.pos[3] = 0; // NO is always the safe initial choice.
  const bool wasBypassed = bypassModeSwitchDebounce;
  bypassModeSwitchDebounce = true; switchMode(&patternConfirmMode);
  bypassModeSwitchDebounce = wasBypassed;
}

FLASHMEM void showPatternConfirmation() {
  FastLEDclear();
  const bool accept = patternConfirmMode.pos[3] == 1;
  const char *title = patternConfirmDelete ? "DEL?" : "SAVE?";
  if (maxX < 32 && !patternConfirmDelete) {
    // Compact S/E/? glyphs fit SAVE? starting at x=2 on a narrow panel.
    static const uint8_t glyphs[3][5] = {{3,2,3,1,3}, {3,2,3,2,3}, {3,1,1,0,1}};
    static const uint8_t starts[3] = {2,13,15};
    for (unsigned glyph = 0; glyph < 3; ++glyph)
      for (unsigned row = 0; row < 5; ++row)
        for (unsigned col = 0; col < 2; ++col)
          if (glyphs[glyph][row] & (2u >> col)) light(starts[glyph] + col, 15 - row, UI_RED);
    drawText("A", 5, 11, UI_RED); drawText("V", 9, 11, UI_RED);
  } else drawText(title, 2, 11, UI_RED);
  drawText(accept ? "YES" : "NO", 2, 5, accept ? UI_GREEN : UI_BLUE);
  drawIndicator('M', accept ? 'G' : 'X', 4, false, false);
  static int8_t lastChoice = -1;
  static uint32_t lastEntry = 0;
  const int8_t choice = accept ? 1 : 0;
  if (lastChoice != choice || lastEntry != patternConfirmRevision) {
    const uint32_t rgb = accept ? 0x00FF00 : 0x0000FF;
    Encoder[1].writeRGBCode(0x000000); Encoder[3].writeRGBCode(rgb);
    lastChoice = choice; lastEntry = patternConfirmRevision;
  }
  if (maxX >= 32 && patternConfirmResource != 1) drawNumber(patternConfirmTarget, UI_RED, 11);
  FastLEDshow();
}


FLASHMEM void finishPatternConfirmation(bool accept) {
  if (currentMode != &patternConfirmMode) return;
  for (int i = 0; i < NUM_ENCODERS; ++i) pressed[i] = false;
  if (!accept) showCanceledAnimation(false);
  else if (fastRecordActive || isRecording) showPatternResult(false);
  else if (patternConfirmDelete) {
    showPatternResult(patternConfirmResource == 0
      ? deleteSelectedPattern(patternConfirmTarget) : deleteConfirmedBrowserResource());
  } else if (patternConfirmResource == 2) saveSamplePack(patternConfirmTarget);
  else savePatternTo(false, patternConfirmTarget);

  const bool returnToBrowser = patternConfirmDelete || !accept
    || (patternConfirmResource == 0 && !lastPatternSaveOk);
  if (returnToBrowser && patternConfirmReturnMode) {
    Mode *browser = patternConfirmReturnMode;
    const bool wasBypassed = bypassModeSwitchDebounce;
    bypassModeSwitchDebounce = true; switchMode(browser);
    bypassModeSwitchDebounce = wasBypassed;
    memcpy(browser->pos, patternConfirmReturnPos, sizeof(patternConfirmReturnPos));
    if (browser == &loadSaveTrack) { browser->pos[2] = 0; SMP_LOAD_SETTINGS = true; }
    if (browser == &set_Wav) restoreSampleBrowserSelection(browser->pos[3]);
    else for (int i = 0; i < NUM_ENCODERS; ++i) Encoder[i].writeCounter((int32_t)browser->pos[i]);
  } else if (currentMode != &draw) switchMode(&draw);
  for (int i = 0; i < NUM_ENCODERS; ++i) pressed[i] = false;
  preventPaintUnpaint = true;
}
