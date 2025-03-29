
void checkMidi() {
  if (MIDI.read()) {
    uint8_t pitch, velocity, channel;
    uint8_t miditype;

    miditype = MIDI.getType();
    channel = MIDI.getChannel();  // Fixed: assign channel from the incoming message

    switch (miditype) {
      case midi::NoteOn:
        pitch = MIDI.getData1();
        // Use the actual velocity from the message (if zero, treat as note off)
        velocity = MIDI.getData2() ? MIDI.getData2() : 127;
        handleNoteOn(channel, pitch, velocity);
        break;

      case midi::NoteOff:
        pitch = MIDI.getData1();
        velocity = MIDI.getData2();
        handleNoteOff(channel, pitch, velocity);
        break;

      case midi::Clock:
        // Process incoming MIDI Clock pulse with improved averaging.
        myClock();
        break;

      case midi::Start:
        handleStart();
        break;

      case midi::Stop:
        handleStop();
        break;

      case midi::SongPosition:
        // Combine two data bytes into a 14-bit number
        handleSongPosition(MIDI.getData1() | (MIDI.getData2() << 7));
        break;

      case midi::TimeCodeQuarterFrame:
        handleTimeCodeQuarterFrame(MIDI.getData1());
        break;

      default:
        // Other MIDI messages can be ignored or processed as needed.
        break;
    }
  }
  
  // If this device is sending MIDI Clock, calculate and send it based on SMP.bpm.
  if (MIDI_CLOCK_SEND) {
    static unsigned long lastClockSent = 0;
    unsigned long now = millis();
    // Calculate the interval per clock pulse (24 per quarter note)
    unsigned long clockInterval = (60000UL / SMP.bpm) / 24;
    if (now - lastClockSent >= clockInterval) {
      MIDI.sendRealTime(midi::Clock);
      lastClockSent = now;
    }
  }
}

void myClock() {
  // Use a sliding window of intervals for a smoother BPM calculation.
  static unsigned long lastClockTime = 0;
  static unsigned long intervals[CLOCK_BUFFER_SIZE] = {0};
  static int index = 0;
  static int count = 0;
  
  unsigned long now = micros();
  if (lastClockTime != 0) {
    unsigned long delta = now - lastClockTime;
    
    // If the gap is too long, assume an interruption and reset the buffer.
    if (delta > 100000) {  
      count = 0;
      index = 0;
    } else {
      intervals[index] = delta;
      index = (index + 1) % CLOCK_BUFFER_SIZE;
      if (count < CLOCK_BUFFER_SIZE) count++;
      
      // Compute the average interval over the buffered pulses.
      unsigned long sum = 0;
      for (int i = 0; i < count; i++) {
        sum += intervals[i];
      }
      float avgInterval = sum / (float) count;
      // 24 clock pulses per quarter note, 60,000,000 microseconds per minute.
      float bpm = 60000000.0 / (avgInterval * 24.0);
      
      // If we are receiving (not sending) clock, update BPM and the note interval.
      if (!MIDI_CLOCK_SEND) {
        SMP.bpm = round(bpm);
        // Calculate note interval for a 16th note (quarter note/4), converted to microseconds.
        playNoteInterval = ((60000UL / SMP.bpm) / 4) * 1000UL;
      }
    }
  }
  lastClockTime = now;
}

void handleStop() {
  // Called when a MIDI STOP message is received.
  unsigned long currentTime = millis();
  if (currentTime - playStartTime > 200) {  // Only pause if play started more than 200ms ago
    pause();
  }
}

void handleNoteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  int currentbeat = beat;
  unsigned int mychannel = SMP.currentChannel;
  if (mychannel < 1 || mychannel > maxFiles) return;

  // Map the received note to a note grid. Base note is set to C3 (MIDI 60).
  unsigned int livenote = (mychannel + 1) + pitch - 60;

  // Adjust for missing octaves.
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;

  if (livenote >= 1 && livenote <= 16) {
    if (isNowPlaying) {
      if (SMP.singleMode) {
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - beatStartTime;
        unsigned int effectiveBeat = currentbeat;

        // If the note arrives late, assign it to the next beat.
        if (elapsedTime > playNoteInterval / 2) {
          effectiveBeat = (beat % (maxX * lastPage)) + 1;
        }
        // Map the note to the grid.
        note[effectiveBeat][livenote][0] = mychannel;
        note[effectiveBeat][livenote][1] = velocity;
        activeNotes[pitch] = true;
        // Play the note immediately.
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
      } else {
        // In multi-mode, play the note without grid assignment.
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
      }
    } else {
      // If not playing, light up the corresponding LED and play the note.
      light(mapXtoPageOffset(SMP.x), livenote, CRGB(0, 0, 255));
      FastLEDshow();
      _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
    }
  }
}

void handleNoteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  if (isNowPlaying) {
    int mychannel = SMP.currentChannel;
    // Only stop the note if it is currently active.
    if (activeNotes[pitch]) {
      _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, false, false);
      activeNotes[pitch] = false;  // Mark the note as inactive.
    }
  }
}

void handleStart() {
  // Called when a MIDI Start message is received.
  waitForFourBars = true;
  pulseCount = 0;  // Reset pulse count on start.
  beatStartTime = millis();  // Sync to current time.
  Serial.println("MIDI Start Received");
  play(true);
}

void handleTimeCodeQuarterFrame(uint8_t data) {
  // Called on receiving a MIDI Time Code Quarter Frame message.
  Serial.println("MIDI TimeCodeQuarterFrame Received");
}

void handleSongPosition(uint16_t beats) {
  // Called when a Song Position Pointer message is received.
  Serial.print("Song Position Pointer Received: ");
  Serial.println(beats);
}
