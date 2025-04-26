void previewSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength, bool firstPreview) {
  if (playSdWav1.isPlaying()) playSdWav1.stop();
  envelope0.noteOff();
  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, sampleID);

  // --- Check if already cached ---
  if (!previewCache.valid || previewCache.folder != folder || previewCache.sampleID != sampleID) {
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      Serial.println("File not found!");
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

    SMP.smplen = fileSize / (PrevSampleRate * 2);

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
    SMP.smplen = previewCache.lengthBytes / (PrevSampleRate * 2);
  }

  // --- Calculate offsets from seek percentages ---
  int startOffset = (SMP.smplen * SMP.seek) / 100;
  int endOffset = (SMP.smplen * SMP.seekEnd) / 100;

  if (setMaxSampleLength) {
    endOffset = SMP.smplen;
    SMP.seekEnd = 100;
    currentMode->pos[2] = SMP.seekEnd;
    Encoder[2].writeCounter((int32_t)SMP.seekEnd);
  }

  int startOffsetBytes = startOffset * PrevSampleRate * 2;
  int endOffsetBytes = endOffset * PrevSampleRate * 2;

  // Clamp offsets to buffer size
  if (startOffsetBytes >= previewCache.lengthBytes) startOffsetBytes = 0;
  if (endOffsetBytes > previewCache.lengthBytes) endOffsetBytes = previewCache.lengthBytes;

  // Ensure alignment for 16-bit playback
  startOffsetBytes &= ~1;
  endOffsetBytes &= ~1;

  int plen = endOffsetBytes - startOffsetBytes;
  uint8_t* startPtr = &sampled[0][startOffsetBytes];

  // --- Playback from offset directly ---
  _samplers[0].removeAllSamples();
  _samplers[0].addSample(36, (int16_t*)startPtr, plen, rateFactor);
  sampleIsLoaded = true;

  Serial.println("NOTE");
  _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
}

void previewSample2(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength, bool firstPreview) {
  _samplers[0].removeAllSamples();
  envelope0.noteOff();
  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, sampleID);
  Serial.print("PLAYING:::");
  Serial.println(OUTPUTf);
  File previewSample = SD.open(OUTPUTf);

  if (previewSample) {
    int fileSize = previewSample.size();
    if (firstPreview) {
      fileSize = min(previewSample.size(), 302000);  // max 10SEC preview
    }

    // Read sample rate info from header:
    previewSample.seek(24);
    for (uint8_t i = 24; i < 25; i++) {
      int g = previewSample.read();
      if (g == 72) PrevSampleRate = 4;
      else if (g == 68) PrevSampleRate = 3;
      else if (g == 34) PrevSampleRate = 2;
      else if (g == 17) PrevSampleRate = 1;
      else if (g == 0) PrevSampleRate = 4;
    }

    // Calculate full sample length (in “time units”)
    SMP.smplen = fileSize / (PrevSampleRate * 2);

    // Use SMP.seek and SMP.seekEnd as percentages (0-100)
    // (Each percent corresponds to 1/100th of the full sample length.)
    int startOffset = (SMP.smplen * SMP.seek) / 100;
    int endOffset = (SMP.smplen * SMP.seekEnd) / 100;

    if (setMaxSampleLength == true) {
      // If you want to force full sample preview:
      endOffset = SMP.smplen;
      SMP.seekEnd = 100;  // 100%
      currentMode->pos[2] = SMP.seekEnd;
      Encoder[2].writeCounter((int32_t)SMP.seekEnd);
    }

    // Convert these “time units” into byte offsets (44-byte header)
    int startOffsetBytes = startOffset * PrevSampleRate * 2;
    int endOffsetBytes = endOffset * PrevSampleRate * 2;
    endOffsetBytes = min(endOffsetBytes, fileSize);

    previewSample.seek(44 + startOffsetBytes);
    memset(sampled[0], 0, sizeof(sampled[0]));
    int plen = 0;
    while (previewSample.available() && (plen < (endOffsetBytes - startOffsetBytes))) {
      int b = previewSample.read();
      sampled[0][plen] = b;
      plen++;
      if (plen >= sizeof(sampled[0])) break;
    }

    sampleIsLoaded = true;
    previewSample.close();

    // Pass the percentage values (or convert them to absolute positions in displaySample)
    //displaySample(SMP.seek, SMP.smplen, SMP.seekEnd);
    _samplers[0].addSample(36, (int16_t *)sampled[0], (int)plen, rateFactor);
    Serial.println("NOTE");
    _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
  }
}

void showWave() {

  int snr = SMP.wav[SMP.currentChannel].fileID;
  if (snr < 1) snr = 1;
  int fnr = getFolderNumber(snr);
  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
  /*
  if (firstcheck) {
    Serial.print("checking: ");
    firstcheck = false;
    if (!SD.exists(OUTPUTf)) {
      Serial.print(OUTPUTf);
      Serial.println(" >NOPE");
      nofile = true;
    } else {
      Serial.print(OUTPUTf);
      nofile = false;
      Serial.println(" >exists!");
    }
  }
  */
  firstcheck = false;
  nofile = false;

  FastLEDclear();
  //exit
  Encoder[3].writeRGBCode(0x0000FF);
  showIcons(HELPER_SELECT, CRGB(0, 0, 50));

  //showIcons("helper_ex", CRGB(10, 10, 0));

  showIcons(HELPER_FOLDER, CRGB(50, 50, 50));
  Encoder[1].writeRGBCode(0xFFFFFF);

  showIcons(HELPER_SEEK, CRGB(10, 0, 10));
  Encoder[2].writeRGBCode(0xFF00FF);

  if (nofile) {
    showIcons(HELPER_SEEKSTART, CRGB(0, 0, 10));
    showIcons(HELPER_EXIT, CRGB(0, 0, 0));
    Encoder[0].writeRGBCode(0x000000);
  } else {
    showIcons(HELPER_SEEKSTART, CRGB(0, 0, 10));
    showIcons(HELPER_EXIT, col[SMP.currentChannel]);
    Encoder[0].writeRGBCode(0x00FF00);
  }
  // Display using current seek positions (as percentages)
  //displaySample(SMP.seek, SMP.smplen, SMP.seekEnd);
  processPeaks();
  drawNumber(snr, col_Folder[fnr], 12);

  // --- UPDATE START POSITION (Encoder 0) ---
  if (sampleIsLoaded && (currentMode->pos[0] != SMP.seek)) {
    envelope0.noteOff();
    playSdWav1.stop();
    Serial.println("SEEK-hit");
    SMP.seek = currentMode->pos[0];  // should be between 0 and 100
    Serial.println(SMP.seek);
    _samplers[0].removeAllSamples();
    previewSample(fnr, getFileNumber(snr), false, false);
  }

  // --- FOLDER SELECTION (Encoder 1) ---
  if (currentMode->pos[1] != SMP.folder) {
    envelope0.noteOff();
    playSdWav1.stop();
    firstcheck = true;
    nofile = false;
    SMP.folder = currentMode->pos[1];
    Serial.println("Folder: " + String(SMP.folder - 1));
    SMP.wav[SMP.currentChannel].fileID = ((SMP.folder - 1) * 100) + 1;
    Serial.print("WAV:");
    Serial.println(SMP.wav[SMP.currentChannel].fileID);
    Encoder[3].writeCounter((int32_t)SMP.wav[SMP.currentChannel].fileID);
  }

  // --- UPDATE END POSITION (Encoder 2) ---
  if (sampleIsLoaded && (currentMode->pos[2] != SMP.seekEnd)) {
    previewIsPlaying = false;

    playSdWav1.stop();
    Serial.println("SEEKEND-hit");
    SMP.seekEnd = currentMode->pos[2];  // value 0-100
    Serial.println("seekEnd updated to: " + String(SMP.seekEnd));
    _samplers[0].removeAllSamples();
    envelope0.noteOff();

    previewSample(fnr, getFileNumber(snr), false, false);
  }

  // --- NEW SAMPLE SELECTION (Encoder 3) ---
  // Ensure encoder3's value is within 1-999. If not, reset it.
  if (currentMode->pos[3] < 1 || currentMode->pos[3] > 999) {
    currentMode->pos[3] = snr;              // snr is already clamped to at least 1
    Encoder[3].writeCounter((int32_t)snr);  // Update the encoder display/counter
  }

  if (currentMode->pos[3] != snr) {
    envelope0.noteOff();
    previewIsPlaying = false;
    playSdWav1.stop();

    sampleIsLoaded = false;
    firstcheck = true;
    nofile = false;
    SMP.wav[SMP.currentChannel].fileID = currentMode->pos[3];

    snr = SMP.wav[SMP.currentChannel].fileID;
    fnr = getFolderNumber(snr);
    FastLEDclear();
    drawNumber(snr, col_Folder[fnr], 12);
    Serial.println("File>> " + String(fnr) + " / " + String(getFileNumber(snr)));
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));

    // --- Reset seek positions when choosing a new sample ---
    currentMode->pos[0] = 0;
    SMP.seek = 0;
    Encoder[0].writeCounter((int32_t)0);
    currentMode->pos[2] = 100;
    SMP.seekEnd = 100;
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

  // Just to be safe, ensure SMP.seekEnd never exceeds 100
  if (SMP.seekEnd > 100) {
    SMP.seekEnd = 100;
    currentMode->pos[2] = SMP.seekEnd;
    Encoder[2].writeCounter((int32_t)SMP.seekEnd);
  }
}

void loadSample(unsigned int packID, unsigned int sampleID) {
  Serial.print("loading Sample:");
  Serial.print(sampleID);
  Serial.print("from pack nr.");
  Serial.println(packID);

  drawNoSD();

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", packID, sampleID);

  if (packID == 0) {
    sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(sampleID), sampleID);
    sampleID = SMP.currentChannel;
  }

  if (!SD.exists(OUTPUTf)) {
    Serial.print("File does not exist: ");
    Serial.println(OUTPUTf);
    SMP.mute[sampleID] = true;
    return;
  } else {
    SMP.mute[sampleID] = false;
  }

  usedFiles[sampleID - 1] = String(OUTPUTf);

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
    SMP.smplen = fileSize / (SampleRate[sampleID] * 2);

    // Convert percentage positions to actual offsets
    unsigned int startOffset = (SMP.seek * SMP.smplen) / 100;
    unsigned int startOffsetBytes = startOffset * SampleRate[sampleID] * 2;

    unsigned int endOffset = (SMP.seekEnd * SMP.smplen) / 100;
    if (SMP.seekEnd == 0) {
      endOffset = SMP.smplen;  // Full length if seekEnd is 0
      SMP.seekEnd = 100;       // Set to 100%
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
    _samplers[sampleID].addSample(36, (int16_t *)sampled[sampleID], (int)i/1 , rateFactor);
  }
}
