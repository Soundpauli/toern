
//extern "C" char *sbrk(int incr);
#define FASTLED_ALLOW_INTERRUPTS 1
#define SERIAL8_RX_BUFFER_SIZE 256  // Increase to 256 bytes
#define SERIAL8_TX_BUFFER_SIZE 256  // Increase if needed for transmission
#define TargetFPS 30

//#define AUDIO_SAMPLE_RATE_EXACT 44117.64706
#include <stdint.h>
#include "3x5.h"
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
#include <TeensyPolyphony.h>
#include "audioinit.h"
#include <FastTouch.h>


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

#define maxPages 8
#define maxFiles 9  // 9 samples, 2 Synths
#define maxFilters 15
#define maxfilterResolution 32

#define numPulsesForAverage 24  // Number of pulses to average over
#define pulsesPerBar (24 * 4)   // 24 pulses per quarter note, 4 quarter notes per bar
//char* menuText[14] = {"","FlLE","PACK","WAVE", "REC","SET","MlDl","LlNE","MlC","CH","+c","+v","DEL?","SHlFt","EFX"};

struct MySettings : public midi ::DefaultSettings {
  static const long BaudRate = 31250;
  static const unsigned SysExMaxSize = 256;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial8, MIDI, MySettings);

char *menuText[5] = { "DAT", "KIT", "WAV", "REC", "SET" };
int lastFile[9] = { 0 };
bool freshPaint, tmpMute = false;

unsigned int menuPosition = 1;
String oldPosString, posString = "1:2:";
String buttonString, oldButtonString = "0000";
unsigned long playStartTime = 0;  // To track when play(true) was last called

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

/*timers*/
volatile unsigned int lastButtonPressTime = 0;
volatile bool resetTimerActive = false;

// runtime
//  variables for program logic
volatile float pulse = 1;
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

volatile unsigned int pagebeat, beat = 1;
unsigned int samplePackID, fileID = 1;
EXTMEM unsigned int lastPreviewedSample[FOLDER_MAX] = {};
IntervalTimer playTimer;
IntervalTimer midiTimer;
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
Mode singleMode = { "SINGLE", { 1, 1, 0, 1 }, { maxY, maxX, maxfilterResolution, maxY }, { 1, 2, maxfilterResolution, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode volume_bpm = { "VOLUME_BPM", { 1, 6, VOL_MIN, BPM_MIN }, { 1, 25, VOL_MAX, BPM_MAX }, { 1, 6, 9, 100 }, { 0x000000, 0xFFFFFF, 0xFF4400, 0x00FFFF } };  //OK
//filtermode has 4 entries
Mode filterMode = { "FILTERMODE", { 1, 1, 1, 1 }, { 100, 100, 100, 100}, { 100, 1, 100, 1}, { 0x000000, 0x000000, 0x000000, 0x000000 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 0, 0, 7 }, { 9, 0, 0, 9 }, { 8, 0, 0, 8 }, { 0xFFFF00, 0x000000, 0x000000, 0xFFFFFF } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { maxY, 1, maxY, maxY }, { maxY, 1, 10, 10 }, { 0xFF4400, 0x000000, 0x0044FF, 0x888888 } };

Mode set_Wav = { "SET_WAV", { 1, 1, 1, 1 }, { 999, FOLDER_MAX, 999, 999 }, { 0, 0, 0, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
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
  unsigned int filter_setting[maxY][maxFilters];
  unsigned int mute[maxY];
  unsigned int channelVol[16];
};

//EXTMEM?
volatile Device SMP = { 
  false, //singleMode
  1, //currentChannel
  10, //volume
  100, //bpm
  10, //velocity
  1, //page
  1,  //edit
  1, //file
  1, //pack
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  //sampleID
  0,  //folder
  false,  //activeCopy
  1,  //x
  2, //y
  0, //seek
  0, //seekEnd
  0, //sampleLen
  0, //shiftX
  0, //shiftY
  {}, //filter_setting
  {}, //mute
  { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 } //channelVol
  };

enum FilterType {ATTACK, DELAY, SUSTAIN, RELEASE, WAVEFORM, MAX_TYPE}; // Define filter types

// Define 2D array where each row represents a button's cycling values
const FilterType buttonValues[][4] = {
    {}, // Button 1 (no cycling values)
    {}, // Button 2 (no cycling values)
    {}, // Button 3 (no cycling values)
    {RELEASE, WAVEFORM, MAX_TYPE, MAX_TYPE} // Button 4 cycles through {RELEASE, WAVEFORM}
};

// ADSR array that stores the currently selected value for each button
FilterType adsr[4] = {ATTACK, DELAY, SUSTAIN, RELEASE}; // Button 4 starts at RELEASE

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
    if (SMP.singleMode) {
      mixer1.gain(0, mapf(SMP.channelVol[SMP.currentChannel], 1, maxY, 0, 1));
      mixer1.gain(2, mapf(SMP.channelVol[SMP.currentChannel], 1, maxY, 0, 1));
      mixer1.gain(3, mapf(SMP.channelVol[SMP.currentChannel], 1, maxY, 0, 1));
      mixer1.gain(4, mapf(SMP.channelVol[SMP.currentChannel], 1, maxY, 0, 1));
    }
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
  Serial.println(newMode->name);
  oldButtonString = buttonString;
  if (newMode != currentMode) {
    currentMode = newMode;
    // Set last saved values for encoders
    for (int i = 0; i < NUM_ENCODERS; i++) {
      Encoder[i].writeRGBCode(currentMode->knobcolor[i]);
      Encoder[i].writeMax((int32_t) currentMode->maxValues[i]);  //maxval
      Encoder[i].writeMin((int32_t) currentMode->minValues[i]);  //minval

      if ((currentMode == &singleMode && oldMode == &draw) || (currentMode == &draw && oldMode == &singleMode)) {
        //do not move Cursor for those modes
        //
      } else {
        Encoder[i].writeCounter((int32_t) currentMode->pos[i]);
      }

        if (currentMode== &filterMode){
         adsr[0]=ATTACK;
         adsr[1]=DELAY;
         adsr[2]=SUSTAIN; 
         adsr[3]=RELEASE;
         Encoder[i].writeCounter((int32_t) SMP.filter_setting[SMP.currentChannel][i]);
      }
    }
  }
  drawPlayButton(0);
}

void drawPlayButton(unsigned int timer) {

  if (currentMode == &draw || currentMode == &singleMode) {
    if (isPlaying) {
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
  if (currentMode == &singleMode && buttonString == "2200") {
    SMP.shiftX = 8;
    SMP.shiftY = 8;

    Encoder[3].writeCounter((int32_t) 8);
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
    if (isPlaying) {
      play(false);
    } else {
    }
    switchMode(&volume_bpm);
  }

  // Switch to volume mode in draw or single mode
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0020") {
    if (isPlaying) {
      play(false);
    } else {
    }
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

    Encoder[0].writeCounter((int32_t) velo);
  }

  // Handle velocity switch in draw mode
  if (!freshPaint && note[SMP.x][SMP.y][0] != 0 && (currentMode == &draw) && buttonString == "0002") {
    unsigned int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 127, 1, maxY));
    unsigned int chvol = SMP.channelVol[SMP.currentChannel];
    Serial.println(velo);
    SMP.velocity = velo;
    SMP.singleMode = false;
    switchMode(&velocity);

    Encoder[0].writeCounter((int32_t) velo);
    Encoder[3].writeCounter((int32_t) chvol);
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
if (currentMode == &filterMode && buttonString == "0001") {
    cycleFilterTypes(3);
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
  }

  // Set SamplePack + Load + Save + Exit
  if ((currentMode == &draw) && buttonString == "2200") {
    switchMode(&set_SamplePack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "0100") {
    saveSamplePack(SMP.pack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "1000") {
    loadSamplePack(SMP.pack,false);
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



void cycleFilterTypes(int button) {
    int numValues = 0;
    // If the button has no cycling values, do nothing
    if (buttonValues[button][0] == MAX_TYPE || buttonValues[button][0] == 0) return;
    // Count valid entries in buttonValues[button] (ignoring MAX_TYPE)
    while (numValues < 4 && buttonValues[button][numValues] != MAX_TYPE) {
        numValues++;
    }
    // Find current position of adsr[button] in buttonValues[button]
    for (int i = 0; i < numValues; i++) {
        if (adsr[button] == buttonValues[button][i]) {
            // Move to the next value, wrapping around
            adsr[button] = buttonValues[button][(i + 1) % numValues];
            Serial.print("Button ");
            Serial.print(button + 1);
            Serial.print(" toggled to: ");
            Serial.println(adsr[button]);
            
            //TOOGLE BETWEEN RELEASE AND WF
              if (adsr[3] == WAVEFORM)  {
                Serial.println("---> WAVE");
                Serial.println(SMP.filter_setting[SMP.currentChannel][WAVEFORM]);
                Encoder[3].writeMax((int32_t) 5);  
                Encoder[3].writeCounter((int32_t) SMP. filter_setting[SMP.currentChannel][WAVEFORM]);
                }
              if (adsr[3] == RELEASE) { 
                Serial.println("---> RELEASE");
                Serial.println(SMP.filter_setting[SMP.currentChannel][RELEASE]);
                Encoder[3].writeMax((int32_t) 100);   // current maxvalues
                Encoder[3].writeCounter((int32_t) SMP.filter_setting[SMP.currentChannel][RELEASE] );
              }
          return;
        }
    }
}

void intro() {
  randomSeed(analogRead(0));
  int rand = random(0, 6);
  for (int i = 0; i < 9; i++) {
    FastLED.clear();
    drawText(welcome[rand][i], 1, 11 - i, col_Folder[i]);
    FastLED.show();
    delay(300);
  }
  FastLED.clear();
}



void setup(void) {
  //NVIC_SET_PRIORITY(IRQ_LPUART8, 0);
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
    Encoder[i].writeFadeRGB(0);
    delay(50);
    Encoder[i].updateStatus();
  }
  switchMode(&draw);
  Serial.print("Initializing SD card...");
  drawNoSD();

  getLastFiles();
  loadSamplePack(samplePackID,true);

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


  AudioInterrupts();

  // set BPM:100
  SMP.bpm = 100;
  playTimer.begin(playNote, playNoteInterval);
  playTimer.priority(230);

  midiTimer.begin(checkMidi, playNoteInterval / 24);

  // turn on the output
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.9);

  sgtl5000_1.unmuteLineout();
  // According to info in libraries\Audio\control_sgtl5000.cpp
  // 31 is LOWEST output of 1.16V and 13 is HIGHEST output of 3.16V
  sgtl5000_1.lineOutLevel(1);
  AudioMemory(24);
  autoLoad();
  switchMode(&draw);


  MIDI.begin(MIDI_CH);
  //Serial8.begin(57600);
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

  if (currentMode == &draw || currentMode == &singleMode) {
    if (posString != oldPosString) {
      oldPosString = posString;
      SMP.x = currentMode->pos[3];
      SMP.y = currentMode->pos[0];
    }
    if (currentMode == &draw) {
      // offset y by one (notes (channel 1/red) start at row #1)
      SMP.currentChannel = SMP.y -1;
    } 

    Encoder[0].writeRGBCode(CRGBToUint32(col[SMP.currentChannel]));
   Encoder[3].writeRGBCode(CRGBToUint32(col[SMP.currentChannel]));
    //WHY???
    //SMP.edit = 1;

    if (paintMode) {
      note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = SMP.currentChannel;
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
        // pitch = MIDI.getData1();
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


void loop() {




  drawPlayButton(pagebeat);
  checkSingleTouch();
  checkMenuTouch();




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
  SMP.edit = 1;


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

    if (isPlaying) {
      drawTimer(pagebeat);
    }
  }

  if (currentMode == &draw || currentMode == &singleMode || currentMode == &velocity) {

    drawBase();

    drawTriggers();
    if (currentMode != &velocity)
      drawCursor();
    if (isPlaying) {
      drawTimer(pagebeat);
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
  delay(50);  // 50????
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
    drawLoadingBar(1, 999, lastFile[f], col_base[f], CRGB(15, 15, 55),false);
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

    Encoder[3].writeCounter((int32_t) 8);
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

    Encoder[0].writeCounter((int32_t) 8);
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
    int channel =  SMP.currentChannel;
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
    pagebeat = 1;
    SMP.page = 1;
    Encoder[2].writeRGBCode(0xFFFF00);
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
  Encoder[2].writeRGBCode(0x005500);
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
      //yield();
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
  SMP.smplen = 0;

  if (previewSample) {
    int fileSize = previewSample.size();

    if (firstPreview) {
      fileSize = min(previewSample.size(), 882000); //max 10SEC preview // max preview len =  X Sec. //toDO: make it better to preview long files
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

    int startOffset = 1 + (200 * SMP.seek);  // Start offset in milliseconds
    int endOffset = (200 * SMP.seekEnd);     // End offset in milliseconds

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
    SMP.smplen = plen;

    // only set the first time to get seekEnd
    if (setMaxSampleLength == true) {
      sampleLengthSet = true;
      SMP.seekEnd = (SMP.smplen / (PrevSampleRate * 2) / 200);
      currentMode->pos[2] = SMP.seekEnd;
      //SET ENCODER SEARCH_END to last byte
      Encoder[2].writeCounter((int32_t) SMP.seekEnd);
    }

    previewSample.close();
    displaySample(SMP.smplen);

    _samplers[0].addSample(36, (int16_t *)sampled[0], (int)(plen / 2), 1);  //-44?

    Serial.println("NOTE");
    _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
  }
}


void drawLoadingBar(int minval, int maxval, int currentval, CRGB color, CRGB fontColor, bool intro) {
  int ypos = 4;
  int height=2;
   int barwidth = mapf(currentval, minval, maxval, 1, maxX);
    for (int x = 1; x <= maxX; x++) {
      light(x, ypos - 1, fontColor);
    }
    //draw the border-ends
    light(1, ypos, fontColor);
    light(maxX, ypos, fontColor);
  
  for (int x = 2; x <= maxX-1; x++) {
    for (int y = 0; y <= (height-1); y++) {
      if (x <= barwidth+1) {
        light(x, ypos + y, color);
      } else {
        light(x, ypos + y, CRGB(0, 0, 0));
      }
    }
  }
  if(!intro){ 
    showNumber(currentval, fontColor, 11);
  }else{
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

    unsigned int startOffset = 1 + (200 * SMP.seek);                   // Start offset in milliseconds
    unsigned int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)

    unsigned int endOffset = 200 * SMP.seekEnd;  // End offset in milliseconds
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
  //if (vol <= 1.0) sgtl5000_1.volume(vol);
  // setvol = true;
}

void updateBrightness() {
  ledBrightness = (currentMode->pos[1] * 10) + 4;
  Serial.println("Brightness: " + String(ledBrightness));
}

void updateBPM() {
  // setvol = false;
  Serial.println("BPM: " + String(currentMode->pos[3]));
  SMP.bpm = currentMode->pos[3];
  playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
  playTimer.update(playNoteInterval);
  midiTimer.update(playNoteInterval / 24);
  drawBPMScreen();
}

void setVolume() {
  drawBPMScreen();

  if (currentMode->pos[1] * 10 != ledBrightness) {
    updateBrightness();
  }

  if (currentMode->pos[2] != SMP.vol) {
    updateVolume();
  }


  if (currentMode->pos[3] != SMP.bpm) {
    updateBPM();
  }
}



void setFilters() {
  // Encoder 1: Adjust attack
  if (currentMode->pos[0] != SMP.filter_setting[SMP.currentChannel][ATTACK]) {
    SMP.filter_setting[SMP.currentChannel][ATTACK] = currentMode->pos[0];
    envelope13.attack(mapf(SMP.filter_setting[SMP.currentChannel][ATTACK], 1, currentMode->maxValues[1], 0, 11880));
    Serial.println("attack: " + String(SMP.filter_setting[SMP.currentChannel][ATTACK]));
  }


  // Encoder2 : Adjust delay
  if (currentMode->pos[1] != SMP.filter_setting[SMP.currentChannel][DELAY]) {
    SMP.filter_setting[SMP.currentChannel][DELAY] = currentMode->pos[1];
    envelope13.delay(mapf(SMP.filter_setting[SMP.currentChannel][DELAY], 1, currentMode->maxValues[2], 0, 3000));
    Serial.println("delay: " + String(SMP.filter_setting[SMP.currentChannel][DELAY]));
  }

  // Encoder 3: Adjust sustain
    if (currentMode->pos[2] != SMP.filter_setting[SMP.currentChannel][SUSTAIN]) {
     SMP.filter_setting[SMP.currentChannel][SUSTAIN] = currentMode->pos[2];
    float mappedSustain = mapf(SMP.filter_setting[SMP.currentChannel][SUSTAIN], 1, currentMode->maxValues[3], 0, 1.0);  // Sustain range: 0.1s to 2.0s
    envelope13.sustain(mappedSustain);
    Serial.println("sustain: " + String(mappedSustain) + " seconds");
    }

    if (adsr[3] == RELEASE){
      // Encoder 4: Adjust release
      if (currentMode->pos[3] != SMP.filter_setting[SMP.currentChannel][RELEASE]) {
        SMP.filter_setting[SMP.currentChannel][RELEASE] = currentMode->pos[3];
        float mappedRelease = mapf(SMP.filter_setting[SMP.currentChannel][RELEASE], 1, currentMode->maxValues[3], 0, 1.0);  // Release range: 0.1s to 2.0s
        envelope13.sustain(mappedRelease);
        Serial.println("release: " + String(mappedRelease) + " seconds");
        }
      }


  if (adsr[3] == WAVEFORM){
  // Encoder 4: Adjust waveform
  if (currentMode->pos[3] != SMP.filter_setting[SMP.currentChannel][WAVEFORM]) {
  SMP.filter_setting[SMP.currentChannel][WAVEFORM] = currentMode->pos[3];

  unsigned int wav = mapf(SMP.filter_setting[SMP.currentChannel][WAVEFORM], 0, 4, 1, 4);  // Release range: 0.1s to 2.0s
   
Serial.print("wv=");
    Serial.println(wav);

    switch (wav) {
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
    drawADSR();
  
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
  if (intro) renderRainbowWave(2500);
  for (unsigned int z = 1; z < maxFiles; z++) {
    
    if (!intro){
    FastLEDclear();
    showIcons("icon_sample", CRGB(20, 20, 20));
    }

    drawLoadingBar(1, maxFiles, z, col_base[(maxFiles+1)-z], CRGB(50, 50, 50),intro);
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
  //drawNoSD();
  FastLEDclear();

  /*
  showIcons("helper_select", col[SMP.currentChannel]);
  showIcons("helper_load", CRGB(0, 20, 0));
  showIcons("helper_seek", CRGB(10, 0, 0));
  showIcons("helper_folder", CRGB(10, 10, 0));
  */


  int snr = SMP.wav[SMP.currentChannel][1];
  int fnr = getFolderNumber(snr);
  char OUTPUTf[50];
  sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
  showNumber(snr, col_Folder[fnr], 11);
  displaySample(SMP.smplen);


  //:::::::: STARTPOSITION SAMPLE  ::::: //
  if (sampleIsLoaded && currentMode->pos[0] != SMP.seek) {
    SMP.seek = currentMode->pos[0];
    Serial.println(SMP.seek);

    //STOP ALREADY PLAYING
    _samplers[0].removeAllSamples();
    envelope0.noteOff();
    if (sampleFile) sampleFile.seek(44);

    if (SD.exists(OUTPUTf)) {
      previewSample(fnr, getFileNumber(snr), false, false);
    }
  }


  //::::::: FOLDER ::::: //
  if (currentMode->pos[1] != SMP.folder) {
    SMP.folder = currentMode->pos[1];
    Serial.println("Folder: " + String(SMP.folder - 1));
    SMP.wav[SMP.currentChannel][1] = ((SMP.folder - 1) * 100) + 1;
    Serial.print("WAV:");
    Serial.println(SMP.wav[SMP.currentChannel][1]);
    Encoder[3].writeCounter((int32_t) SMP.wav[SMP.currentChannel][1]);
  }


  //::::::: ENDPOSITION SAMPLE  ::::: //
  if (sampleIsLoaded && (currentMode->pos[2]) != SMP.seekEnd) {
    SMP.seekEnd = currentMode->pos[2];
    Serial.println("seekEnd:");
    Serial.println(SMP.seekEnd);

    //STOP ALREADY PLAYING
    _samplers[0].removeAllSamples();
    envelope0.noteOff();
    if (sampleFile) sampleFile.seek(44);

    if (SD.exists(OUTPUTf)) {
      if (!sampleLengthSet) previewSample(fnr, getFileNumber(snr), false, false);
    }
    sampleLengthSet = false;
  }



  // GET SAMPLEFILE
  if (currentMode->pos[3] != snr) {
    sampleIsLoaded = false;
    SMP.wav[SMP.currentChannel][1] = currentMode->pos[3];

    int snr = SMP.wav[SMP.currentChannel][1];
    int fnr = getFolderNumber(snr);
    showNumber(snr, col_Folder[fnr], 11);

    Serial.println("File: " + String(fnr) + " / " + String(getFileNumber(snr)));

    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));
    //check if exists?

    // reset SEEK and stop sample playing
    SMP.smplen = 0;
    currentMode->pos[0] = 0;
    SMP.seek = 0;
    Encoder[0].writeCounter((int32_t)0);
    //_samplers[0].removeAllSamples();
    //envelope0.noteOff();
    //if (sampleFile)sampleFile.seek(44);

    if (!sampleIsLoaded) {
      //lastPreviewedSample[fnr] = snr;
      previewSample(fnr, getFileNumber(snr), true, true);
      sampleIsLoaded = true;
    }
  }
}



void renderRainbowWave(int durationMs) {
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
      if (logo[5-y][x] == 1) { light(x + 1, 8+y , color); }
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

void drawADSR() {
  Serial.println(SMP.currentChannel);
    FastLEDclear();

  CRGB volColor = CRGB(SMP.vol * SMP.vol, 20 - SMP.vol, 0);
  showIcons("helper_exit", CRGB(0, 0, 20));

  unsigned int attack =  mapf(SMP.filter_setting[SMP.currentChannel][ATTACK],0,currentMode->maxValues[0],maxY-5,1);
  unsigned int delay =  mapf(SMP.filter_setting[SMP.currentChannel][DELAY],0,currentMode->maxValues[1],1,maxY-6);
  unsigned int sustain =  mapf(SMP.filter_setting[SMP.currentChannel][SUSTAIN],0,currentMode->maxValues[2],1,maxY-6);

  unsigned int release = mapf(SMP.filter_setting[SMP.currentChannel][RELEASE],0,currentMode->maxValues[3],1,maxY-6);
  unsigned int waveform =  mapf(SMP.filter_setting[SMP.currentChannel][WAVEFORM], 1, 4, 1, maxY-8);

    //GLOBAL
  
    int max=0;
    for (unsigned int x = 1; x <= 3; x++) {
      for (unsigned int y = 1; y <= attack; y++) {
        light(x, y, CRGB(y * y, 20 - y, 0));
        max = y;
      }

       light(x, max, CRGB(50,50,50));
    }

    for (unsigned int x = 5; x <= 7; x++) {
      for (unsigned int y = 1; y <= delay; y++) {
        light(x, y, CRGB(0, 20 - y, y * y));
        max = y;
      }
      light(x, max, CRGB(50,50,50));
    }

    for (unsigned int x = 10; x <= 12; x++) {
      for (unsigned int y = 1; y <= sustain; y++) {
        light(x, y, CRGB(20 - y, y * y, 0));
        max = y;
      }
      light(x, max, CRGB(50,50,50));
    }
  
  
  if (adsr[3]==RELEASE){
    for (unsigned int x = 14; x <= 16; x++) {
      for (unsigned int y = 1; y <= release; y++) {
        light(x, y, CRGB(y * y,0 , 20 - y));
         max = y;
      }
      light(x, max, CRGB(50,50,50));
    }
  }
  if (adsr[3]==WAVEFORM){
     for (unsigned int x = 14; x <= 16; x++) {
      for (unsigned int y = 1; y <= waveform; y++) {
        light(x, y, CRGB(y * y,0 , 20 - y));
         max = y;
      }
      light(x, max, CRGB(50,50,50));
    }
} 

  showText("A", 1, 12, CRGB(50, 50, 50));
  showText("D", 5, 12, CRGB(50, 50, 50));
  showText("S", 10, 12, CRGB(50, 50, 50));
  if (adsr[3]==WAVEFORM) showText("W", 14, 12, CRGB(50, 50, 50));
  if (adsr[3]==RELEASE) showText("R", 14, 12, CRGB(50, 50, 50));
}


void drawBase() {
  if (!SMP.singleMode) {
    unsigned int colors = 0;
    for (unsigned int y = 1; y < maxY; y++) {
      //unsigned int filtering = 2;  // mapf(SMP.filter_setting[SMP.currentChannel][y - 1], 0, maxfilterResolution, 50, 5);
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
  light(SMP.x, SMP.y, CRGB(255 - (int)pulse, 255 - (int)pulse, 255 - (int)pulse));
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
    midiTimer.update(playNoteInterval / 24);
    bpm_vol->pos[2] = SMP.vol;


    float vol = float(SMP.vol / 10.0);
    //if (vol <= 1.0) sgtl5000_1.volume(vol);
    // set all Filters
    //for (unsigned int i = 0; i < maxFilters; i++) {
    //filters[i]->frequency(100 * SMP.filter_setting[SMP.currentChannel][i]);
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
      //Serial.print(SMP.filter_setting[SMP.currentChannel][i]);
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
  unsigned int mychannel;
  // return if out of range
  if (!SMP.singleMode) {
    mychannel = SMP.currentChannel;
    if (mychannel < 1 || mychannel > maxFiles) return;
  } else {
    mychannel = SMP.currentChannel;
  }
  //  envelopes[SMP.currentChannel]->release(11880 / 2);
  unsigned int livenote = (mychannel + 1) + pitch - 60;  // set Base to C3

  // fake missing octaves (only 16 notes visible, so step up/down an octave!)
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;

  if (livenote >= 1 && livenote <= 16) {
    if (isPlaying) {
      if (!SMP.mute[SMP.currentChannel]) {
        note[beat][livenote][0] = mychannel;
        note[beat][livenote][1] = velocity;
      }
    } else {
      light(SMP.x, livenote, CRGB(0, 0, 255));
      FastLEDshow();
      _samplers[mychannel].noteEvent(((SampleRate[mychannel] * 12) + pitch - 60), velocity * 8, true, true);
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