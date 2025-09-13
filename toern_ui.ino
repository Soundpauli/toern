

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
 void drawPatternChange(int val){
  int width = 12;
 if (val > 9) width=8;
    
//BLACK BOX
  for (int x = width; x <= 16; x++) {      // 6 pixels wide for the number area
    for (int y = 7; y <= 13; y++) {   // Box height (adjust as needed)
      light(x, y, CRGB(0, 0, 0));
    }
  }
  // Draw the number in light gray / white-ish
  drawNumber(val, CRGB(220, 220, 220), 8);

 }

void drawFilterCheck(int mappedValue, FilterType fx, CRGB color) {
  // Map the value (0-maxfilterResolution) to 0-16 LEDs lit
  int activeLength = ::map(mappedValue, 0, maxfilterResolution, 0, 16);
  
  // Draw gradient line at y = 16
  for (int x = 1; x <= 16; x++) {
    if (x <= activeLength) {
      // Gradient from white -> filter color
      float blend = float(x - 1) / max(1, activeLength - 1);  // Prevent div by zero
      CRGB grad = CRGB(
        255 * (1.0 - blend) + color.r * blend,
        255 * (1.0 - blend) + color.g * blend,
        255 * (1.0 - blend) + color.b * blend
      );
      light(x, 16, grad);
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
  drawNumber(mappedValue, color, 8);
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
  if (!GLOB.singleMode) {
    unsigned int colors = 0;
    for (unsigned int y = 1; y < maxY; y++) {
      for (unsigned int x = 1; x < maxX + 1; x++) {
        if (getMuteState(y - 1)) {
          light(x, y, CRGB(0, 0, 0));  // gemutete Zeilen schwarz
        } else {
          light(x, y, col_base[colors]);  // normale Farbcodierung pro Spur
        }
      }
      colors++;
    }

    drawStatus();

    // 4/4-Takt-Hilfsmarkierung in Zeile 1
    for (unsigned int x = 1; x <= 13; x += 4) {
      light(x, 1, CRGB(10, 10, 10));  // hellgraue Taktpunkte
    }

  } else {
    // ---- SINGLE MODE ----
    unsigned int currentChannel = GLOB.currentChannel;
    bool isMuted = getMuteState(currentChannel);

    for (unsigned int y = 1; y < maxY; y++) {
      for (unsigned int x = 1; x < maxX + 1; x++) {
        CRGB color = col_base[currentChannel];

        // Grundton (z. B. C3) pro Kanal etwas heller darstellen
        if (y == currentChannel + 1 ) {
         color = blend(color, CRGB::White, 5); 
        }        

        light(x, y, color);
      }
    }

    drawStatus();

    // 4/4-Takt-Hilfslinien in Zeile 1 (z. B. Start jeder Viertel)
    for (unsigned int x = 1; x <= 13; x += 4) {
      light(x, 1, CRGB(0, 1, 1));  // türkisfarbene Taktmarkierung
    }
  }

  drawPages();
}



void drawStatus() {
  CRGB ledColor = CRGB(0, 0, 0);
  if (GLOB.activeCopy) 
  ledColor = CRGB(20, 20, 0);
  for (unsigned int s = 1; s <= maxX; s++) {
    light(s, 1, ledColor);
  }
  for (unsigned int s = 1; s <= maxX; s++) {
    light(s, 16, ledColor);
  }

  // Add indicators for copy and noteshift functions when cursor is at y=16 in single mode
  if (GLOB.singleMode && GLOB.y == 16) {
    // Red indicator (x=2-3)
    light(2, 1, CRGB(255, 0, 0));  // Red indicator
    light(3, 1, CRGB(255, 0, 0));
    
    // Copy indicator (x=14-15)
    if (GLOB.activeCopy) {
      light(14, 1, CRGB(255, 255, 0));  // Yellow for copy active
      light(15, 1, CRGB(255, 255, 0));
    } else {
      light(14, 1, CRGB(50, 50, 0));    // Dim yellow for copy available
      light(15, 1, CRGB(50, 50, 0));
    }
    
    // Note shift indicator (x=6-7)
    if (currentMode == &noteShift) {
      light(6, 1, CRGB(255, 255, 255));  // White for noteshift active
      light(7, 1, CRGB(255, 255, 255));
    } else {
      light(6, 1, CRGB(100, 100, 100));  // Gray for noteshift available
      light(7, 1, CRGB(100, 100, 100));
    }
  }

  // Add current channel indicator in single mode (x=4-5)
  if (GLOB.singleMode) {
    CRGB channelColor = col_base[GLOB.currentChannel];
    light(4, 1, channelColor);  // Show current channel color
    light(5, 1, channelColor);
    
    // Show copy source channel if copy is active (x=8-9)
    if (GLOB.activeCopy) {
      CRGB copySourceColor = col_base[GLOB.copyChannel];
      light(8, 1, copySourceColor);  // Show copied channel color
      light(9, 1, copySourceColor);
    } else {
      light(8, 1, CRGB(0, 0, 0));  // Dark when no copy
      light(9, 1, CRGB(0, 0, 0));
    }
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
  drawText("v", 2, 2, UI_DIM_WHITE);
  drawText("c", 10, 2, UI_DIM_WHITE);
}






void drawPages() {
  //GLOB.edit = 1;
  CRGB ledColor;

  for (unsigned int p = 1; p <= maxPages; p++) {  // Assuming maxPages is 8

    // If the page is the current one, set the LED to white
    if (GLOB.page == p && GLOB.edit == p) {
      //ledColor = isNowPlaying ? CRGB(20, 255, 20) : CRGB(200, 250, 200);
      ledColor = CRGB(255, 255, 50);
    } else if (GLOB.page == p) {
      //ledColor = isNowPlaying ? CRGB(0, 15, 0) : CRGB(0, 0, 35);
      ledColor = CRGB(0, 255, 0);
    } else {
      if (GLOB.edit == p) {
        //ledColor = GLOB.page == p ? CRGB(50, 50, 50) : CRGB(20, 20, 20);
        ledColor = CRGB(255, 255, 0);
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
  //GLOB.edit = 1;
  for (unsigned int ix = 1; ix < maxX + 1; ix++) {
    for (unsigned int iy = 1; iy < maxY + 1; iy++) {
      int thisNote = note[((GLOB.edit - 1) * maxX) + ix][iy].channel;
      if (thisNote > 0) {

        if (!getMuteState(thisNote)) {
          //light(ix, iy, getCol(note[((GLOB.edit - 1) * maxX) + ix][iy].channel));
          if (GLOB.singleMode && thisNote == GLOB.currentChannel) {
            
            light(ix, iy, UI_BRIGHT_WHITE); //col[thisNote]);
          }else{
              light(ix, iy, col[thisNote]);
          }
          
          
          if (thisNote != GLOB.currentChannel && currentMode == &singleMode) light(ix, iy, col_base[thisNote]);

          // if there is any note of the same value in the same column, make it less bright
          /*
          for (unsigned int iy2 = 1; iy2 < maxY + 1; iy2++) {
            if (iy2 != iy && note[((GLOB.edit - 1) * maxX) + ix][iy2].channel == note[((GLOB.edit - 1) * maxX) + ix][iy].channel) {
              light(ix, iy2, col_base[thisNote]);
            }
          }
          */


        } else {
          //light(ix, iy, getCol(note[((GLOB.edit - 1) * maxX) + ix][iy].channel) / 24);
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
  
  unsigned int timer = ((beat - 1) % maxX + 1);

  if (GLOB.page == GLOB.edit) {
    if (timer < 1) timer = 1;
    for (unsigned int y = 1; y < maxY; y++) {
      int ch = note[((GLOB.page - 1) * maxX) + timer][y].channel;
      light(timer, y, CRGB(10, 0, 0));

      if (ch> 0) {
        if (getMuteState(ch) == false) {
          
                if( !GLOB.singleMode ) {light(timer, y, UI_BRIGHT_WHITE);
      }else{
        if (GLOB.currentChannel == ch){light(timer, y, UI_BRIGHT_WHITE);}
      }
          
        } else {
          if( !GLOB.singleMode ) light(timer, y, CRGB(00, 00, 00));
        }
      }
    }
  }
}


int mapXtoPageOffset(int x) {
  return x - (GLOB.edit - 1) * maxX;
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
  if (note[GLOB.x][GLOB.y].channel) {
    
    CRGB color = col[note[GLOB.x][GLOB.y].channel];  // aus deiner col[] Farbpalette
    light(mapXtoPageOffset(GLOB.x), GLOB.y, color.nscale8_video(pulse));  // pulse = animierte Helligkeit


  }else{
    light(mapXtoPageOffset(GLOB.x), GLOB.y, CHSV(hue, 255, 255)); // Full saturation and brightness
  }
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
        case HELPER_MINUS:
            iconArray = helper_minus;
            size = sizeof(helper_minus) / sizeof(helper_minus[0]);
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
  unsigned int startX = mapf(GLOB.seek, 0, 100, 1, 16);
unsigned int endX = mapf(GLOB.seekEnd, 0, 100, 1, 16);

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
    yPeak = constrain(yPeak, 5, 10);

    for (int y = 5; y <= yPeak; y++) {
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
    if (barwidth<4) drawText("LOAD", 2, 11, UI_BG_DARK);
    if (barwidth>=4 && barwidth<=8) drawText("KIT", 2, 11, UI_BG_DARK);
    char buf[6];
    snprintf(buf, sizeof(buf), "no.%d", SMP.pack );
    if (barwidth>8) drawText(buf, 2, 11, UI_BG_DARK);
    FastLED.show();
  }
}


void drawBPMScreen() {

  FastLEDclear();
  drawVolume(GLOB.vol);
  drawBrightness();
  CRGB volColor = CRGB(GLOB.vol * GLOB.vol, 20 - GLOB.vol, 0);
  Encoder[2].writeRGBCode(CRGBToUint32(volColor));
  showIcons(HELPER_EXIT, UI_DIM_BLUE);
  showIcons(HELPER_BRIGHT, UI_BG_DIM);
  showIcons(HELPER_VOL, volColor);
  if (MIDI_CLOCK_SEND){ 
      showIcons(HELPER_BPM, UI_CYAN);
      drawNumber(SMP.bpm, UI_CYAN, 6);}
      else{
        drawNumber(SMP.bpm, UI_DIM_RED, 6);
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



