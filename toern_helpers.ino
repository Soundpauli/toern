
// Forward declarations for musical random generation functions
struct HarmonicAnalysis {
  bool hasRoot = false;
  bool hasThird = false;
  bool hasFifth = false;
  bool hasSeventh = false;
  int rootNote = 0;
  bool isMajor = false;
  bool isMinor = false;
};

struct BasePagePattern {
  bool hasNotes[16];           // Which steps have notes for this channel
  int noteRows[16];           // Which rows (pitches) are used on each step
  int velocities[16];         // Velocity values for each step
  int stepCount;              // How many steps have notes
  float density;              // Note density (steps with notes / total steps)
  int mostCommonRow;          // Most frequently used note row
  int rhythmPattern[16];      // Rhythm pattern (1 = note, 0 = rest)
  bool isRhythmic;            // True if this channel has rhythmic patterns
  bool isMelodic;             // True if this channel has melodic patterns
};

// Global buffer for AI generation in external RAM (prevents stack overflow)
EXTMEM BasePagePattern g_channelPatterns[16];

void generateRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBassOrMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBassLine(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateMainMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBasicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateContextAwareRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony, BasePagePattern* basePattern, int pageOffset);
void generateContextAwareMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony, BasePagePattern* basePattern, int pageOffset);
int createHarmonicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
int createMelodicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
int createRhythmicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
void generateSong();
void generateGenreTrack();
void setGenreBPM();
void applyBPMDirectly(int bpm);
void generateTechnoPattern(unsigned int start, unsigned int end, unsigned int page);
void generateHipHopPattern(unsigned int start, unsigned int end, unsigned int page);
void generateDnBPattern(unsigned int start, unsigned int end, unsigned int page);
void generateHousePattern(unsigned int start, unsigned int end, unsigned int page);
void generateAmbientPattern(unsigned int start, unsigned int end, unsigned int page);

// External variables from menu
extern int genreType;
extern int genreLength;

// External variables from main system
extern Device SMP;
extern Mode volume_bpm;
extern IntervalTimer playTimer;
extern float playNoteInterval;
extern float detune[13]; // Global detune array for channels 1-12
extern float channelOctave[9]; // Global octave array for channels 1-8
extern int8_t channelDirection[maxFiles];
extern unsigned int recordingStartBeat;  // Beat where recording started

void EEPROMgetLastFiles() {
  //get lastFile Array from Eeprom
  EEPROM.get(100, lastFile);
  set_Wav.maxValues[3] = lastFile[FOLDER_MAX - 1];
  //if lastFile from eeprom is empty, set it
  if (lastFile[0] == 0) {
    EEPROMsetLastFile();
  }
}





// Wrapper: load all saved SMP settings (parameters, filters, drums, synths)
// for every channel.
void loadSMPSettings() {
  // Don't load settings if SMP_LOAD_SETTINGS is false
  if (!SMP_LOAD_SETTINGS) {
    return;
  }
  
  // CRITICAL FIX: Ensure channels 4-8 are always in sample mode (EFX=0)
  // This prevents saved patterns from incorrectly setting these channels to drum mode
  for (int ch = 4; ch <= 8; ch++) {
    SMP.filter_settings[ch][EFX] = 0;
  }
  
  // Define the valid channels: 1,2,3,4,5,6,7,8,11,13,14
  const int validChannels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numValidChannels = sizeof(validChannels) / sizeof(validChannels[0]);
  

  for (int i = 0; i < numValidChannels; i++) {
    int ch = validChannels[i];
    
    // Load Filters - apply all filter settings for this channel
    for (int f = 0; f < NUM_FILTERS; f++) {
      setFilters((FilterType)f, ch, true);
    }
    
    // Load Parameters - apply all parameter settings for this channel
    for (int p = 0; p < 5; p++) {
    // Set the encoder value to the saved parameter value.S
    //currentMode->pos[3] = SMP.param_settings[ch][p];
    // Process the parameter mapping.
    setParams(p, ch);
  }
    

  
    // Load Drums - apply drum settings (only for channels 1-3)
    if (ch >= 1 && ch <= 3) {
        for (int d = 0; d < NUM_DRUMS; d++) {
           setDrums((DrumTypes)d, ch);
        }
    }
    
    // Load Synths - apply synth settings (only for channel 11)
   if (ch == 11) {
    updateSynthVoice(11);
  }
  return;
  }
  
  // CRITICAL: Initialize drum engines for channels set to DRUM mode (EFX=1)
  // This replicates what happens during manual switching and ensures drums work after loading
  for (int ch = 1; ch <= 3; ch++) {
    if (SMP.filter_settings[ch][EFX] == 1) {
      // Use default parameters
      float tone = 0;      // DRUMTONE = 0
      float decay = 512;   // DRUMDECAY = 32 mapped to 0-1023
      float pitchMod = 512; // DRUMPITCH = 32 mapped to 0-1023  
      int type = 1;        // DRUMTYPE = 1
      
      // Initialize the appropriate drum engine
      if (ch == 1) {
        KD_drum(tone, decay, pitchMod, type);
      } else if (ch == 2) {
        SN_drum(tone, decay, pitchMod, type);
      } else if (ch == 3) {
        HH_drum(tone, decay, pitchMod, type);
      }
      
      // Trigger the shared envelope to initialize it
      // This replicates what happens when samples play and makes drums work
      if (ch == 1) {
        envelope1.noteOn();
      } else if (ch == 2) {
        envelope2.noteOn();
      } else if (ch == 3) {
        envelope3.noteOn();
      }
    }
  }
  
  
  // Optionally update other settings such as channel volumes
  // or call updateFiltersAndParameters() if needed.
  //updateFiltersAndParameters();
}



void writeWavHeader(File &file, uint32_t sampleRate, uint8_t bitsPerSample, uint16_t numChannels) {
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint8_t blockAlign = numChannels * bitsPerSample / 8;

  // WAV header (44 bytes)
  uint8_t header[44] = {
    'R', 'I', 'F', 'F',
    0, 0, 0, 0,  // <- file size - 8 (filled in later)
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  // PCM chunk size
    1, 0,         // Audio format (1 = PCM)
    (uint8_t)(numChannels & 0xff), (uint8_t)(numChannels >> 8),
    (uint8_t)(sampleRate & 0xff), (uint8_t)((sampleRate >> 8) & 0xff),
    (uint8_t)((sampleRate >> 16) & 0xff), (uint8_t)((sampleRate >> 24) & 0xff),
    (uint8_t)(byteRate & 0xff), (uint8_t)((byteRate >> 8) & 0xff),
    (uint8_t)((byteRate >> 16) & 0xff), (uint8_t)((byteRate >> 24) & 0xff),
    blockAlign, 0,
    bitsPerSample, 0,
    'd', 'a', 't', 'a',
    0, 0, 0, 0  // <- data chunk size (filled in later)
  };

  file.write(header, 44);
}


void startRecordingRAM() {
  if (isRecording) return;

  if (playSdWav1.isPlaying()) playSdWav1.stop();
  Encoder[0].writeRGBCode(0x000000);
  Encoder[1].writeRGBCode(0xFF0000);
  Encoder[2].writeRGBCode(0x000000);
  Encoder[3].writeRGBCode(0x000000);
  recTime = 0;
  recWriteIndex = 0;
  
  // Reset peak recording index for fresh waveform display
  extern int peakRecIndex;
  peakRecIndex = 0;
  
  queue1.begin();  // start filling 128‑sample blocks
  showIcons(ICON_REC, UI_DIM_RED);
  FastLEDshow();
  isRecording = true;
  
  // Enable audio input monitoring using VOL menu input level settings
  extern int recMode;
  extern unsigned int lineInLevel;  // From VOL menu (0-15)
  extern unsigned int micGain;      // From VOL menu (0-63)
  extern AudioMixer4 mixer_end;
  
    float monitorGain = 0.0;
  if (recMode == 1) {
    // Mic input: map micGain (0-63) to mixer gain (0.0-0.8)
    monitorGain = mapf(micGain, 0, 63, 0.0, 0.8);
  } else {
    // Line input: map lineInLevel (0-15) to mixer gain (0.0-0.8)
    monitorGain = mapf(lineInLevel, 0, 15, 0.0, 0.8);
    }
    mixer_end.gain(3, monitorGain);
}

void flushAudioQueueToRAM2() {
  if (!isRecording) return;
  // each AudioRecordQueue block is 128 samples
  while (queue1.available() && recWriteIndex + 128 <= BUFFER_SAMPLES) {
    auto block = (int16_t *)queue1.readBuffer();
    memcpy(recBuffer + recWriteIndex, block, 128 * sizeof(int16_t));
    recWriteIndex += 128;
    queue1.freeBuffer();
  }
  // if we ever hit the end of the buffer, stop automatically:
  if (recWriteIndex >= BUFFER_SAMPLES) {
    //Serial.println("⚠️ Buffer full");
    queue1.end();
    isRecording = false;
  }
}

void stopRecordingRAM(int fnr, int snr) {
  if (!isRecording) return;
  flushAudioQueueToRAM();
  queue1.end();
  isRecording = false;
  
  // Disable audio input monitoring
  mixer_end.gain(3, 0.0);

  // open WAV on SD
  char path[64];
  sprintf(path, "samples/%d/_%d.wav", fnr, snr);
  if (SD.exists(path)) SD.remove(path);
  File f = SD.open(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!f) { return; }
  // write 44‑byte WAV header for 16‑bit/mono/22 050 Hz
  writeWavHeader(f, AUDIO_SAMPLE_RATE_EXACT, 16, 1);
  // write all your samples in one chunk
  f.write((uint8_t *)recBuffer, recWriteIndex * sizeof(int16_t));

  f.close();
  //Serial.print("💾 Saved ");
  //Serial.println(path);
  
  // Reload the sample metadata after recording
  extern bool sampleIsLoaded;
  extern bool firstcheck;
  extern CachedSample previewCache;
  
  sampleIsLoaded = false;  // Force reload of new recording
  firstcheck = true;       // Reset check flag
  previewCache.valid = false;  // Invalidate cache so new recording is loaded
  
  // Don't auto-play - user will press encoder[2] to play if desired
  // playSdWav1.play(path);
}



// --------------------
void startFastRecord() {
  if (fastRecordActive) return;

  // Handle different recording modes based on recChannelClear
  if (recChannelClear == 1) {
    // ON mode: Clear all existing notes of channel, then add triggers/notes
    clearAllNotesOfChannel();
  }
  // OFF mode (recChannelClear == 0): Add triggers/notes as soon as recording starts
  // FIX mode (recChannelClear == 2): Don't manipulate any notes - just record

   paintMode = false;
            freshPaint = true;
            unpaintMode = false;
            pressed[3] = false;
            
            // Reset paint/unpaint prevention flag when starting fast record
            extern bool preventPaintUnpaint;
            preventPaintUnpaint = false;
            
  // Only add triggers/notes if not in FIX mode
  if (recChannelClear != 2) {
    note[beat][GLOB.currentChannel+1].channel = GLOB.currentChannel;
    note[beat][GLOB.currentChannel+1].velocity = defaultVelocity;
  }
            
  // 1) Stop & clear any queued audio so old data never sneaks in
  queue1.end();
  while (queue1.available()) {
    queue1.readBuffer();
    queue1.freeBuffer();
  }

  // 2) Reset our write index and drop counter
  int ch = GLOB.currentChannel;
  
  // Immediately stop any playing sound on the recording channel
  extern AudioEffectEnvelope *envelopes[];
  if (ch >= 0 && ch < 15 && envelopes[ch] != nullptr) {
    envelopes[ch]->noteOff();
  }
  
  // Also stop all notes on the sampler for channels 0-8 (sample channels)
  // Stop notes in the typical MIDI range (36-96 covers most sample pitches)
  if (ch >= 0 && ch <= 8) {
    extern arraysampler _samplers[];
    // Stop notes in a reasonable range (MIDI note 36-96, covering most sample pitches)
    for (int note = 36; note <= 96; note++) {
      _samplers[ch].noteEvent(note, 0, false, false);
    }
  }
  
  fastRecWriteIndex[ch] = 0;
  // For ON1 mode, don't drop initial audio - we want a perfect loop from beat 1
  // For other modes, drop first ~25ms to avoid noise/click at start (reduced from 200ms)
  if (recChannelClear == 3) {
    fastDropRemaining = 0;  // ON1 mode: no drop, capture from beat 1
  } else {
    fastDropRemaining = FAST_DROP_BLOCKS;  // Other modes: drop first ~25ms
  }

  // 3) Restart recording queue
  queue1.begin();
  fastRecordActive = true;
  
  // Enable audio input monitoring using VOL menu input level settings
  extern int recMode;
  extern unsigned int lineInLevel;  // From VOL menu (0-15)
  extern unsigned int micGain;      // From VOL menu (0-63)
  extern AudioMixer4 mixer_end;
  
  float monitorGain = 0.0;
  float maxPlaybackGain = 0.4f * 0.4f;  // GAIN_4 * GAIN_2 = 0.16 (match loudest playback)
  if (recMode == 1) {
    // Mic input: map micGain (0-63) to mixer gain (0.0-maxPlaybackGain) to match loudest playback
    monitorGain = mapf(micGain, 0, 63, 0.0, maxPlaybackGain);
  } else {
    // Line input: map lineInLevel (0-15) to mixer gain (0.0-maxPlaybackGain) to match loudest playback
    monitorGain = mapf(lineInLevel, 0, 15, 0.0, maxPlaybackGain);
  }
  mixer_end.gain(3, monitorGain);

  //Serial.printf("FAST RECORD ▶ ch%u (dropping %d blocks)\n", ch, FAST_DROP_BLOCKS);
}

// --------------------
void flushAudioQueueToRAM() {
  if (fastRecordActive) {
    int ch = GLOB.currentChannel;
    auto &idx = fastRecWriteIndex[ch];

    // Treat your byte buffer as an int16_t array for cleaner pointer math:
    int16_t *dest = reinterpret_cast<int16_t *>(sampled[ch]);

    // Pull every pending block
    while (queue1.available()) {
      int16_t *block = (int16_t *)queue1.readBuffer();

      if (fastDropRemaining > 0) {
        // still skipping the first 200ms
        fastDropRemaining--;
      } else if (idx + AUDIO_BLOCK_SAMPLES <= BUFFER_SAMPLES) {
        // copy 128 samples (256 bytes) into our buffer
        memcpy(dest + idx, block, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
        idx += AUDIO_BLOCK_SAMPLES;
      }

      queue1.freeBuffer();

      // auto-stop if full
      if (idx >= BUFFER_SAMPLES) {
        stopFastRecord();
        break;
      }
    }
    return;  // skip your SD path
  }
}

void stopFastRecord() {
  // CRITICAL: Disable audio input first to stop new data from entering queue
  mixer_end.gain(3, 0.0);
  
  int ch = GLOB.currentChannel;
  auto &idx = fastRecWriteIndex[ch];
  int16_t *dest = reinterpret_cast<int16_t *>(sampled[ch]);
  
  // Flush all remaining audio data while fastRecordActive is still true
  // Use multiple passes to ensure we capture all data for the final beat
  int flushCount = 0;
  int lastQueueSize = -1;
  int stableCount = 0;
  
  // Continue flushing until queue is empty or no progress is made
  while (fastRecordActive && queue1.available()) {
    int currentQueueSize = queue1.available();
    flushAudioQueueToRAM();
    flushCount++;
    
    // Check if queue size changed (progress made)
    if (currentQueueSize == lastQueueSize) {
      stableCount++;
      // If queue size hasn't changed for 3 passes, likely done
      if (stableCount >= 3) break;
    } else {
      stableCount = 0;
      lastQueueSize = currentQueueSize;
    }
    
    // Safety limit to prevent infinite loops
    if (flushCount >= 100) break;
  }
  
  // Now safe to stop recording
  fastRecordActive = false;
  
  // Flush one more time before ending the queue
  flushAudioQueueToRAM();
  
  queue1.end();
  
  // Final aggressive flush of any remaining data in the queue
  // Continue until queue is completely empty or buffer is full
  flushCount = 0;
  lastQueueSize = -1;
  stableCount = 0;
  
  while (queue1.available() && idx + AUDIO_BLOCK_SAMPLES <= BUFFER_SAMPLES) {
    int currentQueueSize = queue1.available();
    int16_t *block = (int16_t *)queue1.readBuffer();
    memcpy(dest + idx, block, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
    idx += AUDIO_BLOCK_SAMPLES;
    queue1.freeBuffer();
    flushCount++;
    
    // Check if queue size changed (progress made)
    if (currentQueueSize == lastQueueSize) {
      stableCount++;
      // If queue size hasn't changed for 3 passes, likely done
      if (stableCount >= 3) break;
    } else {
      stableCount = 0;
      lastQueueSize = currentQueueSize;
    }
    
    // Safety limit to prevent infinite loops
    if (flushCount >= 100) break;
  }
  
  // Save recording start beat before resetting (needed for ON1 mode note placement)
  unsigned int savedStartBeat = recordingStartBeat;
  
  // Reset recording start beat tracking
  recordingStartBeat = 0;
  
  // Reset absolute beat counter
  extern unsigned int recordingBeatCount;
  recordingBeatCount = 0;
  
  Serial.print(">>> stopFastRecord: channel=");
  Serial.print(ch);
  Serial.print(", recorded samples=");
  Serial.println(idx);
  
  loadedSampleLen[ch] = idx;
  
  // Defer sampler loading and heavy operations to avoid blocking audio
  // These will happen after the critical stop sequence
  
  // Auto-save recorded sample to samplepack 0
  Serial.print("Fast record stopped, saving to SP0 for channel ");
  Serial.println(ch);
  
  // Do heavy save operations (SD write, EEPROM) - these might cause brief hang
  // but we've already stopped recording, so audio playback should continue
  copySampleToSamplepack0(ch);
  saveSp0StateToEEPROM();
  
  // Now load into sampler (do this after saves to minimize interruption)
  _samplers[ch].removeAllSamples();
  _samplers[ch].addSample(
    36,                            // MIDI note #
    (int16_t*)sampled[ch],         // reinterpret bytes → int16_t
    idx,                           // sample-count
    rateFactor
  );
  channelDirection[ch] = 1;
  
  // For ON1 mode: Set note at x=1 after recording and copy are complete
  if (recChannelClear == 3 && savedStartBeat > 0) {
    // Find the next x=1 position (beat 1 in the next bar)
    // Since recording stopped at maxX, the next x=1 is at the start of the next bar
    unsigned int nextBeat1 = ((savedStartBeat - 1) / maxX + 1) * maxX + 1;
    // Set note at x=1
    note[nextBeat1][ch+1].channel = ch;
    note[nextBeat1][ch+1].velocity = defaultVelocity;
  }
  
  // give back your knob color + preview
 // Encoder[0].writeRGBCode(CRGBToUint32(col[ch]));
 // _samplers[ch].noteEvent(36, defaultVelocity, true, false);
  //Serial.printf("◀ FASTREC ch%u, %u samples\n", ch, (unsigned)idx);

}

void EEPROMsetLastFile() {
  //set maxFiles in folder and show loading...
  for (int f = 0; f <= FOLDER_MAX; f++) {
    //FastLEDclear();

    for (unsigned int i = 1; i < 99; i++) {
      char OUTPUTf[50];
      sprintf(OUTPUTf, "samples/%d/_%d.wav", f, i + (f * 100));
      if (SD.exists(OUTPUTf)) {
        lastFile[f] = i + (f * 100);
      }
    }
    //set last File on Encoder
    drawLoadingBar(1, 999, lastFile[f], col_base[f], CRGB(15, 15, 55), false);
  }
  
  //set lastFile Array into Eeprom
  EEPROM.put(100, lastFile);
}



void clearAllNotesOfChannel() {
  uint8_t channel = GLOB.currentChannel;

  for (uint16_t step = 0; step < maxlen; step++) {
    for (uint8_t pitch = 1; pitch <= 16; pitch++) {
      if (note[step][pitch].channel == channel) {
        note[step][pitch].channel = 0;
        note[step][pitch].velocity = defaultVelocity;
      }
    }
  }

  updateLastPage();  // Optional: If your UI tracks last updated page
  FastLEDshow();     // Optional: Refresh LED grid if used
}

void FastLEDclear() {
  // Clear ALL LEDs in the buffer to prevent flickering from uninitialized LEDs
  // Even if only 1 module is active, we must clear the entire buffer
  for (unsigned int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

void FastLEDshow() {
  if (millis() - lastUpdate > RefreshTime) {
    lastUpdate = millis();
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
  }
}



int getPage(int x) {
  //updateLastPage();
  return (x - 1) / maxX + 1;  // Calculate page number
}

// Harmonic analysis structure for musical random generation (moved to top of file)

void drawRandoms(){
  
  // Determine current page boundaries.
  unsigned int start = ((GLOB.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  
  // --- Step 1: Clear notes on the current page for the current channel ---
  for(unsigned int c = start; c < end; c++){
    for(unsigned int r = 1; r <= 16; r++){
      if(note[c][r].channel == channel){
        note[c][r].channel = 0;
        note[c][r].velocity = defaultVelocity;
      }
    }
  }
  
  // --- Step 2: Analyze existing harmonic content ---
  HarmonicAnalysis harmony;
  
  // Analyze existing notes to determine key/scale
  for(unsigned int c = start; c < end; c++){
    for(unsigned int r = 1; r <= 16; r++){
      if(note[c][r].channel != 0){
        // Map row to scale degrees (1-based)
        if(r == 1 || r == 8 || r == 15) { harmony.hasRoot = true; harmony.rootNote = r; }
        if(r == 3 || r == 10) { harmony.hasThird = true; harmony.isMajor = true; }
        if(r == 6 || r == 13) { harmony.hasThird = true; harmony.isMinor = true; }
        if(r == 5 || r == 12) { harmony.hasFifth = true; }
        if(r == 2 || r == 9) { harmony.hasSeventh = true; }
      }
    }
  }
  
  // --- Step 3: Channel-specific generation ---
  if(channel >= 1 && channel <= 4) {
    // Rhythm channels (1-4): Generate rhythmic patterns
    generateRhythmicPattern(start, end, channel, harmony);
  } else if(channel >= 5 && channel <= 8) {
    // Voice channels (5-8): Generate melodic patterns
    generateMelodicPattern(start, end, channel, harmony);
  } else if(channel == 11) {
    // Channel 11: Generate bass or main melody
    generateBassOrMelody(start, end, channel, harmony);
    } else {
    // Other channels: Use original logic as fallback
    generateBasicPattern(start, end, channel, harmony);
  }
}

// Generate rhythmic patterns for channels 1-4
void generateRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Define 4/4 time signature patterns with more sophisticated rhythms
  const int strongBeats[] = {1, 5, 9, 13};  // Beat 1, 2, 3, 4
  const int weakBeats[] = {3, 7, 11, 15};   // Off-beats
  
  // Different rhythm patterns for different channels
  int patternType = (channel - 1) % 4;
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = ((x_rel - 1) % maxX) + 1;
    
    bool shouldPlay = false;
    int velocity = defaultVelocity;
    
    switch(patternType) {
      case 0: // Channel 1: Kick-like pattern (Bass drum)
        shouldPlay = (beatPosition == 1 || beatPosition == 9); // Beat 1 and 3
        // Add some ghost notes for groove
        if(random(0, 100) < 20 && (beatPosition == 5 || beatPosition == 13)) {
          shouldPlay = true;
          velocity = 60; // Ghost note
        } else {
          velocity = 120; // Strong kick
        }
        break;
      case 1: // Channel 2: Snare-like pattern  
        shouldPlay = (beatPosition == 5 || beatPosition == 13); // Beat 2 and 4
        // Add some ghost snares
        if(random(0, 100) < 30 && (beatPosition == 3 || beatPosition == 7 || beatPosition == 11 || beatPosition == 15)) {
          shouldPlay = true;
          velocity = 70; // Ghost snare
        } else {
          velocity = 100; // Strong snare
        }
        break;
      case 2: // Channel 3: Hi-hat pattern
        shouldPlay = (beatPosition % 2 == 0); // Every other beat
        // Add some open hi-hats
        if(random(0, 100) < 15 && (beatPosition == 5 || beatPosition == 13)) {
          velocity = 90; // Open hi-hat
        } else {
          velocity = 80; // Closed hi-hat
        }
        break;
      case 3: // Channel 4: Percussion pattern
        // More musical percussion with accent patterns
        if(beatPosition == 1 || beatPosition == 9) {
          shouldPlay = true;
          velocity = 110; // Accent
        } else if(random(0, 100) < 40) {
          shouldPlay = true;
          velocity = 85; // Regular hit
        }
        break;
    }
    
    if(shouldPlay) {
      // Choose a rhythmically appropriate note with harmonic awareness
      int noteRow = 1; // Default to root
      if(harmony.hasRoot) {
        noteRow = harmony.rootNote;
        // Add some harmonic variation
        if(random(0, 100) < 20) {
          if(harmony.hasFifth) noteRow = 5;
          else if(harmony.hasThird) noteRow = (harmony.isMajor) ? 3 : 6;
        }
      } else if(harmony.hasFifth) {
        noteRow = 5;
      } else {
        noteRow = random(1, 9); // Random if no harmony detected
      }
      
      // Only place note if slot is empty (channel == 0)
      if(note[c][noteRow].channel == 0) {
        note[c][noteRow].channel = channel;
        note[c][noteRow].velocity = velocity;
      }
    }
  }
}

// Generate melodic patterns for channels 5-8
void generateMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Define scale notes based on harmonic analysis
  int scaleNotes[8];
  int scaleSize = 0;
  
  if(harmony.isMajor) {
    int majorScale[] = {1, 3, 5, 6, 8, 10, 12, 13};
    for(int i = 0; i < 8; i++) {
      scaleNotes[i] = majorScale[i];
    }
    scaleSize = 8;
  } else if(harmony.isMinor) {
    int minorScale[] = {6, 8, 10, 11, 13, 15, 1, 3};
    for(int i = 0; i < 8; i++) {
      scaleNotes[i] = minorScale[i];
    }
    scaleSize = 8;
  } else {
    // Default pentatonic scale
    int pentatonic[] = {1, 3, 5, 8, 10};
    for(int i = 0; i < 5; i++) {
      scaleNotes[i] = pentatonic[i];
    }
    scaleSize = 5;
  }
  
  // Different melodic styles for different voice channels
  int voiceType = (channel - 5) % 4;
  int phraseLength = 4; // 4-beat phrases
  int currentNote = 0;
  int lastDirection = 0; // Track melodic direction for smoother lines
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = ((x_rel - 1) % maxX) + 1;
    
    // Create melodic phrases with musical intervals
    if(beatPosition % phraseLength == 1) {
      // Start of phrase - choose a strong note
      currentNote = random(0, scaleSize);
      lastDirection = 0;
    } else {
      // Continue phrase with musical motion
      int direction = random(0, 5); // More options for musical variety
      
      switch(voiceType) {
        case 0: // Voice 1: Smooth step-wise motion
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;
          else if(direction == 3 && currentNote < scaleSize - 2) currentNote += 2;
          else if(direction == 4 && currentNote > 1) currentNote -= 2;
          break;
        case 1: // Voice 2: More leaps and jumps
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;
          else if(direction == 3 && currentNote < scaleSize - 3) currentNote += 3;
          else if(direction == 4 && currentNote > 2) currentNote -= 3;
          break;
        case 2: // Voice 3: Arpeggio-like patterns
          if(direction == 1) currentNote = (currentNote + 2) % scaleSize;
          else if(direction == 2) currentNote = (currentNote + 3) % scaleSize;
          else if(direction == 3) currentNote = (currentNote + 4) % scaleSize;
          else if(direction == 4) currentNote = (currentNote - 1 + scaleSize) % scaleSize;
          break;
        case 3: // Voice 4: Sustained notes with occasional movement
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;
          // Otherwise stay on same note (sustained)
          break;
        }
      }
    
    // Add rhythmic variation based on voice type
    bool shouldPlay = false;
    int velocity = defaultVelocity;
    
    switch(voiceType) {
      case 0: // Voice 1: Regular rhythm
        if(beatPosition % 2 == 1) shouldPlay = true;
        else if(random(0, 100) < 30) shouldPlay = true;
        velocity += random(-15, 16);
        break;
      case 1: // Voice 2: Syncopated rhythm
        if(random(0, 100) < 60) shouldPlay = true;
        velocity += random(-10, 21);
        break;
      case 2: // Voice 3: Arpeggio rhythm
        if(beatPosition % 4 == 1) shouldPlay = true;
        else if(random(0, 100) < 25) shouldPlay = true;
        velocity += random(-5, 26);
        break;
      case 3: // Voice 4: Sustained notes
        if(beatPosition == 1 || beatPosition == 9) shouldPlay = true;
        else if(random(0, 100) < 20) shouldPlay = true;
        velocity += random(-5, 11); // Less velocity variation
        break;
    }
    
    if(shouldPlay && scaleSize > 0) {
      int noteRow = scaleNotes[currentNote];
      
      // Only place note if slot is empty (channel == 0)
      if(note[c][noteRow].channel == 0) {
        note[c][noteRow].channel = channel;
        note[c][noteRow].velocity = velocity;
      }
    }
  }
}

// Generate bass or main melody for channel 11
void generateBassOrMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Channel 11 should primarily be bass lines (80% bass, 20% melody)
  bool generateBass = (random(0, 100) < 80);
  
  if(generateBass) {
    // Generate bass line
    generateBassLine(start, end, channel, harmony);
  } else {
    // Generate main melody (with reduced chords)
    generateMainMelody(start, end, channel, harmony);
  }
}

// Generate bass line
void generateBassLine(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Classic bass notes: root, third, fifth, octave (low register)
  int bassNotes[] = {1, 2, 3, 4, 5, 6, 7, 8}; // Low register (C3 to C4)
  int bassSize = 8;
  
  int currentBassNote = 0;
  int phraseLength = 4; // Shorter phrases for bass
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = ((x_rel - 1) % maxX) + 1;
    
    // Create bass phrases
    if(beatPosition % phraseLength == 1) {
      currentBassNote = random(0, bassSize);
    } else {
      // Bass motion - mostly stepwise with occasional leaps
      int direction = random(0, 6);
      if(direction == 1 && currentBassNote < bassSize - 1) currentBassNote++;
      else if(direction == 2 && currentBassNote > 0) currentBassNote--;
      else if(direction == 3 && currentBassNote < bassSize - 3) currentBassNote += 2; // Leap up
      else if(direction == 4 && currentBassNote > 2) currentBassNote -= 2; // Leap down
    }
    
    // Bass plays on strong beats with some off-beat variation
    bool shouldPlay = false;
    if(beatPosition == 1 || beatPosition == 5 || beatPosition == 9 || beatPosition == 13) {
      shouldPlay = true; // Strong beats
    } else if(random(0, 100) < 25) {
      shouldPlay = true; // Some off-beats
    }
    
    if(shouldPlay) {
      int noteRow = bassNotes[currentBassNote];
      
      // Only place note if slot is empty (channel == 0)
      if(note[c][noteRow].channel == 0) {
        note[c][noteRow].channel = channel;
        note[c][noteRow].velocity = defaultVelocity + random(10, 31); // Stronger bass with variation
      }
    }
  }
}

// Generate main melody
void generateMainMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Melody uses higher register and more complex patterns
  int melodyNotes[] = {10, 12, 13, 15, 1, 3, 5, 6};
  int melodySize = 8;
  
  int currentNote = 0;
  int phraseLength = 8; // Longer phrases for melody
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = ((x_rel - 1) % maxX) + 1;
    
    // Create melodic phrases
    if(beatPosition % phraseLength == 1) {
      currentNote = random(0, melodySize);
    } else {
      // Melodic motion with larger intervals
      int direction = random(0, 5); // More variation
      if(direction == 1 && currentNote < melodySize - 1) currentNote++;
      else if(direction == 2 && currentNote > 0) currentNote--;
      else if(direction == 3 && currentNote < melodySize - 2) currentNote += 2;
      else if(direction == 4 && currentNote > 1) currentNote -= 2;
    }
    
    // Melody plays more frequently
    bool shouldPlay = false;
    if(random(0, 100) < 70) shouldPlay = true;
    
    if(shouldPlay && melodySize > 0) {
      // Decide how many notes to play (mostly single notes, occasional chords)
      int noteCount;
      int chordChance = random(0, 100);
      if(chordChance < 80) {
        noteCount = 1; // 80% single notes
      } else if(chordChance < 95) {
        noteCount = 2; // 15% two-note chords
      } else {
        noteCount = 3; // 5% three-note chords
      }
      
      int rootNote = melodyNotes[currentNote];
      
      if(noteCount == 1) {
        // Single note - only place if slot is empty
        if(note[c][rootNote].channel == 0) {
          note[c][rootNote].channel = channel;
          note[c][rootNote].velocity = defaultVelocity + random(-10, 31);
        }
      } else if(noteCount == 2) {
        // Two-note chord - convert rootNote (1-16) to piano index (0-15), add intervals, convert back
        int rootPianoIndex = rootNote - 1; // Convert 1-16 to 0-15
        int intervalChoice = random(0, 4); // 0-3 for different intervals
        int secondPianoIndex;
        
        switch(intervalChoice) {
          case 0: secondPianoIndex = rootPianoIndex + 3; break;  // Minor third (C-Eb)
          case 1: secondPianoIndex = rootPianoIndex + 5; break;  // Perfect fourth (C-F)
          case 2: secondPianoIndex = rootPianoIndex + 7; break;  // Perfect fifth (C-G)
          case 3: secondPianoIndex = rootPianoIndex + 12; break; // Perfect octave (C-C)
        }
        
        // Ensure piano index stays within valid range (0-15)
        if(secondPianoIndex > 15) secondPianoIndex -= 12;  // Octave down
        if(secondPianoIndex < 0) secondPianoIndex += 12;   // Octave up
        
        int secondNote = secondPianoIndex + 1; // Convert back to 1-16
        
        // Place both notes - only if slots are empty
        if(note[c][rootNote].channel == 0) {
          note[c][rootNote].channel = channel;
          note[c][rootNote].velocity = defaultVelocity + random(-5, 21); // Root note slightly louder
        }
        
        if(note[c][secondNote].channel == 0) {
          note[c][secondNote].channel = channel;
          note[c][secondNote].velocity = defaultVelocity + random(-10, 16); // Second note
        }
      } else { // noteCount == 3
        // Three-note chord - 50% major, 50% minor (with occasional sus4)
        int chordType;
        if(random(0, 100) < 90) {
          // 90% chance for major or minor (45% each)
          chordType = random(0, 2); // 0 = major, 1 = minor
        } else {
          // 10% chance for sus4
          chordType = 2;
        }
        
        // Convert rootNote (1-16) to piano index (0-15)
        int rootPianoIndex = rootNote - 1;
        int secondPianoIndex, thirdPianoIndex;
        
        switch(chordType) {
          case 0: // Major triad (C-E-G)
            secondPianoIndex = rootPianoIndex + 4;  // Major third (C to E)
            thirdPianoIndex = rootPianoIndex + 7;   // Perfect fifth (C to G)
            break;
          case 1: // Minor triad (C-Eb-G)
            secondPianoIndex = rootPianoIndex + 3;  // Minor third (C to Eb)
            thirdPianoIndex = rootPianoIndex + 7;    // Perfect fifth (C to G)
            break;
          case 2: // Sus4 chord (C-F-G)
            secondPianoIndex = rootPianoIndex + 5;  // Perfect fourth (C to F)
            thirdPianoIndex = rootPianoIndex + 7;    // Perfect fifth (C to G)
            break;
        }
        
        // Ensure piano indices stay within valid range (0-15)
        if(secondPianoIndex > 15) secondPianoIndex -= 12;  // Octave down
        if(thirdPianoIndex > 15) thirdPianoIndex -= 12;    // Octave down
        if(secondPianoIndex < 0) secondPianoIndex += 12;   // Octave up
        if(thirdPianoIndex < 0) thirdPianoIndex += 12;      // Octave up
        
        int secondNote = secondPianoIndex + 1; // Convert back to 1-16
        int thirdNote = thirdPianoIndex + 1;   // Convert back to 1-16
        
        // Place all three notes - only if slots are empty
        if(note[c][rootNote].channel == 0) {
          note[c][rootNote].channel = channel;
          note[c][rootNote].velocity = defaultVelocity + random(-5, 21); // Root note slightly louder
        }
        
        if(note[c][secondNote].channel == 0) {
          note[c][secondNote].channel = channel;
          note[c][secondNote].velocity = defaultVelocity + random(-10, 16); // Second note
        }
        
        if(note[c][thirdNote].channel == 0) {
          note[c][thirdNote].channel = channel;
          note[c][thirdNote].velocity = defaultVelocity + random(-10, 16); // Third note
        }
      }
    }
  }
}

// Context-aware rhythmic pattern generation that preserves base page rhythm
void generateContextAwareRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony, BasePagePattern* basePattern, int pageOffset) {
  // Preserve the original rhythm structure with subtle variations
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = ((x_rel - 1) % maxX) + 1;
    
    // Use the base pattern rhythm as foundation
    bool shouldPlay = basePattern->rhythmPattern[beatPosition - 1] == 1;
    
    // Add subtle variations based on page progression
    if (shouldPlay) {
      // Occasionally skip a note for variation (8% chance - subtle)
      if (random(0, 100) < 8) {
        shouldPlay = false;
      }
    } else {
      // Occasionally add a note for variation (4% chance - subtle)
      if (random(0, 100) < 4) {
        shouldPlay = true;
      }
    }
    
    if (shouldPlay) {
      // Use the base pattern's note row with harmonic progression
      int baseNoteRow = basePattern->noteRows[beatPosition - 1];
      int noteRow = baseNoteRow;
      
        // Apply harmonic progression based on page offset - create real musical progressions
        if (pageOffset > 0) {
          // For rhythm channels (1-4), use conservative pitch variations
          if (channel >= 1 && channel <= 4) {
            noteRow = createRhythmicProgression(baseNoteRow, pageOffset, harmony);
          } else {
            // For melodic channels, use full harmonic progressions
            noteRow = createHarmonicProgression(baseNoteRow, pageOffset, harmony);
          }
        }
      
      // Use base pattern velocity with slight variation
      int baseVelocity = basePattern->velocities[beatPosition - 1];
      int velocity = baseVelocity + random(-10, 11);
      if (velocity < 40) velocity = 40;
      if (velocity > 127) velocity = 127;
      
      // Only place note if slot is empty (channel == 0)
      if(note[c][noteRow].channel == 0) {
        note[c][noteRow].channel = channel;
        note[c][noteRow].velocity = velocity;
      }
    }
  }
}

// Context-aware melodic pattern generation that preserves base page melody
void generateContextAwareMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony, BasePagePattern* basePattern, int pageOffset) {
  // Preserve the original melodic structure with harmonic progressions
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = ((x_rel - 1) % maxX) + 1;
    
    // Use the base pattern rhythm as foundation
    bool shouldPlay = basePattern->rhythmPattern[beatPosition - 1] == 1;
    
    // Add subtle variations based on page progression
    if (shouldPlay) {
      // Occasionally skip a note for variation (6% chance - subtle)
      if (random(0, 100) < 6) {
        shouldPlay = false;
      }
    } else {
      // Occasionally add a note for variation (3% chance - subtle)
      if (random(0, 100) < 3) {
        shouldPlay = true;
      }
    }
    
    if (shouldPlay) {
      // Use the base pattern's note row with harmonic progression
      int baseNoteRow = basePattern->noteRows[beatPosition - 1];
      int noteRow = baseNoteRow;
      
      // Apply harmonic progression based on page offset - create real musical progressions
      if (pageOffset > 0) {
        // For rhythm channels (1-4), use conservative pitch variations
        if (channel >= 1 && channel <= 4) {
          noteRow = createRhythmicProgression(baseNoteRow, pageOffset, harmony);
        } else {
          // For melodic channels, use full melodic progressions
          noteRow = createMelodicProgression(baseNoteRow, pageOffset, harmony);
        }
      }
      
      // Use base pattern velocity with slight variation
      int baseVelocity = basePattern->velocities[beatPosition - 1];
      int velocity = baseVelocity + random(-8, 9);
      if (velocity < 40) velocity = 40;
      if (velocity > 127) velocity = 127;
      
      // Only place note if slot is empty (channel == 0)
      if(note[c][noteRow].channel == 0) {
        note[c][noteRow].channel = channel;
        note[c][noteRow].velocity = velocity;
      }
    }
  }
}

// Create dynamic harmonic progressions with more variation
int createHarmonicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {
  // Analyze the base note's harmonic function within the detected scale
  int scaleNotes[16];
  int scaleSize = 0;
  
  // Build scale based on harmony analysis
  if (harmony.isMajor) {
    // Major scale: C D E F G A B C (1 3 5 6 8 10 12 13)
    int majorScale[] = {1, 3, 5, 6, 8, 10, 12, 13};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = majorScale[i];
    }
    scaleSize = 8;
  } else if (harmony.isMinor) {
    // Minor scale: A B C D E F G A (6 8 10 11 13 15 1 3)
    int minorScale[] = {6, 8, 10, 11, 13, 15, 1, 3};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = minorScale[i];
    }
    scaleSize = 8;
  } else {
    // Pentatonic scale as fallback: C D E G A (1 3 5 8 10)
    int pentatonic[] = {1, 3, 5, 8, 10};
    for (int i = 0; i < 5; i++) {
      scaleNotes[i] = pentatonic[i];
    }
    scaleSize = 5;
  }
  
  // Find the base note's position in the scale
  int scaleDegree = -1;
  for (int i = 0; i < scaleSize; i++) {
    if (baseNote == scaleNotes[i]) {
      scaleDegree = i;
      break;
    }
  }
  
  // If base note not in scale, find closest
  if (scaleDegree == -1) {
    int minDistance = 16;
    for (int i = 0; i < scaleSize; i++) {
      int distance = abs(baseNote - scaleNotes[i]);
      if (distance < minDistance) {
        minDistance = distance;
        scaleDegree = i;
      }
    }
  }
  
  // Create subtle harmonic progressions that preserve character
  // Use gentle, character-preserving progressions
  int progressionPatterns[][8] = {
    {0, 5, 3, 4, 0, 5, 3, 4},  // I-vi-IV-V (classic, gentle)
    {0, 4, 5, 3, 0, 4, 5, 3},  // I-V-vi-IV (gentle pop progression)
    {5, 3, 0, 4, 5, 3, 0, 4},  // vi-IV-I-V (minor start, gentle)
    {0, 2, 3, 4, 0, 2, 3, 4},  // I-iii-IV-V (major with gentle third)
    {0, 4, 2, 3, 0, 4, 2, 3},  // I-V-iii-IV (gentle major progression)
    {0, 1, 3, 4, 0, 1, 3, 4},  // I-ii-IV-V (gentle with minor second)
    {0, 3, 4, 5, 0, 3, 4, 5},  // I-IV-V-vi (gentle major progression)
    {0, 4, 1, 3, 0, 4, 1, 3}   // I-V-ii-IV (gentle with minor second)
  };
  
  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 8;
  int targetDegree = progressionPatterns[patternIndex][progressionStep];
  
  // Add subtle randomness for gentle variation
  if (random(0, 100) < 10) { // 10% chance for subtle variation
    targetDegree += random(-1, 2); // Very gentle offset
  }
  
  // Apply the progression
  int newScaleDegree = (scaleDegree + targetDegree) % scaleSize;
  if (newScaleDegree < 0) newScaleDegree += scaleSize;
  
  int resultNote = scaleNotes[newScaleDegree];
  
  // Add gentle octave variations (much less frequent)
  if (random(0, 100) < 8) { // 8% chance for octave change
    if (random(0, 2) == 0) {
      resultNote += 7; // Up an octave
      if (resultNote > 16) resultNote -= 7; // Keep in range
    } else {
      resultNote -= 7; // Down an octave
      if (resultNote < 1) resultNote += 7; // Keep in range
    }
  }
  
  return resultNote;
}

// Create dynamic melodic progressions with more variation and excitement
int createMelodicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {
  // Analyze the base note's harmonic function
  int scaleNotes[16];
  int scaleSize = 0;
  
  // Build scale based on harmony analysis
  if (harmony.isMajor) {
    int majorScale[] = {1, 3, 5, 6, 8, 10, 12, 13};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = majorScale[i];
    }
    scaleSize = 8;
  } else if (harmony.isMinor) {
    int minorScale[] = {6, 8, 10, 11, 13, 15, 1, 3};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = minorScale[i];
    }
    scaleSize = 8;
  } else {
    int pentatonic[] = {1, 3, 5, 8, 10};
    for (int i = 0; i < 5; i++) {
      scaleNotes[i] = pentatonic[i];
    }
    scaleSize = 5;
  }
  
  // Find the base note's position in the scale
  int scaleDegree = -1;
  for (int i = 0; i < scaleSize; i++) {
    if (baseNote == scaleNotes[i]) {
      scaleDegree = i;
      break;
    }
  }
  
  // If base note not in scale, find closest
  if (scaleDegree == -1) {
    int minDistance = 16;
    for (int i = 0; i < scaleSize; i++) {
      int distance = abs(baseNote - scaleNotes[i]);
      if (distance < minDistance) {
        minDistance = distance;
        scaleDegree = i;
      }
    }
  }
  
  // Create gentle melodic progressions that preserve character
  // Use subtle, character-preserving melodic patterns
  int melodicPatterns[][8] = {
    {0, 1, 2, 1, 0, -1, -2, -1},  // Gentle stepwise motion
    {0, 2, 4, 2, 0, -2, -4, -2},  // Gentle thirds
    {0, 4, 2, 6, 4, 0, -2, 2},    // Gentle arpeggio-like
    {0, 1, 3, 2, 4, 3, 5, 4},    // Gentle chromatic with scale notes
    {0, 2, 1, 3, 2, 4, 3, 5},    // Gentle alternating motion
    {0, 3, 1, 4, 2, 5, 3, 6},    // Gentle mixed intervals
    {0, -1, 1, -2, 2, -3, 3, -4}, // Gentle oscillating
    {0, 1, 0, 2, 1, 3, 2, 4}     // Gentle gradual ascent
  };
  
  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 8;
  int melodicOffset = melodicPatterns[patternIndex][progressionStep];
  
  // Add subtle randomness for gentle variation
  if (random(0, 100) < 12) { // 12% chance for subtle variation
    melodicOffset += random(-1, 2); // Very gentle offset
  }
  
  // Apply the melodic progression
  int newScaleDegree = (scaleDegree + melodicOffset) % scaleSize;
  if (newScaleDegree < 0) newScaleDegree += scaleSize;
  
  int resultNote = scaleNotes[newScaleDegree];
  
  // Add gentle octave variations (much less frequent)
  if (random(0, 100) < 6) { // 6% chance for octave change
    if (random(0, 2) == 0) {
      resultNote += 7; // Up an octave
      if (resultNote > 16) resultNote -= 7; // Keep in range
    } else {
      resultNote -= 7; // Down an octave
      if (resultNote < 1) resultNote += 7; // Keep in range
    }
  }
  
  // Add very subtle chromatic passing tones
  if (random(0, 100) < 5) { // 5% chance for chromatic variation
    if (random(0, 2) == 0) {
      resultNote += 1; // Up a semitone
      if (resultNote > 16) resultNote -= 1;
    } else {
      resultNote -= 1; // Down a semitone
      if (resultNote < 1) resultNote += 1;
    }
  }
  
  return resultNote;
}

// Create very conservative rhythmic progressions that stay close to original pitch
int createRhythmicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {
  // For rhythm channels, we want to stay very close to the original pitch
  // Only make minimal changes to maintain the rhythmic character
  
  // Very conservative pitch variations - mostly stay on the same note
  int rhythmicVariations[][4] = {
    {0, 0, 0, 0},    // Stay exactly the same
    {0, 1, 0, -1},   // Very gentle up/down motion
    {0, 0, 1, 0},    // Occasional step up
    {0, 0, -1, 0},   // Occasional step down
    {0, 2, 0, -2},   // Very gentle third motion
    {0, 0, 2, 0},    // Occasional third up
    {0, 0, -2, 0},   // Occasional third down
    {0, 1, -1, 0}    // Gentle up then down
  };
  
  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 4;
  int pitchOffset = rhythmicVariations[patternIndex][progressionStep];
  
  // Add very minimal randomness (much less than melodic channels)
  if (random(0, 100) < 5) { // Only 5% chance for variation
    pitchOffset += random(-1, 2); // Very gentle offset
  }
  
  // Apply the minimal progression
  int resultNote = baseNote + pitchOffset;
  
  // Keep within bounds
  if (resultNote < 1) resultNote = 1;
  if (resultNote > 16) resultNote = 16;
  
  // Very rarely add octave variation (much less than melodic channels)
  if (random(0, 100) < 2) { // Only 2% chance for octave change
    if (random(0, 2) == 0) {
      resultNote += 7; // Up an octave
      if (resultNote > 16) resultNote -= 7; // Keep in range
    } else {
      resultNote -= 7; // Down an octave
      if (resultNote < 1) resultNote += 7; // Keep in range
    }
  }
  
  return resultNote;
}

// Generate genre-based track from page 1 to selected length
void generateGenreTrack() {
  // Clear ALL pages first (complete reset)
  for (unsigned int page = 1; page <= maxPages; page++) {
    unsigned int start = (page - 1) * 16 + 1;
    unsigned int end = page * 16;
    for (unsigned int c = start; c <= end; c++) {
      for (int row = 1; row <= 16; row++) {
        note[c][row].channel = 0;
        note[c][row].velocity = 0;
      }
    }
  }
  
  // Generate genre-specific patterns ONLY from page 1 to genreLength (exact count)
  for (unsigned int page = 1; page <= genreLength; page++) {
    unsigned int start = (page - 1) * 16 + 1;
    unsigned int end = page * 16;
    
    switch (genreType) {
      case 0: // BLNK - Already cleared above
        break;
        
      case 1: // TECH - Techno patterns
        generateTechnoPattern(start, end, page);
        break;
        
      case 2: // HIPH - Hip-hop patterns
        generateHipHopPattern(start, end, page);
        break;
        
      case 3: // DNB - Drum & Bass patterns
        generateDnBPattern(start, end, page);
        break;
        
      case 4: // HOUS - House patterns
        generateHousePattern(start, end, page);
        break;
        
      case 5: // AMBT - Ambient patterns
        generateAmbientPattern(start, end, page);
        break;
    }
  }
  
  // Set genre-appropriate BPM
  setGenreBPM();
  
  // Auto close menu after generation
  switchMode(&draw);
}

// Set genre-appropriate BPM
void setGenreBPM() {
  int targetBPM = 100; // Default BPM
  
  switch (genreType) {
    case 0: // BLNK
      targetBPM = 100; // Default
      break;
    case 1: // TECH - Techno
      targetBPM = 128 + random(-4, 5); // 124-132 BPM
      break;
    case 2: // HIPH - Hip-hop
      targetBPM = 85 + random(-3, 4); // 82-88 BPM
      break;
    case 3: // DNB - Drum & Bass
      targetBPM = 174 + random(-4, 5); // 170-178 BPM
      break;
    case 4: // HOUS - House
      targetBPM = 124 + random(-4, 5); // 120-128 BPM
      break;
    case 5: // AMBT - Ambient
      targetBPM = 70 + random(-5, 6); // 65-75 BPM
      break;
  }
  
  // Set the BPM directly in SMP.bpm and update the volume_bpm mode
  SMP.bpm = targetBPM;
  
  // Update the volume_bpm mode position
  volume_bpm.pos[3] = targetBPM;
  
  // If currently in volume_bpm mode, update the encoder
  if (currentMode == &volume_bpm) {
    Encoder[3].writeCounter((int32_t)targetBPM);
  }
  
  // Apply the BPM immediately by updating the playback timer
  applyBPMDirectly(targetBPM);
}

// Apply BPM directly without relying on MIDI clock condition
void applyBPMDirectly(int bpm) {
  if (bpm > 0) { // Avoid division by zero
    playNoteInterval = ((60.0 * 1000.0 / bpm) / 4.0) * 1000.0;  // Use floats for precision
    playTimer.update(playNoteInterval);
  }
}

// Techno pattern generation - Enhanced and dynamic
void generateTechnoPattern(unsigned int start, unsigned int end, unsigned int page) {
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    
    // Dynamic kick patterns - varies by page
    bool kickPlay = false;
    if (pageOffset % 4 == 0) {
      // Standard four-on-floor
      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 4 == 1) {
      // Techno variation: 1, 3, 5, 7, 9, 11, 13, 15
      kickPlay = (beat % 2 == 1);
    } else if (pageOffset % 4 == 2) {
      // Break pattern: 1, 4, 7, 9, 12, 15
      kickPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 9 || beat == 12 || beat == 15);
    } else {
      // Complex: 1, 2, 5, 6, 9, 10, 13, 14
      kickPlay = (beat == 1 || beat == 2 || beat == 5 || beat == 6 || beat == 9 || beat == 10 || beat == 13 || beat == 14);
    }
    
    if (kickPlay) {
      note[c][3].channel = 1; // Kick
      note[c][3].velocity = 100 + random(-15, 16);
    }
    
    // Dynamic snare patterns
    bool snarePlay = false;
    if (pageOffset % 3 == 0) {
      // Standard backbeat
      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {
      // Ghost snares
      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 60 + random(-10, 11); // Ghost
      } else {
        note[c][5].velocity = 80 + random(-10, 11); // Main
      }
    } else {
      // Roll pattern
      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 50 + random(-10, 11); // Roll
      } else {
        note[c][5].velocity = 75 + random(-10, 11); // Main
      }
    }
    
    if (snarePlay) {
      note[c][5].channel = 2; // Snare
    }
    
    // Complex hi-hat patterns
    if (beat % 2 == 0) {
      note[c][7].channel = 3; // Closed hi-hat
      note[c][7].velocity = 60 + random(-10, 11);
    }
    
    // Open hi-hats
    if (beat == 5 || beat == 13) {
      note[c][8].channel = 4; // Open hi-hat
      note[c][8].velocity = 70 + random(-10, 11);
    }
    
    // Additional percussion
    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      note[c][6].channel = 5; // Additional percussion
      note[c][6].velocity = 45 + random(-10, 11);
    }
    
    // Dynamic bass lines
    bool bassPlay = false;
    if (pageOffset % 2 == 0) {
      // Standard techno bass
      bassPlay = (beat == 1 || beat == 9);
    } else {
      // Complex bass pattern
      bassPlay = (beat == 1 || beat == 3 || beat == 5 || beat == 7 || beat == 9 || beat == 11 || beat == 13 || beat == 15);
    }
    
    if (bassPlay) {
      note[c][2].channel = 6; // Bass
      note[c][2].velocity = 90 + random(-15, 16);
    }
    
    // Melodic elements
    if (random(0, 100) < 25) { // 25% chance
      note[c][9].channel = 7; // Melodic element
      note[c][9].velocity = 65 + random(-10, 11);
    }
    
    // Additional stabs
    if (random(0, 100) < 15) { // 15% chance
      note[c][10].channel = 8; // Stab
      note[c][10].velocity = 75 + random(-10, 11);
    }
  }
}

// Hip-hop pattern generation - Enhanced and dynamic
void generateHipHopPattern(unsigned int start, unsigned int end, unsigned int page) {
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    
    // Dynamic kick patterns - varies by page
    bool kickPlay = false;
    if (pageOffset % 4 == 0) {
      // Classic hip-hop: 1, 7, 11
      kickPlay = (beat == 1 || beat == 7 || beat == 11);
    } else if (pageOffset % 4 == 1) {
      // Trap variation: 1, 3, 7, 11, 15
      kickPlay = (beat == 1 || beat == 3 || beat == 7 || beat == 11 || beat == 15);
    } else if (pageOffset % 4 == 2) {
      // Complex: 1, 4, 7, 10, 13
      kickPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 10 || beat == 13);
    } else {
      // Break pattern: 1, 2, 7, 8, 11, 12
      kickPlay = (beat == 1 || beat == 2 || beat == 7 || beat == 8 || beat == 11 || beat == 12);
    }
    
    if (kickPlay) {
      note[c][3].channel = 1; // Kick
      note[c][3].velocity = 110 + random(-15, 16);
    }
    
    // Dynamic snare patterns
    bool snarePlay = false;
    if (pageOffset % 3 == 0) {
      // Standard backbeat
      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {
      // Ghost snares
      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 70 + random(-10, 11); // Ghost
      } else {
        note[c][5].velocity = 100 + random(-10, 11); // Main
      }
    } else {
      // Roll pattern
      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 60 + random(-10, 11); // Roll
      } else {
        note[c][5].velocity = 95 + random(-10, 11); // Main
      }
    }
    
    if (snarePlay) {
      note[c][5].channel = 2; // Snare
    }
    
    // Complex hi-hat patterns
    if (beat % 2 == 0) {
      note[c][7].channel = 3; // Closed hi-hat
      note[c][7].velocity = 70 + random(-10, 11);
    }
    
    // Open hi-hats
    if (beat == 5 || beat == 13) {
      note[c][8].channel = 4; // Open hi-hat
      note[c][8].velocity = 80 + random(-10, 11);
    }
    
    // Additional percussion
    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      note[c][6].channel = 5; // Additional percussion
      note[c][6].velocity = 50 + random(-10, 11);
    }
    
    // Dynamic bass lines
    bool bassPlay = false;
    if (pageOffset % 2 == 0) {
      // Standard hip-hop bass
      bassPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else {
      // Complex bass pattern
      bassPlay = (beat == 1 || beat == 3 || beat == 5 || beat == 7 || beat == 9 || beat == 11 || beat == 13 || beat == 15);
    }
    
    if (bassPlay) {
      note[c][2].channel = 6; // Bass
      note[c][2].velocity = 85 + random(-15, 16);
    }
    
    // Melodic elements
    if (random(0, 100) < 20) { // 20% chance
      note[c][9].channel = 7; // Melodic element
      note[c][9].velocity = 70 + random(-10, 11);
    }
    
    // Additional stabs
    if (random(0, 100) < 12) { // 12% chance
      note[c][10].channel = 8; // Stab
      note[c][10].velocity = 80 + random(-10, 11);
    }
  }
}

// Drum & Bass pattern generation - More dynamic and complex
void generateDnBPattern(unsigned int start, unsigned int end, unsigned int page) {
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1; // For variation across pages
    
    // Dynamic kick pattern - varies by page
    bool kickPlay = false;
    if (pageOffset % 4 == 0) {
      // Standard DnB: 1, 9
      kickPlay = (beat == 1 || beat == 9);
    } else if (pageOffset % 4 == 1) {
      // Variation: 1, 5, 9, 13 (four-on-floor)
      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 4 == 2) {
      // Break pattern: 1, 7, 11
      kickPlay = (beat == 1 || beat == 7 || beat == 11);
    } else {
      // Complex: 1, 3, 9, 11
      kickPlay = (beat == 1 || beat == 3 || beat == 9 || beat == 11);
    }
    
    if (kickPlay) {
      note[c][3].channel = 1; // Kick
      note[c][3].velocity = 120 + random(-15, 16);
    }
    
    // Dynamic snare pattern
    bool snarePlay = false;
    if (pageOffset % 3 == 0) {
      // Standard backbeat: 5, 13
      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {
      // Ghost snares: 3, 5, 11, 13
      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 70 + random(-10, 11); // Ghost snare
      } else {
        note[c][5].velocity = 110 + random(-10, 11); // Main snare
      }
    } else {
      // Roll pattern: 4, 5, 6, 12, 13, 14
      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 60 + random(-10, 11); // Roll notes
      } else {
        note[c][5].velocity = 100 + random(-10, 11); // Main snares
      }
    }
    
    if (snarePlay) {
      note[c][5].channel = 2; // Snare
    }
    
    // Complex hi-hat patterns - multiple layers
    if (beat % 2 == 0) {
      note[c][7].channel = 3; // Closed hi-hat
      note[c][7].velocity = 70 + random(-10, 11);
    }
    
    // Open hi-hats on snare beats
    if (beat == 5 || beat == 13) {
      note[c][8].channel = 4; // Open hi-hat
      note[c][8].velocity = 80 + random(-10, 11);
    }
    
    // Additional percussion layer
    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      note[c][6].channel = 5; // Additional percussion
      note[c][6].velocity = 50 + random(-10, 11);
    }
    
    // Dynamic bass line - varies by page
    bool bassPlay = false;
    if (pageOffset % 2 == 0) {
      // Standard DnB: every odd beat
      bassPlay = (beat % 2 == 1);
    } else {
      // Complex: syncopated pattern
      bassPlay = (beat == 1 || beat == 3 || beat == 6 || beat == 9 || beat == 11 || beat == 14);
    }
    
    if (bassPlay) {
      note[c][2].channel = 6; // Bass
      note[c][2].velocity = 95 + random(-15, 16);
    }
    
    // Additional melodic elements
    if (random(0, 100) < 15) { // 15% chance
      note[c][9].channel = 7; // Melodic element
      note[c][9].velocity = 60 + random(-10, 11);
    }
  }
}

// House pattern generation - More dynamic and groovy
void generateHousePattern(unsigned int start, unsigned int end, unsigned int page) {
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1; // For variation across pages
    
    // Dynamic kick pattern - varies by page
    bool kickPlay = false;
    if (pageOffset % 3 == 0) {
      // Standard house: four-on-the-floor
      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 3 == 1) {
      // Groove variation: 1, 3, 5, 7, 9, 11, 13, 15
      kickPlay = (beat % 2 == 1);
    } else {
      // Break pattern: 1, 5, 7, 9, 13, 15
      kickPlay = (beat == 1 || beat == 5 || beat == 7 || beat == 9 || beat == 13 || beat == 15);
    }
    
    if (kickPlay) {
      note[c][3].channel = 1; // Kick
      note[c][3].velocity = 95 + random(-15, 16);
    }
    
    // Dynamic snare pattern
    bool snarePlay = false;
    if (pageOffset % 4 == 0) {
      // Standard backbeat: 5, 13
      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 4 == 1) {
      // Ghost snares: 3, 5, 11, 13
      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 60 + random(-10, 11); // Ghost snare
      } else {
        note[c][5].velocity = 85 + random(-10, 11); // Main snare
      }
    } else if (pageOffset % 4 == 2) {
      // Roll pattern: 4, 5, 6, 12, 13, 14
      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 50 + random(-10, 11); // Roll notes
      } else {
        note[c][5].velocity = 80 + random(-10, 11); // Main snares
      }
    } else {
      // Complex: 2, 5, 8, 13
      snarePlay = (beat == 2 || beat == 5 || beat == 8 || beat == 13);
    }
    
    if (snarePlay) {
      note[c][5].channel = 2; // Snare
    }
    
    // Dynamic hi-hat patterns
    if (beat % 2 == 0) {
      note[c][7].channel = 3; // Closed hi-hat
      note[c][7].velocity = 65 + random(-10, 11);
    }
    
    // Open hi-hats - varies by page
    bool openHatPlay = false;
    if (pageOffset % 2 == 0) {
      // Standard: on snare beats
      openHatPlay = (beat == 5 || beat == 13);
    } else {
      // Variation: 3, 5, 11, 13
      openHatPlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
    }
    
    if (openHatPlay) {
      note[c][8].channel = 4; // Open hi-hat
      note[c][8].velocity = 75 + random(-10, 11);
    }
    
    // Additional percussion layer
    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      note[c][6].channel = 5; // Additional percussion
      note[c][6].velocity = 45 + random(-10, 11);
    }
    
    // Dynamic bass line - varies by page
    bool bassPlay = false;
    if (pageOffset % 3 == 0) {
      // Standard house: 1, 9
      bassPlay = (beat == 1 || beat == 9);
    } else if (pageOffset % 3 == 1) {
      // Groove: 1, 3, 5, 7, 9, 11, 13, 15
      bassPlay = (beat % 2 == 1);
    } else {
      // Complex: 1, 4, 7, 9, 12, 15
      bassPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 9 || beat == 12 || beat == 15);
    }
    
    if (bassPlay) {
      note[c][2].channel = 6; // Bass
      note[c][2].velocity = 90 + random(-15, 16);
    }
    
    // Melodic elements - house style
    if (random(0, 100) < 20) { // 20% chance
      note[c][9].channel = 7; // Melodic element
      note[c][9].velocity = 70 + random(-10, 11);
    }
    
    // Additional stabs
    if (random(0, 100) < 10) { // 10% chance
      note[c][10].channel = 8; // Stab
      note[c][10].velocity = 80 + random(-10, 11);
    }
  }
}

// Ambient pattern generation - Enhanced and atmospheric
void generateAmbientPattern(unsigned int start, unsigned int end, unsigned int page) {
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    
    // Dynamic atmospheric patterns - varies by page
    if (pageOffset % 3 == 0) {
      // Sparse, ethereal
      if (random(0, 100) < 15) { // 15% chance
        int channel = random(5, 9); // Use melodic channels
        int row = random(8, 13); // Higher register
        note[c][row].channel = channel;
        note[c][row].velocity = 30 + random(0, 25); // Very soft
      }
    } else if (pageOffset % 3 == 1) {
      // More active, but still ambient
      if (random(0, 100) < 25) { // 25% chance
        int channel = random(5, 9); // Use melodic channels
        int row = random(7, 12); // Mid to high register
        note[c][row].channel = channel;
        note[c][row].velocity = 40 + random(0, 30); // Soft
      }
    } else {
      // Dense but soft
      if (random(0, 100) < 35) { // 35% chance
        int channel = random(5, 9); // Use melodic channels
        int row = random(6, 11); // Mid register
        note[c][row].channel = channel;
        note[c][row].velocity = 35 + random(0, 35); // Soft to medium
      }
    }
    
    // Soft percussion - varies by page
    if (pageOffset % 2 == 0) {
      // Very sparse percussion
      if (random(0, 100) < 8) { // 8% chance
        note[c][6].channel = 3; // Soft hi-hat
        note[c][6].velocity = 25 + random(0, 15);
      }
    } else {
      // More active percussion
      if (random(0, 100) < 15) { // 15% chance
        note[c][6].channel = 3; // Soft hi-hat
        note[c][6].velocity = 30 + random(0, 20);
      }
    }
    
    // Occasional soft bass
    if (random(0, 100) < 12) { // 12% chance
      note[c][4].channel = 6; // Soft bass
      note[c][4].velocity = 35 + random(0, 25);
    }
    
    // Atmospheric pads
    if (random(0, 100) < 18) { // 18% chance
      note[c][10].channel = 7; // Pad
      note[c][10].velocity = 30 + random(0, 20);
    }
    
    // Occasional soft snare
    if (random(0, 100) < 5) { // 5% chance
      note[c][5].channel = 2; // Soft snare
      note[c][5].velocity = 25 + random(0, 15);
    }
  }
}

// Fallback basic pattern for other channels
void generateBasicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Use simplified version of original logic
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    
    // Strong beats get notes
    if(x_rel == 1 || x_rel == 5 || x_rel == 9 || x_rel == 13) {
      int noteRow = random(1, 9); // Random note in lower register
      
      // Only place note if slot is empty (channel == 0)
      if(note[c][noteRow].channel == 0) {
        note[c][noteRow].channel = channel;
        note[c][noteRow].velocity = defaultVelocity;
      }
    }
  }
}






void clearPage() {
  //GLOB.edit = 1;

  unsigned int start = ((GLOB.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  bool singleMode = GLOB.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();
  
  // Reset paint/unpaint prevention flag after clearPage operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}

void clearPageX(int thatpage) {
  //GLOB.edit = 1;

  unsigned int start = ((thatpage - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  bool singleMode = GLOB.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();
}


// Reset all audio effects/filters to clean defaults
void resetAllAudioEffects() {
  Serial.println("Resetting audio hardware...");
  
  extern const int ALL_CHANNELS[];
  extern const int NUM_ALL_CHANNELS;
  
  // Step 1: Reset all filter mixer gains and audio objects to clean state
  for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
    int idx = ALL_CHANNELS[i];
    setFilterDefaults(idx);  // Resets mixers, filters, reverbs, bitcrushers to default
  }
  
  // Step 2: Reset drum engine defaults
  setDrumDefaults(true);
  
  // Step 3: Force all mixer gains to target (ensures smooth transitions are off)
  forceAllMixerGainsToTarget();
  
  // Step 4: Apply reset data from SMP to audio hardware
  Serial.println("Applying reset data to audio hardware...");
  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);
  
  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    
    // Apply filters to hardware (setFilters reads from SMP.filter_settings and applies)
    setFilters(PASS, ch, true);
    setFilters(FREQUENCY, ch, true);
    setFilters(REVERB, ch, true);
    setFilters(BITCRUSHER, ch, true);
    setFilters(DETUNE, ch, true);
    setFilters(OCTAVE, ch, true);
    
    // Apply parameters to hardware (setParams reads from SMP.param_settings and applies)
    setParams(ATTACK, ch);
    setParams(DECAY, ch);
    setParams(SUSTAIN, ch);
    setParams(RELEASE, ch);
    
    // Apply drums to hardware (only for channels 1-3)
    if (ch >= 1 && ch <= 3) {
      setDrums(DRUMTONE, ch);
      setDrums(DRUMDECAY, ch);
      setDrums(DRUMPITCH, ch);
      setDrums(DRUMTYPE, ch);
    }
    
    // Update synth voice (only for channel 11)
    if (ch == 11) {
      updateSynthVoice(11);
    }
  }
  
  Serial.println("Audio hardware reset complete.");
}


// Complete reset for NEW start - clears everything and loads fresh defaults
void startNew() {
  // Declare all external variables at the beginning
  extern unsigned int samplePackID;
  extern float playNoteInterval;
  extern IntervalTimer playTimer;
  extern bool globalMutes[maxY];
  extern bool pageMutes[maxPages][maxY];
  extern uint8_t songArrangement[64];
  extern bool songModeActive;
  extern bool preventPaintUnpaint;
  extern int patternMode;
  extern uint8_t filterPage[NUM_CHANNELS];
  extern const int ALL_CHANNELS[];
  extern const int NUM_ALL_CHANNELS;
  extern Mode draw;
  extern Mode volume_bpm;
  extern Mode *currentMode;
  extern i2cEncoderLibV2 Encoder[NUM_ENCODERS];
  extern const CRGB col[];
  extern bool isNowPlaying;
  
  Serial.println("=== START NEW - Complete Reset ===");
  
  // 0. STOP PLAYBACK FIRST (if playing)
  if (isNowPlaying) {
    Serial.println("Stopping playback...");
    isNowPlaying = false;
  }
  
  // Visual feedback - clear display and show message
  FastLEDclear();
  drawText("NEW", 6, 8, CRGB(0, 255, 255));
  FastLEDshow();
  
  // 1. CLEAR ALL NOTES (all pages, all channels, all velocities, all probabilities)
  Serial.println("Clearing all notes...");
  for (unsigned int x = 0; x <= maxlen; x++) {
    for (unsigned int y = 0; y <= maxY; y++) {
      note[x][y].channel = 0;
      note[x][y].velocity = defaultVelocity;  // Reset to default velocity
      note[x][y].probability = 100;  // Reset to 100% probability
    }
  }
  
  // 2. RESET GLOBAL VARIABLES FIRST (before using GLOB.currentChannel)
  Serial.println("Resetting global variables...");
  SMP.bpm = 100.0;
  GLOB.vol = 10;
  GLOB.velocity = 10;
  GLOB.currentChannel = 1;  // Set this FIRST before using it
  GLOB.page = 1;
  GLOB.edit = 1;
  GLOB.singleMode = false;
  GLOB.x = 1;
  GLOB.y = 1;
  
  // 3. RESET ALL FILTER/PARAMETER DATA (just data, no audio hardware yet)
  Serial.println("Resetting filter/parameter data...");
  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);
  
  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    
    // Reset channel volume
    SMP.channelVol[ch] = 10;
    
    // Reset filter data (no hardware calls)
    SMP.filter_settings[ch][PASS] = 15;
    SMP.filter_settings[ch][FREQUENCY] = 0;
    SMP.filter_settings[ch][REVERB] = 0;
    SMP.filter_settings[ch][BITCRUSHER] = 0;
    SMP.filter_settings[ch][DETUNE] = 16;
    SMP.filter_settings[ch][OCTAVE] = 16;
    SMP.filter_settings[ch][EFX] = 0;  // Sample mode
    
    // Reset parameter data (no hardware calls)
    SMP.param_settings[ch][ATTACK] = 32;
    SMP.param_settings[ch][DECAY] = 0;
    SMP.param_settings[ch][SUSTAIN] = 10;
    SMP.param_settings[ch][RELEASE] = 5;
    
    // Reset drum data (only for channels 1-3)
    if (ch >= 1 && ch <= 3) {
      SMP.drum_settings[ch][DRUMTONE] = 0;
      SMP.drum_settings[ch][DRUMDECAY] = 16;
      SMP.drum_settings[ch][DRUMPITCH] = 16;
      SMP.drum_settings[ch][DRUMTYPE] = 1;
    }
    
    // Reset synth data (only for channels 11, 13-14)
    if (ch == 11 || ch == 13 || ch == 14) {
      SMP.synth_settings[ch][CUTOFF] = 0;
      SMP.synth_settings[ch][RESONANCE] = 0;
      SMP.synth_settings[ch][FILTER] = 0;
      SMP.synth_settings[ch][CENT] = 16;
      SMP.synth_settings[ch][SEMI] = 0;
      SMP.synth_settings[ch][INSTRUMENT] = 0;
      SMP.synth_settings[ch][FORM] = 0;
    }
  }
  
  // 4. CLEAR ALL MUTE STATES
  Serial.println("Clearing all mute states...");
  for (int ch = 0; ch < maxY; ch++) {
    globalMutes[ch] = false;
    SMP.globalMutes[ch] = false;
    for (int page = 0; page < maxPages; page++) {
      pageMutes[page][ch] = false;
      SMP.pageMutes[page][ch] = false;
    }
    SMP.mute[ch] = 0;
  }
  unmuteAllChannels();
  
  // 5. CLEAR ALL SONG ARRANGEMENT
  Serial.println("Clearing song arrangement...");
  for (int i = 0; i < 64; i++) {
    songArrangement[i] = 0;
    SMP.songArrangement[i] = 0;
  }
  
  // 6. CLEAR ALL SAMPLEPACK 0 (sp0) STATES - Remove all custom samples
  Serial.println("Clearing all sp0Active states...");
  for (int i = 1; i < maxFiles; i++) {
    SMP.sp0Active[i] = false;
  }
  saveSp0StateToEEPROM();
  
  // 7. LOAD SAMPLEPACK 1 (fresh default samples)
  Serial.println("Loading Samplepack 1...");
  SMP.pack = 1;
  samplePackID = 1;
  EEPROM.put(0, (unsigned int)1);  // Save to EEPROM
  loadSamplePack(1, false);
  
  // 8. UPDATE BPM AND TIMER
  Mode *bpm_vol = &volume_bpm;
  bpm_vol->pos[3] = SMP.bpm;
  playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
  playTimer.update(playNoteInterval);
  bpm_vol->pos[2] = GLOB.vol;
  
  // 9. RESET WAVE FILE IDS to defaults
  Serial.println("Resetting wave file IDs...");
  for (int i = 1; i < maxFiles; i++) {
    SMP.wav[i].fileID = i;
    SMP.wav[i].oldID = i;
  }
  
  // 10. UPDATE LAST PAGE (should be 1 since no notes)
  updateLastPage();
  
  // 11. RESET PATTERN MODE FLAGS (before switching mode)
  songModeActive = false;
  SMP_PATTERN_MODE = false;
  patternMode = -1;  // OFF
  
  // Reset paint/unpaint prevention flag
  preventPaintUnpaint = false;
  
  // 12. SWITCH TO DRAW MODE
  Serial.println("Switching to draw mode...");
  delay(300);  // Brief delay to show "NEW" message
  switchMode(&draw);
  
  // 13. SYNC ENCODERS WITH CURRENT POSITION
  // Force encoder positions to match reset state
  Encoder[0].writeCounter((int32_t)GLOB.y);       // Y position (channel)
  Encoder[1].writeCounter((int32_t)GLOB.edit);    // Page
  Encoder[2].writeCounter((int32_t)maxfilterResolution);  // Filter/velocity
  Encoder[3].writeCounter((int32_t)GLOB.x);       // X position
  
  // Update currentMode positions to match
  currentMode->pos[0] = GLOB.y;
  currentMode->pos[1] = GLOB.edit;
  currentMode->pos[2] = maxfilterResolution;
  currentMode->pos[3] = GLOB.x;
  
  // Set encoder colors to match channel 1
  Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
  Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
  
  // 14. RESET ALL AUDIO EFFECTS/FILTERS TO CLEAN DEFAULTS
  resetAllAudioEffects();
  
  // 15. FORCE DISPLAY REFRESH
  // Clear any stale display data and force a complete redraw
  FastLEDclear();
  FastLEDshow();
  
  // 16. AUTOSAVE EMPTY STATE
  // Save the clean/empty state to autosaved.txt
  Serial.println("Autosaving empty state...");
  extern void savePattern(bool autosave);
  savePattern(true);
  
  Serial.println("=== START NEW Complete ===");
  Serial.println("Ready for fresh pattern creation!");
}


int getFolderNumber(int value) {
  int folder = floor(value / 100);
  if (folder > FOLDER_MAX) folder = FOLDER_MAX;
  if (folder <= 0) folder = 0;
  return folder;
}

int getFileNumber(int value) {
  // return the file number in the folder, so 113 = 13, 423 = 23
  int folder = getFolderNumber(value);
  int wavfile = value % 100;
  if (wavfile <= 0) wavfile = 0;
  return wavfile + folder * 100;
}

CRGB getCol(unsigned int g) {
  return col[g] * 10;
}


void setNote(uint16_t step,  uint8_t pitch, uint8_t channel, uint8_t velocity) {
  if (step < maxlen && channel < NUM_CHANNELS) {
    note[step][pitch].channel = channel;
    note[step][pitch].velocity = velocity;
  }
}

// Function to get a note from a given step and channel
Note getNote(uint16_t step, uint8_t channel) {
  if (step < maxlen && channel < NUM_CHANNELS) {
    return note[step][channel];
  }
  // Return a default Note if indices are out-of-range.
  return {0, 0};
}


/***************/
/**** INTRO ****/
/***************/


// ----- Generate Explosion Particles -----
// Called once when Phase 2 starts. Each "1" pixel in the logo becomes a particle.
void generateParticles() {
  particleCount = 0;
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 16; col++) {
      if (logo[row][col] == 1) {
        // Particle's initial position (center of that cell)
        float initX = col + 0.5;
        float initY = row + 0.5;
        // Explosion direction from the center
        float dx = initX - logoCenterX;
        float dy = initY - logoCenterY;
        float length = sqrt(dx * dx + dy * dy);
        float dirX = (length == 0) ? 0 : (dx / length);
        float dirY = (length == 0) ? 0 : (dy / length);
        // Freeze the pixel’s color from timeFactor=1
        CRGB color = getLogoPixelColor(col, row, 1.0);
        particles[particleCount] = { initX, initY, dirX, dirY, color };
        particleCount++;
      }
    }
  }
  particlesGenerated = true;
}



// ----- Determine Color of Each LED Based on Time -----
CRGB getPixelColor(uint8_t x, uint8_t y, unsigned long elapsed) {
  // Convert (x,y) to float center coords
  float cellCenterX = x + 0.5;
  float cellCenterY = y + 0.5;

  if (elapsed < phase1Duration) {
    // PHASE 1: Rainbow Logo
    if (logo[y][x] == 1) {
      float timeFactor = (float)elapsed / phase1Duration;  // 0..1
      return getLogoPixelColor(x, y, timeFactor);
    } else {
      return CRGB::Black;
    }
  } else if (elapsed < totalAnimationTime) {
    
    // PHASE 2: Explosion
    if (!particlesGenerated) {
      //done once
      //playSdWav1.play("intro/008.wav");
      generateParticles();
      initEncoders();
      
    }
    float progress = (float)(elapsed - phase1Duration) / phase2Duration;  // 0..1
    const float maxDisp = 20.0;                                           // how far particles fly outward

    // Check each particle to see if it's near this LED cell
    for (int i = 0; i < particleCount; i++) {
      float px = particles[i].initX + particles[i].dirX * progress * maxDisp;
      float py = particles[i].initY + particles[i].dirY * progress * maxDisp;
      float dist = sqrt((cellCenterX - px) * (cellCenterX - px) + (cellCenterY - py) * (cellCenterY - py));
      if (dist < 0.4) {
        return particles[i].color;
      }
    }
    return CRGB::Black;
  } else {
    // After animation, stay black
    return CRGB::Black;
  }
}








// ----- Run the Animation Once (Called in setup) -----
void runAnimation() {
  // Clear ALL LEDs first to ensure clean start (all matrices stay black)
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLEDshow();
  
  unsigned long startTime = millis();
  unsigned long lastFrameTime = millis();
  const unsigned long frameDelay = 33;  // ~30 FPS for smoother animation
  
  while (true) {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - startTime;
    
    // Frame rate limiting - only update display at consistent intervals
    if (currentTime - lastFrameTime < frameDelay) {
      yield();  // Give CPU time to other tasks
      continue;  // Skip this iteration if not enough time has passed
    }
    lastFrameTime = currentTime;
    
    if (elapsed > totalAnimationTime) {
      elapsed = totalAnimationTime;
    }

    // Update ONLY the first matrix (16×16 grid)
    // Second matrix stays black (already cleared above)
    for (uint8_t y = 0; y < maxY; y++) {
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        CRGB color = getPixelColor(x, y, elapsed);
        // Direct write to LED array for maximum speed
        uint16_t ledIndex;
        if ((y + 1) % 2 == 0) {
          // Even rows: right to left
          ledIndex = (MATRIX_WIDTH - (x + 1)) + (MATRIX_WIDTH * y);
        } else {
          // Odd rows: left to right
          ledIndex = x + (MATRIX_WIDTH * y);
        }
        leds[ledIndex] = color;
      }
    }
    
    FastLEDshow();

    // Stop after totalAnimationTime
    if (elapsed >= totalAnimationTime) {
      break;
    }
  }

  // Clear the display at end - all matrices
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLEDshow();
}

// Enhanced base page pattern analysis structure (moved to top of file)

// AI Song Generation - extends current pattern across multiple pages with context awareness
void generateSong() {
  extern int aiTargetPage; // Access the target page from menu
  extern int aiBaseStartPage; // Access the base start page from menu
  extern int aiBaseEndPage;   // Access the base end page from menu
  
  int currentPage = GLOB.page;
  int targetPage = aiTargetPage;
  
  // Validate target page - ensure it's higher than current page
  if (targetPage <= currentPage) {
    targetPage = currentPage + 1;
  }
  
  // Don't exceed maxPages
  if (targetPage > maxPages) {
    targetPage = maxPages;
  }
  
  // Validate base page range
  if (aiBaseStartPage > aiBaseEndPage) {
    Serial.println("ERROR: Base start page > base end page");
    return;
  }
  
  // Check if base pages have content and analyze patterns
  bool basePagesEmpty = true;
  bool channelsUsed[16] = {false};
  // STACK OVERFLOW FIX: Use global EXTMEM buffer to prevent stack overflow
  BasePagePattern* channelPatterns = g_channelPatterns; // Use global EXTMEM buffer
  
  // Initialize pattern analysis
  for (int ch = 0; ch < 16; ch++) {
    channelPatterns[ch].stepCount = 0;
    channelPatterns[ch].density = 0.0;
    channelPatterns[ch].mostCommonRow = 1;
    channelPatterns[ch].isRhythmic = false;
    channelPatterns[ch].isMelodic = false;
    for (int step = 0; step < 16; step++) {
      channelPatterns[ch].hasNotes[step] = false;
      channelPatterns[ch].noteRows[step] = 0;
      channelPatterns[ch].velocities[step] = defaultVelocity;
      channelPatterns[ch].rhythmPattern[step] = 0;
    }
  }
  
  // Analyze all base pages content with detailed pattern extraction
  for (int basePage = aiBaseStartPage; basePage <= aiBaseEndPage; basePage++) {
    unsigned int pageStartStep = (basePage - 1) * maxX + 1;
    unsigned int pageEndStep = basePage * maxX;
    
    for (unsigned int c = pageStartStep; c <= pageEndStep; c++) {
      for (unsigned int r = 1; r <= maxY; r++) {
        if (note[c][r].channel != 0) {
          basePagesEmpty = false;
          int ch = note[c][r].channel;
          channelsUsed[ch] = true;
          
          // Calculate relative step position within the 16-step pattern
          int relativeStep = ((c - pageStartStep) % 16);
          
          // Record pattern information
          channelPatterns[ch].hasNotes[relativeStep] = true;
          channelPatterns[ch].noteRows[relativeStep] = r;
          channelPatterns[ch].velocities[relativeStep] = note[c][r].velocity;
          channelPatterns[ch].rhythmPattern[relativeStep] = 1;
          channelPatterns[ch].stepCount++;
        }
      }
    }
  }
  
  // Analyze patterns for each used channel
  for (int ch = 1; ch <= 15; ch++) {
    if (channelsUsed[ch]) {
      BasePagePattern* pattern = &channelPatterns[ch];
      
      // Calculate density
      pattern->density = (float)pattern->stepCount / 16.0;
      
      // Find most common note row
      int rowCounts[17] = {0}; // 1-16 for note rows
      for (int step = 0; step < 16; step++) {
        if (pattern->hasNotes[step]) {
          rowCounts[pattern->noteRows[step]]++;
        }
      }
      int maxCount = 0;
      for (int row = 1; row <= 16; row++) {
        if (rowCounts[row] > maxCount) {
          maxCount = rowCounts[row];
          pattern->mostCommonRow = row;
        }
      }
      
      // Determine if rhythmic or melodic based on pattern characteristics
      if (ch >= 1 && ch <= 4) {
        pattern->isRhythmic = true;
        pattern->isMelodic = false;
      } else if (ch >= 5 && ch <= 8) {
        pattern->isRhythmic = false;
        pattern->isMelodic = true;
      } else if (ch == 11) {
        // Channel 11 can be both - determine based on density and note variation
        bool hasNoteVariation = false;
        for (int i = 1; i < 16; i++) {
          if (pattern->hasNotes[i] && pattern->noteRows[i] != pattern->mostCommonRow) {
            hasNoteVariation = true;
            break;
          }
        }
        pattern->isMelodic = hasNoteVariation || pattern->density > 0.3;
        pattern->isRhythmic = !pattern->isMelodic;
      } else {
        // Other channels - analyze based on density and variation
        pattern->isMelodic = pattern->density > 0.2;
        pattern->isRhythmic = !pattern->isMelodic;
      }
      
      Serial.print("Channel ");
      Serial.print(ch);
      Serial.print(": Density=");
      Serial.print(pattern->density);
      Serial.print(", CommonRow=");
      Serial.print(pattern->mostCommonRow);
      Serial.print(", Type=");
      Serial.println(pattern->isMelodic ? "MELODIC" : "RHYTHMIC");
    }
  }
  
  // Debug: Show what musical elements are present in base pages
  Serial.print("Base pages ");
  Serial.print(aiBaseStartPage);
  Serial.print("-");
  Serial.print(aiBaseEndPage);
  Serial.print(" analysis: ");
  int rhythmChannels = 0, melodyChannels = 0, otherChannels = 0;
  for (int ch = 1; ch <= 15; ch++) {
    if (channelsUsed[ch]) {
      if (channelPatterns[ch].isRhythmic) rhythmChannels++;
      else if (channelPatterns[ch].isMelodic) melodyChannels++;
      else otherChannels++;
    }
  }
  Serial.print("Rhythm=");
  Serial.print(rhythmChannels);
  Serial.print(", Melody=");
  Serial.print(melodyChannels);
  Serial.print(", Other=");
  Serial.println(otherChannels);
  
  // Generate pages starting from after the base page range
  int startPage = aiBaseEndPage + 1;
  int endPage = startPage + aiTargetPage - 1;
  
  // Safety check: ensure we don't exceed maxPages
  if (endPage > maxPages) {
    endPage = maxPages;
    Serial.print("WARNING: Limited generation to page ");
    Serial.println(endPage);
  }
  
  // Debug: Show what pages will be generated
  Serial.print("AI Generation: Base pages ");
  Serial.print(aiBaseStartPage);
  Serial.print("-");
  Serial.print(aiBaseEndPage);
  Serial.print(", Additional pages to generate: ");
  Serial.print(aiTargetPage);
  Serial.print(", Start page ");
  Serial.print(startPage);
  Serial.print(", End page ");
  Serial.println(startPage + aiTargetPage - 1);
  Serial.print("Base pages empty: ");
  Serial.println(basePagesEmpty ? "YES" : "NO");
  
  for (int page = startPage; page <= endPage; page++) {
    Serial.print("Generating page ");
    Serial.println(page);
    
    // Save current page context
    int originalPage = GLOB.page;
    
    // Set the page context for generation
    GLOB.page = page;
    
    // Always clear the page since we're generating new content
    // Calculate the step range for this specific page
    unsigned int pageStartStep = (page - 1) * maxX + 1;
    unsigned int pageEndStep = page * maxX;
    
    Serial.print("Clearing page ");
    Serial.print(page);
    Serial.print(" (steps ");
    Serial.print(pageStartStep);
    Serial.print("-");
    Serial.println(pageEndStep);
    
    for (unsigned int c = pageStartStep; c <= pageEndStep; c++) {
      for (unsigned int r = 1; r <= maxY; r++) {
        note[c][r].channel = 0;
        note[c][r].velocity = defaultVelocity;
      }
    }
    
    // Generate pattern for this page
    HarmonicAnalysis harmony;
    
    // Analyze existing harmony from base page for intelligent progression
    harmony.hasRoot = false;
    harmony.hasThird = false;
    harmony.hasFifth = false;
    harmony.hasSeventh = false;
    harmony.rootNote = 1;
    harmony.isMajor = false;
    harmony.isMinor = false;
    
    // Analyze base pages harmony if they exist
    if (!basePagesEmpty) {
      // Find the most common root note across all base pages
      int noteCounts[16] = {0};
      for (int basePage = aiBaseStartPage; basePage <= aiBaseEndPage; basePage++) {
        unsigned int pageStartStep = (basePage - 1) * maxX + 1;
        unsigned int pageEndStep = basePage * maxX;
        
        for (unsigned int c = pageStartStep; c <= pageEndStep; c++) {
          for (unsigned int r = 1; r <= maxY; r++) {
            if (note[c][r].channel != 0) {
              noteCounts[r]++;
            }
          }
        }
      }
      
      // Find the most prominent note as root
      int maxCount = 0;
      for (int i = 1; i <= 16; i++) {
        if (noteCounts[i] > maxCount) {
          maxCount = noteCounts[i];
          harmony.rootNote = i;
        }
      }
    }
    
    // Create harmonic progression based on page number relative to base pages
    if (page > aiBaseEndPage) {
      int pageOffset = page - aiBaseEndPage;
      
      // Common chord progressions: I, ii, iii, IV, V, vi, vii°
      int progression[] = {1, 2, 3, 4, 5, 6, 7, 1}; // I-ii-iii-IV-V-vi-vii°-I
      int progressionIndex = (pageOffset - 1) % 8;
      int chordDegree = progression[progressionIndex];
      
      // Convert chord degree to root note (simplified)
      harmony.rootNote = ((harmony.rootNote + chordDegree - 1) % 8) + 1;
      
      // Determine major/minor based on chord degree
      if (chordDegree == 1 || chordDegree == 4 || chordDegree == 5) {
        harmony.isMajor = true;
        harmony.isMinor = false;
      } else if (chordDegree == 2 || chordDegree == 3 || chordDegree == 6) {
        harmony.isMajor = false;
        harmony.isMinor = true;
      } else {
        // Diminished or other - use alternating pattern
        harmony.isMajor = (page % 2 == 0);
        harmony.isMinor = !harmony.isMajor;
      }
      
      Serial.print("Page ");
      Serial.print(page);
      Serial.print(" harmony: Root=");
      Serial.print(harmony.rootNote);
      Serial.print(", ");
      Serial.println(harmony.isMajor ? "Major" : "Minor");
    }
    
    // Generate patterns for used channels - only for this specific page
    // Use analyzed base page patterns with intelligent variations
    for (int ch = 1; ch <= 15; ch++) {
      if (channelsUsed[ch]) {
        BasePagePattern* basePattern = &channelPatterns[ch];
        
        // Add dynamic variation based on page position relative to base pages
        int pageOffset = page - aiBaseEndPage;
        float intensityMultiplier = 0.8 + (pageOffset * 0.1); // Gradually increase intensity
        if (intensityMultiplier > 1.2) intensityMultiplier = 1.2;
        
        // Generate based on analyzed pattern type
        if (basePattern->isRhythmic) {
          generateContextAwareRhythmicPattern(pageStartStep, pageEndStep, ch, harmony, basePattern, pageOffset);
        } else if (basePattern->isMelodic) {
          generateContextAwareMelodicPattern(pageStartStep, pageEndStep, ch, harmony, basePattern, pageOffset);
        } else {
          // Fallback for other channels
          generateBasicPattern(pageStartStep, pageEndStep, ch, harmony);
        }
      }
    }
    
    // Restore original page context
    GLOB.page = originalPage;
  }
  
  // Auto-close menu and return to main interface
  extern void switchMode(Mode *newMode);
  extern Mode draw;
  switchMode(&draw);
}
