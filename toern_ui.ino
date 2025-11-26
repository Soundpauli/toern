extern const unsigned int maxlen;
extern void triggerGridNote(unsigned int globalX, unsigned int y);
extern const CRGB col[];
extern unsigned int beatForUI;
extern uint8_t lineOutLevelSetting;

#define LINEOUT_LEVEL_MIN 13
#define LINEOUT_LEVEL_MAX 31

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
    case 'B': return CRGB(0, 150, 255);    // Teal for Pong speed
    case 'V': return CRGB(148, 0, 211);    // Violet
    case 'P': return CRGB(50, 0, 50);  // Pink
    case 'Y': return CRGB(255, 255, 0);   // Yellow
    case 'M': return CRGB(255, 0, 255);   // Magenta
    case 'C': return CRGB(0, 0, 0);        // Black (placeholder for CH)
    case 'U': return CRGB(GLOB.vol * GLOB.vol, 20 - GLOB.vol, 0); // Volume color
    case 'D': return CRGB(100, 0, 0);     // Dark Red
    case 'E': return CRGB(0, 100, 0);     // Dark Green
    case 'N': return CRGB(0, 120, 120);   // Cyan
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

  if (encoderNum >= 1 && encoderNum <= NUM_ENCODERS) {
    uint32_t rgbCode = (uint32_t(color.r) << 16) | (uint32_t(color.g) << 8) | color.b;
    Encoder[encoderNum - 1].writeRGBCode(rgbCode);
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
FLASHMEM void drawLowResCircle(int startX, int startY, CRGB color) {
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
  unsigned int timer = (beatForUI - 1) % maxX + 1;
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
    drawText("SD?", 3, 8, UI_RED);
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

static bool pongBallInitialized = false;
static int pongBallX = 1;
static int pongBallY = 1;
static int pongVelocityX = 1;
static int pongVelocityY = 1;
static unsigned long pongLastUpdate = 0;
DMAMEM uint16_t pongUpdateInterval __attribute__((aligned(4))) = 70;

// Comet tail tracking (using int8_t to save RAM)
static const int8_t pongTailLength = 4;
DMAMEM int8_t pongTailX[pongTailLength] __attribute__((aligned(4)));
DMAMEM int8_t pongTailY[pongTailLength] __attribute__((aligned(4)));
DMAMEM int8_t pongTailIndex = 0;
DMAMEM uint8_t pongCollisionCount = 0;

FLASHMEM static void adjustPongAngle(int &vx, int &vy) {
  int signX = (vx >= 0) ? 1 : -1;
  int signY = (vy >= 0) ? 1 : -1;
  int absVx = abs(vx);
  int absVy = abs(vy);

  int currentIdx;
  if (absVx == 1 && absVy == 1) {
    currentIdx = 0;
  } else if (absVx == 2 && absVy == 1) {
    currentIdx = 1;
  } else {
    currentIdx = 2;  // absVx == 1 && absVy == 2
  }

  int newIdx = currentIdx;
  for (int attempts = 0; attempts < 4 && newIdx == currentIdx; attempts++) {
    newIdx = random(0, 3);
  }

  int newAbsVx = 1;
  int newAbsVy = 1;
  switch (newIdx) {
    case 0: newAbsVx = 1; newAbsVy = 1; break;
    case 1: newAbsVx = 2; newAbsVy = 1; break;
    case 2: newAbsVx = 1; newAbsVy = 2; break;
  }

  vx = signX * newAbsVx;
  vy = signY * newAbsVy;
}

 FLASHMEM static void registerPongCollision(int &vx, int &vy) {
  pongCollisionCount++;
  if (pongCollisionCount >= 10) {
    pongCollisionCount = 0;
    adjustPongAngle(vx, vy);
  }
}

FLASHMEM void resetPongGame() {
  pongBallInitialized = false;
  // Clear tail history
  for (int i = 0; i < pongTailLength; i++) {
    pongTailX[i] = -1;
    pongTailY[i] = -1;
  }
  pongTailIndex = 0;
  pongCollisionCount = 0;
}

FLASHMEM static void initPongBall() {
  // Spawn at random position
  pongBallX = random(1, maxX + 1);
  pongBallY = random(1, maxY + 1);

  // Random angle between 25-65° to avoid perfect 45°
  int angle = random(25, 66);
  bool flipX = (random(0, 2) == 0);
  bool flipY = (random(0, 2) == 0);

  // Simple angle-to-velocity mapping without floats:
  // angle 30°→vx=2,vy=1  45°→vx=1,vy=1  60°→vx=1,vy=2
  if (angle < 40) {
    pongVelocityX = 2;
    pongVelocityY = 1;
  } else if (angle > 50) {
    pongVelocityX = 1;
    pongVelocityY = 2;
  } else {
    pongVelocityX = 1;
    pongVelocityY = 1;
  }

  if (flipX) pongVelocityX = -pongVelocityX;
  if (flipY) pongVelocityY = -pongVelocityY;

  pongLastUpdate = millis();
  pongBallInitialized = true;
}

static bool pongNoteAt(int px, int py) {
  if (px < 1 || px > (int)maxX || py < 1 || py > maxY) {
    return false;
  }

  unsigned int pageIndex = (GLOB.edit > 0) ? (GLOB.edit - 1) : 0;
  unsigned int globalX = (pageIndex * maxX) + px;
  if (globalX > maxlen) {
    return false;
  }

  int channel = note[globalX][py].channel;
  if (channel == 0) return false;
  
  // Don't bounce off muted voices
  if (getMuteState(channel)) return false;
  
  return true;
}

static void triggerPongCollision(int px, int py) {
  if (px < 1 || px > (int)maxX || py < 1 || py > maxY) {
    return;
  }

  unsigned int pageIndex = (GLOB.edit > 0) ? (GLOB.edit - 1) : 0;
  unsigned int globalX = (pageIndex * maxX) + px;
  if (globalX > maxlen) {
    return;
  }

  triggerGridNote(globalX, py);
}

FLASHMEM void updatePongBall() {
  if (!pongBallInitialized) {
    initPongBall();
  }

  unsigned long now = millis();
  if (now - pongLastUpdate < pongUpdateInterval) {
    return;
  }
  pongLastUpdate = now;

  int vx = pongVelocityX;
  int vy = pongVelocityY;

  int nextX = pongBallX + vx;
  int nextY = pongBallY + vy;

  if (nextX < 1 || nextX > (int)maxX) {
    vx = -vx;
    nextX = pongBallX + vx;
  }

  if (nextY < 1 || nextY > maxY) {
    vy = -vy;
    nextY = pongBallY + vy;
  }

  bool axisCollision = false;

  // Check X-axis collisions (check ALL cells along the path)
  if (vx != 0) {
    int stepX = (vx > 0) ? 1 : -1;
    for (int step = 1; step <= abs(vx); step++) {
      int checkX = pongBallX + (step * stepX);
      if (checkX >= 1 && checkX <= (int)maxX && pongNoteAt(checkX, pongBallY)) {
        triggerPongCollision(checkX, pongBallY);
        vx = -vx;
        axisCollision = true;
        registerPongCollision(vx, vy);
        break;
      }
    }
  }

  // Check Y-axis collisions (check ALL cells along the path)
  if (vy != 0 && !axisCollision) {
    int stepY = (vy > 0) ? 1 : -1;
    for (int step = 1; step <= abs(vy); step++) {
      int checkY = pongBallY + (step * stepY);
      if (checkY >= 1 && checkY <= maxY && pongNoteAt(pongBallX, checkY)) {
        triggerPongCollision(pongBallX, checkY);
        vy = -vy;
        axisCollision = true;
        registerPongCollision(vx, vy);
        break;
      }
    }
  }

  // Check diagonal collision only if no axis collision
  if (!axisCollision) {
    int collisionX = pongBallX + vx;
    int collisionY = pongBallY + vy;
    if (collisionX >= 1 && collisionX <= (int)maxX && collisionY >= 1 && collisionY <= maxY && pongNoteAt(collisionX, collisionY)) {
      triggerPongCollision(collisionX, collisionY);
      vx = -vx;
      vy = -vy;
      registerPongCollision(vx, vy);
    }
  }

  // Store old position in tail before moving
  pongTailX[pongTailIndex] = pongBallX;
  pongTailY[pongTailIndex] = pongBallY;
  pongTailIndex = (pongTailIndex + 1) % pongTailLength;

  pongBallX = constrain(pongBallX + vx, 1, (int)maxX);
  pongBallY = constrain(pongBallY + vy, 1, maxY);

  pongVelocityX = vx;
  pongVelocityY = vy;
}

FLASHMEM void drawPongBall() {
  // Draw colorful fading tail (oldest to newest)
  for (int i = 0; i < pongTailLength; i++) {
    int tailIdx = (pongTailIndex + i) % pongTailLength;
    int tx = pongTailX[tailIdx];
    int ty = pongTailY[tailIdx];
    
    // Skip invalid positions
    if (tx < 1 || ty < 1) continue;
    
    // Calculate fade (oldest = dimmest, newest = brightest)
    int brightness = (i + 1) * 255 / (pongTailLength + 1);
    
    // Create rainbow effect: cycle through hue based on position in tail
    uint8_t hue = (i * 255 / pongTailLength);
    CRGB color = CHSV(hue, 255, brightness);
    
    light(tx, ty, color);
  }
  
  // Draw bright white ball at current position
  light(pongBallX, pongBallY, CRGB(255, 255, 255));
}

FLASHMEM void drawRecordingBorder() {
  // Draw red border around entire screen
  CRGB redColor = CRGB(255, 0, 0);
  
  // Top and bottom borders
  for (unsigned int x = 1; x <= maxX; x++) {
    light(x, 1, redColor);
    light(x, maxY, redColor);
  }
  
  // Left and right borders
  for (unsigned int y = 2; y < maxY; y++) {
    light(1, y, redColor);
    light(maxX, y, redColor);
  }
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
  }
  
  // Update marquee animation for noteShift mode OR song mode
  extern bool songModeActive;
  extern Mode songMode;
  if (currentMode == &noteShift || songModeActive || currentMode == &songMode) {
    // Make marquee slower in song mode
    float speed = (currentMode == &noteShift) ? 1.0 : 0.2;  // Song mode 5x slower
    
    if (movingForward) {
      marqueePos = marqueePos + speed;
      if (marqueePos > maxX) {
        marqueePos = maxX;
        movingForward = false;
      }
    } else {
      marqueePos = marqueePos - speed;
      if (marqueePos < 1) {
        marqueePos = 1;
        movingForward = true;
      }
    }
  }
}


FLASHMEM void drawText(const char *text, int startX, int startY, CRGB color) {
  int xOffset = startX;

  for (int i = 0; text[i] != '\0'; i++) {
    drawChar(text[i], xOffset, startY, color);
    xOffset += alphabet[text[i] - 32][0] + 1;  // Advance by char width + 1 space
  }
}



FLASHMEM void drawChar(char c, int x, int y, CRGB color) {
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



FLASHMEM void drawNumber(float count, CRGB color, int topY) {
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



FLASHMEM void drawVelocity() {
  unsigned int vy = currentMode->pos[0];    // Velocity on encoder[0]
  unsigned int probStep = currentMode->pos[1];  // Probability on encoder[1]
  unsigned int cv = currentMode->pos[2];    // Channel volume on encoder[2]
  unsigned int condStep = currentMode->pos[3];  // Condition on encoder[3]
  
  FastLEDclear();

  // Draw velocity bar at x=2-3
  for (unsigned int x = 2; x <= 3; x++) {
    for (unsigned int y = 1; y < vy + 1; y++) {
      light(x, y, CRGB(y * y, 20 - y, 0));
    }
  }
  
  // Draw probability bars at x=5-8
  // Layout: y=1 empty, [2-3: 0%] [4: black] [5-6: 25%] [7: black] [8-9: 50%] [10: black] [11-12: 75%] [13: black] [14-15: 100%]
  // White border at y=1 and y=16, plus sides (x=5 and x=8)
  
  // Draw dark white border for probability
  CRGB darkWhite = CRGB(30, 30, 30);
  light(5, 1, darkWhite);   // Bottom left corner
  light(6, 1, darkWhite);   // Bottom
  light(7, 1, darkWhite);   // Bottom
  light(8, 1, darkWhite);  // Bottom right corner
  light(5, 16, darkWhite);  // Top left corner
  light(6, 16, darkWhite);  // Top
  light(7, 16, darkWhite);  // Top
  light(8, 16, darkWhite); // Top right corner
  
  for (unsigned int y = 2; y <= 15; y++) {
    light(5, y, darkWhite);  // Left border
    light(8, y, darkWhite); // Right border
  }
  
  // Draw only the active probability bar (not cumulative)
  for (unsigned int x = 6; x < 8; x++) {
    // Step 1: 0% - red (y=2-3)
    if (probStep == 1) {
      light(x, 2, CRGB(100, 0, 0));
      light(x, 3, CRGB(100, 0, 0));
    }
    // Black spacer at y=4
    
    // Step 2: 25% - orange (y=5-6)
    if (probStep == 2) {
      light(x, 5, CRGB(150, 50, 0));
      light(x, 6, CRGB(150, 50, 0));
    }
    // Black spacer at y=7
    
    // Step 3: 50% - yellow (y=8-9) - only 2 rows
    if (probStep == 3) {
      light(x, 8, CRGB(150, 150, 0));
      light(x, 9, CRGB(150, 150, 0));
    }
    // Black spacer at y=10
    
    // Step 4: 75% - turquoise (y=11-12)
    if (probStep == 4) {
      light(x, 11, CRGB(0, 150, 150));
      light(x, 12, CRGB(0, 150, 150));
    }
    // Black spacer at y=13
    
    // Step 5: 100% - green (y=14-15)
    if (probStep == 5) {
      light(x, 14, CRGB(0, 200, 0));
      light(x, 15, CRGB(0, 200, 0));
    }
  }

  // Draw channel volume at x=10-11
  for (unsigned int x = 10; x <= 11; x++) {
    for (unsigned int y = 1; y < cv + 1; y++) {
      light(x, y, CRGB(0, 20 - y, y * y));
    }
  }
  
  // Draw condition display at x=13-16 using drawText
  // Show condition as "1/1", "1/2", or "1/4" based on condStep
  
  // Map condStep to numerator, denominator and color
  // Positions: 1=1/1, 2=1/2, 3=1/4, 4=1/8, 5=1/16, 6=2/1, 7=4/1, 8=8/1, 9=16/1
  const char* numeratorText;
  const char* denominatorText;
  CRGB textColor;
  if (condStep == 1) {
    numeratorText = "1";
    denominatorText = "1";
    textColor = CRGB(0, 200, 0);  // Green for 1/1
  } else if (condStep == 2) {
    numeratorText = "1";
    denominatorText = "2";
    textColor = CRGB(0, 0, 255);  // Blue for 1/2
  } else if (condStep == 3) {
    numeratorText = "1";
    denominatorText = "4";
    textColor = CRGB(200, 0, 255);  // Violet for 1/4
  } else if (condStep == 4) {
    numeratorText = "1";
    denominatorText = "8";
    textColor = CRGB(255, 100, 0);  // Orange for 1/8
  } else if (condStep == 5) {
    numeratorText = "1";
    denominatorText = "X";
    textColor = CRGB(0, 200, 200);  // Turquoise for 1/16
  } else if (condStep == 6) {
    numeratorText = "2";
    denominatorText = "1";
    textColor = CRGB(0, 150, 255);  // Light blue for 2/1
  } else if (condStep == 7) {
    numeratorText = "4";
    denominatorText = "1";
    textColor = CRGB(150, 0, 255);  // Light violet for 4/1
  } else if (condStep == 8) {
    numeratorText = "8";
    denominatorText = "1";
    textColor = CRGB(255, 150, 0);  // Light orange for 8/1
  } else {  // condStep == 9
    numeratorText = "X";
    denominatorText = "1";
    textColor = CRGB(0, 255, 200);  // Light turquoise for 16/1
  }
  
  // Draw numerator at top: x=13, y=11
  drawText(numeratorText, 13, 11, textColor);
  
  // Draw diagonal slash "/" from bottom right to top left: x=12-16, y=8-11
  // Reuse darkWhite from probability border above
  // Diagonal line from (16, 8) to (12, 11) - bottom right to top left
  light(16, 10, darkWhite);
  light(15, 9, darkWhite);
  light(14, 8, darkWhite);
  light(13, 7, darkWhite);
  
  // Draw denominator at bottom: x=14, y=2
  drawText(denominatorText, 14, 2, textColor);
}

FLASHMEM void drawCtrlVolumeOverlay(int volume) {
  const int overlayWidth = 2;
  const int startX = 6;
  const int endX = min((int)maxX, startX + overlayWidth - 1);
  volume = constrain(volume, 0, 16);

  if (volume == 0) {
    // Show a dim red dot to indicate muted state
    for (int x = startX; x <= endX; ++x) {
      light(x, 1, CRGB(60, 0, 0));
    }
    return;
  }

  for (int x = startX; x <= endX; ++x) {
    for (int y = 1; y <= volume; ++y) {
      uint8_t hue = mapf(y, 1, 16, 0, 96);         // Red -> Green
      uint8_t value = mapf(y, 1, 16, 180, 255);    // Brighter towards the top
      CRGB color = CHSV(hue, 255, value);
      light(x, y, color);
    }
  }
}

FLASHMEM void drawSampleLoadOverlay() {
  FastLEDclear();

  const CRGB frameColor = CRGB(20, 20, 40);
  const CRGB textColor = CRGB(180, 220, 255);

  // Frame around top area
  for (int x = 1; x <= (int)maxX; ++x) {
    light(x, 3, frameColor);
    //light(x, 4, frameColor);
  }

  light(1, 4, frameColor);
  light(maxX, 4, frameColor);


  // Load bar on y = 2
  int safeChannel = constrain(GLOB.currentChannel, 1, NUM_CHANNELS - 1);
  CRGB barColor = col[safeChannel];
  for (int x = 2; x <= (int)maxX - 1; ++x) {
    light(x, 4, barColor);
    light(x, 5, barColor);
  }

  // Text on y = 10
  drawText("LOAD", 1, 10, textColor);
  FastLED.show();
}








void drawPages() {
  //GLOB.edit = 1;
  CRGB ledColor;
  extern int loopLength;

  // First, clear the entire top row across all matrices
  for (unsigned int x = 1; x <= maxX; x++) {
    light(x, maxY, CRGB(0, 0, 0));  // Clear to black
  }

  // Then draw page indicators at positions 1-16 (or up to maxPages)
  for (unsigned int p = 1; p <= maxPages && p <= maxX; p++) {

    // Check if page is outside loop range when loop mode is active
    bool outsideLoop = (loopLength > 0 && p > loopLength);
    
    if (outsideLoop) {
      // Pages outside loop range are dark red
      ledColor = CRGB(10, 0, 0);
    } else {
      // Normal page coloring logic
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
    }

    // Set the LED color at position p on top row
    light(p, maxY, ledColor);
  }

  // After the loop, update the maxValues for Page-select-knob
  //currentMode->maxValues[1] = lastPage + 1;

  // Show green bezier when song mode is active (similar to noteshift marquee) on y=1
  extern bool songModeActive;
  extern float marqueePos;
  extern bool movingForward;
  if (songModeActive) {
    light(round(marqueePos), 1, CRGB(0, 80, 0));  // Darker green moving marquee on y=1
  }
}

/************************************************
      DRAW SAMPLES
  *************************************************/
FLASHMEM void drawTriggers() {
  // why?
  //GLOB.edit = 1;
  for (unsigned int ix = 1; ix < maxX + 1; ix++) {
    for (unsigned int iy = 1; iy < maxY + 1; iy++) {
      int thisNote = note[((GLOB.edit - 1) * maxX) + ix][iy].channel;
      if (thisNote > 0) {

        if (!getMuteState(thisNote)) {
          //light(ix, iy, getCol(note[((GLOB.edit - 1) * maxX) + ix][iy].channel));
          if (GLOB.singleMode && thisNote == GLOB.currentChannel) {
            // Get probability and condition for this note
            uint8_t prob = note[((GLOB.edit - 1) * maxX) + ix][iy].probability;
            uint8_t cond = note[((GLOB.edit - 1) * maxX) + ix][iy].condition;
            if (cond == 0) cond = 1;  // Default to 1 if not set
            CRGB noteColor = UI_BRIGHT_WHITE;
            
            // Add blinking effect for notes with condition
            // 1/X conditions: 2, 4, 8, 16
            // X/1 conditions: 17, 18, 19, 20
            if (cond == 2) {
              // Blue blinking for condition 2 (1/2 - every 2nd loop)
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              if (blinkPhase == 0) {
                noteColor = CRGB(0, 0, 255);  // Blue phase
              } else {
                noteColor = UI_BRIGHT_WHITE;  // White phase
              }
            } else if (cond == 4) {
              // Violet blinking for condition 4 (1/4 - every 4th loop)
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              if (blinkPhase == 0) {
                noteColor = CRGB(200, 0, 255);  // Violet phase
              } else {
                noteColor = UI_BRIGHT_WHITE;  // White phase
              }
            } else if (cond == 8) {
              // Orange blinking for condition 8 (1/8 - every 8th loop)
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              if (blinkPhase == 0) {
                noteColor = CRGB(255, 100, 0);  // Orange phase
              } else {
                noteColor = UI_BRIGHT_WHITE;  // White phase
              }
            } else if (cond == 16) {
              // Turquoise blinking for condition 16 (1/16 - every 16th loop)
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              if (blinkPhase == 0) {
                noteColor = CRGB(0, 200, 200);  // Turquoise phase
              } else {
                noteColor = UI_BRIGHT_WHITE;  // White phase
              }
            } else if (cond == 17) {
              // Light blue blinking for condition 17 (2/1 - first 2 loops)
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              if (blinkPhase == 0) {
                noteColor = CRGB(0, 150, 255);  // Light blue phase
              } else {
                noteColor = UI_BRIGHT_WHITE;  // White phase
              }
            } else if (cond == 18) {
              // Light violet blinking for condition 18 (4/1 - first 4 loops)
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              if (blinkPhase == 0) {
                noteColor = CRGB(150, 0, 255);  // Light violet phase
              } else {
                noteColor = UI_BRIGHT_WHITE;  // White phase
              }
            } else if (cond == 19) {
              // Light orange blinking for condition 19 (8/1 - first 8 loops)
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              if (blinkPhase == 0) {
                noteColor = CRGB(255, 150, 0);  // Light orange phase
              } else {
                noteColor = UI_BRIGHT_WHITE;  // White phase
              }
            } else if (cond == 20) {
              // Light turquoise blinking for condition 20 (16/1 - first 16 loops)
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              if (blinkPhase == 0) {
                noteColor = CRGB(0, 255, 200);  // Light turquoise phase
              } else {
                noteColor = UI_BRIGHT_WHITE;  // White phase
              }
            } else if (prob < 100) {
              // Add blinking red/white effect for notes with probability < 100%
              uint8_t blinkPhase = (millis() / 300) % 2;  // Blink every 300ms
              
              if (blinkPhase == 0) {
                // Red phase - intensity based on probability (lower prob = more red)
                uint8_t redIntensity = mapf(prob, 0, 100, 255, 80);  // More red for lower probability
                noteColor = CRGB(redIntensity, 0, 0);
              } else {
                // White phase - dimmed based on probability
                uint8_t brightness = mapf(prob, 0, 100, 30, 255);  // 30-255 range
                noteColor.nscale8(brightness);
              }
            }
            
            light(ix, iy, noteColor);
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
 
  unsigned int timer = ((beatForUI - 1) % maxX + 1);

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

FLASHMEM void drawCountIn() {
  extern bool countInActive;
  extern int countInBeat;
  
  if (countInActive && countInBeat > 0 && countInBeat <= 4) {
    // Display count-in number (1,2,3,4) on the bar at y=1
    char countText[4];
    snprintf(countText, sizeof(countText), "%d", countInBeat);
    // Display centered on the bar - position based on maxX
    int textX = (maxX / 2) - 1;  // Center the count text
    drawText(countText, textX, 1, CRGB(0, 255, 255));  // Cyan color for count-in
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




FLASHMEM void drawVolume(unsigned int vol) {
  unsigned int maxXVolume = int(vol * 1.3) + 2;
  for (unsigned int x = 0; x <= maxXVolume; x++) {
    light(x + 1, 12, CRGB(vol * vol, 20 - vol, 0));
    light(x + 1, 13, CRGB(vol * vol, 20 - vol, 0));
  }
}

FLASHMEM void drawLineOutVolume(uint8_t level) {
  level = constrain(level, LINEOUT_LEVEL_MIN, LINEOUT_LEVEL_MAX);
  // Flip the mapping: higher level (31) = quieter = less bar, lower level (13) = louder = more bar
  unsigned int filled = ::map(level, LINEOUT_LEVEL_MIN, LINEOUT_LEVEL_MAX, (int)maxX, 0);
  // Ensure minimum bar width of 1
  if (filled == 0) filled = 1;
  for (unsigned int x = 1; x <= maxX; x++) {
    CRGB color = (x <= filled) ? CRGB(50, 0, 50) : CRGB(0, 0, 0);  // Pink color
    light(x, 3, color);
    light(x, 4, color);
  }
}

FLASHMEM void drawBrightness() {
  unsigned int maxBrightness = ((ledBrightness - 65) * (15 - 1)) / (255 - 65) + 3;
  for (unsigned int x = 0; x < maxBrightness; x++) {
    CRGB brightness = CRGB(16 * x, 16 * x, 16 * x);
    light(x, 15, brightness);
    light(x, 16, brightness);
  }
}



FLASHMEM void showIcons(IconType ico, CRGB colors) {
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
            
        case ICON_HOURGLASS:
            iconArray = icon_hourglass;
            size = sizeof(icon_hourglass) / sizeof(icon_hourglass[0]);
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
        case ICON_VOL:
            iconArray = icon_vol;
            size = sizeof(icon_vol) / sizeof(icon_vol[0]);
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
            // Move hourglass up by 2 pixels
            int yOffset = (ico == ICON_HOURGLASS) ? 2 : 0;
            light(iconArray[gx][0], maxY - iconArray[gx][1] + yOffset, colors);
        }
    }
}



void processPeaks() {
  static float interpolatedValues[32];  // Cache interpolated values (max possible width)
  static float lastSeek = -1;
  static float lastSeekEnd = -1;
  static int lastPeakIndex = -1;
  static bool hasCalculated = false;  // Track if we've calculated for current peak data
  
  // Recalculate if:
  // 1. Parameters changed
  // 2. We have peaks but haven't calculated yet (initial display or new sample)
  bool needsRecalc = (GLOB.seek != lastSeek) || 
                     (GLOB.seekEnd != lastSeekEnd) || 
                     (peakIndex != lastPeakIndex) ||
                     (peakIndex > 0 && !hasCalculated);
  
  if (needsRecalc) {
    lastSeek = GLOB.seek;
    lastSeekEnd = GLOB.seekEnd;
    lastPeakIndex = peakIndex;
  }
  
  // Reset calculation flag when no peaks (new sample loading)
  if (peakIndex == 0) {
    hasCalculated = false;
  }

  if (peakIndex > 0 && needsRecalc) {
    hasCalculated = true;  // Mark that we've calculated for current peaks
    // For each display column, map directly to peak array considering seek range
    for (int i = 0; i < maxX; i++) {
      // Map i (0 to maxX-1) to normalized position within seek range
      // This ensures x=1 gets the first peak in the visible range
      float normalizedPos = (float)i / (float)(maxX - 1);  // 0.0 to 1.0
      
      // Map normalized position to seek range
      float seekPosition = GLOB.seek + normalizedPos * (GLOB.seekEnd - GLOB.seek);
      seekPosition = constrain(seekPosition, 0.0f, 100.0f);
      
      // Map seek position (0-100%) to peak array index (0 to peakIndex-1)
      float peakPos = (seekPosition / 100.0f) * (peakIndex - 1);
      peakPos = constrain(peakPos, 0.0f, (float)(peakIndex - 1));
      
      int lowerIndex = floor(peakPos);
      int upperIndex = min(lowerIndex + 1, peakIndex - 1);
      
      // Interpolate between peaks
      if (peakIndex > 1 && lowerIndex != upperIndex) {
        float fraction = peakPos - lowerIndex;
        interpolatedValues[i] = peakValues[lowerIndex] * (1.0f - fraction) + peakValues[upperIndex] * fraction;
      } else {
        interpolatedValues[i] = peakValues[lowerIndex];
      }
    }
  } else if (needsRecalc) {
    // No peaks, default to zero
    for (int i = 0; i < maxX; i++) {
      interpolatedValues[i] = 0;
    }
  }

  // === NORMALIZATION: Calculate once and cache ===
  static float cachedGainFactor = 1.0f;
  
  if (needsRecalc) {
    // Find max peak value
    float maxPeak = 0.0f;
    for (int i = 0; i < maxX; i++) {
      if (interpolatedValues[i] > maxPeak) {
        maxPeak = interpolatedValues[i];
      }
    }
    
    // Calculate gain factor (with minimum threshold to avoid over-amplifying noise)
    if (maxPeak > 0.05f) {  // Only normalize if there's meaningful signal (> 5%)
      cachedGainFactor = 1.0f / maxPeak;  // Scale so max peak reaches 1.0
      cachedGainFactor = constrain(cachedGainFactor, 1.0f, 10.0f);  // Limit gain to 10x max
    } else {
      cachedGainFactor = 1.0f;
    }
  }
  
  float gainFactor = cachedGainFactor;

  // Draw baseline at y=3 across full width
  for (int x = 1; x <= maxX; x++) {
    light(x, 3, CRGB(0, 0, 50));  // Dim blue baseline
  }

  // Highlight seek region on baseline
  int seekStartX = mapf(GLOB.seek, 0, 100, 1, maxX);
  int seekEndX = mapf(GLOB.seekEnd, 0, 100, 1, maxX);
  seekStartX = constrain(seekStartX, 1, maxX);
  seekEndX = constrain(seekEndX, 1, maxX);

  if (seekEndX <= seekStartX) {
    seekEndX = min(maxX, seekStartX + 1);
  }

  for (int x = 1; x <= seekStartX; x++) {
    light(x, 3, CRGB(0, 80, 0));  // Green for start portion
  }

  for (int x = seekEndX; x <= maxX; x++) {
    light(x, 3, CRGB(80, 0, 0));  // Red for end portion
  }

  // Light up LEDs with normalized values - limit drawing to y=5..10
  for (int i = 0; i < maxX; i++) {
    int x = i + 1;  // Ensure x values go from 1 to maxX
    
    // Apply normalization gain
    float normalizedValue = interpolatedValues[i] * gainFactor;
    normalizedValue = constrain(normalizedValue, 0.0f, 1.0f);
    
    // Map to y range 5-10
    int yPeak = mapf(normalizedValue * 100, 0, 100, 5, 10);
    yPeak = constrain(yPeak, 5, 10);

    // Clear rows above the waveform area (except baseline at y=3)
    for (int y = 2; y < 5; y++) {
      if (y == 3) continue;
      light(x, y, CRGB::Black);
    }

    for (int y = 5; y <= yPeak; y++) {
      // Spectral color gradient (rainbow): Red → Orange → Yellow → Green → Cyan → Blue
      // Map y from 5-10 to hue 0-170 (red to cyan-blue range)
      float normalizedY = mapf(y, 5, 10, 0.0f, 1.0f);
      int hue = mapf(normalizedY, 0.0f, 1.0f, 0, 170);  // HSV hue: 0=red, 60=yellow, 120=green, 170=cyan
      
      // Convert HSV to RGB (hue, saturation=255, value=255)
      // Simplified HSV to RGB conversion
      int sector = hue / 60;
      int remainder = hue % 60;
      int p = 0;
      int q = (255 * (60 - remainder)) / 60;
      int t = (255 * remainder) / 60;
      
      CRGB color;
      switch(sector) {
        case 0: color = CRGB(255, t, p); break;      // Red → Yellow
        case 1: color = CRGB(q, 255, p); break;      // Yellow → Green
        case 2: color = CRGB(p, 255, t); break;      // Green → Cyan
        default: color = CRGB(p, q, 255); break;     // Cyan → Blue
      }
      
      // Reduce brightness to 40% for display
      color.r = (color.r * 40) / 100;
      color.g = (color.g * 40) / 100;
      color.b = (color.b * 40) / 100;

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
    FastLEDshow();
  }
}


// Draw small arrow for MIDI INT/EXT indicator
// direction: true = right arrow (INT/send), false = left arrow (EXT/receive)
// x, y: position to draw the arrow (tip position)
// blink: if true, dim the arrow (for clock-synced blinking in EXT mode)
FLASHMEM void drawMidiArrow(bool direction, int x, int y, bool blink = false) {
  CRGB color = direction ? CRGB(0, 255, 0) : CRGB(255, 0, 0); // Green for INT, Red for EXT
  
  // Dim the color if blinking (reduce brightness to 30% for visible blink effect)
  if (blink) {
    color.r = (color.r * 30) / 100;
    color.g = (color.g * 30) / 100;
    color.b = (color.b * 30) / 100;
  }
  
  // Ensure coordinates are within bounds
  if (x < 1 || x > (int)maxX || y < 1 || y > maxY) return;
  
  if (direction) {
    // Right arrow (>): 3 pixels forming a right-pointing arrow
    // Tip at (x+1, y), with tail points at (x, y-1) and (x, y+1)
    if (x + 1 <= (int)maxX) light(x + 1, y, color);      // Tip (rightmost point)
    if (y - 1 >= 1) light(x, y - 1, color);               // Top tail
    if (y + 1 <= maxY) light(x, y + 1, color);            // Bottom tail
  } else {
    // Left arrow (<): 3 pixels forming a left-pointing arrow
    // Tip at (x-1, y), with tail points at (x, y-1) and (x, y+1)
    if (x - 1 >= 1) light(x - 1, y, color);               // Tip (leftmost point)
    if (y - 1 >= 1) light(x, y - 1, color);               // Top tail
    if (y + 1 <= maxY) light(x, y + 1, color);            // Bottom tail
  }
}

// Draw BPM number starting at x=3 with reserved 3-digit space (right-aligned with leading spaces)
FLASHMEM void drawBPMWithReservedSpace(float bpm, CRGB color, int topY) {
  char buffer[16];
  
  // Format as integer with leading spaces to always use 3 characters
  // "123" for 3 digits, " 21" for 2 digits, "  2" for 1 digit
  int bpmInt = (int)round(bpm);
  sprintf(buffer, "%3d", bpmInt);  // %3d = right-aligned with leading spaces
  
  // Start at x=3 (left-aligned position, but number is right-aligned within 3-digit space)
  drawText(buffer, 2, topY, color);
}

void drawBPMScreen() {

  FastLEDclear();
  // Volume bars and indicators removed - volume controls now in VOL menu
  if (drawBaseColorMode) {
    drawBrightness();
  }
  
  // New indicator system: BPM: - | L[W] | - | L[H]
  // Only show indicators for brightness (encoder 1) and BPM (encoder 3)
  // Encoder 0 and 2 (volume controls) removed
  
  if (drawBaseColorMode) {
    drawIndicator('L', 'H', 4);  // Encoder 4: Large Bright Blue for brightness
  }
  
  // Draw MIDI INT/EXT arrow indicator using encoder[2] position
  extern Mode *currentMode;
  extern Mode volume_bpm;
  extern int clockMode;
  bool isInt = false;
  if (currentMode == &volume_bpm) {
    // Get clockMode from encoder[2] position (0=EXT, 1=INT)
    isInt = (currentMode->pos[2] == 1);
  } else {
    // Fallback: use clockMode directly if not in volume_bpm mode
    isInt = (clockMode == 1);
  }
  
  // Draw BPM number starting at x=3 (left-aligned with reserved space)
  // In EXT mode: BLUE if stable, RED if not yet stable
  // In INT mode: CYAN
  extern bool getBPMStable();
  if (MIDI_CLOCK_SEND) {
    // INT mode: always cyan
    drawBPMWithReservedSpace(SMP.bpm, UI_CYAN, 6);
  } else {
    // EXT mode: blue if stable, red if not yet stable
    bool isStable = getBPMStable();
    CRGB extColor = isStable ? CRGB(0, 0, 255) : CRGB(255, 0, 0); // Blue if stable, Red if not
    drawBPMWithReservedSpace(SMP.bpm, extColor, 6);
  }
  
  // Draw arrow at right edge (x=maxX-1, y=8)
  // In EXT mode: blink the arrow in sync with MIDI clock (every 24th note)
  bool shouldBlink = false;
  if (!isInt && !MIDI_CLOCK_SEND) {
    // EXT mode: get clock tick count for blinking
    extern uint16_t getMidiClockTicks();
    uint16_t clockTicks = getMidiClockTicks();
    // Blink every 12 clocks (~half a beat at 24 PPQN): dim for first 6 ticks, bright for next 6
    shouldBlink = ((clockTicks % 12) < 6);
  }
  drawMidiArrow(isInt, maxX - 1, 8, shouldBlink);
  
  // Add indicator to encoder[2]: L size, color matching arrow, encoder 3
  if (currentMode == &volume_bpm) {
    // Green for INT, Red for EXT
    char arrowColor = isInt ? 'G' : 'R';
    drawIndicator('L', arrowColor, 3);  // Encoder 3: Large indicator matching arrow color
    
    // Set encoder color to match
    CRGB indicatorColor = getIndicatorColor(arrowColor);
    Encoder[2].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  }
}

void drawKnobColorDefault(){
  for (int i = 0; i < NUM_ENCODERS; i++) {
      Encoder[i].writeRGBCode(currentMode->knobcolor[i]);
  }
}



/************************************************
      SONG MODE
  *************************************************/
void showSongMode() {
  extern uint8_t songArrangement[64];
  extern Mode songMode;
  extern bool songModeActive;
  
  // Get current position and selected pattern from encoders
  int songPosition = songMode.pos[3];  // Encoder 3: position 1-64
  int selectedPattern = songMode.pos[1];  // Encoder 1: pattern 1-16
  
  // Clear screen
  for (int x = 1; x <= maxX; x++) {
    for (int y = 1; y <= maxY; y++) {
      light(x, y, CRGB(0, 0, 0));
    }
  }
  
  // Show marquee on bottom row (y=1) if song mode is active
  extern float marqueePos;
  if (songModeActive) {
    light(round(marqueePos), 1, CRGB(0, 80, 0));  // Darker green moving marquee on bottom row
  }
  
  // Show indicators for song mode
  drawIndicator('L', 'M', 2);  // Encoder 1: Large Magenta (pattern select)
  drawIndicator('L', 'Y', 4);  // Encoder 4: Large Yellow (position select)
  
  // Show selected pattern number in rainbow color (left side)
  char patText[8];
  snprintf(patText, sizeof(patText), "%02d", selectedPattern);
  int hue1 = (selectedPattern * 16) % 255;
  drawText(patText, 1, 11, CHSV(hue1, 255, 255));
  
  // Show current position number (right side)
  char posText[8];
  snprintf(posText, sizeof(posText), "%02d", songPosition);
  drawText(posText, 10, 11, CRGB(0, 255, 255));
  
  // Show stored pattern at this position (if any)
  int storedPattern = songArrangement[songPosition - 1];
  if (storedPattern > 0) {
    char storedText[8];
    snprintf(storedText, sizeof(storedText), ">%02d", storedPattern);
    int hue2 = (storedPattern * 16) % 255;
    drawText(storedText, 1, 5, CHSV(hue2, 255, 255));
  }
  
  // Draw song arrangement overview in bottom rows
  // Show maxX positions at a time across the full display width
  extern unsigned int maxX;
  int startPos = ((songPosition - 1) / maxX) * maxX;  // Start of current block
  
  for (int i = 0; i < maxX && (startPos + i) < 64; i++) {
    int pos = startPos + i;
    int pattern = songArrangement[pos];
    
    int x = (i % maxX) + 1;
    int y = 3 - (i / maxX);  // Row 3 down to 1 (changed from 4 to make room for playback indicator)
    
    CRGB color;
    if (pos == (songPosition - 1)) {
      // Current position: bright yellow
      color = CRGB(200, 200, 0);
    } else if (pattern > 0) {
      // Has pattern assigned: dim color based on pattern number
      int hue = (pattern * 16) % 255;
      color = CHSV(hue, 255, 80);
    } else {
      // Empty: very dark
      color = CRGB(5, 5, 5);
    }
    
    light(x, y, color);
  }
  
  // Draw song mode active indicator on y=1 (darker green bezier/marquee)
  extern float marqueePos;
  extern bool movingForward;
  if (songModeActive) {
    light(round(marqueePos), 1, CRGB(0, 80, 0));  // Darker green moving marquee to show song mode is active
  }
  
  // Draw playback position indicator on y=2 (white dot) if song mode is active during playback
  extern int currentSongPosition;
  extern bool isNowPlaying;
  if (songModeActive && isNowPlaying && currentSongPosition >= 0) {
    // Check if current playback position is in the visible range
    if (currentSongPosition >= startPos && currentSongPosition < startPos + maxX) {
      int indicatorX = (currentSongPosition - startPos) + 1;
      light(indicatorX, 2, CRGB(255, 255, 255));  // White dot
    }
  }
}



