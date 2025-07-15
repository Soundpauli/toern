//extern "C" char *sbrk(int incr);
#define FASTLED_ALLOW_INTERRUPTS 0
#define SERIAL8_RX_BUFFER_SIZE 255  // Increase to 256 bytes
#define SERIAL8_TX_BUFFER_SIZE 16  // Increase if needed for transmission
#define TargetFPS 30

//#define AUDIO_BLOCK_SAMPLES 128
#define AUDIO_SAMPLE_RATE_EXACT 44100


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
#define SWITCH_2 41  // Pin for TPP223 2
#define SWITCH_3 3   // Pin for TPP223 3 //3==lowerright, lowerleft== 15!
#define VOL_MIN 1
#define VOL_MAX 10
#define BPM_MIN 40
#define BPM_MAX 300

#define GAIN1 1
#define GAIN2 0.4   //0.5
#define GAIN3 0.4   //0.33
#define GAIN4 0.4   //0.25
#define GAIN02 0.4  //0.2;
#define GAIN01 0.4  //0.1;

#define NUM_ENCODERS 4
#define defaultVelocity 63
#define FOLDER_MAX 10

#define maxPages 16
#define maxFiles 9
#define NUM_CHANNELS 16
#define maxFilters 15

#define maxfilterResolution 32
#define numPulsesForAverage 8  // Number of pulses to average over
#define pulsesPerBar (24 * 4)  // 24 pulses per quarter note, 4 quarter notes per bar

#define EEPROM_MENU_ADDR 42

struct MySettings : public midi ::DefaultSettings {
  static const long BaudRate = 1000000;
  static const unsigned SysExMaxSize = 16;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial8, MIDI, MySettings);
unsigned long beatStartTime = 0;  // Timestamp when the current beat started

#define CLOCK_BUFFER_SIZE 24
//elapsedMillis recFlushTimer;
elapsedMillis recTime;


bool lastPinsConnected = false;
unsigned long lastChangeTime = 0;
const unsigned long debounceDelay = 500;  // 100ms debounce

#define EEPROM_MAGIC_ADDR 42
#define EEPROM_DATA_START 43        // 43..48 will be your six mode‐bytes
const uint8_t EEPROM_MAGIC = 0x5A;  // anything nonzero




enum ValueDisplayMode {
  DISPLAY_NUMERIC,
  DISPLAY_ENUM
};

const char* instTypeNames[] = { "WV", "SY", "DR" };
#define INST_ENUM_COUNT (sizeof(instTypeNames) / sizeof(instTypeNames[0]))


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
    {28, DISPLAY_NUMERIC, nullptr, 28},
    {20, DISPLAY_NUMERIC, nullptr, 20},
    {32, DISPLAY_NUMERIC, nullptr, 32},
    {32, DISPLAY_NUMERIC, nullptr, 32}
  },
  // Page 2
  {
    {16, DISPLAY_NUMERIC, nullptr, 5},                  // Shown as 0–4
    {16, DISPLAY_ENUM, instTypeNames, INST_ENUM_COUNT}, // Mapped from 0–16
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
unsigned int SMP_FAST_REC = false;
unsigned int SMP_REC_CHANNEL_CLEAR = true;

bool pendingStartOnBar = false;  // “I hit Play, now wait for bar-1”



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

const char *SynthVoices[11] = { nullptr, "BASS", "KEYS", "CHPT", "PAD", "WOW", "ORG", "FLT", "LEAD", "ARP", "BRSS" };
const char *channelType[5] = { nullptr, "DRUM", "SMP", "SYNTH", "X" };
const char *menuText[12] = { "DAT", "KIT", "WAV", "REC", "BPM", "CLCK", "CHAN", "TRAN", "PMOD", "OTR", "CLR", "PVOL" };

unsigned int infoIndex = 0;

int lastFile[9] = { 0 };
bool freshPaint, tmpMute = false;


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


unsigned int menuPosition = 1;
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
int recChannelClear = 1;
int transportMode = 1;
int patternMode = -1;
int clockMode = 1;
int voiceSelect = 1;

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



const float fullFrequencies[27] PROGMEM = {
  130.81,  // C3
  138.59,  // C#3/Db3
  146.83,  // D3
  155.56,  // D#3/Eb3
  164.81,  // E3
  174.61,  // F3
  185.00,  // F#3/Gb3
  196.00,  // G3
  207.65,  // G#3/Ab3
  220.00,  // A3
  233.08,  // A#3/Bb3
  246.94,  // B3
  261.63,  // C4. // MITTE
  277.18,  // C#4/Db4
  293.66,  // D4
  311.13,  // D#4/Eb4
  329.63,  // E4
  349.23,  // F4
  369.99,  // F#4/Gb4
  392.00,  // G4
  415.30,  // G#4/Ab4
  440.00,  // A4
  466.16,  // A#4/Bb4
  493.88,  // B4
  523.25,  // C5
  554.37,  // C#5/Db5
  587.33   // D5
};


const float pianoFrequencies[16] PROGMEM = {
  130.81,  // C3
  146.83,  // D3
  164.81,  // E3
  174.61,  // F3
  196.00,  // G3
  220.00,  // A3
  246.94,  // B3
  261.62,  // C4 (Mitte)
  293.66,  // D4
  329.62,  // E4
  349.22,  // F4
  391.99,  // G4
  440.00,  // A4
  493.88,  // B4
  523.25,  // C5
  587.33   // D5
};

const char* pianoNoteNames[27] = {
  "C3",  "C#3", "D3",  "D#3", "E3",
  "F3",  "F#3", "G3",  "G#3", "A3",
  "A#3", "B3",  "C4",  "C#4", "D4",
  "D#4", "E4",  "F4",  "F#4", "G4",
  "G#4", "A4",  "A#4", "B4",  "C5",
  "C#5", "D5"
};

elapsedMillis msecs;
elapsedMillis mRecsecs;


CRGB leds[NUM_LEDS];

enum ButtonState {
  IDLE,
  LONG_PRESSED,
  RELEASED
};


struct Mode {
  String name;
  unsigned int minValues[4];
  unsigned int maxValues[4];
  unsigned int pos[4];
  uint32_t knobcolor[4];
};

Mode draw = { "DRAW", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxlen - 1 }, { 1, 1, maxfilterResolution, 1 }, { 0x110011, 0xFFFFFF, 0x00FF00, 0x110011 } };
Mode singleMode = { "SINGLE", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxlen - 1 }, { 1, 2, maxfilterResolution, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode volume_bpm = { "VOLUME_BPM", { 1, 6, VOL_MIN, BPM_MIN }, { 1, 25, VOL_MAX, BPM_MAX }, { 1, 6, 7, 100 }, { 0x000000, 0xFFFFFF, 0xFF4400, 0x00FFFF } };  //OK
//filtermode has 4 entries
Mode filterMode = { "FILTERMODE", { 0, 0, 0, 0 }, { maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution }, { 1, 1, 1, 1 }, { 0x000000, 0x000000, 0x000000, 0x000000 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 7, 0, 7 }, { 9, 9, maxfilterResolution, 9 }, { 8, 8, maxfilterResolution, 8 }, { 0xFFFF00, 0x000000, 0x000000, 0xFFFFFF } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { maxY, 1, maxY, maxY }, { maxY, 1, 10, 10 }, { 0xFF4400, 0x000000, 0x0044FF, 0x888888 } };

Mode set_Wav = { "SET_WAV", { 1, 1, 1, 1 }, { 9999, FOLDER_MAX, 9999, 999 }, { 0, 0, 0, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 1 }, { 1, 1, 99, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode loadSaveTrack = { "LOADSAVE_TRACK", { 1, 1, 1, 1 }, { 1, 1, 1, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode menu = { "MENU", { 1, 1, 1, 1 }, { 1, 1, 1, 12 }, { 1, 1, 1, 1 }, { 0x000000, 0x000000, 0x000000, 0x00FF00 } };
// Declare currentMode as a global variable
Mode *currentMode;



struct Sample {
  unsigned int oldID : 10;   // values 0-1023, enough for 0-999
  unsigned int fileID : 10;  // values 0-1023, enough for 0-999
} __attribute__((packed));


// Declare the device struct
struct Device {
  unsigned int singleMode;  // single Sample Mod
  unsigned int currentChannel;
  unsigned int vol;       // volume
  float bpm;              // bpm
  unsigned int velocity;  // velocity
  unsigned int page;      // current page
  unsigned int edit;      // edit mode or plaing mode?
  unsigned int file;      // current selected save/load id
  unsigned int pack;      // current selected samplepack id
  Sample wav[maxFiles];   // current selected sample
  unsigned int folder;    // current selected folder id
  bool activeCopy;        // is copy/paste active?
  unsigned int x;         // cursor X
  unsigned int y;         // cursor Y
  unsigned int seek;      // skipped into sample
  unsigned int seekEnd;
  unsigned int smplen;  // overall selected samplelength
  unsigned int shiftX;  // note Shift
  unsigned int shiftY;  // note Shift
  float param_settings[maxY][maxFilters];
  unsigned int selectedParameter;
  float filter_settings[maxY][maxFilters];
  float drum_settings[maxY][4];
  unsigned int selectedFilter;
  unsigned int selectedDrum;
  float synth_settings[maxY][8];
  unsigned int selectedSynth;
  unsigned int mute[maxY];
  unsigned int channelVol[maxY];
};

float octave[2];

// in the same file, before tmpMuteAll:





//EXTMEM?
Device SMP = {
  false,                                                                               //singleMode
  1,                                                                                   //currentChannel
  10,                                                                                  //volume
  100.0,                                                                               //bpm
  10,                                                                                  //velocity
  1,                                                                                   //page
  1,                                                                                   //edit
  1,                                                                                   //file
  1,                                                                                   //pack
  { { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 } },  //wav preset
  0,                                                                                   //folder
  false,                                                                               //activeCopy
  1,                                                                                   //x
  2,                                                                                   //y
  0,                                                                                   //seek
  0,                                                                                   //seekEnd
  0,                                                                                   //sampleLen
  0,                                                                                   //shiftX
  0,                                                                                   //shiftY
  {},                                                                                  //param_settings
  0,                                                                                   //selectedParameter
  {},                                                                                  //filter_settings
  {},                                                                                  //drum_settings
  0,                                                                                   //selectedFilter
  0,                                                                                   //selectedDrum
  {},                                                                                  //synth_settings
  0,                                                                                   //selectedSynth
  {},                                                                                  //mute
  { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }                   //channelVol
};

#define NOTE_C0 (16.35)
#define NOTE_CS0 (17.32)
#define NOTE_D0 (18.35)
#define NOTE_DS0 (19.45)
#define NOTE_E0 (20.60)
#define NOTE_F0 (21.83)
#define NOTE_FS0 (23.12)
#define NOTE_G0 (24.50)
#define NOTE_GS0 (25.96)
#define NOTE_A0 (27.50)
#define NOTE_AS0 (29.14)
#define NOTE_B0 (30.87)
#define NOTE_C1 (32.70)
#define NOTE_CS1 (34.65)
#define NOTE_D1 (36.71)
#define NOTE_DS1 (38.89)
#define NOTE_E1 (41.20)
#define NOTE_F1 (43.65)
#define NOTE_FS1 (46.25)
#define NOTE_G1 (49.00)
#define NOTE_GS1 (51.91)
#define NOTE_A1 (55.00)
#define NOTE_AS1 (58.27)
#define NOTE_B1 (61.74)
#define NOTE_C2 (65.41)
#define NOTE_CS2 (69.30)
#define NOTE_D2 (73.42)
#define NOTE_DS2 (77.78)
#define NOTE_E2 (82.41)
#define NOTE_F2 (87.31)
#define NOTE_FS2 (92.50)
#define NOTE_G2 (98.00)
#define NOTE_GS2 (103.83)
#define NOTE_A2 (110.00)
#define NOTE_AS2 (116.54)
#define NOTE_B2 (123.47)
#define NOTE_C3 (130.81)
#define NOTE_CS3 (138.59)
#define NOTE_D3 (146.83)
#define NOTE_DS3 (155.56)
#define NOTE_E3 (164.81)
#define NOTE_F3 (174.61)
#define NOTE_FS3 (185.00)
#define NOTE_G3 (196.00)
#define NOTE_GS3 (207.65)
#define NOTE_A3 (220.00)
#define NOTE_AS3 (233.08)
#define NOTE_B3 (246.94)
#define NOTE_C4 (261.63)
#define NOTE_CS4 (277.18)
#define NOTE_D4 (293.66)
#define NOTE_DS4 (311.13)
#define NOTE_E4 (329.63)
#define NOTE_F4 (349.23)
#define NOTE_FS4 (369.99)
#define NOTE_G4 (392.00)
#define NOTE_GS4 (415.30)
#define NOTE_A4 (440.00)
#define NOTE_AS4 (466.16)
#define NOTE_B4 (493.88)
#define NOTE_C5 (523.25)
#define NOTE_CS5 (554.37)
#define NOTE_D5 (587.33)
#define NOTE_DS5 (622.25)
#define NOTE_E5 (659.25)
#define NOTE_F5 (698.46)
#define NOTE_FS5 (739.99)
#define NOTE_G5 (783.99)
#define NOTE_GS5 (830.61)
#define NOTE_A5 (880.00)
#define NOTE_AS5 (932.33)
#define NOTE_B5 (987.77)
#define NOTE_C6 (1046.50)
#define NOTE_CS6 (1108.73)
#define NOTE_D6 (1174.66)
#define NOTE_DS6 (1244.51)
#define NOTE_E6 (1318.51)
#define NOTE_F6 (1396.91)
#define NOTE_FS6 (1479.98)
#define NOTE_G6 (1567.98)
#define NOTE_GS6 (1661.22)
#define NOTE_A6 (1760.00)
#define NOTE_AS6 (1864.66)
#define NOTE_B6 (1975.53)
#define NOTE_C7 (2093.00)
#define NOTE_CS7 (2217.46)
#define NOTE_D7 (2349.32)
#define NOTE_DS7 (2489.02)
#define NOTE_E7 (2637.02)
#define NOTE_F7 (2793.83)
#define NOTE_FS7 (2959.96)
#define NOTE_G7 (3135.96)
#define NOTE_GS7 (3322.44)
#define NOTE_A7 (3520.00)
#define NOTE_AS7 (3729.31)
#define NOTE_B7 (3951.07)
#define NOTE_C8 (4186.01)
#define NOTE_CS8 (4434.92)
#define NOTE_D8 (4698.63)
#define NOTE_DS8 (4978.03)
#define NOTE_E8 (5274.04)
#define NOTE_F8 (5587.65)
#define NOTE_FS8 (5919.91)
#define NOTE_G8 (6271.93)
#define NOTE_GS8 (6644.88)
#define NOTE_A8 (7040.00)
#define NOTE_AS8 (7458.62)
#define NOTE_B8 (7902.13)

float notesArray[108] = { NOTE_C0, NOTE_CS0, NOTE_D0, NOTE_DS0, NOTE_E0, NOTE_F0, NOTE_FS0, NOTE_G0, NOTE_GS0, NOTE_A0, NOTE_AS0, NOTE_B0,
                          NOTE_C1, NOTE_CS1, NOTE_D1, NOTE_DS1, NOTE_E1, NOTE_F1, NOTE_FS1, NOTE_G1, NOTE_GS1, NOTE_A1, NOTE_AS1, NOTE_B1,
                          NOTE_C2, NOTE_CS2, NOTE_D2, NOTE_DS2, NOTE_E2, NOTE_F2, NOTE_FS2, NOTE_G2, NOTE_GS2, NOTE_A2, NOTE_AS2, NOTE_B2,
                          NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,
                          NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
                          NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
                          NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
                          NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7,
                          NOTE_C8, NOTE_CS8, NOTE_D8, NOTE_DS8, NOTE_E8, NOTE_F8, NOTE_FS8, NOTE_G8, NOTE_GS8, NOTE_A8, NOTE_AS8, NOTE_B8 };


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

//EXTMEM int16_t fastRecBuffer[MAX_CHANNELS][BUFFER_SAMPLES];
static size_t fastRecWriteIndex[MAX_CHANNELS];
bool fastRecordActive = false;
//static uint8_t fastRecordChannel = 0;


// State variables
uint8_t filterPage = 0; // 0-3 pages
uint8_t lastEncoder = 0; // last used encoder index (0-3)



enum ParameterType { DELAY,
                     ATTACK,
                     HOLD,
                     DECAY,
                     SUSTAIN,
                     RELEASE,
                     WAVEFORM,
                     TYPE  //AudioSynthNoiseWhite, AudioSynthSimpleDrums OR WAVEFORM
};                         

int maxParamVal[12] = { 1000, 2000, 1000, 1000, 1, 1000, 1000, 1, 1, 0, 0 };



enum FilterType { NUL,
                  LOWPASS,
                  HIGHPASS,
                  FREQUENCY,
                  REVERB,
                  BITCRUSHER,
                  DETUNE,
                  OCTAVE
};  //Define filter types





enum DrumTypes { DRUMTONE,
                 DRUMDECAY,
                 DRUMPITCH,
                 DRUMTYPE
};  //Define drum types


enum SynthTypes { INSTRUMENT,
                  PARAM1,
                  PARAM2,
                  PARAM3,
                  PARAM4,
                  PARAM5,
                  PARAM6,
};  //Define Synth types


enum MidiSetTypes {
  MIDI_IN_BOOL,
  MIDI_OUT_BOOL,
  SINGLE_OUT_CHANNEL,
  GLOBAL_IN_CHANNEL,
  SEND_CTRL_BOOL,
  RECEIVE_CTRL_BOOL,
};  //Define midi types

FilterType defaultFilter[maxFiles] = { LOWPASS };

char *activeParameterType[8] = { "DLAY", "ATTC", "HOLD", "DCAY", "SUST", "RLSE", "WAV", "TYPE" };
char *activeFilterType[8] = { "", "LOW", "HIGH", "FREQ", "RVRB", "BITC", "DTNE", "OCTV" };
char *activeDrumType[4] = { "TONE", "DCAY", "FREQ", "TYPE" };
char *activeSynthVoice[8] = { "SND", "CUT", "RES", "FLT", "CENT", "SEMI", "WAVE", "PAR7" };
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



// For each page and slot: {arrayType, settingIndex}
static const struct { SettingArray arr; int8_t idx; } sliderDef[4][4] = {
  // page 0: filters
  {{ARR_FILTER, LOWPASS}, {ARR_FILTER, FREQUENCY}, {ARR_FILTER, REVERB},   {ARR_FILTER, BITCRUSHER}},
  // page 1: synth & filter octave
  {{ ARR_SYNTH ,   PARAM1},     {ARR_SYNTH,   PARAM2}, {ARR_SYNTH, PARAM3}, {ARR_FILTER, OCTAVE}},
  // page 2: waveform & voice
  {{ARR_SYNTH,   WAVEFORM}, {ARR_SYNTH,   INSTRUMENT},      {ARR_FILTER, DETUNE},        {ARR_SYNTH,   TYPE}},
  // page 3: DAHDSR params
  {{ARR_PARAM,   ATTACK},  {ARR_PARAM,   DECAY},     {ARR_PARAM, SUSTAIN},        {ARR_PARAM,  RELEASE}}
};  




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
AudioEffectFreeverb *freeverbs[15] = { nullptr, &freeverb1, &freeverb2, nullptr, nullptr, nullptr, nullptr, &freeverb7, &freeverb8, 0, 0, &freeverb11, nullptr, &freeverb13, &freeverb14 };
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
  if (currentMode->pos[0] != SMP.velocity) {
    SMP.velocity = currentMode->pos[0];

    if (SMP.singleMode) {
      note[SMP.x][SMP.y].velocity = round(mapf(SMP.velocity, 1, maxY, 1, 127));
    } else {

      for (unsigned int nx = 1; nx < maxlen; nx++) {
        for (unsigned int ny = 1; ny < maxY + 1; ny++) {
          if (note[nx][ny].channel == note[SMP.x][SMP.y].channel)
            note[nx][ny].velocity = round(mapf(SMP.velocity, 1, maxY, 1, 127));
        }
      }
    }
  }
  //CHANNEL VOLUME
  if (currentMode->pos[2] != SMP.velocity) {
    SMP.channelVol[SMP.currentChannel] = currentMode->pos[2];
    float channelvolume = mapf(SMP.channelVol[SMP.currentChannel], 1, maxY, 0, 1);
    //Serial.println(channelvolume);
    amps[SMP.currentChannel]->gain(channelvolume);
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
  SMP.singleMode = false;
  paintMode = false;
  Mode *oldMode = currentMode;
  /// OLD ACTIONS
  if (currentMode == &set_Wav || currentMode == &filterMode) {
    //RESET left encoder
    SMP.selectedParameter = 0;
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
      Encoder[i].writeMax((int32_t)currentMode->maxValues[i]);  //maxval
      Encoder[i].writeMin((int32_t)currentMode->minValues[i]);  //minval

      if ((currentMode == &singleMode && oldMode == &draw) || (currentMode == &draw && oldMode == &singleMode)) {
        //do not move Cursor for those modes
        //
      } else {
        Encoder[i].writeCounter((int32_t)currentMode->pos[i]);
      }
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

      initSliders(filterPage);
      Encoder[0].begin(
        i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
        | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
        | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);

      //SMP.selectedFX = TYPE;
      //Encoder[3].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel].channel);
    }
  }

  drawPlayButton();
}


void checkFastRec() {
if (SMP.currentChannel>8) return;

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
  // MODIFIED SIGNATURE
  //reset all buttons
  /*for (int i = 0; i < NUM_ENCODERS; i++) {
    if (buttons[i] == 9 || buttons[i] == 1) {  // These are release events, reset them to 0 after processing
      buttons[i] = 0;
      // buttonState[i] = IDLE; // This should be handled by handle_button_state or here if truly done
      // isPressed[i] = false;
    }
  }*/


  //checkFastRec();

  if (isRecording && match_buttons(currentButtonStates, 0, 9, 0, 0)) {  // "0900"
    //stopRecord(getFolderNumber(SMP.wav[SMP.currentChannel].fileID), SMP.wav[SMP.currentChannel].fileID);
    stopRecordingRAM(getFolderNumber(SMP.wav[SMP.currentChannel].fileID), SMP.wav[SMP.currentChannel].fileID);
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

  // Shift notes around in single mode after dblclick of button 4
  if (currentMode == &singleMode && match_buttons(currentButtonStates, 2, 2, 0, 0)) {  // "2200"
    SMP.shiftX = 8;
    SMP.shiftY = 8;

    Encoder[3].writeCounter((int32_t)8);
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
        if (note[nx][ny].channel != SMP.currentChannel) {
          original[nx][ny] = note[nx][ny];
        }
      }
    }
    // Switch to note shift mode
    switchMode(&noteShift);
    SMP.singleMode = true;
  }

  if (currentMode == &menu && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    switchMode(&draw);
    SMP.singleMode = false;
  }

  if (currentMode == &menu && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    switchMenu(menuPosition);
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    paintMode = false;
    unpaintMode = false;
    switchMode(&draw);
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    savePattern(false);
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    loadPattern(false);
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    saveSamplePack(SMP.pack);
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    loadSamplePack(SMP.pack, false);
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    switchMode(&draw);
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"

    loadWav();
    autoSave();
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    switchMode(&singleMode);
    SMP.singleMode = true;
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 0, 2, 0, 0)) {  // "0200"
    startRecordingRAM();
  }

  if (currentMode == &noteShift && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    switchMode(&singleMode);
    SMP.singleMode = true;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0001"
    freshPaint = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 2, 2, 0)) {  // "0220"
    switchMode(&volume_bpm);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 2, 0)) {  // "0020"
    if (SMP.currentChannel == 0 || SMP.currentChannel == 9 || SMP.currentChannel == 10 || SMP.currentChannel == 12 || SMP.currentChannel == 15) return;
    fxType = 0;
    switchMode(&filterMode);
  }

  if (currentMode == &draw && match_buttons(currentButtonStates, 1, 1, 0, 0)) {  // "1100"
    //toggleCopyPaste();
  }

  if ((SMP.y > 15) && (currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0002"
    drawRandoms();
    return;
  }

  if (!freshPaint && note[SMP.x][SMP.y].channel != 0 && (currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0002"
    unsigned int velo = round(mapf(note[SMP.x][SMP.y].velocity, 1, 127, 1, maxY));
    SMP.velocity = velo;
    switchMode(&velocity);
    SMP.singleMode = true;
    Encoder[0].writeCounter((int32_t)velo);
  }

  if (!freshPaint && note[SMP.x][SMP.y].channel != 0 && (currentMode == &draw) && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0002"
    unsigned int velo = round(mapf(note[SMP.x][SMP.y].velocity, 1, 127, 1, maxY));
    unsigned int chvol = SMP.channelVol[SMP.currentChannel];
    SMP.velocity = velo;
    SMP.singleMode = false;
    switchMode(&velocity);
    Encoder[0].writeCounter((int32_t)velo);
    Encoder[3].writeCounter((int32_t)chvol);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 2, 0, 0, 2)) {  // "2002"
    clearPage();
  }

  if (currentMode == &velocity && match_buttons(currentButtonStates, 0, 0, 0, 9)) {  // "0009"
    if (!SMP.singleMode) {
      switchMode(&draw);
    } else {
      switchMode(&singleMode);
      SMP.singleMode = true;
    }
  }


  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 0, 1)) {  // "0010"
setDefaultFilterFromSlider(filterPage,3);
  }


  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 1, 0)) {  // "0010"
setDefaultFilterFromSlider(filterPage,2);

/*
    if (fxType == 0) {
      defaultFilter[SMP.currentChannel] = (FilterType)SMP.selectedParameter;  // Cast needed if SMP.selectedParameter is int
    } else if (fxType == 1) {
      defaultFilter[SMP.currentChannel] = (FilterType)SMP.selectedFilter;  // Cast needed
    } else if (fxType == 2) {
      defaultFilter[SMP.currentChannel] = (FilterType)SMP.selectedDrum;  // Cast needed
    } else if (fxType == 4) {
      defaultFilter[SMP.currentChannel] = (FilterType)SMP.selectedSynth;  // Cast needed
    }
    */
  }

/*
  if (currentMode == &filterMode && match_buttons(currentButtonStates, 2, 0, 0, 0)) {  // "2000"
    setDahdsrDefaults(false);
    currentFilter = activeParameterType[SMP.selectedParameter];
    currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][SMP.selectedParameter];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0002"
    setFilterDefaults(SMP.currentChannel);
    currentFilter = activeFilterType[SMP.selectedFilter];
    currentMode->pos[3] = SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
  }

  */


  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    setDefaultFilterFromSlider(filterPage,1);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
  setDefaultFilterFromSlider(filterPage,0);
  /*
    if (!SMP.singleMode) {
      switchMode(&draw);
    } else {
      switchMode(&singleMode);
      SMP.singleMode = true;
    }

    
    if (defaultFilter[SMP.currentChannel] < sizeof(activeFilterType) / sizeof(activeFilterType[0]) && SMP.filter_settings[SMP.currentChannel][defaultFilter[SMP.currentChannel]] >= Encoder[2].readMin() && SMP.filter_settings[SMP.currentChannel][defaultFilter[SMP.currentChannel]] <= Encoder[2].readMax()) {
      Encoder[2].writeCounter((int32_t)SMP.filter_settings[SMP.currentChannel][defaultFilter[SMP.currentChannel]]);
    }*/

  }

  if (currentMode == &volume_bpm && match_buttons(currentButtonStates, 1, 0, 0, 0)) {  // "1000"
    switchMode(&draw);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 9, 0, 9)) {  // "0909"
    paintMode = false;
    unpaintMode = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 9)) {  // "0009"
    freshPaint = false;
    paintMode = false;
    unpaintMode = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 9, 0, 0, 0)) {  // "9000"
    unpaintMode = false;
    paintMode = false;
  }

  if ((currentMode == &draw) && match_buttons(currentButtonStates, 0, 0, 2, 2)) {  // "0022"
    //switchMode(&loadSaveTrack);
  }

  if ((currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 2, 2)) {  // "0022"
    //set loaded sample
  }

  if ((currentMode == &draw) && match_buttons(currentButtonStates, 2, 2, 0, 0)) {  // "2200"
    //switchMode(&set_SamplePack);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {  // "0100"
    toggleMute();
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2)) {  // "0002"
    paintMode = true;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 2, 0, 0, 0)) {  // "2000"
    unpaint();
    unpaintMode = true;
    deleteActiveCopy();
  }

  // Assuming '3' is a valid state that buttons[i] can take.
  // If not, this block is dead code.
  if (currentMode == &singleMode && match_buttons(currentButtonStates, 3, 0, 0, 0)) {  // "3000"
    switchMode(&draw);
  } else if (currentMode == &draw && match_buttons(currentButtonStates, 3, 0, 0, 0) && ((SMP.currentChannel >= 1 && SMP.currentChannel <= maxFiles) || SMP.currentChannel > 12)) {  // "3000"
    SMP.currentChannel = SMP.currentChannel;
    switchMode(&singleMode);
    SMP.singleMode = true;
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
  }
}








void initSoundChip() {
  // AudioInterrupts();

  AudioMemory(64);
  // turn on the output
  sgtl5000_1.enable();
  sgtl5000_1.volume(1.0);

  //sgtl5000_1.autoVolumeControl(1, 1, 0, -6, 40, 20);
  //sgtl5000_1.audioPostProcessorEnable();
  //sgtl5000_1.audioPreProcessorEnable();
  //sgtl5000_1.audioPostProcessorEnable();
  //   sgtl5000_1.autoVolumeEnable();

  //REC

  sgtl5000_1.inputSelect(recInput);
  sgtl5000_1.micGain(10);  //0-63
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

  mixer1.gain(0, GAIN4);
  mixer1.gain(1, GAIN4);
  mixer1.gain(2, GAIN4);
  mixer1.gain(3, GAIN4);


  freeverbmixer8.gain(0, GAIN02);
  freeverbmixer8.gain(1, GAIN02);
  freeverbmixer8.gain(2, GAIN02);
  freeverbmixer8.gain(3, GAIN02);

  filtermixer8.gain(0, GAIN02);
  filtermixer8.gain(1, GAIN02);
  filtermixer8.gain(2, GAIN02);
  filtermixer8.gain(3, GAIN02);


  synthmixer11.gain(0, GAIN3);
  synthmixer11.gain(1, GAIN3);
  synthmixer11.gain(3, GAIN3);



  synthmixer13.gain(0, GAIN01);
  synthmixer13.gain(1, GAIN01);
  synthmixer13.gain(3, GAIN01);

  synthmixer14.gain(0, GAIN01);
  synthmixer14.gain(1, GAIN01);
  synthmixer14.gain(3, GAIN01);


  mixersynth_end.gain(0, GAIN4);
  
  mixersynth_end.gain(2, GAIN4);
  mixersynth_end.gain(3, GAIN4);


  mixer0.gain(0, GAIN01);  //PREV
  mixer0.gain(1, GAIN01);  //PREV
  

  mixer1.gain(0, GAIN4);
  mixer1.gain(1, GAIN4);
  mixer1.gain(2, GAIN4);
  mixer1.gain(3, GAIN4);

  mixer2.gain(0, GAIN4);
  mixer2.gain(1, GAIN4);
  mixer2.gain(2, GAIN4);
  mixer2.gain(3, GAIN4);


  mixer_end.gain(0, GAIN02);
  mixer_end.gain(1, GAIN02);
  mixer_end.gain(2, GAIN02);


  mixerPlay.gain(0, GAIN02);
  mixerPlay.gain(1, GAIN02);
  

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

  amplitude[13][0] = 0.3f;
  amplitude[13][1] = 0.3f;

  amplitude[14][0] = 0.3f;
  amplitude[14][1] = 0.3f;



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

  //set Defaults
  setDahdsrDefaults(true);
  for (int ch = 1; ch <= 14; ch++) {
    setFilterDefaults(ch);
  }


  setDrumDefaults(true);
  testDrums();
  bass_synth(0, 0, 0, 0, 0, 0, 0);
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
    Encoder[i].writeRGBCode(0xFFFFFF);
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

  // if (CrashReport) { checkCrashReport(); } // Moved this line after Serial.begin()

  EEPROM.get(0, samplePackID);

  ////Serial.print("SamplePackID:");
  //Serial.println(samplePackID);

  if (isnan(samplePackID) || samplePackID == 0) {  // Check for NaN properly
    ////Serial.print("NO SAMPLEPACK SET! Defaulting to 1");
    samplePackID = 1;
  }

  pinMode(INT_PIN, INPUT_PULLUP);  // Interrups for encoder
  pinMode(0, INPUT_PULLDOWN);
  pinMode(SWITCH_3, INPUT_PULLDOWN);  // Use defined name
  pinMode(SWITCH_1, INPUT_PULLDOWN);  // Use defined name

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
  mixer0.gain(1, 0.05);  //PREV Sound
  playSdWav1.play("intro/016.wav");
  
  
  runAnimation();
  playSdWav1.stop();
  EEPROMgetLastFiles();
  loadMenuFromEEPROM();
  loadSamplePack(samplePackID, true);
  SMP.bpm = 100.0;
  initSamples();
  //playTimer.priority(118);
  playTimer.begin(playNote, playNoteInterval);
  //midiTimer.begin(checkMidi, playNoteInterval);
  //midiTimer.priority(10);
  //

  autoLoad();
  //setFilters();

  switchMode(&draw);

  Serial8.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);  // optional MIDI library hook
  //MIDI.setHandleClock(myClock);       // optional if you're using callbacks
  resetMidiClockState();
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
      SMP.x = currentMode->pos[3];
      SMP.y = currentMode->pos[0];

      if (currentMode == &draw) {
        // offset y by one (notes (channel 1/red) start at row #1)
        SMP.currentChannel = SMP.y - 1;
        //write the current filterSetting to the Encoder
        FilterType dfx = defaultFilter[SMP.currentChannel];
        if (dfx < sizeof(activeFilterType) / sizeof(activeFilterType[0]) && SMP.filter_settings[SMP.currentChannel][dfx] >= Encoder[2].readMin() && SMP.filter_settings[SMP.currentChannel][dfx] <= Encoder[2].readMax()) {
          Encoder[2].writeCounter((int32_t)SMP.filter_settings[SMP.currentChannel][dfx]);
        }
        filterfreshsetted = true;
      }

      if (currentMode == &singleMode){
        drawText(pianoNoteNames[12 - SMP.y + 1  + SMP.currentChannel], 3, 5, CRGB(200, 50, 0));
      }


      Encoder[0].writeRGBCode(CRGBToUint32(col[SMP.currentChannel]));
      Encoder[3].writeRGBCode(CRGBToUint32(col[SMP.currentChannel]));
      SMP.edit = getPage(SMP.x);
    }



    if ((SMP.y > 1 && SMP.y <= 14)) {  // SMP.y is 1-based from encoder
      if (paintMode) {
        note[SMP.x][SMP.y].channel = SMP.currentChannel;  // SMP.currentChannel is 0-based
      }
      if (paintMode && currentMode == &singleMode) {
        note[SMP.x][SMP.y].channel = SMP.currentChannel;
      }


      if (unpaintMode) {
        if (SMP.singleMode) {
          if (note[SMP.x][SMP.y].channel == SMP.currentChannel)
            note[SMP.x][SMP.y].channel = 0;
        } else {
          note[SMP.x][SMP.y].channel = 0;
        }
      }
    }


    if (currentMode->pos[1] != editpage) {
      updateLastPage();
      editpage = currentMode->pos[1];
      //Serial.println("p:" + String(editpage));
      int xval = mapXtoPageOffset(SMP.x) + ((editpage - 1) * 16);
      Encoder[3].writeCounter((int32_t)xval);
      SMP.x = xval;
      SMP.edit = editpage;
      if (SMP_PATTERN_MODE) {
        patternChangeTime = millis() + 2000;  // 2 seconds window
        patternChangeActive = true;
      }
    }


    
  filtercheck();

  

  if (filterDrawActive) {
    Serial.print("active:");
      if (millis() <= filterDrawEndTime) {
        Serial.print("draw");
        FilterType fx = defaultFilter[SMP.currentChannel];
        if (fx) drawFilterCheck(SMP.filter_settings[SMP.currentChannel][fx], fx);
      } else {
        // Stop drawing after 2 seconds
        filterDrawActive = false;
      }
    }


  }
}


void filtercheck() {
  FilterType fx = defaultFilter[SMP.currentChannel];
  if (fx && fxType == 1 && !filterfreshsetted && currentMode->pos[2]!=0 && currentMode->pos[2] != SMP.filter_settings[SMP.currentChannel][fx]) {

      Serial.println("filtercheck TRIGGER! Mode: " + currentMode->name +
                   ", fxType: " + String(fxType) +
                   ", !filterfreshsetted: " + String(!filterfreshsetted) +
                   ", enc2_pos: " + String(currentMode->pos[2]) +
                   ", stored_setting: " + String(SMP.filter_settings[SMP.currentChannel][fx]) +
                   ", channel: " + String(SMP.currentChannel));

    SMP.filter_settings[SMP.currentChannel][fx] = currentMode->pos[2];
    float mappedValue = processFilterAdjustment(fx, SMP.currentChannel, 2);
    updateFilterValue(fx, SMP.currentChannel, mappedValue);

    // Activate the 2-second drawing period
    filterDrawActive = true;
    filterDrawEndTime = millis() + 2000;  // 2 seconds window
    drawFilterCheck(SMP.filter_settings[SMP.currentChannel][fx], fx);
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

  // 3) detect “rising edge” of both‐pressed
  bool bothTouched = touchState[0] && touchState[1];
  bool newBoth = bothTouched && !lastBothTouched;
  lastBothTouched = bothTouched;

  if (newBoth) {
    // only on the first frame where both are pressed:
    if (currentMode == &singleMode) {
      //Serial.println("---------<>---------");
      SMP.singleMode = false;
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
        if (!(SMP.currentChannel == 0 || SMP.currentChannel == 9 || SMP.currentChannel == 10 || SMP.currentChannel == 12 || SMP.currentChannel == 15)) {
          animateSingle();
        }
      } else {  // If in any other mode, and Switch 1 is touched, go to draw mode.
        switchMode(&draw);
        SMP.singleMode = false;
      }
    }

    // SWITCH_2
    if (currentMode != &filterMode && touchState[1] && !lastTouchState[1]) {
      if (currentMode == &draw || currentMode == &singleMode) {
        switchMode(&menu);
      } else {  // If in any other mode (e.g. menu, set_wav, etc.) and Switch 2 is touched, go to draw mode.
        switchMode(&draw);
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
  int yStart = SMP.currentChannel;              // 0 to 15
  int yCenter = yStart + 1;                     // Because y grid is 1-based
  CRGB animCol = col_base[SMP.currentChannel];  // Renamed to avoid conflict with global 'col'

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
  SMP.singleMode = true;
}

void checkSingleTouch() {
  int touchValue = fastTouchRead(SWITCH_1);

  // Determine if the touch is above the threshold
  touchState[0] = (touchValue > touchThreshold);
  // Check for a rising edge (LOW to HIGH transition)
  if (touchState[0] && !lastTouchState[0]) {
    // Toggle the mode only on a rising edge
    if (currentMode == &draw) {
      // SMP.currentChannel = SMP.currentChannel; // This line is redundant
      switchMode(&singleMode);
      SMP.singleMode = true;
    } else {
      switchMode(&draw);
      SMP.singleMode = false;
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
    if (isNowPlaying) drawTimer();
  }

  if (fastRecordActive) {
    flushAudioQueueToRAM();     // grab incoming audio into RAM
    checkMode(buttons, false);  // MODIFIED call
    drawTimer();
    checkPendingSampleNotes();
    //return;  // skip the rest while we’re fast-recording // This return might be intended
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
    drawText(buf, 3, 5, CRGB(200, 50, 0));

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

  if (note[SMP.x][SMP.y].channel == 0 && (currentMode == &draw || currentMode == &singleMode) && pressed[3] == true) {
    paintMode = false;
    freshPaint = true;
    unpaintMode = false;
    pressed[3] = false;
    paint();
    // return; // This return might skip drawing updates if not careful
  }

  if ((currentMode == &draw || currentMode == &singleMode) && pressed[0] == true) {
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
    if (isNowPlaying) {
      //filtercheck
      drawTimer();
    }
  }



  for (int ch = 13; ch <= 14; ch++) {
    int noteLen = getNoteDuration(ch);
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
  int timetilloff = mapf(SMP.param_settings[channel][DELAY], 0, maxfilterResolution, 0, maxParamVal[DELAY]) + mapf(SMP.param_settings[channel][ATTACK], 0, maxfilterResolution, 0, maxParamVal[ATTACK]) + mapf(SMP.param_settings[channel][HOLD], 0, maxfilterResolution, 0, maxParamVal[HOLD]) + mapf(SMP.param_settings[channel][DECAY], 0, maxfilterResolution, 0, maxParamVal[DECAY]) + mapf(SMP.param_settings[channel][RELEASE], 0, maxfilterResolution, 0, maxParamVal[RELEASE]);
  return timetilloff;
}


void shiftNotes() {
  unsigned int patternLength = lastPage * maxX;
  if (currentMode->pos[3] != SMP.shiftX) {
    // Determine shift direction (+1 or -1)
    int shiftDirectionX = 0;
    if (currentMode->pos[3] > SMP.shiftX) {
      shiftDirectionX = 1;
    } else {
      shiftDirectionX = -1;
    }
    SMP.shiftX = 8;  // Reset conceptual center for encoder delta

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
        if (note[nx][ny].channel == SMP.currentChannel) {
          int newposX = nx + shiftDirectionX;

          // Handle wrapping around the edges
          if (newposX < 1) {
            newposX = patternLength;
          } else if (newposX > patternLength) {
            newposX = 1;
          }
          tmp[newposX][ny].channel = SMP.currentChannel;
          tmp[newposX][ny].velocity = note[nx][ny].velocity;
        }
      }
    }
    shifted = true;
  }

  if (currentMode->pos[0] != SMP.shiftY) {
    // Determine shift direction (+1 or -1)
    int shiftDirectionY = 0;
    if (currentMode->pos[0] > SMP.shiftY) {
      shiftDirectionY = -1;  // Encoder up usually means lower Y value (higher pitch)
    } else {
      shiftDirectionY = +1;  // Encoder down usually means higher Y value (lower pitch)
    }
    SMP.shiftY = 8;  // Reset conceptual center

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
        if (note[nx][ny].channel == SMP.currentChannel) {
          int newposY = ny + shiftDirectionY;
          // Handle wrapping around the edges
          if (newposY < 1) {
            newposY = maxY;
          } else if (newposY > maxY) {
            newposY = 1;
          }
          tmp[nx][newposY].channel = SMP.currentChannel;
          tmp[nx][newposY].velocity = note[nx][ny].velocity;
        }
      }
    }
    shifted = true;
  }


  if (shifted) {
    // Step 3: Copy original notes of other channels back to the note array
    // And apply shifted notes for current channel
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (original[nx][ny].channel != 0 && original[nx][ny].channel != SMP.currentChannel) {  // Only copy if it's an 'other' channel note
          note[nx][ny] = original[nx][ny];
        } else {  // Clear spot for current channel notes or if it was empty in original
          note[nx][ny].channel = 0;
          note[nx][ny].velocity = defaultVelocity;
        }
        // Then overlay the shifted notes for the current channel
        if (tmp[nx][ny].channel == SMP.currentChannel) {
          note[nx][ny] = tmp[nx][ny];
        }
      }
    }
    shifted = false;
  }



  /* shift only this page:*/
}
void tmpMuteAll(bool pressed) {
  if (pressed) {
    // only on the *transition* into pressed do we save & mute
    if (!tmpMuteActive) {
      // save current mutes
      for (unsigned i = 1; i <= maxFiles; i++) {  // maxFiles or maxY for channels? Assuming mute array is indexed 1..maxFiles
        prevMuteState[i] = SMP.mute[i];
      }
      // mute all except the current channel
      for (unsigned i = 1; i <= maxFiles; i++) {
        SMP.mute[i] = (i == SMP.currentChannel) ? false : true;
      }
      tmpMuteActive = true;
    }
  } else {
    // only on the *transition* into released do we restore
    if (tmpMuteActive) {
      for (unsigned i = 1; i <= maxFiles; i++) {
        SMP.mute[i] = prevMuteState[i];
      }
      tmpMuteActive = false;
    }
  }
}


void toggleMute() {

  if (SMP.mute[SMP.currentChannel]) {
    SMP.mute[SMP.currentChannel] = false;
    //envelopes[SMP.currentChannel]->release(11880 / 2);
  } else {
    // wenn leer oder nicht gemuted:
    SMP.mute[SMP.currentChannel] = true;
    // envelopes[SMP.currentChannel]->release(120);
  }
}

void deleteActiveCopy() {

  SMP.activeCopy = false;
}



void play(bool fromStart) {
  if (CrashReport) {  // This implicitly calls operator bool() or similar if defined by CrashReportClass
    checkCrashReport();
  }

  if (fromStart) {
    updateLastPage();
    deleteActiveCopy();
    beat = 1;
    SMP.page = 1;
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
  updateLastPage();
  deleteActiveCopy();
  autoSave();
  envelope0.noteOff();
  //allOff();
  Encoder[2].writeRGBCode(0x005500);
  beat = 1;      // Reset beat on pause
  SMP.page = 1;  // Reset page on pause
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
// Place this logic in your encoder handler where SMP.edit (page) changes:
void handlePageSwitch(int newEdit) {
  unsigned int oldEdit = SMP.edit;
  if (newEdit != oldEdit) {
    if (SMP_PATTERN_MODE) {
      unsigned int oldStart = (oldEdit - 1) * maxX + 1;
      unsigned int newStart = (newEdit - 1) * maxX + 1;
      unsigned int offset   = beat - oldStart;
      // Preserve relative position within page:
      beat = newStart + offset;
      SMP.page = newEdit;
    }
    SMP.edit = newEdit;
  }
}

void playNote() {

  if (currentMode == &draw || currentMode == &singleMode || currentMode == &velocity) {


    /*if (patternChangeActive){
    if (millis() <= patternChangeTime) {          
          //Serial.println(editpage);
    }else{patternChangeActive=false;}
    }
    drawPatternChange(SMP.edit);
    */
  }

  onBeatTick();

  if (isNowPlaying) {
    drawPlayButton();

    for (unsigned int b = 1; b < maxY + 1; b++) {  // b is 1-indexed (row on grid)
      if (beat > 0 && beat <= maxlen) {            // Ensure beat is within valid range for note array
        int ch = note[beat][b].channel;            // ch is 0-indexed for internal use (e.g. SMP arrays)
        int vel = note[beat][b].velocity;
        if (ch > 0 && !SMP.mute[ch]) {  // ch > 0 means a note is set (0 is no note). SMP.mute is likely 0 or 1 indexed based on channel.
                                        // Assuming SMP.mute uses same indexing as ch (0-15 or 1-16).
                                        // If SMP.mute is 1-indexed for channels 1-16, then use SMP.mute[ch] if ch itself is 1-16,
                                        // or SMP.mute[ch+1] if ch is 0-15.
                                        // From Device struct, mute seems to be `unsigned int mute[maxY]`, so 0 to maxY-1.
                                        // If ch from note[beat][b].channel is 1-15 for actual channels, then SMP.mute[ch-1] or similar.
                                        // Given SMP.currentChannel = SMP.y - 1 (0-indexed), and note.channel is set to this,
                                        // `ch` here is 0-indexed. So `SMP.mute[ch]` should be correct if `SMP.mute` is 0-indexed.

          //MidiSendNoteOn(b, ch, vel); // This needs ch to be 1-16 for MIDI, and b as pitch.
          if (ch < 9) {                                                    // Sample channels (0-8 are _samplers[0] to _samplers[8])
            if (SMP.param_settings[ch][TYPE] == 0) {                       // Drum type for sample channels 0,1,2
              float baseTone = pianoFrequencies[constrain(b - 1, 0, 15)];  // b is 1-16, map to 0-15 for pianoFreq
              float decay = mapf(SMP.drum_settings[ch][DRUMDECAY], 0, 64, 0, 1023);
              float pitchMod = mapf(SMP.drum_settings[ch][DRUMPITCH], 0, 64, 0, 1023);
              float type = SMP.drum_settings[ch][DRUMTYPE];
              float notePitchOffset = mapf(SMP.drum_settings[ch][DRUMTONE], 0, 64, 0, 1023);  // Additional pitch

              if (ch == 0) KD_drum(baseTone + notePitchOffset, decay, pitchMod, type);  // KD on channel 0 (_sampler 1)
              if (ch == 1) SN_drum(baseTone + notePitchOffset, decay, pitchMod, type);  // SN on channel 1 (_sampler 2)
              if (ch == 2) HH_drum(baseTone + notePitchOffset, decay, pitchMod, type);  // HH on channel 2 (_sampler 3)
                                                                                        // Channels 3-8 using this mode are undefined behavior
            } else {                                                                    // Sample playback for channels 0-8
              // `b` is grid row (1-16), map to pitch. `SampleRate` seems to be a pitch offset.
              // The original `b - (ch + 1)` seems like an attempt to map grid row to a MIDI-like note relative to channel.
              // Need to ensure pitch is valid for sampler.
              //int pitch = (12 * SampleRate[ch]) + b - 1;  // b-1 to make it 0-15. SampleRate acts as octave. // Original: 12 * SampleRate[ch] + b - (ch + 1) //

              int pitch = (12 * SampleRate[ch]) +  b - (ch + 1);
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
      beat = 1;
      if (fastRecordActive) stopFastRecord();
      SMP.page = 1;
      isNowPlaying = true;  // Should already be true if MIDI clock started it
      //Serial.println("4 Bars Reached");
      waitForFourBars = false;  // Reset for the next start message
    }

    yield();
    beatStartTime = millis();
     if (SMP_PATTERN_MODE) {
    // Compute the bounds of the current page:
    unsigned int pageStart = (SMP.edit - 1) * maxX + 1;
    unsigned int pageEnd   = pageStart + maxX - 1;

    // Preserve relative beat when switching pages:
    static unsigned int lastEdit = SMP.edit;
    if (SMP.edit != lastEdit) {
      unsigned int oldStart = (lastEdit - 1) * maxX + 1;
      unsigned int offset   = beat - oldStart;
      beat = pageStart + offset;
      lastEdit = SMP.edit;
    }

    // Advance and wrap within this page:
    beat++;
    if (beat < pageStart || beat > pageEnd) {
      beat = pageStart;
    }

    // Keep the displayed page locked:
    SMP.page = SMP.edit;
  } else {
    // Fallback to default behavior:
    beat++;
    checkPages();
  }


  }
    for (int ch = 13; ch <= 14; ch++) {  // Only for synth channels 13, 14
      if (noteOnTriggered[ch] && !persistentNoteOn[ch]) {
        int noteLen = getNoteDuration(ch);                           // Calculate duration
        if ((millis() - startTime[ch] >= (unsigned long)noteLen)) {  // Cast noteLen to ulong for comparison
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
    SMP.page = SMP.edit;
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
  SMP.page = newPage;
}


void unpaint() {
  //SMP.edit = 1; // This line seems to do nothing or is misintended. editpage is used for current page.
  paintMode = false;
  // SMP.x is already global X (1 to maxlen-1), SMP.y is global Y (1 to maxY)
  // No need to recalculate from SMP.edit and SMP.x for the current view
  // unsigned int x_coord = (SMP.edit - 1) * maxX + SMP.x; // This is if SMP.x was page-local
  unsigned int current_x = SMP.x;  // Use the global cursor X
  unsigned int current_y = SMP.y;  // Use the global cursor Y


  if ((current_y > 0 && current_y <= maxY)) {  // y is 1-based, 1 to 16.
                                               // Row 1 (SMP.y==1) is often special (e.g. transport)
                                               // The condition was (y > 1 && y < 16) previously, meaning rows 2-15.
                                               // Assuming general unpaint for rows 1-15, and 16 is special.
    if (current_y < 16) {                      // Rows 1-15 for notes
      if (!SMP.singleMode) {
        note[current_x][current_y].channel = 0;
        note[current_x][current_y].velocity = defaultVelocity;
      } else {
        if (note[current_x][current_y].channel == SMP.currentChannel) {
          note[current_x][current_y].channel = 0;
          note[current_x][current_y].velocity = defaultVelocity;
        }
      }
    } else if (current_y == 16) {  // Row 16 (top row)
      clearPageX(current_x);       // Clear entire column x if cursor is on top row
    }
  }
  updateLastPage();
  FastLEDshow();  // Update display immediately
}


void paint() {
  //SMP.edit = 1; // Same as in unpaint, probably not intended here.
  //Serial.println("!!!PAINT!");

  unsigned int current_x = SMP.x;  // Use global cursor X
  unsigned int current_y = SMP.y;  // Use global cursor Y

  if (!SMP.singleMode) {  // Draw mode
    // current_y is 1-16. SMP.currentChannel is 0-14.
    // If painting on grid rows 1-15 (SMP.y from encoder), channel is SMP.y - 1.
    // Original condition: (y > 1 && y <= 9) || (y == 12) || (y > 13 && y <= 15)
    // This implies specific rows map to specific channel types/groups.
    // For simplicity, let's assume if current_y maps to a valid channel (1-15), we paint it.
    // SMP.currentChannel is already set based on SMP.y in checkEncoders for draw mode.
    if (current_y > 0 && current_y <= 15) {                       // Grid rows 1-15
      if (note[current_x][current_y].channel == 0) {              // Only paint if empty
        note[current_x][current_y].channel = SMP.currentChannel;  // SMP.currentChannel should be correct 0-indexed channel
        note[current_x][current_y].velocity = defaultVelocity;
      }
    } else if (current_y == 16) {  // Top row (SMP.y == 16)
      toggleCopyPaste();
    }
  } else {                                     // Single mode (painting for the globally selected SMP.currentChannel)
    if ((current_y > 0 && current_y <= 15)) {  // Grid rows 1-15
      note[current_x][current_y].channel = SMP.currentChannel;
      note[current_x][current_y].velocity = defaultVelocity;
    }
    // No action for current_y == 16 in single mode paint.
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
      // Map pitch_from_row (1-16) to a MIDI note or sampler-specific pitch.
      // Original: 12 * SampleRate[ch] + y - (ch + 1)
      //int pitch = (12 * SampleRate[painted_channel]) + (pitch_from_row - 1);  // Example mapping
      
      int pitch = 12 * SampleRate[painted_channel] + pitch_from_row - (painted_channel + 1);

      _samplers[painted_channel].noteEvent(pitch, painted_velocity, true, true);
    } else if (painted_channel == 11) {  // Specific synth
      playSound((12 * (int)octave[0]) + transpose + (pitch_from_row - 1), 0);
    } else if (painted_channel >= 13 && painted_channel < 15) {            // General synths
      playSynth(painted_channel, pitch_from_row, painted_velocity, false); 
                                                                           
    }
  }

  updateLastPage();
  FastLEDshow();
}





void toggleCopyPaste() {

  //SMP.edit = 1;
  if (!SMP.activeCopy) {

    // copy the pattern into the memory
    Serial.print("copy now");
    unsigned int src = 0;
    for (unsigned int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {  // maxy?
      src++;
      for (unsigned int y = 1; y < maxY + 1; y++) {

        tmp[src][y] = note[c][y];
      }
    }
  } else {

    // paste the memory into the song
    Serial.print("paste here!");
    unsigned int src = 0;
    for (unsigned int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {
      src++;
      for (unsigned int y = 1; y < maxY + 1; y++) {

        note[c][y] = tmp[src][y];
      }
    }
  }
  updateLastPage();
  SMP.activeCopy = !SMP.activeCopy;  // Toggle the boolean value
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
  SMP.vol = currentMode->pos[2];
  float vol = float(SMP.vol / 10.0);
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
      resetMidiClockState();
    }
  }
  drawBPMScreen();  // Assumed to exist for visual feedback
}

void setVolume() {
  showExit(0);
  drawBPMScreen();  // Assumed to exist

  if (currentMode->pos[1] != (unsigned int)(ledBrightness + 46) / 10) {  // Reverse map for comparison
    updateBrightness();
  }

  if (currentMode->pos[2] != SMP.vol) {
    updateVolume();
  }


  if (currentMode->pos[3] != (unsigned int)SMP.bpm) {  // Cast SMP.bpm for comparison
    updateBPM();
  }
}





void showExit(int index) {
  showIcons(HELPER_EXIT, CRGB(0, 0, 100));
  Encoder[index].writeRGBCode(0x0000FF);
}

void showLoadSave() {

  drawNoSD();
  FastLEDclear();

  showIcons(ICON_LOADSAVE, CRGB(10, 5, 0));
  showIcons(ICON_LOADSAVE2, CRGB(200, 200, 200));
  showIcons(HELPER_SELECT, CRGB(0, 0, 5));
  showIcons(ICON_NEW, CRGB(20, 20, 20));

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%u.txt", SMP.file);  // Use %u for unsigned int
  if (SD.exists(OUTPUTf)) {
    showIcons(HELPER_SAVE, CRGB(1, 0, 0));
    showIcons(HELPER_LOAD, CRGB(0, 20, 0));
    drawNumber(SMP.file, CRGB(0, 0, 20), 11);
  } else {
    showIcons(HELPER_SAVE, CRGB(20, 0, 0));
    showIcons(HELPER_LOAD, CRGB(0, 1, 0));
    drawNumber(SMP.file, CRGB(20, 20, 40), 11);
  }
  FastLED.setBrightness(ledBrightness);  // Already done in updateBrightness, but ok if called again
  FastLED.show();

  if (currentMode->pos[3] != SMP.file) {
    ////Serial.print("File: " + String(currentMode->pos[3]));
    ////Serial.println();
    SMP.file = currentMode->pos[3];
  }
}

void showSamplePack() {
  drawNoSD();
  FastLEDclear();

  showIcons(ICON_SAMPLEPACK, CRGB(10, 10, 0));
  showIcons(HELPER_SELECT, CRGB(0, 0, 10));
  // drawNumber(SMP.pack, CRGB(20, 0, 0), 11); // This one seems redundant given the logic below

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%u/%u.wav", SMP.pack, 1);  // Use %u
  if (SD.exists(OUTPUTf)) {
    showIcons(HELPER_LOAD, CRGB(0, 20, 0));
    showIcons(HELPER_SAVE, CRGB(3, 0, 0));
    drawNumber(SMP.pack, CRGB(0, 20, 0), 11);
  } else {
    showIcons(HELPER_LOAD, CRGB(0, 3, 0));
    showIcons(HELPER_SAVE, CRGB(20, 0, 0));
    drawNumber(SMP.pack, CRGB(20, 0, 0), 11);
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
  EEPROM.put(0, pack_id);                        // Save current pack_id to EEPROM
  for (unsigned int z = 1; z < maxFiles; z++) {  // maxFiles is 9. So loads samples 1 through 8.
                                                 // Sample arrays are often 0-indexed. _samplers[0] to _samplers[8] exist.
                                                 // This loop should probably be z=0 to maxFiles-1 or z=1 to maxFiles (inclusive for maxFiles).
                                                 // Assuming it means load into sampler slots 1 to 8.
    if (!intro) {
      showIcons(ICON_SAMPLE, CRGB(20, 20, 20));
    } else {
      drawText("LOAD", 2, 11, col[(maxFiles + 1) - z]);
    }
    drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], CRGB(50, 50, 50), intro);
    loadSample(pack_id, z);  // loadSample needs to know which sampler slot 'z' corresponds to.
  }
  // char OUTPUTf[50]; // This seems unused here
  // sprintf(OUTPUTf, "%u/%u.wav", pack_id, 1);
  switchMode(&draw);  // Switch back to draw mode after loading
}


void updateLastPage() {
  lastPage = 0;  // Start by assuming no notes
  for (unsigned int p = 1; p <= maxPages; p++) {
    bool pageHasNotesThisPage = false;  // Renamed to avoid conflict
    unsigned int baseIndex = (p - 1) * maxX;
    for (unsigned int ix = 1; ix <= maxX; ix++) {
      for (unsigned int iy = 1; iy <= maxY; iy++) {
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

  //Serial.println("Loading Wave :" + String(SMP.wav[SMP.currentChannel].fileID));
  // loadSample takes (folder_or_pack_id, file_id_or_sampler_slot).
  // Here, it seems SMP.wav[SMP.currentChannel].fileID is the actual sample ID to load.
  // And it should load into the sampler associated with SMP.currentChannel.
  // The second argument to loadSample in loadSamplePack was 'z' (sampler slot).
  // A consistent loadSample(pack_or_folder, file_to_load_id, target_sampler_slot) would be clearer.
  // Assuming loadSample(0, SMP.wav[SMP.currentChannel].fileID) means:
  // 0 might be a special folder/pack ID, or it implies using SMP.folder.
  // And the sample SMP.wav[SMP.currentChannel].fileID is loaded into sampler SMP.currentChannel.
  // This needs clarification based on loadSample's actual implementation.
  // For now, I'll keep it as is, but it's a potential point of confusion/bugs.
  loadSample(0, SMP.wav[SMP.currentChannel].fileID);  // Assuming SMP.folder is the intended first arg, and second is file ID.
                                                               // And loadSample internally knows to use SMP.currentChannel as target.
  switchMode(&singleMode);
  SMP.singleMode = true;
}