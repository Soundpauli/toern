
void savePatternAsMIDI(bool autosave) {
  yield();
  Serial.println("Saving MIDI file");
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
        uint8_t channel = note[sdx][sdy][0] % 16;  // Determine MIDI channel (1-16)
        if (channel == 0) {
          continue;  // Skip invalid channel 0
        }
        uint8_t baseNote = 60;                      // C4 (Middle C)
        uint8_t noteNumber = baseNote + (sdy - 1);  // Adjust note based on sdy
        uint8_t velocity = note[sdx][sdy][1];

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
    Serial.println("MIDI file saved successfully!");
  } else {
    Serial.println("Failed to open file for writing");
  }

  if (!autosave) {
    delay(500);
    switchMode(&draw);
  }
  yield();
}


void savePattern(bool autosave) {
  //yield();
  Serial.println("Saving in slot #" + String(SMP.file));
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
        maxdata = maxdata + note[sdx][sdy][0];
        saveFile.write(note[sdx][sdy][0]);
        saveFile.write(note[sdx][sdy][1]);
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





void saveSamplePack(unsigned int pack) {
  Serial.println("Saving SamplePack in #" + String(pack));
  FastLEDclear();
  char OUTPUTdir[50];
  sprintf(OUTPUTdir, "%d/", pack);
  SD.mkdir(OUTPUTdir);
  delay(500);
  for (unsigned int i = 0; i < sizeof(usedFiles) / sizeof(usedFiles[0]); i++) {
    for (unsigned int f = 1; f < (maxY / 2) + 1; f++) {
      light(i + 1, f, CRGB(4, 0, 0));
    }
    showIcons("icon_samplepack", CRGB(20, 20, 20));
    FastLED.setBrightness(ledBrightness);
    FastLED.show();

    if (SD.exists(usedFiles[i].c_str())) {
      File saveFilePack = SD.open(usedFiles[i].c_str());
      char OUTPUTf[50];
      sprintf(OUTPUTf, "%d/%d.wav", pack, i + 1);

      if (SD.exists(OUTPUTf)) {
        SD.remove(OUTPUTf);
        delay(100);
      }

      File myDestFile = SD.open(OUTPUTf, FILE_WRITE);

      size_t n;
      uint8_t buf[512];
      while ((n = saveFilePack.read(buf, sizeof(buf))) > 0) {
        myDestFile.write(buf, n);
      }
      myDestFile.close();
      saveFilePack.close();
    }

    for (unsigned int f = 1; f < (maxY + 1) + 1; f++) {
      light(i + 1, f, CRGB(0, 20, 0));
    }
    FastLEDshow();
  }
  switchMode(&draw);
}



void loadPattern(bool autoload) {
  yield();
  Serial.println("Loading slot #" + String(SMP.file));
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
        note[sdrx][sdry][0] = b;
        note[sdrx][sdry][1] = v;
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
    // yield();
    Mode *bpm_vol = &volume_bpm;
    bpm_vol->pos[3] = SMP.bpm;
    playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
    playTimer.update(playNoteInterval);
    midiTimer.update(playNoteInterval / 48);
    bpm_vol->pos[2] = SMP.vol;


    float vol = float(SMP.vol / 10.0);
    //if (vol <= 1.0) sgtl5000_1.volume(vol);
    // set all Filters
    //for (unsigned int i = 0; i < maxFilters; i++) {
    //filters[i]->frequency(100 * SMP.param_settings[SMP.currentChannel][i]);
    //}
    SMP.singleMode = false;

    // Display the loaded SMP data
    Serial.println("Loaded SMP Data:");
    Serial.println("singleMode: " + String(SMP.singleMode));
    Serial.println("currentChannel: " + String(SMP.currentChannel));
    Serial.println("vol: " + String(SMP.vol));
    Serial.println("bpm: " + String(SMP.bpm));
    Serial.println("velocity: " + String(SMP.velocity));
    Serial.println("page: " + String(SMP.page));
    Serial.println("edit: " + String(SMP.edit));
    Serial.println("file: " + String(SMP.file));
    Serial.println("pack: " + String(SMP.pack));
    Serial.println("wav: " + String(SMP.wav[SMP.currentChannel][1]));
    Serial.println("folder: " + String(SMP.folder));
    Serial.println("activeCopy: " + String(SMP.activeCopy));
    Serial.println("x: " + String(SMP.x));
    Serial.println("y: " + String(SMP.y));
    Serial.println("seek: " + String(SMP.seek));
    Serial.println("seekEnd: " + String(SMP.seekEnd));
    Serial.println("smplen: " + String(SMP.smplen));
    Serial.println("shiftX: " + String(SMP.shiftX));


    SMP.seek = 0;
    SMP.seekEnd = 0;
    SMP.smplen = 0;
    SMP.shiftX = 0;
    SMP.shiftY = 0;

    for (unsigned int i = 0; i < maxFilters; i++) {
      //Serial.print(SMP.param_settings[SMP.currentChannel][i]);
      Serial.print(", ");
    }

    Serial.println("mute: ");
    for (unsigned int i = 0; i < maxY; i++) {
      Serial.print(SMP.mute[i]);
      Serial.print(", ");
    }
    Serial.println();
  } else {
    for (unsigned int nx = 1; nx < maxlen; nx++) {
      for (unsigned int ny = 1; ny < maxY + 1; ny++) {
        note[nx][ny][0] = 0;
        note[nx][ny][1] = defaultVelocity;
      }
    }
  }

  updateLastPage();

  if (!autoload) {
    delay(500);
    switchMode(&draw);
  }
}



void autoLoad() {
  loadPattern(true);
}

void autoSave() {
  savePatternAsMIDI(true);
  savePattern(true);
}