//extern "C" char *sbrk(int incr);
#define FASTLED_ALLOW_INTERRUPTS 1
#define SERIAL8_RX_BUFFER_SIZE 16  // Increase to 256 bytes
#define SERIAL8_TX_BUFFER_SIZE 16  // Increase if needed for transmission
#define TargetFPS 30

#define AUDIO_BLOCK_SAMPLES 128
#define AUDIO_SAMPLE_RATE_EXACT 44100
#define FLANGE_DELAY_LENGTH 2048

static const int FAST_DROP_BLOCKS = 40;   // ≈200ms @ 44100Hz with 128-sample blocks
static int fastDropRemaining = 0;

// Assume 'flangers' is an array of AudioEffectFlange pointers
// AudioEffectFlange* flangers[MAX_EFFECTS]; // Example declaration
// Assume 'delayline' is a shared or dedicated delay line buffer
// int16_t delayline[FLANGE_DELAY_LENGTH]; // Example declaration
// Assume 'index' is the valid index for the current flanger instance
// Assume 'value' is the input control value (0.0 to 1.0)
// Define the number of flanger instances you might use if this code handles multiple
#define MAX_FLANGERS 5  // Adjust if you have more/less

// Per-instance state variables to avoid glitches and redundant calls
static float flanger_lastValue[MAX_FLANGERS] = { -1.0f };  // Initialize all to -1.0f
static bool flanger_bypassSet[MAX_FLANGERS] = { false };   // Initialize all to false

#include <WS2812Serial.h>  // leds
#define USE_WS2812SERIAL   // leds
#include <FastLED.h>       // leds

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
#define SWITCH_3 3  // Pin for TPP223 3 //3==lowerright, lowerleft== 15!
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

#define maxfilterResolution 64
#define numPulsesForAverage 8  // Number of pulses to average over
#define pulsesPerBar (24 * 4)  // 24 pulses per quarter note, 4 quarter notes per bar

#define EEPROM_MENU_ADDR  42  


struct MySettings : public midi ::DefaultSettings {
  static const long BaudRate = 115200;
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

#define EEPROM_MAGIC_ADDR   42
#define EEPROM_DATA_START   43   // 43..48 will be your six mode‐bytes
const uint8_t EEPROM_MAGIC = 0x5A;  // anything nonzero

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
#define FLANGE_DELAY_LENGTH (12 * AUDIO_BLOCK_SAMPLES)
short delayline[FLANGE_DELAY_LENGTH];

bool MIDI_CLOCK_SEND = true;
bool MIDI_TRANSPORT_RECEIVE = true;
bool MIDI_VOICE_SELECT = false;
bool SMP_PATTERN_MODE = false;
unsigned int SMP_FAST_REC = false;

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
static bool bypassSet = false;
bool activeNotes[128] = { false };  // Track active MIDI notes (0-127)

float rateFactor = 44117.0 / 44100.0;

const char *SynthVoices[11]  = { nullptr, "BASS", "KEYS", "CHPT", "PAD", "WOW", "ORG", "FLT", "LEAD", "ARP", "BRSS" };
const char *channelType[5]  = { nullptr, "DRUM", "SMP", "SYNTH", "X" };
const char *menuText[10]  = { "DAT", "KIT", "WAV", "REC", "BPM", "CLCK", "CHAN", "TRAN", "PMOD", "OTR" };

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

//unsigned long filterchecktime = 0;
unsigned long filterDrawEndTime = 0;
bool filterDrawActive = false;


unsigned long patternChangeTime = 0;
bool patternChangeActive = false;


unsigned int menuPosition = 1;
String oldPosString, posString = "1:2:";
String buttonString, oldButtonString = "0000";
unsigned long playStartTime = 0;  // To track when play(true) was last called

bool previewIsPlaying = false;

const int maxPeaks = 512;  // Adjust based on your needs
float peakValues[maxPeaks];
int peakIndex = 0;


const int maxRecPeaks = 512;  // Adjust based on your needs
float peakRecValues[maxRecPeaks];
int peakRecIndex = 0;



uint8_t ledBrightness = 63;
const unsigned int maxlen = (maxX * maxPages) + 1;
const long ram = 9525600;  // 9* 1058400; //12seconds on 44.1 / 16Bit before: 12582912;  //12MB ram for sounds // 16MB total
const unsigned int SONG_LEN = maxX * maxPages;

bool touchState[4] = { false };      // Current touch state (HIGH/LOW)
bool lastTouchState[4] = { false };  // Previous touch state
const int touchThreshold = 60;

const unsigned int totalPulsesToWait = pulsesPerBar * 2;

const unsigned long CHECK_INTERVAL = 50;  // Interval to check buttons in ms
unsigned long lastCheckTime = 0;          // Get the current time


int recMode = 1;
int fastRecMode = 0;
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
IntervalTimer midiTimer;
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


String usedFiles[maxFiles] = { "samples/_1.wav",
                               "samples/_2.wav",
                               "samples/_3.wav",
                               "samples/_4.wav",
                               "samples/_5.wav",
                               "samples/_6.wav",
                               "samples/_7.wav",
                               "samples/_8.wav",
                               "samples/_9.wav" };

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
Mode filterMode = { "FILTERMODE", { 0, 0, 1, 0 }, { 7, 6, 8, maxfilterResolution }, { 1, 1, 1, 1 }, { 0x000000, 0x000000, 0x000000, 0x000000 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 7, 0, 7 }, { 9, 9, maxfilterResolution, 9 }, { 8, 8, maxfilterResolution, 8 }, { 0xFFFF00, 0x000000, 0x000000, 0xFFFFFF } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { maxY, 1, maxY, maxY }, { maxY, 1, 10, 10 }, { 0xFF4400, 0x000000, 0x0044FF, 0x888888 } };

Mode set_Wav = { "SET_WAV", { 1, 1, 1, 1 }, { 9999, FOLDER_MAX, 9999, 999 }, { 0, 0, 0, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 1 }, { 1, 1, 99, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode loadSaveTrack = { "LOADSAVE_TRACK", { 1, 1, 1, 1 }, { 1, 1, 1, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode menu = { "MENU", { 1, 1, 1, 1 }, { 1, 1, 1, 10 }, { 1, 1, 1, 1 }, { 0x000000, 0x000000, 0x000000, 0x00FF00 } };
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

static bool prevMuteState[maxFiles + 1];
static bool tmpMuteActive = false;

//EXTMEM int16_t fastRecBuffer[MAX_CHANNELS][BUFFER_SAMPLES];
static size_t fastRecWriteIndex[MAX_CHANNELS];
bool fastRecordActive = false;
//static uint8_t fastRecordChannel = 0;



enum ParameterType { DELAY,
                     ATTACK,
                     HOLD,
                     DECAY,
                     SUSTAIN,
                     RELEASE,
                     WAVEFORM,
                     TYPE  //AudioSynthNoiseWhite, AudioSynthSimpleDrums OR WAVEFORM
};                         //Define filter types

int maxParamVal[12] = { 1000, 2000, 1000, 1000, 1, 1000, 1000, 1, 1, 0, 0 };


enum FilterType { NUL,
                  LOWPASS,
                  HIGHPASS,
                  FREQUENCY,
                  REVERB,
                  BITCRUSHER,
                  FLANGER,
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
char *activeFilterType[9] = { "", "LOW", "HIGH", "FREQ", "RVRB", "BITC", "FLNG", "DTNE", "OCTV" };

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

// Variables for double-click detection
unsigned long lastClickTime[NUM_ENCODERS] = { 0 };  // Timestamp of the last click
int clickCount[NUM_ENCODERS] = { 0 };               // Number of clicks


i2cEncoderLibV2 Encoder[NUM_ENCODERS] = {
  i2cEncoderLibV2(0x01),  // third encoder address
  i2cEncoderLibV2(0x41),  // 2nd encoder address +
  i2cEncoderLibV2(0x20),  // First encoder address
  i2cEncoderLibV2(0x61),  // First encoder address
};
// Global variable to track current encoder index for callbacks
int currentEncoderIndex = 0;



EXTMEM arraysampler _samplers[9];
AudioPlayArrayResmp *voices[9] = { &sound0, &sound1, &sound2, &sound3, &sound4, &sound5, &sound6, &sound7, &sound8 };

AudioEffectEnvelope *envelopes[15] = { &envelope0, &envelope1, &envelope2, &envelope3, &envelope4, &envelope5, &envelope6, &envelope7, &envelope8, nullptr, nullptr, &envelope11, &envelope12, &envelope13, &envelope14 };
AudioAmplifier *amps[15] = { &amp0, &amp1, &amp2, &amp3, &amp4, &amp5, &amp6, &amp7, &amp8, nullptr, nullptr, &amp11, &amp12, &amp13, &amp14 };
AudioFilterStateVariable *filters[15] = { nullptr, &filter1, &filter2, &filter3, &filter4, &filter5, &filter6, &filter7, &filter8, nullptr, nullptr, &filter11, &filter12, &filter13, &filter14 };
AudioMixer4 *filtermixers[15] = { nullptr, &filtermixer1, &filtermixer2, &filtermixer3, &filtermixer4, &filtermixer5, &filtermixer6, &filtermixer7, &filtermixer8, nullptr, nullptr, &filtermixer11, &filtermixer12, &filtermixer13, &filtermixer14 };
AudioEffectBitcrusher *bitcrushers[15] = { nullptr, &bitcrusher1, &bitcrusher2, &bitcrusher3, &bitcrusher4, &bitcrusher5, &bitcrusher6, &bitcrusher7, &bitcrusher8, nullptr, nullptr, &bitcrusher11, &bitcrusher12, &bitcrusher13, &bitcrusher14 };
AudioEffectFreeverb *freeverbs[15] = { nullptr, &freeverb1, &freeverb2, nullptr, nullptr, nullptr, nullptr, &freeverb7, &freeverb8, 0, 0, &freeverb11, &freeverb12, &freeverb13, &freeverb14 };
AudioMixer4 *freeverbmixers[15] = { nullptr, &freeverbmixer1, &freeverbmixer2, nullptr, nullptr, nullptr, nullptr, &freeverbmixer7, &freeverbmixer8, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
AudioEffectFlange *flangers[15] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &flange11, &flange12, &flange13, &flange14 };
AudioMixer4 *waveformmixers[15] = { nullptr, &BDMixer, &SNMixer, &HHMixer, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &mixer_waveform11, &mixer_waveform12, &mixer_waveform13, &mixer_waveform14 };


AudioSynthWaveform *synths[15][2];

/*
  AudioSynthWaveformDc *Sdc1[3] = {&dc11_1, &dc11_2, &dc11_3};
  AudioEffectEnvelope *Senvelope1[3] = {&envelope1_1, &envelope1_2, &envelope1_3};
  AudioEffectEnvelope *Senvelope2[3] = {&envelope2_1, &envelope2_2, &envelope2_3};
  AudioEffectEnvelope *SenvelopeFilter1[3] = {&envelopeFilter1_1, &envelopeFilter1_2, &envelopeFilter1_3};

  AudioSynthWaveform *Swaveform1[3] = {&waveform1_1, &waveform2_1, &waveform3_1};
  AudioSynthWaveform *Swaveform2[3] = {&waveform2_2, &waveform2_2, &waveform3_2};
  AudioSynthWaveform *Swaveform3[3] = {&waveform3_1, &waveform3_2, &waveform3_3};

  AudioFilterLadder *Sladder1[3] = {&ladder1_1, &ladder2_1};
  AudioFilterLadder *Sladder2[3] = {&ladder1_2, &ladder2_2};
  AudioFilterLadder *Sladder3[3] = {&ladder1_3, &ladder2_3};

  AudioMixer4 *Smixer1[2] = { &mixer1_1, &mixer2_1};
  AudioMixer4 *Smixer2[2] = { &mixer1_2, &mixer2_2};
  AudioMixer4 *Smixer3[2] = { &mixer1_3, &mixer2_3};
  */

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
    Serial.println(channelvolume);
    amps[SMP.currentChannel]->gain(channelvolume);
  }

  drawVelocity();
}



void staticButtonPushed(i2cEncoderLibV2 *obj) {
  encoder_button_pushed(obj, currentEncoderIndex);
  pressed[currentEncoderIndex] = true;
}


void staticDoublePush(i2cEncoderLibV2 *obj) {
  encoder_double_push(obj, currentEncoderIndex);
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


void encoder_double_push(i2cEncoderLibV2 *obj, int encoderIndex) {
  buttons[encoderIndex] = 3;  // Double click
  //obj->writeRGBCode(0xFFFFFF);
}


void encoder_button_released(i2cEncoderLibV2 *obj, int encoderIndex) {
  buttonState[encoderIndex] = RELEASED;
}






void handle_button_state(i2cEncoderLibV2 *obj, int encoderIndex) {
  // Only process the button if it's the last pressed encoder
  unsigned long currentTime = millis();
  pressDuration[encoderIndex] = currentTime - buttonPressStartTime[encoderIndex];

  switch (buttonState[encoderIndex]) {
    case RELEASED:
      if (pressDuration[encoderIndex] <= longPressDuration[encoderIndex]) {
        buttons[encoderIndex] = 1;  // Released
        buttonState[encoderIndex] = IDLE;
        //obj->writeRGBCode(0x0000FF);
        isPressed[encoderIndex] = false;
        return;
      } else {
        buttons[encoderIndex] = 9;  // Released
        buttonState[encoderIndex] = IDLE;
        //obj->writeRGBCode(0x000000);
        isPressed[encoderIndex] = false;
      }
      break;

    case LONG_PRESSED:
      if (pressDuration[encoderIndex] >= longPressDuration[encoderIndex]) {
        //obj->writeRGBCode(0xFF00FF);  // Purple for long press
        buttons[encoderIndex] = 2;  // Long press
        buttonState[encoderIndex] = IDLE;
        isPressed[encoderIndex] = true;
      }
      break;

    default:
      if (isPressed[encoderIndex] && pressDuration[encoderIndex] >= longPressDuration[encoderIndex]) {
        //obj->writeRGBCode(0xFF00FF);  // Purple for long press
        buttons[encoderIndex] = 2;  // Long press
        buttonState[encoderIndex] = LONG_PRESSED;
        isPressed[encoderIndex] = true;
      }
      break;
  }
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
  Serial.println(newMode->name);
  oldButtonString = buttonString;
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


void checkFastRec(){
      if ((currentMode == &draw || currentMode == &singleMode) && SMP_FAST_REC == 2 || SMP_FAST_REC == 3) {
      bool pinsConnected = (digitalRead(2) == LOW);

      if (pinsConnected != lastPinsConnected && millis() - lastChangeTime > debounceDelay) {
        lastChangeTime = millis();  // Update timestamp
        lastPinsConnected = pinsConnected;

        if (SMP_FAST_REC == 2) {
          if (pinsConnected && !fastRecordActive) {
            Serial.println(">> startFastRecord from pin 2+4");
            startFastRecord();
            paintMode = false;
            freshPaint = true;
            unpaintMode = false;
            pressed[3] = false;
            note[beat][SMP.currentChannel+1].channel = SMP.currentChannel;
            note[beat][SMP.currentChannel+1].velocity = defaultVelocity;
            return;
          } else if (!pinsConnected && fastRecordActive) {
            Serial.println(">> stopFastRecord from pin 2+4");
            stopFastRecord();
          }
        }

        if (SMP_FAST_REC == 3) {
          if (!pinsConnected && !fastRecordActive) {
            Serial.println(">> startFastRecord from pin 2+4");
            startFastRecord();
            paintMode = false;
            freshPaint = true;
            unpaintMode = false;
            pressed[3] = false;
            note[beat][SMP.currentChannel+1].channel = SMP.currentChannel;
            note[beat][SMP.currentChannel+1].velocity = defaultVelocity;
            return;
          } else if (pinsConnected && fastRecordActive) {
            Serial.println(">> stopFastRecord from pin 2+4");
            stopFastRecord();
          }
        }
      }
    }


  int touchValue3 = fastTouchRead(SWITCH_3);
  touchState[2] = (touchValue3 > touchThreshold);

  if (SMP_FAST_REC==1 && !fastRecordActive && touchState[2]) {
    startFastRecord();
    return;  // skip other mode logic while fast recording
  }

  if (SMP_FAST_REC==1 && fastRecordActive && !touchState[2]) {
    stopFastRecord();
  }

}

void checkMode(String buttonString, bool reset) {

  for (int i = 0; i < NUM_ENCODERS; i++) {
    if (buttons[i] == 9 || buttons[i] == 1) {
      buttons[i] = 0;
      buttonState[i] = IDLE;
      isPressed[i] = false;
    }
  }


  checkFastRec();


  if (isRecording && buttonString == "0900") {
    //stopRecord(getFolderNumber(SMP.wav[SMP.currentChannel].fileID), SMP.wav[SMP.currentChannel].fileID);
    stopRecordingRAM(getFolderNumber(SMP.wav[SMP.currentChannel].fileID), SMP.wav[SMP.currentChannel].fileID);
    return;
  }
  if (isRecording) return;


  // Toggle play/pause in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && buttonString == "0010") {

    if (!isNowPlaying) {
      Serial.println("PLAY");
      play(true);
    } else {
      unsigned long currentTime = millis();
      if (currentTime - playStartTime > 200) {  // Check if play started more than 200ms ago
        pause();
      }
    }
  }

  if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && buttonString == "0200") {
    tmpMute = true;
    tmpMuteAll(true);
    Encoder[1].writeRGBCode(0x111111);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0900") {
    if (tmpMute) tmpMuteAll(false);
    drawKnobColorDefault();
  }

  // Shift notes around in single mode after dblclick of button 4
  if (currentMode == &singleMode && buttonString == "2200") {
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

  if (currentMode == &menu && buttonString == "1000") {
    switchMode(&draw);
    SMP.singleMode = false;
  }

  if (currentMode == &menu && buttonString == "0001") {
   
   switchMenu(menuPosition);

  } else if ((currentMode == &loadSaveTrack) && buttonString == "0001") {
    paintMode = false;
    unpaintMode = false;
    switchMode(&draw);
  } else if ((currentMode == &loadSaveTrack) && buttonString == "0100") {
    savePattern(false);
  } else if ((currentMode == &loadSaveTrack) && buttonString == "1000") {
    loadPattern(false);
  } else if ((currentMode == &set_SamplePack) && buttonString == "0100") {
    saveSamplePack(SMP.pack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "1000") {
    loadSamplePack(SMP.pack, false);
  } else if ((currentMode == &set_SamplePack) && buttonString == "0001") {
    switchMode(&draw);
  } else if ((currentMode == &set_Wav) && buttonString == "1000") {
    //set SMP.wav[currentChannel][0] and [1] to current file

    loadWav();
    autoSave();

  } else if ((currentMode == &set_Wav) && buttonString == "0001") {
    switchMode(&singleMode);
    SMP.singleMode = true;
  } else if ((currentMode == &set_Wav) && buttonString == "0200") {
    //startRecord(getFolderNumber(SMP.wav[SMP.currentChannel].fileID), SMP.wav[SMP.currentChannel].fileID);
    startRecordingRAM();
  }



  if (currentMode == &noteShift && buttonString == "0001") {
    switchMode(&singleMode);
    SMP.singleMode = true;
  }



  // Switch to volume mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0001") {
    freshPaint = false;
  }

  // Switch to volume mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0220") {

    switchMode(&volume_bpm);
  }

  // Switch to filter mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0020") {
    if (SMP.currentChannel == 0 || SMP.currentChannel == 9 || SMP.currentChannel == 10 || SMP.currentChannel == 12 || SMP.currentChannel == 15) return;
    fxType = 0;
    switchMode(&filterMode);
  }


  // Toggle copy/paste in draw mode
  if (currentMode == &draw && buttonString == "1100") {
    //toggleCopyPaste();
  }


  // single mode: RANDOMIZE
  if ((SMP.y > 15) && (currentMode == &singleMode) && buttonString == "0002") {
    Serial.println(SMP.y);
    drawRandoms();
    return;
  }



  // Handle velocity switch in single mode
  if (!freshPaint && note[SMP.x][SMP.y].channel != 0 && (currentMode == &singleMode) && buttonString == "0002") {
    unsigned int velo = round(mapf(note[SMP.x][SMP.y].velocity, 1, 127, 1, maxY));

    Serial.println(velo);
    SMP.velocity = velo;
    switchMode(&velocity);
    SMP.singleMode = true;

    Encoder[0].writeCounter((int32_t)velo);
  }

  // Handle velocity switch in draw mode
  if (!freshPaint && note[SMP.x][SMP.y].channel != 0 && (currentMode == &draw) && buttonString == "0002") {
    unsigned int velo = round(mapf(note[SMP.x][SMP.y].velocity, 1, 127, 1, maxY));
    unsigned int chvol = SMP.channelVol[SMP.currentChannel];
    Serial.println(velo);
    SMP.velocity = velo;
    SMP.singleMode = false;
    switchMode(&velocity);

    Encoder[0].writeCounter((int32_t)velo);
    Encoder[3].writeCounter((int32_t)chvol);
  }

  // Print button string if it has changed -------------------- BUTTONS-------------

  // Clear page in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "2002") {
    clearPage();
  }

  // Switch to single mode from velocity mode // SINGLE
  if (currentMode == &velocity && buttonString == "0009") {

    if (!SMP.singleMode) {
      switchMode(&draw);
    } else {
      switchMode(&singleMode);
      SMP.singleMode = true;
    }
  }

  // set DefaultFilterParam
  if (currentMode == &filterMode && buttonString == "0010") {
    if (fxType == 0) {
      defaultFilter[SMP.currentChannel] = SMP.selectedParameter;
    } else if (fxType == 1) {
      defaultFilter[SMP.currentChannel] = SMP.selectedFilter;
    } else if (fxType == 2) {
      defaultFilter[SMP.currentChannel] = SMP.selectedDrum;
    } else if (fxType == 4) {
      defaultFilter[SMP.currentChannel] = SMP.selectedSynth;
    }
  }

  // reset Parameters
  if (currentMode == &filterMode && buttonString == "2000") {
    setDahdsrDefaults(false);
    currentFilter = activeParameterType[SMP.selectedParameter];
    currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][SMP.selectedParameter];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
  }


  // reset Filters
  if (currentMode == &filterMode && buttonString == "0002") {
    setFilterDefaults(SMP.currentChannel);
    currentFilter = activeFilterType[SMP.selectedFilter];
    currentMode->pos[3] = SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
  }

  if (currentMode == &filterMode && buttonString == "0010") {
    //cycleFilterTypes(2);
  }

  if (currentMode == &filterMode && buttonString == "0100") {
    //cycleFilterTypes(1);
  }


  // Switch to draw mode from volume mode
  if (currentMode == &filterMode && buttonString == "1000") {
    if (!SMP.singleMode) {
      switchMode(&draw);
    } else {
      switchMode(&singleMode);
      SMP.singleMode = true;
    }
    // write to fastFilterEncoder
    if (defaultFilter[SMP.currentChannel]) Encoder[2].writeCounter((int32_t)SMP.filter_settings[SMP.currentChannel][defaultFilter[SMP.currentChannel]]);
  }

  // Switch to draw mode from volume mode
  if (currentMode == &volume_bpm && buttonString == "1000") {
    switchMode(&draw);
    // setvol = false;
  }

  // Disable paint and unpaint mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0909") {
    paintMode = false;
    unpaintMode = false;
  }

  // Disable paint and unpaint mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0009") {
    freshPaint = false;
    paintMode = false;
    unpaintMode = false;
  }

  // Disable paint and unpaint mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "9000") {
    unpaintMode = false;
    paintMode = false;
  }


  // Menu Load/Save
  if ((currentMode == &draw) && buttonString == "0022") {
    //switchMode(&loadSaveTrack);
  }




  // Search Wave + Load + Exit
  if ((currentMode == &singleMode) && buttonString == "0022") {
    //set loaded sample
  }

  // Set SamplePack + Load + Save + Exit
  if ((currentMode == &draw) && buttonString == "2200") {
    //switchMode(&set_SamplePack);
  }

  // Toggle mute in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0100") {
    toggleMute();
  }

  // Enable paint mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0002") {
    paintMode = true;
    Serial.println("PAINTMODE ON");
  }

  // Unpaint and delete active copy in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "2000") {
    unpaint();
    unpaintMode = true;
    deleteActiveCopy();
  }


  // Toggle SingleMode
  if (currentMode == &singleMode && buttonString == "3000") {
    switchMode(&draw);
  } else if (currentMode == &draw && buttonString == "3000" && ((SMP.currentChannel >= 1 && SMP.currentChannel <= maxFiles) || SMP.currentChannel > 12)) {
    SMP.currentChannel = SMP.currentChannel;
    switchMode(&singleMode);
    SMP.singleMode = true;
  }

  if (reset) {
    // Reset states after handling
    buttonString = "0000";
    oldButtonString = buttonString;
    for (int i = 0; i < NUM_ENCODERS; i++) {
      buttons[i] = 0;
      buttonState[i] = IDLE;
      isPressed[i] = false;
    }
  }
}








void initSoundChip() {
  AudioInterrupts();
  //AudioMemory(48);
  AudioMemory(96);
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
  sgtl5000_1.micGain(28);  //0-63
  //sgtl5000_1.adcHighPassFilterEnable();
  //sgtl5000_1.adcHighPassFilterDisable();  //for mic?
  sgtl5000_1.unmuteLineout();
  sgtl5000_1.lineOutLevel(14);
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

  synthmixer12.gain(0, GAIN3);
  synthmixer12.gain(1, GAIN3);
  synthmixer12.gain(3, GAIN3);


  synthmixer13.gain(0, GAIN01);
  synthmixer13.gain(1, GAIN01);
  synthmixer13.gain(3, GAIN01);

  synthmixer14.gain(0, GAIN01);
  synthmixer14.gain(1, GAIN01);
  synthmixer14.gain(3, GAIN01);


  mixersynth_end.gain(0, GAIN4);
  mixersynth_end.gain(1, GAIN4);
  mixersynth_end.gain(2, GAIN4);
  mixersynth_end.gain(3, GAIN4);


  mixer0.gain(0, GAIN01);  //PREV
  mixer0.gain(1, GAIN01);  //PREV
  mixer0.gain(2, GAIN01);  //PREV
  mixer0.gain(3, GAIN01);  //PREV

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
  mixerPlay.gain(2, GAIN02);
  mixerPlay.gain(3, GAIN02);

  // Initialize the array with nullptrs
  synths[11][0] = &waveform11_1;
  synths[11][1] = &waveform11_2;

  synths[13][0] = &waveform13_1;
  synths[13][1] = &waveform13_2;

  synths[14][0] = &waveform14_1;
  synths[14][1] = &waveform14_2;

  short waveformType[15][2] = { 0 };  // zero-initialize unused elements
  // Define your actual waveforms at relevant indices

  waveformType[11][0] = WAVEFORM_PULSE;
  waveformType[11][1] = WAVEFORM_SAWTOOTH;

  waveformType[13][0] = WAVEFORM_PULSE;
  waveformType[13][1] = WAVEFORM_SAWTOOTH;

  waveformType[14][0] = WAVEFORM_PULSE;
  waveformType[14][1] = WAVEFORM_SAWTOOTH;

  float amplitude[15][2] = { 0 };

  amplitude[11][0] = 0.3;
  amplitude[11][1] = 0.3;
  // Set amplitude values similarly

  amplitude[13][0] = 0.3;
  amplitude[13][1] = 0.3;

  amplitude[14][0] = 0.3;
  amplitude[14][1] = 0.3;



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
    Sdc1[i].amplitude(1.0);
  }

  // set filters and envelopes for all sounds
  for (unsigned int i = 1; i < 9; i++) {
    filters[i]->octaveControl(6.0);
    filters[i]->resonance(0.7);
    filters[i]->frequency(8000);
  }

  for (unsigned int i = 0; i < 9; i++) {
    Serial.print("Enabling channel/voice:");
    Serial.println(i);
    voices[i]->enableInterpolation(true);
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
  Serial.print(CrashReport);
  delay(1000);
  Serial.println("clearing EEPROM and autosaved.txt...");
  for (unsigned int i = 0; i < EEPROM.length(); i++) EEPROM.write(i, 0);
  char OUTPUTf[50];
  sprintf(OUTPUTf, "autosaved.txt");
  if (SD.exists(OUTPUTf)) {
    SD.remove(OUTPUTf);
  }
  delay(2000);
  Serial.println("clearing completed.");
}



void initEncoders() {
  Serial.println("I2C Encoder init...");
  for (int i = 0; i < NUM_ENCODERS; i++) {
    // Set the global encoder index for callbacks
    currentEncoderIndex = i;
    Encoder[i].reset();
    uint16_t direction;
    if (i > 0) {
      direction = i2cEncoderLibV2::DIRE_RIGHT;
    } else {
      direction = i2cEncoderLibV2::DIRE_LEFT;
    }
    Encoder[i].begin(
      i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
      | direction | i2cEncoderLibV2::IPUP_ENABLE
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
    Encoder[i].onButtonDoublePush = staticDoublePush;

    Encoder[i].onMinMax = staticThresholds;
    Encoder[i].autoconfigInterrupt();
    Encoder[i].writeRGBCode(0xFFFFFF);
    Encoder[i].writeFadeRGB(0);
    delay(50);
    Encoder[i].updateStatus();
  }
}

void setup(void) {
  //NVIC_SET_PRIORITY(IRQ_LPUART8, 128);
  //NVIC_SET_PRIORITY(IRQ_USB1, 128);  // USB1 for Teensy 4.x
  Serial.begin(115200);

  if (CrashReport) { checkCrashReport(); }

  EEPROM.get(0, samplePackID);

  Serial.print("SamplePackID:");
  Serial.println(samplePackID);

  if (samplePackID == NAN || samplePackID == 0) {
    Serial.print("NO SAMPLEPACK SET! Defaulting to 1");
    samplePackID = 1;
  }

  pinMode(INT_PIN, INPUT_PULLUP);  // Interrups for encoder
  pinMode(0, INPUT_PULLDOWN);
  pinMode(3, INPUT_PULLDOWN);
  pinMode(16, INPUT_PULLDOWN);

  pinMode(2, INPUT_PULLUP);   // Pin 2 as input with pull-up
pinMode(4, OUTPUT);         // Pin 4 set as output
digitalWrite(4, LOW);       // Drive Pin 4 LOW

  FastLED.addLeds<WS2812SERIAL, DATA_PIN, BRG>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);


  initSoundChip();
  initSamples();

  drawNoSD();
  delay(50);

  playSdWav1.play("intro/016.wav");
  runAnimation();

  playSdWav1.stop();

  EEPROMgetLastFiles();
  loadMenuFromEEPROM();

  loadSamplePack(samplePackID, true);


  // set BPM:100
  SMP.bpm = 100.0;
  playTimer.begin(playNote, playNoteInterval);
  midiTimer.begin(checkMidi, playNoteInterval);
  //midiTimer.priority(10);
  //playTimer.priority(110);
  autoLoad();

  switchMode(&draw);
  setFilters();
  Serial8.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);  // optional MIDI library hook
  //MIDI.setHandleClock(myClock);       // optional if you're using callbacks
}

void setEncoderColor(int i) {
  Encoder[i].writeRGBCode(0x00FF00);
}


void checkEncoders() {
  buttonString = "";
  posString = "";
  
  for (int i = 0; i < NUM_ENCODERS; i++) {
    currentEncoderIndex = i;
    Encoder[i].updateStatus();
    currentMode->pos[i] = Encoder[i].readCounterInt();
    if (i != 2) posString += String(currentMode->pos[i]) + ":";  // check Encoder 0,1 & 3
    handle_button_state(&Encoder[i], i);
    buttonString += String(buttons[i]);
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
        Encoder[2].writeCounter((int32_t)SMP.filter_settings[SMP.currentChannel][dfx]);
        filterfreshsetted = true;
      }


      Encoder[0].writeRGBCode(CRGBToUint32(col[SMP.currentChannel]));
      Encoder[3].writeRGBCode(CRGBToUint32(col[SMP.currentChannel]));
    }



    if ((SMP.y > 1 && SMP.y <= 14)) {
      if (paintMode) {
        note[SMP.x][SMP.y].channel = SMP.currentChannel;
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
      Serial.println("p:" + String(editpage));
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
      if (millis() <= filterDrawEndTime) {
        FilterType fx = defaultFilter[SMP.currentChannel];
        drawFilterCheck(SMP.filter_settings[SMP.currentChannel][fx], fx);
      } else {
        // Stop drawing after 2 seconds
        filterDrawActive = false;
      }
    }


  }
}


void filtercheck() {
  FilterType fx = defaultFilter[SMP.currentChannel];
  if (fxType == 1 && !filterfreshsetted && currentMode->pos[2] != SMP.filter_settings[SMP.currentChannel][fx]) {
    float mappedValue = processFilterAdjustment(fx, SMP.currentChannel, 2);
    updateFilterValue(fx, SMP.currentChannel, mappedValue);


    //filterchecktime = millis();

    // Activate the 2-second drawing period
    filterDrawActive = true;
    filterDrawEndTime = millis() + 2000;  // 2 seconds window
    drawFilterCheck(SMP.filter_settings[SMP.currentChannel][fx], fx);
  }
}

void checkButtons() {
  unsigned long currentTime = millis();
  if (currentTime - lastCheckTime < CHECK_INTERVAL) return;
  lastCheckTime = currentTime;

  checkMode(buttonString, false);

  // Now clear out only the ones that were RELEASED
  for (int i = 0; i < NUM_ENCODERS; i++) {
    if (buttonState[i] == RELEASED) {
      buttons[i] = 0;
      buttonState[i] = IDLE;
      isPressed[i] = false;
    }
  }
}

void checkButtons2() {
  unsigned long currentTime = millis();
  // Check at defined intervals
  if (currentTime - lastCheckTime >= CHECK_INTERVAL) {

    lastCheckTime = currentTime;  // Reset the timer
    if (buttonString != oldButtonString) {
      // Only trigger if buttonString has meaningful input
      //Serial.println(buttonString);
      oldButtonString = buttonString;

      checkMode(buttonString, false);
    }
  }
}

void checkTouchInputs() {
  int touchValue1 = fastTouchRead(SWITCH_1);
  int touchValue2 = fastTouchRead(SWITCH_2);
  int touchValue3 = fastTouchRead(SWITCH_3);

  // Determine if the touches are above the threshold
  touchState[0] = (touchValue1 > touchThreshold);
  touchState[1] = (touchValue2 > touchThreshold);
  touchState[2] = (touchValue3 > touchThreshold);

  // Handle simultaneous touch
  if (touchState[0] && touchState[1] && !lastTouchState[2]) {
    lastTouchState[2] = true;
    // Optional: Define a special mode when both are touched
    if (currentMode == &singleMode) {
      Serial.println("---------<>---------");
      SMP.singleMode = false;
      switchMode(&set_Wav);

    } else {
      switchMode(&loadSaveTrack);
    }
    // Define specialMode if needed
  } else {
    // Handle individual touch events
    // Check for a rising edge on SWITCH_1 (LOW to HIGH transition)
    if (touchState[0] && !lastTouchState[0]) {
      lastTouchState[2] = false;
      if (currentMode == &draw) {
        if (SMP.currentChannel == 0 || SMP.currentChannel == 9 || SMP.currentChannel == 10 || SMP.currentChannel == 12 || SMP.currentChannel == 15) return;
        animateSingle();
      } else {
        switchMode(&draw);
        SMP.singleMode = false;
      }
    }

    // Check for a rising edge on SWITCH_2
    if (touchState[1] && !lastTouchState[1]) {
      lastTouchState[2] = false;
      if (currentMode == &draw || currentMode == &singleMode) {
        switchMode(&menu);
      } else {
        switchMode(&draw);
      }
    }
  }

  // Update last touch states

  lastTouchState[0] = touchState[0];
  lastTouchState[1] = touchState[1];
  lastTouchState[2] = touchState[2];
}

void animateSingle() {
  int yStart = SMP.currentChannel;  // 0 to 15
  int yCenter = yStart + 1;         // Because y grid is 1-based
  CRGB col = col_base[SMP.currentChannel];

  // Animate outward from center
  for (int offset = 0; offset <= 15; offset++) {
    int yUp = yCenter - offset;
    int yDown = yCenter + offset;

    for (int x = 1; x <= 16; x++) {
      if (yUp >= 1 && yUp <= 15) light(x, yUp, col);
      if (yDown >= 1 && yDown <= 15 && offset != 0) light(x, yDown, col);
    }
    drawTriggers();
    //FastLED.show();
    //delay(5);  // adjust for animation speed
  }
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
      SMP.currentChannel = SMP.currentChannel;
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

void checkMenuTouch() {
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
  unsigned long now = millis();
  for (int i = 0; i < pendingSampleNotes.size(); /* no ++ */) {
    if (now >= pendingSampleNotes[i].triggerTime) {
      auto &ev = pendingSampleNotes[i];
      _samplers[ev.channel].noteEvent(ev.pitch, ev.velocity, true, false);
      pendingSampleNotes.erase(pendingSampleNotes.begin() + i);  // erase and continue
    } else {
      ++i;
    }
  }
}

void loop() {
  if (fastRecordActive) {
    flushAudioQueueToRAM();  // grab incoming audio into RAM
    checkMode(buttonString, false);
    drawTimer();
    checkPendingSampleNotes();
    //return;  // skip the rest while we’re fast-recording
  }

  if (isRecording) {
    //flushAudioQueueToSD();  // SD write is now throttled and safe
    flushAudioQueueToRAM();
    checkEncoders();  // Optional: allow user interaction
    checkMode(buttonString, true);

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
          peakRecValues[peakIndex] = peakRec.read();
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
  checkPendingSampleNotes();

  drawPlayButton();
  checkTouchInputs();


  if (note[SMP.x][SMP.y].channel == 0 && (currentMode == &draw || currentMode == &singleMode) && pressed[3] == true) {
    paintMode = false;
    freshPaint = true;
    unpaintMode = false;
    pressed[3] = false;
    paint();
    return;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && pressed[0] == true) {

    paintMode = false;
    unpaintMode = false;
    pressed[0] = false;
    unpaint();
  }

  checkEncoders();
  
  checkButtons();

  //fix?
  SMP.edit = getPage(SMP.x);

  // Set stateMashine
  if (currentMode->name == "DRAW") {
    //
  } else if (currentMode->name == "FILTERMODE") {
    setFilters();
  } else if (currentMode->name == "VOLUME_BPM") {
    setVolume();
  } else if (currentMode->name == "VELOCITY") {
    setVelocity();
  } else if (currentMode->name == "SINGLE") {
    //checkEncoders();
  } else if (currentMode->name == "LOADSAVE_TRACK") {
    showLoadSave();
  } else if (currentMode->name == "MENU") {
    showMenu();
  } else if (currentMode->name == "SET_SAMPLEPACK") {
    showSamplePack();
  } else if (currentMode->name == "SET_WAV") {
    if (isRecording) return;
    showWave();
  } else if (currentMode->name == "NOTE_SHIFT") {
    shiftNotes();
    drawBase();
    drawTriggers();
    if (isNowPlaying) {
      //filtercheck();
      drawTimer();
    }
  }

  if (currentMode == &draw || currentMode == &singleMode || currentMode == &velocity) {
    drawBase();
    drawTriggers();

    
    /*if (patternChangeActive){
    if (millis() <= patternChangeTime) {          
          Serial.println(editpage);
    }else{patternChangeActive=false;}
    }
    */
    //drawPatternChange(SMP.edit);
    

    if (currentMode != &velocity)
      drawCursor();

    if (isNowPlaying) {
      drawTimer();
      if (!freshPaint && currentMode == &velocity) { drawVelocity(); }
      FastLEDshow();
    }
    if (!freshPaint && currentMode == &velocity) { drawVelocity(); }
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
  FastLEDshow();  // draw!
  filterfreshsetted = false;
  //yield();
  //delay(15); //25
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
    SMP.shiftX = 8;

    Encoder[3].writeCounter((int32_t)8);
    currentMode->pos[3] = 8;
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
      shiftDirectionY = -1;
    } else {
      shiftDirectionY = +1;
    }

    Encoder[0].writeCounter((int32_t)8);
    currentMode->pos[0] = 8;
    SMP.shiftY = 8;
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
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (original[nx][ny].channel != SMP.currentChannel) {
          note[nx][ny].channel = original[nx][ny].channel;
          note[nx][ny].velocity = original[nx][ny].velocity;
        } else {
          note[nx][ny].channel = 0;
          note[nx][ny].velocity = defaultVelocity;
        }
        if (tmp[nx][ny].channel == SMP.currentChannel) {
          note[nx][ny].channel = tmp[nx][ny].channel;
          note[nx][ny].velocity = tmp[nx][ny].velocity;
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
      for (unsigned i = 1; i <= maxFiles; i++) {
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
  if (fromStart) {
    updateLastPage();
    deleteActiveCopy();
    beat = 1;
    SMP.page = 1;
    Encoder[2].writeRGBCode(0xFFFF00);
    if (MIDI_CLOCK_SEND) {
      MIDI.sendRealTime(midi::Start);
      isNowPlaying = true;
      playStartTime = millis();
    } else {
      // slave-mode: arm for the next bar-1 instead of starting now
      pendingStartOnBar = true;
      isNowPlaying = false;
    }
  }
}



void pause() {
  if (MIDI_CLOCK_SEND) MIDI.sendRealTime(midi::Stop);
  isNowPlaying = false;
  pendingStartOnBar = false;
  updateLastPage();
  deleteActiveCopy();
  autoSave();
  envelope0.noteOff();
  //allOff();
  Encoder[2].writeRGBCode(0x005500);
  beat = 1;
  SMP.page = 1;
}


void playSynth(int ch, int b, int vel, bool persistant) {
  float frequency = fullFrequencies[b - ch + 13];  // y-Wert ist 1-basiert, Array ist 0-basiert // b-1??

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

void playNote() {
  onBeatTick();

  if (isNowPlaying) {
    for (unsigned int b = 1; b < maxY + 1; b++) {
      int ch = note[beat][b].channel;
      int vel = note[beat][b].velocity;
      if (beat < maxlen && b < maxY + 1 && ch > 0 && !SMP.mute[ch]) {
        MidiSendNoteOn(b, ch, vel);
        if (ch < 9) {
          if (SMP.param_settings[ch][TYPE] == 0) {
            float tone = pianoFrequencies[b];
            float dec = mapf(SMP.drum_settings[ch][DRUMDECAY], 0, 64, 0, 1023);
            float pit = mapf(SMP.drum_settings[ch][DRUMPITCH], 0, 64, 0, 1023);
            float typ = SMP.drum_settings[ch][DRUMTYPE];  //mapf(, 0, 64, 1, 3);

            float notepitch = mapf(SMP.drum_settings[ch][DRUMTONE], 0, 64, 0, 1023);

            if (ch == 1) KD_drum(tone + notepitch, dec, pit, typ);
            if (ch == 2) SN_drum(tone + notepitch, dec, pit, typ);
            if (ch == 3) HH_drum(tone + notepitch, dec, pit, typ);

          } else {

            //_samplers[ch].noteEvent(12 * SampleRate[ch] + b - (ch + 1), vel, true, false);

            float delay_ms = mapf(SMP.param_settings[ch][DELAY], 0, maxfilterResolution, 0, maxParamVal[DELAY]);

            PendingNoteEvent ev = {
              .channel = (uint8_t)ch,
              .pitch = (uint8_t)(12 * SampleRate[ch] + b - (ch + 1)),
              .velocity = (uint8_t)vel,
              .triggerTime = millis() + delay_ms
            };

            pendingSampleNotes.push_back(ev);
          }
        } else if (ch == 11) {

          playSound(12 * octave[0] + transpose + b, 0);



        } else if (ch >= 13) {
          playSynth(ch, b, vel, false);
        }
      }
    }


    // midi functions
    if (waitForFourBars && pulseCount >= totalPulsesToWait) {
      beat = 1;
      if (fastRecordActive) stopFastRecord();
      SMP.page = 1;
      isNowPlaying = true;
      Serial.println("4 Bars Reached");
      waitForFourBars = false;  // Reset for the next start message
    }
    yield();
    beatStartTime = millis();
    beat++;
    checkPages();
  }


  yield();
}

void checkPages() {
  updateLastPage();  // recompute the highest page that actually has notes
  // compute what page beat should be on
  uint16_t newPage = (beat - 1) / maxX + 1;

  // if we stepped past the last non-empty page, restart at the top
  if (newPage > lastPage) {
    beat = 1;
    if (fastRecordActive) stopFastRecord();
    newPage = 1;
  }

  SMP.page = newPage;
}


void unpaint() {

  //SMP.edit = 1;
  paintMode = false;
  unsigned int y = SMP.y;
  unsigned int x = (SMP.edit - 1) * maxX + SMP.x;

  if ((y > 1 && y < 16)) {
    if (!SMP.singleMode) {

      note[x][y].channel = 0;
      note[x][y].velocity = defaultVelocity;
    } else {
      if (note[x][y].channel == SMP.currentChannel)
        note[x][y].channel = 0;
      note[x][y].velocity = defaultVelocity;
    }
  } else if (y == 16) {
    clearPageX(x);
  }
  updateLastPage();
  FastLEDshow();
}


void paint() {
  //SMP.edit = 1;
  Serial.println("!!!PAINT!");

  unsigned int x = SMP.x;  //(SMP.edit - 1) * maxX + SMP.x;
  unsigned int y = SMP.y;

  if (!SMP.singleMode) {
    if ((y > 1 && y <= 9) || (y == 12) || (y > 13 && y <= 15)) {
      if (note[x][y].channel == 0) {
        note[x][y].channel = (y - 1);
      }

    } else if (y == 16) toggleCopyPaste();  //copypaste if top above

  } else {
    if ((y > 0 && y <= 15)) {
      note[x][y].channel = SMP.currentChannel;
    }
  }

  if (note[x][y].channel > maxY - 2) {
    note[x][y].channel = 1;
    for (unsigned int vx = 1; vx < maxX + 1; vx++) {
      light(vx, note[x][y].channel + 1, col[note[x][y].channel] * 12);
    }
    FastLEDshow();
  }

  if (!isNowPlaying) {
    if (note[x][y].channel <= 9) {
      _samplers[note[x][y].channel].noteEvent(12 * SampleRate[note[x][y].channel] + y - (note[x][y].channel + 1), defaultVelocity, true, false);
      yield();
    } else if (note[x][y].channel == 11) {
      playSound(12 * octave[0] + transpose + y, 0);
    }

    else if (note[x][y].channel >= 13 && note[x][y].channel < 15) {

      int ch = note[x][y].channel;
      float frequency = fullFrequencies[y - (note[x][y].channel + 1) + 14];  // y-Wert ist 1-basiert, Array ist 0-basiert
      synths[ch][0]->frequency(frequency);
      float WaveFormVelocity = mapf(defaultVelocity, 1, 127, 0.0, 1.0);




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

      startTime[ch] = millis();    // Record the start time
      noteOnTriggered[ch] = true;  // Set the flag so we don't trigger noteOn again
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


void clearNoteChannel(unsigned int c, unsigned int yStart, unsigned int yEnd, unsigned int channel, bool singleMode) {

  for (unsigned int y = yStart; y < yEnd; y++) {
    if (singleMode) {
      if (note[c][y].channel == channel)
        note[c][y].channel = 0;
      if (note[c][y].channel == channel)
        note[c][y].velocity = defaultVelocity;
    } else {
      note[c][y].channel = 0;
      note[c][y].velocity = defaultVelocity;
    }
  }
}




void updateVolume() {
  SMP.vol = currentMode->pos[2];
  float vol = float(SMP.vol / 10.0);
  Serial.println("Vol: " + String(vol));
  if (vol <= 1.0) sgtl5000_1.volume(vol);
  //setvol = true;
}

void updateBrightness() {
  ledBrightness = (currentMode->pos[1] * 10) + 4 - 50;
  Serial.println("Brightness: " + String(ledBrightness));
}

void updateBPM() {
  //setvol = false;
  if (MIDI_CLOCK_SEND) {
    Serial.println("BPM: " + String(currentMode->pos[3]));
    SMP.bpm = currentMode->pos[3];
    playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
    playTimer.update(playNoteInterval);
    midiTimer.update(playNoteInterval);
  }
  drawBPMScreen();
}

void setVolume() {
  showExit(0);
  drawBPMScreen();

  if (currentMode->pos[1] != ledBrightness / 10) {
    updateBrightness();
  }

  if (currentMode->pos[2] != SMP.vol) {
    updateVolume();
  }


  if (currentMode->pos[3] != SMP.bpm) {
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
  sprintf(OUTPUTf, "%d.txt", SMP.file);
  if (SD.exists(OUTPUTf)) {
    showIcons(HELPER_SAVE, CRGB(1, 0, 0));
    showIcons(HELPER_LOAD, CRGB(0, 20, 0));
    drawNumber(SMP.file, CRGB(0, 0, 20), 11);
  } else {
    showIcons(HELPER_SAVE, CRGB(20, 0, 0));
    showIcons(HELPER_LOAD, CRGB(0, 1, 0));
    drawNumber(SMP.file, CRGB(20, 20, 40), 11);
  }
  FastLED.setBrightness(ledBrightness);
  FastLED.show();

  if (currentMode->pos[3] != SMP.file) {
    Serial.print("File: " + String(currentMode->pos[3]));
    Serial.println();
    SMP.file = currentMode->pos[3];
  }
}

void showSamplePack() {
  drawNoSD();
  FastLEDclear();

  showIcons(ICON_SAMPLEPACK, CRGB(10, 10, 0));
  showIcons(HELPER_SELECT, CRGB(0, 0, 10));
  drawNumber(SMP.pack, CRGB(20, 0, 0), 11);

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", SMP.pack, 1);
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
    Serial.println("File: " + String(currentMode->pos[3]));
    SMP.pack = currentMode->pos[3];
  }
}

void loadSamplePack(unsigned int pack, bool intro) {
  Serial.println("Loading SamplePack #" + String(pack));
  drawNoSD();
  EEPROM.put(0, pack);
  for (unsigned int z = 1; z < maxFiles; z++) {
    if (!intro) {
      showIcons(ICON_SAMPLE, CRGB(20, 20, 20));
    } else {
      drawText("LOAD", 2, 11, col[(maxFiles + 1) - z]);
    }
    drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], CRGB(50, 50, 50), intro);
    loadSample(pack, z);
  }
  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", pack, 1);
  switchMode(&draw);
}


void updateLastPage() {
  lastPage = 1;  // Reset lastPage before starting
  for (unsigned int p = 1; p <= maxPages; p++) {
    bool pageHasNotes = false;
    unsigned int baseIndex = (p - 1) * maxX;
    for (unsigned int ix = 1; ix <= maxX; ix++) {
      for (unsigned int iy = 1; iy <= maxY; iy++) {
        if (note[baseIndex + ix][iy].channel > 0) {
          pageHasNotes = true;
          break;  // No need to check further, this page has notes
        }
      }
      if (pageHasNotes) {
        lastPage = p;  // Update lastPage to the current page with notes
        break;         // No need to check further columns, go to the next page
      }
    }
    hasNotes[p] = pageHasNotes;
  }
  // Ensure lastPage is at least 1 to avoid potential issues
  if (lastPage == 0)
    lastPage = 1;
}

void loadWav() {
  playSdWav1.stop();

  Serial.println("Loading Wave :" + String(SMP.wav[SMP.currentChannel].fileID));
  loadSample(0, SMP.wav[SMP.currentChannel].fileID);
  switchMode(&singleMode);
  SMP.singleMode = true;
}
