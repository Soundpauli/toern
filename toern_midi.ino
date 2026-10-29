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
extern bool persistentNoteOn[];
extern bool noteOnTriggered[];

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
static const unsigned long NO_CLOCK_TIMEOUT_US = 2000000;
static toern_midi::ClockFollower midiFollower;
static unsigned long lastMidiClockUs = 0;
static uint8_t midiClockTicks = 0;
static uint16_t midiClockTickCounter = 0;
static bool midiStartOnClock = false;
static bool midiFollowerLockNotified = false;

bool getBPMStable() {
  return midiFollower.stable(micros());
}

// Function to get current MIDI clock tick count (for arrow blinking)
// Returns a counter that increments on every clock pulse (for blinking sync)
uint16_t getMidiClockTicks() {
  return midiClockTickCounter;
}
static uint8_t externalStepWithinPage = 0;  // Tracks external clock-derived step (1..maxX)
static unsigned long transportStartDelayUntil = 0;
static float midiClockSendBPM = 0.0f;
static double midiClockIntervalUs = 0;
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
static volatile uint8_t midiHeldVelocity[17][128] = { 0 };

int collectHeldMidiNotes(int ch, uint8_t *pitches, uint8_t *vels, int maxN) {
  int n = 0;
  if (ch < 0 || ch > 16 || !pitches || !vels || maxN <= 0) return 0;
  for (int p = 0; p < 128 && n < maxN; p++) {
    if (!midiHeldNote[ch][p]) continue;
    pitches[n] = (uint8_t)p;
    uint8_t v = midiHeldVelocity[ch][p];
    vels[n] = v > 0 ? v : 100;
    n++;
  }
  return n;
}

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
static volatile uint32_t pulseClockEndUs = 0;
static volatile uint32_t sidechainEndUs = 0;
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

// Sequencer, fills and MIDI clock already use three of the four PIT channels.
// Both output pulses share the remaining timer, with independent deadlines.
class PulseOutputGuard {
public:
  PulseOutputGuard() {
    __asm__ volatile("mrs %0, primask" : "=r"(primask_) :: "memory");
    __disable_irq();
  }
  ~PulseOutputGuard() { if (!primask_) __enable_irq(); }
private:
  uint32_t primask_;
};

static void pulseOutputsEndIsr();

// Called with interrupts masked, including when invoked by the shared ISR.
static bool schedulePulseOutputEnd() {
  if (!pulseClockPinActive && !sidechainPinActive) {
    pulseEndTimer.end();
    return true;
  }
  const uint32_t now = micros();
  int32_t next = INT32_MAX;
  if (pulseClockPinActive) next = (int32_t)(pulseClockEndUs - now);
  if (sidechainPinActive) {
    const int32_t remaining = (int32_t)(sidechainEndUs - now);
    if (remaining < next) next = remaining;
  }
  pulseEndTimer.priority(toern_audio::kTimerPriority);
  if (pulseEndTimer.begin(pulseOutputsEndIsr, next > 0 ? (uint32_t)next : 1u)) return true;
  // No timer available: never leave a trigger stuck active.
  pulseClockPinActive = sidechainPinActive = false;
  digitalWriteFast(PULSE_CLOCK_PIN, pulseIdleLevel());
  digitalWriteFast(SIDECHAIN_OUT_PIN, LOW);
  return false;
}

static void pulseOutputsEndIsr() {
  PulseOutputGuard guard;
  const uint32_t now = micros();
  if (pulseClockPinActive && (int32_t)(now - pulseClockEndUs) >= 0) {
    pulseClockPinActive = false;
    digitalWriteFast(PULSE_CLOCK_PIN, pulseIdleLevel());
  }
  if (sidechainPinActive && (int32_t)(now - sidechainEndUs) >= 0) {
    sidechainPinActive = false;
    digitalWriteFast(SIDECHAIN_OUT_PIN, LOW);
  }
  schedulePulseOutputEnd();
}

static void pulseClockForceIdle() {
  PulseOutputGuard guard;
  pulseClockPinActive = false;
  digitalWriteFast(PULSE_CLOCK_PIN, pulseIdleLevel());
  schedulePulseOutputEnd();
}

static void pulseClockEmit() {
  PulseOutputGuard guard;
  uint32_t us = (uint32_t)pulseClockWidthMs * 1000u;
  if (us < 1000u) us = 1000u;
  pulseClockEndUs = micros() + us;
  pulseClockPinActive = true;
  if (schedulePulseOutputEnd()) digitalWriteFast(PULSE_CLOCK_PIN, pulseActiveLevel());
}

static void pulseClockApplyIdle() {
  pinMode(PULSE_CLOCK_PIN, OUTPUT);
  digitalWriteFast(PULSE_CLOCK_PIN, pulseIdleLevel());
  pulseClockPinActive = false;
}

static void sidechainApplyIdle() {
  pinMode(SIDECHAIN_OUT_PIN, OUTPUT);
  digitalWriteFast(SIDECHAIN_OUT_PIN, LOW);
  sidechainPinActive = false;
}

static uint8_t sidechainVoice = 1;  // 0=OFF, else 1–8 / 11 / 13 / 14
static bool analogInEnabled = true;
static bool analogInPolarityPositive = true;
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

bool getAnalogInEnabled() { return analogInEnabled; }
bool getAnalogInPolarityPositive() { return analogInPolarityPositive; }

static bool analogClockInputActive() {
  return (digitalReadFast(ANALOG_CLOCK_IN_PIN) != 0) == analogInPolarityPositive;
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
  // Slot 53: bits 0-2 = PQ-I PPQN index, bits 3-7 = hold-off ms - 1.
  uint8_t packed = EEPROM.read(EEPROM_DATA_START + 53);
  uint8_t idx = packed & 0x07u;
  if (idx >= ANALOG_IN_PPQN_COUNT) idx = 2;
  analogInPpqnIndex = idx;
  analogInHoldMs = (uint8_t)((packed >> 3) + 1u);
  analogInHoldUs = (uint32_t)analogInHoldMs * 1000u;
  // Slot 59 extends the original rate/hold-off byte without changing its layout.
  uint8_t flags = EEPROM.read(EEPROM_DATA_START + 59);
  if (flags > 3u) flags = 0;  // Older firmware: enabled, positive edge.
  analogInEnabled = (flags & 1u) == 0;
  analogInPolarityPositive = (flags & 2u) == 0;
  analogClockSyncListen();
}

static void saveAnalogInSettings() {
  extern void saveSingleModeToEEPROM(int index, int8_t value);
  const uint8_t packed = (uint8_t)((analogInPpqnIndex & 0x07u) | ((uint8_t)(analogInHoldMs - 1u) << 3));
  saveSingleModeToEEPROM(53, (int8_t)packed);
}

void sidechainOnVoice(uint8_t ch) {
  if (!isNowPlaying || !sidechainVoice || ch != sidechainVoice) return;
  PulseOutputGuard guard;
  uint32_t us = (uint32_t)pulseClockWidthMs * 1000u;
  if (us < 1000u) us = 1000u;
  sidechainEndUs = micros() + us;
  sidechainPinActive = true;
  // J27 is a positive trigger; PPQN polarity controls J25 only.
  if (schedulePulseOutputEnd()) digitalWriteFast(SIDECHAIN_OUT_PIN, HIGH);
}

static uint8_t analogClockInPpqn() {
  return ANALOG_IN_PPQN_TABLE[analogInPpqnIndex];
}

static void saveAnalogInFlags() {
  extern void saveSingleModeToEEPROM(int index, int8_t value);
  saveSingleModeToEEPROM(59, (analogInEnabled ? 0 : 1) | (analogInPolarityPositive ? 0 : 2));
}

static void resetAnalogInTiming() {
  analogClockLastUs = analogClockPeriodUs = 0;
  analogClockFallUs = micros();
  analogStepPulse = analogFillSubs = analogSubstepsLeft = 0;
  extern int clockMode;
  extern IntervalTimer playTimer;
  if (analogExtClock && clockMode != 1) playTimer.end();
}

uint8_t getAnalogInRateSel() { return analogInEnabled ? analogInPpqnIndex + 1u : 0u; }
uint8_t getAnalogInRateSelCount() { return ANALOG_IN_PPQN_COUNT + 1u; }

void setAnalogInRateSel(uint8_t selection) {
  if (selection > ANALOG_IN_PPQN_COUNT) selection = ANALOG_IN_PPQN_COUNT;
  PulseOutputGuard guard;
  analogInEnabled = selection != 0;
  if (selection) analogInPpqnIndex = selection - 1u;
  resetAnalogInTiming();
  analogClockSyncListen();
  saveAnalogInSettings();
  saveAnalogInFlags();
}

void toggleAnalogInPolarity() {
  PulseOutputGuard guard;
  analogInPolarityPositive = !analogInPolarityPositive;
  resetAnalogInTiming();
  analogClockSyncListen();
  saveAnalogInFlags();
}

uint8_t getAnalogInPpqn() { return analogClockInPpqn(); }
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
  if (touchyEnabled) return;
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
    resetTouchFills();
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
  const bool level = analogClockInputActive();
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
  pinMode(ANALOG_CLOCK_IN_PIN, analogInPolarityPositive ? INPUT_PULLDOWN : INPUT_PULLUP);
  analogClockHigh = analogClockInputActive();
  extern int clockMode;
  // EXT is 0 (encoder-3 analog click) or -1 (encoder-2 INT/EXT). Only INT (1) is silent.
  if (!analogInEnabled || !analogExtClock || clockMode == 1) return;
  attachInterrupt(digitalPinToInterrupt(ANALOG_CLOCK_IN_PIN), analogClockIsr, CHANGE);
  NVIC_SET_PRIORITY(IRQ_GPIO6789, toern_audio::kTimerPriority);
}

// If Play is armed while CLK-IN is already active, the edge ISR already ran
// (and did nothing). Catch that pulse now so beat 1 is not delayed to the next one.
void analogClockCatchIfHigh() {
  extern bool pendingStartOnBar;
  if (!analogInEnabled || !analogExtClock || !pendingStartOnBar) return;
  noInterrupts();
  analogClockHigh = analogClockInputActive();
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
uint8_t getPulseClockPpqn() { return PULSE_PPQN_TABLE[pulseClockPpqnIndex]; }
uint8_t getPulseClockRateSelCount() { return PULSE_RATE_SEL_COUNT; }

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
  if (!isfinite(bpm)) return;
  bpm = constrain(bpm, 1.0f, (float)BPM_MAX);
  // IntervalTimer accepts fractional microseconds (quantized to PIT ticks).
  midiClockIntervalUs = 60000000.0 / ((double)bpm * 24.0);
  midiClockSendBPM = bpm;
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
    configureMidiClockSend(SMP.bpm, now);
  }
}

// MIDI library callback for realtime Clock messages
void handleMidiClock() {
  // Use the UART receive timestamp, not the delayed parser execution time.
  myClock(toern_midi::parsedByteUs);
}

static void serviceMidiTransportStart() {
  if (touchyEnabled) transportStartDelayUntil = 0;
  // Fire deferred transport start once the non-blocking delay has elapsed
  if (transportStartDelayUntil && (int32_t)(micros() - transportStartDelayUntil) >= 0) {
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
      double currentPlayNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
      playNoteInterval = currentPlayNoteInterval;
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
    resetTouchFills();
    fillSubTick = 0;
    fillStartSubTick = 0;
    fillTriggerStep = 0;
    fillTriggerRow = 0;
    fillActiveChannel = 0;
    fillActiveVelocity = 0;
    fillActiveRow = 0;
  }
}

void checkMidi() {
  static uint32_t seenOverruns = 0;
  if (seenOverruns != toern_midi::rxOverruns) {
    seenOverruns = toern_midi::rxOverruns;
    midiFollower.reset();
    midiFollowerLockNotified = false;
  }

  // UART timestamps preserve clock timing while callbacks execute in loop().
  // Bound ordinary message work; leave unread messages queued for next call.
  static const uint8_t MAX_MIDI_MESSAGES_PER_LOOP = 16;
  uint8_t processedMessages = 0;
  while (processedMessages < MAX_MIDI_MESSAGES_PER_LOOP && MIDI.read()) {
    uint8_t miditype = MIDI.getType();

    if (miditype == midi::Clock) continue;
    if (miditype == midi::NoteOn) continue;  // handled by callback

    if (miditype == midi::NoteOff) {
      uint8_t pitch = MIDI.getData1();
      uint8_t velocity = MIDI.getData2();
      logMidiRxNoteOff(MIDI.getChannel(), pitch, velocity);
      // Same logical channel as handleNoteOn (cable channel + voice mode), not raw 1..16 only.
      handleNoteOff(MIDI.getChannel(), pitch, velocity);
      continue;
    }

    // Stop reading at the budget: unread messages remain queued for next loop.
    processedMessages++;

    switch (miditype) {
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

  serviceMidiTransportStart();

  // If this device is sending MIDI Clock (master) or analog PPQN pulse is on in INT mode, keep timer in sync.
  extern int clockMode;
  if (MIDI_CLOCK_SEND || (pulseClockEnabled && clockMode == 1)) {
    if (fabsf(SMP.bpm - midiClockSendBPM) > 0.0001f && SMP.bpm > 0) {
      configureMidiClockSend(SMP.bpm, micros());
    }
  }
}

constexpr uint8_t clocksPerStep = 24 / 4;

void resetMidiClockState() { // MODIFIED to reset BPM averaging state for slave
  
  extern int clockMode;
  if (clockMode == 1) {
    unsigned long now = micros();
    // Preserve clock phase unless the actual fractional tempo changed.
    if (midiClockIntervalUs == 0 || fabsf(SMP.bpm - midiClockSendBPM) > 0.0001f) {
      configureMidiClockSend(SMP.bpm, now);
    }
    lastClockSent = now;
    // Ensure playTimer is configured if master
    if (SMP.bpm > 0.0f) {
        double currentPlayNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
        playTimer.begin(playNote, currentPlayNoteInterval);
    } else {
        playTimer.end();
    }
  } else { // SLAVE MODE - run internal timer, sync with external clock
    // Don't stop MIDI clock timer - it should always run in background
    // Timer will just not send clock messages when MIDI_CLOCK_SEND is false
    midiFollower.reset();
    midiFollowerLockNotified = false;
    midiStartOnClock = false;
    midiClockTickCounter = 0;

    // Orange EXT plays from PQ-I pulses only; red EXT runs the timer and syncs it to MIDI clock.
    if (analogExtClock) {
      analogSubstepsLeft = 0;
      playTimer.end();
      fillTimer.end();
    } else if (SMP.bpm > 0.0f) {
      double currentPlayNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
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

  if (!MIDI_CLOCK_SEND) return;

  unsigned long now = micros();
  // Force re-phase regardless of whether BPM changed.
  configureMidiClockSend(SMP.bpm, now);
  lastClockSent = now;

  if (SMP.bpm > 0.0f) {
    double currentPlayNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
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
  // Estimate timing from receive timestamps; keep audio on the PIT timers.
  
  extern int clockMode;
  if (clockMode == 1) { // Follow only in EXT, including NOTE-only output mode.
    return;
  }
  if (analogExtClock) return;  // EXT analog: ignore MIDI F8 for tempo

  if (lastMidiClockUs && uint32_t(now_captured - lastMidiClockUs) > NO_CLOCK_TIMEOUT_US)
    midiFollowerLockNotified = false;
  lastMidiClockUs = now_captured;

  // Analog pulse out follows incoming MIDI clock in EXT mode
  pulseClockMidiTick();

  const bool tempoUpdated = midiFollower.tick(now_captured);
  if (tempoUpdated) SMP.bpm = midiFollower.bpm();

  // MIDI Start defines pulse zero. Start playback on the first subsequent F8,
  // preserving its receive timestamp and the configured receive-side offset.
  const bool firstClockAfterStart = midiStartOnClock;
  if (midiStartOnClock) {
    midiStartOnClock = false;
    transportStartDelayUntil = now_captured + effectiveTransportRecvDelayMs() * 1000UL;
    if (!transportStartDelayUntil) transportStartDelayUntil = 1;
  }

  // --- Handle Start command sync and step tracking FIRST (always needed) ---
  // Keep pulseCount for transport sync, but don't trigger steps
  pulseCount = (pulseCount + 1) % (24 * 4); // 96 pulses = one full 4/4 bar
  // Track external steps from MIDI clock (1..maxX) for debugging
  if (!firstClockAfterStart) midiClockTicks++;
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
          transportStartDelayUntil = now_captured + delayUs;
          if (!transportStartDelayUntil) transportStartDelayUntil = 1;
        }
      }
    }
  }

  if (midiFollower.ready() && (tempoUpdated || midiClockTicks == 0)) {
    const double baseStepUs = midiFollower.stepUs();
    double timerStepUs = baseStepUs;
    extern volatile uint32_t sequencerRecordStepStartedUs;
    const uint32_t playedAt = sequencerRecordStepStartedUs;
    if (isNowPlaying && playedAt) {
      const uint32_t boundaryUs = now_captured -
          (uint32_t)lround(baseStepUs * midiClockTicks / 6.0);
      timerStepUs = midiFollower.correctedStepUs(
          boundaryUs, playedAt, effectiveTransportRecvDelayMs() * 1000UL);
    }
    playNoteInterval = baseStepUs;
    playTimer.update(timerStepUs);
    fillTimer.update(timerStepUs / 4.0);
  }

  // Lock is based on interval consistency, never equality of rounded BPM.
  // Retain the existing auto-exit once per acquisition.
  if (midiFollower.stable(now_captured) && !midiFollowerLockNotified) {
    midiFollowerLockNotified = true;
    extern Mode *currentMode;
    extern Mode bpmMode;
    if (currentMode == &bpmMode) {
      extern Mode draw, singleMode;
      extern bool menuEnteredFromSingleMode;
      extern int currentMenuPage;
      currentMenuPage = 3;
      switchMode(menuEnteredFromSingleMode ? &singleMode : &draw);
    }
  }

  // Playback is driven by playTimer; external MIDI clock updates its interval.
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

void cancelMidiTransportStart() {
  midiStartOnClock = false;
  transportStartDelayUntil = 0;
  pendingStartOnBar = false;
}

void handleStop() {
  if (!MIDI_TRANSPORT_RECEIVE) return;
  cancelMidiTransportStart();
  // External Stop must also work immediately after Start, without debounce.
  if (isNowPlaying) pause(true);
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
uint8_t midiPitchToGridRow(int channel, uint8_t pitch) {
  return toern_midi_pitch::gridRow(channel, pitch, (int)octave[0]);
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
      samplePitch = toern_midi_pitch::sampleNote(SampleRate[ch] * 12, pitch);
    } else {
      samplePitch = (SampleRate[ch] * 12) + (int)row - (ch + 1);
    }
    // OCTV whole semis; DTNE fine tune applied inside triggerSamplerVoice.
    if (ch >= 1 && ch <= 8) samplePitch += (int)lroundf(channelOctave[ch]);
    triggerSamplerVoice(ch, samplePitch, velocity, false);
  } else if (ch == 11) {
    int noteValue = preservePitch
        ? toern_midi_pitch::polyNote(pitch, transpose, (int)octave[0])
        : 12 * (int)octave[0] + transpose + (int)row - (11 + 1);
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
  if (!MIDI_NOTE_RECEIVE || pitch >= 128) return;

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
    midiHeldVelocity[ch][pitch] = velocity;
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

  extern bool synthMidiArpEnabled(int ch);
  extern void midiArpOnHeldNote(int ch);
  if (synthMidiArpEnabled(ch)) {
    midiArpOnHeldNote(ch);
    activeNotes[pitch] = true;
    return;
  }

  if (isNowPlaying) {
      if (GLOB.singleMode) {
        extern bool placeLiveGridNote(uint8_t channel, uint8_t row, uint8_t velocity,
                                      uint8_t midiPitch, bool protectOtherVoices,
                                      bool protectOccupied);
        extern bool MIDI_WRITE_OVERWRITE;
        placeLiveGridNote((uint8_t)ch, livenote, velocity, storedPitch, false,
                          !MIDI_WRITE_OVERWRITE);
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
  // While a synth key remains down, render adjacent continuation cells as
  // G/L. They are written after audio playback, so live MIDI remains the only
  // audible trigger during recording; later sequencer playback holds/slides.
  if (!GLOB.singleMode || currentBeat < 1 || currentBeat >= maxlen) return;
  if (previousBeat < 1 || previousBeat >= maxlen) return;

  const uint8_t synthChannels[] = { 11, 13, 14 };
  extern bool synthMidiArpEnabled(int ch);
  for (uint8_t channel : synthChannels) {
    if (isChildVoiceDisabled(channel)) continue;
    if (synthMidiArpEnabled(channel)) continue;
    for (int p = 0; p < 128; p++) {
      uint8_t pitch = (uint8_t)p;
      if (!midiHeldNote[channel][pitch]) continue;
      uint8_t row = midiPitchToGridRow(channel, pitch);
      Note &current = note[currentBeat][row];
      if (midiRecordCellMatches(current, channel, pitch)) continue;
      Note &previous = note[previousBeat][row];
      if (!midiRecordCellMatches(previous, channel, pitch)) continue;
      extern bool MIDI_WRITE_OVERWRITE;
      if (!MIDI_WRITE_OVERWRITE && current.channel != 0) continue;
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
  extern bool MIDI_VOICE_SELECT;
  if (!MIDI_NOTE_RECEIVE || pitch >= 128) return;

  // YPOS maps Note On to the selected voice. If that selection changes while a
  // key is down, Note Off must still find CH11/13/14 (and samples) by the held
  // bitmap — otherwise the voice rings until Pause.
  const bool ypos = !MIDI_VOICE_SELECT;
  const int mapped = mapMidiToLogicalChannel((int)midiChannel, pitch);
  const int first = ypos ? 1 : mapped;
  const int last = ypos ? 16 : mapped;

  for (int channel = first; channel <= last; ++channel) {
    if (channel < 1 || channel > 16) continue;
    if (ypos && !midiHeldNote[channel][pitch]) continue;

    // Sample ADSR release follows MIDI Note Off too. Keep the held-note counter
    // so releasing one key cannot cut a different key held on this mono voice.
    if (channel >= 1 && channel <= 8) {
      midiHeldNote[channel][pitch] = false;
      midiHeldVelocity[channel][pitch] = 0;
      int held = 0;
      for (int p = 0; p < 128; ++p) if (midiHeldNote[channel][p]) ++held;
      pressedKeyCount[channel] = held;
      if (!held) {
        persistentNoteOn[channel] = false;
        // Grid hits are one-shots (sustain until the sample ends). Don't start
        // release on key-up; default RELEASE is 0 and would chop the file.
        if (envelopes[channel] && !noteOnTriggered[channel]) envelopes[channel]->noteOff();
      }
      continue;
    }

    // Persistent synths (11-14) must match handleNoteOn indexing.
    if (channel >= 11 && channel <= 14) {
      midiHeldNote[channel][pitch] = false;
      midiHeldVelocity[channel][pitch] = 0;
      int cnt = 0;
      for (int p = 0; p < 128; p++) {
        if (midiHeldNote[channel][p]) cnt++;
      }
      pressedKeyCount[channel] = (int16_t)cnt;

      if (channel == 11) {
        // Ch11 uses the polyphonic voice system (Senvelope1/2/filter), not envelopes[11].
        // Arp owns the sounding voice while keys are down; don't kill it on each
        // key-off. Non-arp: release the exact voice identity created by Note On.
        extern bool synthMidiArpEnabled(int ch);
        if (!synthMidiArpEnabled(11)) {
          extern int16_t midiHeldCh11SoundNote[128];
          int note = (int)midiHeldCh11SoundNote[pitch];
          stopSound(note, 0);
        }
        if (pressedKeyCount[11] == 0) {
          persistentNoteOn[11] = false;
          noteOnTriggered[11] = false;
          extern void midiArpResetVoice(int ch);
          midiArpResetVoice(11);
        }
      } else if (pressedKeyCount[channel] == 0) {
        if (channel == 13 || channel == 14) {
          // Arp reset only stops the voice if a phrase was running. MIDI hold
          // (arp off) still needs a real envelope noteOff.
          extern void midiArpResetVoice(int ch);
          midiArpResetVoice(channel);
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
    }
  }
}
void handleStart() {
  if (touchyEnabled) return;
  logMidiRxSimple("Start");
  // only act if we’re supposed to follow external transport
  if (!MIDI_TRANSPORT_RECEIVE) return;

  cancelMidiTransportStart();
  extern int clockMode;
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

  if (clockMode != 1 && analogExtClock) {
    pendingStartOnBar = true;
    analogClockCatchIfHigh();
    return;
  }

  pendingStartOnBar = false;
  if (clockMode != 1) {
    isNowPlaying = false;
    transportStartDelayUntil = 0;
    midiStartOnClock = true;
    return;
  }

  // INT transport receive retains the configured start delay.

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
