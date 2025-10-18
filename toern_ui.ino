

// Fast function to light a specific LED on a specific matrix
// matrixId: 0 = first matrix (left), 1 = second matrix, etc.
// x, y: coordinates within that matrix (1-16, 1-16)
void light_single(unsigned int matrixId, unsigned int x, unsigned int y, CRGB color) {
  if (matrixId >= LED_MODULES) return;  // Invalid matrix ID
  if (x < 1 || x > MATRIX_WIDTH || y < 1 || y > maxY) return;  // Out of bounds
  
  unsigned int matrixOffset = matrixId * 256;  // Each matrix has 256 LEDs
  unsigned int index;
  
  // Calculate index within the matrix using serpentine pattern
  if (y % 2 == 0) {
    // Even rows: right to left
    index = (MATRIX_WIDTH - x) + (MATRIX_WIDTH * (y - 1));
  } else {
    // Odd rows: left to right
    index = (x - 1) + (MATRIX_WIDTH * (y - 1));
  }
  
  // Add matrix offset and set LED
  index += matrixOffset;
  if (index < NUM_LEDS) { 
    leds[index] = color; 
  }
}

// General function to light an LED at global coordinates
void light(unsigned int x, unsigned int y, CRGB color) {
  if (y < 1 || y > maxY || x < 1 || x > maxX) return;
  
  // Determine which matrix (0-based) and local x coordinate within that matrix
  unsigned int matrixNum = (x - 1) / MATRIX_WIDTH;  // Which matrix (0, 1, 2, etc.)
  unsigned int localX = ((x - 1) % MATRIX_WIDTH) + 1;  // Position within that matrix (1-16)
  
  // Use the fast single-matrix function
  light_single(matrixNum, localX, y, color);
  yield();
}

// New indicator system colors
CRGB getIndicatorColor(char colorCode) {
  switch (colorCode) {
    case 'G': return CRGB(0, 255, 0);      // Green
    case 'R': return CRGB(255, 0, 0);      // Red
    case 'X': return CRGB(0, 0, 255);      // Blue
    case 'W': return CRGB(255, 255, 255); // White
    case 'O': return CRGB(255, 165, 0);    // Orange
    case 'H': return CRGB(0, 191, 255);    // Bright Blue
    case 'V': return CRGB(148, 0, 211);    // Violet
    case 'P': return CRGB(255, 192, 203);  // Pink
    case 'Y': return CRGB(255, 255, 0);   // Yellow
    case 'M': return CRGB(255, 0, 255);   // Magenta
    case 'C': return CRGB(0, 0, 0);        // Black (placeholder for CH)
    case 'U': return CRGB(GLOB.vol * GLOB.vol, 20 - GLOB.vol, 0); // Volume color
    case 'D': return CRGB(100, 0, 0);     // Dark Red
    case 'E': return CRGB(0, 100, 0);     // Dark Green
    default: return CRGB(0, 0, 0);        // Default to black
  }
}

// Get current channel color for CH indicator
CRGB getCurrentChannelColor() {
  if (GLOB.currentChannel >= 0 && GLOB.currentChannel < 16) {
    return col_base[GLOB.currentChannel];
  }
  return CRGB(0, 0, 0);
}

// Apply highlighting to a color (brighter or darker)
CRGB applyHighlight(CRGB color, bool highlight) {
  if (highlight) {
    // Make brighter by increasing brightness
    return CRGB(
      min(255, color.r * 1.5),
      min(255, color.g * 1.5),
      min(255, color.b * 1.5)
    );
  } else {
    // Make darker by reducing brightness
    return CRGB(
      color.r * 0.5,
      color.g * 0.5,
      color.b * 0.5
    );
  }
}

// Draw indicator with new format: SIZE[COLOR]
void drawIndicator(char size, char colorCode, int encoderNum, bool highlight = false) {
  CRGB color = getIndicatorColor(colorCode);
  
  // Handle special case for CH (current channel color)
  if (colorCode == 'C') {
    color = getCurrentChannelColor();
  }
  
  // Apply highlighting if requested
  if (highlight) {
    color = applyHighlight(color, true);
  }
  
  // Determine x positions based on encoder number
  int x1, x2, x3;
  switch (encoderNum) {
    case 1:
      x1 = 1; x2 = 2; x3 = 3; // L: 1,2,3 / M: 2,3 / S: 3
      break;
    case 2:
      x1 = 5; x2 = 6; x3 = 7; // L: 5,6,7 / M: 6,7 / S: 6
      break;
    case 3:
      x1 = 9; x2 = 10; x3 = 11; // L: 9,10,11 / M: 10,11 / S: 10
      break;
    case 4:
      x1 = 13; x2 = 14; x3 = 15; // L: 13,14,15 / M: 14,15 / S: 14
      break;
    default:
      return; // Invalid encoder number
  }
  
  // Draw based on size
  switch (size) {
    case 'S': // Small: 1px wide, 1px high
      light(x3, 1, color);
      break;
    case 'M': // Medium: 2px wide, 1px high
      light(x2, 1, color);
      light(x3, 1, color);
      break;
    case 'L': // Large: 3px wide, 1px high
      light(x1, 1, color);
      light(x2, 1, color);
      light(x3, 1, color);
      break;
    case 'C': // Cross: 3px wide, 3px high
      // Draw horizontal line
      light(x1, 1, color);
      light(x2, 1, color);
      light(x3, 1, color);
      // Draw vertical line
      light(x2, 1, color);
      light(x2, 2, color);
      light(x2, 3, color);
      break;
  }
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
  for (int x = width; x <= maxX; x++) {      // Cover to end of display
    for (int y = 7; y <= 13; y++) {   // Box height (adjust as needed)
      light(x, y, CRGB(0, 0, 0));
    }
  }
  // Draw the number in light gray / white-ish
  drawNumber(val, CRGB(220, 220, 220), 8);

 }

void drawFilterCheck(int mappedValue, FilterType fx, CRGB color) {
  // Map the value (0-maxfilterResolution) to 0-maxX LEDs lit
  int activeLength = ::map(mappedValue, 0, maxfilterResolution, 0, maxX);
  
  // Draw gradient line at y = maxY
  for (int x = 1; x <= maxX; x++) {
    if (x <= activeLength) {
      // Gradient from white -> filter color
      float blend = float(x - 1) / max(1, activeLength - 1);  // Prevent div by zero
      CRGB grad = CRGB(
        255 * (1.0 - blend) + color.r * blend,
        255 * (1.0 - blend) + color.g * blend,
        255 * (1.0 - blend) + color.b * blend
      );
      light(x, maxY, grad);
    } else {
      // Empty part stays black
      light(x, maxY, CRGB(0, 0, 0));
    }
    
  }
 int width = 12;
 if (mappedValue > 9) width=8;
  // Draw black box background for the number
  for (int x = width; x <= maxX; x++) {      // Cover to end of display
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
      // Flash red on every 4th beat (works for any display width)
      if (timer % 4 == 1) {
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
          if (drawBaseColorMode) {
            light(x, y, col_base[colors]);  // normale Farbcodierung pro Spur
          } else {
            light(x, y, CRGB(0, 0, 0));  // schwarz wenn drawBaseColorMode = false
          }
        }
      }
      colors++;
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

    
   

    

  }

   // 4/4-Takt-Hilfslinien in Zeile 1 (z. B. Start jeder Viertel)
   // Mark every 4th position across the full width
   for (unsigned int x = 1; x <= maxX - 3; x += 4) {
    light(x, 1, CRGB(10, 10, 10));  // türkisfarbene Taktmarkierung
  }

  drawStatus();

  drawPages();
}



void drawStatus() {
 

  
  // If copy is active, show specific indicators
  if (GLOB.activeCopy) {

  
    // Clear y=16 completely
    for (unsigned int s = 1; s <= maxX; s++) {
      light(s, 16, CRGB(0, 0, 0));
    }
    
    // Show active copy indicators: w[X] / - / - / G[Y]
    drawIndicator('L', 'X', 1);  // Encoder 1: Large Blue
    // Encoder 2: empty (no indicator)
    // Encoder 3: empty (no indicator)
    drawIndicator('L', 'Y', 4);  // Encoder 4: Large Yellow
    
    // Set encoder colors to match active copy indicators
    CRGB blueColor = getIndicatorColor('X'); // Blue
    CRGB yellowColor = getIndicatorColor('Y'); // Yellow
    
    Encoder[0].writeRGBCode(0xFF0000);
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[3].writeRGBCode(yellowColor.r << 16 | yellowColor.g << 8 | yellowColor.b);
    
    return; // Exit early to prevent any other indicators
  }

  // Add indicators for copy and noteshift functions when cursor is at y=16 in single mode
  // Only show indicators when copy is NOT active
  if (GLOB.singleMode && GLOB.y == 16 && !GLOB.activeCopy) {
    // New indicator system: copypaste: M[X] | | | M[G]
    drawIndicator('M', 'X', 1);  // Encoder 1: Medium Blue
    // Encoder 2: empty (no indicator)
    // Encoder 3: empty (no indicator)
    drawIndicator('M', 'G', 4);  // Encoder 4: Medium Green
    
    // Set encoder colors to match copypaste indicators
    CRGB blueColor = getIndicatorColor('X'); // Blue
    CRGB greenColor = getIndicatorColor('G'); // Green
    
    Encoder[0].writeRGBCode(0xFF0000);
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[3].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
    
    // Note shift indicator: L[W] | L[G] | | L[W]
    if (currentMode == &noteShift) {
      
    } 
  }

  // Add current channel indicator in single mode (x=4-5)
  if (GLOB.singleMode) {
    CRGB channelColor = col_base[GLOB.currentChannel];
    
    
 
  }
  
  // Add draw mode indicators when y=16
  if (currentMode == &draw && GLOB.y == 16) {
    // New indicator system: draw(+y=16): M[R] | | | M[Y]
    drawIndicator('M', 'R', 1);  // Encoder 1: Medium Red
    // Encoder 2: empty (no indicator)
    // Encoder 3: empty (no indicator)
    drawIndicator('M', 'Y', 4);  // Encoder 4: Medium Yellow
    
    // Set encoder colors to match draw mode indicators
    CRGB redColor = getIndicatorColor('R'); // Red
    CRGB yellowColor = getIndicatorColor('Y'); // Yellow
    
    Encoder[0].writeRGBCode(redColor.r << 16 | redColor.g << 8 | redColor.b);
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[3].writeRGBCode(yellowColor.r << 16 | yellowColor.g << 8 | yellowColor.b);
  }
  
  // Add single mode indicators when y=16
  if (GLOB.singleMode && GLOB.y == 16) {
    // New indicator system: singlemode(y=16): M[R] | M[W] | | M[Y]
    drawIndicator('M', 'R', 1);  // Encoder 1: Medium Red
    drawIndicator('M', 'W', 2);  // Encoder 2: Medium White
    // Encoder 3: empty (no indicator)
    drawIndicator('M', 'Y', 4);  // Encoder 4: Medium Yellow
    
    // Set encoder colors to match single mode indicators
    CRGB redColor = getIndicatorColor('R'); // Red
    CRGB whiteColor = getIndicatorColor('W'); // White
    CRGB yellowColor = getIndicatorColor('Y'); // Yellow
    
    Encoder[0].writeRGBCode(redColor.r << 16 | redColor.g << 8 | redColor.b);
    Encoder[1].writeRGBCode(whiteColor.r << 16 | whiteColor.g << 8 | whiteColor.b);
    Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[3].writeRGBCode(yellowColor.r << 16 | yellowColor.g << 8 | yellowColor.b);
  } else if (GLOB.singleMode && GLOB.y != 16) {
    // Reset encoder[1] to black when not at y=16 in single mode
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
  }

  if (currentMode == &noteShift) {
    // draw a moving marquee to indicate the note shift mode
    for (unsigned int x = 1; x <= maxX; x++) {
      light(x, 1, CRGB(0, 0, 0));
    }
    light(round(marqueePos), 1, CRGB(120, 120, 120));
      drawIndicator('L', 'G', 1, true);   // Highlighted green for noteshift active
      //drawIndicator('L', 'G', 2, true);  // Highlighted green for noteshift active
      // Encoder 3: empty (no indicator)
      drawIndicator('L', 'X', 4, true);   // Highlighted blue for noteshift active
      
      // Set encoder colors to match noteshift indicators (highlighted)
      CRGB greenColor = applyHighlight(getIndicatorColor('G'), true); // Highlighted Green
      CRGB blueColor = applyHighlight(getIndicatorColor('X'), true); // Highlighted Blue
      
      Encoder[0].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
      Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
      Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
      Encoder[3].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);

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

  int startX = (maxX + 1) - textPixelWidth;  // Align text to right side of display
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

  // First, clear the entire top row across all matrices
  for (unsigned int x = 1; x <= maxX; x++) {
    light(x, maxY, CRGB(0, 0, 0));  // Clear to black
  }

  // Then draw page indicators at positions 1-16 (or up to maxPages)
  for (unsigned int p = 1; p <= maxPages && p <= maxX; p++) {

    // If the page is the current one, set the LED to white
    if (GLOB.page == p && GLOB.edit == p) {
      //ledColor = isNowPlaying ? CRGB(20, 255, 20) : CRGB(200, 250, 200);
      ledColor = CRGB(255, 255, 50);  // Playing and editing
    } else if (GLOB.page == p) {
      //ledColor = isNowPlaying ? CRGB(0, 15, 0) : CRGB(0, 0, 35);
      ledColor = CRGB(0, 255, 0);  // Playing
    } else {
      if (GLOB.edit == p) {
        //ledColor = GLOB.page == p ? CRGB(50, 50, 50) : CRGB(20, 20, 20);
        ledColor = CRGB(255, 255, 0);  // Editing
      } else {
        ledColor = hasNotes[p] ? CRGB(0, 0, 35) : CRGB(1, 0, 0);  // Has notes / empty
      }
    }

    // Set the LED color at position p on top row
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
          
         /* if (thisNote != 11) {
          for (unsigned int iy2 = 1; iy2 < maxY + 1; iy2++) {
            if (iy2 != iy && note[((GLOB.edit - 1) * maxX) + ix][iy2].channel == note[((GLOB.edit - 1) * maxX) + ix][iy].channel) {
              CRGB color = blend(col_base[thisNote], CRGB::Black, 0.2); 
              light(ix, iy2, color);
            }
          }
        }*/
          


        } else {
          //light(ix, iy, getCol(note[((GLOB.edit - 1) * maxX) + ix][iy].channel) / 24);
          light(ix, iy, col_base[thisNote]);
        }
      }
    }
  }
  
  // Simple Notes View: Draw notes at their voice Y position instead of their actual Y position
  if (simpleNotesView == 1 && !GLOB.singleMode) {
    for (unsigned int ix = 1; ix < maxX + 1; ix++) {
      for (unsigned int iy = 1; iy < maxY + 1; iy++) {
        int thisNote = note[((GLOB.edit - 1) * maxX) + ix][iy].channel;
        if (thisNote > 0) {
          // Calculate the voice Y position (channel + 1)
          int voiceY = thisNote + 1;
          
          // Only draw if the voice Y position is within bounds
          if (voiceY >= 1 && voiceY <= maxY) {
            // Clear the original position first
            light(ix, iy, CRGB(0, 0, 0));
            
            // Draw at the voice Y position - use muted color if muted, normal color if not
            if (getMuteState(thisNote)) {
              light(ix, voiceY, col_base[thisNote]);  // Muted color
            } else {
              light(ix, voiceY, col[thisNote]);       // Normal color
            }
          }
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
  float interpolatedValues[maxX];  // Support full width of display

  // Map seek start and end positions to x range (1-maxX)
  unsigned int startX = mapf(GLOB.seek, 0, 100, 1, maxX);
  unsigned int endX = mapf(GLOB.seekEnd, 0, 100, 1, maxX);

  if (peakIndex > 0) {
    // Distribute peak values over maxX positions
    for (int i = 0; i < maxX; i++) {
      float indexMapped = mapf(i, 0, maxX - 1, 0, peakIndex - 1);
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
    for (int i = 0; i < maxX; i++) {
      interpolatedValues[i] = 0;
    }
  }

  // Light up LEDs
  for (int i = 0; i < maxX; i++) {
    int x = i + 1;  // Ensure x values go from 1 to maxX
    int yPeak = mapf(interpolatedValues[i] * 100, 0, 100, 4, 11);
    yPeak = constrain(yPeak, 5, 10);

    for (int y = 5; y <= yPeak; y++) {
      // **Color gradient based on Y (vertical) instead of X**

      CRGB color;
      if (x < startX) {
        color = CRGB(0, 0, 5);  // Green for pre-start region
      } else if (x > endX) {
        color = CRGB(15, 15, 00);  // yellow for post-end region
      } else {

        color = CRGB(((y - 4) * 35) / 4, (255 - ((y - 4) * 35)) / 4, 0);  // Red to green gradient at 25% brightness
      }

      // Light up from y = 4 to y = yPeak
      light(x, y, color);
    }
  }
}



void processRecPeaks() {
  float interpolatedRecValues[maxX];  // Support full width of display



  if (peakIndex > 0) {
    // Distribute peak values over maxX positions
    for (int i = 0; i < maxX; i++) {
      float indexMapped = mapf(i, 0, maxX - 1, 0, peakIndex - 1);
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
    for (int i = 0; i < maxX; i++) {
      interpolatedRecValues[i] = 0;
    }
  }

  // Light up LEDs
  for (int i = 0; i < maxX; i++) {
    int x = i + 1;  // Ensure x values go from 1 to maxX
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
  if (drawBaseColorMode) {
    drawBrightness();
  }
  CRGB volColor = CRGB(GLOB.vol * GLOB.vol, 20 - GLOB.vol, 0);
  Encoder[2].writeRGBCode(CRGBToUint32(volColor));
  
  // New indicator system: BPM: L[H] | L[W] | L[U] | -
  
  drawIndicator('L', 'W', 2);  // Encoder 2: Large White
  drawIndicator('L', 'U', 3);  // Encoder 3: Large Volume Color
  if (drawBaseColorMode) {
    drawIndicator('L', 'H', 4);  // Encoder 1: Large Bright Blue
  }
  // Encoder 4: empty (no indicator)
  
  if (MIDI_CLOCK_SEND){ 
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



