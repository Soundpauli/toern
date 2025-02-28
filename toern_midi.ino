void checkMidi() {

  if (MIDI.read()) {
    int pitch, velocity, channel, d1, d2;
    uint8_t miditype;

    miditype = MIDI.getType();

    switch (miditype) {
      case midi::NoteOn:
        pitch = MIDI.getData1();
        velocity = 127;  //MIDI.getData2();
        //channel = MIDI.getChannel();
        handleNoteOn(channel, pitch, velocity);
        //if (velocity > 0) {
        //Serial.println(String("Note On:  ch=") + channel + ", note=" + pitch + ", velocity=" + velocity);
        //}
        break;

      case midi::NoteOff:
        pitch = MIDI.getData1();
        velocity = 127;
        handleNoteOff(channel, pitch, velocity);
        // velocity = MIDI.getData2();
        // channel = MIDI.getChannel();
        // Serial.println(String("Note Off: ch=") + channel + ", note=" + pitch + ", velocity=" + velocity);
        break;

      default:
        //d1 = MIDI.getData1();
        //d2 = MIDI.getData2();
        //Serial.println(String("Message, type=") + miditype + ", data = " + d1 + " " + d2);
        break;
    }
  }
}


void handleStop() {
  // This function is called when a MIDI STOP message is received
  isNowPlaying = false;
  pulseCount = 0;
  AudioMemoryUsageMaxReset();
  deleteActiveCopy();
  envelope0.noteOff();
  allOff();
  autoSave();
  beat = 1;
  SMP.page = 1;
  waitForFourBars = false;
  yield();
}


//receive Midi notes and velocity, map to note array. if not playing, play the note
void handleNoteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  int currentbeat = beat;
  unsigned int mychannel = SMP.currentChannel;
  if (mychannel < 1 || mychannel > maxFiles) return;

  unsigned int livenote = (mychannel + 1) + pitch - 60;  // set Base to C3

  // Adjust for missing octaves
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;

  if (livenote >= 1 && livenote <= 16) {
    if (isNowPlaying) {
      if (SMP.singleMode) {
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - beatStartTime;
        unsigned int effectiveBeat = currentbeat;

        // If the note arrives late, assign it to the next beat
        if (elapsedTime > playNoteInterval / 2) {
          effectiveBeat = (beat % (maxX * lastPage)) + 1;
        }

        // Assign the note to the effective beat
        note[effectiveBeat][livenote][0] = mychannel;
        note[effectiveBeat][livenote][1] = velocity;

        // Mark the note as active
        activeNotes[pitch] = true;

        // Play the note immediately
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
      } else {
        // Play the note without assigning it to the grid
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
      }
    } else {
      // If not playing, light up the LED and play the note
      light(mapXtoPageOffset(SMP.x), livenote, CRGB(0, 0, 255));
      FastLEDshow();
      _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
    }
  }
}

void handleNoteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  if (isNowPlaying) {
    if (!SMP.singleMode) {
      int mychannel = SMP.currentChannel;

      // Only stop the note if it is currently active
      if (activeNotes[pitch]) {
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, false, false);
        activeNotes[pitch] = false;  // Mark the note as inactive
      }
    }
  }
}

void handleStart() {
  // This function is called when a MIDI Start message is received
  waitForFourBars = true;
  pulseCount = 0;  // Reset pulse count on start
  Serial.println("MIDI Start Received");
}


void handleTimeCodeQuarterFrame(uint8_t data) {
  // This function is called when a MIDI Start message is received
  Serial.println("MIDI TimeCodeQuarterFrame Received");
}



void handleSongPosition(uint16_t beats) {
  // This fuSerinction is called when a Song Position Pointer message is received
  Serial.print("Song Position Pointer Received: ");
  Serial.println(beats);
}



void myClock() {
  static unsigned int lastClockTime = 0;
  static unsigned int totalInterval = 0;
  static unsigned int clockCount = 0;

  unsigned int now = millis();
  if (lastClockTime > 0) {
    totalInterval += now - lastClockTime;
    clockCount++;

    if (clockCount >= numPulsesForAverage) {
      float averageInterval = totalInterval / (float)numPulsesForAverage;
      float bpm = 60000.0 / (averageInterval * 24);
      SMP.bpm = round(bpm);
      playTimer.update(((60 * 1000 / SMP.bpm) / 4) * 1000);
      midiTimer.update((((60 * 1000 / SMP.bpm) / 4) * 1000) / 24);
      clockCount = 0;
      totalInterval = 0;
    }
  }
  lastClockTime = now;
}
