#include <math.h>

// External variables
extern float detune[13]; // Global detune array for channels 1-12
extern float channelOctave[9]; // Global octave array for channels 1-8
extern unsigned int maxX;
extern unsigned int pulseCount;
extern bool isNowPlaying;
extern bool MIDI_TRANSPORT_SEND;
extern bool SMP_PATTERN_MODE;
extern unsigned int beat;
struct GlobalVars;
extern struct GlobalVars GLOB;
void triggerExternalOneBlink();
extern IntervalTimer midiClockTimer;

// move these to file-scope so everybody can reset them
static unsigned long lastClockTime = 0;                  // legacy, no longer used for BPM
static unsigned long lastBPMMeasureTime = 0;             // time of last BPM measurement window
static uint32_t clocksSinceLastBPM = 0;                  // number of MIDI clocks since last BPM measurement
static float smoothedBPM = 0.0f;                         // internal smoothed BPM estimate (Kalman filter state)
static float bpmEstimate = 0.0f;                         // Kalman filter: current BPM estimate
static float bpmEstimateError = 1.0f;                    // Kalman filter: estimate error covariance
static const float BPM_PROCESS_NOISE = 0.8f;             // Kalman filter: process noise (how much BPM can change) - LOWER = harder filtering
static const float BPM_MEASUREMENT_NOISE = 0.8f;         // Kalman filter: measurement noise (BPM measurement uncertainty) - HIGHER = harder filtering
static float displayedBPM = 0.0f;                        // BPM value committed to SMP.bpm / UI
static unsigned long lastBPMUpdateMillis = 0;            // last time we updated displayed BPM
static const unsigned long BPM_UPDATE_INTERVAL_MS = 200; // minimum time between BPM UI updates
static const float BPM_UPDATE_THRESHOLD = 0.5f;          // minimum delta to force BPM UI update
static const uint32_t CLOCKS_PER_BPM_WINDOW = 24 * 4 * 4; // 4/4: 24 clocks/beat * 4 beats/bar * 4 bars = 384 clocks (calculate BPM every 4 bars)
static uint8_t midiClockTicks = 0;
static uint8_t externalStepWithinPage = 0;  // Tracks external clock-derived step (1..maxX)
static float midiClockSendBPM = 0.0f;
static unsigned long midiClockIntervalUs = 0;
static unsigned long midiNextClockMicros = 0;

static void markExternalOne() {
  midiClockTicks = 0;
  pulseCount = 0;
  if (maxX == 0) {
    externalStepWithinPage = 0;
    return;
  }
  externalStepWithinPage = 1;
  //Serial.println("ONE");
  triggerExternalOneBlink();
}
static unsigned long lastClockSent = 0;

// ISR-style callback driven by IntervalTimer for master MIDI clock
void midiClockTick() {
  if (!MIDI_CLOCK_SEND) return;
  MIDI.sendRealTime(midi::Clock);
}

static inline void configureMidiClockSend(float bpm, unsigned long nowMicros) {
  if (bpm < 1.0f) bpm = 1.0f;
  midiClockSendBPM = bpm;
  midiClockIntervalUs = (unsigned long)(60000000.0f / (bpm * 24.0f));
  if (midiClockIntervalUs == 0) midiClockIntervalUs = 1;
  midiNextClockMicros = nowMicros;
  midiClockTimer.begin(midiClockTick, midiClockIntervalUs);
}

void checkMidi() {
  // No serial output for individual MIDI messages - only show calculated BPM
  // Only Clock messages (0xF8) are used for BPM calculation
  // Active Sensing (0xFE), Note messages, and others are IGNORED for BPM
  
  // CRITICAL: Process Clock messages with ABSOLUTE PRIORITY and minimal overhead
  // When Note messages are in the buffer, they can delay Clock processing, causing timing jitter
  // Solution: Check message type FIRST, process Clock immediately with timestamp capture
  // before ANY other processing (including MIDI.getChannel() or MIDI.getData() calls)
  
  while (MIDI.read()) {
    // Get message type FIRST - this is fast and doesn't process the message
    uint8_t miditype = MIDI.getType();
    
    // Handle Clock messages FIRST with zero overhead - capture timestamp IMMEDIATELY
    // This must happen before ANY other MIDI library calls to avoid timing delays
    if (miditype == midi::Clock) {
      unsigned long now = micros();  // Capture timestamp IMMEDIATELY
      if (!MIDI_CLOCK_SEND) {
        myClock(now);  // Process Clock - ONLY increments counter, no other overhead
      }
      continue;  // Skip all other processing for Clock messages
    }
    
    // Process all other message types (non-time-critical) - these can have overhead
    uint8_t pitch, velocity, channel;
    channel = MIDI.getChannel();

    switch (miditype) {
      case midi::NoteOn:
        pitch = MIDI.getData1();
        velocity = MIDI.getData2() ? MIDI.getData2() : 127;
        //handleNoteOn(GLOB.currentChannel, pitch, velocity);
        break;

      case midi::NoteOff:
        pitch = MIDI.getData1();
        velocity = MIDI.getData2();
        handleNoteOff(GLOB.currentChannel, pitch - 60, velocity);
        break;

      case midi::Start:
        handleStart();
        break;

      case midi::Stop:
        handleStop();
        break;

      case midi::SongPosition:
        handleSongPosition(MIDI.getData1() | (MIDI.getData2() << 7));
        break;

      case midi::TimeCodeQuarterFrame:
        handleTimeCodeQuarterFrame(MIDI.getData1());
        break;

      default:
        // Filter out messages that should NOT affect BPM calculation:
        // - Active Sensing (0xFE) - ignore completely
        // - Tune Request, System Exclusive, etc. - ignore
        // Only Clock messages (0xF8) are used for BPM calculation
        break;
    }
  }

  // If this device is sending MIDI Clock (master), keep timer in sync with SMP.bpm.
  if (MIDI_CLOCK_SEND) {
    // If BPM changed since last configure, update the timer interval
    if (fabsf((float)SMP.bpm - midiClockSendBPM) > 0.01f && SMP.bpm > 0.0f) {
      unsigned long now = micros();
      configureMidiClockSend((float)SMP.bpm, now);
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
    unsigned long now = micros();
    configureMidiClockSend((float)SMP.bpm, now);
    lastClockSent = now;
    // Ensure playTimer is configured if master
    if (SMP.bpm > 0.0f) {
        unsigned long currentPlayNoteInterval = (unsigned long)((60000000.0f / SMP.bpm) / 4.0f);
        playTimer.begin(playNote, currentPlayNoteInterval);
    } else {
        playTimer.end();
    }
  } else { // SLAVE MODE - run internal timer, sync with external clock
    // Stop MIDI clock timer when not master
    midiClockTimer.end();
    // Reset BPM averaging state for slave
    clocksSinceLastBPM = 0;
    lastBPMMeasureTime = 0;
    // Reset Kalman filter state when switching to slave mode
    bpmEstimate = 0.0f;
    bpmEstimateError = 1.0f;
    
    // Start internal timer with current BPM (will be adjusted by external clock)
    if (SMP.bpm > 0.0f) {
        unsigned long currentPlayNoteInterval = (unsigned long)((60000000.0f / SMP.bpm) / 4.0f);
        playTimer.begin(playNote, currentPlayNoteInterval);
        Serial.print("SLAVE: Started internal timer at ");
        Serial.print(SMP.bpm);
        Serial.println(" BPM");
    } else {
        playTimer.end();
    }
  }
}

void myClock(unsigned long now_captured) { // Renamed 'now' for clarity
  // SAFETY: This function should ONLY be called for MIDI Clock messages (0xF8)
  // Do NOT call this for Note messages, Active Sensing (0xFE), or any other message type!
  // DO NOT calculate BPM here - just count clocks for later calculation
  
  if (MIDI_CLOCK_SEND) { // Safeguard: Should only run in slave mode (EXT mode)
    return;
  }

  // --- Count Clock messages only (NO BPM calculation during clock reception) ---
  // Count ONLY MIDI Clock messages (0xF8) - will use this count later for BPM calculation
  // Ignore all other messages (Notes, Active Sensing, etc.)
  clocksSinceLastBPM++;

  if (lastBPMMeasureTime == 0) {
    // First tick after reset: just remember the time
    lastBPMMeasureTime = now_captured;
    return; // Don't calculate yet - need more clocks
  }

  // Only calculate BPM when we've accumulated a full window of clocks (384 clocks = 4 bars)
  if (clocksSinceLastBPM >= CLOCKS_PER_BPM_WINDOW) {
    // Measure BPM over 4 bars and update SMP.bpm directly in EXT mode
    unsigned long deltaTotal = now_captured - lastBPMMeasureTime;

    // Expected delta per MIDI clock at BPM_MIN/BPM_MAX
    const unsigned long ABSOLUTE_MIN_DELTA = 7000;   // ~350 BPM upper bound
    const unsigned long ABSOLUTE_MAX_DELTA = 75000;  // ~32 BPM lower bound

    float intervalPerClock = (float)deltaTotal / (float)clocksSinceLastBPM;

    if (intervalPerClock >= ABSOLUTE_MIN_DELTA * 0.8f &&
        intervalPerClock <= ABSOLUTE_MAX_DELTA * 1.2f) {

      // Calculate raw BPM from clock data
      float rawBPM = 60000000.0f / (intervalPerClock * 24.0f);

      // Clamp to valid range before filtering
      float clampedRawBPM = rawBPM;
      if (rawBPM < BPM_MIN) clampedRawBPM = BPM_MIN;
      if (rawBPM > BPM_MAX) clampedRawBPM = BPM_MAX;

      // Kalman filter for BPM smoothing
      float filteredBPM;
      float kalmanGain;
      
      // Initialize filter if this is the first measurement
      if (bpmEstimate == 0.0f) {
        filteredBPM = clampedRawBPM;
        bpmEstimateError = BPM_MEASUREMENT_NOISE;
        kalmanGain = 1.0f;  // Full trust in first measurement
      } else {
        // Prediction step: predict next state (BPM stays mostly the same)
        float predictedBPM = bpmEstimate;  // Assuming BPM doesn't change much between measurements
        float predictedError = bpmEstimateError + BPM_PROCESS_NOISE;
        
        // Update step: combine prediction with measurement
        kalmanGain = predictedError / (predictedError + BPM_MEASUREMENT_NOISE);
        filteredBPM = predictedBPM + kalmanGain * (clampedRawBPM - predictedBPM);
        bpmEstimateError = (1.0f - kalmanGain) * predictedError;
      }
      
      // Update filter state
      bpmEstimate = filteredBPM;
      smoothedBPM = filteredBPM;

      // Serial debug: show raw BPM calculation and filtered value
      Serial.print("BPM CALC (EXT mode): Raw=");
      Serial.print(rawBPM, 2);
      Serial.print(" | Filtered=");
      Serial.print(filteredBPM, 2);
      Serial.print(" | Gain=");
      Serial.print(kalmanGain, 3);
      Serial.print(" | Clocks=");
      Serial.print(clocksSinceLastBPM);
      Serial.print(" | Delta(us)=");
      Serial.print(deltaTotal);
      Serial.print(" | Interval/Clock(us)=");
      Serial.print(intervalPerClock, 2);

      // Round filtered BPM to nearest integer
      int newBPM = round(filteredBPM);
      
      // Clamp final BPM to valid range
      if (newBPM < BPM_MIN) newBPM = BPM_MIN;
      if (newBPM > BPM_MAX) newBPM = BPM_MAX;
      
      // UPDATE SMP.bpm from filtered value (but DON'T sync sequencer timer)
      // Just update the BPM value - sequencer continues using its own internal timer
      // This allows the BPM display to match external clock while sequencer runs independently
      SMP.bpm = (float)newBPM;
      
      Serial.print(" | Updated SMP.bpm=");
      Serial.print(SMP.bpm);
      Serial.println(" -> BPM value synced to external clock (sequencer timer unchanged)");
    } else {
      Serial.print("BPM CALC: REJECTED - intervalPerClock=");
      Serial.print(intervalPerClock, 2);
      Serial.print(" (out of range ");
      Serial.print(ABSOLUTE_MIN_DELTA * 0.8f);
      Serial.print(" - ");
      Serial.print(ABSOLUTE_MAX_DELTA * 1.2f);
      Serial.print(") | Clocks=");
      Serial.print(clocksSinceLastBPM);
      Serial.print(" | Delta(us)=");
      Serial.println(deltaTotal);
    }

    // Reset window for next measurement
    lastBPMMeasureTime = now_captured;
    clocksSinceLastBPM = 0;
  }

  // --- Handle Start command sync ---
  // Keep pulseCount for transport sync, but don't trigger steps
  pulseCount = (pulseCount + 1) % (24 * 4); // 96 pulses = one full 4/4 bar
  // Track external steps from MIDI clock (1..maxX) for debugging
  midiClockTicks++;
  if (midiClockTicks >= clocksPerStep) {
    midiClockTicks = 0;
    if (maxX == 0) {
      externalStepWithinPage = 0;
    } else {
      externalStepWithinPage++;
      if (externalStepWithinPage > maxX) {
        externalStepWithinPage = 1;
      }
      if (externalStepWithinPage == 1) {
        //  Serial.println("ONE");
        triggerExternalOneBlink();

        if (pendingStartOnBar) {
          Serial.println("myClock: Slave Start - synced on beat 1");
          pendingStartOnBar = false;
          isNowPlaying = true;
          if (SMP_PATTERN_MODE) {
            beat = (GLOB.edit - 1) * maxX + 1;  // Start from first beat of current page
            GLOB.page = GLOB.edit;  // Keep the current page
          } else {
            beat = 1;
            GLOB.page = 1;
          }
          playStartTime = millis();

          // Immediately trigger the first step so playback starts on beat 1
          if (SMP.bpm > 0) {
            unsigned long currentPlayNoteInterval = (unsigned long)((60000000.0f / SMP.bpm) / 4.0f);
            playTimer.end();
            playNote();  // Fire the downbeat right away
            playTimer.begin(playNote, currentPlayNoteInterval);  // Re-align timer phase
          } else {
            playNote();
          }
        }
      }
    }
  }
  
  // NOTE: stepIsDue is now triggered by playTimer (internal), not by external clock
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
  // Determine channel based on MIDI mode
  extern int voiceSelect;
  
  if (voiceSelect == 2) {
    // KEYS mode: Map MIDI note to channel (C4=60 -> ch1, C#4=61 -> ch2, etc.)
    // Wrapping: C4-B4 (60-71) = ch1-12, C5-B5 (72-83) = ch1-12, etc.
    ch = ((pitch - 60) % 12) + 1;
    if (ch < 1) ch += 12;  // Handle notes below C4
    if (ch > 8) ch = 8;    // Limit to 8 sample channels
  } else if (!MIDI_VOICE_SELECT) {
    // YPOS mode: use current channel
    ch = GLOB.currentChannel;
  }
  // else: MIDI mode uses the ch parameter as-is

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
      if (GLOB.singleMode) {
        // Store for grid write on next beat
        pendingNotes.push_back({ .pitch = pitch,
                                 .velocity = velocity,
                                 .channel = (uint8_t)ch,
                                 .livenote = (uint8_t)livenote });
      }else{

        if (ch < 9) {
          int samplePitch;
          if (voiceSelect == 2) {
            // KEYS mode: Play at base pitch (no MIDI pitch transposition)
            samplePitch = SampleRate[ch] * 12;
          } else {
            // YPOS/MIDI mode: Transpose by MIDI pitch
            samplePitch = ((SampleRate[ch] * 12) + pitch - 60);
          }
          // Apply detune offset for channels 1-12 (excluding synth channels 13-14)
          if (ch >= 1 && ch <= 12) {
            samplePitch += (int)detune[ch]; // Add detune semitones
          }
          // Apply octave offset for channels 1-8 (excluding synth channels 13-14)
          if (ch >= 1 && ch <= 8) {
            samplePitch += (int)(channelOctave[ch] * 12); // Add octave semitones (12 semitones per octave)
          }
          _samplers[ch].noteEvent(samplePitch, velocity, true, false);
    } else if (ch > 12 && ch < 15) {
      playSynth(ch, livenote, velocity, true);
    } else if (ch == 11) {

      // Map MIDI pitch to match grid rows: MIDI 60 (middle C) = row 6
      // Grid formula: 12 * octave[0] + transpose + (row - 1)
      // So: MIDI pitch - 55 = row - 1 (fixed: was -49, off by 6 semitones)
      playSound(12 * octave[0] + transpose + pitch - 55, 0);
    }
      }
      // Always play the note immediately
      activeNotes[pitch] = true;
    }
    else{ 
    // Live mode: play note and show light
    //light(mapXtoPageOffset(GLOB.x), livenote, CRGB(255, 255, 255));
    //FastLED.show();

    if (ch < 9) {
      int samplePitch;
      if (voiceSelect == 2) {
        // KEYS mode: Play at base pitch (no MIDI pitch transposition)
        samplePitch = SampleRate[ch] * 12;
      } else {
        // YPOS/MIDI mode: Transpose by MIDI pitch
        samplePitch = ((SampleRate[ch] * 12) + pitch - 60);
      }
      // Apply detune offset for channels 1-12 (excluding synth channels 13-14)
      if (ch >= 1 && ch <= 12) {
        samplePitch += (int)detune[ch]; // Add detune semitones
      }
      // Apply octave offset for channels 1-8 (excluding synth channels 13-14)
      if (ch >= 1 && ch <= 8) {
        samplePitch += (int)(channelOctave[ch] * 12); // Add octave semitones (12 semitones per octave)
      }
      _samplers[ch].noteEvent(samplePitch, velocity, true, false);
    } else if (ch > 12 && ch < 15) {
      playSynth(ch, livenote, velocity, true);
    } else if (ch == 11) {

      // Map MIDI pitch to match grid rows: MIDI 60 (middle C) = row 6
      // Grid formula: 12 * octave[0] + transpose + (row - 1)
      // So: MIDI pitch - 55 = row - 1 (fixed: was -49, off by 6 semitones)
      playSound(12 * octave[0] + transpose + pitch - 55, 0);
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

  markExternalOne(); // define "ONE" on incoming Play signal

  //Serial.println("MIDI Start Received");

  // 1) reset everything for beat 1
  if (SMP_PATTERN_MODE) {
    beat = (GLOB.edit - 1) * maxX + 1;  // Start from first beat of current page
    GLOB.page = GLOB.edit;  // Keep the current page
  } else {
    beat = 1;
    GLOB.page = 1;
  }
  deleteActiveCopy();

  if (MIDI_CLOCK_SEND && MIDI_TRANSPORT_SEND) {
    // If we're master and also listening to incoming start, re-broadcast start
    MIDI.sendRealTime(midi::Start);
  }

  // Start immediately on the transport command
  pendingStartOnBar = false;
  isNowPlaying = true;
  beatStartTime = millis();

  // Fire the very first step right away so playback starts on beat 1
  if (SMP.bpm > 0) {
    unsigned long currentPlayNoteInterval = (unsigned long)((60000000.0f / SMP.bpm) / 4.0f);
    playTimer.end();
    playNote();  // Fire the downbeat right away
    playTimer.begin(playNote, currentPlayNoteInterval);  // Re-align timer phase
  } else {
    playNote();
  }
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
