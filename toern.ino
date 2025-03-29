//extern "C" char *sbrk(int incr);
#define FASTLED_ALLOW_INTERRUPTS 1
#define SERIAL8_RX_BUFFER_SIZE 256  // Increase to 256 bytes
#define SERIAL8_TX_BUFFER_SIZE 256  // Increase if needed for transmission
#define TargetFPS 30

#define AUDIO_BLOCK_SAMPLES  128
//#define AUDIO_SAMPLE_RATE_EXACT 44100
#include <stdint.h>
#include <Wire.h>
#include "Arduino.h"
#include <math.h>
#include <Mapf.h>

#include <WS2812Serial.h>  // leds
#define USE_WS2812SERIAL   // leds
#include <FastLED.h>       // leds

#include <i2cEncoderLibV2.h>
#include <MIDI.h>
#include <Audio.h>
#include <EEPROM.h>
#include <FastTouch.h>
#include <TeensyPolyphony.h>

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
#define SWITCH_2 41  // Pin for TPP223 1
#define VOL_MIN 1
#define VOL_MAX 10
#define BPM_MIN 40
#define BPM_MAX 300

#define GAIN1 1
#define GAIN2 0.5
#define GAIN3 0.33
#define GAIN4 0.25

#define NUM_ENCODERS 4
#define defaultVelocity 63
#define FOLDER_MAX 9
#define maxPages 16
#define maxFiles 8

#define maxFilters 15

#define maxfilterResolution 64
#define numPulsesForAverage 8  // Number of pulses to average over
#define pulsesPerBar (24 * 4)   // 24 pulses per quarter note, 4 quarter notes per bar

struct MySettings : public midi ::DefaultSettings {
  static const long BaudRate = 31250;
  static const unsigned SysExMaxSize = 256;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial8, MIDI, MySettings);
unsigned long beatStartTime = 0;  // Timestamp when the current beat started

#define CLOCK_BUFFER_SIZE 24



// Number of samples in each delay line
// Allocate the delay lines for left and right channels
#define FLANGE_DELAY_LENGTH (12 * AUDIO_BLOCK_SAMPLES)
short delayline[FLANGE_DELAY_LENGTH];

bool MIDI_CLOCK_SEND = false;




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
Particle particles[256];  // up to 256 possible "on" pixels
int particleCount = 0;
static bool bypassSet = false;
bool activeNotes[128] = { false };  // Track active MIDI notes (0-127)

float rateFactor = 44117.0 / 44100.0;

char *menuText[6] = { "DAT", "KIT", "WAV", "REC", "BPM", "SET" };
int lastFile[9] = { 0 };
bool freshPaint, tmpMute = false;
bool firstcheck = false;
bool nofile = false;
char *currentFilter = "ADSR";
char *currentDrum = "TONE";
unsigned int fxType = 0;
unsigned int drum_type = 0;

unsigned int selectedFX = 0;

unsigned long filterchecktime = 0;
unsigned long filterDrawEndTime = 0;
bool filterDrawActive = false;

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
int recMode = 1;


unsigned long recordStartTime = 0;
const unsigned long recordDuration = 5000;  // 5 seconds
bool isRecording = false;
File frec;


// which input on the audio shield will be used?
//const int myInput = AUDIO_INPUT_LINEIN;
unsigned int recInput = AUDIO_INPUT_MIC;

/*timers*/
volatile unsigned int lastButtonPressTime = 0;
volatile bool resetTimerActive = false;

// runtime
//  variables for program logic
float pulse = 1;
volatile int dir = 1;
unsigned int MIDI_CH = 1;
float playNoteInterval = 150000.0;
volatile unsigned int RefreshTime = 1000 / TargetFPS;
float marqueePos = maxX;
bool shifted = false;
bool movingForward = true;  // Variable to track the direction of movement
unsigned volatile int lastUpdate = 0;
volatile unsigned int lastClockTime = 0;
volatile unsigned int totalInterval = 0;
volatile unsigned int clockCount = 0;
bool hasNotes[maxPages + 1];
unsigned int startTime[maxY] = { 0 };    // Variable to store the start time
bool noteOnTriggered[maxY] = { false };  // Flag to indicate if noteOn has been triggered
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

EXTMEM unsigned int sample_len[maxFiles];
bool sampleLengthSet = false;
bool isNowPlaying = false;  // global
int PrevSampleRate = 1;
volatile EXTMEM int SampleRate[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
volatile EXTMEM unsigned char sampled[maxFiles][ram / (maxFiles + 1)];


// makes instrument only play once per step
bool BDon; //is the BASE DRUM on?
bool SNon; //is the SNARE on?
bool HHon; //is the HI HAT on?


int kickLed = 70;
int snareLed = 70;
int hihatLed = 70;

//instrument volume
float kickVol = 1;
float snareVol = 1;
float hihatVol = 1;


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
Mode filterMode = { "FILTERMODE", { 0, 0, 1, 0 }, { 7, 3, 8, maxfilterResolution }, { 1, 1, 1, 1 }, { 0x000000, 0x000000, 0x000000, 0x000000 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 7, 0, 7 }, { 9, 9, maxfilterResolution, 9 }, { 8, 8, maxfilterResolution, 8 }, { 0xFFFF00, 0x000000, 0x000000, 0xFFFFFF } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { maxY, 1, maxY, maxY }, { maxY, 1, 10, 10 }, { 0xFF4400, 0x000000, 0x0044FF, 0x888888 } };

Mode set_Wav = { "SET_WAV", { 1, 1, 1, 1 }, { 9999, FOLDER_MAX, 9999, 999 }, { 0, 0, 0, 2 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 1 }, { 1, 1, 99, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode loadSaveTrack = { "LOADSAVE_TRACK", { 1, 1, 1, 1 }, { 1, 1, 1, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };
Mode menu = { "MENU", { 1, 1, 1, 1 }, { 1, 1, 1, 6 }, { 1, 1, 1, 1 }, { 0x000000, 0xFFFFFF, 0x00FF00, 0x000000 } };

// Declare currentMode as a global variable
Mode *currentMode;



// Declare the device struct
struct Device {
  unsigned int singleMode;  // single Sample Mod
  unsigned int currentChannel;
  unsigned int vol;           // volume
  float bpm;           // bpm
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
  float drum_settings[maxY][4];

  unsigned int selectedFilter;
  unsigned int selectedDrum;

  unsigned int mute[maxY];
  unsigned int channelVol[16];
};


//EXTMEM?
volatile Device SMP = {
  false,                                               //singleMode
  1,                                                   //currentChannel
  10,                                                  //volume
  100.0,                                                 //bpm
  10,                                                  //velocity
  1,                                                   //page
  1,                                                   //edit
  1,                                                   //file
  1,                                                   //pack
  {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}},  //wav preset
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
  0,//selectedParameter
  {},  //filter_settings
  {}, //drum_settings
  0, //selectedFilter
  0, //selectedDrum
  {},                                                                 //mute
  { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }  //channelVol
};

#define NUM_PARAMS (sizeof(SMP.param_settings[0]) / sizeof(SMP.param_settings[0][0]))
#define NUM_FILTERS (sizeof(SMP.filter_settings[0]) / sizeof(SMP.filter_settings[0][0]))
#define NUM_DRUMS (sizeof(SMP.drum_settings[0]) / sizeof(SMP.drum_settings[0][0]))


enum VoiceSelect {
};  // Define filter types

enum ParameterType { TYPE,  //AudioSynthNoiseWhite, AudioSynthSimpleDrums OR WAVEFORM
                     WAVEFORM,
                     DELAY,
                     ATTACK,
                     HOLD,
                     DECAY,
                     SUSTAIN,
                     RELEASE,
                     LENGTH,
                     SECONDMIX,
                     PITCHMOD
};  //Define filter types
int maxParamVal[12] = {0,0, 250,2000,1000,1000,1,1000,1000,1,1};




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


enum MidiSetTypes { 
                 MIDI_IN_BOOL,
                 MIDI_OUT_BOOL,
                 SINGLE_OUT_CHANNEL,
                 GLOBAL_IN_CHANNEL,
                 SEND_CTRL_BOOL,
                 RECEIVE_CTRL_BOOL,
};  //Define midi types

FilterType defaultFilter[maxFiles] = { 1 };

char *activeParameterType[8] =  { "TYPE", "WAV", "DLAY", "ATTC", "HOLD", "DCAY", "SUST", "RLSE" };
char *activeFilterType[9] =    { "", "LOW", "HIGH", "FREQ", "RVRB", "BITC", "FLNG", "DTNE", "OCTV" };
char *activeDrumType[4] =     { "TONE", "DCAY", "FREQ", "TYPE" };
char *activeMidiSetType[6] =     { "IN", "OUT", "OUT", "INPT","SCTL", "RCTL" };
/*
enum EffectType {
  EFFECT_FREEVERB,
  EFFECT_FLANGE,
  EFFECT_BITCRUSHER,
  EFFECT_FILTER
};
*/

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
AudioPlayArrayResmp *voices[] = { &sound0, &sound1, &sound2, &sound3, &sound4, &sound5, &sound6, &sound7, &sound8 };
AudioSynthKarplusStrong *strings[] = { 0,0,0,0,0,0,0,0,0,0,0, &string11, &string12, &string13, &string14};

AudioEffectEnvelope *envelopes[] = { &envelope0, &envelope1, &envelope2, &envelope3, &envelope4, &envelope5, &envelope6, &envelope7, &envelope8, nullptr, nullptr, &envelope11, &envelope12, &envelope13, &envelope14 };
AudioAmplifier *amps[] = { &amp0, &amp1, &amp2, &amp3, &amp4, &amp5, &amp6, &amp7, &amp8, nullptr, nullptr, &amp11, &amp12, &amp13, &amp14 };
AudioFilterStateVariable *filters[] = { nullptr, &filter1, &filter2, &filter3, &filter4, &filter5, &filter6, &filter7, &filter8, nullptr, nullptr, &filter11, &filter12, &filter13, &filter14 };
AudioMixer4 *filtermixers[] = { nullptr, &filtermixer1, &filtermixer2, &filtermixer3, &filtermixer4, &filtermixer5, &filtermixer6, &filtermixer7, &filtermixer8, nullptr, nullptr, &filtermixer11, &filtermixer12, &filtermixer13, &filtermixer14 };
AudioEffectBitcrusher *bitcrushers[] = { 0, &bitcrusher1, &bitcrusher2, &bitcrusher3, &bitcrusher4, &bitcrusher5, &bitcrusher6, &bitcrusher7, &bitcrusher8, 0, 0, &bitcrusher11, &bitcrusher12, &bitcrusher13, &bitcrusher14 };
AudioEffectFreeverb *freeverbs[] = { 0, &freeverb1, &freeverb2, 0, 0, 0, 0, &freeverb7, &freeverb8, 0, 0, &freeverb11, &freeverb12, &freeverb13, &freeverb14 };
AudioMixer4 *freeverbmixers[] = { 0, &freeverbmixer1, &freeverbmixer2, 0, 0, 0, 0, &freeverbmixer7, &freeverbmixer8, 0, 0, 0, 0, 0, 0 };
AudioEffectFlange *flangers[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &flange11, &flange12, &flange13, &flange14 };
AudioMixer4 *waveformmixers[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &waveformmixer11, &waveformmixer12, &waveformmixer13, &waveformmixer14};


AudioSynthWaveform *synths[15][2] = { 0 };
AudioSynthSimpleDrum *drums[] = { 0,0,0,0,0,0,0,0,0,0,0, &drum11, &drum12, &drum13, &drum14};




void allOff() {
  for (AudioEffectEnvelope *envelope : envelopes) {
    envelope->noteOff();
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


void testDrums(){


  //BD parameters
  BDsine.begin(0.7, 100, WAVEFORM_SINE);
  BDsaw.begin(0.4, 100, WAVEFORM_TRIANGLE);
  envelope1.sustain(0);
  BDpitchEnv.sustain(0);

  //SN parameters
  envelope2.sustain(0);
  filter2.frequency(1000);
  SNnoise.amplitude(0.5);
  SNtone.begin(0.8, 700, WAVEFORM_SINE);
  SNtone2.begin(0.8, 700, WAVEFORM_SINE);
  SNtoneEnv.sustain(0);
  filter2.resonance(2);
  SNchaosMix.gain(1, 0);
  SNtone.frequencyModulation(0);

  //HH parameters
  envelope3.sustain(0);
  filter3.frequency(6000);
  HHnoise.amplitude(0.6);
  HHtone.begin(0.8, 700, WAVEFORM_SQUARE);
  HHtone2.begin(0.8, 700, WAVEFORM_SQUARE);
  HHtoneEnv.sustain(0);
  filter3.resonance(2);
  HHchaosMix.gain(1, 0);
  HHtone.frequencyModulation(6);
  HHtone2.frequencyModulation(6);


  float kickTone   = 512;  // Maps to ~40–120 Hz for the kick
  float kickDecay  = 512;  // Maps to ~50–300 ms decay
  float kickPitch  = 0;  // Adds up to ~34 Hz extra on saw wave
  int   kickType   = 1;    // Choose type 1 (try 1, 2, or 3)

  float snareTone  = 300;  // Controls blend: higher = more tone
  float snareDecay = 400;  // Maps to ~30–150 ms decay
  float snarePitch = 512;  // Sets filter cutoff and tone frequency
  int   snareType  = 2;    // Choose type 2 for snare character

  float hihatTone  = 512;  // Blend between noise and tone for hi-hat
  float hihatDecay = 512;  // Maps to ~10–50 ms decay (very short)
  float hihatPitch = 512;  // Maps to filter frequency (2000–7000 Hz)
  int   hihatType  = 3;    // Choose type 3 for hi-hat sound

  // Trigger each drum sound (in a real sequencer these would be timed)
  KD_drum(kickTone, kickDecay, kickPitch, kickType);
  delay(500);
  SN_drum(snareTone, snareDecay, snarePitch, snareType);
  delay(500);
  HH_drum(hihatTone, hihatDecay, hihatPitch, hihatType);
  delay(500);
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

        //SMP.selectedFX = TYPE;
        //Encoder[3].writeCounter((int32_t)SMP.param_settings[SMP.currentChannel][0]);
      }
    }
  }
  drawPlayButton();
}


void writeWavHeader(File &file, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint16_t blockAlign = numChannels * bitsPerSample / 8;

  // WAV header (44 bytes)
  uint8_t header[44] = {
    'R', 'I', 'F', 'F',
    0, 0, 0, 0, // <- file size - 8 (filled in later)
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,             // PCM chunk size
    1, 0,                   // Audio format (1 = PCM)
    (uint8_t)(numChannels & 0xff), (uint8_t)(numChannels >> 8),
    (uint8_t)(sampleRate & 0xff), (uint8_t)((sampleRate >> 8) & 0xff),
    (uint8_t)((sampleRate >> 16) & 0xff), (uint8_t)((sampleRate >> 24) & 0xff),
    (uint8_t)(byteRate & 0xff), (uint8_t)((byteRate >> 8) & 0xff),
    (uint8_t)((byteRate >> 16) & 0xff), (uint8_t)((byteRate >> 24) & 0xff),
    blockAlign, 0,
    bitsPerSample, 0,
    'd', 'a', 't', 'a',
    0, 0, 0, 0 // <- data chunk size (filled in later)
  };

  file.write(header, 44);
}

void continueRecording() {
  //Serial.println("rec?");
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
    //AudioNoInterrupts();
    Serial.println("Start recording");
    showIcons("icon_rec", CRGB(20, 0, 0));
    FastLED.show();

    if (SD.exists(OUTPUTf)) {
      SD.remove(OUTPUTf);
    }

    frec = SD.open(OUTPUTf, FILE_WRITE);

    if (frec) {
      writeWavHeader(frec, (uint32_t)AUDIO_SAMPLE_RATE_EXACT, 16, 1);  // 44.1kHz, 16-bit, mono
      queue1.begin();
    }
  }
}

void stopRecord(int fnr, int snr) {
  if (isRecording) {
    //AudioInterrupts();
    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", fnr, getFileNumber(snr));

    Serial.println("Stop Recording");
    queue1.end();
    if (frec) {
      while (queue1.available() > 0) {
        frec.write((uint8_t*)queue1.readBuffer(), 256);
        queue1.freeBuffer();
      }
      frec.close();

      // Update file sizes in header
      File f = SD.open(OUTPUTf, FILE_WRITE);
      if (f) {
        uint32_t fileSize = f.size();
        uint32_t dataSize = fileSize - 44;
        f.seek(4);
        uint32_t riffSize = fileSize - 8;
        f.write((uint8_t *)&riffSize, 4);
        f.seek(40);
        f.write((uint8_t *)&dataSize, 4);
        f.close();
      }
    }
    isRecording = false;

    Serial.println("Start Playing");
    playRaw0.play(OUTPUTf);
  }
}



void drawRecMode(){
  
  if (recMode==1) { drawText("mic", 7, 10, CRGB(200, 200, 200)); recInput = AUDIO_INPUT_MIC;}
  if (recMode==-1) {drawText("line", 6, 10, CRGB(0, 0, 200));recInput = AUDIO_INPUT_LINEIN;}
  sgtl5000_1.inputSelect(recInput);
  
                          
  FastLEDshow();
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

if (currentMode == &menu && buttonString == "1000")
  {
    switchMode(&draw);
    SMP.singleMode = false;
  }

  if (currentMode == &menu && buttonString == "0001")
  {
    switch(menuPosition){
      case 1:
       switchMode(&loadSaveTrack);
      break;

      case 2:
       switchMode(&set_SamplePack);
      break;

      case 3:
      switchMode(&set_Wav);
      currentMode->pos[3] = SMP.wav[SMP.currentChannel][0];
      SMP.wav[SMP.currentChannel][1] = SMP.wav[SMP.currentChannel][0];
     //set encoder to currently Loaded Sample!!
      //Encoder[3].writeCounter((int32_t)((SMP.wav[SMP.currentChannel][0] * 4) - 1));
      break;
    
      case 4:
      recMode = recMode*(-1);
      drawRecMode();
      break;

      case 5:
        switchMode(&volume_bpm);
      break;
}
  }else if ((currentMode == &loadSaveTrack) && buttonString == "0001") {
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
    
    SMP.wav[SMP.currentChannel][0] = SMP.wav[SMP.currentChannel][1];
    currentMode->pos[3] = SMP.wav[SMP.currentChannel][0];
    loadWav();
    autoSave();
  } else if ((currentMode == &set_Wav) && buttonString == "0001") {
    switchMode(&singleMode);
    SMP.singleMode = true;
  }else if ((currentMode == &set_Wav) && buttonString == "0200") {
    Serial.println("RECORDING TO ============> ");
    Serial.println(SMP.wav[SMP.currentChannel][1]);
    record(getFolderNumber(SMP.wav[SMP.currentChannel][1]), SMP.wav[SMP.currentChannel][1]);
  } else if ((currentMode == &set_Wav) && buttonString == "0900") {
    stopRecord(getFolderNumber(SMP.wav[SMP.currentChannel][1]), SMP.wav[SMP.currentChannel][1]);
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
    fxType=0;
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

  // set DefaultFilterParam
  if (currentMode == &filterMode && buttonString == "0010") {
    if (fxType == 1) {
      defaultFilter[SMP.currentChannel] = SMP.selectedFilter;
      
    } else {
      defaultFilter[SMP.currentChannel] = SMP.selectedParameter;
    }
    Serial.println("!!!-------------------");
    Serial.println(defaultFilter[SMP.currentChannel]);
    Serial.println("!!!-------------------");

    
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











void setup(void) {
  NVIC_SET_PRIORITY(IRQ_LPUART8, 1);
  //NVIC_SET_PRIORITY(IRQ_USB1, 128);  // USB1 for Teensy 4.x
  Serial.begin(115200);

  Wire.begin();
  if (CrashReport) {
    Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
    Serial.print(CrashReport);
    
    delay(1000);
    Serial.println("clearing EEPROM and autosaved.txt...");

      for ( unsigned int i = 0 ; i < EEPROM.length() ; i++ ) EEPROM.write(i, 0);

    char OUTPUTf[50];
    sprintf(OUTPUTf, "autosaved.txt");
      if (SD.exists(OUTPUTf)) {
        SD.remove(OUTPUTf);
      }
    delay(2000);
    Serial.println("clearing completed.");
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

  //runAnimation();

  Serial.print("Initializing SD card...");
  drawNoSD();

  EEPROMgetLastFiles();
  loadSamplePack(samplePackID, true);

  for (unsigned int vx = 1; vx < SONG_LEN + 1; vx++) {
    for (unsigned int vy = 1; vy < maxY + 1; vy++) {
      note[vx][vy][1] = defaultVelocity;
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


  mixer0.gain(0,GAIN2);
  mixer0.gain(1,GAIN2);

  mixer1.gain(0, GAIN4);
  mixer1.gain(1, GAIN4);
  mixer1.gain(2, GAIN4);
  mixer1.gain(3, GAIN4);

  mixer2.gain(0, GAIN4);
  mixer2.gain(1, GAIN4);
  mixer2.gain(2, GAIN4);
  mixer2.gain(3, GAIN4);

  synthmixer1.gain(0, GAIN3);
  synthmixer1.gain(1, GAIN3);
  synthmixer1.gain(3, GAIN3);

  synthmixer2.gain(0, GAIN3);
  synthmixer2.gain(1, GAIN3);
  synthmixer2.gain(3, GAIN3);


  synthmixer3.gain(0, GAIN3);
  synthmixer3.gain(1, GAIN3);
  synthmixer3.gain(3, GAIN3);

  synthmixer4.gain(0, GAIN3);
  synthmixer4.gain(1, GAIN3);
  synthmixer4.gain(3, GAIN3);


  mixersynth_end.gain(0,GAIN4);
  mixersynth_end.gain(1,GAIN4);
  mixersynth_end.gain(2,GAIN4);
  mixersynth_end.gain(3,GAIN4);


  mixer_end.gain(0, GAIN3);
  mixer_end.gain(1, GAIN3);
  mixer_end.gain(2, GAIN3);
  


  mixerPlay.gain(0, GAIN3);
  mixerPlay.gain(1, GAIN3);
  mixerPlay.gain(2, GAIN3);
  


  // Initialize the array with nullptrs

  synths[11][0] = &waveform11_1;
  synths[11][1] = &waveform11_2;

  synths[12][0] = &waveform12_1;
  synths[12][1] = &waveform12_2;

  synths[13][0] = &waveform13_1;
  synths[13][1] = &waveform13_2;

  synths[14][0] = &waveform14_1;
  synths[14][1] = &waveform14_2;

  short waveformType[15][2] = { 0 };  // zero-initialize unused elements
  // Define your actual waveforms at relevant indices
  waveformType[11][0] = WAVEFORM_SAWTOOTH;
  waveformType[11][1] = WAVEFORM_SQUARE;

  waveformType[12][0] = WAVEFORM_SINE;
  waveformType[12][1] = WAVEFORM_TRIANGLE;

  waveformType[13][0] = WAVEFORM_PULSE;
  waveformType[13][1] = WAVEFORM_SAWTOOTH;

  waveformType[14][0] = WAVEFORM_PULSE;
  waveformType[14][1] = WAVEFORM_SAWTOOTH;

  float amplitude[15][2] = { 0 };
  // Set amplitude values similarly
  amplitude[11][0] = 0.3;
  amplitude[11][1] = 0.5;

  amplitude[12][0] = 0.4;
  amplitude[12][1] = 0.4;

  amplitude[13][0] = 0.3;
  amplitude[13][1] = 0.3;

  amplitude[14][0] = 0.3;
  amplitude[14][1] = 0.3;

  drums[11]->frequency(60);
  drums[11]->length(250);
  drums[11]->secondMix(0.0);
  drums[11]->pitchMod(0.7);

  drums[12]->frequency(120);
  drums[12]->length(400);
  drums[12]->secondMix(0.3);
  drums[12]->pitchMod(0.6);

  drums[13]->frequency(180);
  drums[13]->length(200);
  drums[13]->secondMix(0.0);
  drums[13]->pitchMod(0.5);

  drums[14]->frequency(240);
  drums[14]->length(150);
  drums[14]->secondMix(0.4);
  drums[14]->pitchMod(0.4);





  // Initialize your waveforms in a loop safely:
  for (int pairIndex = 11; pairIndex <= 13; pairIndex++) {
    for (int synthIndex = 0; synthIndex < 2; synthIndex++) {
      if (synths[pairIndex][synthIndex] != nullptr) {
        synths[pairIndex][synthIndex]->begin(waveformType[pairIndex][synthIndex]);
        synths[pairIndex][synthIndex]->amplitude(amplitude[pairIndex][synthIndex]);
        synths[pairIndex][synthIndex]->phase(0);
      }
    }
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






  // set BPM:100
  SMP.bpm = 100.0;
  playTimer.begin(playNote, playNoteInterval);
  playTimer.priority(110);
  

  midiTimer.begin(checkMidi, 1000);
  midiTimer.priority(10);

  AudioInterrupts();
  AudioMemory(50);
  // turn on the output
  sgtl5000_1.enable();

  sgtl5000_1.volume(0.0);

  sgtl5000_1.volume(0.1);
  sgtl5000_1.volume(0.4);
  sgtl5000_1.volume(0.9);

  

  //sgtl5000_1.autoVolumeControl(1, 1, 0, -6, 40, 20);
  //sgtl5000_1.audioPostProcessorEnable();

  //sgtl5000_1.audioPreProcessorEnable();
  //sgtl5000_1.audioPostProcessorEnable();
  //   sgtl5000_1.autoVolumeEnable();

  //REC

  sgtl5000_1.inputSelect(recInput);
  sgtl5000_1.micGain(33);  //0-63
  sgtl5000_1.adcHighPassFilterEnable();
  //sgtl5000_1.adcHighPassFilterDisable();


  sgtl5000_1.unmuteLineout();
  sgtl5000_1.lineOutLevel(1);

  autoLoad();
  switchMode(&draw);
  MIDI.begin(MIDI_CH);
  //Serial8.begin(57600); 
  
  

  //set Defaults
  setDahdsrDefaults(true);
  for (int ch = 1; ch <= 14; ch++) {
    setFilterDefaults(ch);
  }

    //testDrums();

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
      Serial.println(posString);
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



    //SMP.edit = 1; // überflüssig
if ((SMP.y > 1 && SMP.y <= 9) || (SMP.y >= 11 && SMP.y<=14)){
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


void filtercheck(){
  FilterType fx = defaultFilter[SMP.currentChannel];
    if (fxType == 1 && !filterfreshsetted && currentMode->pos[2] != SMP.filter_settings[SMP.currentChannel][fx]) {
      float mappedValue = processFilterAdjustment(fx, SMP.currentChannel, 2);
      updateFilterValue(fx, SMP.currentChannel, mappedValue);
      

       filterchecktime = millis();

    // Activate the 2-second drawing period
    filterDrawActive = true;
    filterDrawEndTime = millis() + 2000;  // 2 seconds window
      drawFilterCheck(SMP.filter_settings[SMP.currentChannel][fx],fx);
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

    if (isNowPlaying) {
      filtercheck();
      drawTimer();
    }
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
  for (int ch = 11; ch <= 14; ch++) {
    int noteLen = getNoteDuration(ch);
    //Serial.println(noteLen);
    if (noteOnTriggered[ch] && millis() - startTime[ch] >= noteLen) {
      envelopes[ch]->noteOff();
      noteOnTriggered[ch] = false;
    }
  }


   

  FastLEDshow();  // draw!
  yield();
  delay(50);
  yield();
  filterfreshsetted = false;
}

float getNoteDuration(int channel) {
  int timetilloff = mapf(SMP.param_settings[channel][DELAY], 0,maxfilterResolution , 0, maxParamVal[DELAY]) + mapf(SMP.param_settings[channel][ATTACK], 0, maxfilterResolution, 0,  maxParamVal[ATTACK]) + mapf(SMP.param_settings[channel][HOLD], 0, maxfilterResolution, 0,  maxParamVal[HOLD]) + mapf(SMP.param_settings[channel][DECAY], 0, maxfilterResolution, 0,  maxParamVal[DECAY]) + mapf(SMP.param_settings[channel][RELEASE], 0, maxfilterResolution, 0,  maxParamVal[RELEASE]);
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
  //allOff();
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
        int ch = note[beat][b][0];

        if (ch < 9) {
           


          if (SMP.param_settings[ch][TYPE] == 1){ 
            
          float tone = pianoFrequencies[b];
            float dec = mapf(SMP.drum_settings[ch][DRUMDECAY],0,64,0,1023);
            float pit = mapf(SMP.drum_settings[ch][DRUMPITCH],0,64,0,1023);
            float typ = mapf(SMP.drum_settings[ch][DRUMTYPE],0,64, 1,3);
            
            if (ch == 1) KD_drum(tone,dec,pit,typ);
            if (ch == 2) SN_drum(tone,dec,pit,typ);
            if (ch == 3) HH_drum(tone,dec,pit,typ);

          }
          else{
          _samplers[ch].noteEvent(12 * SampleRate[note[beat][b][0]] + b - (note[beat][b][0] + 1), note[beat][b][1], true, false);
          // usbMIDI.sendNoteOn(b, note[beat][b][1], 1);
          }
        }

        if (note[beat][b][0] >= 11) {

          float frequency = pianoFrequencies[b] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert // b-1??
          float WaveFormVelocity = mapf(note[beat][ch][1], 1, 127, 0.0, 1.0);
          synths[ch][0]->frequency(frequency);
          float detune_amount = mapf(SMP.filter_settings[ch][DETUNE], 0, maxfilterResolution, -1.0 / 12.0, 1.0 / 12.0);
          float detune_ratio = pow(2.0, detune_amount);  // e.g., 2^(1/12) ~ 1.05946 (one semitone up)
          // Octave shift: assume OCTAVE parameter is an integer (or can be cast to one)
          // For example, an OCTAVE value of 1 multiplies frequency by 2, -1 divides by 2.
          int octave_shift = mapf(SMP.filter_settings[ch][OCTAVE], 0, maxfilterResolution, -3, +3);
          float octave_ratio = pow(2.0, octave_shift);
          // Apply both adjustments to the base frequency
          synths[ch][1]->frequency(frequency * octave_ratio * detune_ratio);
          synths[ch][0]->amplitude(WaveFormVelocity);
          synths[ch][1]->amplitude(WaveFormVelocity);
          envelopes[ch]->noteOn();
          
          drums[ch]->frequency(frequency * octave_ratio * detune_ratio);
          drums[ch]->noteOn();
          
          startTime[ch] = millis();    // Record the start time
          noteOnTriggered[ch] = true;  // Set the flag so we don't trigger noteOn again
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

 if ((y > 1 && y<=15)) {
  if (!SMP.singleMode) {
    Serial.println("deleting voice:" + String(x));
    note[x][y][0] = 0;
    note[x][y][1] = defaultVelocity;
  } else {
    if (note[x][y][0] == SMP.currentChannel)
      note[x][y][0] = 0;
    note[x][y][1] = defaultVelocity;
  }
  }else if (y == 16){
    clearPageX(x);
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
    if ((y > 1 && y <= 9) || (y >= 11 && y<=15)) {
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
    } else if (y==16) toggleCopyPaste(); //copypaste if top above

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
    if (note[x][y][0] <= 9) {
      _samplers[note[x][y][0]].noteEvent(12 * SampleRate[note[x][y][0]] + y - (note[x][y][0] + 1), defaultVelocity, true, false);
      yield();
    }

    if (note[x][y][0] >= 11) {

      int ch = note[x][y][0];
      float frequency = pianoFrequencies[y] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
      synths[ch][0]->frequency(frequency);
      float WaveFormVelocity = mapf(defaultVelocity, 1, 127, 0.0, 1.0);
      synths[ch][0]->amplitude(WaveFormVelocity);
      envelopes[ch]->noteOn();
      drums[ch]->frequency(frequency);
      drums[ch]->noteOn();
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
  if(MIDI_CLOCK_SEND){
    Serial.println("BPM: " + String(currentMode->pos[3]));
    SMP.bpm = currentMode->pos[3];
    playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
   playTimer.update(playNoteInterval);
   midiTimer.update(playNoteInterval / 1000);
    }
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





void showMenu() {
  FastLEDclear();
  showIcons("helper_exit", CRGB(10, 5, 0));
  //drawNumber(menuPosition, CRGB(20, 20, 40), 0);


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
      drawRecMode();
      break;

    case 5:
      showIcons("icon_bpm", CRGB(0, 50, 0));
      drawText(menuText[menuPosition - 1], 6, menuPosition, CRGB(200, 0, 200));
      break;

    case 6:
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
    drawNumber(SMP.file, CRGB(0, 0, 20), 11);
  } else {
    showIcons("helper_save", CRGB(20, 0, 0));
    showIcons("helper_load", CRGB(0, 1, 0));
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

  showIcons("icon_samplepack", CRGB(10, 10, 0));
  showIcons("helper_select", CRGB(0, 0, 5));
  drawNumber(SMP.pack, CRGB(20, 0, 0), 11);

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", SMP.pack, 1);
  if (SD.exists(OUTPUTf)) {
    showIcons("helper_load", CRGB(0, 20, 0));
    showIcons("helper_save", CRGB(3, 0, 0));
    drawNumber(SMP.pack, CRGB(0, 20, 0), 11);
  } else {
    showIcons("helper_load", CRGB(0, 3, 0));
    showIcons("helper_save", CRGB(20, 0, 0));
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
      FastLEDclear();
      showIcons("icon_sample", CRGB(20, 20, 20));
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


void loadWav() {
  playRaw0.stop();
  Serial.println("Loading Wave :" + String(SMP.wav[SMP.currentChannel][1]));
  loadSample(0, SMP.wav[SMP.currentChannel][1]);
  switchMode(&singleMode);
  SMP.singleMode = true;
}