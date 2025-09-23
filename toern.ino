#define VERSION "v1.1"
//extern "C" char *sbrk(int incr);
#define FASTLED_ALLOW_INTERRUPTS 0
#define SERIAL8_RX_BUFFER_SIZE 255  // Increase to 256 bytes
#define SERIAL8_TX_BUFFER_SIZE 16  // Increase if needed for transmission
#define TargetFPS 30

//#define AUDIO_BLOCK_SAMPLES 128
//#define AUDIO_SAMPLE_RATE_EXACT 44100


static const int FAST_DROP_BLOCKS = 40;  // ≈200ms @ 44100Hz with 128-sample blocks
static int fastDropRemaining = 0;
volatile bool stepIsDue = false;


//#include <Wire.h>x-
#include "Arduino.h"
#include <cstring>  // For memcmp

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

#define maxX 16
#define maxY 16
#define INT_SD 10
#define NUM_LEDS 256
#define DATA_PIN 17  // PIN FOR LEDS
#define INT_PIN 27   // PIN FOR ENOCDER INTERRUPS

#define SWITCH_1 16  // Pin for TPP223 1
#define SWITCH_2 3  // // Pin for TPP223 3 //3==lowerright, lowerleft== 15!
#define SWITCH_3 41   //Pin for TPP223 2

#define VOL_MIN 1
#define VOL_MAX 10
#define BPM_MIN 40
#define BPM_MAX 300

#define GAIN_1 0.4  //0.1;
#define GAIN_2 0.4   //0.33
#define GAIN_3 0.4   //0.25
#define GAIN_4 0.4  //0.2;


#define NUM_ENCODERS 4
#define defaultVelocity 63
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
  static const long BaudRate = 1000000;
  static const unsigned SysExMaxSize = 16;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial8, MIDI, MidiSettings);
unsigned long beatStartTime = 0;  // Timestamp when the current beat started

unsigned long ganularStartTime = 0;  // Timestamp when the current beat started

#define CLOCK_BUFFER_SIZE 24
//elapsedMillis recFlushTimer;
elapsedMillis recTime;


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
static int lastDefaultFastFilterValue[NUM_CHANNELS] = {0};

enum ValueDisplayMode {
  DISPLAY_NUMERIC,
  DISPLAY_ENUM
};

const char* instTypeNames[] = { "BASS", "KEYS", "CHPT", "PAD", "WOW", "ORG", "FLT", "LEAD", "ARP", "BRSS" };
#define INST_ENUM_COUNT (sizeof(instTypeNames) / sizeof(instTypeNames[0]))

// Add at file scope, after instTypeNames[]
const char* sndTypeNames[] = { "SAMP", "DRUM" };
const char* waveformNames[] = { "SIN", "SQR", "SAW", "TRI" };

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


// Number of samples in each delay line
// Allocate the delay lines for left and right channels

bool MIDI_CLOCK_SEND = true;

bool MIDI_TRANSPORT_RECEIVE = true;
bool MIDI_VOICE_SELECT = false;
bool SMP_PATTERN_MODE = false;
bool SMP_FLOW_MODE = false;  // FLOW mode: follows timer position when playing
unsigned int lastFlowPage = 0;  // Track the last page set by FLOW mode
bool recMenuFirstEnter = true;  // Track first entry into REC menu
bool monMenuFirstEnter = true;  // Flag for MON menu first entry
unsigned int SMP_FAST_REC = false;
unsigned int SMP_REC_CHANNEL_CLEAR = true;
bool SMP_LOAD_SETTINGS = true;  // Whether to load SMP settings when loading tracks

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

int lastFile[9] = { 0 };
bool freshPaint, tmpMute = false;
bool preventPaintUnpaint = false;  // Flag to prevent paint/unpaint after certain operations


bool firstcheck = false;
bool nofile = false;
char *currentParam = "DLAY";
char *currentFilter = "TYPE";
char *currentDrum = "TONE";
char *currentSynth = "BASS";
unsigned int fxType = 0;
unsigned int drum_type = 0;

unsigned int selectedFX = 0;


unsigned long filterDrawEndTime = 0;
bool filterDrawActive = false;


unsigned long patternChangeTime = 0;
bool patternChangeActive = false;



String oldPosString, posString = "1:2:";
// String buttonString, oldButtonString = "0000"; // REMOVED
int oldButtons[NUM_ENCODERS] = { 0, 0, 0, 0 };  // ADDED: To store previous button states

unsigned long playStartTime = 0;  // To track when play(true) was last called

bool previewIsPlaying = false;

const int maxPeaks = 512;  // Adjust based on your needs
float peakValues[maxPeaks];
int peakIndex = 0;


const int maxRecPeaks = 512;  // Adjust based on your needs
float peakRecValues[maxRecPeaks];
int peakRecIndex = 0;



uint8_t ledBrightness = 83;
const unsigned int maxlen = (maxX * maxPages) + 1;
const long ram = 9525600;  // 9* 1058400; //12seconds on 44.1 / 16Bit before: 12582912;  //12MB ram for sounds // 16MB total
const unsigned int SONG_LEN = maxX * maxPages;

static bool lastBothTouched = false;
bool touchState[4] = { false };      // Current touch state (HIGH/LOW)
bool lastTouchState[4] = { false };  // Previous touch state
const int touchThreshold = 45;

const unsigned int totalPulsesToWait = pulsesPerBar * 2;

const unsigned long CHECK_INTERVAL = 50;  // Interval to check buttons in ms
unsigned long lastCheckTime = 0;          // Get the current time


int recMode = 1;
unsigned int fastRecMode = 0;
unsigned int previewVol = 2;
int recChannelClear = 1;  // 0=OFF (add triggers), 1=ON (clear then add), 2=FIX (no manipulation)
int transportMode = 1;
int patternMode = -1;
int flowMode = -1;  // FLOW setting: -1 = OFF, 1 = ON
int clockMode = 1;
int voiceSelect = 1;
unsigned int micGain = 10;  // Microphone gain: 0-64, default 10
unsigned int monitorLevel = 0;  // Monitoring level: 0-4, default 0 (OFF) to prevent startup feedback

bool isRecording = false;
File frec;

#define MAXREC_SECONDS 20
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


unsigned int beat = 1;
unsigned int samplePackID, fileID = 1;
EXTMEM unsigned int lastPreviewedSample[FOLDER_MAX] = {};
IntervalTimer playTimer;
//IntervalTimer midiTimer;
unsigned int lastPage = 1;
int editpage = 1;



struct Note {
  uint8_t channel;   // 0 = no note; otherwise, MIDI note value (0-127)
  uint8_t velocity;  // MIDI velocity (0-127)
} __attribute__((packed));


EXTMEM Note note[maxlen + 1][maxY + 1] = {};
EXTMEM Note tmp[maxlen + 1][maxY + 1] = {};
EXTMEM Note original[maxlen + 1][maxY + 1] = {};

EXTMEM unsigned int sample_len[maxFiles];
bool sampleLengthSet = false;

bool isNowPlaying = false;  // global

int PrevSampleRate = 1;
EXTMEM int SampleRate[maxFiles] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
EXTMEM unsigned char sampled[maxFiles][ram / (maxFiles + 1)];
static const uint32_t MAX_SAMPLES = sizeof(sampled[0]) / sizeof(sampled[0][0]);




elapsedMillis msecs;
elapsedMillis mRecsecs;


CRGB leds[NUM_LEDS];

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

Mode draw = { "DRAW", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxlen - 1 }, { 1, 1, maxfilterResolution, 1 }, { 0x110011, 0x000000, 0x00FF00, 0x110011 } };
Mode singleMode = { "SINGLE", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxlen - 1 }, { 1, 2, maxfilterResolution, 1 }, { 0x000000, 0x000000, 0x00FF00, 0x000000 } };
Mode volume_bpm = { "VOLUME_BPM", { 1, 11, VOL_MIN, BPM_MIN }, { 1, 30, VOL_MAX, BPM_MAX }, { 1, 11, 7, 100 }, { 0x000000, 0xFFFFFF, 0xFF4400, 0x00FFFF } };  //OK
//filtermode has 4 entries
Mode filterMode = { "FILTERMODE", { 0, 0, 0, 0 }, { maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution }, { 1, 1, 1, 1 }, { 0x00FFFF, 0xFF00FF, 0xFFFF00, 0x00FF00 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 7, 0, 7 }, { 9, 9, maxfilterResolution, 9 }, { 8, 8, maxfilterResolution, 8 }, { 0xFFFF00, 0xFFFF00, 0x000000, 0xFFFFFF } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { maxY, 1, maxY, maxY }, { maxY, 1, 10, 10 }, { 0xFF4400, 0x000000, 0x0044FF, 0x888888 } };

Mode set_Wav = { "SET_WAV", { 1, 1, 1, 1 }, { 9999, FOLDER_MAX, 9999, 999 }, { 0, 0, 0, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 1 }, { 1, 1, 99, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode loadSaveTrack = { "LOADSAVE_TRACK", { 1, 1, 0, 1 }, { 1, 1, 1, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode menu = { "MENU", { 1, 1, 1, 0 }, { 1, 1, 64, 16 }, { 1, 1, 10, 1 }, { 0x000000, 0x000000, 0x000000, 0x00FF00 } };
Mode newFileMode = { "NEW_FILE", { 0, 1, 0, 0 }, { 5, 16, 0, 0 }, { 0, 8, 0, 0 }, { 0x00FFFF, 0xFF00FF, 0x000000, 0x000000 } };
// Declare currentMode as a global variable
Mode *currentMode;
Mode *oldMode;


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
  float drum_settings[maxY][4];
  float synth_settings[maxY][8];
  unsigned int mute[maxY];
  unsigned int channelVol[maxY];
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
Device SMP = {
  100.0,                                                                               //bpm
  1,                                                                                   //file
  1,                                                                                   //pack
  { { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 } },  //wav preset
  {},                                                                                  //param_settings
  {},                                                                                  //filter_settings
  {},                                                                                  //drum_settings
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
  0                                                                                    //shiftY1
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
#define NUM_DRUMS (sizeof(SMP.drum_settings[0]) / sizeof(SMP.drum_settings[0][0]))
#define MAX_CHANNELS maxY  // maxY is the number of channels (e.g. 16)

uint32_t loadedSampleRate[MAX_CHANNELS];
uint32_t loadedSampleLen[MAX_CHANNELS];
static bool prevMuteState[maxFiles + 1];
static bool tmpMuteActive = false;

// Per-page mute system for PMOD (pattern mode)
static bool pageMutes[maxPages][maxY];  // [page][channel] - stores mute state per page
static bool globalMutes[maxY];          // stores global mute state when PMOD is off

//EXTMEM int16_t fastRecBuffer[MAX_CHANNELS][BUFFER_SAMPLES];
static size_t fastRecWriteIndex[MAX_CHANNELS];
bool fastRecordActive = false;
//static uint8_t fastRecordChannel = 0;


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


enum DrumTypes { DRUMTONE,
                 DRUMDECAY,
                 DRUMPITCH,
                 DRUMTYPE
};  //Define drum types




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

char *activeMidiSetType[6] = { "IN", "OUT", "OUT", "INPT", "SCTL", "RCTL" };

// Arrays to track multiple encoders
int buttons[NUM_ENCODERS] = { 0 };  // Tracks the current state of each encoder
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
enum SettingArray { ARR_FILTER, ARR_SYNTH, ARR_DRUM, ARR_PARAM, ARR_NONE };

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
static SliderDefEntry sliderDef[NUM_CHANNELS][4][4];

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
AudioEffectFreeverb *freeverbs[15] = { nullptr, &freeverb1, &freeverb2, nullptr, nullptr, nullptr, nullptr, &freeverb7, &freeverb8, 0, 0, &freeverb11, nullptr, nullptr, nullptr };
AudioMixer4 *freeverbmixers[15] = { nullptr, &freeverbmixer1, &freeverbmixer2, nullptr, nullptr, nullptr, nullptr, &freeverbmixer7, &freeverbmixer8, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
AudioMixer4 *waveformmixers[15] = { nullptr, &BDMixer, &SNMixer, &HHMixer, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &mixer_waveform11, nullptr, &mixer_waveform13, &mixer_waveform14 };


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



void setVelocity() {
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
  //CHANNEL VOLUME
  if (currentMode->pos[2] != GLOB.velocity) {
    SMP.channelVol[GLOB.currentChannel] = currentMode->pos[2];
    float channelvolume = mapf(SMP.channelVol[GLOB.currentChannel], 1, maxY, 0, 1);
    //Serial.println(channelvolume);
    amps[GLOB.currentChannel]->gain(channelvolume);
  }

  drawVelocity();
}



void staticButtonPushed(i2cEncoderLibV2 *obj) {
  encoder_button_pushed(obj, currentEncoderIndex);
  pressed[currentEncoderIndex] = true;
}




void staticButtonReleased(i2cEncoderLibV2 *obj) {
  encoder_button_released(obj, currentEncoderIndex);
  pressed[currentEncoderIndex] = false;
}

void staticThresholds(i2cEncoderLibV2 *obj) {
  //  encoder_thresholds(obj, currentEncoderIndex);
}



void encoder_button_pushed(i2cEncoderLibV2 *obj, int encoderIndex) {
  unsigned long currentTime = millis();

  buttonPressStartTime[encoderIndex] = currentTime;
  isPressed[encoderIndex] = true;
}



void encoder_button_released(i2cEncoderLibV2 *obj, int encoderIndex) {
  buttonState[encoderIndex] = RELEASED;
}





const unsigned long btnDebounce = 30;
static unsigned long lastBtnChange = 0;
void handle_button_state(i2cEncoderLibV2 *obj, int encoderIndex) {
  unsigned long currentTime = millis();
  // `isPressed[encoderIndex]` is true if button is physically down, false if up.
  // `buttonState[encoderIndex]` is our FSM state: IDLE, LONG_PRESSED, RELEASED.

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
    if (buttonState[encoderIndex] != IDLE) {
      // If it's not pressed, but state wasn't RELEASED to transition to IDLE, force IDLE.
      // This can happen if a release was missed or if initial state is off.
      buttonState[encoderIndex] = IDLE;
    }
    // If button is up and state is IDLE (or just became IDLE)
    // and it wasn't a 1 or 9 event from a RELEASED state this cycle:
    if (buttons[encoderIndex] != 1 && buttons[encoderIndex] != 9) {  // Don't overwrite a just-set release event
      buttons[encoderIndex] = 0;
    }
  }
}

// Helper function for checkMode
bool match_buttons(const int states[], int b0, int b1, int b2, int b3) {
  return states[0] == b0 && states[1] == b1 && states[2] == b2 && states[3] == b3;
}



void testDrums() {

  float kickTone = 512;   // Maps to ~40–120 Hz for the kick
  float kickDecay = 512;  // Maps to ~50–300 ms decay
  float kickPitch = 0;    // Adds up to ~34 Hz extra on saw wave
  int kickType = 1;       // Choose type 1 (try 1, 2, or 3)

  float snareTone = 300;   // Controls blend: higher = more tone
  float snareDecay = 400;  // Maps to ~30–150 ms decay
  float snarePitch = 512;  // Sets filter cutoff and tone frequency
  int snareType = 2;       // Choose type 2 for snare character

  float hihatTone = 512;   // Blend between noise and tone for hi-hat
  float hihatDecay = 512;  // Maps to ~10–50 ms decay (very short)
  float hihatPitch = 512;  // Maps to filter frequency (2000–7000 Hz)
  int hihatType = 3;       // Choose type 3 for hi-hat sound
}



void switchMode(Mode *newMode) {
  updateLastPage();
  
  drawNoSD_hasRun = false;

  unpaintMode = false;
  GLOB.singleMode = false;
  paintMode = false;
  oldMode = currentMode;
  
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

    
    currentMode = newMode;
    // Set last saved values for encoders



    for (int i = 0; i < NUM_ENCODERS; i++) {
      Encoder[i].writeRGBCode(currentMode->knobcolor[i]);
      
      // Special handling for encoder 1 (page control) when pattern mode is ON
      if (i == 1 && SMP_PATTERN_MODE && (currentMode == &draw || currentMode == &singleMode)) {
        // When pattern mode is ON, limit encoder 1 to lastPage instead of maxPages
        updateLastPage();  // Ensure lastPage is up to date
        Encoder[i].writeMax((int32_t)lastPage);
      } else {
        Encoder[i].writeMax((int32_t)currentMode->maxValues[i]);  //maxval
      }
      
      Encoder[i].writeMin((int32_t)currentMode->minValues[i]);  //minval

      if ((currentMode == &singleMode && oldMode == &draw) || (currentMode == &draw && oldMode == &singleMode)) {
        //do not move Cursor for those modes
        //
      } else {
        Encoder[i].writeCounter((int32_t)currentMode->pos[i]);
      }
    }

    // Special handling for singleMode - set currentChannel color on encoders[0] and [3]
    if (currentMode == &singleMode) {
      Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    }
    
    if (currentMode == &set_Wav) {
      //REVERSE left encoder
      Encoder[0].begin(
        i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
        | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
        | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);

      Encoder[3].writeMax((int32_t)999);  //maxval
      Encoder[3].writeMin((int32_t)1);    //minval
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
      // Set encoder 1 position to match current brightness
      // ledBrightness = (pos[1] * 10) - 46, so pos[1] = (ledBrightness + 46) / 10
      unsigned int brightnessPos = (ledBrightness + 46) / 10;
      brightnessPos = constrain(brightnessPos, 6, 25);  // Valid range for volume_bpm mode
      currentMode->pos[1] = brightnessPos;
      Encoder[1].writeCounter((int32_t)brightnessPos);
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
  }

  drawPlayButton();
}


void checkFastRec() {
if (GLOB.currentChannel>8) return;

  if ((currentMode == &draw || currentMode == &singleMode) && SMP_FAST_REC == 2 || SMP_FAST_REC == 3) {
    bool pinsConnected = (digitalRead(2) == LOW);

    if (pinsConnected != lastPinsConnected && millis() - lastChangeTime > debounceDelay) {
      lastChangeTime = millis();  // Update timestamp
      lastPinsConnected = pinsConnected;

      if (SMP_FAST_REC == 2) {
        if (pinsConnected && !fastRecordActive) {
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
  touchState[2] = (touchValue3 > touchThreshold);

  if (SMP_FAST_REC == 1 && !fastRecordActive && touchState[2]) {
    startFastRecord();
    return;  // skip other mode logic while fast recording
  }

  if (SMP_FAST_REC == 1 && fastRecordActive && !touchState[2]) {
    stopFastRecord();
  }
}
}

void checkMode(const int currentButtonStates[NUM_ENCODERS], bool reset) {
  //checkFastRec();

  if (isRecording && match_buttons(currentButtonStates, 0, 9, 0, 0)) {  // "0900"
    stopRecordingRAM(getFolderNumber(SMP.wav[GLOB.currentChannel].fileID), SMP.wav[GLOB.currentChannel].fileID);
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

  if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && match_buttons(currentButtonStates, 0, 2, 0, 0)) {  // "0200"
    tmpMute = true;
    tmpMuteAll(true);
    Encoder[1].writeRGBCode(0x111111);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 9, 0, 0)) {  // "0900"
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
    switchMode(&draw);
    GLOB.singleMode = false;
  }

  if (currentMode == &menu && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    // Get the main setting for the current page
    extern int getCurrentMenuMainSetting();
    int mainSetting = getCurrentMenuMainSetting();
    switchMenu(mainSetting);
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
    GLOB.singleMode = true;
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 0, 2, 0, 0)) {  // "0200"
    startRecordingRAM();
    return;  // Prevent other button actions from being processed
  }

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

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 2, 0)) {  // "0020"
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


  if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0 && (currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0002"
    unsigned int velo = round(mapf(note[GLOB.x][GLOB.y].velocity, 1, 127, 1, maxY));
    GLOB.velocity = velo;
    switchMode(&velocity);
    GLOB.singleMode = true;
    Encoder[0].writeCounter((int32_t)velo);
  }

  if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0 && (currentMode == &draw) && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0002"
    unsigned int velo = round(mapf(note[GLOB.x][GLOB.y].velocity, 1, 127, 1, maxY));
    unsigned int chvol = SMP.channelVol[GLOB.currentChannel];
    GLOB.velocity = velo;
    GLOB.singleMode = false;
    switchMode(&velocity);
    Encoder[0].writeCounter((int32_t)velo);
    Encoder[3].writeCounter((int32_t)chvol);
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
setDefaultFilterFromSlider(filterPage[GLOB.currentChannel],3);
  }
  


  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0010"
    setEnvelopeDefaultValues((unsigned int)GLOB.currentChannel);
    setFiltersDefaultValues((unsigned int)GLOB.currentChannel);
    setDrumDefaultValues((unsigned int)GLOB.currentChannel);
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
setDefaultFilterFromSlider(filterPage[GLOB.currentChannel],2);

  }



  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    setDefaultFilterFromSlider(filterPage[GLOB.currentChannel],1);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
  setDefaultFilterFromSlider(filterPage[GLOB.currentChannel],0);


  }

  if (currentMode == &volume_bpm && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    switchMode(&draw);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 9, 0, 9)) {  // "0909"
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
    toggleMute();
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0002"
    paintMode = true;
    preventPaintUnpaint = false;  // Reset flag when paintMode is activated
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
  AudioMemory(256);
  // turn on the output
  sgtl5000_1.enable();
  sgtl5000_1.volume(1.0);

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
  sgtl5000_1.lineOutLevel(30);
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



  synthmixer11.gain(0, GAIN_3);
  synthmixer11.gain(1, GAIN_3);
  synthmixer11.gain(3, GAIN_3);

  synthmixer13.gain(0, 0.15);  // Reduced to half of current (0.3 * 0.5 = 0.15)
  synthmixer13.gain(1, 0.15);
  synthmixer13.gain(3, 0.15);

  synthmixer14.gain(0, 0.15);  // Reduced to half of current (0.3 * 0.5 = 0.15)
  synthmixer14.gain(1, 0.15);
  synthmixer14.gain(3, 0.15);


  mixer0.gain(0, GAIN_1);  //PREV
  mixer0.gain(1, GAIN_1);  //PREV


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
  
  setDrumDefaults(true);
  testDrums();
    

  /*
 for (int ch = 1; ch <= 8; ch++) {
    setEnvelopeDefaultValues((unsigned int)ch);
    setFiltersDefaultValues((unsigned int)ch);
    setDrumDefaultValues((unsigned int)ch);
    setSynthDefaultValues((unsigned int)ch);
 }
  for (int ch = 11; ch <= 11; ch++) {
    setEnvelopeDefaultValues((unsigned int)ch);
    setFiltersDefaultValues((unsigned int)ch);
    setDrumDefaultValues((unsigned int)ch);
    setSynthDefaultValues((unsigned int)ch);
 }

  for (int ch = 13; ch <= 14; ch++) {
    setEnvelopeDefaultValues((unsigned int)ch);
    setFiltersDefaultValues((unsigned int)ch);
    setDrumDefaultValues((unsigned int)ch);
    setSynthDefaultValues((unsigned int)ch);
 }
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
    Encoder[i].writeMax((int32_t)16 * 4);  //maxval
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
  //NVIC_SET_PRIORITY(IRQ_LPUART8, 128);
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

  initSoundChip();
  
  initEncoders();  // Moved initEncoders here, ensures Serial is up for its prints
  if (CrashReport) {  // This implicitly calls operator bool() or similar if defined by CrashReportClass
    checkCrashReport();
  }
  drawNoSD();
  delay(50);
  initSoundChip();
  
  mixer0.gain(1, 0.05);  //PREV Sound
  
  playSdWav1.play("intro/016.wav");

  runAnimation();
  playSdWav1.stop();
  EEPROMgetLastFiles();
  loadMenuFromEEPROM();
  loadSamplePack(samplePackID, true);
  SMP.bpm = 100.0;
  
  // Initialize GLOB with default runtime values
  initGlobalVars();
  
  initSamples();
  initPageMutes();  // Initialize per-page mute system
  
  //playTimer.priority(118);
  playTimer.begin(playNote, playNoteInterval);
  //midiTimer.begin(checkMidi, playNoteInterval);
  //midiTimer.priority(10);

  autoLoad();
  
  updateSynthVoice(11);
  switchMode(&draw);

  Serial8.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);  // optional MIDI library hook
  //MIDI.setHandleClock(myClock);       // optional if you're using callbacks
  resetMidiClockState();
  

  for (int f = 0; f < 5; f++) {
    for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
        setFilters((FilterType)f, ALL_CHANNELS[i], true);
    }
  }
  
  forceAllMixerGainsToTarget();
  
  // Turn on audio input amplifier after setup is complete
  audioInputAmp.gain(1.0);
  
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
  GLOB.y = 2;
  GLOB.seek = 0;
  GLOB.seekEnd = 0;
  GLOB.smplen = 0;
  GLOB.shiftX = 0;
  GLOB.shiftY = 0;
  GLOB.shiftY1 = 0;
}

void setEncoderColor(int i) {
  Encoder[i].writeRGBCode(0x00FF00);
}


void checkEncoders() {
  // buttonString = ""; // REMOVED
  posString = "";

  for (int i = 0; i < NUM_ENCODERS; i++) {
    currentEncoderIndex = i;  // Ensure this is set before calling encoder methods or callbacks that might use it implicitly
    Encoder[i].updateStatus();
    currentMode->pos[i] = Encoder[i].readCounterInt();
    if (i != 2) posString += String(currentMode->pos[i]) + ":";  // check Encoder 0,1 & 3

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

  if (currentMode == &draw || currentMode == &singleMode) {
    if (posString != oldPosString) {
      oldPosString = posString;
      GLOB.x = currentMode->pos[3];
      GLOB.y = currentMode->pos[0];
      //filterDrawActive = false;
      if (currentMode == &draw) {
        GLOB.currentChannel = GLOB.y - 1;
        FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
        int page, slot;
        if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
          int val = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);
          Encoder[2].writeCounter((int32_t)val);
        }
        filterfreshsetted = true;
      }

      //if (currentMode == &singleMode){
       // drawText(pianoNoteNames[12 - GLOB.y + 1  + GLOB.currentChannel], 3, 5, CRGB(200, 50, 0));
      //}

      Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      GLOB.edit = getPage(GLOB.x);
    }

    // --- update value from encoder2 in draw mode for all SettingArray types ---
    if (currentMode == &draw) {
      FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
      int page, slot;
      if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
        int encVal = currentMode->pos[2];
        if (getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx) != encVal) {
          setDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx, encVal);
          
          // Apply the filter changes like in filtermode page
          switch (dft.arr) {
            case ARR_FILTER:
              setFilters(dft.idx, GLOB.currentChannel, false);
              break;
            case ARR_SYNTH:
              // Handle synth updates if needed
              break;
            case ARR_DRUM:
              setDrums(dft.idx, GLOB.currentChannel);
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

    if ((GLOB.y > 1 && GLOB.y <= 14)) {  // GLOB.y is 1-based from encoder
      if (paintMode && !preventPaintUnpaint) {
        note[GLOB.x][GLOB.y].channel = GLOB.currentChannel;  // GLOB.currentChannel is 0-based
        note[GLOB.x][GLOB.y].velocity = defaultVelocity;
      }
      if (paintMode && currentMode == &singleMode && !preventPaintUnpaint) {
        note[GLOB.x][GLOB.y].channel = GLOB.currentChannel;
        note[GLOB.x][GLOB.y].velocity = defaultVelocity;
      }


      if (unpaintMode && !preventPaintUnpaint) {
        if (GLOB.singleMode) {
          if (note[GLOB.x][GLOB.y].channel == GLOB.currentChannel) {
            note[GLOB.x][GLOB.y].channel = 0;
            note[GLOB.x][GLOB.y].velocity = defaultVelocity;
            updateLastPage();
            // Update encoder 1 limit if pattern mode is ON or if in single mode
            if ((SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
              Encoder[1].writeMax((int32_t)lastPage);
            }
          }
        } else {
          note[GLOB.x][GLOB.y].channel = 0;
          note[GLOB.x][GLOB.y].velocity = defaultVelocity;
          updateLastPage();
          // Update encoder 1 limit if pattern mode is ON or if in single mode
          if ((SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
            Encoder[1].writeMax((int32_t)lastPage);
          }
        }
      }
    }


    if (currentMode->pos[1] != editpage) {
      updateLastPage();
      editpage = currentMode->pos[1];
      //Serial.println("p:" + String(editpage));
      int xval = mapXtoPageOffset(GLOB.x) + ((editpage - 1) * 16);
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


    
  filtercheck();

  

  if (filterDrawActive && isNowPlaying) {
      //Serial.println("active");
      if (millis() <= filterDrawEndTime) {
        
        FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
        int page, slot;
        if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
          int val = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);
          drawFilterCheck(val, dft.idx, filterColors[page][slot]);
        }
      } else {
        // Stop drawing after 2 seconds
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
            filterDrawEndTime = millis() + 1000; // 1s after last change
            drawFilterCheck(currVal, dft.idx, filterColors[page][slot]);
        }
    }
}

void checkButtons() {
  // `buttons` array is populated by `checkEncoders()` -> `handle_button_state()`
  bool changed = (memcmp(buttons, oldButtons, sizeof(buttons)) != 0);
  checkFastRec();

  if (changed) {
    Serial.print("Button event for checkMode (from checkButtons): ");
    for (int i = 0; i < NUM_ENCODERS; ++i) Serial.print(buttons[i]);
    Serial.println();

    // Create a temporary copy for checkMode to use, so checkMode cannot inadvertently
    // alter the state `checkButtons` intends to process for consumption logic later,
    // AND so that checkMode is working with a snapshot.
    int snapshotButtons[NUM_ENCODERS];
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
 // int tv3 = fastTouchRead(SWITCH_3);

  // 2) threshold into boolean states
  touchState[0] = (tv1 > touchThreshold);
  touchState[1] = (tv2 > touchThreshold);
  //touchState[2] = (tv3 > touchThreshold);

  // 3) detect "rising edge" of both‐pressed
  bool bothTouched = touchState[0] && touchState[1];
  bool newBoth = bothTouched && !lastBothTouched;
  lastBothTouched = bothTouched;

  if (newBoth) {
    // only on the first frame where both are pressed:
    if (currentMode == &singleMode) {
      //Serial.println("---------<>---------");
      GLOB.singleMode = false;
      switchMode(&set_Wav);
    } else {
      switchMode(&loadSaveTrack);
    }
    // skip any individual‐touch handling this frame
  } else if (!bothTouched) {
    // 4) only if *not* holding both, handle individual rising edges

    // SWITCH_1
    if (touchState[0] && !lastTouchState[0]) {
      if (currentMode == &draw) {
        if (!(GLOB.currentChannel == 0 || GLOB.currentChannel == 9 || GLOB.currentChannel == 10 || GLOB.currentChannel == 12 || GLOB.currentChannel == 15)) {
          animateSingle();
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
    if (currentMode != &filterMode && touchState[1] && !lastTouchState[1]) {
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
      } else {  // If in any other mode (e.g. menu, set_wav, etc.) and Switch 2 is touched, go to draw mode.
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

void loop() {
   checkMidi();

  if (currentMode == &draw || currentMode == &singleMode) {
    drawBase();
    drawTriggers();
    drawPages();  // Render page helper after drawTriggers
    if (isNowPlaying) drawTimer();
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

  if (fastRecordActive) {
    flushAudioQueueToRAM();     // grab incoming audio into RAM
    checkMode(buttons, false);  // MODIFIED call
    drawTimer();
    checkPendingSampleNotes();
    //return;  // skip the rest while we're fast-recording // This return might be intended
  }

  if (isRecording) {
    //flushAudioQueueToSD();  // SD write is now throttled and safe
    flushAudioQueueToRAM2();
    checkEncoders();           // Optional: allow user interaction
    checkMode(buttons, true);  // MODIFIED call

    for (int x = 1; x < maxX; x++) {
      for (int y = 5; y < 10; y++) {
        light(x, y, CRGB(0, 0, 0));
      }
    }
    float seconds = recTime / 1000.0f;
    char buf[8];

    snprintf(buf, sizeof(buf), "%.2f", seconds);
    drawText(buf, 3, 5, UI_ORANGE);

    if (mRecsecs > 5) {
      if (peakRecIndex < maxRecPeaks) {
        if (peakRec.available()) {
          mRecsecs = 0;
          // Store the peak value
          peakRecValues[peakRecIndex] = peakRec.read();  // Corrected index from peakIndex to peakRecIndex
          peakRecIndex++;
        }
      }
    }
    processRecPeaks();
    FastLED.show();
    return;  // Skip the rest for performance
  }


  if (previewIsPlaying) {
    if (msecs > 5) {
      if (playSdWav1.isPlaying() && peakIndex < maxPeaks) {
        if (peak1.available()) {
          msecs = 0;
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
  
  // Update smooth filter mixer gains
  updateMixerGains(GLOB.currentChannel);

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
    showMenu();
  } else if (currentMode->name == "SET_SAMPLEPACK") {
    showSamplePack();
  } else if (currentMode->name == "SET_WAV") {
    if (isRecording) { /* Do Nothing */
    }                  // Avoid return here if FastLEDshow is needed
    else { showWave(); }
  } else if (currentMode->name == "NOTE_SHIFT") {
    shiftNotes();
    drawBase();
    drawTriggers();
    drawPages();  // Render page helper after drawTriggers
    if (isNowPlaying) {
      //filtercheck
      drawTimer();
    }
  } else if (currentMode->name == "NEW_FILE") {
    showNewFileMode();
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
  FastLEDshow();

   if (stepIsDue) {
    stepIsDue = false; // Immediately reset the flag
    playNote();      // Now, do the heavy lifting here in the non-time-critical main loop
  }

  yield();
}

float getNoteDuration(int channel) {
   float timetilloff = mapf(SMP.param_settings[channel][DELAY], 0, maxfilterResolution, 0, maxParamVal[DELAY]) + mapf(SMP.param_settings[channel][ATTACK], 0, maxfilterResolution, 0, maxParamVal[ATTACK]) + mapf(SMP.param_settings[channel][HOLD], 0, maxfilterResolution, 0, maxParamVal[HOLD]) + mapf(SMP.param_settings[channel][DECAY], 0, maxfilterResolution, 0, maxParamVal[DECAY]) + mapf(SMP.param_settings[channel][RELEASE], 0, maxfilterResolution, 0, maxParamVal[RELEASE]);
  return timetilloff;
}


void shiftNotes() {
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
            // Store the velocity before clearing
            int originalVelocity = pageTmp[nx][ny].velocity;
            // Clear the old position
            pageTmp[nx][ny].channel = 0;
            pageTmp[nx][ny].velocity = defaultVelocity;
            // Set the new position
            pageTmp[nx][newposY].channel = GLOB.currentChannel;
            pageTmp[nx][newposY].velocity = originalVelocity;
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
            // Store the velocity before clearing
            int originalVelocity = pageTmp[nx][ny].velocity;
            // Clear the old position
            pageTmp[nx][ny].channel = 0;
            pageTmp[nx][ny].velocity = defaultVelocity;
            // Set the new position
            pageTmp[nx][newposY].channel = GLOB.currentChannel;
            pageTmp[nx][newposY].velocity = originalVelocity;
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


void toggleMute() {
  bool currentMuteState = getMuteState(GLOB.currentChannel);
  setMuteState(GLOB.currentChannel, !currentMuteState);
}

void deleteActiveCopy() {

  GLOB.activeCopy = false;
}



void play(bool fromStart) {
  if (CrashReport) {  // This implicitly calls operator bool() or similar if defined by CrashReportClass
    checkCrashReport();
  }

  if (fromStart) {
    updateLastPage();
    
    // Update encoder 1 limit if pattern mode is ON
    if (SMP_PATTERN_MODE && (currentMode == &draw || currentMode == &singleMode)) {
      Encoder[1].writeMax((int32_t)lastPage);
    }
    
    lastFlowPage = 0;  // Reset FLOW page tracking when playback starts
    deleteActiveCopy();
    
    // In pattern mode, start from the current page's first beat
    if (SMP_PATTERN_MODE) {
      beat = (GLOB.edit - 1) * maxX + 1;  // Start from first beat of current page
      GLOB.page = GLOB.edit;  // Keep the current page
    } else {
      beat = 1;
      GLOB.page = 1;
    }
    
    Encoder[2].writeRGBCode(0xFFFF00);
    if (MIDI_CLOCK_SEND) {
      resetMidiClockState();
      MIDI.sendRealTime(midi::Start);
      isNowPlaying = true;
      playStartTime = millis();
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
  if (MIDI_CLOCK_SEND) {MIDI.sendRealTime(midi::Stop);
        resetMidiClockState();}

  isNowPlaying = false;
  pendingStartOnBar = false;
  lastFlowPage = 0;  // Reset FLOW page tracking when playback stops
  updateLastPage();
  
  // Update encoder 1 limit if pattern mode is ON
  if (SMP_PATTERN_MODE && (currentMode == &draw || currentMode == &singleMode)) {
    Encoder[1].writeMax((int32_t)lastPage);
  }
  
  deleteActiveCopy();
  autoSave();
  envelope0.noteOff();
  //allOff();
  Encoder[2].writeRGBCode(0x005500);
  beat = 1;      // Reset beat on pause
  GLOB.page = 1;  // Reset page on pause
}


void playSynth(int ch, int b, int vel, bool persistant) {
  if (ch < 0 || ch >= 15) return;                // Bounds check for synths array
  if (!synths[ch][0] || !synths[ch][1]) return;  // Check if synth objects exist

  float frequency = fullFrequencies[constrain(b - ch + 13, 0, 26)];  // y-Wert ist 1-basiert, Array ist 0-basiert // b-1??
                                                                     // Constrain index for fullFrequencies

  //float frequency = pianoFrequencies[b+12]; //C4

  float WaveFormVelocity = mapf(vel, 1, 127, 0.0, 1.0);


  float detune_amount = mapf(SMP.filter_settings[ch][DETUNE], 0, maxfilterResolution, -1.0 / 12.0, 1.0 / 12.0);
  float detune_ratio = pow(2.0, detune_amount);  // e.g., 2^(1/12) ~ 1.05946 (one semitone up)
  // Octave shift: assume OCTAVE parameter is an integer (or can be cast to one)
  // For example, an OCTAVE value of 1 multiplies frequency by 2, -1 divides by 2.
  int octave_shift = mapf(SMP.filter_settings[ch][OCTAVE], 0, maxfilterResolution, -3, +3);
  float octave_ratio = pow(2.0, octave_shift);
  // Apply both adjustments to the base frequency
  synths[ch][0]->frequency(frequency);
  synths[ch][1]->frequency(frequency * octave_ratio * detune_ratio);
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

  if (isNowPlaying) {
    drawPlayButton();

    for (unsigned int b = 1; b < maxY + 1; b++) {  // b is 1-indexed (row on grid)
      if (beat > 0 && beat <= maxlen) {            // Ensure beat is within valid range for note array
        int ch = note[beat][b].channel;            // ch is 0-indexed for internal use (e.g. SMP arrays)
        int vel = note[beat][b].velocity;
        if (ch > 0 && !getMuteState(ch)) {  // Use new per-page mute system when PMOD is enabled

          //MidiSendNoteOn(b, ch, vel); // This needs ch to be 1-16 for MIDI, and b as pitch.
          if (ch < 9) {                                                    // Sample channels (0-8 are _samplers[0] to _samplers[8])
            if (SMP.filter_settings[ch][EFX] == 1) {                       // Drum type for sample channels 0,1,2
              float baseTone = pianoFrequencies[constrain(b - 1, 0, 15)];  // b is 1-16, map to 0-15 for pianoFreq
              float decay = mapf(SMP.drum_settings[ch][DRUMDECAY], 0, 64, 0, 1023);
              float pitchMod = mapf(SMP.drum_settings[ch][DRUMPITCH], 0, 64, 0, 1023);
              float type = SMP.drum_settings[ch][DRUMTYPE];
              float notePitchOffset = mapf(SMP.drum_settings[ch][DRUMTONE], 0, 64, 0, 1023);  // Additional pitch
              
              // Apply detune offset for channels 1-12 (excluding synth channels 13-14)
              float detuneOffset = 0.0;
              if (ch >= 1 && ch <= 12) {
                detuneOffset = detune[ch] * 100.0; // Convert semitones to cents for drum pitch
              }
              
              // Apply octave offset for channels 1-8 (excluding synth channels 13-14)
              float octaveOffset = 0.0;
              if (ch >= 1 && ch <= 8) {
                octaveOffset = channelOctave[ch] * 1200.0; // Convert octaves to cents for drum pitch (12 semitones * 100 cents)
              }

              
              Serial.print(ch);  
              Serial.print("-Drum params: baseTone+notePitchOffset=");
                
                Serial.print(baseTone + notePitchOffset + detuneOffset + octaveOffset);
                Serial.print(", decay=");
                Serial.print(decay);
                Serial.print(", pitchMod=");
                Serial.println(pitchMod);
                
              
              if (ch == 1) KD_drum(baseTone + notePitchOffset + detuneOffset + octaveOffset, decay, pitchMod, type);  // KD on channel 1 (_sampler 1)
              if (ch == 2) SN_drum(baseTone + notePitchOffset + detuneOffset + octaveOffset, decay, pitchMod, type);  // SN on channel 2 (_sampler 2)
              if (ch == 3) HH_drum(baseTone + notePitchOffset + detuneOffset + octaveOffset, decay, pitchMod, type);  // HH on channel 3 (_sampler 3)
                                                                                        // Channels 4-8 using this mode are undefined behavior
            } else {                                                                    // Sample playback for channels 0-8
              // `b` is grid row (1-16), map to pitch. `SampleRate` seems to be a pitch offset.
              // The original `b - (ch + 1)` seems like an attempt to map grid row to a MIDI-like note relative to channel.
              // Need to ensure pitch is valid for sampler.
              //int pitch = (12 * SampleRate[ch]) + b - 1;  // b-1 to make it 0-15. SampleRate acts as octave. // Original: 12 * SampleRate[ch] + b - (ch + 1) //

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
              

           

              // float delay_ms = mapf(SMP.param_settings[ch][DELAY], 0, maxfilterResolution, 0, maxParamVal[DELAY]);
              // PendingNoteEvent logic was here, removed for brevity as it was commented out in original.
            }
          } else if (ch == 11) {  // Assuming ch 11 is a specific synth
            // `octave[0]` and `transpose` affect pitch. `b` is grid row (1-16).
            // playSound likely expects a MIDI note number.
            //playSound((12 * (int)octave[0]) + transpose + (b - 1), vel);  // b-1 for 0-indexed pitch offset from row
            playSound(12 * (int)octave[0] + transpose + b, 0);
            
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
    // Compute the bounds of the current page:
    unsigned int pageStart = (GLOB.edit - 1) * maxX + 1;
    unsigned int pageEnd   = pageStart + maxX - 1;

    // Preserve relative beat when switching pages:
    static unsigned int lastEdit = GLOB.edit;
    if (GLOB.edit != lastEdit) {
      unsigned int oldStart = (lastEdit - 1) * maxX + 1;
      unsigned int offset   = beat - oldStart;
      beat = pageStart + offset;
      lastEdit = GLOB.edit;
    }

    // Advance and wrap within this page:
    beat++;
    if (beat < pageStart || beat > pageEnd) {
      beat = pageStart;
    }

    // Keep the displayed page locked:
    GLOB.page = GLOB.edit;
  } else {
    // Fallback to default behavior:
    beat++;
    checkPages();
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
    if (SMP_PATTERN_MODE) {
    // Always display the editable page when looping in pattern mode.
    GLOB.page = GLOB.edit;
    return;
  }
  
  if (SMP_FLOW_MODE && isNowPlaying) {
    // FLOW mode: follow the timer position when playing
    uint16_t timerPage = (beat - 1) / maxX + 1;
    
    // Handle page looping like normal mode
    updateLastPage();  // recompute the highest page that actually has notes
    if (timerPage > lastPage && lastPage > 0) {
      // If we stepped past the last non-empty page, restart at the top
      beat = 1;
      timerPage = 1;
    } else if (lastPage == 0) {
      // Should not happen if updateLastPage ensures it's at least 1
      beat = 1;
      timerPage = 1;
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
  
  updateLastPage();  // recompute the highest page that actually has notes
  // compute what page beat should be on
  uint16_t newPage = (beat - 1) / maxX + 1;

  // if we stepped past the last non-empty page, restart at the top
  if (newPage > lastPage && lastPage > 0) {  // Ensure lastPage is valid before comparison
    beat = 1;
    //if (fastRecordActive) stopFastRecord(); // This might stop recording prematurely if looping
    newPage = 1;
  } else if (lastPage == 0) {  // Should not happen if updateLastPage ensures it's at least 1
    beat = 1;
    newPage = 1;
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
        note[current_x][current_y].channel = 0;
        note[current_x][current_y].velocity = defaultVelocity;
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
  if ((SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
    Encoder[1].writeMax((int32_t)lastPage);
  }
  
  FastLEDshow();  // Update display immediately
}


void paint() {
  //GLOB.edit = 1; // Same as in unpaint, probably not intended here.
  //Serial.println("!!!PAINT!");
  preventPaintUnpaint = false;  // Reset flag when paint function is called

  unsigned int current_x = GLOB.x;  // Use global cursor X
  unsigned int current_y = GLOB.y;  // Use global cursor Y

  if (!GLOB.singleMode) {  // Draw mode
    // current_y is 1-16. GLOB.currentChannel is 0-14.
    // If painting on grid rows 1-15 (GLOB.y from encoder), channel is GLOB.y - 1.
    // Original condition: (y > 1 && y <= 9) || (y == 12) || (y > 13 && y <= 15)
    // This implies specific rows map to specific channel types/groups.
    // For simplicity, let's assume if current_y maps to a valid channel (1-15), we paint it.
    // GLOB.currentChannel is already set based on GLOB.y in checkEncoders for draw mode.
    if (current_y > 0 && current_y <= 15) {                       // Grid rows 1-15
      if (note[current_x][current_y].channel == 0) {              // Only paint if empty
        note[current_x][current_y].channel = GLOB.currentChannel;  // GLOB.currentChannel should be correct 0-indexed channel
        note[current_x][current_y].velocity = defaultVelocity;
      }
    } else if (current_y == 16) {  // Top row (GLOB.y == 16)
      toggleCopyPaste();
    }
  } else {                                     // Single mode (painting for the globally selected GLOB.currentChannel)
    if ((current_y > 0 && current_y <= 15)) {  // Grid rows 1-15
      note[current_x][current_y].channel = GLOB.currentChannel;
      note[current_x][current_y].velocity = defaultVelocity;
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
      // Check if channel is set to DRUM mode (EFX = 1)
      if (SMP.filter_settings[painted_channel][EFX] == 1 && painted_channel >= 1 && painted_channel <= 3) {
        // Play drum instead of sample
        float baseTone = pianoFrequencies[constrain(pitch_from_row - 1, 0, 15)];  // pitch_from_row is 1-16, map to 0-15 for pianoFreq
        float decay = mapf(SMP.drum_settings[painted_channel][DRUMDECAY], 0, 64, 0, 1023);
        float pitchMod = mapf(SMP.drum_settings[painted_channel][DRUMPITCH], 0, 64, 0, 1023);
        float type = SMP.drum_settings[painted_channel][DRUMTYPE];
        float notePitchOffset = mapf(SMP.drum_settings[painted_channel][DRUMTONE], 0, 64, 0, 1023);  // Additional pitch
        
        // Apply detune offset for channels 1-12 (excluding synth channels 13-14)
        float detuneOffset = 0.0;
        if (painted_channel >= 1 && painted_channel <= 12) {
          detuneOffset = detune[painted_channel] * 100.0; // Convert semitones to cents for drum pitch
        }
        
        // Apply octave offset for channels 1-8 (excluding synth channels 13-14)
        float octaveOffset = 0.0;
        if (painted_channel >= 1 && painted_channel <= 8) {
          octaveOffset = channelOctave[painted_channel] * 1200.0; // Convert octaves to cents for drum pitch (12 semitones * 100 cents)
        }

        if (painted_channel == 1) KD_drum(baseTone + notePitchOffset + detuneOffset + octaveOffset, decay, pitchMod, type);  // KD on channel 1
        if (painted_channel == 2) SN_drum(baseTone + notePitchOffset + detuneOffset + octaveOffset, decay, pitchMod, type);  // SN on channel 2
        if (painted_channel == 3) HH_drum(baseTone + notePitchOffset + detuneOffset + octaveOffset, decay, pitchMod, type);  // HH on channel 3
      } else {
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
      }
    } else if (painted_channel == 11) {  // Specific synth
      playSound((12 * (int)octave[0]) + transpose + (pitch_from_row - 1), 0);
    } else if (painted_channel >= 13 && painted_channel < 15) {            // General synths
      playSynth(painted_channel, pitch_from_row, painted_velocity, false); 
                                                                           
    }
  }

  updateLastPage();
  
  // Update encoder 1 limit if pattern mode is ON or if in single mode
  if ((SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
    Encoder[1].writeMax((int32_t)lastPage);
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
          }
          // Then paste notes from the copied channel to the current channel
          if (tmp[src][y].channel == GLOB.copyChannel) {
            note[c][y].channel = GLOB.currentChannel;
            note[c][y].velocity = tmp[src][y].velocity;
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
  if ((SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
    Encoder[1].writeMax((int32_t)lastPage);
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
        }
      } else {  // Clear all notes in the range on column c regardless of their channel
        note[c][y].channel = 0;
        note[c][y].velocity = defaultVelocity;
      }
    }
  }
}




void updateVolume() {
  GLOB.vol = currentMode->pos[2];
  float vol = float(GLOB.vol / 10.0);
  //Serial.println("Vol: " + String(vol));
  if (vol >= 0.0 && vol <= 1.0) sgtl5000_1.volume(vol);  // Ensure vol is in valid range
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

void updateBPM() {
  if (MIDI_CLOCK_SEND) {
    
    //Serial.println("BPM: " + String(currentMode->pos[3]));
    SMP.bpm = currentMode->pos[3];                                    // BPM from encoder
    if (SMP.bpm > 0) {                                                // Avoid division by zero
      playNoteInterval = ((60.0 * 1000.0 / SMP.bpm) / 4.0) * 1000.0;  // Use floats for precision
      playTimer.update(playNoteInterval);
      //midiTimer.update(playNoteInterval);  // If midiTimer exists and shares interval
      
      
      // this causes delays....resetMidiClockState();
    }
  }
  drawBPMScreen();  // Assumed to exist for visual feedback
}

void setVolume() {
  showExit(0);
  drawBPMScreen();  // Assumed to exist

  // Track previous encoder position to only update brightness when it changes
  static unsigned int lastPos1 = 0;
  static bool firstCall = true;
  
  if (firstCall) {
    // On first call, just initialize lastPos1
    lastPos1 = currentMode->pos[1];
    firstCall = false;
  }
  
  if (currentMode->pos[1] != lastPos1) {
    updateBrightness();
    lastPos1 = currentMode->pos[1];
  }

  if (currentMode->pos[2] != GLOB.vol) {
    updateVolume();
  }

  if (currentMode->pos[3] != (unsigned int)SMP.bpm) {  // Cast SMP.bpm for comparison
    updateBPM();
  }
}





void showExit(int index) {
  showIcons(HELPER_EXIT, UI_DIM_BLUE);
  Encoder[index].writeRGBCode(0x0000FF);
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
  FastLED.show();

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

  // Validate samplepack value - ensure it's within valid range (1-99)
  if (SMP.pack < 1 || SMP.pack > 99) {
    Serial.print("INVALID SAMPLEPACK VALUE! Defaulting to 1");
    SMP.pack = 1;
    currentMode->pos[3] = 1;
    Encoder[3].writeCounter((int32_t)1);
    EEPROM.put(0, SMP.pack);  // Save the corrected value to EEPROM
  }

  FastLED.setBrightness(ledBrightness);
  FastLED.show();
  if (currentMode->pos[3] != SMP.pack) {
    //Serial.println("File: " + String(currentMode->pos[3]));
    SMP.pack = currentMode->pos[3];
  }
}

void loadSamplePack(unsigned int pack_id, bool intro) {  // Renamed pack to pack_id to avoid conflict
  //Serial.println("Loading SamplePack #" + String(pack_id));
  drawNoSD();
  
  // Validate pack_id - ensure it's within valid range (1-99)
  if (pack_id < 1 || pack_id > 99) {
    Serial.print("INVALID SAMPLEPACK ID! Defaulting to 1");
    pack_id = 1;
  }
  
  EEPROM.put(0, pack_id);                        // Save current pack_id to EEPROM
  for (unsigned int z = 1; z < maxFiles; z++) {  // maxFiles is 9. So loads samples 1 through 8.
                                                 // Sample arrays are often 0-indexed. _samplers[0] to _samplers[8] exist.
                                                 // This loop should probably be z=0 to maxFiles-1 or z=1 to maxFiles (inclusive for maxFiles).
                                                 // Assuming it means load into sampler slots 1 to 8.
    if (!intro) {
      showIcons(ICON_SAMPLE, UI_BG_DIM);
    } else {
      drawText("LOAD", 2, 11, col[(maxFiles + 1) - z]);
    }
    drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], UI_DIM_WHITE, intro);
    loadSample(pack_id, z);  // loadSample needs to know which sampler slot 'z' corresponds to.
  }
  // char OUTPUTf[50]; // This seems unused here
  // sprintf(OUTPUTf, "%u/%u.wav", pack_id, 1);
  switchMode(&draw);  // Switch back to draw mode after loading
  
  // Reset paint/unpaint prevention flag after loadSamplePack operation
  preventPaintUnpaint = false;
}


void updateLastPage() {
  lastPage = 0;  // Start by assuming no notes
  for (unsigned int p = 1; p <= maxPages; p++) {
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
  playSdWav1.stop();

  //Serial.println("Loading Wave :" + String(SMP.wav[GLOB.currentChannel].fileID));
  // loadSample takes (folder_or_pack_id, file_id_or_sampler_slot).
  // Here, it seems SMP.wav[GLOB.currentChannel].fileID is the actual sample ID to load.
  // And it should load into the sampler associated with GLOB.currentChannel.
  // The second argument to loadSample in loadSamplePack was 'z' (sampler slot).
  // A consistent loadSample(pack_or_folder, file_to_load_id, target_sampler_slot) would be clearer.
  // Assuming loadSample(0, SMP.wav[GLOB.currentChannel].fileID) means:
  // 0 might be a special folder/pack ID, or it implies using GLOB.folder.
  // And the sample SMP.wav[GLOB.currentChannel].fileID is loaded into sampler GLOB.currentChannel.
  // This needs clarification based on loadSample's actual implementation.
  // For now, I'll keep it as is, but it's a potential point of confusion/bugs.
  loadSample(0, SMP.wav[GLOB.currentChannel].fileID);  // Assuming GLOB.folder is the intended first arg, and second is file ID.
                                                               // And loadSample internally knows to use GLOB.currentChannel as target.
  switchMode(&singleMode);
  GLOB.singleMode = true;
  
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
        case ARR_DRUM:   return SMP.drum_settings[channel][idx];
        case ARR_PARAM:  return SMP.param_settings[channel][idx];
     
    }
}
void setDefaultFastFilterValue(int channel, SettingArray arr, int8_t idx, int value) {
    switch (arr) {
        case ARR_FILTER: SMP.filter_settings[channel][idx] = value; break;
        case ARR_SYNTH:  SMP.synth_settings[channel][idx] = value; break;
        case ARR_DRUM:   SMP.drum_settings[channel][idx] = value; break;
        case ARR_PARAM:  SMP.param_settings[channel][idx] = value; break;
        default: break;
    }
}

// Add an array to define how many filter pages to render for each channel
uint8_t filterPageCount[NUM_CHANNELS] = {0,4,4,4,3,3,3,3,3,0,0,4,0,3,3,0}; // default 4, channel 1 (index 1) has 3 pages
// ... existing code ...
// (If you want to be explicit for all channels, you can do: {4,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4})
// ... existing code ...

void setSliderDefForChannel(int channel) {
    // Channel 1-3 (index 1-3):
    if (channel >= 1 && channel <= 3) {
        static const SliderDefEntry ch1_3Template[4][4] = {
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
                {ARR_FILTER, EFX, "SND", 1, DISPLAY_ENUM, sndTypeNames, 2},
            },
            {
                {ARR_PARAM, ATTACK, "ATTC", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, DECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, SUSTAIN, "SUST", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_PARAM, RELEASE, "RLSE", 32, DISPLAY_NUMERIC, nullptr, 32}
            },
            {
                {ARR_DRUM, DRUMTONE, "NOTE", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_DRUM, DRUMDECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_DRUM, DRUMPITCH, "PTCH", 32, DISPLAY_NUMERIC, nullptr, 32},
                {ARR_DRUM, DRUMTYPE, "TYPE", 6, DISPLAY_NUMERIC, nullptr, 3}
            }
        };
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
                {ARR_FILTER, REVERB, "RVRB", 32, DISPLAY_NUMERIC, nullptr, 32},
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
void initPageMutes() {
  for (int page = 0; page < maxPages; page++) {
    for (int ch = 0; ch < maxY; ch++) {
      pageMutes[page][ch] = false;  // All channels unmuted by default
    }
  }
  // Copy current global mutes to globalMutes array
  for (int ch = 0; ch < maxY; ch++) {
    globalMutes[ch] = SMP.mute[ch];
  }
}

// Get mute state for a channel, considering PMOD setting
bool getMuteState(int channel) {
  if (SMP_PATTERN_MODE) {
    // Use page-specific mutes when PMOD is enabled
    return pageMutes[GLOB.edit - 1][channel];  // GLOB.edit is 1-indexed, array is 0-indexed
  } else {
    // Use global mutes when PMOD is disabled
    return globalMutes[channel];
  }
}

// Set mute state for a channel, considering PMOD setting
void setMuteState(int channel, bool muted) {
  if (SMP_PATTERN_MODE) {
    // Set page-specific mute when PMOD is enabled
    pageMutes[GLOB.edit - 1][channel] = muted;
  } else {
    // Set global mute when PMOD is disabled
    globalMutes[channel] = muted;
    SMP.mute[channel] = muted;  // Keep SMP.mute in sync for backward compatibility
  }
}

// Save current page mutes to global mutes when switching from PMOD to non-PMOD
void savePageMutesToGlobal() {
  for (int ch = 0; ch < maxY; ch++) {
    globalMutes[ch] = pageMutes[GLOB.edit - 1][ch];
    SMP.mute[ch] = globalMutes[ch];  // Keep SMP.mute in sync
  }
}

// Load global mutes to current page when switching from non-PMOD to PMOD
void loadGlobalMutesToPage() {
  for (int ch = 0; ch < maxY; ch++) {
    pageMutes[GLOB.edit - 1][ch] = globalMutes[ch];
  }
}
