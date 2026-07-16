
FLASHMEM void savePattern(bool autosave) {
  extern bool isNowPlaying;
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();

  drawNoSD();
  // Autosave often runs right after pause while sample/synth voices are still decaying.
  // Keep Audio library fed for the whole SD burst (isNowPlaying is already false by then).
  if (autosave) {
    sdIoBeginAudioSafe();
  }
  stopSdPreviewIfPlaying();
  
  // Only clear display for manual saves, not autosaves
  if (!autosave) {
    FastLEDclear();
  }
  
  unsigned int maxdata = 0;
  char OUTPUTf[50];
  // Save to autosave.txt if autosave is true
  if (autosave) {
    sprintf(OUTPUTf, "autosaved.txt");
  } else {
    sprintf(OUTPUTf, "%d.txt", SMP.file);
  }
  // Truncate in place — avoid SD.remove() which can stall audio for a long time on large files.
  File saveFile = SD.open(OUTPUTf, O_WRITE | O_CREAT | O_TRUNC);
  if (saveFile) {
    // Buffer note records (4 bytes each) to avoid per-byte SD calls while playing.
    uint8_t buf[512];
    size_t bufLen = 0;
    unsigned rowCount = 0;

    for (unsigned int sdx = 1; sdx < maxlen; sdx++) {
      for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
        maxdata = maxdata + note[sdx][sdy].channel;
        if (bufLen + 4 > sizeof(buf)) {
          saveFile.write(buf, bufLen);
          bufLen = 0;
          sdIoYield();
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
    if (bufLen > 0) {
      saveFile.write(buf, bufLen);
      bufLen = 0;
      sdIoYield();
    }

    // Use a unique marker to indicate the end of notes and start of SMP data
    saveFile.write((uint8_t)0xFF);
    saveFile.write((uint8_t)0xFE);
    sdIoYield();

    // Save current mute states to SMP before writing
    for (int ch = 0; ch < maxY; ch++) {
      SMP.globalMutes[ch] = globalMutes[ch];
      for (int page = 0; page < maxPages; page++) {
        SMP.pageMutes[page][ch] = pageMutes[page][ch];
      }
    }
    
    // Save SMP struct (file/pattern specific data) in chunks
    const uint8_t *smpBytes = (const uint8_t *)&SMP;
    size_t smpLeft = sizeof(SMP);
    size_t smpOff = 0;
    const size_t smpChunk = 256;
    while (smpLeft > 0) {
      size_t n = min(smpChunk, smpLeft);
      saveFile.write(smpBytes + smpOff, n);
      smpOff += n;
      smpLeft -= n;
      sdIoYield();
    }
    saveFile.close();
  }
  // Only delete empty files for manual saves, not autosaves
  // (We want to preserve empty state in autosaved.txt)
  if (maxdata == 0 && !autosave) {
    SD.remove(OUTPUTf);
  }
  if (!autosave) {
    // Skip long delay while playing — it starves UI/audio cooperation.
    if (!isNowPlaying) {
      delay(500);
    }
    switchMode(&draw);
  }
  
  // Reset paint/unpaint prevention flag after savePattern operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
  
  sdIoYield();
  if (autosave) {
    sdIoEndAudioSafe();
  }
}


FLASHMEM void saveSamplePack(int pack) {
    extern void stopSdPreviewIfPlaying();
    extern size_t sdIoChunkSize();
    extern void sdIoYield();

    stopSdPreviewIfPlaying();
    
    char filename[64];
    char sourcePath[64];

    char dirPath[64];
    // Ensure the directory exists
    snprintf(dirPath, sizeof(dirPath), "%d", pack);
    if (!SD.exists(dirPath)) {
        SD.mkdir(dirPath);
    }

    // Iterate over each sample channel (1..8)
    for (uint8_t ch = 1; ch <= 8; ch++) {
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
                SD.remove(filename);
            }
            
            // Open source file for reading
            File sourceFile = SD.open(sourcePath, FILE_READ);
            if (!sourceFile) {
                continue;
            }
            
            // Open destination file for writing
            File destFile = SD.open(filename, FILE_WRITE);
            if (!destFile) {
                sourceFile.close();
                continue;
            }
            
            // Reuse EXTMEM scratch; read/write in sdIoChunkSize() slices while playing.
            static EXTMEM uint8_t packCopyBuf[65536];
            const size_t chunk = sdIoChunkSize();
            size_t bytesRead;
            while ((bytesRead = sourceFile.read(packCopyBuf, chunk)) > 0) {
                destFile.write(packCopyBuf, bytesRead);
                sdIoYield();
            }
            
            sourceFile.close();
            destFile.close();
        } else if (loadedSampleLen[ch] > 0) {
            // Save from RAM (current loaded sample)
            // Remove any existing file at this path
            if (SD.exists(filename)) {
                SD.remove(filename);
            }
            
            // Open a new file for writing
            File outFile = SD.open(filename, FILE_WRITE);
            if (!outFile) {
                continue;
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
              outFile.write(dataPtr + offset, chunkSize);
              sdIoYield();
            }

            finalizeWavHeader(outFile, totalBytes);
            
            outFile.close();
        } else {
            // Voices without a sample get a zero-byte placeholder file so banks
            // still contain all 8 slots and load back as silence instead of mute.
            if (SD.exists(filename)) {
                SD.remove(filename);
            }

            File emptyFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);
            if (emptyFile) {
                emptyFile.close();
            }
        }
        sdIoYield();
    }
    
    // Clear sp0Active flags when saving a samplepack (all samples now part of this pack)
    for (uint8_t ch = 1; ch < maxFiles; ch++) {
        SMP.sp0Active[ch] = false;
    }
    saveSp0StateToEEPROM();
    
    // Reset paint/unpaint prevention flag after saveSamplePack operation
    extern bool preventPaintUnpaint;
    preventPaintUnpaint = false;
    
   sdIoYield();
}


FLASHMEM void loadPattern(bool autoload) {
  extern bool isNowPlaying;
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();

  drawNoSD();
  stopSdPreviewIfPlaying();
  
  FastLEDclear();
  char OUTPUTf[50];
  if (autoload) {
    sprintf(OUTPUTf, "autosaved.txt");
  } else {
    sprintf(OUTPUTf, "%d.txt", SMP.file);
  }
  
  // Load .txt file
  if (SD.exists(OUTPUTf)) {
    File loadFile = SD.open(OUTPUTf);
    if (loadFile) {
      unsigned int sdry = 1;
      unsigned int sdrx = 1;
      unsigned rowCount = 0;
      uint8_t rec[4];

      while (loadFile.available()) {
        int b = loadFile.read();
        if (b < 0) break;

        // Check for the marker indicating the end of notes
        if (b == 0xFF && loadFile.peek() == 0xFE) {
          loadFile.read();  // Consume the second marker byte
          break;            // Exit the loop to load SMP data
        }

        // Read remaining 3 bytes of the note record (velocity, probability, condition)
        rec[0] = (uint8_t)b;
        int n = loadFile.read(rec + 1, 3);
        if (n < 1) {
          // Legacy short record: velocity only
          note[sdrx][sdry].channel = rec[0];
          note[sdrx][sdry].velocity = 0;
          note[sdrx][sdry].probability = 100;
          note[sdrx][sdry].condition = 1;
        } else {
          note[sdrx][sdry].channel = rec[0];
          note[sdrx][sdry].velocity = (n >= 1) ? rec[1] : 0;
          note[sdrx][sdry].probability = (n >= 2) ? rec[2] : 100;
          note[sdrx][sdry].condition = (n >= 3) ? rec[3] : 1;
        }
        sdry++;
        if (sdry > maxY) {
          sdry = 1;
          sdrx++;
        }
        if (sdrx > maxlen)
          sdrx = 1;
        if ((++rowCount & 0x3Fu) == 0u) {
          sdIoYield();
        }
      }

      // Load SMP struct after marker (file/pattern specific data)
      if (loadFile.available()) {
        uint8_t *smpBytes = (uint8_t *)&SMP;
        size_t smpLeft = sizeof(SMP);
        size_t smpOff = 0;
        while (smpLeft > 0 && loadFile.available()) {
          size_t n = min((size_t)512, smpLeft);
          int got = loadFile.read(smpBytes + smpOff, n);
          if (got <= 0) break;
          smpOff += (size_t)got;
          smpLeft -= (size_t)got;
          sdIoYield();
        }
        
        // Validate BPM after loading (default to 100 if invalid)
        if (SMP.bpm < 40.0f || SMP.bpm > 300.0f) {
          SMP.bpm = 100.0f;
        }
        
        // Load mute states from SMP
        for (int ch = 0; ch < maxY; ch++) {
          globalMutes[ch] = SMP.globalMutes[ch];
          for (int page = 0; page < maxPages; page++) {
            pageMutes[page][ch] = SMP.pageMutes[page][ch];
          }
        }
        
        // Unmute all channels first, then apply loaded mutes based on PMOD state
        unmuteAllChannels();
        applyMutesAfterPMODSwitch();
      }
    }
    loadFile.close();
    sdIoYield();
  }
  
  // Reset basic runtime flags when loading a pattern
  GLOB.singleMode = false;
  
  // Validate BPM before using (default to 100 if invalid)
  if (SMP.bpm < 40.0f || SMP.bpm > 300.0f) {
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
  
  // If no file was loaded, show NEW screen
  if (!SD.exists(OUTPUTf)) {
    // File not found - show NEW screen for genre generation when creating new file
    extern void showNewFileScreen();
    showNewFileScreen();
    return; // Don't continue with normal load flow
  }

  updateLastPage();
  
  // Load SMP settings
  if (!isNowPlaying) {
    delay(500);
  }
  loadSMPSettings();
  
  if (!autoload) {
    if (!isNowPlaying) {
      delay(500);
    }
    switchMode(&draw);
  }
  
  sdIoYield();
}



FLASHMEM void autoLoad() {
  
  loadPattern(true);
  
  // Reset paint/unpaint prevention flag after autoLoad operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}

FLASHMEM void autoSave() {
  // Prevent autosave if last save was less than 10 seconds ago
  static unsigned long lastAutoSaveTime = 0;
  const unsigned long AUTO_SAVE_COOLDOWN_MS = 10000;  // 10 seconds
  
  unsigned long currentTime = millis();
  if (lastAutoSaveTime > 0 && (currentTime - lastAutoSaveTime) < AUTO_SAVE_COOLDOWN_MS) {
    // Too soon since last autosave - skip this one
    return;
  }
  
  savePattern(true);
  lastAutoSaveTime = currentTime;  // Update timestamp after successful save
  
  // Reset paint/unpaint prevention flag after autoSave operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}
