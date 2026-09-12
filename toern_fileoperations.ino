static const char *PATTERN_TMP_PATH = "pattern.tmp";
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
  if (SD.exists(PATTERN_TMP_PATH)) SD.remove(PATTERN_TMP_PATH);
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

FLASHMEM void savePattern(bool autosave) {
  extern bool isNowPlaying;
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();

  // FILE slot 0 is autosaved.txt — load-only from the FILE menu.
  if (!autosave && SMP.file == 0) {
    return;
  }

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
  if (autosave) {
    sprintf(OUTPUTf, "%s", AUTOSAVE_PATH);
  } else {
    sprintf(OUTPUTf, "%d.txt", (int)SMP.file);
  }

  // Atomic write: pattern.tmp → rename. Never truncate autosaved.txt first —
  // a crash mid-save used to leave a truncated/garbage file that autoload treated as notes.
  if (SD.exists(PATTERN_TMP_PATH)) {
    SD.remove(PATTERN_TMP_PATH);
    sdIoYield();
  }
  File saveFile = SD.open(PATTERN_TMP_PATH, O_WRITE | O_CREAT | O_TRUNC);
  bool wroteOk = false;
  if (saveFile) {
    uint8_t buf[512];
    size_t bufLen = 0;
    unsigned rowCount = 0;
    wroteOk = true;

    for (unsigned int sdx = 1; sdx < maxlen && wroteOk; sdx++) {
      for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
        maxdata = maxdata + note[sdx][sdy].channel;
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
    if (SD.exists(OUTPUTf)) {
      SD.remove(OUTPUTf);
      sdIoYield();
    }
    if (!SD.rename(PATTERN_TMP_PATH, OUTPUTf)) {
      wroteOk = false;
      // Keep pattern.tmp — autoLoad will rename it if dest is missing.
    }
  } else if (SD.exists(PATTERN_TMP_PATH)) {
    SD.remove(PATTERN_TMP_PATH);
  }

  // Only delete empty files for manual saves, not autosaves
  // (We want to preserve empty state in autosaved.txt)
  if (wroteOk && maxdata == 0 && !autosave) {
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

  const bool isAutosaveSlot = autoload || SMP.file == 0;
  bool loadedOk = false;
  const size_t expectedNotes = (size_t)(maxlen - 1) * (size_t)maxY;

  if (SD.exists(OUTPUTf)) {
    File loadFile = SD.open(OUTPUTf);
    if (loadFile) {
      const size_t minBytes = expectedNotes * 4u + 2u;
      if (loadFile.size() >= minBytes) {
        clearPatternNotes();

        uint8_t buf[512];
        size_t have = 0;
        size_t pos = 0;
        unsigned int sdry = 1;
        unsigned int sdrx = 1;
        size_t notesRead = 0;
        bool foundMarker = false;

        while (notesRead < expectedNotes) {
          if (pos >= have) {
            have = 0;
            pos = 0;
            if (!loadFile.available()) break;
            int got = loadFile.read(buf, sizeof(buf));
            if (got <= 0) break;
            have = (size_t)got;
            sdIoYield();
          }
          if (have - pos < 4) {
            if (pos > 0) {
              memmove(buf, buf + pos, have - pos);
              have -= pos;
              pos = 0;
            }
            if (!loadFile.available()) break;
            int got = loadFile.read(buf + have, sizeof(buf) - have);
            if (got <= 0) break;
            have += (size_t)got;
            sdIoYield();
            continue;
          }
          if (sdrx < maxlen && sdry >= 1 && sdry <= maxY) {
            note[sdrx][sdry].channel = buf[pos];
            note[sdrx][sdry].velocity = buf[pos + 1];
            note[sdrx][sdry].probability = buf[pos + 2];
            note[sdrx][sdry].condition = buf[pos + 3];
            note[sdrx][sdry].midiPitch = NOTE_MIDI_PITCH_NONE;
          }
          pos += 4;
          notesRead++;
          sdry++;
          if (sdry > maxY) {
            sdry = 1;
            sdrx++;
          }
        }

        if (notesRead == expectedNotes) {
          while (have - pos < 2) {
            if (pos > 0 && pos < have) {
              memmove(buf, buf + pos, have - pos);
              have -= pos;
              pos = 0;
            } else if (pos >= have) {
              have = 0;
              pos = 0;
            }
            if (!loadFile.available()) break;
            int got = loadFile.read(buf + have, sizeof(buf) - have);
            if (got <= 0) break;
            have += (size_t)got;
            sdIoYield();
          }
          if (have - pos >= 2 && buf[pos] == 0xFF && buf[pos + 1] == 0xFE) {
            pos += 2;
            foundMarker = true;
          }
        }

        if (foundMarker) {
          uint8_t *smpBytes = (uint8_t *)&SMP;
          size_t smpLeft = sizeof(SMP);
          size_t smpOff = 0;
          if (pos < have) {
            size_t take = min(have - pos, smpLeft);
            memcpy(smpBytes, buf + pos, take);
            smpOff += take;
            smpLeft -= take;
            pos += take;
          }
          while (smpLeft > 0 && loadFile.available()) {
            size_t n = min((size_t)512, smpLeft);
            int got = loadFile.read(smpBytes + smpOff, n);
            if (got <= 0) break;
            smpOff += (size_t)got;
            smpLeft -= (size_t)got;
            sdIoYield();
          }

          // Read the optional trailing pitch extension. A missing, truncated,
          // or unknown extension leaves every note in legacy row-derived mode.
          if (smpLeft == 0 && loadFile.available() >= (int)sizeof(PITCH_EXTENSION_HEADER)) {
            uint8_t pitchHeader[sizeof(PITCH_EXTENSION_HEADER)];
            int headerRead = loadFile.read(pitchHeader, sizeof(pitchHeader));
            bool validPitchExtension =
                headerRead == (int)sizeof(pitchHeader) &&
                memcmp(pitchHeader, PITCH_EXTENSION_HEADER,
                       sizeof(PITCH_EXTENSION_HEADER)) == 0 &&
                loadFile.available() >= (int)expectedNotes;
            if (validPitchExtension) {
              size_t pitchIndex = 0;
              while (pitchIndex < expectedNotes) {
                size_t want = min(sizeof(buf), expectedNotes - pitchIndex);
                int got = loadFile.read(buf, want);
                if (got != (int)want) {
                  validPitchExtension = false;
                  break;
                }
                for (int i = 0; i < got; i++, pitchIndex++) {
                  unsigned int x = (unsigned int)(pitchIndex / maxY) + 1;
                  unsigned int y = (unsigned int)(pitchIndex % maxY) + 1;
                  uint8_t stored = buf[i];
                  note[x][y].midiPitch =
                      (stored <= 127 || stored == NOTE_MIDI_PITCH_NONE)
                        ? stored : NOTE_MIDI_PITCH_NONE;
                }
                sdIoYield();
              }
              if (!validPitchExtension) {
                for (unsigned int x = 1; x < maxlen; x++) {
                  for (unsigned int y = 1; y <= maxY; y++) {
                    note[x][y].midiPitch = NOTE_MIDI_PITCH_NONE;
                  }
                }
              }
            }
          }

          if (SMP.bpm < 40.0f || SMP.bpm > 300.0f) {
            SMP.bpm = 100.0f;
          }
          if (SMP.file > 99) SMP.file = 1;
          if (SMP.pack < 1 || SMP.pack > 99) SMP.pack = 1;

          for (int ch = 0; ch < maxY; ch++) {
            globalMutes[ch] = SMP.globalMutes[ch];
            for (int page = 0; page < maxPages; page++) {
              pageMutes[page][ch] = SMP.pageMutes[page][ch];
            }
          }

          unmuteAllChannels();
          applyMutesAfterPMODSwitch();
          loadedOk = true;
        }
      }
      loadFile.close();
    }
    sdIoYield();
  }

  if (!loadedOk) {
    // EXTMEM/PSRAM is not a blank canvas after reset — always zero the grid.
    clearPatternNotes();
    const bool fileWasPresent = SD.exists(OUTPUTf);
    if (isAutosaveSlot && fileWasPresent) {
      SD.remove(OUTPUTf);
    }
    if (!autoload) sdIoEndAudioSafe();
    if (fileWasPresent) {
      extern void FastLEDclear();
      extern void FastLEDshow();
      extern void drawText(const char *text, int startX, int startY, CRGB color);
      FastLEDclear();
      drawText("ERR", 3, 8, CRGB(255, 0, 0));
      FastLEDshow();
      delay(700);
      if (autoload) {
        g_enterNewFileAfterCorruptAutoload = true;
      } else {
        extern void showNewFileScreen();
        showNewFileScreen();
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
  }
  
  sdIoYield();
  if (!autoload) sdIoEndAudioSafe();
}



FLASHMEM void autoLoad() {
  // Finish an interrupted atomic save (pattern.tmp already complete, rename never ran).
  if (!SD.exists(AUTOSAVE_PATH) && SD.exists(PATTERN_TMP_PATH)) {
    SD.rename(PATTERN_TMP_PATH, AUTOSAVE_PATH);
  }
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
  lastAutoSaveTime = currentTime;

  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}
