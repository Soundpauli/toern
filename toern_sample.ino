
extern int8_t channelDirection[maxFiles];
extern bool manifestLoaded;
extern uint16_t manifestFolderCount;
extern uint16_t manifestFileCount[];
extern char manifestFolderNames[][24];
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
  if (f.read(hdr, sizeof(hdr)) != (int)sizeof(hdr)) return false;
  if (!(hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F' && hdr[8]=='W' && hdr[9]=='A' && hdr[10]=='V' && hdr[11]=='E')) {
    return false;
  }
  f.seek(12);
  while (f.available()) {
    char id[4];
    if (f.read((uint8_t*)id, 4) != 4) break;
    uint8_t szb[4];
    if (f.read(szb, 4) != 4) break;
    uint32_t sz = (uint32_t)szb[0] | ((uint32_t)szb[1] << 8) | ((uint32_t)szb[2] << 16) | ((uint32_t)szb[3] << 24);
    if (id[0]=='d' && id[1]=='a' && id[2]=='t' && id[3]=='a') {
      outStart = (uint32_t)f.position();
      outSize = sz;
      return (outSize >= 2);
    }
    uint32_t skip = sz + (sz & 1);
    uint32_t nextPos = (uint32_t)f.position() + skip;
    if (!f.seek(nextPos)) break;
  }
  return false;
}

static void stopPeakScan() {
  if (g_peakScan.active) {
    g_peakScan.f.close();
    g_peakScan.active = false;
  }
}

static void startPeakScan(const char *path) {
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
  g_peakScan.totalSamples = dz / 2; // assume 16-bit PCM
  g_peakScan.sampleIdx = 0;
  g_peakScan.outIndex = 0;
  g_peakScan.samplesInWindow = 0;
  g_peakScan.windowPeak = 0;
  g_peakScan.active = true;

  peakIndex = 0;
  memset(peakValues, 0, sizeof(peakValues));
  g_peakScan.f.seek(g_peakScan.dataStart);
}

static void servicePeakScan(uint32_t maxBytesThisCall) {
  if (!g_peakScan.active) return;
  if (!g_peakScan.f.available()) { stopPeakScan(); return; }

  // Emulate the "shape" of AudioAnalyzePeak capture:
  // In PREV==ON you effectively sample peaks at ~15ms intervals (see main loop).
  // 15ms @ 44.1kHz ≈ 662 samples. Use a fixed window of ~640 samples (5 audio blocks of 128).
  const uint32_t WINDOW_SAMPLES = 640; // close to 15ms stride; produces similar visual detail

  uint8_t buf[512];
  uint32_t bytesLeft = maxBytesThisCall;
  while (bytesLeft > 0 && g_peakScan.active && g_peakScan.outIndex < (uint16_t)maxPeaks) {
    uint32_t toRead = min((uint32_t)sizeof(buf), bytesLeft);
    int n = g_peakScan.f.read(buf, (int)toRead);
    if (n <= 0) { stopPeakScan(); break; }
    bytesLeft -= (uint32_t)n;

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
        peakValues[g_peakScan.outIndex] = (float)g_peakScan.windowPeak / 32768.0f;
        g_peakScan.outIndex++;
        g_peakScan.samplesInWindow = 0;
        g_peakScan.windowPeak = 0;
        peakIndex = (int)g_peakScan.outIndex; // progressive display
        if (g_peakScan.outIndex >= (uint16_t)maxPeaks) { stopPeakScan(); break; }
      }

      if (g_peakScan.sampleIdx >= g_peakScan.totalSamples) {
        // Flush any partial window so the waveform doesn't "truncate early"
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
  if (playSdWav1.isPlaying()) playSdWav1.stop();
  envelope0.noteOff();
  // PREV==PRSS: stop background peak scanning before any SD-heavy preview work
  // (avoids SD contention and potential crashes when starting preview playback).
  if (previewTriggerMode == PREVIEW_MODE_PRESS) {
    stopPeakScan();
  }
  char OUTPUTf[64];
  buildSamplePath(folder, sampleID, OUTPUTf, sizeof(OUTPUTf));

  // If no trimming is requested (seek at 0, seekEnd at 100), avoid loading into RAM
  // and stream directly from SD to keep memory usage low.
  bool usingFullRange = (GLOB.seek == 0) && (GLOB.seekEnd == 100 || GLOB.seekEnd == 0);
  if (usingFullRange) {
    if (!SD.exists(OUTPUTf)) {
      return;
    }

    // Normalize seekEnd if it was left at 0 (treat as 100%)
    if (GLOB.seekEnd == 0) {
      GLOB.seekEnd = 100;
      currentMode->pos[2] = GLOB.seekEnd;
      Encoder[2].writeCounter((int32_t)GLOB.seekEnd);
    }

    // Clear cached preview metadata so trimmed previews will reload when needed
    previewCache.valid = false;
    previewCache.lengthBytes = 0;
    previewCache.plen = 0;

    previewIsPlaying = true;
    // Only reset peaks when PREV==ON. In PREV==PRSS peaks are generated by the silent peak builder.
    if (previewTriggerMode == PREVIEW_MODE_ON) {
      peakIndex = 0;
      memset(peakValues, 0, sizeof(peakValues));
    }
    playSdWav1.play(OUTPUTf);  // Stream from SD (no RAM copy)
    sampleIsLoaded = true;
    return;
  }

  // --- Check if already cached ---
  if (!previewCache.valid || previewCache.folder != folder || previewCache.sampleID != sampleID) {
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      return;
    }

    int fileSize = previewFile.size();

    // Read sample rate from header at offset 24
    previewFile.seek(24);
    int g = previewFile.read();
    if (g == 72) PrevSampleRate = 4;
    else if (g == 68) PrevSampleRate = 3;
    else if (g == 34) PrevSampleRate = 2;
    else if (g == 17) PrevSampleRate = 1;
    else PrevSampleRate = 4;

    GLOB.smplen = fileSize / (PrevSampleRate * 2);

    // Decide whether to load full file or only the trimmed window to keep UI responsive
    // Trimmed window applies when not forcing max length and the file is larger than buffer.
    bool loadPartial = (!setMaxSampleLength) && (fileSize > (int)sizeof(sampled[0]));

    // Load full sample into RAM buffer (no blanket memset to keep this fast on large files)
    int plen = 0;
    int readStartBytes = 44;
    int bytesToReadAll = fileSize - 44;
    if (loadPartial) {
      // Compute requested window in bytes based on seek/seekEnd (after smplen computed)
      int startOffset = (GLOB.smplen * GLOB.seek) / 100;
      int endOffset = (GLOB.smplen * GLOB.seekEnd) / 100;
      endOffset = min(endOffset, GLOB.smplen);
      int startOffsetBytes = startOffset * PrevSampleRate * 2;
      int endOffsetBytes = endOffset * PrevSampleRate * 2;
      startOffsetBytes &= ~1;
      endOffsetBytes &= ~1;
      if (endOffsetBytes <= startOffsetBytes) {
        startOffsetBytes = 0;
        endOffsetBytes = min(fileSize - 44, (int)sizeof(sampled[0]));
      }
      readStartBytes = 44 + startOffsetBytes;
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
    PrevSampleRate = previewCache.rate;
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
    Encoder[2].writeCounter((int32_t)GLOB.seekEnd);
  }

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
    loadSample.seek(24);
    for (uint8_t i = 24; i < 25; i++) {
      int g = loadSample.read();
      if (g == 0) SampleRate[sampleID] = 4;
      if (g == 17) SampleRate[sampleID] = 1;
      if (g == 34) SampleRate[sampleID] = 2;
      if (g == 68) SampleRate[sampleID] = 3;
      if (g == 72) SampleRate[sampleID] = 4;
    }

    // Calculate sample length in milliseconds
    GLOB.smplen = fileSize / (SampleRate[sampleID] * 2);

    // Convert percentage positions to actual offsets
    unsigned int startOffset = (GLOB.seek * GLOB.smplen) / 100;
    unsigned int startOffsetBytes = startOffset * SampleRate[sampleID] * 2;

    unsigned int endOffset = (GLOB.seekEnd * GLOB.smplen) / 100;
    if (endOffset > GLOB.smplen) endOffset = GLOB.smplen;
    if (GLOB.seekEnd == 0) {
      endOffset = GLOB.smplen;  // Full length if seekEnd is 0
      GLOB.seekEnd = 100;       // Set to 100%
    }
    unsigned int endOffsetBytes = endOffset * SampleRate[sampleID] * 2;
    endOffsetBytes = min(endOffsetBytes, fileSize);

    loadSample.seek(44 + startOffsetBytes);
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

    i = i / 2;
    _samplers[sampleID].removeAllSamples();
    loadedSampleRate[sampleID] = SampleRate[sampleID];  // e.g. 44100, or whatever
    loadedSampleLen[sampleID] = i; 
    _samplers[sampleID].addSample(36, (int16_t *)sampled[sampleID], (int)i/1 , rateFactor);
    channelDirection[sampleID] = 1;
  }
  yield();
}




void showWave() {
  // Ensure manifest is available so folder browsing uses real folder names
  if (!manifestLoaded) {
    if (!loadSampleManifest()) {
      scanAndWriteManifest();
    }
  }
  // ---- Helpers -------------------------------------------------------------
  auto resetSeekRange = []() {
    currentMode->pos[0] = 0;
    GLOB.seek = 0;
    Encoder[0].writeCounter((int32_t)0);
    currentMode->pos[2] = 100;
    GLOB.seekEnd = 100;
    Encoder[2].writeCounter((int32_t)100);
  };

  auto startSdPreview = [&](const char* path) {
    previewIsPlaying = true;
    peakIndex = 0;
    memset(peakValues, 0, sizeof(peakValues));
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

    // Legacy migration: older saves stored a combined 3-digit-ish ID in fileID (folder*100+local).
    // That overflows the 10-bit field once folder>=10 and local>=24, and also doesn't work with manifest indexing.
    // Detect and convert to (folderIdx,fileIdx) in manifest space when possible.
    if (manifestLoaded) {
      bool folderValid = (folderIdx >= 0 && folderIdx < (int)manifestFolderCount);
      bool fileValid = false;
      if (folderValid) {
        int fc = (int)manifestFileCount[folderIdx];
        fileValid = (fileIdx >= 1 && fileIdx <= fc + 1);
      }
      if (!folderValid || !fileValid) {
        int legacyCombined = fileIdx;
        if (legacyCombined >= 100) {
          int legacyFolder = legacyCombined / 100;
          int legacyLocal = legacyCombined % 100;
          legacyFolder = constrain(legacyFolder, 0, (int)manifestFolderCount - 1);
          char legacyName[16];
          snprintf(legacyName, sizeof(legacyName), "_%d.wav", legacyLocal);
          int mapped = manifestFindFileIdx(legacyFolder, legacyName);
          if (mapped < 1) {
            // Fallback: treat legacyLocal as a 1-based index into the manifest list
            int fc = (int)manifestFileCount[legacyFolder];
            mapped = constrain(legacyLocal, 1, max(1, fc));
          }
          folderIdx = legacyFolder;
          fileIdx = mapped;
          SMP.wav[GLOB.currentChannel].oldID = (unsigned int)folderIdx;
          SMP.wav[GLOB.currentChannel].fileID = (unsigned int)fileIdx;
        } else {
          // If it's just out of range, clamp into a valid spot
          folderIdx = constrain(folderIdx, 0, (int)manifestFolderCount - 1);
          int fc = (int)manifestFileCount[folderIdx];
          fileIdx = constrain(fileIdx, 1, max(1, fc + 1));
          SMP.wav[GLOB.currentChannel].oldID = (unsigned int)folderIdx;
          SMP.wav[GLOB.currentChannel].fileID = (unsigned int)fileIdx;
        }
      }
    }

    if (manifestLoaded) folderIdx = constrain(folderIdx, 0, (int)manifestFolderCount - 1);
    if (folderIdx < 0) folderIdx = 0;
    GLOB.folder = (unsigned int)folderIdx;
    currentMode->pos[1] = folderIdx;
    Encoder[1].writeCounter((int32_t)folderIdx);

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
  
  int folderIdx = manifestLoaded ? (int)GLOB.folder : (int)SMP.wav[GLOB.currentChannel].oldID;
  if (manifestLoaded) folderIdx = constrain(folderIdx, 0, (int)manifestFolderCount - 1);
  if (folderIdx < 0) folderIdx = 0;
  int fileIdx = (int)SMP.wav[GLOB.currentChannel].fileID;
  if (fileIdx < 1) fileIdx = 1;
  char OUTPUTf[64];
  buildSamplePath(folderIdx, fileIdx, OUTPUTf, sizeof(OUTPUTf));
  
  firstcheck = false;
  nofile = false;

  FastLEDclear();
  
  // Clamp folder encoder to existing folders (manifest if loaded)
  int maxFolders = manifestLoaded ? manifestFolderCount : FOLDER_MAX;
  if (maxFolders < 1) maxFolders = 1;
  Encoder[1].writeMin((int32_t)0);
  Encoder[1].writeMax((int32_t)(maxFolders - 1));
  if ((int)currentMode->pos[1] >= maxFolders) {
    currentMode->pos[1] = maxFolders - 1;
    Encoder[1].writeCounter((int32_t)currentMode->pos[1]);
  }
  
  // Indicators: ENC1 folder, ENC2 trim, ENC3 load
  drawIndicator('L', 'W', 2);  // Encoder 1: folder browse
  drawIndicator('L', 'Y', 3);  // Encoder 2: trim/seek
  drawIndicator('L', 'G', 4);  // Encoder 3: load sample
  if (previewTriggerMode == PREVIEW_MODE_PRESS) {
    // Preview-on-press mode: show encoder 0 in pink
    drawIndicator('L', 'P', 1);
    Encoder[0].writeRGBCode(0xFF00FF);
  } else {
    // Default: no indicator/color on encoder 0 in set_wav
    Encoder[0].writeRGBCode(0x000000);
  }
  
  // Set encoder colors to match indicators
  Encoder[1].writeRGBCode(0xFFFFFF); // White
  Encoder[2].writeRGBCode(0xFFFF00); // Yellow
  Encoder[2].writeMax((int32_t)100);
  if (currentMode->pos[2] > 100) {
    currentMode->pos[2] = 100;
    Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
  }
  Encoder[3].writeRGBCode(0x00FF00); // Green

  // ---- Encoder 0: Seek start ----------------------------------------------
  {
    int newSeek = constrain(currentMode->pos[0], 0, GLOB.seekEnd - 1);  // Ensure seek < seekEnd
    if (newSeek != GLOB.seek) {
      GLOB.seek = newSeek;
      currentMode->pos[0] = newSeek;  // Update encoder to match
      if (sampleIsLoaded && previewTriggerMode == PREVIEW_MODE_ON) {
      yield(); // Yield before file operations
      previewSample(folderIdx, fileIdx, false);  // Uses cached buffer when available
      }
    }
  }

  // ---- Encoder 1: Folder --------------------------------------------------
  // Read encoder counter directly and clamp to existing folders
  int32_t encoder1Counter = Encoder[1].readCounterInt();
  // Reuse maxFolders from above (already calculated)
  int clampedFolderPos = constrain((int)encoder1Counter, 0, maxFolders - 1);
  
  // Sync encoder hardware and mode position if clamped
  if (clampedFolderPos != (int)encoder1Counter) {
    Encoder[1].writeCounter((int32_t)clampedFolderPos);
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

  // ---- Encoder 2: Seek end ------------------------------------------------
  {
    int newSeekEnd = constrain(currentMode->pos[2], GLOB.seek + 1, 100);  // Ensure seekEnd > seek
    if (newSeekEnd != GLOB.seekEnd) {
      GLOB.seekEnd = newSeekEnd;
      currentMode->pos[2] = newSeekEnd;  // Update encoder to match
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

    // Log file info for debugging large-sample browsing
    static unsigned long lastInfoLogMs = 0;
    unsigned long nowInfo = millis();
    if (nowInfo - lastInfoLogMs > 150) {
      lastInfoLogMs = nowInfo;
      File infoFile = SD.open(OUTPUTf);
      if (infoFile) {
        int fileSize = infoFile.size();
        float ramUsePct = (float)fileSize / (float)sizeof(sampled[0]) * 100.0f;
        Serial.printf("SETWAV preview %s size=%.1fKB buf=%.1f%%\n", OUTPUTf, fileSize / 1024.0f, ramUsePct);
        infoFile.close();
      }
    }

    // Invalidate preview cache and reset seek positions for new sample
    previewCache.valid = false;
    resetSeekRange();

    if (previewTriggerMode == PREVIEW_MODE_ON) {
      startSdPreview(OUTPUTf);
    } else if (previewTriggerMode == PREVIEW_MODE_PRESS) {
      // PREV==PRSS: start (or restart) incremental SD peak scan immediately (no debounce)
      startPeakScan(OUTPUTf);
    }
  }
  // Advance peak scan in small slices to keep UI responsive and avoid audio glitches
  if (previewTriggerMode == PREVIEW_MODE_PRESS) {
    servicePeakScan(1024); // ~1KB per frame
  }

  // Safety clamp
  if (GLOB.seekEnd > 100) {
    GLOB.seekEnd = 100;
    currentMode->pos[2] = GLOB.seekEnd;
    Encoder[2].writeCounter((int32_t)GLOB.seekEnd);
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
    int labelFolderIdx = manifestLoaded ? (int)GLOB.folder : folderIdx;
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
  Encoder[2].writeCounter((int32_t)GLOB.seekEnd);
  
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
    
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      return;
    }
    
    int fileSize = previewFile.size();
    
    // Read sample rate from header
    previewFile.seek(24);
    int g = previewFile.read();
    int rate;
    if (g == 72) rate = 4;
    else if (g == 68) rate = 3;
    else if (g == 34) rate = 2;
    else if (g == 17) rate = 1;
    else rate = 4;
    
    // Load full sample into RAM buffer (no blanket memset)
    previewFile.seek(44);
    int plen = 0;
    // Read in chunks with periodic yield() to prevent blocking CPU during large file operations
    // Larger chunk to reduce SD overhead on big files
    uint8_t chunk[2048];  // Read 2KB at a time
    int chunkCount = 0;
    while (previewFile.available() && plen < sizeof(sampled[0])) {
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
  int endSample = (numSamples * GLOB.seekEnd) / 100;
  
  if (endSample <= startSample) {
    endSample = numSamples;
  }
  
  int trimmedSamples = endSample - startSample;
  
  // Copy the trimmed portion from preview (channel 0) to target channel
  int16_t* previewBuffer = (int16_t*)sampled[0];
  int16_t* targetBuffer = (int16_t*)sampled[targetChannel];
  
  for (int i = 0; i < trimmedSamples && i < (sizeof(sampled[targetChannel]) / 2); i++) {
    targetBuffer[i] = previewBuffer[startSample + i];
  }
  
  // Update the sampler for the target channel
  _samplers[targetChannel].removeAllSamples();
  loadedSampleRate[targetChannel] = previewCache.rate;
  loadedSampleLen[targetChannel] = trimmedSamples;
  _samplers[targetChannel].addSample(36, targetBuffer, trimmedSamples, rateFactor);
  channelDirection[targetChannel] = 1;
  
  // Persist manifest selection for the target channel (folderIdx in oldID, fileIdx in fileID)
  SMP.wav[targetChannel].oldID = SMP.wav[GLOB.currentChannel].oldID;
  SMP.wav[targetChannel].fileID = SMP.wav[GLOB.currentChannel].fileID;
}
