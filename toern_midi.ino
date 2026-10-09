// External variables
extern float detune[13]; // Global detune array for channels 1-12
extern float channelOctave[9]; // Global octave array for channels 1-8
extern unsigned int maxX;
extern unsigned int pulseCount;
extern bool isNowPlaying;
extern bool MIDI_TRANSPORT_SEND;
extern bool SMP_PATTERN_MODE;
extern unsigned int beat;
extern int8_t transportSendDelayMs;
extern int8_t transportRcveDelayMs;

// Signed SYNC delays (MENU>MIDI SNC1/SNC2): positive = delay this path; negative = delay the other path by |v|.
unsigned long effectiveTransportSendDelayMs() {
  long ms = 0;
  if (transportSendDelayMs > 0) ms += (long)transportSendDelayMs;
  if (transportRcveDelayMs < 0) ms += -(long)transportRcveDelayMs;
  if (ms < 0) ms = 0;
  return (unsigned long)ms;
}

unsigned long effectiveTransportRecvDelayMs() {
  long ms = 0;
  if (transportRcveDelayMs > 0) ms += (long)transportRcveDelayMs;
  if (transportSendDelayMs < 0) ms += -(long)transportSendDelayMs;
  if (ms < 0) ms = 0;
  return (unsigned long)ms;
}
struct GlobalVars;
extern struct GlobalVars GLOB;
void triggerExternalOneBlink();
extern IntervalTimer midiClockTimer;
void handleNoteOff(uint8_t midiChannel, uint8_t pitch, uint8_t velocity);
void stopSynthChannel(int ch);
bool isChildVoiceDisabled(int channel);

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
static const uint32_t CLOCKS_PER_BPM_WINDOW = 24 * 4;    // 4/4: 24 clocks/beat * 4 beats/bar = 96 clocks (calculate BPM every 1 bar for faster sync)
static const unsigned long NO_CLOCK_TIMEOUT_US = 2000000; // 2 seconds: if no clock received, reset Kalman state
static unsigned long lastClockReceivedTime = 0;          // Timestamp of last received clock (MIDI or analog)
static unsigned long lastMidiClockUs = 0;                // MIDI F8 only — analog in yields while this is fresh
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
static volatile bool midiHeldNote[17][128] = { false };

// Analog clock / sidechain on rev H dedicated 2-pin JSTs (not the J11 expansion row).
// CLK-OUT(5V) J25 ← pin 37 via 74AHCT1G125; CLK-IN(3–5V) J26 → pin 38 via 74LVC1G17;
// SIDEC-5V J27 ← pin 28 via 74AHCT1G125. Pin 31 on J11 is unused GPIO.
#define PULSE_CLOCK_PIN 37
#define ANALOG_CLOCK_IN_PIN 38
#define SIDECHAIN_OUT_PIN 28
#define LEGACY_PULSE_CLOCK_PIN 31
#define PULSE_WIDTH_MS_MIN 1
#define PULSE_WIDTH_MS_MAX 50
#define PULSE_WIDTH_MS_DEFAULT 12
#define ANALOG_CLOCK_REARM_US 500u
#define ANALOG_IN_HOLD_MS_MIN 1
#define ANALOG_IN_HOLD_MS_MAX 32
// Encoder 3 turn: OFF + these PPQN values
static const uint8_t PULSE_PPQN_TABLE[] = { 1, 2, 4, 8, 12, 16, 24, 32 };
static const uint8_t PULSE_PPQN_COUNT = sizeof(PULSE_PPQN_TABLE) / sizeof(PULSE_PPQN_TABLE[0]);
static const uint8_t PULSE_RATE_SEL_COUNT = PULSE_PPQN_COUNT + 1;  // 0=OFF, 1..N = table
// Below 4 PPQN a pulse spans several 16th steps; see analogDirectPulse().
static const uint8_t ANALOG_IN_PPQN_TABLE[] = { 1, 2, 4, 8, 12, 16, 24, 32 };
static const uint8_t ANALOG_IN_PPQN_COUNT = sizeof(ANALOG_IN_PPQN_TABLE) / sizeof(ANALOG_IN_PPQN_TABLE[0]);

static bool pulseClockEnabled = false;
static bool pulseClockPolarityPositive = true;  // +: active high; -: active low
static bool pulseClockStopWithPlay = false;     // STOP: gate with transport; CONT: always
static uint8_t pulseClockWidthMs = PULSE_WIDTH_MS_DEFAULT;
static uint8_t pulseClockPpqnIndex = 6;         // default 24 PPQN (table index)
static uint16_t pulseClockPhase = 0;
static IntervalTimer pulseEndTimer;
static volatile bool pulseClockPinActive = false;
static IntervalTimer sidechainEndTimer;
static volatile bool sidechainPinActive = false;
static volatile uint32_t analogClockLastUs = 0;   // last counted rise

bool analogClockPulseFresh() {
  if (!analogClockLastUs) return false;
  return ((micros() - analogClockLastUs) / 1000u) < 80u;
}
static volatile uint32_t analogClockFallUs = 0;
static volatile uint32_t analogClockPeriodUs = 0; // rise to rise (1/2 PPQN spacing)
static volatile bool analogClockHigh = false;
static uint8_t analogInPpqnIndex = 2;  // default 4 PPQN (Volca / 16ths)
static volatile uint8_t analogSubstepsLeft = 0;  // 1/2 PPQN: steps still due before the next pulse
static uint8_t analogInHoldMs = ANALOG_IN_HOLD_MS_MIN;
static volatile uint32_t analogInHoldUs = ANALOG_IN_HOLD_MS_MIN * 1000u;
static volatile uint8_t analogStepPulse = 0;  // pulse index within the current step
static volatile uint8_t analogFillSubs = 0;   // fill sub-ticks fired in the current step

static inline uint8_t pulseIdleLevel() {
  return pulseClockPolarityPositive ? LOW : HIGH;
}
static inline uint8_t pulseActiveLevel() {
  return pulseClockPolarityPositive ? HIGH : LOW;
}

static void pulseClockEndIsr() {
  digitalWriteFast(PULSE_CLOCK_PIN, pulseIdleLevel());
  pulseEndTimer.end();
  pulseClockPinActive = false;
}

static void pulseClockForceIdle() {
  if (pulseClockPinActive) {
    pulseEndTimer.end();
    pulseClockPinActive = false;
  }
  digitalWriteFast(PULSE_CLOCK_PIN, pulseIdleLevel());
}

static void pulseClockEmit() {
  digitalWriteFast(PULSE_CLOCK_PIN, pulseActiveLevel());
  pulseClockPinActive = true;
  uint32_t us = (uint32_t)pulseClockWidthMs * 1000u;
  if (us < 1000u) us = 1000u;
  pulseEndTimer.priority(toern_audio::kTimerPriority);
  pulseEndTimer.begin(pulseClockEndIsr, us);
}

static void pulseClockApplyIdle() {
  pinMode(PULSE_CLOCK_PIN, OUTPUT);
  digitalWriteFast(PULSE_CLOCK_PIN, pulseIdleLevel());
  pulseClockPinActive = false;
}

static void sidechainApplyIdle() {
  pinMode(SIDECHAIN_OUT_PIN, OUTPUT);
  digitalWriteFast(SIDECHAIN_OUT_PIN, pulseIdleLevel());
  sidechainPinActive = false;
}

static void sidechainEndIsr() {
  digitalWriteFast(SIDECHAIN_OUT_PIN, pulseIdleLevel());
  sidechainEndTimer.end();
  sidechainPinActive = false;
}

static uint8_t sidechainVoice = 1;  // 0=OFF, else 1–8 / 11 / 13 / 14
static bool analogExtClock = false;  // EXT source: false = MIDI F8, true = analog CLK-IN (J26)

static bool sidechainVoiceOk(uint8_t ch) {
  return ch == 0 || (ch >= 1 && ch <= 8) || ch == 11 || ch == 13 || ch == 14;
}

uint8_t getSidechainVoice() { return sidechainVoice; }

void setSidechainVoice(uint8_t ch) {
  if (!sidechainVoiceOk(ch)) ch = 1;
  sidechainVoice = ch;
  extern void saveSingleModeToEEPROM(int index, int8_t value);
  saveSingleModeToEEPROM(51, (int8_t)sidechainVoice);
}

void loadSidechainFromEEPROM() {
  uint8_t ch = EEPROM.read(EEPROM_DATA_START + 51);
  if (!sidechainVoiceOk(ch)) ch = 1;
  sidechainVoice = ch;
}

bool getAnalogExtClock() { return analogExtClock; }

static void analogClockSyncListen();

void setAnalogExtClock(bool analog) {
  analogExtClock = analog;
  analogClockSyncListen();
  extern void saveSingleModeToEEPROM(int index, int8_t value);
  saveSingleModeToEEPROM(52, analogExtClock ? 1 : 0);
}

void toggleAnalogExtClock() {
  setAnalogExtClock(!analogExtClock);
}

void loadAnalogExtClockFromEEPROM() {
  uint8_t v = EEPROM.read(EEPROM_DATA_START + 52);
  analogExtClock = (v == 1);
  // Slot 53: bits 0-2 = CIN PPQN index, bits 3-7 = hold-off ms - 1.
  uint8_t packed = EEPROM.read(EEPROM_DATA_START + 53);
  uint8_t idx = packed & 0x07u;
  if (idx >= ANALOG_IN_PPQN_COUNT) idx = 2;
  analogInPpqnIndex = idx;
  analogInHoldMs = (uint8_t)((packed >> 3) + 1u);
  analogInHoldUs = (uint32_t)analogInHoldMs * 1000u;
  analogClockSyncListen();
}

static void saveAnalogInSettings() {
  extern void saveSingleModeToEEPROM(int index, int8_t value);
  const uint8_t packed = (uint8_t)((analogInPpqnIndex & 0x07u) | ((uint8_t)(analogInHoldMs - 1u) << 3));
  saveSingleModeToEEPROM(53, (int8_t)packed);
}

void sidechainOnVoice(uint8_t ch) {
  if (!sidechainVoice || ch != sidechainVoice) return;
  digitalWriteFast(SIDECHAIN_OUT_PIN, pulseActiveLevel());
  sidechainPinActive = true;
  uint32_t us = (uint32_t)pulseClockWidthMs * 1000u;
  if (us < 1000u) us = 1000u;
  sidechainEndTimer.priority(toern_audio::kTimerPriority);
  sidechainEndTimer.begin(sidechainEndIsr, us);
}

static uint8_t analogClockInPpqn() {
  return ANALOG_IN_PPQN_TABLE[analogInPpqnIndex];
}

uint8_t getAnalogInPpqn() { return analogClockInPpqn(); }
uint8_t getAnalogInPpqnIndex() { return analogInPpqnIndex; }
uint8_t getAnalogInPpqnCount() { return ANALOG_IN_PPQN_COUNT; }

void setAnalogInPpqnIndex(uint8_t idx) {
  if (idx >= ANALOG_IN_PPQN_COUNT) idx = ANALOG_IN_PPQN_COUNT - 1;
  if (idx == analogInPpqnIndex) return;
  noInterrupts();
  analogInPpqnIndex = idx;
  analogStepPulse = 0;
  interrupts();
  saveAnalogInSettings();
}

uint8_t getAnalogInHoldMs() { return analogInHoldMs; }

void setAnalogInHoldMs(uint8_t ms) {
  if (ms < ANALOG_IN_HOLD_MS_MIN) ms = ANALOG_IN_HOLD_MS_MIN;
  if (ms > ANALOG_IN_HOLD_MS_MAX) ms = ANALOG_IN_HOLD_MS_MAX;
  if (ms == analogInHoldMs) return;
  analogInHoldMs = ms;
  analogInHoldUs = (uint32_t)ms * 1000u;
  saveAnalogInSettings();
}

void pulseClockOnTransportStart();

static void analogPlayWholeStep() {
  extern void playNote();
  extern void playFillNote();
  playNote();
  for (uint8_t k = 0; k < 4u; k++) playFillNote();
}

static void analogSubstepIsr() {
  extern IntervalTimer playTimer;
  if (!analogSubstepsLeft || !isNowPlaying) {
    analogSubstepsLeft = 0;
    playTimer.end();
    return;
  }
  analogSubstepsLeft--;
  analogPlayWholeStep();
  if (!analogSubstepsLeft) playTimer.end();
}

// Orange EXT: pulses are the only clock. Steps and fill sub-ticks fire from
// here; playTimer/fillTimer stay stopped (see resetMidiClockState).
static void analogDirectPulse() {
  extern void playNote();
  extern void playFillNote();
  extern bool pendingStartOnBar;
  extern int patternMode;
  extern unsigned long playStartTime;
  extern IntervalTimer playTimer;
  extern IntervalTimer fillTimer;
  if (pendingStartOnBar) {
    pendingStartOnBar = false;
    if (SMP_PATTERN_MODE && patternMode == 3) {
      GLOB.page = GLOB.edit;
      beat = (GLOB.page - 1) * maxX + 1;
    } else if (SMP_PATTERN_MODE) {
      beat = (GLOB.edit - 1) * maxX + 1;
      GLOB.page = GLOB.edit;
    } else {
      beat = 1;
      GLOB.page = 1;
    }
    fillHasTriggered = false;
    fillRunning = false;
    fillSubTick = 0;
    fillStartSubTick = 0;
    fillTriggerStep = 0;
    fillTriggerRow = 0;
    fillActiveChannel = 0;
    fillActiveVelocity = 0;
    fillActiveRow = 0;
    fillActiveMidiPitch = NOTE_MIDI_PITCH_NONE;
    playStartTime = millis();
    analogStepPulse = 0;
    analogSubstepsLeft = 0;
    playTimer.end();
    fillTimer.end();
    isNowPlaying = true;
    pulseClockOnTransportStart();
  }
  if (!isNowPlaying) return;

  const uint8_t ppqn = analogClockInPpqn();
  if (ppqn < 4u) {
    // One pulse spans 4/ppqn steps: play one now, space the rest over the last
    // pulse interval. Steps the next pulse finds still due fire at once so the
    // bar stays on the pulses.
    extern IntervalTimer playTimer;
    playTimer.end();
    while (analogSubstepsLeft) {
      analogSubstepsLeft--;
      analogPlayWholeStep();
    }
    analogPlayWholeStep();
    const uint8_t stepsPerPulse = (uint8_t)(4u / ppqn);
    const uint32_t period = analogClockPeriodUs;
    if (period && period <= NO_CLOCK_TIMEOUT_US) {
      analogSubstepsLeft = (uint8_t)(stepsPerPulse - 1u);
      playTimer.begin(analogSubstepIsr, period / stepsPerPulse);
    }
    return;
  }

  const uint8_t pulsesPerStep = ppqn / 4u;
  const uint8_t i = analogStepPulse;
  const bool lastPulse = (uint8_t)(i + 1u) >= pulsesPerStep;
  if (i == 0) {
    analogFillSubs = 0;
    playNote();
  }
  // Four fill sub-ticks per step, placed on the nearest pulse; below 16 PPQN
  // the step's last pulse fires the ones that have no pulse of their own.
  const uint8_t due = lastPulse ? 4u : (uint8_t)(4u * i / pulsesPerStep + 1u);
  while (analogFillSubs < due) {
    analogFillSubs++;
    playFillNote();
  }
  analogStepPulse = lastPulse ? 0 : (uint8_t)(i + 1u);
}

FASTRUN static inline void analogClockCountRise(uint32_t now) {
  if (analogClockLastUs && (uint32_t)(now - analogClockLastUs) < analogInHoldUs) return;
  if (analogClockLastUs) analogClockPeriodUs = now - analogClockLastUs;
  analogClockLastUs = now;
  midiClockTickCounter++;
  if (analogClockPeriodUs && analogClockPeriodUs <= NO_CLOCK_TIMEOUT_US) {
    const float bpm = 60000000.0f / ((float)analogClockPeriodUs * (float)analogClockInPpqn());
    int shown = (int)lroundf(bpm);
    if (shown < BPM_MIN) shown = BPM_MIN;
    if (shown > BPM_MAX) shown = BPM_MAX;
    SMP.bpm = (float)shown;
    extern double playNoteInterval;
    playNoteInterval = (double)analogClockPeriodUs * (double)analogClockInPpqn() / 4.0;
  }
  analogDirectPulse();
}

FASTRUN static void analogClockIsr() {
  const uint32_t now = micros();
  const bool level = digitalReadFast(ANALOG_CLOCK_IN_PIN) != 0;
  if (level) {
    if (analogClockHigh) {
      // Missed the falling edge: this CHANGE is a new pulse if hold-off has passed.
      if (analogClockLastUs && (uint32_t)(now - analogClockLastUs) >= analogInHoldUs)
        analogClockCountRise(now);
      return;
    }
    analogClockHigh = true;
    if ((uint32_t)(now - analogClockFallUs) < ANALOG_CLOCK_REARM_US) return;
    analogClockCountRise(now);
  } else if (analogClockHigh) {
    analogClockHigh = false;
    analogClockFallUs = now;
  } else if ((uint32_t)(now - analogClockFallUs) >= ANALOG_CLOCK_REARM_US) {
    analogClockCountRise(now);
    analogClockFallUs = now;
  }
}

static void analogClockSyncListen() {
  detachInterrupt(digitalPinToInterrupt(ANALOG_CLOCK_IN_PIN));
  pinMode(ANALOG_CLOCK_IN_PIN, INPUT_PULLDOWN);
  analogClockHigh = digitalReadFast(ANALOG_CLOCK_IN_PIN) != 0;
  extern int clockMode;
  // EXT is 0 (encoder-3 analog click) or -1 (encoder-2 INT/EXT). Only INT (1) is silent.
  if (!analogExtClock || clockMode == 1) return;
  attachInterrupt(digitalPinToInterrupt(ANALOG_CLOCK_IN_PIN), analogClockIsr, CHANGE);
  NVIC_SET_PRIORITY(IRQ_GPIO6789, toern_audio::kTimerPriority);
}

// If Play is armed while CLK-IN is already high, the rise ISR already ran
// (and did nothing). Catch that pulse now so beat 1 is not delayed to the next one.
void analogClockCatchIfHigh() {
  extern bool pendingStartOnBar;
  if (!analogExtClock || !pendingStartOnBar) return;
  noInterrupts();
  analogClockHigh = digitalReadFast(ANALOG_CLOCK_IN_PIN) != 0;
  // DirectPulse, not CountRise: this pulse's rise already set lastUs, so hold-off
  // would drop the catch and delay beat 1 until the next clock.
  if (analogClockHigh) analogDirectPulse();
  interrupts();
}

void initAnalogClockHardware() {
  pinMode(LEGACY_PULSE_CLOCK_PIN, INPUT);
  pulseClockApplyIdle();
  sidechainApplyIdle();
  analogClockSyncListen();
}

void pulseClockMidiTick() {
  if (!pulseClockEnabled) return;
  extern bool isNowPlaying;
  if (pulseClockStopWithPlay && !isNowPlaying) {
    pulseClockForceIdle();
    return;
  }
  uint8_t ppqn = PULSE_PPQN_TABLE[pulseClockPpqnIndex];
  pulseClockPhase = (uint16_t)(pulseClockPhase + ppqn);
  while (pulseClockPhase >= 24) {
    pulseClockPhase = (uint16_t)(pulseClockPhase - 24);
    pulseClockEmit();
  }
}

// Downbeat pulse with sequencer / MIDI Start. Phase stays 0 so the next
// midiClockTick is pulse #2 (24 PPQN) or the start of the divider (lower rates).
// Without this, STOP waits a full 24 MIDI clocks at 1 PPQN, and even 24 PPQN
// waits one IntervalTimer period because midiClockTick is not called on start.
void pulseClockOnTransportStart() {
  if (!pulseClockEnabled) return;
  noInterrupts();
  pulseClockForceIdle();
  pulseClockPhase = 0;
  pulseClockEmit();
  interrupts();
}

bool getPulseClockEnabled() { return pulseClockEnabled; }
bool getPulseClockPolarityPositive() { return pulseClockPolarityPositive; }
bool getPulseClockStopWithPlay() { return pulseClockStopWithPlay; }
uint8_t getPulseClockWidthMs() { return pulseClockWidthMs; }
uint8_t getPulseClockWidthMsMin() { return PULSE_WIDTH_MS_MIN; }
uint8_t getPulseClockWidthMsMax() { return PULSE_WIDTH_MS_MAX; }
uint8_t getPulseClockPpqnIndex() { return pulseClockPpqnIndex; }
uint8_t getPulseClockPpqn() { return PULSE_PPQN_TABLE[pulseClockPpqnIndex]; }
uint8_t getPulseClockPpqnCount() { return PULSE_PPQN_COUNT; }
uint8_t getPulseClockRateSelCount() { return PULSE_RATE_SEL_COUNT; }
uint8_t getPulseClockPpqnAt(uint8_t idx) {
  if (idx >= PULSE_PPQN_COUNT) idx = PULSE_PPQN_COUNT - 1;
  return PULSE_PPQN_TABLE[idx];
}

// Encoder 4 position: 0=OFF, 1..8 = PPQN table entries
uint8_t getPulseClockRateSel() {
  if (!pulseClockEnabled) return 0;
  return (uint8_t)(pulseClockPpqnIndex + 1);
}

static void pulseClockSaveToEEPROM() {
  // Slot 10: bit0=on, bit1=polarity-, bits2-4=ppqn index, bit5=STOP (0=CONT for old EEPROM)
  uint8_t packed = (pulseClockEnabled ? 1 : 0)
                 | (pulseClockPolarityPositive ? 0 : 2)
                 | ((pulseClockPpqnIndex & 0x07) << 2)
                 | (pulseClockStopWithPlay ? 0x20 : 0);
  extern void saveSingleModeToEEPROM(int index, int8_t value);
  saveSingleModeToEEPROM(10, (int8_t)packed);
}

static void pulseClockSaveWidthToEEPROM() {
  extern void saveSingleModeToEEPROM(int index, int8_t value);
  saveSingleModeToEEPROM(30, (int8_t)pulseClockWidthMs);
}

void setPulseClockPolarityPositive(bool positive) {
  pulseClockPolarityPositive = positive;
  if (!pulseClockPinActive) {
    digitalWriteFast(PULSE_CLOCK_PIN, pulseIdleLevel());
  }
  if (!sidechainPinActive) {
    digitalWriteFast(SIDECHAIN_OUT_PIN, pulseIdleLevel());
  }
  pulseClockSaveToEEPROM();
}

void togglePulseClockPolarity() {
  setPulseClockPolarityPositive(!pulseClockPolarityPositive);
}

void setPulseClockStopWithPlay(bool stopWithPlay) {
  pulseClockStopWithPlay = stopWithPlay;
  if (pulseClockStopWithPlay) {
    extern bool isNowPlaying;
    if (!isNowPlaying) pulseClockForceIdle();
  }
  pulseClockSaveToEEPROM();
}

void setPulseClockWidthMs(uint8_t ms) {
  if (ms < PULSE_WIDTH_MS_MIN) ms = PULSE_WIDTH_MS_MIN;
  if (ms > PULSE_WIDTH_MS_MAX) ms = PULSE_WIDTH_MS_MAX;
  pulseClockWidthMs = ms;
  pulseClockSaveWidthToEEPROM();
}

void setPulseClockPpqnIndex(uint8_t idx) {
  if (idx >= PULSE_PPQN_COUNT) idx = PULSE_PPQN_COUNT - 1;
  pulseClockPpqnIndex = idx;
  pulseClockPhase = 0;
  pulseClockSaveToEEPROM();
}

void setPulseClockRateSel(uint8_t sel) {
  if (sel >= PULSE_RATE_SEL_COUNT) sel = PULSE_RATE_SEL_COUNT - 1;
  if (sel == 0) {
    setPulseClockEnabled(false);
    return;
  }
  uint8_t idx = (uint8_t)(sel - 1);
  if (idx >= PULSE_PPQN_COUNT) idx = PULSE_PPQN_COUNT - 1;
  if (pulseClockPpqnIndex != idx) {
    pulseClockPpqnIndex = idx;
    pulseClockPhase = 0;
  }
  if (!pulseClockEnabled) {
    setPulseClockEnabled(true);
  } else {
    pulseClockSaveToEEPROM();
  }
}

void setPulseClockEnabled(bool enabled) {
  if (enabled == pulseClockEnabled) return;

  if (enabled) {
    pulseClockEnabled = true;
    pulseClockPhase = 0;
    pulseClockApplyIdle();
    // Keep master 24-PPQN timer running so pulses track BPM even if MIDI clock send is off
    extern void updateMidiClockOutput();
    updateMidiClockOutput();
  } else {
    pulseClockForceIdle();
    pulseClockEnabled = false;
    pulseClockApplyIdle();
  }
  pulseClockSaveToEEPROM();
}

void loadPulseClockFromEEPROM() {
  uint8_t packed = EEPROM.read(EEPROM_DATA_START + 10);
  uint8_t idx = (packed >> 2) & 0x07;
  if (idx >= PULSE_PPQN_COUNT) idx = 6;  // default 24
  pulseClockPpqnIndex = idx;
  pulseClockPolarityPositive = ((packed & 2) == 0);
  pulseClockStopWithPlay = ((packed & 0x20) != 0);  // unset bit = CONT (legacy)
  uint8_t w = EEPROM.read(EEPROM_DATA_START + 30);
  if (w < PULSE_WIDTH_MS_MIN || w > PULSE_WIDTH_MS_MAX) w = PULSE_WIDTH_MS_DEFAULT;
  pulseClockWidthMs = w;
  bool on = (packed & 1) != 0;
  pulseClockEnabled = false;
  initAnalogClockHardware();
  if (on) setPulseClockEnabled(true);
}

// Master clock callback. UART, I2S DMA and audio may preempt this shared PIT ISR.
// Serial8.write can wait if its ring is full; attached buffers cover note bursts.
void midiClockTick() {
  // Analog pulse out from master 24-PPQN timer (INT only — EXT follows myClock)
  extern int clockMode;
  if (pulseClockEnabled && clockMode == 1) {
    pulseClockMidiTick();
  }

  // Always send clock if in master mode - timer never stops, even during pause/stop
  if (MIDI_CLOCK_SEND) {
    // Write MIDI clock byte (0xF8) directly to Serial8 hardware
    // Real TX storage is attached in setup(). If it still fills, UART + I2S
    // DMA + audio update may preempt PIT while write() waits for space.
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
  // All IntervalTimers share PIT: priority 0 here also elevated playNote/fill
  // above I2S DMA and audio. Audio deadlines take precedence over clock jitter.
  midiClockTimer.priority(toern_audio::kTimerPriority);
  midiClockTimer.begin(midiClockTick, midiClockIntervalUs);
  
}

// Public function to update MIDI clock with exact BPM value (for use from updateBPM)
void updateMidiClockOutput() {
  // Keep timer alive for MIDI clock send and/or analog pulse out (PPQN) in INT mode
  extern int clockMode;
  if (MIDI_CLOCK_SEND || (pulseClockEnabled && clockMode == 1)) {
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
    extern int patternMode;
    if (SMP_PATTERN_MODE && patternMode == 3) {
      // NEXT starts from the page visible when the delayed transport actually
      // begins, not the page visible when Play was first armed.
      GLOB.page = GLOB.edit;
      beat = (GLOB.page - 1) * maxX + 1;
      pendingPage = 0;
    }
    isNowPlaying = true;
    playStartTime = millis();
    pulseClockOnTransportStart();
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
    fillTriggerStep = 0;
    fillTriggerRow = 0;
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

  // If this device is sending MIDI Clock (master) or analog PPQN pulse is on in INT mode, keep timer in sync.
  extern int clockMode;
  if (MIDI_CLOCK_SEND || (pulseClockEnabled && clockMode == 1)) {
    int currentRoundedBPM = (int)round(SMP.bpm);
    int lastRoundedBPM = (int)round(midiClockSendBPM);
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
    
    // Orange EXT plays from CIN pulses only; red EXT runs the timer and syncs it to MIDI clock.
    if (analogExtClock) {
      analogSubstepsLeft = 0;
      playTimer.end();
      fillTimer.end();
    } else if (SMP.bpm > 0.0f) {
      unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)SMP.bpm * 4.0));
      playTimer.begin(playNote, currentPlayNoteInterval);
    } else {
      playTimer.end();
    }
  }
  analogClockSyncListen();
}

// Transport-start variant: ALWAYS re-phases the MIDI clock timer and sends
// Start + first Clock back-to-back so the slave's beat-1 is phase-aligned.
// Without re-phasing, the first Clock after Start has a random offset
// (0 to one full clock period) that scales with BPM, making any fixed-ms
// SNC compensation inconsistent across tempos.
void resetMidiClockForTransportStart() {
  lastClockTime = 0;

  if (!MIDI_CLOCK_SEND) return;

  unsigned long now = micros();
  int roundedBPM = (int)round(SMP.bpm);
  if (roundedBPM < 1) roundedBPM = 1;
  if (roundedBPM > BPM_MAX) roundedBPM = BPM_MAX;

  // Force re-phase regardless of whether BPM changed.
  configureMidiClockSend((float)roundedBPM, now);
  lastClockSent = now;

  if (SMP.bpm > 0.0f) {
    unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)SMP.bpm * 4.0));
    playTimer.begin(playNote, currentPlayNoteInterval);
  } else {
    playTimer.end();
  }

  // Send Start + first Clock back-to-back via direct serial writes.
  // Per MIDI spec the slave begins playback on the first Clock after Start.
  if (MIDI_TRANSPORT_SEND) {
    Serial8.write(0xFA);  // MIDI Start
    Serial8.write(0xF8);  // First MIDI Clock — slave's beat-1 fires here
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
  if (analogExtClock) return;  // EXT analog: ignore MIDI F8 for tempo

  lastMidiClockUs = now_captured;

  // Analog pulse out follows incoming MIDI clock in EXT mode
  pulseClockMidiTick();

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

          // Defer first step by effective receive-side delay (SNC2 + cross from negative SNC1).
          unsigned long delayUs = effectiveTransportRecvDelayMs() * 1000UL;
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
    } else {
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
  if (isChildVoiceDisabled(channel)) return;
  if (velocity < 0) velocity = 0;
  if (velocity > 127) velocity = 127;
  int note = constrain(pitch, 0, 127);

  // Send the Note On event using the MIDI library.
  MIDI.sendNoteOn(note, velocity, channel);

  // Keep a bounded list for release at the next sequencer step (or pause).
  // At most one cell per grid row can trigger during a step.
  static_assert(maxY <= 16, "MIDI output release queue assumes at most 16 rows");
  extern uint8_t sequencerMidiOutCount;
  extern uint8_t sequencerMidiOutPitch[16];
  extern uint8_t sequencerMidiOutChannel[16];
  if (sequencerMidiOutCount < 16) {
    sequencerMidiOutPitch[sequencerMidiOutCount] = (uint8_t)note;
    sequencerMidiOutChannel[sequencerMidiOutCount] = (uint8_t)channel;
    sequencerMidiOutCount++;
  }
}

void MidiReleaseSequencerNotes() {
  extern uint8_t sequencerMidiOutCount;
  extern uint8_t sequencerMidiOutPitch[16];
  extern uint8_t sequencerMidiOutChannel[16];
  uint8_t count = sequencerMidiOutCount;
  sequencerMidiOutCount = 0;
  for (uint8_t i = 0; i < count && i < 16; i++) {
    MIDI.sendNoteOff(sequencerMidiOutPitch[i], 0,
                     sequencerMidiOutChannel[i]);
  }
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

// Fold an incoming pitch into the editable single-mode rows without losing its
// pitch class. Repeated wrapping handles the complete MIDI 0..127 range.
static uint8_t midiPitchToGridRow(int channel, uint8_t pitch) {
  int anchorRow;
  if (channel == 11) {
    anchorRow = 12;            // ch11 calibrated manual anchor
  } else if (channel == 13 || channel == 14) {
    anchorRow = channel - 11;  // ch13 row2 / ch14 row3 = manual C3 base
  } else {
    anchorRow = channel + 1;   // sample voice's normal DRAW row
  }
  int row = anchorRow + (int)pitch - 60;
  while (row < 1) row += 12;
  while (row > 15) row -= 12;
  return (uint8_t)row;
}

static bool preserveIncomingMidiPitch() {
  extern bool MIDI_NOTE_CLAMP;
  extern int voiceSelect;
  return !MIDI_NOTE_CLAMP && voiceSelect != 2;
}

static void playIncomingMidiNote(int ch, uint8_t pitch, uint8_t row,
                                 uint8_t velocity) {
  extern int voiceSelect;
  const bool preservePitch = preserveIncomingMidiPitch();

  if (ch < 9) {
    int samplePitch;
    if (voiceSelect == 2) {
      // KEYS remains a pitch-class-to-sample-voice trigger mode.
      samplePitch = SampleRate[ch] * 12;
    } else if (preservePitch) {
      samplePitch = (SampleRate[ch] * 12) + (int)pitch - 72;
    } else {
      samplePitch = (SampleRate[ch] * 12) + (int)row - (ch + 1);
    }
    // OCTV whole semis; DTNE fine tune applied inside triggerSamplerVoice.
    if (ch >= 1 && ch <= 8) samplePitch += (int)lroundf(channelOctave[ch]);
    triggerSamplerVoice(ch, samplePitch, velocity, false);
  } else if (ch == 11) {
    int noteValue = 12 * (int)octave[0] + transpose;
    noteValue += preservePitch ? ((int)pitch - 60) : ((int)row - 1);
    extern int16_t midiHeldCh11SoundNote[128];
    midiHeldCh11SoundNote[pitch] = (int16_t)noteValue;
    playSound(noteValue, 0, velocity);
  } else if (ch == 13 || ch == 14) {
    if (preservePitch) {
      extern void playSynthMidi(int ch, uint8_t midiPitch, int vel, bool persistant);
      playSynthMidi(ch, pitch, velocity, true);
    } else {
      playSynth(ch, row, velocity, true);
    }
  }
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
  if (isChildVoiceDisabled(ch)) return;

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

  const uint8_t livenote = midiPitchToGridRow(ch, pitch);
  const uint8_t storedPitch = preserveIncomingMidiPitch()
                                ? pitch : NOTE_MIDI_PITCH_NONE;

  if (isNowPlaying) {
      if (GLOB.singleMode) {
        extern bool placeLiveGridNote(uint8_t channel, uint8_t row, uint8_t velocity,
                                      uint8_t midiPitch, bool protectOtherVoices);
        placeLiveGridNote((uint8_t)ch, livenote, velocity, storedPitch, false);
        playIncomingMidiNote(ch, pitch, livenote, velocity);
      } else {
        playIncomingMidiNote(ch, pitch, livenote, velocity);
      }
    // Always play the note immediately
    activeNotes[pitch] = true;
  } else {
    // Live mode: play note and show light
    //light(mapXtoPageOffset(GLOB.x), livenote, CRGB(255, 255, 255));
    //FastLED.show();
    playIncomingMidiNote(ch, pitch, livenote, velocity);
  }
}

static bool midiRecordCellMatches(const Note &cell, uint8_t channel,
                                  uint8_t pitch) {
  if (cell.channel != channel) return false;
  return cell.midiPitch <= 127 ? cell.midiPitch == pitch
                               : cell.midiPitch == NOTE_MIDI_PITCH_NONE;
}

void onBeatTick(unsigned int currentBeat, unsigned int previousBeat) {
  PendingNote pn;
  extern bool dequeuePendingNote(PendingNote &out);
  while (dequeuePendingNote(pn)) {
    if (isChildVoiceDisabled((int)pn.channel)) continue;
    unsigned int targetBeat = pn.targetBeat;
    if (targetBeat < 1 || targetBeat >= maxlen) continue;
    note[targetBeat][pn.livenote].channel = pn.channel;
    note[targetBeat][pn.livenote].velocity = pn.velocity;
    note[targetBeat][pn.livenote].probability = 100;
    note[targetBeat][pn.livenote].condition = 1;
    note[targetBeat][pn.livenote].midiPitch = pn.pitch;
  }

  // While a synth key remains down, render adjacent continuation cells as
  // G/L. They are written after audio playback, so live MIDI remains the only
  // audible trigger during recording; later sequencer playback holds/slides.
  if (!GLOB.singleMode || currentBeat < 1 || currentBeat >= maxlen) return;
  if (previousBeat < 1 || previousBeat >= maxlen) return;

  const uint8_t synthChannels[] = { 11, 13, 14 };
  for (uint8_t channel : synthChannels) {
    if (isChildVoiceDisabled(channel)) continue;
    for (int p = 0; p < 128; p++) {
      uint8_t pitch = (uint8_t)p;
      if (!midiHeldNote[channel][pitch]) continue;
      uint8_t row = midiPitchToGridRow(channel, pitch);
      Note &current = note[currentBeat][row];
      if (midiRecordCellMatches(current, channel, pitch)) continue;
      Note &previous = note[previousBeat][row];
      if (!midiRecordCellMatches(previous, channel, pitch)) continue;
      if (current.channel != 0 && current.channel != channel) continue;
      current.channel = channel;
      current.velocity = previous.velocity;
      current.probability = 100;
      current.condition = NOTE_CONDITION_GLIDE;
      current.midiPitch = previous.midiPitch;
    }
  }
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

    if (channel == 11) {
      // Ch11 uses the polyphonic voice system (Senvelope1/2/filter), not envelopes[11].
      // Release the exact voice identity created by Note On. This remains
      // correct even if CLMP is toggled while a key is held.
      extern int16_t midiHeldCh11SoundNote[128];
      int note = (int)midiHeldCh11SoundNote[pitch];
      stopSound(note, 0);
      if (pressedKeyCount[11] == 0) {
        persistentNoteOn[11] = false;
        noteOnTriggered[11] = false;
      }
    } else if (pressedKeyCount[channel] == 0 && persistentNoteOn[channel]) {
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

  if (!MIDI_CLOCK_SEND && analogExtClock) {
    pendingStartOnBar = true;
    analogClockCatchIfHigh();
    return;
  }

  // Start after effective receive-side delay.
  pendingStartOnBar = false;

  unsigned long delayUs = effectiveTransportRecvDelayMs() * 1000UL;
  transportStartDelayUntil = micros() + delayUs;
}


void armMasterTransportStartDelay() {
  transportStartDelayUntil = micros() + effectiveTransportSendDelayMs() * 1000UL;
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
