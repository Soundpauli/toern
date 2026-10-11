#include "src/pattern_transaction.h"
static bool lastPatternSaveOk = false;
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
  if (success) showSaveSuccessAnimation(false);
  else showSaveFailureAnimation(false);
}

FLASHMEM void savePattern(bool autosave) {
  extern bool isNowPlaying;
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();

  lastPatternSaveOk = false;
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
  if (!autosave) {
    FastLEDclear();
  }
  

  char OUTPUTf[50];
  if (autosave) {
    sprintf(OUTPUTf, "%s", AUTOSAVE_PATH);
  } else {
    sprintf(OUTPUTf, "%d.txt", (int)SMP.file);
  }

  char tmpPath[64], backupPath[64];
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", OUTPUTf);
  snprintf(backupPath, sizeof(backupPath), "%s.bak", OUTPUTf);
  bool empty = true;
  for (unsigned int x = 1; x < maxlen && empty; x++) {
    for (unsigned int y = 1; y <= maxY; y++) {
      if (note[x][y].channel != 0) { empty = false; break; }
    }
  }
  // Preserve the intentional gesture: saving an empty grid clears this slot.
  // Autosave still retains the empty session, rather than reviving old notes.
  if (!autosave && empty) {
    lastPatternSaveOk = deletePatternTransaction(SD, OUTPUTf, tmpPath, backupPath);
    sdIoEndAudioSafe();
    showPatternResult(lastPatternSaveOk);
    switchMode(&draw);
    return;
  }
  recoverPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);
  // Do not discard the only recoverable generation after failed recovery.
  if (!validPatternFile(OUTPUTf, false) && (validPatternFile(tmpPath, true) || validPatternFile(backupPath, false))) {
    sdIoEndAudioSafe();
    if (!autosave) { showPatternResult(false); switchMode(&draw); }
    return;
  }
  if (!autosave) { drawText("SAVE", 1, 8, UI_GREEN); FastLEDshow(); }
  File saveFile = SD.open(tmpPath, O_WRITE | O_CREAT | O_TRUNC);
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
          bufLen = 0;
        }
        buf[bufLen++] = note[sdx][sdy].channel;
        buf[bufLen++] = note[sdx][sdy].velocity;
        buf[bufLen++] = note[sdx][sdy].probability;
        buf[bufLen++] = note[sdx][sdy].condition;
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
          buf[pitchLen++] = note[sdx][sdy].midiPitch;
          if (pitchLen == sizeof(buf)) {
            wroteOk = patternWriteAll(saveFile, buf, pitchLen);
            pitchLen = 0;
            sdIoYield();
          }
        }
      }
      if (wroteOk && pitchLen > 0) {
        wroteOk = patternWriteAll(saveFile, buf, pitchLen);
      }
    }
    saveFile.close();
  }

  if (wroteOk) {
    wroteOk = commitPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);
  } else {
    // Only this destination's incomplete temporary file is disposable.
    SD.remove(tmpPath);
  }
  lastPatternSaveOk = wroteOk;
  if (!autosave) {
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


FLASHMEM void saveSamplePack(int pack) {
    extern void stopSdPreviewIfPlaying();
    extern size_t sdIoChunkSize();
    extern void sdIoYield();
    extern void sdIoBeginAudioSafe();
    extern void sdIoEndAudioSafe();

    sdIoBeginAudioSafe();
    stopSdPreviewIfPlaying();
    
    bool packSaved = true;
    char filename[64];
    char sourcePath[64];

    char dirPath[64];
    // Ensure the directory exists
    snprintf(dirPath, sizeof(dirPath), "%d", pack);
    if (!SD.exists(dirPath)) {
        packSaved = SD.mkdir(dirPath);
    }

    // Iterate over each sample channel (1..8)
    for (uint8_t ch = 1; ch <= 8 && packSaved; ch++) {
        // Use the same progress bar UI as samplepack load
        drawLoadingBar(1, maxFiles, ch, col_base[(maxFiles + 1) - ch], UI_DIM_WHITE, false);
        // Pack = samplepack icon
        showIcons(OLD_ICON_SAMPLEPACK, UI_BG_DIM);
        // Don't change FastLED global brightness - matrix is dimmed in software (light_single)
        FastLEDshow();

        // Check if this voice has a custom sample in samplepack 0
        snprintf(sourcePath, sizeof(sourcePath), "0/%d.wav", ch);
        snprintf(filename, sizeof(filename), "%d/%d.wav", pack, ch);
        
        bool useSp0Override = SMP.sp0Active[ch] && SD.exists(sourcePath);

        if (useSp0Override) {
            // Copy from samplepack 0
            // Remove existing file if it exists
            if (SD.exists(filename)) {
                if (!SD.remove(filename)) { packSaved = false; break; }
            }
            
            // Open source file for reading
            File sourceFile = SD.open(sourcePath, FILE_READ);
            if (!sourceFile) {
                packSaved = false;
                break;
            }
            
            // Open destination file for writing
            File destFile = SD.open(filename, FILE_WRITE);
            if (!destFile) {
                sourceFile.close();
                packSaved = false;
                break;
            }
            
            // Reuse EXTMEM scratch; read/write in sdIoChunkSize() slices while playing.
            static EXTMEM uint8_t packCopyBuf[65536];
            const size_t chunk = sdIoChunkSize();
            const size_t sourceSize = sourceFile.size();
            size_t copied = 0;
            int bytesRead;
            while ((bytesRead = sourceFile.read(packCopyBuf, chunk)) > 0) {
                if (!patternWriteAll(destFile, packCopyBuf, (size_t)bytesRead)) { packSaved = false; break; }
                copied += (size_t)bytesRead;
                sdIoYield();
            }
            
            packSaved = packSaved && copied == sourceSize;
            sourceFile.close();
            destFile.close();
        } else if (loadedSampleLen[ch] > 0) {
            // Save from RAM (current loaded sample)
            // Remove any existing file at this path
            if (SD.exists(filename)) {
                if (!SD.remove(filename)) { packSaved = false; break; }
            }
            
            // Open a new file for writing
            File outFile = SD.open(filename, FILE_WRITE);
            if (!outFile) {
                packSaved = false;
                break;
            }
            
            // Write standard 44-byte WAV header (mono, 16-bit, AUDIO_SAMPLE_RATE_EXACT)
            writeWavHeader(outFile, AUDIO_SAMPLE_RATE_EXACT, 16, 1);
            
            // Determine how many samples were recorded for this channel
            uint32_t sampleCount = loadedSampleLen[ch];  // number of int16_t samples
            
            // Write the raw PCM data from the sampled buffer in audio-safe chunks
            uint32_t totalBytes = sampleCount * sizeof(int16_t);
            uint8_t* dataPtr = reinterpret_cast<uint8_t*>(sampled[ch]);
            const size_t CHUNK_SIZE = sdIoChunkSize();

            for (uint32_t offset = 0; offset < totalBytes; offset += CHUNK_SIZE) {
              size_t chunkSize = min(CHUNK_SIZE, (size_t)(totalBytes - offset));
              if (!patternWriteAll(outFile, dataPtr + offset, chunkSize)) { packSaved = false; break; }
              sdIoYield();
            }

            finalizeWavHeader(outFile, totalBytes);
            
            packSaved = packSaved && outFile.size() == totalBytes + 44u;
            outFile.close();
            // Header finalisation also writes to the card. Size alone cannot
            // detect a failed RIFF/data-length update, so verify after close.
            if (packSaved) {
                File verifyFile = SD.open(filename, FILE_READ);
                uint8_t header[44] = {};
                packSaved = verifyFile && verifyFile.size() == totalBytes + 44u
                    && verifyFile.read(header, sizeof(header)) == (int)sizeof(header);
                uint32_t riffBytes = 0, pcmBytes = 0;
                memcpy(&riffBytes, header + 4, 4);
                memcpy(&pcmBytes, header + 40, 4);
                packSaved = packSaved && memcmp(header, "RIFF", 4) == 0
                    && memcmp(header + 8, "WAVE", 4) == 0
                    && riffBytes == totalBytes + 36u && pcmBytes == totalBytes;
                verifyFile.close();
            }
        } else {
            // Voices without a sample get a zero-byte placeholder file so banks
            // still contain all 8 slots and load back as silence instead of mute.
            if (SD.exists(filename)) {
                if (!SD.remove(filename)) { packSaved = false; break; }
            }

            File emptyFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);
            if (emptyFile) {
                emptyFile.close();
            } else { packSaved = false; }
        }
        sdIoYield();
    }
    
    // Clear sp0Active flags when saving a samplepack (all samples now part of this pack)
    for (uint8_t ch = 1; packSaved && ch < maxFiles; ch++) {
        SMP.sp0Active[ch] = false;
    }
    if (packSaved) saveSp0StateToEEPROM();
    
    // Reset paint/unpaint prevention flag after saveSamplePack operation
    extern bool preventPaintUnpaint;
    preventPaintUnpaint = false;
    
   sdIoYield();
   sdIoEndAudioSafe();
   showPatternResult(packSaved);
   switchMode(&draw);
}


FLASHMEM void loadPattern(bool autoload) {
  extern bool isNowPlaying;
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();

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
  if (autoload || SMP.file == 0) {
    sprintf(OUTPUTf, "%s", AUTOSAVE_PATH);
  } else {
    sprintf(OUTPUTf, "%d.txt", (int)SMP.file);
  }

  char tmpPath[64], backupPath[64];
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", OUTPUTf);
  snprintf(backupPath, sizeof(backupPath), "%s.bak", OUTPUTf);
  recoverPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);
  if (!autoload) { drawText("LOAD", 1, 8, UI_GREEN); FastLEDshow(); }
  const bool isAutosaveSlot = autoload || SMP.file == 0;
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
        sdIoYield();
      }
    }
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
      memcpy(&SMP, payload + expectedNotes * 4u + 2u, sizeof(SMP));
      if (!isfinite(SMP.bpm) || SMP.bpm < 40.0f || SMP.bpm > 300.0f) SMP.bpm = 100.0f;
      if (SMP.file > 999) SMP.file = 1;
      if (SMP.pack > 999) SMP.pack = 1;
      for (int ch = 0; ch < maxY; ch++) {
        globalMutes[ch] = SMP.globalMutes[ch];
        for (int page = 0; page < maxPages; page++) pageMutes[page][ch] = SMP.pageMutes[page][ch];
      }
      unmuteAllChannels();
      applyMutesAfterPMODSwitch();
      loadedOk = true;
    }
    sdIoYield();
  }

  if (!loadedOk) {
    // EXTMEM/PSRAM is not a blank canvas after reset — always zero the grid.
    if (autoload) clearPatternNotes();
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
    } else if (!autoload && SMP.file != 0) {
      extern void showNewFileScreen();
      showNewFileScreen();
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
  
  Mode *bpm_vol = &volume_bpm;
  bpm_vol->pos[3] = SMP.bpm;
  playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
  playTimer.update((uint32_t)round(playNoteInterval));
  bpm_vol->pos[2] = GLOB.vol;

  // Reset paint/unpaint prevention flag after loadPattern operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;

  updateLastPage();
  
  // Load SMP settings
  if (!autoload && !isNowPlaying) {
    delay(500);
  }
  loadSMPSettings();
  
  if (!autoload) {
    if (!isNowPlaying) {
      delay(500);
    }
    switchMode(&draw);
    applyDrawSingleYEncoderLimits();
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

FLASHMEM void autoSave() {
  // Only skip if we already saved within the last 5 seconds.
  static unsigned long lastAutoSaveTime = 0;
  const unsigned long AUTO_SAVE_COOLDOWN_MS = 5000;

  unsigned long currentTime = millis();
  if (lastAutoSaveTime > 0 && (currentTime - lastAutoSaveTime) < AUTO_SAVE_COOLDOWN_MS) {
    return;
  }

  savePattern(true);
  if (lastPatternSaveOk) lastAutoSaveTime = currentTime;

  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}
