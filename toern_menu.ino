void loadMenuFromEEPROM() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
    // first run! write magic + defaults
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
    EEPROM.write(EEPROM_DATA_START + 0,  1);   // recMode default
    EEPROM.write(EEPROM_DATA_START + 1,  1);   // clockMode default
    EEPROM.write(EEPROM_DATA_START + 2,  1);   // transportMode default
    EEPROM.write(EEPROM_DATA_START + 3,  1);   // patternMode default
    EEPROM.write(EEPROM_DATA_START + 4,  1);   // voiceSelect default
    EEPROM.write(EEPROM_DATA_START + 5,  1);   // fastRecMode default
    EEPROM.write(EEPROM_DATA_START + 6,  1);   // recChannelClear default
    EEPROM.write(EEPROM_DATA_START + 7,  0);   // previewVol default

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
 if (recChannelClear>1 || recChannelClear< -1) recChannelClear=1;

  //Serial.println(F("Loaded Menu values from EEPROM:"));
  //Serial.print(F("  recMode="));       //Serial.println(recMode);
  //Serial.print(F("  clockMode="));     //Serial.println(clockMode);
  //Serial.print(F("  transportMode=")); //Serial.println(transportMode);
  //Serial.print(F("  patternMode="));   //Serial.println(patternMode);
  //Serial.print(F("  voiceSelect="));   //Serial.println(voiceSelect);
  //Serial.print(F("  fastRecMode="));   //Serial.println(fastRecMode);
  //Serial.print(F("  recChannelClear="));   //Serial.println(recChannelClear);
  //Serial.print(F("  previewVol="));   //Serial.println(previewVol);


  // re‐derive your dependent flags:
  recInput               = (recMode ==  1) ? AUDIO_INPUT_MIC  : AUDIO_INPUT_LINEIN;
  MIDI_CLOCK_SEND        = (clockMode   == 1);
  MIDI_TRANSPORT_RECEIVE = (transportMode == 1);
  SMP_PATTERN_MODE       = (patternMode   == 1);
  MIDI_VOICE_SELECT      = (voiceSelect   == 1);
  SMP_FAST_REC           = fastRecMode;
  SMP_REC_CHANNEL_CLEAR  = (recChannelClear == 1);
/*
  // update the display so it reflects the loaded state immediately:
  drawRecMode();
  drawClockMode();
  drawMidiTransport();
  drawPatternMode();
  drawMidiVoiceSelect();
  drawFastRecMode();
  drawRecChannelClear();
  drawPreviewVol();
  */

   
}

// call this after you change *any* one of the six modes in switchMenu():
void saveSingleModeToEEPROM(int index, int8_t value) {
  EEPROM.write(EEPROM_DATA_START + index, (uint8_t)value);
}


void showMenu() {
  FastLEDclear();
  showExit(0);

  //drawNumber(menuPosition, CRGB(20, 20, 40), 0);


  switch (menuPosition) {
    case 1:
      showIcons(ICON_LOADSAVE, CRGB(0, 20, 0));
      showIcons(ICON_LOADSAVE2, CRGB(20, 20, 20));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(0, 200, 0));
      Encoder[3].writeRGBCode(0x00FF00);
      break;
    case 2:
      showIcons(ICON_SAMPLEPACK, CRGB(0, 0, 20));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(0, 0, 200));
      Encoder[3].writeRGBCode(0x0000FF);
      break;

    case 3:
      showIcons(ICON_SAMPLE, CRGB(20, 0, 20));
      if (SMP.currentChannel > 0 && SMP.currentChannel < 9) drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(200, 200, 0));
      if (SMP.currentChannel < 1 || SMP.currentChannel > 8) drawText("(-)", 6, menuPosition, CRGB(200, 200, 0));
      Encoder[3].writeRGBCode(0xFFFF00);
      break;

    case 4:
      showIcons(ICON_REC, CRGB(20, 0, 0));
      showIcons(ICON_REC2, CRGB(20, 20, 20));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(200, 0, 0));
      Encoder[3].writeRGBCode(0xFF0000);
      drawRecMode();
      break;

    case 5:
      showIcons(ICON_BPM, CRGB(0, 50, 0));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(200, 0, 200));
      Encoder[3].writeRGBCode(0xFF00FF);
      break;

    case 6:
      //showIcons(ICON_SETTINGS, CRGB(50, 50, 50));
      drawText(menuText[menuPosition - 1], 2, menuPosition, CRGB(200, 200, 200));
      Encoder[3].writeRGBCode(0xFFFFFF);
      drawClockMode();
      break;

    case 7:
     // showIcons(ICON_SETTINGS, CRGB(50, 50, 50));
      drawText(menuText[menuPosition - 1], 2, menuPosition, CRGB(200, 200, 200));
      Encoder[3].writeRGBCode(0xFFFFFF);
      drawMidiVoiceSelect();
      break;

    case 8:
     // showIcons(ICON_SETTINGS, CRGB(50, 50, 50));
      drawText(menuText[menuPosition - 1], 2, menuPosition, CRGB(200, 200, 200));
      Encoder[3].writeRGBCode(0xFFFFFF);
      drawMidiTransport();
      break;

      case 9:
      //showIcons(ICON_SETTINGS, CRGB(50, 50, 50));
      drawText(menuText[menuPosition - 1], 2, menuPosition, CRGB(0, 200, 200));
      Encoder[3].writeRGBCode(0x00FFFF);
      drawPatternMode();
      break;

      case 10:
      //showIcons(ICON_SETTINGS, CRGB(50, 50, 50));
      drawText(menuText[menuPosition - 1], 2, menuPosition, CRGB(200, 0, 20));
      Encoder[3].writeRGBCode(0xFF00AA);
      drawFastRecMode();
      break;

      case 11:
      //showIcons(ICON_SETTINGS, CRGB(50, 50, 50));
      drawText(menuText[menuPosition - 1], 2, menuPosition, CRGB(200, 0, 20));
      Encoder[3].writeRGBCode(0x0000FF);
      drawRecChannelClear();
      break;

       case 12:
      //showIcons(ICON_SETTINGS, CRGB(50, 50, 50));
      drawText(menuText[menuPosition - 1], 2, menuPosition, CRGB(200, 0, 20));
      Encoder[3].writeRGBCode(0xAAFFAA);
      drawPreviewVol();
      break;


    default:
      break;
  }



  FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  if (currentMode->pos[3] != menuPosition) {
    changeMenu(currentMode->pos[3]);
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
        if (SMP.currentChannel < 1 || SMP.currentChannel > 8) return;
        switchMode(&set_Wav);
        currentMode->pos[3] = SMP.wav[SMP.currentChannel].oldID;
        SMP.wav[SMP.currentChannel].fileID = SMP.wav[SMP.currentChannel].oldID;
        //set encoder to currently Loaded Sample!!
        //Encoder[3].writeCounter((int32_t)((SMP.wav[SMP.currentChannel][0] * 4) - 1));
        break;

      case 4:
        recMode = recMode * (-1);
        saveSingleModeToEEPROM(0, recMode);
        drawRecMode();

        break;

      case 5:
        switchMode(&volume_bpm);
        break;

      case 6:
        clockMode = clockMode * (-1);
        saveSingleModeToEEPROM(1, clockMode);

        //Serial.println(clockMode);
        drawClockMode();
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
        drawMidiVoiceSelect();
        break;

      case 8:
        transportMode = transportMode * (-1);
        saveSingleModeToEEPROM(2, transportMode);
        drawMidiTransport();
        break;
    
      case 9:
        patternMode = patternMode * (-1);
        saveSingleModeToEEPROM(3, patternMode);
        drawPatternMode();
        break;

      case 10:
        fastRecMode = fastRecMode + 1;
                            
        if (fastRecMode>3) fastRecMode=0;
        saveSingleModeToEEPROM(5, fastRecMode);
        drawFastRecMode();
        break;

         case 11:
        recChannelClear = recChannelClear * (-1);
        saveSingleModeToEEPROM(6, recChannelClear);
        drawRecChannelClear();
        break;

        case 12:
        previewVol = previewVol + 1;                   
        if (previewVol>3) previewVol=0;
        saveSingleModeToEEPROM(7, previewVol);
        drawPreviewVol();
        break;
    }
    //saveMenutoEEPROM();
}

void drawRecChannelClear(){
  if (recChannelClear == 1) {
    drawText("ON", 5, 5, CRGB(0, 200, 0));
    SMP_REC_CHANNEL_CLEAR = true;
  } else {
    drawText("OFF", 5, 5, CRGB(200, 0, 0));
    SMP_REC_CHANNEL_CLEAR = false;
  }
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

void drawClockMode() {

  if (clockMode == 1) {
    drawText("INT", 5, 1, CRGB(0, 200, 0));
    MIDI_CLOCK_SEND = true;
  }else{
    drawText("EXT", 5, 1, CRGB(200, 200, 0));
    MIDI_CLOCK_SEND = false;
  }

  FastLEDshow();
}


void drawMidiVoiceSelect() {

  if (voiceSelect == 1) {
    drawText("MID", 5, 1, CRGB(200, 0, 200));
    MIDI_VOICE_SELECT = true;
  }else{
    drawText("MAN", 5, 1, CRGB(0, 0, 200));
     MIDI_VOICE_SELECT = false;
  }

  FastLEDshow();
}



void drawPreviewVol() {

if (previewVol == 3) {
    drawText("SPLT", 2, 6, CRGB(0, 0, 50));
    previewVol = 3;
    mixer_stereoL.gain(0, 1);
    mixer_stereoL.gain(1, 0);

    mixer_stereoR.gain(0, 0);
    mixer_stereoR.gain(1, 1);
    
    mixer0.gain(1, 0.4);  //PREV
  }


  if (previewVol == 2) {
    drawText("HIGH", 2, 6, CRGB(50, 50, 0));
    previewVol = 2;
    mixer_stereoL.gain(0, 1);
    mixer_stereoL.gain(1, 1);

    mixer_stereoR.gain(0, 1);
    mixer_stereoR.gain(1, 1);

       mixer0.gain(1, 0.6);  //PREV

  }


  if (previewVol == 1) {
    drawText("MID", 2, 6, CRGB(50, 50, 0));
    previewVol = 1;

    mixer_stereoL.gain(0, 1);
    mixer_stereoL.gain(1, 1);

    mixer_stereoR.gain(0, 1);
    mixer_stereoR.gain(1, 1);

    mixer0.gain(1, 0.4);  //PREV
  }

  if (previewVol == 0) {
    drawText("LOW", 2, 6, CRGB(200, 0, 0));
    previewVol = 0;
    
    mixer_stereoL.gain(0, 1);
    mixer_stereoL.gain(1, 1);

    mixer_stereoR.gain(0, 1);
    mixer_stereoR.gain(1, 1);

    mixer0.gain(1, 0.1);  //PREV
  }
  FastLEDshow();
  
}

void drawFastRecMode() {

if (fastRecMode == 3) {
    drawText("+CON", 2, 4, CRGB(0, 0, 50));
    SMP_FAST_REC = 3;
  }


  if (fastRecMode == 2) {
    drawText("-CON", 2, 4, CRGB(50, 50, 0));
    SMP_FAST_REC = 2;
   
  }


  if (fastRecMode == 1) {
    drawText("SENS", 2, 4, CRGB(50, 50, 0));
    SMP_FAST_REC = 1;
   
  }

  if (fastRecMode == 0) {
    drawText("OFF", 2, 4, CRGB(200, 0, 0));
    SMP_FAST_REC = 0;
  }
  FastLEDshow();
}


void drawPatternMode() {

  if (patternMode == 1) {
    drawText("ON", 5, 3, CRGB(0, 200, 0));
    SMP_PATTERN_MODE = true;
  }

  if (patternMode == -1) {
    drawText("OFF", 5, 3, CRGB(200, 0, 0));
    SMP_PATTERN_MODE = false;
  }

  FastLEDshow();
}


void drawMidiTransport() {

  if (transportMode == 1) {
    drawText("ON", 5, 2, CRGB(0, 200, 0));
    MIDI_TRANSPORT_RECEIVE = true;
  }

  if (transportMode == -1) {
    drawText("OFF", 5, 2, CRGB(200, 0, 0));
    MIDI_TRANSPORT_RECEIVE = false;
  }

  FastLEDshow();
}
