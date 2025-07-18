
/*
void flushAudioQueueToSD() {
  if (!isRecording || !frec) return;
  // Only flush every ~10–15 ms to prevent overload
  //if (recFlushTimer > 100) {
  while (queue1.available() >= 2) {
    uint8_t buffer[512];
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer + 256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    frec.write(buffer, 512);
  }
  recFlushTimer = 0;
  //}
}




void startRecordSD(int fnr, int snr) {
  if (!isRecording) {
    playSdWav1.stop();
    Encoder[0].writeRGBCode(0x000000);
    Encoder[1].writeRGBCode(0xFF0000);
    Encoder[2].writeRGBCode(0x000000);
    Encoder[3].writeRGBCode(0x000000);

    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
    showIcons(ICON_REC, CRGB(20, 0, 0));

    FastLED.show();
    if (SD.exists(OUTPUTf)) {
      SD.remove(OUTPUTf);
    }
    frec = SD.open(OUTPUTf, FILE_WRITE);
    if (frec) {
      recFlushTimer = 0;
      delay(100);
      writeWavHeader(frec, (uint32_t)AUDIO_SAMPLE_RATE_EXACT, 16, 1);  // 44.1kHz, 16-bit, mono
      isRecording = true;
      queue1.begin();
    }
  }
  return;
}

void stopRecordSD(int fnr, int snr) {
  if (isRecording) {
    isRecording = false;
    queue1.end();

    //AudioInterrupts();
    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));

    //Encoder[2].writeRGBCode(0x000000);
    if (frec) {
      while (queue1.available() > 0) {
        frec.write((uint8_t *)queue1.readBuffer(), 256);
        queue1.freeBuffer();
      }
      frec.close();
      delay(100);
      // Update file sizes in header
      File f = SD.open(OUTPUTf, FILE_WRITE);
      if (f) {
        uint32_t fileSize = f.size();
        uint32_t dataSize = fileSize - 44;
        f.seek(4);
        uint32_t riffSize = fileSize - 8;
        f.write((uint8_t *)&riffSize, 4);
        f.seek(40);
        f.write((uint8_t *)&dataSize, 4);
        f.close();
      }
    }
    delay(100);
    playSdWav1.play(OUTPUTf);

    previewIsPlaying = false;
    sampleIsLoaded = false;
    processPeaks();
    showWave();
  }
}

*/


/*void savePatternAsMIDI(bool autosave) {
  
  yield();
  //Serial.println("Saving MIDI file");
  drawNoSD();
  FastLEDclear();

  char OUTPUTf[50];
  if (autosave) {
    sprintf(OUTPUTf, "autosaved.mid");
  } else {
    sprintf(OUTPUTf, "%d.mid", SMP.file);
  }

  if (SD.exists(OUTPUTf)) {
    SD.remove(OUTPUTf);
  }

  File saveFile = SD.open(OUTPUTf, FILE_WRITE);
  if (saveFile) {
    // Write MIDI header
    saveFile.write("MThd", 4);  // Chunk type

    uint8_t chunkLength[] = { 0x00, 0x00, 0x00, 0x06 };  // Chunk length
    saveFile.write(chunkLength, 4);

    uint8_t formatType[] = { 0x00, 0x01 };  // Format type (1: multi-track)
    saveFile.write(formatType, 2);

    uint8_t numTracks[] = { 0x00, 0x01 };  // Number of tracks
    saveFile.write(numTracks, 2);

    uint8_t division[] = { 0x00, 0x60 };  // Division (96 ticks per quarter note)
    saveFile.write(division, 2);

    // Write Track Chunk Header
    saveFile.write("MTrk", 4);  // Chunk type

    // Placeholder for track length (to be updated later)
    uint32_t trackLengthPos = saveFile.position();
    uint8_t placeholder[] = { 0x00, 0x00, 0x00, 0x00 };
    saveFile.write(placeholder, 4);

    // Write SMP BPM as a meta event
    saveFile.write(0x00);  // Delta time
    saveFile.write(0xFF);  // Meta event
    saveFile.write(0x51);  // Set Tempo
    saveFile.write(0x03);  // Length
    uint32_t microsecondsPerQuarter = 60000000 / SMP.bpm;
    saveFile.write((uint8_t)((microsecondsPerQuarter >> 16) & 0xFF));
    saveFile.write((uint8_t)((microsecondsPerQuarter >> 8) & 0xFF));
    saveFile.write((uint8_t)(microsecondsPerQuarter & 0xFF));

    // Write MIDI events (notes)
    unsigned int trackLength = 0;
    for (unsigned int sdx = 1; sdx < maxlen; sdx++) {
      for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
        uint8_t channel = note[sdx][sdy].channel % 16;  // Determine MIDI channel (1-16)
        if (channel == 0) {
          continue;  // Skip invalid channel 0
        }
        uint8_t baseNote = 60;                      // C4 (Middle C)
        uint8_t noteNumber = baseNote + (sdy - 1);  // Adjust note based on sdy
        uint8_t velocity = note[sdx][sdy].velocity;

        if (velocity > 0) {
          // Note On event
          saveFile.write(0x00);                  // Delta time
          saveFile.write(0x90 | (channel - 1));  // Note On, specific channel (0-indexed internally)
          saveFile.write(noteNumber);            // Note number
          saveFile.write(velocity);              // Velocity
          trackLength += 4;

          // Note Off event
          saveFile.write(0x60);                  // Delta time (96 ticks later)
          saveFile.write(0x80 | (channel - 1));  // Note Off, specific channel (0-indexed internally)
          saveFile.write(noteNumber);            // Note number
          saveFile.write(0x00);                  // Velocity
          trackLength += 4;
        }
      }
    }

    // End of track event
    saveFile.write(0x00);  // Delta time
    saveFile.write(0xFF);  // Meta event
    saveFile.write(0x2F);  // End of track
    saveFile.write(0x00);  // Length
    trackLength += 4;

    // Update track length in the header
    uint32_t endPos = saveFile.position();
    saveFile.seek(trackLengthPos);
    uint8_t trackLengthBytes[] = {
      (uint8_t)((trackLength >> 24) & 0xFF),
      (uint8_t)((trackLength >> 16) & 0xFF),
      (uint8_t)((trackLength >> 8) & 0xFF),
      (uint8_t)(trackLength & 0xFF)
    };
    saveFile.write(trackLengthBytes, 4);
    saveFile.seek(endPos);

    saveFile.close();
    //Serial.println("MIDI file saved successfully!");
  } else {
    //Serial.println("Failed to open file for writing");
  }

  if (!autosave) {
    delay(500);
    switchMode(&draw);
  }
  yield();
}
*/

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

    // Save SMP struct
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
    showIcons(ICON_SAMPLEPACK, CRGB(20, 20, 20));
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

      // Load SMP struct after marker
      if (loadFile.available()) {
        loadFile.read((uint8_t *)&SMP, sizeof(SMP));
      }
    }
    loadFile.close();
    yield();
    Mode *bpm_vol = &volume_bpm;
    bpm_vol->pos[3] = SMP.bpm;
    playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
    playTimer.update(playNoteInterval);
    //midiTimer.update(playNoteInterval / 48);
    bpm_vol->pos[2] = SMP.vol;


    float vol = float(SMP.vol / 10.0);

    SMP.singleMode = false;

    // Display the loaded SMP data
    //Serial.println("Loaded SMP Data:");
    //Serial.println("singleMode: " + String(SMP.singleMode));
    //Serial.println("currentChannel: " + String(SMP.currentChannel));
    //Serial.println("vol: " + String(SMP.vol));
    //Serial.println("bpm: " + String(SMP.bpm));
    //Serial.println("velocity: " + String(SMP.velocity));
    //Serial.println("page: " + String(SMP.page));
    //Serial.println("edit: " + String(SMP.edit));
    //Serial.println("file: " + String(SMP.file));
    //Serial.println("pack: " + String(SMP.pack));
    //Serial.println("wav: " + String(SMP.wav[SMP.currentChannel].fileID));
    //Serial.println("folder: " + String(SMP.folder));
    //Serial.println("activeCopy: " + String(SMP.activeCopy));
    //Serial.println("x: " + String(SMP.x));
    //Serial.println("y: " + String(SMP.y));
    //Serial.println("seek: " + String(SMP.seek));
    //Serial.println("seekEnd: " + String(SMP.seekEnd));
    //Serial.println("smplen: " + String(SMP.smplen));
    //Serial.println("shiftX: " + String(SMP.shiftX));


    SMP.seek = 0;
    SMP.seekEnd = 0;
    SMP.smplen = 0;
    SMP.shiftX = 0;
    SMP.shiftY = 0;
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
  } else {
    for (unsigned int nx = 1; nx < maxlen; nx++) {
      for (unsigned int ny = 1; ny < maxY + 1; ny++) {
        note[nx][ny].channel = 0;
        note[nx][ny].velocity = defaultVelocity;
      }
    }
  }

  updateLastPage();
  //loadSMPSettings();
  
  if (!autoload) {
    delay(500);
    switchMode(&draw);
  }
  //applySMPSettings();
  yield();
}



void autoLoad() {
  
  loadPattern(true);
  
}

void autoSave() {
  //savePatternAsMIDI(true);
  savePattern(true);
}