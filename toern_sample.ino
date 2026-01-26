
extern int8_t channelDirection[maxFiles];
extern bool manifestLoaded;
extern uint16_t manifestFolderCount;
extern uint16_t manifestFileCount[];
extern char manifestFolderNames[][32];
bool scanAndWriteManifest();
extern int previewTriggerMode;
extern const int PREVIEW_MODE_ON;
extern const int PREVIEW_MODE_PRESS;

// Track last sample selection in showWave() across calls
static int lastEncoder3Value_forShowWave = -1;

// --- PREV==PRSS: incremental SD peak scanner (no audio playback) ------------
// This avoids AudioPlaySdWav SD reads in the audio thread (which can click/crash).
struct PeakScanState {
  File f;
  bool active = false;
  char path[64] = {0};
  uint32_t dataStart = 0;
  uint32_t dataSize = 0;
  uint32_t totalSamples = 0;     // 16-bit samples
  uint32_t sampleIdx = 0;
  uint16_t outIndex = 0;         // 0..maxPeaks-1 (matches peakIndex progression)
  uint32_t samplesInWindow = 0;  // samples accumulated in current window
  int16_t windowPeak = 0;        // max abs within current window
};

static PeakScanState g_peakScan;
static bool g_encoder0PressedMode = false;  // Track if encoder(0) is pressed - makes preview behave like seek > 0

// Expose encoder(0) pressed mode state for main loop
bool isEncoder0PressedMode() {
  return g_encoder0PressedMode;
}

void setEncoder0PressedMode(bool state) {
  g_encoder0PressedMode = state;
}

// Find 1-based file index within a manifest folder by exact filename (case-insensitive).
static int manifestFindFileIdx(int folderIdx, const char *filename) {
  if (!manifestLoaded) return -1;
  if (folderIdx < 0 || folderIdx >= (int)manifestFolderCount) return -1;
  if (!filename || !filename[0]) return -1;
  int fc = (int)manifestFileCount[folderIdx];
  for (int i = 0; i < fc; i++) {
    if (strcasecmp(manifestFileNames[folderIdx][i], filename) == 0) {
      return i + 1; // 1-based
    }
  }
  return -1;
}

static bool findWavDataChunk(File &f, uint32_t &outStart, uint32_t &outSize) {
  outStart = 0;
  outSize = 0;
  uint8_t hdr[12];
  f.seek(0);
  if (f.read(hdr, sizeof(hdr)) != (int)sizeof(hdr)) {
    return false;
  }
  if (!(hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F' && hdr[8]=='W' && hdr[9]=='A' && hdr[10]=='V' && hdr[11]=='E')) {
    return false;
  }
  
  // Check RIFF chunk size
  uint32_t riffSize = (uint32_t)hdr[4] | ((uint32_t)hdr[5] << 8) | ((uint32_t)hdr[6] << 16) | ((uint32_t)hdr[7] << 24);
  uint32_t fileSize = f.size();
  uint32_t expectedRiffSize = fileSize - 8; // RIFF size = file size - 8 bytes (RIFF + size fields)
  
  if (riffSize != expectedRiffSize) {
  }
  
  f.seek(12);
  uint32_t maxIterations = 100; // Prevent infinite loops from malformed files
  uint32_t iterations = 0;
  while (f.available() && iterations < maxIterations) {
    iterations++;
    char id[4];
    if (f.read((uint8_t*)id, 4) != 4) {
      break;
    }
    uint8_t szb[4];
    if (f.read(szb, 4) != 4) {
      break;
    }
    uint32_t sz = (uint32_t)szb[0] | ((uint32_t)szb[1] << 8) | ((uint32_t)szb[2] << 16) | ((uint32_t)szb[3] << 24);
    
    // Safety check: chunk size should be reasonable (not larger than file)
    uint32_t currentPos = f.position();
    if (sz > fileSize || (currentPos + sz) > fileSize) {
      break;
    }
    
    if (id[0]=='d' && id[1]=='a' && id[2]=='t' && id[3]=='a') {
      outStart = currentPos;
      outSize = sz;
      return (outSize >= 2);
    }
    
    // Skip this chunk (skip size includes padding for even alignment)
    uint32_t skip = sz + (sz & 1);
    uint32_t nextPos = currentPos + skip;
    if (nextPos > fileSize || !f.seek(nextPos)) {
      break;
    }
  }
  
  if (iterations >= maxIterations) {
  } else {
  }
  return false;
}

static void stopPeakScan() {
  if (g_peakScan.active) {
    if (g_peakScan.f) {
      g_peakScan.f.close();
    }
    g_peakScan.active = false;
    yield();  // Yield to ensure file is fully closed before any other SD operations
  }
}

// Public wrapper to start peak scan
void startPeakScan(const char *path, bool resetPeaks = true) {
  stopPeakScan();
  strncpy(g_peakScan.path, path, sizeof(g_peakScan.path) - 1);
  g_peakScan.path[sizeof(g_peakScan.path) - 1] = 0;

  if (!SD.exists(g_peakScan.path)) return;
  g_peakScan.f = SD.open(g_peakScan.path, FILE_READ);
  if (!g_peakScan.f) return;

  uint32_t ds = 0, dz = 0;
  if (!findWavDataChunk(g_peakScan.f, ds, dz)) {
    // fallback: many of your own WAVs are standard 44-byte headers
    ds = 44;
    dz = (uint32_t)max(0, (int)g_peakScan.f.size() - 44);
  }
  if (dz < 2) { g_peakScan.f.close(); return; }

  g_peakScan.dataStart = ds;
  g_peakScan.dataSize = dz;
  g_peakScan.totalSamples = dz / 2;
  
  if (resetPeaks) {
    g_peakScan.sampleIdx = 0;
    g_peakScan.outIndex = 0;
    peakIndex = 0;
    memset(peakValues, 0, sizeof(peakValues));
  } else {
    g_peakScan.outIndex = (uint16_t)peakIndex;
    g_peakScan.sampleIdx = (uint32_t)peakIndex * 640;
    if (g_peakScan.sampleIdx >= g_peakScan.totalSamples) {
       stopPeakScan();
       return;
    }
  }
  
  g_peakScan.samplesInWindow = 0;
  g_peakScan.windowPeak = 0;
  g_peakScan.active = true;

  g_peakScan.f.seek(g_peakScan.dataStart + (g_peakScan.sampleIdx * 2));
}

static void servicePeakScan(uint32_t maxBytesThisCall) {
  if (!g_peakScan.active) return;
  
  if (playSdWav1.isPlaying()) {
    stopPeakScan();
    return;
  }
  
  if (!g_peakScan.f.available()) { stopPeakScan(); return; }

  const uint32_t WINDOW_SAMPLES = 640;
  uint8_t buf[2048];  
  uint32_t bytesLeft = maxBytesThisCall;
  uint32_t bytesProcessed = 0;
  const uint32_t YIELD_INTERVAL = 2048; // Yield every 2KB to keep UI responsive
  
  while (bytesLeft > 0 && g_peakScan.active && g_peakScan.outIndex < (uint16_t)maxPeaks) {
    // Yield periodically to prevent blocking
    if (bytesProcessed >= YIELD_INTERVAL) {
      yield();
      bytesProcessed = 0;
    }
    
    uint32_t toRead = min((uint32_t)sizeof(buf), bytesLeft);
    int n = g_peakScan.f.read(buf, (int)toRead);
    if (n <= 0) { stopPeakScan(); break; }
    bytesLeft -= (uint32_t)n;
    bytesProcessed += (uint32_t)n;

    int samplesInChunk = n / 2;
    for (int i = 0; i < samplesInChunk; i++) {
      int lo = buf[i * 2];
      int hi = (int8_t)buf[i * 2 + 1];
      int16_t v = (int16_t)((hi << 8) | lo);
      int16_t a = (v < 0) ? (int16_t)(-v) : v;
      if (a > g_peakScan.windowPeak) g_peakScan.windowPeak = a;

      g_peakScan.sampleIdx++;
      g_peakScan.samplesInWindow++;

      if (g_peakScan.samplesInWindow >= WINDOW_SAMPLES) {
        // Bounds check before writing to prevent buffer overflow
        if (g_peakScan.outIndex >= (uint16_t)maxPeaks) {
          stopPeakScan();
          break;
        }
        peakValues[g_peakScan.outIndex] = (float)g_peakScan.windowPeak / 32768.0f;
        g_peakScan.outIndex++;
        g_peakScan.samplesInWindow = 0;
        g_peakScan.windowPeak = 0;
        peakIndex = (int)g_peakScan.outIndex; // progressive display
        if (g_peakScan.outIndex >= (uint16_t)maxPeaks) { stopPeakScan(); break; }
      }

      if (g_peakScan.sampleIdx >= g_peakScan.totalSamples) {
        if (g_peakScan.samplesInWindow > 0 && g_peakScan.outIndex < (uint16_t)maxPeaks) {
          peakValues[g_peakScan.outIndex] = (float)g_peakScan.windowPeak / 32768.0f;
          g_peakScan.outIndex++;
          peakIndex = (int)g_peakScan.outIndex;
        }
        stopPeakScan();
        break;
      }
    }
  }
}

FLASHMEM void previewSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength) {
  stopPeakScan();
  
  if (playSdWav1.isPlaying()) playSdWav1.stop();
  envelope0.noteOff();
  
  char OUTPUTf[64];
  buildSamplePath(folder, sampleID, OUTPUTf, sizeof(OUTPUTf));

  // When encoder(0) is pressed, behave like seek > 0: skip full range path, just play audio
  bool usingFullRange = !g_encoder0PressedMode && (GLOB.seek == 0) && (GLOB.seekEnd == 100 || GLOB.seekEnd == 0);
  if (usingFullRange && (previewTriggerMode == PREVIEW_MODE_ON || previewTriggerMode == PREVIEW_MODE_PRESS)) {
    yield(); // Yield before SD.exists() to prevent blocking main sequencer
    if (!SD.exists(OUTPUTf)) {
      return;
    }

    if (GLOB.seekEnd == 0) {
      GLOB.seekEnd = 100;
      currentMode->pos[2] = GLOB.seekEnd;
      Encoder[1].writeCounter((int32_t)GLOB.seekEnd);
    }

    previewCache.valid = false;
    previewCache.lengthBytes = 0;
    previewCache.plen = 0;

    previewIsPlaying = true;
    
    // Only clear peaks if this is a NEW sample (not just a preview of the same sample)
    static unsigned int lastPreviewFolder = 999;
    static unsigned int lastPreviewSampleID = 999;
    bool isNewSample = (folder != lastPreviewFolder || sampleID != lastPreviewSampleID);
    
    if (isNewSample) {
      peakIndex = 0;
      memset(peakValues, 0, sizeof(peakValues));
      lastPreviewFolder = folder;
      lastPreviewSampleID = sampleID;
    }
    
    extern void updatePreviewVolume();
    updatePreviewVolume();
    
    yield();
    playSdWav1.play(OUTPUTf);  // Stream from SD (no RAM copy)
    
    sampleIsLoaded = true;
    return;
  }

  if (!previewCache.valid || previewCache.folder != folder || previewCache.sampleID != sampleID) {
    yield(); // Yield before SD.open() to prevent blocking main sequencer
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      return;
    }

    int fileSize = previewFile.size();
    uint32_t dataStart = 44, dataSize = 0;
    if (!findWavDataChunk(previewFile, dataStart, dataSize)) {
      dataStart = 44;
      dataSize = (uint32_t)max(0, fileSize - 44);
    }

    // All samples are 44.1kHz Mono - no need to check sample rate
    // For 44.1kHz, the original code used PrevSampleRate = 3 (byte 0x44 at offset 24)
    // Keep the original calculation: dataSize / (PrevSampleRate * 2) for compatibility
    PrevSampleRate = 3;  // 44.1kHz Mono (original mapping: byte 68 = 0x44)

    // IMPORTANT: Use WAV data chunk size (not file size). This avoids playing non-audio chunks at the end.
    // Original calculation: dataSize / (PrevSampleRate * 2) = dataSize / 6 for 44.1kHz
    GLOB.smplen = (int)(dataSize / (uint32_t)(PrevSampleRate * 2));

    bool loadPartial = (!setMaxSampleLength) && ((int)dataSize > (int)sizeof(sampled[0]));

    // Load full sample into RAM buffer (no blanket memset to keep this fast on large files)
    int plen = 0;
    int readStartBytes = (int)dataStart;
    int bytesToReadAll = (int)dataSize;
    if (loadPartial) {
      // Compute requested window in bytes based on seek/seekEnd (after smplen computed)
      int startOffset = (GLOB.smplen * GLOB.seek) / 100;
      int endOffset = (GLOB.smplen * GLOB.seekEnd) / 100;
      endOffset = min(endOffset, GLOB.smplen);
      // Use original calculation: offset * PrevSampleRate * 2
      int startOffsetBytes = startOffset * PrevSampleRate * 2;
      int endOffsetBytes = endOffset * PrevSampleRate * 2;
      startOffsetBytes &= ~1;
      endOffsetBytes &= ~1;
      if (endOffsetBytes <= startOffsetBytes) {
        startOffsetBytes = 0;
        endOffsetBytes = min((int)dataSize, (int)sizeof(sampled[0]));
      }
      readStartBytes = (int)dataStart + startOffsetBytes;
      bytesToReadAll = min(endOffsetBytes - startOffsetBytes, (int)sizeof(sampled[0]));
    }

    previewFile.seek(readStartBytes);
    // Read in chunks with periodic yield() to prevent blocking CPU during large file operations
    // Larger chunk to reduce SD overhead on big files
    uint8_t chunk[2048];  // Read 2KB at a time
    int chunkCount = 0;
    while (previewFile.available() && plen < bytesToReadAll) {
      size_t toRead = min(sizeof(chunk), (size_t)(bytesToReadAll - plen));
      size_t bytesRead = previewFile.read(chunk, toRead);
      if (bytesRead == 0) break;
      memcpy(&sampled[0][plen], chunk, bytesRead);
      plen += bytesRead;
      // Yield every chunk; extra yield every few chunks to stay responsive
      yield();
      if ((++chunkCount & 0x3) == 0) yield();
    }
    previewFile.close();

    // Update cache
    previewCache.folder = folder;
    previewCache.sampleID = sampleID;
    previewCache.lengthBytes = plen;
    previewCache.rate = PrevSampleRate;
    previewCache.valid = true;
    previewCache.plen = plen;
  } else {
    PrevSampleRate = previewCache.rate;  // Keep for compatibility, should be 3 for 44.1kHz
    // Original calculation: lengthBytes / (PrevSampleRate * 2)
    GLOB.smplen = previewCache.lengthBytes / (PrevSampleRate * 2);
  }

  // --- Calculate offsets from seek percentages ---
  int startOffset = (GLOB.smplen * GLOB.seek) / 100;
  int endOffset = (GLOB.smplen * GLOB.seekEnd) / 100;
  endOffset = min(endOffset, GLOB.smplen);

  if (setMaxSampleLength) {
    endOffset = GLOB.smplen;
    GLOB.seekEnd = 100;
    currentMode->pos[2] = GLOB.seekEnd;
    Encoder[1].writeCounter((int32_t)GLOB.seekEnd);
  }

  // Use original calculation: offset * PrevSampleRate * 2
  int startOffsetBytes = startOffset * PrevSampleRate * 2;
  int endOffsetBytes = endOffset * PrevSampleRate * 2;

  // Clamp offsets to buffer size
  if (startOffsetBytes >= previewCache.lengthBytes) startOffsetBytes = 0;
  if (endOffsetBytes > previewCache.lengthBytes) endOffsetBytes = previewCache.lengthBytes;

  // Ensure alignment for 16-bit playback
  startOffsetBytes &= ~1;
  endOffsetBytes &= ~1;

   // Clamp into the file/buffer
  startOffsetBytes = constrain(startOffsetBytes, 0, previewCache.lengthBytes);
  endOffsetBytes   = constrain(endOffsetBytes,   0, previewCache.lengthBytes);

  // If the end lies before (or equal to) the start, nothing to play
  if (endOffsetBytes <= startOffsetBytes) {
    return;
  }

  // Compute number of payload bytes, force even
  int byteCount = (endOffsetBytes - startOffsetBytes) & ~1;

  // Convert bytes -> 16-bit frames
  int16_t* samplePtr   = (int16_t*)&sampled[0][startOffsetBytes];
  size_t    frameCount = byteCount / 2;

  // Finally hand off only valid data to the sampler
  _samplers[0].removeAllSamples();
  _samplers[0].addSample(
    36,              // midi note root
    samplePtr,       // pointer to your aligned data
    frameCount,      // number of 16-bit samples
    rateFactor
  );
  sampleIsLoaded = true;

  _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
}



FLASHMEM void loadSample(unsigned int packID, unsigned int sampleID) {
  drawNoSD();

  char OUTPUTf[64];
  
  // Check if we're loading from samplepack 0 (check if file exists first)
  sprintf(OUTPUTf, "0/%d.wav", sampleID);
  bool loadingFromSp0 = (packID == 0 && SD.exists(OUTPUTf));
  // If we are loading from a regular samplepack, this voice is no longer "SP0 custom".
  // (SP0 custom should only be true when we use 0/<voice>.wav)
  if (packID != 0 && sampleID < maxFiles) {
    SMP.sp0Active[sampleID] = false;
  } else if (loadingFromSp0 && sampleID < maxFiles) {
    SMP.sp0Active[sampleID] = true;
  }
  
  if (loadingFromSp0) {
    // Loading from samplepack 0 - path is already correct
  } else if (packID == 0) {
    // No numeric fallback: if samplepack 0 doesn't have this voice file, treat as missing.
    OUTPUTf[0] = '\0';
  } else {
    // Loading from regular samplepack
    snprintf(OUTPUTf, sizeof(OUTPUTf), "%d/%d.wav", packID, sampleID);
  }

  if (!SD.exists(OUTPUTf)) {
    setMuteState(sampleID, true);
    return;
  }
  // IMPORTANT: do not auto-unmute on successful load.
  // Loading a sample/samplepack should preserve the user's mute states.

  

  File loadSample = SD.open(OUTPUTf);
  if (loadSample) {
    int fileSize = loadSample.size();
    // All samples are 44.1kHz Mono - no need to check sample rate
    // For 44.1kHz, the original code used SampleRate = 3 (byte 0x44 at offset 24)
    SampleRate[sampleID] = 3;  // 44.1kHz Mono (original mapping: byte 68 = 0x44)

    // Use WAV data chunk size, not file size (prevents reading/listening beyond audio data).
    uint32_t dataStart = 44, dataSize = 0;
    if (!findWavDataChunk(loadSample, dataStart, dataSize)) {
      dataStart = 44;
      dataSize = (uint32_t)max(0, fileSize - 44);
    }

    // Original calculation: dataSize / (SampleRate * 2) = dataSize / 6 for 44.1kHz
    GLOB.smplen = (int)(dataSize / (uint32_t)(SampleRate[sampleID] * 2));

    // Convert percentage positions to actual offsets
    // Use original calculation: offset * SampleRate * 2
    unsigned int startOffset = (GLOB.seek * GLOB.smplen) / 100;
    unsigned int startOffsetBytes = startOffset * SampleRate[sampleID] * 2;

    // Empirical end-click guard:
    // Users report the end-click disappears when moving seekEnd by 1 step.
    // So we always trim by 1 seekEnd step for playback (i.e. use seekEnd-1%),
    // while keeping the UI/setting unchanged.
    unsigned int seekEndPct = GLOB.seekEnd;
    if (seekEndPct == 0) seekEndPct = 100;               // 0 means full length in UI
    if (seekEndPct > 0) seekEndPct = seekEndPct - 1;     // trim one encoder step
    // Ensure end stays > start
    if (seekEndPct <= GLOB.seek) seekEndPct = min((unsigned int)100, (unsigned int)(GLOB.seek + 1));

    unsigned int endOffset = (seekEndPct * GLOB.smplen) / 100;
    if (endOffset > GLOB.smplen) endOffset = GLOB.smplen;
    // Use original calculation: offset * SampleRate * 2
    unsigned int endOffsetBytes = endOffset * SampleRate[sampleID] * 2;
    endOffsetBytes = min(endOffsetBytes, (unsigned int)dataSize);

    loadSample.seek((uint32_t)dataStart + startOffsetBytes);
    unsigned int i = 0;
    // Clear target buffer so new load starts from a clean state
    memset(sampled[sampleID], 0, sizeof(sampled[sampleID]));

    // Read in chunks with periodic yield() to prevent blocking CPU during large file operations
    // Larger chunk to reduce SD overhead on big files
    uint8_t chunk[2048];  // Read 2KB at a time
    size_t bytesToRead = min((size_t)(endOffsetBytes - startOffsetBytes), sizeof(sampled[sampleID]));
    int chunkCount = 0;
    while (loadSample.available() && i < bytesToRead) {
      size_t toRead = min(sizeof(chunk), bytesToRead - i);
      size_t bytesRead = loadSample.read(chunk, toRead);
      if (bytesRead == 0) break;
      memcpy(&sampled[sampleID][i], chunk, bytesRead);
      i += bytesRead;
      if (i >= sizeof(sampled[sampleID])) break;
      // Yield every chunk to maintain responsiveness (critical for long samples)
      yield();
      if ((++chunkCount & 0x3) == 0) yield();
    }
    loadSample.close();

    // i is bytes read; convert to 16-bit samples
    i = i / 2;
    _samplers[sampleID].removeAllSamples();
    loadedSampleRate[sampleID] = SampleRate[sampleID];  // e.g. 44100, or whatever

    // Add a small zero-tail pad to let downstream FX (filter/bitcrusher) settle to zero.
    // This helps eliminate end-clicks when the player stops at the end of the buffer.
    const int END_PAD_SAMPLES = 1024; // ~23ms @ 44.1kHz
    int bufferSamples = (int)(sizeof(sampled[sampleID]) / 2);
    int samplesRead = (int)i;
    if (samplesRead < 0) samplesRead = 0;
    if (samplesRead > bufferSamples) samplesRead = bufferSamples;
    int pad = END_PAD_SAMPLES;
    if (pad > (bufferSamples - samplesRead)) pad = (bufferSamples - samplesRead);
    
    // Explicitly zero the pad area to ensure no old data remains
    if (pad > 0 && (samplesRead + pad) <= bufferSamples) {
      memset(&sampled[sampleID][samplesRead * 2], 0, pad * 2);
    }
    
    int playLen = samplesRead + pad;

    loadedSampleLen[sampleID] = playLen;
    _samplers[sampleID].addSample(36, (int16_t *)sampled[sampleID], playLen, rateFactor);
    channelDirection[sampleID] = 1;
  }
  yield();
}




void showWave() {
  // Ensure manifest is available so folder browsing uses real folder names
  // Always use map.txt - if missing, scan and write it, then load it
  if (!manifestLoaded) {
    if (!loadSampleManifest()) {
      // map.txt is missing, scan and write it
      scanAndWriteManifest();
      // Now load the newly created map.txt
      loadSampleManifest();
    }
  }
  // Require manifest to be loaded - never use legacy mode
  if (!manifestLoaded) {
    return; // Cannot proceed without manifest
  }
  // ---- Helpers -------------------------------------------------------------
  auto resetSeekRange = []() {
    currentMode->pos[0] = 0;
    GLOB.seek = 0;
    Encoder[0].writeCounter((int32_t)0);
    currentMode->pos[2] = 100;
    GLOB.seekEnd = 100;
    Encoder[1].writeCounter((int32_t)100);
  };

  auto startSdPreview = [&](const char* path) {
    previewIsPlaying = true;
    peakIndex = 0;
    memset(peakValues, 0, sizeof(peakValues));
    
    // Ensure PREV volume setting is applied before starting SD playback
    extern void updatePreviewVolume();
    updatePreviewVolume();
    
    yield(); // Yield before file playback operations
    if (playSdWav1.isPlaying()) {
      playSdWav1.stop();
    }
    playSdWav1.play(path);
    sampleIsLoaded = true;
  };

  auto refreshPeaksDisplay = [&]() {
    // Update peaks whenever data exists
    if (peakIndex > 0) {
      processPeaks();
    }
  };

  // ---- Early exits / setup -------------------------------------------------
  extern int fastTouchRead(int);
  extern const int touchThreshold;
  extern Mode recordMode;
  int touchValue3 = fastTouchRead(SWITCH_3);
  if (touchValue3 > touchThreshold) {
    switchMode(&recordMode);
    return;
  }
  
  yield(); // Allow other tasks to run, especially important during file operations
  
  // Track last channel to detect channel changes
  static int lastChannelInShowWave = -1;
  
  // Only initialize encoder 3 when channel changes (not every frame)
  if (lastChannelInShowWave != GLOB.currentChannel) {
    lastChannelInShowWave = GLOB.currentChannel;
    // Manifest-based selection: folderIdx in oldID, fileIdx in fileID
    int folderIdx = (int)SMP.wav[GLOB.currentChannel].oldID;
    int fileIdx = (int)SMP.wav[GLOB.currentChannel].fileID;

    // Always use manifest mode - never use legacy mode
    // If indices are out of range, clamp them to valid values
    if (manifestLoaded) {
      bool folderValid = (folderIdx >= 0 && folderIdx < (int)manifestFolderCount);
      bool fileValid = false;
      if (folderValid) {
        int fc = (int)manifestFileCount[folderIdx];
        fileValid = (fileIdx >= 1 && fileIdx <= fc + 1);
      }
      if (!folderValid || !fileValid) {
        // Clamp to valid range - no legacy mode fallback
        folderIdx = constrain(folderIdx, 0, (int)manifestFolderCount - 1);
        int fc = (int)manifestFileCount[folderIdx];
        fileIdx = constrain(fileIdx, 1, max(1, fc + 1));
        SMP.wav[GLOB.currentChannel].oldID = (unsigned int)folderIdx;
        SMP.wav[GLOB.currentChannel].fileID = (unsigned int)fileIdx;
      }
    } else {
      // Manifest must be loaded - if not, we cannot proceed
      return;
    }

    if (manifestLoaded) folderIdx = constrain(folderIdx, 0, (int)manifestFolderCount - 1);
    if (folderIdx < 0) folderIdx = 0;
    GLOB.folder = (unsigned int)folderIdx;
    currentMode->pos[1] = folderIdx;
    Encoder[2].writeCounter((int32_t)folderIdx);

    int fileCount = (manifestLoaded && folderIdx < (int)manifestFolderCount) ? (int)manifestFileCount[folderIdx] : 0;
    int maxFilesInFolder = fileCount + 1; // include NEW slot
    if (maxFilesInFolder < 1) maxFilesInFolder = 1;
    Encoder[3].writeMin((int32_t)1);
    Encoder[3].writeMax((int32_t)maxFilesInFolder);

    if (fileIdx < 1 || fileIdx > maxFilesInFolder) fileIdx = 1;
    currentMode->pos[3] = fileIdx;
    Encoder[3].writeCounter((int32_t)fileIdx);
    lastEncoder3Value_forShowWave = -1;  // Force re-preview
  }
  
  // Always use manifest mode - never use legacy mode
  if (!manifestLoaded) {
    return; // Cannot proceed without manifest
  }
  int folderIdx = (int)GLOB.folder;
  folderIdx = constrain(folderIdx, 0, (int)manifestFolderCount - 1);
  if (folderIdx < 0) folderIdx = 0;
  int fileIdx = (int)SMP.wav[GLOB.currentChannel].fileID;
  if (fileIdx < 1) fileIdx = 1;
  char OUTPUTf[64];
  buildSamplePath(folderIdx, fileIdx, OUTPUTf, sizeof(OUTPUTf));
  
  firstcheck = false;
  nofile = false;

  FastLEDclear();
  
  // Clamp folder encoder to existing folders (always use manifest)
  // NOTE: Encoder functions are swapped in SET_WAV:
  // - Encoder[2] (rotate) = folder selection
  // - Encoder[1] (rotate) = seekEnd (trim)
  // Always use manifest mode - never use legacy mode
  if (!manifestLoaded) {
    return; // Cannot proceed without manifest
  }
  int maxFolders = manifestFolderCount;
  if (maxFolders < 1) maxFolders = 1;
  Encoder[2].writeMin((int32_t)0);
  Encoder[2].writeMax((int32_t)(maxFolders - 1));
  if ((int)currentMode->pos[1] >= maxFolders) {
    currentMode->pos[1] = maxFolders - 1;
    Encoder[2].writeCounter((int32_t)currentMode->pos[1]);
  }
  
  // Indicators: ENC0 preview (press), ENC2 seekEnd, ENC3 folder, ENC4 load
  // Manual preview via encoder(0) press is supported for both PREV==ON and PREV==PRSS.
  drawIndicator('L', 'P', 1);
  Encoder[0].writeRGBCode(0xFF00FF);

  drawIndicator('L', 'Y', 2);  // Encoder 1: trim/seekEnd (swapped)
  drawIndicator('L', 'W', 3);  // Encoder 2: folder browse (swapped)
  drawIndicator('L', 'G', 4);  // Encoder 3: load sample
  // (Encoder[0] is intentionally kept pink in SET_WAV to advertise preview-press.)
  
  // Set encoder colors to match indicators
  Encoder[2].writeRGBCode(0xFFFFFF); // White (folder)
  Encoder[1].writeRGBCode(0xFFFF00); // Yellow (seekEnd)
  Encoder[1].writeMin((int32_t)0);
  Encoder[1].writeMax((int32_t)100);
  if (currentMode->pos[2] > 100) {
    currentMode->pos[2] = 100;
    Encoder[1].writeCounter((int32_t)currentMode->pos[2]);
  }
  Encoder[3].writeRGBCode(0x00FF00); // Green

  // ---- Encoder 0: Seek start ----------------------------------------------
  {
    int newSeek = constrain(currentMode->pos[0], 0, GLOB.seekEnd - 1);  // Ensure seek < seekEnd
    if (newSeek != GLOB.seek) {
      GLOB.seek = newSeek;
      currentMode->pos[0] = newSeek;  // Update encoder to match
      // Stop peak building when seek changes (for both PREV modes)
      stopPeakScan();
      if (sampleIsLoaded && previewTriggerMode == PREVIEW_MODE_ON) {
      yield(); // Yield before file operations
      previewSample(folderIdx, fileIdx, false);  // Uses cached buffer when available
      }
    }
  }

  // ---- Encoder 2: Folder (swapped) ----------------------------------------
  // Read encoder counter directly and clamp to existing folders
  int32_t encoder2Counter = Encoder[2].readCounterInt();
  // Reuse maxFolders from above (already calculated)
  int clampedFolderPos = constrain((int)encoder2Counter, 0, maxFolders - 1);
  
  // Sync encoder hardware and mode position if clamped
  if (clampedFolderPos != (int)encoder2Counter) {
    Encoder[2].writeCounter((int32_t)clampedFolderPos);
  }
  if (clampedFolderPos != (int)currentMode->pos[1]) {
    currentMode->pos[1] = clampedFolderPos;
  }
  
  if (currentMode->pos[1] != GLOB.folder) {
    yield(); // Yield before audio/file operations
    envelope0.noteOff();
    playSdWav1.stop();
    firstcheck = true;
    nofile = false;
    GLOB.folder = clampedFolderPos;  // Use clamped value, 0-based folder index
    int folderIdx = max(0, (int)GLOB.folder);
    int fileCount = manifestLoaded && folderIdx < (int)manifestFolderCount ? (int)manifestFileCount[folderIdx] : 0;
    int maxFilesInFolder = fileCount + 1; // include NEW slot
    if (maxFilesInFolder < 1) maxFilesInFolder = 1;
    // Store manifest selection into SMP (folderIdx in oldID, fileIdx in fileID)
    SMP.wav[GLOB.currentChannel].oldID = (unsigned int)folderIdx;
    SMP.wav[GLOB.currentChannel].fileID = 1;
    currentMode->pos[3] = 1;
    Encoder[3].writeMin((int32_t)1);
    Encoder[3].writeMax((int32_t)maxFilesInFolder);
    Encoder[3].writeCounter((int32_t)1);
    lastEncoder3Value_forShowWave = -1;  // Force re-preview after folder change
  }

  // ---- Encoder 1: Seek end (swapped) --------------------------------------
  {
    int32_t encoder1Counter = Encoder[1].readCounterInt();
    int newSeekEnd = constrain((int)encoder1Counter, GLOB.seek + 1, 100);  // Ensure seekEnd > seek
    if (newSeekEnd != (int)encoder1Counter) {
      Encoder[1].writeCounter((int32_t)newSeekEnd);
    }
    if (newSeekEnd != GLOB.seekEnd) {
      GLOB.seekEnd = newSeekEnd;
      currentMode->pos[2] = newSeekEnd;  // Update encoder to match
      // Stop peak building when seekEnd changes (for both PREV modes)
      stopPeakScan();
      if (sampleIsLoaded && previewTriggerMode == PREVIEW_MODE_ON) {
      yield(); // Yield before file operations
      previewSample(folderIdx, fileIdx, false);  // Uses cached buffer when available
      }
    }
  }

  // ---- Encoder 3: Sample selection ---------------------------------------
  int encoderSampleValue = (int)currentMode->pos[3];
  int folderIdxForSample = max(0, (int)GLOB.folder);
  int fileCountForSample = manifestLoaded && folderIdxForSample < (int)manifestFolderCount ? (int)manifestFileCount[folderIdxForSample] : 0;
  int maxFilesForSample = fileCountForSample + 1; // include NEW slot
  if (maxFilesForSample < 1) maxFilesForSample = 1;
  int encoderMin = 1;
  int encoderMax = maxFilesForSample;
  if (encoderSampleValue < encoderMin) encoderSampleValue = encoderMin;
  if (encoderSampleValue > encoderMax) encoderSampleValue = encoderMax;
  if (encoderSampleValue != currentMode->pos[3]) {
    currentMode->pos[3] = encoderSampleValue;
    Encoder[3].writeCounter((int32_t)encoderSampleValue);
  }

  bool sampleChanged = (encoderSampleValue != lastEncoder3Value_forShowWave) ||
                       (encoderSampleValue != SMP.wav[GLOB.currentChannel].fileID);

  if (sampleChanged) {
    lastEncoder3Value_forShowWave = encoderSampleValue;
    fileIdx = encoderSampleValue;
    SMP.wav[GLOB.currentChannel].oldID = (unsigned int)folderIdxForSample;
    SMP.wav[GLOB.currentChannel].fileID = (unsigned int)fileIdx;

    // Reset preview / state
    yield(); // Yield before file operations
    envelope0.noteOff();
    previewIsPlaying = false;
    playSdWav1.stop();
    sampleIsLoaded = false;
    firstcheck = true;
    nofile = false;

    buildSamplePath(folderIdxForSample, fileIdx, OUTPUTf, sizeof(OUTPUTf));
    yield(); // Yield before opening file


    // Invalidate preview cache and reset seek positions for new sample
    previewCache.valid = false;
    resetSeekRange();

    if (previewTriggerMode == PREVIEW_MODE_ON) {
      startSdPreview(OUTPUTf);
    } else if (previewTriggerMode == PREVIEW_MODE_PRESS) {
      // PREV==PRESS: start incremental SD peak scan (no audio playback yet)
      startPeakScan(OUTPUTf, true);
    }
  }
  
  if (previewTriggerMode == PREVIEW_MODE_PRESS && !playSdWav1.isPlaying()) {
    // Process peaks in smaller chunks with yields to avoid blocking
    servicePeakScan(4096);
    yield(); // Yield after peak processing to keep UI responsive
  }

  // Safety clamp
  if (GLOB.seekEnd > 100) {
    GLOB.seekEnd = 100;
    currentMode->pos[2] = GLOB.seekEnd;
    Encoder[1].writeCounter((int32_t)GLOB.seekEnd);
  }
  
  // Display peaks AFTER encoder processing (so display matches audio)
  refreshPeaksDisplay();

  // Optional overlay: show folder/file names at y=12, animated ping-pong
  static int scrollFileOffset = 0;
  static int scrollFolderOffset = 0;
  static unsigned long nextFileStepMs = 0;
  static unsigned long nextFolderStepMs = 0;
  static bool showFolderLine = false;  // true = show folder, false = show filename
  static unsigned long folderDisplayUntilMs = 0;  // When to switch back to filename after folder change
  static bool forceFolderDisplay = false;  // explicit flag to lock folder display until timer expires
  static int lastScrollFile = -1;
  static int lastScrollFolder = -1;
  static int lastEncoder1Value = -1;
  static int lastEncoder3Value = -1;
  static bool overlayInit = false;
  static int fileDir = 1;  // 1 forward, -1 backward
  static int folderDir = 1;

  auto textPixelWidth = [](const char* text) {
    int w = 0;
    for (int i = 0; text[i] != '\0'; i++) {
      char c = text[i];
      if (c < 32 || c > 126) continue;
      w += alphabet[c - 32][0] + 1;
    }
    return w;
  };

  auto renderLine = [&](const char* text, int y, CRGB color, int& offset, int& dir, unsigned long& nextStepMs, bool isFilename = false) {
    int viewStart = 1;
    int viewEnd = 14;
    int viewWidth = viewEnd - viewStart + 1;
    int width = textPixelWidth(text);
    int fullStrLen = strlen(text);  // Full string length, not displayed part

    // x=14 only if WHOLE string length is exactly 1
    if (fullStrLen == 1) {
      drawText(text, 14, y, color);
      offset = 0;
      dir = 1;
      return;
    }

    // For filenames longer than 4 chars, start marquee at x=4 instead of x=1
    int initialStartX = 1;
    if (isFilename && fullStrLen > 4) {
      initialStartX = 4;
    }

    // If short (<=4 chars) or fits in view, show immediately at initialStartX (no scroll)
    if (fullStrLen <= 4 || width <= viewWidth) {
      drawText(text, initialStartX, y, color);
      offset = 0;
      dir = 1;
      return;
    }

    // Scroll ping-pong: animate until last char is at x=14, wait, animate back until first char is at initialStartX, wait, repeat
    // offset represents pixels scrolled: when offset=0, startX=initialStartX
    // To get last char to x=14: startX + width - 1 = 14, so startX = 15 - width
    // Since startX = initialStartX - offset, we have: initialStartX - offset = 15 - width
    // So: offset = initialStartX + width - 15
    int offsetMax = max(0, initialStartX + width - 15);
    unsigned long now = millis();
    
    if (now >= nextStepMs) {
      if (dir > 0) {
        // Scrolling forward: move right until last char hits x=14
        offset++;
        if (offset >= offsetMax) {
          offset = offsetMax;
          dir = -1;
          nextStepMs = now + 500;  // pause at end
        } else {
          nextStepMs = now + 100;  // scroll speed
        }
      } else {
        // Scrolling backward: move left until first char is at initialStartX
        offset--;
        if (offset <= 0) {
          offset = 0;
          dir = 1;
          nextStepMs = now + 500;  // pause at start
        } else {
          nextStepMs = now + 100;  // scroll speed
        }
      }
    }
    
    int startX = initialStartX - offset;
    drawText(text, startX, y, color);
  };

  {
    // Detect encoder changes
    int currentEncoder1 = currentMode->pos[1];
    int currentEncoder3 = currentMode->pos[3];
    
    // Initialize overlay state once
    if (!overlayInit) {
      overlayInit = true;
      lastEncoder1Value = currentEncoder1;
      lastEncoder3Value = currentEncoder3;
      showFolderLine = false;  // Default to filename
      folderDisplayUntilMs = 0;
      forceFolderDisplay = false;
    }
    
    // Encoder 1 (folder) changed - FORCE show folder name for minimum 0.5s (block filename during this)
    if (currentEncoder1 != lastEncoder1Value) {
      forceFolderDisplay = true;
      showFolderLine = true;
      scrollFolderOffset = 0;
      folderDir = 1;
      unsigned long nowFolder = millis();
      nextFolderStepMs = nowFolder + 300;
      folderDisplayUntilMs = nowFolder + 1500;   // Show folder for minimum 0.5 second
      lastScrollFolder = folderIdx;
      lastEncoder1Value = currentEncoder1;
      // Folder change auto-sets fileIdx=1; keep overlay state in sync
      lastEncoder3Value = 1;
    }
    
    // Encoder 3 (sample) changed by user -> queue filename after folder timer
    if (!forceFolderDisplay && currentEncoder3 != lastEncoder3Value) {
      showFolderLine = false;
      scrollFileOffset = 0;
      fileDir = 1;
      nextFileStepMs = millis() + 300;
      lastScrollFile = fileIdx;
      lastEncoder3Value = currentEncoder3;
    }
    
    // Check timer: if folder was forced and timer expired, allow filename view
    unsigned long now = millis();
    if (forceFolderDisplay && folderDisplayUntilMs > 0 && now >= folderDisplayUntilMs) {
      // Timer expired: stop forcing folder, switch to filename
      forceFolderDisplay = false;
      folderDisplayUntilMs = 0;
      showFolderLine = false;
      scrollFileOffset = 0;
      fileDir = 1;
      nextFileStepMs = now + 300;
    }
    
    // FORCE folder display if flag is set (regardless of other state)
    if (forceFolderDisplay) {
      showFolderLine = true;
    }
    
    // Reset scroll when sample/folder ID changes (but not encoder change)
    if ((fileIdx != lastScrollFile || folderIdx != lastScrollFolder) && 
        currentEncoder1 == lastEncoder1Value && currentEncoder3 == lastEncoder3Value) {
      if (!showFolderLine) {
        scrollFileOffset = 0;
        fileDir = 1;
        nextFileStepMs = millis() + 300;
      } else {
        scrollFolderOffset = 0;
        folderDir = 1;
        nextFolderStepMs = millis() + 300;
      }
      lastScrollFile = fileIdx;
      lastScrollFolder = folderIdx;
    }

    char folderLabel[32];
    char fileLabel[32];
    // Always use manifest mode - never use legacy mode
    if (!manifestLoaded) {
      return; // Cannot proceed without manifest
    }
    int labelFolderIdx = (int)GLOB.folder;
    int localIdx = fileIdx;
    if (localIdx < 1) localIdx = 1;
    if (manifestLoaded && labelFolderIdx < (int)manifestFolderCount) {
      snprintf(folderLabel, sizeof(folderLabel), "%s", manifestFolderNames[labelFolderIdx]);
      int fileCount = (int)manifestFileCount[labelFolderIdx];
      if (localIdx >= 1 && localIdx <= fileCount) {
        const char* fname = manifestFileNames[labelFolderIdx][localIdx - 1];
        size_t len = strlen(fname);
        if (len > 4 && strcasecmp(fname + len - 4, ".wav") == 0) {
          // Copy without .wav
          size_t copyLen = len - 4;
          if (copyLen >= sizeof(fileLabel)) copyLen = sizeof(fileLabel) - 1;
          memcpy(fileLabel, fname, copyLen);
          fileLabel[copyLen] = '\0';
        } else {
          snprintf(fileLabel, sizeof(fileLabel), "%s", fname);
        }
      } else {
        snprintf(fileLabel, sizeof(fileLabel), "NEW");
      }
    } else {
      // Manifest not available: don't fall back to numeric names in the UI
      snprintf(folderLabel, sizeof(folderLabel), "NO MAP");
      snprintf(fileLabel, sizeof(fileLabel), "");
    }

    // Folder colors: col_Folder has only 10 entries; wrap safely for folder indexes >=10.
    const int folderColorCount = (int)(sizeof(col_Folder) / sizeof(col_Folder[0]));
    CRGB folderColor = col_Folder[(labelFolderIdx >= 0 ? labelFolderIdx : 0) % max(1, folderColorCount)];
    if (showFolderLine) {
      renderLine(folderLabel, 12, folderColor, scrollFolderOffset, folderDir, nextFolderStepMs, false);
    } else {
      renderLine(fileLabel, 12, folderColor, scrollFileOffset, fileDir, nextFileStepMs, true);
    }
  }
  
  yield(); // Yield at end of function to maintain responsiveness
}

// Copy currently loaded sample to samplepack 0
FLASHMEM void copySampleToSamplepack0(unsigned int channel) {
  // Ensure samplepack 0 directory exists
  if (!SD.exists("0")) {
    SD.mkdir("0");
  }
  
  char outputPath[50];
  sprintf(outputPath, "0/%d.wav", channel);
  
  // Remove existing file if it exists
  if (SD.exists(outputPath)) {
    SD.remove(outputPath);
  }
  
  // Open new file for writing
  File outFile = SD.open(outputPath, FILE_WRITE);
  if (!outFile) {
    return;
  }
  
  // Get sample data size
  uint32_t sampleCount = loadedSampleLen[channel];  // number of int16_t samples
  uint32_t dataSize = sampleCount * sizeof(int16_t);  // size in bytes
  
  // Write complete WAV header with correct sizes
  uint32_t sampleRate = (uint32_t)AUDIO_SAMPLE_RATE_EXACT;  // Convert to integer
  uint32_t byteRate = sampleRate * 1 * 16 / 8;  // mono, 16-bit
  uint8_t blockAlign = 1 * 16 / 8;  // mono, 16-bit
  
  // WAV header (44 bytes) with correct file size
  uint8_t header[44] = {
    'R', 'I', 'F', 'F',
    (uint8_t)((dataSize + 36) & 0xff), (uint8_t)(((dataSize + 36) >> 8) & 0xff),
    (uint8_t)(((dataSize + 36) >> 16) & 0xff), (uint8_t)(((dataSize + 36) >> 24) & 0xff),  // file size - 8
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  // PCM chunk size
    1, 0,         // Audio format (1 = PCM)
    1, 0,         // num channels (mono)
    (uint8_t)(sampleRate & 0xff), (uint8_t)((sampleRate >> 8) & 0xff),
    (uint8_t)((sampleRate >> 16) & 0xff), (uint8_t)((sampleRate >> 24) & 0xff),
    (uint8_t)(byteRate & 0xff), (uint8_t)((byteRate >> 8) & 0xff),
    (uint8_t)((byteRate >> 16) & 0xff), (uint8_t)((byteRate >> 24) & 0xff),
    blockAlign, 0,
    16, 0,  // bits per sample
    'd', 'a', 't', 'a',
    (uint8_t)(dataSize & 0xff), (uint8_t)((dataSize >> 8) & 0xff),
    (uint8_t)((dataSize >> 16) & 0xff), (uint8_t)((dataSize >> 24) & 0xff)  // data chunk size
  };
  
  outFile.write(header, 44);
  
  // Write the sample data from RAM
  // Write the raw PCM data from the sampled buffer in chunks to prevent blocking
  // Large samples (930KB) can block CPU for 500-1000ms without chunking
  uint8_t* dataPtr = reinterpret_cast<uint8_t*>(sampled[channel]);
  const size_t CHUNK_SIZE = 8192;  // 8KB chunks
  
  for (uint32_t offset = 0; offset < dataSize; offset += CHUNK_SIZE) {
    size_t chunkSize = min((size_t)CHUNK_SIZE, (size_t)(dataSize - offset));
    outFile.write(dataPtr + offset, chunkSize);
    // Yield periodically during large file writes to maintain responsiveness
    if ((offset % (CHUNK_SIZE * 4)) == 0) yield();  // Yield every 32KB
  }
  
  outFile.close();
  
  // Mark this channel as using samplepack 0
  SMP.sp0Active[channel] = true;
}

// Reverse the preview sample (channel 0) in RAM - used in showWave
void reversePreviewSample() {
  extern CachedSample previewCache;
  
  if (!previewCache.valid || previewCache.lengthBytes == 0) {
    return;
  }
  
  // Reverse the byte array in sampled[0]
  uint8_t* buffer = sampled[0];
  int numSamples = previewCache.lengthBytes / 2;  // Number of 16-bit samples
  
  // Reverse as int16_t samples (not individual bytes)
  int16_t* sampleBuffer = (int16_t*)buffer;
  for (int i = 0; i < numSamples / 2; i++) {
    int16_t temp = sampleBuffer[i];
    sampleBuffer[i] = sampleBuffer[numSamples - 1 - i];
    sampleBuffer[numSamples - 1 - i] = temp;
  }
  
  // Flip the peak visualization values array
  extern int peakIndex;
  extern float peakValues[];
  
  for (int i = 0; i < peakIndex / 2; i++) {
    float temp = peakValues[i];
    peakValues[i] = peakValues[peakIndex - 1 - i];
    peakValues[peakIndex - 1 - i] = temp;
  }
  
  // Swap seek and seekEnd positions (they're now reversed)
  int tempSeek = GLOB.seek;
  GLOB.seek = 100 - GLOB.seekEnd;
  GLOB.seekEnd = 100 - tempSeek;
  
  // Update encoder positions to match
  extern Mode* currentMode;
  currentMode->pos[0] = GLOB.seek;
  currentMode->pos[2] = GLOB.seekEnd;
  
  extern i2cEncoderLibV2 Encoder[];
  Encoder[0].writeCounter((int32_t)GLOB.seek);
  Encoder[1].writeCounter((int32_t)GLOB.seekEnd);
  
  // Trigger re-preview with current seek settings
  // Re-preview using manifest selection (folderIdx in oldID, fileIdx in fileID)
  int folderIdx = (int)SMP.wav[GLOB.currentChannel].oldID;
  int fileIdx = (int)SMP.wav[GLOB.currentChannel].fileID;
  if (fileIdx < 1) fileIdx = 1;
  previewSample(folderIdx, fileIdx, false);
}

// Copy the preview sample (channel 0) to the target channel
void loadPreviewToChannel(unsigned int targetChannel) {
  if (targetChannel < 1 || targetChannel >= maxFiles) {
    return;
  }
  
  extern CachedSample previewCache;
  
  bool needToLoadPreview = (!previewCache.valid || previewCache.lengthBytes == 0);
  
  // If preview cache is not valid, ensure the sample is loaded first
  if (needToLoadPreview) {
    int folderIdx = (int)SMP.wav[GLOB.currentChannel].oldID;
    int fileIdx = (int)SMP.wav[GLOB.currentChannel].fileID;
    if (fileIdx < 1) fileIdx = 1;
    
    // Load the sample into cache without triggering preview playback
    // We'll manually load it into sampled[0] and set up the cache
    char OUTPUTf[64];
    buildSamplePath(folderIdx, fileIdx, OUTPUTf, sizeof(OUTPUTf));
    
    yield(); // Yield before SD.open() to prevent blocking main sequencer
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      return;
    }

    int fileSize = previewFile.size();
    uint32_t dataStart = 44, dataSize = 0;
    if (!findWavDataChunk(previewFile, dataStart, dataSize)) {
      dataStart = 44;
      dataSize = (uint32_t)max(0, fileSize - 44);
    }
    
    // All samples are 44.1kHz Mono - no need to check sample rate
    // For 44.1kHz, the original code used rate = 3 (byte 0x44 at offset 24)
    int rate = 3;  // 44.1kHz Mono (original mapping: byte 68 = 0x44)
    
    // Load data chunk into RAM buffer (no blanket memset)
    previewFile.seek(dataStart);
    int plen = 0;
    // Read in chunks with periodic yield() to prevent blocking CPU during large file operations
    // Larger chunk to reduce SD overhead on big files
    uint8_t chunk[2048];  // Read 2KB at a time
    int chunkCount = 0;
    int maxToRead = (int)min((uint32_t)sizeof(sampled[0]), dataSize);
    while (previewFile.available() && plen < maxToRead) {
      size_t toRead = min(sizeof(chunk), (size_t)(sizeof(sampled[0]) - plen));
      size_t bytesRead = previewFile.read(chunk, toRead);
      if (bytesRead == 0) break;
      memcpy(&sampled[0][plen], chunk, bytesRead);
      plen += bytesRead;
      // Yield every chunk to maintain responsiveness (critical for long samples)
      yield();
      if ((++chunkCount & 0x3) == 0) yield();
    }
    previewFile.close();
    
    // Update cache
    previewCache.folder = folderIdx;
    previewCache.sampleID = fileIdx;
    previewCache.lengthBytes = plen;
    previewCache.rate = rate;
    previewCache.valid = true;
    previewCache.plen = plen;
  }
  
  // Calculate the trimmed portion based on seek/seekEnd
  int numSamples = previewCache.lengthBytes / 2;  // Total samples in preview
  int startSample = (numSamples * GLOB.seek) / 100;
  // Same end-click guard as loadSample(): trim by 1 seekEnd step for playback.
  int seekEndPct = (GLOB.seekEnd == 0) ? 100 : (int)GLOB.seekEnd;
  if (seekEndPct > 0) seekEndPct -= 1;
  if (seekEndPct <= (int)GLOB.seek) seekEndPct = min(100, (int)GLOB.seek + 1);

  int endSample = (numSamples * seekEndPct) / 100;
  
  if (endSample <= startSample) {
    endSample = numSamples;
  }
  
  int trimmedSamples = endSample - startSample;
  
  // Clear target buffer first to remove any old sample data
  memset(sampled[targetChannel], 0, sizeof(sampled[targetChannel]));
  
  // Copy the trimmed portion from preview (channel 0) to target channel
  int16_t* previewBuffer = (int16_t*)sampled[0];
  int16_t* targetBuffer = (int16_t*)sampled[targetChannel];
  
  for (int i = 0; i < trimmedSamples && i < (sizeof(sampled[targetChannel]) / 2); i++) {
    targetBuffer[i] = previewBuffer[startSample + i];
  }
  
  // Update the sampler for the target channel
  _samplers[targetChannel].removeAllSamples();
  loadedSampleRate[targetChannel] = previewCache.rate;
  // Add small zero-tail pad (see loadSample()) to reduce end-clicks through FX chain.
  const int END_PAD_SAMPLES = 1024;
  int bufferSamples = (int)(sizeof(sampled[targetChannel]) / 2);
  int samplesRead = trimmedSamples;
  if (samplesRead < 0) samplesRead = 0;
  if (samplesRead > bufferSamples) samplesRead = bufferSamples;
  int pad = END_PAD_SAMPLES;
  if (pad > (bufferSamples - samplesRead)) pad = (bufferSamples - samplesRead);
  
  // Explicitly zero the pad area to ensure no old data remains
  if (pad > 0 && (samplesRead + pad) <= bufferSamples) {
    memset(&sampled[targetChannel][samplesRead * 2], 0, pad * 2);
  }
  
  int playLen = samplesRead + pad;

  loadedSampleLen[targetChannel] = playLen;
  _samplers[targetChannel].addSample(36, targetBuffer, playLen, rateFactor);
  channelDirection[targetChannel] = 1;
  
  // Persist manifest selection for the target channel (folderIdx in oldID, fileIdx in fileID)
  SMP.wav[targetChannel].oldID = SMP.wav[GLOB.currentChannel].oldID;
  SMP.wav[targetChannel].fileID = SMP.wav[GLOB.currentChannel].fileID;
}
