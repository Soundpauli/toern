

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



// Draws the above shape, with its top-left at (startX, startY).
void drawLowResCircle(int startX, int startY, CRGB color) {
  // The pattern has 7 rows and 9 columns.
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 9; col++) {
      if (LOWRES_CIRCLE[row][col] == 'X') {
        // Light only the positions marked 'X'.
        light(startX + col, startY + row, color);
      }
    }
  }
}


void drawFilterCheck(int mappedValue, FilterType fx) {
  // Map the value (0-maxfilterResolution) to 0-16 LEDs lit
  int activeLength = ::map(mappedValue, 0, maxfilterResolution, 0, 16);
  
  // Draw gradient line at y = 16
  for (int x = 1; x <= 16; x++) {
    if (x <= activeLength) {
      // Gradient from white -> filter color
      float blend = float(x - 1) / max(1, activeLength - 1);  // Prevent div by zero
      CRGB color = CRGB(
        255 * (1.0 - blend) + filter_col[fx].r * blend,
        255 * (1.0 - blend) + filter_col[fx].g * blend,
        255 * (1.0 - blend) + filter_col[fx].b * blend
      );
      light(x, 16, color);
    } else {
      // Empty part stays black
      light(x, 16, CRGB(0, 0, 0));
    }
    
  }
 int width = 12;
 if (mappedValue > 9) width=8;
  // Draw black box background for the number
  for (int x = width; x <= 16; x++) {      // 6 pixels wide for the number area
    for (int y = 7; y <= 13; y++) {   // Box height (adjust as needed)
      light(x, y, CRGB(0, 0, 0));
    }
  }

  // Draw the number in light gray / white-ish
  drawNumber(mappedValue, filter_col[fx], 8);
}


void drawPlayButton() {
  unsigned int timer = (beat - 1) % maxX + 1;
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
  playSdWav1.stop();
  if (drawNoSD_hasRun) return;  // Prevent running again
  

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
  }

  drawNoSD_hasRun = true;  // Mark it as run
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
      
  /*for ( int x = 1; x <= 16; x++) {
    int brightness;
    if (x <= 8) {
      brightness = ::map(x, 2, 8, 0, 80);  // Ramp up from black to white
    } else {
      brightness = ::map(x, 9, 15, 80, 0); // Ramp down from white to black
    }
    brightness = constrain(brightness, 0, 255); // Safety limit
    light(x, 11, CRGB(brightness, brightness, brightness));
  }*/


  //4-4 helper takt
    for (unsigned int x = 1; x <= 13; x += 4) {
      light(x, 1, CRGB(10, 10, 10));  
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
  for (unsigned int s = 1; s <= maxX; s++) {
    light(s, 16, ledColor);
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



void drawNumber(float count, CRGB color, int topY) {
  char buffer[16];

  // Check if the number is "effectively" an integer
  if (fabs(count - (int)count) < 0.01) {
    // It's a whole number, print as int
    sprintf(buffer, "%d", (int)count);
  } else {
    // It's a float with decimals, print with 2 decimals
    dtostrf(count, 0, 2, buffer);  // Example: 1.25 -> "1.25"
  }

  // Dynamically calculate pixel width for alignment
  int textPixelWidth = 0;
  for (int i = 0; buffer[i] != '\0'; i++) {
    if (buffer[i] >= 32 && buffer[i] <= 126) {
      textPixelWidth += alphabet[buffer[i] - 32][0] + 1;  // Char width + 1 space
    }
  }

  int startX = 17 - textPixelWidth;  // Adjust 14 based on your screen width
  if (startX < 0) startX = 0;

  drawText(buffer, startX, topY, color);
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






void drawPages() {
  //SMP.edit = 1;
  CRGB ledColor;

  for (unsigned int p = 1; p <= maxPages; p++) {  // Assuming maxPages is 8

    // If the page is the current one, set the LED to white
    if (SMP.page == p && SMP.edit == p) {
      ledColor = isNowPlaying ? CRGB(20, 255, 20) : CRGB(200, 250, 200);
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
      int thisNote = note[((SMP.edit - 1) * maxX) + ix][iy].channel;
      if (thisNote > 0) {

        if (!SMP.mute[thisNote]) {
          //light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy].channel));
          if (SMP.singleMode && thisNote == SMP.currentChannel) {
            
            light(ix, iy, CRGB(200,200,200)); //col[thisNote]);
          }else{
              light(ix, iy, col[thisNote]);
          }
          
          
          if (thisNote != SMP.currentChannel && currentMode == &singleMode) light(ix, iy, col_base[thisNote]);

          // if there is any note of the same value in the same column, make it less bright
          /*
          for (unsigned int iy2 = 1; iy2 < maxY + 1; iy2++) {
            if (iy2 != iy && note[((SMP.edit - 1) * maxX) + ix][iy2].channel == note[((SMP.edit - 1) * maxX) + ix][iy].channel) {
              light(ix, iy2, col_base[thisNote]);
            }
          }
          */


        } else {
          //light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy].channel) / 24);
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
  
  unsigned int timer = (beat - 1) % maxX + 1;

  if (SMP.page == SMP.edit) {
    if (timer < 1) timer = 1;
    for (unsigned int y = 1; y < maxY; y++) {
      int ch = note[((SMP.page - 1) * maxX) + timer][y].channel;
      light(timer, y, CRGB(10, 0, 0));

      if (ch> 0) {
        if (SMP.mute[ch] == 0) {
          
          if( !SMP.singleMode ) {light(timer, y, CRGB(200, 200, 200));
          }else{
            if (SMP.currentChannel == ch){light(timer, y, CRGB(200, 200, 200));}
          }
          
        } else {
          if( !SMP.singleMode ) light(timer, y, CRGB(00, 00, 00));
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
    pulse += 8;
  if (dir == -1)
    pulse -= 8;
  if (pulse > 230) {
    dir = -1;
  }
  if (pulse < 1) {
    dir = 1;
  }

  uint8_t hue = pulse; // Directly use pulse as hue for smooth cycling
  light(mapXtoPageOffset(SMP.x), SMP.y, CHSV(hue, 255, 255)); // Full saturation and brightness
}



void drawRecMode() {

  if (recMode == 1) {
    drawText("mic", 7, 10, CRGB(200, 200, 200));
    recInput = AUDIO_INPUT_MIC;
  }
  if (recMode == -1) {
    drawText("line", 6, 10, CRGB(0, 0, 200));
    recInput = AUDIO_INPUT_LINEIN;
  }
  sgtl5000_1.inputSelect(recInput);


  FastLEDshow();
}


void drawVolume(unsigned int vol) {
  unsigned int maxXVolume = int(vol * 1.3) + 2;
  for (unsigned int x = 0; x <= maxXVolume; x++) {
    light(x + 1, 12, CRGB(vol * vol, 20 - vol, 0));
    light(x + 1, 13, CRGB(vol * vol, 20 - vol, 0));
  }
}

void drawBrightness() {
  unsigned int maxBrightness = ((ledBrightness - 65) * (15 - 1)) / (255 - 65) + 3;
  for (unsigned int x = 0; x < maxBrightness; x++) {
    CRGB brightness = CRGB(16 * x, 16 * x, 16 * x);
    light(x, 15, brightness);
    light(x, 16, brightness);
  }
}



void showIcons(IconType ico, CRGB colors) {
    // Pointer to icon array and its size
    const uint8_t (*iconArray)[2] = nullptr;
    unsigned int size = 0;

    // Select the icon based on the enum value
    switch (ico) {
        case ICON_DELETE:
            iconArray = icon_delete;
            size = sizeof(icon_delete) / sizeof(icon_delete[0]);
            break;
        case ICON_SAMPLEPACK:
            iconArray = icon_samplepack;
            size = sizeof(icon_samplepack) / sizeof(icon_samplepack[0]);
            break;
        case ICON_SAMPLE:
            iconArray = icon_sample;
            size = sizeof(icon_sample) / sizeof(icon_sample[0]);
            break;
        case ICON_LOADSAVE:
            iconArray = icon_loadsave;
            size = sizeof(icon_loadsave) / sizeof(icon_loadsave[0]);
            break;
        case ICON_LOADSAVE2:
            iconArray = icon_loadsave2;
            size = sizeof(icon_loadsave2) / sizeof(icon_loadsave2[0]);
            break;

        case ICON_NEW:
            iconArray = icon_new;
            size = sizeof(icon_new) / sizeof(icon_new[0]);
            break;

        case HELPER_LOAD:
            iconArray = helper_load;
            size = sizeof(helper_load) / sizeof(helper_load[0]);
            break;
        case HELPER_SEEK:
            iconArray = helper_seek;
            size = sizeof(helper_seek) / sizeof(helper_seek[0]);
            break;
        case HELPER_SEEKSTART:
            iconArray = helper_seekstart;
            size = sizeof(helper_seekstart) / sizeof(helper_seekstart[0]);
            break;
        case HELPER_FOLDER:
            iconArray = helper_folder;
            size = sizeof(helper_folder) / sizeof(helper_folder[0]);
            break;
        case HELPER_SAVE:
            iconArray = helper_save;
            size = sizeof(helper_save) / sizeof(helper_save[0]);
            break;
        case HELPER_EXIT:
            iconArray = helper_exit;
            size = sizeof(helper_exit) / sizeof(helper_exit[0]);
            break;
        case HELPER_SELECT:
            iconArray = helper_select;
            size = sizeof(helper_select) / sizeof(helper_select[0]);
            break;
        case HELPER_VOL:
            iconArray = helper_vol;
            size = sizeof(helper_vol) / sizeof(helper_vol[0]);
            break;
        case HELPER_BRIGHT:
            iconArray = helper_bright;
            size = sizeof(helper_bright) / sizeof(helper_bright[0]);
            break;
        case HELPER_BPM:
            iconArray = helper_bpm;
            size = sizeof(helper_bpm) / sizeof(helper_bpm[0]);
            break;
        case ICON_BPM:
            iconArray = icon_bpm;
            size = sizeof(icon_bpm) / sizeof(icon_bpm[0]);
            break;
        case ICON_SETTINGS:
            iconArray = icon_settings;
            size = sizeof(icon_settings) / sizeof(icon_settings[0]);
            break;
        case ICON_REC:
            iconArray = icon_rec;
            size = sizeof(icon_rec) / sizeof(icon_rec[0]);
            break;
        case ICON_REC2:
            iconArray = icon_rec2;
            size = sizeof(icon_rec2) / sizeof(icon_rec2[0]);
            break;
        default:
            // Optionally handle unrecognized values
            break;
    }

    // If a valid icon array is selected, call the light function for each element.
    if (iconArray != nullptr) {
        for (unsigned int gx = 0; gx < size; gx++) {
            light(iconArray[gx][0], maxY - iconArray[gx][1], colors);
        }
    }
}



void processPeaks() {
  float interpolatedValues[16];  // Ensure at least 16 values

  // Map seek start and end positions to x range (1-16)
  unsigned int startX = mapf(SMP.seek, 0, 100, 1, 16);
unsigned int endX = mapf(SMP.seekEnd, 0, 100, 1, 16);

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



void processRecPeaks() {
  float interpolatedRecValues[16];  // Ensure at least 16 values



  if (peakIndex > 0) {
    // Distribute peak values over 16 positions
    for (int i = 0; i < 16; i++) {
      float indexMapped = mapf(i, 0, 15, 0, peakIndex - 1);
      int lowerIndex = floor(indexMapped);
      int upperIndex = min(lowerIndex + 1, peakIndex - 1);

      if (peakIndex > 1) {
        float fraction = indexMapped - lowerIndex;
        interpolatedRecValues[i] = peakRecValues[lowerIndex] * (1 - fraction) + peakRecValues[upperIndex] * fraction;
      } else {
        interpolatedRecValues[i] = peakRecValues[0];  // Duplicate if only one value exists
      }
    }
  } else {
    // No peaks, default to zero
    for (int i = 0; i < 16; i++) {
      interpolatedRecValues[i] = 0;
    }
  }

  // Light up LEDs
  for (int i = 0; i < 16; i++) {
    int x = i + 1;  // Ensure x values go from 1 to 16
    int yPeak = mapf(interpolatedRecValues[i] * 100, 0, 100, 4, 11);
    yPeak = constrain(yPeak, 4, 10);

    for (int y = 4; y <= yPeak; y++) {
      // **Color gradient based on Y (vertical) instead of X**

      CRGB color;
     

        color = CRGB(((y - 4)*0) / 4, (255 - ((y - 4) * 135)) / 4, 0);  // Red to green gradient at 25% brightness
    

      // Light up from y = 4 to y = yPeak
      light(x, y, color);
    }
  }
}



void drawLoadingBar(int minval, int maxval, int currentval, CRGB color, CRGB fontColor, bool intro) {
  int ypos = 4;
  int height = 2;
  int barwidth = mapf(currentval, minval, maxval, 1, maxX);
  FastLEDclear();
  
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
    if (barwidth<4) drawText("LOAD", 2, 11, CRGB(20,20,20));
    if (barwidth>=4 && barwidth<=8) drawText("KIT", 2, 11, CRGB(20,20,20));
    char buf[6];
    snprintf(buf, sizeof(buf), "no.%d", SMP.pack );
    if (barwidth>8) drawText(buf, 2, 11, CRGB(20,20,20));
    FastLED.show();
  }
}


void drawBPMScreen() {

  FastLEDclear();
  drawVolume(SMP.vol);
  drawBrightness();
  CRGB volColor = CRGB(SMP.vol * SMP.vol, 20 - SMP.vol, 0);
  Encoder[2].writeRGBCode(CRGBToUint32(volColor));

  showIcons(HELPER_BRIGHT, CRGB(20, 20, 20));
  showIcons(HELPER_VOL, volColor);
  if (MIDI_CLOCK_SEND){ 
      showIcons(HELPER_BPM, CRGB(0, 50, 120));
      drawNumber(SMP.bpm, CRGB(0, 50, 120), 6);}
      else{
        drawNumber(SMP.bpm, CRGB(20, 0 , 0), 6);
      }
}

void drawKnobColorDefault(){
  for (int i = 0; i < NUM_ENCODERS; i++) {
      Encoder[i].writeRGBCode(currentMode->knobcolor[i]);
  }
}



/*void deleted_displaySample(unsigned int start, unsigned int size, unsigned int end) {
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
}*/




