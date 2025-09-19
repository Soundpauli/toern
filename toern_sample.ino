
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
  //Serial.print("loading Sample:");
  //Serial.print(sampleID);
  //Serial.print("from pack nr.");
  //Serial.println(packID);
    

  drawNoSD();

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", packID, sampleID);

  if (packID == 0) {
    sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(sampleID), sampleID);
    sampleID = GLOB.currentChannel;
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
  showIcons(ICON_SAMPLE, UI_DIM_MAGENTA);
  
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
  envelope0.noteOff();
  playSdWav1.stop();

  // force a fresh slice of the same file:
  previewCache.valid = false;

  //Serial.println("SEEK-hit");
  GLOB.seek = currentMode->pos[0];
  _samplers[0].removeAllSamples();
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
  playSdWav1.stop();

  // force reload so your new end‐point takes effect
  previewCache.valid = false;

  //Serial.println("SEEKEND-hit");
  GLOB.seekEnd = currentMode->pos[2];
  _samplers[0].removeAllSamples();
  envelope0.noteOff();
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
