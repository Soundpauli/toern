

void savePattern(bool autosave) {
  
  drawNoSD();
  FastLEDclear();
  unsigned int maxdata = 0;
  char OUTPUTf[50];
  // Save to autosave.txt if autosave is true
  if (autosave) {
    sprintf(OUTPUTf, "autosaved.txt");
  } else {
    sprintf(OUTPUTf, "%d.txt", SMP.file);
  }
  if (SD.exists(OUTPUTf)) {
    SD.remove(OUTPUTf);
  }
  File saveFile = SD.open(OUTPUTf, FILE_WRITE);
  if (saveFile) {
    // Save notes
    for (unsigned int sdx = 1; sdx < maxlen; sdx++) {
      for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
        maxdata = maxdata + note[sdx][sdy].channel;
        saveFile.write(note[sdx][sdy].channel);
        saveFile.write(note[sdx][sdy].velocity);
      }
    }
    // Use a unique marker to indicate the end of notes and start of SMP data
    saveFile.write(0xFF);  // Marker byte to indicate end of notes
    saveFile.write(0xFE);  // Marker byte to indicate start of SMP

    // Save SMP struct (file/pattern specific data)
    saveFile.write((uint8_t *)&SMP, sizeof(SMP));
  }
  saveFile.close();
  if (maxdata == 0) {
    SD.remove(OUTPUTf);
  }
  if (!autosave) {
    delay(500);
    switchMode(&draw);
  }
  
  // Reset paint/unpaint prevention flag after savePattern operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
  
  yield();
}


void saveSamplePack(int pack) {
    
    
    char filename[64];
    


         char dirPath[64];
    // Ensure the directory exists
    snprintf(dirPath, sizeof(dirPath), "%d", pack);
         if (!SD.exists(dirPath)) {
        SD.mkdir(dirPath);
    }

    // Iterate over each sample channel (1..FOLDER_MAX)
    for (uint8_t ch = 1; ch <= 8; ch++) {

       for (unsigned int f = 1; f < (maxY / 2) + 1; f++) {
      light(ch + 1, f, CRGB(4, 0, 0));
    }
    showIcons(ICON_SAMPLEPACK, UI_BG_DIM);
    FastLED.setBrightness(ledBrightness);
    FastLED.show();

        // Construct the output path: samples/<pack>/<ch>.wav
        snprintf(filename, sizeof(filename), "%d/%d.wav", pack, ch);
    

      //  //Serial.println(filename);
        // Remove any existing file at this path
        if (SD.exists(filename)) {
            SD.remove(filename);
        }
        
        // Open a new file for writing
        File outFile = SD.open(filename, FILE_WRITE);
        if (!outFile) {
            //Serial.print("Failed to create ");
            //Serial.println(filename);
            continue;
        }
        
        // Write standard 44-byte WAV header (mono, 16-bit, AUDIO_SAMPLE_RATE_EXACT)
        writeWavHeader(outFile, AUDIO_SAMPLE_RATE_EXACT, 16, 1);
        
        // Determine how many samples were recorded for this channel
        uint32_t sampleCount = loadedSampleLen[ch];  // number of int16_t samples
        
        // Write the raw PCM data from the sampled buffer
        outFile.write(reinterpret_cast<uint8_t*>(sampled[ch]), sampleCount * sizeof(int16_t));
        
        outFile.close();
        //Serial.print("Saved sample ");
        //Serial.println(filename);
         for (unsigned int f = 1; f < (maxY + 1) + 1; f++) {
      light(ch + 1, f, CRGB(0, 20, 0));
    }
    FastLEDshow();
    }
    
    // Reset paint/unpaint prevention flag after saveSamplePack operation
    extern bool preventPaintUnpaint;
    preventPaintUnpaint = false;
    
   yield();
}


void loadPattern(bool autoload) {
  
  //Serial.println("Loading slot #" + String(SMP.file));
  drawNoSD();
  
  FastLEDclear();
  char OUTPUTf[50];
  if (autoload) {
    sprintf(OUTPUTf, "autosaved.txt");
  } else {
    sprintf(OUTPUTf, "%d.txt", SMP.file);
  }
  if (SD.exists(OUTPUTf)) {
    // showNumber(SMP.file, CRGB(0, 0, 50), 0);
    File loadFile = SD.open(OUTPUTf);
    if (loadFile) {
      unsigned int sdry = 1;
      unsigned int sdrx = 1;

      while (loadFile.available()) {
        int b = loadFile.read();

        // Check for the marker indicating the end of notes
        if (b == 0xFF && loadFile.peek() == 0xFE) {
          loadFile.read();  // Consume the second marker byte
          break;            // Exit the loop to load SMP data
        }

        int v = loadFile.read();
        note[sdrx][sdry].channel = b;
        note[sdrx][sdry].velocity = v;
        sdry++;
        if (sdry > maxY) {
          sdry = 1;
          sdrx++;
        }
        if (sdrx > maxlen)
          sdrx = 1;
      }

      // Load SMP struct after marker (file/pattern specific data)
      if (loadFile.available()) {
        loadFile.read((uint8_t *)&SMP, sizeof(SMP));
      }
    }
    loadFile.close();
    yield();
    
    // Reset basic runtime flags when loading a pattern
    GLOB.singleMode = false;
    
    Mode *bpm_vol = &volume_bpm;
    bpm_vol->pos[3] = SMP.bpm;
    playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
    playTimer.update(playNoteInterval);
    //midiTimer.update(playNoteInterval / 48);
    bpm_vol->pos[2] = GLOB.vol;

    float vol = float(GLOB.vol / 10.0);

    // Display the loaded SMP data
    //Serial.println("Loaded SMP Data:");
    //Serial.println("singleMode: " + String(GLOB.singleMode));
    //Serial.println("currentChannel: " + String(GLOB.currentChannel));
    //Serial.println("vol: " + String(GLOB.vol));
    //Serial.println("bpm: " + String(SMP.bpm));
    //Serial.println("velocity: " + String(GLOB.velocity));
    //Serial.println("page: " + String(GLOB.page));
    //Serial.println("edit: " + String(GLOB.edit));
    //Serial.println("file: " + String(SMP.file));
    //Serial.println("pack: " + String(SMP.pack));
    //Serial.println("wav: " + String(SMP.wav[GLOB.currentChannel].fileID));
    //Serial.println("folder: " + String(GLOB.folder));
    //Serial.println("activeCopy: " + String(GLOB.activeCopy));
    //Serial.println("x: " + String(GLOB.x));
    //Serial.println("y: " + String(GLOB.y));
    //Serial.println("seek: " + String(GLOB.seek));
    //Serial.println("seekEnd: " + String(GLOB.seekEnd));
    //Serial.println("smplen: " + String(GLOB.smplen));
    //Serial.println("shiftX: " + String(GLOB.shiftX));
    //Serial.print("pram_settings: ");
    for (unsigned int i = 0; i < 8; i++) {
      //Serial.print(SMP.param_settings[1][i]);
      //Serial.print(", ");
    }
    //Serial.println();
    //Serial.print("filter_settings: ");
    for (unsigned int i = 0; i < 8; i++) {
      //Serial.print(SMP.filter_settings[1][i]);
      //Serial.print(", ");
    }
    //Serial.println();
      //Serial.print("drum_settings: ");
    for (unsigned int i = 0; i < 4; i++) {
      //Serial.print(SMP.drum_settings[1][i]);
      //Serial.print(", ");
    }

    //Serial.println();
    
    //Serial.print("mute: ");
    for (unsigned int i = 0; i < maxY; i++) {
      //Serial.print(SMP.mute[i]);
      //Serial.print(", ");
    }
        //Serial.println();
    
    // Reset paint/unpaint prevention flag after loadPattern operation
    extern bool preventPaintUnpaint;
    preventPaintUnpaint = false;
} else {
    // File not found - show NEW screen for genre generation when creating new file
    extern void showNewFileScreen();
    showNewFileScreen();
    return; // Don't continue with normal load flow
  }

  updateLastPage();
  loadSMPSettings();
  
  if (!autoload) {
    delay(500);
    switchMode(&draw);
  }
  //applySMPSettings();
  yield();
}



void autoLoad() {
  
  loadPattern(true);
  
  // Reset paint/unpaint prevention flag after autoLoad operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}

void autoSave() {
  //savePatternAsMIDI(true);
  savePattern(true);
  
  // Reset paint/unpaint prevention flag after autoSave operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}