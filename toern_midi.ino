
// move these to file-scope so everybody can reset them
static unsigned long lastClockTime = 0;
static unsigned long intervalsBuf[CLOCK_BUFFER_SIZE]  = { 0 };

static int bufIndex = 0;
static int bufCount = 0;
static float smoothedBPM = 0.0f;
static uint8_t midiClockTicks = 0;
static unsigned long lastClockSent = 0;

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
        
        //handleNoteOn(SMP.currentChannel, pitch, velocity);
        break;

      case midi::NoteOff:
        pitch = MIDI.getData1();
        velocity = MIDI.getData2();
        handleNoteOff(SMP.currentChannel, pitch - 60, velocity);
        break;

      case midi::Clock:
        {
          unsigned long now = micros();  // Capture time ASAP
          if (!MIDI_CLOCK_SEND) myClock(now);                  // Pass it into the clock handler
        }
        break;

      case midi::Start:
        handleStart();
        break;

      case midi::Stop:
        handleStop();
        break;

      case midi::SongPosition:
        // Combine two data bytes into a 14-^ number
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


    unsigned long now = micros();

    // Use a float for smoothing even though SMP.bpm is an int.

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
      //lastClockSent = now;
    }
  }
}


// file-scope variables (ensure these are indeed at file scope)
// static unsigned long lastClockTime = 0; // Already declared
// static unsigned long intervalsBuf[CLOCK_BUFFER_SIZE]  = { 0 }; // Already declared
// static int bufIndex = 0; // Already declared
// static int bufCount = 0; // Already declared
// static float smoothedBPM = 0.0f; // Already declared
// static uint8_t midiClockTicks = 0; // Already declared
constexpr uint8_t clocksPerStep = 24 / 4;

void resetMidiClockState() { // MODIFIED to reset BPM averaging state for slave
  lastClockTime = 0;
  
  if (MIDI_CLOCK_SEND) {
    smoothedBPM = (float)SMP.bpm; // For master, use current BPM
    lastClockSent = micros();
    // Ensure playTimer is configured if master
    if (SMP.bpm > 0.0f) {
        unsigned long currentPlayNoteInterval = (unsigned long)((60000000.0f / SMP.bpm) / 4.0f);
        playTimer.begin(playNote, currentPlayNoteInterval);
    } else {
        playTimer.end();
    }
  } else { // SLAVE MODE
    playTimer.end(); // SLAVE mode, internal timer off
    // Reset BPM averaging state for slave to re-learn quickly
    bufCount = 0;
    bufIndex = 0;
    smoothedBPM = 0.0f; // This will make it use the first valid rawBPM
    // Or, you could set smoothedBPM to a typical starting BPM like 120.0f
    // smoothedBPM = 120.0f; 
    // SMP.bpm will be updated by myClock
  }
}

void myClock(unsigned long now_captured) { // Renamed 'now' for clarity
  if (MIDI_CLOCK_SEND) { // Safeguard: Should only run in slave mode
    return;
  }

  // --- BPM Averaging ---
  if (lastClockTime != 0) {
    unsigned long delta = now_captured - lastClockTime;
    bool accept_delta = false;

    // 1. Basic Sanity Checks for Delta
    // Min plausible delta (around 400 BPM, e.g. 60M / (400*24) = 6250 us).
    // Max plausible delta (around 30 BPM, e.g. 60M / (30*24) = 83333 us).
    // Your current "delta < 100000" is a loose upper bound.
    // We need a tighter lower bound to prevent BPM shooting up.
    
    // Let's define a MIN_DELTA based on slightly faster than your max BPM
    // For 300 BPM, delta is ~8333 us. Let's say min acceptable is ~7000 us (~350 BPM).
    // And MAX_DELTA based on slightly slower than your min BPM
    // For 40 BPM, delta is ~62500 us. Let's say max acceptable is ~75000 us (~32 BPM).
    const unsigned long ABSOLUTE_MIN_DELTA = 7000; 
    const unsigned long ABSOLUTE_MAX_DELTA = 75000; // was 100000, tightened it

    if (delta >= ABSOLUTE_MIN_DELTA && delta <= ABSOLUTE_MAX_DELTA) {
        // If smoothedBPM is somewhat stable, we can be more restrictive
        if (smoothedBPM > (float)BPM_MIN * 0.8f && bufCount > CLOCK_BUFFER_SIZE / 2) {
            float expectedInterval = 60000000.0f / (smoothedBPM * 24.0f);
            // Allow delta to be LARGER than expected, but not much SMALLER
            // If BPM jumps UP, delta was too SMALL.
            if (delta >= expectedInterval * 0.80f &&  // Not more than 20% smaller
                delta <= expectedInterval * 1.50f) { // Not more than 50% larger
                accept_delta = true;
            }
        } else {
            // Initial phase, or smoothedBPM is not yet reliable, accept any delta within absolute bounds
            accept_delta = true;
        }
    }
    
    /* // Optional: Log rejected deltas
    if (!accept_delta && delta > 0 && isNowPlaying) { // Log only if playing
        Serial.print(millis());
        Serial.print(" Delta REJ: "); Serial.print(delta);
        Serial.print(" sBPM: "); Serial.println(smoothedBPM, 1);
    }
    */

    if (accept_delta) {
      intervalsBuf[bufIndex] = delta;
      bufIndex = (bufIndex + 1) % CLOCK_BUFFER_SIZE;
      if (bufCount < CLOCK_BUFFER_SIZE) bufCount++;

      unsigned long sum = 0;
      for (int i = 0; i < bufCount; i++) sum += intervalsBuf[i];
      
      if (bufCount > 0) { // Ensure bufCount is not zero
        float avgInterval = (float)sum / bufCount;
        if (avgInterval >= ABSOLUTE_MIN_DELTA * 0.8f ) { // also check avgInterval is plausible
            float rawBPM = 60000000.0f / (avgInterval * 24.0f);
            const float alpha_smooth = 0.1f; // Smoothing factor (can try 0.05 for more smoothing)

            if (smoothedBPM == 0.0f || bufCount < CLOCK_BUFFER_SIZE / 3) { // Faster initial lock
                smoothedBPM = rawBPM;
            } else {
                smoothedBPM = alpha_smooth * rawBPM + (1.0f - alpha_smooth) * smoothedBPM;
            }

            // Clamp to defined min/max
            if (smoothedBPM < BPM_MIN) smoothedBPM = BPM_MIN;
            if (smoothedBPM > BPM_MAX) smoothedBPM = BPM_MAX;

            SMP.bpm = round(smoothedBPM);
        }
      }
    }
  }
  lastClockTime = now_captured;

  // --- Step Scheduling (for slave mode) ---
  pulseCount = (pulseCount + 1) % (24 * 2); // 48 pulses = 2 quarter notes or half a 4/4 bar

  midiClockTicks++;
  if (midiClockTicks >= clocksPerStep) { // clocksPerStep = 6
    midiClockTicks = 0;
    
    if (pendingStartOnBar && pulseCount == 0) { // pulseCount == 0 is on a half-bar
      // Serial.println("myClock: Slave Start - pendingStartOnBar & pulseCount == 0");
      pendingStartOnBar = false;
      isNowPlaying = true;
      beat = 1;
      SMP.page = 1;
      playStartTime = millis();
    }

    if (isNowPlaying) {
      stepIsDue = true;
    }
  }
}




void resetMidiClockState2() {
  lastClockTime = 0;
  smoothedBPM = SMP.bpm;     // start your smoothing at the current BPM
  lastClockSent = micros();  // so when we jump into myClock() we don’t burst
}
// number of MIDI clocks per “step” (sixteenth note)



void myClock2(unsigned long now) {
  // ————— 1) BPM averaging (unchanged) —————
  if (lastClockTime != 0) {
    unsigned long delta = now - lastClockTime;
    if (delta < 100000) {
      intervalsBuf[bufIndex] = delta;
      bufIndex = (bufIndex + 1) % CLOCK_BUFFER_SIZE;
      if (bufCount < CLOCK_BUFFER_SIZE) bufCount++;

      unsigned long sum = 0;
      for (int i = 0; i < bufCount; i++) sum += intervalsBuf[i];
      float avgInterval = sum / float(bufCount);
      float rawBPM = 60000000.0f / (avgInterval * 24.0f);
      const float α = 0.1f;
      if (smoothedBPM == 0.0f) smoothedBPM = rawBPM;
      smoothedBPM = α * rawBPM + (1 - α) * smoothedBPM;
      SMP.bpm = round(smoothedBPM);
      
    }
  }
  lastClockTime = now;

    // ————— 2) Step scheduling —————
    if (!MIDI_CLOCK_SEND) {
      pulseCount = (pulseCount + 1) % (24 * 2);

    midiClockTicks++;
    if (midiClockTicks >= clocksPerStep) {
      midiClockTicks = 0;
      
           // **first** check for an armed start
      if (pendingStartOnBar && pulseCount == 0) {
        // bar-1 just hit!
        pendingStartOnBar = false;
        isNowPlaying = true;
        beat = 1;
        SMP.page = 1;
        playStartTime = millis();
      }

 if (isNowPlaying) {
      //playNote();
      stepIsDue = true;  // <--- INSTEAD, JUST SET THE FLAG. This is instant.
      }
    }
  }
  
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
  
  if (!isNowPlaying || !MIDI_TRANSPORT_RECEIVE) return;

  // Called when a MIDI STOP message is received.
  unsigned long currentTime = millis();
  if (currentTime - playStartTime > 500) {  // Only pause if play started more than 200ms ago
    pause();
  }
}




void handleNoteOn(int ch, uint8_t pitch, uint8_t velocity) {
  //Serial.println(pitch);
  // For persistent channels (11-14), use the actual MIDI channel
  if (!MIDI_VOICE_SELECT) ch = SMP.currentChannel;

  if (ch < 1 || ch > 16) return;
  pressedKeyCount[ch]++;  // Increment count for this channel

  if (pressedKeyCount[ch] == 1) {
    persistentNoteOn[ch] = true;
  }

  unsigned int livenote = (ch + 1) + pitch - 60;
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;
  if (livenote >= 1 && livenote <= 16) {

    if (isNowPlaying) {
      if (SMP.singleMode) {
        // Store for grid write on next beat
        pendingNotes.push_back({ .pitch = pitch,
                                 .velocity = velocity,
                                 .channel = (uint8_t)ch,
                                 .livenote = (uint8_t)livenote });
      }else{

        if (ch < 9) {
     _samplers[ch].noteEvent(((SampleRate[ch] * 12) + pitch - 60), velocity, true, false);
    } else if (ch > 12 && ch < 15) {
      playSynth(ch, livenote, velocity, true);
    } else if (ch == 11) {

      playSound(12 * octave[0] + transpose + pitch - 60 + 12, 0);
    }
      }
      // Always play the note immediately
      activeNotes[pitch] = true;
    }
    else{ 
    // Live mode: play note and show light
    //light(mapXtoPageOffset(SMP.x), livenote, CRGB(255, 255, 255));
    //FastLED.show();

    if (ch < 9) {
     _samplers[ch].noteEvent(((SampleRate[ch] * 12) + pitch - 60), velocity, true, false);
    } else if (ch > 12 && ch < 15) {
      playSynth(ch, livenote, velocity, true);
    } else if (ch == 11) {

      playSound(12 * octave[0] + transpose + pitch - 60 + 12, 0);
    }
    }
  }
}

void onBeatTick() {
  
  for (auto &pn : pendingNotes) {
    int targetBeat = beat; //(beat == 1) ? (maxX * lastPage) : (beat);
    note[targetBeat][pn.livenote].channel = pn.channel;
    note[targetBeat][pn.livenote].velocity = pn.velocity;
  }
  pendingNotes.clear();
  

  return;
  // Reset your UI timer immediately for the new beat
  beatStartTime = millis();

  // Flush any pending grid-writes into the _new_ beat slot?
  
}




void handleNoteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {

  // For persistent channels (11-14) use the counter.
  if (channel >= 11 && channel <= 14) {
    if (pressedKeyCount[channel] > 0)
      pressedKeyCount[channel]--;  // Decrement counter

    // Only release if no keys remain pressed
    if (pressedKeyCount[channel] == 0 && persistentNoteOn[channel]) {
      if (!envelopes[channel]) return;
      envelopes[channel]->noteOff();
      persistentNoteOn[channel] = false;
      noteOnTriggered[channel] = false;
    }
    return;
  }

  // Existing logic for non-persistent channels goes here...
}
void handleStart() {
  // only act if we’re supposed to follow external transport
  if (!MIDI_TRANSPORT_RECEIVE) return;

  //Serial.println("MIDI Start Received");

  // 1) reset everything for beat 1
  beat           = 1;
  SMP.page       = 1;
  deleteActiveCopy();
  isNowPlaying   = true;
  beatStartTime = millis();

  // 2) if we're the master, also send a MIDI-Start
  if (MIDI_CLOCK_SEND) {
    MIDI.sendRealTime(midi::Start);
  }

  // 3) immediately fire the very first step
  playNote();
}

void handleStart2() {
  if (isNowPlaying || !MIDI_TRANSPORT_RECEIVE) return;
  //Serial.println("MIDI Start Received");
  //if (!MIDI_CLOCK_SEND) playTimer.end(); // just to be sure
   // ---------- SLAVE MODE: start immediately ----------
  if (!MIDI_CLOCK_SEND) {
    // reset to bar‐1
    beat     = 1;
    SMP.page = 1;
    // turn playback on
    isNowPlaying  = true;
    beatStartTime = millis();
    // fire the very first step now
    playNote();
    return;
  }

  // ---------- MASTER MODE: your existing behavior ----------
  if (isNowPlaying) return;       // avoid double-starts
  midiClockTicks = 0;
  pulseCount     = 0;             // clear any previous clock counting
  // if you still had a four-bar arm, clear it:
  waitForFourBars = false;
  // now invoke your normal play() which will send MIDI Start
  play(true);
  playNote();
}

void handleTimeCodeQuarterFrame(uint8_t data) {
  // Called on receiving a MIDI Time Code Quarter Frame message.
  //Serial.println("MIDI TimeCodeQuarterFrame Received");
}

void handleSongPosition(uint16_t beats) {
  // Called when a Song Position Pointer message is received.
  //Serial.print("Song Position Pointer Received: ");
  //Serial.println(beats);
}
