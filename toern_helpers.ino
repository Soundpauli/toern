
void EEPROMgetLastFiles() {
  //get lastFile Array from Eeprom
  EEPROM.get(100, lastFile);
  set_Wav.maxValues[3] = lastFile[FOLDER_MAX - 1];
  //if lastFile from eeprom is empty, set it
  if (lastFile[0] == 0) {
    EEPROMsetLastFile();
  }
}





// Wrapper: load all saved SMP settings (parameters, filters, drums, synths)
// for every channel.
void loadSMPSettings() {
  // Don't load settings if SMP_LOAD_SETTINGS is false
  if (!SMP_LOAD_SETTINGS) {
    return;
  }
  
  // Define the valid channels: 1,2,3,4,5,6,7,8,11,13,14
  const int validChannels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numValidChannels = sizeof(validChannels) / sizeof(validChannels[0]);
  
  for (int i = 0; i < numValidChannels; i++) {
    int ch = validChannels[i];
    
    // Load Filters - apply all filter settings for this channel
    for (int f = 0; f < NUM_FILTERS; f++) {
      setFilters((FilterType)f, ch, true);
    }
    
    // Load Parameters - apply all parameter settings for this channel
    for (int p = 0; p < 5; p++) {
    // Set the encoder value to the saved parameter value.S
    //currentMode->pos[3] = SMP.param_settings[ch][p];
    // Process the parameter mapping.
    setParams(p, ch);
  }
    
    // Load Drums - apply drum settings (only for channels 1-3)
    if (ch >= 1 && ch <= 3) {
        for (int d = 0; d < NUM_DRUMS; d++) {
           setDrums((DrumTypes)d, ch);
    }
    }
    
    // Load Synths - apply synth settings (only for channel 11)
   if (ch == 11) {
    updateSynthVoice(11);
  }
  }
  
  // Optionally update other settings such as channel volumes
  // or call updateFiltersAndParameters() if needed.
  //updateFiltersAndParameters();
}



void writeWavHeader(File &file, uint32_t sampleRate, uint8_t bitsPerSample, uint16_t numChannels) {
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint8_t blockAlign = numChannels * bitsPerSample / 8;

  // WAV header (44 bytes)
  uint8_t header[44] = {
    'R', 'I', 'F', 'F',
    0, 0, 0, 0,  // <- file size - 8 (filled in later)
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  // PCM chunk size
    1, 0,         // Audio format (1 = PCM)
    (uint8_t)(numChannels & 0xff), (uint8_t)(numChannels >> 8),
    (uint8_t)(sampleRate & 0xff), (uint8_t)((sampleRate >> 8) & 0xff),
    (uint8_t)((sampleRate >> 16) & 0xff), (uint8_t)((sampleRate >> 24) & 0xff),
    (uint8_t)(byteRate & 0xff), (uint8_t)((byteRate >> 8) & 0xff),
    (uint8_t)((byteRate >> 16) & 0xff), (uint8_t)((byteRate >> 24) & 0xff),
    blockAlign, 0,
    bitsPerSample, 0,
    'd', 'a', 't', 'a',
    0, 0, 0, 0  // <- data chunk size (filled in later)
  };

  file.write(header, 44);
}


void startRecordingRAM() {
  if (isRecording) return;

  if (playSdWav1.isPlaying()) playSdWav1.stop();
  Encoder[0].writeRGBCode(0x000000);
  Encoder[1].writeRGBCode(0xFF0000);
  Encoder[2].writeRGBCode(0x000000);
  Encoder[3].writeRGBCode(0x000000);
  recTime = 0;
  recWriteIndex = 0;
  queue1.begin();  // start filling 128‑sample blocks
  showIcons(ICON_REC, UI_DIM_RED);
  FastLED.show();
  isRecording = true;
}

void flushAudioQueueToRAM2() {
  if (!isRecording) return;
  // each AudioRecordQueue block is 128 samples
  while (queue1.available() && recWriteIndex + 128 <= BUFFER_SAMPLES) {
    auto block = (int16_t *)queue1.readBuffer();
    memcpy(recBuffer + recWriteIndex, block, 128 * sizeof(int16_t));
    recWriteIndex += 128;
    queue1.freeBuffer();
  }
  // if we ever hit the end of the buffer, stop automatically:
  if (recWriteIndex >= BUFFER_SAMPLES) {
    //Serial.println("⚠️ Buffer full");
    queue1.end();
    isRecording = false;
  }
}

void stopRecordingRAM(int fnr, int snr) {
  if (!isRecording) return;
  flushAudioQueueToRAM();
  queue1.end();
  isRecording = false;

  // open WAV on SD
  char path[64];
  sprintf(path, "samples/%d/_%d.wav", fnr, snr);
  if (SD.exists(path)) SD.remove(path);
  File f = SD.open(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!f) { return; }
  // write 44‑byte WAV header for 16‑bit/mono/22 050 Hz
  writeWavHeader(f, AUDIO_SAMPLE_RATE_EXACT, 16, 1);
  // write all your samples in one chunk
  f.write((uint8_t *)recBuffer, recWriteIndex * sizeof(int16_t));

  f.close();
  //Serial.print("💾 Saved ");
  //Serial.println(path);
  // playback if you like
  playSdWav1.play(path);
}



// --------------------
void startFastRecord() {
  if (fastRecordActive) return;

  // Handle different recording modes based on recChannelClear
  if (recChannelClear == 1) {
    // ON mode: Clear all existing notes of channel, then add triggers/notes
    clearAllNotesOfChannel();
  }
  // OFF mode (recChannelClear == 0): Add triggers/notes as soon as recording starts
  // FIX mode (recChannelClear == 2): Don't manipulate any notes - just record

   paintMode = false;
            freshPaint = true;
            unpaintMode = false;
            pressed[3] = false;
            
            // Reset paint/unpaint prevention flag when starting fast record
            extern bool preventPaintUnpaint;
            preventPaintUnpaint = false;
            
  // Only add triggers/notes if not in FIX mode
  if (recChannelClear != 2) {
    note[beat][GLOB.currentChannel+1].channel = GLOB.currentChannel;
    note[beat][GLOB.currentChannel+1].velocity = defaultVelocity;
  }
            
  // 1) Stop & clear any queued audio so old data never sneaks in
  queue1.end();
  while (queue1.available()) {
    queue1.readBuffer();
    queue1.freeBuffer();
  }

  // 2) Reset our write index and drop counter
  int ch = GLOB.currentChannel;
  fastRecWriteIndex[ch] = 0;
  fastDropRemaining = FAST_DROP_BLOCKS;

  // 3) Restart recording queue
  queue1.begin();
  fastRecordActive = true;
  
  // Enable audio input monitoring only if monitorLevel > 0
  if (monitorLevel > 0) {
    float monitorGain = 0.0;
    switch (monitorLevel) {
      case 1: monitorGain = 0.1; break;    // Low
      case 2: monitorGain = 0.3; break;    // Medium
      case 3: monitorGain = 0.5; break;    // High
      case 4: monitorGain = 0.8; break;    // Full
      default: monitorGain = 0.0; break;   // Default to OFF
    }
    mixer_end.gain(3, monitorGain);
  } else {
    // Ensure monitoring is OFF if monitorLevel is 0
    mixer_end.gain(3, 0.0);
  }

  //Serial.printf("FAST RECORD ▶ ch%u (dropping %d blocks)\n", ch, FAST_DROP_BLOCKS);
}

// --------------------
void flushAudioQueueToRAM() {
  if (fastRecordActive) {
    int ch = GLOB.currentChannel;
    auto &idx = fastRecWriteIndex[ch];

    // Treat your byte buffer as an int16_t array for cleaner pointer math:
    int16_t *dest = reinterpret_cast<int16_t *>(sampled[ch]);

    // Pull every pending block
    while (queue1.available()) {
      int16_t *block = (int16_t *)queue1.readBuffer();

      if (fastDropRemaining > 0) {
        // still skipping the first 200ms
        fastDropRemaining--;
      } else if (idx + AUDIO_BLOCK_SAMPLES <= BUFFER_SAMPLES) {
        // copy 128 samples (256 bytes) into our buffer
        memcpy(dest + idx, block, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
        idx += AUDIO_BLOCK_SAMPLES;
      }

      queue1.freeBuffer();

      // auto-stop if full
      if (idx >= BUFFER_SAMPLES) {
        stopFastRecord();
        break;
      }
    }
    return;  // skip your SD path
  }
}

void stopFastRecord() {
  fastRecordActive = false;
  queue1.end();
  
  // Disable audio input monitoring
  mixer_end.gain(3, 0.0);
  
  auto  ch  = GLOB.currentChannel;
  auto& idx = fastRecWriteIndex[ch];
  loadedSampleLen[ch] = idx;
  // load into that channel’s sampler voice:
  _samplers[ch].removeAllSamples();
  _samplers[ch].addSample(
    36,                            // MIDI note #
    (int16_t*)sampled[ch],         // reinterpret bytes → int16_t
    idx,                           // sample-count
    rateFactor
  );
  // give back your knob color + preview
 // Encoder[0].writeRGBCode(CRGBToUint32(col[ch]));
 // _samplers[ch].noteEvent(36, defaultVelocity, true, false);
  //Serial.printf("◀ FASTREC ch%u, %u samples\n", ch, (unsigned)idx);
 
}

void EEPROMsetLastFile() {
  //set maxFiles in folder and show loading...
  for (int f = 0; f <= FOLDER_MAX; f++) {
    //FastLEDclear();

    for (unsigned int i = 1; i < 99; i++) {
      char OUTPUTf[50];
      sprintf(OUTPUTf, "samples/%d/_%d.wav", f, i + (f * 100));
      if (SD.exists(OUTPUTf)) {
        lastFile[f] = i + (f * 100);
      }
    }
    //set last File on Encoder
    drawLoadingBar(1, 999, lastFile[f], col_base[f], CRGB(15, 15, 55), false);
  }
  
  //set lastFile Array into Eeprom
  EEPROM.put(100, lastFile);
}



void clearAllNotesOfChannel() {
  uint8_t channel = GLOB.currentChannel;

  for (uint16_t step = 0; step < maxlen; step++) {
    for (uint8_t pitch = 1; pitch <= 16; pitch++) {
      if (note[step][pitch].channel == channel) {
        note[step][pitch].channel = 0;
        note[step][pitch].velocity = defaultVelocity;
      }
    }
  }

  updateLastPage();  // Optional: If your UI tracks last updated page
  FastLEDshow();     // Optional: Refresh LED grid if used
}

void FastLEDclear() {
  FastLED.clear();
}

void FastLEDshow() {
  if (millis() - lastUpdate > RefreshTime) {
    lastUpdate = millis();
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
  }
}



int getPage(int x) {
  //updateLastPage();
  return (x - 1) / maxX + 1;  // Calculate page number
}

void drawRandoms(){
  
  // Determine current page boundaries.
  unsigned int start = ((GLOB.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  
  // --- Step 1: Clear notes on the current page for the current channel ---
  for(unsigned int c = start; c < end; c++){
    for(unsigned int r = 1; r <= 16; r++){
      if(note[c][r].channel == channel){
        note[c][r].channel = 0;
        note[c][r].velocity = defaultVelocity;
      }
    }
  }
  
  // --- Process each column individually ---
  for(unsigned int c = start; c < end; c++){
    // Determine the column's tonality based on existing notes (from any channel).
    bool foundMajor = false, foundMinor = false;
    for(unsigned int r = 1; r <= 16; r++){
      if(note[c][r].channel != 0){
        if(r == 1 || r == 3 || r == 5)
          foundMajor = true;
        if(r == 6 || r == 13)
          foundMinor = true;
      }
    }
    // Decide the allowed set:
    bool useMajor;
    if(foundMajor && !foundMinor) {
      useMajor = true;
    } else if(foundMinor && !foundMajor) {
      useMajor = false;
    } else {
      // If ambiguous or empty, choose randomly.
      useMajor = (random(0,2) == 0);
    }
    
    // Define allowed chord tones (1-based indices into pianoFrequencies).
    const int allowedMajor[] = {1, 3, 5, 8, 10, 12, 15};
    const int allowedMinor[] = {6, 8, 10, 13, 15};
    
    const int numMajor = sizeof(allowedMajor) / sizeof(allowedMajor[0]);
    const int numMinor = sizeof(allowedMinor) / sizeof(allowedMinor[0]);
    
    const int* allowedArray = useMajor ? allowedMajor : allowedMinor;
    int numAllowed = useMajor ? numMajor : numMinor;
    
    // Determine the relative x position within the page.
    int x_rel = c - start + 1;
    
    // Set the probability to add a note:
    // Strong beats (columns 1, 4, 8, 12) always get a note.
    // Otherwise, use an 80% chance.
    int prob = (x_rel == 1 || x_rel == 4 || x_rel == 8 || x_rel == 12) ? 100 : 80;
    
    // Decide whether to add a note in this column.
    if(random(0, 100) < prob){
      // --- Step 2: Choose a chord tone index.
      int baseIndex = -1;
      for(int i = 0; i < numAllowed; i++){
        int tone = allowedArray[i];
        if(note[c][tone].channel != 0){
          baseIndex = i;
          break;
        }
      }
      if(baseIndex < 0) {
        baseIndex = random(0, numAllowed);
      }
      
      // --- Step 3: Add the main note.
      int mainRow = allowedArray[baseIndex];
      if(note[c][mainRow].channel == 0){
        note[c][mainRow].channel = channel;
        note[c][mainRow].velocity = defaultVelocity;
      }
      
      // --- Step 4: Optionally add extra chord tones (only in empty slots)
      // With 20% chance, add one adjacent note.
      if(random(0, 100) < 20){
        int extraIndex = baseIndex;
        if(baseIndex > 0 && random(0,2) == 0)
          extraIndex = baseIndex - 1;
        else if(baseIndex < numAllowed - 1)
          extraIndex = baseIndex + 1;
        int extraRow = allowedArray[extraIndex];
        if(extraRow != mainRow && note[c][extraRow].channel == 0){
          note[c][extraRow].channel = channel;
          note[c][extraRow].velocity = defaultVelocity;
        }
      }
      // With a 10% chance, add both adjacent tones.
      if(random(0, 100) < 10){
        if(baseIndex > 0){
          int extraRow = allowedArray[baseIndex - 1];
          if(extraRow != mainRow && note[c][extraRow].channel == 0){
            note[c][extraRow].channel = channel;
            note[c][extraRow].velocity = defaultVelocity;
          }
        }
        if(baseIndex < numAllowed - 1){
          int extraRow = allowedArray[baseIndex + 1];
          if(extraRow != mainRow && note[c][extraRow].channel == 0){
            note[c][extraRow].channel = channel;
            note[c][extraRow].velocity = defaultVelocity;
          }
        }
      }
    }
    // If probability check fails, the column remains unchanged.
  } // End for each column
}






void clearPage() {
  //GLOB.edit = 1;

  unsigned int start = ((GLOB.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  bool singleMode = GLOB.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();
  
  // Reset paint/unpaint prevention flag after clearPage operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}

void clearPageX(int thatpage) {
  //GLOB.edit = 1;

  unsigned int start = ((thatpage - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  bool singleMode = GLOB.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();
}



void changeMenu(int newMenuPosition) {
  // This function is now used to set the main setting for the current page
  // The actual page navigation is handled in showMenu()
}


int getFolderNumber(int value) {
  int folder = floor(value / 100);
  if (folder > FOLDER_MAX) folder = FOLDER_MAX;
  if (folder <= 0) folder = 0;
  return folder;
}

int getFileNumber(int value) {
  // return the file number in the folder, so 113 = 13, 423 = 23
  int folder = getFolderNumber(value);
  int wavfile = value % 100;
  if (wavfile <= 0) wavfile = 0;
  return wavfile + folder * 100;
}

CRGB getCol(unsigned int g) {
  return col[g] * 10;
}


void setNote(uint16_t step,  uint8_t pitch, uint8_t channel, uint8_t velocity) {
  if (step < maxlen && channel < NUM_CHANNELS) {
    note[step][pitch].channel = channel;
    note[step][pitch].velocity = velocity;
  }
}

// Function to get a note from a given step and channel
Note getNote(uint16_t step, uint8_t channel) {
  if (step < maxlen && channel < NUM_CHANNELS) {
    return note[step][channel];
  }
  // Return a default Note if indices are out-of-range.
  return {0, 0};
}


/***************/
/**** INTRO ****/
/***************/


// ----- Generate Explosion Particles -----
// Called once when Phase 2 starts. Each "1" pixel in the logo becomes a particle.
void generateParticles() {
  particleCount = 0;
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 16; col++) {
      if (logo[row][col] == 1) {
        // Particle's initial position (center of that cell)
        float initX = col + 0.5;
        float initY = row + 0.5;
        // Explosion direction from the center
        float dx = initX - logoCenterX;
        float dy = initY - logoCenterY;
        float length = sqrt(dx * dx + dy * dy);
        float dirX = (length == 0) ? 0 : (dx / length);
        float dirY = (length == 0) ? 0 : (dy / length);
        // Freeze the pixel’s color from timeFactor=1
        CRGB color = getLogoPixelColor(col, row, 1.0);
        particles[particleCount] = { initX, initY, dirX, dirY, color };
        particleCount++;
      }
    }
  }
  particlesGenerated = true;
}



// ----- Determine Color of Each LED Based on Time -----
CRGB getPixelColor(uint8_t x, uint8_t y, unsigned long elapsed) {
  // Convert (x,y) to float center coords
  float cellCenterX = x + 0.5;
  float cellCenterY = y + 0.5;

  if (elapsed < phase1Duration) {
    // PHASE 1: Rainbow Logo
    if (logo[y][x] == 1) {
      float timeFactor = (float)elapsed / phase1Duration;  // 0..1
      return getLogoPixelColor(x, y, timeFactor);
    } else {
      return CRGB::Black;
    }
  } else if (elapsed < totalAnimationTime) {
    
    // PHASE 2: Explosion
    if (!particlesGenerated) {
      //done once
      //playSdWav1.play("intro/008.wav");
      generateParticles();
      initEncoders();
      
    }
    float progress = (float)(elapsed - phase1Duration) / phase2Duration;  // 0..1
    const float maxDisp = 20.0;                                           // how far particles fly outward

    // Check each particle to see if it's near this LED cell
    for (int i = 0; i < particleCount; i++) {
      float px = particles[i].initX + particles[i].dirX * progress * maxDisp;
      float py = particles[i].initY + particles[i].dirY * progress * maxDisp;
      float dist = sqrt((cellCenterX - px) * (cellCenterX - px) + (cellCenterY - py) * (cellCenterY - py));
      if (dist < 0.4) {
        return particles[i].color;
      }
    }
    return CRGB::Black;
  } else {
    // After animation, stay black
    return CRGB::Black;
  }
}








// ----- Run the Animation Once (Called in setup) -----
void runAnimation() {
  unsigned long startTime = millis();
  while (true) {
    unsigned long elapsed = millis() - startTime;
    if (elapsed > totalAnimationTime) {
      elapsed = totalAnimationTime;
    }

    // Update each LED in the 16×16 grid (0-based coords)
    for (uint8_t y = 0; y < maxY; y++) {
      for (uint8_t x = 0; x < maxX; x++) {
        CRGB color = getPixelColor(x, y, elapsed);
        // The light() function uses 1-based coords:
        light(x + 1, y + 1, color);
      }
    }
    FastLED.show();
    delay(10);

    // Stop after totalAnimationTime
    if (elapsed >= totalAnimationTime) {
      break;
    }
  }

  // Clear the display at end
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}
