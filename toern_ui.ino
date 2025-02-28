

void light(unsigned int x, unsigned int y, CRGB color) {
  unsigned int index = 0;
  if (y >= 1 && y <= maxY && x >= 1 && x <= maxX) {
    if (y > maxY) y = 1;
    if (y % 2 == 0) {
      index = (maxX - x) + (maxX * (y - 1));
    } else {
      index = (x - 1) + (maxX * (y - 1));
    }
    if (index < NUM_LEDS) { leds[index] = color; }
  }
  yield();
}




void drawPlayButton() {
  unsigned int timer = (beat - 2) % maxX + 1;
  if (currentMode == &draw || currentMode == &singleMode) {
    if (isNowPlaying) {
      if (timer == 1 || timer == 5 || timer == 9 || timer == 13) {
        Encoder[2].writeRGBCode(0xFF0000);
      } else {
        Encoder[2].writeRGBCode(0x550000);
      }
    }
  }
}

void drawNoSD() {
  bool noSDfound = false;

  while (!SD.begin(INT_SD)) {
    FastLEDclear();
    for (unsigned int gx = 0; gx < 48; gx++) {
      light(noSD[gx][0], maxY - noSD[gx][1], CRGB(50, 0, 0));
    }
    FastLEDshow();
    delay(1000);
    noSDfound = true;
  }

  if (noSDfound && SD.begin(INT_SD)) {
    FastLEDclear();
    EEPROMsetLastFile();
    noSDfound = false;
  }
}



void drawBase() {
  if (!SMP.singleMode) {
    unsigned int colors = 0;
    for (unsigned int y = 1; y < maxY; y++) {
      //unsigned int filtering = 2;  // mapf(SMP.param_settings[SMP.currentChannel][y - 1], 0, maxfilterResolution, 50, 5);
      for (unsigned int x = 1; x < maxX + 1; x++) {
        if (SMP.mute[y - 1]) {
          light(x, y, CRGB(0, 0, 0));
        } else {
          light(x, y, col_base[colors]);
        }
      }
      colors++;
    }

    drawStatus();

    for (unsigned int x = 1; x <= 13; x += 4) {
      light(x, 1, CRGB(10, 10, 10));  //4-4 helper
    }

  } else {
    unsigned int currentChannel = SMP.currentChannel;
    bool isMuted = SMP.mute[currentChannel];

    for (unsigned int y = 1; y < maxY; y++) {
      for (unsigned int x = 1; x < maxX + 1; x++) {
        light(x, y, col_base[currentChannel]);
      }
    }
    for (unsigned int x = 1; x <= 13; x += 4) {
      light(x, 1, CRGB(0, 1, 1));  // türkis
    }
  }

  drawPages();
}

void drawStatus() {
  CRGB ledColor = CRGB(0, 0, 0);
  if (SMP.activeCopy)
    ledColor = CRGB(20, 20, 0);
  for (unsigned int s = 1; s <= maxX; s++) {
    light(s, 1, ledColor);
  }

  if (currentMode == &noteShift) {
    // draw a moving marquee to indicate the note shift mode
    for (unsigned int x = 1; x <= maxX; x++) {
      light(x, 1, CRGB(0, 0, 0));
    }
    light(round(marqueePos), 1, CRGB(120, 120, 120));
    if (movingForward) {
      marqueePos = marqueePos + 1;
      if (marqueePos > maxX) {
        marqueePos = maxX;
        movingForward = false;
      }
    } else {
      marqueePos = marqueePos - 1;
      if (marqueePos < 1) {
        marqueePos = 1;
        movingForward = true;
      }
    }
  }
}


void drawText(const char *text, int startX, int startY, CRGB color) {
  int xOffset = startX;

  for (int i = 0; text[i] != '\0'; i++) {
    drawChar(text[i], xOffset, startY, color);
    xOffset += alphabet[text[i] - 32][0] + 1;  // Advance by char width + 1 space
  }
}


void drawChar(char c, int x, int y, CRGB color) {
  if (c < 32 || c > 126) return;  // Skip unsupported characters

  uint8_t index = c - 32;              // Calculate index in alphabet array
  uint8_t width = alphabet[index][0];  // Get the width of the character

  for (int col = 0; col < width; col++) {
    uint8_t columnData = alphabet[index][col + 1];  // Get column data

    // Reverse the bits to flip the character vertically
    uint8_t flippedColumn = 0;
    for (int row = 0; row < 5; row++) {
      if (columnData & (1 << row)) {
        flippedColumn |= (1 << (4 - row));  // Reverse the row positions
      }
    }

    for (int row = 0; row < 5; row++) {
      if (flippedColumn & (1 << row)) {  // Check if the bit is set
        light(x + col, y + row, color);  // Light up the corresponding LED
      }
    }
  }
}




void drawNumber(unsigned int count, CRGB color, int topY) {
  char buffer[12];
  unsigned int width = 0;
  if (count > 9) width = 4;
  if ((count > 99)) width = 8;
  sprintf(buffer, "%u", count);
  drawText(buffer, 14 - width, topY, color);
  FastLEDshow();
}


void drawVelocity() {
  //FastLEDclear();

  unsigned int vy = currentMode->pos[0];

  //GLOBAL
  unsigned int cv = currentMode->pos[2];
  FastLEDclear();


  for (unsigned int x = 1; x < 6; x++) {
    for (unsigned int y = 1; y < vy + 1; y++) {
      light(x, y, CRGB(y * y, 20 - y, 0));
    }
  }

  for (unsigned int x = 9; x < 13 + 1; x++) {
    for (unsigned int y = 1; y < cv + 1; y++) {
      light(x, y, CRGB(0, 20 - y, y * y));
    }
  }
  drawText("v", 2, 2, CRGB(50, 50, 50));
  drawText("c", 10, 2, CRGB(50, 50, 50));
}



void colorBelowCurve(int xStart, int xEnd, int yStart, int yEnd, CRGB color) {
  for (int x = xStart; x <= xEnd; x++) {
    int yCurve = mapf(x, xStart, xEnd, yStart, yEnd);  // Compute envelope's y-value at this x
    for (int y = 1; y <= yCurve; y++) {
      light(x, y, color);  // Fill from y=1 up to the envelope
    }
  }
}


void drawADSR(char *txt, int activeParameter) {
  FastLED.clear();


  //showIcons("helper_exit", CRGB(0, 0, 20));

  // Amplitude definitions:
  int baseAmplitude = 1;        // baseline (minimum amplitude)
  int attackHeight = maxY - 6;  // fixed attack (and hold) height
  // Sustain's height is variable, controlled by the SUSTAIN parameter:
  int sustainHeight = mapf(SMP.param_settings[SMP.currentChannel][SUSTAIN], 0, 32, baseAmplitude, attackHeight);
  // Map widths (x durations) for each stage:
  int delayWidth = mapf(SMP.param_settings[SMP.currentChannel][DELAY], 0, 32, 0, 3);      // Delay: min 0, max 3
  int attackWidth = mapf(SMP.param_settings[SMP.currentChannel][ATTACK], 0, 32, 0, 4);    // Attack: duration only (0 to 4)
  int holdWidth = mapf(SMP.param_settings[SMP.currentChannel][HOLD], 0, 32, 0, 3);        // Hold: width (0 to 3)
  int decayWidth = mapf(SMP.param_settings[SMP.currentChannel][DECAY], 0, 32, 0, 4);      // Decay: now 0 to 4
  const int sustainFixedWidth = 2;                                                        // Sustain: fixed width of 2
  int releaseWidth = mapf(SMP.param_settings[SMP.currentChannel][RELEASE], 0, 32, 0, 4);  // Release: 0 to 4

  // Compute x-positions for each stage:
  int xDelayStart = 1;
  int xDelayEnd = xDelayStart + delayWidth;
  int xAttackStart = xDelayEnd;
  int xAttackEnd = xAttackStart + attackWidth;
  int xHoldEnd = xAttackEnd + holdWidth;
  int xDecayEnd = xHoldEnd + decayWidth;
  int xSustainEnd = xDecayEnd + sustainFixedWidth;
  int xReleaseEnd = xSustainEnd + releaseWidth;



  CRGB activeParameterColor;

  switch (activeParameter) {
    case 2:
      activeParameterColor = CRGB(100, 0, 0);
      break;
    case 3:
      activeParameterColor = CRGB(0, 100, 0);
      break;
    case 4:
      activeParameterColor = CRGB(0, 0, 100);
      break;
    case 5:
      activeParameterColor = CRGB(100, 100, 0);
      break;
    case 6:
      activeParameterColor = CRGB(100, 0, 100);
      break;
    case 7:
      activeParameterColor = CRGB(0, 100, 100);
      break;
    default:
      activeParameterColor = CRGB(0, 0, 0);  // default color (adjust as needed)
      break;
  }

  // --- Draw colored envelopes first ---
  // Delay stage: flat line at baseAmplitude.
  colorBelowCurve(xDelayStart, xDelayEnd, baseAmplitude, baseAmplitude,
                  activeParameter == 2 ? CRGB(100, 0, 0) : CRGB(4, 0, 0));
  // Attack stage: rising line from baseAmplitude to fixed attackHeight.
  colorBelowCurve(xAttackStart, xAttackEnd, baseAmplitude, attackHeight,
                  activeParameter == 3 ? CRGB(0, 100, 0) : CRGB(0, 4, 0));
  // Hold stage: flat line at attackHeight.
  colorBelowCurve(xAttackEnd, xHoldEnd, attackHeight, attackHeight,
                  activeParameter == 4 ? CRGB(0, 0, 100) : CRGB(0, 0, 4));
  // Decay stage: falling line from attackHeight down to sustainHeight.
  colorBelowCurve(xHoldEnd, xDecayEnd, attackHeight, sustainHeight,
                  activeParameter == 5 ? CRGB(100, 100, 0) : CRGB(4, 4, 0));
  // Sustain stage: flat line at sustainHeight.
  colorBelowCurve(xDecayEnd, xSustainEnd, sustainHeight, sustainHeight,
                  activeParameter == 6 ? CRGB(100, 0, 100) : CRGB(4, 0, 4));
  // Release stage: falling line from sustainHeight down to baseAmplitude.
  colorBelowCurve(xSustainEnd, xReleaseEnd, sustainHeight, baseAmplitude,
                  activeParameter == 7 ? CRGB(0, 100, 100) : CRGB(0, 4, 4));

  // --- Now overlay the white envelope outline ---
  drawLine(xDelayStart, baseAmplitude, xDelayEnd, baseAmplitude, CRGB::Red);
  drawLine(xAttackStart, baseAmplitude, xAttackEnd, attackHeight, CRGB(30, 255, 104));
  drawLine(xAttackEnd, attackHeight, xHoldEnd, attackHeight, CRGB::White);
  drawLine(xHoldEnd, attackHeight, xDecayEnd, sustainHeight, CRGB::White);
  drawLine(xDecayEnd, sustainHeight, xSustainEnd, sustainHeight, CRGB::White);
  drawLine(xSustainEnd, sustainHeight, xReleaseEnd, baseAmplitude, CRGB::White);

  drawText(txt, 1, 12, activeParameterColor);
}

void drawLine(int x1, int y1, int x2, int y2, CRGB color) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    light(x1, y1, color);
    if (x1 == x2 && y1 == y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}


void drawPages() {
  //SMP.edit = 1;
  CRGB ledColor;

  for (unsigned int p = 1; p <= maxPages; p++) {  // Assuming maxPages is 8

    // If the page is the current one, set the LED to white
    if (SMP.page == p && SMP.edit == p) {
      ledColor = isNowPlaying ? CRGB(20, 255, 20) : CRGB(50, 50, 50);
    } else if (SMP.page == p) {
      ledColor = isNowPlaying ? CRGB(0, 15, 0) : CRGB(0, 0, 35);
    } else {
      if (SMP.edit == p) {
        ledColor = SMP.page == p ? CRGB(50, 50, 50) : CRGB(20, 20, 20);
      } else {
        ledColor = hasNotes[p] ? CRGB(0, 0, 35) : CRGB(1, 0, 0);
      }
    }

    // Set the LED color
    light(p, maxY, ledColor);
  }

  // After the loop, update the maxValues for Page-select-knob
  //currentMode->maxValues[1] = lastPage + 1;

  // Additional logic can be added here if needed
}

/************************************************
      DRAW SAMPLES
  *************************************************/
void drawTriggers() {
  // why?
  //SMP.edit = 1;
  for (unsigned int ix = 1; ix < maxX + 1; ix++) {
    for (unsigned int iy = 1; iy < maxY + 1; iy++) {
      int thisNote = note[((SMP.edit - 1) * maxX) + ix][iy][0];
      if (thisNote > 0) {

        if (!SMP.mute[thisNote]) {
          //light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy][0]));
          light(ix, iy, col[thisNote]);

          // if there is any note of the same value in the same column, make it less bright
          for (unsigned int iy2 = 1; iy2 < maxY + 1; iy2++) {
            if (iy2 != iy && note[((SMP.edit - 1) * maxX) + ix][iy2][0] == note[((SMP.edit - 1) * maxX) + ix][iy][0]) {
              light(ix, iy2, col[thisNote]);
            }
          }
        } else {
          //light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy][0]) / 24);
          light(ix, iy, col_base[thisNote]);
        }
      }
    }
  }
}


/************************************************
      TIMER
  *************************************************/

void drawTimer() {
  unsigned int timer = (beat - 2) % maxX + 1;
  if (SMP.page == SMP.edit) {
    if (timer < 1) timer = 1;
    for (unsigned int y = 1; y < maxY; y++) {
      light(timer, y, CRGB(10, 0, 0));

      if (note[((SMP.page - 1) * maxX) + timer][y][0] > 0) {
        if (SMP.mute[note[((SMP.page - 1) * maxX) + timer][y][0]] == 0) {
          light(timer, y, CRGB(200, 200, 200));
        } else {
          light(timer, y, CRGB(00, 00, 00));
        }
      }
    }
  }
}


int mapXtoPageOffset(int x) {
  return x - (SMP.edit - 1) * maxX;
}

/************************************************
      USER CURSOR
  *************************************************/
void drawCursor() {
  if (dir == 1)
    pulse = pulse + 1;
  if (dir == -1)
    pulse = pulse - 1;
  if (pulse > 220) {
    dir = -1;
  }
  if (pulse < 1) {
    dir = 1;
  }
  light(mapXtoPageOffset(SMP.x), SMP.y, CRGB(255 - (int)pulse, 255 - (int)pulse, 255 - (int)pulse));
}





void showIcons(String ico, CRGB colors) {
  const uint8_t(*iconArray)[2] = nullptr;  // Change to const int

  unsigned int size = 0;

  if (ico == "icon_samplepack") {
    iconArray = icon_samplepack;
    size = sizeof(icon_samplepack) / sizeof(icon_samplepack[0]);
  } else if (ico == "icon_sample") {
    iconArray = icon_sample;
    size = sizeof(icon_sample) / sizeof(icon_sample[0]);
  } else if (ico == "icon_loadsave") {
    iconArray = icon_loadsave;
    size = sizeof(icon_loadsave) / sizeof(icon_loadsave[0]);
  } else if (ico == "icon_loadsave2") {
    iconArray = icon_loadsave2;
    size = sizeof(icon_loadsave2) / sizeof(icon_loadsave2[0]);
  } else if (ico == "helper_load") {
    iconArray = helper_load;
    size = sizeof(helper_load) / sizeof(helper_load[0]);
  } else if (ico == "helper_seek") {
    iconArray = helper_seek;
    size = sizeof(helper_seek) / sizeof(helper_seek[0]);
  } else if (ico == "helper_folder") {
    iconArray = helper_folder;
    size = sizeof(helper_folder) / sizeof(helper_folder[0]);
  } else if (ico == "helper_save") {
    iconArray = helper_save;
    size = sizeof(helper_save) / sizeof(helper_save[0]);
  } else if (ico == "helper_exit") {
    iconArray = helper_exit;
    size = sizeof(helper_exit) / sizeof(helper_select[0]);
  } else if (ico == "helper_exit") {
    iconArray = helper_select;
    size = sizeof(helper_select) / sizeof(helper_select[0]);
  } else if (ico == "helper_vol") {
    iconArray = helper_vol;
    size = sizeof(helper_vol) / sizeof(helper_vol[0]);
  } else if (ico == "helper_bright") {
    iconArray = helper_bright;
    size = sizeof(helper_bright) / sizeof(helper_bright[0]);
  } else if (ico == "helper_bpm") {
    iconArray = helper_bpm;
    size = sizeof(helper_bpm) / sizeof(helper_bpm[0]);
  } else if (ico == "icon_bpm") {
    iconArray = icon_bpm;
    size = sizeof(icon_bpm) / sizeof(icon_bpm[0]);
  } else if (ico == "icon_settings") {
    iconArray = icon_settings;
    size = sizeof(icon_settings) / sizeof(icon_settings[0]);
  }

  else if (ico == "icon_rec") {
    iconArray = icon_rec;
    size = sizeof(icon_rec) / sizeof(icon_rec[0]);
  } else if (ico == "icon_rec2") {
    iconArray = icon_rec2;
    size = sizeof(icon_rec2) / sizeof(icon_rec2[0]);
  }

  if (iconArray != nullptr) {
    for (unsigned int gx = 0; gx < size; gx++) {
      light(iconArray[gx][0], maxY - iconArray[gx][1], colors);
    }
  }
}



void processPeaks() {
  float interpolatedValues[16];  // Ensure at least 16 values

  // Map seek start and end positions to x range (1-16)
  unsigned int startX = mapf(SMP.seek * SEARCHSIZE, 0, SMP.smplen, 1, 16);
  unsigned int endX = mapf(SMP.seekEnd * SEARCHSIZE, 0, SMP.smplen, 1, 16);

  if (peakIndex > 0) {
    // Distribute peak values over 16 positions
    for (int i = 0; i < 16; i++) {
      float indexMapped = mapf(i, 0, 15, 0, peakIndex - 1);
      int lowerIndex = floor(indexMapped);
      int upperIndex = min(lowerIndex + 1, peakIndex - 1);

      if (peakIndex > 1) {
        float fraction = indexMapped - lowerIndex;
        interpolatedValues[i] = peakValues[lowerIndex] * (1 - fraction) + peakValues[upperIndex] * fraction;
      } else {
        interpolatedValues[i] = peakValues[0];  // Duplicate if only one value exists
      }
    }
  } else {
    // No peaks, default to zero
    for (int i = 0; i < 16; i++) {
      interpolatedValues[i] = 0;
    }
  }

  // Light up LEDs
  for (int i = 0; i < 16; i++) {
    int x = i + 1;  // Ensure x values go from 1 to 16
    int yPeak = mapf(interpolatedValues[i] * 100, 0, 100, 4, 11);
    yPeak = constrain(yPeak, 4, 10);

    for (int y = 4; y <= yPeak; y++) {
      // **Color gradient based on Y (vertical) instead of X**

      CRGB color;
      if (x < startX) {
        color = CRGB(0, 0, 5);  // Green for pre-start region
      } else if (x > endX) {
        color = CRGB(10, 0, 15);  // Dark red for post-end region
      } else {

        color = CRGB(((y - 4) * 35) / 4, (255 - ((y - 4) * 35)) / 4, 0);  // Red to green gradient at 25% brightness
      }

      // Light up from y = 4 to y = yPeak
      light(x, y, color);
    }
  }
}



void displaySample(unsigned int start, unsigned int size, unsigned int end) {
  return;
  unsigned int length = mapf(size, 0, 302000, 1, maxX);
  unsigned int starting = mapf(start * SEARCHSIZE, 0, size, 1, maxX);
  unsigned int ending = mapf(end * SEARCHSIZE, 0, size, 1, maxX);

  //for (unsigned int s = 1; s <= maxX; s++) {
  //light(s, 5, CRGB(10, 20, 10));
  //}

  for (unsigned int s = 1; s <= length; s++) {
    light(s, 9, CRGB(10, 10, 0));
  }

  for (unsigned int s = 1; s <= starting; s++) {
    light(s, 7, CRGB(0, 10, 0));
  }

  for (unsigned int s = starting; s <= ending; s++) {
    light(s, 6, CRGB(10, 10, 10));
  }


  for (unsigned int s = ending; s <= maxX; s++) {
    light(s, 7, CRGB(10, 0, 0));
  }
}


void drawLoadingBar(int minval, int maxval, int currentval, CRGB color, CRGB fontColor, bool intro) {
  int ypos = 4;
  int height = 2;
  int barwidth = mapf(currentval, minval, maxval, 1, maxX);
  for (int x = 1; x <= maxX; x++) {
    light(x, ypos - 1, fontColor);
  }
  //draw the border-ends
  light(1, ypos, fontColor);
  light(maxX, ypos, fontColor);

  for (int x = 2; x <= maxX - 1; x++) {
    for (int y = 0; y <= (height - 1); y++) {
      if (x <= barwidth + 1) {
        light(x, ypos + y, color);
      } else {
        light(x, ypos + y, CRGB(0, 0, 0));
      }
    }
  }
  if (!intro) {
    drawNumber(currentval, fontColor, 11);
  } else {
    FastLED.show();
  }
}



void drawBPMScreen() {

  FastLEDclear();
  drawVolume(SMP.vol);
  drawBrightness();
  CRGB volColor = CRGB(SMP.vol * SMP.vol, 20 - SMP.vol, 0);
  Encoder[2].writeRGBCode(CRGBToUint32(volColor));

  showIcons("helper_exit", CRGB(0, 0, 20));

  showIcons("helper_bright", CRGB(20, 20, 20));
  showIcons("helper_vol", volColor);
  showIcons("helper_bpm", CRGB(0, 50, 120));
  drawNumber(SMP.bpm, CRGB(0, 50, 120), 6);
}
