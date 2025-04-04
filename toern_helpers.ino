
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
      generateParticles();
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
