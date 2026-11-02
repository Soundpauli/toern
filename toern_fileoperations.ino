#include "src/pattern_transaction.h"
static bool lastPatternSaveOk = false;
static bool patternStageOnly = false;
static const char *AUTOSAVE_PATH = "autosaved.txt";
static const uint8_t PITCH_EXTENSION_HEADER[8] = {
  'T', 'P', 'I', 'T', 1, 0x10, 0x00, 0
}; // magic, version 1, 4096 entries (big-endian), reserved
bool g_enterNewFileAfterCorruptAutoload = false;

FLASHMEM void clearPatternNotes() {
  for (unsigned int x = 0; x <= maxlen; x++) {
    for (unsigned int y = 0; y <= maxY; y++) {
      note[x][y].channel = 0;
      note[x][y].velocity = defaultVelocity;
      note[x][y].probability = 100;
      note[x][y].condition = 1;
      note[x][y].midiPitch = NOTE_MIDI_PITCH_NONE;
    }
  }
}

FLASHMEM void deleteAutosaveFile() {
  if (SD.exists(AUTOSAVE_PATH)) SD.remove(AUTOSAVE_PATH);
  SD.remove("autosaved.txt.tmp");
  SD.remove("autosaved.txt.bak");
}

static bool patternWriteAll(File &f, const uint8_t *data, size_t len) {
  extern void sdIoYield();
  while (len > 0) {
    size_t n = f.write(data, len);
    if (n == 0) return false;
    data += n;
    len -= n;
    sdIoYield();
  }
  return true;
}

// Validate without changing live notes/settings. Legacy patterns have no pitch tail.
FLASHMEM bool validPatternFile(const char *path, bool requirePitch) {
  File f = SD.open(path);
  if (!f) return false;
  const size_t notes = (size_t)(maxlen - 1) * maxY;
  const size_t base = notes * 4u + 2u + sizeof(SMP);
  const size_t total = base + sizeof(PITCH_EXTENSION_HEADER) + notes;
  bool ok = f.size() >= (requirePitch ? total : base);
  if (ok) {
    ok = f.seek(notes * 4u) && f.read() == 0xFF && f.read() == 0xFE;
  }
  if (ok && requirePitch) {
    uint8_t header[sizeof(PITCH_EXTENSION_HEADER)];
    ok = f.seek(base) && f.read(header, sizeof(header)) == (int)sizeof(header)
      && memcmp(header, PITCH_EXTENSION_HEADER, sizeof(header)) == 0;
  }
  // Check readability of the entire required payload, including settings.
  if (ok) {
    ok = f.seek(0);
    uint8_t buf[512];
    size_t left = requirePitch ? total : base;
    while (ok && left) {
      const size_t count = min(left, sizeof(buf));
      ok = f.read(buf, count) == (int)count;
      left -= count;
      sdIoYield();
    }
  }
  f.close();
  return ok;
}

FLASHMEM void showPatternResult(bool success) {
  storageTransferFinish(success,false);
}

// FN+Save targets the selected numbered family, independently of the live slot.
FLASHMEM void deleteSelectedPatternFamily(unsigned target) {
  beginPatternFamilyDelete(target);
}

FLASHMEM void savePattern(bool autosave) {
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();

  lastPatternSaveOk = false;
  if (!autosave && !subpatternQuietSave
      && (subpatternReady || subpatternOldSavePending || subpatternSaveNew)) return;
  SMP.file = loadedPatternFile();
  const auto *savedNotes = subpatternSavingPrevious ? subpatternOutgoingGrid : note;
  const uint8_t savedSlot = subpatternFile == SMP.file
      ? (subpatternSavingPrevious ? subpatternPreviousSlot : subpatternSlot) : 0;
  // A deleted blank slot stays absent when a paused switch saves its outgoing grid.
  // Explicit Save, or adding notes, creates it again. Session recovery still saves.
  if (!autosave && subpatternQuietSave && (deletedSubpatternSlots & (1u << savedSlot))) {
    bool blank = true;
    for (unsigned x = 1; x < maxlen && blank; ++x)
      for (unsigned y = 1; y <= maxY; ++y)
        if (savedNotes[x][y].channel) { blank = false; break; }
    if (blank) { lastPatternSaveOk = true; return; }
  }
  // PATT slot 0 is autosaved.txt — load-only from the FILE menu.
  if (!autosave && SMP.file == 0) {
    return;
  }

  drawNoSD();
  // Keep Audio library fed for the whole SD burst — manual saves while playing
  // and pause-autosaves both starve the pool if chunks run too large.
  sdIoBeginAudioSafe();
  stopSdPreviewIfPlaying();
  
  // Only clear display for manual saves, not autosaves
  if (!autosave && !subpatternQuietSave) {
    FastLEDclear();
  }
  

  char OUTPUTf[50];
  if (autosave) {
    sprintf(OUTPUTf, "%s", AUTOSAVE_PATH);
  } else {
    patternDestination(OUTPUTf, sizeof(OUTPUTf), SMP.file);
  }

  char tmpPath[64], backupPath[64];
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", OUTPUTf);
  snprintf(backupPath, sizeof(backupPath), "%s.bak", OUTPUTf);
  recoverPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);
  // Do not discard the only recoverable generation after failed recovery.
  if (!validPatternFile(OUTPUTf, false) && (validPatternFile(tmpPath, true) || validPatternFile(backupPath, false))) {
    sdIoEndAudioSafe();
    if (!autosave && !subpatternQuietSave) { showPatternResult(false); switchMode(&draw); }
    return;
  }
  const bool showProgress=!autosave&&!subpatternQuietSave;
  const size_t visualTotal=(maxlen-1)*maxY*5u+2u+sizeof(SMP)+16u;
  if(showProgress)storageTransferBegin(false,true);
  File saveFile = SD.open(tmpPath, FILE_WRITE_BEGIN);
  if (saveFile && !saveFile.truncate(0)) saveFile.close();
  bool wroteOk = false;
  if (saveFile) {
    uint8_t buf[512];
    size_t bufLen = 0;
    unsigned rowCount = 0;
    wroteOk = true;

    for (unsigned int sdx = 1; sdx < maxlen && wroteOk; sdx++) {
      for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
        if (bufLen + 4 > sizeof(buf)) {
          if (!patternWriteAll(saveFile, buf, bufLen)) {
            wroteOk = false;
            break;
          }
          if(showProgress)storageTransferProgress((unsigned)((uint64_t)rowCount*4u*90u/visualTotal));
          bufLen = 0;
        }
        buf[bufLen++] = savedNotes[sdx][sdy].channel;
        buf[bufLen++] = savedNotes[sdx][sdy].velocity;
        buf[bufLen++] = savedNotes[sdx][sdy].probability;
        buf[bufLen++] = savedNotes[sdx][sdy].condition;
        if ((++rowCount & 0x1Fu) == 0u) {
          sdIoYield();
        }
      }
    }
    if (wroteOk && bufLen > 0) {
      wroteOk = patternWriteAll(saveFile, buf, bufLen);
      bufLen = 0;
    }

    if (wroteOk) {
      const uint8_t marker[2] = { 0xFF, 0xFE };
      wroteOk = patternWriteAll(saveFile, marker, 2);
    }

    for (int ch = 0; ch < maxY; ch++) {
      SMP.globalMutes[ch] = globalMutes[ch];
      for (int page = 0; page < maxPages; page++) {
        SMP.pageMutes[page][ch] = pageMutes[page][ch];
      }
    }

    if (wroteOk) {
      const uint8_t *smpBytes = (const uint8_t *)&SMP;
      size_t smpLeft = sizeof(SMP);
      size_t smpOff = 0;
      const size_t smpChunk = 256;
      while (smpLeft > 0 && wroteOk) {
        size_t n = min(smpChunk, smpLeft);
        wroteOk = patternWriteAll(saveFile, smpBytes + smpOff, n);
        smpOff += n;
        smpLeft -= n;
        if(showProgress)storageTransferProgress((unsigned)(((uint64_t)(maxlen-1)*maxY*4u+2u+smpOff)*90u/visualTotal));
      }
    }

    // Optional v1 pitch extension. It follows the unchanged legacy note blob,
    // marker, and SMP payload, so older firmware can safely ignore it.
    if (wroteOk) {
      wroteOk = patternWriteAll(saveFile, PITCH_EXTENSION_HEADER,
                                sizeof(PITCH_EXTENSION_HEADER));
    }
    if (wroteOk) {
      size_t pitchLen = 0;
      for (unsigned int sdx = 1; sdx < maxlen && wroteOk; sdx++) {
        for (unsigned int sdy = 1; sdy <= maxY; sdy++) {
          buf[pitchLen++] = savedNotes[sdx][sdy].midiPitch;
          if (pitchLen == sizeof(buf)) {
            wroteOk = patternWriteAll(saveFile, buf, pitchLen);
            pitchLen = 0;
            if(showProgress)storageTransferProgress((unsigned)(((uint64_t)(maxlen-1)*maxY*4u+2u+sizeof(SMP)+8u+(sdx-1)*maxY+sdy)*90u/visualTotal));
            sdIoYield();
          }
        }
      }
      if (wroteOk && pitchLen > 0) {
        wroteOk = patternWriteAll(saveFile, buf, pitchLen);
      }
    }
    // Optional session identity tail: old firmware safely ignores it.
    if (wroteOk) {
      const uint8_t identity[8] = {'T','S','U','B',1,
        savedSlot,
        0, (uint8_t)(autosave ? deletedSubpatternSlots : 0)};
      wroteOk = patternWriteAll(saveFile, identity, sizeof(identity));
    }
    saveFile.close();
  }

  if (wroteOk) {
    if(showProgress)storageTransferProgress(95);
    wroteOk = patternStageOnly ? validPatternFile(tmpPath, true)
        : commitPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);
  } else {
    // Only this destination's incomplete temporary file is disposable.
    SD.remove(tmpPath);
  }
  lastPatternSaveOk = wroteOk;
  if (wroteOk && !autosave && !patternStageOnly) {
    deletedSubpatternSlots &= ~(1u << savedSlot);
    ++patternStorageRevision;
  }
  if (wroteOk && !autosave && !patternStageOnly && subpatternFile != SMP.file) {
    subpatternFile = SMP.file; subpatternSlot = 0;
    subpatternNotePagesLoaded();
  }
  if (!autosave && !subpatternQuietSave) {
    if (wroteOk) {
      showPatternResult(true);
      switchMode(&draw);
    } else {
      showPatternResult(false);
      switchMode(&draw);
    }
  }
  
  // Reset paint/unpaint prevention flag after savePattern operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
  
  sdIoYield();
  sdIoEndAudioSafe();
}

FLASHMEM void saveSelectedPatternFamily(unsigned target, bool overwrite) {
  beginPatternFamilySave(target, overwrite);
}

FLASHMEM void requestPatternSave(unsigned target) {
  if (currentMode != &loadSaveTrack || target < 1 || target > 999 || fastRecordActive || isRecording) return;
  queuePatternFamilySave(target);
}

#include "src/toern_kit_writer.h"
static DMAMEM uint8_t kitSaveSector[toern_kit_io::transferBytes];
static toern_kit_io::Writer<File> kitSaveJob(kitSaveSector);
static unsigned kitSaveTarget=0,kitSaveSourcePack=0;
static uint32_t kitSaveFeedbackUntil=0;
static bool kitSaveFeedbackOk=false,kitSaveChecking=false,kitSaveSourceChanged=false;
static unsigned kitSaveCheckVoice=1,kitSaveCheckPart=0;
static void confirmKitSave(unsigned target);
bool kitSavePending(){return kitSaveChecking||kitSaveJob.busy();}
unsigned kitSaveProgress(){return kitSaveChecking?0:kitSaveJob.busy()?kitSaveJob.progress():100;}
unsigned kitSaveFeedback(){
  if(kitSavePending())return 1;
  return kitSaveFeedbackUntil&&(int32_t)(millis()-kitSaveFeedbackUntil)<0?(kitSaveFeedbackOk?2:3):0;
}
void cancelKitSaveForSampleChange(unsigned ch) {
  if(ch<1||ch>8)return;
  kitSaveChecking=false;kitSaveSourceChanged=true;kitSaveJob.cancel();
}

// Begin only captures pointers/lengths. Disk work runs one operation per loop,
// with interrupts enabled and no stop/pause or EEPROM writes at completion.
void beginKitSave(int pack) {
  if(pack<0||pack>999||kitSaveJob.busy()||fastRecordActive||isRecording)return;
  toern_kit_io::Source sources[8];
  // Save what is actually loaded, including a live custom sample whose deferred
  // SP0 disk copy has not run yet. Never read that stale scratch file instead.
  for(unsigned ch=1;ch<=8;++ch)sources[ch-1]={reinterpret_cast<const int16_t*>(sampled[ch]),loadedSampleLen[ch]};
  if(!kitSaveJob.begin((unsigned)pack,sources,FILE_WRITE_BEGIN))return;
  storageTransferBegin(true,true);
  kitSaveChecking=false;kitSaveSourceChanged=false;kitSaveTarget=(unsigned)pack;kitSaveSourcePack=samplePackID;
  kitSaveFeedbackUntil=0;
}

void serviceKitSave() {
  if(kitSaveChecking) {
    if(currentMode!=&set_SamplePack||currentMode->pos[3]!=kitSaveTarget||samplePackID!=kitSaveSourcePack
        ||fastRecordActive||isRecording){kitSaveChecking=false;return;}
    char path[40];snprintf(path,sizeof(path),"%u/%u.wav%s",kitSaveTarget,kitSaveCheckVoice,kitSaveCheckPart?".kit-bak":"");
    const bool occupied=SD.exists(path);
    if(occupied){kitSaveChecking=false;confirmKitSave(kitSaveTarget);}
    else if(++kitSaveCheckPart==2) {
      kitSaveCheckPart=0;
      if(++kitSaveCheckVoice==9){kitSaveChecking=false;saveSamplePack((int)kitSaveTarget);}
    }
    return;
  }
  if(!kitSaveJob.busy()) {
    if(kitSaveFeedbackUntil&&(int32_t)(millis()-kitSaveFeedbackUntil)>=0) {
      kitSaveFeedbackUntil=0;
      if(currentMode==&set_SamplePack&&currentMode->pos[3]==kitSaveTarget)switchMode(&draw);
    }
    return;
  }
  if(samplePackID!=kitSaveSourcePack||fastRecordActive||isRecording||kitSaveSourceChanged)kitSaveJob.cancel();
  const auto result=kitSaveJob.tick(SD);
  if(storageTransferIs(true,true))storageTransferProgress(kitSaveJob.progress());
  // One bounded operation, then back to normal MIDI/encoder/audio housekeeping.
  if(kitSaveJob.busy())return;
  preventPaintUnpaint=false;
  if(result==toern_kit_io::Result::Cancelled){if(storageTransferIs(true,true))storageTransferFinish(false,false);return;}
  kitSaveFeedbackOk=result==toern_kit_io::Result::Saved;
  kitSaveFeedbackUntil=millis()+700;
  if(storageTransferIs(true,true))storageTransferFinish(kitSaveFeedbackOk,false);
  if(kitSaveFeedbackOk&&samplePackID==kitSaveSourcePack
      &&!fastRecordActive&&!isRecording&&!kitSaveSourceChanged) {
    for(unsigned ch=1;ch<=8;++ch)SMP.sp0Active[ch]=false;
    samplePackID=kitSaveTarget;
    if(currentMode!=&set_SamplePack||currentMode->pos[3]==kitSaveTarget)SMP.pack=kitSaveTarget;
    extern void queueKitIdentityPersist();
    queueKitIdentityPersist();
  }
}

// Scan both primaries and recovery generations cooperatively, before showing
// the existing overwrite prompt. No SD calls run in the button handler.
FLASHMEM void requestKitSave(unsigned target) {
  if(currentMode!=&set_SamplePack||target>999||fastRecordActive||isRecording||kitSavePending())return;
  kitSaveTarget=target;kitSaveSourcePack=samplePackID;kitSaveCheckVoice=1;kitSaveCheckPart=0;
  kitSaveChecking=true;kitSaveFeedbackUntil=0;
}

static void confirmKitSave(unsigned target) {
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

FLASHMEM void saveSamplePack(int pack) {
  beginKitSave(pack);
}

FLASHMEM void loadPattern(bool autoload, unsigned targetFile) {
  if (!autoload && targetFile > 1000) return;
  if (!autoload && (patternFamilyBusy() || subpatternOldSavePending || (subpatternSaveNew && !subpatternReady))) { subpatternFailure(); return; }
  extern bool isNowPlaying;
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();

  subpatternReady = false; subpatternApplied = false; subpatternSelector = false; subpatternSaveNew = false;
  subpatternCancelPreview();
  // Startup must never consult the not-yet-restored PSRAM identity.
  const unsigned requestedFile = autoload ? 0u : (targetFile <= 999 ? targetFile : loadedPatternFile());
  const uint8_t requestedSlot = (subpatternFile == requestedFile) ? subpatternSlot : 0;
  uint8_t identity[8] = {};
  drawNoSD();
  stopSdPreviewIfPlaying();
  // Menu load runs with I2S DMA + encoder/fader I2C already live. Byte-at-a-time
  // SD reads deadlock the eDMA (hang, no CrashReport). Autoload is before initEncoders.
  if (!autoload) {
    sdIoBeginAudioSafe();
  }

  FastLEDclear();
  char OUTPUTf[50];
  // autoload, or FILE menu slot 0 → autosaved.txt
  if (autoload || requestedFile == 0) {
    sprintf(OUTPUTf, "%s", AUTOSAVE_PATH);
  } else {
    toern_pattern_slots::path(OUTPUTf, sizeof(OUTPUTf), requestedFile, requestedSlot);
  }

  char tmpPath[64], backupPath[64];
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", OUTPUTf);
  snprintf(backupPath, sizeof(backupPath), "%s.bak", OUTPUTf);
  recoverPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);
  if (!autoload) storageTransferBegin(false,false);
  bool loadedOk = false;
  const size_t expectedNotes = (size_t)(maxlen - 1) * (size_t)maxY;

  // Stage a complete payload in PSRAM. Failed reads never partly replace the
  // live notes or Device settings. This does not change the on-card format.
  static EXTMEM uint8_t payload[(maxlen - 1) * maxY * 5u + 2u
                              + sizeof(SMP) + sizeof(PITCH_EXTENSION_HEADER)];
  const size_t baseBytes = expectedNotes * 4u + 2u + sizeof(SMP);
  File loadFile = SD.open(OUTPUTf);
  bool pitchPresent = false;
  if (loadFile) {
    size_t offset = 0;
    bool readOk = loadFile.size() >= baseBytes;
    while (readOk && offset < baseBytes) {
      const size_t count = min((size_t)512, baseBytes - offset);
      readOk = loadFile.read(payload + offset, count) == (int)count;
      offset += count;
      if(!autoload)storageTransferProgress((unsigned)((uint64_t)offset*90u/std::max(baseBytes,(size_t)loadFile.size())));
      sdIoYield();
    }
    readOk = readOk && payload[expectedNotes * 4u] == 0xFF
                    && payload[expectedNotes * 4u + 1u] == 0xFE;
    if (readOk && loadFile.available() >= (int)(sizeof(PITCH_EXTENSION_HEADER) + expectedNotes)) {
      pitchPresent = loadFile.read(payload + baseBytes, sizeof(PITCH_EXTENSION_HEADER))
                       == (int)sizeof(PITCH_EXTENSION_HEADER)
        && memcmp(payload + baseBytes, PITCH_EXTENSION_HEADER, sizeof(PITCH_EXTENSION_HEADER)) == 0;
      size_t pitchOffset = 0;
      while (pitchPresent && pitchOffset < expectedNotes) {
        const size_t count = min((size_t)512, expectedNotes - pitchOffset);
        pitchPresent = loadFile.read(payload + baseBytes + sizeof(PITCH_EXTENSION_HEADER)
                                     + pitchOffset, count) == (int)count;
        pitchOffset += count;
        if(!autoload)storageTransferProgress((unsigned)((uint64_t)(baseBytes+sizeof(PITCH_EXTENSION_HEADER)+pitchOffset)*90u/std::max(baseBytes,(size_t)loadFile.size())));
        sdIoYield();
      }
    }
    if (readOk && pitchPresent && loadFile.available() >= 8)
      loadFile.read(identity, sizeof(identity));
    loadFile.close();
    if (readOk) {
      clearPatternNotes();
      size_t index = 0;
      for (unsigned int x = 1; x < maxlen; x++) {
        for (unsigned int y = 1; y <= maxY; y++, index++) {
          const uint8_t *cell = payload + index * 4u;
          note[x][y].channel = cell[0] <= 16 ? cell[0] : 0;
          note[x][y].velocity = min(cell[1], (uint8_t)127);
          note[x][y].probability = min(cell[2], (uint8_t)100);
          note[x][y].condition = cell[3];
          const uint8_t pitch = pitchPresent
            ? payload[baseBytes + sizeof(PITCH_EXTENSION_HEADER) + index] : NOTE_MIDI_PITCH_NONE;
          note[x][y].midiPitch = (pitch <= 127 || pitch == NOTE_MIDI_PITCH_NONE)
            ? pitch : NOTE_MIDI_PITCH_NONE;
        }
      }
      // OFF keeps both the live audio and its editable/savable settings intact.
      // Startup recovery always restores the complete session.
      static EXTMEM Device previousSound;
      const bool loadSound = autoload || SMP_LOAD_SETTINGS;
      if (!loadSound) memcpy(&previousSound, &SMP, sizeof(SMP));
      memcpy(&SMP, payload + expectedNotes * 4u + 2u, sizeof(SMP));
      if (!loadSound) {
        SMP.pack = previousSound.pack;
        memcpy(SMP.wav, previousSound.wav, sizeof(SMP.wav));
        memcpy(SMP.samplePathRel, previousSound.samplePathRel, sizeof(SMP.samplePathRel));
        memcpy(SMP.sp0Active, previousSound.sp0Active, sizeof(SMP.sp0Active));
        memcpy(SMP.param_settings, previousSound.param_settings, sizeof(SMP.param_settings));
        memcpy(SMP.filter_settings, previousSound.filter_settings, sizeof(SMP.filter_settings));
        memcpy(SMP.synth_settings, previousSound.synth_settings, sizeof(SMP.synth_settings));
        memcpy(SMP.channelVol, previousSound.channelVol, sizeof(SMP.channelVol));
      }
      if (!isfinite(SMP.bpm) || SMP.bpm < 40.0f || SMP.bpm > 300.0f) SMP.bpm = 100.0f;
      if (SMP.file > 999) SMP.file = 1;
      if (SMP.pack > 999) SMP.pack = 1;
      for (int ch = 0; ch < maxY; ch++) {
        globalMutes[ch] = SMP.globalMutes[ch];
        for (int page = 0; page < maxPages; page++) pageMutes[page][ch] = SMP.pageMutes[page][ch];
      }
      unmuteAllChannels();
      applyMutesAfterPMODSwitch();
      subpatternFile = autoload || requestedFile == 0 ? SMP.file : requestedFile;
      // Byte 6 of TSUB used to gate a lettered family; it is now reserved.
      // Slot identity is independent of whether an A file has been saved.
      subpatternSlot = autoload || requestedFile == 0
          ? (memcmp(identity, "TSUB", 4) == 0 && identity[4] == 1
              && identity[5] < toern_pattern_slots::count ? identity[5] : 0)
          : requestedSlot;
      SMP.file = subpatternFile;
      pendingNewPatternFile = 0;
      deletedSubpatternSlots = (autoload || requestedFile == 0)
          && memcmp(identity, "TSUB", 4) == 0 && identity[4] == 1 ? identity[7] & 0x1F : 0;
      loadedOk = true;
    }
    sdIoYield();
  }

  if (!loadedOk) {
    // EXTMEM/PSRAM is not a blank canvas after reset — always zero the grid.
    if (autoload) {
      clearPatternNotes();
      SMP.file = 1;
      resetFileSubpattern();
    }
    const bool fileWasPresent = SD.exists(OUTPUTf);
    if (!autoload) sdIoEndAudioSafe();
    if (fileWasPresent) {
      extern void FastLEDclear();
      extern void FastLEDshow();
      extern void drawText(const char *text, int startX, int startY, CRGB color);
      showPatternResult(false);
      if (autoload) {
        g_enterNewFileAfterCorruptAutoload = true;
      } else {
        switchMode(&draw);
      }
    } else if (!autoload && requestedFile != 0) {
      storageTransferDismiss();
      pendingNewPatternFile = requestedFile;
      extern void showNewFileScreen();
      showNewFileScreen();
    } else if(!autoload) {
      storageTransferDismiss();
    }
    extern bool preventPaintUnpaint;
    preventPaintUnpaint = false;
    return;
  }
  
  // Reset basic runtime flags when loading a pattern
  GLOB.singleMode = false;
  
  // Validate BPM before using (default to 100 if invalid)
  if (!isfinite(SMP.bpm) || SMP.bpm < 40.0f || SMP.bpm > 300.0f) {
    SMP.bpm = 100.0f;
  }
  
  syncBpmEncoderPositions();
  playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
  playTimer.update(playNoteInterval);
  fillTimer.update(playNoteInterval / 4.0);
  bpmMode.pos[2] = (clockMode == 1) ? 1 : 0;

  // Reset paint/unpaint prevention flag after loadPattern operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;

  updateLastPage();
  subpatternNotePagesLoaded();
  
  // Load SMP settings
  if (autoload || SMP_LOAD_SETTINGS) loadSMPSettings();
  // samplePathRel/sp0Active are valid now (kit load on boot ran earlier with empty paths).
  extern void syncVoiceLoadedPathsFromStored();
  syncVoiceLoadedPathsFromStored();
  
  if (!autoload) {
    storageTransferFinish(true,false);
    switchMode(&draw);
    applyDrawSingleYEncoderLimits();
    refreshCtrlEncoderConfig();
  }
  
  sdIoYield();
  if (!autoload) sdIoEndAudioSafe();
}

FLASHMEM void autoLoad() {
  // loadPattern recovers only autosaved.txt.tmp / .bak. The historical shared
  // pattern.tmp has no destination identity and must never become an autosave.
  loadPattern(true);
  
  // Reset paint/unpaint prevention flag after autoLoad operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}

static unsigned autoSaveWriteSerial=0;
FLASHMEM void autoSave() {
  // Only skip if we already saved within the last 5 seconds.
  static unsigned long lastAutoSaveTime = 0;
  const unsigned long AUTO_SAVE_COOLDOWN_MS = 5000;

  unsigned long currentTime = millis();
  if (lastAutoSaveTime > 0 && (currentTime - lastAutoSaveTime) < AUTO_SAVE_COOLDOWN_MS) {
    return;
  }

  savePattern(true);
  ++autoSaveWriteSerial;
  if (lastPatternSaveOk) lastAutoSaveTime = currentTime;

  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}

void autoSaveWithFeedback() {
  const unsigned before=autoSaveWriteSerial;
  autoSave();
  if(autoSaveWriteSerial!=before)storageTransferFinish(lastPatternSaveOk,true);
}

// ---- Voice filter-page presets (PSET) for ch11 / 13 / 14 ----
// SD layout: presets/<ch>/<nnn>.pset  (nnn = 001..099)

static bool isVoicePresetChannel(int ch) {
  return ch == 11 || ch == 13 || ch == 14;
}

static uint16_t g_presetSlotByChannel[16] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static bool g_presetUiDirty = false;

void invalidateVoicePresetUi() { g_presetUiDirty = true; }
bool takeVoicePresetUiDirty() {
  if (!g_presetUiDirty) return false;
  g_presetUiDirty = false;
  return true;
}

uint16_t getVoicePresetSlot(int ch) {
  if (!isVoicePresetChannel(ch)) return 1;
  uint16_t s = g_presetSlotByChannel[ch];
  if (s < 1) s = 1;
  if (s > 99) s = 99;
  return s;
}

void setVoicePresetSlot(int ch, uint16_t slot) {
  if (!isVoicePresetChannel(ch)) return;
  if (slot < 1) slot = 1;
  if (slot > 99) slot = 99;
  g_presetSlotByChannel[ch] = slot;
}

static bool buildVoicePresetPath(int ch, uint16_t slot, char *out, size_t outSize) {
  if (!out || outSize < 24 || !isVoicePresetChannel(ch)) return false;
  if (slot < 1) slot = 1;
  if (slot > 99) slot = 99;
  snprintf(out, outSize, "presets/%d/%03u.pset", ch, (unsigned)slot);
  return true;
}

static bool ensureVoicePresetDirs(int ch) {
  if (!SD.exists("presets")) {
    if (!SD.mkdir("presets")) return false;
  }
  char dir[24];
  snprintf(dir, sizeof(dir), "presets/%d", ch);
  if (!SD.exists(dir)) {
    if (!SD.mkdir(dir)) return false;
  }
  return true;
}

static uint8_t readPresetSettingValue(int ch, SettingArray arr, int idx) {
  switch (arr) {
    case ARR_FILTER:
      return (uint8_t)constrain((int)lroundf(filterSetting(ch, idx)), 0, 255);
    case ARR_SYNTH:
      if (idx < 0 || idx >= 11) return 0;
      return (uint8_t)constrain((int)lroundf(SMP.synth_settings[ch][idx]), 0, 255);
    case ARR_PARAM:
      if (idx < 0 || idx >= PARAM_COUNT) return 0;
      return (uint8_t)constrain((int)lroundf(SMP.param_settings[ch][idx]), 0, 255);
    default:
      return 0;
  }
}

static void writePresetSettingValue(int ch, SettingArray arr, int idx, uint8_t value) {
  switch (arr) {
    case ARR_FILTER:
      filterSetting(ch, idx) = value;
      break;
    case ARR_SYNTH:
      if (idx >= 0 && idx < 11) SMP.synth_settings[ch][idx] = value;
      break;
    case ARR_PARAM:
      if (idx >= 0 && idx < PARAM_COUNT) SMP.param_settings[ch][idx] = value;
      break;
    default:
      break;
  }
}

static bool voicePresetFileValid(const char *path, bool /*allowTmp*/) {
  if (!path || !path[0] || !SD.exists(path)) return false;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  uint8_t hdr[4];
  bool ok = (f.read(hdr, 4) == 4) &&
            hdr[0] == 'T' && hdr[1] == 'P' && hdr[2] == 'S' && hdr[3] == 'T';
  f.close();
  return ok;
}

bool voicePresetExists(int ch, uint16_t slot) {
  char path[40];
  if (!buildVoicePresetPath(ch, slot, path, sizeof(path))) return false;
  return voicePresetFileValid(path, false);
}

FLASHMEM bool saveVoicePreset(int ch, uint16_t slot) {
  if (!isVoicePresetChannel(ch)) return false;
  if (!ensureVoicePresetDirs(ch)) return false;

  extern const uint8_t filterPageCount[NUM_CHANNELS];
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();
  extern void sdIoYield();

  char path[40];
  char tmpPath[48];
  char bakPath[48];
  if (!buildVoicePresetPath(ch, slot, path, sizeof(path))) return false;
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
  snprintf(bakPath, sizeof(bakPath), "%s.bak", path);

  struct Entry {
    uint8_t arr;
    uint8_t idx;
    uint8_t value;
  };
  Entry entries[48];
  uint8_t count = 0;

  auto pushEntry = [&](SettingArray arr, int idx) {
    if (count >= 48) return;
    if (arr == ARR_NONE || idx < 0) return;
    if (arr == ARR_PARAM && idx >= PARAM_COUNT) return;
    if (arr == ARR_SYNTH && idx >= 11) return;
    for (uint8_t i = 0; i < count; i++) {
      if (entries[i].arr == (uint8_t)arr && entries[i].idx == (uint8_t)idx) {
        entries[i].value = readPresetSettingValue(ch, arr, idx);
        return;
      }
    }
    entries[count].arr = (uint8_t)arr;
    entries[count].idx = (uint8_t)idx;
    entries[count].value = readPresetSettingValue(ch, arr, idx);
    count++;
  };

  int pages = filterPageCount[ch];
  if (pages <= 0) pages = 4;
  if (pages > FILTER_PAGE_CAP) pages = FILTER_PAGE_CAP;
  for (int p = 0; p < pages; p++) {
    for (int s = 0; s < 4; s++) {
      const SliderDefEntry &d = sliderDef[ch][p][s];
      pushEntry(d.arr, d.idx);
    }
  }
  if (ch == 11) pushEntry(ARR_SYNTH, FORM);

  sdIoBeginAudioSafe();
  if (SD.exists(tmpPath)) SD.remove(tmpPath);
  File f = SD.open(tmpPath, O_WRITE | O_CREAT | O_TRUNC);
  if (!f) {
    sdIoEndAudioSafe();
    return false;
  }

  const uint8_t magic[4] = { 'T', 'P', 'S', 'T' };
  const uint8_t version = 1;
  const uint8_t channelByte = (uint8_t)ch;
  bool ok = (f.write(magic, 4) == 4) &&
            (f.write(&version, 1) == 1) &&
            (f.write(&channelByte, 1) == 1) &&
            (f.write(&count, 1) == 1);
  for (uint8_t i = 0; ok && i < count; i++) {
    ok = (f.write(&entries[i].arr, 1) == 1) &&
         (f.write(&entries[i].idx, 1) == 1) &&
         (f.write(&entries[i].value, 1) == 1);
    if ((i & 7) == 0) sdIoYield();
  }
  f.close();
  if (!ok) {
    SD.remove(tmpPath);
    sdIoEndAudioSafe();
    return false;
  }

  ok = commitPatternTransaction(SD, path, tmpPath, bakPath, voicePresetFileValid);
  sdIoEndAudioSafe();
  if (ok) {
    setVoicePresetSlot(ch, slot);
    invalidateVoicePresetUi();
  }
  return ok;
}

FLASHMEM bool loadVoicePreset(int ch, uint16_t slot) {
  if (!isVoicePresetChannel(ch)) return false;

  char path[40];
  if (!buildVoicePresetPath(ch, slot, path, sizeof(path))) return false;
  if (!voicePresetFileValid(path, false)) return false;

  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();
  extern void sdIoYield();
  extern void applyMonoSynthCrf(int ch);
  extern void updateSynthVoice(int channel);
  extern void midiArpResetVoice(int ch);

  sdIoBeginAudioSafe();
  File f = SD.open(path, FILE_READ);
  if (!f) {
    sdIoEndAudioSafe();
    return false;
  }

  uint8_t magic[4];
  uint8_t version = 0;
  uint8_t fileCh = 0;
  uint8_t count = 0;
  bool ok = (f.read(magic, 4) == 4) &&
            magic[0] == 'T' && magic[1] == 'P' && magic[2] == 'S' && magic[3] == 'T' &&
            (f.read(&version, 1) == 1) && version == 1 &&
            (f.read(&fileCh, 1) == 1) &&
            (f.read(&count, 1) == 1) &&
            count <= 48;
  // Allow loading a preset saved on a sibling synth voice onto the current one.
  if (ok && fileCh != 11 && fileCh != 13 && fileCh != 14) ok = false;

  for (uint8_t i = 0; ok && i < count; i++) {
    uint8_t arr = 0, idx = 0, value = 0;
    ok = (f.read(&arr, 1) == 1) && (f.read(&idx, 1) == 1) && (f.read(&value, 1) == 1);
    if (!ok) break;
    writePresetSettingValue(ch, (SettingArray)arr, (int)idx, value);
    if ((i & 7) == 0) sdIoYield();
  }
  f.close();
  sdIoEndAudioSafe();
  if (!ok) return false;

  // Push stored values into audio hardware (same chain as filter-page edits).
  extern const uint8_t filterPageCount[NUM_CHANNELS];
  int pages = filterPageCount[ch];
  if (pages <= 0) pages = 4;
  if (pages > FILTER_PAGE_CAP) pages = FILTER_PAGE_CAP;
  for (int p = 0; p < pages; p++) {
    for (int s = 0; s < 4; s++) {
      const SliderDefEntry &d = sliderDef[ch][p][s];
      if (d.arr == ARR_NONE || d.idx < 0) continue;
      if (d.arr == ARR_FILTER) {
        setFilters((FilterType)d.idx, ch, false);
      } else if (d.arr == ARR_PARAM && d.idx < PARAM_COUNT) {
        setParams((ParameterType)d.idx, ch);
      }
    }
  }
  if (ch == 11) {
    updateSynthVoice(11);
  } else {
    applyMonoSynthCrf(ch);
  }
  midiArpResetVoice(ch);

  if (currentMode == &filterMode && (int)GLOB.currentChannel == ch) {
    extern uint8_t filterPage[NUM_CHANNELS];
    extern void initSliders(uint8_t page, uint8_t chan);
    initSliders(filterPage[ch], (uint8_t)ch);
  }

  setVoicePresetSlot(ch, slot);
  invalidateVoicePresetUi();
  return true;
}
