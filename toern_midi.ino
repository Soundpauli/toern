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

// Reduce Serial spam in clock/BPM code paths (can stall Serial and hurt responsiveness).
#ifndef DEBUG_MIDI_CLOCK_SERIAL
#define DEBUG_MIDI_CLOCK_SERIAL 0
#endif

// move these to file-scope so everybody can reset them
static unsigned long lastClockTime = 0;                  // legacy, no longer used for BPM
static unsigned long lastBPMMeasureTime = 0;             // time of last BPM measurement window
static uint32_t clocksSinceLastBPM = 0;                  // number of MIDI clocks since last BPM measurement
static float smoothedBPM = 0.0f;                         // internal smoothed BPM estimate (Kalman filter state)
static float bpmEstimate = 0.0f;                         // Kalman filter: current BPM estimate
static float bpmEstimateError = 1.0f;                    // Kalman filter: estimate error covariance
static const float BPM_PROCESS_NOISE = 0.8f;             // Kalman filter: process noise (how much BPM can change) - LOWER = harder filtering
static const float BPM_MEASUREMENT_NOISE = 0.5f;         // Kalman filter: measurement noise (BPM measurement uncertainty) - HIGHER = harder filtering (reduced from 0.8 for faster sync)
static float displayedBPM = 0.0f;                        // BPM value committed to SMP.bpm / UI
static unsigned long lastBPMUpdateMillis = 0;            // last time we updated displayed BPM
static const unsigned long BPM_UPDATE_INTERVAL_MS = 200; // minimum time between BPM UI updates
static const float BPM_UPDATE_THRESHOLD = 0.5f;          // minimum delta to force BPM UI update
static const uint32_t CLOCKS_PER_BPM_WINDOW = 24 * 4;    // 4/4: 24 clocks/beat * 4 beats/bar = 96 clocks (calculate BPM every 1 bar for faster sync)
static const unsigned long NO_CLOCK_TIMEOUT_US = 2000000; // 2 seconds: if no clock received, reset Kalman state
static unsigned long lastClockReceivedTime = 0;          // Timestamp of last received clock
static int lastStableBPM = 0;                            // Last BPM value for stability tracking
static int stableBPMCount = 0;                           // Count of consecutive same BPM values
static bool isBPMStable = false;                         // True if BPM has been stable (>2 consecutive same values)
static uint8_t midiClockTicks = 0;  // Resets every step (for step tracking)
static uint16_t midiClockTickCounter = 0;  // Continuous counter for blinking (never resets, wraps at 65535)
static bool initialBpmSyncDone = false;  // Track if initial background BPM sync is complete
static const uint8_t MAX_MIDI_MESSAGES_PER_LOOP = 16;   // Increased limit for faster NoteOn processing (was 4)

// Function to check if external BPM is stable (for UI display)
bool getBPMStable() {
  return isBPMStable;
}

// Function to get current MIDI clock tick count (for arrow blinking)
// Returns a counter that increments on every clock pulse (for blinking sync)
uint16_t getMidiClockTicks() {
  return midiClockTickCounter;
}
static uint8_t externalStepWithinPage = 0;  // Tracks external clock-derived step (1..maxX)
static float midiClockSendBPM = 0.0f;
static unsigned long midiClockIntervalUs = 0;
static unsigned long midiNextClockMicros = 0;

static void markExternalOne() {
  midiClockTicks = 0;
  // Don't reset midiClockTickCounter - keep it continuous for blinking
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

// ISR-style callback driven by dedicated IntervalTimer for master MIDI clock
// Writes directly to Serial8 hardware to bypass MIDI library buffering and avoid blocking
// This ensures precise timing even when sending many MIDI notes
// IMPORTANT: Timer always runs in background - never stopped to maintain precise timing
void midiClockTick() {
  // Always send clock if in master mode - timer never stops, even during pause/stop
  if (MIDI_CLOCK_SEND) {
    // Write MIDI clock byte (0xF8) directly to Serial8 hardware
    // This bypasses the MIDI library buffer and won't block even when sending many notes
    // Serial8.write() is non-blocking if hardware buffer has space (which it should for single byte)
    // Serial8 is already declared globally (from Teensy core)
    // Note: SERIAL8_TX_BUFFER_SIZE is set to 128 in toern.ino to ensure adequate buffer space
    Serial8.write(0xF8);  // MIDI Clock message (realtime message, single byte)
    // Never skip clock pulses - missing a pulse would cause drift
  }
}

static inline void configureMidiClockSend(float bpm, unsigned long nowMicros) {
  // Use exact rounded BPM value for precise MIDI clock output
  int roundedBPM = (int)round(bpm);
  if (roundedBPM < 1) roundedBPM = 1;
  if (roundedBPM > BPM_MAX) roundedBPM = BPM_MAX;
  
  // Calculate interval with high precision using floating point, then round to nearest microsecond.
  // Formula: 60,000,000 microseconds per minute / (BPM * 24 clocks per quarter note)
  // Using a fixed period timer avoids per-tick timer reprogramming jitter.
  double intervalUsDouble = 60000000.0 / ((double)roundedBPM * 24.0);
  midiClockIntervalUs = (unsigned long)round(intervalUsDouble);
  if (midiClockIntervalUs == 0) midiClockIntervalUs = 1;
  
  midiClockSendBPM = (float)roundedBPM;
  midiNextClockMicros = nowMicros;
  
  // Configure dedicated IntervalTimer for MIDI clock output (separate from MIDI input/output)
  // Set highest priority (0 = highest on ARM) to ensure precise timing
  midiClockTimer.begin(midiClockTick, midiClockIntervalUs);
  midiClockTimer.priority(0);  // Highest priority - MIDI clock must be precise
  
  // Verify actual BPM from calculated interval (for debugging)
  double actualBPM = 60000000.0 / ((double)midiClockIntervalUs * 24.0);
  
  #if DEBUG_MIDI_CLOCK_SERIAL
    Serial.print("MIDI Clock configured: ");
    Serial.print(roundedBPM);
    Serial.print(" BPM, interval: ");
    Serial.print(midiClockIntervalUs);
    Serial.print(" us, actual: ");
    Serial.print(actualBPM, 3);
    Serial.println(" BPM (dedicated timer, direct Serial8 write)");
  #endif
}

// Public function to update MIDI clock with exact BPM value (for use from updateBPM)
void updateMidiClockOutput() {
  if (MIDI_CLOCK_SEND) {
    unsigned long now = micros();
    // Use exact rounded BPM value from SMP.bpm
    int roundedBPM = (int)round(SMP.bpm);
    if (roundedBPM < 1) roundedBPM = 1;
    if (roundedBPM > BPM_MAX) roundedBPM = BPM_MAX;
    configureMidiClockSend((float)roundedBPM, now);
  }
}

// MIDI library callback for realtime Clock messages
void handleMidiClock() {
  // Process clock with precise timestamp
  myClock(micros());
}

void checkMidi() {
  // No serial output for individual MIDI messages - only show calculated BPM
  // Only Clock messages (0xF8) are used for BPM calculation
  // Active Sensing (0xFE), Note messages, and others are IGNORED for BPM
  
  // CRITICAL: Process Clock messages with ABSOLUTE PRIORITY and minimal overhead
  // When Note messages are in the buffer, they can delay Clock processing, causing timing jitter
  // Solution: Check message type FIRST, process Clock immediately with timestamp capture
  // before ANY other processing (including MIDI.getChannel() or MIDI.getData() calls)
  
  uint8_t processedMessages = 0;
  while (processedMessages < MAX_MIDI_MESSAGES_PER_LOOP && MIDI.read()) {
    // Get message type FIRST - this is fast and doesn't process the message
    uint8_t miditype = MIDI.getType();
    
    // Clock messages are handled via MIDI callback (handleMidiClock) for immediate processing
    // MIDI.read() already triggered the callback, so skip manual processing
    if (miditype == midi::Clock) {
      continue;
    }
    
    // NoteOn messages are handled via MIDI callback (handleNoteOn) for immediate processing
    // MIDI.read() already triggered the callback, so skip manual processing
    // This reduces latency - callback is triggered immediately when message arrives
    // rather than processing it later in this loop
    if (miditype == midi::NoteOn) {
      continue;  // Skip - callback already handled it immediately
    }
    
    // Process all other message types (non-time-critical) - these can have overhead
    uint8_t pitch, velocity, channel;
    channel = MIDI.getChannel();
    processedMessages++;

    switch (miditype) {

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

      case midi::PitchBend: {
        // Map pitch bend (14-bit) to current channel fast filter
        uint16_t bend14 = (uint16_t)((MIDI.getData1() & 0x7F) | (MIDI.getData2() << 7));
        applyExternalFastFilter(bend14);
        break;
      }

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
  // Use exact rounded BPM value for precise clock output
  if (MIDI_CLOCK_SEND) {
    // Round SMP.bpm to integer and check if it changed
    int currentRoundedBPM = (int)round(SMP.bpm);
    int lastRoundedBPM = (int)round(midiClockSendBPM);
    
    // Update if rounded BPM changed (check integer values, not float precision)
    if (currentRoundedBPM != lastRoundedBPM && currentRoundedBPM > 0) {
      unsigned long now = micros();
      configureMidiClockSend((float)currentRoundedBPM, now);
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
    // Use exact rounded BPM value for precise clock output
    int roundedBPM = (int)round(SMP.bpm);
    if (roundedBPM < 1) roundedBPM = 1;
    // IMPORTANT: Don't restart/re-phase the MIDI clock timer unless BPM actually changed.
    // Restarting the IntervalTimer on transport events can cause short-window BPM displays
    // on external gear to "dip/jiggle" even when paused.
    int lastRoundedBPM = (int)round(midiClockSendBPM);
    if (midiClockIntervalUs == 0 || roundedBPM != lastRoundedBPM) {
      configureMidiClockSend((float)roundedBPM, now);
    }
    lastClockSent = now;
    // Ensure playTimer is configured if master
    if (SMP.bpm > 0.0f) {
        unsigned long currentPlayNoteInterval = (unsigned long)((60000000.0f / SMP.bpm) / 4.0f);
        playTimer.begin(playNote, currentPlayNoteInterval);
    } else {
        playTimer.end();
    }
  } else { // SLAVE MODE - run internal timer, sync with external clock
    // Don't stop MIDI clock timer - it should always run in background
    // Timer will just not send clock messages when MIDI_CLOCK_SEND is false
    // Reset BPM averaging state for slave
    clocksSinceLastBPM = 0;
    lastBPMMeasureTime = 0;
    // Reset Kalman filter state when switching to slave mode
    bpmEstimate = 0.0f;
    bpmEstimateError = 1.0f;
    
    // Reset stability tracking when switching to slave mode
    lastStableBPM = 0;
    stableBPMCount = 0;
    isBPMStable = false;
    lastClockReceivedTime = 0;  // Reset clock timeout tracking
    midiClockTickCounter = 0;   // Reset clock tick counter for blinking
    initialBpmSyncDone = false; // Reset initial sync flag - will sync again in background
    
    // Start internal timer with current BPM (will be adjusted by external clock)
    if (SMP.bpm > 0.0f) {
      unsigned long currentPlayNoteInterval = (unsigned long)((60000000.0f / SMP.bpm) / 4.0f);
      playTimer.begin(playNote, currentPlayNoteInterval);
      #if DEBUG_MIDI_CLOCK_SERIAL
        Serial.print("SLAVE: Started internal timer at ");
        Serial.print(SMP.bpm);
        Serial.println(" BPM");
      #endif
    } else {
      playTimer.end();
    }
  }
}

// Note: MIDI clock timer is never stopped - it always runs in background for precise timing
// The timer continues running even during pause/stop operations

void myClock(unsigned long now_captured) { // Renamed 'now' for clarity
  // SAFETY: This function should ONLY be called for MIDI Clock messages (0xF8)
  // Do NOT call this for Note messages, Active Sensing (0xFE), or any other message type!
  // DO NOT calculate BPM here - just count clocks for later calculation
  
  if (MIDI_CLOCK_SEND) { // Safeguard: Should only run in slave mode (EXT mode)
    return;
  }

  // Check if we've been without clock for too long - reset Kalman state if so
  // (check BEFORE updating lastClockReceivedTime)
  if (lastClockReceivedTime > 0 && (now_captured - lastClockReceivedTime) > NO_CLOCK_TIMEOUT_US) {
    // No clock received for >2 seconds: reset Kalman filter to prevent stale state
    bpmEstimate = 0.0f;
    bpmEstimateError = 1.0f;
    lastBPMMeasureTime = 0;
    clocksSinceLastBPM = 0;
    lastStableBPM = 0;
    stableBPMCount = 0;
    isBPMStable = false;
    #if DEBUG_MIDI_CLOCK_SERIAL
      Serial.println("BPM: No clock received for >2s - resetting Kalman filter state");
    #endif
  }
  
  // Update last clock received time
  lastClockReceivedTime = now_captured;

  // --- Count Clock messages only (NO BPM calculation during clock reception) ---
  // Count ONLY MIDI Clock messages (0xF8) - will use this count later for BPM calculation
  // Ignore all other messages (Notes, Active Sensing, etc.)
  clocksSinceLastBPM++;

  if (lastBPMMeasureTime == 0) {
    // First tick after reset: just remember the time
    lastBPMMeasureTime = now_captured;
    return; // Don't calculate yet - need more clocks
  }

  // --- Handle Start command sync and step tracking FIRST (always needed) ---
  // Keep pulseCount for transport sync, but don't trigger steps
  pulseCount = (pulseCount + 1) % (24 * 4); // 96 pulses = one full 4/4 bar
  // Track external steps from MIDI clock (1..maxX) for debugging
  midiClockTicks++;
  midiClockTickCounter++;  // Increment continuous counter for blinking (wraps at 65535)
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
          #if DEBUG_MIDI_CLOCK_SERIAL
            Serial.println("myClock: Slave Start - synced on beat 1");
          #endif
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

  // Only calculate BPM when we've accumulated a full window of clocks (96 clocks = 1 bar for faster sync)
  // Calculate if: (1) on BPM page, OR (2) initial sync not done yet (background sync until stable)
  if (clocksSinceLastBPM >= CLOCKS_PER_BPM_WINDOW) {
    // Check if we're on the BPM page
    extern Mode *currentMode;
    extern Mode volume_bpm;
    bool onBpmPage = (currentMode == &volume_bpm);
    
    // Allow calculation if on BPM page OR if initial sync not done yet
    if (!onBpmPage && initialBpmSyncDone) {
      // Not on BPM page and initial sync already done: skip calculation but reset window to prevent overflow
      lastBPMMeasureTime = now_captured;
      clocksSinceLastBPM = 0;
      return; // Skip expensive BPM calculation
    }
    
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
      #if DEBUG_MIDI_CLOCK_SERIAL
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
      #endif

      // Round filtered BPM to nearest integer
      int newBPM = round(filteredBPM);
      
      // Clamp final BPM to valid range
      if (newBPM < BPM_MIN) newBPM = BPM_MIN;
      if (newBPM > BPM_MAX) newBPM = BPM_MAX;

      // Track stability: count consecutive same BPM values
      if (lastStableBPM == 0) {
        // First measurement: initialize
        lastStableBPM = newBPM;
        stableBPMCount = 1;
        isBPMStable = false;
      } else if (newBPM == lastStableBPM) {
        // Same BPM value: increment counter
        stableBPMCount++;
        // BPM is stable if we've seen the same value more than 2 times
        bool wasStable = isBPMStable;
        isBPMStable = (stableBPMCount > 2);
        
        // If BPM just became stable, sync playTimer and mark initial sync complete if needed
        if (isBPMStable && !wasStable) {
          // Sync playTimer to the stable BPM
          extern IntervalTimer playTimer;
          extern void playNote();
          if (newBPM > 0) {
            unsigned long currentPlayNoteInterval = (unsigned long)((60000000.0f / (float)newBPM) / 4.0f);
            playTimer.begin(playNote, currentPlayNoteInterval);
            #if DEBUG_MIDI_CLOCK_SERIAL
              Serial.print("BPM: Synced playTimer to stable BPM ");
              Serial.println(newBPM);
            #endif
          }
          
          // If initial sync not done, mark it complete
          if (!initialBpmSyncDone) {
            initialBpmSyncDone = true;
            #if DEBUG_MIDI_CLOCK_SERIAL
              Serial.print("BPM INITIAL SYNC COMPLETE: Stable at ");
              Serial.print(newBPM);
              Serial.println(" BPM (background sync done, will only recalculate on BPM page now)");
            #endif
          }
        }
      } else {
        // BPM changed, reset stability counter
        stableBPMCount = 1;
        lastStableBPM = newBPM;
        isBPMStable = false;
      }

      // UPDATE SMP.bpm from filtered value (but DON'T sync sequencer timer)
      // Just update the BPM value - sequencer continues using its own internal timer
      // This allows the BPM display to match external clock while sequencer runs independently
      SMP.bpm = (float)newBPM;
      
      #if DEBUG_MIDI_CLOCK_SERIAL
        Serial.print(" | Updated SMP.bpm=");
          Serial.print(SMP.bpm);
        Serial.println(" -> BPM value synced to external clock (sequencer timer unchanged)");
      #endif
    } else {
      #if DEBUG_MIDI_CLOCK_SERIAL
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
      #endif
    }

    // Reset window for next measurement
    lastBPMMeasureTime = now_captured;
    clocksSinceLastBPM = 0;
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
        // Store for grid write on next beat (ISR-safe ring buffer).
        extern bool enqueuePendingNote(uint8_t pitch, uint8_t velocity, uint8_t channel, uint8_t livenote);
        enqueuePendingNote(pitch, velocity, (uint8_t)ch, (uint8_t)livenote);
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
  
  PendingNote pn;
  extern bool dequeuePendingNote(PendingNote &out);
  while (dequeuePendingNote(pn)) {
    int targetBeat = beat; //(beat == 1) ? (maxX * lastPage) : (beat);
    note[targetBeat][pn.livenote].channel = pn.channel;
    note[targetBeat][pn.livenote].velocity = pn.velocity;
  }
  

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
    // Send early for better sync
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
