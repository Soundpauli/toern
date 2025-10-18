
void previewSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength, bool firstPreview) {
  if (playSdWav1.isPlaying()) playSdWav1.stop();
  envelope0.noteOff();
  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, sampleID);

  // --- Check if already cached ---
  if (!previewCache.valid || previewCache.folder != folder || previewCache.sampleID != sampleID) {
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      //Serial.println("File not found!");
      return;
    }

    int fileSize = previewFile.size();
    if (firstPreview) {
      fileSize = min(fileSize, 302000);  // max 10 seconds
    }

    // Read sample rate from header at offset 24
    previewFile.seek(24);
    int g = previewFile.read();
    if (g == 72) PrevSampleRate = 4;
    else if (g == 68) PrevSampleRate = 3;
    else if (g == 34) PrevSampleRate = 2;
    else if (g == 17) PrevSampleRate = 1;
    else PrevSampleRate = 4;

    GLOB.smplen = fileSize / (PrevSampleRate * 2);

    // Load full sample into RAM buffer
    previewFile.seek(44);
    memset(sampled[0], 0, sizeof(sampled[0]));
    int plen = 0;
    while (previewFile.available() && plen < sizeof(sampled[0])) {
      sampled[0][plen++] = previewFile.read();
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
    //Serial.println("Invalid sample range; skipping playback.");
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

  //Serial.println("NOTE");
  _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
}



void loadSample(unsigned int packID, unsigned int sampleID) {
  Serial.print("loadSample: packID=");
  Serial.print(packID);
  Serial.print(", sampleID=");
  Serial.println(sampleID);
    

  drawNoSD();

  char OUTPUTf[50];
  
  // Check if we're loading from samplepack 0 (check if file exists first)
  sprintf(OUTPUTf, "0/%d.wav", sampleID);
  bool loadingFromSp0 = (packID == 0 && SD.exists(OUTPUTf));
  
  if (loadingFromSp0) {
    // Loading from samplepack 0 - path is already correct
    Serial.print("Loading from SP0: ");
    Serial.println(OUTPUTf);
  } else if (packID == 0) {
    // Old behavior: loading individual sample from samples folder
    sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(sampleID), sampleID);
    sampleID = GLOB.currentChannel;
    Serial.print("Loading from samples folder: ");
    Serial.println(OUTPUTf);
  } else {
    // Loading from regular samplepack
    sprintf(OUTPUTf, "%d/%d.wav", packID, sampleID);
    Serial.print("Loading from pack: ");
    Serial.println(OUTPUTf);
  }

  if (!SD.exists(OUTPUTf)) {
    //Serial.print("File does not exist: ");
    //Serial.println(OUTPUTf);
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
    if (GLOB.seekEnd == 0) {
      endOffset = GLOB.smplen;  // Full length if seekEnd is 0
      GLOB.seekEnd = 100;       // Set to 100%
    }
    unsigned int endOffsetBytes = endOffset * SampleRate[sampleID] * 2;
    endOffsetBytes = min(endOffsetBytes, fileSize);

    loadSample.seek(44 + startOffsetBytes);
    unsigned int i = 0;
    //memset(sampled[sampleID], 0, sizeof(sampled[sampleID]));
    memset((void *)sampled[0], 0, sizeof(sampled[0]));

    while (loadSample.available() && (i < (endOffsetBytes - startOffsetBytes))) {
      int b = loadSample.read();
      sampled[sampleID][i] = b;
      i++;
      if (i >= sizeof(sampled[sampleID])) break;
    }
    loadSample.close();

    i = i / 2;
    _samplers[sampleID].removeAllSamples();
    loadedSampleRate[sampleID] = SampleRate[sampleID];  // e.g. 44100, or whatever
    loadedSampleLen[sampleID] = i; 
    _samplers[sampleID].addSample(36, (int16_t *)sampled[sampleID], (int)i/1 , rateFactor);
  }
  yield();
}




void showWave() {
  int snr = SMP.wav[GLOB.currentChannel].fileID;

  if (snr < 1) snr = 1;
  int fnr = getFolderNumber(snr);
  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
  
  firstcheck = false;
  nofile = false;

  FastLEDclear();
  
  // Show big icon
  //showIcons(ICON_SAMPLE, UI_DIM_MAGENTA);
  
  // New indicator system: wave: M[CH] | M[W] | S[P] | L[X]
  drawIndicator('L', 'C', 1);  // Encoder 1: Medium Current Channel Color
  drawIndicator('L', 'W', 2);  // Encoder 2: Large White
  drawIndicator('L', 'Y', 3);  // Encoder 3: Large Yellow
  drawIndicator('L', 'X', 4);  // Encoder 4: Large Blue
  
  // Set encoder colors to match indicators
  // Encoder 0: Keep existing logic for file status (no corresponding indicator)
  
  
  
  // Encoder 1: Large Current Channel Color (L[C]) - matches indicator 1
  CRGB channelColor = getCurrentChannelColor();
  Encoder[0].writeRGBCode(channelColor.r << 16 | channelColor.g << 8 | channelColor.b);
  
  // Encoder 2: Large White (L[W]) - matches indicator 2
  Encoder[1].writeRGBCode(0xFFFFFF); // White
  
  // Encoder 3: Large Yellow (L[Y]) - matches indicator 3
  Encoder[2].writeRGBCode(0xFFFF00); // Yellow
  
  // Encoder 4: Large Blue (L[X]) - matches indicator 4
  Encoder[3].writeRGBCode(0x0000FF); // Blue
  // Display using current seek positions (as percentages)
  //displaySample(GLOB.seek, SMP.smplen, GLOB.seekEnd);
  processPeaks();
  drawNumber(snr, col_Folder[fnr], 12);

 // --- UPDATE START POSITION (Encoder 0) ---
if (sampleIsLoaded && currentMode->pos[0] != GLOB.seek) {
  GLOB.seek = currentMode->pos[0];
  // Use existing previewSample function - it will use cached data (fast!) and play the trimmed section
  previewSample(fnr, getFileNumber(snr), false, false);
}

  // --- FOLDER SELECTION (Encoder 1) ---
  if (currentMode->pos[1] != GLOB.folder) {
    envelope0.noteOff();
    playSdWav1.stop();
    firstcheck = true;
    nofile = false;
    GLOB.folder = currentMode->pos[1];
    //Serial.println("Folder: " + String(GLOB.folder - 1));
    SMP.wav[GLOB.currentChannel].fileID = ((GLOB.folder - 1) * 100) + 1;
    //Serial.print("WAV:");
    //Serial.println(SMP.wav[GLOB.currentChannel].fileID);
    Encoder[3].writeCounter((int32_t)SMP.wav[GLOB.currentChannel].fileID);
  }


// --- UPDATE END POSITION (Encoder 2) ---
if (sampleIsLoaded && currentMode->pos[2] != GLOB.seekEnd) {
  GLOB.seekEnd = currentMode->pos[2];
  // Use existing previewSample function - it will use cached data (fast!) and play the trimmed section
  previewSample(fnr, getFileNumber(snr), false, false);
}





  // --- NEW SAMPLE SELECTION (Encoder 3) ---
  // Ensure encoder3's value is within 1-999. If not, reset it.
  if (currentMode->pos[3] < 1 || currentMode->pos[3] > 999) {
    //currentMode->pos[3] = snr;              // snr is already clamped to at least 1
    //Encoder[3].writeCounter((int32_t)snr);  // Update the encoder display/counter
  }
  

  if (currentMode->pos[3] != snr) {
    envelope0.noteOff();
    previewIsPlaying = false;
    playSdWav1.stop();
    //currentMode->pos[3] = snr;
    sampleIsLoaded = false;
    firstcheck = true;
    nofile = false;

    snr = currentMode->pos[3];
    SMP.wav[GLOB.currentChannel].fileID = snr;
    
    //Serial.println(snr);

    fnr = getFolderNumber(snr);
    FastLEDclear();
    drawNumber(snr, col_Folder[fnr], 12);
    //Serial.println("File>> " + String(fnr) + " / " + String(getFileNumber(snr)));
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));

    // --- Invalidate preview cache when selecting new sample ---
    previewCache.valid = false;

    // --- Reset seek positions when choosing a new sample ---
    currentMode->pos[0] = 0;
    GLOB.seek = 0;
    Encoder[0].writeCounter((int32_t)0);
    currentMode->pos[2] = 100;
    GLOB.seekEnd = 100;
    Encoder[2].writeCounter((int32_t)100);

    if (!previewIsPlaying && !sampleIsLoaded) {
      previewIsPlaying = true;
      if (playSdWav1.isPlaying()) {
        playSdWav1.stop();
        playSdWav1.play(OUTPUTf);
      } else {
        playSdWav1.play(OUTPUTf);
      }

      peakIndex = 0;
      memset(peakValues, 0, sizeof(peakValues));
      sampleIsLoaded = true;
    }
  }

  // Just to be safe, ensure GLOB.seekEnd never exceeds 100
  if (GLOB.seekEnd > 100) {
    GLOB.seekEnd = 100;
    currentMode->pos[2] = GLOB.seekEnd;
    Encoder[2].writeCounter((int32_t)GLOB.seekEnd);
  }
}

// Copy currently loaded sample to samplepack 0
void copySampleToSamplepack0(unsigned int channel) {
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
    Serial.print("Failed to create samplepack 0 file: ");
    Serial.println(outputPath);
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
  outFile.write(reinterpret_cast<uint8_t*>(sampled[channel]), dataSize);
  
  outFile.close();
  
  // Mark this channel as using samplepack 0
  SMP.sp0Active[channel] = true;
  
  Serial.print("Saved sample to samplepack 0: ");
  Serial.print(outputPath);
  Serial.print(" (");
  Serial.print(dataSize);
  Serial.println(" bytes)");
}

// Reverse the preview sample (channel 0) in RAM - used in showWave
void reversePreviewSample() {
  extern CachedSample previewCache;
  
  if (!previewCache.valid || previewCache.lengthBytes == 0) {
    Serial.println("No preview sample loaded to reverse");
    return;
  }
  
  Serial.print("Reversing preview sample, length: ");
  Serial.print(previewCache.lengthBytes);
  Serial.println(" bytes");
  
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
  
  Serial.println("Preview sample reversed!");
  
  // Flip the peak visualization values array
  extern int peakIndex;
  extern float peakValues[];
  
  for (int i = 0; i < peakIndex / 2; i++) {
    float temp = peakValues[i];
    peakValues[i] = peakValues[peakIndex - 1 - i];
    peakValues[peakIndex - 1 - i] = temp;
  }
  
  Serial.println("Peak values flipped!");
  
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
  
  Serial.print("Seek positions swapped: seek=");
  Serial.print(GLOB.seek);
  Serial.print(", seekEnd=");
  Serial.println(GLOB.seekEnd);
  
  // Trigger re-preview with current seek settings
  extern int getFolderNumber(int fileID);
  extern int getFileNumber(int fileID);
  
  int snr = SMP.wav[GLOB.currentChannel].fileID;
  int fnr = getFolderNumber(snr);
  
  previewSample(fnr, getFileNumber(snr), false, false);
}

// Copy the preview sample (channel 0) to the target channel
void loadPreviewToChannel(unsigned int targetChannel) {
  if (targetChannel < 1 || targetChannel >= maxFiles) {
    Serial.println("Invalid target channel for preview load");
    return;
  }
  
  extern CachedSample previewCache;
  extern int getFolderNumber(int fileID);
  extern int getFileNumber(int fileID);
  
  bool needToLoadPreview = (!previewCache.valid || previewCache.lengthBytes == 0);
  
  // If preview cache is not valid, ensure the sample is loaded first
  if (needToLoadPreview) {
    Serial.println("Preview cache invalid - loading sample first");
    int snr = SMP.wav[GLOB.currentChannel].fileID;
    int fnr = getFolderNumber(snr);
    
    // Load the sample into cache without triggering preview playback
    // We'll manually load it into sampled[0] and set up the cache
    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
    
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      Serial.println("Failed to open sample file");
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
    
    // Load full sample into RAM buffer
    previewFile.seek(44);
    memset(sampled[0], 0, sizeof(sampled[0]));
    int plen = 0;
    while (previewFile.available() && plen < sizeof(sampled[0])) {
      sampled[0][plen++] = previewFile.read();
    }
    previewFile.close();
    
    // Update cache
    previewCache.folder = fnr;
    previewCache.sampleID = getFileNumber(snr);
    previewCache.lengthBytes = plen;
    previewCache.rate = rate;
    previewCache.valid = true;
    previewCache.plen = plen;
    
    Serial.println("Sample loaded to cache without preview");
  }
  
  Serial.print("Loading preview sample to channel ");
  Serial.println(targetChannel);
  
  // Calculate the trimmed portion based on seek/seekEnd
  int numSamples = previewCache.lengthBytes / 2;  // Total samples in preview
  int startSample = (numSamples * GLOB.seek) / 100;
  int endSample = (numSamples * GLOB.seekEnd) / 100;
  
  if (endSample <= startSample) {
    endSample = numSamples;
  }
  
  int trimmedSamples = endSample - startSample;
  
  Serial.print("Copying samples ");
  Serial.print(startSample);
  Serial.print(" to ");
  Serial.print(endSample);
  Serial.print(" (");
  Serial.print(trimmedSamples);
  Serial.println(" samples)");
  
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
  
  Serial.println("Preview loaded to channel!");
}
