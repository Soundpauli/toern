
extern "C" char *sbrk(int incr);

#define FASTLED_ALLOW_INTERRUPTS 1
//#define AUDIO_SAMPLE_RATE_EXACT 44117.64706
#define TargetFPS 32

#include <Wire.h>
#include <i2cEncoderLibV2.h>
#include "Arduino.h"
#include <Mapf.h>
#include <WS2812Serial.h>  // leds
#define USE_WS2812SERIAL   // leds

#include <FastLED.h>  // leds
#include <Audio.h>

#include <EEPROM.h>

#include "colors.h"
#include <TeensyPolyphony.h>
#include "audioinit.h"
#include <FastTouch.h>


#define maxX 16
#define maxY 16
#define INT_SD 10
#define NUM_LEDS 256
#define DATA_PIN 17  // PIN FOR LEDS
#define INT_PIN 27   // PIN FOR ENOCDER INTERRUPS
#define SWITCH_1 16 // Pin for TPP223 1
#define SWITCH_2 41 // Pin for TPP223 1
#define VOL_MIN 1
#define VOL_MAX 10
#define BPM_MIN 40
#define BPM_MAX 300
#define GAIN 0.25

#define NUM_ENCODERS 4

#define defaultVelocity 63
#define FOLDER_MAX 9

#define maxPages 8
#define maxFiles 9  // 9 samples, 2 Synths
#define maxFilters 15
#define maxfilterResolution 32

#define numPulsesForAverage 24  // Number of pulses to average over
#define pulsesPerBar (24 * 4)   // 24 pulses per quarter note, 4 quarter notes per bar

int lastFile[9] = { 0 };
bool tmpMute = false;
unsigned int menuPosition;
String oldPosString, posString = "0:0:";
String buttonString, oldButtonString = "0000";
unsigned long playStartTime = 0;  // To track when play(true) was last called

volatile uint8_t ledBrightness = 63;
const unsigned int maxlen = (maxX * maxPages) + 1;
const long ram = 11742820;  //(30sec) 15989988 - 5000 - 22168 - 220000;  // 16MB total, - 5kb for images+Icons, - 22kb for notes[] + 30k
const unsigned int SONG_LEN = maxX * maxPages;

bool touchState[] = {false};  // Current touch state (HIGH/LOW)
bool lastTouchState[] = {false};  // Previous touch state
const int touchThreshold = 40; 

const unsigned int totalPulsesToWait = pulsesPerBar * 2;

const unsigned long CHECK_INTERVAL = 50;  // Interval to check buttons in ms
unsigned long lastCheckTime = 0;          // Get the current time

/*timers*/
volatile unsigned int lastButtonPressTime = 0;
volatile bool resetTimerActive = false;

// runtime
//  variables for program logic
volatile float pulse = 1;
volatile int dir = 1;

unsigned int playNoteInterval = 150000;
volatile unsigned int RefreshTime = 1000 / TargetFPS;
float marqueePos = maxX;
bool shifted = false;
bool movingForward = true;  // Variable to track the direction of movement
unsigned volatile int lastUpdate = 0;
volatile unsigned int lastClockTime = 0;
volatile unsigned int totalInterval = 0;
volatile unsigned int clockCount = 0;
bool hasNotes[maxPages + 1];
unsigned int startTime = 0;    // Variable to store the start time
bool noteOnTriggered = false;  // Flag to indicate if noteOn has been triggered
volatile bool waitForFourBars = false;
volatile unsigned int pulseCount = 0;
bool sampleIsLoaded = false;
bool unpaintMode, paintMode = false;

volatile unsigned int pagebeat, beat = 1;
unsigned int samplePackID, fileID = 1;
EXTMEM unsigned int lastPreviewedSample[FOLDER_MAX] = {};
IntervalTimer playTimer;
unsigned int lastPage = 1;

EXTMEM unsigned int tmp[maxlen][maxY + 1][2] = {};
EXTMEM unsigned int original[maxlen][maxY + 1][2] = {};
EXTMEM unsigned int note[maxlen][maxY + 1][2] = {};

unsigned int sample_len[maxFiles];
bool sampleLengthSet = false;
bool isPlaying = false;  // global
int PrevSampleRate = 1;
volatile EXTMEM int SampleRate[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
volatile EXTMEM unsigned char sampled[maxFiles][ram / (maxFiles + 1)];

const float pianoFrequencies[16] = {
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


const String usedFiles[13] = { "samples/_1.wav",
                               "samples/_2.wav",
                               "samples/_3.wav",
                               "samples/_4.wav",
                               "samples/_5.wav",
                               "samples/_6.wav",
                               "samples/_7.wav",
                               "samples/_8.wav",
                               "samples/_9.wav",
                               "samples/_10.wav",
                               "samples/_11.wav",
                               "samples/_12.wav",
                               "samples/_13.wav" };

const int number[10][24][2] = {
  { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 1, 3 }, { 4, 3 }, { 1, 4 }, { 4, 4 }, { 1, 5 }, { 4, 5 }, { 1, 6 }, { 4, 6 }, { 1, 7 }, { 4, 7 }, { 1, 8 }, { 1, 9 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 1, 2 }, { 4, 8 }, { 4, 8 }, { 4, 8 } },  //0
  { { 3, 1 }, { 2, 2 }, { 3, 2 }, { 1, 3 }, { 3, 3 }, { 3, 4 }, { 3, 5 }, { 3, 6 }, { 3, 7 }, { 3, 8 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 } },  //1
  { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 4, 3 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 }, { 1, 6 }, { 1, 7 }, { 1, 8 }, { 1, 9 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 1, 2 }, { 4, 8 }, { 4, 8 }, { 4, 8 }, { 4, 8 }, { 4, 8 } },  //2
  { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 4, 3 }, { 4, 4 }, { 2, 5 }, { 3, 5 }, { 4, 5 }, { 4, 6 }, { 4, 7 }, { 4, 8 }, { 1, 9 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 } },  //3
  { { 1, 1 }, { 4, 1 }, { 1, 2 }, { 4, 2 }, { 1, 3 }, { 4, 3 }, { 1, 4 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 }, { 4, 6 }, { 4, 7 }, { 4, 8 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 } },  //4
  { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 }, { 4, 6 }, { 4, 7 }, { 4, 8 }, { 1, 9 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 } },  //5
  { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 }, { 1, 6 }, { 4, 6 }, { 1, 7 }, { 4, 7 }, { 1, 8 }, { 4, 8 }, { 1, 9 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 }, { 4, 9 } },  //6
  { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 4, 3 }, { 4, 4 }, { 4, 5 }, { 4, 6 }, { 4, 7 }, { 4, 8 }, { 4, 9 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 } },  //7
  { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 1, 3 }, { 4, 3 }, { 1, 4 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 }, { 1, 6 }, { 4, 6 }, { 1, 7 }, { 4, 7 }, { 1, 8 }, { 1, 9 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 1, 2 }, { 4, 8 } },  //8
  { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 1, 3 }, { 4, 3 }, { 1, 4 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 }, { 4, 6 }, { 4, 7 }, { 1, 9 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 1, 2 }, { 4, 8 }, { 4, 8 }, { 4, 8 }, { 4, 8 } }
};

const int logo[102][2] = {
  { 1, 2 }, { 2, 2 }, { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 }, { 7, 2 }, { 8, 2 }, { 9, 2 }, { 10, 2 }, { 11, 2 }, { 12, 2 }, { 13, 2 }, { 14, 2 }, { 15, 2 }, { 15, 3 }, { 15, 4 }, { 15, 5 }, { 15, 6 }, { 15, 7 }, { 15, 8 }, { 14, 8 }, { 13, 8 }, { 12, 8 }, { 11, 8 }, { 11, 9 }, { 11, 10 }, { 11, 11 }, { 11, 12 }, { 11, 13 }, { 11, 14 }, { 10, 14 }, { 9, 14 }, { 9, 13 }, { 9, 12 }, { 9, 11 }, { 9, 10 }, { 9, 9 }, { 9, 8 }, { 9, 7 }, { 9, 6 }, { 9, 5 }, { 9, 4 }, { 8, 4 }, { 7, 4 }, { 6, 4 }, { 5, 4 }, { 4, 4 }, { 3, 4 }, { 3, 5 }, { 3, 6 }, { 3, 7 }, { 4, 7 }, { 5, 7 }, { 6, 7 }, { 7, 7 }, { 7, 8 }, { 7, 9 }, { 7, 10 }, { 7, 11 }, { 7, 12 }, { 7, 13 }, { 7, 14 }, { 6, 14 }, { 5, 14 }, { 4, 14 }, { 3, 14 }, { 2, 14 }, { 1, 14 }, { 1, 13 }, { 1, 12 }, { 2, 12 }, { 3, 12 }, { 4, 12 }, { 5, 12 }, { 5, 11 }, { 5, 10 }, { 5, 9 }, { 4, 9 }, { 3, 9 }, { 2, 9 }, { 1, 9 }, { 1, 8 }, { 1, 7 }, { 1, 6 }, { 1, 5 }, { 1, 4 }, { 1, 3 }, { 11, 4 }, { 12, 4 }, { 13, 4 }, { 13, 5 }, { 13, 6 }, { 12, 6 }, { 11, 6 }, { 11, 5 }

};

const int icon_samplepack[18][2] = { { 2, 1 }, { 2, 2 }, { 3, 2 }, { 2, 3 }, { 2, 4 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 4, 5 }, { 5, 5 }, { 1, 6 }, { 2, 6 }, { 4, 6 }, { 4, 7 }, { 3, 8 }, { 4, 8 }, { 3, 9 }, { 4, 9 } };
const int icon_sample[19][2] = { { 3, 1 }, { 3, 2 }, { 3, 3 }, { 4, 3 }, { 3, 5 }, { 4, 2 }, { 5, 3 }, { 3, 4 }, { 3, 5 }, { 3, 6 }, { 1, 7 }, { 2, 7 }, { 3, 7 }, { 1, 8 }, { 2, 8 }, { 3, 8 }, { 1, 9 }, { 2, 9 }, { 3, 9 } };
const int icon_loadsave[20][2] = { { 3, 1 }, { 3, 2 }, { 3, 3 }, { 3, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 }, { 5, 5 }, { 2, 6 }, { 3, 6 }, { 4, 6 }, { 3, 7 }, { 1, 8 }, { 5, 8 }, { 1, 9 }, { 2, 9 }, { 3, 9 }, { 4, 9 }, { 5, 9 } };
const int icon_bpm[38][2] = { { 2, 11 }, { 3, 11 }, { 4, 11 }, { 7, 11 }, { 8, 11 }, { 9, 11 }, { 11, 11 }, { 15, 11 }, { 2, 12 }, { 4, 12 }, { 7, 12 }, { 9, 12 }, { 11, 12 }, { 12, 12 }, { 14, 12 }, { 15, 12 }, { 2, 13 }, { 3, 13 }, { 4, 13 }, { 5, 13 }, { 7, 13 }, { 8, 13 }, { 9, 13 }, { 11, 13 }, { 13, 13 }, { 15, 13 }, { 2, 14 }, { 5, 14 }, { 7, 14 }, { 11, 14 }, { 15, 14 }, { 2, 15 }, { 3, 15 }, { 4, 15 }, { 5, 15 }, { 7, 15 }, { 11, 15 }, { 15, 15 } };
const int helper_load[3][2] = { { 1, 15 }, { 2, 15 }, { 3, 15 } };
const int helper_folder[5][2] = { { 6, 13 }, { 6, 14 }, { 6, 15 }, { 7, 14 }, { 7, 15 } };
const int helper_seek[2][2] = { { 10, 15 }, { 10, 14 } };

const int helper_bright[5][2] =  { { 5, 14 }, { 6, 13 }, { 6 ,15 }, { 6, 14 }, { 7, 14 } };
const int helper_vol[5][2] = { { 9, 13 }, { 11, 13 }, { 10, 15 }, { 9, 14 }, { 11, 14 } };
const int helper_bpm[7][2] = { { 13, 13 }, { 13, 14 }, { 13, 15 }, { 14, 14 }, { 15, 14 }, { 14, 15 }, { 15, 15 } };


const int helper_save[3][2] = { { 5, 15 }, { 6, 15 }, { 7, 15 } };
const int helper_select[3][2] = { { 13, 15 }, { 14, 15 }, { 15, 15 } };

const int noSD[48][2] = { { 2, 4 }, { 3, 4 }, { 4, 4 }, { 5, 4 }, { 7, 4 }, { 8, 4 }, { 9, 4 }, { 12, 4 }, { 13, 4 }, { 14, 4 }, { 15, 4 }, { 2, 5 }, { 5, 5 }, { 7, 5 }, { 9, 5 }, { 10, 5 }, { 12, 5 }, { 15, 5 }, { 2, 6 }, { 7, 6 }, { 10, 6 }, { 15, 6 }, { 2, 7 }, { 3, 7 }, { 4, 7 }, { 5, 7 }, { 7, 7 }, { 10, 7 }, { 13, 7 }, { 14, 7 }, { 15, 7 }, { 5, 8 }, { 7, 8 }, { 10, 8 }, { 13, 8 }, { 2, 9 }, { 5, 9 }, { 7, 9 }, { 9, 9 }, { 10, 9 }, { 2, 10 }, { 3, 10 }, { 4, 10 }, { 5, 10 }, { 7, 10 }, { 8, 10 }, { 9, 10 }, { 13, 10 } };

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

Mode draw = { "DRAW", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxY }, { 1, 1, maxfilterResolution, 1 }, { 0x110011, 0xFFFFFF, 0x00FF00, 0x110011 } };
Mode singleMode = { "SINGLE", { 1, 1, 0, 1 }, { maxY, maxX, maxfilterResolution, maxY }, { 1, 1, maxfilterResolution, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode volume_bpm = { "VOLUME_BPM", { 1, 63, VOL_MIN, BPM_MIN }, { 1, 255, VOL_MAX, BPM_MAX }, { 1, 63, 9, 100 }, { 0x000000, 0xFFFFFF, 0xFF8800, 0xFFFF00 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 0, 0, 7 }, { 9, 0, 0, 9 }, { 8, 0, 0, 8 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { 1, 1, maxY, maxY }, { 1, 1, 10, 10 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode set_Wav = { "SET_WAV", { 1, 1, 1, 1 }, { 999, FOLDER_MAX, 999, 999 }, { 44, 44, 999, 999 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 1 }, { 1, 1, 99, 99 }, { 1, 1, 1, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode loadSaveTrack = { "LOADSAVE_TRACK", { 1, 1, 1, 1 }, { 1, 1, 12, 12 }, { 1, 1, 1, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode menu = { "MENU", { 1, 1, 1, 1 }, { 1, 1, 1, 5 }, { 1, 1, 1, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };

// Declare currentMode as a global variable
Mode *currentMode = &draw;



// Declare the device struct
struct Device {
  unsigned int singleMode;  // single Sample Mod
  unsigned int currentChannel;
  unsigned int vol;           // volume
  unsigned int bpm;           // bpm
  unsigned int velocity;      // velocity
  unsigned int page;          // current page
  unsigned int edit;          // edit mode or plaing mode?
  unsigned int file;          // current selected save/load id
  unsigned int pack;          // current selected samplepack id
  unsigned int wav[maxY][2];  // current selected sample id
  unsigned int folder;        // current selected folder id
  bool activeCopy;            // is copy/paste active?
  unsigned int x;             // cursor X
  unsigned int y;             // cursor Y
  unsigned int seek;          // skipped into sample
  unsigned int seekEnd;
  unsigned int smplen;  // overall selected samplelength
  unsigned int shiftX;  // note Shift
  unsigned int shiftY;  // note Shift
  unsigned int filter_knob[maxFilters];
  unsigned int mute[maxY];
};


//EXTMEM?
volatile Device SMP = { false, 1, 10, 100, 10, 1, 1, 1, 1, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0, false, 1, 16, 0, 0, 0, 0, 0, { maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution }, {} };

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
  i2cEncoderLibV2(0b0100000),  // third encoder address
  i2cEncoderLibV2(0b1110000),  // 2nd encoder address +
  i2cEncoderLibV2(0b1000100),  // First encoder address
  i2cEncoderLibV2(0b1000101),  // First encoder address
};
// Global variable to track current encoder index for callbacks
int currentEncoderIndex = 0;

EXTMEM arraysampler _samplers[13];

AudioPlayArrayResmp *voices[] = { &sound0, &sound1, &sound2, &sound3, &sound4, &sound5, &sound6, &sound7, &sound8, &sound9, &sound10, &sound11, &sound12 };
AudioEffectEnvelope *envelopes[] = { &envelope0, &envelope1, &envelope2, &envelope3, &envelope4, &envelope5, &envelope6, &envelope7, &envelope8, &envelope9, &envelope10, &envelope11, &envelope12, &envelope13, &envelope14 };
AudioFilterStateVariable *filters[] = { &filter0, &filter1, &filter2, &filter3, &filter4, &filter5, &filter6, &filter7, &filter8, &filter9, &filter10, &filter11, &filter12, &filter13, &filter14 };


void resetAllFilters() {
  for (unsigned int i = 0; i < maxFilters; i++) {
    filters[i]->frequency(0);
    filters[i]->resonance(0);
  }
}


void allOff() {
  for (AudioEffectEnvelope *envelope : envelopes) {
    envelope->noteOff();
  }
}

void FastLEDclear() {
  FastLED.clear();
}

void FastLEDshow() {
  if (millis() - lastUpdate > RefreshTime) {
    lastUpdate = millis();
     FastLED.setBrightness(ledBrightness);
    FastLED.show();
  }
}

void drawNoSD() {
  bool noSDfound = false;

  while (!SD.begin(INT_SD)) {
    FastLEDclear();
    for (unsigned int gx = 0; gx < 48; gx++) {
      light(noSD[gx][0], maxY - noSD[gx][1], CRGB(50, 0, 0));
    }
    FastLEDshow();
    delay(1000);
    noSDfound = true;
  }

  if (noSDfound && SD.begin(INT_SD)) {
    FastLEDclear();
    setLastFile();
    noSDfound = false;
  }
}

void copyPosValues(Mode *source, Mode *destination) {
  //  for (unsigned int i = 0; i < 3; i++) {
  //  destination->pos[i] = source->pos[i];
  // }
}

void setVelocity() {

  if (currentMode->pos[3] != SMP.velocity) {
    SMP.velocity = currentMode->pos[3];
    Serial.println(currentMode->pos[3]);
    if (!SMP.singleMode) {
      note[SMP.x][SMP.y][1] = round(mapf(currentMode->pos[3], 1, maxY, 1, 127));
    } else {
      Serial.println("Overal Velocity: " + String(currentMode->pos[3]));
      for (unsigned int nx = 1; nx < maxlen; nx++) {
        for (unsigned int ny = 1; ny < maxY + 1; ny++) {
          if (note[nx][ny][0] == SMP.currentChannel)
            note[nx][ny][1] = round(mapf(currentMode->pos[3], 1, maxY, 1, 127));
        }
      }
    }
  }
  drawVelocity(CRGB(0, 40, 0));
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
  obj->writeRGBCode(0xFFFFFF);
}


void encoder_button_released(i2cEncoderLibV2 *obj, int encoderIndex) {
  buttonState[encoderIndex] = RELEASED;
}


/* 
//not used
void encoder_threshfolds(i2cEncoderLibV2 *obj, int encoderIndex) {
  if (obj->readStatus(i2cEncoderLibV2::RMAX)) {
    obj->writeRGBCode(0xFF0000);
  } else {
    obj->writeRGBCode(0xFFFF00);
  }
}
*/



void handle_button_state(i2cEncoderLibV2 *obj, int encoderIndex) {
  // Only process the button if it's the last pressed encoder
  unsigned long currentTime = millis();
  pressDuration[encoderIndex] = currentTime - buttonPressStartTime[encoderIndex];

  switch (buttonState[encoderIndex]) {
    case RELEASED:
      if (pressDuration[encoderIndex] <= longPressDuration[encoderIndex]) {
        buttons[encoderIndex] = 1;  // Released
        buttonState[encoderIndex] = IDLE;
        obj->writeRGBCode(0x0000FF);
        isPressed[encoderIndex] = false;
        return;
      } else {


        buttons[encoderIndex] = 9;  // Released
        buttonState[encoderIndex] = IDLE;
        obj->writeRGBCode(0x000000);
        isPressed[encoderIndex] = false;
      }
      break;

    case LONG_PRESSED:
      if (pressDuration[encoderIndex] >= longPressDuration[encoderIndex]) {
        obj->writeRGBCode(0xFF00FF);  // Purple for long press
        buttons[encoderIndex] = 2;    // Long press
        buttonState[encoderIndex] = IDLE;
        isPressed[encoderIndex] = true;
      }
      break;

    default:
      if (isPressed[encoderIndex] && pressDuration[encoderIndex] >= longPressDuration[encoderIndex]) {
        obj->writeRGBCode(0xFF00FF);  // Purple for long press
        buttons[encoderIndex] = 2;    // Long press
        buttonState[encoderIndex] = LONG_PRESSED;
        isPressed[encoderIndex] = true;
      }
      break;
  }
}

void switchMode(Mode *newMode) {

  unpaintMode = false;
  SMP.singleMode = false;
  paintMode = false;


  Serial.println(newMode->name);
  oldButtonString = buttonString;
  if (newMode != currentMode) {
    currentMode = newMode;
    // Set last saved values for encoders
    for (int i = 0; i < NUM_ENCODERS; i++) {
      Encoder[i].writeRGBCode(currentMode->knobcolor[i]);
      Encoder[i].writeMax((int32_t)currentMode->maxValues[i]);  //maxval
      Encoder[i].writeMin((int32_t)currentMode->minValues[i]);  //minval
      if (currentMode != &singleMode &&  currentMode != &draw) Encoder[i].writeCounter((int32_t)(currentMode->pos[i]));
    }
  }
}


void printPressed() {
  for (int i = 0; i < NUM_ENCODERS; i++) {
    Serial.print(pressed[i]);
    Serial.print("-");
  }
  Serial.println();
}
void checkMode(String buttonString, bool reset) {

  Serial.print("-----> | ");
  Serial.println(buttonString);
  // Toggle play/pause in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && buttonString == "0010") {


    if (!isPlaying) {
      Serial.println("PLAY");
      play(true);
    } else {

      unsigned long currentTime = millis();
      if (currentTime - playStartTime > 200) {  // Check if play started more than 200ms ago
        pause();
      }
    }

    //if (isPlaying) pause();
  }

  if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && buttonString == "0200") {
    tmpMute = true;
    tmpMuteAll(true);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0900") {
    if (tmpMute) tmpMuteAll(false);
  }

  // Shift notes around in single mode after dblclick of button 4
  if (currentMode == &singleMode && buttonString == "0220") {
    SMP.shiftX = 8;
    SMP.shiftY = 8;
    //encoders[2].write(8 * 4);
    Encoder[3].writeCounter((int32_t)(8));
    unsigned int patternLength = lastPage * maxX;

    for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
      for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
        original[nx][ny][0] = 0;
        original[nx][ny][1] = defaultVelocity;
      }
    }

    // Step 2: Backup non-current channel notes into the original array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (note[nx][ny][0] != SMP.currentChannel) {
          original[nx][ny][0] = note[nx][ny][0];
          original[nx][ny][1] = note[nx][ny][1];
        }
      }
    }




    // Switch to note shift mode
    switchMode(&noteShift);
    SMP.singleMode = true;
  }

  if (currentMode == &noteShift && buttonString == "0001") {
    switchMode(&singleMode);
    SMP.singleMode = true;
  }

  if (currentMode == &noteShift && buttonString == "1000") {
    // toDO: undo note shift on cancel
    switchMode(&singleMode);
    SMP.singleMode = true;
  }

  // Switch to volume mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0020") {
    if (isPlaying) {
      play(false);
    } else {
    }
    switchMode(&volume_bpm);
  }

  // Toggle copy/paste in draw mode
  if (currentMode == &draw && buttonString == "1100") {
    toggleCopyPaste();
  }



  // Handle velocity switch in single mode
  if ((currentMode == &singleMode) && buttonString == "0005") {
    int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 127, 1, maxY));
    Serial.println(velo);
    SMP.velocity = velo;
    switchMode(&velocity);
    SMP.singleMode = true;
    //encoders[2].write(velo);
    Encoder[3].writeCounter((int32_t)(velo));
  }

  // Handle velocity switch in draw mode
  if ((currentMode == &draw) && buttonString == "0005") {
    int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 127, 1, maxY));
    Serial.println(velo);
    SMP.velocity = velo;
    SMP.singleMode = false;
    switchMode(&velocity);
    //encoders[2].write(velo);
    Encoder[3].writeCounter((int32_t)(velo));
  }

  // Print button string if it has changed -------------------- BUTTONS-------------

  // Clear page in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "2002") {
    clearPage();
  }

  // Switch to single mode from velocity mode
  if (currentMode == &velocity && SMP.singleMode && buttonString == "0001") {
    switchMode(&singleMode);
    SMP.singleMode = true;
  }

  // Switch to draw mode from velocity mode
  if (currentMode == &velocity && !SMP.singleMode && buttonString == "0001") {
    switchMode(&draw);
  }

  // Switch to draw mode from volume mode
  if (currentMode == &volume_bpm && buttonString == "0090") {
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
    switchMode(&loadSaveTrack);
  } else if ((currentMode == &loadSaveTrack) && buttonString == "0001") {
    paintMode = false;
    unpaintMode = false;
    switchMode(&draw);
  } else if ((currentMode == &loadSaveTrack) && buttonString == "0100") {
    savePattern(false);
  } else if ((currentMode == &loadSaveTrack) && buttonString == "1000") {
    loadPattern(false);
  }

  // Search Wave + Load + Exit
  if ((currentMode == &singleMode) && buttonString == "2200") {
    //set loaded sample
    switchMode(&set_Wav);
    currentMode->pos[3] = SMP.wav[SMP.currentChannel][0];
    SMP.wav[SMP.currentChannel][1] = SMP.wav[SMP.currentChannel][0];
    //encoders[2].write((SMP.wav[SMP.currentChannel][0] * 4)-1);
    //Encoder[3].writeCounter((int32_t)((SMP.wav[SMP.currentChannel][0] * 4) - 1));

  } else if ((currentMode == &set_Wav) && buttonString == "1000") {
    //set SMP.wav[currentChannel][0] and [1] to current file
    SMP.wav[SMP.currentChannel][0] = SMP.wav[SMP.currentChannel][1];
    currentMode->pos[3] = SMP.wav[SMP.currentChannel][0];
    loadWav();
    autoSave();
  } else if ((currentMode == &set_Wav) && buttonString == "0001") {
    switchMode(&singleMode);
    SMP.singleMode = true;
  }

  // Set SamplePack + Load + Save + Exit
  if ((currentMode == &draw) && buttonString == "2200") {
    switchMode(&set_SamplePack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "0100") {
    saveSamplePack(SMP.pack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "1000") {
    loadSamplePack(SMP.pack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "0001") {
    switchMode(&draw);
  }

  // Toggle mute in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0100") {
    toggleMute();
  }

  // Enable paint mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0002") {
    paintMode = true;
  }

  // Unpaint and delete active copy in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "2000") {
    unpaint();
    unpaintMode = true;
    deleteActiveCopy();
  }


/*
  // Normal paint in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0005") {
    paintMode = false;
    unpaintMode = false;
    paint();
  }

  // Normal unpaint in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "5000") {
    paintMode = false;
    unpaintMode = false;
    unpaint();
  }

*/

  // Toggle SingleMode
  if (currentMode == &singleMode && buttonString == "3000") {
    //copyPosValues(&singleMode, &draw);
    switchMode(&draw);
  } else if (currentMode == &draw && buttonString == "3000" && ((SMP.y - 1 >= 1 && SMP.y - 1 <= maxFiles) || SMP.y - 1 > 12)) {
    SMP.currentChannel = SMP.y - 1;
    //copyPosValues(&draw, &singleMode);
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

void setup(void) {
  NVIC_SET_PRIORITY(IRQ_USB1, 128);  // USB1 for Teensy 4.x
  pinMode(INT_PIN, INPUT_PULLUP);    // Interrups for encoder
  Serial.println("**** I2C Encoder V2 Multi-Encoder Example ****");
  Wire.begin();
  
  

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

    Encoder[i].writeAntibouncingPeriod(10);
    Encoder[i].writeCounter((int32_t)1);
    Encoder[i].writeMax((int32_t)16);  //maxval
    Encoder[i].writeMin((int32_t)1);   //minval
    Encoder[i].writeStep((int32_t)1);  //steps

    // Assign static callback wrappers
    Encoder[i].onButtonPush = staticButtonPushed;
    Encoder[i].onButtonRelease = staticButtonReleased;
    Encoder[i].onButtonDoublePush = staticDoublePush;

    Encoder[i].onMinMax = staticThresholds;
    Encoder[i].autoconfigInterrupt();
    Encoder[i].writeRGBCode(0xFFFFFF);
    Encoder[i].writeFadeRGB(1);
    delay(50);
    Encoder[i].updateStatus();
  }




  delay(200);
  Serial.begin(9600);

  if (CrashReport) {
    Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
    Serial.print(CrashReport);
    delay(1000);
  }

  usbMIDI.setHandleClock(myClock);
  usbMIDI.setHandleStart(handleStart);
  usbMIDI.setHandleStop(handleStop);
  usbMIDI.setHandleSongPosition(handleSongPosition);  // Set the handler for SPP messages
  usbMIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  usbMIDI.setHandleNoteOn(handleNoteOn);
  usbMIDI.setHandleNoteOff(handleNoteOff);

  EEPROM.get(0, samplePackID);
  Serial.print("SamplePackID:");
  Serial.println(samplePackID);

  if (samplePackID == NAN || samplePackID == 0) {
    Serial.print("NO SAMPLEPACK SET! Defaulting to 1");
    samplePackID = 1;
  }

  pinMode(0, INPUT_PULLDOWN);
  pinMode(3, INPUT_PULLDOWN);
  pinMode(16, INPUT_PULLDOWN);
  FastLED.addLeds<WS2812SERIAL, DATA_PIN, BRG>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);
  //showIntro();
  switchMode(&draw);

  Serial.print("Initializing SD card...");
  drawNoSD();

  getLastFiles();
  loadSamplePack(samplePackID);

  for (unsigned int vx = 1; vx < SONG_LEN + 1; vx++) {
    for (unsigned int vy = 1; vy < maxY + 1; vy++) {
      note[vx][vy][1] = defaultVelocity;
    }
  }

  _samplers[0].addVoice(sound0, mixer4, 3, envelope0);

  _samplers[1].addVoice(sound1, mixer1, 0, envelope1);
  _samplers[2].addVoice(sound2, mixer1, 1, envelope2);
  _samplers[3].addVoice(sound3, mixer1, 2, envelope3);
  _samplers[4].addVoice(sound4, mixer1, 3, envelope4);

  _samplers[5].addVoice(sound5, mixer2, 0, envelope5);
  _samplers[6].addVoice(sound6, mixer2, 1, envelope6);
  _samplers[7].addVoice(sound7, mixer2, 2, envelope7);
  _samplers[8].addVoice(sound8, mixer2, 3, envelope8);

  _samplers[9].addVoice(sound9, mixer3, 0, envelope9);
  _samplers[10].addVoice(sound10, mixer3, 1, envelope10);
  _samplers[11].addVoice(sound11, mixer3, 2, envelope11);
  _samplers[12].addVoice(sound12, mixer3, 3, envelope12);

  //_samplers[13].addVoice(sound13, mixer4, 0, envelope13);
  //_samplers[15].addVoice(sound15, mixer4, 2 , envelope15);

  mixer1.gain(0, GAIN);
  mixer1.gain(1, GAIN);
  mixer1.gain(2, GAIN);
  mixer1.gain(3, GAIN);

  mixer2.gain(0, GAIN);
  mixer2.gain(1, GAIN);
  mixer2.gain(2, GAIN);
  mixer2.gain(3, GAIN);

  mixer3.gain(0, GAIN);
  mixer3.gain(1, GAIN);
  mixer3.gain(2, GAIN);
  mixer3.gain(3, GAIN);

  mixer4.gain(0, GAIN);
  mixer4.gain(1, GAIN);
  mixer4.gain(2, GAIN);
  mixer4.gain(3, GAIN);

  mixer_end.gain(0, GAIN);
  mixer_end.gain(1, GAIN);
  mixer_end.gain(2, GAIN);
  mixer_end.gain(3, GAIN);



  // configure what the synth will sound like
  //FIRST SYNTH
  sound13.begin(WAVEFORM_SAWTOOTH);
  sound13.amplitude(0.3);
  sound13.frequency(261.62);
  sound13.phase(0);

  envelope13.attack(0);
  envelope13.decay(200);
  envelope13.sustain(1);
  envelope13.release(500);


  //SECOND SYNTH
  sound14.begin(WAVEFORM_SQUARE);
  sound14.amplitude(0.5);
  sound14.frequency(261.62);  //C
  sound14.phase(0);

  envelope14.attack(0);
  envelope14.decay(200);
  envelope14.sustain(1);
  envelope14.release(500);  // Release time set to 200 ms

  // set filters and envelopes for all sounds
  for (unsigned int i = 1; i < maxFilters; i++) {
    envelopes[i]->attack(0);
    filters[i]->octaveControl(6.0);
    filters[i]->resonance(0.7);
    filters[i]->frequency(100 * 32);
  }

  for (unsigned int i = 0; i < maxFiles; i++) {
    Serial.print("START VOICE:");
    Serial.println(i);
    voices[i]->enableInterpolation(true);
  }


  //AudioInterrupts();

  // set BPM:100
  SMP.bpm = 100;
  playTimer.priority(230);
  playTimer.begin(playNote, playNoteInterval);
  playTimer.priority(230);


  // turn on the output
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.9);

  sgtl5000_1.unmuteLineout();
  // According to info in libraries\Audio\control_sgtl5000.cpp
  // 31 is LOWEST output of 1.16V and 13 is HIGHEST output of 3.16V
  sgtl5000_1.lineOutLevel(1);
  AudioMemory(32);
  autoLoad();
}

void setEncoderColor(int i) {
  Encoder[i].writeRGBCode(0x00FF00);
}


void checkEncoders() {
  buttonString = "";
  for (int i = 0; i < NUM_ENCODERS; i++) {
    currentEncoderIndex = i;
    Encoder[i].updateStatus();
    currentMode->pos[i] = Encoder[i].readCounterInt();
    posString += String(currentMode->pos[i]) + ":";
    handle_button_state(&Encoder[i], i);



    buttonString += String(buttons[i]);
  }

  if (posString != oldPosString) {
    oldPosString = posString;
    SMP.x = currentMode->pos[3];
    SMP.y = currentMode->pos[0];
    if (currentMode == &draw || currentMode == &singleMode) Encoder[3].writeRGBCode(CRGBToUint32(col[SMP.y-1]));

    //WHY???
    //SMP.edit = 1;

    if (paintMode) {
      note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = SMP.y - 1;
    }
    if (paintMode && currentMode == &singleMode) {
      note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = SMP.currentChannel;
    }

    if (unpaintMode) {
      if (SMP.singleMode) {
        if (note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] == SMP.currentChannel)
          note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = 0;
      } else {
        note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = 0;
      }
    }

    unsigned int editpage = currentMode->pos[1];
    if (editpage != SMP.edit && editpage <= lastPage) {
      SMP.edit = editpage;
      //Serial.println("p:" + String(SMP.edit));
    }
  }
}


void checkButtons() {
  unsigned long currentTime = millis();
  // Check at defined intervals
  if (currentTime - lastCheckTime >= CHECK_INTERVAL) {
    lastCheckTime = currentTime;  // Reset the timer  
    if (buttonString != oldButtonString) {
      // Only trigger if buttonString has meaningful input
      //Serial.println(buttonString);
      oldButtonString = buttonString;
      checkMode(buttonString, true);
    }
  }
}


void checkSingleTouch(){
  int touchValue = fastTouchRead(SWITCH_1);
 
  // Determine if the touch is above the threshold
  touchState[0] = (touchValue > touchThreshold);
  // Check for a rising edge (LOW to HIGH transition)
  if (touchState[0] && !lastTouchState[0]) {
    // Toggle the mode only on a rising edge
    if (currentMode == &draw) {
       SMP.currentChannel = SMP.y - 1;
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

void checkMenuTouch(){
  int touchValue = fastTouchRead(SWITCH_2);
   Serial.println(touchValue);
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


void loop() {

  //checkSingleTouch();
  checkMenuTouch();




  if ((currentMode == &draw || currentMode == &singleMode) && pressed[3] == true) {
    paintMode = false;
    unpaintMode = false;
    pressed[3] = false;
    paint();
  }

  if ((currentMode == &draw || currentMode == &singleMode) && pressed[0] == true) {
    Serial.println("UNPAINT");
    paintMode = false;
    unpaintMode = false;
    pressed[0] = false;
    unpaint();
  }


  checkEncoders();
  checkButtons();

  //fix?
  SMP.edit = 1;


  // Continuously check encoders


  // Set stateMashine
  if (currentMode->name == "DRAW") {
    //checkEncoders();
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
  }else if (currentMode->name == "SET_SAMPLEPACK") {
    showSamplePack(); 
  } else if (currentMode->name == "SET_WAV") {
    showWave();


  } else if (currentMode->name == "NOTE_SHIFT") {
    shiftNotes();
    drawBase();
    drawTriggers();
    if (isPlaying) {
      drawTimer(pagebeat);
    }
  }

  if (currentMode == &draw || currentMode == &singleMode) {
    drawBase();
    drawTriggers();
    if (currentMode != &velocity)
      drawCursor();
    if (isPlaying) {
      drawTimer(pagebeat);
      
      FastLEDshow();
    }
  }

  // end synthsound after X ms
  if (noteOnTriggered && millis() - startTime >= 200) {
    envelope14.noteOff();
    envelope13.noteOff();
    noteOnTriggered = false;
  }

  FastLEDshow();  // draw!
  yield();
  delay(50);  // 50????
  yield();    // 50????
}


void setLastFile() {
  //set maxFiles in folder and show loading...
  for (int f = 0; f <= FOLDER_MAX; f++) {
    FastLEDclear();

    for (unsigned int i = 1; i < 99; i++) {
      char OUTPUTf[50];
      sprintf(OUTPUTf, "samples/%d/_%d.wav", f, i + (f * 100));
      if (SD.exists(OUTPUTf)) {
        lastFile[f] = i + (f * 100);
      }
    }
    drawLoadingBar(1, 999, lastFile[f], col_Folder[f], CRGB(15, 15, 55));
  }
  //set lastFile Array into Eeprom
  EEPROM.put(100, lastFile);
}

void getLastFiles() {
  //get lastFile Array from Eeprom
  EEPROM.get(100, lastFile);
  //if lastFile from eeprom is empty, set it
  if (lastFile[0] == 0) {
    setLastFile();
  }
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
    //encoders[2].write(8 * 4);
    Encoder[3].writeCounter((int32_t)(8));
    currentMode->pos[3] = 8;
    // Step 1: Clear the tmp array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
      for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
        tmp[nx][ny][0] = 0;
        tmp[nx][ny][1] = defaultVelocity;
      }
    }

    // Step 2: Shift notes of the current channel into tmp array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (note[nx][ny][0] == SMP.currentChannel) {
          int newposX = nx + shiftDirectionX;

          // Handle wrapping around the edges
          if (newposX < 1) {
            newposX = patternLength;
          } else if (newposX > patternLength) {
            newposX = 1;
          }
          tmp[newposX][ny][0] = SMP.currentChannel;
          tmp[newposX][ny][1] = note[nx][ny][1];
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
    //encoders[0].write(8 * 4);
    Encoder[0].writeCounter((int32_t)((8)));
    currentMode->pos[0] = 8;
    SMP.shiftY = 8;
    // Step 1: Clear the tmp array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
      for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
        tmp[nx][ny][0] = 0;
        tmp[nx][ny][1] = defaultVelocity;
      }
    }

    // Step 2: Shift notes of the current channel into tmp array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (note[nx][ny][0] == SMP.currentChannel) {
          int newposY = ny + shiftDirectionY;
          // Handle wrapping around the edges
          if (newposY < 1) {
            newposY = maxY;
          } else if (newposY > maxY) {
            newposY = 1;
          }
          tmp[nx][newposY][0] = SMP.currentChannel;
          tmp[nx][newposY][1] = note[nx][ny][1];
        }
      }
    }
    shifted = true;
  }


  if (shifted) {
    // Step 3: Copy original notes of other channels back to the note array
    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (original[nx][ny][0] != SMP.currentChannel) {
          note[nx][ny][0] = original[nx][ny][0];
          note[nx][ny][1] = original[nx][ny][1];
        } else {
          note[nx][ny][0] = 0;
          note[nx][ny][1] = defaultVelocity;
        }
        if (tmp[nx][ny][0] == SMP.currentChannel) {
          note[nx][ny][0] = tmp[nx][ny][0];
          note[nx][ny][1] = tmp[nx][ny][1];
        }
      }
    }
    shifted = false;
  }
}


void tmpMuteAll(bool pressed) {
  if (pressed > 0) {
    //mute all channels except current
    for (unsigned int i = 1; i < maxFiles + 1; i++) {
      if (i != SMP.y - 1) {
        SMP.mute[i] = true;
      }else{
        SMP.mute[i] = false;
      }
    }
  } else {
    //unmute all channels
    tmpMute = false;
    for (unsigned int i = 1; i < maxFiles + 1; i++) {
      SMP.mute[i] = false;
    }
  }
}


void toggleMute() {

  if (SMP.mute[SMP.y - 1]) {
    SMP.mute[SMP.y - 1] = false;
    //envelopes[SMP.y - 1]->release(11880 / 2);
  } else {
    // wenn leer oder nicht gemuted:
    SMP.mute[SMP.y - 1] = true;
    // envelopes[SMP.y - 1]->release(120);
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
    pagebeat = 1;
    SMP.page = 1;
  }
  playStartTime = millis();  // Record the current time
  isPlaying = true;
}

void pause() {
  isPlaying = false;
  updateLastPage();
  deleteActiveCopy();
  autoSave();
  envelope0.noteOff();
  allOff();
  beat = 1;
  pagebeat = 1;
  SMP.page = 1;
}

void togglePlay(bool &value) {
  updateLastPage();
  deleteActiveCopy();
  value = !value;  // Toggle the boolean value
  Serial.println(value ? "Playing" : "Paused");

  if (value == false) {
    autoSave();
    envelope0.noteOff();
    allOff();
  }

  beat = 1;
  pagebeat = 1;
  SMP.page = 1;
}


void playNote() {
  playTimer.end();  // Stop the timer
  if (isPlaying) {
    for (unsigned int b = 1; b < maxY + 1; b++) {
      if (beat < maxlen && b < maxY + 1 && note[beat][b][0] > 0 && !SMP.mute[note[beat][b][0]]) {
        if (note[beat][b][0] < 9) {
          _samplers[note[beat][b][0]].noteEvent(12 * SampleRate[note[beat][b][0]] + b - (note[beat][b][0] + 1), note[beat][b][1], true, false);
          // usbMIDI.sendNoteOn(b, note[beat][b][1], 1);
        }

        if (note[beat][b][0] == 14) {
          float frequency = pianoFrequencies[b] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
          sound14.frequency(frequency);
          float WaveFormVelocity = mapf(note[beat][b][1], 1, 127, 0.0, 1.0);
          sound14.amplitude(WaveFormVelocity);
          envelope14.noteOn();
          startTime = millis();    // Record the start time
          noteOnTriggered = true;  // Set the flag so we don't trigger noteOn again
        }

        if (note[beat][b][0] == 13) {
          float frequency = pianoFrequencies[b - 1] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
          sound13.frequency(frequency);
          float WaveFormVelocity = mapf(note[beat][b][1], 1, 127, 0.0, 1.0);
          sound13.amplitude(WaveFormVelocity);
          envelope13.noteOn();
          startTime = millis();    // Record the start time
          noteOnTriggered = true;  // Set the flag so we don't trigger noteOn again
        }
      }
    }
    yield();
    beat++;
    // pagebeat is always 1-16, calcs from beat. if beat 1-16, pagebeat is 1, if beat 17-32, pagebeat is 2, etc.
    pagebeat = (beat - 1) % maxX + 1;
    yield();

    // midi functions
    if (waitForFourBars && pulseCount >= totalPulsesToWait) {
      beat = 1;
      pagebeat = 1;
      SMP.page = 1;
      isPlaying = true;
      Serial.println("4 Bars Reached");
      waitForFourBars = false;  // Reset for the next start message
    }

    if (beat > SMP.page * maxX) {
      SMP.page = SMP.page + 1;
      if (SMP.page > maxPages)
        SMP.page = 1;
      if (SMP.page > lastPage)
        SMP.page = 1;
    }

    if (beat > maxX * lastPage) {
      beat = 1;
      SMP.page = 1;
      pagebeat = 1;
    }
  }
  //Draw Things
  playTimer.begin(playNote, playNoteInterval);
  yield();
}


void unpaint() {
  //SMP.edit = 1;
  paintMode = false;
  unsigned int y = SMP.y;
  unsigned int x = (SMP.edit - 1) * maxX + SMP.x;
  if (!SMP.singleMode) {
    Serial.println("deleting voice:" + String(x));
    note[x][y][0] = 0;
  } else {
    if (note[x][y][0] == SMP.currentChannel)
      note[x][y][0] = 0;
  }
  updateLastPage();
  FastLEDshow();
}


void paint() {
  //SMP.edit = 1;
  unsigned int sample = 1;
  unsigned int x = (SMP.edit - 1) * maxX + SMP.x;
  unsigned int y = SMP.y;

  if (!SMP.singleMode) {
    if ((y > 1 && y <= maxFiles + 1) || y >= maxY - 2) {
      if (note[x][y][0] == 0) {
        note[x][y][0] = (y - 1);
      } 
      
      /* // IF NOTE EXISTS
      else {
        if ((note[x][y][0] + 1) > 8) note[x][y][0] = 0;  // do not cycle over the synth voices

        note[x][y][0] = note[x][y][0] + sample;
        for (unsigned int vx = 1; vx < maxX + 1; vx++) {
          light(vx, note[x][y][0] + 1, col[note[x][y][0]] * 12);
          FastLED.show();
          //FastLED.delay(1);
        }
      }
      */
    }
  } else {
    note[x][y][0] = SMP.currentChannel;
  }

  if (note[x][y][0] > maxY - 2) {
    note[x][y][0] = 1;
    for (unsigned int vx = 1; vx < maxX + 1; vx++) {
      light(vx, note[x][y][0] + 1, col[note[x][y][0]] * 12);
    }
    FastLEDshow();
  }

  if (!isPlaying) {
    if (note[x][y][0] < 9) {
      _samplers[note[x][y][0]].noteEvent(12 * SampleRate[note[x][y][0]] + y - (note[x][y][0] + 1), defaultVelocity, true, false);
      yield();
    }

    if (note[x][y][0] == 14) {
      float frequency = pianoFrequencies[y] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
      sound14.frequency(frequency);
      float WaveFormVelocity = mapf(defaultVelocity, 1, 127, 0.0, 1.0);
      sound14.amplitude(WaveFormVelocity);
      envelope14.noteOn();
      startTime = millis();    // Record the start time
      noteOnTriggered = true;  // Set the flag so we don't trigger noteOn again
    }

    if (note[x][y][0] == 13) {
      float frequency = pianoFrequencies[y - 1] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
      sound13.frequency(frequency);
      float WaveFormVelocity = mapf(defaultVelocity, 1, 127, 0.0, 1.0);
      sound13.amplitude(WaveFormVelocity);
      envelope13.noteOn();
      startTime = millis();    // Record the start time
      noteOnTriggered = true;  // Set the flag so we don't trigger noteOn again
    }
  }

  updateLastPage();
  FastLEDshow();
}


void light(unsigned int x, unsigned int y, CRGB color) {
  unsigned int index = 0;

  if (y > 0 && y < 17 && x > 0 && x < 17) {
    if (y > maxY) y = 1;
    if (y % 2 == 0) {
      index = (maxX - x) + (maxX * (y - 1));

    } else {
      index = (x - 1) + (maxX * (y - 1));
    }
    if (index < 256) { leds[index] = color; }
  }


  yield();
}







void displaySample(unsigned int len) {
  unsigned int length = mapf(len, 0, 1329920, 1, maxX);
  unsigned int skip = mapf(SMP.seek * 200, 44, len, 1, maxX);

  for (unsigned int s = 1; s <= maxX; s++) {
    light(s, 5, CRGB(1, 1, 1));
  }

  for (unsigned int s = 1; s <= length; s++) {
    light(s, 5, CRGB(20, 20, 20));
  }

  for (unsigned int s = 1; s <= maxX; s++) {
    light(s, 4, CRGB(4, 0, 0));
  }

  for (unsigned int s = 1; s <= skip; s++) {
    light(s, 4, CRGB(0, 4, 0));
  }
 FastLED.setBrightness(ledBrightness);
  FastLED.show();
}



void preloadSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength) {

  char OUTPUTf[50];
  int plen = 0;
  int previewsample = ((folder)*100) + sampleID;
  sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, previewsample);
  Serial.println(OUTPUTf);
  File previewSample = SD.open(OUTPUTf);
  SMP.smplen = 0;

  if (previewSample) {
    int fileSize = previewSample.size();
    previewSample.seek(24);
    for (uint8_t i = 24; i < 25; i++) {
      int g = previewSample.read();
      if (g == 72)
        PrevSampleRate = 4;
      if (g == 68)
        PrevSampleRate = 3;
      if (g == 34)
        PrevSampleRate = 2;
      if (g == 17)
        PrevSampleRate = 1;
      if (g == 0)
        PrevSampleRate = 4;
    }

    int startOffset = 200 * SMP.seek;   // Start offset in milliseconds
    int endOffset = 200 * SMP.seekEnd;  // End offset in milliseconds

    if (setMaxSampleLength == true) {
      endOffset = fileSize;
    }

    int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)
    int endOffsetBytes = endOffset * PrevSampleRate * 2;      // Convert to bytes (assuming 16-bit samples)

    // Adjust endOffsetBytes to avoid reading past the file end
    endOffsetBytes = min(endOffsetBytes, fileSize - 44);

    previewSample.seek(44 + startOffsetBytes);
    //memset(sampled[0], 0, sizeof(sample_len[0]));
    // o1:
    memset(sampled[0], 0, sizeof(sampled[0]));

    plen = 0;

    while (previewSample.available() && (plen < (endOffsetBytes - startOffsetBytes))) {
      int b = previewSample.read();
      sampled[0][plen] = b;
      plen++;
      if (plen >= sizeof(sampled[0])) break;  // Prevent buffer overflow
    }

    sampleIsLoaded = true;
    SMP.smplen = plen;

    // only set the first time to get seekEnd
    if (setMaxSampleLength == true) {
      Serial.print("before:");
      Serial.println(currentMode->maxValues[3]);
      Serial.println(currentMode->pos[2]);
      Serial.print("SET SAMPLELEN:");
      Serial.print(SMP.smplen / (PrevSampleRate * 2) / 200);

      sampleLengthSet = true;
      SMP.seekEnd = (SMP.smplen / (PrevSampleRate * 2) / 200);

      currentMode->pos[2] = SMP.seekEnd;
      //encoders[3].write(SMP.seekEnd * 4);
      Encoder[2].writeCounter((int32_t)((SMP.seekEnd)));
    }

    previewSample.close();
    displaySample(SMP.smplen);
  }
}

void preview(unsigned int PrevSampleRate, unsigned int plen) {
  // envelopes[0]->release(0);
  _samplers[0].removeAllSamples();
  envelope0.noteOff();
  _samplers[0].addSample(36, (int16_t *)sampled[0], (int)(plen / 2) - 44, 1);
  _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
}

void previewSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength, bool firstPreview) {
  // envelopes[0]->release(0);
  _samplers[0].removeAllSamples();
  envelope0.noteOff();

  char OUTPUTf[50];
  int plen = 0;

  sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, sampleID);
  Serial.println(OUTPUTf);
  File previewSample = SD.open(OUTPUTf);
  SMP.smplen = 0;


  if (previewSample) {
    int fileSize = previewSample.size();

    if (firstPreview) {
      fileSize = min(previewSample.size(), 300000);  // max preview len =  X Sec.
                                                     //toDO: make it better to preview long files
    }
    previewSample.seek(24);
    for (uint8_t i = 24; i < 25; i++) {
      int g = previewSample.read();
      if (g == 72)
        PrevSampleRate = 4;
      if (g == 68)
        PrevSampleRate = 3;
      if (g == 34)
        PrevSampleRate = 2;
      if (g == 17)
        PrevSampleRate = 1;
      if (g == 0)
        PrevSampleRate = 4;
    }

    int startOffset = 200 * SMP.seek;   // Start offset in milliseconds
    int endOffset = 200 * SMP.seekEnd;  // End offset in milliseconds

    if (setMaxSampleLength == true) {
      endOffset = fileSize;
    }

    int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)
    int endOffsetBytes = endOffset * PrevSampleRate * 2;      // Convert to bytes (assuming 16-bit samples)

    // Adjust endOffsetBytes to avoid reading past the file end
    endOffsetBytes = min(endOffsetBytes, fileSize - 44);

    previewSample.seek(44 + startOffsetBytes);
    //memset(sampled[0], 0, sizeof(sample_len[0]));
    //o1
    memset(sampled[0], 0, sizeof(sampled[0]));
    plen = 0;

    while (previewSample.available() && (plen < (endOffsetBytes - startOffsetBytes))) {
      int b = previewSample.read();
      sampled[0][plen] = b;
      plen++;
      if (plen >= sizeof(sampled[0])) break;  // Prevent buffer overflow
    }

    sampleIsLoaded = true;
    SMP.smplen = plen;

    // only set the first time to get seekEnd
    if (setMaxSampleLength == true) {
      Serial.print("before:");
      Serial.println(currentMode->maxValues[3]);
      Serial.println(currentMode->pos[2]);
      Serial.print("SET SAMPLELEN:");
      Serial.print(SMP.smplen / (PrevSampleRate * 2) / 200);

      sampleLengthSet = true;
      SMP.seekEnd = (SMP.smplen / (PrevSampleRate * 2) / 200);

      currentMode->pos[2] = SMP.seekEnd;
      //encoders[3].write(SMP.seekEnd * 4);
      Encoder[2].writeCounter((int32_t)(SMP.seekEnd));
    }

    previewSample.close();
    displaySample(SMP.smplen);

    _samplers[0].addSample(36, (int16_t *)sampled[0], (int)(plen / 2) - 44, 1);

    Serial.println("NOTE");
    _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
  }
}


void drawLoadingBar(int minval, int maxval, int currentval, CRGB color, CRGB fontColor) {
  int ypos = 3;

  int barwidth = mapf(currentval, minval, maxval, 0, maxX);
  for (int x = 1; x <= maxX; x++) {
    light(x, ypos - 1, CRGB(5, 5, 5));
    // light(x, ypos+2, CRGB(5, 5, 5));
  }
  //draw the border-ends
  light(1, ypos, CRGB(5, 5, 5));
  //light(1, ypos+1, CRGB(5, 5, 5));
  light(maxX, ypos, CRGB(5, 5, 5));
  //light(maxX, ypos+1, CRGB(5, 5, 5));

  for (int x = 2; x < maxX; x++) {
    for (int y = 0; y <= 1; y++) {
      if (x < barwidth) {
        light(x, ypos + y, color);
      } else {
        light(x, ypos + y, CRGB(0, 0, 0));
      }
    }
  }
  showNumber(currentval, fontColor, 0);
}



void loadSample(unsigned int packID, unsigned int sampleID) {
  Serial.print("loading");
  Serial.println(packID);
  drawNoSD();

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", packID, sampleID);

  if (packID == 0) {
    // SingleTrack from Samples-Folder
    sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(sampleID), sampleID);
    sampleID = SMP.currentChannel;
  }

  if (!SD.exists(OUTPUTf)) {
    Serial.print("File does not exist: ");
    Serial.println(OUTPUTf);
    // mute the channel
    SMP.mute[sampleID] = true;
    return;
  } else {
    //unmute the channel
    SMP.mute[sampleID] = false;
  }

  usedFiles[sampleID - 1] = OUTPUTf;

  File loadSample = SD.open(OUTPUTf);
  if (loadSample) {
    int fileSize = loadSample.size();
    loadSample.seek(24);
    for (uint8_t i = 24; i < 25; i++) {
      int g = loadSample.read();
      if (g == 0)
        SampleRate[sampleID] = 4;
      if (g == 17)
        SampleRate[sampleID] = 1;
      if (g == 34)
        SampleRate[sampleID] = 2;
      if (g == 68)
        SampleRate[sampleID] = 3;
      if (g == 72)
        SampleRate[sampleID] = 4;
    }

    SMP.seek = 0;

    unsigned int startOffset = 1 + (200 * SMP.seek);                   // Start offset in milliseconds
    unsigned int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)

    unsigned int endOffset = 200 * SMP.seekEnd;  // End offset in milliseconds
    if (SMP.seekEnd == 0) {
      // If seekEnd is not set, default to the full length of the sample
      endOffset = fileSize;
    }
    unsigned int endOffsetBytes = endOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)
    // Adjust endOffsetBytes to avoid reading past the file end
    endOffsetBytes = min(endOffsetBytes, fileSize - 44);

    loadSample.seek(44 + startOffsetBytes);
    unsigned int i = 0;
    //memset(sampled[sampleID], 0, sizeof(sample_len[sampleID]));
    //o1
    memset(sampled[sampleID], 0, sizeof(sampled[sampleID]));  // FIXED memset


    while (loadSample.available() && (i < (endOffsetBytes - startOffsetBytes))) {
      int b = loadSample.read();
      sampled[sampleID][i] = b;
      i++;
      if (i >= sizeof(sampled[sampleID])) break;  // Prevent buffer overflow
    }
    loadSample.close();

    i = i / 2;
    _samplers[sampleID].removeAllSamples();
    _samplers[sampleID].addSample(36, (int16_t *)sampled[sampleID], (int)i - 44, 1);
  }
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
        for (unsigned int b = 0; b < 2; b++) {
          tmp[src][y][b] = note[c][y][b];
        }
      }
    }
  } else {
    // paste the memory into the song
    Serial.print("paste here!");
    unsigned int src = 0;
    for (unsigned int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {
      src++;
      for (unsigned int y = 1; y < maxY + 1; y++) {
        for (unsigned int b = 0; b < 2; b++) {
          note[c][y][b] = tmp[src][y][b];
        }
      }
    }
  }
  updateLastPage();
  SMP.activeCopy = !SMP.activeCopy;  // Toggle the boolean value
}

void clearNoteChannel(unsigned int c, unsigned int yStart, unsigned int yEnd, unsigned int channel, bool singleMode) {
  for (unsigned int y = yStart; y < yEnd; y++) {
    if (singleMode) {
      if (note[c][y][0] == channel)
        note[c][y][0] = 0;
      if (note[c][y][0] == channel)
        note[c][y][1] = defaultVelocity;
    } else {
      note[c][y][0] = 0;
      note[c][y][1] = defaultVelocity;
    }
  }
}

void clearPage() {
  //SMP.edit = 1;
  unsigned int start = (SMP.edit - 1) * maxX + 1;
  unsigned int end = start + maxX;
  unsigned int channel = SMP.currentChannel;
  bool singleMode = SMP.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();
}

void drawBPMScreen() {
  FastLEDclear();
  drawVolume(SMP.vol);
  drawBrightness();
  CRGB volColor = CRGB(SMP.vol * SMP.vol, 20 - SMP.vol, 0);
  showIcons("helper_bright", CRGB(255,255,255));
  showIcons("helper_vol", volColor);
  showIcons("helper_bpm", CRGB(0, 50, 120));
  showNumber(SMP.bpm, CRGB(0,50,120), -1);
}



void updateVolume() {
  SMP.vol = currentMode->pos[2];
  float vol = float(SMP.vol / 10.0);
  Serial.println("Vol: " + String(vol));
  //if (vol <= 1.0) sgtl5000_1.volume(vol);
  // setvol = true;
}

void updateBrightness() {
  ledBrightness = currentMode->pos[1];
  Serial.println("Brightness: " + String(ledBrightness));
}

void updateBPM() {
  // setvol = false;
  Serial.println("BPM: " + String(currentMode->pos[3]));
  SMP.bpm = currentMode->pos[3];
  playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
  playTimer.update(playNoteInterval);
  drawBPMScreen();
}

void setVolume() {
  drawBPMScreen();

if (currentMode->pos[1] != ledBrightness) {
    updateBrightness();
  }

  if (currentMode->pos[2] != SMP.vol) {
    updateVolume();
  }

 
  if (currentMode->pos[3] != SMP.bpm) {
    updateBPM();
  }
}

CRGB getCol(unsigned int g) {
  return col[g] * 10;
}

void drawVolume(unsigned int vol) {
  unsigned int maxXVolume = int(vol * 1.3) + 2;
  for (unsigned int y = 5; y <= 6; y++) {
    for (unsigned int x = 0; x <= maxXVolume; x++) {
      light(x + 1, y, CRGB(vol * vol, 20 - vol, 0));
    }
  }
}

void drawBrightness( ) {
  unsigned int maxBrightness = ((ledBrightness - 65) * (15 - 1)) / (255 - 65) + 1;
    for (unsigned int x = 0; x <= maxBrightness; x++) {
      CRGB brightness = CRGB(16*x,16*x,16*x);
      light(x, 4, brightness);
    }
  }

void changeMenu(int newMenuPosition){
  menuPosition = newMenuPosition;
  
}


void showMenu(){
  FastLEDclear();
  showIcons("icon_loadsave", CRGB(10, 5, 0));
  showNumber(menuPosition, CRGB(20, 20, 40), 0);


  if (currentMode->pos[3] != menuPosition) {
    changeMenu(currentMode->pos[3]);
    FastLED.setBrightness(ledBrightness);  
    FastLED.show();
  }




}

void showLoadSave() {

  drawNoSD();
  FastLEDclear();

  showIcons("icon_loadsave", CRGB(10, 5, 0));
  showIcons("helper_select", CRGB(0, 0, 5));

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d.txt", SMP.file);
  if (SD.exists(OUTPUTf)) {
    showIcons("helper_save", CRGB(1, 0, 0));
    showIcons("helper_load", CRGB(0, 20, 0));
    showNumber(SMP.file, CRGB(0, 0, 20), 0);
  } else {
    showIcons("helper_save", CRGB(20, 0, 0));
    showIcons("helper_load", CRGB(0, 1, 0));
    showNumber(SMP.file, CRGB(20, 20, 40), 0);
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

  showIcons("icon_samplepack", CRGB(10, 10, 0));
  showIcons("helper_select", CRGB(0, 0, 5));
  showNumber(SMP.pack, CRGB(20, 0, 0), 0);

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", SMP.pack, 1);
  if (SD.exists(OUTPUTf)) {
    showIcons("helper_load", CRGB(0, 20, 0));
    showIcons("helper_save", CRGB(3, 0, 0));
    showNumber(SMP.pack, CRGB(0, 20, 0), 0);
  } else {
    showIcons("helper_load", CRGB(0, 3, 0));
    showIcons("helper_save", CRGB(20, 0, 0));
    showNumber(SMP.pack, CRGB(20, 0, 0), 0);
  }
   FastLED.setBrightness(ledBrightness);
  FastLED.show();
  if (currentMode->pos[3] != SMP.pack) {
    Serial.println("File: " + String(currentMode->pos[3]));
    SMP.pack = currentMode->pos[3];
  }
}

void loadSamplePack(unsigned int pack) {
  Serial.println("Loading SamplePack #" + String(pack));
  drawNoSD();
  FastLEDclear();
  SMP.smplen = 0;
  SMP.seekEnd = 0;
  EEPROM.put(0, pack);

  for (unsigned int z = 1; z < maxFiles; z++) {
    FastLEDclear();
    showIcons("icon_sample", CRGB(20, 20, 20));
    drawLoadingBar(1, maxFiles, z, col[z], CRGB(15, 55, 15));
    loadSample(pack, z);
  }
  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", pack, 1);


  switchMode(&draw);
}

void saveSamplePack(unsigned int pack) {
  Serial.println("Saving SamplePack in #" + String(pack));
  FastLEDclear();
  char OUTPUTdir[50];
  sprintf(OUTPUTdir, "%d/", pack);
  SD.mkdir(OUTPUTdir);
  delay(500);
  for (unsigned int i = 0; i < sizeof(usedFiles) / sizeof(usedFiles[0]); i++) {
    for (unsigned int f = 1; f < (maxY / 2) + 1; f++) {
      light(i + 1, f, CRGB(4, 0, 0));
    }
    showIcons("icon_samplepack", CRGB(20, 20, 20));
    FastLED.setBrightness(ledBrightness);
    FastLED.show();

    if (SD.exists(usedFiles[i].c_str())) {
      File saveFilePack = SD.open(usedFiles[i].c_str());
      char OUTPUTf[50];
      sprintf(OUTPUTf, "%d/%d.wav", pack, i + 1);

      if (SD.exists(OUTPUTf)) {
        SD.remove(OUTPUTf);
        delay(100);
      }

      File myDestFile = SD.open(OUTPUTf, FILE_WRITE);

      size_t n;
      uint8_t buf[512];
      while ((n = saveFilePack.read(buf, sizeof(buf))) > 0) {
        myDestFile.write(buf, n);
      }
      myDestFile.close();
      saveFilePack.close();
    }

    for (unsigned int f = 1; f < (maxY + 1) + 1; f++) {
      light(i + 1, f, CRGB(0, 20, 0));
    }
    FastLEDshow();
  }
  switchMode(&draw);
}



int getFolderNumber(int value) {
  int folder = floor(value / 100);
  if (folder > FOLDER_MAX) folder = FOLDER_MAX;
  if (folder <= 0) folder = 0;
  return folder;
}

int getFileNumber(int value) {
  // return the file number in the folder, so 113 = 13, 423 = 23
  int folder = getFolderNumber(value);
  int wavfile = value % 100;
  if (wavfile <= 0) wavfile = 0;
  return wavfile + folder * 100;
}


void showWave() {

  File sampleFile;
  drawNoSD();

  FastLEDclear();
  if (SMP.wav[SMP.currentChannel][1] < 100)
    showIcons("icon_sample", col[SMP.y - 1]);
  showIcons("helper_select", col[SMP.y - 1]);
  showIcons("helper_load", CRGB(0, 20, 0));
  showIcons("helper_seek", CRGB(10, 0, 0));
  showIcons("helper_folder", CRGB(10, 10, 0));
  showNumber(SMP.wav[SMP.currentChannel][1], col_Folder[getFolderNumber(SMP.wav[SMP.currentChannel][1])], 0);
  displaySample(SMP.smplen);



  if (currentMode->pos[1] != SMP.folder) {
    //change FOLDER
    SMP.folder = currentMode->pos[1];
    Serial.println("Folder: " + String(SMP.folder - 1));
    SMP.wav[SMP.currentChannel][1] = ((SMP.folder - 1) * 100);
    Serial.println("wav: " + String(SMP.wav[SMP.currentChannel][1]));
    Encoder[1].writeCounter((int32_t)((SMP.wav[SMP.currentChannel][1]) - 1));
  }

  // ENDPOSITION SAMPLE

  if ((currentMode->pos[2]) != SMP.seekEnd && sampleIsLoaded) {
    SMP.seekEnd = currentMode->pos[2];
    Serial.println("seekEnd:");
    Serial.println(SMP.seekEnd);
    _samplers[0].removeAllSamples();
    envelope0.noteOff();
    if (sampleFile) {
      sampleFile.seek(0);
    }

    char OUTPUTf[50];

    sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(SMP.wav[SMP.currentChannel][1]), getFileNumber(SMP.wav[SMP.currentChannel][1]));

    if (SD.exists(OUTPUTf)) {
      showIcons("helper_select", col[SMP.y - 1]);
      showIcons("helper_load", CRGB(0, 20, 0));
      showIcons("helper_seek", CRGB(10, 0, 0));
      showIcons("helper_folder", CRGB(10, 30, 0));
      showNumber(SMP.wav[SMP.currentChannel][1], col_Folder[getFolderNumber(SMP.wav[SMP.currentChannel][1])], 0);
      if (!sampleLengthSet) previewSample(getFolderNumber(SMP.wav[SMP.currentChannel][1]), getFileNumber(SMP.wav[SMP.currentChannel][1]), false, false);
    } else {
      showIcons("helper_select", col[SMP.y - 1]);
      showIcons("helper_load", CRGB(0, 0, 0));
      showIcons("helper_folder", CRGB(10, 10, 0));
      showNumber(SMP.wav[SMP.currentChannel][1], col_Folder[getFolderNumber(SMP.wav[SMP.currentChannel][1])], 0);
    }

    sampleLengthSet = false;
  }


  // STARTPOSITION SAMPLE
  if ((currentMode->pos[0]) - 1 != SMP.seek && sampleIsLoaded) {
    SMP.seek = currentMode->pos[0] - 1;
    Serial.println(SMP.seek);
    _samplers[0].removeAllSamples();
    envelope0.noteOff();
    if (sampleFile) {
      sampleFile.seek(0);
    }

    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(SMP.wav[SMP.currentChannel][1]), getFileNumber(SMP.wav[SMP.currentChannel][1]));
    if (SD.exists(OUTPUTf)) {
      showIcons("helper_select", col[SMP.y - 1]);
      showIcons("helper_load", CRGB(0, 20, 0));
      showIcons("helper_folder", CRGB(10, 30, 0));
      showNumber(SMP.wav[SMP.currentChannel][1], col_Folder[getFolderNumber(SMP.wav[SMP.currentChannel][1])], 0);
      previewSample(getFolderNumber(SMP.wav[SMP.currentChannel][1]), getFileNumber(SMP.wav[SMP.currentChannel][1]), false, false);
    } else {
      showIcons("helper_select", col[SMP.y - 1]);
      showIcons("helper_load", CRGB(0, 0, 0));
      showIcons("helper_folder", CRGB(10, 10, 0));
      showNumber(SMP.wav[SMP.currentChannel][1], col_Folder[getFolderNumber(SMP.wav[SMP.currentChannel][1])], 0);
    }
  }

  // SAMPLEFILE
  if (currentMode->pos[3] != SMP.wav[SMP.currentChannel][1]) {

    sampleIsLoaded = false;
    SMP.wav[SMP.currentChannel][1] = currentMode->pos[3];
    Serial.println("File: " + String(getFolderNumber(SMP.wav[SMP.currentChannel][1])) + " / " + String(getFileNumber(SMP.wav[SMP.currentChannel][1])));

    // reset SEEK and stop sample playing
    SMP.smplen = 0;
    currentMode->pos[0] = 1;
    //encoders[0].write(1);
    Encoder[0].writeCounter((int32_t)1);
    SMP.seek = 0;
    _samplers[0].removeAllSamples();
    envelope0.noteOff();
    if (sampleFile) {
      sampleFile.seek(0);
    }
    if (SMP.wav[SMP.currentChannel][1] < 100) showIcons("icon_sample", col[SMP.y - 1]);

    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(SMP.wav[SMP.currentChannel][1]), getFileNumber(SMP.wav[SMP.currentChannel][1]));
    Serial.println("------");
    Serial.println(OUTPUTf);

    // if (SD.exists(OUTPUTf)) {



    if (SMP.wav[SMP.currentChannel][1] < getFolderNumber(SMP.wav[SMP.currentChannel][1] + 1) * 100) {
      Serial.print("exceeded first number of folder ");
      Serial.println(getFolderNumber(SMP.wav[SMP.currentChannel][1] + 1));
      SMP.wav[SMP.currentChannel][1] = lastFile[getFolderNumber(SMP.wav[SMP.currentChannel][1])];
      SMP.folder = getFolderNumber(SMP.wav[SMP.currentChannel][1]);
      //write encoder
      Encoder[3].writeCounter((int32_t)(SMP.wav[SMP.currentChannel][1]) - 1);
      Encoder[1].writeCounter((int32_t)(SMP.folder) - 1);
    }


    if (lastPreviewedSample[getFolderNumber(SMP.wav[SMP.currentChannel][1])] < SMP.wav[SMP.currentChannel][1]) {
      if (SMP.wav[SMP.currentChannel][1] > lastFile[getFolderNumber(SMP.wav[SMP.currentChannel][1])]) {
        Serial.print("exceeding last file number of folder ");
        Serial.println(getFolderNumber(SMP.wav[SMP.currentChannel][1]));
        SMP.wav[SMP.currentChannel][1] = ((getFolderNumber(SMP.wav[SMP.currentChannel][1]) + 1) * 100);
        SMP.folder = getFolderNumber(SMP.wav[SMP.currentChannel][1] + 1);
        //write encoder
        Encoder[3].writeCounter((int32_t)(SMP.wav[SMP.currentChannel][1]) - 1);
        Encoder[1].writeCounter((int32_t)(SMP.folder) - 1);
      }
    }

    lastPreviewedSample[getFolderNumber(SMP.wav[SMP.currentChannel][1])] = SMP.wav[SMP.currentChannel][1];
    showIcons("helper_select", col[SMP.y - 1]);
    showIcons("helper_load", CRGB(0, 20, 0));
    showIcons("helper_folder", CRGB(10, 30, 0));
    showNumber(SMP.wav[SMP.currentChannel][1], col_Folder[getFolderNumber(SMP.wav[SMP.currentChannel][1])], 0);
    previewSample(getFolderNumber(SMP.wav[SMP.currentChannel][1]), getFileNumber(SMP.wav[SMP.currentChannel][1]), true, true);
    sampleIsLoaded = true;
  }
}

void showIntro() {
  FastLED.clear();
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
  for (int gx = 0; gx < 102; gx++) {
    light(logo[gx][0], maxY - logo[gx][1], CRGB(150, 150, 150));
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
    delay(20);
  }
  delay(200);

  for (int fade = 0; fade < 10 + 1; fade++) {
    for (int u = 0; u < NUM_LEDS; u++) {
      leds[u] = leds[u].fadeToBlackBy(fade * 10);
    }
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
    delay(50);
  }

  int bright = 100;
  for (int y = -15; y < 3; y++) {
    FastLED.clear();

    showNumber(101, CRGB(0, 0, 15), y - 2);
    showNumber(101, CRGB(10, 5, 0), y - 1);
    showNumber(101, CRGB(15, 0, 0), y);
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
    delay(50);
  }
  delay(200);
  for (int y = 3; y < 16; y++) {
    FastLED.clear();
    showNumber(101, CRGB(0, 0, 15), y - 2);
    showNumber(101, CRGB(10, 5, 0), y - 1);
    showNumber(101, CRGB(15, 0, 0), y);
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
    delay(50);
  }
  FastLED.clear();
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
}

void showNumber(unsigned int count, CRGB color, int topY) {
  if (!color)
    color = CRGB(200, 200, 200);
  char buf[4];
  sprintf(buf, "%03i", count);
  unsigned int stelle2 = buf[0] - '0';
  unsigned int stelle1 = buf[1] - '0';
  unsigned int stelle0 = buf[2] - '0';

  unsigned int ypos = maxY - topY;
  for (unsigned int gx = 0; gx < 24; gx++) {
    if (stelle2 > 0)
      light(1 + number[stelle2][gx][0], ypos - number[stelle2][gx][1], color);
    if ((stelle1 > 0 || stelle2 > 0))
      light(6 + number[stelle1][gx][0], ypos - number[stelle1][gx][1], color);
    light(11 + number[stelle0][gx][0], ypos - number[stelle0][gx][1], color);
  }
  FastLEDshow();
}

void drawVelocity(CRGB color) {
  FastLEDclear();
  unsigned int vy = currentMode->pos[3];
  if (!SMP.singleMode) {
    for (unsigned int y = 1; y < vy + 1; y++) {
      light(SMP.x, y, CRGB(y * y, 20 - y, 0));
    }
  } else {
    Serial.println("single");
    for (unsigned int x = 1; x < maxX + 1; x++) {
      for (unsigned int y = 1; y < vy + 1; y++) {
        light(x, y, CRGB(y * y, 20 - y, 0));
      }
    }
  }
}

void drawBase() {
  if (!SMP.singleMode) {
    unsigned int colors = 0;
    for (unsigned int y = 1; y < maxY; y++) {
      //unsigned int filtering = 2;  // mapf(SMP.filter_knob[y - 1], 0, maxfilterResolution, 50, 5);
      for (unsigned int x = 1; x < maxX + 1; x++) {
        if (SMP.mute[y - 1]) {
          light(x, y, CRGB(0, 0, 0));
        } else {
          light(x, y, col_base[colors]);
        }
      }
      colors++;
    }
    for (unsigned int x = 1; x <= 13; x += 4) {
      light(x, 1, CRGB(10, 10, 10));  //4-4 helper
    }
  } else {
    unsigned int currentChannel = SMP.currentChannel;
    bool isMuted = SMP.mute[currentChannel];
    
    for (unsigned int y = 1; y < maxY; y++) {
      for (unsigned int x = 1; x < maxX + 1; x++) {
        light(x, y, col_base[currentChannel]);
      }
    }
    for (unsigned int x = 1; x <= 13; x += 4) {
      light(x, 1, CRGB(0, 1, 1));  // türkis
    }
  }
  drawPages();
  drawStatus();
}

void drawStatus() {
  CRGB ledColor = CRGB(0, 0, 0);
  if (SMP.activeCopy)
    ledColor = CRGB(20, 20, 0);
  for (unsigned int s = 9; s <= maxX; s++) {
    light(s, maxY, ledColor);
  }

  if (currentMode == &noteShift) {
    // draw a moving marquee to indicate the note shift mode
    for (unsigned int x = 9; x <= maxX; x++) {
      light(x, maxY, CRGB(0, 0, 0));
    }
    light(round(marqueePos), maxY, CRGB(20, 20, 20));
    if (movingForward) {
      marqueePos = marqueePos + 0.1;
      if (marqueePos > maxX) {
        marqueePos = maxX;
        movingForward = false;
      }
    } else {
      marqueePos = marqueePos - 0.1;
      if (marqueePos < 9) {
        marqueePos = 9;
        movingForward = true;
      }
    }
  }
}

void updateLastPage() {
  lastPage = 1;  // Reset lastPage before starting

  for (unsigned int p = 1; p <= maxPages; p++) {
    bool pageHasNotes = false;

    for (unsigned int ix = 1; ix <= maxX; ix++) {
      for (unsigned int iy = 1; iy <= maxY; iy++) {
        if (note[((p - 1) * maxX) + ix][iy][0] > 0) {
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

void drawPages() {
  //SMP.edit = 1;
  CRGB ledColor;

  for (unsigned int p = 1; p <= maxPages; p++) {  // Assuming maxPages is 8

    // If the page is the current one, set the LED to white
    if (SMP.page == p && SMP.edit == p) {
      ledColor = isPlaying ? CRGB(20, 255, 20) : CRGB(50, 50, 50);
    } else if (SMP.page == p) {
      ledColor = isPlaying ? CRGB(0, 15, 0) : CRGB(0, 0, 35);
    } else {
      if (SMP.edit == p) {
        ledColor = SMP.page == p ? CRGB(50, 50, 50) : CRGB(20, 20, 20);
      } else {
        ledColor = hasNotes[p] ? CRGB(0, 0, 35) : CRGB(1, 0, 0);
      }
    }

    // Set the LED color
    light(p, maxY, ledColor);
  }

  // After the loop, update the maxValues for Page-select-knob
  currentMode->maxValues[1] = lastPage + 1;

  // Additional logic can be added here if needed
}

/************************************************
      DRAW SAMPLES
  *************************************************/
void drawTriggers() {
  // why?
  SMP.edit = 1;
  for (unsigned int ix = 1; ix < maxX + 1; ix++) {
    for (unsigned int iy = 1; iy < maxY + 1; iy++) {
      int thisNote = note[((SMP.edit - 1) * maxX) + ix][iy][0];
      if (thisNote > 0) {

        if (!SMP.mute[thisNote]) {
          //light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy][0]));
          light(ix, iy, col[thisNote]);

          // if there is any note of the same value in the same column, make it less bright
          for (unsigned int iy2 = 1; iy2 < maxY + 1; iy2++) {
            if (iy2 != iy && note[((SMP.edit - 1) * maxX) + ix][iy2][0] == note[((SMP.edit - 1) * maxX) + ix][iy][0]) {
              light(ix, iy2, col[thisNote]);
            }
          }
        } else {
          //light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy][0]) / 24);
          light(ix, iy, col_base[thisNote]);
        }
      }
    }
  }
}


/************************************************
      TIMER
  *************************************************/

void drawTimer(unsigned int timer) {
  if (SMP.page == SMP.edit) {
    for (unsigned int y = 1; y < maxY; y++) {
      light(timer, y, CRGB(10, 0, 0));

      if (note[((SMP.page - 1) * maxX) + timer][y][0] > 0) {
        if (SMP.mute[note[((SMP.page - 1) * maxX) + timer][y][0]] == 0) {
          light(timer, y, CRGB(200, 200, 200));
        } else {
          light(timer, y, CRGB(00, 00, 00));
        }
      }
    }
  }
}

/************************************************
      USER CURSOR
  *************************************************/
void drawCursor() {
  /* if (dir == 1)
    pulse = pulse + 1;
  if (dir == -1)
    pulse = pulse - 1;
  if (pulse > 220) {
    dir = -1;
  }
  if (pulse < 1) {
    dir = 1;
  }
*/
  light(SMP.x, SMP.y, CRGB(255 - (int)pulse, 255 - (int)pulse, 255 - (int)pulse));
  
}

void showIcons(String ico, CRGB colors) {
  const int(*iconArray)[2] = nullptr;  // Change to const int

  unsigned int size = 0;

  if (ico == "icon_samplepack") {
    iconArray = icon_samplepack;
    size = sizeof(icon_samplepack) / sizeof(icon_samplepack[0]);
  } else if (ico == "icon_sample") {
    iconArray = icon_sample;
    size = sizeof(icon_sample) / sizeof(icon_sample[0]);
  } else if (ico == "icon_loadsave") {
    iconArray = icon_loadsave;
    size = sizeof(icon_loadsave) / sizeof(icon_loadsave[0]);
  } else if (ico == "helper_load") {
    iconArray = helper_load;
    size = sizeof(helper_load) / sizeof(helper_load[0]);
  } else if (ico == "helper_seek") {
    iconArray = helper_seek;
    size = sizeof(helper_seek) / sizeof(helper_seek[0]);
  } else if (ico == "helper_folder") {
    iconArray = helper_folder;
    size = sizeof(helper_folder) / sizeof(helper_folder[0]);
  } else if (ico == "helper_save") {
    iconArray = helper_save;
    size = sizeof(helper_save) / sizeof(helper_save[0]);
  } else if (ico == "helper_select") {
    iconArray = helper_select;
    size = sizeof(helper_select) / sizeof(helper_select[0]);
  } else if (ico == "helper_vol") {
    iconArray = helper_vol;
    size = sizeof(helper_vol) / sizeof(helper_vol[0]);
  } else if (ico == "helper_bright") {
    iconArray = helper_bright;
    size = sizeof(helper_bright) / sizeof(helper_bright[0]);
  } else if (ico == "helper_bpm") {
    iconArray = helper_bpm;
    size = sizeof(helper_bpm) / sizeof(helper_bpm[0]);
  } else if (ico == "icon_bpm") {
    iconArray = icon_bpm;
    size = sizeof(icon_bpm) / sizeof(icon_bpm[0]);
  }

  if (iconArray != nullptr) {
    for (unsigned int gx = 0; gx < size; gx++) {
      light(iconArray[gx][0], maxY - iconArray[gx][1], colors);
    }
  }
}

void loadWav() {
  Serial.println("Loading Wave :" + String(SMP.wav[SMP.currentChannel][1]));
  loadSample(0, SMP.wav[SMP.currentChannel][1]);
  switchMode(&singleMode);
  SMP.singleMode = true;
}

void savePattern(bool autosave) {
  yield();
  Serial.println("Saving in slot #" + String(SMP.file));
  drawNoSD();
  FastLEDclear();
  unsigned int maxdata = 0;
  char OUTPUTf[50];
  // Save to autosave.txt if autosave is true
  if (autosave) {
    sprintf(OUTPUTf, "autosaved.txt");
  } else {
    sprintf(OUTPUTf, "%d.txt", SMP.file);
  }
  if (SD.exists(OUTPUTf)) {
    SD.remove(OUTPUTf);
  }
  File saveFile = SD.open(OUTPUTf, FILE_WRITE);
  if (saveFile) {
    // Save notes
    for (unsigned int sdx = 1; sdx < maxlen; sdx++) {
      for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
        maxdata = maxdata + note[sdx][sdy][0];
        saveFile.write(note[sdx][sdy][0]);
        saveFile.write(note[sdx][sdy][1]);
      }
    }
    // Use a unique marker to indicate the end of notes and start of SMP data
    saveFile.write(0xFF);  // Marker byte to indicate end of notes
    saveFile.write(0xFE);  // Marker byte to indicate start of SMP

    // Save SMP struct
    saveFile.write((uint8_t *)&SMP, sizeof(SMP));
  }
  saveFile.close();
  if (maxdata == 0) {
    SD.remove(OUTPUTf);
  }
  if (!autosave) {
    delay(500);
    switchMode(&draw);
  }
  yield();
}

void autoSave() {

  savePattern(true);
  yield();
}

void loadPattern(bool autoload) {
  yield();
  Serial.println("Loading slot #" + String(SMP.file));
  drawNoSD();
  FastLEDclear();
  char OUTPUTf[50];
  if (autoload) {
    sprintf(OUTPUTf, "autosaved.txt");
  } else {
    sprintf(OUTPUTf, "%d.txt", SMP.file);
  }
  if (SD.exists(OUTPUTf)) {
    // showNumber(SMP.file, CRGB(0, 0, 50), 0);
    File loadFile = SD.open(OUTPUTf);
    if (loadFile) {
      unsigned int sdry = 1;
      unsigned int sdrx = 1;

      while (loadFile.available()) {
        int b = loadFile.read();

        // Check for the marker indicating the end of notes
        if (b == 0xFF && loadFile.peek() == 0xFE) {
          loadFile.read();  // Consume the second marker byte
          break;            // Exit the loop to load SMP data
        }

        int v = loadFile.read();
        note[sdrx][sdry][0] = b;
        note[sdrx][sdry][1] = v;
        sdry++;
        if (sdry > maxY) {
          sdry = 1;
          sdrx++;
        }
        if (sdrx > maxlen)
          sdrx = 1;
      }

      // Load SMP struct after marker
      if (loadFile.available()) {
        loadFile.read((uint8_t *)&SMP, sizeof(SMP));
      }
    }
    loadFile.close();
    yield();
    Mode *bpm_vol = &volume_bpm;
    bpm_vol->pos[3] = SMP.bpm;
    playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
    playTimer.update(playNoteInterval);
    bpm_vol->pos[2] = SMP.vol;


    float vol = float(SMP.vol / 10.0);
    //if (vol <= 1.0) sgtl5000_1.volume(vol);
    // set all Filters
    //for (unsigned int i = 0; i < maxFilters; i++) {
    //filters[i]->frequency(100 * SMP.filter_knob[i]);
    //}
    SMP.singleMode = false;

    // Display the loaded SMP data
    Serial.println("Loaded SMP Data:");
    Serial.println("singleMode: " + String(SMP.singleMode));
    Serial.println("currentChannel: " + String(SMP.currentChannel));
    Serial.println("vol: " + String(SMP.vol));
    Serial.println("bpm: " + String(SMP.bpm));
    Serial.println("velocity: " + String(SMP.velocity));
    Serial.println("page: " + String(SMP.page));
    Serial.println("edit: " + String(SMP.edit));
    Serial.println("file: " + String(SMP.file));
    Serial.println("pack: " + String(SMP.pack));
    Serial.println("wav: " + String(SMP.wav[SMP.currentChannel][1]));
    Serial.println("folder: " + String(SMP.folder));
    Serial.println("activeCopy: " + String(SMP.activeCopy));
    Serial.println("x: " + String(SMP.x));
    Serial.println("y: " + String(SMP.y));
    Serial.println("seek: " + String(SMP.seek));
    Serial.println("seekEnd: " + String(SMP.seekEnd));
    Serial.println("smplen: " + String(SMP.smplen));
    Serial.println("shiftX: " + String(SMP.shiftX));


    SMP.seek = 0;
    SMP.seekEnd = 0;
    SMP.smplen = 0;
    SMP.shiftX = 0;
    SMP.shiftY = 0;

    Serial.println("filter_knob: ");
    for (unsigned int i = 0; i < maxFilters; i++) {
      Serial.print(SMP.filter_knob[i]);
      Serial.print(", ");
    }

    Serial.println("mute: ");
    for (unsigned int i = 0; i < maxY; i++) {
      Serial.print(SMP.mute[i]);
      Serial.print(", ");
    }
    Serial.println();
  } else {
    for (unsigned int nx = 1; nx < maxlen; nx++) {
      for (unsigned int ny = 1; ny < maxY + 1; ny++) {
        note[nx][ny][0] = 0;
        note[nx][ny][1] = defaultVelocity;
      }
    }
  }

  updateLastPage();

  if (!autoload) {
    delay(500);
    switchMode(&draw);
  }
}

void autoLoad() {
  loadPattern(true);
}



void handleStop() {
  // This function is called when a MIDI STOP message is received
  isPlaying = false;
  pulseCount = 0;
  AudioMemoryUsageMaxReset();
  deleteActiveCopy();
  envelope0.noteOff();
  allOff();
  autoSave();
  beat = 1;
  pagebeat = 1;
  SMP.page = 1;
  waitForFourBars = false;
  yield();
}


//receive Midi notes and velocity, map to note array. if not playing, play the note
void handleNoteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  // return if out of range
  if (SMP.y - 1 < 1 || SMP.y - 1 > maxFiles) return;

  //  envelopes[SMP.y - 1]->release(11880 / 2);
  unsigned int livenote = SMP.y + pitch - 60;  // set Base to C3

  // fake missing octaves (only 16 notes visible, so step up/down an octave!)
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;

  if (livenote >= 1 && livenote <= 16) {
    light(SMP.x, livenote, CRGB(0, 0, 255));
    FastLEDshow();
    _samplers[SMP.y - 1].noteEvent(((SampleRate[SMP.y - 1] * 12) + pitch - 60), velocity * 8, true, false);
    if (isPlaying) {
      if (!SMP.mute[SMP.y - 1]) {
        note[beat][livenote][0] = SMP.y - 1;
        note[beat][livenote][1] = velocity;
      }
    }
  }
}

void handleNoteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  /*if (isPlaying) {
    if (pitch > 0 && pitch < 17) {
      if (note[beat][pitch][0] == SMP.currentChannel) {
        note[beat][pitch][0] = 0;
        light(pitch, SMP.currentChannel + 1, CRGB(0, 0, 0));
        FastLEDshow();
      }
    }
  } else {
    if (pitch > 0 && pitch < 17) {
      if (note[beat][pitch][0] == SMP.curr  entChannel) {
        note[beat][pitch][0] = 0;
        light(pitch, SMP.currentChannel + 1, CRGB(0, 0, 0));
        FastLEDshow();
      }
    }
  }*/
}

void handleStart() {
  // This function is called when a MIDI Start message is received
  waitForFourBars = true;
  pulseCount = 0;  // Reset pulse count on start
  Serial.println("MIDI Start Received");
}


void handleTimeCodeQuarterFrame(uint8_t data) {
  // This function is called when a MIDI Start message is received
  Serial.println("MIDI TimeCodeQuarterFrame Received");
}



void handleSongPosition(uint16_t beats) {
  // This fuSerinction is called when a Song Position Pointer message is received
  Serial.print("Song Position Pointer Received: ");
  Serial.println(beats);
}


void myClock() {
  static unsigned int lastClockTime = 0;
  static unsigned int totalInterval = 0;
  static unsigned int clockCount = 0;

  unsigned int now = millis();
  if (lastClockTime > 0) {
    totalInterval += now - lastClockTime;
    clockCount++;

    if (clockCount >= numPulsesForAverage) {
      float averageInterval = totalInterval / (float)numPulsesForAverage;
      float bpm = 60000.0 / (averageInterval * 24);
      SMP.bpm = round(bpm);
      playTimer.update(((60 * 1000 / SMP.bpm) / 4) * 1000);
      clockCount = 0;
      totalInterval = 0;
    }
  }
  lastClockTime = now;
}
