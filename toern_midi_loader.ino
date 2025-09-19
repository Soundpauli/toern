// MIDI File Loader for Toern
// Loads MIDI files and converts them to the internal pattern format

// Channel mapping as specified:
// Channel 1 → Bass Drum (Kick) 
// Channel 2 → Snare 
// Channel 3 → Hi-Hat 
// Channel 4 → Clap 
// Channel 5 → Instrument / Sample 1 
// Channel 6 → Instrument / Sample 2 
// Channel 7 → Instrument / Sample 3 
// Channel 8 → Instrument / Sample 4
// Channel 9 → do not use
// Channel 10 → do not use
// Channel 11 → Bass 
// Channel 12 → do not use 
// Channel 13 → Lead 1 
// Channel 14 → Lead 2

// MIDI note to channel mapping
const int MIDI_CHANNEL_MAP[15] = {
  0,  // Channel 0 - unused
  1,  // Channel 1 - Bass Drum
  2,  // Channel 2 - Snare
  3,  // Channel 3 - Hi-Hat
  4,  // Channel 4 - Clap
  5,  // Channel 5 - Sample 1
  6,  // Channel 6 - Sample 2
  7,  // Channel 7 - Sample 3
  8,  // Channel 8 - Sample 4
  0,  // Channel 9 - unused
  0,  // Channel 10 - unused
  11, // Channel 11 - Bass
  0,  // Channel 12 - unused
  13, // Channel 13 - Lead 1
  14  // Channel 14 - Lead 2
};

// MIDI note to frequency mapping for each channel
// Each channel has different frequency ranges
const int CHANNEL_FREQ_RANGES[15][2] = {
  {0, 0},     // Channel 0 - unused
  {36, 36},   // Channel 1 - Bass Drum (C1)
  {38, 38},   // Channel 2 - Snare (D1)
  {42, 42},   // Channel 3 - Hi-Hat (F#1)
  {39, 39},   // Channel 4 - Clap (D#1)
  {60, 84},   // Channel 5 - Sample 1 (C4 to C6)
  {60, 84},   // Channel 6 - Sample 2 (C4 to C6)
  {60, 84},   // Channel 7 - Sample 3 (C4 to C6)
  {60, 84},   // Channel 8 - Sample 4 (C4 to C6)
  {0, 0},     // Channel 9 - unused
  {0, 0},     // Channel 10 - unused
  {40, 64},   // Channel 11 - Bass (E2 to E4)
  {0, 0},     // Channel 12 - unused
  {72, 96},   // Channel 13 - Lead 1 (C5 to C7)
  {72, 96}    // Channel 14 - Lead 2 (C5 to C7)
};

// Convert MIDI note to internal note index based on channel
int midiNoteToInternalNote(int midiNote, int channel) {
  if (channel < 1 || channel > 14) return 0;
  
  int mappedChannel = MIDI_CHANNEL_MAP[channel];
  if (mappedChannel == 0) return 0; // Unused channel
  
  int minNote = CHANNEL_FREQ_RANGES[mappedChannel][0];
  int maxNote = CHANNEL_FREQ_RANGES[mappedChannel][1];
  
  // Clamp MIDI note to channel range
  if (midiNote < minNote) midiNote = minNote;
  if (midiNote > maxNote) midiNote = maxNote;
  
  // Convert to internal note index (1-based, max 16 for grid)
  int noteIndex = midiNote - minNote + 1;
  if (noteIndex > 16) noteIndex = 16;
  if (noteIndex < 1) noteIndex = 1;
  
  return noteIndex;
}

// Convert MIDI velocity to internal velocity (1-127 -> 1-15)
int midiVelocityToInternal(int midiVelocity) {
  if (midiVelocity < 1) return 1;
  if (midiVelocity > 127) return 15;
  return mapf(midiVelocity, 1, 127, 1, 15);
}

// Convert MIDI channel volume to internal volume (0-127 -> 0-100)
int midiVolumeToInternal(int midiVolume) {
  if (midiVolume < 0) return 0;
  if (midiVolume > 127) return 100;
  return mapf(midiVolume, 0, 127, 0, 100);
}

// Simple MIDI file parser

// Parse MIDI file header
bool parseMIDIHeader(File& file) {
  char header[4];
  if (file.readBytes(header, 4) != 4) return false;
  
  // Check for "MThd" signature
  if (header[0] != 'M' || header[1] != 'T' || header[2] != 'h' || header[3] != 'd') {
    return false;
  }
  
  // Read header length (should be 6)
  uint32_t headerLength = 0;
  for (int i = 0; i < 4; i++) {
    headerLength = (headerLength << 8) | file.read();
  }
  
  if (headerLength != 6) return false;
  
  // Read format (should be 1 for multitrack)
  uint16_t format = (file.read() << 8) | file.read();
  if (format != 1) return false;
  
  // Read number of tracks (should be 14)
  uint16_t numTracks = (file.read() << 8) | file.read();
  if (numTracks < 1 || numTracks > 16) return false;
  
  // Read division (ticks per quarter note)
  uint16_t division = (file.read() << 8) | file.read();
  
  return true;
}

// Parse MIDI track header
bool parseTrackHeader(File& file) {
  char header[4];
  if (file.readBytes(header, 4) != 4) return false;
  
  // Check for "MTrk" signature
  if (header[0] != 'M' || header[1] != 'T' || header[2] != 'r' || header[3] != 'k') {
    return false;
  }
  
  // Read track length
  uint32_t trackLength = 0;
  for (int i = 0; i < 4; i++) {
    trackLength = (trackLength << 8) | file.read();
  }
  
  return true;
}

// Read variable length quantity
uint32_t readVariableLength(File& file) {
  uint32_t value = 0;
  uint8_t byte;
  
  do {
    byte = file.read();
    value = (value << 7) | (byte & 0x7F);
  } while (byte & 0x80);
  
  return value;
}

// Parse MIDI event
bool parseMIDIEvent(File& file, MIDIEvent& event) {
  // Read delta time
  event.deltaTime = readVariableLength(file);
  
  // Read event byte
  uint8_t eventByte = file.read();
  
  if (eventByte == 0xFF) {
    // Meta event
    uint8_t metaType = file.read();
    uint32_t metaLength = readVariableLength(file);
    
    // Skip meta event data
    for (uint32_t i = 0; i < metaLength; i++) {
      file.read();
    }
    
    return false; // Not a note event
  }
  
  if (eventByte == 0xF0 || eventByte == 0xF7) {
    // System exclusive event
    uint32_t sysexLength = readVariableLength(file);
    for (uint32_t i = 0; i < sysexLength; i++) {
      file.read();
    }
    return false; // Not a note event
  }
  
  // Channel event
  event.eventType = eventByte & 0xF0;
  event.channel = eventByte & 0x0F;
  
  if (event.eventType == 0x90 || event.eventType == 0x80) {
    // Note On or Note Off
    event.data1 = file.read(); // Note number
    event.data2 = file.read(); // Velocity
    
    return true; // This is a note event
  } else if (event.eventType == 0xB0) {
    // Control Change
    event.data1 = file.read(); // Controller number
    event.data2 = file.read(); // Controller value
    
    return false; // Not a note event
  } else if (event.eventType == 0xC0) {
    // Program Change
    event.data1 = file.read(); // Program number
    
    return false; // Not a note event
  }
  
  return false; // Unknown event
}

// Load MIDI file and convert to internal pattern format
bool loadMIDIFile(const char* filename) {
  File file = SD.open(filename);
  if (!file) {
    return false;
  }
  
  // Clear existing notes
  for (unsigned int x = 1; x < maxlen; x++) {
    for (unsigned int y = 1; y < maxY + 1; y++) {
      note[x][y].channel = 0;
      note[x][y].velocity = 0;
    }
  }
  
  // Parse MIDI header
  if (!parseMIDIHeader(file)) {
    file.close();
    return false;
  }
  
  // Track timing variables
  uint32_t currentTime = 0;
  uint32_t ticksPerBeat = 480; // Default from example
  uint32_t ticksPerStep = ticksPerBeat / 4; // 16th notes
  
  // Process each track
  for (int trackNum = 0; trackNum < 14; trackNum++) {
    if (!parseTrackHeader(file)) {
      break;
    }
    
    uint32_t trackStartTime = currentTime;
    
    // Process events in this track
    while (file.available()) {
      MIDIEvent event;
      if (!parseMIDIEvent(file, event)) {
        continue;
      }
      
      // Update current time
      currentTime += event.deltaTime;
      
      // Process note events - only Note On with velocity > 0
      if (event.eventType == 0x90 && event.data2 > 0) { // Note On with velocity
        int channel = event.channel + 1; // Convert to 1-based
        int mappedChannel = MIDI_CHANNEL_MAP[channel];
        
        if (mappedChannel == 0) continue; // Skip unused channels
        
        // Convert MIDI note to internal note
        int internalNote = midiNoteToInternalNote(event.data1, channel);
        if (internalNote == 0) continue;
        
        // Convert velocity
        int internalVelocity = midiVelocityToInternal(event.data2);
        
        // Calculate step position (1-based)
        // Each bar = 4 beats, each beat = 4 steps (16th notes)
        // So each bar = 16 steps, 16 pages = 256 steps total
        uint32_t stepTime = currentTime - trackStartTime;
        int step = (stepTime / ticksPerStep) + 1;
        
        // Limit to maxlen (16 pages * 16 steps = 256 steps)
        // maxlen = 257, so valid range is 1-256
        if (step > maxlen - 1) step = maxlen - 1; // maxlen - 1 = 256
        if (step < 1) step = 1;
        
        // Set note in pattern: note[tick(1-256)][ypos(1-16)] = channel(1-14)
        note[step][internalNote].channel = mappedChannel;
        note[step][internalNote].velocity = internalVelocity;
        
        // Debug output (can be removed later)
        //Serial.print("MIDI: Ch="); Serial.print(channel); 
        //Serial.print(" -> MappedCh="); Serial.print(mappedChannel);
        //Serial.print(" Note="); Serial.print(event.data1);
        //Serial.print(" -> InternalNote="); Serial.print(internalNote);
        //Serial.print(" Step="); Serial.print(step);
        //Serial.print(" Vel="); Serial.println(internalVelocity);
      }
    }
  }
  
  file.close();
  return true;
}

// Main function to load MIDI file
void loadMIDIPattern(const char* filename) {
  if (loadMIDIFile(filename)) {
    // Reset basic runtime flags when loading a MIDI pattern
    GLOB.singleMode = false;
    
    // Set default BPM (can be overridden by user)
    Mode *bpm_vol = &volume_bpm;
    bpm_vol->pos[3] = 120; // Default BPM
    playNoteInterval = ((60 * 1000 / 120) / 4) * 1000;
    playTimer.update(playNoteInterval);
    bpm_vol->pos[2] = GLOB.vol;
    
    // Update last page
    updateLastPage();
    
    // Don't load SMP settings for MIDI files as requested
    // loadSMPSettings(); // Commented out as per requirements
    
    delay(500);
    switchMode(&draw);
  }
}
