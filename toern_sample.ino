
extern int8_t channelDirection[maxFiles];

// Track last sample selection in showWave() across calls
static int lastEncoder3Value_forShowWave = -1;

FLASHMEM void previewSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength) {
  if (playSdWav1.isPlaying()) playSdWav1.stop();
  envelope0.noteOff();
  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, sampleID);

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
    peakIndex = 0;
    memset(peakValues, 0, sizeof(peakValues));

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

  char OUTPUTf[50];
  
  // Check if we're loading from samplepack 0 (check if file exists first)
  sprintf(OUTPUTf, "0/%d.wav", sampleID);
  bool loadingFromSp0 = (packID == 0 && SD.exists(OUTPUTf));
  
  if (loadingFromSp0) {
    // Loading from samplepack 0 - path is already correct
  } else if (packID == 0) {
    // Old behavior: loading individual sample from samples folder
    sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(sampleID), sampleID);
    sampleID = GLOB.currentChannel;
  } else {
    // Loading from regular samplepack
    sprintf(OUTPUTf, "%d/%d.wav", packID, sampleID);
  }

  if (!SD.exists(OUTPUTf)) {
    setMuteState(sampleID, true);
    return;
  } else {
    setMuteState(sampleID, false);
  }

  

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
    int snr = SMP.wav[GLOB.currentChannel].fileID;
    if (snr < 1) snr = 1;
    snr = constrain(snr, 1, 999);
    currentMode->pos[3] = snr;
    Encoder[3].writeCounter((int32_t)snr);
    lastEncoder3Value_forShowWave = -1;  // Force re-preview
  }
  
  int snr = SMP.wav[GLOB.currentChannel].fileID;
  if (snr < 1) snr = 1;
  int fnr = getFolderNumber(snr);
  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
  
  firstcheck = false;
  nofile = false;

  FastLEDclear();
  
  // New indicator system: wave: M[CH] | M[W] | S[P] | L[X]
  drawIndicator('L', 'C', 1);  // Encoder 1: Medium Current Channel Color
  drawIndicator('L', 'W', 2);  // Encoder 2: Large White
  drawIndicator('L', 'Y', 3);  // Encoder 3: Large Yellow
  drawIndicator('L', 'X', 4);  // Encoder 4: Large Blue
  
  // Set encoder colors to match indicators
  CRGB channelColor = getCurrentChannelColor();
  Encoder[0].writeRGBCode(channelColor.r << 16 | channelColor.g << 8 | channelColor.b);
  Encoder[1].writeRGBCode(0xFFFFFF); // White
  Encoder[2].writeRGBCode(0xFFFF00); // Yellow
  Encoder[2].writeMax((int32_t)100);
  if (currentMode->pos[2] > 100) {
    currentMode->pos[2] = 100;
    Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
  }
  Encoder[3].writeRGBCode(0x0000FF); // Blue

  // ---- Encoder 0: Seek start ----------------------------------------------
  if (sampleIsLoaded) {
    int newSeek = constrain(currentMode->pos[0], 0, GLOB.seekEnd - 1);  // Ensure seek < seekEnd
    if (newSeek != GLOB.seek) {
      GLOB.seek = newSeek;
      currentMode->pos[0] = newSeek;  // Update encoder to match
      yield(); // Yield before file operations
      previewSample(fnr, getFileNumber(snr), false);  // Uses cached buffer when available
    }
  }

  // ---- Encoder 1: Folder --------------------------------------------------
  if (currentMode->pos[1] != GLOB.folder) {
    yield(); // Yield before audio/file operations
    envelope0.noteOff();
    playSdWav1.stop();
    firstcheck = true;
    nofile = false;
    GLOB.folder = currentMode->pos[1];
    SMP.wav[GLOB.currentChannel].fileID = ((GLOB.folder - 1) * 100) + 1;
    Encoder[3].writeCounter((int32_t)SMP.wav[GLOB.currentChannel].fileID);
    lastEncoder3Value_forShowWave = -1;  // Force re-preview after folder change
  }

  // ---- Encoder 2: Seek end ------------------------------------------------
  if (sampleIsLoaded) {
    int newSeekEnd = constrain(currentMode->pos[2], GLOB.seek + 1, 100);  // Ensure seekEnd > seek
    if (newSeekEnd != GLOB.seekEnd) {
      GLOB.seekEnd = newSeekEnd;
      currentMode->pos[2] = newSeekEnd;  // Update encoder to match
      yield(); // Yield before file operations
      previewSample(fnr, getFileNumber(snr), false);  // Uses cached buffer when available
    }
  }

  // ---- Encoder 3: Sample selection ---------------------------------------
  int encoderSampleValue = constrain(currentMode->pos[3], 1, 999);
  if (encoderSampleValue != currentMode->pos[3]) {
    currentMode->pos[3] = encoderSampleValue;
    Encoder[3].writeCounter((int32_t)encoderSampleValue);
  }

  bool sampleChanged = (encoderSampleValue != lastEncoder3Value_forShowWave) ||
                       (encoderSampleValue != SMP.wav[GLOB.currentChannel].fileID);

  if (sampleChanged) {
    lastEncoder3Value_forShowWave = encoderSampleValue;
    snr = encoderSampleValue;
    SMP.wav[GLOB.currentChannel].fileID = snr;
    fnr = getFolderNumber(snr);

    // Reset preview / state
    yield(); // Yield before file operations
    envelope0.noteOff();
    previewIsPlaying = false;
    playSdWav1.stop();
    sampleIsLoaded = false;
    firstcheck = true;
    nofile = false;

    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
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

    startSdPreview(OUTPUTf);
  }

  // Safety clamp
  if (GLOB.seekEnd > 100) {
    GLOB.seekEnd = 100;
    currentMode->pos[2] = GLOB.seekEnd;
    Encoder[2].writeCounter((int32_t)GLOB.seekEnd);
  }
  
  // Display peaks AFTER encoder processing (so display matches audio)
  refreshPeaksDisplay();
  drawNumber(snr, col_Folder[fnr], 12);
  
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
  extern int getFolderNumber(int fileID);
  extern int getFileNumber(int fileID);
  
  int snr = SMP.wav[GLOB.currentChannel].fileID;
  int fnr = getFolderNumber(snr);
  
  previewSample(fnr, getFileNumber(snr), false);
}

// Copy the preview sample (channel 0) to the target channel
void loadPreviewToChannel(unsigned int targetChannel) {
  if (targetChannel < 1 || targetChannel >= maxFiles) {
    return;
  }
  
  extern CachedSample previewCache;
  extern int getFolderNumber(int fileID);
  extern int getFileNumber(int fileID);
  
  bool needToLoadPreview = (!previewCache.valid || previewCache.lengthBytes == 0);
  
  // If preview cache is not valid, ensure the sample is loaded first
  if (needToLoadPreview) {
    int snr = SMP.wav[GLOB.currentChannel].fileID;
    int fnr = getFolderNumber(snr);
    
    // Load the sample into cache without triggering preview playback
    // We'll manually load it into sampled[0] and set up the cache
    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
    
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
    previewCache.folder = fnr;
    previewCache.sampleID = getFileNumber(snr);
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
  
  // Update the target channel's fileID to match the current channel's fileID
  // This ensures the saved pattern remembers which sample is loaded
  SMP.wav[targetChannel].fileID = SMP.wav[GLOB.currentChannel].fileID;
}
