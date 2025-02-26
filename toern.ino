//extern "C" char *sbrk(int incr);
#define FASTLED_ALLOW_INTERRUPTS 1
#define SERIAL8_RX_BUFFER_SIZE 256  // Increase to 256 bytes
#define SERIAL8_TX_BUFFER_SIZE 256  // Increase if needed for transmission
#define TargetFPS 30
#define SEARCHSIZE 100
//#define AUDIO_SAMPLE_RATE_EXACT 44117.64706
#include <stdint.h>
#include "font_3x5.h"
#include <Wire.h>
#include <i2cEncoderLibV2.h>
#include "Arduino.h"
#include <Mapf.h>
#include <WS2812Serial.h>  // leds
#define USE_WS2812SERIAL   // leds

#include <math.h>
#include "files.h"
#include <MIDI.h>
#include <FastLED.h>  // leds

#include <Audio.h>
#include <EEPROM.h>
#include "colors.h"
#include <FastTouch.h>
#include <TeensyPolyphony.h>
#include "audioinit.h"


#define maxX 16
#define maxY 16
#define INT_SD 10
#define NUM_LEDS 256
#define DATA_PIN 17  // PIN FOR LEDS
#define INT_PIN 27   // PIN FOR ENOCDER INTERRUPS
#define SWITCH_1 16  // Pin for TPP223 1
#define SWITCH_2 41  // Pin for TPP223 1
#define VOL_MIN 1
#define VOL_MAX 10
#define BPM_MIN 40
#define BPM_MAX 300
#define GAIN 0.25
#define NUM_ENCODERS 4
#define defaultVelocity 63
#define FOLDER_MAX 9
#define maxPages 16
#define maxFiles 9  // 9 samples, 2 Synths
#define maxFilters 15
#define maxfilterResolution 32
#define numPulsesForAverage 24  // Number of pulses to average over
#define pulsesPerBar (24 * 4)   // 24 pulses per quarter note, 4 quarter notes per bar

struct MySettings : public midi ::DefaultSettings {
  static const long BaudRate = 31250;
  static const unsigned SysExMaxSize = 256;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial8, MIDI, MySettings);
unsigned long beatStartTime = 0;  // Timestamp when the current beat started



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
bool particlesGenerated = false;
Particle particles[256];  // up to 256 possible "on" pixels
int particleCount = 0;

bool activeNotes[128] = { false };  // Track active MIDI notes (0-127)

char *menuText[5] = { "DAT", "KIT", "WAV", "REC", "SET" };
int lastFile[9] = { 0 };
bool freshPaint, tmpMute = false;
bool firstcheck = false;
bool nofile = false;
char *currentFilter = "ADSR";
unsigned int fxType = 0;
unsigned int selectedFX = 0;
unsigned int menuPosition = 1;
String oldPosString, posString = "1:2:";
String buttonString, oldButtonString = "0000";
unsigned long playStartTime = 0;  // To track when play(true) was last called

bool previewIsPlaying = false;

const int maxPeaks = 512;  // Adjust based on your needs
float peakValues[maxPeaks];
int peakIndex = 0;

volatile uint8_t ledBrightness = 63;
const unsigned int maxlen = (maxX * maxPages) + 1;
const long ram = 12582912;  //12MB ram for sounds // 16MB total
const unsigned int SONG_LEN = maxX * maxPages;

bool touchState[] = { false };      // Current touch state (HIGH/LOW)
bool lastTouchState[] = { false };  // Previous touch state
const int touchThreshold = 40;

const unsigned int totalPulsesToWait = pulsesPerBar * 2;

const unsigned long CHECK_INTERVAL = 50;  // Interval to check buttons in ms
unsigned long lastCheckTime = 0;          // Get the current time



unsigned long recordStartTime = 0;
const unsigned long recordDuration = 5000;  // 5 seconds
bool isRecording = false;
File frec;


// which input on the audio shield will be used?
//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

/*timers*/
volatile unsigned int lastButtonPressTime = 0;
volatile bool resetTimerActive = false;

// runtime
//  variables for program logic
float pulse = 1;
volatile int dir = 1;
unsigned int MIDI_CH = 1;
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

volatile unsigned int beat = 1;
unsigned int samplePackID, fileID = 1;
EXTMEM unsigned int lastPreviewedSample[FOLDER_MAX] = {};
IntervalTimer playTimer;
IntervalTimer midiTimer;
unsigned int lastPage = 1;
int editpage = 1;

EXTMEM unsigned int tmp[maxlen][maxY + 1][2] = {};
EXTMEM unsigned int original[maxlen][maxY + 1][2] = {};
EXTMEM unsigned int note[maxlen][maxY + 1][2] = {};

unsigned int sample_len[maxFiles];
bool sampleLengthSet = false;
bool isNowPlaying = false;  // global
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

elapsedMillis msecs;


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
Mode volume_bpm = { "VOLUME_BPM", { 1, 6, VOL_MIN, BPM_MIN }, { 1, 25, VOL_MAX, BPM_MAX }, { 1, 6, 9, 100 }, { 0x000000, 0xFFFFFF, 0xFF4400, 0x00FFFF } };  //OK
//filtermode has 4 entries
Mode filterMode = { "FILTERMODE", { 0, 1, 0, 0 }, { 7, 1, 8, 32 }, { 1, 1, 1, 1 }, { 0x000000, 0x000000, 0x000000, 0x000000 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 7, 7, 7 }, { 9, 9, 9, 9 }, { 8, 8, 8, 8 }, { 0xFFFF00, 0x000000, 0x000000, 0xFFFFFF } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { maxY, 1, maxY, maxY }, { maxY, 1, 10, 10 }, { 0xFF4400, 0x000000, 0x0044FF, 0x888888 } };

Mode set_Wav = { "SET_WAV", { 1, 1, 1, 1 }, { 9999, FOLDER_MAX, 9999, 999 }, { 0, 0, 0, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 1 }, { 1, 1, 99, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode loadSaveTrack = { "LOADSAVE_TRACK", { 1, 1, 1, 1 }, { 1, 1, 12, 12 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode menu = { "MENU", { 1, 1, 1, 1 }, { 1, 1, 1, 5 }, { 1, 1, 1, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };

// Declare currentMode as a global variable
Mode *currentMode;



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
  float param_settings[maxY][maxFilters];
  unsigned int selectedParameter;

  float filter_settings[maxY][maxFilters];
  unsigned int selectedFilter;

  unsigned int mute[maxY];
  unsigned int channelVol[16];
};

//EXTMEM?
volatile Device SMP = {
  false,                                               //singleMode
  1,                                                   //currentChannel
  10,                                                  //volume
  100,                                                 //bpm
  10,                                                  //velocity
  1,                                                   //page
  1,                                                   //edit
  1,                                                   //file
  1,                                                   //pack
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  //sampleID
  0,                                                   //folder
  false,                                               //activeCopy
  1,                                                   //x
  2,                                                   //y
  0,                                                   //seek
  0,                                                   //seekEnd
  0,                                                   //sampleLen
  0,                                                   //shiftX
  0,                                                   //shiftY
  {},                                                  //param_settings
  0,
  {},  //filter_settings
  0,
  {},                                                                 //mute
  { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }  //channelVol
};


enum VoiceSelect {
};  // Define filter types

enum ParameterType {
  TYPE,  //AudioSynthNoiseWhite, AudioSynthSimpleDrums OR WAVEFORM
  WAVEFORM,
  DELAY,
  ATTACK,
  HOLD,
  DECAY,
  SUSTAIN,
  RELEASE
};  // Define filter types

char *activeParameterType[10] = { "TYPE", "WAV", "DLAY", "ATTC", "HOLD", "DCAY", "SUST", "RLSE" };


enum FilterType { LOWPASS,
                  HIGHPASS,
                  FREQUENCY,  //AudioFilterStateVariable ->freqency
                  FLANGER,
                  ECHO,
                  DISTORTION,
                  RINGMOD,
                  DETUNE,
                  NOISE
};  // Define filter types

char *activeFilterType[10] = { "LOW", "HIGH", "FREQ", "FLNG", "ECHO", "DIST", "RING", "DTNE", "NOIS" };


// ADSR array that stores the currently selected value for each button


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



EXTMEM arraysampler _samplers[13];
AudioPlayArrayResmp *voices[] = { &sound0, &sound1, &sound2, &sound3, &sound4, &sound5, &sound6, &sound7, &sound8, &sound9, &sound10, &sound11, &sound12 };
AudioEffectEnvelope *envelopes[] = { &envelope0, &envelope1, &envelope2, &envelope3, &envelope4, &envelope5, &envelope6, &envelope7, &envelope8, &envelope9, &envelope10, &envelope11, &envelope12, &envelope13, &envelope14 };
AudioFilterStateVariable *filters[] = { &filter0, &filter1, &filter2, &filter3, &filter4, &filter5, &filter6, &filter7, &filter8, &filter9, &filter10, &filter11, &filter12, &filter13, &filter14 };
AudioAmplifier *amps[] = { &amp0, &amp1, &amp2, &amp3, &amp4, &amp5, &amp6, &amp7, &amp8, &amp9, &amp10, &amp11, &amp12, &amp13, &amp14 };

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



void setDahdsrDefaults(bool allChannels) {
  if (allChannels) {
    for (int ch = 0; ch < 16; ch++) {
      // First clear all 15 settings to zero
      for (int param = 0; param < 15; param++) {
        SMP.param_settings[ch][param] = 0;
      }
      // Now assign default envelope values for a DAHDSR shape:
      SMP.param_settings[ch][DELAY] = 11;    // No delay
      SMP.param_settings[ch][ATTACK] = 24;   // Quick attack duration
      SMP.param_settings[ch][HOLD] = 21;     // MIN hold time
      SMP.param_settings[ch][DECAY] = 27;    // MID decay period
      SMP.param_settings[ch][SUSTAIN] = 16;  // MID sustain level
      SMP.param_settings[ch][RELEASE] = 23;  // MID release
    }
  } else {
    int ch = SMP.currentChannel;
    for (int param = 0; param < 15; param++) {
      SMP.param_settings[ch][param] = 0;
    }
    // Now assign default envelope values for a DAHDSR shape:
    SMP.param_settings[ch][DELAY] = 11;    // No delay
    SMP.param_settings[ch][ATTACK] = 24;   // Quick attack duration
    SMP.param_settings[ch][HOLD] = 21;     // MIN hold time
    SMP.param_settings[ch][DECAY] = 27;    // MID decay period
    SMP.param_settings[ch][SUSTAIN] = 16;  // MID sustain level
    SMP.param_settings[ch][RELEASE] = 23;  // MID release
  }
}



void setVelocity() {
  if (currentMode->pos[0] != SMP.velocity) {
    SMP.velocity = currentMode->pos[0];

    if (SMP.singleMode) {
      note[SMP.x][SMP.y][1] = round(mapf(SMP.velocity, 1, maxY, 1, 127));
    } else {

      for (unsigned int nx = 1; nx < maxlen; nx++) {
        for (unsigned int ny = 1; ny < maxY + 1; ny++) {
          if (note[nx][ny][0] == note[SMP.x][SMP.y][0])
            note[nx][ny][1] = round(mapf(SMP.velocity, 1, maxY, 1, 127));
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

void switchMode(Mode *newMode) {

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

    if (currentMode == &set_Wav) {
      //REVERSE left encoder
      Encoder[0].begin(
        i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
        | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
        | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);
    }

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

      if (currentMode == &filterMode) {
        //REVERSE left encoder
        Encoder[0].begin(
          i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
          | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
          | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);

        //selectedFX = TYPE;
        //Encoder[3].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel][0]);
      }
    }
  }
  drawPlayButton();
}


void interpolatePeaks() {
  int lastValidIndex = peakIndex - 1;

  if (lastValidIndex >= 1) {
    float step = (float)(peakValues[lastValidIndex] - peakValues[0]) / lastValidIndex;

    for (int i = 1; i < lastValidIndex; i++) {
      peakValues[i] = peakValues[0] + (step * i);
    }
  }
}

void continueRecording() {
  if (queue1.available() >= 2) {
    uint8_t buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer + 256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    //elapsedMicros usec = 0;
    frec.write(buffer, 512);
    // Uncomment these lines to see how long SD writes
    // are taking.  A pair of audio blocks arrives every
    // 5802 microseconds, so hopefully most of the writes
    // take well under 5802 us.  Some will take more, as
    // the SD library also must write to the FAT tables
    // and the SD card controller manages media erase and
    // wear leveling.  The queue1 object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
    //Serial.print("SD write, us=");
    //Serial.println(usec);
  }
}

void record(int fnr, int snr) {


  if (!isRecording) {

    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));

    Serial.print("RECORDING IN >>>> ");
    Serial.println(OUTPUTf);

    isRecording = true;
    Serial.println("Start recording");
    showIcons("icon_rec", CRGB(20, 0, 0));
    FastLED.show();

    if (SD.exists(OUTPUTf)) {
      SD.remove(OUTPUTf);
    }

    frec = SD.open(OUTPUTf, FILE_WRITE);

    if (frec) {
      queue1.begin();
    }
  }
}

void stopRecord(int fnr, int snr) {
  if (isRecording) {

    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));

    Serial.println("Stop Recording");
    queue1.end();
    if (frec) {
      while (queue1.available() > 0) {
        frec.write((uint8_t *)queue1.readBuffer(), 256);
        queue1.freeBuffer();
      }
      frec.close();
    }
    isRecording = false;

    Serial.println("Start Playing");
    playRaw0.play(OUTPUTf);
  }
}

void drawPlayButton() {
  unsigned int timer = (beat - 2) % maxX + 1;
  if (currentMode == &draw || currentMode == &singleMode) {
    if (isNowPlaying) {
      if (timer == 1 || timer == 5 || timer == 9 || timer == 13) {
        Encoder[2].writeRGBCode(0xFF0000);
      } else {
        Encoder[2].writeRGBCode(0x550000);
      }
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


    if (!isNowPlaying) {
      Serial.println("PLAY");
      playNote();
      play(true);

    } else {

      unsigned long currentTime = millis();
      if (currentTime - playStartTime > 200) {  // Check if play started more than 200ms ago
        pause();
      }
    }

    //if (isNowPlaying) pause();
  }

  if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && buttonString == "0200") {
    tmpMute = true;
    tmpMuteAll(true);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0900") {
    if (tmpMute) tmpMuteAll(false);
  }

  // Shift notes around in single mode after dblclick of button 4
  if (currentMode == &singleMode && buttonString == "2200") {
    SMP.shiftX = 8;
    SMP.shiftY = 8;

    Encoder[3].writeCounter((int32_t)8);
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
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0001") {
    freshPaint = false;
  }

  // Switch to volume mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0220") {

    switchMode(&volume_bpm);
  }

  // Switch to filter mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0020") {

    switchMode(&filterMode);
  }


  // Toggle copy/paste in draw mode
  if (currentMode == &draw && buttonString == "1100") {
    toggleCopyPaste();
  }



  // Handle velocity switch in single mode
  if (!freshPaint && note[SMP.x][SMP.y][0] != 0 && (currentMode == &singleMode) && buttonString == "0002") {
    unsigned int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 127, 1, maxY));

    Serial.println(velo);
    SMP.velocity = velo;
    switchMode(&velocity);
    SMP.singleMode = true;

    Encoder[0].writeCounter((int32_t)velo);
  }

  // Handle velocity switch in draw mode
  if (!freshPaint && note[SMP.x][SMP.y][0] != 0 && (currentMode == &draw) && buttonString == "0002") {
    unsigned int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 127, 1, maxY));
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

  // switch Filters
  if (currentMode == &filterMode && buttonString == "0002") {
    setDahdsrDefaults(false);
    currentFilter = activeParameterType[SMP.selectedParameter];
    currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][SMP.selectedParameter];
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
  if ((currentMode == &singleMode) && buttonString == "0022") {
    //set loaded sample
    switchMode(&set_Wav);
    currentMode->pos[3] = SMP.wav[SMP.currentChannel][0];
    SMP.wav[SMP.currentChannel][1] = SMP.wav[SMP.currentChannel][0];
    //set encoder to currently Loaded Sample!!
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
  } else if ((currentMode == &set_Wav) && buttonString == "0200") {

    Serial.println("RECORDING TO ============> ");
    Serial.println(SMP.wav[SMP.currentChannel][1]);

    record(getFolderNumber(SMP.wav[SMP.currentChannel][1]), SMP.wav[SMP.currentChannel][1]);
  } else if ((currentMode == &set_Wav) && buttonString == "0900") {
    stopRecord(getFolderNumber(SMP.wav[SMP.currentChannel][1]), SMP.wav[SMP.currentChannel][1]);
  }

  // Set SamplePack + Load + Save + Exit
  if ((currentMode == &draw) && buttonString == "2200") {
    switchMode(&set_SamplePack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "0100") {
    saveSamplePack(SMP.pack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "1000") {
    loadSamplePack(SMP.pack, false);
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
    Serial.println("PAINTMODE ON");
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



void _cycleFilterTypes(int button) {
  int numValues = 0;
  // If the button has no cycling values, do nothing
  //if (buttonValues[button][0] == MAX_TYPE || buttonValues[button][0] == 0) return;
  // Count valid entries in buttonValues[button] (ignoring MAX_TYPE)
  //while (numValues < 4 && buttonValues[button][numValues] != MAX_TYPE) {
  //  numValues++;
  //}
  // Find current position of adsr[button] in buttonValues[button]
  for (int i = 0; i < numValues; i++) {
    //if (adsr[button] == buttonValues[button][i])
    {
      // Move to the next value, wrapping around
      //adsr[button] = buttonValues[button][(i + 1) % numValues];
      Serial.print("Button ");
      Serial.print(button + 1);
      Serial.print(" toggled to: ");


      /*
      //TOOGLE BETWEEN RELEASE AND WF
      if (selectedFX == WAVEFORM) {
        Serial.println("---> WAVE");
        Serial.println(SMP.param_settings[SMP.currentChannel][WAVEFORM]);
        Encoder[0].writeMax((int32_t)5);
        Encoder[0].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel][WAVEFORM]);
      }

      if (adsr[3] == SUSTAIN) {
        Serial.println("---> SUSTAIN");
        Serial.println(SMP.param_settings[SMP.currentChannel][SUSTAIN]);
        Encoder[3].writeMax((int32_t)100);  // current maxvalues
        Encoder[3].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel][SUSTAIN]);
      }

        if (adsr[3] == RELEASE) {
        Serial.println("---> RELEASE");
        Serial.println(SMP.param_settings[SMP.currentChannel][RELEASE]);
        Encoder[3].writeMax((int32_t)100);  // current maxvalues
        Encoder[3].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel][RELEASE]);
      }

      */
      return;
    }
  }
}



// ----- Generate Explosion Particles -----
// Called once when Phase 2 starts. Each "1" pixel in the logo becomes a particle.
void generateParticles() {
  particleCount = 0;
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 16; col++) {
      if (logo[row][col] == 1) {
        // Particle's initial position (center of that cell)
        float initX = col + 0.5;
        float initY = row + 0.5;
        // Explosion direction from the center
        float dx = initX - logoCenterX;
        float dy = initY - logoCenterY;
        float length = sqrt(dx * dx + dy * dy);
        float dirX = (length == 0) ? 0 : (dx / length);
        float dirY = (length == 0) ? 0 : (dy / length);
        // Freeze the pixel’s color from timeFactor=1
        CRGB color = getLogoPixelColor(col, row, 1.0);
        particles[particleCount] = { initX, initY, dirX, dirY, color };
        particleCount++;
      }
    }
  }
  particlesGenerated = true;
}


// ----- Determine Color of Each LED Based on Time -----
CRGB getPixelColor(uint8_t x, uint8_t y, unsigned long elapsed) {
  // Convert (x,y) to float center coords
  float cellCenterX = x + 0.5;
  float cellCenterY = y + 0.5;

  if (elapsed < phase1Duration) {
    // PHASE 1: Rainbow Logo
    if (logo[y][x] == 1) {
      float timeFactor = (float)elapsed / phase1Duration;  // 0..1
      return getLogoPixelColor(x, y, timeFactor);
    } else {
      return CRGB::Black;
    }
  } else if (elapsed < totalAnimationTime) {
    // PHASE 2: Explosion
    if (!particlesGenerated) {
      generateParticles();
    }
    float progress = (float)(elapsed - phase1Duration) / phase2Duration;  // 0..1
    const float maxDisp = 20.0;                                           // how far particles fly outward

    // Check each particle to see if it's near this LED cell
    for (int i = 0; i < particleCount; i++) {
      float px = particles[i].initX + particles[i].dirX * progress * maxDisp;
      float py = particles[i].initY + particles[i].dirY * progress * maxDisp;
      float dist = sqrt((cellCenterX - px) * (cellCenterX - px) + (cellCenterY - py) * (cellCenterY - py));
      if (dist < 0.4) {
        return particles[i].color;
      }
    }
    return CRGB::Black;
  } else {
    // After animation, stay black
    return CRGB::Black;
  }
}

// ----- Run the Animation Once (Called in setup) -----
void runAnimation() {
  unsigned long startTime = millis();
  while (true) {
    unsigned long elapsed = millis() - startTime;
    if (elapsed > totalAnimationTime) {
      elapsed = totalAnimationTime;
    }

    // Update each LED in the 16×16 grid (0-based coords)
    for (uint8_t y = 0; y < maxY; y++) {
      for (uint8_t x = 0; x < maxX; x++) {
        CRGB color = getPixelColor(x, y, elapsed);
        // The light() function uses 1-based coords:
        light(x + 1, y + 1, color);
      }
    }
    FastLED.show();
    delay(10);

    // Stop after totalAnimationTime
    if (elapsed >= totalAnimationTime) {
      break;
    }
  }

  // Clear the display at end
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}




void setup(void) {
  NVIC_SET_PRIORITY(IRQ_LPUART8, 1);
  //NVIC_SET_PRIORITY(IRQ_USB1, 128);  // USB1 for Teensy 4.x
  Serial.begin(115200);

  Wire.begin();
  if (CrashReport) {
    Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
    Serial.print(CrashReport);
    delay(1000);
  }


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
  FastLED.addLeds<WS2812SERIAL, DATA_PIN, BRG>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);




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

  runAnimation();


  switchMode(&draw);
  Serial.print("Initializing SD card...");
  drawNoSD();





  getLastFiles();
  loadSamplePack(samplePackID, true);

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


  mixer0.gain(0, GAIN);
  mixer0.gain(1, GAIN);
  mixer0.gain(2, GAIN);
  mixer0.gain(3, GAIN);

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


  mixerPlay.gain(0, GAIN);
  mixerPlay.gain(1, GAIN);
  mixerPlay.gain(2, GAIN);
  mixerPlay.gain(3, GAIN);



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


  AudioInterrupts();

  // set BPM:100
  SMP.bpm = 100;
  playTimer.begin(playNote, playNoteInterval);
  playTimer.priority(170);

  midiTimer.begin(checkMidi, playNoteInterval / 24);
  AudioMemory(80);
  // turn on the output
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.9);

  //REC

  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.micGain(35);  //0-63
  sgtl5000_1.adcHighPassFilterDisable();

  sgtl5000_1.unmuteLineout();
  // According to info in libraries\Audio\control_sgtl5000.cpp
  // 31 is LOWEST output of 1.16V and 13 is HIGHEST output of 3.16V
  sgtl5000_1.lineOutLevel(1);

  autoLoad();
  switchMode(&draw);


  MIDI.begin(MIDI_CH);
  //Serial8.begin(57600);


  setDahdsrDefaults(true);
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
    posString += String(currentMode->pos[i]) + ":";
    handle_button_state(&Encoder[i], i);



    buttonString += String(buttons[i]);
  }

  if (currentMode == &draw || currentMode == &singleMode) {
    if (posString != oldPosString) {
      oldPosString = posString;
      Serial.println(posString);
      SMP.x = currentMode->pos[3];
      SMP.y = currentMode->pos[0];
    }
    if (currentMode == &draw) {
      // offset y by one (notes (channel 1/red) start at row #1)
      SMP.currentChannel = SMP.y - 1;
    }

    Encoder[0].writeRGBCode(CRGBToUint32(col[SMP.currentChannel]));
    Encoder[3].writeRGBCode(CRGBToUint32(col[SMP.currentChannel]));
    //WHY???
    //SMP.edit = 1;

    if (paintMode) {
      note[SMP.x][SMP.y][0] = SMP.currentChannel;
    }
    if (paintMode && currentMode == &singleMode) {
      note[SMP.x][SMP.y][0] = SMP.currentChannel;
    }

    if (unpaintMode) {
      if (SMP.singleMode) {
        if (note[SMP.x][SMP.y][0] == SMP.currentChannel)
          note[SMP.x][SMP.y][0] = 0;
      } else {
        note[SMP.x][SMP.y][0] = 0;
      }
    }


    if (currentMode->pos[1] != editpage) {
      //SMP.edit = editpage;
      editpage = currentMode->pos[1];
      Serial.println("p:" + String(editpage));
      int xval = mapXtoPageOffset(SMP.x) + ((editpage - 1) * 16);
      Encoder[3].writeCounter((int32_t)xval);
      SMP.x = xval;
      SMP.edit = editpage;
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

void checkTouchInputs() {
  int touchValue1 = fastTouchRead(SWITCH_1);
  int touchValue2 = fastTouchRead(SWITCH_2);

  // Determine if the touches are above the threshold
  touchState[0] = (touchValue1 > touchThreshold);
  touchState[1] = (touchValue2 > touchThreshold);

  // Handle simultaneous touch
  if (touchState[0] && touchState[1] && !lastTouchState[2]) {
    lastTouchState[2] = true;
    // Optional: Define a special mode when both are touched
    if (currentMode == &singleMode) {
      Serial.println("---------<>---------");
      SMP.singleMode = false;
      switchMode(&set_Wav);

    } else {
      Serial.println("!!!!!!!!!");
      switchMode(&loadSaveTrack);
    }
    // Define specialMode if needed
  } else {
    // Handle individual touch events

    // Check for a rising edge on SWITCH_1 (LOW to HIGH transition)
    if (touchState[0] && !lastTouchState[0]) {
      lastTouchState[2] = false;
      if (currentMode == &draw) {

        SMP.currentChannel = SMP.currentChannel;
        switchMode(&singleMode);
        SMP.singleMode = true;
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


void checkMidi() {

  if (MIDI.read()) {
    int pitch, velocity, channel, d1, d2;
    uint8_t miditype;

    miditype = MIDI.getType();

    switch (miditype) {
      case midi::NoteOn:
        pitch = MIDI.getData1();
        velocity = 127;  //MIDI.getData2();
        //channel = MIDI.getChannel();
        handleNoteOn(channel, pitch, velocity);
        //if (velocity > 0) {
        //Serial.println(String("Note On:  ch=") + channel + ", note=" + pitch + ", velocity=" + velocity);
        //}
        break;

      case midi::NoteOff:
        pitch = MIDI.getData1();
        velocity = 127;
        handleNoteOff(channel, pitch, velocity);
        // velocity = MIDI.getData2();
        // channel = MIDI.getChannel();
        // Serial.println(String("Note Off: ch=") + channel + ", note=" + pitch + ", velocity=" + velocity);
        break;

      default:
        //d1 = MIDI.getData1();
        //d2 = MIDI.getData2();
        //Serial.println(String("Message, type=") + miditype + ", data = " + d1 + " " + d2);
        break;
    }
  }
}


int getPage(int x) {
  return (x - 1) / maxX + 1;  // Calculate page number
}

void loop() {

  if (isRecording) {
    continueRecording();
    checkEncoders();
    checkButtons();
    return;
  }


  if (previewIsPlaying) {
    if (msecs > 5) {
      if (playRaw0.isPlaying() && peakIndex < maxPeaks) {
        if (peak1.available()) {
          msecs = 0;
          // Store the peak value
          peakValues[peakIndex] = peak1.read();
          Serial.println(peakValues[peakIndex]);
          peakIndex++;
        }
      }
    }

    if (!playRaw0.isPlaying()) {
      previewIsPlaying = false;  // Playback finished
    }
  }

  // dont forget
  drawPlayButton();

  //checkSingleTouch();
  //checkMenuTouch();

  checkTouchInputs();


  if (note[SMP.x][SMP.y][0] == 0 && (currentMode == &draw || currentMode == &singleMode) && pressed[3] == true) {
    Serial.println("------------------");
    Serial.println(note[SMP.x][SMP.y][0]);
    Serial.println("------------------");
    paintMode = false;
    freshPaint = true;
    unpaintMode = false;
    pressed[3] = false;
    paint();
    return;
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
  SMP.edit = getPage(SMP.x);


  // Continuously check encoders


  // Set stateMashine
  if (currentMode->name == "DRAW") {
    //checkEncoders();
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
    showWave();


  } else if (currentMode->name == "NOTE_SHIFT") {
    shiftNotes();
    drawBase();
    drawTriggers();
  }

  if (currentMode == &draw || currentMode == &singleMode || currentMode == &velocity) {
    drawBase();
    drawTriggers();
    if (currentMode != &velocity)
      drawCursor();
    if (isNowPlaying) {
      drawTimer();
      if (!freshPaint && currentMode == &velocity) drawVelocity();
      FastLEDshow();
    }

    // Is there a MIDI message incoming ?
    //checkMidi();
    if (!freshPaint && currentMode == &velocity) drawVelocity();
  }

  // end synthsound after X ms
  if (noteOnTriggered && millis() - startTime >= 200) {
    envelope14.noteOff();
    envelope13.noteOff();
    noteOnTriggered = false;
  }

  FastLEDshow();  // draw!
  yield();
  delay(50);
  yield();
}


void setLastFile() {
  //set maxFiles in folder and show loading...
  for (int f = 0; f <= FOLDER_MAX; f++) {
    //FastLEDclear();

    for (unsigned int i = 1; i < 99; i++) {
      char OUTPUTf[50];
      sprintf(OUTPUTf, "samples/%d/_%d.wav", f, i + (f * 100));
      if (SD.exists(OUTPUTf)) {
        lastFile[f] = i + (f * 100);
      }
    }
    //set last File on Encoder
    drawLoadingBar(1, 999, lastFile[f], col_base[f], CRGB(15, 15, 55), false);
  }
  //set lastFile Array into Eeprom
  EEPROM.put(100, lastFile);
}

void getLastFiles() {
  //get lastFile Array from Eeprom
  EEPROM.get(100, lastFile);
  set_Wav.maxValues[3] = lastFile[FOLDER_MAX - 1];
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

    Encoder[3].writeCounter((int32_t)8);
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

    Encoder[0].writeCounter((int32_t)8);
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



  /* shift only this page:*/
}


void tmpMuteAll(bool pressed) {
  if (pressed > 0) {
    int channel = SMP.currentChannel;
    //mute all channels except current
    for (unsigned int i = 1; i < maxFiles + 1; i++) {
      if (i != channel) {
        SMP.mute[i] = true;
      } else {
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
  }
  isNowPlaying = true;
  playNote();
  playStartTime = millis();  // Record the current time
}

void pause() {
  isNowPlaying = false;
  updateLastPage();
  deleteActiveCopy();
  autoSave();
  envelope0.noteOff();
  allOff();
  Encoder[2].writeRGBCode(0x005500);
  beat = 1;
  SMP.page = 1;
}




void playNote() {


  if (isNowPlaying) {

    playTimer.end();  // Stop the timer
    //drawTimer();

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


    // midi functions
    if (waitForFourBars && pulseCount >= totalPulsesToWait) {
      beat = 1;
      SMP.page = 1;
      isNowPlaying = true;
      Serial.println("4 Bars Reached");
      waitForFourBars = false;  // Reset for the next start message
    }
    yield();


    beatStartTime = millis();
    beat++;




    if (beat > SMP.page * maxX) {
      updateLastPage();
      SMP.page = SMP.page + 1;
      if (SMP.page > maxPages)
        SMP.page = 1;
      if (SMP.page > lastPage)
        SMP.page = 1;
    }

    if (beat > maxX * lastPage) {
      beat = 1;
      SMP.page = 1;
    }
    playTimer.begin(playNote, playNoteInterval);
  }
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
    note[x][y][1] = defaultVelocity;
  } else {
    if (note[x][y][0] == SMP.currentChannel)
      note[x][y][0] = 0;
    note[x][y][1] = defaultVelocity;
  }
  updateLastPage();
  FastLEDshow();
}


void paint() {
  //SMP.edit = 1;
  Serial.println("!!!PAINT!");
  unsigned int sample = 1;
  unsigned int x = SMP.x;  //(SMP.edit - 1) * maxX + SMP.x;
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

  if (!isNowPlaying) {
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
  if (y >= 1 && y <= maxY && x >= 1 && x <= maxX) {
    if (y > maxY) y = 1;
    if (y % 2 == 0) {
      index = (maxX - x) + (maxX * (y - 1));
    } else {
      index = (x - 1) + (maxX * (y - 1));
    }
    if (index < NUM_LEDS) { leds[index] = color; }
  }
  yield();
}



void displaySample(unsigned int start, unsigned int size, unsigned int end) {
  return;
  unsigned int length = mapf(size, 0, 302000, 1, maxX);
  unsigned int starting = mapf(start * SEARCHSIZE, 0, size, 1, maxX);
  unsigned int ending = mapf(end * SEARCHSIZE, 0, size, 1, maxX);

  //for (unsigned int s = 1; s <= maxX; s++) {
  //light(s, 5, CRGB(10, 20, 10));
  //}

  for (unsigned int s = 1; s <= length; s++) {
    light(s, 9, CRGB(10, 10, 0));
  }

  for (unsigned int s = 1; s <= starting; s++) {
    light(s, 7, CRGB(0, 10, 0));
  }

  for (unsigned int s = starting; s <= ending; s++) {
    light(s, 6, CRGB(10, 10, 10));
  }


  for (unsigned int s = ending; s <= maxX; s++) {
    light(s, 7, CRGB(10, 0, 0));
  }

  //FastLED.setBrightness(ledBrightness);
  //FastLED.show();
}





void previewSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength, bool firstPreview) {
  // envelopes[0]->release(0);


  _samplers[0].removeAllSamples();
  envelope0.noteOff();
  char OUTPUTf[50];
  int plen = 0;
  sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, sampleID);
  Serial.print("PLAYING:::");
  Serial.println(OUTPUTf);
  File previewSample = SD.open(OUTPUTf);
  //SMP.smplen = 0;

  if (previewSample) {
    int fileSize = previewSample.size();

    if (firstPreview) {
      fileSize = min(previewSample.size(), 302000);  //max 10SEC preview // max preview len =  X Sec. //toDO: make it better to preview long files
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

    int startOffset = 1 + (SEARCHSIZE * SMP.seek);  // Start offset in milliseconds
    int endOffset = (SEARCHSIZE * SMP.seekEnd);     // End offset in milliseconds

    if (setMaxSampleLength == true) {
      endOffset = fileSize;
    }

    int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)
    int endOffsetBytes = endOffset * PrevSampleRate * 2;      // Convert to bytes (assuming 16-bit samples)

    // Adjust endOffsetBytes to avoid reading past the file end
    endOffsetBytes = min(endOffsetBytes, fileSize);  //- 44?

    previewSample.seek(44 + startOffsetBytes);
    memset(sampled[0], 0, sizeof(sampled[0]));
    plen = 0;

    while (previewSample.available() && (plen < (endOffsetBytes - startOffsetBytes))) {
      int b = previewSample.read();
      sampled[0][plen] = b;
      plen++;
      if (plen >= sizeof(sampled[0])) break;  // Prevent buffer overflow
    }

    sampleIsLoaded = true;
    SMP.smplen = fileSize / (PrevSampleRate * 2);


    if (setMaxSampleLength) {
      SMP.seekEnd = SMP.smplen / SEARCHSIZE;
      currentMode->pos[2] = SMP.seekEnd;
      Encoder[2].writeCounter((int32_t)SMP.seekEnd);
      sampleLengthSet = true;
    }

    previewSample.close();
    displaySample(SMP.seek, SMP.smplen, SMP.seekEnd);

    _samplers[0].addSample(36, (int16_t *)sampled[0], (int)plen, 1);  //-44?

    Serial.println("NOTE");
    _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
  }
}


void drawLoadingBar(int minval, int maxval, int currentval, CRGB color, CRGB fontColor, bool intro) {
  int ypos = 4;
  int height = 2;
  int barwidth = mapf(currentval, minval, maxval, 1, maxX);
  for (int x = 1; x <= maxX; x++) {
    light(x, ypos - 1, fontColor);
  }
  //draw the border-ends
  light(1, ypos, fontColor);
  light(maxX, ypos, fontColor);

  for (int x = 2; x <= maxX - 1; x++) {
    for (int y = 0; y <= (height - 1); y++) {
      if (x <= barwidth + 1) {
        light(x, ypos + y, color);
      } else {
        light(x, ypos + y, CRGB(0, 0, 0));
      }
    }
  }
  if (!intro) {
    showNumber(currentval, fontColor, 11);
  } else {
    FastLED.show();
  }
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

    //SMP.seek = 0;

    unsigned int startOffset = 1 + (SEARCHSIZE * SMP.seek);            // Start offset in milliseconds
    unsigned int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)

    unsigned int endOffset = SEARCHSIZE * SMP.seekEnd;  // End offset in milliseconds
    if (SMP.seekEnd == 0) {
      // If seekEnd is not set, default to the full length of the sample
      endOffset = fileSize;
    }
    unsigned int endOffsetBytes = endOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)
    // Adjust endOffsetBytes to avoid reading past the file end
    endOffsetBytes = min(endOffsetBytes, fileSize);  //-44?

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
    _samplers[sampleID].addSample(36, (int16_t *)sampled[sampleID], (int)i, 1);  //-44?
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

  unsigned int start = ((SMP.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = SMP.currentChannel;
  bool singleMode = SMP.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  //updateLastPage();
}

void drawBPMScreen() {

  FastLEDclear();
  drawVolume(SMP.vol);
  drawBrightness();
  CRGB volColor = CRGB(SMP.vol * SMP.vol, 20 - SMP.vol, 0);
  Encoder[2].writeRGBCode(CRGBToUint32(volColor));

  showIcons("helper_exit", CRGB(0, 0, 20));

  showIcons("helper_bright", CRGB(20, 20, 20));
  showIcons("helper_vol", volColor);
  showIcons("helper_bpm", CRGB(0, 50, 120));
  showNumber(SMP.bpm, CRGB(0, 50, 120), 6);
}




void updateVolume() {
  SMP.vol = currentMode->pos[2];
  float vol = float(SMP.vol / 10.0);
  Serial.println("Vol: " + String(vol));
  if (vol <= 1.0) sgtl5000_1.volume(vol);
  //setvol = true;
}

void updateBrightness() {
  ledBrightness = (currentMode->pos[1] * 10) + 4;
  Serial.println("Brightness: " + String(ledBrightness));
}

void updateBPM() {
  //setvol = false;
  Serial.println("BPM: " + String(currentMode->pos[3]));
  SMP.bpm = currentMode->pos[3];
  playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
  playTimer.update(playNoteInterval);
  midiTimer.update(playNoteInterval / 24);
  drawBPMScreen();
}

void setVolume() {
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


void drawFilters(char *txt, int activeFilter) {
  FastLED.clear();
  // Amplitude definitions:
  int baseAmplitude = 1;        // baseline (minimum amplitude)
  CRGB activeFilterColor;
  switch (activeFilter) {
    case 0:
      activeFilterColor = CRGB(100, 0, 0);
      break;
    case 1:
      activeFilterColor = CRGB(0, 100, 0);
      break;
    case 2:
      activeFilterColor = CRGB(0, 0, 100);
      break;
    case 3:
      activeFilterColor = CRGB(100, 100, 0);
      break;
    case 4:
      activeFilterColor = CRGB(100, 0, 100);
      break;
    case 5:
      activeFilterColor = CRGB(0, 100, 100);
      break;
    case 6:
      activeFilterColor = CRGB(100, 100, 100);
      break;
    case 7:
      activeFilterColor = CRGB(100, 50, 100);
      break;
    case 8:
      activeFilterColor = CRGB(50, 10, 10);
      break;
    default:
      activeFilterColor = CRGB(0, 0, 0);  // default color (adjust as needed)
      break;
  }

  showText(txt, 1, 12, activeFilterColor);
  int FilterValue = mapf(SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter],0,32,0,10);
  showNumber(FilterValue, CRGB(100,100,100),5);
}

void setFilters() {

  if (fxType==0) drawADSR(currentFilter, SMP.selectedParameter);

  if (fxType==1) drawFilters(currentFilter,SMP.selectedFilter);

  // Encoder 3: Adjust Filters
  if (currentMode->pos[2] != SMP.selectedFilter) {
    FastLED.clear();
    SMP.selectedFilter = currentMode->pos[2];
    currentFilter = activeFilterType[SMP.selectedFilter];
    fxType = 1;
    selectedFX = currentMode->pos[2];
    Serial.print("FILTER: ");
    Serial.println(currentFilter);
    Serial.println(fxType);
    currentMode->pos[3] = SMP.filter_settings[SMP.currentChannel][SMP.selectedFilter];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    drawFilters(currentFilter,SMP.selectedFilter);
  }

  // Encoder 1: Adjust Params
  if (currentMode->pos[0] != SMP.selectedParameter) {
    FastLED.clear();
    SMP.selectedParameter = currentMode->pos[0];
    fxType = 0;

    selectedFX = currentMode->pos[0];
    currentFilter = activeParameterType[SMP.selectedParameter];
    Serial.print("PARAMS: ");
    Serial.println(currentFilter);
    currentMode->pos[3] = SMP.param_settings[SMP.currentChannel][SMP.selectedParameter];
    Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    drawADSR(currentFilter, SMP.selectedParameter);
  }

  if (fxType == 1) {
    if (selectedFX == LOWPASS) {
      // Adjust delay
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][LOWPASS]) {
        SMP.filter_settings[SMP.currentChannel][LOWPASS] = currentMode->pos[3];
        float lowPassVal = mapf(SMP.filter_settings[SMP.currentChannel][LOWPASS], 0, 32, 0, 10);
        envelope13.delay(lowPassVal);
        Serial.println("lowpass: " + String(lowPassVal));
      }
    }

    if (selectedFX == HIGHPASS) {
      //Adjust highpass
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][HIGHPASS]) {
        SMP.filter_settings[SMP.currentChannel][HIGHPASS] = currentMode->pos[3];
        float highPassVal = mapf(SMP.filter_settings[SMP.currentChannel][HIGHPASS], 0, 32, 0, 10);
        envelope13.delay(highPassVal);
        Serial.println("highpass: " + String(highPassVal));
      }
    }

    if (selectedFX == FREQUENCY) {
      //Adjust frequency
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][FREQUENCY]) {
        SMP.filter_settings[SMP.currentChannel][FREQUENCY] = currentMode->pos[3];
        float freqVal = mapf(SMP.filter_settings[SMP.currentChannel][FREQUENCY], 0, 32, 0, 10);
        envelope13.delay(freqVal);
        Serial.println("frequency: " + String(freqVal));
      }
    }



    if (selectedFX == FLANGER) {
      //Adjust flanger
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][FLANGER]) {
        SMP.filter_settings[SMP.currentChannel][FLANGER] = currentMode->pos[3];
        float flangeVal = mapf(SMP.filter_settings[SMP.currentChannel][FLANGER], 0, 32, 0, 10);
        envelope13.delay(flangeVal);
        Serial.println("flangerVal: " + String(flangeVal));
      }
    }


    if (selectedFX == ECHO) {
      //Adjust echo
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][ECHO]) {
        SMP.filter_settings[SMP.currentChannel][ECHO] = currentMode->pos[3];
        float echoVal = mapf(SMP.filter_settings[SMP.currentChannel][ECHO], 0, 32, 0, 10);
        envelope13.delay(echoVal);
        Serial.println("echoVal: " + String(echoVal));
      }
    }


    if (selectedFX == DISTORTION) {
      //Adjust distortion
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][DISTORTION]) {
        SMP.filter_settings[SMP.currentChannel][DISTORTION] = currentMode->pos[3];
        float distortVal = mapf(SMP.filter_settings[SMP.currentChannel][DISTORTION], 0, 32, 0, 10);
        envelope13.delay(distortVal);
        Serial.println("distortVal: " + String(distortVal));
      }
    }

    if (selectedFX == RINGMOD) {
      //Adjust ringmod
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][RINGMOD]) {
        SMP.filter_settings[SMP.currentChannel][RINGMOD] = currentMode->pos[3];
        float ringModVal = mapf(SMP.filter_settings[SMP.currentChannel][RINGMOD], 0, 32, 0, 10);
        envelope13.delay(ringModVal);
        Serial.println("ringModVal: " + String(ringModVal));
      }
    }


    if (selectedFX == DETUNE) {
      //Adjust detune
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][DETUNE]) {
        SMP.filter_settings[SMP.currentChannel][DETUNE] = currentMode->pos[3];
        float detuneVal = mapf(SMP.filter_settings[SMP.currentChannel][DETUNE], 0, 32, 0, 10);
        envelope13.delay(detuneVal);
        Serial.println("detuneVal: " + String(detuneVal));
      }
    }



    if (selectedFX == NOISE) {
      //Adjust noise
      if (currentMode->pos[3] != SMP.filter_settings[SMP.currentChannel][NOISE]) {
        SMP.filter_settings[SMP.currentChannel][NOISE] = currentMode->pos[3];
        float noiseVal = mapf(SMP.filter_settings[SMP.currentChannel][NOISE], 0, 32, 0, 10);
        envelope13.delay(noiseVal);
        Serial.println("noiseVal: " + String(noiseVal));
      }
    }
  }

  if (fxType == 0) {
    if (selectedFX == DELAY) {
      // Adjust delay
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][DELAY]) {
        SMP.param_settings[SMP.currentChannel][DELAY] = currentMode->pos[3];
        float delayVal = mapf(SMP.param_settings[SMP.currentChannel][DELAY], 0, 32, 0, 10);
        envelope13.delay(delayVal);
        Serial.println("delay: " + String(delayVal));
      }
    }

    if (selectedFX == ATTACK) {
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][ATTACK]) {
        SMP.param_settings[SMP.currentChannel][ATTACK] = currentMode->pos[3];
        float attackVal = mapf(SMP.param_settings[SMP.currentChannel][ATTACK], 0, 32, 0, 11880);
        envelope13.attack(attackVal);
        Serial.println("attack: " + String(attackVal));
      }
    }


    if (selectedFX == HOLD) {
      //  Adjust hold
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][HOLD]) {
        SMP.param_settings[SMP.currentChannel][HOLD] = currentMode->pos[3];
        float holdVal = mapf(SMP.param_settings[SMP.currentChannel][HOLD], 0, 32, 0, 11880);
        envelope13.hold(holdVal);
        Serial.println("hold: " + String(holdVal));
      }
    }

    if (selectedFX == DECAY) {
      //  Adjust decay
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][DECAY]) {
        SMP.param_settings[SMP.currentChannel][DECAY] = currentMode->pos[3];
        float decayVal = mapf(SMP.param_settings[SMP.currentChannel][DECAY], 0, 32, 0, 11880);
        envelope13.decay(decayVal);
        Serial.println("decay: " + String(decayVal));
      }
    }

    if (selectedFX == SUSTAIN) {
      //  Adjust sustain
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][SUSTAIN]) {
        SMP.param_settings[SMP.currentChannel][SUSTAIN] = currentMode->pos[3];
        float sustainVal = mapf(SMP.param_settings[SMP.currentChannel][SUSTAIN], 0, 32, 0, 1.0);  // Sustain range: 0.1s to 2.0s
        envelope13.sustain(sustainVal);
        Serial.println("sustain: " + String(sustainVal) + "");
      }
    }

    if (selectedFX == RELEASE) {
      //  Adjust release
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][RELEASE]) {
        SMP.param_settings[SMP.currentChannel][RELEASE] = currentMode->pos[3];
        float releaseVal = mapf(SMP.param_settings[SMP.currentChannel][RELEASE], 0, 32, 0, 1000.0);  // Release range: 0ms to 1.0s
        envelope13.sustain(releaseVal);
        Serial.println("release: " + String(releaseVal) + "");
      }
    }





    if (selectedFX == TYPE) {
      // Adjust type
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][TYPE]) {
        SMP.param_settings[SMP.currentChannel][TYPE] = currentMode->pos[3];
        float typeVAL = mapf(SMP.param_settings[SMP.currentChannel][TYPE], 1, 100, 0, 4.0);  // Release range: 0ms to 1.0s
        envelope13.sustain(typeVAL);
        Serial.println("type: " + String(typeVAL) + "");
      }
    }





    if (selectedFX == WAVEFORM) {
      // adjust waveform
      if (currentMode->pos[3] != SMP.param_settings[SMP.currentChannel][WAVEFORM]) {
        SMP.param_settings[SMP.currentChannel][WAVEFORM] = currentMode->pos[3];
        unsigned int wavformVal = mapf(SMP.param_settings[SMP.currentChannel][WAVEFORM], 0, 4, 1, 4);  // 0-4
        Serial.println("WaveForm: " + String(wavformVal));

        switch (wavformVal) {
          case 1:
            sound13.begin(WAVEFORM_SINE);
            break;
          case 2:
            sound13.begin(WAVEFORM_SAWTOOTH);
            break;
          case 3:
            sound13.begin(WAVEFORM_SQUARE);
            break;
          case 4:
            sound13.begin(WAVEFORM_TRIANGLE);
            break;
        }
      }
    }
  }
}







CRGB getCol(unsigned int g) {
  return col[g] * 10;
}

void drawVolume(unsigned int vol) {
  unsigned int maxXVolume = int(vol * 1.3) + 2;
  for (unsigned int x = 0; x <= maxXVolume; x++) {
    light(x + 1, 12, CRGB(vol * vol, 20 - vol, 0));
    light(x + 1, 13, CRGB(vol * vol, 20 - vol, 0));
  }
}

void drawBrightness() {
  unsigned int maxBrightness = ((ledBrightness - 65) * (15 - 1)) / (255 - 65) + 3;
  for (unsigned int x = 0; x < maxBrightness; x++) {
    CRGB brightness = CRGB(16 * x, 16 * x, 16 * x);
    light(x, 15, brightness);
    light(x, 16, brightness);
  }
}

void changeMenu(int newMenuPosition) {
  menuPosition = newMenuPosition;
}


void showMenu() {
  FastLEDclear();
  //showIcons("icon_loadsave", CRGB(10, 5, 0));
  //showNumber(menuPosition, CRGB(20, 20, 40), 0);


  switch (menuPosition) {
    case 1:
      showIcons("icon_loadsave", CRGB(0, 20, 0));
      showIcons("icon_loadsave2", CRGB(20, 20, 20));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(0, 200, 0));
      break;
    case 2:
      showIcons("icon_samplepack", CRGB(0, 0, 20));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(0, 0, 200));
      break;

    case 3:
      showIcons("icon_sample", CRGB(20, 0, 20));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(200, 200, 0));
      break;

    case 4:
      showIcons("icon_rec", CRGB(20, 0, 0));
      showIcons("icon_rec2", CRGB(20, 20, 20));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(200, 0, 0));
      break;

    case 5:
      showIcons("icon_settings", CRGB(50, 50, 50));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(0, 200, 200));
      break;





    default:
      break;
  }



  FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  if (currentMode->pos[3] != menuPosition) {
    changeMenu(currentMode->pos[3]);
  }
}

void showLoadSave() {

  drawNoSD();
  FastLEDclear();

  showIcons("icon_loadsave", CRGB(10, 5, 0));
  showIcons("icon_loadsave2", CRGB(200, 200, 200));
  showIcons("helper_select", CRGB(0, 0, 5));

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d.txt", SMP.file);
  if (SD.exists(OUTPUTf)) {
    showIcons("helper_save", CRGB(1, 0, 0));
    showIcons("helper_load", CRGB(0, 20, 0));
    showNumber(SMP.file, CRGB(0, 0, 20), 11);
  } else {
    showIcons("helper_save", CRGB(20, 0, 0));
    showIcons("helper_load", CRGB(0, 1, 0));
    showNumber(SMP.file, CRGB(20, 20, 40), 11);
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
  showNumber(SMP.pack, CRGB(20, 0, 0), 11);

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", SMP.pack, 1);
  if (SD.exists(OUTPUTf)) {
    showIcons("helper_load", CRGB(0, 20, 0));
    showIcons("helper_save", CRGB(3, 0, 0));
    showNumber(SMP.pack, CRGB(0, 20, 0), 11);
  } else {
    showIcons("helper_load", CRGB(0, 3, 0));
    showIcons("helper_save", CRGB(20, 0, 0));
    showNumber(SMP.pack, CRGB(20, 0, 0), 11);
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

  SMP.smplen = 0;
  SMP.seekEnd = 0;
  EEPROM.put(0, pack);

  for (unsigned int z = 1; z < maxFiles; z++) {
    if (!intro) {
      FastLEDclear();
      showIcons("icon_sample", CRGB(20, 20, 20));
    } else {
      showText("LOAD", 2, 11, col[(maxFiles + 1) - z]);
    }

    drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], CRGB(50, 50, 50), intro);
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
void processPeaks() {
  float interpolatedValues[16];  // Ensure at least 16 values

  // Map seek start and end positions to x range (1-16)
  unsigned int startX = mapf(SMP.seek * SEARCHSIZE, 0, SMP.smplen, 1, 16);
  unsigned int endX = mapf(SMP.seekEnd * SEARCHSIZE, 0, SMP.smplen, 1, 16);

  if (peakIndex > 0) {
    // Distribute peak values over 16 positions
    for (int i = 0; i < 16; i++) {
      float indexMapped = mapf(i, 0, 15, 0, peakIndex - 1);
      int lowerIndex = floor(indexMapped);
      int upperIndex = min(lowerIndex + 1, peakIndex - 1);

      if (peakIndex > 1) {
        float fraction = indexMapped - lowerIndex;
        interpolatedValues[i] = peakValues[lowerIndex] * (1 - fraction) + peakValues[upperIndex] * fraction;
      } else {
        interpolatedValues[i] = peakValues[0];  // Duplicate if only one value exists
      }
    }
  } else {
    // No peaks, default to zero
    for (int i = 0; i < 16; i++) {
      interpolatedValues[i] = 0;
    }
  }

  // Light up LEDs
  for (int i = 0; i < 16; i++) {
    int x = i + 1;  // Ensure x values go from 1 to 16
    int yPeak = mapf(interpolatedValues[i] * 100, 0, 100, 4, 11);
    yPeak = constrain(yPeak, 4, 10);

    for (int y = 4; y <= yPeak; y++) {
      // **Color gradient based on Y (vertical) instead of X**

      CRGB color;
      if (x < startX) {
        color = CRGB(0, 0, 5);  // Green for pre-start region
      } else if (x > endX) {
        color = CRGB(10, 0, 15);  // Dark red for post-end region
      } else {

        color = CRGB(((y - 4) * 35) / 4, (255 - ((y - 4) * 35)) / 4, 0);  // Red to green gradient at 25% brightness
      }

      // Light up from y = 4 to y = yPeak
      light(x, y, color);
    }
  }
}

void showWave() {
  File sampleFile;
  //drawNoSD();
  int snr = SMP.wav[SMP.currentChannel][1];
  if (snr < 1) snr = 1;
  int fnr = getFolderNumber(snr);

  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));

  if (firstcheck) {
    Serial.print("checking: ");
    firstcheck = false;
    if (!SD.exists(OUTPUTf)) {
      Serial.print(OUTPUTf);
      Serial.println(" >NOPE");
      nofile = true;
    } else {
      Serial.print(OUTPUTf);
      nofile = false;
      Serial.println(" >exists!");
    }
  }


  FastLEDclear();
  showIcons("helper_select", col[SMP.currentChannel]);
  showIcons("helper_folder", CRGB(10, 10, 0));
  showIcons("helper_seek", CRGB(10, 0, 0));
  if (nofile) {
    showIcons("helper_load", CRGB(20, 0, 0));
  } else {
    showIcons("helper_load", CRGB(0, 20, 0));
  }

  displaySample(SMP.seek, SMP.smplen + 1, SMP.seekEnd);

  processPeaks();

  // Update the LED display
  showNumber(snr, col_Folder[fnr], 12);

  if (sampleLengthSet) {
    SMP.seekEnd = SMP.smplen / SEARCHSIZE;
    currentMode->pos[2] = SMP.seekEnd;
    Encoder[2].writeCounter((int32_t)SMP.seekEnd);
    sampleLengthSet = false;  // Reset the flag after setting seekEnd
  }

  //:::::::: STARTPOSITION SAMPLE  ::::: //
  if (sampleIsLoaded && currentMode->pos[0] != SMP.seek) {
    playRaw0.stop();
    Serial.println("SEEK-hit");
    SMP.seek = currentMode->pos[0];
    Serial.println(SMP.seek);
    //STOP ALREADY PLAYING
    _samplers[0].removeAllSamples();
    envelope0.noteOff();
    previewSample(fnr, getFileNumber(snr), false, false);
  }


  //::::::: FOLDER ::::: //
  if (currentMode->pos[1] != SMP.folder) {
    playRaw0.stop();
    firstcheck = true;
    nofile = false;
    SMP.folder = currentMode->pos[1];
    Serial.println("Folder: " + String(SMP.folder - 1));
    SMP.wav[SMP.currentChannel][1] = ((SMP.folder - 1) * 100) + 1;
    Serial.print("WAV:");
    Serial.println(SMP.wav[SMP.currentChannel][1]);
    Encoder[3].writeCounter((int32_t)SMP.wav[SMP.currentChannel][1]);
  }


  //::::::: ENDPOSITION SAMPLE  ::::: //
  if (sampleIsLoaded && (currentMode->pos[2]) != SMP.seekEnd) {
    previewIsPlaying = false;
    playRaw0.stop();
    Serial.println("SEEKEND-hit");

    SMP.seekEnd = currentMode->pos[2];
    Serial.println("seekEnd updated to: " + String(SMP.seekEnd));


    //STOP ALREADY PLAYING
    _samplers[0].removeAllSamples();
    envelope0.noteOff();
    //if (!sampleLengthSet)
    previewSample(fnr, getFileNumber(snr), false, false);
    sampleLengthSet = false;
  }

  // GET SAMPLEFILE
  if (currentMode->pos[3] != snr) {
    previewIsPlaying = false;
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
    File selectedFile = SD.open(OUTPUTf);
    int PrevSampleRate;
    selectedFile.seek(24);
    for (uint8_t i = 24; i < 25; i++) {
      int g = selectedFile.read();
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

    memset(sampled[0], 0, sizeof(sampled[0]));
    sampleIsLoaded = false;
    firstcheck = true;
    nofile = false;
    SMP.wav[SMP.currentChannel][1] = currentMode->pos[3];

    int snr = SMP.wav[SMP.currentChannel][1];
    int fnr = getFolderNumber(snr);
    FastLEDclear();
    showNumber(snr, col_Folder[fnr], 12);

    Serial.println("File>> " + String(fnr) + " / " + String(getFileNumber(snr)));
    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
    //check if exists?

    // reset SEEK and stop sample playing

    currentMode->pos[0] = 0;
    SMP.seek = 0;
    Encoder[0].writeCounter((int32_t)0);


    sampleLengthSet = true;
    Serial.print("F---->");
    Serial.print(selectedFile.size());
    Serial.print(" / ");
    Serial.print(PrevSampleRate);
    Serial.println();

    //SMP.seekEnd = (selectedFile.size() / (PrevSampleRate * 2) / SEARCHSIZE);
    SMP.smplen = (selectedFile.size() / (PrevSampleRate * 2) - 44 / SEARCHSIZE);
    SMP.seekEnd = SMP.smplen;
    currentMode->pos[2] = SMP.seekEnd;
    //SET ENCODER SEARCH_END to last byte
    Encoder[2].writeCounter((int32_t)SMP.seekEnd);

    //envelope0.noteOff();
    playRaw0.stop();
    if (!previewIsPlaying && !sampleIsLoaded) {
      //lastPreviewedSample[fnr] = snr;
      playRaw0.play(OUTPUTf);
      previewIsPlaying = true;
      peakIndex = 0;
      memset(peakValues, 0, sizeof(peakValues));



      // Read peaks until the file ends or the array is full








      //previewSample(fnr, getFileNumber(snr), true, true);
      sampleIsLoaded = true;
    }
  }

  if (SMP.seekEnd > SMP.smplen / SEARCHSIZE) {
    SMP.seekEnd = SMP.smplen / SEARCHSIZE;
    currentMode->pos[2] = SMP.seekEnd;
    Encoder[2].writeCounter((int32_t)SMP.seekEnd);
  }
}



void _renderRainbowWave(int durationMs) {
  int elapsedMs = 0;
  const int frameTime = 40;       // Update every 50ms
  const float speed = 0.1f;       // Speed of the wave
  const float wavelength = 8.0f;  // Wavelength of the wave

  while (elapsedMs < durationMs) {
    for (int y = 0; y < 6; ++y) {
      for (int x = 0; x < 17; ++x) {
        // Calculate the diagonal wave position
        float wavePosition = (x + y) * speed - elapsedMs * 0.005f;
        // Map the wave position to a hue
        float hue = fmod(wavePosition / wavelength, 1.0f) * 2.0f * M_PI;
        // Get the rainbow color for the hue
        CRGB color = rainbowColor(hue);
        CRGB color2 = rainbowColor(hue / 5);
        if (logo[5 - y][x] == 1) { light(x + 1, 8 + y, color); }
      }
    }


    FastLED.show();
    // Delay to control the frame rate
    FastLED.delay(frameTime);
    elapsedMs += frameTime;
  }
}




void drawText(const char *text, int startX, int startY, CRGB color) {
  int xOffset = startX;

  for (int i = 0; text[i] != '\0'; i++) {
    drawChar(text[i], xOffset, startY, color);
    xOffset += alphabet[text[i] - 32][0] + 1;  // Advance by char width + 1 space
  }
}


void drawChar(char c, int x, int y, CRGB color) {
  if (c < 32 || c > 126) return;  // Skip unsupported characters

  uint8_t index = c - 32;              // Calculate index in alphabet array
  uint8_t width = alphabet[index][0];  // Get the width of the character

  for (int col = 0; col < width; col++) {
    uint8_t columnData = alphabet[index][col + 1];  // Get column data

    // Reverse the bits to flip the character vertically
    uint8_t flippedColumn = 0;
    for (int row = 0; row < 5; row++) {
      if (columnData & (1 << row)) {
        flippedColumn |= (1 << (4 - row));  // Reverse the row positions
      }
    }

    for (int row = 0; row < 5; row++) {
      if (flippedColumn & (1 << row)) {  // Check if the bit is set
        light(x + col, y + row, color);  // Light up the corresponding LED
      }
    }
  }
}



// Function to render text
void showText(const char *text, int startX, int startY, CRGB color) {
  int xOffset = startX;

  for (int i = 0; text[i] != '\0'; i++) {
    drawChar(text[i], xOffset, startY, color);
    xOffset += 4;  // Move to the next character (3 pixels + 1 pixel spacing)
  }
  //FastLED.show();
}



void showNumber2(unsigned int count, CRGB color, int topY) {
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


void showNumber(unsigned int count, CRGB color, int topY) {
  char buffer[12];
  unsigned int width = 0;
  if (count > 9) width = 4;
  if ((count > 99)) width = 8;
  sprintf(buffer, "%u", count);
  drawText(buffer, 14 - width, topY, color);
  FastLEDshow();
}


void drawVelocity() {
  //FastLEDclear();

  unsigned int vy = currentMode->pos[0];

  //GLOBAL
  unsigned int cv = currentMode->pos[2];
  FastLEDclear();


  for (unsigned int x = 1; x < 6; x++) {
    for (unsigned int y = 1; y < vy + 1; y++) {
      light(x, y, CRGB(y * y, 20 - y, 0));
    }
  }

  for (unsigned int x = 9; x < 13 + 1; x++) {
    for (unsigned int y = 1; y < cv + 1; y++) {
      light(x, y, CRGB(0, 20 - y, y * y));
    }
  }
  showText("v", 2, 2, CRGB(50, 50, 50));
  showText("c", 10, 2, CRGB(50, 50, 50));
}

void drawADSR(char *txt, int activeParameter) {
  FastLED.clear();


  //showIcons("helper_exit", CRGB(0, 0, 20));

  // Amplitude definitions:
  int baseAmplitude = 1;        // baseline (minimum amplitude)
  int attackHeight = maxY - 6;  // fixed attack (and hold) height
  // Sustain's height is variable, controlled by the SUSTAIN parameter:
  int sustainHeight = mapf(SMP.param_settings[SMP.currentChannel][SUSTAIN], 0, 32, baseAmplitude, attackHeight);
  // Map widths (x durations) for each stage:
  int delayWidth = mapf(SMP.param_settings[SMP.currentChannel][DELAY], 0, 32, 0, 3);      // Delay: min 0, max 3
  int attackWidth = mapf(SMP.param_settings[SMP.currentChannel][ATTACK], 0, 32, 0, 4);    // Attack: duration only (0 to 4)
  int holdWidth = mapf(SMP.param_settings[SMP.currentChannel][HOLD], 0, 32, 0, 3);        // Hold: width (0 to 3)
  int decayWidth = mapf(SMP.param_settings[SMP.currentChannel][DECAY], 0, 32, 0, 4);      // Decay: now 0 to 4
  const int sustainFixedWidth = 2;                                                        // Sustain: fixed width of 2
  int releaseWidth = mapf(SMP.param_settings[SMP.currentChannel][RELEASE], 0, 32, 0, 4);  // Release: 0 to 4

  // Compute x-positions for each stage:
  int xDelayStart = 1;
  int xDelayEnd = xDelayStart + delayWidth;
  int xAttackStart = xDelayEnd;
  int xAttackEnd = xAttackStart + attackWidth;
  int xHoldEnd = xAttackEnd + holdWidth;
  int xDecayEnd = xHoldEnd + decayWidth;
  int xSustainEnd = xDecayEnd + sustainFixedWidth;
  int xReleaseEnd = xSustainEnd + releaseWidth;



  CRGB activeParameterColor;

  switch (activeParameter) {
    case 2:
      activeParameterColor = CRGB(100, 0, 0);
      break;
    case 3:
      activeParameterColor = CRGB(0, 100, 0);
      break;
    case 4:
      activeParameterColor = CRGB(0, 0, 100);
      break;
    case 5:
      activeParameterColor = CRGB(100, 100, 0);
      break;
    case 6:
      activeParameterColor = CRGB(100, 0, 100);
      break;
    case 7:
      activeParameterColor = CRGB(0, 100, 100);
      break;
    default:
      activeParameterColor = CRGB(0, 0, 0);  // default color (adjust as needed)
      break;
  }

  // --- Draw colored envelopes first ---
  // Delay stage: flat line at baseAmplitude.
  colorBelowCurve(xDelayStart, xDelayEnd, baseAmplitude, baseAmplitude,
                  activeParameter == 2 ? CRGB(100, 0, 0) : CRGB(4, 0, 0));
  // Attack stage: rising line from baseAmplitude to fixed attackHeight.
  colorBelowCurve(xAttackStart, xAttackEnd, baseAmplitude, attackHeight,
                  activeParameter == 3 ? CRGB(0, 100, 0) : CRGB(0, 4, 0));
  // Hold stage: flat line at attackHeight.
  colorBelowCurve(xAttackEnd, xHoldEnd, attackHeight, attackHeight,
                  activeParameter == 4 ? CRGB(0, 0, 100) : CRGB(0, 0, 4));
  // Decay stage: falling line from attackHeight down to sustainHeight.
  colorBelowCurve(xHoldEnd, xDecayEnd, attackHeight, sustainHeight,
                  activeParameter == 5 ? CRGB(100, 100, 0) : CRGB(4, 4, 0));
  // Sustain stage: flat line at sustainHeight.
  colorBelowCurve(xDecayEnd, xSustainEnd, sustainHeight, sustainHeight,
                  activeParameter == 6 ? CRGB(100, 0, 100) : CRGB(4, 0, 4));
  // Release stage: falling line from sustainHeight down to baseAmplitude.
  colorBelowCurve(xSustainEnd, xReleaseEnd, sustainHeight, baseAmplitude,
                  activeParameter == 7 ? CRGB(0, 100, 100) : CRGB(0, 4, 4));

  // --- Now overlay the white envelope outline ---
  drawLine(xDelayStart, baseAmplitude, xDelayEnd, baseAmplitude, CRGB::Red);
  drawLine(xAttackStart, baseAmplitude, xAttackEnd, attackHeight, CRGB(30, 255, 104));
  drawLine(xAttackEnd, attackHeight, xHoldEnd, attackHeight, CRGB::White);
  drawLine(xHoldEnd, attackHeight, xDecayEnd, sustainHeight, CRGB::White);
  drawLine(xDecayEnd, sustainHeight, xSustainEnd, sustainHeight, CRGB::White);
  drawLine(xSustainEnd, sustainHeight, xReleaseEnd, baseAmplitude, CRGB::White);

  showText(txt, 1, 12, activeParameterColor);
}

void drawLine(int x1, int y1, int x2, int y2, CRGB color) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    light(x1, y1, color);
    if (x1 == x2 && y1 == y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

void colorBelowCurve(int xStart, int xEnd, int yStart, int yEnd, CRGB color) {
  for (int x = xStart; x <= xEnd; x++) {
    int yCurve = mapf(x, xStart, xEnd, yStart, yEnd);  // Compute envelope's y-value at this x
    for (int y = 1; y <= yCurve; y++) {
      light(x, y, color);  // Fill from y=1 up to the envelope
    }
  }
}


void drawBase() {



  if (!SMP.singleMode) {
    unsigned int colors = 0;
    for (unsigned int y = 1; y < maxY; y++) {
      //unsigned int filtering = 2;  // mapf(SMP.param_settings[SMP.currentChannel][y - 1], 0, maxfilterResolution, 50, 5);
      for (unsigned int x = 1; x < maxX + 1; x++) {
        if (SMP.mute[y - 1]) {
          light(x, y, CRGB(0, 0, 0));
        } else {
          light(x, y, col_base[colors]);
        }
      }
      colors++;
    }

    drawStatus();

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
}

void drawStatus() {
  CRGB ledColor = CRGB(0, 0, 0);
  if (SMP.activeCopy)
    ledColor = CRGB(20, 20, 0);
  for (unsigned int s = 1; s <= maxX; s++) {
    light(s, 1, ledColor);
  }

  if (currentMode == &noteShift) {
    // draw a moving marquee to indicate the note shift mode
    for (unsigned int x = 1; x <= maxX; x++) {
      light(x, 1, CRGB(0, 0, 0));
    }
    light(round(marqueePos), 1, CRGB(120, 120, 120));
    if (movingForward) {
      marqueePos = marqueePos + 1;
      if (marqueePos > maxX) {
        marqueePos = maxX;
        movingForward = false;
      }
    } else {
      marqueePos = marqueePos - 1;
      if (marqueePos < 1) {
        marqueePos = 1;
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
      ledColor = isNowPlaying ? CRGB(20, 255, 20) : CRGB(50, 50, 50);
    } else if (SMP.page == p) {
      ledColor = isNowPlaying ? CRGB(0, 15, 0) : CRGB(0, 0, 35);
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
  //currentMode->maxValues[1] = lastPage + 1;

  // Additional logic can be added here if needed
}

/************************************************
      DRAW SAMPLES
  *************************************************/
void drawTriggers() {
  // why?
  //SMP.edit = 1;
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

void drawTimer() {
  unsigned int timer = (beat - 2) % maxX + 1;
  if (SMP.page == SMP.edit) {
    if (timer < 1) timer = 1;
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


int mapXtoPageOffset(int x) {
  return x - (SMP.edit - 1) * maxX;
}

/************************************************
      USER CURSOR
  *************************************************/
void drawCursor() {
  if (dir == 1)
    pulse = pulse + 1;
  if (dir == -1)
    pulse = pulse - 1;
  if (pulse > 220) {
    dir = -1;
  }
  if (pulse < 1) {
    dir = 1;
  }
  light(mapXtoPageOffset(SMP.x), SMP.y, CRGB(255 - (int)pulse, 255 - (int)pulse, 255 - (int)pulse));
}


void loadWav() {
  Serial.println("Loading Wave :" + String(SMP.wav[SMP.currentChannel][1]));
  loadSample(0, SMP.wav[SMP.currentChannel][1]);
  switchMode(&singleMode);
  SMP.singleMode = true;
}


void savePatternAsMIDI(bool autosave) {
  yield();
  Serial.println("Saving MIDI file");
  drawNoSD();
  FastLEDclear();

  char OUTPUTf[50];
  if (autosave) {
    sprintf(OUTPUTf, "autosaved.mid");
  } else {
    sprintf(OUTPUTf, "%d.mid", SMP.file);
  }

  if (SD.exists(OUTPUTf)) {
    SD.remove(OUTPUTf);
  }

  File saveFile = SD.open(OUTPUTf, FILE_WRITE);
  if (saveFile) {
    // Write MIDI header
    saveFile.write("MThd", 4);  // Chunk type

    uint8_t chunkLength[] = { 0x00, 0x00, 0x00, 0x06 };  // Chunk length
    saveFile.write(chunkLength, 4);

    uint8_t formatType[] = { 0x00, 0x01 };  // Format type (1: multi-track)
    saveFile.write(formatType, 2);

    uint8_t numTracks[] = { 0x00, 0x01 };  // Number of tracks
    saveFile.write(numTracks, 2);

    uint8_t division[] = { 0x00, 0x60 };  // Division (96 ticks per quarter note)
    saveFile.write(division, 2);

    // Write Track Chunk Header
    saveFile.write("MTrk", 4);  // Chunk type

    // Placeholder for track length (to be updated later)
    uint32_t trackLengthPos = saveFile.position();
    uint8_t placeholder[] = { 0x00, 0x00, 0x00, 0x00 };
    saveFile.write(placeholder, 4);

    // Write SMP BPM as a meta event
    saveFile.write(0x00);  // Delta time
    saveFile.write(0xFF);  // Meta event
    saveFile.write(0x51);  // Set Tempo
    saveFile.write(0x03);  // Length
    uint32_t microsecondsPerQuarter = 60000000 / SMP.bpm;
    saveFile.write((uint8_t)((microsecondsPerQuarter >> 16) & 0xFF));
    saveFile.write((uint8_t)((microsecondsPerQuarter >> 8) & 0xFF));
    saveFile.write((uint8_t)(microsecondsPerQuarter & 0xFF));

    // Write MIDI events (notes)
    unsigned int trackLength = 0;
    for (unsigned int sdx = 1; sdx < maxlen; sdx++) {
      for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
        uint8_t channel = note[sdx][sdy][0] % 16;  // Determine MIDI channel (1-16)
        if (channel == 0) {
          continue;  // Skip invalid channel 0
        }
        uint8_t baseNote = 60;                      // C4 (Middle C)
        uint8_t noteNumber = baseNote + (sdy - 1);  // Adjust note based on sdy
        uint8_t velocity = note[sdx][sdy][1];

        if (velocity > 0) {
          // Note On event
          saveFile.write(0x00);                  // Delta time
          saveFile.write(0x90 | (channel - 1));  // Note On, specific channel (0-indexed internally)
          saveFile.write(noteNumber);            // Note number
          saveFile.write(velocity);              // Velocity
          trackLength += 4;

          // Note Off event
          saveFile.write(0x60);                  // Delta time (96 ticks later)
          saveFile.write(0x80 | (channel - 1));  // Note Off, specific channel (0-indexed internally)
          saveFile.write(noteNumber);            // Note number
          saveFile.write(0x00);                  // Velocity
          trackLength += 4;
        }
      }
    }

    // End of track event
    saveFile.write(0x00);  // Delta time
    saveFile.write(0xFF);  // Meta event
    saveFile.write(0x2F);  // End of track
    saveFile.write(0x00);  // Length
    trackLength += 4;

    // Update track length in the header
    uint32_t endPos = saveFile.position();
    saveFile.seek(trackLengthPos);
    uint8_t trackLengthBytes[] = {
      (uint8_t)((trackLength >> 24) & 0xFF),
      (uint8_t)((trackLength >> 16) & 0xFF),
      (uint8_t)((trackLength >> 8) & 0xFF),
      (uint8_t)(trackLength & 0xFF)
    };
    saveFile.write(trackLengthBytes, 4);
    saveFile.seek(endPos);

    saveFile.close();
    Serial.println("MIDI file saved successfully!");
  } else {
    Serial.println("Failed to open file for writing");
  }

  if (!autosave) {
    delay(500);
    switchMode(&draw);
  }

  yield();
}


void savePattern(bool autosave) {
  //yield();
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
  savePatternAsMIDI(true);
  savePattern(true);
  // yield();
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
    // yield();
    Mode *bpm_vol = &volume_bpm;
    bpm_vol->pos[3] = SMP.bpm;
    playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
    playTimer.update(playNoteInterval);
    midiTimer.update(playNoteInterval / 48);
    bpm_vol->pos[2] = SMP.vol;


    float vol = float(SMP.vol / 10.0);
    //if (vol <= 1.0) sgtl5000_1.volume(vol);
    // set all Filters
    //for (unsigned int i = 0; i < maxFilters; i++) {
    //filters[i]->frequency(100 * SMP.param_settings[SMP.currentChannel][i]);
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

    for (unsigned int i = 0; i < maxFilters; i++) {
      //Serial.print(SMP.param_settings[SMP.currentChannel][i]);
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
  isNowPlaying = false;
  pulseCount = 0;
  AudioMemoryUsageMaxReset();
  deleteActiveCopy();
  envelope0.noteOff();
  allOff();
  autoSave();
  beat = 1;
  SMP.page = 1;
  waitForFourBars = false;
  yield();
}


//receive Midi notes and velocity, map to note array. if not playing, play the note
void handleNoteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  int currentbeat = beat;
  unsigned int mychannel = SMP.currentChannel;
  if (mychannel < 1 || mychannel > maxFiles) return;

  unsigned int livenote = (mychannel + 1) + pitch - 60;  // set Base to C3

  // Adjust for missing octaves
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;

  if (livenote >= 1 && livenote <= 16) {
    if (isNowPlaying) {
      if (SMP.singleMode) {
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - beatStartTime;
        unsigned int effectiveBeat = currentbeat;

        // If the note arrives late, assign it to the next beat
        if (elapsedTime > playNoteInterval / 2) {
          effectiveBeat = (beat % (maxX * lastPage)) + 1;
        }

        // Assign the note to the effective beat
        note[effectiveBeat][livenote][0] = mychannel;
        note[effectiveBeat][livenote][1] = velocity;

        // Mark the note as active
        activeNotes[pitch] = true;

        // Play the note immediately
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
      } else {
        // Play the note without assigning it to the grid
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
      }
    } else {
      // If not playing, light up the LED and play the note
      light(mapXtoPageOffset(SMP.x), livenote, CRGB(0, 0, 255));
      FastLEDshow();
      _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, true, true);
    }
  }
}

void handleNoteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  if (isNowPlaying) {
    if (!SMP.singleMode) {
      int mychannel = SMP.currentChannel;

      // Only stop the note if it is currently active
      if (activeNotes[pitch]) {
        _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity, false, false);
        activeNotes[pitch] = false;  // Mark the note as inactive
      }
    }
  }
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
      midiTimer.update((((60 * 1000 / SMP.bpm) / 4) * 1000) / 24);
      clockCount = 0;
      totalInterval = 0;
    }
  }
  lastClockTime = now;
}



void showIcons(String ico, CRGB colors) {
  const uint8_t(*iconArray)[2] = nullptr;  // Change to const int

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
  } else if (ico == "icon_loadsave2") {
    iconArray = icon_loadsave2;
    size = sizeof(icon_loadsave2) / sizeof(icon_loadsave2[0]);
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
  } else if (ico == "helper_exit") {
    iconArray = helper_exit;
    size = sizeof(helper_exit) / sizeof(helper_select[0]);
  } else if (ico == "helper_exit") {
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
  } else if (ico == "icon_settings") {
    iconArray = icon_settings;
    size = sizeof(icon_settings) / sizeof(icon_settings[0]);
  }

  else if (ico == "icon_rec") {
    iconArray = icon_rec;
    size = sizeof(icon_rec) / sizeof(icon_rec[0]);
  } else if (ico == "icon_rec2") {
    iconArray = icon_rec2;
    size = sizeof(icon_rec2) / sizeof(icon_rec2[0]);
  }

  if (iconArray != nullptr) {
    for (unsigned int gx = 0; gx < size; gx++) {
      light(iconArray[gx][0], maxY - iconArray[gx][1], colors);
    }
  }
}