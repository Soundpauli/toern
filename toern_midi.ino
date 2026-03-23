// External variables
extern float detune[13]; // Global detune array for channels 1-12
extern float channelOctave[9]; // Global octave array for channels 1-8
extern unsigned int maxX;
extern unsigned int pulseCount;
extern bool isNowPlaying;
extern bool MIDI_TRANSPORT_SEND;
extern bool SMP_PATTERN_MODE;
extern unsigned int beat;
extern uint8_t transportSendDelayMs;
extern uint8_t transportRcveDelayMs;
struct GlobalVars;
extern struct GlobalVars GLOB;
void triggerExternalOneBlink();
extern IntervalTimer midiClockTimer;
void handleNoteOff(uint8_t midiChannel, uint8_t pitch, uint8_t velocity);
void stopSynthChannel(int ch);

// Reduce Serial spam in clock/BPM code paths (can stall Serial and hurt responsiveness).
#ifndef DEBUG_MIDI_CLOCK_SERIAL
#define DEBUG_MIDI_CLOCK_SERIAL 0
#endif
#ifndef DEBUG_MIDI_RX_SERIAL
#define DEBUG_MIDI_RX_SERIAL 0
#endif

static inline void logMidiRxNoteOn(uint8_t ch, uint8_t pitch, uint8_t velocity) {
#if DEBUG_MIDI_RX_SERIAL
  Serial.print("MIDI RX NoteOn  ch=");
  Serial.print(ch);
  Serial.print(" note=");
  Serial.print(pitch);
  Serial.print(" vel=");
  Serial.println(velocity);
#endif
}

static inline void logMidiRxNoteOff(uint8_t ch, uint8_t pitch, uint8_t velocity) {
#if DEBUG_MIDI_RX_SERIAL
  Serial.print("MIDI RX NoteOff ch=");
  Serial.print(ch);
  Serial.print(" note=");
  Serial.print(pitch);
  Serial.print(" vel=");
  Serial.println(velocity);
#endif
}

static inline void logMidiRxSimple(const char *msg) {
#if DEBUG_MIDI_RX_SERIAL
  Serial.print("MIDI RX ");
  Serial.println(msg);
#endif
}

static inline void logMidiRxPitchBend(uint8_t ch, uint16_t bend14) {
#if DEBUG_MIDI_RX_SERIAL
  Serial.print("MIDI RX PitchBend ch=");
  Serial.print(ch);
  Serial.print(" value=");
  Serial.println(bend14);
#endif
}

static inline void logMidiRxSongPosition(uint16_t beats) {
#if DEBUG_MIDI_RX_SERIAL
  Serial.print("MIDI RX SongPosition beats=");
  Serial.println(beats);
#endif
}

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
static unsigned long transportStartDelayUntil = 0;
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
// Track held notes per logical channel to avoid stuck counters on duplicate NoteOn events.
static bool midiHeldNote[17][128] = { false };

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
  // Fire deferred transport start once the non-blocking delay has elapsed
  if (transportStartDelayUntil && micros() >= transportStartDelayUntil) {
    transportStartDelayUntil = 0;
    isNowPlaying = true;
    playStartTime = millis();
    if (SMP.bpm > 0) {
      unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)SMP.bpm * 4.0));
      playTimer.end();
      playNote();
      playTimer.begin(playNote, currentPlayNoteInterval);
      if (currentPlayNoteInterval >= 4) {
        fillTimer.begin(playFillNote, currentPlayNoteInterval / 4);
      }
    } else {
      playNote();
    }
    fillHasTriggered = false;
    fillRunning = false;
    fillSubTick = 0;
    fillStartSubTick = 0;
    fillActiveChannel = 0;
    fillActiveVelocity = 0;
    fillActiveRow = 0;
  }

  // CRITICAL: Process Clock messages with ABSOLUTE PRIORITY and minimal overhead
  // When Note messages are in the buffer, they can delay Clock processing, causing timing jitter
  // Solution: Check message type FIRST, process Clock immediately with timestamp capture
  // before ANY other processing (including MIDI.getChannel() or MIDI.getData() calls)

  static const uint8_t MAX_MIDI_MESSAGES_PER_LOOP = 4;
  uint8_t processedMessages = 0;
  while (MIDI.read()) {
    uint8_t miditype = MIDI.getType();

    if (miditype == midi::Clock) continue;
    if (miditype == midi::NoteOn) continue;  // handled by callback

    if (processedMessages >= MAX_MIDI_MESSAGES_PER_LOOP) continue;
    processedMessages++;

    switch (miditype) {
      case midi::NoteOff: {
        uint8_t pitch = MIDI.getData1();
        uint8_t velocity = MIDI.getData2();
        logMidiRxNoteOff(MIDI.getChannel(), pitch, velocity);
        // Same logical channel as handleNoteOn (cable channel + voice mode), not raw 1..16 only.
        handleNoteOff(MIDI.getChannel(), pitch, velocity);
        break;
      }
      case midi::Stop:
        logMidiRxSimple("Stop");
        handleStop();
        break;
      case midi::PitchBend: {
        uint16_t bend14 = (uint16_t)((MIDI.getData1() & 0x7F) | (MIDI.getData2() << 7));
        logMidiRxPitchBend(MIDI.getChannel(), bend14);
        applyExternalFastFilter(bend14);
        break;
      }
      case midi::SongPosition: {
        uint16_t beats = MIDI.getData1() | (MIDI.getData2() << 7);
        logMidiRxSongPosition(beats);
        handleSongPosition(beats);
        break;
      }
      case midi::TimeCodeQuarterFrame:
        logMidiRxSimple("TimeCodeQuarterFrame");
        handleTimeCodeQuarterFrame(MIDI.getData1());
        break;
      default:
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
        unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)SMP.bpm * 4.0));
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
      unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)SMP.bpm * 4.0));
      playTimer.begin(playNote, currentPlayNoteInterval);
      #if DEBUG_MIDI_CLOCK_SERIAL
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
          #endif
          pendingStartOnBar = false;
          // Keep isNowPlaying = false until the deferred playNote() fires,
          // so the background playTimer ISR cannot race and play beat 1 early.
          if (SMP_PATTERN_MODE) {
            beat = (GLOB.edit - 1) * maxX + 1;
            GLOB.page = GLOB.edit;
          } else {
            beat = 1;
            GLOB.page = 1;
          }
          playStartTime = millis();

          // Defer first step by transportRcveDelayMs (0-127). 0 = no delay.
          unsigned long delayUs = (unsigned long)transportRcveDelayMs * 1000UL;
          transportStartDelayUntil = micros() + delayUs;
        }
      }
    }
  }

  // Always calculate BPM in EXT mode whenever a full window of clocks has arrived.
  // This keeps SMP.bpm and isBPMStable current regardless of which screen is active.
  if (clocksSinceLastBPM >= CLOCKS_PER_BPM_WINDOW) {
    // Measure BPM over the window and update SMP.bpm directly in EXT mode
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
        
        // If BPM just became stable, sync playTimer and auto-exit BPM menu if open
        if (isBPMStable && !wasStable) {
          extern IntervalTimer playTimer;
          extern void playNote();
          if (newBPM > 0) {
            unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)newBPM * 4.0));
            // Only re-phase (begin) if idle - avoids sync jumps during playback
            // or while waiting for bar-1 (pendingStartOnBar). In those states just
            // update the period so the phase that was already locked stays intact.
            if (!isNowPlaying && !pendingStartOnBar) {
              playTimer.begin(playNote, currentPlayNoteInterval);
            } else {
              playTimer.update(currentPlayNoteInterval);
            }
          }

          if (!initialBpmSyncDone) {
            initialBpmSyncDone = true;
          }

          // Auto-exit BPM menu when clock locks
          extern Mode *currentMode;
          extern Mode volume_bpm;
          if (currentMode == &volume_bpm) {
            extern void switchMode(Mode *);
            extern Mode draw;
            extern Mode singleMode;
            extern bool menuEnteredFromSingleMode;
            extern int currentMenuPage;
            currentMenuPage = 3;
            switchMode(menuEnteredFromSingleMode ? &singleMode : &draw);
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
      #endif
    } else {
      #if DEBUG_MIDI_CLOCK_SERIAL
      #endif
    }

    // Reset window for next measurement
    lastBPMMeasureTime = now_captured;
    clocksSinceLastBPM = 0;
  }
  
  // NOTE: stepIsDue is now triggered by playTimer (internal), not by external clock
}







void MidiSendNoteOn(int pitch, int channel, int velocity) {
  // Check if MIDI note sending is enabled
  extern bool MIDI_NOTE_SEND;
  if (!MIDI_NOTE_SEND) {
    return;  // Don't send notes if disabled
  }
  
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
  // skipSave=true: avoid blocking SD write while external clock is still ticking.
  // The local pause() button path will save normally.
  unsigned long currentTime = millis();
  if (currentTime - playStartTime > 500) {
    pause(true);
  }
}




// Map incoming MIDI (cable channel + pitch) to the same logical channel 1..16 as note routing.
static int mapMidiToLogicalChannel(int midiChannel, uint8_t pitch) {
  extern int voiceSelect;
  extern bool MIDI_VOICE_SELECT;

  if (voiceSelect == 2) {
    // KEYS mode: Map MIDI note to channel (C4=60 -> ch1, C#4=61 -> ch2, etc.)
    int ch = ((int)pitch - 60) % 12 + 1;
    if (ch < 1) ch += 12;
    if (ch > 8) ch = 8;
    return ch;
  }
  if (!MIDI_VOICE_SELECT) {
    // YPOS mode: use currently selected channel (13/14 for synth, etc.)
    return (int)GLOB.currentChannel;
  }
  // MIDI mode: use the channel from the MIDI cable
  return midiChannel;
}

void handleNoteOn(int ch, uint8_t pitch, uint8_t velocity) {
  logMidiRxNoteOn((uint8_t)ch, pitch, velocity);
  extern bool MIDI_NOTE_RECEIVE;
  if (!MIDI_NOTE_RECEIVE) return;

  // Many controllers use Note On velocity 0 as release; treat like Note Off.
  if (velocity == 0) {
    handleNoteOff((uint8_t)ch, pitch, velocity);
    return;
  }

  ch = mapMidiToLogicalChannel(ch, pitch);

  if (ch < 1 || ch > 16) return;
  // Only count a NoteOn once per (logical channel, pitch) until a matching NoteOff arrives.
  if (pitch < 128 && !midiHeldNote[ch][pitch]) {
    midiHeldNote[ch][pitch] = true;
  }
  // Keep pressedKeyCount in sync with the held-note bitmap (fixes stuck / missed releases).
  {
    int cnt = 0;
    for (int p = 0; p < 128; p++) {
      if (midiHeldNote[ch][p]) cnt++;
    }
    pressedKeyCount[ch] = (int16_t)cnt;
  }

  if (pressedKeyCount[ch] > 0) {
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




void handleNoteOff(uint8_t midiChannel, uint8_t pitch, uint8_t velocity) {
  extern bool MIDI_NOTE_RECEIVE;
  if (!MIDI_NOTE_RECEIVE) return;

  int channel = mapMidiToLogicalChannel((int)midiChannel, pitch);

  // For persistent channels (11-14) use the counter (must match handleNoteOn indexing).
  if (channel >= 11 && channel <= 14) {
    if (pitch < 128) {
      midiHeldNote[channel][pitch] = false;
    }
    {
      int cnt = 0;
      for (int p = 0; p < 128; p++) {
        if (midiHeldNote[channel][p]) cnt++;
      }
      pressedKeyCount[channel] = (int16_t)cnt;
    }

    if (pressedKeyCount[channel] == 0 && persistentNoteOn[channel]) {
      if (channel == 13 || channel == 14) {
        stopSynthChannel(channel);
      } else if (envelopes[channel]) {
        envelopes[channel]->noteOff();
        persistentNoteOn[channel] = false;
        noteOnTriggered[channel] = false;
      } else {
        persistentNoteOn[channel] = false;
        noteOnTriggered[channel] = false;
      }
    }
    return;
  }

  // Existing logic for non-persistent channels goes here...
}
void handleStart() {
  logMidiRxSimple("Start");
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

  // Start after transportRcveDelayMs (0-127). 0 = no delay.
  pendingStartOnBar = false;
  beatStartTime = millis();

  unsigned long delayUs = (unsigned long)transportRcveDelayMs * 1000UL;
  transportStartDelayUntil = micros() + delayUs;
}


void armMasterTransportStartDelay() {
  transportStartDelayUntil = micros() + (unsigned long)transportSendDelayMs * 1000UL;
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
