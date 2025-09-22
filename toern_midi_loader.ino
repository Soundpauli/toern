// MIDI File Loader for Toern
// Handles loading MIDI files and mapping them to the pattern system

#include <SD.h>

// External variables from main code
extern const unsigned int maxlen;
#define maxY 16
#define defaultVelocity 63

// MIDI file structure definitions
struct MIDIHeader {
  char chunkType[4];    // "MThd"
  uint32_t length;      // Always 6
  uint16_t format;      // 0, 1, or 2
  uint16_t tracks;      // Number of tracks
  uint16_t division;    // Ticks per quarter note
};

struct MIDITrack {
  char chunkType[4];    // "MTrk"
  uint32_t length;      // Track data length
};

struct MIDIEvent {
  uint32_t deltaTime;   // Delta time in ticks
  uint8_t eventType;    // MIDI event type
  uint8_t channel;      // MIDI channel (0-15)
  uint8_t track;        // MIDI track number (0-based)
  uint8_t data1;        // First data byte
  uint8_t data2;        // Second data byte (if applicable)
};

// Global variables for MIDI parsing
EXTMEM MIDIEvent midiEvents[4096];  // Buffer for MIDI events
uint16_t eventCount = 0;
uint16_t currentTrack = 0;
uint32_t ticksPerQuarter = 480;  // Default MIDI resolution
uint32_t currentTime = 0;
uint8_t runningStatus = 0;  // For handling running status
MIDIHeader midiHeader;  // Global header for access across functions

// Track mapping: MIDI track -> Device channel
uint8_t trackToChannel[8] = {1, 2, 3, 4, 5, 6, 7, 8};  // Track 0->Ch1, Track 1->Ch2, etc.

// Function to read 32-bit big-endian integer
uint32_t readBigEndian32(File& file) {
  uint32_t value = 0;
  value |= (uint32_t)file.read() << 24;
  value |= (uint32_t)file.read() << 16;
  value |= (uint32_t)file.read() << 8;
  value |= (uint32_t)file.read();
  return value;
}

// Function to read 16-bit big-endian integer
uint16_t readBigEndian16(File& file) {
  uint16_t value = 0;
  value |= (uint16_t)file.read() << 8;
  value |= (uint16_t)file.read();
  return value;
}

// Function to read variable-length quantity
uint32_t readVariableLength(File& file) {
  uint32_t value = 0;
  uint8_t byte;
  
  do {
    byte = file.read();
    value = (value << 7) | (byte & 0x7F);
  } while (byte & 0x80);
  
  return value;
}

// Function to parse MIDI file header
bool parseMIDIHeader(File& file) {
  Serial.println("Parsing MIDI header...");
  
  // Read chunk type
  file.readBytes(midiHeader.chunkType, 4);
  midiHeader.chunkType[4] = '\0';  // Null terminate for printing
  Serial.println("Chunk type: " + String(midiHeader.chunkType));
  
  if (strncmp(midiHeader.chunkType, "MThd", 4) != 0) {
    Serial.println("ERROR: Not a valid MIDI file header");
    return false;  // Not a valid MIDI file
  }
  
  // Read length (should be 6)
  midiHeader.length = readBigEndian32(file);
  Serial.println("Header length: " + String(midiHeader.length));
  if (midiHeader.length != 6) {
    Serial.println("ERROR: Invalid header length");
    return false;
  }
  
  // Read format, tracks, and division
  midiHeader.format = readBigEndian16(file);
  midiHeader.tracks = readBigEndian16(file);
  midiHeader.division = readBigEndian16(file);
  
  Serial.println("Format: " + String(midiHeader.format));
  Serial.println("Tracks: " + String(midiHeader.tracks));
  Serial.println("Division: " + String(midiHeader.division));
  
  // Store global values
  ticksPerQuarter = midiHeader.division;
  
  Serial.println("MIDI header parsed successfully");
  return true;
}

// Function to parse a MIDI track
bool parseMIDITrack(File& file) {
  MIDITrack track;
  
  Serial.println("Parsing MIDI track " + String(currentTrack + 1) + "...");
  
  // Read chunk type
  if (file.available() < 4) {
    Serial.println("ERROR: Not enough data for track header");
    return false;
  }
  
  // Read the first 4 bytes and show them as hex
  uint8_t headerBytes[4];
  file.readBytes(headerBytes, 4);
  track.chunkType[0] = headerBytes[0];
  track.chunkType[1] = headerBytes[1];
  track.chunkType[2] = headerBytes[2];
  track.chunkType[3] = headerBytes[3];
  track.chunkType[4] = '\0';  // Null terminate for printing
  
  Serial.print("Track header bytes: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(String(headerBytes[i], HEX) + " ");
  }
  Serial.println();
  Serial.println("Track chunk type: '" + String(track.chunkType) + "'");
  
  if (strncmp(track.chunkType, "MTrk", 4) != 0) {
    Serial.println("ERROR: Not a valid MIDI track (expected 'MTrk', got '" + String(track.chunkType) + "')");
    return false;  // Not a valid track
  }
  
  // Read track length
  track.length = readBigEndian32(file);
  Serial.println("Track length: " + String(track.length) + " bytes");
  
  // Parse track events
  uint32_t bytesRead = 0;
  uint32_t deltaTime = 0;
  uint16_t eventsInTrack = 0;
  runningStatus = 0;  // Reset running status for each track
  
  while (bytesRead < track.length && eventCount < 4095) {
    Serial.println("--- Event " + String(eventsInTrack + 1) + " ---");
    Serial.println("Bytes read so far: " + String(bytesRead) + "/" + String(track.length));
    
    // Read delta time
    deltaTime = readVariableLength(file);
    Serial.println("Delta time: " + String(deltaTime) + " ticks");
    
    // Read status byte (or use running status)
    uint8_t statusByte;
    if (!file.available()) {
      Serial.println("ERROR: No status byte available");
      break;
    }
    
    uint8_t firstByte = file.read();
    bytesRead++;
    
    Serial.println("Read byte: " + String(firstByte, HEX) + " (dec: " + String(firstByte) + ")");
    
    // Check if this is a status byte or data byte
    if (firstByte >= 0x80) {
      // This is a status byte
      statusByte = firstByte;
      runningStatus = statusByte;
      Serial.println("Status byte: " + String(statusByte, HEX));
    } else {
      // This is a data byte, use running status
      statusByte = runningStatus;
      Serial.println("Running status: " + String(statusByte, HEX) + " Data byte: " + String(firstByte, HEX));
      // We need to process this as the first data byte
      goto processEvent;
    }
    
    if (statusByte == 0xFF) {
      // Meta event - handle some important ones
      uint8_t metaType = file.read();
      bytesRead++;
      uint32_t metaLength = readVariableLength(file);
      
      Serial.println("Meta event: Type=" + String(metaType, HEX) + " Length=" + String(metaLength));
      
      // Skip most meta events but log them
      file.seek(file.position() + metaLength);
      bytesRead += metaLength;
      continue;
    }
    
    if (statusByte == 0xF0 || statusByte == 0xF7) {
      // System exclusive - skip for now
      uint32_t sysexLength = readVariableLength(file);
      file.seek(file.position() + sysexLength);
      bytesRead += sysexLength;
      continue;
    }
    
    processEvent:
    // Parse MIDI event
    MIDIEvent event;
    event.deltaTime = deltaTime;
    event.eventType = statusByte & 0xF0;
    event.channel = statusByte & 0x0F;
    event.track = currentTrack;  // Assign current track number
    
    if (event.eventType == 0x90 || event.eventType == 0x80) {  // Note On/Off
      if (firstByte < 0x80) {
        // Running status case - firstByte is already the note number
        event.data1 = firstByte;
        event.data2 = file.read();  // Velocity
        bytesRead += 1;
      } else {
        // Normal case - read both data bytes
        event.data1 = file.read();  // Note number
        event.data2 = file.read();  // Velocity
        bytesRead += 2;
      }
      
      Serial.println("Note event: Track=" + String(event.track) + " Ch=" + String(event.channel) + " Note=" + String(event.data1) + " Vel=" + String(event.data2) + " Type=" + String(event.eventType, HEX));
      
      // Only process Note On events (velocity > 0)
      if (event.eventType == 0x90 && event.data2 > 0) {
        midiEvents[eventCount] = event;
        eventCount++;
        eventsInTrack++;
        Serial.println("  -> Added Note On event #" + String(eventCount));
      }
    } else if (event.eventType == 0xB0) {  // Controller Change
      if (firstByte < 0x80) {
        // Running status case - firstByte is already the controller number
        event.data1 = firstByte;
        event.data2 = file.read();  // Controller value
        bytesRead += 1;
      } else {
        // Normal case - read both data bytes
        event.data1 = file.read();  // Controller number
        event.data2 = file.read();  // Controller value
        bytesRead += 2;
      }
      
      Serial.println("Controller event: Ch=" + String(event.channel) + " CC=" + String(event.data1) + " Val=" + String(event.data2));
      
      // Store controller events for volume/mute
      midiEvents[eventCount] = event;
      eventCount++;
      eventsInTrack++;
    } else {
      // Skip other events
      Serial.println("Skipping event type: " + String(event.eventType, HEX));
      if (event.eventType == 0xC0 || event.eventType == 0xD0) {
        if (firstByte < 0x80) {
          event.data1 = firstByte;
          bytesRead += 0;  // Already counted firstByte
        } else {
          event.data1 = file.read();
          bytesRead++;
        }
      } else if (event.eventType == 0xE0) {
        if (firstByte < 0x80) {
          event.data1 = firstByte;
          event.data2 = file.read();
          bytesRead += 1;
        } else {
          event.data1 = file.read();
          event.data2 = file.read();
          bytesRead += 2;
        }
      }
    }
  }
  
  Serial.println("Track " + String(currentTrack + 1) + " completed: " + String(eventsInTrack) + " events processed");
  return true;
}

// Function to convert MIDI note to grid position
uint8_t midiNoteToGrid(uint8_t midiNote, uint8_t channel) {
  // Map MIDI note (0-127) to grid position (1-16)
  // Each channel has its own frequency range
  // Use the same logic as the existing handleNoteOn function
  
  unsigned int livenote = (channel + 1) + midiNote - 60;
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;
  
  return constrain(livenote, 1, 16);
}

// Function to convert MIDI time to step position
uint16_t midiTimeToStep(uint32_t midiTime) {
  // Convert MIDI time to step position (1-256)
  // 1 bar = 4 steps, 4 bars = 16 steps
  // Use ticks per quarter note to calculate step position
  
  uint32_t stepsPerQuarter = 1;  // 1 step per quarter note (4 steps per bar)
  uint32_t stepTime = ticksPerQuarter / stepsPerQuarter;
  if (stepTime == 0) stepTime = 1;  // Prevent division by zero
  uint16_t step = (midiTime / stepTime) + 1;
  
  return constrain(step, 1, maxlen);
}

// Function to map MIDI channel to device channel
uint8_t mapMIDIChannel(uint8_t midiChannel) {
  // Map MIDI channels (0-15) to device channels (1-8)
  // If more than 8 tracks, map them to channels 1-8
  return (midiChannel % 8) + 1;
}

// Main function to load MIDI file
bool loadMIDIFile(const char* filename) {
  Serial.println("=== MIDI Loader Debug ===");
  Serial.println("Loading MIDI file: " + String(filename));
  
  // Clear existing notes but keep current settings
  extern Note note[maxlen + 1][maxY + 1];
  extern Device SMP;
  
  // Clear all notes
  Serial.println("Clearing existing notes...");
  for (uint16_t x = 1; x <= maxlen; x++) {
    for (uint16_t y = 1; y <= maxY; y++) {
      note[x][y].channel = 0;
      note[x][y].velocity = defaultVelocity;
    }
  }
  
  // Reset MIDI parsing variables
  eventCount = 0;
  currentTrack = 0;
  currentTime = 0;
  
  // Open MIDI file
  File midiFile = SD.open(filename);
  if (!midiFile) {
    Serial.println("ERROR: Could not open MIDI file");
    return false;
  }
  
  Serial.println("MIDI file opened successfully, size: " + String(midiFile.size()) + " bytes");
  
  // Parse MIDI header
  if (!parseMIDIHeader(midiFile)) {
    midiFile.close();
    return false;
  }
  
  // Parse tracks
  Serial.println("Parsing tracks...");
  Serial.println("Header indicates " + String(midiHeader.tracks) + " tracks");
  for (uint16_t track = 0; track < midiHeader.tracks && track < 8; track++) {  // Use actual track count, limit to 8
    Serial.println("File position before track " + String(track + 1) + ": " + String(midiFile.position()));
    Serial.println("Available bytes: " + String(midiFile.available()));
    
    if (midiFile.available()) {
      if (!parseMIDITrack(midiFile)) {
        Serial.println("Failed to parse track " + String(track + 1));
        break;
      }
      currentTrack++;
    } else {
      Serial.println("No more tracks available");
      break;
    }
  }
  
  midiFile.close();
  
  Serial.println("Total events parsed: " + String(eventCount));
  Serial.println("Track mapping: Track 0->Ch1, Track 1->Ch2, Track 2->Ch3, etc.");
  
  // Process MIDI events and convert to note array
  Serial.println("Processing MIDI events to note array...");
  uint32_t currentTime = 0;
  
  for (uint16_t i = 0; i < eventCount; i++) {
    MIDIEvent event = midiEvents[i];
    currentTime += event.deltaTime;
    
    if (event.eventType == 0x90) {  // Note On
      uint8_t deviceChannel = trackToChannel[event.track];  // Map track to device channel
      uint8_t gridY = midiNoteToGrid(event.data1, deviceChannel);
      uint16_t stepX = midiTimeToStep(currentTime);
      
      Serial.println("Processing Note On: Track=" + String(event.track) + " -> Dev Ch=" + String(deviceChannel) + 
                     " Note=" + String(event.data1) + " -> Grid Y=" + String(gridY) + 
                     " Time=" + String(currentTime) + " -> Step X=" + String(stepX) + 
                     " Vel=" + String(event.data2));
      
      // Set note in the pattern
      if (stepX <= maxlen && gridY <= maxY) {
        note[stepX][gridY].channel = deviceChannel;
        note[stepX][gridY].velocity = event.data2;
        Serial.println("  -> Set note[" + String(stepX) + "][" + String(gridY) + "] = Ch" + String(deviceChannel) + " Vel" + String(event.data2));
      } else {
        Serial.println("  -> Note out of bounds: stepX=" + String(stepX) + " gridY=" + String(gridY));
      }
    } else if (event.eventType == 0xB0) {  // Controller Change
      if (event.data1 == 7) {  // Volume controller
        uint8_t deviceChannel = trackToChannel[event.track];
        if (deviceChannel <= 8) {
          SMP.channelVol[deviceChannel] = event.data2;
          Serial.println("Set channel " + String(deviceChannel) + " volume to " + String(event.data2));
        }
      } else if (event.data1 == 123) {  // All Notes Off
        uint8_t deviceChannel = trackToChannel[event.track];
        if (deviceChannel <= 8) {
          SMP.mute[deviceChannel] = 1;  // Mute channel
          Serial.println("Muted channel " + String(deviceChannel));
        }
      }
    }
  }
  
  Serial.println("=== MIDI Loading Complete ===");
  Serial.println("Total events processed: " + String(eventCount));
  Serial.println("Notes should now be visible in the pattern");
  
  return true;
}

// Function to check if MIDI file exists and load it
bool loadPatternOrMIDI(bool autoload) {
  extern Device SMP;
  
  char txtFile[50];
  char midiFile[50];
  
  if (autoload) {
    sprintf(txtFile, "autosaved.txt");
    sprintf(midiFile, "autosaved.mid");
  } else {
    sprintf(txtFile, "%d.txt", SMP.file);
    sprintf(midiFile, "%d.mid", SMP.file);
  }
  
  // Check for MIDI file first
  if (SD.exists(midiFile)) {
    return loadMIDIFile(midiFile);
  } else if (SD.exists(txtFile)) {
    // Fall back to regular pattern loading
    extern void loadPattern(bool autoload);
    loadPattern(autoload);
    return true;
  }
  
  return false;
}
