#define VERSION "v1.4"
extern "C" char *sbrk(int incr);
#define FASTLED_ALLOW_INTERRUPTS 0
#define SERIAL8_RX_BUFFER_SIZE 2048  // Larger MIDI input buffer for high-frequency clock messages (default is 64)
#define SERIAL8_TX_BUFFER_SIZE 128   // Larger transmit buffer for safety
#define TargetFPS 30

//#define AUDIO_BLOCK_SAMPLES 128
//#define AUDIO_SAMPLE_RATE_EXACT 44100

//STILL FREE PINS: 24, 25, 31, 22, 5, 15, 2, 4, 14, 9, 32, 33
static const int FAST_DROP_BLOCKS = 5;  // ≈25ms @ 44100Hz with 128-sample blocks (reduced from 200ms to minimize trimming)
static int fastDropRemaining = 0;
volatile bool stepIsDue = false;


//#include <Wire.h>x-

#include <cstring>  // For memcmp
#include "src/effect_freeverb_dmabuf.h"
#define USE_WS2812SERIAL   // leds
#include <WS2812Serial.h>  // leds

#include <FastLED.h>  // leds

#include <i2cEncoderLibV2.h>
#include <MIDI.h>
#include <Audio.h>
#include <EEPROM.h>
#include <FastTouch.h>
#include <TeensyPolyphony.h>

#include <Mapf.h>
#include "notes.h"
#include "colors.h"
#include "audioinit.h"
#include "font_3x5.h"
#include "icons.h"

#define LED_MODULES 2  // Max number of 16x16 matrices that can be chained (hardware limit)
#define MATRIX_WIDTH 16  // Width of each individual matrix
#define maxY 16
#define INT_SD 10

// maxX is now a runtime variable, calculated from ledModules menu setting
extern unsigned int maxX;
extern void handleMidiClock();
#define NUM_LEDS (256 * LED_MODULES)  // 256 LEDs per matrix
#define DATA_PIN 17  // PIN FOR LEDS
#define INT_PIN 27   // PIN FOR ENOCDER INTERRUPS

#define SWITCH_1 16  // Pin for TPP223 1
#define SWITCH_2 3  // // Pin for TPP223 3 //3==lowerright, lowerleft== 15!
#define SWITCH_3 41   //Pin for TPP223 2

#define VOL_MIN 1
#define VOL_MAX 10
#define LINEOUT_MIN 13   // SGTL5000 line-out valid range (datasheet: 13..31)
#define LINEOUT_MAX 31
#define BPM_MIN 40
#define BPM_MAX 300

#define GAIN_1 0.4  //0.1;
#define GAIN_2 0.8   //0.33 (increased to 0.8 for 64% total gain)
#define GAIN_3 0.4   //0.25
#define GAIN_4 0.8  //0.2; (increased to 0.8 for 64% total gain)


#define NUM_ENCODERS 4
#define defaultVelocity 100
#define FOLDER_MAX 10

#define maxPages 16
#define maxFiles 9
#define NUM_CHANNELS 16
#define maxFilters 15

#define maxfilterResolution 32.0
#define numPulsesForAverage 8  // Number of pulses to average over
#define pulsesPerBar (24 * 4)  // 24 pulses per quarter note, 4 quarter notes per bar

#define EEPROM_MENU_ADDR 42

struct MidiSettings : public midi ::DefaultSettings {
  static const long BaudRate = 31250;
  static const unsigned SysExMaxSize = 16;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial8, MIDI, MidiSettings);
unsigned long beatStartTime = 0;  // Timestamp when the current beat started

unsigned long ganularStartTime = 0;  // Timestamp when the current beat started

#define EXTERNAL_ONE_ENCODER_INDEX 2  // Encoder(2) -> third hardware knob
static bool externalOneBlinkActive = false;
static unsigned long externalOneBlinkUntil = 0;

// Input monitoring toggle state: 0=off, 1=on (y==1 only), 2=all (always on)
// 0 = OFF (dark white helper LEDs)
// 1 = ON (dark green helper LEDs, only when y==1)
// 2 = ALL (dark yellow helper LEDs, always on regardless of y position)
int inputMonitoringState = 0;

void triggerExternalOneBlink();
void updateExternalOneBlink();
void updatePreviewVolume();

#define CLOCK_BUFFER_SIZE 24
//elapsedMillis recFlushTimer;
elapsedMillis recTime;

bool showChannelNr = true;
int cursorType = 0;  // 0=NORM, 1=BIG (CHNR uses showChannelNr=true, cursorType=0)
bool lastPinsConnected = false;
unsigned long lastChangeTime = 0;
const unsigned long debounceDelay = 500;  // 100ms debounce

#define EEPROM_MAGIC_ADDR 42
#define EEPROM_DATA_START 43        // 43..48 will be your six mode‐bytes
const uint8_t EEPROM_MAGIC = 0x5A;  // anything nonzero


// Add at file scope:

// Extern declarations for menu system
extern int currentMenuPage;

// Function declarations for per-page mute system
bool getMuteState(int channel);
void setMuteState(int channel, bool muted);
void savePageMutesToGlobal();
void loadGlobalMutesToPage();
void initPageMutes();
void drawIndicator(char size, char colorCode, int encoderNum, bool highlight = false);
void updatePongBall();
void drawPongBall();
void resetPongGame();
void triggerGridNote(unsigned int globalX, unsigned int y);
void drawCtrlVolumeOverlay(int volume);
void drawInputGainOverlay(int gain, int maxGain);
void drawChannelNrOverlay(int channelNum, int channelIdx);
void drawSampleLoadOverlay();
static int16_t lastDefaultFastFilterValue[NUM_CHANNELS] = {0};  // Changed from int to int16_t (32 bytes saved)

enum ValueDisplayMode {
  DISPLAY_NUMERIC,
  DISPLAY_ENUM
};

const char* const instTypeNames[] PROGMEM = { "BASS", "KEYS", "CHPT", "PAD", "WOW", "ORG", "FLT", "LEAD", "ARP", "BRSS" };
#define INST_ENUM_COUNT (sizeof(instTypeNames) / sizeof(instTypeNames[0]))

// Add at file scope, after instTypeNames[]
const char* const sndTypeNames[] PROGMEM = { "SAMP" };
const char* const waveformNames[] PROGMEM = { "SIN", "SQR", "SAW", "TRI" };

struct SliderMeta {
  uint8_t maxValue;          // physical encoder range, e.g. 16
  uint8_t displayMode;       // enum or numeric
  const char** enumNames;    // for DISPLAY_ENUM
  uint8_t displayRange;      // NEW: number of distinct display values (e.g. 5, 3)
};


static const SliderMeta sliderMeta[4][4] = {
  // Page 0
  {
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32}
  },
  // Page 1
  {
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32}
  },
  // Page 2
  {
    {16, DISPLAY_NUMERIC, nullptr, 5},                  // Shown as 0–4
    {9, DISPLAY_ENUM, instTypeNames, 10 }, // Mapped from 0–9
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32}
  },
  // Page 3
  {
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32}
  }
};


struct PendingNoteEvent {
  uint8_t channel;
  uint8_t pitch;
  uint8_t velocity;
  unsigned long triggerTime;  // when to trigger
};

std::vector<PendingNoteEvent> pendingSampleNotes;

struct CachedSample {
  uint8_t folder;
  uint16_t sampleID;
  uint32_t lengthBytes;
  uint8_t rate;
  bool valid;
  int plen;
};

CachedSample previewCache;

// Flag to disable threshold from external code
bool disableThresholdFlag = false;

// Number of samples in each delay line
// Allocate the delay lines for left and right channels

bool MIDI_CLOCK_SEND = true;

bool MIDI_TRANSPORT_RECEIVE = true;
bool MIDI_TRANSPORT_SEND = false;
bool MIDI_VOICE_SELECT = false;
bool SMP_PATTERN_MODE = false;
bool SMP_FLOW_MODE = false;  // FLOW mode: follows timer position when playing
unsigned int lastFlowPage = 0;  // Track the last page set by FLOW mode
bool recMenuFirstEnter = true;  // Track first entry into REC menu
unsigned int SMP_FAST_REC = false;
unsigned int SMP_REC_CHANNEL_CLEAR = true;
bool SMP_LOAD_SETTINGS = true;  // Whether to load SMP settings when loading tracks
uint8_t lineOutLevelSetting = 30;

bool pendingStartOnBar = false;  // "I hit Play, now wait for bar-1"



bool drawNoSD_hasRun = false;

bool notePending = false;
uint8_t pendingPitch = 0;
uint8_t pendingVelocity = 0;
unsigned long pendingTime = 0;

struct PendingNote {
  uint8_t pitch;
  uint8_t velocity;
  uint8_t channel;
  uint8_t livenote;
};

// a small FIFO for upcoming grid-writes
static std::vector<PendingNote> pendingNotes;


// ----- Intro Animation Timing (in ms) -----
// Phase 1: 5 seconds (rainbow logo)
// Phase 2: 2 seconds (explosion)
const unsigned long phase1Duration = 3000;
const unsigned long phase2Duration = 1500;
const unsigned long totalAnimationTime = phase1Duration + phase2Duration;
// We'll treat the center of the grid as (7.5, 7.5) for the explosion.
const float logoCenterX = 6;
const float logoCenterY = 7;
// ----- Particle Structure for Explosion Phase -----
struct Particle {
  float initX;
  float initY;
  float dirX;
  float dirY;
  CRGB color;
};
bool filterfreshsetted = true;
bool particlesGenerated = false;
DMAMEM Particle particles[256];  // up to 256 possible "on" pixels
int particleCount = 0;

bool activeNotes[128] = { false };  // Track active MIDI notes (0-127)

float rateFactor = 44117.0 / 44100.0;


unsigned int infoIndex = 0;

uint16_t lastFile[FOLDER_MAX + 1] = { 0 };  // 16-bit to safely store file ids (no overflow)
bool freshPaint, tmpMute = false;
bool preventPaintUnpaint = false;  // Flag to prevent paint/unpaint after certain operations


bool firstcheck = false;
bool nofile = false;
char *currentParam = "DLAY";
char *currentFilter = "TYPE";
char *currentSynth = "BASS";
unsigned int fxType = 0;

unsigned int selectedFX = 0;


unsigned long filterDrawEndTime = 0;
bool filterDrawActive = false;


unsigned long patternChangeTime = 0;
bool patternChangeActive = false;



// Replaced String with hash for memory efficiency (~100-200 bytes saved)
uint32_t oldPosHash = 0;
// String buttonString, oldButtonString = "0000"; // REMOVED
uint8_t oldButtons[NUM_ENCODERS] = { 0, 0, 0, 0 };  // Changed from int to uint8_t (4 bytes saved per element)

unsigned long playStartTime = 0;  // To track when play(true) was last called

bool previewIsPlaying = false;

const int maxPeaks = 512;  // Adjust based on your needs
DMAMEM float peakValues[maxPeaks];
int peakIndex = 0;


const int maxRecPeaks = 512;  // Adjust based on your needs
DMAMEM float peakRecValues[maxRecPeaks];
int peakRecIndex = 0;



uint8_t ledBrightness = 83;
bool drawBaseColorMode = true;  // true = channel colors, false = black
bool pong = false;
const long ram = 12582912;  // 9* 1058400; //12seconds on 44.1 / 16Bit before: 12582912;  //12MB ram for sounds // 16MB total
const unsigned int MAX_STEPS = MATRIX_WIDTH * maxPages;  // Fixed total steps (16 pages * 16 cols = 256)
const unsigned int maxlen = MAX_STEPS + 1;               // Note array width (1-based)
const unsigned int SONG_LEN = MAX_STEPS;                 // Song length equals total steps

static bool lastBothTouched = false;
bool touchState[4] = { false };      // Current touch state (HIGH/LOW)
bool lastTouchState[4] = { false };  // Previous touch state
const int touchThreshold = 45;

const unsigned int totalPulsesToWait = pulsesPerBar * 2;

const unsigned long CHECK_INTERVAL = 50;  // Interval to check buttons in ms
unsigned long lastCheckTime = 0;          // Get the current time


int recMode = 1;
unsigned int fastRecMode = 0;
unsigned int previewVol = 20;  // Default 20 (0-50 range, 0.00-0.50)
int recChannelClear = 1;  // 0=OFF (add triggers), 1=ON (clear then add), 2=FIX (no manipulation), 3=ON1 (count-in then record on beat 1)
int transportMode = 1;
int patternMode = -1;
int flowMode = -1;  // FLOW setting: -1 = OFF, 1 = ON
int clockMode = 1;
int voiceSelect = 1;
int simpleNotesView = 1;  // Simple notes view: 1=EASY, 2=FULL
int loopLength = 0;  // Loop length: 0=OFF, 1-8=force pattern length
int ledModules = 1;  // Number of LED modules: 1 or 2
unsigned int maxX = MATRIX_WIDTH * 1;  // Runtime variable: total display width (16 or 32)
unsigned int micGain = 0;  // Microphone gain: 0-64, default 10
unsigned int lineInLevel = 8;  // Line input level: 0-15, default 8
int ctrlMode = 0;  // 0=page, 1=volume control for encoder 1
static int ctrlLastChannel = -1;
static int ctrlLastVolume = -1;
static bool ctrlVolumeOverlayActive = false;
static unsigned long ctrlVolumeOverlayUntil = 0;
static int ctrlVolumeOverlayValue = 0;
static bool inputGainOverlayActive = false;
static unsigned long inputGainOverlayUntil = 0;
static int inputGainOverlayValue = 0;
static int inputGainOverlayMax = 63;

static bool channelNrOverlayActive = false;
static unsigned long channelNrOverlayUntil = 0;
static int channelNrOverlayChannel = 0;
static const int DEFAULT_CHANNEL_VOLUME = 10;

bool isRecording = false;
File frec;

#define MAXREC_SECONDS 12
#define BUFFER_SAMPLES (22100 * MAXREC_SECONDS)
#define BUFFER_BYTES (BUFFER_SAMPLES * sizeof(int16_t))

EXTMEM int16_t recBuffer[BUFFER_SAMPLES];
volatile size_t recWriteIndex = 0;


// which input on the audio shield will be used?
//const int myInput = AUDIO_INPUT_LINEIN;
unsigned int recInput = AUDIO_INPUT_MIC;

/*timers*/
unsigned int lastButtonPressTime = 0;
bool resetTimerActive = false;

// runtime
//  variables for program logic
float pulse = 1;
int dir = 1;
unsigned int MIDI_CH = 1;
float playNoteInterval = 150000.0;
unsigned int RefreshTime = 1000 / TargetFPS;
float marqueePos = maxX;
bool shifted = false;
bool movingForward = true;  // Variable to track the direction of movement
unsigned int lastUpdate = 0;

unsigned int totalInterval = 0;


bool hasNotes[maxPages + 1];
unsigned int startTime[maxY] = { 0 };    // Variable to store the start time
bool noteOnTriggered[maxY] = { false };  // Flag to indicate if noteOn has been triggered
bool persistentNoteOn[maxY] = { false };
int pressedKeyCount[maxY] = { 0 };

bool waitForFourBars = false;
unsigned int pulseCount = 0;
bool sampleIsLoaded = false;
bool unpaintMode, paintMode = false;


// Global sequencer position (1..maxlen)
unsigned int beat = 1;
// Beat index that was last used for audio playback / sequencing.
// UI components should use this to stay visually in sync with what was just played.
unsigned int beatForUI = 1;
// Loop counter for condition feature (increments when beat resets to 1)
uint16_t loopCount = 0;
unsigned int samplePackID, fileID = 1;
EXTMEM unsigned int lastPreviewedSample[FOLDER_MAX] = {};
IntervalTimer playTimer;
IntervalTimer midiClockTimer;
//IntervalTimer midiTimer;
unsigned int lastPage = 1;
int editpage = 1;



struct Note {
  uint8_t channel;      // 0 = no note; otherwise, MIDI note value (0-127)
  uint8_t velocity;     // MIDI velocity (0-127)
  uint8_t probability;  // Probability 0-100 (in 25% steps: 0, 25, 50, 75, 100)
  uint8_t condition;   // Condition: 1=1/1, 2=1/2, 4=1/4, 8=1/8, 16=1/16, 17=2/1, 18=4/1, 19=8/1, 20=16/1 (default 1)
} __attribute__((packed));


EXTMEM Note note[maxlen + 1][maxY + 1] = {};
EXTMEM Note tmp[maxlen + 1][maxY + 1] = {};
EXTMEM Note original[maxlen + 1][maxY + 1] = {};

EXTMEM unsigned int sample_len[maxFiles];
bool sampleLengthSet = false;

bool isNowPlaying = false;  // global

int PrevSampleRate = 1;
EXTMEM int SampleRate[maxFiles] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
EXTMEM unsigned char sampled[maxFiles][ram / (maxFiles)];
static const uint32_t MAX_SAMPLES = sizeof(sampled[0]) / sizeof(sampled[0][0]);




elapsedMillis msecs;
elapsedMillis mRecsecs;
elapsedMillis diagStatsTimer;


DMAMEM CRGB leds[NUM_LEDS];

enum ButtonState {
  IDLE,
  LONG_PRESSED,
  RELEASED
};

const int ALL_CHANNELS[] = {1,2,3,4,5,6,7,8,11,13,14};
const int NUM_ALL_CHANNELS = sizeof(ALL_CHANNELS)/sizeof(ALL_CHANNELS[0]);


struct Mode {
  String name;
  unsigned int minValues[4];
  unsigned int maxValues[4];
  unsigned int pos[4];
  uint32_t knobcolor[4];
};

Mode draw = { "DRAW", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxlen - 1 }, { 1, maxY, maxfilterResolution, 1 }, { 0x110011, 0x000000, 0x00FF00, 0x110011 } };
Mode singleMode = { "SINGLE", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxlen - 1 }, { 1, 2, maxfilterResolution, 1 }, { 0x000000, 0x000000, 0x00FF00, 0x000000 } };
Mode volume_bpm = { "VOLUME_BPM", { 11, 11, 0, BPM_MIN }, { 30, 30, 1, BPM_MAX }, { 11, 11, 0, 100 }, { 0x000000, 0x000000, 0xFF4400, 0x00FFFF } };  // BPM only - volume controls removed, encoder[2] for INT/EXT, encoders 0-1 black
//filtermode has 4 entries
Mode filterMode = { "FILTERMODE", { 0, 0, 0, 0 }, { maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution }, { 1, 1, 1, 1 }, { 0x00FFFF, 0xFF00FF, 0xFFFF00, 0x00FF00 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 7, 0, 7 }, { 9, 9, maxfilterResolution, 9 }, { 8, 8, maxfilterResolution, 8 }, { 0xFFFF00, 0xFFFF00, 0x000000, 0xFFFFFF } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { maxY, 5, maxY, maxY }, { maxY, 5, 10, 10 }, { 0xFF4400, 0x00FF88, 0x0044FF, 0x888888 } };

Mode set_Wav = { "SET_WAV", { 1, 1, 1, 1 }, { 9999, FOLDER_MAX, 9999, 999 }, { 0, 0, 0, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode recordMode = { "RECORD_MODE", { 0, 1, 1, 1 }, { 100, FOLDER_MAX, 9999, 999 }, { 0, 0, 0, 1 }, { 0xFF0000, 0x00FF00, 0x0000FF, 0x000000 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 0 }, { 1, 1, 99, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode loadSaveTrack = { "LOADSAVE_TRACK", { 1, 1, 0, 1 }, { 1, 1, 1, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode menu = { "MENU", { 1, 1, 1, 0 }, { 1, 1, 64, 16 }, { 1, 1, 10, 1 }, { 0x000000, 0x000000, 0x000000, 0x00FF00 } };
Mode newFileMode = { "NEW_FILE", { 0, 1, 0, 0 }, { 5, 16, 0, 0 }, { 0, 8, 0, 0 }, { 0x00FFFF, 0xFF00FF, 0x000000, 0x000000 } };
Mode subpatternMode = { "SUBPATTERN", { 1, 0, 0, 1 }, { 1, 7, maxfilterResolution, 1 }, { 1, 1, maxfilterResolution, 1 }, { 0xFF00FF, 0x00FFFF, 0x000000, 0xFF00FF } };
Mode songMode = { "SONGMODE", { 1, 1, 1, 1 }, { 1, 16, 1, 64 }, { 1, 1, 1, 1 }, { 0x000000, 0xFF00FF, 0x000000, 0xFFFF00 } };
// Declare currentMode as a global variable
Mode *currentMode;
Mode *oldMode;
Mode *muteModeReturn = nullptr;
bool muteModeActive = false;
bool muteModeReturnSingleState = false;
int muteModeLastChannel = -1;
int8_t muteModeArrowDirection = 0;
unsigned long muteModeArrowUntil = 0;
uint8_t muteModeEncoderValue = 0;

static void refreshSamplerChannel(uint8_t channel);
static void reverseChannelInMemory(uint8_t channel);
static void applyChannelDirection(uint8_t channel, int8_t targetDir);

// Song arrangement data: 64 positions, each holds a pattern number (1-16, or 0 for empty)
uint8_t songArrangement[64] = {0};
bool songModeActive = false;  // When true, playback follows song arrangement
int currentSongPosition = 0;  // Current position in song arrangement (0-63)


struct Sample {
  unsigned int oldID : 10;   // values 0-1023, enough for 0-999
  unsigned int fileID : 10;  // values 0-1023, enough for 0-999
} __attribute__((packed));


// Declare the device struct
struct Device {
  // File/Pattern specific variables
  float bpm;              // bpm
  unsigned int file;      // current selected save/load id
  unsigned int pack;      // current selected samplepack id
  Sample wav[maxFiles];   // current selected sample
  float param_settings[maxY][maxFilters];
  float filter_settings[maxY][maxFilters];
  float synth_settings[maxY][8];
  unsigned int mute[maxY];
  unsigned int channelVol[maxY];
  // Mute system for PMOD
  bool globalMutes[maxY];           // Global mute states (when PMOD is off)
  bool pageMutes[maxPages][maxY];   // Page-specific mute states (when PMOD is on)
  // Samplepack 0 tracking - which voices have individually loaded samples
  bool sp0Active[maxFiles];         // Track which voices are using samplepack 0
  // Song arrangement - which pattern plays at each position
  uint8_t songArrangement[64];      // Song arrangement: 64 positions, each holds pattern 1-16 (or 0 for empty)
};

// Global variables struct
struct GlobalVars {
  unsigned int singleMode;  // single Sample Mod
  unsigned int currentChannel;
  unsigned int vol;       // volume
  unsigned int velocity;  // velocity
  unsigned int page;      // current page
  unsigned int edit;      // edit mode or plaing mode?
  unsigned int folder;    // current selected folder id
  bool activeCopy;        // is copy/paste active?
  unsigned int copyChannel; // channel that was copied (for cross-channel paste)
  unsigned int x;         // cursor X
  unsigned int y;         // cursor Y
  unsigned int seek;      // skipped into sample
  unsigned int seekEnd;
  unsigned int smplen;  // overall selected samplelength
  unsigned int shiftX;  // note Shift
  unsigned int shiftY;  // note Shift
  unsigned int shiftY1; // note Shift for encoder 1 (current channel and page only)
  unsigned int subpattern; // current subpattern (0-15)
};

float octave[2];

// Global detune array for channels 1-12 (excluding synth channels 13-14)
float detune[13]; // Index 0 unused, indices 1-12 for channels 1-12

// Global octave array for channels 1-8 (excluding synth channels 13-14)
float channelOctave[9]; // Index 0 unused, indices 1-8 for channels 1-8
                             
//#define GRANULAR_MEMORY_SIZE 95280  // enough for 800 ms at 44.1 kHz
//DMAMEM int16_t granularMemory[GRANULAR_MEMORY_SIZE];


// in the same file, before tmpMuteAll:





//EXTMEM?
EXTMEM Device SMP = {
  100.0,                                                                               //bpm
  1,                                                                                   //file
  1,                                                                                   //pack
  { { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 } },  //wav preset
  {},                                                                                  //param_settings
  {},                                                                                  //filter_settings
  {},                                                                                  //synth_settings
  {},                                                                                  //mute
  { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }                   //channelVol
};

GlobalVars GLOB = {
  false,                                                                               //singleMode
  1,                                                                                   //currentChannel
  10,                                                                                  //volume
  10,                                                                                  //velocity
  1,                                                                                   //page
  1,                                                                                   //edit
  0,                                                                                   //folder
  false,                                                                               //activeCopy
  1,                                                                                   //x
  2,                                                                                   //y
  0,                                                                                   //seek
  0,                                                                                   //seekEnd
  0,                                                                                   //smplen
  0,                                                                                   //shiftX
  0,                                                                                   //shiftY
  0,                                                                                   //shiftY1
  0                                                                                    //subpattern
};


float myFreq = 80;
float freqValue = 1000;
float resValue = 0.1;
float octValue = 2.0;
int transpose = 0;
int scale = 0;
int menuIndex = 0;
float gainValue = 2.0;


#define NUM_PARAMS (sizeof(SMP.param_settings[0]) / sizeof(SMP.param_settings[0][0]))
#define NUM_FILTERS (sizeof(SMP.filter_settings[0]) / sizeof(SMP.filter_settings[0][0]))
#define MAX_CHANNELS maxY  // maxY is the number of channels (e.g. 16)

uint32_t loadedSampleRate[MAX_CHANNELS];
uint32_t loadedSampleLen[MAX_CHANNELS];
int8_t channelDirection[maxFiles];
static bool prevMuteState[maxY + 1];
static bool tmpMuteActive = false;

// Per-page mute system for PMOD (pattern mode)
static bool pageMutes[maxPages][maxY];  // [page][channel] - stores mute state per page
static bool globalMutes[maxY];          // stores global mute state when PMOD is off

//EXTMEM int16_t fastRecBuffer[MAX_CHANNELS][BUFFER_SAMPLES];
static size_t fastRecWriteIndex[MAX_CHANNELS];
bool fastRecordActive = false;
//static uint8_t fastRecordChannel = 0;
bool countInActive = false;  // Count-in active for ON1 mode
int countInBeat = 0;  // Current count-in beat (0-4, where 0=waiting, 1-4=count, 5=start recording)
bool countInComplete = false;  // Flag to track when count-in (1,2,3,4) is complete
bool countInPending = false;  // Flag to track when waiting for next beat 1 to start count-in
unsigned long touch3PressStartTime = 0;  // When touch3 was first pressed
bool touch3PressProcessed = false;  // Whether the >300ms press has been processed
unsigned int recordingStartBeat = 0;  // Beat where recording started (to stop before reaching it again)
unsigned int recordingBeatCount = 0;  // Absolute beat counter for recording (doesn't wrap)
bool pendingStopFastRecord = false;  // Flag to defer stopFastRecord() from interrupt to main loop


// State variables
uint8_t filterPage[NUM_CHANNELS] = {0};
uint8_t lastEncoder = 0; // last used encoder index (0-3)



enum ParameterType { DELAY,
                     ATTACK,
                     HOLD,
                     DECAY,
                     SUSTAIN,
                     RELEASE,
                     WAVEFORM,
                     TYPE
};                         

// Number of parameter slots (ATTACK..RELEASE indexes 0-5)
constexpr int PARAM_COUNT = 6;


int maxParamVal[12] = { 1000.0, 2000.0, 1000.0, 1000.0, 1.0, 1000.0};



enum FilterType { NULL_,
                  PASS,
                  FREQUENCY,
                  REVERB,
                  BITCRUSHER,
                  DETUNE,
                  OCTAVE,
                  RES,
                  RATE,
                  AMOUNT,
                  OFFSET,
                  SPEED,
                  PITCH,
                  ACTIVE,
                  EFX,
                  FILTER_WAVEFORM
  };  //Define filter types



enum SynthTypes { INSTRUMENT,
                  CUTOFF,
                  RESONANCE,
                  FILTER,
                  CENT,
                  SEMI,
                  FORM
};  //Define Synth types


enum MidiSetTypes {
  MIDI_IN_BOOL,
  MIDI_OUT_BOOL,
  SINGLE_OUT_CHANNEL,
  GLOBAL_IN_CHANNEL,
  SEND_CTRL_BOOL,
  RECEIVE_CTRL_BOOL
};  //Define midi types

FilterType defaultFilter[maxFiles] = { PASS };

const char* const activeMidiSetType[6] PROGMEM = { "IN", "OUT", "OUT", "INPT", "SCTL", "RCTL" };

// Arrays to track multiple encoders
uint8_t buttons[NUM_ENCODERS] = { 0 };  // Changed from int to uint8_t (12 bytes saved)
unsigned long buttonPressStartTime[NUM_ENCODERS] = { 0 };
unsigned long pressDuration[NUM_ENCODERS] = { 0 };
unsigned long longPressDuration[NUM_ENCODERS] = { 200, 200, 200, 200 };  // 300ms for long press

ButtonState buttonState[NUM_ENCODERS] = { IDLE };
bool isPressed[NUM_ENCODERS] = { false };
bool pressed[NUM_ENCODERS] = { false };


i2cEncoderLibV2 Encoder[NUM_ENCODERS] = {
  i2cEncoderLibV2(0x01),  // third encoder address
  i2cEncoderLibV2(0x41),  // 2nd encoder address +
  i2cEncoderLibV2(0x20),  // First encoder address
  i2cEncoderLibV2(0x61),  // First encoder address
};
// Global variable to track current encoder index for callbacks
int currentEncoderIndex = 0;

// Slider column positions (2 LEDs each)
static const uint8_t sliderCols[4][2] = {{2,3},{6,7},{10,11},{14,15}};

// Define which data array each slider uses
enum SettingArray { ARR_FILTER, ARR_SYNTH, ARR_PARAM, ARR_NONE };

// Define named struct for sliderDef
struct SliderDefEntry {
  SettingArray arr;
  int8_t idx;
  const char* name;
  uint8_t maxValue;
  uint8_t displayMode;
  const char** enumNames;
  uint8_t displayRange;
};

// For each page and slot: {arrayType, settingIndex, name}
DMAMEM static SliderDefEntry sliderDef[NUM_CHANNELS][4][4];

void initSliderDefTemplates() {
    static const SliderDefEntry sliderDefTemplate[4][4] = {
        {
          {ARR_FILTER, PASS, "PASS", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_FILTER, FREQUENCY, "FREQ", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_FILTER, REVERB, "RVRB", 32, DISPLAY_NUMERIC, nullptr, 32},   
          {ARR_FILTER, BITCRUSHER, "BITC", 32, DISPLAY_NUMERIC, nullptr, 32}
        },
        {
          {ARR_SYNTH, CUTOFF, "CUT", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_SYNTH, RESONANCE, "RES", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_SYNTH, FILTER, "FLT", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0}
        },
        {
          {ARR_FILTER, FILTER_WAVEFORM, "WAVE", 16, DISPLAY_ENUM, waveformNames, 4}, 
          {ARR_SYNTH, INSTRUMENT, "INST", 9, DISPLAY_ENUM, instTypeNames, 10}, 
          {ARR_SYNTH, CENT, "CENT", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_SYNTH, SEMI, "SEMI", 32, DISPLAY_NUMERIC, nullptr, 32}
        },
        {
          {ARR_PARAM, ATTACK, "ATTC", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_PARAM, DECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_PARAM, SUSTAIN, "SUST", 32, DISPLAY_NUMERIC, nullptr, 32}, 
          {ARR_PARAM, RELEASE, "RLSE", 32, DISPLAY_NUMERIC, nullptr, 32}
          }
    };
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        for (int p = 0; p < 4; ++p) {
            for (int s = 0; s < 4; ++s) {
                sliderDef[ch][p][s] = sliderDefTemplate[p][s];
            }
        }
    }
    // For channel 1 (index 1), set the fourth page to ARR_NONE for all slots
    for (int s = 0; s < 4; ++s) {
        sliderDef[1][3][s].arr = ARR_NONE;
        sliderDef[1][3][s].idx = 0;
    }
    
}

struct FilterTarget {
  SettingArray arr;
  uint8_t idx;
};
FilterTarget defaultFastFilter[NUM_CHANNELS]; // one per channel

EXTMEM arraysampler _samplers[9];
AudioPlayArrayResmp *voices[9] = { &sound0, &sound1, &sound2, &sound3, &sound4, &sound5, &sound6, &sound7, &sound8 };

AudioEffectEnvelope *envelopes[15] = { &envelope0, &envelope1, &envelope2, &envelope3, &envelope4, &envelope5, &envelope6, &envelope7, &envelope8, nullptr, nullptr, &envelope11, nullptr, &envelope13, &envelope14 };
AudioAmplifier *amps[15] = { nullptr, &amp1, &amp2, &amp3, &amp4, &amp5, &amp6, &amp7, &amp8, nullptr, nullptr, &amp11, nullptr, &amp13, &amp14 };
AudioFilterStateVariable *filters[15] = { nullptr, &filter1, &filter2, &filter3, &filter4, &filter5, &filter6, &filter7, &filter8, nullptr, nullptr, &filter11, nullptr, &filter13, &filter14 };
AudioMixer4 *filtermixers[15] = { nullptr, &filtermixer1, &filtermixer2, &filtermixer3, &filtermixer4, &filtermixer5, &filtermixer6, &filtermixer7, &filtermixer8, nullptr, nullptr, &filtermixer11, nullptr, &filtermixer13, &filtermixer14 };
AudioEffectBitcrusher *bitcrushers[15] = { nullptr, &bitcrusher1, &bitcrusher2, &bitcrusher3, &bitcrusher4, &bitcrusher5, &bitcrusher6, &bitcrusher7, &bitcrusher8, nullptr, nullptr, &bitcrusher11, nullptr, &bitcrusher13, &bitcrusher14 };
AudioEffectFreeverbDMAMEM *freeverbs[15] = { nullptr, &freeverb1, &freeverb2, nullptr, nullptr, &freeverb5, &freeverb6, &freeverb7, &freeverb8, 0, 0, &freeverb11, nullptr, nullptr, nullptr };
AudioMixer4 *freeverbmixers[15] = { nullptr, &freeverbmixer1, &freeverbmixer2, nullptr, nullptr, &freeverbmixer5, &freeverbmixer6, &freeverbmixer7, &freeverbmixer8, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
AudioMixer4 *waveformmixers[15] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &mixer_waveform11, nullptr, &mixer_waveform13, &mixer_waveform14 };

// Stop synth channels (13/14) immediately
static inline void stopSynthChannel(int ch) {
  if (ch < 13 || ch > 14) return;
  if (envelopes[ch]) envelopes[ch]->noteOff();
  noteOnTriggered[ch] = false;
  persistentNoteOn[ch] = false;
}

// Map external 14-bit input (e.g. pitchbend) to current channel fast filter
static inline void applyExternalFastFilter(uint16_t value14) {
  FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
  if (dft.arr == ARR_NONE) return;
  int mapped = constrain((int)mapf(value14, 0.0f, 16383.0f, 0.0f, maxfilterResolution), 0, (int)maxfilterResolution);

  // Guard PARAM index
  if (dft.arr == ARR_PARAM && dft.idx >= PARAM_COUNT) return;

  setDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx, mapped);

  switch (dft.arr) {
    case ARR_FILTER:
      setFilters(dft.idx, GLOB.currentChannel, false);
      break;
    case ARR_SYNTH:
      if (GLOB.currentChannel == 11) updateSynthVoice(11);
      break;
    case ARR_PARAM:
      setParams(dft.idx, GLOB.currentChannel);
      break;
    default:
      break;
  }
}

AudioSynthWaveform *synths[15][2];


//for FULL patch:

AudioSynthWaveformDc *Sdc1[3] = { &Sdc1_0, &Sdc1_1, &Sdc1_2 };
AudioEffectEnvelope *Senvelope1[3] = { &Senvelope1_0, &Senvelope1_1, &Senvelope1_2 };
AudioEffectEnvelope *Senvelope2[3] = { &Senvelope2_0, &Senvelope2_1, &Senvelope2_2 };
AudioEffectEnvelope *SenvelopeFilter1[3] = { &SenvelopeFilter1_0, &SenvelopeFilter1_1, &SenvelopeFilter1_2 };

AudioSynthWaveform *Swaveform1[3] = { &Swaveform1_0, &Swaveform1_1, &Swaveform1_2 };
AudioSynthWaveform *Swaveform2[3] = { &Swaveform2_0, &Swaveform2_1, &Swaveform2_2 };
AudioSynthWaveform *Swaveform3[3] = { &Swaveform3_0, &Swaveform3_1, &Swaveform3_2 };

AudioFilterLadder *Sladder1[3] = { &Sladder1_0, &Sladder1_1, &Sladder1_2 };
AudioFilterLadder *Sladder2[3] = { &Sladder2_0, &Sladder2_1, &Sladder2_2 };

AudioMixer4 *Smixer1[3] = { &Smixer1_0, &Smixer1_1, &Smixer1_2 };
AudioMixer4 *Smixer2[3] = { &Smixer2_0, &Smixer2_1, &Smixer2_2 };
/**/

void allOff() {
  for (AudioEffectEnvelope *envelope : envelopes) {
    if (!envelope) continue;
    envelope->noteOff();
  }
}



FLASHMEM void setVelocity() {
  // Update velocity (encoder[0])
  if (currentMode->pos[0] != GLOB.velocity) {
    GLOB.velocity = currentMode->pos[0];

    if (GLOB.singleMode) {
      note[GLOB.x][GLOB.y].velocity = round(mapf(GLOB.velocity, 1, maxY, 1, 127));
    } else {

      for (unsigned int nx = 1; nx < maxlen; nx++) {
        for (unsigned int ny = 1; ny < maxY + 1; ny++) {
          if (note[nx][ny].channel == note[GLOB.x][GLOB.y].channel)
            note[nx][ny].velocity = round(mapf(GLOB.velocity, 1, maxY, 1, 127));
        }
      }
    }
  }
  
  // Update probability (encoder[1])
  static int lastProbStep = -1;
  int currentProbStep = currentMode->pos[1];
  
  if (currentProbStep != lastProbStep) {
    lastProbStep = currentProbStep;
    
    // Map encoder steps 1-5 to probability values 0%, 25%, 50%, 75%, 100%
    uint8_t probValue;
    switch (currentProbStep) {
      case 1: probValue = 0; break;
      case 2: probValue = 25; break;
      case 3: probValue = 50; break;
      case 4: probValue = 75; break;
      case 5: probValue = 100; break;
      default: probValue = 100; break;
    }
    note[GLOB.x][GLOB.y].probability = probValue;
  }
  
  // Update condition (encoder[3])
  static int lastCondStep = -1;
  int currentCondStep = currentMode->pos[3];
  
  if (currentCondStep != lastCondStep) {
    lastCondStep = currentCondStep;
    
    // Map encoder steps 1-9 to condition values
    // Positions 1-5: 1/X conditions -> values 1, 2, 4, 8, 16
    // Positions 6-9: X/1 conditions -> values 17, 18, 19, 20
    // Use PROGMEM lookup table to save RAM
    static const uint8_t condValues[9] PROGMEM = {1, 2, 4, 8, 16, 17, 18, 19, 20};
    uint8_t condValue;
    if (currentCondStep >= 1 && currentCondStep <= 9) {
      condValue = pgm_read_byte(&condValues[currentCondStep - 1]);
    } else {
      condValue = 1;  // Default
    }
    note[GLOB.x][GLOB.y].condition = condValue;
    
    // Condition changed (debug removed)
  }
  
  //CHANNEL VOLUME
  if (currentMode->pos[2] != GLOB.velocity) {
    SMP.channelVol[GLOB.currentChannel] = currentMode->pos[2];
    float channelvolume = mapf(SMP.channelVol[GLOB.currentChannel], 0, maxY, 0, 1);
    //Serial.println(channelvolume);
    amps[GLOB.currentChannel]->gain(channelvolume);
  }

  drawVelocity();
}




void staticButtonPushed(i2cEncoderLibV2 *obj) {
  pressed[currentEncoderIndex] = true;
  encoder_button_pushed(obj, currentEncoderIndex);
  
}




void staticButtonReleased(i2cEncoderLibV2 *obj) {
  encoder_button_released(obj, currentEncoderIndex);
  pressed[currentEncoderIndex] = false;
}

void staticThresholds(i2cEncoderLibV2 *obj) {
  //  encoder_thresholds(obj, currentEncoderIndex);
}



// Debounce tracking for button presses/releases (in fast RAM for interrupt handlers)
unsigned long lastButtonChange[NUM_ENCODERS] = {0, 0, 0, 0};
const unsigned long btnDebounce = 30;  // Debounce time in milliseconds

void encoder_button_pushed(i2cEncoderLibV2 *obj, int encoderIndex) {
  unsigned long currentTime = millis();
  
  // Debounce: Only accept button press if enough time has passed since last change
  if (currentTime - lastButtonChange[encoderIndex] < btnDebounce) {
    return;  // Ignore spurious button press (debounce)
  }
  lastButtonChange[encoderIndex] = currentTime;

  buttonPressStartTime[encoderIndex] = currentTime;
  isPressed[encoderIndex] = true;
}



void encoder_button_released(i2cEncoderLibV2 *obj, int encoderIndex) {
  // Debounce: Only accept button release if enough time has passed since last change
  unsigned long currentTime = millis();
  if (currentTime - lastButtonChange[encoderIndex] < btnDebounce) {
    return;  // Ignore spurious button release (debounce)
  }
  lastButtonChange[encoderIndex] = currentTime;
  
  buttonState[encoderIndex] = RELEASED;
}





static unsigned long lastBtnChange = 0;
const unsigned long BUTTON_STUCK_TIMEOUT_MS = 10000; // 10 seconds - if button appears stuck, force reset
void handle_button_state(i2cEncoderLibV2 *obj, int encoderIndex) {
  unsigned long currentTime = millis();
  // `isPressed[encoderIndex]` is true if button is physically down, false if up.
  // `buttonState[encoderIndex]` is our FSM state: IDLE, LONG_PRESSED, RELEASED.

  // Recovery mechanism: If button appears stuck pressed for too long, force reset
  // This handles cases where I2C communication fails or callbacks are missed
  // Also reset if button state is 2 (long press) for more than 10 seconds
  if (isPressed[encoderIndex] && buttonState[encoderIndex] != IDLE) {
    unsigned long pressDuration;
    // Handle millis() wraparound (happens every ~50 days)
    if (currentTime < buttonPressStartTime[encoderIndex]) {
      // Wraparound occurred
      pressDuration = (0xFFFFFFFF - buttonPressStartTime[encoderIndex]) + currentTime;
    } else {
      pressDuration = currentTime - buttonPressStartTime[encoderIndex];
    }
    
    if (pressDuration > BUTTON_STUCK_TIMEOUT_MS) {
      // Button has been "pressed" for too long (10+ seconds) - likely stuck state, force reset
      isPressed[encoderIndex] = false;
      buttonState[encoderIndex] = IDLE;
      buttons[encoderIndex] = 0;
      buttonPressStartTime[encoderIndex] = 0;  // Reset for next valid press
    }
  } else if (buttons[encoderIndex] == 2 && buttonState[encoderIndex] == LONG_PRESSED) {
    // Also check if button state 2 (long press) has been active for too long
    unsigned long pressDuration;
    // Handle millis() wraparound
    if (currentTime < buttonPressStartTime[encoderIndex]) {
      pressDuration = (0xFFFFFFFF - buttonPressStartTime[encoderIndex]) + currentTime;
    } else {
      pressDuration = currentTime - buttonPressStartTime[encoderIndex];
    }
    
    if (pressDuration > BUTTON_STUCK_TIMEOUT_MS) {
      // Button state 2 (long press) has been stuck for 10+ seconds - force reset
      isPressed[encoderIndex] = false;
      buttonState[encoderIndex] = IDLE;
      buttons[encoderIndex] = 0;
      buttonPressStartTime[encoderIndex] = 0;  // Reset for next valid press
    }
  }

  if (buttonState[encoderIndex] == RELEASED) {
    // This state was set by encoder_button_released callback.
    // We are now processing the consequences of that release.
    pressDuration[encoderIndex] = currentTime - buttonPressStartTime[encoderIndex];  // Calculate final duration

    if (pressDuration[encoderIndex] <= longPressDuration[encoderIndex]) {
      buttons[encoderIndex] = 1;  // Event: Short release occurred
    } else {
      buttons[encoderIndex] = 9;  // Event: Long release occurred
    }
    buttonState[encoderIndex] = IDLE;  // Transition back to IDLE
    isPressed[encoderIndex] = false;   // Button is now physically up

  } else if (isPressed[encoderIndex]) {  // Button is currently physically down
    pressDuration[encoderIndex] = currentTime - buttonPressStartTime[encoderIndex];

    if (buttonState[encoderIndex] == IDLE) {
      if (pressDuration[encoderIndex] >= longPressDuration[encoderIndex]) {
        buttons[encoderIndex] = 2;                 // Event: Long press threshold reached (still held)
        buttonState[encoderIndex] = LONG_PRESSED;  // Change state
      } else {
        // Still pressed, but not yet long. No "1" or "9" event.
        // `buttons[encoderIndex]` should be 0 if no other event is active.
        // It might have been set to 1 or 9 by a previous release and then cleared by checkButtons.
        // If it's a fresh press, it should be 0.
        // To be safe, if no specific event (2) is generated:
        if (buttons[encoderIndex] != 2) buttons[encoderIndex] = 0;
      }
    } else if (buttonState[encoderIndex] == LONG_PRESSED) {
      // Already in long press state, button still held. Event '2' is sticky.
      buttons[encoderIndex] = 2;  // Keep it as 2
    }

  } else {  // Button is not pressed (isPressed[encoderIndex] is false)
    // Additional safety: If we think button is pressed but encoder says it's not,
    // verify this state persists before clearing (helps filter I2C glitches)
    static unsigned long lastNotPressedTime[NUM_ENCODERS] = {0, 0, 0, 0};
    if (buttonState[encoderIndex] != IDLE || isPressed[encoderIndex]) {
      // If it's not pressed, but state wasn't RELEASED to transition to IDLE, 
      // wait a bit to ensure it's not a transient I2C glitch
      if (currentTime - lastNotPressedTime[encoderIndex] > btnDebounce) {
        // State has been "not pressed" for debounce period, safe to reset
        // This handles cases where isPressed got stuck due to missed callback
        buttonState[encoderIndex] = IDLE;
        isPressed[encoderIndex] = false;  // Force reset if stuck
        buttons[encoderIndex] = 0;
      }
      lastNotPressedTime[encoderIndex] = currentTime;
    } else {
      lastNotPressedTime[encoderIndex] = 0;  // Reset when in IDLE
    }
    // If button is up and state is IDLE (or just became IDLE)
    // and it wasn't a 1 or 9 event from a RELEASED state this cycle:
    if (buttons[encoderIndex] != 1 && buttons[encoderIndex] != 9) {  // Don't overwrite a just-set release event
      buttons[encoderIndex] = 0;
    }
  }
}

// Helper function for checkMode
bool match_buttons(const uint8_t states[], int b0, int b1, int b2, int b3) {
  return states[0] == b0 && states[1] == b1 && states[2] == b2 && states[3] == b3;
}

// Helper function to check if previous button state was 0000
bool was_buttons_0000(const uint8_t prevStates[]) {
  return prevStates[0] == 0 && prevStates[1] == 0 && prevStates[2] == 0 && prevStates[3] == 0;
}

// Helper function to check if previous button state was NOT 0000
bool was_not_buttons_0000(const uint8_t prevStates[]) {
  return !was_buttons_0000(prevStates);
}

// Hash function for String (simple djb2-like hash)
uint32_t hashString(const String& str) {
  uint32_t hash = 5381;
  for (size_t i = 0; i < str.length(); i++) {
    hash = ((hash << 5) + hash) + str.charAt(i);  // hash * 33 + char
  }
  return hash;
}





static void resetCtrlModeState() {
  ctrlLastChannel = -1;
  ctrlLastVolume = -1;
}

FLASHMEM void showCtrlVolumeChange(int volume) {
  ctrlVolumeOverlayValue = constrain(volume, 0, maxY);
  ctrlVolumeOverlayActive = true;
  ctrlVolumeOverlayUntil = millis() + 600;
}

FLASHMEM void showInputGainOverlay(int gain, int maxGain) {
  inputGainOverlayValue = constrain(gain, 0, maxGain);
  inputGainOverlayMax = maxGain;
  inputGainOverlayActive = true;
  inputGainOverlayUntil = millis() + 600;
}

void refreshCtrlEncoderConfig() {
  if (!(currentMode == &draw || currentMode == &singleMode)) {
    resetCtrlModeState();
    ctrlVolumeOverlayActive = false;
    inputGainOverlayActive = false;
    return;
  }

  if (ctrlMode == 0) {
    resetCtrlModeState();
    updateLastPage();
    int adjustedMax = SMP_PATTERN_MODE ? lastPage : (MAX_STEPS / maxX);
    if (adjustedMax < 1) adjustedMax = 1;

    currentMode->minValues[1] = 1;
    currentMode->maxValues[1] = adjustedMax;

    int target = constrain(editpage, 1, adjustedMax);
    currentMode->pos[1] = target;
    Encoder[1].writeMin((int32_t)1);
    Encoder[1].writeMax((int32_t)adjustedMax);
    Encoder[1].writeCounter((int32_t)target);
    Encoder[1].writeRGBCode(currentMode->knobcolor[1]);
    ctrlVolumeOverlayActive = false;
  } else {
    ctrlLastChannel = -1;
    ctrlLastVolume = -1;
    currentMode->minValues[1] = 0;
    currentMode->maxValues[1] = 16;

    int currentVol = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);
    currentMode->pos[1] = currentVol;
    Encoder[1].writeMin((int32_t)0);
    Encoder[1].writeMax((int32_t)16);
    Encoder[1].writeCounter((int32_t)currentVol);
    CRGB volColor = CRGB(currentVol * currentVol, max(0, 20 - currentVol), 0);
    Encoder[1].writeRGBCode(volColor.r << 16 | volColor.g << 8 | volColor.b);
  }
}

void switchMode(Mode *newMode) {
  updateLastPage();
  
  drawNoSD_hasRun = false;

  unpaintMode = false;
  GLOB.singleMode = false;
  paintMode = false;
  oldMode = currentMode;
  
  // Additional safety: Reset paintMode when leaving draw/singleMode
  if ((oldMode == &draw || oldMode == &singleMode) && (newMode != &draw && newMode != &singleMode)) {
    paintMode = false;
    unpaintMode = false;
    preventPaintUnpaint = false;
  }

  if (currentMode == &recordMode && newMode != &recordMode) {
    extern AudioMixer4 mixer_end;
    mixer_end.gain(3, 0.0f);
  }
  
  /// OLD ACTIONS
  if (currentMode == &set_Wav || currentMode == &filterMode) {
    //RESET left encoder
    Encoder[0].begin(
      i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
      | i2cEncoderLibV2::DIRE_LEFT | i2cEncoderLibV2::IPUP_ENABLE
      | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);
  }
  /// NEW ACTIONS
  //Serial.println(newMode->name);
  // oldButtonString = buttonString; // REMOVED
  // Synchronize oldButtons with current buttons state
  memcpy(oldButtons, buttons, sizeof(buttons));  // ADDED

  if (newMode != currentMode) {

    
    if (muteModeActive && currentMode == &subpatternMode && newMode != &subpatternMode) {
      if (tmpMute) {
        tmpMuteAll(false);
        tmpMute = false;
      }
      muteModeActive = false;
      muteModeReturn = nullptr;
      muteModeLastChannel = -1;
      muteModeReturnSingleState = false;
      muteModeArrowDirection = 0;
      muteModeArrowUntil = 0;
    }

    currentMode = newMode;
    if (currentMode == &subpatternMode && muteModeActive) {
      muteModeLastChannel = GLOB.currentChannel;
      currentMode->pos[2] = 0;
      muteModeArrowDirection = 0;
      muteModeArrowUntil = 0;
      muteModeEncoderValue = 0;
    }
    // Set last saved values for encoders

    // When entering draw/single, restore X (encoder 3) from grid position, not prior mode state
    if (currentMode == &draw || currentMode == &singleMode) {
      int maxStepsRuntime = (int)MAX_STEPS;
      GLOB.x = constrain(GLOB.x, 1, maxStepsRuntime);
      currentMode->pos[3] = GLOB.x;
    }



    for (int i = 0; i < NUM_ENCODERS; i++) {
      Encoder[i].writeRGBCode(currentMode->knobcolor[i]);
      int32_t minVal = currentMode->minValues[i];
      int32_t maxVal = currentMode->maxValues[i];
      int32_t counterVal = currentMode->pos[i];

      // Special handling for encoder 1 (page control)
      if (i == 1 && (currentMode == &draw || currentMode == &singleMode)) {
        if (ctrlMode == 0) {
          if (SMP_PATTERN_MODE) {
            updateLastPage();
            maxVal = lastPage;
          } else {
            maxVal = MAX_STEPS / maxX;
          }
          minVal = 1;
          counterVal = constrain(editpage, (int)minVal, (int)maxVal);
        } else {
          minVal = 0;
          maxVal = 16;
          counterVal = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);
        }
      } else if (currentMode == &subpatternMode && muteModeActive && i == 2) {
        maxVal = 1;
        minVal = -1;
        counterVal = 0;
        currentMode->pos[2] = 0;
      } else if (i == 2 && (currentMode == &draw || currentMode == &singleMode) && oldMode == &filterMode) {
        // When exiting filtermode to draw/singleMode, preserve the fastfilter value in encoder 2
        FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
        int page, slot;
        if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
          counterVal = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);
          currentMode->pos[2] = counterVal;  // Update mode's pos[2] to match fastfilter value
        }
      } else if (currentMode == &velocity && i == 3) {
        // Encoder[3] in velocity mode: condition range 1-9
        // Positions: 1=1/1, 2=1/2, 3=1/4, 4=1/8, 5=1/16, 6=2/1, 7=4/1, 8=8/1, 9=16/1
        maxVal = 9;
        minVal = 1;
        // Initialize with current note's condition value
        if (note[GLOB.x][GLOB.y].channel != 0) {
          uint8_t cond = note[GLOB.x][GLOB.y].condition;
          if (cond == 0) cond = 1;  // Default to 1 if not set
          // Map condition value to encoder position
          // Values 1-16: 1/X conditions -> positions 1-5
          // Values 17-20: X/1 conditions -> positions 6-9
          if (cond <= 16) {
            counterVal = (cond == 1) ? 1 : (cond == 2) ? 2 : (cond == 4) ? 3 : (cond == 8) ? 4 : 5;
          } else {
            // X/1 conditions: 17=2/1, 18=4/1, 19=8/1, 20=16/1
            counterVal = (cond == 17) ? 6 : (cond == 18) ? 7 : (cond == 19) ? 8 : 9;
          }
          currentMode->pos[3] = counterVal;
        } else {
          counterVal = 1;  // Default condition
          currentMode->pos[3] = 1;
        }
      } else {
      }

      Encoder[i].writeMax((int32_t)maxVal);
      Encoder[i].writeMin((int32_t)minVal);

      if ((currentMode == &singleMode && oldMode == &draw) || (currentMode == &draw && oldMode == &singleMode)) {
        //do not move Cursor for those modes
        //
      } else {
        Encoder[i].writeCounter(counterVal);
      }
    }

    // Special handling for singleMode - set currentChannel color on encoders[0] and [3]
    if (currentMode == &singleMode) {
      Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    }

    if (currentMode == &draw || currentMode == &singleMode) {
      refreshCtrlEncoderConfig();
    } else {
      resetCtrlModeState();
    }
    
    if (currentMode == &set_Wav) {
      //REVERSE left encoder
      Encoder[0].begin(
        i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
        | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
        | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);

      Encoder[3].writeMax((int32_t)999);  //maxval
      Encoder[3].writeMin((int32_t)1);    //minval
      
      // Initialize encoder 3 with the current channel's saved fileID
      int fileID = SMP.wav[GLOB.currentChannel].fileID;
      if (fileID < 1) fileID = 1;  // Ensure valid range
      fileID = constrain(fileID, 1, 999);
      currentMode->pos[3] = fileID;
      Encoder[3].writeCounter((int32_t)fileID);
    }
    
    if (currentMode == &filterMode) {
      //REVERSE left encoder
      Serial.println("LOAD FILTERS");
      initSliders(filterPage[GLOB.currentChannel], GLOB.currentChannel);
      Encoder[0].begin(
        i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
        | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
        | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);

      // Set encoder colors dynamically based on current filter page and default fast filter
      updateFilterEncoderColors();

      //SMP.selectedFX = TYPE;
      //Encoder[3].writeCounter((int32_t)SMP.param_settings[GLOB.currentChannel].channel);
    }
    
    if (currentMode == &volume_bpm) {
      // Set encoder 0 and 1 to black (no color)
      Encoder[0].writeRGBCode(0x000000);
      Encoder[1].writeRGBCode(0x000000);
      
      // Set encoder 1 position to match current brightness
      // ledBrightness = (pos[1] * 10) - 46, so pos[1] = (ledBrightness + 46) / 10
      unsigned int brightnessPos = (ledBrightness + 46) / 10;
      brightnessPos = constrain(brightnessPos, 6, 25);  // Valid range for volume_bpm mode
      currentMode->pos[1] = brightnessPos;
      Encoder[1].writeCounter((int32_t)brightnessPos);
      
      // Set encoder 2 position to match current clockMode (1=INT -> 1, -1=EXT -> 0)
      extern int clockMode;
      currentMode->pos[2] = (clockMode == 1) ? 1 : 0;
      Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
    }
    
    if (currentMode == &loadSaveTrack) {
      // Initialize encoder[2] to current SMP_LOAD_SETTINGS value
      // Only allow settings loading if file exists
      char OUTPUTf[50];
      sprintf(OUTPUTf, "%u.txt", SMP.file);
      if (SD.exists(OUTPUTf)) {
        currentMode->pos[2] = SMP_LOAD_SETTINGS ? 1 : 0;
      } else {
        // For empty files, force settings loading off
        currentMode->pos[2] = 0;
      }
      Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
    }

    if (currentMode == &subpatternMode && muteModeActive) {
      tmpMute = true;
      tmpMuteAll(true);
    }
  }

  drawPlayButton();
}


void checkFastRec() {
  bool channelAllowed = (GLOB.currentChannel <= 8);

  if (!channelAllowed && !fastRecordActive) return;

  bool allowStart = channelAllowed && (GLOB.y > 1);

  if ((currentMode == &draw || currentMode == &singleMode) && SMP_FAST_REC == 2 || SMP_FAST_REC == 3) {
    bool pinsConnected = (digitalRead(2) == LOW);

    if (pinsConnected != lastPinsConnected && millis() - lastChangeTime > debounceDelay) {
      lastChangeTime = millis();  // Update timestamp
      lastPinsConnected = pinsConnected;

      if (SMP_FAST_REC == 2) {
        if (pinsConnected && !fastRecordActive) {
          if (!allowStart) return;
          //Serial.println(">> startFastRecord from pin 2+4");
          startFastRecord();
          return;
        } else if (!pinsConnected && fastRecordActive) {
          //Serial.println(">> stopFastRecord from pin 2+4");
          stopFastRecord();
        }
      }

      if (SMP_FAST_REC == 3) {
        if (!pinsConnected && !fastRecordActive) {
          if (!allowStart) return;
          //Serial.println(">> startFastRecord from pin 2+4");
          startFastRecord();
          return;
        } else if (pinsConnected && fastRecordActive) {
          //Serial.println(">> stopFastRecord from pin 2+4");
          stopFastRecord();
        }
      }
    }
  }

if (currentMode == &draw || currentMode == &singleMode){
  int touchValue3 = fastTouchRead(SWITCH_3);
  bool currentTouchState = (touchValue3 > touchThreshold);
  touchState[2] = currentTouchState;

  if (SMP_FAST_REC == 1) {
    // ON1 mode: requires >300ms press, count-in, then auto-start/stop at beat boundaries
    if (recChannelClear == 3) {
      if (!fastRecordActive && !countInActive) {
        if (currentTouchState && !touch3PressProcessed) {
          // Touch3 just pressed - start timer
          if (touch3PressStartTime == 0) {
            touch3PressStartTime = millis();
          }
          // Check if pressed for >300ms
          if (millis() - touch3PressStartTime >= 300) {
            // >300ms press detected - trigger count-in
            if (!allowStart) {
              touch3PressStartTime = 0;  // Reset if not allowed
              return;
            }
            touch3PressProcessed = true;  // Mark as processed to avoid retriggering
            
            // If playing, mark that we want to start count-in
            // We'll wait for the next beat 1 (full bar) before showing count-in
            if (isNowPlaying) {
              countInPending = true;  // Wait for next beat 1 to start count-in
              countInActive = false;
              countInBeat = 0;  // Reset count-in
              countInComplete = false;  // Reset completion flag
              touch3PressStartTime = 0;  // Reset timer
              return;
            }
            touch3PressStartTime = 0;  // Reset timer
          }
        } else if (!currentTouchState && !touch3PressProcessed) {
          // Touch3 released before >300ms - reset timer
          touch3PressStartTime = 0;
        }
      }
      
      // Reset processed flag when touch3 is released
      if (!currentTouchState && touch3PressProcessed) {
        touch3PressProcessed = false;
        touch3PressStartTime = 0;
      }
    } else {
      // Non-ON1 modes: start/stop directly on touch/release (old behavior)
      if (currentTouchState && !fastRecordActive) {
        // Touch3 pressed - start recording immediately
        if (allowStart) {
          recordingStartBeat = beat;  // Track where recording starts
          recordingBeatCount = 0;  // Reset absolute beat counter
          startFastRecord();
        }
      } else if (!currentTouchState && fastRecordActive) {
        // Touch3 released - stop recording immediately
    stopFastRecord();
      }
    }
  }
}
}

void checkMode(const uint8_t currentButtonStates[NUM_ENCODERS], bool reset) {
  //checkFastRec();

  
  // In recordMode, allow encoder[2] press to stop recording (no auto-playback)
  if (isRecording && currentMode == &recordMode && pressed[2]) {
    stopRecordingRAM(getFolderNumber(SMP.wav[GLOB.currentChannel].fileID), SMP.wav[GLOB.currentChannel].fileID);
    
    // Turn off threshold trigger when stopping
    extern bool disableThresholdFlag;
    disableThresholdFlag = true;
    
    // Don't auto-play - user will press encoder[2] again to play
    return;
  }
  
  if (isRecording) return;


  // Toggle play/pause in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && match_buttons(currentButtonStates, 0, 0, 1, 0)) {  // "0010"

    if (!isNowPlaying) {
      //Serial.println("PLAY");
      play(true);
    } else {
      unsigned long currentTime = millis();
      if (currentTime - playStartTime > 200) {  // Check if play started more than 200ms ago
        pause();
      }
    }
  }

  if (GLOB.y != 16 && (currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && match_buttons(currentButtonStates, 0, 2, 0, 0) && was_buttons_0000(oldButtons)) {  // "0200" - must be 0000 before
    if (!muteModeActive) {
      muteModeActive = true;
      muteModeReturn = currentMode;
      muteModeReturnSingleState = GLOB.singleMode;
      switchMode(&subpatternMode);
    }
  }

  if (GLOB.y == 16 && (currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && match_buttons(currentButtonStates, 0, 2, 0, 0) && was_buttons_0000(oldButtons)) {  // "0200" - must be 0000 before
   //switch subpattern
    switchMode(&subpatternMode);
    GLOB.singleMode = (currentMode == &singleMode);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 9, 0, 0) && was_not_buttons_0000(oldButtons)) {  // "0900" - must be !=0000 before
    if (tmpMute) tmpMuteAll(false);
    drawKnobColorDefault();
  }

  /**/

  // Shift notes around in single mode when y=16 and 0-1-0-0 (like copypaste)
  if (currentMode == &singleMode && GLOB.y == 16 && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100" when y=16
    preventPaintUnpaint = true;  // Prevent paint/unpaint immediately when entering shiftnotes
    GLOB.shiftX = 8;
    GLOB.shiftY = 8;
    GLOB.shiftY1 = 8;

    Encoder[3].writeCounter((int32_t)8);
    Encoder[1].writeCounter((int32_t)8);
    unsigned int patternLength = lastPage * maxX;

    for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
      for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
        original[nx][ny].channel = 0;
        original[nx][ny].velocity = defaultVelocity;
        original[nx][ny].probability = 100;  // Default probability
        original[nx][ny].condition = 1;      // Default condition
      }
    }

    // Step 2: Backup non-current channel notes into the original array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (note[nx][ny].channel != GLOB.currentChannel) {
          original[nx][ny] = note[nx][ny];
        }
      }
    }
    // Switch to note shift mode
    switchMode(&noteShift);
    GLOB.singleMode = true;
    return;  // Prevent other button actions from being processed
  }

  if (currentMode == &menu && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    extern bool inLookSubmenu;
    extern bool inRecsSubmenu;
    extern bool inMidiSubmenu;
    extern bool inVolSubmenu;
    extern int currentMenuPage;
    
    // If in LOOK submenu, exit back to main menu at LOOK page (index 4)
    if (inLookSubmenu) {
      inLookSubmenu = false;
      currentMenuPage = 4;
      Encoder[3].writeCounter((int32_t)4);
      return;
    }
    
    // If in RECS submenu, exit back to main menu at RECS page (index 5)
    if (inRecsSubmenu) {
      inRecsSubmenu = false;
      currentMenuPage = 5;
      Encoder[3].writeCounter((int32_t)5);
      return;
    }
    
    // If in MIDI submenu, exit back to main menu at MIDI page (index 6)
    if (inMidiSubmenu) {
      inMidiSubmenu = false;
      currentMenuPage = 6;
      Encoder[3].writeCounter((int32_t)6);
      return;
    }
    
    // If in VOL submenu, exit back to main menu at VOL page (index 4)
    if (inVolSubmenu) {
      inVolSubmenu = false;
      currentMenuPage = 4;
      Encoder[3].writeCounter((int32_t)4);
      return;
    }
    
    // Get the main setting for the current page
    extern int getCurrentMenuMainSetting();
    int mainSetting = getCurrentMenuMainSetting();
    
    // Special case: If on AI menu (setting 15), trigger AI generation
    if (mainSetting == 15) {
      switchMenu(mainSetting);  // Trigger AI generation
    } else {
      // For all other menu pages, exit to draw mode
      switchMode(&draw);
      GLOB.singleMode = false;
    }
  }

  if (currentMode == &menu && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    // Get the main setting for the current page
    extern int getCurrentMenuMainSetting();
    int mainSetting = getCurrentMenuMainSetting();
    
    // Encoder[3] triggers action for all menu items EXCEPT AI (which uses encoder[0])
    // Note: Encoder button does NOT exit submenus - only touch buttons do
    if (mainSetting != 15) {
      switchMenu(mainSetting);
    }
  } else if (currentMode == &newFileMode && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    // Exit NEW mode and switch back to DAT view (unchanged)
    extern void resetNewModeState();
    resetNewModeState();
    switchMode(&loadSaveTrack);
    return;
  } else if (currentMode == &newFileMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    // Generate genre track and exit NEW mode
    extern void generateGenreTrack();
    extern void resetNewModeState();
    resetNewModeState();
    generateGenreTrack();
    return;
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    paintMode = false;
    unpaintMode = false;
    preventPaintUnpaint = false;  // Reset flag when exiting loadSaveTrack mode
    switchMode(&draw);
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    savePattern(false);
    preventPaintUnpaint = true;  // Prevent paint/unpaint after save
    return;  // Prevent other button actions from being processed
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    loadPattern(false);
    preventPaintUnpaint = true;  // Prevent paint/unpaint after load
    return;  // Prevent other button actions from being processed
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    saveSamplePack(SMP.pack);
    return;  // Prevent other button actions from being processed
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    loadSamplePack(SMP.pack, false);
    return;  // Prevent other button actions from being processed
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    switchMode(&draw);
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"

    loadWav();
    autoSave();
    return;  // Prevent other button actions from being processed
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    switchMode(&singleMode);
  } else if ((currentMode == &recordMode) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001" - Exit recordMode
    if (isRecording) {
      stopRecordingRAM(getFolderNumber(SMP.wav[GLOB.currentChannel].fileID), SMP.wav[GLOB.currentChannel].fileID);
      // Turn off threshold trigger when stopping
      extern bool disableThresholdFlag;
      disableThresholdFlag = true;
    }
    // Stop any playback
    extern AudioPlaySdWav playSdWav1;
    if (playSdWav1.isPlaying()) {
      playSdWav1.stop();
    }
    // Use direct SD playback like first preview in showWave (sample is already on SD)
    extern bool sampleIsLoaded, firstcheck;
    extern CachedSample previewCache;
    
    int fileID = SMP.wav[GLOB.currentChannel].fileID;
    int fnr = getFolderNumber(fileID);
    int fileNum = getFileNumber(fileID);
    
    // Invalidate cache and reset flags
    previewCache.valid = false;
    sampleIsLoaded = false;
    firstcheck = true;
    
    switchMode(&set_Wav);  // Switch to showWave mode
    
    // Use direct SD playback (same as first preview in showWave)
    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, fileNum);
    
    previewIsPlaying = true;
    playSdWav1.play(OUTPUTf);
    
    peakIndex = 0;
    memset(peakValues, 0, sizeof(peakValues));
    sampleIsLoaded = true;
  } else if (currentMode == &songMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000" - Encoder 0 pressed - remove assignment
    extern uint8_t songArrangement[64];
    int songPosition = songMode.pos[3];  // 1-64
    songArrangement[songPosition - 1] = 0;  // Clear assignment
    Serial.print("Song position ");
    Serial.print(songPosition);
    Serial.println(" cleared");
    return;
  } else if (currentMode == &songMode && (match_buttons(currentButtonStates, 0, 1, 0, 0) || match_buttons(currentButtonStates, 0, 0, 0, 1))) {  // "0100" or "0001" - Encoder 1 or 3 pressed
    // Save selected pattern to current song position
    extern uint8_t songArrangement[64];
    int songPosition = songMode.pos[3];  // 1-64
    int selectedPattern = songMode.pos[1];  // 1-16
    songArrangement[songPosition - 1] = selectedPattern;
    Serial.print("Song position ");
    Serial.print(songPosition);
    Serial.print(" set to pattern ");
    Serial.println(selectedPattern);
    return;
  } else if (currentMode == &songMode && match_buttons(currentButtonStates, 0, 0, 1, 0)) {  // "0010" - Encoder 2 pressed - Toggle play/pause + songModeActive
    extern bool songModeActive;
    if (isNowPlaying) {
      // Currently playing - pause and deactivate song mode
      pause();
      songModeActive = false;
      patternMode = -1;  // Set PMOD to OFF
      saveSingleModeToEEPROM(3, patternMode);
      Serial.println("Song playback paused, song mode deactivated");
    } else {
      // Not playing - activate song mode and start playback
      songModeActive = true;
      patternMode = 2;  // Set PMOD to SONG
      saveSingleModeToEEPROM(3, patternMode);
      Serial.println("Song playback started");
      play(true);
    }
    return;
  }
  // Removed: 0200 no longer triggers recording in SET_WAV mode

  if (currentMode == &noteShift && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    switchMode(&singleMode);
    GLOB.singleMode = true;
    preventPaintUnpaint = false;  // Reset flag when exiting noteShift mode
  }


  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    freshPaint = false;
    preventPaintUnpaint = false;  // Reset flag when freshPaint is cleared
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 2, 2, 0)) {  // "0220"
    switchMode(&volume_bpm);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 2, 0) && was_buttons_0000(oldButtons)) {  // "0020" - must be 0000 before
    if (GLOB.currentChannel == 0 || GLOB.currentChannel == 9 || GLOB.currentChannel == 10 || GLOB.currentChannel == 12 || GLOB.currentChannel == 15) return;

    // Only update currentChannel from y-position if in draw mode (not single mode)
    if (currentMode == &draw) {
      GLOB.currentChannel = GLOB.y - 1;
    }
    // In single mode, keep the existing GLOB.currentChannel

    fxType = 0;
      switchMode(&filterMode);
    return;
  }

  // Channel switching in single mode with button combination 0-1-0-0 when not at y=16
  if (currentMode == &singleMode && GLOB.y != 16 && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100" when not at y=16
    // Cycle through channels 1-8 (avoiding special channels 0, 9, 10, 12, 15)
    GLOB.currentChannel++;
    if (GLOB.currentChannel > 8) {
      GLOB.currentChannel = 1;
    }
    // Skip special channels
    if (GLOB.currentChannel == 9 || GLOB.currentChannel == 10 || GLOB.currentChannel == 12 || GLOB.currentChannel == 15) {
      GLOB.currentChannel = 1;
    }
    
    // Update encoder colors to reflect new channel
    Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    
    // Update channel volume encoder
    Encoder[2].writeCounter((int32_t)SMP.channelVol[GLOB.currentChannel]);
    currentMode->pos[2] = SMP.channelVol[GLOB.currentChannel];
    
    return;  // Prevent other button actions from being processed
  }

  if (currentMode == &draw && match_buttons(currentButtonStates, 1, 1, 0, 0)) {  // "1100"
    //toggleCopyPaste();
  }


  if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0 && (currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2) && was_buttons_0000(oldButtons)) {  // "0002" - must be 0000 before
    unsigned int velo = round(mapf(note[GLOB.x][GLOB.y].velocity, 1, 127, 1, maxY));
    GLOB.velocity = velo;
    
    // Map probability to encoder range 1-5 (0%, 25%, 50%, 75%, 100%)
    unsigned int prob = note[GLOB.x][GLOB.y].probability;
    unsigned int probStep;
    if (prob == 0) probStep = 1;
    else if (prob == 25) probStep = 2;
    else if (prob == 50) probStep = 3;
    else if (prob == 75) probStep = 4;
    else probStep = 5;  // 100%
    
    // Map condition to encoder range 1-9
    // Values 1-16: 1/X conditions -> positions 1-5
    // Values 17-20: X/1 conditions -> positions 6-9
    uint8_t cond = note[GLOB.x][GLOB.y].condition;
    if (cond == 0) cond = 1;  // Default to 1 if not set
    unsigned int condStep;
    if (cond <= 16) {
      condStep = (cond == 1) ? 1 : (cond == 2) ? 2 : (cond == 4) ? 3 : (cond == 8) ? 4 : 5;
    } else {
      condStep = (cond == 17) ? 6 : (cond == 18) ? 7 : (cond == 19) ? 8 : 9;
    }
    
    switchMode(&velocity);
    GLOB.singleMode = true;
    Encoder[0].writeCounter((int32_t)velo);
    Encoder[1].writeCounter((int32_t)probStep);
    Encoder[3].writeCounter((int32_t)condStep);
    currentMode->pos[1] = probStep;
    currentMode->pos[3] = condStep;
  }

  if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0 && (currentMode == &draw) && match_buttons(currentButtonStates, 0, 0, 0, 2) && was_buttons_0000(oldButtons)) {  // "0002" - must be 0000 before
    unsigned int velo = round(mapf(note[GLOB.x][GLOB.y].velocity, 1, 127, 1, maxY));
    unsigned int chvol = SMP.channelVol[GLOB.currentChannel];
    GLOB.velocity = velo;
    
    // Map probability to encoder range 1-5 (0%, 25%, 50%, 75%, 100%)
    unsigned int prob = note[GLOB.x][GLOB.y].probability;
    unsigned int probStep;
    if (prob == 0) probStep = 1;
    else if (prob == 25) probStep = 2;
    else if (prob == 50) probStep = 3;
    else if (prob == 75) probStep = 4;
    else probStep = 5;  // 100%
    
    // Map condition to encoder range 1-9
    // Values 1-16: 1/X conditions -> positions 1-5
    // Values 17-20: X/1 conditions -> positions 6-9
    uint8_t cond = note[GLOB.x][GLOB.y].condition;
    if (cond == 0) cond = 1;  // Default to 1 if not set
    unsigned int condStep;
    if (cond <= 16) {
      condStep = (cond == 1) ? 1 : (cond == 2) ? 2 : (cond == 4) ? 3 : (cond == 8) ? 4 : 5;
    } else {
      condStep = (cond == 17) ? 6 : (cond == 18) ? 7 : (cond == 19) ? 8 : 9;
    }
    
    GLOB.singleMode = false;
    switchMode(&velocity);
    Encoder[0].writeCounter((int32_t)velo);
    Encoder[1].writeCounter((int32_t)probStep);
    Encoder[2].writeCounter((int32_t)chvol);
    Encoder[3].writeCounter((int32_t)condStep);
    currentMode->pos[1] = probStep;
    currentMode->pos[3] = condStep;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 2, 0, 0, 2)) {  // "2002"
    clearPage();
    preventPaintUnpaint = true;  // Prevent paint/unpaint after deleteall
    return;  // Prevent other button actions from being processed
  }


  if (currentMode == &velocity && match_buttons(currentButtonStates, 0, 0, 0, 9)) {  // "0009"
    if (!GLOB.singleMode) {
      switchMode(&draw);
    } else {
      switchMode(&singleMode);
      GLOB.singleMode = true;
    }
  }


  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0010"
    toggleDefaultFilterFromSlider(filterPage[GLOB.currentChannel],3);
  }
  


  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 0, 2) && was_buttons_0000(oldButtons)) {  // "0002" - must be 0000 before
    setEnvelopeDefaultValues((unsigned int)GLOB.currentChannel);
    setFiltersDefaultValues((unsigned int)GLOB.currentChannel);
    setSynthDefaultValues((unsigned int)GLOB.currentChannel);
  }
  

  
  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 2, 0)) {  // "0010"
    if (!isNowPlaying) {
      play(true);
    } else {
      unsigned long currentTime = millis();
      if (currentTime - playStartTime > 1000) {  // Check if play started more than 200ms ago
        pause();
      }
    }
    return;
  }
  

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 1, 0)) {  // "0010"
    toggleDefaultFilterFromSlider(filterPage[GLOB.currentChannel],2);
  }



  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    toggleDefaultFilterFromSlider(filterPage[GLOB.currentChannel],1);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    toggleDefaultFilterFromSlider(filterPage[GLOB.currentChannel],0);
  }

  if (currentMode == &volume_bpm && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    drawBaseColorMode = !drawBaseColorMode;  // Toggle drawBASE color mode
  }

  if (currentMode == &volume_bpm && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    switchMode(&draw);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 9, 0, 9) && was_not_buttons_0000(oldButtons)) {  // "0909" - must be !=0000 before
    paintMode = false;
    unpaintMode = false;
    preventPaintUnpaint = false;  // Reset flag when paint/unpaint modes are cleared
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 9)) {  // "0009"
    freshPaint = false;
    paintMode = false;
    unpaintMode = false;
    preventPaintUnpaint = false;  // Reset flag when paint/unpaint modes are cleared
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 9, 0, 0, 0)) {  // "9000"
    unpaintMode = false;
    paintMode = false;
    preventPaintUnpaint = false;  // Reset flag when paint/unpaint modes are cleared
  }

  if ((currentMode == &draw) && match_buttons(currentButtonStates, 0, 0, 2, 2)) {  // "0022"
    //switchMode(&loadSaveTrack);
  }

  if ((currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 2, 2)) {  // "0022"
    //set loaded sample
  }

  //if ((currentMode == &draw) && match_buttons(currentButtonStates, 2, 2, 0, 0)) {  // "2200"
    //switchMode(&set_SamplePack);
  //}

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    // At y==1 in draw mode with CTRL==VOL: cycle input monitoring (off -> on -> all -> off)
    if (currentMode == &draw && GLOB.y == 1 && ctrlMode == 1) {
      inputMonitoringState = (inputMonitoringState + 1) % 3;  // Cycle: 0->1->2->0
      return;  // Skip mute toggle at y==1
    }
    
    bool wasMuted = getMuteState(GLOB.currentChannel);
    toggleMute();
    bool isMuted = getMuteState(GLOB.currentChannel);

    if (ctrlMode == 1 && wasMuted && !isMuted) {
      SMP.channelVol[GLOB.currentChannel] = DEFAULT_CHANNEL_VOLUME;
      float channelvolume = mapf(DEFAULT_CHANNEL_VOLUME, 0, maxY, 0, 1);
      // Check if amps[channel] exists before accessing (channel 0, 9, 10, 12 are nullptr)
      if (amps[GLOB.currentChannel] != nullptr) {
        amps[GLOB.currentChannel]->gain(channelvolume);
      }

      currentMode->pos[1] = DEFAULT_CHANNEL_VOLUME;
      Encoder[1].writeCounter((int32_t)DEFAULT_CHANNEL_VOLUME);
      ctrlLastVolume = DEFAULT_CHANNEL_VOLUME;

      if (GLOB.y != 16) {
        showCtrlVolumeChange(DEFAULT_CHANNEL_VOLUME);
        CRGB volColor = CRGB(DEFAULT_CHANNEL_VOLUME * DEFAULT_CHANNEL_VOLUME,
                             max(0, 20 - DEFAULT_CHANNEL_VOLUME), 0);
        Encoder[1].writeRGBCode(volColor.r << 16 | volColor.g << 8 | volColor.b);
        ctrlVolumeOverlayActive = true;
      } else {
        Encoder[1].writeRGBCode(currentMode->knobcolor[1]);
        ctrlVolumeOverlayActive = false;
      }

      setMuteState(GLOB.currentChannel, false);
      SMP.mute[GLOB.currentChannel] = false;
    }
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2) && was_buttons_0000(oldButtons)) {  // "0002" - must be 0000 before
    // Only activate paintMode if we're actually in draw/singleMode (safety check)
    if (currentMode == &draw || currentMode == &singleMode) {
    paintMode = true;
    preventPaintUnpaint = false;  // Reset flag when paintMode is activated
    }
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    // In single mode, trigger clear page function ONLY when y=16
    if (GLOB.singleMode && GLOB.y == 16) {
      clearPage();
      preventPaintUnpaint = true;  // Prevent paint/unpaint after deleteall
      return;  // Prevent other button actions from being processed
    }
    // In draw mode, no action (or could add different functionality if needed)
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 2, 0, 0, 0)) {  // "2000"
    // In single mode, trigger random function ONLY when y=16
    if (GLOB.singleMode && GLOB.y == 16) {
      drawRandoms();
      preventPaintUnpaint = true;  // Prevent paint/unpaint after random
      return;  // Prevent other button actions from being processed
    } else {
      // Normal unpaint functionality for draw mode
      unpaint();
      unpaintMode = true;
      deleteActiveCopy();
      preventPaintUnpaint = false;  // Reset flag after unpaint operation
    }
  }

  // Assuming '3' is a valid state that buttons[i] can take.
  // If not, this block is dead code.
  if (currentMode == &singleMode && match_buttons(currentButtonStates, 3, 0, 0, 0)) {  // "3000"
    GLOB.currentChannel = GLOB.y - 1;  // Set currentChannel based on Y position when exiting single mode
    switchMode(&draw);
  } else if (currentMode == &draw && match_buttons(currentButtonStates, 3, 0, 0, 0) && ((GLOB.currentChannel >= 1 && GLOB.currentChannel <= maxFiles) || GLOB.currentChannel > 12)) {  // "3000"
    GLOB.currentChannel = GLOB.currentChannel;
    switchMode(&singleMode);
    GLOB.singleMode = true;
  }

  if (reset) {
    // Reset states after handling
    // buttonString = "0000"; // REMOVED
    // oldButtonString = buttonString; // REMOVED
    // `oldButtons` will be updated by `checkButtons` after this reset is reflected in `buttons`

    for (int i = 0; i < NUM_ENCODERS; i++) {
      buttons[i] = 0;
      buttonState[i] = IDLE;
      isPressed[i] = false;
    }
    
    // Reset paint/unpaint prevention flag when buttons are released
    preventPaintUnpaint = false;
  }
}








void initSoundChip() {
  // AudioInterrupts();
  // Reduced from 256 to 128 blocks to prevent memory fragmentation
  // 256 blocks = 64KB, which can cause slow SD card access and unresponsiveness
  // 128 blocks = 32KB, still plenty for complex audio routing
  AudioMemory(64);
  // turn on the output
  sgtl5000_1.enable();
  
  // Load audio settings from globals (they should be loaded from EEPROM before this is called)
  // GLOB, micGain, lineInLevel, and lineOutLevelSetting are already in scope as globals
  
  // Apply initial volume (will be fully applied later via applyAudioSettingsFromGlobals)
  float vol = GLOB.vol / 10.0f;
  vol = constrain(vol, 0.0f, 1.0f);
  sgtl5000_1.volume(vol);

  // Initialize audio input amplifier - turned off at startup
  audioInputAmp.gain(0.0);

  //FOR GRANULAR
  //granular1.begin(granularMemory, GRANULAR_MEMORY_SIZE);

  //sgtl5000_1.autoVolumeControl(1, 1, 0, -6, 40, 20);
  //sgtl5000_1.audioPostProcessorEnable();
  //sgtl5000_1.audioPreProcessorEnable();
  //sgtl5000_1.audioPostProcessorEnable();
  //sgtl5000_1.autoVolumeEnable();

  //REC
  sgtl5000_1.inputSelect(recInput);
  sgtl5000_1.micGain(micGain);  //0-63
  //sgtl5000_1.adcHighPassFilterEnable();
  //sgtl5000_1.adcHighPassFilterDisable();  //for mic?
  sgtl5000_1.unmuteLineout();
  sgtl5000_1.lineOutLevel(lineOutLevelSetting);
  sgtl5000_1.lineInLevel(lineInLevel);  // Apply line input level
}


void initSamples() {
  for (unsigned int vx = 1; vx < SONG_LEN + 1; vx++) {
    for (unsigned int vy = 1; vy < maxY + 1; vy++) {
      note[vx][vy].velocity = defaultVelocity;
    }
  }
  //preview
  _samplers[0].addVoice(sound0, mixer0, 1, envelope0);
  //voice 1-
  _samplers[1].addVoice(sound1, mixer1, 0, envelope1);
  _samplers[2].addVoice(sound2, mixer1, 1, envelope2);
  _samplers[3].addVoice(sound3, mixer1, 2, envelope3);
  _samplers[4].addVoice(sound4, mixer1, 3, envelope4);

  _samplers[5].addVoice(sound5, mixer2, 0, envelope5);
  _samplers[6].addVoice(sound6, mixer2, 1, envelope6);
  _samplers[7].addVoice(sound7, mixer2, 2, envelope7);
  _samplers[8].addVoice(sound8, mixer2, 3, envelope8);



  // Reduced synth gains to prevent clipping (was 0.4 for 11, 0.15 for 13/14)
  synthmixer11.gain(0, 0.12);  // Reduced from GAIN_3 (0.4) to prevent clipping
  synthmixer11.gain(1, 0.12);
  synthmixer11.gain(3, 0.12);

  synthmixer13.gain(0, 0.10);  // Further reduced from 0.15 to prevent clipping
  synthmixer13.gain(1, 0.10);
  synthmixer13.gain(3, 0.10);

  synthmixer14.gain(0, 0.10);  // Further reduced from 0.15 to prevent clipping
  synthmixer14.gain(1, 0.10);
  synthmixer14.gain(3, 0.10);


  // mixer0.gain(0, ...) and mixer0.gain(1, ...) will be set by updatePreviewVolume() based on PREV volume


  mixersynth_end.gain(0, GAIN_4);
  mixersynth_end.gain(2, GAIN_4);
  mixersynth_end.gain(3, GAIN_4);

  

  mixer1.gain(0, GAIN_4);
  mixer1.gain(1, GAIN_4);
  mixer1.gain(2, GAIN_4);
  mixer1.gain(3, GAIN_4);

  mixer2.gain(0, GAIN_4);
  mixer2.gain(1, GAIN_4);
  mixer2.gain(2, GAIN_4);
  mixer2.gain(3, GAIN_4);


  mixer_end.gain(0, GAIN_2);
  mixer_end.gain(1, GAIN_2);
  mixer_end.gain(2, GAIN_2);
  mixer_end.gain(3, 0.0);  // No monitoring by default - ensure OFF on startup


  mixerPlay.gain(0, GAIN_2);
  mixerPlay.gain(1, GAIN_2);
  

  // Initialize the array with nullptrs
  synths[11][0] = &waveform11_1;
  synths[11][1] = &waveform11_2;

  synths[13][0] = &waveform13_1;
  synths[13][1] = &waveform13_2;

  synths[14][0] = &waveform14_1;
  synths[14][1] = &waveform14_2;

  short waveformType[15][2] = { { 0 } };  // zero-initialize unused elements
  // Define your actual waveforms at relevant indices

  waveformType[11][0] = WAVEFORM_PULSE;
  waveformType[11][1] = WAVEFORM_SAWTOOTH;

  waveformType[13][0] = WAVEFORM_PULSE;
  waveformType[13][1] = WAVEFORM_SAWTOOTH;

  waveformType[14][0] = WAVEFORM_PULSE;
  waveformType[14][1] = WAVEFORM_SAWTOOTH;

  float amplitude[15][2] = { { 0.0f } };

  amplitude[11][0] = 0.3f;
  amplitude[11][1] = 0.3f;
  // Set amplitude values similarly

  amplitude[13][0] = 0.1125f;  // Reduced to half of current (0.225 * 0.5 = 0.1125)
  amplitude[13][1] = 0.1125f;

  amplitude[14][0] = 0.1125f;  // Reduced to half of current (0.225 * 0.5 = 0.1125)
  amplitude[14][1] = 0.1125f;



  // Initialize your waveforms in a loop safely:
  for (int pairIndex = 11; pairIndex <= 14; pairIndex++) {
    for (int synthIndex = 0; synthIndex < 2; synthIndex++) {
      if (synths[pairIndex][synthIndex] != nullptr) {
        synths[pairIndex][synthIndex]->begin(waveformType[pairIndex][synthIndex]);
        synths[pairIndex][synthIndex]->amplitude(amplitude[pairIndex][synthIndex]);
        synths[pairIndex][synthIndex]->phase(0);
      }
    }
  }

  //chiptune_synth(0,0,0);
  for (int i = 0; i < 3; i++) {
    Sdc1[i]->amplitude(1.0);
  }

  // set filters and envelopes for all sounds
  for (unsigned int i = 1; i < 9; i++) {
    if (filters[i]) {  // Check if filter exists
      filters[i]->octaveControl(6.0);
      filters[i]->resonance(0.7);
      filters[i]->frequency(8000);
    }
  }

  for (unsigned int i = 0; i < 9; i++) {
    ////Serial.print("Enabling channel/voice:");
    ////Serial.println(i);
    if (voices[i]) {  // Check if voice exists
      voices[i]->enableInterpolation(true);
    }
  }


initSliderDefTemplates();
for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
    setSliderDefForChannel(ALL_CHANNELS[i]);
    setFilterDefaults(i);
}
  
  /*
 // Example: reset all channels
 // for (int ch = 1; ch <= 8; ch++) {
 //   setEnvelopeDefaultValues((unsigned int)ch);
 //   setFiltersDefaultValues((unsigned int)ch);
 //   setSynthDefaultValues((unsigned int)ch);
 // }
 */

}


void checkCrashReport() {
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  if (CrashReport) {  // Check if CrashReport is non-empty before printing
    Serial.print(CrashReport);
  }
  delay(1000);
  //Serial.println("clearing EEPROM and autosaved.txt...");
  for (unsigned int i = 0; i < EEPROM.length(); i++) EEPROM.write(i, 0);
  char OUTPUTf[50];
  sprintf(OUTPUTf, "autosaved.txt");
  if (SD.exists(OUTPUTf)) {
    SD.remove(OUTPUTf);
  }
  delay(2000);
  //Serial.println("clearing completed.");
}



void initEncoders() {
  //Serial.println("I2C Encoder init...");
  for (int i = 0; i < NUM_ENCODERS; i++) {
    // Set the global encoder index for callbacks
    currentEncoderIndex = i;
    Encoder[i].reset();
    uint16_t direction_flag;  // Renamed to avoid conflict
    if (i > 0) {              // Assuming Encoder[0] is different direction
      direction_flag = i2cEncoderLibV2::DIRE_RIGHT;
    } else {
      direction_flag = i2cEncoderLibV2::DIRE_LEFT;
    }
    Encoder[i].begin(
      i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
      | direction_flag | i2cEncoderLibV2::IPUP_ENABLE  // Use renamed variable
      | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);

    Encoder[0].writeDoublePushPeriod(0);
    Encoder[1].writeDoublePushPeriod(0);
    Encoder[2].writeDoublePushPeriod(0);
    Encoder[3].writeDoublePushPeriod(0);

    Encoder[i].writeAntibouncingPeriod(10);  //10?
    Encoder[i].writeCounter((int32_t)1);
    Encoder[i].writeMax((int32_t)maxX * 4);  //maxval scaled to display width
    Encoder[i].writeMin((int32_t)1);       //minval
    Encoder[i].writeStep((int32_t)1);      //steps

    // Assign static callback wrappers
    Encoder[i].onButtonPush = staticButtonPushed;
    Encoder[i].onButtonRelease = staticButtonReleased;


    Encoder[i].onMinMax = staticThresholds;
    Encoder[i].autoconfigInterrupt();
    //Encoder[i].writeRGBCode(0xFFFFFF);
    Encoder[i].writeFadeRGB(0);
    delay(50);
    Encoder[i].updateStatus();
  }
}

void setup() {
  //Wire.begin();
  //Wire.setClock(10000); // Set to 400 kHz (standard speed)
  NVIC_SET_PRIORITY(IRQ_SOFTWARE, 208);
  // Set MIDI (Serial8/LPUART8) interrupt priority - lower number = higher priority
  // Priority 112 = higher priority than software (208) for faster MIDI handling
  NVIC_SET_PRIORITY(IRQ_LPUART8, 112);  // MIDI serial interrupt priority
  //NVIC_SET_PRIORITY(IRQ_USB1, 128);  // USB1 for Teensy 4.x
  Serial.begin(115200);
  EEPROM.get(0, samplePackID);
  if (isnan(samplePackID) || samplePackID == 0 || samplePackID < 1) {  // Check for NaN, zero, or invalid values
    Serial.print("NO SAMPLEPACK SET OR INVALID VALUE! Defaulting to 1");
    samplePackID = 1;
    EEPROM.put(0, samplePackID);  // Save the default to EEPROM
  }
  
  // Synchronize SMP.pack with the loaded samplePackID
  SMP.pack = samplePackID;

  pinMode(INT_PIN, INPUT_PULLUP);  // Interrups for encoder
  pinMode(0, INPUT_PULLDOWN);
  pinMode(SWITCH_1, INPUT_PULLDOWN);  // Use defined name
  pinMode(SWITCH_2, INPUT_PULLDOWN);  // Use defined name
  pinMode(SWITCH_3, INPUT_PULLDOWN);  // Use defined name

  pinMode(2, INPUT_PULLUP);  // Pin 2 as input with pull-up
  pinMode(4, OUTPUT);        // Pin 4 set as output
  digitalWrite(4, LOW);      // Drive Pin 4 LOW

  FastLED.addLeds<WS2812SERIAL, DATA_PIN, BRG>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);

  // Early EEPROM load for audio settings (before initSoundChip)
  // Load GLOB.vol, micGain, previewVol, lineInLevel, lineOutLevelSetting directly from EEPROM
  // GLOB, micGain, previewVol, lineInLevel, and lineOutLevelSetting are already in scope as globals
  
  // Load GLOB.vol from EEPROM (stored at EEPROM_DATA_START + 17, separate from transportMode)
  GLOB.vol = (int8_t)EEPROM.read(EEPROM_DATA_START + 17);
  if (GLOB.vol < 0 || GLOB.vol > 10) {
    GLOB.vol = 10;  // Default to 10 if invalid
  }
  
  // Load micGain from EEPROM
  micGain = (unsigned int)EEPROM.read(EEPROM_DATA_START + 9);
  if (micGain > 63) {
    micGain = 10;  // Default to 10 if invalid
  }
  
  // Load previewVol from EEPROM
  previewVol = (unsigned int)EEPROM.read(EEPROM_DATA_START + 7);
  if (previewVol > 50) {
    previewVol = 20;  // Default to mid if invalid
  }
  
  // Load lineInLevel from EEPROM
  lineInLevel = (unsigned int)EEPROM.read(EEPROM_DATA_START + 16);
  if (lineInLevel > 15) {
    lineInLevel = 8;  // Default to 8 if invalid
  }
  
  // Load lineOutLevelSetting from EEPROM
  lineOutLevelSetting = (uint8_t)EEPROM.read(EEPROM_DATA_START + 15);
  if (lineOutLevelSetting < LINEOUT_MIN || lineOutLevelSetting > LINEOUT_MAX) {
    lineOutLevelSetting = 30;  // Default to 30 if invalid
  }
  
  initSoundChip();
  
  initEncoders();  // Moved initEncoders here, ensures Serial is up for its prints
  if (CrashReport) {  // This implicitly calls operator bool() or similar if defined by CrashReportClass
    checkCrashReport();
  }
  drawNoSD();
  delay(500);
  
  mixer0.gain(1, 0.05);  //PREV Sound
  
  // Check if touch2 (menu button) is pressed during startup for INIT mode
  int touch2Value = fastTouchRead(SWITCH_2);
  if (touch2Value > touchThreshold) {
    // INIT MODE: Show hourglass, version, write reset flag file, and enter endless loop
    FastLEDclear();
    showIcons(ICON_HOURGLASS, CRGB(100, 100, 0));  // Yellow hourglass
    drawText(VERSION, 3, 1, CRGB(0, 100, 100));    // Cyan version at (1,1)
    FastLEDshow();
    
    // Write reset flag file
    File resetFile = SD.open("reset.dat", FILE_WRITE);
    if (resetFile) {
      resetFile.println("RESET");
      resetFile.close();
    }
    
    // Endless loop with delay
    while (true) {
      delay(500);
      yield();  // Allow system to handle background tasks
    }
  }
  
  playSdWav1.play("intro/016.wav");

  runAnimation();
  playSdWav1.stop();
  EEPROMgetLastFiles();
  loadMenuFromEEPROM();
  
  // Apply all audio settings from globals (after loadMenuFromEEPROM which may have updated values)
  extern void applyAudioSettingsFromGlobals();
  applyAudioSettingsFromGlobals();
  
  loadSp0StateFromEEPROM();  // Load samplepack 0 state before loading samplepack
  loadSamplePack(samplePackID, true);
  SMP.bpm = 100.0;
  
  // Initialize GLOB with default runtime values (but preserve vol from EEPROM)
  int savedVol = GLOB.vol;
  initGlobalVars();
  GLOB.vol = savedVol;  // Restore vol from EEPROM
  
  initSamples();
  initPageMutes();  // Initialize per-page mute system
  
  //playTimer.priority(118);
  playTimer.begin(playNote, playNoteInterval);
  //midiTimer.begin(checkMidi, playNoteInterval);
  //midiTimer.priority(10);

  autoLoad();
  
  // Initialize probability and condition fields for all existing notes (default 100% probability, condition 1)
  for (unsigned int x = 0; x <= maxlen; x++) {
    for (unsigned int y = 0; y <= maxY; y++) {
      if (note[x][y].probability == 0 && note[x][y].channel != 0) {
        // If probability is 0 but note exists, set to 100% (for backward compatibility)
        note[x][y].probability = 100;
      } else if (note[x][y].channel != 0 && note[x][y].probability == 0) {
        note[x][y].probability = 100;
      }
      // Initialize condition to 1 if not set (for backward compatibility)
      if (note[x][y].channel != 0 && note[x][y].condition == 0) {
        note[x][y].condition = 1;
    }
  }
  }
  // Note probabilities and conditions initialized (debug removed)
  
  // Check for reset flag file and trigger startNew() if it exists
  if (SD.exists("reset.dat")) {
    SD.remove("reset.dat");  // Delete the flag file
    Serial.println("Reset flag file detected - triggering startNew()");
    startNew();  // Trigger factory reset
  }
  
  updateSynthVoice(11);
  switchMode(&draw);

  Serial8.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);  // optional MIDI library hook
  MIDI.setHandleClock(handleMidiClock);  // dedicated callback for MIDI clock to reduce jitter
  resetMidiClockState();
  

  for (int f = 0; f < 5; f++) {
    for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
        setFilters((FilterType)f, ALL_CHANNELS[i], true);
    }
  }
  
  forceAllMixerGainsToTarget();
  
  // Turn on audio input amplifier after setup is complete
  audioInputAmp.gain(1.0);
  
  // Final application of audio settings to ensure all are applied after all globals are loaded/initialized
  extern void applyAudioSettingsFromGlobals();
  applyAudioSettingsFromGlobals();
  
// END SETUP
}


void initGlobalVars() {
  GLOB.singleMode = false;
  GLOB.currentChannel = 1;
  GLOB.vol = 10;
  GLOB.velocity = 10;
  GLOB.copyChannel = 0;
  GLOB.page = 1;
  GLOB.edit = 1;
  GLOB.folder = 0;
  GLOB.activeCopy = false;
  GLOB.x = 1;
  GLOB.y = 16;
  GLOB.seek = 0;
  GLOB.seekEnd = 0;
  GLOB.smplen = 0;
  GLOB.shiftX = 0;
  GLOB.shiftY = 0;
  GLOB.shiftY1 = 0;
  GLOB.subpattern = 0;

  for (int i = 0; i < maxFiles; ++i) {
    channelDirection[i] = 1;
  }
}

void setEncoderColor(int i) {
  Encoder[i].writeRGBCode(0x00FF00);
}


void checkEncoders() {
  // Track mode changes globally for this function to detect re-entry into Draw/Single modes
  static Mode* lastSeenMode = nullptr;
  bool modeChangedGlobal = (currentMode != lastSeenMode);
  lastSeenMode = currentMode;

  // Calculate hash of encoder positions (replaces String for memory efficiency)
  uint32_t posHash = 0;

  for (int i = 0; i < NUM_ENCODERS; i++) {
    currentEncoderIndex = i;  // Ensure this is set before calling encoder methods or callbacks that might use it implicitly
    Encoder[i].updateStatus();
    int rawValue = Encoder[i].readCounterInt();

    if (currentMode == &subpatternMode && muteModeActive && i == 2) {
      if (rawValue != 0) {
        bool turnedRight = rawValue > 0;
        int8_t targetDir = turnedRight ? 1 : -1;
        muteModeArrowDirection = targetDir;
        muteModeArrowUntil = millis() + 250;
        muteModeEncoderValue = turnedRight ? 1 : 0;
        Encoder[2].writeCounter((int32_t)0);
        rawValue = 0;
        uint8_t soloChannel = static_cast<uint8_t>(GLOB.currentChannel);
        if (soloChannel < maxFiles) {
          applyChannelDirection(soloChannel, targetDir);
        }
      }
      currentMode->pos[2] = muteModeEncoderValue;
    } else {
      currentMode->pos[i] = rawValue;
    }

    // Build hash from encoder positions (skip encoder 2)
    if (i != 2) {
      posHash = (posHash << 10) | (currentMode->pos[i] & 0x3FF);  // 10 bits per encoder (0-1023 range)
    }

    // Ensure buttons[i] is reset before handle_button_state if it's not sticky
    // or handle_button_state should ensure it's 0 if no event.
    // Assuming handle_button_state correctly sets buttons[i] to 0 if no press/release event is active.
    // If not, buttons[i] might need a `buttons[i] = 0;` before `handle_button_state` if it's meant to be non-sticky.
    // Based on `handle_button_state`, it seems to set `buttons[i]` on events, but might not reset it to 0 otherwise unless `!isPressed` and `buttonState == IDLE`.
    // Let's ensure buttons[i] is reset if it's a release event that's been processed by checkMode
    if (buttons[i] == 1 || buttons[i] == 9) {  // If it was a release, clear it before new evaluation
                                               // This might be better placed in checkMode's reset or after processing in checkButtons
    } else {
      buttons[i] = 0;  // Default to no event before checking
    }

    handle_button_state(&Encoder[i], i);  // This updates buttons[i] based on new physical state
    // buttonString += String(buttons[i]); // REMOVED
  }

  // Handle subpattern mode encoder[1] changes and auto-exit on release
  if (currentMode == &subpatternMode) {
    if (muteModeActive) {
      if (buttons[2] == 9) {  // Button release event on encoder 2
        muteModeEncoderValue = 0;
        currentMode->pos[2] = 0;
        Encoder[2].writeCounter((int32_t)0);
      }
      if (buttons[1] == 9) {  // Button release event
        tmpMuteAll(false);
        tmpMute = false;
        muteModeActive = false;
        Mode *returnMode = muteModeReturn;
        bool restoreSingleState = muteModeReturnSingleState;
        muteModeReturn = nullptr;
        muteModeLastChannel = -1;
        muteModeReturnSingleState = false;
        muteModeArrowDirection = 0;
        muteModeArrowUntil = 0;
        muteModeEncoderValue = 0;
        currentMode->pos[2] = 0;
        Encoder[2].writeCounter((int32_t)0);
        if (returnMode) {
          switchMode(returnMode);
        } else {
          switchMode(&draw);
        }
        GLOB.singleMode = restoreSingleState;
      }
    } else {
      GLOB.subpattern = currentMode->pos[1];  // Direct mapping: encoder 0-7 = subpattern 0-7
      if (GLOB.subpattern > 7) GLOB.subpattern = 7;
      if (GLOB.subpattern < 0) GLOB.subpattern = 0;
      
      // Auto-exit subpattern mode when encoder[1] button is released
      if (buttons[1] == 9) {  // Button release event
        if (GLOB.singleMode) {
          switchMode(&singleMode);
        } else {
          switchMode(&draw);
        }
      }
    }
  }

  if (currentMode == &draw || currentMode == &singleMode) {
    if (posHash != oldPosHash) {
      oldPosHash = posHash;
      static int lastY = -1;
      int maxStepsRuntime = (int)MAX_STEPS;  // total grid length, independent of modules
      currentMode->pos[3] = constrain(currentMode->pos[3], 1, maxStepsRuntime);
      Encoder[3].writeMax((int32_t)maxStepsRuntime);
      GLOB.x = currentMode->pos[3];
      GLOB.y = currentMode->pos[0];
      
      // Check y change after updating GLOB.y
      bool yChanged = (GLOB.y != lastY);
      lastY = GLOB.y;
      
      //filterDrawActive = false;
      if (currentMode == &draw) {
        GLOB.currentChannel = GLOB.y - 1;
        
        // Show channel number overlay when y changes (if enabled and channel is valid)
        if (yChanged && showChannelNr) {
          // Valid channels: 1-8 only (y=2-9 maps to channels 1-8)
          // GLOB.currentChannel = GLOB.y - 1 (0-indexed: y=2->1, y=3->2, ..., y=9->8)
          // Display number = GLOB.currentChannel (y=2->channel 1->display "1", y=3->channel 2->display "2", etc.)
          int channelNum = GLOB.currentChannel; // Display number equals currentChannel (1-indexed display)
          if (channelNum >= 1 && channelNum <= 8) {
            channelNrOverlayChannel = channelNum;
            channelNrOverlayActive = true;
            channelNrOverlayUntil = millis() + 800; // Show for 800ms
          }
        }
        FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
        int page, slot;
        if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
          int val = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);
          Encoder[2].writeCounter((int32_t)val);
          // Reset the last encoder value tracking when channel changes
          static int16_t lastEncVal[NUM_CHANNELS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};  // Changed from int to int16_t
          lastEncVal[GLOB.currentChannel] = val;  // Sync tracking with encoder
        }
        filterfreshsetted = true;
      }

      //if (currentMode == &singleMode){
       // drawText(pianoNoteNames[12 - GLOB.y + 1  + GLOB.currentChannel], 3, 5, CRGB(200, 50, 0));
      //}

      Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      
      // Only update edit page from X position if NOT in song mode
      extern bool songModeActive;
      if (!songModeActive) {
        GLOB.edit = getPage(GLOB.x);
      }
    }

    // --- update value from encoder2 in draw mode for all SettingArray types ---
    if (currentMode == &draw) {
      FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
      int page, slot;
      if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
        // Only check when encoder value actually changes to avoid excessive I2C reads/writes
        static int16_t lastEncVal[NUM_CHANNELS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};  // Changed from int to int16_t
        int encVal = currentMode->pos[2];
        
        // Only process if encoder value changed for this channel
        if (encVal != lastEncVal[GLOB.currentChannel]) {
          lastEncVal[GLOB.currentChannel] = encVal;
          
          // Only update if the stored value is different
        if (getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx) != encVal) {
          setDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx, encVal);
          
          // Apply the filter changes like in filtermode page
          switch (dft.arr) {
            case ARR_FILTER:
              setFilters(dft.idx, GLOB.currentChannel, false);
              break;
            case ARR_SYNTH:
              // Apply synth settings changes for channel 11
              if (GLOB.currentChannel == 11) {
                updateSynthVoice(11);
              }
              break;
            case ARR_PARAM:
              setParams(dft.idx, GLOB.currentChannel);
              break;
            default:
              break;
            }
          }
        }
      }
    }

    // Recovery mechanism: Auto-reset stuck paintMode/unpaintMode after timeout
    static unsigned long paintModeSetTime = 0;
    static unsigned long unpaintModeSetTime = 0;
    static bool lastPaintMode = false;
    static bool lastUnpaintMode = false;
    unsigned long now = millis();
    
    if (paintMode != lastPaintMode) {
      paintModeSetTime = now;
      lastPaintMode = paintMode;
    } else if (paintMode && (now - paintModeSetTime > 30000)) {
      // paintMode has been active for 30+ seconds - likely stuck, force reset
      paintMode = false;
      paintModeSetTime = 0;
    }
    
    if (unpaintMode != lastUnpaintMode) {
      unpaintModeSetTime = now;
      lastUnpaintMode = unpaintMode;
    } else if (unpaintMode && (now - unpaintModeSetTime > 30000)) {
      // unpaintMode has been active for 30+ seconds - likely stuck, force reset
      unpaintMode = false;
      unpaintModeSetTime = 0;
    }

    if ((GLOB.y > 1 && GLOB.y <= 15)) {  // Allow paint/unpaint up to row 15; row 16 reserved for UI
      if (paintMode && !preventPaintUnpaint) {
        // Only set probability to 100% if slot was empty (preserve existing probability)
        if (note[GLOB.x][GLOB.y].channel == 0) {
          note[GLOB.x][GLOB.y].probability = 100;  // Default 100% probability for new notes
          note[GLOB.x][GLOB.y].condition = 1;      // Default condition: 1 (every loop)
        }
        note[GLOB.x][GLOB.y].channel = GLOB.currentChannel;  // GLOB.currentChannel is 0-based
        note[GLOB.x][GLOB.y].velocity = defaultVelocity;
      }
      // Safety check: Only allow paintMode when actually in singleMode
      if (paintMode && currentMode == &singleMode && !preventPaintUnpaint) {
        // Only set probability to 100% if slot was empty (preserve existing probability)
        if (note[GLOB.x][GLOB.y].channel == 0) {
          note[GLOB.x][GLOB.y].probability = 100;  // Default 100% probability for new notes
          note[GLOB.x][GLOB.y].condition = 1;      // Default condition: 1 (every loop)
        }
        note[GLOB.x][GLOB.y].channel = GLOB.currentChannel;
        note[GLOB.x][GLOB.y].velocity = defaultVelocity;
      }


      // Safety check: Only allow unpaintMode when actually in draw/singleMode
      if (unpaintMode && (currentMode == &draw || currentMode == &singleMode) && !preventPaintUnpaint) {
        if (GLOB.singleMode) {
          if (note[GLOB.x][GLOB.y].channel == GLOB.currentChannel) {
            note[GLOB.x][GLOB.y].channel = 0;
            note[GLOB.x][GLOB.y].velocity = defaultVelocity;
            updateLastPage();
            // Update encoder 1 limit if pattern mode is ON or if in single mode
            if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
              Encoder[1].writeMax((int32_t)lastPage);
            }
          }
        } else {
          note[GLOB.x][GLOB.y].channel = 0;
          note[GLOB.x][GLOB.y].velocity = defaultVelocity;
          updateLastPage();
          // Update encoder 1 limit if pattern mode is ON or if in single mode
          if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
            Encoder[1].writeMax((int32_t)lastPage);
          }
        }
      }
    }


    // Only allow encoder[1] page changes if NOT in song mode (song controls pages automatically)
    extern bool songModeActive;
    if (ctrlMode == 0 && currentMode->pos[1] != editpage && !songModeActive) {
      updateLastPage();
      editpage = currentMode->pos[1];
      //Serial.println("p:" + String(editpage));
      int xval = mapXtoPageOffset(GLOB.x) + ((editpage - 1) * maxX);  // Use maxX instead of hardcoded 16
      Encoder[3].writeCounter((int32_t)xval);
      GLOB.x = xval;
      
      // Handle mute system when page changes in PMOD mode
      if (SMP_PATTERN_MODE) {
        // Save current page mutes before changing page
        for (int ch = 0; ch < maxY; ch++) {
          pageMutes[GLOB.edit - 1][ch] = getMuteState(ch);
        }
        // Load mutes for the new page
        GLOB.edit = editpage;
        for (int ch = 0; ch < maxY; ch++) {
          // The getMuteState function will now use the new page
        }
        patternChangeTime = millis() + 2000;  // 2 seconds window
        patternChangeActive = true;
      } else {
        GLOB.edit = editpage;
      }
    }


    if (ctrlMode == 1) {
      static int lastY = -1;
      
      bool yChanged = (GLOB.y != lastY);
      
      lastY = GLOB.y;

      // Special handling for draw mode at y==1: control input gain instead of channel volume
      if (currentMode == &draw && GLOB.y == 1) {
        // Only enable encoder(1) when monitoring is ON or ALL
        if (inputMonitoringState == 0) {
          // Monitoring is OFF: disable encoder(1) - don't process changes
          // Reset encoder to prevent drift
          static int lastDisabledPos = -1;
          extern int recMode;
          int currentGain = (recMode == 1) ? (int)micGain : (int)lineInLevel;
          if (lastDisabledPos != currentGain || currentMode->pos[1] != currentGain) {
            currentMode->pos[1] = currentGain;
            Encoder[1].writeCounter((int32_t)currentGain);
            lastDisabledPos = currentGain;
          }
          // Set encoder to black/disabled color
          Encoder[1].writeRGBCode(0x000000);
          // Skip all encoder(1) processing when monitoring is off - continue to next encoder
        } else {
          // Monitoring is ON: process encoder(1) normally
          ctrlLastChannel = -1; // Force volume control re-init when leaving y=1
        extern int recMode;
        static int lastInputGain = -1;
        static bool inputGainFirstEnter = true;
        
        // Reset first enter flag when y changes or mode changes
        if (yChanged || modeChangedGlobal) {
          inputGainFirstEnter = true;
          inputGainOverlayActive = false;  // Reset overlay when y changes
        }
        
        // Enforce limits (in case they were reset by other functions like play/pause)
        int targetMax = (recMode == 1) ? 63 : 15;
        if (currentMode->maxValues[1] != targetMax) {
            Encoder[1].writeMax((int32_t)targetMax);
            Encoder[1].writeMin((int32_t)0);
            currentMode->maxValues[1] = targetMax;
            currentMode->minValues[1] = 0;
            
            // Limits were reset, meaning the encoder value was likely reset too.
            // Force re-initialization to restore correct gain value.
            inputGainFirstEnter = true;
        }

        if (inputGainFirstEnter) {
          // Initialize encoder based on current input type
          if (recMode == 1) {
            // Mic input: use micGain (0-63)
            Encoder[1].writeCounter((int32_t)micGain);
            // Limits handled by enforcement above
            currentMode->pos[1] = micGain;
          } else {
            // Line input: use lineInLevel (0-15)
            Encoder[1].writeCounter((int32_t)lineInLevel);
            // Limits handled by enforcement above
            currentMode->pos[1] = lineInLevel;
          }
          lastInputGain = currentMode->pos[1];
          inputGainFirstEnter = false;
          
          // Set encoder color (red for mic, blue for line) on entry
          CRGB gainColor = (recMode == 1) ? CRGB(55, 0, 0) : CRGB(0, 0, 55);
          Encoder[1].writeRGBCode(gainColor.r << 16 | gainColor.g << 8 | gainColor.b);
          
          // Always show overlay when entering y==1
          showInputGainOverlay(currentMode->pos[1], targetMax);
        }
        
        // Handle encoder changes
        if (currentMode->pos[1] != lastInputGain) {
          int newGain = constrain((int)currentMode->pos[1], 0, targetMax);
          
          if (recMode == 1) {
            // Mic input
            micGain = (unsigned int)newGain;
            sgtl5000_1.micGain(micGain);
            // Update monitoring gain (match loudest playback: amps×mixer1/2×mixer_end = 1.0×GAIN_4×GAIN_2 = 0.16)
            extern AudioMixer4 mixer_end;
            float maxPlaybackGain = GAIN_4 * GAIN_2;  // 0.4 × 0.4 = 0.16
            float monitorGain = mapf((float)micGain, 0.0f, 63.0f, 0.0f, maxPlaybackGain);
            mixer_end.gain(3, monitorGain);
          } else {
            // Line input
            lineInLevel = (unsigned int)newGain;
            sgtl5000_1.lineInLevel(lineInLevel);
            // Update monitoring gain (match loudest playback: amps×mixer1/2×mixer_end = 1.0×GAIN_4×GAIN_2 = 0.16)
            extern AudioMixer4 mixer_end;
            float maxPlaybackGain = GAIN_4 * GAIN_2;  // 0.4 × 0.4 = 0.16
            float monitorGain = mapf((float)lineInLevel, 0.0f, 15.0f, 0.0f, maxPlaybackGain);
            mixer_end.gain(3, monitorGain);
          }
          
          lastInputGain = newGain;
          Encoder[1].writeCounter((int32_t)newGain);
          
          // Set encoder color (red for mic, blue for line)
          CRGB gainColor = (recMode == 1) ? CRGB(55, 0, 0) : CRGB(0, 0, 55);
          Encoder[1].writeRGBCode(gainColor.r << 16 | gainColor.g << 8 | gainColor.b);
          
          // Show overlay
          showInputGainOverlay(newGain, targetMax);
        }
        }  // End of monitoring ON else block
      } else if (GLOB.y == 16) {
        int channelVol = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);
        currentMode->pos[1] = channelVol;
        Encoder[1].writeCounter((int32_t)channelVol);
        Encoder[1].writeRGBCode(currentMode->knobcolor[1]);
        ctrlVolumeOverlayActive = false;
        ctrlLastChannel = -1;
        ctrlLastVolume = channelVol;
      } else {
        if (ctrlLastChannel != (int)GLOB.currentChannel) {
          ctrlLastChannel = GLOB.currentChannel;
          int channelVol = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);
          currentMode->pos[1] = channelVol;
          Encoder[1].writeCounter((int32_t)channelVol);
          ctrlLastVolume = channelVol;
        }

        int requestedVol = constrain((int)currentMode->pos[1], 0, 16);
        int actualVol = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);
        bool protectedChannel = (GLOB.currentChannel == 0 || GLOB.currentChannel == 9 ||
                                 GLOB.currentChannel == 10 || GLOB.currentChannel == 12);
        int prevVolume = ctrlLastVolume;

        if (protectedChannel) {
          requestedVol = actualVol;
          if (currentMode->pos[1] != actualVol) {
            currentMode->pos[1] = actualVol;
            Encoder[1].writeCounter((int32_t)actualVol);
          }
        }

        if (requestedVol != currentMode->pos[1]) {
          currentMode->pos[1] = requestedVol;
          Encoder[1].writeCounter((int32_t)requestedVol);
        }

        CRGB volColor = CRGB(requestedVol * requestedVol, max(0, 20 - requestedVol), 0);

        bool volumeChanged = (requestedVol != prevVolume);

        if (volumeChanged) {
          ctrlLastVolume = requestedVol;
          if (!protectedChannel) {
            SMP.channelVol[GLOB.currentChannel] = requestedVol;
            float channelvolume = mapf(SMP.channelVol[GLOB.currentChannel], 0, maxY, 0, 1);
            amps[GLOB.currentChannel]->gain(channelvolume);
            showCtrlVolumeChange(requestedVol);
          } else {
            showCtrlVolumeChange(actualVol);
          }
        }

        Encoder[1].writeRGBCode(volColor.r << 16 | volColor.g << 8 | volColor.b);

        if (volumeChanged && GLOB.currentChannel >= 0 && GLOB.currentChannel < maxY) {
          int effectiveVol = protectedChannel ? actualVol : requestedVol;
          bool shouldMute = (effectiveVol == 0);
          setMuteState(GLOB.currentChannel, shouldMute);
          SMP.mute[GLOB.currentChannel] = shouldMute;
        }
      }
    }
    
    filtercheck();

  

  if (filterDrawActive) {
      //Serial.println("active");
      if (millis() <= filterDrawEndTime) {
        
        FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
        int page, slot;
        if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
          int val = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);
          drawFilterCheck(val, dft.idx, filterColors[page][slot]);
        }
      } else {
        // Stop drawing after timeout
        filterDrawActive = false;
      }
    }


  }
}


void filtercheck() {
  
    FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
    int page, slot;
    static int lastNonzeroVal[NUM_CHANNELS] = {0};

    if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot) && !filterfreshsetted) {
        int currVal = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);

        // Debounce: if currVal is 0 but last was not, ignore this frame
        if (currVal == 0 && lastDefaultFastFilterValue[GLOB.currentChannel] != 0) {
            currVal = lastDefaultFastFilterValue[GLOB.currentChannel];
        }

        if (currVal != lastDefaultFastFilterValue[GLOB.currentChannel]) {
            lastDefaultFastFilterValue[GLOB.currentChannel] = currVal;
            filterDrawActive = true;
            // Show longer when not playing (2s) vs when playing (1s)
            filterDrawEndTime = millis() + (isNowPlaying ? 1000 : 500);
            drawFilterCheck(currVal, dft.idx, filterColors[page][slot]);
        }
    }
}

void checkButtons() {
  // `buttons` array is populated by `checkEncoders()` -> `handle_button_state()`
  bool changed = (memcmp(buttons, oldButtons, sizeof(buttons)) != 0);
  checkFastRec();

  if (changed) {
    // Filter out invalid "9" values: a "9" (long release) is only valid if previous state was 1 or 2 (not 0)
    // This prevents ghost "9" events when transitioning from 0
    for (int i = 0; i < NUM_ENCODERS; i++) {
      if (buttons[i] == 9 && oldButtons[i] == 0) {
        buttons[i] = 0;  // Invalidate: "9" can only come from state 1 or 2
      }
    }

    Serial.print("Button event for checkMode (from checkButtons): ");
    for (int i = 0; i < NUM_ENCODERS; ++i) Serial.print(buttons[i]);
    Serial.println();

    // Create a temporary copy for checkMode to use, so checkMode cannot inadvertently
    // alter the state `checkButtons` intends to process for consumption logic later,
    // AND so that checkMode is working with a snapshot.
    uint8_t snapshotButtons[NUM_ENCODERS];
    memcpy(snapshotButtons, buttons, sizeof(buttons));

    checkMode(snapshotButtons, false);  // Pass the snapshot

    // After checkMode has processed the snapshot, update oldButtons to this snapshot state.
    // This records what was just acted upon.
    memcpy(oldButtons, snapshotButtons, sizeof(buttons));

    // Now, consume momentary events (1 and 9) from the *global* `buttons` array.
    // This prepares the global `buttons` array for the next cycle.
    // `handle_button_state` will repopulate it based on new physical actions.
    for (int i = 0; i < NUM_ENCODERS; i++) {
      if (buttons[i] == 1 || buttons[i] == 9) {  // If the *current* global buttons still show the event
        buttons[i] = 0;                          // Clear it from global buttons
      }
    }
  }
}



void checkTouchInputs() {
  // remember last time both were held
  // static bool lastBothTouched = false; // Already global

  // 1) read raw touch values
  int tv1 = fastTouchRead(SWITCH_1);
  int tv2 = fastTouchRead(SWITCH_2);
  int tv3 = fastTouchRead(SWITCH_3);
  
  // Tap tempo for BPM menu using touch3
  if (currentMode == &volume_bpm) {
    static bool lastTouch3State = false;
    static unsigned long tapTimes[4] = {0, 0, 0, 0}; // Store last 4 tap times
    static uint8_t tapIndex = 0;
    static unsigned long lastTapTime = 0;
    const unsigned long TAP_TIMEOUT = 2000; // Reset if no tap for 2 seconds
    
    // Kalman filter state for BPM smoothing
    static float kalmanBPM = 0.0f;  // Estimated BPM
    static float kalmanP = 1.0f;    // Estimation error covariance
    const float kalmanQ = 0.01f;    // Process noise covariance (small = trust model more)
    const float kalmanR = 2.0f;     // Measurement noise covariance (larger = trust measurements less)
    
    bool currentTouch3State = (tv3 > touchThreshold);
    
    // Detect rising edge (tap)
    if (currentTouch3State && !lastTouch3State) {
      unsigned long currentTime = millis();
      
      // Reset if too much time passed since last tap
      if (lastTapTime > 0 && (currentTime - lastTapTime) > TAP_TIMEOUT) {
        tapIndex = 0;
        for (uint8_t i = 0; i < 4; i++) tapTimes[i] = 0;
        // Reset Kalman filter
        kalmanBPM = 0.0f;
        kalmanP = 1.0f;
      }
      
      // Store tap time
      tapTimes[tapIndex % 4] = currentTime;
      tapIndex++;
      lastTapTime = currentTime;
      
      // Calculate BPM from tap intervals (need at least 2 taps)
      if (tapIndex >= 2) {
        // Use last 2-4 taps to calculate average interval
        uint8_t numTaps = min(tapIndex, (uint8_t)4);
        unsigned long totalInterval = 0;
        uint8_t intervalCount = 0;
        
        // Calculate intervals between consecutive taps
        for (uint8_t i = 1; i < numTaps; i++) {
          uint8_t idx1 = (tapIndex - i - 1) % 4;
          uint8_t idx2 = (tapIndex - i) % 4;
          if (tapTimes[idx1] > 0 && tapTimes[idx2] > 0) {
            unsigned long interval = tapTimes[idx2] - tapTimes[idx1];
            if (interval > 0 && interval < 3000) { // Valid interval: 0-3000ms (20-300 BPM)
              totalInterval += interval;
              intervalCount++;
            }
          }
        }
        
        // Calculate BPM from average interval
        if (intervalCount > 0) {
          float avgInterval = (float)totalInterval / (float)intervalCount; // milliseconds per beat
          float measuredBPM = 60000.0f / avgInterval; // BPM = 60000ms / interval_ms
          
          // Clamp to valid range
          measuredBPM = constrain(measuredBPM, (float)BPM_MIN, (float)BPM_MAX);
          
          // Apply Kalman filter
          if (kalmanBPM == 0.0f) {
            // First measurement: initialize filter
            kalmanBPM = measuredBPM;
            kalmanP = kalmanR;
          } else {
            // Prediction step
            float P_pred = kalmanP + kalmanQ;
            
            // Update step
            float K = P_pred / (P_pred + kalmanR);  // Kalman gain
            kalmanBPM = kalmanBPM + K * (measuredBPM - kalmanBPM);
            kalmanP = (1.0f - K) * P_pred;
          }
          
          // Use filtered BPM
          float calculatedBPM = kalmanBPM;
          calculatedBPM = constrain(calculatedBPM, (float)BPM_MIN, (float)BPM_MAX);
          
          // Update BPM
          SMP.bpm = calculatedBPM;
          currentMode->pos[3] = (unsigned int)calculatedBPM;
          Encoder[3].writeCounter((int32_t)calculatedBPM);
          
          // Apply BPM immediately
          extern void applyBPMDirectly(int bpm);
          applyBPMDirectly((int)calculatedBPM);
          
          // Update display
          extern void drawBPMScreen();
          drawBPMScreen();
          
          // Note: BPM is applied via applyBPMDirectly which updates playTimer
          // BPM value is stored in SMP.bpm and will persist in currentMode->pos[3]
        }
      }
    }
    
    lastTouch3State = currentTouch3State;
  }

  // 2) threshold into boolean states
  bool newTouchState[2];
  newTouchState[0] = (tv1 > touchThreshold);
  newTouchState[1] = (tv2 > touchThreshold);
  //touchState[2] = (tv3 > touchThreshold);

  // 3) detect "rising edge" of both‐pressed
  bool bothTouched = newTouchState[0] && newTouchState[1];
  bool newBoth = bothTouched && !lastBothTouched;
  lastBothTouched = bothTouched;
  
  // State machine to prevent conflicts when one touch is held and another is pressed
  static bool touchConflict = false;
  static bool touch1Held = false;
  static bool touch2Held = false;
  static int firstTouchPressed = 0; // 0=none, 1=left first, 2=right first
  
  // Track which touch was pressed first
  if (newTouchState[0] && !lastTouchState[0] && !touch2Held) {
    firstTouchPressed = 1; // Left pressed first
  }
  if (newTouchState[1] && !lastTouchState[1] && !touch1Held) {
    firstTouchPressed = 2; // Right pressed first
  }
  
  // Check for touch conflicts (one held, another pressed)
  if (newTouchState[0] && !lastTouchState[0] && touch2Held) {
    touchConflict = true;
  }
  if (newTouchState[1] && !lastTouchState[1] && touch1Held) {
    touchConflict = true;
  }
  
  // Reset conflict when both touches are released
  if (!newTouchState[0] && !newTouchState[1]) {
    touchConflict = false;
    touch1Held = false;
    touch2Held = false;
    firstTouchPressed = 0;
  }
  
  // Update held states
  if (newTouchState[0]) touch1Held = true;
  if (newTouchState[1]) touch2Held = true;
  
  // Update touch states only after processing
  touchState[0] = newTouchState[0];
  touchState[1] = newTouchState[1];

  if (newBoth) {
    // only on the first frame where both are pressed:
    // Action depends on which touch was pressed first
    // Right pressed first (2) -> go to filter mode (if valid channel)
    // Left pressed first (1) OR simultaneous (0) -> go to set_Wav mode
    
    if (currentMode == &singleMode || currentMode == &draw || currentMode == &menu) {
      // Validate current channel before any mode switch
      if (GLOB.currentChannel < 0 || GLOB.currentChannel > 15) {
        GLOB.currentChannel = 1; // Safe default
      }
      
      if (firstTouchPressed == 2) {
        // Right pressed first -> go to filter mode
        // Only switch to filter mode if current channel is valid for filters
        if (GLOB.currentChannel != 0 && GLOB.currentChannel != 9 && GLOB.currentChannel != 10 && GLOB.currentChannel != 12 && GLOB.currentChannel != 15) {
          // Reset filter page to 1 when entering filter mode via both touch inputs
          filterPage[GLOB.currentChannel] = 1;
          initSliders(filterPage[GLOB.currentChannel], GLOB.currentChannel);
          switchMode(&filterMode);
        }
        // If channel doesn't support filters, do nothing (don't crash)
      } else {
        // Left pressed first (1) OR simultaneous (0) -> go to set_Wav
        // Only go to set_Wav if channel supports samples (1-8)
        if (GLOB.currentChannel >= 1 && GLOB.currentChannel <= 8) {
          if (currentMode == &singleMode) {
            GLOB.singleMode = false;
          }
          switchMode(&set_Wav);
        }
        // If channel doesn't support samples, do nothing (don't crash)
      }
    }
    // skip any individual‐touch handling this frame
  } else if (!bothTouched) {
    // 4) only if *not* holding both, handle individual rising edges
    // Add debouncing to prevent rapid state changes
    static unsigned long lastTouchTime[2] = {0, 0};
    unsigned long currentTime = millis();
    const unsigned long DEBOUNCE_TIME = 50; // 50ms debounce

    // SWITCH_1
    if (touchState[0] && !lastTouchState[0] && (currentTime - lastTouchTime[0] > DEBOUNCE_TIME) && !touchConflict) {
      lastTouchTime[0] = currentTime;
      if (currentMode == &draw) {
        if (!(GLOB.currentChannel == 0 || GLOB.currentChannel == 9 || GLOB.currentChannel == 10 || GLOB.currentChannel == 12 || GLOB.currentChannel == 15)) {
          animateSingle();
        }
      } else if (currentMode == &menu) {
        extern bool inLookSubmenu;
        extern bool inRecsSubmenu;
        extern bool inMidiSubmenu;
        extern bool inVolSubmenu;
        extern int currentMenuPage;
        // If in LOOK submenu, exit back to main menu at LOOK page (index 4)
        if (inLookSubmenu) {
          inLookSubmenu = false;
          currentMenuPage = 4;
          Encoder[3].writeCounter((int32_t)4);
        } else if (inRecsSubmenu) {
          // If in RECS submenu, exit back to main menu at RECS page (index 5)
          inRecsSubmenu = false;
          currentMenuPage = 5;
          Encoder[3].writeCounter((int32_t)5);
        } else if (inMidiSubmenu) {
          // If in MIDI submenu, exit back to main menu at MIDI page (index 6)
          inMidiSubmenu = false;
          currentMenuPage = 6;
          Encoder[3].writeCounter((int32_t)6);
        } else if (inVolSubmenu) {
          // If in VOL submenu, exit back to main menu at VOL page (index 4)
          inVolSubmenu = false;
          currentMenuPage = 4;
          Encoder[3].writeCounter((int32_t)4);
        } else {
          // Otherwise exit to draw mode
          switchMode(&draw);
          GLOB.singleMode = false;
          // Update encoder colors to reflect the new currentChannel when switching to draw mode
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
          Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        }
      } else {  // If in any other mode, and Switch 1 is touched, go to draw mode.
        if (currentMode == &singleMode) {
          GLOB.currentChannel = GLOB.y - 1;  // Set currentChannel based on Y position when exiting single mode
        }
        switchMode(&draw);
        GLOB.singleMode = false;
        // Update encoder colors to reflect the new currentChannel when switching to draw mode
        Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      }
    }

    // SWITCH_2
    if (currentMode != &filterMode && touchState[1] && !lastTouchState[1] && (currentTime - lastTouchTime[1] > DEBOUNCE_TIME) && !touchConflict) {
      lastTouchTime[1] = currentTime;
      if (currentMode == &draw || currentMode == &singleMode) {
        switchMode(&menu);
      } else if (currentMode == &newFileMode) {
        // Exit NEW mode without generating
        extern void resetNewModeState();
        resetNewModeState();
        switchMode(&draw);
        // Update encoder colors to reflect the current channel when exiting newFileMode
        Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else if (currentMode == &menu) {
        extern bool inLookSubmenu;
        extern bool inRecsSubmenu;
        extern bool inMidiSubmenu;
        extern bool inVolSubmenu;
        extern int currentMenuPage;
        // If in LOOK submenu, exit back to main menu at LOOK page (index 4)
        if (inLookSubmenu) {
          inLookSubmenu = false;
          currentMenuPage = 4;
          Encoder[3].writeCounter((int32_t)4);
        } else if (inRecsSubmenu) {
          // If in RECS submenu, exit back to main menu at RECS page (index 5)
          inRecsSubmenu = false;
          currentMenuPage = 5;
          Encoder[3].writeCounter((int32_t)5);
        } else if (inMidiSubmenu) {
          // If in MIDI submenu, exit back to main menu at MIDI page (index 6)
          inMidiSubmenu = false;
          currentMenuPage = 6;
          Encoder[3].writeCounter((int32_t)6);
        } else if (inVolSubmenu) {
          // If in VOL submenu, exit back to main menu at VOL page (index 4)
          inVolSubmenu = false;
          currentMenuPage = 4;
          Encoder[3].writeCounter((int32_t)4);
        } else {
          // Otherwise exit to draw mode
          switchMode(&draw);
          // Update encoder colors to reflect the current channel when exiting menu
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
          Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        }
      } else {  // If in any other mode (e.g. set_wav, etc.) and Switch 2 is touched, go to draw mode.
        switchMode(&draw);
        // Update encoder colors to reflect the current channel when exiting other modes
        Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      }
    }
  }
  // else: holding both but *not* a new press → do nothing

  // 5) update lastTouchState for next pass
  lastTouchState[0] = touchState[0];
  lastTouchState[1] = touchState[1];
  lastTouchState[2] = touchState[2];
}

void animateSingle() {
  int yStart = GLOB.currentChannel;              // 0 to 15
  int yCenter = yStart + 1;                     // Because y grid is 1-based
  CRGB animCol = col_base[GLOB.currentChannel];  // Renamed to avoid conflict with global 'col'

  // Animate outward from center
  for (int offset = 0; offset <= 15; offset++) {
    int yUp = yCenter - offset;
    int yDown = yCenter + offset;

    for (int x = 1; x <= 16; x++) {
      if (yUp >= 1 && yUp <= 15) light(x, yUp, animCol);
      if (yDown >= 1 && yDown <= 15 && offset != 0) light(x, yDown, animCol);
    }
    drawTriggers();
    //FastLED.show(); // Consider if FastLEDshow() is better here or after full animation
    //delay(5);  // adjust for animation speed
  }
  FastLEDshow();  // Show after the animation loop if not inside
  switchMode(&singleMode);
  GLOB.singleMode = true;
}

void checkSingleTouch() {
  int touchValue = fastTouchRead(SWITCH_1);

  // Determine if the touch is above the threshold
  touchState[0] = (touchValue > touchThreshold);
  // Check for a rising edge (LOW to HIGH transition)
  if (touchState[0] && !lastTouchState[0]) {
    // Toggle the mode only on a rising edge
    if (currentMode == &draw) {
      // GLOB.currentChannel = GLOB.currentChannel; // This line is redundant
      switchMode(&singleMode);
      GLOB.singleMode = true;
    } else if (currentMode == &menu) {
      extern bool inLookSubmenu;
      // If in LOOK submenu, exit back to main menu
      if (inLookSubmenu) {
        inLookSubmenu = false;
      } else {
        // Otherwise exit to draw mode
        if (currentMode == &singleMode) {
          GLOB.currentChannel = GLOB.y - 1;  // Set currentChannel based on Y position when exiting single mode
        }
        switchMode(&draw);
        GLOB.singleMode = false;
        // Update encoder colors to reflect the new currentChannel when switching to draw mode
        Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      }
    } else {
      if (currentMode == &singleMode) {
        GLOB.currentChannel = GLOB.y - 1;  // Set currentChannel based on Y position when exiting single mode
      }
      switchMode(&draw);
      GLOB.singleMode = false;
      // Update encoder colors to reflect the new currentChannel when switching to draw mode
      Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    }
  }
  // Update the last touch state
  lastTouchState[0] = touchState[0];
}

void _checkMenuTouch() {
  int touchValue = fastTouchRead(SWITCH_2);

  // Determine if the touch is above the threshold
  touchState[1] = (touchValue > touchThreshold);
  // Check for a rising edge (LOW to HIGH transition)
  if (touchState[1] && !lastTouchState[1]) {
    // Toggle the mode only on a rising edge
    if (currentMode == &draw || currentMode == &singleMode) {
      switchMode(&menu);
    } else if (currentMode == &menu) {
      extern bool inLookSubmenu;
      // If in LOOK submenu, exit back to main menu
      if (inLookSubmenu) {
        inLookSubmenu = false;
      } else {
        // Otherwise exit to draw mode
        switchMode(&draw);
        // Update encoder colors to reflect the current channel when exiting menu
        Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      }
    } else {
      switchMode(&draw);
      // Update encoder colors to reflect the current channel when exiting menu
      Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    }
  }
  // Update the last touch state
  lastTouchState[1] = touchState[1];
}



void checkPendingSampleNotes() {
  // return; // This function seems disabled, keeping it as is.
  unsigned long now = millis();
  
  // Safety: Limit vector size to prevent memory exhaustion
  const size_t MAX_PENDING_NOTES = 64;
  if (pendingSampleNotes.size() > MAX_PENDING_NOTES) {
    // Remove oldest events if too many pending
    pendingSampleNotes.erase(pendingSampleNotes.begin(), 
                            pendingSampleNotes.begin() + (pendingSampleNotes.size() - MAX_PENDING_NOTES / 2));
  }
  
  // Clean up stale events (older than 10 seconds) to prevent unbounded growth
  const unsigned long STALE_THRESHOLD = 10000; // 10 seconds
  for (size_t i = 0; i < pendingSampleNotes.size(); /* no ++ */) {
    if (pendingSampleNotes[i].triggerTime < now - STALE_THRESHOLD) {
      // Stale event - remove it
      pendingSampleNotes.erase(pendingSampleNotes.begin() + i);
    } else {
      ++i;
    }
  }
  
  // Process pending events
  for (size_t i = 0; i < pendingSampleNotes.size(); /* no ++ */) {  // Use size_t for vector index
    if (now >= pendingSampleNotes[i].triggerTime) {
      auto &ev = pendingSampleNotes[i];
      _samplers[ev.channel].noteEvent(ev.pitch, ev.velocity, true, true);
      pendingSampleNotes.erase(pendingSampleNotes.begin() + i);  // erase and continue
    } else {
      ++i;
    } 
  }
}

void showDoRecord() {
  // State machine: NORMAL -> RECORDING -> NORMAL -> PLAYBACK -> NORMAL
  enum RecordState { STATE_NORMAL, STATE_RECORDING, STATE_PLAYBACK };
  static RecordState state = STATE_NORMAL;
  static bool forcedLineMonitor = false;
  static float lastLineMonitorGain = 0.0f;
  
  // Persistent state variables
  static bool thresholdActive = false;
  static bool lastPressed0 = false, lastPressed2 = false;
  static unsigned long recordingStartTime = 0;
  static bool playbackActive = false;
  static unsigned long playbackStartTime = 0;
  static uint32_t lastModeNameHash = 0;  // Replaced String with hash for memory efficiency
  
  // Externs
  extern bool disableThresholdFlag;
  extern AudioAnalyzePeak peakRec;
  extern AudioPlaySdWav playSdWav1;
  extern bool previewIsPlaying;
  extern int peakRecIndex;
  extern const int maxRecPeaks;
  extern float peakRecValues[];
  extern elapsedMillis mRecsecs;
  
  // === MODE ENTRY INITIALIZATION ===
  uint32_t currentModeNameHash = hashString(currentMode->name);
  if (currentModeNameHash != lastModeNameHash) {
    lastModeNameHash = currentModeNameHash;
    state = STATE_NORMAL;
    thresholdActive = false;
    playbackActive = false;
    forcedLineMonitor = false;
    lastLineMonitorGain = 0.0f;
    extern bool sampleIsLoaded, firstcheck;
    extern CachedSample previewCache;
    sampleIsLoaded = false;
    firstcheck = true;
    previewCache.valid = false;
  }

  // === DIRECT INPUT PASS-THROUGH WHEN PREVIEW IS PLAYING AND CURSOR AT Y==1 ===
  extern AudioMixer4 mixer_end;
  extern AudioAmplifier audioInputAmp;
  static bool directPassThroughActive = false;
  
  if (previewIsPlaying && GLOB.y == 1) {
    // Enable direct pass-through: input → output with no additional amplification
    // Only use the set mic/line input gain from codec (already applied via sgtl5000_1)
    audioInputAmp.gain(1.0f);  // Unity gain, no additional amplification
    mixer_end.gain(3, 1.0f);   // Full pass-through of input (other sources continue playing)
    directPassThroughActive = true;
  } else if (directPassThroughActive) {
    // Restore normal routing when conditions no longer met
    audioInputAmp.gain(1.0f);  // Keep at unity gain
    mixer_end.gain(3, 0.0f);   // Disable input pass-through
    directPassThroughActive = false;
    forcedLineMonitor = false;  // Reset line monitor flag
    lastLineMonitorGain = 0.0f;
  }
  
  // === INPUT MONITORING WHEN IN RECORD MODE ===
  // Only apply if direct pass-through is not active
  // Uses VOL menu input level settings (LVL for line, MIC for mic)
  if (!directPassThroughActive) {
    extern int recMode;
    extern unsigned int lineInLevel;  // From VOL menu (0-15)
    extern unsigned int micGain;      // From VOL menu (0-63)
    
    float monitorGain = 0.0f;
    float maxPlaybackGain = GAIN_4 * GAIN_2;  // Match loudest playback: amps×mixer1/2×mixer_end = 1.0×0.4×0.4 = 0.16
    if (recMode == 1) {
      // Mic input: map micGain (0-63) to mixer gain (0.0-maxPlaybackGain) to match loudest playback
      monitorGain = mapf((float)micGain, 0.0f, 63.0f, 0.0f, maxPlaybackGain);
    } else {
      // Line input: map lineInLevel (0-15) to mixer gain (0.0-maxPlaybackGain) to match loudest playback
      monitorGain = mapf((float)lineInLevel, 0.0f, 15.0f, 0.0f, maxPlaybackGain);
    }
    
    mixer_end.gain(3, monitorGain);
    lastLineMonitorGain = monitorGain;
    forcedLineMonitor = true;
  } else if (forcedLineMonitor) {
    // Disable monitoring when direct pass-through becomes active
    mixer_end.gain(3, 0.0f);
    forcedLineMonitor = false;
    lastLineMonitorGain = 0.0f;
  }
  
  // Check global flag to disable threshold
  if (disableThresholdFlag) {
    thresholdActive = false;
    disableThresholdFlag = false;
  }
  
  // === READ INPUT LEVEL (always) ===
  static uint16_t inputLevelSamples[8] = {0};  // Store as 0-10000 (0-100.00%)
  static int sampleIndex = 0;
  float currentInputLevel = 0.0f;
  
  if (peakRec.available()) {
    inputLevelSamples[sampleIndex] = (uint16_t)(peakRec.read() * 10000.0f);
    sampleIndex = (sampleIndex + 1) % 8;
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) sum += inputLevelSamples[i];
    currentInputLevel = sum / 80.0f;  // Divide by 80 = (8 samples × 100 scale)
  }
  
  int triggerThreshold = currentMode->pos[0];
  
  // === DETERMINE STATE ===
  // Update state based on recording and playback flags
  if (isRecording) {
    flushAudioQueueToRAM2();
    checkEncoders();           // Optional: allow user interaction
    checkMode(buttons, true);

    state = STATE_RECORDING;
  } else if (playbackActive) {
    state = STATE_PLAYBACK;
  } else {
    state = STATE_NORMAL;
  }
  
  // === STATE MACHINE LOGIC ===
  switch (state) {
    case STATE_NORMAL: {
      // Toggle threshold with encoder[0]
      if (pressed[0] && !lastPressed0) thresholdActive = !thresholdActive;
      lastPressed0 = pressed[0];
      
      // Start recording on encoder[1] press or audio trigger
      bool manualTrigger = pressed[1];
      bool audioTrigger = (thresholdActive && triggerThreshold > 0 && currentInputLevel > triggerThreshold);
      
      if (manualTrigger || audioTrigger) {
        // Disable threshold to prevent accidental re-trigger
        thresholdActive = false;
        FastLEDclear();  // Clear LEDs before starting recording
        startRecordingRAM();
        recordingStartTime = millis();
      }
      
      // Encoder[2]: Start playback using playSdWav1
      if (pressed[2] && !lastPressed2) {
        int fnr = getFolderNumber(SMP.wav[GLOB.currentChannel].fileID);
        int fileNum = getFileNumber(SMP.wav[GLOB.currentChannel].fileID);
        char path[50];
        sprintf(path, "samples/%d/_%d.wav", fnr, fileNum);
        
        if (playSdWav1.isPlaying()) playSdWav1.stop();
        playSdWav1.play(path);
        
        previewIsPlaying = true;
        playbackActive = true;
        playbackStartTime = millis();
      }
      lastPressed2 = pressed[2];
      // Note: Encoder[3] exit handled by checkMode() via "0001" button pattern
      break;
    }
    
    case STATE_RECORDING: {
      // Allow toggling threshold off during recording (encoder[0] press)
      if (pressed[0] && !lastPressed0) {
        thresholdActive = false;  // Can only turn OFF during recording, not back ON
      }
      lastPressed0 = pressed[0];
      
      // Auto-stop on audio trigger (with 1 second debounce)
      if (millis() - recordingStartTime > 1000) {
        if (thresholdActive && triggerThreshold > 0 && currentInputLevel < (triggerThreshold * 0.5f)) {
          stopRecordingRAM(getFolderNumber(SMP.wav[GLOB.currentChannel].fileID), SMP.wav[GLOB.currentChannel].fileID);
          // Disable threshold to prevent accidental re-trigger from loud stop click
          thresholdActive = false;
        }
      }
      
      // Collect peak data
      if (mRecsecs > 5 && peakRecIndex < maxRecPeaks && peakRec.available()) {
        mRecsecs = 0;
        peakRecValues[peakRecIndex++] = peakRec.read();
      }
      // Note: isRecording flag is checked externally, encoder[2] stop handled in checkMode()
      break;
    }
    
    case STATE_PLAYBACK: {
      // Encoder[2]: Stop playback
      if (pressed[2] && !lastPressed2) {
        playSdWav1.stop();
        previewIsPlaying = false;
        playbackActive = false;
      }
      lastPressed2 = pressed[2];
      // Note: Encoder[3] exit handled by checkMode() via "0001" button pattern
      
      // Auto-return to normal when playback finishes (check after 300ms grace period)
      if (millis() - playbackStartTime > 300) {
        if (!playSdWav1.isPlaying()) {
          previewIsPlaying = false;
          playbackActive = false;
        }
      }
      break;
    }
  }
  
  // === DRAW UI (common for all states) ===
  FastLEDclear();
  
  // Encoder colors and indicators
  Encoder[0].writeRGBCode(0xFFFF00);  // Yellow
  drawIndicator('L', 'Y', 1);
  
  if (state == STATE_RECORDING) {
    Encoder[1].writeRGBCode(0x000000);  // Black during recording
  } else {
    Encoder[1].writeRGBCode(0xFF0000);  // Red
    drawIndicator('L', 'R', 2);
  }
  
  Encoder[2].writeRGBCode(0x00FF00);  // Green
  drawIndicator('L', 'G', 3);
  
  Encoder[3].writeRGBCode(0x0000FF);  // Blue
  drawIndicator('L', 'X', 4);
  
  // Draw threshold and input level bars
  int thresholdHeight = mapf(triggerThreshold, 0, 100, 0, maxY);
  int inputHeight = mapf(currentInputLevel, 0, 100, 0, maxY);
  
  // x=1: Threshold bar with live signal overlay
  for (int y = 1; y <= thresholdHeight && y <= maxY; y++) {
    CRGB color = thresholdActive ? CRGB(0, 255, 0) : CRGB(255, 255, 0);
    light(1, y, color);
  }
  for (int y = 1; y <= inputHeight && y <= maxY; y++) {
    if (y <= thresholdHeight && thresholdActive) {
      light(1, y, CRGB(100, 255, 100));
    } else if (y <= thresholdHeight) {
      light(1, y, CRGB(255, 200, 0));
    } else {
      light(1, y, CRGB(255, 0, 0));
    }
  }
  
  // x=2: Live input level bar
  for (int y = 1; y <= inputHeight && y <= maxY; y++) {
    CRGB barColor = CRGB(255, 0, 0);
    light(2, y, barColor);
  }
  
  // State-specific text display (position 3,5 to match main recording timer)
  if (state == STATE_RECORDING) {
    // Recording timer at (3, 5) - matches main loop recording display
    char buf[8];
    snprintf(buf, sizeof(buf), "%4.1f", recTime / 1000.0f);  // Right-aligned, 1 decimal
    drawText(buf, 3, 5, UI_ORANGE);
  } else if (state == STATE_PLAYBACK) {
    // Playback timer at (3, 5) - EXACTLY same position as recording timer
    char buf[8];
    unsigned long playbackTime = millis() - playbackStartTime;
    snprintf(buf, sizeof(buf), "%4.1f", playbackTime / 1000.0f);  // Right-aligned, 1 decimal
    drawText(buf, 3, 5, CRGB(0, 255, 0));  // Green playback timer
  } else {
    // STATE_NORMAL: Only draw RDY when NOT recording and NOT playing
    drawText("RDY", 5, 5, CRGB(255, 100, 0));  // Orange ready text
  }
}

void loop() {
   updateExternalOneBlink();
   checkMidi();  // Process MIDI early in loop for lower latency
   
   yield(); // Yield early to maintain responsiveness during file operations

  bool pongActive = pong && currentMode == &draw;

  if (pongActive) {
    updatePongBall();
  } else {
    resetPongGame();
  }

  if (currentMode == &draw || currentMode == &singleMode) {
    drawBase();
    drawTriggers();
    if (isNowPlaying) drawTimer();
    
    // Draw count-in for ON1 mode
    extern void drawCountIn();
    if (countInActive) {
      drawCountIn();
    }
    
    // Draw red border when recording
    extern void drawRecordingBorder();
    if (fastRecordActive) {
      drawRecordingBorder();
    }
    if (pongActive) {
      drawPongBall();
    }
  }


//granular on ch=8
/*
if (SMP.filter_settings[8][ACTIVE]>0){
      unsigned long granulartime = mapf(SMP.filter_settings[8][PITCH], 0, maxfilterResolution, 0.0, 1500.0); 
                 if (millis() - ganularStartTime > granulartime){
                granular1.beginFreeze(SMP.filter_settings[8][OFFSET]);
                ganularStartTime = millis();
                }
}else{granular1.stop();}
*/

  // Handle deferred stop recording (from timer interrupt)
  if (pendingStopFastRecord) {
    pendingStopFastRecord = false;
    stopFastRecord();
  }

  if (fastRecordActive) {
    flushAudioQueueToRAM();     // grab incoming audio into RAM
    checkMode(buttons, false);  // MODIFIED call
    drawTimer();
    checkPendingSampleNotes();
    //return;  // skip the rest while we're fast-recording // This return might be intended
  }



  if (previewIsPlaying) {
    static elapsedMillis peakCaptureTimer;
    if (peakCaptureTimer > 15) { // capture peaks at ~66 fps max
      peakCaptureTimer = 0;
      if (playSdWav1.isPlaying() && peakIndex < maxPeaks) {
        if (peak1.available()) {
          // Store the peak value
          peakValues[peakIndex] = peak1.read();
          peakIndex++;
        }
      }
    }

    if (!playSdWav1.isPlaying()) {
      playSdWav1.stop();
      previewIsPlaying = false;  // Playback finished
    }
  }


  checkEncoders();
  if (currentMode != &velocity && currentMode != &filterMode) drawCursor();
  checkButtons();
  checkTouchInputs();
  checkPendingSampleNotes();
  
  // Periodic system stats to Serial (every ~5s)
  if (diagStatsTimer > 5000) {
    diagStatsTimer = 0;
    float cpu = AudioProcessorUsage();
    float cpuMax = AudioProcessorUsageMax();
    int audioMem = AudioMemoryUsage();
    int audioMemMax = AudioMemoryUsageMax();
    char* heapEnd = (char*)sbrk(0);
    char* stackPtr = (char*)__builtin_frame_address(0);
    unsigned long freeRam = 0;
    if ((uintptr_t)stackPtr > (uintptr_t)heapEnd) {
      freeRam = (unsigned long)((uintptr_t)stackPtr - (uintptr_t)heapEnd);
    }
    unsigned long previewBuf = (unsigned long)sizeof(sampled[0]);
    float currentGain = GAIN_4 * GAIN_2;  // Current total gain for samples
    float maxPossibleGain = 1.0f;  // Maximum theoretical gain (1.0 × 1.0)
    float gainPercent = (currentGain / maxPossibleGain) * 100.0f;
    Serial.printf("SYS cpu=%.1f%% max=%.1f%% audioMem=%d/%d freeRAM=%luKB previewBuf=%luKB gain=%.2f (%.1f%%)\n",
                  cpu, cpuMax, audioMem, audioMemMax, freeRam / 1024, previewBuf / 1024, currentGain, gainPercent);
  }
  
  yield(); // Yield periodically to prevent unresponsiveness, especially during file operations
  
  // === INPUT MONITORING WHEN IN DRAW MODE ===
  // Enable if monitoring toggle is ON (y==1 only) or ALL (always on), and not in record mode
  static bool drawModeInputMonitorActive = false;
  bool shouldMonitor = false;
  if (currentMode == &draw && currentMode->name != "RECORD_MODE") {
    if (inputMonitoringState == 1 && GLOB.y == 1) {
      // ON mode: only when y==1
      shouldMonitor = true;
    } else if (inputMonitoringState == 2) {
      // ALL mode: always on regardless of y position
      shouldMonitor = true;
    }
  }
  
  if (shouldMonitor) {
    extern AudioMixer4 mixer_end;
    extern AudioAmplifier audioInputAmp;
    extern int recMode;
    
    // Enable input monitoring with current gain settings
    audioInputAmp.gain(1.0f);  // Unity gain, no additional amplification
    
    float monitorGain = 0.0f;
    float maxPlaybackGain = GAIN_4 * GAIN_2;  // Match loudest playback: amps×mixer1/2×mixer_end = 1.0×0.4×0.4 = 0.16
    if (recMode == 1) {
      // Mic input: map micGain (0-63) to mixer gain (0.0-maxPlaybackGain) to match loudest playback
      monitorGain = mapf((float)micGain, 0.0f, 63.0f, 0.0f, maxPlaybackGain);
    } else {
      // Line input: map lineInLevel (0-15) to mixer gain (0.0-maxPlaybackGain) to match loudest playback
      monitorGain = mapf((float)lineInLevel, 0.0f, 15.0f, 0.0f, maxPlaybackGain);
    }
    
    mixer_end.gain(3, monitorGain);
    drawModeInputMonitorActive = true;
  } else if (drawModeInputMonitorActive && currentMode->name != "RECORD_MODE") {
    // Disable monitoring when conditions are no longer met
    extern AudioMixer4 mixer_end;
    mixer_end.gain(3, 0.0f);
    drawModeInputMonitorActive = false;
  }
  
  // Update smooth filter mixer gains
  updateMixerGains(GLOB.currentChannel);

  // Recovery mechanism: Detect and reset stuck pressed[] states
  // If pressed[] is true but no button event is active, it might be stuck
  static unsigned long pressedStateTime[NUM_ENCODERS] = {0, 0, 0, 0};
  static bool lastPressedState[NUM_ENCODERS] = {false, false, false, false};
  unsigned long now = millis();
  for (int i = 0; i < NUM_ENCODERS; i++) {
    if (pressed[i] != lastPressedState[i]) {
      // State changed - reset timer
      pressedStateTime[i] = now;
      lastPressedState[i] = pressed[i];
    } else if (pressed[i] && (now - pressedStateTime[i] > 2000)) {
      // pressed[i] has been true for 2+ seconds without change - likely stuck
      // Check if button is actually pressed by reading encoder status
      uint8_t encoderStatus = Encoder[i].readStatus();
      bool actualButtonPressed = (encoderStatus != 0xFF && (encoderStatus & 0x08) != 0);
      if (!actualButtonPressed) {
        // Hardware says button is NOT pressed but pressed[i] is true - force reset
        pressed[i] = false;
        pressedStateTime[i] = 0;
      }
    }
  }

  // Recovery mechanism: Auto-reset preventPaintUnpaint if stuck for too long
  static unsigned long preventPaintUnpaintSetTime = 0;
  static bool lastPreventState = false;
  if (preventPaintUnpaint != lastPreventState) {
    preventPaintUnpaintSetTime = now;
    lastPreventState = preventPaintUnpaint;
  } else if (preventPaintUnpaint && (now - preventPaintUnpaintSetTime > 5000)) {
    // preventPaintUnpaint has been true for 5+ seconds - likely stuck, force reset
    preventPaintUnpaint = false;
    preventPaintUnpaintSetTime = 0;
  }

  if (note[GLOB.x][GLOB.y].channel == 0 && (currentMode == &draw || currentMode == &singleMode) && pressed[3] == true && !preventPaintUnpaint) {
    paintMode = false;
    freshPaint = true;
    unpaintMode = false;
    pressed[3] = false;
    paint();
    preventPaintUnpaint = false;  // Reset flag after paint operation
    // return; // This return might skip drawing updates if not careful
  }

  if ((currentMode == &draw || currentMode == &singleMode) && pressed[0] == true && !preventPaintUnpaint) {
    paintMode = false;
    unpaintMode = false;
    pressed[0] = false;
    unpaint();
  }

  // Handle encoder 2 press in SET_WAV mode - reverse preview sample
  if (currentMode == &set_Wav && pressed[2] == true && !isRecording) {
    pressed[2] = false;
    Serial.println("Encoder 2 pressed in SET_WAV mode - reversing preview");
    reversePreviewSample();
  }

  // Set stateMashine
  if (currentMode->name == "FILTERMODE") {
    //setFilters();

    setNewFilters();

  } else if (currentMode->name == "VOLUME_BPM") {
    setVolume();
  } else if (currentMode->name == "VELOCITY") {
    setVelocity();
  } else if (currentMode->name == "LOADSAVE_TRACK") {
    showLoadSave();
  } else if (currentMode->name == "MENU") {
    extern bool inLookSubmenu;
    extern bool inRecsSubmenu;
    extern bool inMidiSubmenu;
    extern bool inVolSubmenu;
    extern void showLookMenu();
    extern void showRecsMenu();
    extern void showMidiMenu();
    extern void showVolMenu();
    if (inLookSubmenu) {
      showLookMenu();
    } else if (inRecsSubmenu) {
      showRecsMenu();
    } else if (inMidiSubmenu) {
      showMidiMenu();
    } else if (inVolSubmenu) {
      showVolMenu();
    } else {
      showMenu();
    }
  } else if (currentMode->name == "SET_SAMPLEPACK") {
    showSamplePack();
  } else if (currentMode->name == "SET_WAV") {
    if (isRecording) { /* Do Nothing */
    }                  // Avoid return here if FastLEDshow is needed
    else { showWave(); }
  } else if (currentMode->name == "RECORD_MODE") {
    showDoRecord();
  } else if (currentMode->name == "NOTE_SHIFT") {
    shiftNotes();
    drawBase();
    drawTriggers();
    if (isNowPlaying) {
      //filtercheck
      drawTimer();
    }
  } else if (currentMode->name == "NEW_FILE") {
    showNewFileMode();
  } else if (currentMode->name == "SUBPATTERN") {
    switchSubPattern();
  } else if (currentMode->name == "SONGMODE") {
    extern void showSongMode();
    showSongMode();
  }



  for (int ch = 13; ch <= 14; ch++) {
    unsigned long noteLen = getNoteDuration(ch);
    // Only auto-release if the note is not persistent (i.e. not from a live MIDI press)

    if (noteOnTriggered[ch] && !persistentNoteOn[ch] && (millis() - startTime[ch] >= noteLen)) {
      if (!envelopes[ch]) continue;
      envelopes[ch]->noteOff();
      noteOnTriggered[ch] = false;
    }
  }
  
  autoOffActiveNotes();
  
  filterfreshsetted = false;

  if (ctrlVolumeOverlayActive && ctrlMode == 1 && (currentMode == &draw || currentMode == &singleMode)) {
    if (millis() <= ctrlVolumeOverlayUntil) {
      drawCtrlVolumeOverlay(ctrlVolumeOverlayValue);
    } else {
      ctrlVolumeOverlayActive = false;
    }
  }

  // Draw input gain overlay when in draw mode at y==1
  if (inputGainOverlayActive && currentMode == &draw && GLOB.y == 1 && ctrlMode == 1) {
    if (millis() <= inputGainOverlayUntil) {
      drawInputGainOverlay(inputGainOverlayValue, inputGainOverlayMax);
    } else {
      inputGainOverlayActive = false;
    }
  }

  // Draw channel number overlay when in draw mode
  if (channelNrOverlayActive && currentMode == &draw && showChannelNr) {
    if (millis() <= channelNrOverlayUntil && GLOB.y <= 9) {
      // Pass both channel number (1-indexed for display) and channel index (0-indexed for color)
      // Only render if y <= 9 (channels 1-8)
      drawChannelNrOverlay(channelNrOverlayChannel, GLOB.currentChannel);
    } else {
      channelNrOverlayActive = false;
    }
  }

  FastLEDshow();

   if (stepIsDue) {
    stepIsDue = false; // Immediately reset the flag
    playNote();      // Now, do the heavy lifting here in the non-time-critical main loop
  }

  yield();
}

FLASHMEM void triggerExternalOneBlink() {
  if (MIDI_CLOCK_SEND) return;        // External clock only
  if (isNowPlaying) return;           // Only blink while waiting to start

  Encoder[EXTERNAL_ONE_ENCODER_INDEX].writeRGBCode(0xFFFFFF);  // White blink
  externalOneBlinkActive = true;

  unsigned long beatDurationMs = 120;  // default short blink
  if (SMP.bpm > 0.0f) {
    beatDurationMs = (unsigned long)(60000.0f / (SMP.bpm * 4.0f)); // quarter-beat blink
    if (beatDurationMs < 50) beatDurationMs = 50;
  }
  externalOneBlinkUntil = millis() + beatDurationMs;
}

FLASHMEM void updateExternalOneBlink() {
  if (!externalOneBlinkActive) return;
  if ((long)(millis() - externalOneBlinkUntil) >= 0) {
    externalOneBlinkActive = false;
    Encoder[EXTERNAL_ONE_ENCODER_INDEX].writeRGBCode(0x000000);  // Turn back off
  }
}

float getNoteDuration(int channel) {
  // Calculate ADSR-derived length in milliseconds
  float delayMs  = mapf(SMP.param_settings[channel][DELAY],  0, maxfilterResolution, 0, maxParamVal[DELAY]);
  float attackMs = mapf(SMP.param_settings[channel][ATTACK], 0, maxfilterResolution, 0, maxParamVal[ATTACK]);
  float holdMs   = mapf(SMP.param_settings[channel][HOLD],   0, maxfilterResolution, 0, maxParamVal[HOLD]);
  float decayMs  = mapf(SMP.param_settings[channel][DECAY],  0, maxfilterResolution, 0, maxParamVal[DECAY]);
  float releaseMs= mapf(SMP.param_settings[channel][RELEASE],0, maxfilterResolution, 0, maxParamVal[RELEASE]);

  float timetilloff = delayMs + attackMs + holdMs + decayMs + releaseMs;

  // Ensure attack is actually reached before note-off by enforcing at least (attack + small cushion)
  float minLen = attackMs + 5.0f; // 5ms cushion beyond attack
  if (timetilloff < minLen) timetilloff = minLen;

  // Absolute floor to avoid zero-length
  if (timetilloff < 5.0f) timetilloff = 5.0f;
  return timetilloff;
}


FLASHMEM void shiftNotes() {
  unsigned int patternLength = lastPage * maxX;
  if (currentMode->pos[3] != GLOB.shiftX) {
    // Determine shift direction (+1 or -1)
    int shiftDirectionX = 0;
    if (currentMode->pos[3] > GLOB.shiftX) {
      shiftDirectionX = 1;
    } else {
      shiftDirectionX = -1;
    }
    GLOB.shiftX = 8;  // Reset conceptual center for encoder delta

    Encoder[3].writeCounter((int32_t)8);  // Reset physical encoder to center
    currentMode->pos[3] = 8;              // Reflect this in currentMode's tracked position
    // Step 1: Clear the tmp array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
      for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
        tmp[nx][ny].channel = 0;
        tmp[nx][ny].velocity = defaultVelocity;
        tmp[nx][ny].probability = 100;  // Default probability
        tmp[nx][ny].condition = 1;      // Default condition
      }
    }

    // Step 2: Shift notes of the current channel into tmp array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (note[nx][ny].channel == GLOB.currentChannel) {
          int newposX = nx + shiftDirectionX;

          // Handle wrapping around the edges
          if (newposX < 1) {
            newposX = patternLength;
          } else if (newposX > patternLength) {
            newposX = 1;
          }
          tmp[newposX][ny].channel = GLOB.currentChannel;
          tmp[newposX][ny].velocity = note[nx][ny].velocity;
          tmp[newposX][ny].probability = note[nx][ny].probability;
          tmp[newposX][ny].condition = note[nx][ny].condition;
        }
      }
    }
    shifted = true;
  }

  if (currentMode->pos[0] != GLOB.shiftY) {
    // Determine shift direction (+1 or -1)
    int shiftDirectionY = 0;
    if (currentMode->pos[0] > GLOB.shiftY) {
      shiftDirectionY = -1;  // Encoder up usually means lower Y value (higher pitch)
    } else {
      shiftDirectionY = +1;  // Encoder down usually means higher Y value (lower pitch)
    }
    GLOB.shiftY = 8;  // Reset conceptual center

    Encoder[0].writeCounter((int32_t)8);  // Reset physical encoder
    currentMode->pos[0] = 8;              // Reflect in currentMode
    // Step 1: Clear the tmp array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
      for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
        tmp[nx][ny].channel = 0;
        tmp[nx][ny].velocity = defaultVelocity;
        tmp[nx][ny].probability = 100;  // Default probability
        tmp[nx][ny].condition = 1;      // Default condition
      }
    }

    // Step 2: Shift notes of the current channel into tmp array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (note[nx][ny].channel == GLOB.currentChannel) {
          int newposY = ny + shiftDirectionY;
          // Handle wrapping around the edges
          if (newposY < 1) {
            newposY = maxY;
          } else if (newposY > maxY) {
            newposY = 1;
          }
          tmp[nx][newposY].channel = GLOB.currentChannel;
          tmp[nx][newposY].velocity = note[nx][ny].velocity;
        }
      }
    }
    shifted = true;
  }

  // Handle encoder 1 Y-shift for current channel and current page only
  if (currentMode->pos[1] != GLOB.shiftY1) {
    // Determine shift direction (+1 or -1)
    int shiftDirectionY1 = 0;
    if (currentMode->pos[1] > GLOB.shiftY1) {
      shiftDirectionY1 = -1;  // Encoder up usually means lower Y value (higher pitch)
    } else {
      shiftDirectionY1 = +1;  // Encoder down usually means higher Y value (lower pitch)
    }
    GLOB.shiftY1 = 8;  // Reset conceptual center

    Encoder[1].writeCounter((int32_t)8);  // Reset physical encoder
    currentMode->pos[1] = 8;              // Reflect in currentMode

    // Calculate the start and end positions for the current visible page (GLOB.edit)
    unsigned int pageStartX = ((GLOB.edit - 1) * maxX) + 1;
    unsigned int pageEndX = GLOB.edit * maxX;

    // Create a temporary array for this page only to avoid overwriting other pages
    Note pageTmp[maxX + 1][maxY + 1] = {};

    // Step 1: Copy current page notes to pageTmp
    for (unsigned int nx = pageStartX; nx <= pageEndX; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        pageTmp[nx - pageStartX + 1][ny] = note[nx][ny];
      }
    }

    // Step 2: Shift notes of the current channel on the current page
    // Process in the correct order to avoid overwriting unprocessed notes
    if (shiftDirectionY1 > 0) {
      // Shifting down: process from bottom to top
      for (unsigned int nx = 1; nx <= maxX; nx++) {
        for (int ny = maxY; ny >= 1; ny--) {
          if (pageTmp[nx][ny].channel == GLOB.currentChannel) {
            int newposY = ny + shiftDirectionY1;
            // Handle wrapping around the edges
            if (newposY < 1) {
              newposY = maxY;
            } else if (newposY > maxY) {
              newposY = 1;
            }
            // Store the note data before clearing
            int originalVelocity = pageTmp[nx][ny].velocity;
            uint8_t originalProbability = pageTmp[nx][ny].probability;
            uint8_t originalCondition = pageTmp[nx][ny].condition;
            // Clear the old position
            pageTmp[nx][ny].channel = 0;
            pageTmp[nx][ny].velocity = defaultVelocity;
            pageTmp[nx][ny].probability = 100;  // Default probability
            pageTmp[nx][ny].condition = 1;      // Default condition
            // Set the new position
            pageTmp[nx][newposY].channel = GLOB.currentChannel;
            pageTmp[nx][newposY].velocity = originalVelocity;
            pageTmp[nx][newposY].probability = originalProbability;
            pageTmp[nx][newposY].condition = originalCondition;
          }
        }
      }
    } else {
      // Shifting up: process from top to bottom
      for (unsigned int nx = 1; nx <= maxX; nx++) {
        for (unsigned int ny = 1; ny <= maxY; ny++) {
          if (pageTmp[nx][ny].channel == GLOB.currentChannel) {
            int newposY = ny + shiftDirectionY1;
            // Handle wrapping around the edges
            if (newposY < 1) {
              newposY = maxY;
            } else if (newposY > maxY) {
              newposY = 1;
            }
            // Store the note data before clearing
            int originalVelocity = pageTmp[nx][ny].velocity;
            uint8_t originalProbability = pageTmp[nx][ny].probability;
            uint8_t originalCondition = pageTmp[nx][ny].condition;
            // Clear the old position
            pageTmp[nx][ny].channel = 0;
            pageTmp[nx][ny].velocity = defaultVelocity;
            pageTmp[nx][ny].probability = 100;  // Default probability
            pageTmp[nx][ny].condition = 1;      // Default condition
            // Set the new position
            pageTmp[nx][newposY].channel = GLOB.currentChannel;
            pageTmp[nx][newposY].velocity = originalVelocity;
            pageTmp[nx][newposY].probability = originalProbability;
            pageTmp[nx][newposY].condition = originalCondition;
          }
        }
      }
    }

    // Step 3: Apply the shifted notes back to the note array for the current page only
    for (unsigned int nx = pageStartX; nx <= pageEndX; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        note[nx][ny] = pageTmp[nx - pageStartX + 1][ny];
      }
    }
    // Don't set shifted = true to avoid triggering the global shift logic
  }


  if (shifted) {
    // Step 3: Copy original notes of other channels back to the note array
    // And apply shifted notes for current channel
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (original[nx][ny].channel != 0 && original[nx][ny].channel != GLOB.currentChannel) {  // Only copy if it's an 'other' channel note
          note[nx][ny] = original[nx][ny];
        } else {  // Clear spot for current channel notes or if it was empty in original
          note[nx][ny].channel = 0;
          note[nx][ny].velocity = defaultVelocity;
          note[nx][ny].probability = 100;  // Default probability
          note[nx][ny].condition = 1;      // Default condition
        }
        // Then overlay the shifted notes for the current channel
        if (tmp[nx][ny].channel == GLOB.currentChannel) {
          note[nx][ny] = tmp[nx][ny];
        }
      }
    }
    shifted = false;
  }

  /* shift only this page:*/
  
  // Reset paint/unpaint prevention flag after shiftNotes operation
  preventPaintUnpaint = false;
}
void tmpMuteAll(bool pressed) {
  if (pressed) {
    // only on the *transition* into pressed do we save & mute
    if (!tmpMuteActive) {
      // save current mutes
      for (unsigned i = 1; i <= maxY; i++) {  // Use maxY to include all channels (1-16)
        prevMuteState[i] = getMuteState(i);
      }
      // mute all except the current channel
      for (unsigned i = 1; i <= maxY; i++) {
        setMuteState(i, (i == GLOB.currentChannel) ? false : true);
      }
      tmpMuteActive = true;
    }
  } else {
    // only on the *transition* into released do we restore
    if (tmpMuteActive) {
      for (unsigned i = 1; i <= maxY; i++) {
        setMuteState(i, prevMuteState[i]);
      }
      tmpMuteActive = false;
    }
  }
}


FLASHMEM void toggleMute() {
  bool currentMuteState = getMuteState(GLOB.currentChannel);
  setMuteState(GLOB.currentChannel, !currentMuteState);
}

void deleteActiveCopy() {

  GLOB.activeCopy = false;
}



void play(bool fromStart) {
  // Send MIDI Start FIRST (before any other operations) to advance it by ~5ms
  if (MIDI_CLOCK_SEND && MIDI_TRANSPORT_SEND) {
    MIDI.sendRealTime(midi::Start);  // Send as early as possible for better sync
    
    // Send first clock pulse immediately after Start to establish timing reference
    // This is correct MIDI behavior and reduces audible delay, especially at low BPM
    // Timer will continue from there, so this is not "extra" clock, just the first one
    Serial8.write(0xF8);  // Send first clock pulse immediately (same as timer does)
  }
  
  if (CrashReport) {  // This implicitly calls operator bool() or similar if defined by CrashReportClass
    checkCrashReport();
  }

  ctrlVolumeOverlayActive = false;
  // Ensure synth voices are idle before starting
  stopSynthChannel(13);
  stopSynthChannel(14);

  if (fromStart) {
    updateLastPage();
    
    // Update encoder 1 limit if pattern mode is ON
    if (currentMode == &draw || currentMode == &singleMode) {
      if (ctrlMode == 0 && SMP_PATTERN_MODE) {
        Encoder[1].writeMax((int32_t)lastPage);
      } else if (ctrlMode == 1) {
        refreshCtrlEncoderConfig();
      }
    }
    
    lastFlowPage = 0;  // Reset FLOW page tracking when playback starts
    deleteActiveCopy();
    
    // In song mode, start from the first defined pattern
    extern bool songModeActive;
    extern uint8_t songArrangement[64];
    extern int currentSongPosition;
    
    if (songModeActive) {
      // Song mode requires PMOD to be active
      patternMode = 2;  // Ensure PMOD is set to SONG
      SMP_PATTERN_MODE = true;
      
      // Find first non-empty position in song arrangement
      currentSongPosition = -1;
      for (int i = 0; i < 64; i++) {
        if (songArrangement[i] > 0) {
          currentSongPosition = i;
          break;
        }
      }
      
      if (currentSongPosition >= 0 && songArrangement[currentSongPosition] > 0) {
        int pattern = songArrangement[currentSongPosition];
        GLOB.edit = pattern;
        GLOB.page = pattern;
        beat = (pattern - 1) * maxX + 1;  // Start from first beat of first pattern
        Serial.print("Song: Starting at position ");
        Serial.print(currentSongPosition + 1);
        Serial.print(" -> pattern ");
        Serial.println(pattern);
      } else {
        // No patterns defined, fall back to pattern mode behavior
        songModeActive = false;
        patternMode = 1;  // Set to normal pattern mode instead
        SMP_PATTERN_MODE = true;
        beat = (GLOB.edit - 1) * maxX + 1;  // Start from first beat of current page
        GLOB.page = GLOB.edit;
        Serial.println("Song: No patterns defined, falling back to pattern mode");
      }
    } else if (SMP_PATTERN_MODE) {
      // In pattern mode, start from the current page's first beat
      beat = (GLOB.edit - 1) * maxX + 1;  // Start from first beat of current page
      GLOB.page = GLOB.edit;  // Keep the current page
    } else {
      beat = 1;
      GLOB.page = 1;
    }
    
    // Reset loop count to 1 when starting playback (first loop)
    loopCount = 1;
    Serial.print("Play started: loopCount reset to ");
    Serial.println(loopCount);
    
    Encoder[2].writeRGBCode(0xFFFF00);
    if (MIDI_CLOCK_SEND) {
      // Master mode: start internal clock and play immediately on the current beat
      // Note: MIDI Start was already sent at the beginning of play() function for ~5ms advance
      resetMidiClockState();
      isNowPlaying = true;
      playStartTime = millis();

      // Fire the very first step right away so beat 1 is heard as soon as Play is pressed.
      // Subsequent steps will be driven by playTimer at regular intervals.
      if (SMP.bpm > 0.0f) {
        playNote();
      }
    } else {
      // slave-mode: arm for the next bar-1 instead of starting now
      pendingStartOnBar = true;
      isNowPlaying = false;  // It's not playing YET. MIDI clock will start it.
    }
  }
  // If !fromStart, it implies a continue, which MIDI also supports.
  // However, current logic only handles fromStart explicitly for MIDI.
  // If play(false) is ever called, MIDI.sendRealTime(midi::Continue) might be appropriate if MIDI_CLOCK_SEND.
}



void pause() {
  // Send MIDI Stop FIRST (before any other operations) to advance it by ~5ms
  if (MIDI_CLOCK_SEND && MIDI_TRANSPORT_SEND) {
    MIDI.sendRealTime(midi::Stop);  // Send as early as possible for better sync
    }
  
  // MIDI clock timer continues running in background - never stopped for precise timing
  // Don't reset clock state - timer keeps running in background
  // Only reset playback state

  ctrlVolumeOverlayActive = false;
  isNowPlaying = false;
  pendingStartOnBar = false;
  lastFlowPage = 0;  // Reset FLOW page tracking when playback stops
  updateLastPage();
  
  // Update encoder 1 limit if pattern mode is ON
  if (currentMode == &draw || currentMode == &singleMode) {
    if (ctrlMode == 0 && SMP_PATTERN_MODE) {
      Encoder[1].writeMax((int32_t)lastPage);
    } else if (ctrlMode == 1) {
      refreshCtrlEncoderConfig();
    }
  }
  
  deleteActiveCopy();
  autoSave();  // Now safe to do blocking SD card operations - timer is stopped
  envelope0.noteOff();
  // Ensure synth voices 13/14 are silenced on pause
  stopSynthChannel(13);
  stopSynthChannel(14);
  //allOff();
  Encoder[2].writeRGBCode(0x005500);
  beat = 1;      // Reset beat on pause
  beatForUI = beat;  // Keep UI timer in sync with reset position
  GLOB.page = 1;  // Reset page on pause
  loopCount = 0;  // Reset loop count to 0 on pause (will be set to 1 on next play)
  
  // Reset static tracking variables on pause to ensure clean state on next play
  // Note: static variables will be reset when play() is called and we check for page 1, beat 1
}


void playSynth(int ch, int b, int vel, bool persistant) {
  if (ch < 0 || ch >= 15) return;                // Bounds check for synths array
  if (!synths[ch][0] || !synths[ch][1]) return;  // Check if synth objects exist

  float frequency = fullFrequencies[constrain(b - ch + 13, 0, 26)];  // y-Wert ist 1-basiert, Array ist 0-basiert // b-1??
                                                                     // Constrain index for fullFrequencies
  float WaveFormVelocity = mapf(vel, 1, 127, 0.0, 1.0);


  // Ensure any prior note is released so the new trigger starts clean
  stopSynthChannel(ch);

  // Global cent shift for both oscillators (0..32 -> -24..+24 semitones)
  float centSemis = mapf(SMP.synth_settings[ch][CENT], 0, maxfilterResolution, -24.0f, 24.0f);
  float cent_ratio = pow(2.0, centSemis / 12.0);

  float detune_amount = mapf(SMP.filter_settings[ch][DETUNE], 0, maxfilterResolution, -1.0 / 12.0, 1.0 / 12.0);
  float detune_ratio = pow(2.0, detune_amount);  // e.g., 2^(1/12) ~ 1.05946 (one semitone up)
  // Octave shift: assume OCTAVE parameter is an integer (or can be cast to one)
  // For example, an OCTAVE value of 1 multiplies frequency by 2, -1 divides by 2.
  int octave_shift = mapf(SMP.filter_settings[ch][OCTAVE], 0, maxfilterResolution, -3, +3);
  float octave_ratio = pow(2.0, octave_shift);
  // Apply both adjustments to the base frequency
  synths[ch][0]->frequency(frequency * cent_ratio);
  synths[ch][1]->frequency(frequency * cent_ratio * octave_ratio * detune_ratio);
  synths[ch][0]->amplitude(WaveFormVelocity);
  synths[ch][1]->amplitude(WaveFormVelocity);
  if (!envelopes[ch]) return;
  envelopes[ch]->noteOn();

  if (persistant) {
    persistentNoteOn[ch] = true;
  } else {
    persistentNoteOn[ch] = false;
  }

  //unsigned long delay_ms = mapf(SMP.param_settings[ch][DELAY], 0, maxfilterResolution, 0, maxParamVal[DELAY]);
  startTime[ch] = millis();    // + delay_ms;    // Record the start time
  noteOnTriggered[ch] = true;  // Set the flag so we don't trigger noteOn again
}


// Adjust beat offset when manually switching pages in pattern mode:
// Place this logic in your encoder handler where GLOB.edit (page) changes:
void handlePageSwitch(int newEdit) {
  unsigned int oldEdit = GLOB.edit;
  if (newEdit != oldEdit) {
    if (SMP_PATTERN_MODE) {
      unsigned int oldStart = (oldEdit - 1) * maxX + 1;
      unsigned int newStart = (newEdit - 1) * maxX + 1;
      unsigned int offset   = beat - oldStart;
      // Preserve relative position within page:
      beat = newStart + offset;
      GLOB.page = newEdit;
    }
    GLOB.edit = newEdit;
  }
}

void playNote() {

  // Remember which beat is actually being played for UI highlighting
  beatForUI = beat;

  // Handle count-in pending: wait for next beat 1 (full bar) before starting count-in
  if (countInPending && isNowPlaying) {
    unsigned int currentBeatInBar = ((beat - 1) % maxX) + 1;
    
    // When we reach beat 1, start the count-in (wait complete - full bar has passed)
    if (currentBeatInBar == 1) {
      countInPending = false;  // No longer pending
      countInActive = true;    // Start showing count-in
      countInBeat = 0;         // Reset count-in
      countInComplete = false; // Reset completion flag
    }
  }

  // Handle count-in for ON1 mode
  if (countInActive && isNowPlaying) {
    unsigned int currentBeatInBar = ((beat - 1) % maxX) + 1;
    
    // Count-in should span a full bar (maxX beats), showing 1,2,3,4 spread evenly
    // Show 1 at beat 1, 2 at beat (1 + maxX/4), 3 at beat (1 + 2*maxX/4), 4 at beat (1 + 3*maxX/4)
    // For maxX=16: beats 1,5,9,13 show 1,2,3,4, then start recording at beat 1 of next bar
    unsigned int beatInterval = maxX / 4;  // 4 for maxX=16
    unsigned int countInNumber = ((currentBeatInBar - 1) / beatInterval) + 1;
    
    // Update countInBeat to track which count-in number we're showing (1-4)
    if (countInNumber <= 4) {
      countInBeat = countInNumber;
      
      // Mark count-in as complete when we show "4"
      if (countInNumber == 4) {
        countInComplete = true;
      }
    } else {
      // Beyond beat 13 (for maxX=16), count-in is complete
      countInBeat = 0;
    }
    
    // When we're at beat 1 again after showing "4", start recording
    if (currentBeatInBar == 1 && countInComplete) {
      // Count-in complete (shown 1,2,3,4) - start recording at beat 1
      countInActive = false;
      countInBeat = 0;
      countInComplete = false;
      // Clear all triggers from selected channel
      clearAllNotesOfChannel();
      // Track starting beat for auto-stop before reaching it again
      recordingStartBeat = beat;
      recordingBeatCount = 0;  // Reset absolute beat counter
      // Start fast recording (note at x=1 will be set after recording completes)
      startFastRecord();
    }
  }

  // Auto-stop recording at loop end (ONLY for ON1 mode)
  // Recording should record from beat 1 until the loop end is reached
  // Stop AFTER loop end is recorded, BEFORE beat 1 (ONE) plays again
  if (fastRecordActive && recChannelClear == 3 && recordingStartBeat > 0 && isNowPlaying) {
    unsigned int currentBeatInBar = ((beat - 1) % maxX) + 1;
    
    // Increment absolute beat counter (doesn't wrap, unlike beat in pattern mode)
    recordingBeatCount++;
    
    // Calculate loop length in beats
    // If loopLength > 0, use it (in pages), otherwise use lastPage (actual pattern length)
    extern int loopLength;
    unsigned int loopEndBeat;
    if (loopLength > 0) {
      loopEndBeat = loopLength * maxX;  // Loop length in beats
    } else {
      extern unsigned int lastPage;
      loopEndBeat = lastPage * maxX;  // Pattern length in beats
    }
    
    // Stop AFTER recording the full loop (including the complete last beat)
    // We check at the start of playNote(), so:
    // - Recording starts at beat 1, recordingBeatCount = 0
    // - Each call increments recordingBeatCount (absolute, doesn't wrap)
    // - When recordingBeatCount = loopEndBeat, we've recorded beats 1 through (loopEndBeat - 1)
    // - When recordingBeatCount = loopEndBeat + 1, we've recorded beats 1 through loopEndBeat
    //   At this point we're about to play beat (loopEndBeat + 1), which is beat 1 again
    //   Stop here to capture the complete loop including the full audio of the last beat
    // Defer actual stop to main loop to avoid blocking timer interrupt
    if (currentBeatInBar == 1 && recordingBeatCount >= loopEndBeat + 1) {
      pendingStopFastRecord = true;  // Defer to main loop
      recordingStartBeat = 0;  // Reset tracking
      recordingBeatCount = 0;  // Reset counter
    }
  }

  if (currentMode == &draw || currentMode == &singleMode || currentMode == &velocity) {


    /*if (patternChangeActive){
    if (millis() <= patternChangeTime) {          
          //Serial.println(editpage);
    }else{patternChangeActive=false;}
    }
    drawPatternChange(GLOB.edit);
    */
  }

  onBeatTick();
  
  // Process MIDI again before playing sequencer notes to reduce external MIDI latency
  checkMidi();

  if (isNowPlaying) {
    drawPlayButton();

    for (unsigned int b = 1; b < maxY + 1; b++) {  // b is 1-indexed (row on grid)
      if (beat > 0 && beat <= maxlen) {            // Ensure beat is within valid range for note array
        int ch = note[beat][b].channel;            // ch is 0-indexed for internal use (e.g. SMP arrays)
        int vel = note[beat][b].velocity;
        uint8_t prob = note[beat][b].probability;   // Get probability (0-100)
        uint8_t cond = note[beat][b].condition;     // Get condition (1, 2, 4 for 1, 1/2, 1/4)
        if (cond == 0) cond = 1;  // Default to 1 if not set
        
        if (ch > 0 && !getMuteState(ch)) {  // Use new per-page mute system when PMOD is enabled
          
          // Check condition - skip if not the right loop iteration
          if (cond > 1) {
            bool shouldPlay = false;
            if (cond <= 16) {
              // 1/X conditions: play when (loopCount % cond) == 0
              // 1/2: every 2nd loop (2, 4, 6, 8...)
              // 1/4: every 4th loop (4, 8, 12, 16...)
              shouldPlay = (loopCount % cond) == 0;
            } else {
              // X/1 conditions: play on every Xth loop, starting with the first
              // 2/1: every 2nd loop starting with first (1, 3, 5, 7...)
              // 4/1: every 4th loop starting with first (1, 5, 9, 13...)
              // Values: 17=2/1, 18=4/1, 19=8/1, 20=16/1
              // Map: 17->2, 18->4, 19->8, 20->16
              uint8_t x = (cond == 17) ? 2 : (cond == 18) ? 4 : (cond == 19) ? 8 : 16;
              shouldPlay = (loopCount % x) == 1;
            }
            
            if (!shouldPlay) {
              continue;  // Skip this note based on condition
            }
          }
          
          // Check probability - if random(0-99) >= probability, skip this note
          if (prob < 100) {
            int randValue = random(100);  // Generate random value 0-99
            if (randValue >= prob) {
              continue;  // Skip this note based on probability
            }
          }

          // Trigger MIDI and audio as close together as possible.
          // NOTE: 'ch' is stored 1-based in 'note' (1=voice1, 2=voice2, etc.), and MIDI channels are 1-16.
          // 'b' is the grid row (1-16) which MidiSendNoteOn maps to a MIDI note.
          MidiSendNoteOn(b, ch, vel);
          if (ch < 9) {                                                    // Sample channels (0-8 are _samplers[0] to _samplers[8])
            int pitch = (12 * SampleRate[ch]) +  b - (ch + 1);
            
            // Apply detune offset for channels 1-12 (excluding synth channels 13-14)
            if (ch >= 1 && ch <= 12) {
              pitch += (int)detune[ch]; // Add detune semitones
            }
            
            // Apply octave offset for channels 1-8 (excluding synth channels 13-14)
            if (ch >= 1 && ch <= 8) {
              pitch += (int)(channelOctave[ch] * 12); // Add octave semitones (12 semitones per octave)
            }
            
            _samplers[ch].noteEvent(pitch, vel, true, true);
          } else if (ch == 11) {  // Assuming ch 11 is a specific synth
            // `octave[0]` and `transpose` affect pitch. `b` is grid row (1-16).
            // playSound expects MIDI note number (0-indexed pitch offset from row)
            playSound(12 * (int)octave[0] + transpose + (b - 1), 0);  // b-1 to match paint preview
            
          } else if (ch >= 13 && ch < 15) {                               // Synth channels 13, 14
            playSynth(ch, b, vel, false);                                 // b is 1-indexed for grid row
          }
        }
      }
      yield();
    }

    // midi functions
    if (waitForFourBars && pulseCount >= totalPulsesToWait) {
      if (SMP_PATTERN_MODE) {
        beat = (GLOB.edit - 1) * maxX + 1;  // Start from first beat of current page
        GLOB.page = GLOB.edit;  // Keep the current page
      } else {
        beat = 1;
        GLOB.page = 1;
      }
      if (fastRecordActive) stopFastRecord();
      isNowPlaying = true;  // Should already be true if MIDI clock started it
      //Serial.println("4 Bars Reached");
      waitForFourBars = false;  // Reset for the next start message
    }

    yield();
    beatStartTime = millis();
     if (SMP_PATTERN_MODE) {
    extern bool songModeActive;
    
    // Compute the bounds of the current page:
    unsigned int pageStart = (GLOB.edit - 1) * maxX + 1;
    unsigned int pageEnd   = pageStart + maxX - 1;

    // Preserve relative beat when switching pages (but NOT in song mode - let song handle beat position)
    static unsigned int lastEdit = GLOB.edit;
    if (GLOB.edit != lastEdit && !songModeActive) {
      unsigned int oldStart = (lastEdit - 1) * maxX + 1;
      unsigned int offset   = beat - oldStart;
      beat = pageStart + offset;
      lastEdit = GLOB.edit;
    }
    
    // Update lastEdit tracker in song mode too
    if (songModeActive) {
      lastEdit = GLOB.edit;
    }

    // Advance and wrap within this page:
    // Track previous state BEFORE incrementing
    static unsigned int lastPageAfterWrap = 0;
    static unsigned int lastBeatAfterWrap = 0;
    unsigned int previousPage = GLOB.page;
    unsigned int previousBeat = beat;
    
    beat++;
    
    if (songModeActive) {
      // In song mode, check if we need to advance to next pattern
      if (beat > pageEnd) {
        // Pattern finished - advance to next pattern in song
        Serial.print("Pattern finished at beat ");
        Serial.println(beat);
        checkPages();
      } else {
        // Still within pattern - just keep displaying current pattern
        GLOB.page = GLOB.edit;
      }
    } else {
      // Normal pattern mode - wrap within page
      if (beat < pageStart || beat > pageEnd) {
        beat = pageStart;
      }
    }
    
    // Update page after wrapping
      GLOB.page = GLOB.edit;
    
    // Simple loopCount logic: increment when we transition TO page 1, beat 1 while playing
    static bool wasAtStart = false;  // Track if we were at page 1, beat 1 in previous call
    
    if (!isNowPlaying) {
      wasAtStart = false;  // Reset when not playing
    } else {
      bool atStart = (GLOB.page == 1 && beat == 1);
      // Increment if we just arrived at start (weren't here before, but are now)
      if (atStart && !wasAtStart) {
        loopCount++;
        if (loopCount > 256) loopCount = 1;  // Prevent overflow
        Serial.print("Loop count incremented: loopCount=");
        Serial.println(loopCount);
      }
      wasAtStart = atStart;
    }
  } else {
    // Fallback to default behavior:
    beat++;
    checkPages();
    // Simple loopCount logic for non-pattern mode: increment when we transition TO page 1, beat 1
    static bool wasAtStartNP = false;  // Track if we were at page 1, beat 1 in previous call
    
    if (!isNowPlaying) {
      wasAtStartNP = false;  // Reset when not playing
    } else {
      bool atStartNP = (GLOB.page == 1 && beat == 1);
      // Increment if we just arrived at start (weren't here before, but are now)
      if (atStartNP && !wasAtStartNP) {
        loopCount++;
        if (loopCount > 256) loopCount = 1;  // Prevent overflow
        Serial.print("Loop count incremented: loopCount=");
        Serial.println(loopCount);
      }
      wasAtStartNP = atStartNP;
    }
  }


  }
    for (int ch = 13; ch <= 14; ch++) {  // Only for synth channels 13, 14
      if (noteOnTriggered[ch] && !persistentNoteOn[ch]) {
        float noteLen = getNoteDuration(ch);                           // Calculate duration
        if ((millis() - startTime[ch] >= noteLen)) {  // Cast noteLen to ulong for comparison
          if (!envelopes[ch]) continue;
          envelopes[ch]->noteOff();
          noteOnTriggered[ch] = false;
        }
      }
    }


  yield();
}

void checkPages() {
  extern bool songModeActive;
  extern uint8_t songArrangement[64];
  extern int currentSongPosition;
  
  // Check song mode FIRST, before SMP_PATTERN_MODE
  if (songModeActive) {
    // Song mode: play through the song arrangement
    Serial.println("Song checkPages called - advancing to next pattern");
    
    // Safety check: if currentSongPosition is invalid, try to find first valid position
    if (currentSongPosition < 0) {
      currentSongPosition = 0;
      for (int i = 0; i < 64; i++) {
        if (songArrangement[i] > 0) {
          currentSongPosition = i;
          break;
        }
      }
    }
    
    // Move to next position in song
    int startPos = currentSongPosition;
    int attempts = 0;
    do {
      currentSongPosition++;
      if (currentSongPosition >= 64) {
        currentSongPosition = 0;  // Loop back to start
      }
      attempts++;
      // Stop if we've checked all 64 positions
      if (attempts >= 64 || currentSongPosition == startPos) {
        break;
      }
    } while (songArrangement[currentSongPosition] == 0);  // Skip empty slots
    
    // Get the pattern at this position
    int pattern = songArrangement[currentSongPosition];
    if (pattern > 0 && pattern <= 16) {
      GLOB.edit = pattern;
      GLOB.page = pattern;
      beat = (pattern - 1) * maxX + 1;  // Start at beginning of this pattern
      // No loopCount changes here; handled centrally in playNote()
      Serial.print("Song: Moving to position ");
      Serial.print(currentSongPosition + 1);
      Serial.print(" -> pattern ");
      Serial.println(pattern);
    } else {
      // No valid pattern found in entire song - disable song mode and fall back to pattern mode
      Serial.println("ERROR: No valid patterns in song arrangement - disabling song mode");
      songModeActive = false;
      patternMode = 1;  // Fall back to pattern mode
      SMP_PATTERN_MODE = true;
      beat = (GLOB.edit - 1) * maxX + 1;  // Start from first beat of current page
      GLOB.page = GLOB.edit;
    }
    
    return;
  }
  
  if (SMP_PATTERN_MODE) {
    // Always display the editable page when looping in pattern mode.
    GLOB.page = GLOB.edit;
    return;
  }
  
  if (SMP_FLOW_MODE && isNowPlaying) {
    // FLOW mode: follow the timer position when playing
    uint16_t timerPage = (beat - 1) / maxX + 1;
    
    // Only update lastPage when crossing page boundaries or at beat 1 (when looping)
    // Skip update if fastrecord is active to save CPU cycles
    extern bool fastRecordActive;
    static uint16_t lastTimerPage = 0;
    if ((timerPage != lastTimerPage || beat == 1) && !fastRecordActive) {
    updateLastPage();  // recompute the highest page that actually has notes
      lastTimerPage = timerPage;
    } else if (timerPage != lastTimerPage) {
      lastTimerPage = timerPage;  // Update tracking even if we skip updateLastPage
    }
    
    if (timerPage > lastPage && lastPage > 0) {
      // If we stepped past the last non-empty page, restart at the top
      beat = 1;
      timerPage = 1;
      // loopCount handled centrally in playNote()
    } else if (lastPage == 0) {
      // Should not happen if updateLastPage ensures it's at least 1
      beat = 1;
      timerPage = 1;
      // loopCount handled centrally in playNote()
    }
    
    // Update the page
    GLOB.page = timerPage;
    
    // Only update encoder if the page actually changed
    if (timerPage != lastFlowPage && (currentMode == &draw || currentMode == &singleMode)) {
      currentMode->pos[1] = timerPage;  // Update encoder 1 position
      Encoder[1].writeCounter((int32_t)timerPage);  // Update the physical encoder
      lastFlowPage = timerPage;  // Remember this page
    }
    return;
  }
  
  // Normal mode: only update lastPage when crossing page boundaries or looping
  // Skip update if fastrecord is active to save CPU cycles
  extern bool fastRecordActive;
  uint16_t newPage = (beat - 1) / maxX + 1;
  static uint16_t lastCheckedPage = 0;
  
  // Only call updateLastPage() when page changes or when we're at beat 1 (looping)
  // But never during fastrecord to avoid CPU overhead
  if ((newPage != lastCheckedPage || beat == 1) && !fastRecordActive) {
    updateLastPage();  // recompute the highest page that actually has notes
    lastCheckedPage = newPage;
  } else if (newPage != lastCheckedPage) {
    lastCheckedPage = newPage;  // Update tracking even if we skip updateLastPage
  }

  // if we stepped past the last non-empty page, restart at the top
  if (newPage > lastPage && lastPage > 0) {  // Ensure lastPage is valid before comparison
    beat = 1;
    //if (fastRecordActive) stopFastRecord(); // This might stop recording prematurely if looping
    newPage = 1;
    lastCheckedPage = 0;  // Reset to force update on next check
  } else if (lastPage == 0) {  // Should not happen if updateLastPage ensures it's at least 1
    beat = 1;
    newPage = 1;
    lastCheckedPage = 0;
  }
  GLOB.page = newPage;
}


void unpaint() {
  //GLOB.edit = 1; // This line seems to do nothing or is misintended. editpage is used for current page.
  paintMode = false;
  preventPaintUnpaint = false;  // Reset flag when unpaint function is called
  // GLOB.x is already global X (1 to maxlen-1), GLOB.y is global Y (1 to maxY)
  // No need to recalculate from GLOB.edit and GLOB.x for the current view
  // unsigned int x_coord = (GLOB.edit - 1) * maxX + GLOB.x; // This is if GLOB.x was page-local
  unsigned int current_x = GLOB.x;  // Use the global cursor X
  unsigned int current_y = GLOB.y;  // Use the global cursor Y


  if ((current_y > 0 && current_y <= maxY)) {  // y is 1-based, 1 to 16.
                                               // Row 1 (GLOB.y==1) is often special (e.g. transport)
                                               // The condition was (y > 1 && y < 16) previously, meaning rows 2-15.
                                               // Assuming general unpaint for rows 1-15, and 16 is special.
    if (current_y < 16) {                      // Rows 1-15 for notes
      if (!GLOB.singleMode) {
        if (simpleNotesView == 1) {
          // In simple notes view, find the voice that should be at this Y position (voice = Y-1)
          int voiceToUnpaint = current_y - 1;  // Y position 3 = voice 2, Y position 4 = voice 3, etc.
          if (voiceToUnpaint >= 0 && voiceToUnpaint < maxY) {
            // Unpaint all notes of this voice at this X position
            for (unsigned int y = 1; y <= maxY; y++) {
              if (note[current_x][y].channel == voiceToUnpaint) {
                note[current_x][y].channel = 0;
                note[current_x][y].velocity = defaultVelocity;
              }
            }
          }
        } else {
          // Normal unpaint behavior
          note[current_x][current_y].channel = 0;
          note[current_x][current_y].velocity = defaultVelocity;
        }
      } else {
        if (note[current_x][current_y].channel == GLOB.currentChannel) {
          note[current_x][current_y].channel = 0;
          note[current_x][current_y].velocity = defaultVelocity;
        }
      }
    } else if (current_y == 16) {  // Row 16 (top row)
      clearPageX(current_x);       // Clear entire column x if cursor is on top row
    }
  }
  updateLastPage();
  
  // Update encoder 1 limit if pattern mode is ON or if in single mode
  if (currentMode == &draw || currentMode == &singleMode) {
    if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode)) {
      Encoder[1].writeMax((int32_t)lastPage);
    } else if (ctrlMode == 1) {
      refreshCtrlEncoderConfig();
    }
  }
  
  FastLEDshow();  // Update display immediately
}


void triggerGridNote(unsigned int globalX, unsigned int y) {
  if (globalX < 1 || globalX > maxlen || y < 1 || y > maxY) return;

  Note& cell = note[globalX][y];
  int channel = cell.channel;
  if (channel == 0) return;
  
  // Don't trigger muted voices
  if (getMuteState(channel)) return;

  int velocity = cell.velocity > 0 ? cell.velocity : defaultVelocity;
  int pitch_from_row = y;

  if (channel > 0 && channel < 9) {
    int pitch = (12 * SampleRate[channel]) + pitch_from_row - (channel + 1);

    if (channel >= 1 && channel <= 12) {
      pitch += static_cast<int>(detune[channel]);
    }

    if (channel >= 1 && channel <= 8) {
      pitch += static_cast<int>(channelOctave[channel] * 12);
    }

    _samplers[channel].noteEvent(pitch, velocity, true, true);
  } else if (channel == 11) {
    playSound((12 * static_cast<int>(octave[0])) + transpose + (pitch_from_row - 1), 0);
  } else if (channel >= 13 && channel < 15) {
    playSynth(channel, pitch_from_row, velocity, false);
  }
}


void paint() {
  //GLOB.edit = 1; // Same as in unpaint, probably not intended here.
  //Serial.println("!!!PAINT!");
  preventPaintUnpaint = false;  // Reset flag when paint function is called

  unsigned int current_x = GLOB.x;  // Use global cursor X
  unsigned int current_y = GLOB.y;  // Use global cursor Y

  bool channelBlocked = (GLOB.currentChannel == 9 || GLOB.currentChannel == 10 || GLOB.currentChannel == 12);

  if (!GLOB.singleMode) {  // Draw mode
    // current_y is 1-16. GLOB.currentChannel is 0-14.
    // If painting on grid rows 1-15 (GLOB.y from encoder), channel is GLOB.y - 1.
    // Original condition: (y > 1 && y <= 9) || (y == 12) || (y > 13 && y <= 15)
    // This implies specific rows map to specific channel types/groups.
    // For simplicity, let's assume if current_y maps to a valid channel (1-15), we paint it.
    // GLOB.currentChannel is already set based on GLOB.y in checkEncoders for draw mode.
    if (current_y > 0 && current_y <= 15) {                       // Grid rows 1-15
      if (!channelBlocked && note[current_x][current_y].channel == 0) {              // Only paint if empty
        note[current_x][current_y].channel = GLOB.currentChannel;  // GLOB.currentChannel should be correct 0-indexed channel
        note[current_x][current_y].velocity = defaultVelocity;
        note[current_x][current_y].probability = 100;  // Default 100% probability
        note[current_x][current_y].condition = 1;      // Default condition: 1 (every loop)
      }
    } else if (current_y == 16) {  // Top row (GLOB.y == 16)
      toggleCopyPaste();
    }
  } else {                                     // Single mode (painting for the globally selected GLOB.currentChannel)
    if ((current_y > 0 && current_y <= 15)) {  // Grid rows 1-15
      if (!channelBlocked) {
        note[current_x][current_y].channel = GLOB.currentChannel;
        note[current_x][current_y].velocity = defaultVelocity;
        note[current_x][current_y].probability = 100;  // Default 100% probability
        note[current_x][current_y].condition = 1;      // Default condition: 1 (every loop)
      }
    } else if (current_y == 16) {  // Top row (GLOB.y == 16) - enable copypaste in single mode
      toggleCopyPaste();
    }
  }

  // Visual feedback if channel goes out of bounds (original logic)
  if (note[current_x][current_y].channel > maxY - 2) {  // maxY-2 is 14. Channels 0-14.
    note[current_x][current_y].channel = 0;             // Reset to channel 0 (sampler 1, typically kick)
                                                        // Original used 1, but 0 seems more consistent for 0-indexed channels.
    for (unsigned int vx = 1; vx < maxX + 1; vx++) {    // Light up the row
      light(vx, note[current_x][current_y].channel + 1, col[note[current_x][current_y].channel] * 12);
    }
    FastLEDshow();
  }

  // Play sound on paint if not currently playing sequence
  if (!isNowPlaying) {
    int painted_channel = note[current_x][current_y].channel;
    int painted_velocity = note[current_x][current_y].velocity;
    int pitch_from_row = current_y;  // 1-16

    if (painted_channel > 0 && painted_channel < 9) {  // Sampler channels
        // Play sample as normal
        int pitch = 12 * SampleRate[painted_channel] + pitch_from_row - (painted_channel + 1);
        
        // Apply detune offset for channels 1-12 (excluding synth channels 13-14)
        if (painted_channel >= 1 && painted_channel <= 12) {
          pitch += (int)detune[painted_channel]; // Add detune semitones
        }
        
        // Apply octave offset for channels 1-8 (excluding synth channels 13-14)
        if (painted_channel >= 1 && painted_channel <= 8) {
          pitch += (int)(channelOctave[painted_channel] * 12); // Add octave semitones (12 semitones per octave)
        }
        
        _samplers[painted_channel].noteEvent(pitch, painted_velocity, true, true);
    } else if (painted_channel == 11) {  // Specific synth
      playSound((12 * (int)octave[0]) + transpose + (pitch_from_row - 1), 0);
    } else if (painted_channel >= 13 && painted_channel < 15) {            // General synths
      playSynth(painted_channel, pitch_from_row, painted_velocity, false); 
                                                                           
    }
  }

  updateLastPage();
  
  // Update encoder 1 limit if pattern mode is ON or if in single mode
  if (currentMode == &draw || currentMode == &singleMode) {
    if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode)) {
      Encoder[1].writeMax((int32_t)lastPage);
    } else if (ctrlMode == 1) {
      refreshCtrlEncoderConfig();
    }
  }
  
  FastLEDshow();
}





void toggleCopyPaste() {

  //GLOB.edit = 1;
  if (!GLOB.activeCopy) {

    // copy the pattern into the memory
    Serial.print("copy now");
    unsigned int src = 0;
    for (unsigned int c = ((GLOB.edit - 1) * maxX) + 1; c < ((GLOB.edit - 1) * maxX) + (maxX + 1); c++) {  // maxy?
      src++;
      for (unsigned int y = 1; y < maxY + 1; y++) {
        if (GLOB.singleMode) {
          // In single mode, only copy notes for the current channel
          if (note[c][y].channel == GLOB.currentChannel) {
            tmp[src][y] = note[c][y];
          } else {
            // Clear other channels in the temp buffer
            tmp[src][y].channel = 0;
            tmp[src][y].velocity = defaultVelocity;
            tmp[src][y].probability = 100;  // Default probability
            tmp[src][y].condition = 1;      // Default condition
          }
        } else {
          // In draw mode, copy all notes
          tmp[src][y] = note[c][y];
        }
      }
    }
    // Store the source channel for cross-channel pasting
    GLOB.copyChannel = GLOB.currentChannel;
  } else {

    // paste the memory into the song
    Serial.print("paste here!");
    unsigned int src = 0;
    for (unsigned int c = ((GLOB.edit - 1) * maxX) + 1; c < ((GLOB.edit - 1) * maxX) + (maxX + 1); c++) {
      src++;
      for (unsigned int y = 1; y < maxY + 1; y++) {
        if (GLOB.singleMode) {
          // In single mode, first clear any existing notes for the current channel
          if (note[c][y].channel == GLOB.currentChannel) {
            note[c][y].channel = 0;
            note[c][y].velocity = defaultVelocity;
            note[c][y].probability = 100;  // Default probability
            note[c][y].condition = 1;      // Default condition
          }
          // Then paste notes from the copied channel to the current channel
          if (tmp[src][y].channel == GLOB.copyChannel) {
            note[c][y].channel = GLOB.currentChannel;
            note[c][y].velocity = tmp[src][y].velocity;
            note[c][y].probability = tmp[src][y].probability;
            note[c][y].condition = tmp[src][y].condition;
          }
          // Don't modify other channels when pasting in single mode
        } else {
          // In draw mode, paste all notes
          note[c][y] = tmp[src][y];
        }
      }
    }
  }
  updateLastPage();
  
  // Update encoder 1 limit if pattern mode is ON or if in single mode
  if (currentMode == &draw || currentMode == &singleMode) {
    if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode)) {
      Encoder[1].writeMax((int32_t)lastPage);
    } else if (ctrlMode == 1) {
      refreshCtrlEncoderConfig();
    }
  }
  
  GLOB.activeCopy = !GLOB.activeCopy;  // Toggle the boolean value
}



void clearNoteChannel(unsigned int c, unsigned int yStart, unsigned int yEnd, unsigned int channel_to_clear, bool singleModeActive) {
  // c is x-coordinate (1 to maxlen)
  // yStart, yEnd are 1-based y-coordinates
  // channel_to_clear is 0-indexed channel
  for (unsigned int y = yStart; y <= yEnd; y++) {      // Iterate up to yEnd inclusive
    if (c > 0 && c <= maxlen && y > 0 && y <= maxY) {  // Bounds check
      if (singleModeActive) {
        if (note[c][y].channel == channel_to_clear) {
          note[c][y].channel = 0;
          note[c][y].velocity = defaultVelocity;
          note[c][y].probability = 100;  // Default probability
          note[c][y].condition = 1;      // Default condition
        }
      } else {  // Clear all notes in the range on column c regardless of their channel
        note[c][y].channel = 0;
        note[c][y].velocity = defaultVelocity;
        note[c][y].probability = 100;  // Default probability
        note[c][y].condition = 1;      // Default condition
      }
    }
  }
  if (channel_to_clear == 13 || channel_to_clear == 14) {
    stopSynthChannel(channel_to_clear);
  }
}




void updateVolume() {
  GLOB.vol = currentMode->pos[2];
  float vol = float(GLOB.vol / 10.0);
  //Serial.println("Vol: " + String(vol));
  if (vol >= 0.0 && vol <= 1.0) sgtl5000_1.volume(vol);  // Ensure vol is in valid range
}

FLASHMEM void updatePreviewVolume() {
  unsigned int level = constrain(previewVol, 0u, 50u);
  // Map previewVol (0-50) to gain (0.0-0.5): vol = previewVol * 0.01f
  float gain = (float)level * 0.01f;  // Map 0→0.0, 50→0.5
  
  // Apply PREV volume to both preview paths:
  // - sound0 (voice0) → envelope0 → mixer0 channel 0
  // - playSdWav1 → ampPreview → mixer0 channel 1
  // Both paths should have the same total gain for consistent volume
  mixer0.gain(0, gain * 0.8f);  // sound0 preview volume (20% reduction)
  mixer0.gain(1, gain);  // ampPreview path - apply same gain as voice0
  ampPreview.gain(1.0f);  // Set ampPreview to unity gain (volume controlled by mixer0)
}

FLASHMEM void updateLineOutLevel() {
  unsigned int newLevel = constrain(currentMode->pos[0], LINEOUT_MIN, LINEOUT_MAX);
  if (newLevel != currentMode->pos[0]) {
    currentMode->pos[0] = newLevel;
    Encoder[0].writeCounter((int32_t)newLevel);
  }
  if (lineOutLevelSetting != newLevel) {
    lineOutLevelSetting = newLevel;
    EEPROM.write(EEPROM_DATA_START + 15, lineOutLevelSetting);  // Save to EEPROM
  }
  sgtl5000_1.lineOutLevel(lineOutLevelSetting);
}

void updateBrightness() {
  // Original: ledBrightness = (currentMode->pos[1] * 10) + 4 - 50;
  // This seems to map encoder pos[1] (range 6-25 for volume_bpm mode) to brightness
  // If pos[1] is 6 -> 60+4-50 = 14
  // If pos[1] is 25 -> 250+4-50 = 204
  // Brightness for FastLED is 0-255.
  ledBrightness = constrain((currentMode->pos[1] * 10) - 46, 10, 255);  // Simplified and constrained
  //Serial.println("Brightness: " + String(ledBrightness));
  FastLED.setBrightness(ledBrightness);
}

FLASHMEM void switchSubPattern() {
  FastLEDclear();

  if (muteModeActive) {
    Mode *savedMode = currentMode;
    bool savedSingle = GLOB.singleMode;

    currentMode = muteModeReturnSingleState ? &singleMode : &draw;
    GLOB.singleMode = muteModeReturnSingleState;

    drawBase();
    drawTriggers();
    if (isNowPlaying) {
      drawTimer();
    }
    drawCursor();

    currentMode = savedMode;
    GLOB.singleMode = savedSingle;

    if (muteModeArrowDirection != 0) {
      if (millis() <= muteModeArrowUntil) {
        const char *arrow = (muteModeArrowDirection > 0) ? ">>" : "<<";
        drawText(arrow, 7, 8, CRGB(255, 255, 255));
      } else {
        muteModeArrowDirection = 0;
        muteModeArrowUntil = 0;
        muteModeEncoderValue = 0;
        currentMode->pos[2] = 0;
        Encoder[2].writeCounter((int32_t)0);
      }
    }

    drawIndicator('L', 'W', 3);
    Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    CRGB whiteColor = getIndicatorColor('W');
    Encoder[2].writeRGBCode(whiteColor.r << 16 | whiteColor.g << 8 | whiteColor.b);
    Encoder[1].writeRGBCode(0x000000);
    Encoder[3].writeRGBCode(0x000000);
  } else {
    drawText("SUB", 2, 10, CRGB(255, 0, 255));

    char subpatternText[8];
    sprintf(subpatternText, "SP %d", GLOB.subpattern);
    drawText(subpatternText, 2, 3, CRGB(0, 255, 255));

    drawIndicator('L', 'M', 1);

    CRGB magentaColor = getIndicatorColor('M');

    Encoder[0].writeRGBCode(0x000000);
    Encoder[1].writeRGBCode(magentaColor.r << 16 | magentaColor.g << 8 | magentaColor.b);
    Encoder[2].writeRGBCode(0x000000);
    Encoder[3].writeRGBCode(0x000000);
  }

  FastLEDshow();
}

static void reverseChannelInMemory(uint8_t channel) {
  if (channel >= maxFiles) return;
  uint32_t sampleCount = loadedSampleLen[channel];
  if (sampleCount <= 1) return;

  int16_t *buffer = reinterpret_cast<int16_t *>(sampled[channel]);
  for (uint32_t i = 0, j = sampleCount - 1; i < j; ++i, --j) {
    int16_t temp = buffer[i];
    buffer[i] = buffer[j];
    buffer[j] = temp;
  }
}

static void refreshSamplerChannel(uint8_t channel) {
  if (channel >= maxFiles) return;
  uint32_t sampleCount = loadedSampleLen[channel];
  if (sampleCount == 0) return;

  int16_t *buffer = reinterpret_cast<int16_t *>(sampled[channel]);
  _samplers[channel].removeAllSamples();
  _samplers[channel].addSample(36, buffer, sampleCount, rateFactor);
}

static void applyChannelDirection(uint8_t channel, int8_t targetDir) {
  if (channel >= maxFiles) return;
  int8_t normalizedDir = (targetDir >= 0) ? 1 : -1;
  if (channelDirection[channel] == normalizedDir) {
    return;
  }

  reverseChannelInMemory(channel);
  channelDirection[channel] = normalizedDir;
  refreshSamplerChannel(channel);
}

FLASHMEM void updateBPM() {
  if (MIDI_CLOCK_SEND) {
    
    //Serial.println("BPM: " + String(currentMode->pos[3]));
    SMP.bpm = currentMode->pos[3];                                    // BPM from encoder
    if (SMP.bpm > 0) {                                                // Avoid division by zero
      playNoteInterval = ((60.0 * 1000.0 / SMP.bpm) / 4.0) * 1000.0;  // Use floats for precision
      playTimer.update(playNoteInterval);
      //midiTimer.update(playNoteInterval);  // If midiTimer exists and shares interval
      
      // Update MIDI clock output immediately with exact rounded BPM value
      extern void updateMidiClockOutput();
      updateMidiClockOutput();
    }
  }
  drawBPMScreen();  // Assumed to exist for visual feedback
}

FLASHMEM void setVolume() {
  //showExit(0);
  drawBPMScreen();  // Assumed to exist

  // Track previous encoder positions to only update when they change
  static unsigned int lastPos1 = 0;
  static int lastPos2 = -1;
  static Mode *lastMode = nullptr;
  
  extern int clockMode;
  
  // Reset tracking when switching to volume_bpm mode
  if (lastMode != currentMode) {
    lastPos1 = 0;
    lastPos2 = -1;
    lastMode = currentMode;
  }
  
  // Initialize encoder[2] if not set or if clockMode changed externally
  if (lastPos2 == -1) {
    // Initialize encoder[2] based on current clockMode (1=INT, -1=EXT -> 0=EXT, 1=INT)
    currentMode->pos[2] = (clockMode == 1) ? 1 : 0;
    Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
    lastPos2 = currentMode->pos[2];
    lastPos1 = currentMode->pos[1];
  }
  
  // Handle encoder 1 (brightness)
  if (currentMode->pos[1] != lastPos1) {
    updateBrightness();
    lastPos1 = currentMode->pos[1];
  }
  
  // Handle encoder 2 (MIDI INT/EXT)
  if (currentMode->pos[2] != lastPos2) {
    // Constrain to 0 or 1 (toggle between EXT and INT)
    int newPos2 = constrain(currentMode->pos[2], 0, 1);
    if (newPos2 != currentMode->pos[2]) {
      currentMode->pos[2] = newPos2;
      Encoder[2].writeCounter((int32_t)newPos2);
    }
    
    // Update clockMode: 0 = EXT (-1), 1 = INT (1)
    clockMode = (newPos2 == 1) ? 1 : -1;
    
    // Save to EEPROM
    extern void saveSingleModeToEEPROM(int index, int8_t value);
    saveSingleModeToEEPROM(1, clockMode);
    
    // Update MIDI_CLOCK_SEND flag
    extern bool MIDI_CLOCK_SEND;
    MIDI_CLOCK_SEND = (clockMode == 1);
    
    // Update MIDI clock state
    extern void resetMidiClockState();
    resetMidiClockState();
    
    // Redraw to show arrow
    drawBPMScreen();
    
    lastPos2 = newPos2;
  }

  if (currentMode->pos[3] != (unsigned int)SMP.bpm) {  // Cast SMP.bpm for comparison
    updateBPM();
  }
}





void showExit(int index) {
  //showIcons(HELPER_EXIT, UI_DIM_BLUE);
  //Encoder[index].writeRGBCode(0x0000FF);
}

void showLoadSave() {

  drawNoSD();
  FastLEDclear();

  // Show big icons
  showIcons(ICON_LOADSAVE, UI_DIM_GREEN);
  showIcons(ICON_LOADSAVE2, UI_WHITE);
  
  // New indicator system: file: M[G] | M[R] | C[W] (conditional) | L[X]
  drawIndicator('M', 'G', 1);  // Encoder 1: Medium Green
  drawIndicator('M', 'R', 2);  // Encoder 2: Medium Red  
  // Encoder 3: Cross White only if SMP_LOAD_SETTINGS is true
  if (SMP_LOAD_SETTINGS) {
    drawIndicator('C', 'W', 3);  // Encoder 3: Cross White
  }
  drawIndicator('L', 'X', 4);  // Encoder 4: Large Blue
  
  // Check for .txt file
  char OUTPUTf[50];
  sprintf(OUTPUTf, "%u.txt", SMP.file);
  
  bool txtExists = SD.exists(OUTPUTf);
  
  if (txtExists) {
    // .txt file exists - bright green for load, dark red for save
    drawIndicator('M', 'G', 1);   // Bright green for load
    drawIndicator('M', 'D', 2);   // Dark red for save
    drawNumber(SMP.file, UI_BRIGHT_GREEN, 11); // Bright green number for existing file
  } else {
    // No file exists - dark green for load, bright red for save
    drawIndicator('M', 'E', 1);   // Dark green for load
    drawIndicator('M', 'R', 2);   // Bright red for save
    drawNumber(SMP.file, UI_BLUE, 11); // Blue number for non-existing file
  }
  FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  if (currentMode->pos[3] != SMP.file) {
    SMP.file = currentMode->pos[3];
  }
  
  // Update SMP_LOAD_SETTINGS based on encoder[2] position
  // Only allow settings loading if .txt file exists
  if (txtExists) {
    if (currentMode->pos[2] == 1) {
      SMP_LOAD_SETTINGS = true;
    } else {
      SMP_LOAD_SETTINGS = false;
    }
  } else {
    // For empty files, force settings loading off and disable encoder
    SMP_LOAD_SETTINGS = false;
    currentMode->pos[2] = 0;
    Encoder[2].writeCounter((int32_t)0);
  }
}

void showSamplePack() {
  drawNoSD();
  FastLEDclear();

  // Show big icons
  showIcons(ICON_SAMPLEPACK, UI_DIM_YELLOW);
  
  // New indicator system: pack: M[G] | M[R] | | L[X]
  drawIndicator('M', 'G', 1);  // Encoder 1: Medium Green
  drawIndicator('M', 'R', 2);  // Encoder 2: Medium Red
  // Encoder 3: empty (no indicator)
  drawIndicator('L', 'X', 4);  // Encoder 4: Large Blue
  
  // Apply different colors for load/save operations based on file existence
  char OUTPUTf[50];
  sprintf(OUTPUTf, "%u/%u.wav", SMP.pack, 1);
  if (SD.exists(OUTPUTf)) {
    // File exists - bright green for load, dark red for save
    drawIndicator('M', 'G', 1);   // Bright green for load
    drawIndicator('M', 'D', 2);   // Dark red for save
    drawNumber(SMP.pack, UI_BRIGHT_GREEN, 11); // Bright green number for existing file
  } else {
    // File doesn't exist - dark green for load, bright red for save
    drawIndicator('M', 'E', 1);   // Dark green for load
    drawIndicator('M', 'R', 2);   // Bright red for save
    drawNumber(SMP.pack, UI_BLUE, 11); // Blue number for non-existing file
  }

  // Validate samplepack value - ensure it's within valid range (0-99)
  if (SMP.pack < 0 || SMP.pack > 99) {
    Serial.print("INVALID SAMPLEPACK VALUE! Defaulting to 0");
    SMP.pack = 0;
    currentMode->pos[3] = 0;
    Encoder[3].writeCounter((int32_t)0);
    EEPROM.put(0, SMP.pack);  // Save the corrected value to EEPROM
  }

  FastLED.setBrightness(ledBrightness);
  FastLEDshow();
  if (currentMode->pos[3] != SMP.pack) {
    //Serial.println("File: " + String(currentMode->pos[3]));
    SMP.pack = currentMode->pos[3];
  }
}

void loadSamplePack(unsigned int pack_id, bool intro) {  // Renamed pack to pack_id to avoid conflict
  //Serial.println("Loading SamplePack #" + String(pack_id));
  drawNoSD();
  
  // Validate pack_id - ensure it's within valid range (0-99)
  if (pack_id < 0 || pack_id > 99) {
    Serial.print("INVALID SAMPLEPACK ID! Defaulting to 0");
    pack_id = 0;
  }
  
  EEPROM.put(0, pack_id);                        // Save current pack_id to EEPROM
  
  Serial.println("=== Loading Samplepack ===");
  Serial.print("Pack ID: ");
  Serial.println(pack_id);
  
  // First, load samples from samplepack 0 for voices that have custom samples
  Serial.println("--- Checking SP0 Active Voices ---");
  for (unsigned int z = 1; z < maxFiles; z++) {
    Serial.print("Voice ");
    Serial.print(z);
    Serial.print(": sp0Active = ");
    Serial.println(SMP.sp0Active[z] ? "TRUE" : "FALSE");
    
    if (SMP.sp0Active[z]) {
      Serial.print(">>> Loading voice ");
      Serial.print(z);
      Serial.println(" from SAMPLEPACK 0 <<<");
      
      if (!intro) {
        showIcons(ICON_SAMPLE, UI_BG_DIM);
      } else {
        drawText("SP0", 2, 11, col[(maxFiles + 1) - z]);
      }
      drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], UI_DIM_WHITE, intro);
      loadSample(0, z);  // Load from samplepack 0
    }
  }
  
  Serial.println("--- Loading Regular Samplepack ---");
  
  // Then, load samples from the requested samplepack, but skip voices with sp0Active
  for (unsigned int z = 1; z < maxFiles; z++) {  // maxFiles is 9. So loads samples 1 through 8.
                                                 // Sample arrays are often 0-indexed. _samplers[0] to _samplers[8] exist.
                                                 // This loop should probably be z=0 to maxFiles-1 or z=1 to maxFiles (inclusive for maxFiles).
                                                 // Assuming it means load into sampler slots 1 to 8.
    if (!SMP.sp0Active[z]) {  // Only load if NOT using samplepack 0
      Serial.print(">>> Loading voice ");
      Serial.print(z);
      Serial.print(" from Pack ");
      Serial.println(pack_id);
      
      if (!intro) {
        showIcons(ICON_SAMPLE, UI_BG_DIM);
      } else {
        drawText("LOAD", 2, 11, col[(maxFiles + 1) - z]);
      }
      drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], UI_DIM_WHITE, intro);
      loadSample(pack_id, z);  // loadSample needs to know which sampler slot 'z' corresponds to.
    } else {
      Serial.print("--- Skipping voice ");
      Serial.print(z);
      Serial.println(" (using SP0)");
    }
  }
  Serial.println("=== Samplepack Loading Complete ===");
  // char OUTPUTf[50]; // This seems unused here
  // sprintf(OUTPUTf, "%u/%u.wav", pack_id, 1);
  switchMode(&draw);  // Switch back to draw mode after loading
  
  // Reset paint/unpaint prevention flag after loadSamplePack operation
  preventPaintUnpaint = false;
}


void updateLastPage() {
  // Calculate max selectable pages based on LED modules
  int numModules = maxX / MATRIX_WIDTH;  // 1 or 2
  int effectiveMaxPages = MAX_STEPS / maxX;  // Keep total steps fixed; fewer pages when wider
  
  // If LOOP is set (1-8), force lastPage to that value
  extern int loopLength;
  if (loopLength > 0) {
    lastPage = min(loopLength, effectiveMaxPages);
    // Still update hasNotes array for potential other uses
    for (unsigned int p = 1; p <= effectiveMaxPages; p++) {
      bool pageHasNotesThisPage = false;
      unsigned int baseIndex = (p - 1) * maxX;
      for (unsigned int ix = 1; ix <= maxX; ix++) {
        for (unsigned int iy = 1; iy <= maxY; iy++) {
          if (note[baseIndex + ix][iy].channel > 0) {
            pageHasNotesThisPage = true;
            break;
          }
        }
        if (pageHasNotesThisPage) break;
      }
      hasNotes[p] = pageHasNotesThisPage;
    }
    return;
  }
  
  // Original logic when LOOP is OFF
  lastPage = 0;  // Start by assuming no notes
  for (unsigned int p = 1; p <= effectiveMaxPages; p++) {
    bool pageHasNotesThisPage = false;  // Renamed to avoid conflict
    unsigned int baseIndex = (p - 1) * maxX;
    for (unsigned int ix = 1; ix <= maxX; ix++) {
      for (unsigned int iy = 1; iy <= maxY; iy++) {
        // Always consider notes from any channel for playback range
        // The current channel filtering should only affect visual display, not playback
        if (note[baseIndex + ix][iy].channel > 0) {
          pageHasNotesThisPage = true;
          break;
        }
      }
      if (pageHasNotesThisPage) {
        lastPage = p;
        break;
      }
    }
    hasNotes[p] = pageHasNotesThisPage;  // Store if this page has notes
    if (!pageHasNotesThisPage && p > 1 && !hasNotes[p - 1]) {
      // If current page is empty and previous was also empty,
      // we can potentially stop early if lastPage was already found.
      // However, the current logic correctly finds the *highest* page with notes.
    }
  }
  if (lastPage == 0) {  // If no notes found on any page
    lastPage = 1;       // Default to page 1
  }
}

void loadWav() {
  drawSampleLoadOverlay();
  FastLEDshow();

  playSdWav1.stop();

  //Serial.println("Loading Wave :" + String(SMP.wav[GLOB.currentChannel].fileID));
  
  // Load the preview sample (which may be reversed/edited) to the target channel
  // This copies from sampled[0] (preview) to sampled[targetChannel]
  extern void loadPreviewToChannel(unsigned int targetChannel);
  loadPreviewToChannel(GLOB.currentChannel);
  
  // Auto-save to samplepack 0 after loading individual sample
  copySampleToSamplepack0(GLOB.currentChannel);
  saveSp0StateToEEPROM();
  
  // Store the current edit page before switching modes
  int savedEditPage = GLOB.edit;
  
  switchMode(&singleMode);
  GLOB.singleMode = true;
  
  // Restore the edit page and set encoder[1] to match
  GLOB.edit = savedEditPage;
  currentMode->pos[1] = savedEditPage;
  Encoder[1].writeCounter((int32_t)savedEditPage);
  
  // Reset paint/unpaint prevention flag after loadWav operation
  preventPaintUnpaint = false;
}

// Helper: map (arr, idx) to (page, slot)
bool findSliderDefPageSlot(int chan, SettingArray arr, int8_t idx, int &page, int &slot) {
    for (int p = 0; p < 4; ++p) {
        for (int s = 0; s < 4; ++s) {
            if (sliderDef[chan][p][s].arr == arr && sliderDef[chan][p][s].idx == idx) {
                page = p;
                slot = s;
                return true;
            }
        }
    }
    return false;
}

// Helpers to get/set value for defaultFastFilter
int getDefaultFastFilterValue(int channel, SettingArray arr, int8_t idx) {
    switch (arr) {
        case ARR_FILTER: return SMP.filter_settings[channel][idx];
        case ARR_SYNTH:  return SMP.synth_settings[channel][idx];
        case ARR_PARAM:  return SMP.param_settings[channel][idx];
        default: return 0;
    }
}
void setDefaultFastFilterValue(int channel, SettingArray arr, int8_t idx, int value) {
    switch (arr) {
        case ARR_FILTER: SMP.filter_settings[channel][idx] = value; break;
        case ARR_SYNTH:  SMP.synth_settings[channel][idx] = value; break;
        case ARR_PARAM:  SMP.param_settings[channel][idx] = value; break;
        default: break;
    }
}

// Add an array to define how many filter pages to render for each channel
uint8_t filterPageCount[NUM_CHANNELS] = {0,3,3,3,3,3,3,3,3,0,0,4,0,3,3,0}; // default 3 for sample channels, 4 for channel 11
// ... existing code ...
// (If you want to be explicit for all channels, you can do: {4,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4})
// ... existing code ...

void setSliderDefForChannel(int channel) {
    // Channel 1-3 (index 1-3):
    if (channel >= 1 && channel <= 3) {
        static const SliderDefEntry ch1_3Template[3][4] = {
            {
                {ARR_FILTER, PASS, "PASS", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, FREQUENCY, "FREQ", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, REVERB, "RVRB", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, BITCRUSHER, "BITC", 32, DISPLAY_NUMERIC, nullptr, 32}
            },
            {
                {ARR_FILTER, RES, "RES", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, DETUNE, "DTNE", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, OCTAVE, "OCTV", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, EFX, "SND", 1, DISPLAY_ENUM, sndTypeNames, 1},
            },
            {
                {ARR_PARAM, ATTACK, "ATTC", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, DECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, SUSTAIN, "SUST", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, RELEASE, "RLSE", 32, DISPLAY_NUMERIC, nullptr, 32}
            }
        };
        // Clear all pages first to avoid stale data in unused slots
        for (int p = 0; p < 4; ++p) {
          for (int s = 0; s < 4; ++s) {
            sliderDef[channel][p][s] = {ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0};
          }
        }
        memcpy(sliderDef[channel], ch1_3Template, sizeof(ch1_3Template));
        return;
    }
    // Channel 4-8 (index 4-8):
    if (channel >= 4 && channel <= 8) {
        static const SliderDefEntry ch4_8Template[3][4] = {
            {
                {ARR_FILTER, PASS, "PASS", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, FREQUENCY, "FREQ", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, REVERB, "RVRB", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, BITCRUSHER, "BITC", 32, DISPLAY_NUMERIC, nullptr, 32}
        },{
                {ARR_FILTER, RES, "RES", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, DETUNE, "DTNE", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, OCTAVE, "OCTV", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0}
        },{
                {ARR_PARAM, ATTACK, "ATTC", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, DECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, SUSTAIN, "SUST", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, RELEASE, "RLSE", 32, DISPLAY_NUMERIC, nullptr, 32}
        }
        /* FOR GRANULAR
        ,{{ARR_FILTER, ACTIVE, "ACTV", 1, DISPLAY_NUMERIC, nullptr, 2},
            {ARR_FILTER, OFFSET, "OFFS", 32, DISPLAY_NUMERIC, nullptr, 32},
            {ARR_FILTER, PITCH, "PTCH", 32, DISPLAY_NUMERIC, nullptr, 32},
            {ARR_FILTER, SPEED, "SPED", 32, DISPLAY_NUMERIC, nullptr, 32}}*/      
        };

        // Clear all 4 pages first
        memset(sliderDef[channel], 0, sizeof(sliderDef[channel]));
        // Copy 3 pages
        memcpy(sliderDef[channel][0], ch4_8Template[0], sizeof(ch4_8Template[0]));
        memcpy(sliderDef[channel][1], ch4_8Template[1], sizeof(ch4_8Template[1]));
        memcpy(sliderDef[channel][2], ch4_8Template[2], sizeof(ch4_8Template[2]));
        // Set 4th page to ARR_NONE
        for (int s = 0; s < 4; ++s) {
            sliderDef[channel][3][s].arr = ARR_NONE;
            sliderDef[channel][3][s].idx = 0;
        }
        return;
    }

    
    // Channel 13 and 14: custom 3 pages
    if (channel == 13 || channel == 14) {
        static const SliderDefEntry ch13_14Template[3][4] = {
            {
                {ARR_FILTER, PASS, "PASS", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, FREQUENCY, "FREQ", 32, DISPLAY_NUMERIC, nullptr, 32},
                // Use CENT (synth) instead of REVERB to shift oscillators up/down
                {ARR_SYNTH, CENT, "CENT", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, BITCRUSHER, "BITC", 32, DISPLAY_NUMERIC, nullptr, 32}
            },
            {
                {ARR_FILTER, RES, "RES", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, DETUNE, "DTNE", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, OCTAVE, "OCTV", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_FILTER, FILTER_WAVEFORM, "WAVE", 16, DISPLAY_ENUM, waveformNames, 4}
            },
            {
                {ARR_PARAM, ATTACK, "ATTC", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, DECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, SUSTAIN, "SUST", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, RELEASE, "RLSE", 32, DISPLAY_NUMERIC, nullptr, 32}
            }
        };
        // Clear all 4 pages first
        memset(sliderDef[channel], 0, sizeof(sliderDef[channel]));
        // Copy 3 pages
        memcpy(sliderDef[channel][0], ch13_14Template[0], sizeof(ch13_14Template[0]));
        memcpy(sliderDef[channel][1], ch13_14Template[1], sizeof(ch13_14Template[1]));
        memcpy(sliderDef[channel][2], ch13_14Template[2], sizeof(ch13_14Template[2]));
        // Set 4th page to ARR_NONE
        for (int s = 0; s < 4; ++s) {
            sliderDef[channel][3][s].arr = ARR_NONE;
            sliderDef[channel][3][s].idx = 0;
        }
        return;
    }
}

// Initialize page mutes
FLASHMEM void initPageMutes() {
  // Initialize from SMP data if available, otherwise use defaults
  for (int ch = 0; ch < maxY; ch++) {
    globalMutes[ch] = SMP.globalMutes[ch];
    for (int page = 0; page < maxPages; page++) {
      pageMutes[page][ch] = SMP.pageMutes[page][ch];
    }
  }
  
  // Fallback: if SMP data is not available (old files), use SMP.mute for global mutes
  bool hasValidMuteData = false;
  for (int ch = 0; ch < maxY; ch++) {
    if (SMP.globalMutes[ch] != false || SMP.pageMutes[0][ch] != false) {
      hasValidMuteData = true;
      break;
    }
  }
  
  if (!hasValidMuteData) {
    // This is likely an old file, use SMP.mute as global mutes
    for (int ch = 0; ch < maxY; ch++) {
      globalMutes[ch] = SMP.mute[ch];
      // Initialize all pages with the global mute state
      for (int page = 0; page < maxPages; page++) {
        pageMutes[page][ch] = SMP.mute[ch];
      }
    }
  }
}

// Get mute state for a channel, considering PMOD setting
FLASHMEM bool getMuteState(int channel) {
  if (SMP_PATTERN_MODE) {
    // Use page-specific mutes when PMOD is enabled
    return pageMutes[GLOB.edit - 1][channel];  // GLOB.edit is 1-indexed, array is 0-indexed
  } else {
    // Use global mutes when PMOD is disabled
    return globalMutes[channel];
  }
}

// Set mute state for a channel, considering PMOD setting
FLASHMEM void setMuteState(int channel, bool muted) {
  if (SMP_PATTERN_MODE) {
    // Set page-specific mute when PMOD is enabled
    pageMutes[GLOB.edit - 1][channel] = muted;
  } else {
    // Set global mute when PMOD is disabled
    globalMutes[channel] = muted;
    SMP.mute[channel] = muted;  // Keep SMP.mute in sync for backward compatibility
  }
  // Immediately stop synth voices when muted
  if (muted && (channel == 13 || channel == 14)) {
    stopSynthChannel(channel);
  }
}

// Note: Global and page mutes are now completely separate stores
// No copying between them when switching PMOD modes

// Unmute all channels in audio system
FLASHMEM void unmuteAllChannels() {
  for (int ch = 0; ch < maxY; ch++) {
    SMP.mute[ch] = false;  // Unmute in audio system
  }
}

// Apply saved mutes after PMOD switch
FLASHMEM void applyMutesAfterPMODSwitch() {
  for (int ch = 0; ch < maxY; ch++) {
    bool muteState = getMuteState(ch);
    SMP.mute[ch] = muteState;  // Apply to audio system
  }
}
