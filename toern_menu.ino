// Menu page system - completely independent from maxPages
#define MENU_PAGES_COUNT 16

// External variables
extern Mode *currentMode;

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
  {"REC", 4, true, "GAIN"},             // Recording Mode + Mic Gain
  {"BPM", 5, false, nullptr},           // BPM/Volume
  {"CLK", 6, false, nullptr},           // Clock Mode
  {"CHN", 7, false, nullptr},           // MIDI Voice Select
  {"TRN", 8, false, nullptr},           // MIDI Transport
  {"PMD", 9, false, nullptr},           // Pattern Mode
  {"FLW", 10, false, nullptr},          // Flow Mode
  {"OTR", 11, false, nullptr},          // Fast Rec Mode
  {"CLR", 12, false, nullptr},          // Rec Channel Clear
  {"PVL", 13, false, nullptr},          // Preview Volume
  {"MON", 14, true, "LEVEL"},           // Monitor Level + Level Control
  {"AUTO", 15, true, "PAGES"},            // AI Song Generation + Page Count
  {"RST", 16, false, nullptr}           // Reset
};

int currentMenuPage = 0;
int aiTargetPage = 6; // Default target page for AI song generation
int aiBaseStartPage = 1; // Start of base page range for AI analysis
int aiBaseEndPage = 1;   // End of base page range for AI analysis

// Genre generation variables
int genreType = 0; // 0=BLNK, 1=TECH, 2=HIPH, 3=DNB, 4=HOUS, 5=AMBT
int genreLength = 8; // Default length for genre generation

// NEW mode state management
bool newScreenFirstEnter = true;

void loadMenuFromEEPROM() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
    // first run! write magic + defaults
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
    EEPROM.put(0, (unsigned int)1);            // samplePackID default (1)
    EEPROM.write(EEPROM_DATA_START + 0,  1);   // recMode default
    EEPROM.write(EEPROM_DATA_START + 1,  1);   // clockMode default
    EEPROM.write(EEPROM_DATA_START + 2,  1);   // transportMode default
    EEPROM.write(EEPROM_DATA_START + 3,  1);   // patternMode default
    EEPROM.write(EEPROM_DATA_START + 4,  1);   // voiceSelect default
    EEPROM.write(EEPROM_DATA_START + 5,  1);   // fastRecMode default
    EEPROM.write(EEPROM_DATA_START + 6,  1);   // recChannelClear default
    EEPROM.write(EEPROM_DATA_START + 7,  0);   // previewVol default
    EEPROM.write(EEPROM_DATA_START + 8, -1);   // flowMode default (OFF)
    EEPROM.write(EEPROM_DATA_START + 9, 10);   // micGain default (10)
    EEPROM.write(EEPROM_DATA_START + 10, 0);   // monitorLevel default (0 = OFF)

    //Serial.println(F("EEPROM initialized with defaults."));
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
  
  // Safety: Ensure monitoring is OFF on startup to prevent feedback
  mixer_end.gain(3, 0.0);
  
  if (recChannelClear < 0 || recChannelClear > 2) recChannelClear = 1;  // Default to ON if invalid
  
  // Ensure flowMode is valid (-1 or 1)
  if (flowMode != -1 && flowMode != 1) {
    flowMode = -1;  // Default to OFF if invalid value
  }

  //Serial.println(F("Loaded Menu values from EEPROM:"));
  //Serial.print(F("  recMode="));       //Serial.println(recMode);
  //Serial.print(F("  clockMode="));     //Serial.println(clockMode);
  //Serial.print(F("  transportMode=")); //Serial.println(transportMode);
  //Serial.print(F("  patternMode="));   //Serial.println(patternMode);
  //Serial.print(F("  flowMode="));      //Serial.println(flowMode);
  //Serial.print(F("  voiceSelect="));   //Serial.println(voiceSelect);
  //Serial.print(F("  fastRecMode="));   //Serial.println(fastRecMode);
  //Serial.print(F("  recChannelClear=")); //Serial.println(recChannelClear);
  //Serial.print(F("  previewVol="));    //Serial.println(previewVol);

  // Set global flags
  SMP_PATTERN_MODE       = (patternMode   == 1);
  SMP_FLOW_MODE          = (flowMode      == 1);
  MIDI_VOICE_SELECT      = (voiceSelect   == 1);
  MIDI_TRANSPORT_RECEIVE = (transportMode == 1);
  SMP_FAST_REC           = fastRecMode;
  SMP_REC_CHANNEL_CLEAR  = (recChannelClear == 1);  // Only true for ON mode
  
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

void showMenu() {
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
  
  // Draw page indicator as a line at y=16
  // Show current page as red, others as blue
  // Shifted right by 1: page 0 = LED 1, page 1 = LED 2, etc.
  for (int i = 0; i < MENU_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? UI_RED : UI_BLUE;
    light(i + 1, 16, indicatorColor);
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
    Encoder[3].writeMax((int32_t)(MENU_PAGES_COUNT - 1));
    Encoder[3].writeMin((int32_t)0);
    menuFirstEnter = false;
  }
  
  if (currentMode->pos[3] != lastPagePosition) {
    currentMenuPage = currentMode->pos[3];
    if (currentMenuPage >= MENU_PAGES_COUNT) currentMenuPage = MENU_PAGES_COUNT - 1;
    if (currentMenuPage < 0) currentMenuPage = 0;
    lastPagePosition = currentMenuPage;
  }
  
  // Set the menu position to the current page's main setting
  if (currentMode->pos[3] != mainSetting) {
    changeMenu(mainSetting);
  }
  
  // Handle encoder 2 changes for pages with additional features
  handleAdditionalFeatureControls(mainSetting);
}

void drawMainSettingStatus(int setting) {
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
      drawText("MIDI", 2, 10, UI_WHITE);
      drawMidiVoiceSelect();
      break;
      
    case 8: // TRN - MIDI Transport
      drawText("TRSP", 2, 10, UI_WHITE);
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
      drawText("REC", 2, 10, UI_ORANGE);
      drawFastRecMode();
      break;
      
    case 12: // CLR - Rec Channel Clear
      drawText("CLR", 2, 10, UI_ORANGE);
      drawRecChannelClear();
      break;
      
    case 13: // PVL - Preview Volume
      drawText("PVOL", 2, 10, UI_ORANGE);
      drawPreviewVol();
      break;
      
    case 14: // MON - Monitor Level
      drawText("LIVE", 2, 10, CRGB(200, 0, 20));
      drawMonitorLevel();
      break;
      
    case 15: // AI - Song Generation
      drawText("AUTO", 2, 10, CRGB(255, 0, 255));
      // New indicator system: menu/AI: L[G] | L[Y] | L[W] | S[X]
      drawIndicator('L', 'G', 1);  // Encoder 1: Large Green
      drawIndicator('L', 'Y', 2);  // Encoder 2: Large Yellow
      drawIndicator('L', 'W', 3);  // Encoder 3: Large White
      drawIndicator('L', 'X', 4);  // Encoder 4: Large Blue
      break;
      
    case 16: // RST - Reset
      drawText("RSET", 2, 10, CRGB(255, 100, 0));
      drawText(VERSION, 2, 3, CRGB(0, 0, 10));
      break;
  }
}

void drawAdditionalFeatures(int setting) {
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
      
    case 14: { // MON page - Monitor Level
      //drawText("LEVEL:", 2, 12, UI_DIM_WHITE);
      char levelText[8];
      sprintf(levelText, "%d", monitorLevel);
      //drawText(levelText, 9, 12, UI_WHITE);
      
      // Draw level meter on y=15 (moved from y=16 to avoid conflict with page indicator)
      int levelLength = mapf(monitorLevel, 0, 4, 0, 16);
      for (int x = 1; x <= 16; x++) {
        if (x <= levelLength) {
          // Gradient from red -> green
          float blend = float(x - 1) / max(1, levelLength - 1);
          CRGB grad = CRGB(
            255 * (1.0 - blend) + 0 * blend,
            0 * (1.0 - blend) + 255 * blend,
            0 * (1.0 - blend) + 0 * blend
          );
          light(x, 15, grad);
        } else {
          light(x, 15, CRGB(0, 0, 0));
        }
      }
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
    
  }
}

void drawGenreSelection() {
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

void handleAdditionalFeatureControls(int setting) {
  static bool recMenuFirstEnter = true;
  static bool monMenuFirstEnter = true;
  static bool aiMenuFirstEnter = true;
  static bool menuFirstEnter = true;
  static int lastSetting = -1;
  
  // Reset first enter flags when switching to a different setting
  if (setting != lastSetting) {
    recMenuFirstEnter = true;
    monMenuFirstEnter = true;
    aiMenuFirstEnter = true;
    menuFirstEnter = true;
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
      
    case 15: { // AI page - Target Count (enc0), Base Start (enc1), Base End (enc2)
      static int lastAiTargetPage = -1;
      static int lastAiBaseStartPage = -1;
      static int lastAiBaseEndPage = -1;
      
      // Set encoder counters only on first entry
      if (aiMenuFirstEnter) {
        Encoder[0].writeCounter((int32_t)aiTargetPage);
        Encoder[0].writeMax((int32_t)16);
        Encoder[0].writeMin((int32_t)1);
        
        Encoder[1].writeCounter((int32_t)aiBaseStartPage);
        Encoder[1].writeMax((int32_t)16);
        Encoder[1].writeMin((int32_t)1);
        
        Encoder[2].writeCounter((int32_t)aiBaseEndPage);
        Encoder[2].writeMax((int32_t)16);
        Encoder[2].writeMin((int32_t)1);
        
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
    
      
         default:
       // Reset first enter flags when not on pages with additional features
       recMenuFirstEnter = true;
       monMenuFirstEnter = true;
       aiMenuFirstEnter = true;
       menuFirstEnter = true;
       break;
  }
}

void switchMenu(int menuPosition){
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
        voiceSelect = voiceSelect * (-1);
        saveSingleModeToEEPROM(4, voiceSelect);

        //Serial.println(voiceSelect);
        drawMainSettingStatus(menuPosition);
        break;

      case 8:
        transportMode = transportMode * (-1);
        saveSingleModeToEEPROM(2, transportMode);
        drawMainSettingStatus(menuPosition);
        break;
    
      case 9:
        patternMode = patternMode * (-1);
        saveSingleModeToEEPROM(3, patternMode);
        drawMainSettingStatus(menuPosition);
        
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
        if (recChannelClear > 2) recChannelClear = 0;  // Cycle: 0->1->2->0
        saveSingleModeToEEPROM(6, recChannelClear);
        drawMainSettingStatus(menuPosition);
        break;

        case 13:
        previewVol = previewVol + 1;                   
        if (previewVol>3) previewVol=0;
        saveSingleModeToEEPROM(7, previewVol);
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
        // Reset all filters, envelopes, drums and synths to default
        resetAllToDefaults();
        break;
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
void showNewFileScreen() {
  // Switch to new file mode
  switchMode(&newFileMode);
}

// Show NEW file mode screen
void showNewFileMode() {
  FastLEDclear();
  
  // Draw page title
  drawText("NEW", 6, 12, CRGB(0, 255, 255));
  
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
  return menuPages[currentMenuPage].mainSetting;
}

void drawRecChannelClear(){
  if (recChannelClear == 1) {
    drawText("ON", 2, 3, UI_GREEN);
    SMP_REC_CHANNEL_CLEAR = true;  // Clear mode
  } else if (recChannelClear == 0) {
    drawText("OFF",2, 3, UI_RED);
    SMP_REC_CHANNEL_CLEAR = false; // Add triggers mode
  } else if (recChannelClear == 2) {
    drawText("FIX", 2, 3, UI_YELLOW);
    SMP_REC_CHANNEL_CLEAR = false; // FIX mode - no manipulation
  }
}

void drawRecMode() {

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

void drawClockMode() {

  if (clockMode == 1) {
    drawText("INT", 2, 3, UI_GREEN);
    MIDI_CLOCK_SEND = true;
  }else{
    drawText("EXT", 2, 3, UI_YELLOW);
    MIDI_CLOCK_SEND = false;
  }

  FastLEDshow();
}


void drawMidiVoiceSelect() {

  if (voiceSelect == 1) {
    drawText("MIDI", 2, 3, UI_BLUE);
    MIDI_VOICE_SELECT = true;
  }else{
    drawText("YPOS", 2, 3, UI_GREEN);
     MIDI_VOICE_SELECT = false;
  }

  FastLEDshow();
}



void drawPreviewVol() {

  if (previewVol == 3) {
    drawText("SPLT", 2, 3, UI_BLUE);
    
    
    mixer_stereoL.gain(0, 1);
    mixer_stereoL.gain(1, 0);

    mixer_stereoR.gain(0, 0);
    mixer_stereoR.gain(1, 1);

    mixer0.gain(1, 0.4);  //PREV
  }

  if (previewVol == 2) {
    drawText("HIGH", 2, 3, UI_RED);
    
    
    mixer_stereoL.gain(0, 1);
    mixer_stereoL.gain(1, 1);

    mixer_stereoR.gain(0, 1);
    mixer_stereoR.gain(1, 1);

    mixer0.gain(1, 0.6);  //PREV
  }

  if (previewVol == 1) {
    drawText("MID", 2, 3, UI_ORANGE);
    
    
    mixer_stereoL.gain(0, 1);
    mixer_stereoL.gain(1, 1);

    mixer_stereoR.gain(0, 1);
    mixer_stereoR.gain(1, 1);

    mixer0.gain(1, 0.3);  //PREV
  }

  if (previewVol == 0) {
    drawText("LOW", 2, 3, UI_GREEN);
  
    
    mixer_stereoL.gain(0, 1);
    mixer_stereoL.gain(1, 1);

    mixer_stereoR.gain(0, 1);
    mixer_stereoL.gain(1, 1);

    mixer0.gain(1, 0.1);  //PREV
  }
  FastLEDshow();
  
}

void drawMonitorLevel() {
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

void drawFastRecMode() {

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


void drawPatternMode() {

  if (patternMode == 1) {
    drawText("ON", 2, 3, UI_GREEN);
    SMP_PATTERN_MODE = true;
  }

  if (patternMode == -1) {
    drawText("OFF", 2, 3, UI_RED);
    SMP_PATTERN_MODE = false;
  }

  FastLEDshow();
}

void drawFlowMode() {
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


void drawMidiTransport() {

  if (transportMode == 1) {
    drawText("ON", 2, 3, UI_GREEN);
    MIDI_TRANSPORT_RECEIVE = true;
  }

  if (transportMode == -1) {
    drawText("OFF", 2, 3, UI_RED);
    MIDI_TRANSPORT_RECEIVE = false;
  }

  FastLEDshow();
}
