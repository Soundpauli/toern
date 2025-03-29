
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
      {
        unsigned long now = micros();  // Capture time ASAP
        myClock(now);  // Pass it into the clock handler
      }
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
  static unsigned long lastClockSent = micros();
  unsigned long now = micros();

  // Use a float for smoothing even though SMP.bpm is an int.
  static float smoothedBPM = (float)SMP.bpm;  
  const float alpha = 0.1f;  // Smoothing factor (0.0 to 1.0); lower is smoother.
  smoothedBPM = alpha * ((float)SMP.bpm) + (1.0f - alpha) * smoothedBPM;

  // Calculate clock pulse interval in microseconds using the smoothed BPM.
  // MIDI clocks: 24 ticks per quarter note.
  float clockInterval = 60000000.0f / (smoothedBPM * 24.0f);
  unsigned long clockInterval_us = (unsigned long)clockInterval;

  // Increment the lastClockSent time by the fixed interval to reduce jitter.
  while (now - lastClockSent >= clockInterval_us) {
    MIDI.sendRealTime(midi::Clock);
    lastClockSent += clockInterval_us;
  }
}

  
}


void myClock(unsigned long now) {
  static unsigned long lastClockTime = 0;
  static unsigned long intervals[CLOCK_BUFFER_SIZE] = {0};
  static int index = 0;
  static int count = 0;

  if (lastClockTime != 0) {
    unsigned long delta = now - lastClockTime;

    if (delta > 100000) {
      count = 0;
      index = 0;
    } else {
      intervals[index] = delta;
      index = (index + 1) % CLOCK_BUFFER_SIZE;
      if (count < CLOCK_BUFFER_SIZE) count++;

      unsigned long sum = 0;
      for (int i = 0; i < count; i++) sum += intervals[i];

      float avgInterval = sum / (float)count;
      float bpm = 60000000.0 / (avgInterval * 24.0);

      if (!MIDI_CLOCK_SEND) {
        SMP.bpm = round(bpm);
        playNoteInterval = ((60000UL / SMP.bpm) / 4) * 1000UL;
      }
    }
  }
  lastClockTime = now;
}

void MidiSendNoteOn(int pitch, int channel, int velocity) {
  // Clamp channel and velocity to valid MIDI ranges.
  if (channel < 1) channel = 1;
  if (channel > 16) channel = 16;
  if (velocity < 0) velocity = 0;
  if (velocity > 127) velocity = 127;
  
  // Optionally, if y is not already a MIDI note number, map it here.
  // For example, if y is from 0 to 15 and you want to map it to notes 60-75:
  // y = map(y, 0, 15, 60, 75);


  // Optionally, if you want a specific scale (e.g. only C major notes) use an array:
  // const int scaleNotes[16] = {48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74};
  // int note = scaleNotes[y - 1];

  const int baseNote = 48;
  // Map y to a MIDI note: y=1 gives baseNote, y=2 gives baseNote+1, etc.
  int note = baseNote + (pitch - 1);
  
  // Send the Note On event using the MIDI library.
  MIDI.sendNoteOn(note, velocity, channel);
}


void handleStop() {
  // Called when a MIDI STOP message is received.
  unsigned long currentTime = millis();
  if (currentTime - playStartTime > 200) {  // Only pause if play started more than 200ms ago
    pause();
  }
}
void handleNoteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  unsigned int mychannel = SMP.currentChannel;
  if (mychannel < 1 || mychannel > maxFiles) return;

  unsigned int livenote = (mychannel + 1) + pitch - 60;
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;

  if (livenote >= 1 && livenote <= 16) {
    if (isNowPlaying) {
      if (SMP.singleMode) {
        // Store for grid write on next beat
        pendingNote = {
          .pitch = pitch,
          .velocity = velocity,
          .channel = mychannel,
          .livenote = livenote,
          .active = true
        };
      }

      // Always play the note immediately
      activeNotes[pitch] = true;
      _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
    } else {
      // Not playing: play note and show light
      light(mapXtoPageOffset(SMP.x), livenote, CRGB(0, 0, 255));
      FastLEDshow();
      _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
    }
  }
}


void onBeatTick() {
  beatStartTime = millis();

  if (pendingNote.active) {
    int targetBeat = (beat == 1) ? (maxX * lastPage) : (beat - 1);

    if (pendingNote.livenote >= 1 && pendingNote.livenote <= 16) {
      note[targetBeat][pendingNote.livenote][0] = pendingNote.channel;
      note[targetBeat][pendingNote.livenote][1] = pendingNote.velocity;
    }

    pendingNote.active = false;
  }

  // Now play the notes for beat
}


void _handleNoteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  int currentbeat = beat;
  unsigned int mychannel = SMP.currentChannel;
  if (mychannel < 1 || mychannel > maxFiles) return;

  unsigned int livenote = (mychannel + 1) + pitch - 60;

  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;

  if (livenote >= 1 && livenote <= 16) {
    if (isNowPlaying) {
      if (SMP.singleMode) {
        // If there is a pending note, commit it now
        if (notePending) {
          unsigned int effectiveBeat = currentbeat;
          unsigned long elapsedTime = millis() - beatStartTime;

          if (elapsedTime > playNoteInterval / 2) {
            effectiveBeat = (beat % (maxX * lastPage)) + 1;
          }

          unsigned int pendingLiveNote = (mychannel + 1) + pendingPitch - 60;
          if (pendingLiveNote > 16) pendingLiveNote -= 12;
          if (pendingLiveNote < 1) pendingLiveNote += 12;

          if (pendingLiveNote >= 1 && pendingLiveNote <= 16) {
            note[effectiveBeat][pendingLiveNote][0] = mychannel;
            note[effectiveBeat][pendingLiveNote][1] = pendingVelocity;
          }

          notePending = false; // Clear the buffer
        }

        // Store current note as pending for next trigger
        pendingPitch = pitch;
        pendingVelocity = velocity;
        pendingTime = millis();
        notePending = true;

        // Play the current note now, but don’t write to grid
        activeNotes[pitch] = true;
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
      } else {
        // In multi-mode, play note immediately
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
      }
    } else {
      // Not playing: play and light LED
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
