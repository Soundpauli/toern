
void EEPROMgetLastFiles() {
  //get lastFile Array from Eeprom
  EEPROM.get(100, lastFile);
  set_Wav.maxValues[3] = lastFile[FOLDER_MAX - 1];
  //if lastFile from eeprom is empty, set it
  if (lastFile[0] == 0) {
    EEPROMsetLastFile();
  }
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
  return (x - 1) / maxX + 1;  // Calculate page number
}

void drawRandoms(){
  
  // Determine current page boundaries.
  unsigned int start = ((SMP.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = SMP.currentChannel;
  
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

void drawRandoms1(){
  // Determine the current page boundaries.
  unsigned int start = ((SMP.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = SMP.currentChannel;
  
  // --- Clear any existing notes on this page for the current channel ---
  for (unsigned int c = start; c < end; c++){
    for (unsigned int y = 1; y <= 16; y++){
      if (note[c][y].channel == channel) {
        note[c][y].channel = 0;
        note[c][y].velocity = defaultVelocity;
      }
    }
  }
  
  // Randomly choose between DUR (major) or MOLL (minor)
  bool isMajor = (random(0, 2) == 0);  // 50/50 chance
  
  // Allowed chord tone indices (1-based, corresponding to your pianoFrequencies array).
  // For DUR (major) we might use, for example, C, E, G and their octave neighbors.
  const int allowedMajor[] = {1, 3, 5, 8, 10, 12, 15};
  // For MOLL (minor) we might use, for example, A, C, and E (in various octaves).
  const int allowedMinor[] = {6, 8, 10, 13, 15};
  
  const int numMajor = sizeof(allowedMajor) / sizeof(allowedMajor[0]);
  const int numMinor = sizeof(allowedMinor) / sizeof(allowedMinor[0]);
  
  const int* allowedArray = isMajor ? allowedMajor : allowedMinor;
  int numAllowed = isMajor ? numMajor : numMinor;
  
  // Use a weighted random walk to decide which chord tone to use in each column.
  bool started = false;
  int k = 0;  // index into allowedArray
  
  // Process each column on the current page.
  for (unsigned int c = start; c < end; c++){
    int x_rel = c - start + 1;  // relative column number (1 .. maxX)
    
    // Set a higher chance to place a note on strong beats.
    int prob = 50; // default 50% chance to add a note
    if(x_rel == 1 || x_rel == 4 || x_rel == 8 || x_rel == 12) {
      prob = 80;
    }
    
    // Only add a note in this column if the probability check passes.
    if(random(0, 100) < prob) {
      // If this is the first note added on the page, pick a starting chord tone at random.
      if(!started){
        k = random(0, numAllowed);
        started = true;
      } else {
        // Weighted random walk:
        // 40% chance to step -1, 40% chance to stay, 10% chance to step +1,
        // and 10% chance to jump by ±2.
        int r = random(0, 100);
        int step = 0;
        if(r < 40) {
          step = -1;
        } else if(r < 80) {
          step = 0;
        } else if(r < 90) {
          step = 1;
        } else {
          step = (random(0, 2) == 0) ? -2 : 2;
        }
        k += step;
        if(k < 0) k = 0;
        if(k >= numAllowed) k = numAllowed - 1;
      }
      
      // Get the main note (row) from the allowed chord tones.
      int mainRow = allowedArray[k];
      // Only add the note if the slot is empty.
      if(note[c][mainRow].channel == 0){
        note[c][mainRow].channel = channel;
        note[c][mainRow].velocity = defaultVelocity;
      }
      
      // With 20% chance, add one extra note (a simple two-note chord) if the target slot is empty.
      if(random(0, 100) < 20) {
        int extraIndex = k;
        // Try an adjacent tone (prefer left half the time).
        if(k > 0 && random(0,2) == 0) {
          extraIndex = k - 1;
        } else if(k < numAllowed - 1) {
          extraIndex = k + 1;
        }
        int extraRow = allowedArray[extraIndex];
        if(extraRow != mainRow && note[c][extraRow].channel == 0){
          note[c][extraRow].channel = channel;
          note[c][extraRow].velocity = defaultVelocity;
        }
      }
      
      // With a 10% chance, add both adjacent notes (a fuller chord) if their slots are empty.
      if(random(0, 100) < 10) {
        if(k > 0) {
          int extraRow = allowedArray[k - 1];
          if(extraRow != mainRow && note[c][extraRow].channel == 0){
            note[c][extraRow].channel = channel;
            note[c][extraRow].velocity = defaultVelocity;
          }
        }
        if(k < numAllowed - 1) {
          int extraRow = allowedArray[k + 1];
          if(extraRow != mainRow && note[c][extraRow].channel == 0){
            note[c][extraRow].channel = channel;
            note[c][extraRow].velocity = defaultVelocity;
          }
        }
      }
    }
    // If the probability check fails, this column is left unchanged.
  }
  
  // Finally, update the display.
  drawBase();
  drawTriggers();
  FastLEDshow();
}


void drawRandoms2(){

  // Determine current page boundaries.
  unsigned int start = ((SMP.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = SMP.currentChannel;
  
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
    // First, determine the column's tonality based on existing notes (from any channel).
    // We'll mark as "major" if we see a note in an exclusively major row (1,3,5),
    // and "minor" if we see a note in an exclusively minor row (6,13).
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
    // For major (DUR), for example, we might allow C, E, G and their octave neighbors.
    const int allowedMajor[] = {1, 3, 5, 8, 10, 12, 15};
    // For minor (MOLL), for example, we might allow A, C, and E.
    const int allowedMinor[] = {6, 8, 10, 13, 15};
    
    const int numMajor = sizeof(allowedMajor) / sizeof(allowedMajor[0]);
    const int numMinor = sizeof(allowedMinor) / sizeof(allowedMinor[0]);
    
    const int* allowedArray = useMajor ? allowedMajor : allowedMinor;
    int numAllowed = useMajor ? numMajor : numMinor;
    
    // Determine the relative x position within the page.
    int x_rel = c - start + 1;
    
    // Set probability to add a note in this column.
    // For strong beats (e.g. x_rel 1,4,8,12) use 80%, otherwise 50%.
    int prob = 50;
    if(x_rel == 1 || x_rel == 4 || x_rel == 8 || x_rel == 12)
      prob = 80;
    
    // Decide whether to add a note in this column.
    if(random(0, 100) < prob){
      // --- Step 2: Choose a chord tone index.
      // Check if the column already has a note in one of the allowed positions.
      int baseIndex = -1;
      for(int i = 0; i < numAllowed; i++){
        int tone = allowedArray[i];
        if(note[c][tone].channel != 0){
          baseIndex = i;
          break;
        }
      }
      // If none found, choose randomly.
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
        if(baseIndex > 0 && random(0, 2) == 0)
          extraIndex = baseIndex - 1;
        else if(baseIndex < numAllowed - 1)
          extraIndex = baseIndex + 1;
        int extraRow = allowedArray[extraIndex];
        if(extraRow != mainRow && note[c][extraRow].channel == 0){
          note[c][extraRow].channel = channel;
          note[c][extraRow].velocity = defaultVelocity;
        }
      }
      // With 10% chance, add both adjacent tones.
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
    // If probability check fails, leave the column unchanged.
  } // End for each column
}


void clearPage() {
  //SMP.edit = 1;

  unsigned int start = ((SMP.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = SMP.currentChannel;
  bool singleMode = SMP.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  //updateLastPage();
}

void clearPageX(int thatpage) {
  //SMP.edit = 1;

  unsigned int start = ((thatpage - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = SMP.currentChannel;
  bool singleMode = SMP.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();
}



void changeMenu(int newMenuPosition) {
  menuPosition = newMenuPosition;
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




void _cycleFilterTypes(int button) {
  int numValues = 0;
  // If the button has no cycling values, do nothing
  //if (buttonValues[button][0] == MAX_TYPE || buttonValues[button][0] == 0) return;
  // Count valid entries in buttonValues[button] (ignoring MAX_TYPE)
  //while (numValues < 4 && buttonValues[button][numValues] != MAX_TYPE) {
  //  numValues++;
  //}
  // Find current position of adsr[button] in buttonValues[button]
  for (int i = 0; i < numValues; i++) {
    //if (adsr[button] == buttonValues[button][i])
    {
      // Move to the next value, wrapping around
      //adsr[button] = buttonValues[button][(i + 1) % numValues];
      Serial.print("Button ");
      Serial.print(button + 1);
      Serial.print(" toggled to: ");


      /*
      //TOOGLE BETWEEN RELEASE AND WF
      if (selectedFX == WAVEFORM) {
        Serial.println("---> WAVE");
        Serial.println(SMP.param_settings[SMP.currentChannel][WAVEFORM]);
        Encoder[0].writeMax((int32_t)5);
        Encoder[0].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel][WAVEFORM]);
      }

      if (adsr[3] == SUSTAIN) {
        Serial.println("---> SUSTAIN");
        Serial.println(SMP.param_settings[SMP.currentChannel][SUSTAIN]);
        Encoder[3].writeMax((int32_t)100);  // current maxvalues
        Encoder[3].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel][SUSTAIN]);
      }

        if (adsr[3] == RELEASE) {
        Serial.println("---> RELEASE");
        Serial.println(SMP.param_settings[SMP.currentChannel][RELEASE]);
        Encoder[3].writeMax((int32_t)100);  // current maxvalues
        Encoder[3].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel][RELEASE]);
      }

      */
      return;
    }
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
