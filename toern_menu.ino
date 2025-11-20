// Menu page system - completely independent from maxPages
#define MENU_PAGES_COUNT 10
#define LOOK_PAGES_COUNT 7
#define RECS_PAGES_COUNT 5
#define MIDI_PAGES_COUNT 3

// External variables
extern Mode *currentMode;
extern int ctrlMode;
void refreshCtrlEncoderConfig();
extern bool pong;
extern uint16_t pongUpdateInterval;
extern bool MIDI_TRANSPORT_SEND;
void updatePreviewVolume();

// Page definitions - each page contains one main setting + additional features
struct MenuPage {
  const char* name;
  int mainSetting;  // The main setting for this page (menuPosition from old system)
  bool hasAdditionalFeatures;  // Whether this page has extra controls (like mic gain)
  const char* additionalFeatureName;  // Name of additional feature if any
};

MenuPage menuPages[MENU_PAGES_COUNT] = {
  {"DAT", 1, false, nullptr},           // Load/Save
  {"KIT", 2, false, nullptr},           // Sample Pack
  {"WAV", 3, false, nullptr},           // Wave Selection
  {"BPM", 5, false, nullptr},           // BPM/Volume
  {"PLAY", 19, false, nullptr},         // PLAY submenu (FLW, VIEW, PMD, LOOP)
  {"RECS", 20, false, nullptr},         // RECS submenu (REC, OTR, CLR, PVL, MON)
  {"MIDI", 21, false, nullptr},         // MIDI submenu (CHN, TRANSP, CLCK)
  {"SONG", 22, false, nullptr},         // Song Mode - arrange patterns into songs
  {"AUTO", 15, true, "PAGES"},          // AI Song Generation + Page Count
  {"RST", 16, true, "MODE"}            // Reset Effects
};

// LOOK submenu pages
MenuPage lookPages[LOOK_PAGES_COUNT] = {
  {"FLW", 10, false, nullptr},          // Flow Mode
  {"VIEW", 17, false, nullptr},         // Simple Notes View
  {"PMD", 9, false, nullptr},           // Pattern Mode
  {"LOOP", 18, false, nullptr},         // Loop Length
  {"CTRL", 25, false, nullptr},         // Encoder control mode
  {"LEDS", 23, false, nullptr},         // LED Modules Count (1 or 2)
  {"PONG", 24, false, nullptr}          // Pong Toggle
};

static inline uint16_t pongSpeedToInterval(int speed) {
  speed = constrain(speed, 1, 99);
  return static_cast<uint16_t>(mapf(speed, 1, 99, 300, 20));
}

static inline int pongIntervalToSpeed(uint16_t interval) {
  interval = constrain(interval, 20, 300);
  return static_cast<int>(mapf(interval, 300, 20, 1, 99));
}

// RECS submenu pages
MenuPage recsPages[RECS_PAGES_COUNT] = {
  {"MIC", 4, true, "GAIN"},             // Recording Mode + Mic Gain
  {"TRIG", 11, false, nullptr},          // Fast Rec Mode (On-The-fly Recording)
  {"CLR", 12, false, nullptr},          // Rec Channel Clear
{"PVOL", 13, true, "LEVEL"},          // Preview Volume
  {"LIVE", 14, true, "LEVEL"}           // Monitor Level + Level Control
};

// MIDI submenu pages
MenuPage midiPages[MIDI_PAGES_COUNT] = {
  {"CH", 7, false, nullptr},            // MIDI Voice Select (Channel)
  {"TRAN", 8, false, nullptr},          // MIDI Transport
  {"CLCK", 6, false, nullptr}           // Clock Mode
};

int currentMenuPage = 0;
int currentLookPage = 0;
int currentRecsPage = 0;
int currentMidiPage = 0;
bool inLookSubmenu = false;
bool inRecsSubmenu = false;
bool inMidiSubmenu = false;
int aiTargetPage = 6; // Default target page for AI song generation
int aiBaseStartPage = 1; // Start of base page range for AI analysis
int aiBaseEndPage = 1;   // End of base page range for AI analysis

// Genre generation variables
int genreType = 0; // 0=BLNK, 1=TECH, 2=HIPH, 3=DNB, 4=HOUS, 5=AMBT
int genreLength = 8; // Default length for genre generation

// NEW mode state management
bool newScreenFirstEnter = true;

// Reset menu option: 0 = EFX reset, 1 = FULL reset
int resetMenuOption = 0;

void loadMenuFromEEPROM() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
    // first run! write magic + defaults
    Serial.println("First run detected - initializing EEPROM with defaults");
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
    EEPROM.put(0, (unsigned int)1);            // samplePackID default (1)
    EEPROM.write(EEPROM_DATA_START + 0,  1);   // recMode default
    EEPROM.write(EEPROM_DATA_START + 1,  1);   // clockMode default
    EEPROM.write(EEPROM_DATA_START + 2,  1);   // transportMode default
    EEPROM.write(EEPROM_DATA_START + 3,  1);   // patternMode default
    EEPROM.write(EEPROM_DATA_START + 4,  1);   // voiceSelect default
    EEPROM.write(EEPROM_DATA_START + 5,  1);   // fastRecMode default
    EEPROM.write(EEPROM_DATA_START + 6,  1);   // recChannelClear default
    EEPROM.write(EEPROM_DATA_START + 7,  2);   // previewVol default (0-5 range, middle = 2)
    EEPROM.write(EEPROM_DATA_START + 8, -1);   // flowMode default (OFF)
    EEPROM.write(EEPROM_DATA_START + 9, 10);   // micGain default (10)
    EEPROM.write(EEPROM_DATA_START + 10, 0);   // monitorLevel default (0 = OFF)
    EEPROM.write(EEPROM_DATA_START + 11, 1);   // simpleNotesView default (1 = EASY)
    EEPROM.write(EEPROM_DATA_START + 12, 0);   // loopLength default (0 = OFF)
    EEPROM.write(EEPROM_DATA_START + 13, 1);   // ledModules default (1)
    EEPROM.write(EEPROM_DATA_START + 14, 0);   // ctrlMode default (0 = PAGE)
    EEPROM.write(EEPROM_DATA_START + 15, 30);  // lineOutLevelSetting default (30)

    Serial.println("EEPROM initialized with defaults.");
  }

  // now pull them in
  recMode       = (int8_t) EEPROM.read(EEPROM_DATA_START + 0);
  clockMode     = (int8_t) EEPROM.read(EEPROM_DATA_START + 1);
  transportMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 2);
  patternMode   = (int8_t) EEPROM.read(EEPROM_DATA_START + 3);
  voiceSelect   = (int8_t) EEPROM.read(EEPROM_DATA_START + 4);
  fastRecMode   = (int8_t) EEPROM.read(EEPROM_DATA_START + 5);
  recChannelClear   = (int8_t) EEPROM.read(EEPROM_DATA_START + 6);
  previewVol   = (int8_t) EEPROM.read(EEPROM_DATA_START + 7);
  flowMode     = (int8_t) EEPROM.read(EEPROM_DATA_START + 8);
  micGain     = (int8_t) EEPROM.read(EEPROM_DATA_START + 9);
  monitorLevel = (int8_t) EEPROM.read(EEPROM_DATA_START + 10);
  simpleNotesView = (int) EEPROM.read(EEPROM_DATA_START + 11);
  loopLength = (int) EEPROM.read(EEPROM_DATA_START + 12);
  ledModules = (int) EEPROM.read(EEPROM_DATA_START + 13);
  ctrlMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 14);
  
  if (previewVol < 0 || previewVol > 5) previewVol = 2;

  // Safety: Ensure monitoring is OFF on startup to prevent feedback
  mixer_end.gain(3, 0.0);
  
  // Update maxX based on loaded ledModules value
  extern unsigned int maxX;
  maxX = MATRIX_WIDTH * ledModules;
  
  if (recChannelClear < 0 || recChannelClear > 3) recChannelClear = 1;  // Default to ON if invalid
  
  // Ensure flowMode is valid (-1 or 1)
  if (flowMode != -1 && flowMode != 1) {
    flowMode = -1;  // Default to OFF if invalid value
  }
  
  // Ensure simpleNotesView is valid (1-2)
  if (simpleNotesView < 1 || simpleNotesView > 2) {
    simpleNotesView = 1;  // Default to EASY if invalid value
  }
  
  if (transportMode != -1 && transportMode != 1 && transportMode != 2) {
    transportMode = -1;  // Default to OFF if invalid value
  }

  // Ensure loopLength is valid (0-8)
  if (loopLength < 0 || loopLength > 8) {
    loopLength = 0;  // Default to OFF if invalid value
  }
  
  // Ensure voiceSelect is valid (-1, 1, or 2)
  if (voiceSelect != -1 && voiceSelect != 1 && voiceSelect != 2) {
    voiceSelect = 1;  // Default to MIDI if invalid value
  }
  
  // Ensure patternMode is valid (-1, 1, or 2)
  if (patternMode != -1 && patternMode != 1 && patternMode != 2) {
    patternMode = -1;  // Default to OFF if invalid value
  }
  
  // Ensure ledModules is valid (1 or 2)
  if (ledModules < 1 || ledModules > 2) {
  if (ctrlMode != 0 && ctrlMode != 1) {
    ctrlMode = 0;
  }

    Serial.print("Invalid ledModules value: ");
    Serial.print(ledModules);
    Serial.println(" - defaulting to 1");
    ledModules = 1;  // Default to 1 if invalid value
    EEPROM.write(EEPROM_DATA_START + 13, 1);  // Save corrected value
  }

  Serial.println("Loaded Menu values from EEPROM:");
  Serial.print("  ledModules="); Serial.print(ledModules); Serial.print(" (maxX="); Serial.print(maxX); Serial.println(")");
  Serial.print("  samplePackID="); Serial.println(samplePackID);
  Serial.print("  recMode="); Serial.println(recMode);
  Serial.print("  clockMode="); Serial.println(clockMode);
  Serial.print("  patternMode="); Serial.println(patternMode);
  Serial.print("  loopLength="); Serial.println(loopLength);
  //Serial.print(F("  recChannelClear=")); //Serial.println(recChannelClear);
  //Serial.print(F("  previewVol="));    //Serial.println(previewVol);

  // Set global flags
  SMP_PATTERN_MODE       = (patternMode   == 1 || patternMode == 2);  // ON or SONG mode
  SMP_FLOW_MODE          = (flowMode      == 1);
  MIDI_VOICE_SELECT      = (voiceSelect   == 1 || voiceSelect == 2);  // MIDI or KEYS mode
  MIDI_TRANSPORT_RECEIVE = (transportMode == 1);
  MIDI_TRANSPORT_SEND = (transportMode == 2);
  SMP_FAST_REC           = fastRecMode;
  SMP_REC_CHANNEL_CLEAR  = (recChannelClear == 1);  // Only true for ON mode
  
  // Set song mode active flag
  extern bool songModeActive;
  songModeActive = (patternMode == 2);
  
  // Set audio input based on recMode
  recInput = (recMode == 1) ? AUDIO_INPUT_MIC : AUDIO_INPUT_LINEIN;
  
  // Set MIDI clock send based on clockMode
  MIDI_CLOCK_SEND = (clockMode == 1);
  
  // Apply mic gain if in mic mode
  if (recMode == 1) {
    sgtl5000_1.micGain(micGain);
  }

  drawRecMode();
  drawClockMode();
  drawMidiTransport();
  drawPatternMode();
  drawFlowMode();
  drawMidiVoiceSelect();
  drawFastRecMode();
  drawRecChannelClear();
  drawPreviewVol();
  drawMonitorLevel();
}

// call this after you change *any* one of the six modes in switchMenu():
void saveSingleModeToEEPROM(int index, int8_t value) {
  EEPROM.write(EEPROM_DATA_START + index, (uint8_t)value);
}

// Save samplepack 0 state to EEPROM (which voices are using sp0)
void saveSp0StateToEEPROM() {
  Serial.println("=== Saving SP0 State to EEPROM ===");
  // Use addresses 200-208 for sp0 state (8 voices = 8 bytes)
  for (int i = 1; i < maxFiles; i++) {
    uint8_t value = SMP.sp0Active[i] ? 1 : 0;
    EEPROM.write(200 + i, value);
    Serial.print("Voice ");
    Serial.print(i);
    Serial.print(": sp0Active = ");
    Serial.print(SMP.sp0Active[i] ? "TRUE" : "FALSE");
    Serial.print(", saving ");
    Serial.println(value);
  }
  Serial.println("=== SP0 State Saved ===");
}

// Load samplepack 0 state from EEPROM
void loadSp0StateFromEEPROM() {
  Serial.println("=== Loading SP0 State from EEPROM ===");
  for (int i = 1; i < maxFiles; i++) {
    uint8_t stored = EEPROM.read(200 + i);
    SMP.sp0Active[i] = (stored == 1);
    Serial.print("Voice ");
    Serial.print(i);
    Serial.print(": EEPROM value = ");
    Serial.print(stored);
    Serial.print(", sp0Active = ");
    Serial.println(SMP.sp0Active[i] ? "TRUE" : "FALSE");
  }
  Serial.println("=== SP0 State Loaded ===");
}

FLASHMEM void showMenu() {
  FastLEDclear();
  //showExit(0);

  // New indicator system: menu: | | | L[X]
  // Encoder 1: empty (no indicator)
  // Encoder 2: empty (no indicator)
  // Encoder 3: empty (no indicator)
  drawIndicator('L', 'G', 4);  // Encoder 4: Large Blue

  // Get current page info
  int pageIndex = currentMenuPage;
  MenuPage* currentPageInfo = &menuPages[pageIndex];
  
  // Draw page title at top
  //drawText(currentPageInfo->name, 6, 1, UI_WHITE);
  
  // Draw page indicator as a line at y=maxY
  // Show current page as red, others as blue
  // Shifted right by 1: page 0 = LED 1, page 1 = LED 2, etc.
  for (int i = 0; i < MENU_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? UI_RED : UI_BLUE;
    light(i + 1, maxY, indicatorColor);
  }

  // Handle the main setting for this page
  int mainSetting = currentPageInfo->mainSetting;
  
  // Set encoder colors to match indicators based on main setting
  if (mainSetting == 4 && recMode == 1) {
    // REC page in MIC mode: L[R] indicator for encoder 3
    CRGB indicatorColor = getIndicatorColor('R'); // Red
    Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
    Encoder[3].writeRGBCode(0x000000); // Black (no indicator)
  } else if (mainSetting == 15) {
    // AI page: multiple indicators - L[G], L[Y], L[W], L[X]
    CRGB greenColor = getIndicatorColor('G'); // Green for encoder 1
    CRGB yellowColor = getIndicatorColor('Y'); // Yellow for encoder 2  
    CRGB whiteColor = getIndicatorColor('W'); // White for encoder 3
    CRGB blueColor = getIndicatorColor('X'); // Blue for encoder 4
    
    Encoder[0].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
    Encoder[1].writeRGBCode(yellowColor.r << 16 | yellowColor.g << 8 | yellowColor.b);
    Encoder[2].writeRGBCode(whiteColor.r << 16 | whiteColor.g << 8 | whiteColor.b);
    Encoder[3].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
  } else {
    // Default: L[G] indicator for encoder 4 (green)
    CRGB indicatorColor = getIndicatorColor('G'); // Green
    Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  }

  // Draw the main setting status
  drawMainSettingStatus(mainSetting);
  
  // Draw additional features if this page has them
  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }

  FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  // Handle page navigation with encoder 3
  static int lastPagePosition = -1;
  static bool menuFirstEnter = true;
  
  if (menuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentMenuPage);
    menuFirstEnter = false;
  }
  
  // Always set encoder limits to ensure they match current MENU_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(MENU_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
  if (currentMode->pos[3] != lastPagePosition) {
    currentMenuPage = currentMode->pos[3];
    if (currentMenuPage >= MENU_PAGES_COUNT) currentMenuPage = MENU_PAGES_COUNT - 1;
    if (currentMenuPage < 0) currentMenuPage = 0;
    lastPagePosition = currentMenuPage;
  }
  
  // Handle encoder 2 changes for pages with additional features
  handleAdditionalFeatureControls(mainSetting);
}

FLASHMEM void showLookMenu() {
  FastLEDclear();

  // New indicator system: lookMenu: | | | L[G]
  drawIndicator('L', 'G', 4);  // Encoder 4: Large Green (different from main menu)

  // Get current page info
  int pageIndex = currentLookPage;
  MenuPage* currentPageInfo = &lookPages[pageIndex];
  
  // Draw page indicator as a line at y=maxY
  // Show current page as red, others as green (matching PLAY menu color)
  // Shifted right by 1: page 0 = LED 1, page 1 = LED 2, etc.
  for (int i = 0; i < LOOK_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? UI_RED : CRGB(0, 255, 0); // Green like PLAY
    light(i + 1, maxY, indicatorColor);
  }

  // Handle the main setting for this page
  int mainSetting = currentPageInfo->mainSetting;
  
  // Default: L[G] indicator for encoder 4 (green)
  CRGB indicatorColor = getIndicatorColor('G'); // Green
  Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);

  // Draw the main setting status
  drawMainSettingStatus(mainSetting);
  
  // Draw additional features if this page has them
  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }

  FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  // Handle page navigation with encoder 3
  static int lastLookPagePosition = -1;
  static bool lookMenuFirstEnter = true;
  
  if (lookMenuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentLookPage);
    lookMenuFirstEnter = false;
  }
  
  // Always set encoder limits to ensure they match current LOOK_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(LOOK_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
  if (currentMode->pos[3] != lastLookPagePosition) {
    currentLookPage = currentMode->pos[3];
    if (currentLookPage >= LOOK_PAGES_COUNT) currentLookPage = LOOK_PAGES_COUNT - 1;
    if (currentLookPage < 0) currentLookPage = 0;
    lastLookPagePosition = currentLookPage;
  }
  
  // Handle encoder 2 changes for pages with additional features
  handleAdditionalFeatureControls(mainSetting);
}

FLASHMEM void showRecsMenu() {
  FastLEDclear();

  // New indicator system: recsMenu: | | | L[O] (orange)
  drawIndicator('L', 'O', 4);  // Encoder 4: Large Orange

  // Get current page info
  int pageIndex = currentRecsPage;
  MenuPage* currentPageInfo = &recsPages[pageIndex];
  
  // Draw page indicator as a line at y=maxY
  // Show current page as red, others as magenta (matching RECS menu color)
  // Shifted right by 1: page 0 = LED 1, page 1 = LED 2, etc.
  for (int i = 0; i < RECS_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? UI_RED : UI_MAGENTA; // Magenta like RECS
    light(i + 1, maxY, indicatorColor);
  }

  // Handle the main setting for this page
  int mainSetting = currentPageInfo->mainSetting;
  
  // Set encoder colors based on main setting
  if (mainSetting == 4 && recMode == 1) {
    // REC page in MIC mode: L[R] indicator for encoder 3
    CRGB redColor = getIndicatorColor('R'); // Red
    CRGB orangeColor = getIndicatorColor('O'); // Orange
    Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(redColor.r << 16 | redColor.g << 8 | redColor.b);
    Encoder[3].writeRGBCode(orangeColor.r << 16 | orangeColor.g << 8 | orangeColor.b);
  } else {
    // Default: L[O] indicator for encoder 4 (orange)
    CRGB indicatorColor = getIndicatorColor('O'); // Orange
    Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  }

  // Draw the main setting status
  drawMainSettingStatus(mainSetting);
  
  // Draw additional features if this page has them
  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }

  FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  // Handle page navigation with encoder 3
  static int lastRecsPagePosition = -1;
  static bool recsMenuFirstEnter = true;
  
  if (recsMenuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentRecsPage);
    recsMenuFirstEnter = false;
  }
  
  // Always set encoder limits to ensure they match current RECS_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(RECS_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
  if (currentMode->pos[3] != lastRecsPagePosition) {
    currentRecsPage = currentMode->pos[3];
    if (currentRecsPage >= RECS_PAGES_COUNT) currentRecsPage = RECS_PAGES_COUNT - 1;
    if (currentRecsPage < 0) currentRecsPage = 0;
    lastRecsPagePosition = currentRecsPage;
  }
  
  // Handle encoder 2 changes for pages with additional features
  handleAdditionalFeatureControls(mainSetting);
}

FLASHMEM void showMidiMenu() {
  FastLEDclear();

  // New indicator system: midiMenu: | | | L[W] (white)
  drawIndicator('L', 'W', 4);  // Encoder 4: Large White

  // Get current page info
  int pageIndex = currentMidiPage;
  MenuPage* currentPageInfo = &midiPages[pageIndex];
  
  // Draw page indicator as a line at y=maxY
  // Show current page as red, others as white (matching MIDI menu color)
  // Shifted right by 1: page 0 = LED 1, page 1 = LED 2, etc.
  for (int i = 0; i < MIDI_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? UI_RED : UI_WHITE; // White like MIDI
    light(i + 1, maxY, indicatorColor);
  }

  // Handle the main setting for this page
  int mainSetting = currentPageInfo->mainSetting;
  
  // Default: L[W] indicator for encoder 4 (white)
  CRGB indicatorColor = getIndicatorColor('W'); // White
  Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);

  // Draw the main setting status
  drawMainSettingStatus(mainSetting);
  
  // Draw additional features if this page has them
  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }

  FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  // Handle page navigation with encoder 3
  static int lastMidiPagePosition = -1;
  static bool midiMenuFirstEnter = true;
  
  if (midiMenuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentMidiPage);
    midiMenuFirstEnter = false;
  }
  
  // Always set encoder limits to ensure they match current MIDI_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(MIDI_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
  if (currentMode->pos[3] != lastMidiPagePosition) {
    currentMidiPage = currentMode->pos[3];
    if (currentMidiPage >= MIDI_PAGES_COUNT) currentMidiPage = MIDI_PAGES_COUNT - 1;
    if (currentMidiPage < 0) currentMidiPage = 0;
    lastMidiPagePosition = currentMidiPage;
  }
  
  // Handle encoder 2 changes for pages with additional features
  handleAdditionalFeatureControls(mainSetting);
}

FLASHMEM void drawMainSettingStatus(int setting) {
  switch (setting) {
    case 1: // DAT - Load/Save
      showIcons(ICON_LOADSAVE, UI_DIM_GREEN);
      showIcons(ICON_LOADSAVE2, UI_DIM_WHITE);
      drawText("FILE", 2, 3, UI_GREEN);
      break;
      
    case 2: // KIT - Sample Pack
      showIcons(ICON_SAMPLEPACK, UI_DIM_YELLOW);
      drawText("PACK", 2, 3, UI_BLUE);
      break;
      
    case 3: // WAV - Wave Selection
      showIcons(ICON_SAMPLE, UI_DIM_MAGENTA);
      if (GLOB.currentChannel > 0 && GLOB.currentChannel < 9) {
        drawText("WAVE", 2, 3, UI_YELLOW);
      } else {
        drawText("(-)", 2, 3, UI_YELLOW);
      }
      break;
      
    case 4: // REC - Recording Mode (menu/mic)
      showIcons(ICON_REC, UI_DIM_RED);
      showIcons(ICON_REC2, UI_DIM_WHITE);
      // New indicator system: menu/mic: S[G] | | M[W] | L[X]
      // Only show L[R] if value is MIC (recMode == 1)
      if (recMode == 1) {
        drawIndicator('L', 'R', 3);  // Encoder 3: Large Red (only for mic page)
      }
      drawRecMode();
      break;
      
    case 5: // BPM - BPM/Volume
      showIcons(ICON_BPM, UI_DIM_GREEN);
      drawText("BPM", 2, 3, UI_MAGENTA);
      break;
      
    case 6: // CLK - Clock Mode
      drawText("CLCK", 2, 10, UI_WHITE);
      drawClockMode();
      break;
      
    case 7: // CHN - MIDI Voice Select
      drawText("CH", 2, 10, UI_WHITE);
      drawMidiVoiceSelect();
      break;
      
    case 8: // TRN - MIDI Transport
      drawText("TRAN", 2, 10, UI_WHITE);
      drawMidiTransport();
      break;
      
    case 9: // PMD - Pattern Mode
      drawText("PMODE", 2, 10, UI_CYAN);
      drawPatternMode();
      break;
      
    case 10: // FLW - Flow Mode
      drawText("FLOW", 2, 10, UI_CYAN);
      drawFlowMode();
      break;
      
    case 11: // OTR - Fast Rec Mode
      drawText("TRIG", 2, 10, UI_MAGENTA);
      drawFastRecMode();
      break;
      
    case 12: // CLR - Rec Channel Clear
      drawText("CLR", 2, 10, UI_MAGENTA);
      drawRecChannelClear();
      break;
      
    case 13: // PVL - Preview Volume
      drawText("PVOL", 2, 10, UI_MAGENTA);
      drawIndicator('L', 'N', 3);  // Encoder 3: Large Cyan
      drawPreviewVol();
      break;
      
    case 14: // MON - Monitor Level
      drawText("LIVE", 2, 10, UI_MAGENTA);
      drawMonitorLevel();
      break;
      
    case 15: // AI - Song Generation
      drawText("AUTO", 2, 10, CRGB(0, 100, 255));  // Blue
      // New indicator system: menu/AI: L[M] | L[Y] | L[W] | L[X]
      drawIndicator('L', 'M', 1);  // Encoder 1: Large Magenta (trigger)
      drawIndicator('L', 'Y', 2);  // Encoder 2: Large Yellow (base start)
      drawIndicator('L', 'W', 3);  // Encoder 3: Large White (base end)
      drawIndicator('L', 'X', 4);  // Encoder 4: Large Blue (exit)
      break;
      
    case 16: // RST - Reset
      drawText("RSET", 2, 10, CRGB(255, 0, 0));  // Red
      /*if (resetMenuOption == 0) {
        drawText("EFX", 2, 3, CRGB(100, 0, 0));  // Dark Red
      } else {
        drawText("FULL", 2, 3, CRGB(100, 0, 0));  // Dark Red
      }*/
      drawIndicator('L', 'O', 3);  // Encoder 3: Large Orange indicator
      
      break;
      
    case 19: // PLAY - Submenu
      drawText("PLAY", 2, 10, CRGB(0, 255, 0));
      drawText("MENU", 2, 3, CRGB(0, 10, 0));
      break;
      
    case 20: // RECS - Submenu
      drawText("REC", 2, 10, UI_MAGENTA); // Orange
      drawText("MENU", 2, 3, CRGB(15, 0, 8));    // Dark Orange
      break;
      
    case 21: // MIDI - Submenu
      drawText("MIDI", 2, 10, CRGB(255, 255, 255)); // White
      drawText("MENU", 2, 3, CRGB(10, 10, 10));  // Gray
      break;
      
    case 22: // SONG - Song Mode
      drawText("SONG", 2, 10, CRGB(255, 255, 0)); // Yellow
      drawText("MODE", 2, 3, CRGB(10, 10, 0));  // Dark Yellow
      break;
      
    case 17: // VIEW - Simple Notes View
      drawText("VIEW", 2, 10, CRGB(0, 255, 100));
      drawSimpleNotesView();
      break;
      
    case 18: // LOOP - Loop Length
      drawText("LOOP", 2, 10, CRGB(100, 200, 255));
      drawLoopLength();
      break;
      
    case 23: // LEDS - LED Modules Count
      drawText("LEDS", 2, 10, CRGB(0, 255, 0));
      drawLedModules();
      break;

    case 24: // PONG toggle
      drawText("PONG", 2, 10, CRGB(255, 255, 255));
      if (pong) {
        drawText("ON", 10, 3, UI_GREEN);
      } else {
        drawText("OFF", 6, 3, UI_RED);
      }
      if (pong) {
        char speedText[6];
        int speedValue = pongIntervalToSpeed(pongUpdateInterval);
        snprintf(speedText, sizeof(speedText), "%02d", speedValue);
        drawText(speedText, 2, 3, CRGB(0, 150, 255));
        {
          CRGB indicatorColor = getIndicatorColor('B');
          Encoder[2].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
        }
        drawIndicator('L', 'B', 3);
      } else {
        Encoder[2].writeRGBCode(0x000000);
      }
      break;

    case 25: { // CTRL - encoder behaviour
      drawText("CTRL", 2, 10, CRGB(0, 255, 100));
      if (ctrlMode == 0) {
        drawText("PAGE", 2, 3, UI_GREEN);
      } else {
        drawText("VOL", 2, 3, UI_ORANGE);
      }
      break;
    }
  }
}

FLASHMEM void drawAdditionalFeatures(int setting) {
  switch (setting) {
    case 4: { // REC page - Mic Gain
      //drawText("GAIN:", 2, 12, UI_DIM_WHITE);
      char gainText[8];
      sprintf(gainText, "%d", micGain);
      //drawText(gainText, 8, 12, UI_WHITE);
      
      // Mic gain meter is now drawn vertically in drawRecMode() on x=16
      // No horizontal meter needed here since page indicator uses y=16
      break;
    }
      
    
    case 15: { // AI page - Base Page Range + Additional Pages Count (all on one line)
      // Draw everything on y=8: Orange base range + Green additional pages count
      for (int x = 1; x <= 16; x++) {
        if (x >= aiBaseStartPage && x <= aiBaseEndPage) {
          light(x, 8, CRGB(255, 165, 0)); // Orange for base pages
        } else if (x > aiBaseEndPage && x <= aiBaseEndPage + aiTargetPage) {
          light(x, 8, CRGB(0, 255, 0)); // Green for additional pages to generate
        } else {
          light(x, 8, CRGB(0, 0, 0));
        }
      }
      
      // Draw "PAGE" text in pink at the bottom
      drawText("PAGE", 2, 3, CRGB(0, 0, 10)); // Pink color
      break;
    }

    case 16: { // RST page - show current reset mode
      const char* modeText = (resetMenuOption == 0) ? "EFX" : "FULL";
      for (int x = 1; x <= 16; x++) {
        light(x, 8, CRGB::Black);
      }
      drawText(modeText, 2, 3, UI_ORANGE);
      FastLEDshow();
      break;
    }
    
  }
}

FLASHMEM void drawGenreSelection() {
  // Draw genre options on y=8
  const char* genres[] = {"BLNK", "TECH", "HIPH", "DNB", "HOUS", "AMBT"};
  CRGB genreColors[] = {
    CRGB(100, 100, 100), // BLNK - gray
    CRGB(255, 100, 0),   // TECH - orange
    CRGB(255, 0, 255),   // HIPH - magenta
    CRGB(0, 255, 0),     // DNB - green
    CRGB(0, 100, 255),   // HOUS - blue
    CRGB(255, 255, 0)    // AMBT - yellow
  };
  
  // Draw current genre text
  drawText(genres[genreType], 2, 3, genreColors[genreType]);
}

FLASHMEM void handleAdditionalFeatureControls(int setting) {
  static bool recMenuFirstEnter = true;
  static bool monMenuFirstEnter = true;
  static bool aiMenuFirstEnter = true;
  static bool menuFirstEnter = true;
  static bool pongMenuFirstEnter = true;
  static bool previewMenuFirstEnter = true;
  static int lastPongSpeed = -1;
  static int lastSetting = -1;
  
  // Reset first enter flags when switching to a different setting
  if (setting != lastSetting) {
    recMenuFirstEnter = true;
    monMenuFirstEnter = true;
    aiMenuFirstEnter = true;
    menuFirstEnter = true;
    pongMenuFirstEnter = true;
    previewMenuFirstEnter = true;
    lastPongSpeed = -1;
    lastSetting = setting;
  }
  
  switch (setting) {
    case 4: // REC page - Mic Gain control
      static int lastMicGain = -1;
      
      // Set encoder counter only on first entry
      if (recMenuFirstEnter) {
        Encoder[2].writeCounter((int32_t)micGain);
        Encoder[2].writeMax((int32_t)64);
        Encoder[2].writeMin((int32_t)0);
        recMenuFirstEnter = false;
      }
      
      if (currentMode->pos[2] != lastMicGain) {
        micGain = currentMode->pos[2];
        saveSingleModeToEEPROM(9, micGain);
        sgtl5000_1.micGain(micGain);
        drawMainSettingStatus(setting);
        drawAdditionalFeatures(setting);
        lastMicGain = micGain;
      }
      break;
      
    case 13: { // PVOL page - Preview Volume control
      static int lastPreview = -1;

      if (previewMenuFirstEnter) {
        Encoder[2].writeCounter((int32_t)previewVol);
        Encoder[2].writeMax((int32_t)5);
        Encoder[2].writeMin((int32_t)0);
        previewMenuFirstEnter = false;
        lastPreview = previewVol;
      }

      if (currentMode->pos[2] != lastPreview) {
        previewVol = constrain(currentMode->pos[2], 0, 5);
        saveSingleModeToEEPROM(7, previewVol);
        updatePreviewVolume();
        drawMainSettingStatus(setting);
        drawAdditionalFeatures(setting);
        lastPreview = previewVol;
      }
      break;
    }

    case 14: // MON page - Monitor Level control
      static int lastMonitorLevel = -1;
      
      // Set encoder counter only on first entry
      if (monMenuFirstEnter) {
        Encoder[2].writeCounter((int32_t)monitorLevel);
        Encoder[2].writeMax((int32_t)4);
        Encoder[2].writeMin((int32_t)0);
        monMenuFirstEnter = false;
      }
      
      if (currentMode->pos[2] != lastMonitorLevel) {
        monitorLevel = currentMode->pos[2];
        if (monitorLevel > 4) monitorLevel = 4;
        if (monitorLevel < 0) monitorLevel = 0;
        saveSingleModeToEEPROM(10, monitorLevel);
        drawMainSettingStatus(setting);
        drawAdditionalFeatures(setting);
        lastMonitorLevel = monitorLevel;
      }
      break;
      
    case 15: { // AI page - Base Start (enc1), Base End (enc2), Target Count (enc0)
      static int lastAiTargetPage = -1;
      static int lastAiBaseStartPage = -1;
      static int lastAiBaseEndPage = -1;
      
      // Set encoder counters only on first entry
      if (aiMenuFirstEnter) {
        // Encoder 0 is now the trigger button, not used for values
        
        Encoder[1].writeCounter((int32_t)aiBaseStartPage);
        Encoder[1].writeMax((int32_t)16);
        Encoder[1].writeMin((int32_t)1);
        
        Encoder[2].writeCounter((int32_t)aiBaseEndPage);
        Encoder[2].writeMax((int32_t)16);
        Encoder[2].writeMin((int32_t)1);
        
        Encoder[0].writeCounter((int32_t)aiTargetPage);
        Encoder[0].writeMax((int32_t)16);
        Encoder[0].writeMin((int32_t)1);
        
        aiMenuFirstEnter = false;
      }
      
      // Handle target page count (encoder 0)
      if (currentMode->pos[0] != lastAiTargetPage) {
        aiTargetPage = currentMode->pos[0];
        if (aiTargetPage > 16) aiTargetPage = 16;
        if (aiTargetPage < 1) aiTargetPage = 1;
        drawAdditionalFeatures(setting);
        lastAiTargetPage = aiTargetPage;
      }
      
      // Handle base start page (encoder 1)
      if (currentMode->pos[1] != lastAiBaseStartPage) {
        aiBaseStartPage = currentMode->pos[1];
        if (aiBaseStartPage > 16) aiBaseStartPage = 16;
        if (aiBaseStartPage < 1) aiBaseStartPage = 1;
        
        // Ensure base start <= base end
        if (aiBaseStartPage > aiBaseEndPage) {
          aiBaseEndPage = aiBaseStartPage;
          Encoder[2].writeCounter((int32_t)aiBaseEndPage);
        }
        
        drawAdditionalFeatures(setting);
        lastAiBaseStartPage = aiBaseStartPage;
      }
      
      // Handle base end page (encoder 2)
      if (currentMode->pos[2] != lastAiBaseEndPage) {
        aiBaseEndPage = currentMode->pos[2];
        if (aiBaseEndPage > 16) aiBaseEndPage = 16;
        if (aiBaseEndPage < 1) aiBaseEndPage = 1;
        
        // Ensure base end >= base start
        if (aiBaseEndPage < aiBaseStartPage) {
          aiBaseStartPage = aiBaseEndPage;
          Encoder[1].writeCounter((int32_t)aiBaseStartPage);
        }
        
        drawAdditionalFeatures(setting);
        lastAiBaseEndPage = aiBaseEndPage;
      }
      break;
    }
    
    case 16: { // RST page - Choose between EFX and FULL reset
      static int lastResetOption = -1;

      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)resetMenuOption);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        menuFirstEnter = false;
      }

      if (currentMode->pos[2] != lastResetOption) {
        resetMenuOption = constrain(currentMode->pos[2], 0, 1);
        Encoder[2].writeCounter((int32_t)resetMenuOption);
        drawMainSettingStatus(setting);
        lastResetOption = resetMenuOption;
      }
      break;
    }

    case 24: { // PONG page - speed control on encoder 2
      if (!pong) {
        pongMenuFirstEnter = true;
        lastPongSpeed = -1;
        Encoder[2].writeRGBCode(0x000000);
        break;
      }

      const int pongSpeedMin = 1;
      const int pongSpeedMax = 99;

      if (pongMenuFirstEnter) {
        int currentSpeed = pongIntervalToSpeed(pongUpdateInterval);
        Encoder[2].writeCounter(static_cast<int32_t>(currentSpeed));
        Encoder[2].writeMax(static_cast<int32_t>(pongSpeedMax));
        Encoder[2].writeMin(static_cast<int32_t>(pongSpeedMin));
        currentMode->pos[2] = currentSpeed;
        lastPongSpeed = currentSpeed;
        pongMenuFirstEnter = false;
      }

      if (currentMode->pos[2] != lastPongSpeed) {
        int newSpeed = currentMode->pos[2];
        newSpeed = constrain(newSpeed, pongSpeedMin, pongSpeedMax);
        if (newSpeed != lastPongSpeed) {
          uint16_t newInterval = pongSpeedToInterval(newSpeed);
          if (newInterval != pongUpdateInterval) {
            pongUpdateInterval = newInterval;
          }
          drawMainSettingStatus(setting);
        }
        lastPongSpeed = newSpeed;
        // Keep encoder within bounds in case constrain clipped it
        Encoder[2].writeCounter(static_cast<int32_t>(newSpeed));
      }
      break;
    }

         default:
       // Reset first enter flags when not on pages with additional features
       recMenuFirstEnter = true;
       monMenuFirstEnter = true;
       aiMenuFirstEnter = true;
       menuFirstEnter = true;
       pongMenuFirstEnter = true;
       break;
  }
}

void switchMenu(int menuPosition){
   Serial.print("DEBUG: switchMenu called with menuPosition=");
   Serial.println(menuPosition);
   switch (menuPosition) {
      case 1:
        switchMode(&loadSaveTrack);
        break;

      case 2:
        switchMode(&set_SamplePack);
        break;

      case 3:
        if (GLOB.currentChannel < 1 || GLOB.currentChannel > 8) return;
        switchMode(&set_Wav);
        currentMode->pos[3] = SMP.wav[GLOB.currentChannel].oldID;
        SMP.wav[GLOB.currentChannel].fileID = SMP.wav[GLOB.currentChannel].oldID;
        //set encoder to currently Loaded Sample!!
        //Encoder[3].writeCounter((int32_t)((SMP.wav[GLOB.currentChannel][0] * 4) - 1));
        break;

      case 4:
        recMode = recMode * (-1);
        saveSingleModeToEEPROM(0, recMode);
        
        // Apply mic gain if switching to mic mode
        if (recMode == 1) {
          sgtl5000_1.micGain(micGain);
        }
        
        drawMainSettingStatus(menuPosition);
        drawAdditionalFeatures(menuPosition);
        break;

      case 5:
        switchMode(&volume_bpm);
        break;

      case 6:
        clockMode = clockMode * (-1);
        saveSingleModeToEEPROM(1, clockMode);

        //Serial.println(clockMode);
        drawMainSettingStatus(menuPosition);
        if (clockMode == 1) {
          playTimer.begin(playNote, playNoteInterval);
        } else {
          playTimer.end();
        }
        break;

      case 7:
        // Cycle through: -1 (YPOS) -> 1 (MIDI) -> 2 (KEYS) -> -1 (YPOS) ...
        if (voiceSelect == -1) {
          voiceSelect = 1;  // YPOS -> MIDI
        } else if (voiceSelect == 1) {
          voiceSelect = 2;  // MIDI -> KEYS
        } else {
          voiceSelect = -1; // KEYS -> YPOS
        }
        saveSingleModeToEEPROM(4, voiceSelect);

        //Serial.println(voiceSelect);
        drawMainSettingStatus(menuPosition);
        break;

      case 8:
        if (transportMode == -1) {
          transportMode = 1; // OFF -> GET
        } else if (transportMode == 1) {
          transportMode = 2; // GET -> SEND
        } else {
          transportMode = -1; // SEND -> OFF
        }
        saveSingleModeToEEPROM(2, transportMode);
        drawMainSettingStatus(menuPosition);
        break;
    
      case 9:
        // Cycle through: -1 (OFF) -> 1 (ON) -> 2 (SONG) -> -1 (OFF) ...
        if (patternMode == -1) {
          patternMode = 1;  // OFF -> ON
        } else if (patternMode == 1) {
          patternMode = 2;  // ON -> SONG
        } else {
          patternMode = -1; // SONG -> OFF
        }
        saveSingleModeToEEPROM(3, patternMode);
        drawMainSettingStatus(menuPosition);
        
        // Update songModeActive flag
        extern bool songModeActive;
        songModeActive = (patternMode == 2);
        Serial.print("PMOD changed - songModeActive: ");
        Serial.println(songModeActive ? "TRUE" : "FALSE");
        
        // Handle mute system when PMOD is toggled
        if (SMP_PATTERN_MODE) {
          // Switching TO PMOD mode - unmute all, then apply saved page mutes
          unmuteAllChannels();
          applyMutesAfterPMODSwitch();
        } else {
          // Switching FROM PMOD mode - unmute all, then apply global mutes
          unmuteAllChannels();
          applyMutesAfterPMODSwitch();
        }
        
        // Update encoder 1 limit when pattern mode is toggled
        if (currentMode == &draw || currentMode == &singleMode) {
          if (SMP_PATTERN_MODE) {
            // Pattern mode is ON - limit to lastPage
            updateLastPage();
            Encoder[1].writeMax((int32_t)lastPage);
          } else {
            // Pattern mode is OFF - allow up to maxPages
            Encoder[1].writeMax((int32_t)maxPages);
          }
        }
        break;

      case 10:
        flowMode = flowMode * (-1);
        saveSingleModeToEEPROM(8, flowMode);
        drawMainSettingStatus(menuPosition);
        
        // Reset lastFlowPage when FLOW mode is toggled
        lastFlowPage = 0;
        break;

      case 11:
        fastRecMode = fastRecMode + 1;
                            
        if (fastRecMode>3) fastRecMode=0;
        saveSingleModeToEEPROM(5, fastRecMode);
        drawMainSettingStatus(menuPosition);
        break;

         case 12:
        recChannelClear = recChannelClear + 1;
        if (recChannelClear > 3) recChannelClear = 0;  // Cycle: 0->1->2->3->0 (OFF->ON->FIX->ON1->OFF)
        saveSingleModeToEEPROM(6, recChannelClear);
        drawMainSettingStatus(menuPosition);
        break;

        case 13:
        // Preview volume is now adjusted via encoder 2 on PVOL page; button press has no action.
        drawMainSettingStatus(menuPosition);
        break;

        case 14:
        monitorLevel = monitorLevel + 1;                   
        if (monitorLevel>4) monitorLevel=0;
        saveSingleModeToEEPROM(10, monitorLevel);
        drawMainSettingStatus(menuPosition);
        drawAdditionalFeatures(menuPosition);
        break;

        case 15:
        // AI Song Generation - generate song from current page to target page
        generateSong();
        break;

        case 16:
        if (resetMenuOption == 0) {
        // Reset effects/parameters to defaults
        resetAllToDefaults();
        } else {
        // Full reset
        extern void startNew();
        startNew();
        }
        break;
        
        case 19:
        // Enter LOOK submenu at first page
        inLookSubmenu = true;
        currentLookPage = 0;
        Encoder[3].writeCounter((int32_t)0);
        break;
        
        case 20:
        // Enter RECS submenu at first page
        inRecsSubmenu = true;
        currentRecsPage = 0;
        Encoder[3].writeCounter((int32_t)0);
        break;
        
        case 21:
        // Enter MIDI submenu at first page
        inMidiSubmenu = true;
        currentMidiPage = 0;
        Encoder[3].writeCounter((int32_t)0);
        break;
        
        case 17:
        // Cycle through VIEW modes: 1=EASY, 2=FULL
        simpleNotesView = (simpleNotesView == 1) ? 2 : 1;
        saveSingleModeToEEPROM(11, simpleNotesView);
        drawMainSettingStatus(menuPosition);
        break;
        
        case 18:
        // Cycle through LOOP length: 0=OFF, 1-8
        loopLength = loopLength + 1;
        if (loopLength > 8) loopLength = 0;
        saveSingleModeToEEPROM(12, loopLength);
        drawMainSettingStatus(menuPosition);
        break;
        
        case 22: {
        // Enter SONG mode
        Serial.println("DEBUG: Case 22 (SONG) triggered - entering song mode");
        extern Mode songMode;
        switchMode(&songMode);
        break;
        }
        
        case 23: {
        // Toggle LED modules: 1 or 2
        Serial.println("DEBUG: Case 23 (LEDS) triggered - toggling LED modules");
        extern int ledModules;
        extern unsigned int maxX;
        extern Mode draw;
        extern Mode singleMode;
        ledModules = (ledModules == 1) ? 2 : 1;
        maxX = MATRIX_WIDTH * ledModules;  // Update maxX runtime variable
        
        // Update encoder[1] max for draw/single mode based on new maxX
        if (currentMode == &draw || currentMode == &singleMode) {
          int numModules = maxX / MATRIX_WIDTH;  // 1 or 2
          int adjustedMaxPages = maxPages / numModules;  // 16 or 8
          Encoder[1].writeMax((int32_t)adjustedMaxPages);
          
          // Clamp current page to new max if needed
          if (currentMode->pos[1] > adjustedMaxPages) {
            currentMode->pos[1] = adjustedMaxPages;
            Encoder[1].writeCounter((int32_t)adjustedMaxPages);
          }
        }
        
        saveSingleModeToEEPROM(13, ledModules);
        drawMainSettingStatus(menuPosition);
        Serial.print("LED Modules set to: ");
        Serial.print(ledModules);
        Serial.print(", maxX now: ");
        Serial.print(maxX);
        Serial.print(", max pages now: ");
        Serial.println(maxPages / (maxX / MATRIX_WIDTH));
        break;
        }

      case 24: {
        pong = !pong;
        drawMainSettingStatus(menuPosition);
        break;
      }

      case 25: {
        ctrlMode = ctrlMode ? 0 : 1;
        drawMainSettingStatus(menuPosition);
        refreshCtrlEncoderConfig();
        saveSingleModeToEEPROM(14, ctrlMode);
        break;
      }
    }
    //saveMenutoEEPROM();
}

// New functions for menu page navigation
void nextMenuPage() {
  currentMenuPage = (currentMenuPage + 1) % MENU_PAGES_COUNT;
}

void previousMenuPage() {
  currentMenuPage = (currentMenuPage - 1 + MENU_PAGES_COUNT) % MENU_PAGES_COUNT;
}

void goToMenuPage(int page) {
  if (page >= 0 && page < MENU_PAGES_COUNT) {
    currentMenuPage = page;
  }
}

// Function to reset menu state when leaving menu mode
void resetMenuState() {
  // This function can be called when exiting menu mode to reset any state
  // For now, we'll let the static variables handle the reset automatically
}

// Reset NEW mode state when exiting
void resetNewModeState() {
  newScreenFirstEnter = true;
}

// Show NEW screen when creating a new file via DAT

// Simple Notes View functions
void drawSimpleNotesView() {
  // Show current state: 1=EASY, 2=FULL
  switch (simpleNotesView) {
    case 1:
      drawText("EASY", 2, 3, CRGB(0, 255, 0));
      break;
    case 2:
      drawText("FULL", 2, 3, CRGB(0, 0, 255));
      break;
    default:
      // Default to EASY if invalid value
      simpleNotesView = 1;
      drawText("EASY", 2, 3, CRGB(0, 255, 0));
      break;
  }
}

void drawLoopLength() {
  // Show current state: 0=OFF, 1-8=forced length
  if (loopLength == 0) {
    drawText("OFF", 2, 3, CRGB(100, 100, 100));
  } else {
    drawNumber(loopLength, CRGB(0, 200, 255), 3);
  }
}

void drawLedModules() {
  // Show current LED modules count: 1 or 2
  extern int ledModules;
  drawNumber(ledModules, CRGB(0, 255, 0), 3);
}

void showNewFileScreen() {
  // Switch to new file mode
  switchMode(&newFileMode);
}

// Show NEW file mode screen
void showNewFileMode() {
  FastLEDclear();
  
  // Draw page title
  drawText("FULL", 6, 12, CRGB(0, 255, 255));
  
  // Draw genre selection
  drawGenreSelection();
  
  // Draw length meter on y=8 in pink (only for non-BLNK genres)
  if (genreType != 0) { // Not BLNK
    int lengthLength = mapf(genreLength, 1, 16, 1, 16);
    for (int x = 1; x <= 16; x++) {
      if (x <= lengthLength) {
        light(x, 10, CRGB(255, 0, 255)); // Pink for length
      } else {
        light(x, 10, CRGB(0, 0, 0));
      }
    }
  } else {
    // Clear the length meter area for BLNK
    for (int x = 1; x <= 16; x++) {
      light(x, 10, CRGB(0, 0, 0));
    }
  }
  
  // New indicator system: new: M[G] | | M[V] | S[X]
  drawIndicator('M', 'G', 1);  // Encoder 1: Medium Green
  // Encoder 2: empty (no indicator)
  
  // Only show L[V] if not BLNK (genreType != 0)
  if (genreType != 0) {
    drawIndicator('L', 'V', 3);  // Encoder 3: Large Violet (only for non-BLNK)
  }
  // Encoder 3: empty if BLNK
  
  drawIndicator('L', 'X', 4);  // Encoder 4: Large Blue (was missing!)

  // Set encoder colors to match indicators
  // Encoder 1: Medium Green (M[G])
  CRGB greenColor = getIndicatorColor('G'); // Green
  Encoder[0].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
  
  // Encoder 2: Black (no indicator)
  Encoder[1].writeRGBCode(0x000000); // Black
  
  // Encoder 3: Large Violet (L[V]) only if genreType != 0 (not BLNK)
  if (genreType != 0) {
    CRGB violetColor = getIndicatorColor('V'); // Violet
    Encoder[2].writeRGBCode(violetColor.r << 16 | violetColor.g << 8 | violetColor.b);
  } else {
    Encoder[2].writeRGBCode(0x000000); // Black when BLNK
  }
  
  // Encoder 4: Large Blue (L[X])
  CRGB blueColor = getIndicatorColor('X'); // Blue
  Encoder[3].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);

  
  FastLED.setBrightness(ledBrightness);
  FastLEDshow();
  
  // Initialize encoders for genre and length control
  static int lastGenreType = -1;
  static int lastGenreLength = -1;
  
  // Always initialize encoders when entering NEW mode
  if (newScreenFirstEnter) {
    // Genre selection by last encoder (encoder 3)
    Encoder[3].writeCounter((int32_t)genreType);
    Encoder[3].writeMax((int32_t)5);
    Encoder[3].writeMin((int32_t)0);
    
    if (genreType != 0) { // Only enable length control for non-BLNK genres
      // Length selection by 3rd encoder (encoder 2)
      Encoder[2].writeCounter((int32_t)genreLength);
      Encoder[2].writeMax((int32_t)16);
      Encoder[2].writeMin((int32_t)1);
    } else {
      // Disable encoder 2 for BLNK
      Encoder[2].writeCounter((int32_t)0);
      Encoder[2].writeMax((int32_t)0);
      Encoder[2].writeMin((int32_t)0);
    }
    
    // Update currentMode positions to match encoders
    currentMode->pos[3] = genreType;
    currentMode->pos[2] = genreType != 0 ? genreLength : 0;
    
    lastGenreType = genreType;
    lastGenreLength = genreLength;
    newScreenFirstEnter = false;
  }
  
  // Handle genre type (encoder 3 - last encoder)
  int currentGenreType = currentMode->pos[3];
  if (currentGenreType != lastGenreType) {
    genreType = currentGenreType;
    if (genreType > 5) genreType = 5;
    if (genreType < 0) genreType = 0;
    
    // Update encoder to match clamped value
    if (genreType != currentGenreType) {
      Encoder[3].writeCounter((int32_t)genreType);
      currentMode->pos[3] = genreType;
    }
    
    drawGenreSelection();
    
    // Update pink helper based on genre type
    if (genreType != 0) { // Not BLNK - show pink helper dot
      light(10, 1, CRGB(255, 0, 255)); // Pink helper dot
    } else { // BLNK - clear pink helper
      light(10, 1, CRGB(0, 0, 0)); // Clear pink helper
    }
    FastLEDshow(); // Update display to show pink helper change
    
    // Update encoder 2 based on genre type
    if (genreType != 0) { // Enable length control for non-BLNK genres
      Encoder[2].writeCounter((int32_t)genreLength);
      Encoder[2].writeMax((int32_t)16);
      Encoder[2].writeMin((int32_t)1);
      currentMode->pos[2] = genreLength;
    } else {
      // Disable encoder 2 for BLNK
      Encoder[2].writeCounter((int32_t)0);
      Encoder[2].writeMax((int32_t)0);
      Encoder[2].writeMin((int32_t)0);
      currentMode->pos[2] = 0;
    }
    
    lastGenreType = genreType;
  }
  
  // Handle genre length (encoder 2 - 3rd encoder) - only for non-BLNK genres
  if (genreType != 0) {
    int currentGenreLength = currentMode->pos[2];
    if (currentGenreLength != lastGenreLength) {
      genreLength = currentGenreLength;
      if (genreLength > 16) genreLength = 16;
      if (genreLength < 1) genreLength = 1;
      
      // Update encoder to match clamped value
      if (genreLength != currentGenreLength) {
        Encoder[2].writeCounter((int32_t)genreLength);
        currentMode->pos[2] = genreLength;
      }
      
      // Redraw length meter
      int lengthLength = mapf(genreLength, 1, 16, 1, 16);
      for (int x = 1; x <= 16; x++) {
        if (x <= lengthLength) {
          light(x, 10, CRGB(255, 0, 255)); // Pink for length
        } else {
          light(x, 10, CRGB(0, 0, 0));
        }
      }
      FastLEDshow();
      
      lastGenreLength = genreLength;
    }
  }
}

// Helper function to get main setting for current menu page
int getCurrentMenuMainSetting() {
  if (inLookSubmenu) {
    return lookPages[currentLookPage].mainSetting;
  }
  if (inRecsSubmenu) {
    return recsPages[currentRecsPage].mainSetting;
  }
  if (inMidiSubmenu) {
    return midiPages[currentMidiPage].mainSetting;
  }
  return menuPages[currentMenuPage].mainSetting;
}

FLASHMEM void drawRecChannelClear(){
  if (recChannelClear == 1) {
    drawText("ON", 2, 3, UI_GREEN);
    SMP_REC_CHANNEL_CLEAR = true;  // Clear mode
  } else if (recChannelClear == 0) {
    drawText("OFF",2, 3, UI_RED);
    SMP_REC_CHANNEL_CLEAR = false; // Add triggers mode
  } else if (recChannelClear == 2) {
    drawText("FIX", 2, 3, UI_YELLOW);
    SMP_REC_CHANNEL_CLEAR = false; // FIX mode - no manipulation
  } else if (recChannelClear == 3) {
    drawText("ON1", 2, 3, UI_CYAN);
    SMP_REC_CHANNEL_CLEAR = true;  // ON1 mode - count-in then record on beat 1
  }
}

FLASHMEM void drawRecMode() {

  if (recMode == 1) {
    drawText("MIC", 2, 3, UI_WHITE);
    
    // Draw mic gain meter vertically on x=16 - white to red gradient
    int activeLength = mapf(micGain, 0, 64, 0, 16);
    for (int y = 1; y <= 16; y++) {
      if (y <= activeLength) {
        // Gradient from white -> red
        float blend = float(y - 1) / max(1, activeLength - 1);  // Prevent div by zero
        CRGB grad = CRGB(
          255 * (1.0 - blend) + 255 * blend,  // Red component
          255 * (1.0 - blend) + 0 * blend,    // Green component  
          255 * (1.0 - blend) + 0 * blend     // Blue component
        );
        light(16, y, grad);
      } else {
        // Empty part stays black
        light(16, y, CRGB(0, 0, 0));
      }
    }
    
    recInput = AUDIO_INPUT_MIC;
  }
  if (recMode == -1) {
    drawText("LINE", 2, 3, UI_BLUE);
    // No gain level display for LINE input
    recInput = AUDIO_INPUT_LINEIN;
  }
  sgtl5000_1.inputSelect(recInput);


  FastLEDshow();
}

FLASHMEM void drawClockMode() {

  if (clockMode == 1) {
    drawText("INT", 2, 3, UI_GREEN);
    MIDI_CLOCK_SEND = true;
  }else{
    drawText("EXT", 2, 3, UI_YELLOW);
    MIDI_CLOCK_SEND = false;
  }

  FastLEDshow();
}


FLASHMEM void drawMidiVoiceSelect() {

  if (voiceSelect == 1) {
    drawText("MIDI", 2, 3, UI_BLUE);
    MIDI_VOICE_SELECT = true;
  } else if (voiceSelect == 2) {
    drawText("KEYS", 2, 3, UI_MAGENTA);
    MIDI_VOICE_SELECT = true;  // KEYS mode also uses MIDI channel info
  } else {
    drawText("YPOS", 2, 3, UI_GREEN);
     MIDI_VOICE_SELECT = false;
  }

  FastLEDshow();
}



FLASHMEM void drawPreviewVol() {
  int barLength = mapf(previewVol, 0, 5, 1, maxX);
  for (int x = 1; x <= (int)maxX; x++) {
    CRGB color = (x <= barLength) ? CRGB(0, 160, 80) : CRGB(0, 0, 0);
    light(x, 6, color);
    light(x, 7, color);
  }
  char label[8];
  //snprintf(label, sizeof(label), "PV %d", previewVol);  // Display 0-5
  //drawText(label, 2, 3, UI_CYAN);
  FastLEDshow();
}

FLASHMEM void drawMonitorLevel() {
  if (monitorLevel == 0) {
    drawText("OFF", 2, 3, UI_RED);
  } else if (monitorLevel == 1) {
    drawText("LOW", 2, 3, UI_GREEN);
  } else if (monitorLevel == 2) {
    drawText("MED", 2, 3, UI_YELLOW);
  } else if (monitorLevel == 3) {
    drawText("HIGH", 2, 3, UI_ORANGE);
  } else if (monitorLevel == 4) {
    drawText("FULL", 2, 3, UI_BLUE);
  }
  FastLEDshow();
}

FLASHMEM void drawFastRecMode() {

if (fastRecMode == 3) {
    drawText("+CON", 2, 3, UI_DIM_BLUE);
    SMP_FAST_REC = 3;
  }


  if (fastRecMode == 2) {
    drawText("-CON", 2, 3, UI_DIM_YELLOW);
    SMP_FAST_REC = 2;
   
  }


  if (fastRecMode == 1) {
    drawText("SENS", 2, 3, UI_GREEN);
    SMP_FAST_REC = 1;
   
  }

  if (fastRecMode == 0) {
    drawText("OFF", 2, 3, UI_RED);
    SMP_FAST_REC = 0;
  }
  FastLEDshow();
}


FLASHMEM void drawPatternMode() {

  if (patternMode == 2) {
    drawText("SONG", 2, 3, CRGB(255, 255, 0)); // Yellow for SONG mode
    SMP_PATTERN_MODE = true;
  } else if (patternMode == 1) {
    drawText("ON", 2, 3, UI_GREEN);
    SMP_PATTERN_MODE = true;
  } else {
    drawText("OFF", 2, 3, UI_RED);
    SMP_PATTERN_MODE = false;
  }

  FastLEDshow();
}

FLASHMEM void drawFlowMode() {
  if (flowMode == 1) {
    drawText("ON", 2, 3, UI_GREEN);  // Use same coordinates as drawRecChannelClear
    SMP_FLOW_MODE = true;
  } else if (flowMode == -1) {
    drawText("OFF", 2, 3, UI_RED);  // Use same coordinates as drawRecChannelClear
    SMP_FLOW_MODE = false;
  } else {
    // Fallback for any unexpected values
    drawText("OFF", 2, 3, UI_RED);  // Use same coordinates as drawRecChannelClear
    SMP_FLOW_MODE = false;
    flowMode = -1;  // Reset to valid value
  }

  FastLEDshow();
}


FLASHMEM void drawMidiTransport() {
  if (transportMode == 2) {
    drawText("SEND", 2, 3, UI_BLUE);
    MIDI_TRANSPORT_RECEIVE = false;
    MIDI_TRANSPORT_SEND = true;
  } else if (transportMode == 1) {
    drawText("GET", 2, 3, UI_GREEN);
    MIDI_TRANSPORT_RECEIVE = true;
    MIDI_TRANSPORT_SEND = false;
  } else {
    drawText("OFF", 2, 3, UI_RED);
    MIDI_TRANSPORT_RECEIVE = false;
    MIDI_TRANSPORT_SEND = false;
    transportMode = -1;
  }

  FastLEDshow();
}
