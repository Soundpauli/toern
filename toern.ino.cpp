# 1 "/var/folders/3h/f1rm8vzs1lv5ljhwy05p7nfhf1r72j/T/tmp7j4_0ky1"
#include <Arduino.h>
# 1 "/Users/jank/git/toern/toern.ino"
#include "src/toern_build_types.h"
#include "toern_touch_input.h"

#define VERSION "v3.01h"
extern "C" char *sbrk(int incr);
#define FASTLED_ALLOW_INTERRUPTS 0


#define TargetFPS 60


#define DEBUG_PLAYBACK_SERIAL 0



struct Mode;
FLASHMEM void applyDrawSingleYEncoderLimits();





static const int FAST_DROP_BLOCKS = 5;
static int fastDropRemaining = 0;



volatile bool isrPlayButtonTick = false;
bool pendingPauseUIUpdate = false;
bool pendingPauseSkipSave = false;
bool pendingPauseAutoSave = false;
elapsedMillis pauseSdWaitMs;
static uint8_t pauseSdSettleGen = 0;
bool sdIoAudioSettled();
volatile bool isrDbgPatternFinished = false;
volatile uint16_t isrDbgPatternFinishedBeat = 0;
volatile bool isrDbgLoopCountChanged = false;
volatile uint16_t isrDbgLoopCountValue = 0;


#include <cstring>
#include "src/effect_freeverb_dmabuf.h"
#include "src/effect_delay_ext.h"
#define USE_WS2812SERIAL 
#include <WS2812Serial.h>

#include <FastLED.h>

#include <i2cEncoderLibV2.h>
#include <MIDI.h>
#include <Audio.h>
#include "src/toern_audio_runtime.h"
#include "src/toern_pcm_interpolation.h"
#include "src/toern_legacy_filter.h"
#include <EEPROM.h>
#include <FastTouch.h>
#include <sampler.h>

#define byte uint8_t
#include "Adafruit_seesaw.h"
#undef byte

#include <Mapf.h>
#include "notes.h"
#include "colors.h"
#include "audioinit.h"
#include "font_3x5.h"
#include "icons.h"


enum ValueDisplayMode : uint8_t {
  DISPLAY_NUMERIC = 0,
  DISPLAY_ENUM = 1,
};

enum ParameterType : uint8_t {
  DELAY,
  ATTACK,
  HOLD,
  DECAY,
  SUSTAIN,
  RELEASE,
  WAVEFORM,
  TYPE,
};

enum FilterType : uint8_t {
  NULL_,
  HCUT,
  LOWCUT,
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
  FILTER_WAVEFORM,
};

enum SynthTypes : uint8_t {
  INSTRUMENT,
  CUTOFF,
  RESONANCE,
  FILTER,
  CENT,
  SEMI,
  FORM,
  LFO_RATE,
  LFO_DEPTH,
  LFO_PHASE,
  ARP_STEP,
};
#define ARP_SPAN LFO_PHASE

enum MidiSetTypes : uint8_t {
  MIDI_IN_BOOL,
  MIDI_OUT_BOOL,
  SINGLE_OUT_CHANNEL,
  GLOBAL_IN_CHANNEL,
  SEND_CTRL_BOOL,
  RECEIVE_CTRL_BOOL,
};


enum SettingArray : uint8_t { ARR_FILTER,
                              ARR_SYNTH,
                              ARR_PARAM,
                              ARR_NONE,
                              ARR_STRETCH };


struct SliderDefEntry {
  SettingArray arr;
  int8_t idx;
  const char *name;
  uint8_t maxValue;
  uint8_t displayMode;
  const char **enumNames;
  uint8_t displayRange;
};

#define FILTER_PAGE_CAP 5

struct Note {
  uint8_t channel;
  uint8_t velocity;
  uint8_t probability;
  uint8_t condition;
  uint8_t midiPitch;
} __attribute__((packed));
static_assert(sizeof(Note) == 5, "Note persistence layout must remain packed");

static constexpr uint8_t NOTE_MIDI_PITCH_NONE = 0xFF;
static constexpr uint8_t NOTE_CONDITION_FILL = 21;
static constexpr uint8_t NOTE_CONDITION_GLIDE = 22;
static constexpr uint8_t NOTE_CONDITION_STEP_COUNT = 11;
static const uint8_t noteConditionValues[NOTE_CONDITION_STEP_COUNT] PROGMEM = {
  1, 2, 4, 8, 16, 17, 18, 19, 20, NOTE_CONDITION_FILL, NOTE_CONDITION_GLIDE
};

#define LED_MODULES 2
#define MATRIX_WIDTH 16
#define maxY 16
#define INT_SD 10


extern unsigned int maxX;
extern void handleMidiClock();
extern void handleStart();
#define NUM_LEDS (256 * LED_MODULES)
#define DATA_PIN 17
#define INT_PIN 27

#define SWITCH_1 2
#define SWITCH_2 3
#define BUTTON_A 5
#define BUTTON_B 22
#define SWITCH_3 4
#define SWITCH_4 6
#define SWITCH_5 39



#define BATT_ADC_PIN A16
#define BATT_RAW_ZERO 4
#define BATT_RAW_FULL 4095


#define BATT_DIVIDER_RATIO 0.565f


#define BATT_V_MIN 3.67663f
#define BATT_V_MAX 4.125f
#define BATT_PCT_CURVE 1.0f
#define BATT_WARN_PCT 8

#define VOL_MIN 1
#define VOL_MAX 10
#define LINEOUT_MIN 13
#define LINEOUT_MAX 31
#define BPM_MIN 40
#define BPM_MAX 300

#define GAIN_1 0.4
#define GAIN_2 0.8
#define GAIN_3 0.4
#define GAIN_4 0.8
# 228 "/Users/jank/git/toern/toern.ino"
#define MIX_BUS_HEADROOM (1.0f / 3.0f)
#define MIX_END_SAMPLES_GAIN 0.50f
#define MIX_END_SYNTH_GAIN 0.60f


#define CH11_POLY_SUM_GAIN 0.55f
#define CH11_SYNTHMIX_GAIN 0.72f
#define LFO_CUTOFF_OCTAVES 6.0f
#define LFO_RES_SWEEP 4.2f
#define ARP_CENTER 16

#define MIX_GAIN_UNITY 10
#define MIX_GAIN_MAX 20
#define EEPROM_MIX_GAIN_14 38
#define EEPROM_MIX_GAIN_58 39
#define EEPROM_MIX_GAIN_SYN 40
#define EEPROM_MIX_GAIN_ALL 41

#define NUM_ENCODERS 4
#define defaultVelocity 100
#define FOLDER_MAX 10

#define maxPages 16
#define maxFiles 9
#define NUM_CHANNELS 16
#define maxFilters 15

#define maxfilterResolution 32.0
#define numPulsesForAverage 8
#define pulsesPerBar (24 * 4)

#define EEPROM_MENU_ADDR 42


extern void sampleBrowserRefreshList(int channel);
extern void sampleBrowserSyncBrowseFromStoredPath(int channel);
extern void sampleBrowserNavigatePress(int channel);
extern void sampleBrowserLastEncoderPress(int channel);
extern void sampleBrowserGoUpPress(int channel);
extern void sampleBrowserClampBrowseIndexAndHardware(int channel);
extern int sampleBrowserBrowseIndexMax(int channel);
extern unsigned long g_setWavIgnoreLoadUntilMs;
extern bool sampleBrowserIsLoadableSelection(int channel);
extern bool shouldSkipVoiceTriggerForSyncPreview(int ch);
extern void triggerSetWavSyncPreview(uint8_t vel);
extern void previewSample(bool setMaxSampleLength);
extern void previewSample(bool setMaxSampleLength, bool playAudio);
extern void previewSampleNow();
extern void serviceSetWavSyncPreview();
extern void serviceSdPreviewRequests();
extern uint16_t g_wavPickCount;
extern uint16_t g_folderPickCount;

struct MidiSettings : public midi ::DefaultSettings {
  static const long BaudRate = 31250;
  static const unsigned SysExMaxSize = 16;
  static const bool Use1ByteParsing = false;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial8, MIDI, MidiSettings);
static uint8_t midiTxExtra[128];
static uint8_t midiRxExtra[512];

#define EXTERNAL_ONE_ENCODER_INDEX 2
static bool externalOneBlinkActive = false;
static unsigned long externalOneBlinkUntil = 0;





int inputMonitoringState = 0;

void triggerExternalOneBlink();
void updateExternalOneBlink();
void updatePreviewVolume();

#define CLOCK_BUFFER_SIZE 24

elapsedMillis recTime;

extern i2cEncoderLibV2 Encoder[NUM_ENCODERS];

bool showChannelNr = true;
bool lastPinsConnected = false;
unsigned long lastChangeTime = 0;
const unsigned long debounceDelay = 500;

#define EEPROM_MAGIC_ADDR 42
#define EEPROM_DATA_START 43
const uint8_t EEPROM_MAGIC = 0x5A;




extern int currentMenuPage;


bool getMuteState(int channel);
bool getMuteStateForUI(int channel);
void setMuteState(int channel, bool muted);
bool isChildVoiceDisabled(int channel);
bool isChildVoiceMuted(int channel);
void enforceChildModeRestrictions();
void applyMutesAfterPMODSwitch();
void copyPageMutes(int fromPage, int toPage);


enum class FullMuteView : uint8_t { AllMuted = 0, Ch12, Custom, UnmuteAll };

void fullMuteEnter();
void fullMuteExitRestore();
void fullMuteToggleView(FullMuteView target);
void fullMuteApplyView();
bool fullMuteIsActive();
void savePageMutesToGlobal();
void loadGlobalMutesToPage();
void initPageMutes();
void drawIndicator(char size, char colorCode, int encoderNum, bool highlight = false, bool setEncoderLed = true, uint8_t brightness = 255);
void drawFullMuteBorder();
void drawFullMuteIndicators();
void getIndicatorXPositions(int encoderNum, int &x1, int &x2, int &x3);
void updatePongBall();
void drawPongBall();
void resetPongGame();
void showSaveSuccessAnimation(bool returnToDraw = true);
void showSaveFailureAnimation(bool returnToDraw = true);
void triggerGridNote(unsigned int globalX, unsigned int y, bool allowMuted = false);
int mapXtoPageOffset(int x);
void stopSound(int note, int ch);
void playSound(int note, int ch, int velocity);
void playSequencedSound(int note, int ch, int velocity, uint32_t tick,
                        bool allowLegato);
void finishSequencedSoundTick(uint32_t tick, bool hadNotes);
void resetSequencedSynthLegato();
void playSequencedSynth(int ch, int b, int vel, uint32_t tick,
                        bool allowLegato, int midiPitchOverride);
void playSynthMidi(int ch, uint8_t midiPitch, int vel, bool persistant);
void finishSequencedMonoSynthTick(uint32_t tick, bool had13, bool had14);
void stopSynthChannel(int ch);
void midiArpRestart(int ch);
void midiArpOnHeldNote(int ch);
void midiArpResetVoice(int ch);
bool synthMidiArpEnabled(int ch);
void serviceSynthLfo();
void serviceMidiArp();
void serviceMidiArpOnPlayhead(uint16_t playedBeat);
static inline void resetMidiPressedKeyCount11to14();
void drawCtrlVolumeOverlay(int volume);
void initNeoSlider();
void updateNeoSliderVolume();
void reapplyAllSampleChannelGains();
void resetAllChannelVolumesToDefault();
void extendLastPageForLiveNote(uint8_t channel, uint8_t row, uint16_t targetBeat);
void updateLastPage();
void triggerSamplerVoice(int ch, int pitch, int vel, bool retrigger);
uint8_t scaledNoteVelocity(int ch, int vel);
void updateAllMixerGains();
void forceAllMixerGainsToTarget();
void drawInputGainOverlay(int gain, int maxGain);
void drawChannelNrOverlay(int channelNum, int channelIdx);
void drawPageNrOverlay(int pageNum);
void drawSampleLoadOverlay(uint8_t progressPercent);
bool loadPreviewToChannel(unsigned int targetChannel, bool showLoadProgress = false);
void copySampleToSamplepack0(unsigned int channel, bool showLoadProgress = false);
void requestDeferredSampleLoadAutosave();
void servicePendingSamplePersist();
void saveSp0StateToEEPROM();
bool voicePresetExists(int ch, uint16_t slot);
bool saveVoicePreset(int ch, uint16_t slot);
bool loadVoicePreset(int ch, uint16_t slot);
uint16_t getVoicePresetSlot(int ch);
void setVoicePresetSlot(int ch, uint16_t slot);
bool takeVoicePresetUiDirty();
FLASHMEM void showPresetLoadSave();
void saveSingleModeToEEPROM(int index, int8_t value);
FLASHMEM void flushSettingsBackupNow();
void stopAllSetWavPreviewAudio();
void stopSdPreviewIfPlaying();
void beginChannelSampleReload(unsigned int ch);
void endChannelSampleReload(unsigned int ch);
bool isChannelSampleReloadBusy(unsigned int ch);
size_t sdIoChunkSize();
void sdIoYield();
void sdIoBeginAudioSafe();
void sdIoEndAudioSafe();
bool soloRandomPickPath(char* outRel, size_t outRelSize);
bool soloRandomRenewPlaylist();
bool soloRandomStepPlaylist(int delta);
bool soloRandomBuildPlayPath(const char* stored, char* out, size_t outSz);
bool soloRandomIsPlayableWav(const char* fullPath);
void soloRandomCommitPathToChannel(int ch, const char* fullSdPath);
void soloRandomDropCurrentEntry();
void soloRandomPreviewCurrentVoice();
void soloRandomStepAndPreview(int delta);
const char* soloRandomGetLastPreviewPath();
void soloRandomLoadLastPreview();
static int16_t lastDefaultFastFilterValue[NUM_CHANNELS] = { 0 };

const char *const instTypeNames[] PROGMEM = { "BASS", "KEYS", "CHPT", "PAD", "WOW", "ORG", "FLT", "LEAD", "ARP", "BRSS", "303", "DXEP", "DXBS" };
#define INST_ENUM_COUNT (sizeof(instTypeNames) / sizeof(instTypeNames[0]))


const char *const sndTypeNames[] PROGMEM = { "SAMP" };
const char *const waveformNames[] PROGMEM = { "SIN", "SQR", "SAW", "TRI" };

struct SliderMeta {
  uint8_t maxValue;
  uint8_t displayMode;
  const char **enumNames;
  uint8_t displayRange;
};

static const SliderMeta sliderMeta[4][4] = {

  {
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 } },

  {
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 } },

  {
    { 16, DISPLAY_NUMERIC, nullptr, 5 },
    { 12, DISPLAY_ENUM, instTypeNames, 13 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 } },

  {
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 },
    { 32, DISPLAY_NUMERIC, nullptr, 32 } }
};

struct PendingNoteEvent {
  uint8_t channel;
  uint8_t pitch;
  uint8_t velocity;
  unsigned long triggerTime;
};

std::vector<PendingNoteEvent> pendingSampleNotes;

struct CachedSample {
  char pathRel[128];
  uint32_t lengthBytes;
  uint8_t rate;
  bool valid;
  int plen;
};

CachedSample previewCache;


bool disableThresholdFlag = false;

bool MIDI_CLOCK_SEND = true;
bool MIDI_NOTE_SEND = true;
bool MIDI_NOTE_RECEIVE = true;
bool MIDI_NOTE_CLAMP = true;
bool MIDI_WRITE_OVERWRITE = false;

bool MIDI_TRANSPORT_RECEIVE = true;
bool MIDI_TRANSPORT_SEND = false;
int8_t transportSendDelayMs = 17;
int8_t transportRcveDelayMs = 17;
bool MIDI_VOICE_SELECT = false;
bool SMP_PATTERN_MODE = false;
bool SMP_FLOW_MODE = true;
bool voiceMode = false;
bool imageMode = false;
int imgBrushChannel = 1;
uint8_t eyesMode = 1;
uint8_t fireVoice = 0;
uint8_t fireLevel = 8;
uint8_t fireSize = 1;
uint8_t fireGravity = 0;
uint8_t fireColor = 8;
uint8_t fireFocus = 0;
uint8_t voicePageLen[16] = {0};
uint16_t voicePageMask[16] = {0};
uint16_t voiceRowMask[16] = {0};
uint8_t voiceModePages = 1;
uint16_t voiceLoopCount[16] = {0};
uint8_t voiceEditPage[16] = {0};
int8_t voicePageOffset[16] = {0};
static bool spkrEnabled = true;
static bool adcHpfEnabled = true;
static bool usbSmpPreview = false;
static bool childLockEnabled = false;
static bool bootFullResetRequested = false;
unsigned int lastFlowPage = 0;
unsigned int SMP_FAST_REC = false;
unsigned int SMP_REC_CHANNEL_CLEAR = true;
bool SMP_LOAD_SETTINGS = true;
uint8_t lineOutLevelSetting = 30;

bool pendingStartOnBar = false;

bool drawNoSD_hasRun = false;




volatile uint16_t liveRecordSkipBeat = 0;
volatile uint16_t liveRecordSkipRows = 0;
void clearLiveRecordSkip();
static void markLiveRecordSkip(uint16_t targetBeat, uint8_t row);
static bool consumeLiveRecordSkip(uint16_t srcBeat, unsigned int row);
bool placeLiveGridNote(uint8_t channel, uint8_t row, uint8_t velocity,
                       uint8_t midiPitch, bool protectOtherVoices,
                       bool protectOccupied);
bool readTouch1Pressed();
bool readTouch2Pressed();
bool readRecTouchHeld();
bool getSpkrEnabled();
void setSpkrEnabled(bool enabled);
bool getAdcHpfEnabled();
void setAdcHpfEnabled(bool enabled);
bool getUsbSmpPreview();
void setUsbSmpPreview(bool enabled);
bool getChildLockEnabled();
void setChildLockEnabled(bool enabled);
uint16_t quantizedMidiRecordBeat();
FLASHMEM void noteUserActivity();
FLASHMEM void enforceImageModeConstraints();
float& filterSetting(int channel, int index);
FLASHMEM bool isChildVoiceMuted(int channel);
FLASHMEM void enforceChildModeRestrictions();
bool encoderI2cWritesAllowed();
static float faderVelScaleFromSlide(uint16_t slideVal, bool inMute, bool inFull);
static int faderVolFromSlide(uint16_t slideVal, bool inMute, bool inFull);
FLASHMEM void initSliderDefTemplates();
void setSampleTimeStretch(uint8_t channel, int value);
FLASHMEM void applyStereoChannelRouting();
FLASHMEM void applyMixBusGains();
void allOff();
static uint8_t noteConditionFromStep(int step);
static int noteConditionToStep(uint8_t condition);
FLASHMEM void setVelocity();
void staticButtonPushed(i2cEncoderLibV2 *obj);
void staticButtonReleased(i2cEncoderLibV2 *obj);
void staticThresholds(i2cEncoderLibV2 *obj);
void encoder_button_pushed(i2cEncoderLibV2 *obj, int encoderIndex);
void encoder_button_released(i2cEncoderLibV2 *obj, int encoderIndex);
void handle_button_state(i2cEncoderLibV2 *obj, int encoderIndex);
bool match_buttons(const uint8_t states[], int b0, int b1, int b2, int b3);
bool was_buttons_0000(const uint8_t prevStates[]);
bool was_not_buttons_0000(const uint8_t prevStates[]);
uint32_t hashString(const String &str);
static void resetCtrlModeState();
FLASHMEM void showCtrlVolumeChange(int volume);
FLASHMEM void showInputGainOverlay(int gain, int maxGain);
void showDrawAfterPatternTransfer();
FLASHMEM void switchMode(Mode *newMode);
void checkFastRec();
FLASHMEM void checkMode(const uint8_t currentButtonStates[NUM_ENCODERS], bool reset);
FLASHMEM void applySgtl5000CodecOutputPath();
FLASHMEM void applySgtl5000AdcHighPass();
FLASHMEM void initSoundChip();
FLASHMEM void initSamples();
FLASHMEM void checkCrashReport();
static void neoSliderFlushPendingLeds();
FLASHMEM void initNeoSlider();
static void applyPlayAmplitude(int ch, float amp);
static float playAmpForVoice(int ch);
void samplerNoteEvent(int ch, uint8_t pitch, uint8_t velocity, bool on, bool retrigger);
void samplerRemoveSamples(int ch);
void samplerAddSample(int ch, uint8_t root, int16_t *data, uint32_t frames, uint16_t channels);
FLASHMEM void reapplyAllSampleChannelGains();
FLASHMEM void resetAllChannelVolumesToDefault();
static void processOneNeoSlider(int i, uint16_t *lastRaw);
FLASHMEM void initEncoders();
FLASHMEM void loadColorSchemeAndBrightness();
FLASHMEM void loadLedModeEarlyFromEEPROM();
FLASHMEM void setup();
FLASHMEM void initGlobalVars();
int getBatteryRaw();
int getBatteryPercent();
bool isRunningOnBattery();
bool eyesScreensaverEnabled();
uint32_t eyesScreensaverIdleMs();
bool getBatteryWarningActive();
FLASHMEM void checkEncoders();
void filtercheck();
void checkButtons();
void exitMenuFromTouchInput();
void checkTouchInputs();
void checkPendingSampleNotes();
void showDoRecord();
void checkSerialColors();
void loop();
FLASHMEM void triggerExternalOneBlink();
FLASHMEM void updateExternalOneBlink();
float getNoteDuration(int channel);
FLASHMEM void shiftNotes();
void tmpMuteAll(bool pressed);
static void silenceAllVoicesForFullMute();
static void syncMuteArraysToSmp();
FLASHMEM void fullMuteApplyView();
FLASHMEM void fullMuteEnter();
FLASHMEM void fullMuteExitRestore();
FLASHMEM void fullMuteToggleView(FullMuteView target);
FLASHMEM void toggleMute();
void deleteActiveCopy();
void play(bool fromStart);
void pause(bool skipSave);
static int arpSigned(int ch);
static bool arpUsesEighths(int ch);
static int arpPingPongIndex(uint32_t step, int spanSteps);
static int arpSpanSteps(int ch);
static void computeMonoSynthFilterEnv(int ch, unsigned long nowMs, float baseHz,
                                      float baseRes, float &outHz,
                                      float &outRes, bool &outActive);
static void playSynthInternal(int ch, int b, int vel, bool persistant,
                              bool legato, int midiPitchOverride);
void playSynth(int ch, int b, int vel, bool persistant);
void playNote();
void checkPages();
void unpaint();
void playFillNote();
static void triggerCurrentVoiceAudition(int pitchRow, int auditionVelocity);
void triggerGridNote(unsigned int globalX, unsigned int y, bool allowMuted);
void paint();
void toggleCopyPaste();
void clearNoteChannel(unsigned int c, unsigned int yStart, unsigned int yEnd, unsigned int channel_to_clear, bool singleModeActive);
FLASHMEM void updatePreviewVolume();
void updateBrightness();
FLASHMEM void switchSubPattern();
FLASHMEM void updateBPM();
FLASHMEM void setBPM();
FLASHMEM void showLoadSave();
FLASHMEM void showSamplePack();
FLASHMEM void loadSamplePack(unsigned int pack_id, bool intro, bool preserveSp0Custom);
void updateVoiceLengths();
void voiceFillColumn(unsigned int col, unsigned int globalPage, unsigned int srcOut[16]);
unsigned int voicePickStep(const unsigned int src[16], unsigned int row);
void voiceCuePage(int channel, int page);
unsigned int voiceOwnedStep(unsigned int row, unsigned int col, unsigned int globalPage);
bool voicePageFilled(int channel, int page);
int voiceLastFilledPage(int channel);
void voiceLimitPageEncoder(int channel);
void voiceApplyEditPage(int channel);
FLASHMEM void loadWav();
bool findSliderDefPageSlot(int chan, SettingArray arr, int8_t idx, int &page, int &slot);
int getDefaultFastFilterValue(int channel, SettingArray arr, int8_t idx);
void setDefaultFastFilterValue(int channel, SettingArray arr, int8_t idx, int value);
void setSliderDefForChannel(int channel);
FLASHMEM void initPageMutes();
FLASHMEM bool getMuteStateForUI(int channel);
static void stopMutedVoiceAudio(int ch);
FLASHMEM void setMuteState(int channel, bool muted);
FLASHMEM void unmuteAllChannels();
FLASHMEM void applyMutesAfterPMODSwitch();
FLASHMEM void copyPageMutes(int fromPage, int toPage);
FLASHMEM void clearPatternNotes();
FLASHMEM void deleteAutosaveFile();
static bool patternWriteAll(File &f, const uint8_t *data, size_t len);
FLASHMEM bool validPatternFile(const char *path, bool requirePitch);
FLASHMEM void showPatternResult(bool success);
FLASHMEM void savePattern(bool autosave);
FLASHMEM void saveSamplePack(int pack);
FLASHMEM void loadPattern(bool autoload);
FLASHMEM void autoLoad();
FLASHMEM void autoSave();
static bool isVoicePresetChannel(int ch);
void invalidateVoicePresetUi();
static bool buildVoicePresetPath(int ch, uint16_t slot, char *out, size_t outSize);
static bool ensureVoicePresetDirs(int ch);
static uint8_t readPresetSettingValue(int ch, SettingArray arr, int idx);
static void writePresetSettingValue(int ch, SettingArray arr, int idx, uint8_t value);
static bool voicePresetFileValid(const char *path, bool );
FLASHMEM bool saveVoicePreset(int ch, uint16_t slot);
FLASHMEM bool loadVoicePreset(int ch, uint16_t slot);
void updateMixerGains(int i);
void initFilterTransition(int index);
void applyVoiceCuts(int index);
static float voiceDelayMsForSlider(int slider);
void applyVoiceDelay(int ch);
void applyAllVoiceDelays();
void applyMonoSynthCrf(int ch);
void setFilters(FilterType filterType, int index, bool initial);
void setFilterDefaults(int channel);
static bool channelHasFreeverb(uint8_t chan);
uint8_t scaleToDisplay(const SliderDefEntry& meta, uint8_t val);
uint8_t readSetting(SettingArray arr, int8_t idx, uint8_t chan);
void drawSliderName(uint8_t x0, uint8_t x1, const char* name, CRGB filtercol);
void drawCornerValueCustom(uint8_t encoderIndex, uint8_t val, const SliderDefEntry& meta);
void initSliders(uint8_t page, uint8_t chan);
void slider(uint8_t page);
void toggleDefaultFilterFromSlider(uint8_t page, uint8_t encoder);
void processAdjustments_new(uint8_t page);
void setNewFilters();
void showFilterPages(uint8_t chan);
void showFilterNames(uint8_t chan);
void updateFilterEncoderColors();
unsigned int sampleBrowserHashName(const char* s);
static bool hasWavExt(const char* name);
static void copyBaseName64(const char* rawName, char* out, size_t outSz);
static bool soloRandomIsJunkName(const char* base);
static void soloRandomJoinRel(char* out, size_t outSz, const char* dir, const char* name);
static void soloRandomSetLastFromIndex();
static bool wavIndexPushPath(const char* path);
FLASHMEM bool loadSampleManifest();
FLASHMEM bool scanAndWriteManifest();
static bool soloRandomAppendOneFromIndex();
FLASHMEM bool soloRandomRenewPlaylist();
static void sortNameBlock(char names[][SAMPLE_BROWSER_NAME_MAX], int n);
static void sortNameBlockWithSizes(char names[][SAMPLE_BROWSER_NAME_MAX], uint32_t sizes[], int n);
static void browseDirGoParent(int ch);
void sampleBrowserInvalidate();
int sampleBrowserBrowseIndexMax(int channel);
void sampleBrowserClampBrowseIndex(int channel);
void sampleBrowserClampBrowseIndexAndHardware(int channel);
static bool sampleBrowserAppendFolderSegment(int channel, const char* name);
static void sampleBrowserGetLeafFolderName(int channel, char* out, size_t outSize);
uint8_t sampleBrowserEntryTypeAt(int idx);
static void sampleBrowserFinishFolderStep(int channel, unsigned int prevWavPos, const char* preferredFolderName);
FLASHMEM void sampleBrowserRefreshList(int channel);
FLASHMEM void sampleBrowserSyncBrowseFromStoredPath(int ch);
bool sampleBrowserIsLoadableSelection(int channel);
FLASHMEM void sampleBrowserNavigatePress(int channel);
FLASHMEM void sampleBrowserLastEncoderPress(int channel);
FLASHMEM void sampleBrowserGoUpPress(int channel);
FLASHMEM void sampleBrowserClearAll();
FLASHMEM void generateNextNumericName(int folderIdx, char* outName, size_t outSize);
FLASHMEM void buildSamplePath(int folderIdx, int fileIdx, char* out, size_t outSize);
FLASHMEM void loadSMPSettings();
void writeWavHeader(File &file, uint32_t sampleRate, uint8_t bitsPerSample, uint16_t numChannels);
void finalizeWavHeader(File &file, uint32_t dataSize);
float recInputMonitorGain();
void startRecordingRAM();
void flushAudioQueueToRAM2();
void stopRecordingRAM(int fnr, int snr);
void startFastRecord();
void flushAudioQueueToRAM();
void stopFastRecord();
FLASHMEM void clearAllNotesOfChannel();
FLASHMEM void FastLEDclear();
FLASHMEM void FastLEDshow();
static void presentGridThenRecordingOverlay(bool dotted);
void presentRecordingBorder();
int getPage(int x);
FLASHMEM void drawRandoms();
FLASHMEM void drawRandomsMultiPage();
FLASHMEM void generateRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
FLASHMEM void generateMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
FLASHMEM void generateBassOrMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
FLASHMEM void generateBassLine(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
FLASHMEM void generateMainMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
FLASHMEM int createHarmonicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
FLASHMEM int createMelodicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
FLASHMEM int createRhythmicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
FLASHMEM void generateGenreTrack();
FLASHMEM void setGenreBPM();
FLASHMEM void generateTechnoPattern(unsigned int start, unsigned int end, unsigned int page);
FLASHMEM void generateHipHopPattern(unsigned int start, unsigned int end, unsigned int page);
FLASHMEM void generateDnBPattern(unsigned int start, unsigned int end, unsigned int page);
FLASHMEM void generateHousePattern(unsigned int start, unsigned int end, unsigned int page);
FLASHMEM void generateAmbientPattern(unsigned int start, unsigned int end, unsigned int page);
FLASHMEM void generateBasicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
FLASHMEM void clearPage();
FLASHMEM void resetAllAudioEffects();
FLASHMEM void startNew();
int getFolderNumber(int value);
FLASHMEM void runAnimation();
FLASHMEM void runInfoLogoAnimationLoop();
FLASHMEM void generateSong();
void initLedStrip();
void setLedStripEnabled(bool enabled);
bool getLedStripEnabled();
void triggerLedStripRipple(uint8_t channel);
void updateLedStrip();
void onNoteTriggered(uint8_t channel);
FLASHMEM void clearLedStripForScreensaver();
static MenuPage* activeSystemPages();
static int activeSystemPageCount();
int systemMenuParentPage();
FLASHMEM void menuRequestFullRedraw();
FLASHMEM void togglePulseClockPolarityFromMenu();
FLASHMEM void toggleMixGainEnc3Target();
static int textPixelWidth_3x5(const char *text);
FLASHMEM void resetEtcInfoPageAnimation();
static int computeReverbUsagePercent();
static int computeAudioUsedPercent();
static int computeAudioFreeBlocks();
FLASHMEM void noteMenuExit();
FLASHMEM void invalidateEnc3RangeCache();
FLASHMEM void prepareMenuEntry();
static uint32_t crc32_compute(const uint8_t *data, size_t len);
static bool readLine(File &f, char *buf, size_t bufSize);
static int hexNibble(char c);
static bool decodeHexBytes(const char *hex, uint8_t *out, size_t outLen);
static void encodeHexBytes(const uint8_t *in, size_t inLen, char *out, size_t outSize);
void markSettingsBackupDirty();
FLASHMEM void serviceSettingsBackup();
FLASHMEM void loadMenuFromEEPROM();
FLASHMEM void applyAudioSettingsFromGlobals();
FLASHMEM void saveSingleModeToEEPROM(int index, int8_t value);
static void saveTransportDelayToEEPROM(int slotBase, int8_t value);
FLASHMEM void saveSp0StateToEEPROM();
FLASHMEM void loadSp0StateFromEEPROM();
FLASHMEM void showMenu();
FLASHMEM void showLookMenu();
FLASHMEM void showRecsMenu();
FLASHMEM void showMidiMenu();
FLASHMEM void showVolMenu();
FLASHMEM void showEtcMenu();
FLASHMEM void drawMainSettingStatus(int setting);
FLASHMEM void drawAdditionalFeatures(int setting);
FLASHMEM void drawGenreSelection();
void cycleFireMenuFocus();
FLASHMEM bool handleAdditionalFeatureControls(int setting);
FLASHMEM void switchMenu(int menuPosition);
FLASHMEM void resetNewModeState();
FLASHMEM void drawSimpleNotesView();
FLASHMEM void drawLoopLength();
FLASHMEM void drawLedModules();
FLASHMEM void showNewFileScreen();
FLASHMEM void showNewFileMode();
FLASHMEM void enterEtcSdFromHost();
FLASHMEM int getCurrentMenuMainSetting();
FLASHMEM void clearTextArea(int startX, int startY, int width);
FLASHMEM void drawRecChannelClear();
FLASHMEM void drawRecMode();
FLASHMEM void drawMidiVoiceSelect();
FLASHMEM void drawFastRecMode();
FLASHMEM void drawPatternMode();
FLASHMEM void drawFlowMode();
FLASHMEM void drawMidiTransport();
FLASHMEM void drawMidiSend();
FLASHMEM void openTouchyMainMenu();
unsigned long effectiveTransportSendDelayMs();
unsigned long effectiveTransportRecvDelayMs();
bool getBPMStable();
uint16_t getMidiClockTicks();
static void markExternalOne();
int collectHeldMidiNotes(int ch, uint8_t *pitches, uint8_t *vels, int maxN);
bool analogClockPulseFresh();
static void pulseClockEndIsr();
static void pulseClockForceIdle();
static void pulseClockEmit();
static void pulseClockApplyIdle();
static void sidechainApplyIdle();
static void sidechainEndIsr();
static bool sidechainVoiceOk(uint8_t ch);
uint8_t getSidechainVoice();
void setSidechainVoice(uint8_t ch);
void loadSidechainFromEEPROM();
bool getAnalogExtClock();
void setAnalogExtClock(bool analog);
void toggleAnalogExtClock();
void loadAnalogExtClockFromEEPROM();
static void saveAnalogInSettings();
void sidechainOnVoice(uint8_t ch);
static uint8_t analogClockInPpqn();
uint8_t getAnalogInPpqn();
uint8_t getAnalogInPpqnIndex();
uint8_t getAnalogInPpqnCount();
void setAnalogInPpqnIndex(uint8_t idx);
uint8_t getAnalogInHoldMs();
void setAnalogInHoldMs(uint8_t ms);
static void analogPlayWholeStep();
static void analogSubstepIsr();
static void analogDirectPulse();
void analogClockCatchIfHigh();
void initAnalogClockHardware();
void pulseClockMidiTick();
bool getPulseClockEnabled();
bool getPulseClockPolarityPositive();
bool getPulseClockStopWithPlay();
uint8_t getPulseClockWidthMs();
uint8_t getPulseClockWidthMsMin();
uint8_t getPulseClockWidthMsMax();
uint8_t getPulseClockPpqn();
uint8_t getPulseClockRateSelCount();
uint8_t getPulseClockRateSel();
static void pulseClockSaveToEEPROM();
static void pulseClockSaveWidthToEEPROM();
void setPulseClockPolarityPositive(bool positive);
void togglePulseClockPolarity();
void setPulseClockStopWithPlay(bool stopWithPlay);
void setPulseClockWidthMs(uint8_t ms);
void setPulseClockRateSel(uint8_t sel);
void setPulseClockEnabled(bool enabled);
void loadPulseClockFromEEPROM();
void midiClockTick();
void updateMidiClockOutput();
void handleMidiClock();
void checkMidi();
void resetMidiClockState();
void resetMidiClockForTransportStart();
void myClock(unsigned long now_captured);
void MidiSendNoteOn(int pitch, int channel, int velocity);
void MidiReleaseSequencerNotes();
void handleStop();
static int mapMidiToLogicalChannel(int midiChannel, uint8_t pitch);
uint8_t midiPitchToGridRow(int channel, uint8_t pitch);
static bool preserveIncomingMidiPitch();
static void playIncomingMidiNote(int ch, uint8_t pitch, uint8_t row,
                                 uint8_t velocity);
void handleNoteOn(int ch, uint8_t pitch, uint8_t velocity);
static bool midiRecordCellMatches(const Note &cell, uint8_t channel,
                                  uint8_t pitch);
void onBeatTick(unsigned int currentBeat, unsigned int previousBeat);
void handleStart();
void armMasterTransportStartDelay();
void handleTimeCodeQuarterFrame(uint8_t data);
void handleSongPosition(uint16_t beats);
void setParams(ParameterType paramType, int index);
static int defaultSettingValue(int ch, SettingArray arr, int8_t idx);
static void applyDefaultAdsrSettings(int ch);
static void setFilterPageDefaultValues(int ch, uint8_t page);
void setCurrentFilterPageDefaultValues(int ch);
void setAllFilterPagesDefaultValues(int ch);
static uint8_t synthInstrumentFormDefault(int instrumentIdx);
uint8_t synthInstrumentWaveDefault(int instrumentIdx);
void applySynthInstrumentFormDefault(int channel, int instrumentIdx);
void resetAllToDefaults();
bool isEncoder0PressedMode();
void setEncoder0PressedMode(bool state);
bool shouldSkipVoiceTriggerForSyncPreview(int ch);
void triggerSetWavSyncPreview(uint8_t vel);
void serviceSetWavSyncPreview();
static bool buildCurrentBrowseSamplePath(char *out, size_t outSize);
static bool commitCurrentBrowsePathToChannel(int ch);
static void stopSdPreviewAndWait();
static void startSdPreviewNow(const char *path);
static void requestSdPreview(const char *path);
void serviceSdPreviewRequests();
static bool findWavDataChunk(File &f, uint32_t &outStart, uint32_t &outSize);
static void stopPeakScan();
static void servicePeakScan(uint32_t maxBytesThisCall);
static void loadEmptySampleToChannel(unsigned int sampleID);
FLASHMEM void loadSample(unsigned int packID, unsigned int sampleID);
static CRGB fileSizeToColor(uint32_t bytes);
static void drawSampleBrowserDepthVisual(int ch);
void invalidateShowWaveBrowserSync();
void showWave();
static bool sampleLoadProgressAllowed();
static void requestDeferredSp0Copy(unsigned int channel);
static void sampleLoadUiThrottled(uint8_t percent, bool enable, uint8_t &lastDrawn);
FLASHMEM void servicePendingSamplePersist();
FLASHMEM void copySampleToSamplepack0(unsigned int channel, bool showLoadProgress);
void reversePreviewSample();
bool loadPreviewToChannel(unsigned int targetChannel, bool showLoadProgress);
static void sdSerDrainRx();
static void sdSerAudioStopForSd();
static void sdSerSdLock();
static void sdSerSdUnlock();
static uint32_t sdSerCrc32Update(uint32_t crc, const uint8_t *data, size_t len);
static void sdSerReply(const char *msg);
static void sdSerReplyf(const char *fmt, ...);
static void sdSerReplyFlush(const char *msg);
static bool sdSerReadExact(uint8_t *dst, size_t len, uint32_t timeoutMs);
static bool sdSerNormalizePath(const char *in, char *out, size_t outLen);
static void sdSerCmdPing();
static void sdSerCmdList(const char *pathArg);
static void sdSerCmdMv(char *args);
static void sdSerCmdRm(const char *pathArg);
static void sdSerCmdMkdir(const char *pathArg);
static void sdSerCmdPut(char *args);
static bool sdSerWriteAll(const uint8_t *data, size_t len);
static bool sdSerWaitAck(uint32_t timeoutMs);
static void sdSerCmdGet(const char *pathArg);
static void sdSerNoteHostPresent();
static void sdSerExpireClient();
static void sdPatPack();
static bool sdPatApply();
static void sdSerCmdGetPat();
static void sdSerCmdPutPat(char *args);
static void sdSerHandleLine(char *line);
bool sdSerialServerIsActive();
bool sdSerialServerClientConnected();
void sdSerialServerSetActive(bool on);
void sdSerialServerPoll();
static void sdSerHandleOffPage(char *line);
void sdSerialServerPollNeedSdHint();
static float synthNoteFrequency(int noteIndex);
static void setSoundVoice(int note, int ch, int velocity, int voiceIdx,
                          bool triggerEnvelope);
void arp_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void lead_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void keys_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void pad_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void organ_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void flute_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void wow_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void bass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void brass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void chiptune_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void acid303_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void dx7_epiano_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void dx7_bass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6);
void updateVals(int ch);
static uint8_t synthWaveformControlColumn(uint8_t rawValue);
static void applySynthWaveformControl(int instrument, int ch,
                                      uint8_t rawValue);
static void releasePolyVoice(int voiceIdx);
static unsigned synthAutoOffMsForInstrument(int inst);
void autoOffActiveNotes();
void updateSynthVoice(int channel);
void switchSynthVoice(int typeValue, int ch, int p1,int p2, int p3, int p4, int p5, int p6);
void onTouchyModeEnter();
void syncTouchyEncoders();
bool consumeTouchyEntryHold(const uint8_t states[NUM_ENCODERS]);
FLASHMEM void setTouchyEnabled(bool enabled);
static void selectTouchyVoice(uint8_t voice);
FLASHMEM void checkTouchyTouches();
FLASHMEM void handleTouchyButtons(const uint8_t states[NUM_ENCODERS]);
FLASHMEM void showTouchyMode();
void light(unsigned int x, unsigned int y, CRGB color);
CRGB getIndicatorColor(char colorCode);
CRGB getCurrentChannelColor();
CRGB applyHighlight(CRGB color, bool highlight);
CRGB normalizeToMaxBrightness(CRGB color);
void drawIndicator(char size, char colorCode, int encoderNum, bool highlight, bool setEncoderLed, uint8_t brightness);
void drawFilterCheck(int mappedValue, FilterType fx, CRGB color);
void drawPlayButton();
void drawNoSD();
void drawBase();
FLASHMEM void resetPongGame();
static bool pongNoteAt(int px, int py);
static void triggerPongCollision(int px, int py);
FLASHMEM void updatePongBall();
FLASHMEM void drawPongBall();
FLASHMEM void showSaveSuccessAnimation(bool returnToDraw);
FLASHMEM void showSaveFailureAnimation(bool returnToDraw);
FLASHMEM void drawRecordingBorder();
FLASHMEM void drawRecordingBorderDotted();
FLASHMEM void drawFullMuteBorder();
FLASHMEM void drawFullMuteIndicators();
FLASHMEM void drawBatteryWarning();
void drawStatus();
FLASHMEM void drawText(const char *text, int startX, int startY, CRGB color);
FLASHMEM void drawChar(char c, int x, int y, CRGB color);
FLASHMEM void drawNumber(float count, CRGB color, int topY);
FLASHMEM void drawVelocity();
FLASHMEM void drawCtrlVolumeOverlay(int volume);
FLASHMEM void drawInputGainOverlay(int gain, int maxGain);
FLASHMEM void drawChannelNrOverlay(int channelNum, int channelIdx);
FLASHMEM void drawPageNrOverlay(int pageNum);
FLASHMEM void drawSampleLoadOverlay(uint8_t progressPercent);
void drawPages();
FLASHMEM void drawTriggers();
static void fireClear();
void drawTimer();
FLASHMEM void drawCountIn();
FLASHMEM void drawSensCountIn();
void drawCursor();
FLASHMEM void drawBrightness();
FLASHMEM void showIconsAt(IconType ico, CRGB colors, int ox, int oy);
FLASHMEM void showIcons(IconType ico, CRGB colors);
void processPeaks();
void drawLoadingBar(int minval, int maxval, int currentval, CRGB color, CRGB fontColor, bool intro);
FLASHMEM void drawBPMWithReservedSpace(float bpm, CRGB color, int topY);
FLASHMEM void drawBPMScreen();
void drawKnobColorDefault();
FLASHMEM void showSongMode();
#line 543 "/Users/jank/git/toern/toern.ino"
void clearLiveRecordSkip() {
  noInterrupts();
  liveRecordSkipBeat = 0;
  liveRecordSkipRows = 0;
  interrupts();
}

static void markLiveRecordSkip(uint16_t targetBeat, uint8_t row) {
  extern unsigned int beat;
  noInterrupts();
  uint16_t upcoming = (uint16_t)beat;
  if (targetBeat == upcoming && row >= 1 && row <= 15) {
    if (liveRecordSkipBeat != targetBeat) {
      liveRecordSkipBeat = targetBeat;
      liveRecordSkipRows = 0;
    }
    liveRecordSkipRows |= (uint16_t)(1u << row);
  }
  interrupts();
}

static bool consumeLiveRecordSkip(uint16_t srcBeat, unsigned int row) {
  if (liveRecordSkipBeat != srcBeat || row > 15) return false;
  uint16_t bit = (uint16_t)(1u << row);
  if ((liveRecordSkipRows & bit) == 0) return false;
  liveRecordSkipRows = (uint16_t)(liveRecordSkipRows & ~bit);
  if (liveRecordSkipRows == 0) liveRecordSkipBeat = 0;
  return true;
}



bool placeLiveGridNote(uint8_t channel, uint8_t row, uint8_t velocity,
                       uint8_t midiPitch, bool protectOtherVoices,
                       bool protectOccupied) {
  extern Note note[][maxY + 1];
  if (channel < 1 || row < 1 || row > 15) return false;
  if (isChildVoiceDisabled((int)channel)) return false;
  extern uint16_t quantizedMidiRecordBeat();
  uint16_t targetBeat = quantizedMidiRecordBeat();
  if (targetBeat < 1) return false;
  Note &cell = note[targetBeat][row];
  if (protectOccupied && cell.channel != 0) return false;
  if (protectOtherVoices && cell.channel != 0 && cell.channel != channel) {
    return false;
  }
  markLiveRecordSkip(targetBeat, row);
  cell.velocity = velocity > 0 ? velocity : (uint8_t)defaultVelocity;
  cell.probability = 100;
  cell.condition = 1;
  cell.midiPitch = midiPitch;
  cell.channel = channel;



  extendLastPageForLiveNote(channel, row, targetBeat);
  return true;
}



const unsigned long phase1Duration = 1000;
const unsigned long phase2Duration = 2500;
const unsigned long phase3Duration = 1500;
const unsigned long totalAnimationTime =
    phase1Duration + phase2Duration + phase3Duration;
bool filterfreshsetted = true;

DMAMEM bool activeNotes[128] = { false };
DMAMEM int16_t midiHeldCh11SoundNote[128] = {};
DMAMEM uint8_t sequencerMidiOutPitch[16] = {};
DMAMEM uint8_t sequencerMidiOutChannel[16] = {};
uint8_t sequencerMidiOutCount = 0;

float rateFactor = 44117.0 / 44100.0;


bool freshPaint, tmpMute = false;
bool preventPaintUnpaint = false;
unsigned long suppressDrawRMuteUntilMs = 0;

bool firstcheck = false;
bool nofile = false;
unsigned int fxType = 0;

unsigned long filterDrawEndTime = 0;
bool filterDrawActive = false;
int filterDrawValue = 0;
CRGB filterDrawColor = CRGB::White;

unsigned long patternChangeTime = 0;
bool patternChangeActive = false;


uint32_t oldPosHash = 0;
uint8_t oldButtons[NUM_ENCODERS] = { 0, 0, 0, 0 };

unsigned long playStartTime = 0;

bool previewIsPlaying = false;
extern bool g_previewUseLivePeaks;

const int maxPeaks = 512;
DMAMEM float peakValues[maxPeaks];
int peakIndex = 0;

const int maxRecPeaks = 512;
DMAMEM float peakRecValues[maxRecPeaks];
int peakRecIndex = 0;

uint8_t ledBrightness = 83;
bool drawBaseColorMode = true;
bool pong = false;
const long ram = 14461440;
const unsigned int MAX_STEPS = MATRIX_WIDTH * maxPages;
const unsigned int maxlen = MAX_STEPS + 1;
const unsigned int SONG_LEN = MAX_STEPS;

static bool lastBothTouched = false;
DMAMEM bool touchState[4] = { false };
DMAMEM bool lastTouchState[4] = { false };
struct EncoderTurnEvent {
  int encoder;
  int delta;
  bool fn;
};

static bool dispatchFnTurn(const EncoderTurnEvent &event);

const int touchThreshold = 45;
bool readDebouncedTouch(int index);
static bool bypassModeSwitchDebounce = false;
static bool touch1ModeTogglePending = false;
static bool touch1ModeToggleToSingle = false;
static unsigned long touch1ModeToggleDueMs = 0;
static const unsigned long TOUCH1_CHORD_GRACE_MS = 140;


static const bool exttouch = false;

static const bool deviceHasFaders = exttouch;

bool readTouch1Pressed() {
  return readDebouncedTouch(0);
}

bool readTouch2Pressed() {
  return readDebouncedTouch(1);
}

bool readRecTouchHeld() {
  return readDebouncedTouch(2);
}

const unsigned int totalPulsesToWait = pulsesPerBar * 2;




int recMode = -1;
unsigned int fastRecMode = 0;
unsigned int previewVol = 20;
uint8_t mixGain14 = MIX_GAIN_UNITY;
uint8_t mixGain58 = MIX_GAIN_UNITY;
uint8_t mixGainSynth = MIX_GAIN_UNITY;
uint8_t mixGainMaster = MIX_GAIN_UNITY;

extern const int PREVIEW_MODE_ON = 0;
extern const int PREVIEW_MODE_PRESS = 1;
extern const int PREVIEW_MODE_SYNC = 2;
int previewTriggerMode = PREVIEW_MODE_ON;
int recChannelClear = 0;
int transportMode = 1;
int midiSendMode = 2;
int patternMode = -1;
int flowMode = 1;
int clockMode = 1;
int voiceSelect = 1;
int simpleNotesView = 1;
int loopLength = 0;
int ledModules = 1;
bool ledModulesRotated = false;
unsigned int maxX = MATRIX_WIDTH * 1;
unsigned int micGain = 0;
unsigned int lineInLevel = 8;
int ctrlMode = 1;
static int ctrlLastChannel = -1;
static int ctrlLastVolume = -1;
static bool ctrlVolumeOverlayActive = false;

DMAMEM static uint8_t lastChannelVolBeforeMute[maxY] = { 0 };
static unsigned long ctrlVolumeOverlayUntil = 0;
static int ctrlVolumeOverlayValue = 0;
static bool inputGainOverlayActive = false;
static unsigned long inputGainOverlayUntil = 0;
static int inputGainOverlayValue = 0;
static int inputGainOverlayMax = 63;

static bool channelNrOverlayActive = false;
static unsigned long channelNrOverlayUntil = 0;
static int channelNrOverlayChannel = 0;
static bool pageNrOverlayActive = false;
static unsigned long pageNrOverlayUntil = 0;
static int pageNrOverlayPage = 0;
static const int DEFAULT_CHANNEL_VOLUME = 16;


bool getSpkrEnabled() {
  return spkrEnabled;
}

void applySpkrHardware();
void applySgtl5000AdcHighPass();
void applyStereoChannelRouting();


void setSpkrEnabled(bool enabled) {
  spkrEnabled = enabled;
  applySpkrHardware();
}

bool getAdcHpfEnabled() { return adcHpfEnabled; }
void setAdcHpfEnabled(bool enabled) {
  adcHpfEnabled = enabled;
  applySgtl5000AdcHighPass();
}
bool getUsbSmpPreview() { return usbSmpPreview; }
void setUsbSmpPreview(bool enabled) {
  usbSmpPreview = enabled;
  applyStereoChannelRouting();
}


bool getChildLockEnabled() {
  return childLockEnabled;
}


void setChildLockEnabled(bool enabled) {
  childLockEnabled = enabled;
  enforceChildModeRestrictions();
}

bool isRecording = false;
File frec;

#define MAXREC_SECONDS 12
#define BUFFER_SAMPLES (22100 * MAXREC_SECONDS)
#define BUFFER_BYTES (BUFFER_SAMPLES * sizeof(int16_t))

EXTMEM int16_t recBuffer[BUFFER_SAMPLES];
volatile size_t recWriteIndex = 0;



unsigned int recInput = AUDIO_INPUT_LINEIN;





float pulse = 1;
int dir = 1;
double playNoteInterval = 150000.0;


unsigned int RefreshTime = 1000UL / TargetFPS;
float marqueePos = maxX;
bool shifted = false;
bool movingForward = true;
unsigned int lastUpdate = 0;
unsigned int lastDrawUpdate = 0;

unsigned int totalInterval = 0;

DMAMEM bool hasNotes[maxPages + 1];
DMAMEM unsigned int startTime[maxY] = { 0 };
DMAMEM bool noteOnTriggered[maxY] = { false };
DMAMEM bool persistentNoteOn[maxY] = { false };
DMAMEM int16_t pressedKeyCount[maxY] = { 0 };
DMAMEM uint32_t sequencedMonoSynthLastTick[maxY] = { 0 };
DMAMEM int16_t sequencedMonoSynthLastPitch[maxY] = { 0 };
DMAMEM bool sequencedMonoSynthActive[maxY] = { false };

bool waitForFourBars = false;
unsigned int pulseCount = 0;
bool sampleIsLoaded = false;
bool unpaintMode, paintMode = false;



static inline bool isPaintableDrawRow(unsigned int y) {

  if (y <= 1 || y > 15) return false;


  if (!imageMode && (y == 10 || y == 11 || y == 13)) return false;
  return true;
}

static inline bool isSingleModeChannelAllowed(int channel) {
  return (channel >= 1 && channel <= 8) ||
         channel == 11 || channel == 13 || channel == 14;
}


static inline int basePitchRowForChannel(int channel) {
  if (channel >= 1 && channel <= 8) return channel + 1;
  if (channel == 11) return 12;
  if (channel == 13 || channel == 14) return channel - 11;
  return -1;
}



static inline int imageModePaintChannel(int existingChannel) {
  static const int seq[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  const int n = (int)(sizeof(seq) / sizeof(seq[0]));
  int start = 0;
  if (existingChannel > 0) {
    for (int i = 0; i < n; i++) {
      if (seq[i] == existingChannel) {
        start = (i + 1) % n;
        break;
      }
    }
  }
  for (int k = 0; k < n; k++) {
    int ch = seq[(start + k) % n];
    if (!isChildVoiceDisabled(ch)) {
      setMuteState(ch, false);
      return ch;
    }
  }
  setMuteState(1, false);
  return 1;
}

static inline bool canEnterSingleMode(unsigned int y, int channel) {
  if (imageMode) return false;
  return y < 16 &&
         isSingleModeChannelAllowed(channel) &&
         !isChildVoiceDisabled(channel);
}

void enforceImageModeConstraints();


unsigned int beat = 1;
volatile uint16_t sequencerRecordBeat = 1;
volatile uint16_t sequencerPreviousRecordBeat = 1;
volatile uint32_t sequencerRecordStepStartedUs = 0;
volatile uint32_t sequencerRecordStepDurationUs = 150000;
uint32_t sequencerSynthTick = 0;


unsigned int beatForUI = 1;

uint16_t quantizedMidiRecordBeat() {
  uint16_t playedBeat;
  uint16_t upcomingBeat;
  uint32_t startedUs;
  uint32_t durationUs;
  noInterrupts();
  playedBeat = sequencerRecordBeat;
  upcomingBeat = (uint16_t)beat;
  startedUs = sequencerRecordStepStartedUs;
  durationUs = sequencerRecordStepDurationUs;
  interrupts();

  if (startedUs == 0 || durationUs == 0) return upcomingBeat;
  uint32_t elapsed = micros() - startedUs;


  return elapsed < (durationUs / 2u) ? playedBeat : upcomingBeat;
}

uint16_t loopCount = 0;
unsigned int samplePackID, fileID = 1;
IntervalTimer playTimer;
IntervalTimer midiClockTimer;
IntervalTimer fillTimer;



volatile unsigned int fillSubBeat = 0;


volatile uint32_t fillSubTick = 0;




volatile bool fillRunning = false;
volatile bool fillHasTriggered = false;
volatile uint32_t fillStartSubTick = 0;

volatile unsigned int fillTriggerStep = 0;
volatile unsigned int fillTriggerRow = 0;

volatile int fillActiveChannel = 0;
volatile int fillActiveVelocity = 0;
volatile unsigned int fillActiveRow = 0;
volatile uint8_t fillActiveMidiPitch = NOTE_MIDI_PITCH_NONE;
unsigned int lastPage = 1;
int editpage = 1;
EXTMEM Note note[maxlen + 1][maxY + 1] = {};
EXTMEM Note tmp[maxlen + 1][maxY + 1] = {};
EXTMEM Note original[maxlen + 1][maxY + 1] = {};
EXTMEM Note pageTmp[(MATRIX_WIDTH * 2) + 1][maxY + 1] = {};

bool isNowPlaying = false;


uint32_t lastUserActivityMs = 0;
FLASHMEM void noteUserActivity() {
  lastUserActivityMs = millis();
}

int PrevSampleRate = 1;
EXTMEM int SampleRate[maxFiles] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
EXTMEM unsigned char sampled[maxFiles][ram / (maxFiles)];


static const size_t FASTREC_SLOT_SAMPLES = sizeof(sampled[0]) / sizeof(int16_t);
static const size_t FASTREC_MAX_SECONDS = 12;
static const size_t FASTREC_MAX_SAMPLES =
    ((size_t)44100 * FASTREC_MAX_SECONDS < FASTREC_SLOT_SAMPLES)
        ? ((size_t)44100 * FASTREC_MAX_SECONDS)
        : FASTREC_SLOT_SAMPLES;

elapsedMillis msecs;
elapsedMillis mRecsecs;
elapsedMillis diagStatsTimer;

DMAMEM CRGB leds[NUM_LEDS];

enum ButtonState {
  IDLE,
  LONG_PRESSED,
  RELEASED
};

const int ALL_CHANNELS[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14 };
const int NUM_ALL_CHANNELS = sizeof(ALL_CHANNELS) / sizeof(ALL_CHANNELS[0]);

struct Mode {
  String name;
  unsigned int minValues[4];
  unsigned int maxValues[4];
  unsigned int pos[4];
  uint32_t knobcolor[4];
};

Mode draw = { "DRAW", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxlen - 1 }, { 1, maxY, maxfilterResolution, 1 }, { 0x110011, 0x000000, 0x00FF00, 0x110011 } };
Mode singleMode = { "SINGLE", { 1, 1, 0, 1 }, { maxY, maxPages, maxfilterResolution, maxlen - 1 }, { 1, 2, maxfilterResolution, 1 }, { 0x000000, 0x000000, 0x00FF00, 0x000000 } };
Mode bpmMode = { "BPM", { 11, 0, 0, BPM_MIN }, { 30, 252, 1, BPM_MAX }, { 11, 0, 0, 100 }, { 0x000000, 0x000000, 0xFF4400, 0x00FFFF } };

Mode filterMode = { "FILTERMODE", { 0, 0, 0, 0 }, { maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution }, { 0, 0, 0, 0 }, { 0x00FFFF, 0xFF00FF, 0xFFFF00, 0x00FF00 } };
Mode noteShift = { "NOTE_SHIFT", { 7, 7, 0, 7 }, { 9, 9, maxfilterResolution, 9 }, { 8, 8, maxfilterResolution, 8 }, { 0xFFFF00, 0xFFFF00, 0x000000, 0xFFFFFF } };
Mode velocity = { "VELOCITY", { 1, 1, 1, 0 }, { maxY, 5, NOTE_CONDITION_STEP_COUNT, maxY }, { maxY, 5, 1, 10 }, { 0xFF4400, 0x00FF88, 0x888888, 0x0044FF } };

Mode set_Wav = { "SET_WAV", { 1, 0, 1, 1 }, { 9999, 999, 9999, 999 }, { 0, 0, 0, 1 }, { 0x000000, 0x000000, 0x00FF00, 0xFFFFFF } };
Mode recordMode = { "RECORD_MODE", { 0, 1, 1, 1 }, { 100, FOLDER_MAX, 9999, 999 }, { 0, 0, 0, 1 }, { 0xFF0000, 0x00FF00, 0x0000FF, 0x000000 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 0 }, { 1, 1, 99, 999 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };

Mode loadSaveTrack = { "LOADSAVE_TRACK", { 1, 1, 0, 0 }, { 1, 1, 1, 1000 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };

Mode loadSavePreset = { "LOADSAVE_PRESET", { 1, 1, 1, 1 }, { 1, 1, 1, 99 }, { 1, 1, 1, 1 }, { 0x00FF00, 0xFF0000, 0x000000, 0x0000FF } };

Mode menu = { "MENU", { 1, 1, 0, 0 }, { 1, 1, 255, 16 }, { 1, 1, 10, 1 }, { 0x000000, 0x000000, 0x000000, 0x00FF00 } };

Mode newFileMode = { "NEW_FILE", { 0, 1, 0, 0 }, { 0, 16, 5, 0 }, { 0, 8, 0, 0 }, { 0x00FF00, 0xFF00FF, 0xFFFF00, 0x0000FF } };
Mode subpatternMode = { "SUBPATTERN", { 1, 0, 0, 1 }, { 1, 7, maxfilterResolution, 1 }, { 1, 1, maxfilterResolution, 1 }, { 0xFF00FF, 0x00FFFF, 0x000000, 0xFF00FF } };
bool touchyEnabled = false;
Mode touchyMode = { "TCHY", { 0, 1, 0, 0 }, { 48, 127, 32, 32 }, { 24, 100, 0, 0 }, { 0x7F2020, 0x7F5510, 0x207F40, 0x20407F } };
Mode songMode = { "SONGMODE", { 1, 1, 1, 1 }, { 1, 16, 1, 64 }, { 1, 1, 1, 1 }, { 0x000000, 0xFF00FF, 0x000000, 0xFFFF00 } };

Mode *currentMode = &draw;
Mode *oldMode = &draw;
Mode *muteModeReturn = nullptr;
bool muteModeActive = false;
bool muteModeRingsDirty = false;
bool muteModeReturnSingleState = false;
int muteModeLastChannel = -1;
int8_t muteModeArrowDirection = 0;
unsigned long muteModeArrowUntil = 0;
uint8_t muteModeEncoderValue = 0;
int8_t soloRandomArrowDirection = 0;
unsigned long soloRandomArrowUntil = 0;

static void refreshSamplerChannel(uint8_t channel);
static void updateFnState();
static uint8_t fnButtonEvent(int index, uint8_t physicalState);
static bool fnHeld = false;
static bool fnButtonHeld[NUM_ENCODERS] = {};
static bool handleFnEncoder(int index, int &rawValue, bool allowWrite);
static void reverseChannelInMemory(uint8_t channel);
static void applyChannelDirection(uint8_t channel, int8_t targetDir);



bool songModeActive = false;
int currentSongPosition = 0;



unsigned int pendingPage = 0;

struct Sample {

  unsigned int oldID : 10;
  unsigned int fileID : 10;
} __attribute__((packed));


struct Device {

  float bpm;
  unsigned int file;
  unsigned int pack;
  Sample wav[maxFiles];
  char samplePathRel[maxFiles][128];
  float param_settings[maxY][maxFilters];
  float filter_settings[maxY][maxFilters];
  float synth_settings[maxY][11];
  unsigned int mute[maxY];
  unsigned int channelVol[maxY];

  bool globalMutes[maxY];
  bool pageMutes[maxPages][maxY];

  bool sp0Active[maxFiles];

  uint8_t songArrangement[64];
};


struct GlobalVars {
  unsigned int singleMode;
  unsigned int currentChannel;
  unsigned int vol;
  unsigned int velocity;
  unsigned int page;
  unsigned int edit;
  unsigned int folder;
  bool activeCopy;
  unsigned int copyChannel;
  unsigned int x;
  unsigned int y;
  unsigned int seek;
  unsigned int seekEnd;
  unsigned int smplen;
  unsigned int shiftX;
  unsigned int shiftY;
  unsigned int shiftY1;
  unsigned int subpattern;
};

DMAMEM float octave[2];


DMAMEM float detune[13];
DMAMEM float channelOctave[9];







EXTMEM Device SMP = {
  100.0,
  1,
  1,

  { { 0, 1 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 }, { 0, 8 } },
  {{0}},
  {},
  {},
  {},
  {},
  { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }
};

GlobalVars GLOB = {
  false,
  1,
  10,
  10,
  1,
  1,
  0,
  false,
  1,
  2,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};


static inline void cycleImgBrush() {
  extern int drawMode;
  imgBrushChannel = imageModePaintChannel(imgBrushChannel);
  GLOB.currentChannel = imgBrushChannel;
  if (drawMode == 0) {
    Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
  }
  Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
}


static inline int imageModeResolvePaintChannel(uint8_t existingCh) {
  int ch;
  if (existingCh == 0) {
    ch = imgBrushChannel;
    if (ch < 1 || ch > 8 || isChildVoiceDisabled(ch)) {
      ch = imageModePaintChannel(0);
    } else {
      setMuteState(ch, false);
    }
  } else {
    ch = imageModePaintChannel(existingCh);
  }
  imgBrushChannel = ch;
  return ch;
}


FLASHMEM void enforceImageModeConstraints() {
  if (!imageMode) return;

  if (voiceMode) {
    voiceMode = false;
    EEPROM.write(EEPROM_DATA_START + 42, 0);
    updateLastPage();
  }

  if (recChannelClear != 2) {
    recChannelClear = 2;
    EEPROM.write(EEPROM_DATA_START + 6, 2);
    SMP_REC_CHANNEL_CLEAR = false;
  }

  if (GLOB.singleMode || currentMode == &singleMode) {
    GLOB.singleMode = false;
    if (currentMode == &singleMode) {
      switchMode(&draw);
    }
  }
}



static inline int effectivePageCount() {
  int n = (maxX > 0) ? (int)(MAX_STEPS / maxX) : (int)maxPages;
  if (n < 1) n = 1;
  if (n > (int)maxPages) n = (int)maxPages;
  return n;
}

static inline int encoderPageMax() {
  if (childLockEnabled) return 1;
  int maxPg = effectivePageCount();


  if (patternMode == 3) return maxPg;
  if (SMP_PATTERN_MODE && (int)lastPage >= 1 && (int)lastPage < maxPg) return (int)lastPage;
  return maxPg;
}

static inline void clampGridCursor() {
  unsigned int xmax = childLockEnabled ? maxX : MAX_STEPS;
  if (xmax < 1) xmax = 1;
  if (GLOB.x < 1) GLOB.x = 1;
  else if (GLOB.x > xmax) GLOB.x = xmax;
  if (GLOB.y < 1) GLOB.y = 1;
  else if (GLOB.y > maxY) GLOB.y = maxY;
}




FLASHMEM void applyDrawSingleYEncoderLimits() {
  if (currentMode != &draw && currentMode != &singleMode) return;
  int y = (int)GLOB.y;
  if (currentMode->pos[0] >= 1u && currentMode->pos[0] <= maxY) {
    y = (int)currentMode->pos[0];
  }
  y = constrain(y, 1, (int)maxY);
  GLOB.y = (unsigned int)y;
  currentMode->pos[0] = (unsigned int)y;
  Encoder[0].writeMax((int32_t)maxY);
  Encoder[0].writeMin((int32_t)1);
  Encoder[0].writeCounter((int32_t)y);
}

int transpose = 0;
int scale = 0;

#define NUM_PARAMS (sizeof(SMP.param_settings[0]) / sizeof(SMP.param_settings[0][0]))
static_assert(FILTER_WAVEFORM == maxFilters, "Review legacy waveform slot mapping");
float& filterSetting(int channel, int index) {
  return toern_audio::legacyFilterValue(SMP.filter_settings, SMP.synth_settings, channel, index);
}

#define NUM_FILTERS (sizeof(SMP.filter_settings[0]) / sizeof(SMP.filter_settings[0][0]))
#define MAX_CHANNELS maxY

EXTMEM uint32_t loadedSampleRate[MAX_CHANNELS];
EXTMEM uint32_t loadedSampleLen[MAX_CHANNELS];
DMAMEM int8_t channelDirection[maxFiles];
DMAMEM static bool prevMuteState[maxY + 1];
static bool tmpMuteActive = false;

DMAMEM static bool fullMuteSavedGlobal[maxY];
DMAMEM static bool fullMuteSavedPages[maxPages][maxY];
static bool fullMuteActive = false;
static FullMuteView fullMuteView = FullMuteView::AllMuted;

uint16_t drawRFullMuteCustomUnmuteMask = 0x0006;

bool fullMuteIsActive() {
  return fullMuteActive;
}


DMAMEM static bool pageMutes[maxPages][maxY];
DMAMEM static bool globalMutes[maxY];

bool isChildVoiceDisabled(int channel) {
  return childLockEnabled && channel >= 9 && channel <= 14;
}

FLASHMEM bool isChildVoiceMuted(int channel) {
  return childLockEnabled && (channel == 11 || channel == 13 || channel == 14);
}

FLASHMEM void enforceChildModeRestrictions() {
  if (!childLockEnabled) return;

  int localX = ((int)GLOB.x - 1) % (int)maxX;
  if (localX < 0) localX = 0;
  pendingPage = 0;
  editpage = 1;
  GLOB.edit = 1;
  GLOB.page = 1;
  if (beat < 1 || beat > maxX) beat = 1;

  GLOB.x = (unsigned int)localX + 1;
  fullMuteActive = false;
  fullMuteView = FullMuteView::AllMuted;
  tmpMute = false;
  tmpMuteActive = false;
  muteModeActive = false;
  muteModeReturn = nullptr;
  muteModeLastChannel = -1;
  muteModeReturnSingleState = false;
  muteModeArrowDirection = 0;
  muteModeArrowUntil = 0;
  muteModeEncoderValue = 0;

  if (isChildVoiceDisabled((int)GLOB.currentChannel)) {
    GLOB.currentChannel = 1;
    if (currentMode == &singleMode) {
      GLOB.y = 2;
      currentMode->pos[0] = 2;
      Encoder[0].writeCounter((int32_t)2);
    }
  }

  for (int ch = 0; ch < maxY; ch++) {
    bool childMuted = isChildVoiceMuted(ch);
    globalMutes[ch] = childMuted;
    SMP.globalMutes[ch] = childMuted;
    SMP.mute[ch] = childMuted ? 1u : 0u;
    for (int page = 0; page < maxPages; page++) {
      pageMutes[page][ch] = childMuted;
      SMP.pageMutes[page][ch] = childMuted;
    }
  }

  for (int noteValue = 0; noteValue <= 107; noteValue++) {
    stopSound(noteValue, 0);
    stopSound(noteValue, 1);
  }
  stopSynthChannel(13);
  stopSynthChannel(14);
  resetMidiPressedKeyCount11to14();

  if (currentMode == &draw || currentMode == &singleMode) {
    currentMode->pos[1] = 1;
    currentMode->pos[3] = GLOB.x;
    Encoder[1].writeMin((int32_t)1);
    Encoder[1].writeMax((int32_t)1);
    Encoder[1].writeCounter((int32_t)1);
    Encoder[3].writeMax((int32_t)maxX);
    Encoder[3].writeCounter((int32_t)GLOB.x);
  }
}


EXTMEM static size_t fastRecWriteIndex[MAX_CHANNELS];
bool fastRecordActive = false;
uint8_t fastRecordChannel = 0;




void applySpkrHardware() {
  pinMode(30, OUTPUT);
  const bool hwOn = spkrEnabled && !isRecording && !fastRecordActive;
  digitalWrite(30, hwOn ? HIGH : LOW);
}
bool recordingBorderArmed = false;
bool countInActive = false;
int countInBeat = 0;
bool countInComplete = false;
bool countInPending = false;
bool sensCountActive = false;
int sensCountSec = 0;
unsigned long sensCountStartMs = 0;
unsigned long touch3PressStartTime = 0;
bool touch3PressProcessed = false;
unsigned int recordingStartBeat = 0;
unsigned int recordingBeatCount = 0;
bool pendingStopFastRecord = false;
bool pendingStartFastRecord = false;
bool fastRecWaitRelease = false;


uint8_t filterPage[NUM_CHANNELS] = { 0 };
static int16_t lastEncVal[NUM_CHANNELS] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };


uint8_t currentColorScheme = 0;
volatile bool colorsUpdatedViaSerial = false;


constexpr int PARAM_COUNT = 6;

FLASHMEM const float maxParamVal[12] = { 1000.0, 2000.0, 1000.0, 1000.0, 1.0, 1000.0 };

FilterType defaultFilter[maxFiles] = { HCUT };

const char *const activeMidiSetType[6] PROGMEM = { "IN", "OUT", "OUT", "INPT", "SCTL", "RCTL" };


uint8_t buttons[NUM_ENCODERS] = { 0 };
EXTMEM unsigned long buttonPressStartTime[NUM_ENCODERS] = { 0 };
EXTMEM unsigned long pressDuration[NUM_ENCODERS] = { 0 };
const unsigned long longPressDuration[NUM_ENCODERS] = { 200, 200, 200, 200 };

ButtonState buttonState[NUM_ENCODERS] = { IDLE };
bool isPressed[NUM_ENCODERS] = { false };
bool pressed[NUM_ENCODERS] = { false };

i2cEncoderLibV2 Encoder[NUM_ENCODERS] = {

  i2cEncoderLibV2(exttouch ? 0x61 : 0x01),
  i2cEncoderLibV2(exttouch ? 0x20 : 0x41),
  i2cEncoderLibV2(exttouch ? 0x41 : 0x20),
  i2cEncoderLibV2(exttouch ? 0x01 : 0x61),
};

int currentEncoderIndex = 0;




#define NEOSLIDER_COUNT 4
#define NEOSLIDER_NUM_LEDS 4

#define NEOSLIDER_LED_BRIGHTNESS 255

static const uint8_t neoSliderAddrs[NEOSLIDER_COUNT] = { 0x30, 0x31, 0x32, 0x33 };
static const uint8_t neoSliderVoiceCh[NEOSLIDER_COUNT] = { 1, 2, 3, 4 };


static inline bool isVolumeVoiceChannel(int ch) {
  return (ch >= 1 && ch <= 8) || ch == 11 || ch == 13 || ch == 14;
}
#define SS_NEOPIXEL_BASE 0x0E
#define SS_NEOPIXEL_PIN 0x01
#define SS_NEOPIXEL_SPEED 0x02
#define SS_NEOPIXEL_BUF_LENGTH 0x03
#define SS_NEOPIXEL_BUF 0x04
#define SS_NEOPIXEL_SHOW 0x05

#define SS_NEOSLIDER_ADC_CH 18
#define SS_NEOSLIDER_LED_PIN 14



class NeoSliderSeesaw : public Adafruit_seesaw {
public:
  bool writeReg(uint8_t regHigh, uint8_t regLow, uint8_t *buf, uint8_t num) {
    return write(regHigh, regLow, buf, num);
  }
  bool writeReg8(uint8_t regHigh, uint8_t regLow, uint8_t value) {
    return write8(regHigh, regLow, value);
  }


  bool analogReadFast(uint8_t pin, uint16_t &value) {
    uint8_t buf[2];
    if (!read(SEESAW_ADC_BASE, (uint8_t)(SEESAW_ADC_CHANNEL_OFFSET + pin), buf, 2, 15)) {
      return false;
    }
    uint16_t v = ((uint16_t)buf[0] << 8) | buf[1];
    if (v > 1023) return false;
    value = v;
    return true;
  }
};

static NeoSliderSeesaw neoSliderSS[NEOSLIDER_COUNT];
static bool neoSliderPresent[NEOSLIDER_COUNT];
static bool neoSliderAnyPresent = false;
static int neoSliderLastVol[NEOSLIDER_COUNT];
static bool neoSliderMuteLatched[NEOSLIDER_COUNT];
static bool neoSliderInMuteDz[NEOSLIDER_COUNT];
static bool neoSliderInFullDz[NEOSLIDER_COUNT];
static float neoSliderVelScale[NEOSLIDER_COUNT];
static float neoSliderLastVelNorm[NEOSLIDER_COUNT];
static CRGB neoSliderLeds[NEOSLIDER_COUNT][NEOSLIDER_NUM_LEDS];
static elapsedMillis neoSliderActiveUntil[NEOSLIDER_COUNT];
static int neoSliderLedVolShown[NEOSLIDER_COUNT];
static int neoSliderLedVolWanted[NEOSLIDER_COUNT];
static elapsedMillis g_faderMovedAgo = 1000;

bool encoderI2cWritesAllowed() {
  if (!deviceHasFaders || !neoSliderAnyPresent) return true;

  if (currentMode == &set_Wav || currentMode == &menu) return true;
  return g_faderMovedAgo >= 80;
}

static void applyLongCableI2cTiming();

static inline bool isNeoSliderControlledChannel(int ch) {
  return deviceHasFaders && ch >= 1 && ch <= NEOSLIDER_COUNT && neoSliderPresent[ch - 1];
}


static const uint16_t FADER_ADC_MAX = 1023;
static const uint16_t FADER_MUTE_RAW = 51;
static const uint16_t FADER_FULL_RAW = 972;
static const uint16_t FADER_RAW_HYST = 24;

static float faderVelScaleFromSlide(uint16_t slideVal, bool inMute, bool inFull) {
  if (inMute) return 0.0f;
  if (inFull) return 1.0f;
  const float lo = (float)(FADER_MUTE_RAW + 1);
  const float hi = (float)(FADER_FULL_RAW - 1);
  float t = ((float)slideVal - lo) / (hi - lo);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  return t;
}

static int faderVolFromSlide(uint16_t slideVal, bool inMute, bool inFull) {
  if (inMute) return 0;
  if (inFull) return 16;
  const int lo = (int)FADER_MUTE_RAW + 1;
  const int hi = (int)FADER_FULL_RAW - 1;
  int span = hi - lo;
  if (span < 1) span = 1;
  int pos = (int)slideVal - lo;
  if (pos < 0) pos = 0;
  if (pos > span) pos = span;
  int vol = 1 + (pos * 15) / span;
  if (vol < 1) vol = 1;
  if (vol > 15) vol = 15;
  return vol;
}

static void applyVoiceAmpGainFromVol(int ch, int vol);


static const uint8_t sliderCols[4][2] = { { 2, 3 }, { 6, 7 }, { 10, 11 }, { 14, 15 } };


DMAMEM SliderDefEntry sliderDef[NUM_CHANNELS][FILTER_PAGE_CAP][4];

FLASHMEM void initSliderDefTemplates() {
  static const SliderDefEntry sliderDefTemplate[4][4] = {
    { { ARR_FILTER, HCUT, "HCUT", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_FILTER, LOWCUT, "LCUT", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_FILTER, REVERB, "RVRB", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_FILTER, BITCRUSHER, "BITC", 32, DISPLAY_NUMERIC, nullptr, 32 } },
    { { ARR_SYNTH, CUTOFF, "CUT", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_SYNTH, RESONANCE, "RES", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_SYNTH, FILTER, "FLT", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_PARAM, DELAY, "DLAY", 32, DISPLAY_NUMERIC, nullptr, 32 } },
    { { ARR_FILTER, FILTER_WAVEFORM, "WAVE", 16, DISPLAY_ENUM, waveformNames, 4 },
      { ARR_SYNTH, INSTRUMENT, "INST", 12, DISPLAY_ENUM, instTypeNames, 13 },
      { ARR_SYNTH, CENT, "CENT", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_SYNTH, SEMI, "SEMI", 32, DISPLAY_NUMERIC, nullptr, 32 } },
    { { ARR_PARAM, ATTACK, "ATTC", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_PARAM, DECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_PARAM, SUSTAIN, "SUST", 32, DISPLAY_NUMERIC, nullptr, 32 },
      { ARR_PARAM, RELEASE, "RLSE", 32, DISPLAY_NUMERIC, nullptr, 32 } }
  };
  for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
    for (int p = 0; p < FILTER_PAGE_CAP; ++p) {
      for (int s = 0; s < 4; ++s) {
        if (p < 4) sliderDef[ch][p][s] = sliderDefTemplate[p][s];
        else sliderDef[ch][p][s] = SliderDefEntry{ ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0 };
      }
    }
  }

  for (int s = 0; s < 4; ++s) {
    sliderDef[1][3][s].arr = ARR_NONE;
    sliderDef[1][3][s].idx = 0;
  }
}

struct FilterTarget {
  SettingArray arr;
  uint8_t idx;
};
DMAMEM FilterTarget defaultFastFilter[NUM_CHANNELS];

EXTMEM arraysampler _samplers[9];
AudioPlayArrayResmp *voices[9] = { &sound0, &sound1, &sound2, &sound3, &sound4, &sound5, &sound6, &sound7, &sound8 };
static uint8_t sampleTimeStretch[9] = {11,11,11,11,11,11,11,11,11};
void setSampleTimeStretch(uint8_t channel, int value) {
  if (channel < 1 || channel > 8) return;
  sampleTimeStretch[channel] = constrain(value, 0, 22);
  voices[channel]->setTimeStretchAmount(sampleTimeStretch[channel]);
}

AudioEffectEnvelope *envelopes[15] = { &envelope0, &envelope1, &envelope2, &envelope3, &envelope4, &envelope5, &envelope6, &envelope7, &envelope8, nullptr, nullptr, &envelope11, nullptr, &envelope13, &envelope14 };
AudioAmplifier *amps[15] = { nullptr, &amp1, &amp2, &amp3, &amp4, &amp5, &amp6, &amp7, &amp8, nullptr, nullptr, &amp11, nullptr, &amp13, &amp14 };
AudioFilterStateVariable *filters[15] = { nullptr, &filter1, &filter2, &filter3, &filter4, &filter5, &filter6, &filter7, &filter8, nullptr, nullptr, &filter11, nullptr, &filter13, &filter14 };
AudioMixer4 *filtermixers[15] = { nullptr, &filtermixer1, &filtermixer2, &filtermixer3, &filtermixer4, &filtermixer5, &filtermixer6, &filtermixer7, &filtermixer8, nullptr, nullptr, &filtermixer11, nullptr, &filtermixer13, &filtermixer14 };
AudioEffectBitcrusher *bitcrushers[15] = { nullptr, &bitcrusher1, &bitcrusher2, &bitcrusher3, &bitcrusher4, &bitcrusher5, &bitcrusher6, &bitcrusher7, &bitcrusher8, nullptr, nullptr, &bitcrusher11, nullptr, &bitcrusher13, &bitcrusher14 };
AudioEffectFreeverbDMAMEM *freeverbs[15] = { nullptr, &freeverb1, &freeverb2, nullptr, nullptr, &freeverb5, &freeverb6, &freeverb7, &freeverb8, 0, 0, &freeverb11, nullptr, nullptr, nullptr };
AudioMixer4 *freeverbmixers[15] = { nullptr, &freeverbmixer1, &freeverbmixer2, nullptr, nullptr, &freeverbmixer5, &freeverbmixer6, &freeverbmixer7, &freeverbmixer8, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
AudioMixer4 *waveformmixers[15] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &mixer_waveform11, nullptr, &mixer_waveform13, &mixer_waveform14 };






int8_t stereoChannel = 0;

FLASHMEM void applyStereoChannelRouting() {






  float mainL = 0, prevL = 0, busL = 0;
  float mainR = 0, prevR = 0, busR = 0;
  if (stereoChannel == 0) {
    mainL = mainR = 1.0f;
    prevL = prevR = 1.0f;
  } else if (stereoChannel == 1) {
    mainL = 1.0f;
    prevR = 1.0f;
  } else {
    busL = busR = 1.0f;
  }

  mixer_stereoL.gain(0, mainL);
  mixer_stereoL.gain(1, prevL);
  mixer_stereoL.gain(2, busL);
  mixer_stereoR.gain(0, mainR);
  mixer_stereoR.gain(1, prevR);
  mixer_stereoR.gain(2, busR);
  mixer_stereoR.gain(3, 0.0f);
  mixer_stereoL.gain(3, 0.0f);

  const bool inSmp = (currentMode == &set_Wav);
  if (inSmp && !usbSmpPreview) {
    prevL = 0.0f;
    prevR = 0.0f;
  } else if (inSmp && usbSmpPreview && prevL == 0.0f && prevR == 0.0f) {
    prevL = prevR = 1.0f;
  }
  usbMixL.gain(0, mainL);
  usbMixL.gain(1, prevL);
  usbMixL.gain(2, busL);
  usbMixL.gain(3, 0.0f);
  usbMixR.gain(0, mainR);
  usbMixR.gain(1, prevR);
  usbMixR.gain(2, busR);
  usbMixR.gain(3, 0.0f);
}

static inline uint8_t clampMixGain(uint8_t g) {
  return (g > MIX_GAIN_MAX) ? (uint8_t)MIX_GAIN_UNITY : g;
}

static inline float mixGainScale(uint8_t g) {
  return (float)clampMixGain(g) * (1.0f / (float)MIX_GAIN_UNITY);
}



FLASHMEM void applyMixBusGains() {
  const float master = mixGainScale(mixGainMaster);
  const float g14 = MIX_BUS_HEADROOM * mixGainScale(mixGain14) * master;
  const float g58 = MIX_BUS_HEADROOM * mixGainScale(mixGain58) * master;
  mixer1.gain(0, g14);
  mixer1.gain(1, g14);
  mixer1.gain(2, g14);
  mixer1.gain(3, g14);
  mixer2.gain(0, g58);
  mixer2.gain(1, g58);
  mixer2.gain(2, g58);
  mixer2.gain(3, g58);
  mixer_end.gain(0, MIX_END_SAMPLES_GAIN);
  mixer_end.gain(1, MIX_END_SAMPLES_GAIN);
  mixer_end.gain(2, MIX_END_SYNTH_GAIN * mixGainScale(mixGainSynth) * master);
}


void stopSynthChannel(int ch) {
  if (ch < 13 || ch > 14) return;
  toern_audio::StateGuard guard;
  if (envelopes[ch]) envelopes[ch]->noteOff();
  noteOnTriggered[ch] = false;
  persistentNoteOn[ch] = false;
  sequencedMonoSynthActive[ch] = false;
}



static inline void resetMidiPressedKeyCount11to14() {
  for (int c = 11; c <= 14; ++c) {
    pressedKeyCount[c] = 0;
  }
}


static volatile uint32_t arpStepCounter[15] = { 0 };

static inline void applyExternalFastFilter(uint16_t value14) {
  FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
  if (dft.arr == ARR_NONE) return;
  int mapped = constrain((int)mapf(value14, 0.0f, 16383.0f, 0.0f, maxfilterResolution), 0, (int)maxfilterResolution);


  if (dft.arr == ARR_PARAM && dft.idx >= PARAM_COUNT) return;

  setDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx, mapped);

  switch (dft.arr) {
    case ARR_FILTER:
      setFilters(dft.idx, GLOB.currentChannel, false);
      break;
    case ARR_SYNTH:
      if (GLOB.currentChannel == 11) {
        updateSynthVoice(11);
      } else if ((GLOB.currentChannel == 13 || GLOB.currentChannel == 14) &&
                 (dft.idx == CUTOFF || dft.idx == RESONANCE || dft.idx == FILTER)) {
        extern void applyMonoSynthCrf(int ch);
        applyMonoSynthCrf(GLOB.currentChannel);
      }
      break;
    case ARR_PARAM:
      setParams(dft.idx, GLOB.currentChannel);
      break;
    default:
      break;
  }
}

AudioSynthWaveform *synths[15][2];



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


void allOff() {
  for (AudioEffectEnvelope *envelope : envelopes) {
    if (!envelope) continue;
    envelope->noteOff();
  }
}

static uint8_t noteConditionFromStep(int step) {
  if (step < 1 || step > NOTE_CONDITION_STEP_COUNT) return 1;
  return pgm_read_byte(&noteConditionValues[step - 1]);
}

static int noteConditionToStep(uint8_t condition) {
  if (condition == 0) condition = 1;
  for (int step = 1; step <= NOTE_CONDITION_STEP_COUNT; step++) {
    if (pgm_read_byte(&noteConditionValues[step - 1]) == condition) return step;
  }
  return 1;
}

FLASHMEM void setVelocity() {
  if (childLockEnabled) {
    switchMode(GLOB.singleMode ? &singleMode : &draw);
    return;
  }


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


  static int lastProbStep = -1;
  int currentProbStep = currentMode->pos[1];

  if (currentProbStep != lastProbStep) {
    lastProbStep = currentProbStep;


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


  static int lastCondStep = -1;
  int currentCondStep = currentMode->pos[2];

  if (currentCondStep != lastCondStep) {
    lastCondStep = currentCondStep;





    note[GLOB.x][GLOB.y].condition = noteConditionFromStep(currentCondStep);
  }


  {
    int ch = GLOB.currentChannel;
    unsigned int newVol = currentMode->pos[3];
    if (ch >= 0 && ch < 15 && amps[ch] != nullptr && SMP.channelVol[ch] != newVol) {
      SMP.channelVol[ch] = newVol;
      applyVoiceAmpGainFromVol(ch, (int)newVol);
    }
  }

  drawVelocity();
}

void staticButtonPushed(i2cEncoderLibV2 *obj) {
  pressed[currentEncoderIndex] = !fnHeld || currentMode == &touchyMode;
  encoder_button_pushed(obj, currentEncoderIndex);
}

void staticButtonReleased(i2cEncoderLibV2 *obj) {
  encoder_button_released(obj, currentEncoderIndex);
  pressed[currentEncoderIndex] = false;
}

void staticThresholds(i2cEncoderLibV2 *obj) {

}

void encoder_button_pushed(i2cEncoderLibV2 *obj, int encoderIndex) {
  unsigned long currentTime = millis();

  fnButtonHeld[encoderIndex] = fnHeld;
  buttonPressStartTime[encoderIndex] = currentTime;
  isPressed[encoderIndex] = true;
}

void encoder_button_released(i2cEncoderLibV2 *obj, int encoderIndex) {
  buttonState[encoderIndex] = RELEASED;
}

const unsigned long BUTTON_STUCK_TIMEOUT_MS = 10000;
void handle_button_state(i2cEncoderLibV2 *obj, int encoderIndex) {
  unsigned long currentTime = millis();






  const bool muteHold = (muteModeActive && encoderIndex == 1) || (touchyEnabled && fastRecordActive && encoderIndex == 0);




  if (!muteHold && isPressed[encoderIndex] && buttonState[encoderIndex] != IDLE) {
    unsigned long pressDuration;

    if (currentTime < buttonPressStartTime[encoderIndex]) {

      pressDuration = (0xFFFFFFFF - buttonPressStartTime[encoderIndex]) + currentTime;
    } else {
      pressDuration = currentTime - buttonPressStartTime[encoderIndex];
    }

    if (pressDuration > BUTTON_STUCK_TIMEOUT_MS) {

      isPressed[encoderIndex] = false;
      buttonState[encoderIndex] = IDLE;
      buttons[encoderIndex] = 0;
      buttonPressStartTime[encoderIndex] = 0;
    }
  } else if (!muteHold && buttons[encoderIndex] == 2 && buttonState[encoderIndex] == LONG_PRESSED) {

    unsigned long pressDuration;

    if (currentTime < buttonPressStartTime[encoderIndex]) {
      pressDuration = (0xFFFFFFFF - buttonPressStartTime[encoderIndex]) + currentTime;
    } else {
      pressDuration = currentTime - buttonPressStartTime[encoderIndex];
    }

    if (pressDuration > BUTTON_STUCK_TIMEOUT_MS) {

      isPressed[encoderIndex] = false;
      buttonState[encoderIndex] = IDLE;
      buttons[encoderIndex] = 0;
      buttonPressStartTime[encoderIndex] = 0;
    }
  }

  if (buttonState[encoderIndex] == RELEASED) {


    pressDuration[encoderIndex] = currentTime - buttonPressStartTime[encoderIndex];

    if (pressDuration[encoderIndex] <= longPressDuration[encoderIndex]) {
      buttons[encoderIndex] = 1;
    } else {
      buttons[encoderIndex] = 9;
    }
    buttonState[encoderIndex] = IDLE;
    isPressed[encoderIndex] = false;

  } else if (isPressed[encoderIndex]) {
    pressDuration[encoderIndex] = currentTime - buttonPressStartTime[encoderIndex];

    if (buttonState[encoderIndex] == IDLE) {
      if (pressDuration[encoderIndex] >= longPressDuration[encoderIndex]) {
        buttons[encoderIndex] = 2;
        buttonState[encoderIndex] = LONG_PRESSED;
      } else {





        if (buttons[encoderIndex] != 2) buttons[encoderIndex] = 0;
      }
    } else if (buttonState[encoderIndex] == LONG_PRESSED) {

      buttons[encoderIndex] = 2;
    }

  } else {
    if (buttonState[encoderIndex] != IDLE) {
      buttonState[encoderIndex] = IDLE;
      isPressed[encoderIndex] = false;
    }


    if (buttons[encoderIndex] != 1 && buttons[encoderIndex] != 9) {
      buttons[encoderIndex] = 0;
    }
  }
}


bool match_buttons(const uint8_t states[], int b0, int b1, int b2, int b3) {
  return states[0] == b0 && states[1] == b1 && states[2] == b2 && states[3] == b3;
}


bool was_buttons_0000(const uint8_t prevStates[]) {
  return prevStates[0] == 0 && prevStates[1] == 0 && prevStates[2] == 0 && prevStates[3] == 0;
}


bool was_not_buttons_0000(const uint8_t prevStates[]) {
  return !was_buttons_0000(prevStates);
}


uint32_t hashString(const String &str) {
  uint32_t hash = 5381;
  for (size_t i = 0; i < str.length(); i++) {
    hash = ((hash << 5) + hash) + str.charAt(i);
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
    int adjustedMax = encoderPageMax();
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


void showDrawAfterPatternTransfer() {
  extern bool inLookSubmenu;
  extern bool inRecsSubmenu;
  extern bool inMidiSubmenu;
  extern bool inVolSubmenu;
  extern bool inEtcSubmenu;
  inLookSubmenu = false;
  inRecsSubmenu = false;
  inMidiSubmenu = false;
  inVolSubmenu = false;
  inEtcSubmenu = false;
  if (currentMode == &draw) {
    applyDrawSingleYEncoderLimits();
    return;
  }
  bypassModeSwitchDebounce = true;
  switchMode(&draw);
  bypassModeSwitchDebounce = false;
}

FLASHMEM void switchMode(Mode *newMode) {
  if (childLockEnabled && !touchyEnabled && (newMode == &filterMode || newMode == &velocity || newMode == &subpatternMode)) {
    newMode = GLOB.singleMode ? &singleMode : &draw;
  }
  if (touchyEnabled && (newMode == &draw || newMode == &singleMode)) newMode = &touchyMode;
  enforceChildModeRestrictions();


  static unsigned long lastModeSwitchTime = 0;
  const unsigned long MODE_SWITCH_DEBOUNCE_MS = 80;
  unsigned long now = millis();
  if (newMode != currentMode && lastModeSwitchTime != 0 && !bypassModeSwitchDebounce
      && (now - lastModeSwitchTime) < MODE_SWITCH_DEBOUNCE_MS) {
    return;
  }
  if (newMode != currentMode) {
    lastModeSwitchTime = now;
  }

  updateLastPage();

  drawNoSD_hasRun = false;

  unpaintMode = false;
  GLOB.singleMode = false;
  paintMode = false;
  oldMode = currentMode;


  if ((oldMode == &draw || oldMode == &singleMode) && (newMode != &draw && newMode != &singleMode)) {
    paintMode = false;
    unpaintMode = false;
    preventPaintUnpaint = false;
  }

  if (currentMode == &recordMode && newMode != &recordMode) {
    extern AudioMixer4 mixer_end;
    mixer_end.gain(3, 0.0f);
  }


  if (currentMode == &set_Wav || currentMode == &filterMode || currentMode == &recordMode) {

    Encoder[0].begin(
      i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
      | i2cEncoderLibV2::DIRE_LEFT | i2cEncoderLibV2::IPUP_ENABLE
      | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);
    applyLongCableI2cTiming();
  }


  memcpy(oldButtons, buttons, sizeof(buttons));

  if (newMode != currentMode) {

    if (currentMode == &set_Wav && newMode != &set_Wav) {
      stopAllSetWavPreviewAudio();
    }

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



    if (oldMode == &menu && newMode != &menu) {
      FastLEDclear();
      FastLEDshow();
    }


    if ((newMode == &draw || newMode == &singleMode)
        && (oldMode == &menu || oldMode == &loadSaveTrack || oldMode == &loadSavePreset
            || oldMode == &set_SamplePack
            || oldMode == &bpmMode || oldMode == &songMode || oldMode == &newFileMode
            || oldMode == &set_Wav)) {
      extern void noteMenuExit();
      noteMenuExit();
    }
    if (newMode == &menu && oldMode != &menu) {
      extern void prepareMenuEntry();
      prepareMenuEntry();
      extern void menuRequestFullRedraw();
      menuRequestFullRedraw();
    }

    currentMode = newMode;
    if ((oldMode == &set_Wav) != (currentMode == &set_Wav)) applyStereoChannelRouting();
    if (currentMode == &touchyMode) {
      extern void onTouchyModeEnter();
      extern void syncTouchyEncoders();
      onTouchyModeEnter();
      syncTouchyEncoders();
    }
    if (currentMode == &subpatternMode && muteModeActive) {
      muteModeLastChannel = GLOB.currentChannel;
      currentMode->pos[0] = 0;
      muteModeArrowDirection = 0;
      muteModeArrowUntil = 0;
      muteModeEncoderValue = 0;
    }



    if (currentMode == &draw || currentMode == &singleMode) {
      int maxStepsRuntime = childLockEnabled ? (int)maxX : (int)MAX_STEPS;
      GLOB.x = constrain(GLOB.x, 1, maxStepsRuntime);
      currentMode->pos[3] = GLOB.x;

      if (currentMode == &singleMode) {
        currentMode->pos[0] = constrain(GLOB.y, 1u, (unsigned int)maxY);
      }
    }

    for (int i = 0; i < NUM_ENCODERS; i++) {
      Encoder[i].writeRGBCode(currentMode->knobcolor[i]);
      int32_t minVal = currentMode->minValues[i];
      int32_t maxVal = currentMode->maxValues[i];
      int32_t counterVal = currentMode->pos[i];


      if (i == 1 && (currentMode == &draw || currentMode == &singleMode)) {
        if (ctrlMode == 0) {
          if (childLockEnabled) {
            maxVal = 1;
          } else {
            updateLastPage();
            maxVal = encoderPageMax();
          }
          minVal = 1;
          counterVal = constrain(editpage, (int)minVal, (int)maxVal);
        } else {
          minVal = 0;
          maxVal = 16;
          counterVal = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);
        }
      } else if (currentMode == &subpatternMode && muteModeActive && i == 0) {
        maxVal = 1;
        minVal = -1;
        counterVal = 0;
        currentMode->pos[0] = 0;
      } else if (currentMode == &subpatternMode && muteModeActive && i == 2) {

        FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
        int page, slot;
        int val = 0;
        if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
          val = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);
        }
        val = constrain(val, 0, (int)maxfilterResolution);
        minVal = 0;
        maxVal = (int32_t)maxfilterResolution;
        counterVal = val;
        currentMode->pos[2] = (unsigned int)val;
      } else if (currentMode == &subpatternMode && muteModeActive && i == 3) {

        maxVal = 1;
        minVal = -1;
        counterVal = 0;
        currentMode->pos[3] = 0;
      } else if (i == 2 && (currentMode == &draw || currentMode == &singleMode) && (oldMode == &filterMode || oldMode == &subpatternMode)) {

        FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
        int page, slot;
        if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
          counterVal = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);
          currentMode->pos[2] = counterVal;
        }
      } else if (currentMode == &velocity && i == 2) {

        maxVal = NOTE_CONDITION_STEP_COUNT;
        minVal = 1;
        if (note[GLOB.x][GLOB.y].channel != 0) {
          counterVal = noteConditionToStep(note[GLOB.x][GLOB.y].condition);
          currentMode->pos[2] = counterVal;
        } else {
          counterVal = 1;
          currentMode->pos[2] = 1;
        }
      } else if (currentMode == &velocity && i == 3) {

        maxVal = maxY;
        minVal = 0;
        counterVal = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, (int)maxY);
        currentMode->pos[3] = counterVal;
      } else if (i == 0 && (currentMode == &draw || currentMode == &singleMode)) {
        minVal = 1;
        maxVal = (int32_t)maxY;
        counterVal = constrain(currentMode->pos[0] >= 1u ? (int)currentMode->pos[0] : (int)GLOB.y, 1, (int)maxY);
        currentMode->pos[0] = (unsigned int)counterVal;
        GLOB.y = (unsigned int)counterVal;
      } else {
      }

      Encoder[i].writeMax((int32_t)maxVal);
      Encoder[i].writeMin((int32_t)minVal);

      if ((currentMode == &singleMode && oldMode == &draw) || (currentMode == &draw && oldMode == &singleMode)) {


      } else {
        Encoder[i].writeCounter(counterVal);
      }
    }


    if (currentMode == &singleMode) {
      extern int drawMode;
      if (drawMode == 0) {

        Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else {

        Encoder[0].writeRGBCode(0x000000);
      }
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    }

    if (currentMode == &draw || currentMode == &singleMode) {
      refreshCtrlEncoderConfig();
      enforceChildModeRestrictions();
    } else {
      resetCtrlModeState();
    }

    if (currentMode == &recordMode) {
      Encoder[0].begin(
        i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
        | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
        | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);
      applyLongCableI2cTiming();
      int thr = constrain((int)currentMode->pos[0], 0, 100);
      currentMode->pos[0] = thr;
      Encoder[0].writeMin((int32_t)0);
      Encoder[0].writeMax((int32_t)100);
      Encoder[0].writeCounter((int32_t)thr);
    }

    if (currentMode == &set_Wav) {
      Encoder[0].begin(
        i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
        | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
        | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);
      applyLongCableI2cTiming();


      int ch = constrain((int)GLOB.currentChannel, 1, 8);
      sampleBrowserRefreshList(ch);
      sampleBrowserSyncBrowseFromStoredPath(ch);
      SMP.wav[ch].fileID = (unsigned int)currentMode->pos[3];
      SMP.wav[ch].oldID = 0;

      currentMode->pos[1] = 0;
      Encoder[2].writeMin((int32_t)0);
      Encoder[2].writeMax((int32_t)0);
      Encoder[2].writeCounter((int32_t)0);

      int encMax = max(1, (int)g_wavPickCount);
      int fileIdx = (int)currentMode->pos[3];
      if (fileIdx < 1 || fileIdx > encMax) fileIdx = 1;
      currentMode->pos[3] = fileIdx;
      Encoder[3].writeMin((int32_t)1);
      Encoder[3].writeMax((int32_t)encMax);
      Encoder[3].writeCounter((int32_t)fileIdx);

      Encoder[1].writeMin((int32_t)0);
      Encoder[1].writeMax((int32_t)100);
      int seekEnd = (int)GLOB.seekEnd;
      if (seekEnd <= 0) seekEnd = 100;
      seekEnd = constrain(seekEnd, 0, 100);
      currentMode->pos[2] = seekEnd;
      Encoder[1].writeCounter((int32_t)seekEnd);
    }

    if (currentMode == &filterMode) {

      int ch = constrain(GLOB.currentChannel, 0, 15);
      if (ch == 0 || ch == 9 || ch == 10 || ch == 12 || ch == 15) {
        ch = 1;
        GLOB.currentChannel = ch;
      }
      Encoder[0].begin(
        i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
        | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_ENABLE
        | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);
      applyLongCableI2cTiming();
      initSliders(filterPage[ch], ch);


      updateFilterEncoderColors();



    }

    if (currentMode == &bpmMode) {

      Encoder[0].writeRGBCode(0x000000);
      Encoder[1].writeRGBCode(0x000000);




      unsigned int brightnessPos = (ledBrightness >= 3) ? (ledBrightness - 3) : 0;
      brightnessPos = constrain(brightnessPos, 0, 252);
      currentMode->pos[1] = brightnessPos;
      Encoder[1].writeCounter((int32_t)brightnessPos);
      Encoder[1].writeMax((int32_t)252);
      Encoder[1].writeMin((int32_t)0);


      extern int clockMode;
      currentMode->pos[2] = (clockMode == 1) ? 1 : 0;
      Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
    }

    if (currentMode == &loadSaveTrack) {

      unsigned int fileUi = (SMP.file == 0) ? 1u : (SMP.file + 1u);
      if (fileUi > 1000u) fileUi = 1000u;
      currentMode->pos[3] = fileUi;
      Encoder[3].writeMin((int32_t)0);
      Encoder[3].writeMax((int32_t)1000);
      Encoder[3].writeCounter((int32_t)fileUi);


      char OUTPUTf[50];
      if (SMP.file == 0) {
        sprintf(OUTPUTf, "autosaved.txt");
      } else {
        sprintf(OUTPUTf, "%u.txt", SMP.file);
      }
      if (SD.exists(OUTPUTf)) {
        currentMode->pos[2] = SMP_LOAD_SETTINGS ? 1 : 0;
      } else {
        currentMode->pos[2] = 0;
      }
      Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
    }

    if (currentMode == &loadSavePreset) {
      int ch = (int)GLOB.currentChannel;
      uint16_t slot = getVoicePresetSlot(ch);
      currentMode->pos[3] = slot;
      Encoder[3].writeMin((int32_t)1);
      Encoder[3].writeMax((int32_t)99);
      Encoder[3].writeCounter((int32_t)slot);
      currentMode->pos[0] = 1;
      currentMode->pos[1] = 1;
      currentMode->pos[2] = 1;
      Encoder[0].writeMin((int32_t)1);
      Encoder[0].writeMax((int32_t)1);
      Encoder[1].writeMin((int32_t)1);
      Encoder[1].writeMax((int32_t)1);
      Encoder[2].writeMin((int32_t)1);
      Encoder[2].writeMax((int32_t)1);
    }

    if (currentMode == &subpatternMode && muteModeActive) {
      tmpMute = true;
      tmpMuteAll(true);
    }
  } else if (newMode == &draw || newMode == &singleMode) {
    applyDrawSingleYEncoderLimits();
  }

  drawPlayButton();
}

void checkFastRec() {
  if (touchyEnabled) return;
  if (SMP_FAST_REC != 2 && sensCountActive) {
    sensCountActive = false;
    sensCountSec = 0;
  }
  bool channelAllowed = (GLOB.currentChannel <= 8);

  if (!channelAllowed && !fastRecordActive) return;

  bool allowStart = channelAllowed && (GLOB.y > 1);



  auto armRecordingBorder = []() {
    recordingBorderArmed = true;
    extern void presentRecordingBorder();
    presentRecordingBorder();
  };
  auto requestFastRecord = [&]() {
    if (fastRecWaitRelease) return;
    recordingBorderArmed = true;
    pendingStartFastRecord = true;
  };

  if ((currentMode == &draw || currentMode == &singleMode) && (SMP_FAST_REC == 3 || SMP_FAST_REC == 4)) {
    bool pinsConnected = (digitalRead(2) == LOW);
    if (SMP_FAST_REC == 3 && !pinsConnected) fastRecWaitRelease = false;
    if (SMP_FAST_REC == 4 && pinsConnected) fastRecWaitRelease = false;

    if (pinsConnected != lastPinsConnected && millis() - lastChangeTime > debounceDelay) {
      lastChangeTime = millis();
      lastPinsConnected = pinsConnected;

      if (SMP_FAST_REC == 3) {
        if (pinsConnected && !fastRecordActive) {
          if (!allowStart) return;
          requestFastRecord();
          return;
        } else if (!pinsConnected && fastRecordActive) {
          stopFastRecord();
        }
      }

      if (SMP_FAST_REC == 4) {
        if (!pinsConnected && !fastRecordActive) {
          if (!allowStart) return;
          requestFastRecord();
          return;
        } else if (pinsConnected && fastRecordActive) {
          stopFastRecord();
        }
      }
    }
  }

  if (currentMode == &draw || currentMode == &singleMode) {
    bool currentTouchState = readRecTouchHeld();
    touchState[2] = currentTouchState;


    if (recChannelClear == 4 && isNowPlaying && currentTouchState && !lastTouchState[2]) {
      bool validRow = (currentMode == &draw && isPaintableDrawRow(GLOB.y)) || (currentMode == &singleMode && GLOB.y >= 1 && GLOB.y <= 15);
      if (beat >= 1 && beat <= maxlen && GLOB.y >= 1 && GLOB.y <= (int)maxY && GLOB.currentChannel >= 0 && GLOB.currentChannel <= 15 && validRow && !isChildVoiceDisabled((int)GLOB.currentChannel)) {
        note[beat][GLOB.y].channel = GLOB.currentChannel;
        note[beat][GLOB.y].velocity = defaultVelocity;
        note[beat][GLOB.y].probability = 100;
        note[beat][GLOB.y].condition = 1;
        note[beat][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
      return;
    }


    if (isNowPlaying && GLOB.y == 1 && currentTouchState && !lastTouchState[2]) {
      pause(false);
      return;
    }

    if (SMP_FAST_REC == 2) {


      if (!fastRecordActive) {
        if (currentTouchState) {
          if (!sensCountActive) {
            if (!allowStart) return;
            sensCountActive = true;
            sensCountStartMs = millis();
            sensCountSec = 3;
            lastDrawUpdate = 0;
          }
          const unsigned long elapsed = millis() - sensCountStartMs;
          if (elapsed >= 3000UL) {
            sensCountActive = false;
            sensCountSec = 0;
            recordingStartBeat = beat;
            recordingBeatCount = 0;
            requestFastRecord();
          } else {
            const int remaining = 3 - (int)(elapsed / 1000UL);
            if (remaining != sensCountSec) {
              sensCountSec = remaining;
              lastDrawUpdate = 0;
            }
          }
        } else if (sensCountActive) {
          sensCountActive = false;
          sensCountSec = 0;
          lastDrawUpdate = 0;
        }
        if (!currentTouchState) fastRecWaitRelease = false;
      } else if (!currentTouchState && fastRecordActive) {
        stopFastRecord();
        if (!currentTouchState) fastRecWaitRelease = false;
      }
    } else if (SMP_FAST_REC == 1) {
      if (sensCountActive) {
        sensCountActive = false;
        sensCountSec = 0;
      }

      if (recChannelClear == 3) {
        if (!fastRecordActive && !countInActive) {
          if (currentTouchState && !touch3PressProcessed) {

            if (touch3PressStartTime == 0) {
              touch3PressStartTime = millis();
            }

            if (millis() - touch3PressStartTime >= 300) {

              if (!allowStart) {
                touch3PressStartTime = 0;
                return;
              }
              touch3PressProcessed = true;



              if (isNowPlaying) {
                countInPending = true;
                countInActive = false;
                countInBeat = 0;
                countInComplete = false;
                touch3PressStartTime = 0;
                armRecordingBorder();
                return;
              }
              touch3PressStartTime = 0;
            }
          } else if (!currentTouchState && !touch3PressProcessed) {

            touch3PressStartTime = 0;
          }
        }


        if (!currentTouchState && touch3PressProcessed) {
          touch3PressProcessed = false;
          touch3PressStartTime = 0;
        }
        if (!currentTouchState) fastRecWaitRelease = false;
      } else {

        if (currentTouchState && !fastRecordActive) {

          if (allowStart) {
            recordingStartBeat = beat;
            recordingBeatCount = 0;
            requestFastRecord();
          }
        } else if (!currentTouchState && fastRecordActive) {

          stopFastRecord();
        }
        if (!currentTouchState) fastRecWaitRelease = false;
      }
    }
  }
}

unsigned int cursorNoteStep();


FLASHMEM void checkMode(const uint8_t currentButtonStates[NUM_ENCODERS], bool reset) {
  if (currentMode == &touchyMode) {
    extern void handleTouchyButtons(const uint8_t states[NUM_ENCODERS]);
    handleTouchyButtons(currentButtonStates);
    return;
  }
  extern bool consumeTouchyEntryHold(const uint8_t states[NUM_ENCODERS]);
  if (consumeTouchyEntryHold(currentButtonStates)) return;


  uint8_t fnEvents[NUM_ENCODERS];
  bool modifiedEvent = false;
  for (int i = 0; i < NUM_ENCODERS; i++) {
    fnEvents[i] = fnButtonEvent(i, currentButtonStates[i]);
    if (fnEvents[i] == 3 || fnEvents[i] == 4 || fnEvents[i] == 8) modifiedEvent = true;
  }
  if (fnHeld || modifiedEvent) return;




  if (isRecording && currentMode == &recordMode) {
    if (match_buttons(currentButtonStates, 0, 0, 0, 1)) {
      stopRecordingRAM((int)SMP.wav[GLOB.currentChannel].oldID, (int)SMP.wav[GLOB.currentChannel].fileID);
      extern bool disableThresholdFlag;
      disableThresholdFlag = true;
      extern AudioPlaySdWav playSdWav1;
      if (playSdWav1.isPlaying()) playSdWav1.stop();
      extern bool sampleIsLoaded, firstcheck;
      extern CachedSample previewCache;
      previewCache.valid = false;
      sampleIsLoaded = false;
      firstcheck = true;
      extern void invalidateShowWaveBrowserSync();
      invalidateShowWaveBrowserSync();
      switchMode(&set_Wav);
      return;
    }
    if (pressed[2]) {
      stopRecordingRAM((int)SMP.wav[GLOB.currentChannel].oldID, (int)SMP.wav[GLOB.currentChannel].fileID);
      extern bool disableThresholdFlag;
      disableThresholdFlag = true;
      return;
    }
  }

  if (isRecording) return;



  {
    extern int drawMode;
    const bool drawRFullMuteContext =
        !childLockEnabled && drawMode == 1
        && (currentMode == &draw || currentMode == &singleMode) && GLOB.y < 16;

    if (drawRFullMuteContext && fullMuteIsActive()) {
      if (match_buttons(currentButtonStates, 0, 1, 0, 0)) {
        fullMuteToggleView(FullMuteView::Ch12);
        return;
      }
      if (match_buttons(currentButtonStates, 0, 0, 1, 0)) {
        fullMuteToggleView(FullMuteView::Custom);
        return;
      }
      if (match_buttons(currentButtonStates, 0, 0, 0, 1)) {
        fullMuteToggleView(FullMuteView::UnmuteAll);
        return;
      }
      if (match_buttons(currentButtonStates, 1, 0, 0, 0)) {
        if (millis() < suppressDrawRMuteUntilMs) {
          return;
        }
        fullMuteExitRestore();
        return;
      }
      if (match_buttons(currentButtonStates, 2, 0, 0, 0)) {
        if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0) {
          suppressDrawRMuteUntilMs = millis() + 800;
          return;
        }
        if (millis() < suppressDrawRMuteUntilMs) {
          return;
        }
        fullMuteExitRestore();
        return;
      }
    }
  }


  if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && match_buttons(currentButtonStates, 0, 0, 1, 0)) {

    if (!isNowPlaying) {
      play(true);
    } else {
      unsigned long currentTime = millis();
      if (currentTime - playStartTime > 200) {
        pause(false);
      }
    }
  }

  if (!childLockEnabled && GLOB.y != 16 && (currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && match_buttons(currentButtonStates, 0, 2, 0, 0) && was_buttons_0000(oldButtons)) {
    if (!muteModeActive) {
      muteModeActive = true;
      muteModeRingsDirty = true;
      muteModeReturn = currentMode;
      muteModeReturnSingleState = GLOB.singleMode;
      switchMode(&subpatternMode);

      soloRandomRenewPlaylist();
    }
  }



  if (currentMode == &subpatternMode && muteModeActive &&
      (match_buttons(currentButtonStates, 0, 2, 0, 1) ||
       match_buttons(currentButtonStates, 0, 0, 0, 1))) {
    if (GLOB.currentChannel >= 1 && GLOB.currentChannel <= 8) {
      soloRandomLoadLastPreview();
    }
    return;
  }






  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 9, 0, 0) && was_not_buttons_0000(oldButtons)) {
    if (tmpMute) tmpMuteAll(false);
    drawKnobColorDefault();
  }




  if (currentMode == &singleMode && GLOB.y == 16 && match_buttons(currentButtonStates, 0, 1, 0, 0)) {
    preventPaintUnpaint = true;
    GLOB.shiftX = 8;
    GLOB.shiftY = 8;
    GLOB.shiftY1 = 8;

    Encoder[3].writeCounter((int32_t)8);
    Encoder[1].writeCounter((int32_t)8);
    unsigned int patternLength = lastPage * maxX;

    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        original[nx][ny].channel = 0;
        original[nx][ny].velocity = defaultVelocity;
        original[nx][ny].probability = 100;
        original[nx][ny].condition = 1;
        original[nx][ny].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    }


    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (note[nx][ny].channel != GLOB.currentChannel) {
          original[nx][ny] = note[nx][ny];
        }
      }
    }

    switchMode(&noteShift);
    GLOB.singleMode = true;
    return;
  }

  if (currentMode == &menu && match_buttons(currentButtonStates, 1, 0, 0, 0)) {
    extern bool inLookSubmenu;
    extern bool inRecsSubmenu;
    extern bool inMidiSubmenu;
    extern bool inVolSubmenu;
    extern bool inEtcSubmenu;
    extern bool menuEnteredFromSingleMode;


    if (inLookSubmenu || inRecsSubmenu || inMidiSubmenu || inVolSubmenu || inEtcSubmenu) {

      if (inEtcSubmenu) {
        extern int getCurrentMenuMainSetting();
        int etcSetting = getCurrentMenuMainSetting();
        if (etcSetting == 15) {
          switchMenu(15);
          return;
        }
      }
      int parentPage = 0;
      if (inLookSubmenu) parentPage = 5;
      else if (inRecsSubmenu) parentPage = 6;
      else if (inMidiSubmenu) parentPage = 7;
      else if (inVolSubmenu) parentPage = 4;
      else if (inEtcSubmenu) { extern int systemMenuParentPage(); parentPage = systemMenuParentPage(); }
      inLookSubmenu = false;
      inRecsSubmenu = false;
      inMidiSubmenu = false;
      inVolSubmenu = false;
      inEtcSubmenu = false;
      currentMenuPage = parentPage;
      currentMode->pos[3] = parentPage;
      Encoder[3].writeCounter((int32_t)parentPage);
      if (menuEnteredFromSingleMode) {
        extern bool imageMode;
        if (imageMode) {
          switchMode(&draw);
          GLOB.singleMode = false;
        } else {
          switchMode(&singleMode);
          GLOB.singleMode = true;
        }
      } else {
        switchMode(&draw);
        GLOB.singleMode = false;
      }
      extern int drawMode;
      if (drawMode == 0) {
        Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else {
        Encoder[0].writeRGBCode(0x000000);
      }
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      return;
    }


    extern int getCurrentMenuMainSetting();
    int mainSetting = getCurrentMenuMainSetting();


    if (mainSetting == 15) {
      switchMenu(mainSetting);
    } else {

      if (menuEnteredFromSingleMode) {
        extern bool imageMode;
        if (imageMode) {
          switchMode(&draw);
          GLOB.singleMode = false;
        } else {
          switchMode(&singleMode);
          GLOB.singleMode = true;
        }
      } else {
        switchMode(&draw);
        GLOB.singleMode = false;
      }
      extern int drawMode;
      if (drawMode == 0) {
        Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else {
        Encoder[0].writeRGBCode(0x000000);
      }
      Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    }
  }

  if (currentMode == &menu && match_buttons(currentButtonStates, 0, 1, 0, 0)) {
    extern bool inEtcSubmenu;
    extern int getCurrentMenuMainSetting();
    if (inEtcSubmenu && getCurrentMenuMainSetting() == 16) {
      switchMenu(16);
      return;
    }
  }

  if (currentMode == &menu && match_buttons(currentButtonStates, 0, 0, 0, 1)) {

    extern int getCurrentMenuMainSetting();
    int mainSetting = getCurrentMenuMainSetting();





    extern bool inEtcSubmenu;
    if (inEtcSubmenu && mainSetting == 41) {
      switchMenu(41);
      return;
    }


    if (inEtcSubmenu && mainSetting == 39) {
      extern void runInfoLogoAnimationLoop();
      runInfoLogoAnimationLoop();
      return;
    }


    if (inEtcSubmenu && mainSetting == 16) {
      return;
    }


    extern bool inLookSubmenu;
    extern bool inMidiSubmenu;
    extern bool inVolSubmenu;
    extern bool inEtcSubmenu;
    const bool encoder2ValuePage = (mainSetting == 11 || mainSetting == 4 || mainSetting == 12 ||
        (inLookSubmenu && (mainSetting == 9 || mainSetting == 10 || mainSetting == 17 ||
        mainSetting == 18 || mainSetting == 23 || mainSetting == 24 || mainSetting == 25 ||
        mainSetting == 32 || mainSetting == 33 || mainSetting == 38)) ||
        (inMidiSubmenu && (mainSetting == 7 || mainSetting == 8 || mainSetting == 13 || mainSetting == 44 || mainSetting == 45 || mainSetting == 50 || mainSetting == 51 || mainSetting == 58 || mainSetting == 59 || mainSetting == 60)) ||
        (inVolSubmenu && (mainSetting == 43 || mainSetting == 46 || mainSetting == 53)) ||
        (inEtcSubmenu && (mainSetting == 9 || mainSetting == 10 || mainSetting == 18 || mainSetting == 23 || mainSetting == 24 || mainSetting == 32 || mainSetting == 54 || mainSetting == 63 || mainSetting == 56 || mainSetting == 57 || mainSetting == 40 || mainSetting == 41 || mainSetting == 48 || mainSetting == 55)));
    if (mainSetting != 15 && !encoder2ValuePage) {
      switchMenu(mainSetting);
    }
  } else if (currentMode == &newFileMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {

    extern void generateGenreTrack();
    extern void resetNewModeState();
    resetNewModeState();
    generateGenreTrack();
    return;
  } else if (currentMode == &newFileMode && match_buttons(currentButtonStates, 0, 0, 0, 1)) {

    extern void resetNewModeState();
    resetNewModeState();
    loadSaveTrack.pos[3] = 0;
    switchMode(&loadSaveTrack);
    return;
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {
    paintMode = false;
    unpaintMode = false;
    preventPaintUnpaint = false;
    switchMode(&draw);
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {

    if (currentMode->pos[3] != 0 && SMP.file != 0) {
      savePattern(false);
      preventPaintUnpaint = true;
    }
    return;
  } else if ((currentMode == &loadSaveTrack) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {
    if (currentMode->pos[3] == 0) {

      extern void showNewFileScreen();
      showNewFileScreen();
      return;
    }
    loadPattern(false);
    preventPaintUnpaint = true;
    return;
  } else if ((currentMode == &loadSavePreset) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {
    preventPaintUnpaint = false;
    switchMode(GLOB.singleMode ? &singleMode : &draw);
  } else if ((currentMode == &loadSavePreset) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {
    int ch = (int)GLOB.currentChannel;
    uint16_t slot = (uint16_t)constrain((int)currentMode->pos[3], 1, 99);
    extern void showPatternResult(bool success);
    showPatternResult(saveVoicePreset(ch, slot));
    preventPaintUnpaint = false;
    switchMode(GLOB.singleMode ? &singleMode : &draw);
    return;
  } else if ((currentMode == &loadSavePreset) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {
    int ch = (int)GLOB.currentChannel;
    uint16_t slot = (uint16_t)constrain((int)currentMode->pos[3], 1, 99);
    extern void showPatternResult(bool success);
    showPatternResult(loadVoicePreset(ch, slot));
    preventPaintUnpaint = false;
    switchMode(GLOB.singleMode ? &singleMode : &draw);
    return;
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {
    saveSamplePack(SMP.pack);
    return;
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {

    loadSamplePack(SMP.pack, false, false);
    return;
  } else if ((currentMode == &set_SamplePack) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {
    switchMode(&draw);
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 0, 0, 1, 0)) {
    if (!pressed[3] && !isPressed[3] &&
        millis() >= g_setWavIgnoreLoadUntilMs &&
        sampleBrowserIsLoadableSelection(GLOB.currentChannel)) {
      loadWav();
      if (isNowPlaying) {
        extern void requestDeferredSampleLoadAutosave();
        requestDeferredSampleLoadAutosave();
      } else {
        autoSave();
      }
    }
    return;
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {
    if (previewTriggerMode != PREVIEW_MODE_SYNC || !isNowPlaying) {
      if (sampleBrowserIsLoadableSelection(GLOB.currentChannel)) {
        previewSampleNow();
      }
    }
    return;
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 2, 0, 0, 0)) {
    if (previewTriggerMode != PREVIEW_MODE_SYNC || !isNowPlaying) {
      if (sampleBrowserIsLoadableSelection(GLOB.currentChannel)) {
        previewSampleNow();
      }
    }
    return;
  } else if ((currentMode == &set_Wav) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {
    if (!pressed[2] && !isPressed[2]) {
      sampleBrowserLastEncoderPress(GLOB.currentChannel);
    }
    return;
  } else if ((currentMode == &recordMode) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {
    if (isRecording) {
      stopRecordingRAM((int)SMP.wav[GLOB.currentChannel].oldID, (int)SMP.wav[GLOB.currentChannel].fileID);

      extern bool disableThresholdFlag;
      disableThresholdFlag = true;
    }

    extern AudioPlaySdWav playSdWav1;
    if (playSdWav1.isPlaying()) {
      playSdWav1.stop();
    }

    extern bool sampleIsLoaded, firstcheck;
    extern CachedSample previewCache;


    previewCache.valid = false;
    sampleIsLoaded = false;
    firstcheck = true;
    extern void invalidateShowWaveBrowserSync();
    invalidateShowWaveBrowserSync();

    switchMode(&set_Wav);
  } else if (currentMode == &songMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {
    int songPosition = songMode.pos[3];
    SMP.songArrangement[songPosition - 1] = 0;
    return;
  } else if (currentMode == &songMode && (match_buttons(currentButtonStates, 0, 1, 0, 0) || match_buttons(currentButtonStates, 0, 0, 0, 1))) {

    int songPosition = songMode.pos[3];
    int selectedPattern = songMode.pos[1];
    SMP.songArrangement[songPosition - 1] = selectedPattern;
    return;
  } else if (currentMode == &songMode && match_buttons(currentButtonStates, 0, 0, 1, 0)) {
    extern bool songModeActive;
    if (isNowPlaying) {

      pause(false);
      songModeActive = false;
      patternMode = -1;
      saveSingleModeToEEPROM(3, patternMode);
    } else {

      songModeActive = true;
      patternMode = 2;
      saveSingleModeToEEPROM(3, patternMode);
      play(true);
    }
    return;
  }


  if (currentMode == &noteShift && match_buttons(currentButtonStates, 0, 0, 0, 1)) {
    switchMode(&singleMode);
    GLOB.singleMode = true;
    preventPaintUnpaint = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 1)) {
    freshPaint = false;
    preventPaintUnpaint = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 2, 2, 0)) {
    switchMode(&bpmMode);
  }

  if (!childLockEnabled && (currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 2, 0) && was_buttons_0000(oldButtons)) {
    if (GLOB.currentChannel == 0 || GLOB.currentChannel == 9 || GLOB.currentChannel == 10 || GLOB.currentChannel == 12 || GLOB.currentChannel == 15) return;


    if (currentMode == &draw) {
      GLOB.currentChannel = GLOB.y - 1;
    }


    fxType = 0;
    switchMode(&filterMode);
    return;
  }



  if (currentMode == &draw && match_buttons(currentButtonStates, 1, 1, 0, 0)) {

  }


  extern int drawMode;
  if (drawMode == 0 && !childLockEnabled && !imageMode) {

    if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0 && (currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2) && was_buttons_0000(oldButtons)) {
      unsigned int velo = round(mapf(note[GLOB.x][GLOB.y].velocity, 1, 127, 1, maxY));
      GLOB.velocity = velo;


      unsigned int prob = note[GLOB.x][GLOB.y].probability;
      unsigned int probStep;
      if (prob == 0) probStep = 1;
      else if (prob == 25) probStep = 2;
      else if (prob == 50) probStep = 3;
      else if (prob == 75) probStep = 4;
      else probStep = 5;

      unsigned int condStep =
          noteConditionToStep(note[GLOB.x][GLOB.y].condition);

      switchMode(&velocity);
      GLOB.singleMode = true;
      Encoder[0].writeCounter((int32_t)velo);
      Encoder[1].writeCounter((int32_t)probStep);
      Encoder[2].writeCounter((int32_t)condStep);
      Encoder[3].writeCounter((int32_t)constrain((int)SMP.channelVol[GLOB.currentChannel], 0, (int)maxY));
      currentMode->pos[1] = probStep;
      currentMode->pos[2] = condStep;
      currentMode->pos[3] = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, (int)maxY);
    }

    if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0 && (currentMode == &draw) && match_buttons(currentButtonStates, 0, 0, 0, 2) && was_buttons_0000(oldButtons)) {
      unsigned int velo = round(mapf(note[GLOB.x][GLOB.y].velocity, 1, 127, 1, maxY));
      unsigned int chvol = SMP.channelVol[GLOB.currentChannel];
      GLOB.velocity = velo;


      unsigned int prob = note[GLOB.x][GLOB.y].probability;
      unsigned int probStep;
      if (prob == 0) probStep = 1;
      else if (prob == 25) probStep = 2;
      else if (prob == 50) probStep = 3;
      else if (prob == 75) probStep = 4;
      else probStep = 5;

      unsigned int condStep =
          noteConditionToStep(note[GLOB.x][GLOB.y].condition);

      GLOB.singleMode = false;
      switchMode(&velocity);
      Encoder[0].writeCounter((int32_t)velo);
      Encoder[1].writeCounter((int32_t)probStep);
      Encoder[2].writeCounter((int32_t)condStep);
      Encoder[3].writeCounter((int32_t)chvol);
      currentMode->pos[1] = probStep;
      currentMode->pos[2] = condStep;
      currentMode->pos[3] = chvol;
    }
  }


  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 2, 0, 0, 2)) {
    clearPage();
    preventPaintUnpaint = true;
    return;
  }

  if (currentMode == &velocity && match_buttons(currentButtonStates, 0, 0, 0, 9)) {
    if (!GLOB.singleMode) {
      switchMode(&draw);
    } else {
      switchMode(&singleMode);
      GLOB.singleMode = true;
    }
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 0, 1)) {
    toggleDefaultFilterFromSlider(filterPage[GLOB.currentChannel], 3);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 0, 2) && was_buttons_0000(oldButtons)) {

    extern void setAllFilterPagesDefaultValues(int ch);
    setAllFilterPagesDefaultValues((int)GLOB.currentChannel);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 2, 0, 0, 0) && was_buttons_0000(oldButtons)) {
    setCurrentFilterPageDefaultValues((unsigned int)GLOB.currentChannel);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 2, 0)) {
    if (!isNowPlaying) {
      play(true);
    } else {
      unsigned long currentTime = millis();
      if (currentTime - playStartTime > 1000) {
        pause(false);
      }
    }
    return;
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 0, 1, 0)) {
    toggleDefaultFilterFromSlider(filterPage[GLOB.currentChannel], 2);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 0, 1, 0, 0)) {
    toggleDefaultFilterFromSlider(filterPage[GLOB.currentChannel], 1);
  }

  if (currentMode == &filterMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {
    toggleDefaultFilterFromSlider(filterPage[GLOB.currentChannel], 0);
  }



  if (currentMode == &menu && match_buttons(currentButtonStates, 0, 0, 1, 0)) {
    extern bool inMidiSubmenu;
    extern bool inVolSubmenu;
    extern int getCurrentMenuMainSetting();
    extern void togglePulseClockPolarityFromMenu();
    extern void toggleMixGainEnc3Target();
    int mainSetting = getCurrentMenuMainSetting();
    if (inMidiSubmenu && mainSetting == 50) {
      togglePulseClockPolarityFromMenu();
      return;
    }
    if (inVolSubmenu && mainSetting == 53) {
      toggleMixGainEnc3Target();
      return;
    }
    extern bool inEtcSubmenu;
    if (inEtcSubmenu && mainSetting == 55) {
      extern void cycleFireMenuFocus();
      cycleFireMenuFocus();
      return;
    }
  }


  if (currentMode == &menu && match_buttons(currentButtonStates, 0, 0, 2, 0) && was_buttons_0000(oldButtons)) {
    extern bool inMidiSubmenu;
    extern int getCurrentMenuMainSetting();
    if (inMidiSubmenu && getCurrentMenuMainSetting() == 45) {
      if (!isNowPlaying) {
        play(true);
      } else {
        unsigned long currentTime = millis();
        if (currentTime - playStartTime > 1000) {
          pause(false);
        }
      }
      return;
    }
  }

  if (currentMode == &bpmMode && match_buttons(currentButtonStates, 0, 0, 1, 0)) {

    extern void toggleAnalogExtClock();
    extern void resetMidiClockState();
    extern void drawBPMScreen();
    toggleAnalogExtClock();
    if (clockMode == 1) {
      clockMode = 0;
      currentMode->pos[2] = 0;
      Encoder[2].writeCounter((int32_t)0);
      extern void saveSingleModeToEEPROM(int index, int8_t value);
      saveSingleModeToEEPROM(1, clockMode);
      extern bool MIDI_CLOCK_SEND;
      extern bool MIDI_NOTE_SEND;
      extern int midiSendMode;
      MIDI_CLOCK_SEND = false;
      MIDI_NOTE_SEND = (midiSendMode != 0);
    }
    resetMidiClockState();
    drawBPMScreen();
    return;
  }

  if (currentMode == &bpmMode && match_buttons(currentButtonStates, 0, 1, 0, 0)) {
    drawBaseColorMode = !drawBaseColorMode;
  }

  if (currentMode == &bpmMode && match_buttons(currentButtonStates, 1, 0, 0, 0)) {
    switchMode(&draw);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 9, 0, 9) && was_not_buttons_0000(oldButtons)) {
    paintMode = false;
    unpaintMode = false;
    preventPaintUnpaint = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 9)) {
    freshPaint = false;
    paintMode = false;
    unpaintMode = false;
    preventPaintUnpaint = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 9, 0, 0, 0)) {
    unpaintMode = false;
    paintMode = false;
    preventPaintUnpaint = false;
  }

  if ((currentMode == &draw) && match_buttons(currentButtonStates, 0, 0, 2, 2)) {

  }

  if ((currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 2, 2)) {

  }

  if (!childLockEnabled && (currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 1, 0, 0)) {

    if (currentMode == &draw && GLOB.y == 1 && ctrlMode == 1) {
      inputMonitoringState = (inputMonitoringState + 1) % 3;
      return;
    }

    int ch = GLOB.currentChannel;


    bool wasMuted = getMuteStateForUI(ch);


    if (!wasMuted) {
      int v = constrain((int)SMP.channelVol[ch], 0, 16);
      if (v > 0 && ch >= 0 && ch < maxY) {
        lastChannelVolBeforeMute[ch] = (uint8_t)v;
      }
    }

    toggleMute();

    bool isMuted = getMuteStateForUI(ch);


    if (ctrlMode == 1 && wasMuted && !isMuted) {
      int restoreVol = (ch >= 0 && ch < maxY) ? (int)lastChannelVolBeforeMute[ch] : 0;
      restoreVol = constrain(restoreVol, 0, 16);
      if (restoreVol == 0) restoreVol = DEFAULT_CHANNEL_VOLUME;


      SMP.channelVol[ch] = (unsigned int)restoreVol;
      applyVoiceAmpGainFromVol(ch, restoreVol);


      currentMode->pos[1] = restoreVol;
      Encoder[1].writeCounter((int32_t)restoreVol);
      ctrlLastVolume = restoreVol;

      if (GLOB.y != 16) {
        showCtrlVolumeChange(restoreVol);
        CRGB volColor = CRGB(restoreVol * restoreVol, max(0, 20 - restoreVol), 0);
        Encoder[1].writeRGBCode(volColor.r << 16 | volColor.g << 8 | volColor.b);
        ctrlVolumeOverlayActive = true;
      } else {
        Encoder[1].writeRGBCode(currentMode->knobcolor[1]);
        ctrlVolumeOverlayActive = false;
      }
    }
  }


  extern int drawMode;
  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 1) && !preventPaintUnpaint) {
    if (drawMode == 1 || imageMode) {
      const unsigned int step = cursorNoteStep();
      if (note[step][GLOB.y].channel == 0 || imageMode) {

        freshPaint = true;
        paint();
      } else if (drawMode == 1) {

        unpaint();
      }
      preventPaintUnpaint = false;
    }
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 0, 0, 0, 2) && was_buttons_0000(oldButtons)) {

    if (currentMode == &draw || currentMode == &singleMode) {
      if (drawMode == 0) {

        paintMode = true;
        unpaintMode = false;
        preventPaintUnpaint = false;
      } else {

        if (note[cursorNoteStep()][GLOB.y].channel == 0 || imageMode) {

          paintMode = true;
          unpaintMode = false;
        } else {

          paintMode = false;
          unpaintMode = true;
        }
        preventPaintUnpaint = false;
      }
    }
  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 1, 0, 0, 0)) {

    if (GLOB.activeCopy) {
      deleteActiveCopy();
      preventPaintUnpaint = true;
      return;
    }
    extern int drawMode;
    if (!childLockEnabled && drawMode == 1 && GLOB.y < 16) {
      if (millis() < suppressDrawRMuteUntilMs) {
        return;
      }
      if (!fullMuteIsActive()) {
        fullMuteEnter();
      }
      return;
    }

    if (GLOB.singleMode && GLOB.y == 16) {
      clearPage();
      preventPaintUnpaint = true;
      return;
    }

  }

  if ((currentMode == &draw || currentMode == &singleMode) && match_buttons(currentButtonStates, 2, 0, 0, 0)) {
    extern int drawMode;


    if (currentMode == &draw && !GLOB.singleMode && (touchState[3] || readDebouncedTouch(3))) {
      extern void drawRandomsMultiPage();
      drawRandomsMultiPage();
      deleteActiveCopy();
      preventPaintUnpaint = true;
      return;
    }

    if (!childLockEnabled && drawMode == 1 && GLOB.y < 16) {

      if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0) {
        suppressDrawRMuteUntilMs = millis() + 800;
        return;
      }
      if (millis() < suppressDrawRMuteUntilMs) {
        return;
      }
      if (!fullMuteIsActive()) {
        fullMuteEnter();
      }
      return;
    }

    if (GLOB.singleMode && GLOB.y == 16) {
      drawRandoms();
      preventPaintUnpaint = true;
      return;
    }

    if (currentMode == &draw && GLOB.y == 16) {
      clearPage();
      deleteActiveCopy();
      preventPaintUnpaint = true;
      return;
    }

    unpaint();
    unpaintMode = true;
    deleteActiveCopy();
    preventPaintUnpaint = false;
  }

  if (reset) {



    for (int i = 0; i < NUM_ENCODERS; i++) {
      buttons[i] = 0;
      buttonState[i] = IDLE;
      isPressed[i] = false;
    }


    preventPaintUnpaint = false;
  }
}


uint16_t codecHfCut = 0;

FLASHMEM void applySgtl5000CodecOutputPath() {




  const bool audioWasEnabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
  NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
  sgtl5000_1.autoVolumeDisable();
  sgtl5000_1.surroundSoundDisable();
  sgtl5000_1.enhanceBassDisable();

  if (codecHfCut == 0) {
    sgtl5000_1.audioProcessorDisable();
    if (audioWasEnabled) NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
    return;
  }

  uint32_t v = codecHfCut;
  if (v > 256u) v = 256u;
  if (v != 256u) v = (v / 10u) * 10u;

  const float gTreble = -(float)v * (1.0f / 256.0f);

  sgtl5000_1.audioPostProcessorEnable();
  sgtl5000_1.eqSelect(GRAPHIC_EQUALIZER);
  sgtl5000_1.eqBands(0.0f, 0.0f, 0.0f, 0.0f, gTreble);
  if (audioWasEnabled) NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
}




FLASHMEM void applySgtl5000AdcHighPass() {
  sgtl5000_1.adcHighPassFilterDisable();
  recInputHpf.setHighpass(0, 20.0f, 0.707f);
  const float dry = adcHpfEnabled ? 0.0f : 1.0f;
  const float hp = adcHpfEnabled ? 1.0f : 0.0f;
  recInputHpfMix.gain(0, dry);
  recInputHpfMix.gain(1, hp);
  recInputHpfMix.gain(2, 0.0f);
  recInputHpfMix.gain(3, 0.0f);
}

#ifndef TOERN_AUDIO_MEMORY_BLOCKS
#define TOERN_AUDIO_MEMORY_BLOCKS 96
#endif

FLASHMEM void initSoundChip() {


  AudioMemory(TOERN_AUDIO_MEMORY_BLOCKS);
  for (int i = 0; i < VOICE_DELAY_COUNT; i++) {
    voiceDelay[i].begin(voiceDelayStore[i], VOICE_DELAY_SAMPLES);
    voiceDelayFb[i].gain(0, 1.0f);
    voiceDelayFb[i].gain(1, 0.0f);
    voiceDelayFb[i].gain(2, 0.0f);
    voiceDelayFb[i].gain(3, 0.0f);
    voiceDelayMix[i].gain(0, 0.0f);
    voiceDelayMix[i].gain(1, 0.0f);
    voiceDelayMix[i].gain(2, 0.0f);
    voiceDelayMix[i].gain(3, 1.0f);
  }

  sgtl5000_1.enable();





  float vol = GLOB.vol / 10.0f;
  vol = constrain(vol, 0.0f, 1.0f);
  sgtl5000_1.volume(vol);
# 3593 "/Users/jank/git/toern/toern.ino"
  sgtl5000_1.inputSelect(recInput);
  if (recInput == AUDIO_INPUT_MIC) {
    sgtl5000_1.micGain(micGain);
  } else {

    sgtl5000_1.micGain(0);
  }

  applySgtl5000AdcHighPass();

  sgtl5000_1.lineInLevel(lineInLevel);

  applySgtl5000CodecOutputPath();
}

FLASHMEM void initSamples() {
  for (unsigned int vx = 1; vx < SONG_LEN + 1; vx++) {
    for (unsigned int vy = 1; vy < maxY + 1; vy++) {
      note[vx][vy].velocity = defaultVelocity;
    }
  }

  _samplers[0].addVoice(sound0, mixer0, 1, envelope0);

  _samplers[1].addVoice(sound1, mixer1, 0, envelope1);
  _samplers[2].addVoice(sound2, mixer1, 1, envelope2);
  _samplers[3].addVoice(sound3, mixer1, 2, envelope3);
  _samplers[4].addVoice(sound4, mixer1, 3, envelope4);

  _samplers[5].addVoice(sound5, mixer2, 0, envelope5);
  _samplers[6].addVoice(sound6, mixer2, 1, envelope6);
  _samplers[7].addVoice(sound7, mixer2, 2, envelope7);
  _samplers[8].addVoice(sound8, mixer2, 3, envelope8);

  synthmixer11.gain(0, CH11_SYNTHMIX_GAIN);
  synthmixer11.gain(1, 0.0f);
  synthmixer11.gain(3, CH11_SYNTHMIX_GAIN);


  SmixerL4.gain(0, CH11_POLY_SUM_GAIN);
  SmixerL4.gain(1, CH11_POLY_SUM_GAIN);
  SmixerL4.gain(2, CH11_POLY_SUM_GAIN);
  SmixerL4.gain(3, 0.0f);
  SmixerR4.gain(0, CH11_POLY_SUM_GAIN);
  SmixerR4.gain(1, CH11_POLY_SUM_GAIN);
  SmixerR4.gain(2, CH11_POLY_SUM_GAIN);
  SmixerR4.gain(3, 0.0f);




  CHMixer11.gain(0, 0.5f);
  CHMixer11.gain(1, 0.5f);
  CHMixer11.gain(2, 0.0f);
  CHMixer11.gain(3, 0.0f);





  mixer_waveform11.gain(0, 0.0f);
  mixer_waveform11.gain(1, 0.0f);
  mixer_waveform11.gain(2, 0.0f);
  mixer_waveform11.gain(3, 1.0f);

  synthmixer13.gain(0, 0.10);
  synthmixer13.gain(1, 0.10);
  synthmixer13.gain(3, 0.10);

  synthmixer14.gain(0, 0.10);
  synthmixer14.gain(1, 0.10);
  synthmixer14.gain(3, 0.10);





  mixersynth_end.gain(0, GAIN_4);
  mixersynth_end.gain(2, GAIN_4);
  mixersynth_end.gain(3, GAIN_4);

  applyMixBusGains();
  mixer_end.gain(3, 0.0f);


  applyStereoChannelRouting();


  synths[11][0] = &waveform11_1;
  synths[11][1] = &waveform11_2;

  synths[13][0] = &waveform13_1;
  synths[13][1] = &waveform13_2;

  synths[14][0] = &waveform14_1;
  synths[14][1] = &waveform14_2;

  short waveformType[15][2] = { { 0 } };


  waveformType[11][0] = WAVEFORM_PULSE;
  waveformType[11][1] = WAVEFORM_SAWTOOTH;

  waveformType[13][0] = WAVEFORM_PULSE;
  waveformType[13][1] = WAVEFORM_SAWTOOTH;

  waveformType[14][0] = WAVEFORM_SAWTOOTH;
  waveformType[14][1] = WAVEFORM_PULSE;

  float amplitude[15][2] = { { 0.0f } };


  amplitude[11][0] = 0.0f;
  amplitude[11][1] = 0.0f;


  amplitude[13][0] = 0.1125f;
  amplitude[13][1] = 0.1125f;

  amplitude[14][0] = 0.1125f;
  amplitude[14][1] = 0.1125f;



  for (int pairIndex = 13; pairIndex <= 14; pairIndex++) {
    for (int synthIndex = 0; synthIndex < 2; synthIndex++) {
      if (synths[pairIndex][synthIndex] != nullptr) {
        synths[pairIndex][synthIndex]->begin(waveformType[pairIndex][synthIndex]);
        synths[pairIndex][synthIndex]->amplitude(amplitude[pairIndex][synthIndex]);
        synths[pairIndex][synthIndex]->phase(0);
      }
    }
  }


  for (int i = 0; i < 3; i++) {
    Sdc1[i]->amplitude(1.0);
  }


  for (unsigned int i = 1; i < 9; i++) {
    if (filters[i]) {
      filters[i]->octaveControl(6.0);
      filters[i]->resonance(0.7);
      filters[i]->frequency(8000);
    }
  }

  for (unsigned int i = 0; i < 9; i++) {
    if (voices[i]) {
      voices[i]->enableInterpolation(true);
      if (i == 0) ToernWsola::init();
    }
  }

  initSliderDefTemplates();
  for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
    setSliderDefForChannel(ALL_CHANNELS[i]);
    setFilterDefaults(ALL_CHANNELS[i]);
  }
# 3762 "/Users/jank/git/toern/toern.ino"
}

FLASHMEM void checkCrashReport() {


  if (!CrashReport) return;



  AudioNoInterrupts();

  File errorFile = SD.open("ERROR.txt", O_WRONLY | O_CREAT | O_APPEND);
  if (errorFile) {
    static char buf[120];
    uint32_t uptimeMs = millis();
    uint32_t uptimeSec = uptimeMs / 1000;
    uint32_t h = uptimeSec / 3600;
    uint32_t m = (uptimeSec % 3600) / 60;
    uint32_t s = uptimeSec % 60;

    errorFile.println("\n========================================");
    sprintf(buf, "CRASH REPORT - %s %s %s", __FILE__, __DATE__, __TIME__);
    errorFile.println(buf);
    errorFile.println("========================================");
    errorFile.print(CrashReport);
    errorFile.println();
    errorFile.println("--- System Information ---");
    sprintf(buf, "Uptime: %luh, %lumin, %lusec (%lu ms)", h, m, s, uptimeMs);
    errorFile.println(buf);

    char *heapEnd = (char *)sbrk(0);
    char *stackPtr = (char *)__builtin_frame_address(0);
    uint32_t freeRam = ((uintptr_t)stackPtr > (uintptr_t)heapEnd) ? (uint32_t)((uintptr_t)stackPtr - (uintptr_t)heapEnd) : 0;
    sprintf(buf, "Free RAM: %lu bytes", freeRam);
    errorFile.println(buf);

    sprintf(buf, "Audio CPU: %.1f%% (max: %.1f%%)",
            AudioProcessorUsage(), AudioProcessorUsageMax());
    errorFile.println(buf);
    sprintf(buf, "Audio Memory: %d/%d blocks (pool %d)",
            AudioMemoryUsage(), AudioMemoryUsageMax(),
            TOERN_AUDIO_MEMORY_BLOCKS);
    errorFile.println(buf);

    errorFile.println("========================================");
    errorFile.println();
    errorFile.close();
  } else {
    Serial.print(CrashReport);
  }
  AudioInterrupts();
}


FLASHMEM static void neoSliderShowLeds(int idx) {
  if (idx < 0 || idx >= NEOSLIDER_COUNT || !neoSliderPresent[idx]) return;

  uint8_t writeBuf[2 + NEOSLIDER_NUM_LEDS * 3];
  writeBuf[0] = 0;
  writeBuf[1] = 0;
  for (uint8_t i = 0; i < NEOSLIDER_NUM_LEDS; i++) {
    CRGB c = neoSliderLeds[idx][i];
    c.nscale8(NEOSLIDER_LED_BRIGHTNESS);
    const uint8_t base = 2 + i * 3;
    writeBuf[base + 0] = c.g;
    writeBuf[base + 1] = c.r;
    writeBuf[base + 2] = c.b;
  }
  neoSliderSS[idx].writeReg(SS_NEOPIXEL_BASE, SS_NEOPIXEL_BUF, writeBuf, sizeof(writeBuf));
  neoSliderSS[idx].writeReg(SS_NEOPIXEL_BASE, SS_NEOPIXEL_SHOW, nullptr, 0);
  applyLongCableI2cTiming();
}



FLASHMEM static CRGB neoSliderPlasticCompensated(CRGB c) {
  if ((c.r | c.g | c.b) == 0) return c;

  CHSV hsv = rgb2hsv_approximate(c);


  if (hsv.h > 80) return c;



  hsv.h = qadd8(hsv.h, scale8(hsv.h, 100));

  uint8_t desat = scale8(hsv.h, 120);
  hsv.s = qsub8(hsv.s, desat);

  hsv.v = qadd8(hsv.v, scale8(desat, 90));

  CRGB out = hsv;

  out.g = qadd8(out.g, scale8(out.g, 90));
  out.b = qadd8(out.b, scale8(out.b, 70));
  return out;
}

FLASHMEM static void neoSliderUpdateLeds(int idx, int vol) {


  if (idx < 0 || idx >= NEOSLIDER_COUNT) return;
  vol = constrain(vol, 0, 16);
  int lit = (vol <= 0) ? 0 : ((vol * NEOSLIDER_NUM_LEDS + 15) / 16);
  if (lit > NEOSLIDER_NUM_LEDS) lit = NEOSLIDER_NUM_LEDS;

  const int voiceCh = neoSliderVoiceCh[idx];
  extern CRGB col[];
  CRGB on = col[voiceCh];
  if (on.r == 0 && on.g == 0 && on.b == 0) {
    on = CRGB(0, 180, 40);
  }
  on = neoSliderPlasticCompensated(on);

  fill_solid(neoSliderLeds[idx], NEOSLIDER_NUM_LEDS, CRGB::Black);

  for (int i = 0; i < lit; i++) {
    neoSliderLeds[idx][NEOSLIDER_NUM_LEDS - 1 - i] = on;
  }
  neoSliderShowLeds(idx);
  neoSliderLedVolShown[idx] = vol;
}




static void neoSliderFlushPendingLeds() {
  if (digitalRead(INT_PIN) == LOW) return;
  if (playSdWav1.isPlaying()) return;
  if (currentMode == &menu || currentMode == &set_Wav) return;

  static uint8_t next = 0;
  for (int n = 0; n < NEOSLIDER_COUNT; n++) {
    int i = (int)((next + n) % NEOSLIDER_COUNT);
    if (!neoSliderPresent[i]) continue;
    int want = neoSliderLedVolWanted[i];
    if (want < 0) continue;
    want = constrain(want, 0, 16);
    int lit = (want <= 0) ? 0 : ((want * NEOSLIDER_NUM_LEDS + 15) / 16);
    int shownLit = (neoSliderLedVolShown[i] <= 0) ? 0
                                                 : ((neoSliderLedVolShown[i] * NEOSLIDER_NUM_LEDS + 15) / 16);
    if (lit == shownLit && neoSliderLedVolShown[i] >= 0) continue;
    if (digitalRead(INT_PIN) == LOW) return;
    neoSliderUpdateLeds(i, want);
    next = (uint8_t)((i + 1) % NEOSLIDER_COUNT);
    return;
  }
}

FLASHMEM void initNeoSlider() {
  neoSliderAnyPresent = false;
  if (!deviceHasFaders) return;

  for (int i = 0; i < NEOSLIDER_COUNT; i++) {
    neoSliderPresent[i] = false;
    neoSliderLastVol[i] = -1;
    neoSliderMuteLatched[i] = false;
    neoSliderInMuteDz[i] = false;
    neoSliderInFullDz[i] = false;
    neoSliderVelScale[i] = 1.0f;
    neoSliderLastVelNorm[i] = 1.0f;
    neoSliderActiveUntil[i] = 1000;
    neoSliderLedVolShown[i] = -1;
    neoSliderLedVolWanted[i] = -1;
    fill_solid(neoSliderLeds[i], NEOSLIDER_NUM_LEDS, CRGB::Black);

    if (!neoSliderSS[i].begin(neoSliderAddrs[i])) {
      continue;
    }

    uint16_t pid = 0;
    uint8_t year = 0, mon = 0, day = 0;
    neoSliderSS[i].getProdDatecode(&pid, &year, &mon, &day);
    if (pid != 5295) {
      continue;
    }


    neoSliderSS[i].writeReg8(SS_NEOPIXEL_BASE, SS_NEOPIXEL_SPEED, 1);
    const uint8_t numBytes = NEOSLIDER_NUM_LEDS * 3;
    uint8_t lenBuf[2] = { (uint8_t)(numBytes >> 8), (uint8_t)(numBytes & 0xFF) };
    neoSliderSS[i].writeReg(SS_NEOPIXEL_BASE, SS_NEOPIXEL_BUF_LENGTH, lenBuf, 2);
    neoSliderSS[i].writeReg8(SS_NEOPIXEL_BASE, SS_NEOPIXEL_PIN, SS_NEOSLIDER_LED_PIN);
    neoSliderPresent[i] = true;
    neoSliderAnyPresent = true;

  }
}

static void applyVoiceAmpGainFromVol(int ch, int vol) {
  vol = constrain(vol, 0, 16);
  if (vol <= 0) return;
  if (ch >= 0 && ch < 15 && amps[ch] != nullptr) {
    float channelvolume = mapf((float)vol, 0, maxY, 0.0f, 1.0f);
    amps[ch]->gain(channelvolume);
  }
}

static inline uint8_t effectiveNoteVelocity(int vel) {
  if (vel <= 0) return (uint8_t)defaultVelocity;
  if (vel > 127) return 127;
  return (uint8_t)vel;
}

uint8_t scaledNoteVelocity(int ch, int vel) {
  uint8_t v = effectiveNoteVelocity(vel);
  if (!isNeoSliderControlledChannel(ch)) return v;
  float s = neoSliderVelScale[ch - 1];
  if (s <= 0.0f) return 0;
  int out = (int)((float)v * s + 0.5f);
  if (out < 1) out = 1;
  if (out > 127) out = 127;
  return (uint8_t)out;
}

static void applyPlayAmplitude(int ch, float amp) {
  if (ch < 0 || ch > 8 || voices[ch] == nullptr) return;
  if (amp < 0.0f) amp = 0.0f;
  if (amp > 1.0f) amp = 1.0f;
  voices[ch]->setAmplitude(amp);
}

static float playAmpForVoice(int ch) {
  if (isNeoSliderControlledChannel(ch)) {
    return neoSliderLastVelNorm[ch - 1] * neoSliderVelScale[ch - 1];
  }
  return 1.0f;
}

void samplerNoteEvent(int ch, uint8_t pitch, uint8_t velocity, bool on, bool retrigger) {
  if (ch < 0 || ch > 8) return;
  toern_audio::StateGuard guard;
  _samplers[ch].noteEvent(pitch, velocity, on, retrigger);
}

void samplerRemoveSamples(int ch) {
  if (ch < 0 || ch > 8) return;
  toern_audio::StateGuard guard;
  _samplers[ch].removeAllSamples();
}

void samplerAddSample(int ch, uint8_t root, int16_t *data, uint32_t frames, uint16_t channels) {
  if (ch < 0 || ch > 8) return;
  toern_audio::StateGuard guard;
  _samplers[ch].addSample(root, data, frames, channels);
}

void triggerSamplerVoice(int ch, int pitch, int vel, bool retrigger) {
  if (ch < 0 || ch > 8) return;

  if (fastRecordActive && (uint8_t)ch == fastRecordChannel) return;
  const float noteN = (float)effectiveNoteVelocity(vel) / 127.0f;
  const float scale = isNeoSliderControlledChannel(ch) ? neoSliderVelScale[ch - 1] : 1.0f;
  const float amp = noteN * scale;
  if (amp <= 0.0f) return;
  extern void sidechainOnVoice(uint8_t);
  sidechainOnVoice((uint8_t)ch);
  toern_audio::StateGuard guard;
  if (isNeoSliderControlledChannel(ch)) {
    neoSliderLastVelNorm[ch - 1] = noteN;
  }
  uint8_t sv = (uint8_t)constrain((int)(amp * 127.0f + 0.5f), 1, 127);
  const uint8_t midiPitch = (uint8_t)constrain(pitch, 0, 127);
  samplerNoteEvent(ch, midiPitch, sv, true, retrigger);
  if (voices[ch] != nullptr) {
    voices[ch]->snapAmplitude(amp);
  }

  if (ch >= 1 && ch <= 8 && voices[ch] != nullptr) {
    const float fine = detune[ch];
    if (fine != 0.0f) {

      voices[ch]->setPlaybackRate(powf(2.0f, ((float)midiPitch + fine - 36.0f) / 12.0f));
    }
  }
}




FLASHMEM void reapplyAllSampleChannelGains() {
  for (int ch = 1; ch <= 8; ch++) {
    if (amps[ch] == nullptr) continue;
    int vol = constrain((int)SMP.channelVol[ch], 0, 16);
    if (vol <= 0) continue;
    float channelvolume = mapf((float)vol, 0, maxY, 0.0f, 1.0f);
    amps[ch]->gain(channelvolume);
  }
  for (int ch = 11; ch <= 14; ch++) {
    if (ch == 12 || amps[ch] == nullptr) continue;
    int vol = constrain((int)SMP.channelVol[ch], 0, 16);
    if (vol <= 0) continue;
    float channelvolume = mapf((float)vol, 0, maxY, 0.0f, 1.0f);
    amps[ch]->gain(channelvolume);
  }
  forceAllMixerGainsToTarget();
}

FLASHMEM void resetAllChannelVolumesToDefault() {
  for (int ch = 0; ch < (int)maxY; ch++) {
    SMP.channelVol[ch] = (unsigned int)DEFAULT_CHANNEL_VOLUME;
    lastChannelVolBeforeMute[ch] = (uint8_t)DEFAULT_CHANNEL_VOLUME;
  }
  reapplyAllSampleChannelGains();
}


static void processOneNeoSlider(int i, uint16_t *lastRaw) {
  uint16_t raw = 0;
  if (!neoSliderSS[i].analogReadFast(SS_NEOSLIDER_ADC_CH, raw)) {
    return;
  }
  uint16_t slideVal = (uint16_t)(FADER_ADC_MAX - raw);
  if (slideVal > FADER_ADC_MAX) slideVal = FADER_ADC_MAX;

  const bool first = (lastRaw[i] == 0xFFFF);
  int rawDelta = 0;
  if (!first) {
    rawDelta = (int)slideVal - (int)lastRaw[i];
    if (rawDelta < 0) rawDelta = -rawDelta;
  }

  const bool wasMuteDz = neoSliderInMuteDz[i];
  const bool wasFullDz = neoSliderInFullDz[i];
  if (neoSliderInMuteDz[i]) {
    if (slideVal >= (uint16_t)(FADER_MUTE_RAW + FADER_RAW_HYST)) neoSliderInMuteDz[i] = false;
  } else if (slideVal <= FADER_MUTE_RAW) {
    neoSliderInMuteDz[i] = true;
  }
  if (neoSliderInFullDz[i]) {
    if (slideVal <= (uint16_t)(FADER_FULL_RAW - FADER_RAW_HYST)) neoSliderInFullDz[i] = false;
  } else if (slideVal >= FADER_FULL_RAW) {
    neoSliderInFullDz[i] = true;
  }
  const bool inMuteDz = neoSliderInMuteDz[i];
  const bool inFullDz = neoSliderInFullDz[i];
  const bool leftMuteDz = wasMuteDz && !inMuteDz;
  const bool muteDzChanged = (inMuteDz != wasMuteDz);
  const bool fullDzChanged = (inFullDz != wasFullDz);

  int vol = faderVolFromSlide(slideVal, inMuteDz, inFullDz);

  if (neoSliderLastVol[i] >= 0 && vol != neoSliderLastVol[i] && !first) {
    int stepDelta = vol - neoSliderLastVol[i];
    if (stepDelta < 0) stepDelta = -stepDelta;
    if (stepDelta == 1 && rawDelta < (int)FADER_RAW_HYST) {
      vol = neoSliderLastVol[i];
    }
  }

  const bool rawMoved = first || (rawDelta >= (int)FADER_RAW_HYST);
  if (rawMoved) neoSliderActiveUntil[i] = 0;

  lastRaw[i] = slideVal;

  const int ch = neoSliderVoiceCh[i];
  if (ch < 0 || ch >= (int)maxY) return;

  if (inMuteDz) {
    if (!getMuteStateForUI(ch)) {
      setMuteState(ch, true);
      SMP.mute[ch] = 1u;
    }
  } else if (!first && (leftMuteDz || rawMoved) && getMuteStateForUI(ch)) {

    setMuteState(ch, false);
    SMP.mute[ch] = 0u;
  }
  neoSliderMuteLatched[i] = getMuteStateForUI(ch);

  if (rawMoved || muteDzChanged || fullDzChanged) {
    neoSliderVelScale[i] = faderVelScaleFromSlide(slideVal, inMuteDz, inFullDz);
    applyPlayAmplitude(ch, neoSliderLastVelNorm[i] * neoSliderVelScale[i]);
  }

  if (rawMoved) g_faderMovedAgo = 0;
  neoSliderLedVolWanted[i] = vol;
  if (vol == neoSliderLastVol[i]) return;
  neoSliderLastVol[i] = vol;
  g_faderMovedAgo = 0;


  showCtrlVolumeChange(vol);
}

void updateNeoSliderVolume() {
  if (!deviceHasFaders || !neoSliderAnyPresent) return;

  if (currentMode == &menu || currentMode == &set_Wav) return;


  if (digitalRead(INT_PIN) == LOW) return;

  if (playSdWav1.isPlaying()) return;

  bool moving = false;
  for (int n = 0; n < NEOSLIDER_COUNT; n++) {
    if (neoSliderPresent[n] && neoSliderActiveUntil[n] < 250) {
      moving = true;
      break;
    }
  }
  static elapsedMillis neoSliderPoll;
  if (neoSliderPoll < (moving ? 4 : 8)) return;
  neoSliderPoll = 0;

  static uint16_t lastRaw[NEOSLIDER_COUNT] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
  for (int i = 0; i < NEOSLIDER_COUNT; i++) {
    if (!neoSliderPresent[i]) continue;
    if (digitalRead(INT_PIN) == LOW) return;
    processOneNeoSlider(i, lastRaw);
  }
  neoSliderFlushPendingLeds();
}




static void applyLongCableI2cTiming() {
  if (!exttouch) return;
  IMXRT_LPI2C_t *port = &IMXRT_LPI2C1;
  port->MCR = 0;
  port->MCCR0 = LPI2C_MCCR0_CLKHI(55) | LPI2C_MCCR0_CLKLO(59) |
                LPI2C_MCCR0_DATAVD(25) | LPI2C_MCCR0_SETHOLD(40);
  port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(3);
  port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(15) | LPI2C_MCFGR2_FILTSCL(15) |
                 LPI2C_MCFGR2_BUSIDLE(4095);
  port->MCFGR3 = LPI2C_MCFGR3_PINLOW(4095);
  port->MCCR1 = port->MCCR0;
  port->MCFGR0 = 0;
  port->MFCR = LPI2C_MFCR_RXWATER(1) | LPI2C_MFCR_TXWATER(1);
  port->MCR = LPI2C_MCR_MEN;
}

FLASHMEM void initEncoders() {

  AudioNoInterrupts();
  for (int i = 0; i < NUM_ENCODERS; i++) {

    currentEncoderIndex = i;
    Encoder[i].reset();
    uint16_t direction_flag;
    if (i > 0) {
      direction_flag = i2cEncoderLibV2::DIRE_RIGHT;
    } else {
      direction_flag = i2cEncoderLibV2::DIRE_LEFT;
    }
    Encoder[i].begin(
      i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
      | direction_flag | i2cEncoderLibV2::IPUP_ENABLE
      | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);

    Encoder[0].writeDoublePushPeriod(0);
    Encoder[1].writeDoublePushPeriod(0);
    Encoder[2].writeDoublePushPeriod(0);
    Encoder[3].writeDoublePushPeriod(0);

      Encoder[i].writeAntibouncingPeriod(10);


    if (i == 0) {
      Encoder[i].writeMax((int32_t)maxY);
      Encoder[i].writeMin((int32_t)1);
    } else if (i == 1) {

      Encoder[i].writeMax((int32_t)effectivePageCount());
      Encoder[i].writeMin((int32_t)1);
    } else if (i == 3) {
      Encoder[i].writeMax((int32_t)(maxlen - 1));
      Encoder[i].writeMin((int32_t)1);
    } else {
      Encoder[i].writeMax((int32_t)maxX * 4);
      Encoder[i].writeMin((int32_t)1);
    }
    Encoder[i].writeStep((int32_t)1);
    Encoder[i].writeCounter((int32_t)1);


    Encoder[i].onButtonPush = staticButtonPushed;
    Encoder[i].onButtonRelease = staticButtonReleased;

    Encoder[i].onMinMax = staticThresholds;
    Encoder[i].autoconfigInterrupt();

    Encoder[i].writeFadeRGB(0);
    delay(50);
    Encoder[i].updateStatus();
  }
  AudioInterrupts();
}

FLASHMEM void loadColorSchemeAndBrightness() {

  uint8_t savedColorScheme = EEPROM.read(EEPROM_DATA_START + 22);
  if (savedColorScheme > 3) {
    savedColorScheme = 0;
    EEPROM.write(EEPROM_DATA_START + 22, 0);
    markSettingsBackupDirty();
  }

  extern void applyColorScheme(uint8_t scheme);
  extern volatile bool colorsUpdatedViaSerial;
  currentColorScheme = savedColorScheme;


  if (savedColorScheme == 3) {
    bool scheme3Loaded = false;
    if (SD.exists("scheme.txt")) {
      File schemeFile = SD.open("scheme.txt", FILE_READ);
      if (schemeFile && schemeFile.size() >= 90) {
        uint8_t buf[90];
        if (schemeFile.read(buf, 90) == 90) {
          for (int i = 0; i < 15; i++) {
            col[i] = CRGB(buf[i * 3], buf[i * 3 + 1], buf[i * 3 + 2]);
            col_base[i] = CRGB(buf[45 + i * 3], buf[46 + i * 3], buf[47 + i * 3]);
          }
          scheme3Loaded = true;
          colorsUpdatedViaSerial = true;
        }
      }
      if (schemeFile) schemeFile.close();
    }
    if (!scheme3Loaded) applyColorScheme(0);
  } else {
    applyColorScheme(currentColorScheme);
  }


  uint8_t savedBrightness = EEPROM.read(EEPROM_DATA_START + 26);
  if (savedBrightness < 3 || savedBrightness > 255) {
    savedBrightness = 64;
    EEPROM.write(EEPROM_DATA_START + 26, 64);
    markSettingsBackupDirty();
  }
  ledBrightness = savedBrightness;
}

FLASHMEM void loadLedModeEarlyFromEEPROM() {
  uint8_t ledMode = EEPROM.read(EEPROM_DATA_START + 13);
  if (ledMode < 1 || ledMode > 4) {
    ledMode = 1;
    EEPROM.write(EEPROM_DATA_START + 13, 1);
    markSettingsBackupDirty();
  }

  ledModules = (ledMode == 1 || ledMode == 3) ? 1 : 2;
  ledModulesRotated = (ledMode >= 3);
  maxX = MATRIX_WIDTH * ledModules;
}

FLASHMEM void setup() {
  Serial.begin(115200);
  if (exttouch) {
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
  }


  pinMode(36, OUTPUT);
  digitalWrite(36, LOW);
  delay(500);
  digitalWrite(36, HIGH);


  NVIC_SET_PRIORITY(IRQ_SOFTWARE, toern_audio::kAudioPriority);

  NVIC_SET_PRIORITY(IRQ_LPUART5, 32);



  pendingSampleNotes.reserve(64);


  FastLED.addLeds<WS2812SERIAL, DATA_PIN, BRG>(leds, NUM_LEDS);



  extern CRGB stripLeds[];
  extern void initLedStrip();
  FastLED.addLeds<WS2812SERIAL, 24, BRG>(stripLeds, 256);



  loadLedModeEarlyFromEEPROM();





  if (readTouch1Pressed()) {
    extern void FastLEDclear();
    extern void drawText(const char *text, int startX, int startY, CRGB color);
    extern void FastLEDshow();

    const unsigned long CLR_MS = 3000UL;
    const unsigned long FULL_MS = 10000UL;
    const unsigned long startMs = millis();
    bool eepromCleared = false;
    int lastSecShown = -1;
    int lastStage = -1;

    while (readTouch1Pressed()) {
      unsigned long held = millis() - startMs;

      if (!eepromCleared && held >= CLR_MS) {
        eepromCleared = true;
        FastLEDclear();
        drawText("CLR", 3, 3, CRGB(255, 255, 0));
        drawText("RAM", 3, 10, CRGB(255, 255, 0));
        FastLEDshow();
        for (unsigned int i = 0; i < EEPROM.length(); i++) {
          EEPROM.write(i, 0);
        }
        lastSecShown = -1;
        delay(400);
        continue;
      }

      if (eepromCleared && held >= FULL_MS) {
        bootFullResetRequested = true;
        FastLEDclear();
        drawText("FULL", 2, 3, CRGB(0, 255, 0));
        drawText("OK", 5, 10, CRGB(0, 255, 0));
        FastLEDshow();
        delay(1000);
        break;
      }

      if (!eepromCleared) {
        int secLeft = (int)((CLR_MS - held + 999UL) / 1000UL);
        if (secLeft < 1) secLeft = 1;
        if (lastStage != 0 || secLeft != lastSecShown) {
          lastStage = 0;
          lastSecShown = secLeft;
          FastLEDclear();
          drawText("CLR", 3, 3, CRGB(255, 255, 0));
          char buf[4];
          snprintf(buf, sizeof(buf), "%d", secLeft);
          drawText(buf, 6, 10, CRGB(255, 255, 0));
          FastLEDshow();
        }
      } else {
        int secLeft = (int)((FULL_MS - held + 999UL) / 1000UL);
        if (secLeft < 1) secLeft = 1;
        if (lastStage != 1 || secLeft != lastSecShown) {
          lastStage = 1;
          lastSecShown = secLeft;
          FastLEDclear();
          drawText("FULL", 2, 3, CRGB(255, 255, 0));
          char buf[4];
          snprintf(buf, sizeof(buf), "%d", secLeft);
          drawText(buf, 6, 10, CRGB(255, 255, 0));
          FastLEDshow();
        }
      }
      delay(20);
      yield();
    }
  }

  EEPROM.get(0, samplePackID);
  if (samplePackID == 0 || samplePackID > 999) {
    samplePackID = 1;
    EEPROM.put(0, samplePackID);
    markSettingsBackupDirty();
  }


  SMP.pack = samplePackID;

  pinMode(INT_PIN, INPUT_PULLUP);
  pinMode(BATT_ADC_PIN, INPUT);
  analogReadResolution(12);
  pinMode(SWITCH_1, INPUT_PULLDOWN);
  pinMode(SWITCH_2, INPUT_PULLDOWN);
  pinMode(SWITCH_3, INPUT_PULLDOWN);
  pinMode(SWITCH_5, INPUT_PULLDOWN);
  pinMode(SWITCH_4, INPUT_PULLDOWN);


  pinMode(30, INPUT_PULLDOWN);

  extern void initAnalogClockHardware();
  initAnalogClockHardware();
# 4456 "/Users/jank/git/toern/toern.ino"
  GLOB.vol = (int8_t)EEPROM.read(EEPROM_DATA_START + 17);
  if (GLOB.vol <= 0 || GLOB.vol > 100) {
    GLOB.vol = 100;
  } else if (GLOB.vol > 10 && GLOB.vol <= 100) {

  } else {

  }


  micGain = (unsigned int)EEPROM.read(EEPROM_DATA_START + 9);
  if (micGain < 1 || micGain > 63) {
    micGain = 10;
  }


  previewVol = (unsigned int)EEPROM.read(EEPROM_DATA_START + 7);
  if (previewVol < 1 || previewVol > 50) {
    previewVol = 20;
  }


  previewTriggerMode = (int)EEPROM.read(EEPROM_DATA_START + 20);
  if (previewTriggerMode != PREVIEW_MODE_ON && previewTriggerMode != PREVIEW_MODE_PRESS && previewTriggerMode != PREVIEW_MODE_SYNC) {
    previewTriggerMode = PREVIEW_MODE_ON;
  }


  lineInLevel = (unsigned int)EEPROM.read(EEPROM_DATA_START + 16);
  if (lineInLevel > 15) {
    lineInLevel = 8;
  }


  lineOutLevelSetting = (uint8_t)EEPROM.read(EEPROM_DATA_START + 15);
  if (lineOutLevelSetting < LINEOUT_MIN || lineOutLevelSetting > LINEOUT_MAX) {
    lineOutLevelSetting = 30;
  }



  recMode = (int8_t)EEPROM.read(EEPROM_DATA_START + 0);
  if (recMode != 1 && recMode != -1) {
    recMode = 1;
  }
  recInput = (recMode == 1) ? AUDIO_INPUT_MIC : AUDIO_INPUT_LINEIN;


  if (readTouch2Pressed()) {

    FastLEDclear();
    showIcons(ICON_HOURGLASS, CRGB(100, 100, 0));
    drawText(VERSION, 3, 1, CRGB(0, 100, 100));
    FastLEDshow();


    while (!SD.begin(INT_SD)) {
      FastLEDclear();
      drawText("SD?", 3, 8, CRGB(100, 0, 0));
      FastLEDshow();
      delay(1000);
      yield();
      noteUserActivity();
    }

    File resetFile = SD.open("reset.dat", FILE_WRITE);
    if (resetFile) {
      resetFile.println("RESET");
      resetFile.close();
    }

    while (true) {
      delay(500);
      yield();
    }
  }

  runAnimation();





  AudioNoInterrupts();
  drawNoSD();
  {
    extern bool loadSampleManifest();
    loadSampleManifest();
  }
  if (CrashReport) {


    CrashReport.clear();
  }

  loadMenuFromEEPROM();




  loadColorSchemeAndBrightness();




  initSamples();
  loadSp0StateFromEEPROM();

  loadSamplePack(samplePackID, true, true);
  SMP.bpm = 100.0;


  int savedVol = GLOB.vol;
  initGlobalVars();
  GLOB.vol = savedVol;

  initPageMutes();

  playTimer.priority(toern_audio::kTimerPriority);
  fillTimer.priority(toern_audio::kTimerPriority);

  playTimer.begin(playNote, (uint32_t)lround(playNoteInterval));

  if (playNoteInterval >= 4) {
    fillTimer.begin(playFillNote, (uint32_t)lround(playNoteInterval / 4.0));
  }



  autoLoad();

  initSoundChip();

  {
    extern void applyAudioSettingsFromGlobals();
    applyAudioSettingsFromGlobals();
  }
  AudioInterrupts();


  loadSMPSettings();

  initEncoders();
  if (deviceHasFaders) initNeoSlider();
  applyLongCableI2cTiming();

  extern bool g_enterNewFileAfterCorruptAutoload;
  if (g_enterNewFileAfterCorruptAutoload) {
    g_enterNewFileAfterCorruptAutoload = false;
    extern void showNewFileScreen();
    showNewFileScreen();
  }


  for (unsigned int x = 0; x <= maxlen; x++) {
    for (unsigned int y = 0; y <= maxY; y++) {
      if (note[x][y].probability == 0 && note[x][y].channel != 0) {

        note[x][y].probability = 100;
      } else if (note[x][y].channel != 0 && note[x][y].probability == 0) {
        note[x][y].probability = 100;
      }

      if (note[x][y].channel != 0 && note[x][y].condition == 0) {
        note[x][y].condition = 1;
      }
    }
  }



  if (SD.exists("reset.dat") || bootFullResetRequested) {
    if (SD.exists("reset.dat")) {
      SD.remove("reset.dat");
    }
    startNew();
    if (bootFullResetRequested) {

      FastLEDclear();
      drawText("SCAN", 2, 3, CRGB(0, 255, 0));
      FastLEDshow();
      extern bool scanAndWriteManifest();
      extern void sampleBrowserInvalidate();
      scanAndWriteManifest();
      sampleBrowserInvalidate();
      showSaveSuccessAnimation(false);
      bootFullResetRequested = false;
    }
  }

  updateSynthVoice(11);
  switchMode(&draw);
  applyDrawSingleYEncoderLimits();

  Serial8.addMemoryForWrite(midiTxExtra, sizeof(midiTxExtra));
  Serial8.addMemoryForRead(midiRxExtra, sizeof(midiRxExtra));
  Serial8.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleClock(handleMidiClock);
  MIDI.setHandleStart(handleStart);
  resetMidiClockState();


  audioInputAmp.gain(1.0);


  extern void applyAudioSettingsFromGlobals();
  applyAudioSettingsFromGlobals();



  for (int ch = 11; ch <= 14; ch++) {
    pressedKeyCount[ch] = 0;
    persistentNoteOn[ch] = false;
    noteOnTriggered[ch] = false;
  }




  for (int ch : {13, 14}) {
    extern void initFilterTransition(int index);

    initFilterTransition(ch);
    if (filtermixers[ch]) {
      filtermixers[ch]->gain(0, 1.0f);
      filtermixers[ch]->gain(1, 0.0f);
      filtermixers[ch]->gain(2, 0.0f);
      filtermixers[ch]->gain(3, 0.0f);
    }

    setFilters(HCUT, ch, true);
    setFilters(LOWCUT, ch, true);
    setFilters(REVERB, ch, true);
    setFilters(BITCRUSHER, ch, true);
    setFilters(DETUNE, ch, true);
    setFilters(OCTAVE, ch, true);
    setParams(DELAY, ch);
    setParams(ATTACK, ch);
    setParams(HOLD, ch);
    setParams(DECAY, ch);
    setParams(SUSTAIN, ch);
    setParams(RELEASE, ch);
  }
  forceAllMixerGainsToTarget();


  initLedStrip();

  lastUserActivityMs = millis();


}

FLASHMEM void initGlobalVars() {
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



  octave[0] = -1.0f;
  octave[1] = 0.0f;

  for (int i = 0; i < maxFiles; ++i) {
    channelDirection[i] = 1;
  }
}



#define BATT_DEBUG_SERIAL 0


int getBatteryRaw() {
  return analogRead(BATT_ADC_PIN);
}

int getBatteryPercent() {
  const int samples = 8;
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(BATT_ADC_PIN);
    delay(1);
  }
  int adc = (int)(sum / samples);
  int corrected = adc - BATT_RAW_ZERO;
  if (corrected < 0) corrected = 0;
  float vMeasured = (float)corrected * (3.3f / (float)(BATT_RAW_FULL - BATT_RAW_ZERO));
  float vBat = vMeasured / BATT_DIVIDER_RATIO;
  float norm = (vBat - BATT_V_MIN) / (BATT_V_MAX - BATT_V_MIN);
  if (norm < 0.0f) norm = 0.0f;
  if (norm > 1.0f) norm = 1.0f;
  float pct = powf(norm, BATT_PCT_CURVE) * 100.0f;
  int result = (int)(pct + 0.5f);
  return constrain(result, 0, 100);
}


bool isRunningOnBattery() {
  extern volatile uint8_t usb_configuration;
  return usb_configuration == 0;
}


bool eyesScreensaverEnabled() {
  return eyesMode != 0;
}


uint32_t eyesScreensaverIdleMs() {
  switch (eyesMode) {
    case 1: return 60000UL;
    case 2: return 300000UL;
    case 3: return 900000UL;
    default: return 0;
  }
}


bool getBatteryWarningActive() {
  static elapsedMillis batteryCycleTimer;
  static bool batteryWarningActive = false;
  static elapsedMillis batteryWarningTimer;
  static int pendingPct = -1;
  static bool firstSampleDone = false;

  if (!firstSampleDone && batteryCycleTimer >= 55000) {
    int pct = getBatteryPercent();
    pendingPct = (pct >= 1 && pct <= BATT_WARN_PCT) ? pct : -1;
    firstSampleDone = true;
  }

  if (firstSampleDone && batteryCycleTimer >= 60000) {
    batteryCycleTimer = 0;
    firstSampleDone = false;
    int pct = getBatteryPercent();
    if (pendingPct >= 0 && pct == pendingPct && pct >= 1 && pct <= BATT_WARN_PCT) {
      batteryWarningActive = true;
      batteryWarningTimer = 0;
    }
    pendingPct = -1;
  }

  if (batteryWarningActive) {
    if (batteryWarningTimer >= 5000) {
      batteryWarningActive = false;
    } else {
      return true;
    }
  }

  return false;
}


FLASHMEM void checkEncoders() {

  static Mode *lastSeenMode = nullptr;
  bool modeChangedGlobal = (currentMode != lastSeenMode);
  lastSeenMode = currentMode;


  uint32_t posHash = 0;

  static int32_t screensaverPrevEnc[NUM_ENCODERS];
  static bool screensaverEncBaselineDone = false;
  const bool allowEncWrite = encoderI2cWritesAllowed();

  if (modeChangedGlobal && (currentMode == &draw || currentMode == &singleMode)) {
    applyDrawSingleYEncoderLimits();
  }

  updateFnState();
  if (fnHeld) {
    for (int i = 0; i < NUM_ENCODERS; i++) {
      pressed[i] = false;
      if (isPressed[i]) fnButtonHeld[i] = true;
    }
  }

  for (int i = 0; i < NUM_ENCODERS; i++) {
    currentEncoderIndex = i;
    Encoder[i].updateStatus();
    int rawValue = Encoder[i].readCounterInt();
    const bool fnTurn = handleFnEncoder(i, rawValue, allowEncWrite);

    if (!fnTurn && (currentMode == &draw || currentMode == &singleMode)) {
      if (i == 0) {
        int clampedY = rawValue;
        if (clampedY < 1) clampedY = 1;
        if (clampedY > (int)maxY) clampedY = (int)maxY;
        if (clampedY != rawValue && allowEncWrite) {
          Encoder[0].writeMax((int32_t)maxY);
          Encoder[0].writeMin((int32_t)1);
          Encoder[0].writeCounter((int32_t)clampedY);
        }
        rawValue = clampedY;
      } else if (i == 1 && ctrlMode == 0) {
        int pmax = encoderPageMax();
        if (rawValue < 1) rawValue = 1;
        if (rawValue > pmax) rawValue = pmax;
      } else if (i == 3) {
        int xmax = childLockEnabled ? (int)maxX : (int)MAX_STEPS;
        if (rawValue < 1) rawValue = 1;
        if (rawValue > xmax) rawValue = xmax;
      }
    }

    if (screensaverEncBaselineDone && rawValue != screensaverPrevEnc[i]) {
      noteUserActivity();
    }
    screensaverPrevEnc[i] = rawValue;

    if (fnTurn) {

    } else if (currentMode == &subpatternMode && muteModeActive && i == 0) {
      if (rawValue != 0) {
        bool turnedRight = rawValue > 0;
        int8_t targetDir = turnedRight ? -1 : 1;
        muteModeArrowDirection = targetDir;
        muteModeArrowUntil = millis() + 250;
        muteModeEncoderValue = turnedRight ? 1 : 0;
        if (allowEncWrite) Encoder[0].writeCounter((int32_t)0);
        rawValue = 0;

        if (GLOB.y >= 2 && GLOB.y <= 9) {
          uint8_t soloChannel = static_cast<uint8_t>(GLOB.currentChannel);
          if (soloChannel < maxFiles) {
            applyChannelDirection(soloChannel, targetDir);
          }
        }
      }
      currentMode->pos[0] = muteModeEncoderValue;
    } else if (currentMode == &subpatternMode && muteModeActive && i == 3) {
      if (rawValue != 0) {
        int delta = (rawValue > 0) ? 1 : -1;
        if (allowEncWrite) Encoder[3].writeCounter((int32_t)0);
        rawValue = 0;
        if (GLOB.currentChannel >= 1 && GLOB.currentChannel <= 8) {
          soloRandomArrowDirection = (int8_t)delta;
          soloRandomArrowUntil = millis() + 250;
          soloRandomStepAndPreview(delta);
        }
      }
      currentMode->pos[3] = 0;
    } else if (currentMode == &set_Wav) {




      if (i == 1) {
        currentMode->pos[2] = rawValue;
      } else if (i == 2) {
        if (rawValue != 0 && allowEncWrite) Encoder[2].writeCounter((int32_t)0);
        currentMode->pos[1] = 0;
      } else if (i == 3) {
        int ch = constrain((int)GLOB.currentChannel, 1, 8);
        int vmax = sampleBrowserBrowseIndexMax(ch);
        int v = constrain(rawValue, 1, vmax);
        if (v != rawValue && allowEncWrite) Encoder[3].writeCounter((int32_t)v);
        currentMode->pos[3] = v;
      } else {
        currentMode->pos[i] = rawValue;
      }
    } else {
      currentMode->pos[i] = rawValue;
    }




    if (i != 2) {
      posHash = (posHash << 10) | (currentMode->pos[i] & 0x3FF);
    }







    if (buttons[i] == 1 || buttons[i] == 9) {

    } else {
      buttons[i] = 0;
    }

    handle_button_state(&Encoder[i], i);
  }
  screensaverEncBaselineDone = true;

  for (int i = 0; i < NUM_ENCODERS; i++) {
    if (isPressed[i]) noteUserActivity();
  }


  if (currentMode == &subpatternMode) {
    if (muteModeActive) {
      if (buttons[0] == 9) {
        muteModeEncoderValue = 0;
        currentMode->pos[0] = 0;
        if (allowEncWrite) Encoder[0].writeCounter((int32_t)0);
      }
      if (buttons[1] == 9) {
        tmpMuteAll(false);
        tmpMute = false;
        Mode *returnMode = muteModeReturn ? muteModeReturn : &draw;
        bool restoreSingleState = muteModeReturnSingleState;
        muteModeReturn = nullptr;
        muteModeLastChannel = -1;
        muteModeReturnSingleState = false;
        muteModeArrowDirection = 0;
        muteModeArrowUntil = 0;
        muteModeEncoderValue = 0;
        soloRandomArrowDirection = 0;
        soloRandomArrowUntil = 0;
        currentMode->pos[0] = 0;
        if (allowEncWrite) Encoder[0].writeCounter((int32_t)0);

        bypassModeSwitchDebounce = true;
        switchMode(returnMode);
        bypassModeSwitchDebounce = false;
        muteModeActive = false;
        GLOB.singleMode = restoreSingleState;

        stopAllSetWavPreviewAudio();
      }
    } else {

      bypassModeSwitchDebounce = true;
      switchMode(GLOB.singleMode ? &singleMode : &draw);
      bypassModeSwitchDebounce = false;
    }
  }

  if (currentMode == &draw || currentMode == &singleMode) {
    if (posHash != oldPosHash) {
      oldPosHash = posHash;
      static int lastY = -1;
      int maxStepsRuntime = childLockEnabled ? (int)maxX : (int)MAX_STEPS;
      currentMode->pos[3] = constrain(currentMode->pos[3], 1, maxStepsRuntime);
      if (allowEncWrite) Encoder[3].writeMax((int32_t)maxStepsRuntime);
      GLOB.x = currentMode->pos[3];
      clampGridCursor();

      {
        unsigned int yClamped = constrain(currentMode->pos[0], 1u, maxY);
        if (currentMode->pos[0] != yClamped) {
          currentMode->pos[0] = yClamped;
          if (allowEncWrite) {
            Encoder[0].writeMax((int32_t)maxY);
            Encoder[0].writeCounter((int32_t)yClamped);
          }
        }
        GLOB.y = yClamped;
      }


      bool yChanged = (GLOB.y != lastY);
      lastY = GLOB.y;


      if (imageMode) {

        uint8_t noteCh = note[cursorNoteStep()][GLOB.y].channel;
        if (noteCh != 0 && isSingleModeChannelAllowed((int)noteCh)
            && !isChildVoiceDisabled((int)noteCh)) {
          GLOB.currentChannel = noteCh;
        }
      } else if (currentMode == &draw) {
        GLOB.currentChannel = GLOB.y - 1;
      }

      if (currentMode == &draw || imageMode) {

        static int lastOverlayCh = -1;
        if (showChannelNr && (yChanged || GLOB.currentChannel != lastOverlayCh)) {
          lastOverlayCh = (int)GLOB.currentChannel;
          int channelNum = GLOB.currentChannel;
          if (channelNum >= 1 && channelNum <= 8) {
            channelNrOverlayChannel = channelNum;
            channelNrOverlayActive = true;
            channelNrOverlayUntil = millis() + 800;
            pageNrOverlayActive = false;
          }
        }
        FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
        int page, slot;
        if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
          int val = getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx);
          if (allowEncWrite) Encoder[2].writeCounter((int32_t)val);
          currentMode->pos[2] = val;
          lastEncVal[GLOB.currentChannel] = val;
        }
        filterfreshsetted = true;
      }





      extern int drawMode;
      if (drawMode == 0) {

        if (allowEncWrite) Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else {

        if (allowEncWrite) Encoder[0].writeRGBCode(0x000000);
      }
      if (allowEncWrite) Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));




      extern bool songModeActive;
      extern int patternMode;
      if (voiceMode && isNowPlaying && !songModeActive && !childLockEnabled) {

        unsigned int page = GLOB.edit < 1 ? 1 : GLOB.edit;
        unsigned int pageStart = (page - 1) * maxX + 1;
        unsigned int pageEnd = pageStart + maxX - 1;
        if (GLOB.x < pageStart || GLOB.x > pageEnd) {
          int rel = ((int)GLOB.x - 1) % (int)maxX + 1;
          if (rel < 1) rel = 1;
          unsigned int xval = pageStart + (unsigned int)rel - 1;
          if (xval < pageStart) xval = pageStart;
          if (xval > pageEnd) xval = pageEnd;
          GLOB.x = xval;
          currentMode->pos[3] = xval;
          if (allowEncWrite) Encoder[3].writeCounter((int32_t)xval);
        }
      } else if (childLockEnabled) {
        GLOB.edit = 1;
        editpage = 1;
        GLOB.page = 1;
      } else if (!songModeActive &&
                 !(SMP_FLOW_MODE && isNowPlaying && patternMode != 3)) {
        unsigned int newEditPage = getPage(GLOB.x);


        if (patternMode == 3) {

          if (newEditPage != GLOB.edit) {
            GLOB.edit = newEditPage;
            if (isNowPlaying) {
              pendingPage =
                  (newEditPage != GLOB.page) ? newEditPage : 0;
            } else {
              GLOB.page = newEditPage;
              beat = (newEditPage - 1) * maxX + 1;
              pendingPage = 0;
            }

            if (ctrlMode == 0) {
              currentMode->pos[1] = (int)newEditPage;
              if (allowEncWrite) Encoder[1].writeCounter((int32_t)newEditPage);
              editpage = newEditPage;
            }
          }
        } else {

          GLOB.edit = newEditPage;
        }
      }
      if (currentMode == &draw && yChanged && voiceMode && !songModeActive && !childLockEnabled) {
        voiceApplyEditPage((int)GLOB.currentChannel);
      }
    }


    static unsigned long paintModeSetTime = 0;
    static unsigned long unpaintModeSetTime = 0;
    static bool lastPaintMode = false;
    static bool lastUnpaintMode = false;

    static unsigned int lastImgPaintStep = 0;
    static unsigned int lastImgPaintY = 0;
    static int imgPaintBrushCh = 0;
    unsigned long now = millis();

    if (paintMode != lastPaintMode) {
      paintModeSetTime = now;
      lastPaintMode = paintMode;
      lastImgPaintStep = 0;
      lastImgPaintY = 0;
      imgPaintBrushCh = 0;
    } else if (paintMode && (now - paintModeSetTime > 30000)) {

      paintMode = false;
      paintModeSetTime = 0;
    }

    if (unpaintMode != lastUnpaintMode) {
      unpaintModeSetTime = now;
      lastUnpaintMode = unpaintMode;
    } else if (unpaintMode && (now - unpaintModeSetTime > 30000)) {

      unpaintMode = false;
      unpaintModeSetTime = 0;
    }

    if ((GLOB.y >= 1 && GLOB.y <= 15)) {
      const unsigned int paintStep = cursorNoteStep();
      if (paintMode && !preventPaintUnpaint) {

        if (currentMode == &draw && isPaintableDrawRow(GLOB.y)) {
          uint8_t existingCh = note[paintStep][GLOB.y].channel;
          if (imageMode) {
            const bool newCell = (paintStep != lastImgPaintStep || GLOB.y != lastImgPaintY);
            if (imgPaintBrushCh == 0) {

              int ch = (existingCh == 0) ? imageModeResolvePaintChannel(0) : (int)existingCh;
              if (ch >= 1 && ch <= 8 && !isChildVoiceDisabled(ch)) {
                if (existingCh == 0) {
                  note[paintStep][GLOB.y].probability = 100;
                  note[paintStep][GLOB.y].condition = 1;
                  note[paintStep][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
                } else {
                  imgBrushChannel = ch;
                  setMuteState(ch, false);
                }
                note[paintStep][GLOB.y].channel = (uint8_t)ch;
                note[paintStep][GLOB.y].velocity = defaultVelocity;
                setMuteState(ch, false);
                imgPaintBrushCh = ch;
                GLOB.currentChannel = ch;
                lastImgPaintStep = paintStep;
                lastImgPaintY = GLOB.y;
                if (voiceMode) updateLastPage();
              }
            } else if (newCell) {

              int ch = imgPaintBrushCh;
              if (ch >= 1 && ch <= 8 && !isChildVoiceDisabled(ch)) {
                if (existingCh == 0) {
                  note[paintStep][GLOB.y].probability = 100;
                  note[paintStep][GLOB.y].condition = 1;
                  note[paintStep][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
                }
                note[paintStep][GLOB.y].channel = (uint8_t)ch;
                note[paintStep][GLOB.y].velocity = defaultVelocity;
                setMuteState(ch, false);
                GLOB.currentChannel = ch;
                lastImgPaintStep = paintStep;
                lastImgPaintY = GLOB.y;
                if (voiceMode) updateLastPage();
              }
            }
          } else if (!isChildVoiceDisabled((int)GLOB.currentChannel)) {

            if (existingCh == 0) {
              note[paintStep][GLOB.y].probability = 100;
              note[paintStep][GLOB.y].condition = 1;
              note[paintStep][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
            }
            note[paintStep][GLOB.y].channel = GLOB.currentChannel;
            note[paintStep][GLOB.y].velocity = defaultVelocity;
            if (voiceMode) updateLastPage();
          }
        }
      }

      if (paintMode && currentMode == &singleMode && !preventPaintUnpaint) {

        uint8_t existingCh = note[paintStep][GLOB.y].channel;
        if (imageMode) {
          const bool newCell = (paintStep != lastImgPaintStep || GLOB.y != lastImgPaintY);
          if (imgPaintBrushCh == 0) {
            int ch = (existingCh == 0) ? imageModeResolvePaintChannel(0) : (int)existingCh;
            if (ch >= 1 && ch <= 8 && !isChildVoiceDisabled(ch)) {
              if (existingCh == 0) {
                note[paintStep][GLOB.y].probability = 100;
                note[paintStep][GLOB.y].condition = 1;
                note[paintStep][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
              } else {
                imgBrushChannel = ch;
                setMuteState(ch, false);
              }
              note[paintStep][GLOB.y].channel = (uint8_t)ch;
              note[paintStep][GLOB.y].velocity = defaultVelocity;
              setMuteState(ch, false);
              imgPaintBrushCh = ch;
              GLOB.currentChannel = ch;
              lastImgPaintStep = paintStep;
              lastImgPaintY = GLOB.y;
              if (voiceMode) updateLastPage();
            }
          } else if (newCell) {
            int ch = imgPaintBrushCh;
            if (ch >= 1 && ch <= 8 && !isChildVoiceDisabled(ch)) {
              if (existingCh == 0) {
                note[paintStep][GLOB.y].probability = 100;
                note[paintStep][GLOB.y].condition = 1;
                note[paintStep][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
              }
              note[paintStep][GLOB.y].channel = (uint8_t)ch;
              note[paintStep][GLOB.y].velocity = defaultVelocity;
              setMuteState(ch, false);
              GLOB.currentChannel = ch;
              lastImgPaintStep = paintStep;
              lastImgPaintY = GLOB.y;
              if (voiceMode) updateLastPage();
            }
          }
        } else if (!isChildVoiceDisabled((int)GLOB.currentChannel) &&
                   (existingCh == 0 || existingCh == GLOB.currentChannel)) {

          if (existingCh == 0) {
            note[paintStep][GLOB.y].probability = 100;
            note[paintStep][GLOB.y].condition = 1;
            note[paintStep][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
          }
          note[paintStep][GLOB.y].channel = GLOB.currentChannel;
          note[paintStep][GLOB.y].velocity = defaultVelocity;
          if (voiceMode) updateLastPage();
        }
      }


      if (unpaintMode && (currentMode == &draw || currentMode == &singleMode) && !preventPaintUnpaint) {
        if (GLOB.singleMode) {
          if (note[paintStep][GLOB.y].channel == GLOB.currentChannel) {
            note[paintStep][GLOB.y].channel = 0;
            note[paintStep][GLOB.y].velocity = defaultVelocity;
            note[paintStep][GLOB.y].probability = 100;
            note[paintStep][GLOB.y].condition = 1;
            note[paintStep][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
            updateLastPage();

            if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
              Encoder[1].writeMax((int32_t)encoderPageMax());
            }
          }
        } else {
          note[paintStep][GLOB.y].channel = 0;
          note[paintStep][GLOB.y].velocity = defaultVelocity;
          note[paintStep][GLOB.y].probability = 100;
          note[paintStep][GLOB.y].condition = 1;
          note[paintStep][GLOB.y].midiPitch = NOTE_MIDI_PITCH_NONE;
          updateLastPage();

          if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode) && (currentMode == &draw || currentMode == &singleMode)) {
            Encoder[1].writeMax((int32_t)encoderPageMax());
          }
        }
      }
    }


    extern bool songModeActive;
    extern int patternMode;
    if (childLockEnabled && ctrlMode == 0) {
      if (editpage != 1 || currentMode->pos[1] != 1) {
        editpage = 1;
        GLOB.edit = 1;
        GLOB.page = 1;
        pendingPage = 0;
        currentMode->pos[1] = 1;
        Encoder[1].writeCounter((int32_t)1);
      }
    } else if (ctrlMode == 0 && currentMode->pos[1] != (unsigned int)editpage && !songModeActive) {
      updateLastPage();
      int pmax = encoderPageMax();
      int oldEdit = editpage;
      int requested = constrain((int)currentMode->pos[1], 1, pmax);
      if (voiceMode && (currentMode == &draw || currentMode == &singleMode)) {
        int ch = (int)GLOB.currentChannel;
        int last = voiceLastFilledPage(ch);
        if (last >= 1) {

          requested = constrain(requested, 1, last);
          voiceLimitPageEncoder(ch);
        } else {
          Encoder[1].writeMin((int32_t)1);
          Encoder[1].writeMax((int32_t)pmax);
        }
      }
      currentMode->pos[1] = (unsigned int)requested;
      Encoder[1].writeCounter((int32_t)requested);
      if (requested == oldEdit) {
        editpage = requested;
      } else {
        editpage = requested;

        int rel = ((int)GLOB.x - 1) % (int)maxX + 1;
        if (rel < 1) rel = 1;
        if (rel > (int)maxX) rel = (int)maxX;
        int xval = rel + (editpage - 1) * (int)maxX;
        xval = constrain(xval, 1, (int)MAX_STEPS);


        if (showChannelNr && (currentMode == &draw || currentMode == &singleMode)) {
          pageNrOverlayPage = editpage;
          pageNrOverlayActive = true;
          pageNrOverlayUntil = millis() + 800;
          channelNrOverlayActive = false;
        }

        if (patternMode == 3) {
          GLOB.edit = (unsigned int)editpage;
          extern bool isNowPlaying;
          if (isNowPlaying) {
            pendingPage =
                (GLOB.edit != GLOB.page) ? GLOB.edit : 0;
          } else {
            GLOB.page = (unsigned int)editpage;
            beat = (editpage - 1) * maxX + 1;
            pendingPage = 0;
          }
          Encoder[3].writeCounter((int32_t)xval);
          GLOB.x = (unsigned int)xval;
          currentMode->pos[3] = (unsigned int)xval;
        } else {
          Encoder[3].writeCounter((int32_t)xval);
          GLOB.x = (unsigned int)xval;
          currentMode->pos[3] = (unsigned int)xval;

          if (SMP_PATTERN_MODE) {
            unsigned int muteFrom = (oldEdit < 1) ? 1u : (unsigned int)oldEdit;
            if (muteFrom > maxPages) muteFrom = maxPages;
            for (int ch = 0; ch < maxY; ch++) {
              pageMutes[muteFrom - 1][ch] = getMuteState(ch);
            }
            GLOB.edit = (unsigned int)editpage;
            patternChangeTime = millis() + 2000;
            patternChangeActive = true;
          } else {
            GLOB.edit = (unsigned int)editpage;
          }
        }
        if (voiceMode && !songModeActive) {
          int ch = (int)GLOB.currentChannel;
          if (ch >= 0 && ch < 16 && editpage >= 1 && editpage <= 255) {
            voiceEditPage[ch] = (uint8_t)editpage;
            voiceCuePage(ch, editpage);
          }
        }
        clampGridCursor();
      }
    }

    if (ctrlMode == 1) {
      static int lastY = -1;

      bool yChanged = (GLOB.y != lastY);

      lastY = GLOB.y;


      if (currentMode == &draw && GLOB.y == 1) {

        if (inputMonitoringState == 0) {


          static int lastDisabledPos = -1;
          extern int recMode;
          int currentGain = (recMode == 1) ? (int)micGain : (int)lineInLevel;
          if (lastDisabledPos != currentGain || currentMode->pos[1] != currentGain) {
            currentMode->pos[1] = currentGain;
            Encoder[1].writeCounter((int32_t)currentGain);
            lastDisabledPos = currentGain;
          }

          Encoder[1].writeRGBCode(0x000000);

        } else {

          ctrlLastChannel = -1;
          extern int recMode;
          static int lastInputGain = -1;
          static bool inputGainFirstEnter = true;


          if (yChanged || modeChangedGlobal) {
            inputGainFirstEnter = true;
            inputGainOverlayActive = false;
          }


          int targetMax = (recMode == 1) ? 63 : 15;
          if (currentMode->maxValues[1] != targetMax) {
            Encoder[1].writeMax((int32_t)targetMax);
            Encoder[1].writeMin((int32_t)0);
            currentMode->maxValues[1] = targetMax;
            currentMode->minValues[1] = 0;



            inputGainFirstEnter = true;
          }

          if (inputGainFirstEnter) {

            if (recMode == 1) {

              Encoder[1].writeCounter((int32_t)micGain);

              currentMode->pos[1] = micGain;
            } else {

              Encoder[1].writeCounter((int32_t)lineInLevel);

              currentMode->pos[1] = lineInLevel;
            }
            lastInputGain = currentMode->pos[1];
            inputGainFirstEnter = false;


            CRGB gainColor = (recMode == 1) ? CRGB(55, 0, 0) : CRGB(0, 0, 55);
            Encoder[1].writeRGBCode(gainColor.r << 16 | gainColor.g << 8 | gainColor.b);


            showInputGainOverlay(currentMode->pos[1], targetMax);
          }


          if (currentMode->pos[1] != lastInputGain) {
            int newGain = constrain((int)currentMode->pos[1], 0, targetMax);

            if (recMode == 1) {

              micGain = (unsigned int)newGain;
              sgtl5000_1.micGain(micGain);
              mixer_end.gain(3, recInputMonitorGain());
            } else {

              lineInLevel = (unsigned int)newGain;
              sgtl5000_1.lineInLevel(lineInLevel);
              mixer_end.gain(3, recInputMonitorGain());
            }

            lastInputGain = newGain;
            Encoder[1].writeCounter((int32_t)newGain);


            CRGB gainColor = (recMode == 1) ? CRGB(55, 0, 0) : CRGB(0, 0, 55);
            Encoder[1].writeRGBCode(gainColor.r << 16 | gainColor.g << 8 | gainColor.b);


            showInputGainOverlay(newGain, targetMax);
          }
        }
      } else if (GLOB.y == 16) {


        int channelVol = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);
        if (currentMode->pos[1] != channelVol || ctrlLastChannel != -1) {
          currentMode->pos[1] = channelVol;
          Encoder[1].writeCounter((int32_t)channelVol);
          Encoder[1].writeRGBCode(currentMode->knobcolor[1]);
        }
        ctrlLastChannel = -1;
        ctrlLastVolume = channelVol;
      } else {
        bool channelJustChanged = (ctrlLastChannel != (int)GLOB.currentChannel);
        if (channelJustChanged) {
          ctrlLastChannel = GLOB.currentChannel;
          int channelVol = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);
          currentMode->pos[1] = channelVol;
          Encoder[1].writeCounter((int32_t)channelVol);
          ctrlLastVolume = channelVol;
        }

        int requestedVol = constrain((int)currentMode->pos[1], 0, 16);
        int actualVol = constrain((int)SMP.channelVol[GLOB.currentChannel], 0, 16);

        bool protectedChannel = !isVolumeVoiceChannel((int)GLOB.currentChannel);
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
            int ch = GLOB.currentChannel;
            if (ch >= 0 && ch < 15 && amps[ch] != nullptr) {

              if (requestedVol > 0 && ch < (int)maxY) {
                lastChannelVolBeforeMute[ch] = (uint8_t)requestedVol;
                SMP.channelVol[ch] = requestedVol;
                applyVoiceAmpGainFromVol(ch, requestedVol);
                applyPlayAmplitude(ch, playAmpForVoice(ch));
              } else {

                applyPlayAmplitude(ch, 0.0f);
              }
            }
            showCtrlVolumeChange(requestedVol);

            reapplyAllSampleChannelGains();
          } else {
            showCtrlVolumeChange(actualVol);
          }
        }




        if (!protectedChannel && requestedVol >= 16 && Encoder[1].readStatus(i2cEncoderLibV2::RMAX)) {
          showCtrlVolumeChange(16);
        }



        if (volumeChanged || channelJustChanged) {
          Encoder[1].writeRGBCode(volColor.r << 16 | volColor.g << 8 | volColor.b);
        }

        if (!childLockEnabled && volumeChanged && GLOB.currentChannel >= 0 && GLOB.currentChannel < maxY) {
          int effectiveVol = protectedChannel ? actualVol : requestedVol;
          bool shouldMute = (effectiveVol == 0);
          setMuteState(GLOB.currentChannel, shouldMute);
          SMP.mute[GLOB.currentChannel] = shouldMute;
        }
      }
    }

    filtercheck();
  }



  if (currentMode == &draw || (muteModeActive && currentMode == &subpatternMode)) {
    FilterTarget dft = defaultFastFilter[GLOB.currentChannel];
    int page, slot;
    if (findSliderDefPageSlot(GLOB.currentChannel, dft.arr, dft.idx, page, slot)) {
      int encVal = currentMode->pos[2];
      if (encVal != lastEncVal[GLOB.currentChannel]) {
        lastEncVal[GLOB.currentChannel] = encVal;
        if (getDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx) != encVal) {
          setDefaultFastFilterValue(GLOB.currentChannel, dft.arr, dft.idx, encVal);
          switch (dft.arr) {
            case ARR_FILTER:
              setFilters(dft.idx, GLOB.currentChannel, false);
              break;
            case ARR_SYNTH:
              if (GLOB.currentChannel == 11) {
                updateSynthVoice(11);
              } else if ((GLOB.currentChannel == 13 || GLOB.currentChannel == 14) &&
                         (dft.idx == CUTOFF || dft.idx == RESONANCE || dft.idx == FILTER)) {
                extern void applyMonoSynthCrf(int ch);
                applyMonoSynthCrf(GLOB.currentChannel);
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
    if (muteModeActive) filtercheck();
  }
}

void filtercheck() {
  int ch = GLOB.currentChannel;
  if (ch < 0 || ch >= 15) return;

  FilterTarget dft = defaultFastFilter[ch];
  int page, slot;

  if (findSliderDefPageSlot(ch, dft.arr, dft.idx, page, slot) && !filterfreshsetted) {
    int currVal = getDefaultFastFilterValue(ch, dft.arr, dft.idx);


    if (currVal == 0 && lastDefaultFastFilterValue[ch] != 0) {
      currVal = lastDefaultFastFilterValue[ch];
    }

    if (currVal != lastDefaultFastFilterValue[ch]) {
      lastDefaultFastFilterValue[ch] = currVal;
      filterDrawActive = true;
      filterDrawValue = currVal;
      filterDrawColor = filterColors[page][slot];

      filterDrawEndTime = millis() + (isNowPlaying ? 1000 : 500);
    }
  }
}

void checkButtons() {

  bool changed = (memcmp(buttons, oldButtons, sizeof(buttons)) != 0);
  checkFastRec();

  if (changed) {
    noteUserActivity();


    for (int i = 0; i < NUM_ENCODERS; i++) {
      if (buttons[i] == 9 && oldButtons[i] == 0) {
        buttons[i] = 0;
      }
    }




    uint8_t snapshotButtons[NUM_ENCODERS];
    memcpy(snapshotButtons, buttons, sizeof(buttons));

    checkMode(snapshotButtons, false);



    memcpy(oldButtons, snapshotButtons, sizeof(buttons));




    for (int i = 0; i < NUM_ENCODERS; i++) {
      if (buttons[i] == 1 || buttons[i] == 9) {
        buttons[i] = 0;
      }
    }
  }
}

void exitMenuFromTouchInput() {
  extern bool inLookSubmenu;
  extern bool inRecsSubmenu;
  extern bool inMidiSubmenu;
  extern bool inVolSubmenu;
  extern bool inEtcSubmenu;
  extern bool menuEnteredFromSingleMode;
  extern int currentMenuPage;


  if (inLookSubmenu || inRecsSubmenu || inMidiSubmenu || inVolSubmenu || inEtcSubmenu) {
    int parentPage = 0;
    if (inLookSubmenu) parentPage = 5;
    else if (inRecsSubmenu) parentPage = 6;
    else if (inMidiSubmenu) parentPage = 7;
    else if (inVolSubmenu) parentPage = 4;
    else if (inEtcSubmenu) { extern int systemMenuParentPage(); parentPage = systemMenuParentPage(); }

    inLookSubmenu = false;
    inRecsSubmenu = false;
    inMidiSubmenu = false;
    inVolSubmenu = false;
    inEtcSubmenu = false;

    currentMenuPage = parentPage;
    menu.pos[3] = parentPage;
    Encoder[3].writeCounter((int32_t)parentPage);
  }


  if (menuEnteredFromSingleMode && !imageMode) {
    switchMode(&singleMode);
    GLOB.singleMode = true;
  } else {
    switchMode(&draw);
    GLOB.singleMode = false;
  }

  extern int drawMode;
  if (drawMode == 0) {
    Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
  } else {
    Encoder[0].writeRGBCode(0x000000);
  }
  Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
}

static void applyPendingTouch1ModeToggle(unsigned long now);
static void scheduleTouch1ModeToggle(bool toSingle, unsigned long now);
void enterSingleModeDirect();
void exitDrawFromSingleDirect();
static void triggerCurrentVoiceAudition(int pitchRow, int auditionVelocity = -1);

void checkTouchInputs() {
  if (currentMode == &touchyMode) {
    extern void checkTouchyTouches();
    checkTouchyTouches();
    return;
  }




  bool tv1 = readTouch1Pressed();
  bool tv2 = readTouch2Pressed();
  int tv3 = readRecTouchHeld() ? touchThreshold + 1 : 0;
  const bool touch4Pressed = readDebouncedTouch(3);


  if (tv1 || tv2 || tv3 > touchThreshold || touch4Pressed) {
    noteUserActivity();
  }




  {
    if (currentMode != &recordMode && touch4Pressed && !lastTouchState[3]) {
      const int ch = (int)GLOB.currentChannel;
      int auditionRow = basePitchRowForChannel(ch);
      if (currentMode == &singleMode && !isNowPlaying && GLOB.y >= 1 && GLOB.y <= 15) {
        auditionRow = (int)GLOB.y;
      }
      triggerCurrentVoiceAudition(auditionRow);
      if (currentMode == &singleMode && isNowPlaying) {
        const int row = basePitchRowForChannel(ch);
        if (row >= 1 && row <= 15 && isSingleModeChannelAllowed(ch)
            && !isChildVoiceDisabled(ch)) {
          placeLiveGridNote((uint8_t)ch, (uint8_t)row, (uint8_t)defaultVelocity,
                            NOTE_MIDI_PITCH_NONE, true, false);
        }
      }
    }
    lastTouchState[3] = touch4Pressed;
    touchState[3] = touch4Pressed;
  }


  if (currentMode == &bpmMode) {
    static bool lastTouch3State = false;
    static unsigned long tapTimes[4] = { 0, 0, 0, 0 };
    static uint8_t tapIndex = 0;
    static unsigned long lastTapTime = 0;
    const unsigned long TAP_TIMEOUT = 2000;


    static float kalmanBPM = 0.0f;
    static float kalmanP = 1.0f;
    const float kalmanQ = 0.01f;
    const float kalmanR = 2.0f;

    bool currentTouch3State = (tv3 > touchThreshold);


    if (currentTouch3State && !lastTouch3State) {
      unsigned long currentTime = millis();


      if (lastTapTime > 0 && (currentTime - lastTapTime) > TAP_TIMEOUT) {
        tapIndex = 0;
        for (uint8_t i = 0; i < 4; i++) tapTimes[i] = 0;

        kalmanBPM = 0.0f;
        kalmanP = 1.0f;
      }


      tapTimes[tapIndex % 4] = currentTime;
      tapIndex++;
      lastTapTime = currentTime;


      if (tapIndex >= 2) {

        uint8_t numTaps = min(tapIndex, (uint8_t)4);
        unsigned long totalInterval = 0;
        uint8_t intervalCount = 0;


        for (uint8_t i = 1; i < numTaps; i++) {
          uint8_t idx1 = (tapIndex - i - 1) % 4;
          uint8_t idx2 = (tapIndex - i) % 4;
          if (tapTimes[idx1] > 0 && tapTimes[idx2] > 0) {
            unsigned long interval = tapTimes[idx2] - tapTimes[idx1];
            if (interval > 0 && interval < 3000) {
              totalInterval += interval;
              intervalCount++;
            }
          }
        }


        if (intervalCount > 0) {
          float avgInterval = (float)totalInterval / (float)intervalCount;
          float measuredBPM = 60000.0f / avgInterval;


          measuredBPM = constrain(measuredBPM, (float)BPM_MIN, (float)BPM_MAX);


          if (kalmanBPM == 0.0f) {

            kalmanBPM = measuredBPM;
            kalmanP = kalmanR;
          } else {

            float P_pred = kalmanP + kalmanQ;


            float K = P_pred / (P_pred + kalmanR);
            kalmanBPM = kalmanBPM + K * (measuredBPM - kalmanBPM);
            kalmanP = (1.0f - K) * P_pred;
          }


          float calculatedBPM = kalmanBPM;
          calculatedBPM = constrain(calculatedBPM, (float)BPM_MIN, (float)BPM_MAX);


          SMP.bpm = calculatedBPM;
          currentMode->pos[3] = (unsigned int)calculatedBPM;
          Encoder[3].writeCounter((int32_t)calculatedBPM);


          extern void applyBPMDirectly(int bpm);
          applyBPMDirectly((int)calculatedBPM);


          extern void drawBPMScreen();
          drawBPMScreen();



        }
      }
    }

    lastTouch3State = currentTouch3State;
  }


  if (currentMode == &filterMode) {
    static bool lastFilterTouch3 = false;
    bool pressed = tv3 > touchThreshold;
    if (pressed && !lastFilterTouch3) {
      setCurrentFilterPageDefaultValues((unsigned int)GLOB.currentChannel);
    }
    lastFilterTouch3 = pressed;
  }

  bool newTouchState[2];
  newTouchState[0] = tv1;
  newTouchState[1] = tv2;



  bool bothTouched = newTouchState[0] && newTouchState[1];
  bool newBoth = bothTouched && !lastBothTouched;
  lastBothTouched = bothTouched;


  static bool touch1Held = false;
  static bool touch2Held = false;
  static int firstTouchPressed = 0;


  if (newTouchState[0] && !lastTouchState[0] && !touch2Held) {
    firstTouchPressed = 1;
  }
  if (newTouchState[1] && !lastTouchState[1] && !touch1Held) {
    firstTouchPressed = 2;
  }




  if (!newTouchState[0] && !newTouchState[1]) {
    firstTouchPressed = 0;
  }



  if (!newTouchState[0]) touch1Held = false;
  if (!newTouchState[1]) touch2Held = false;
  if (newTouchState[0]) touch1Held = true;
  if (newTouchState[1]) touch2Held = true;


  touchState[0] = newTouchState[0];
  touchState[1] = newTouchState[1];

  {
    const unsigned long touchNow = millis();
    if (touchState[1] || bothTouched) {
      touch1ModeTogglePending = false;
    } else {
      applyPendingTouch1ModeToggle(touchNow);
    }
  }

  if (newBoth) {
    touch1ModeTogglePending = false;







    static unsigned long lastBothTouchHandledTime = 0;
    const unsigned long BOTH_TOUCH_COOLDOWN_MS = 350;
    unsigned long now = millis();
    if (lastBothTouchHandledTime != 0 && (now - lastBothTouchHandledTime) < BOTH_TOUCH_COOLDOWN_MS) {

      goto skip_individual_touch;
    }

    if (currentMode == &singleMode || currentMode == &draw || currentMode == &menu) {

      if (GLOB.currentChannel < 0 || GLOB.currentChannel > 15) {
        GLOB.currentChannel = 1;
      }

      if (firstTouchPressed == 2) {


        if (GLOB.currentChannel != 0 && GLOB.currentChannel != 9 && GLOB.currentChannel != 10 && GLOB.currentChannel != 12 && GLOB.currentChannel != 15) {

          filterPage[GLOB.currentChannel] = 1;
          bypassModeSwitchDebounce = true;
          switchMode(&filterMode);
          bypassModeSwitchDebounce = false;
          lastBothTouchHandledTime = now;
        }

      } else {

        if (GLOB.currentChannel >= 1 && GLOB.currentChannel <= 8) {
          if (currentMode == &singleMode) {
            GLOB.singleMode = false;
          }
          bypassModeSwitchDebounce = true;
          switchMode(&set_Wav);
          bypassModeSwitchDebounce = false;
          lastBothTouchHandledTime = now;
        } else if (GLOB.currentChannel == 11 || GLOB.currentChannel == 13 || GLOB.currentChannel == 14) {
          bypassModeSwitchDebounce = true;
          switchMode(&loadSavePreset);
          bypassModeSwitchDebounce = false;
          lastBothTouchHandledTime = now;
        }
      }
    }
skip_individual_touch:
    ;
  }
  if (!bothTouched) {


    static unsigned long lastTouchTime[2] = { 0, 0 };
    unsigned long currentTime = millis();
    const unsigned long DEBOUNCE_TIME = 30;


    static bool childLockTouch2Pressed = false;
    static unsigned long childLockTouch2Time = 0;
    const unsigned long CHILD_LOCK_TIMEOUT_MS = 2000;


    if (childLockTouch2Pressed && (currentTime - childLockTouch2Time > CHILD_LOCK_TIMEOUT_MS)) {
      childLockTouch2Pressed = false;
    }


    if (childLockTouch2Pressed && touchState[0] && !lastTouchState[0] && (currentTime - lastTouchTime[0] > DEBOUNCE_TIME)) {
      if (currentMode == &draw || currentMode == &singleMode) {
        lastTouchTime[0] = currentTime;
        childLockTouch2Pressed = false;
        extern bool menuEnteredFromSingleMode;
        menuEnteredFromSingleMode = (currentMode == &singleMode);
        switchMode(&menu);
        goto end_switch1;
      }
    }


    if (touchState[0] && !lastTouchState[0] && (currentTime - lastTouchTime[0] > DEBOUNCE_TIME) && !touchState[1]) {



      if (currentMode == &draw && GLOB.y == 1) {
        static unsigned long lastTouch1PlayTime = 0;
        const unsigned long TOUCH1_PLAY_DEBOUNCE_MS = 100;
        if (lastTouch1PlayTime == 0 || (currentTime - lastTouch1PlayTime) >= TOUCH1_PLAY_DEBOUNCE_MS) {
          lastTouch1PlayTime = currentTime;
          lastTouchTime[0] = currentTime;
          play(true);
        }
        goto end_switch1;
      }

      if (currentMode == &draw) {
        if (imageMode) {

          lastTouchTime[0] = currentTime;
          cycleImgBrush();
        } else {
          scheduleTouch1ModeToggle(true, currentTime);
        }
      } else if (currentMode == &menu) {
        exitMenuFromTouchInput();
        lastTouchTime[0] = currentTime;
      } else if (currentMode == &singleMode) {
        scheduleTouch1ModeToggle(false, currentTime);
      } else {
        switchMode(&draw);
        if (currentMode == &draw) {
          GLOB.singleMode = false;
          lastTouchTime[0] = currentTime;
          extern int drawMode;
          if (drawMode == 0) {
            Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
          } else {
            Encoder[0].writeRGBCode(0x000000);
          }
          Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        }
      }
    }
end_switch1:



    if (currentMode != &filterMode && touchState[1] && !lastTouchState[1] && (currentTime - lastTouchTime[1] > DEBOUNCE_TIME) && !touchState[0]) {
      lastTouchTime[1] = currentTime;
      if (currentMode == &draw || currentMode == &singleMode) {
        extern bool getChildLockEnabled();
        if (getChildLockEnabled()) {

          childLockTouch2Pressed = true;
          childLockTouch2Time = currentTime;

          if (currentMode == &draw) {
            if (imageMode) {
              cycleImgBrush();
            } else {
              enterSingleModeDirect();
            }
          } else if (currentMode == &singleMode) {
            exitDrawFromSingleDirect();
          }
        } else {

          extern bool menuEnteredFromSingleMode;
          menuEnteredFromSingleMode = (currentMode == &singleMode);
          switchMode(&menu);
        }
      } else if (currentMode == &newFileMode) {

        extern void resetNewModeState();
        resetNewModeState();
        switchMode(&draw);

        extern int drawMode;
        if (drawMode == 0) {
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        } else {
          Encoder[0].writeRGBCode(0x000000);
        }
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else if (currentMode == &loadSaveTrack) {

        extern bool menuEnteredFromSingleMode;
        extern int currentMenuPage;
        extern Mode menu;
        currentMenuPage = 0;
        menu.pos[3] = 0;
        Encoder[3].writeCounter((int32_t)0);
        if (menuEnteredFromSingleMode) {
          switchMode(&singleMode);
          GLOB.singleMode = true;
        } else {
          switchMode(&draw);
          GLOB.singleMode = false;
        }
        extern int drawMode;
        if (drawMode == 0) {
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        } else {
          Encoder[0].writeRGBCode(0x000000);
        }
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else if (currentMode == &set_SamplePack) {

        extern bool menuEnteredFromSingleMode;
        extern int currentMenuPage;
        extern Mode menu;
        currentMenuPage = 1;
        menu.pos[3] = 1;
        Encoder[3].writeCounter((int32_t)1);
        if (menuEnteredFromSingleMode) {
          switchMode(&singleMode);
          GLOB.singleMode = true;
        } else {
          switchMode(&draw);
          GLOB.singleMode = false;
        }
        extern int drawMode;
        if (drawMode == 0) {
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        } else {
          Encoder[0].writeRGBCode(0x000000);
        }
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else if (currentMode == &loadSavePreset) {

        extern bool menuEnteredFromSingleMode;
        extern int currentMenuPage;
        extern Mode menu;
        currentMenuPage = 2;
        menu.pos[3] = 2;
        Encoder[3].writeCounter((int32_t)2);
        if (menuEnteredFromSingleMode) {
          switchMode(&singleMode);
          GLOB.singleMode = true;
        } else {
          switchMode(&draw);
          GLOB.singleMode = false;
        }
        extern int drawMode;
        if (drawMode == 0) {
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        } else {
          Encoder[0].writeRGBCode(0x000000);
        }
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else if (currentMode == &set_Wav) {

        extern bool menuEnteredFromSingleMode;
        extern int currentMenuPage;
        extern Mode menu;
        currentMenuPage = 2;
        menu.pos[3] = 2;
        Encoder[3].writeCounter((int32_t)2);
        if (menuEnteredFromSingleMode) {
          switchMode(&singleMode);
          GLOB.singleMode = true;
        } else {
          switchMode(&draw);
          GLOB.singleMode = false;
        }
        extern int drawMode;
        if (drawMode == 0) {
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        } else {
          Encoder[0].writeRGBCode(0x000000);
        }
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else if (currentMode == &bpmMode) {

        extern bool menuEnteredFromSingleMode;
        extern int currentMenuPage;
        extern Mode menu;
        currentMenuPage = 3;
        menu.pos[3] = 3;
        Encoder[3].writeCounter((int32_t)3);
        if (menuEnteredFromSingleMode) {
          switchMode(&singleMode);
          GLOB.singleMode = true;
        } else {
          switchMode(&draw);
          GLOB.singleMode = false;
        }
        extern int drawMode;
        if (drawMode == 0) {
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        } else {
          Encoder[0].writeRGBCode(0x000000);
        }
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else if (currentMode == &songMode) {

        extern bool menuEnteredFromSingleMode;
        extern int currentMenuPage;
        extern Mode menu;
        currentMenuPage = 8;
        menu.pos[3] = 8;
        Encoder[3].writeCounter((int32_t)8);
        if (menuEnteredFromSingleMode) {
          switchMode(&singleMode);
          GLOB.singleMode = true;
        } else {
          switchMode(&draw);
          GLOB.singleMode = false;
        }
        extern int drawMode;
        if (drawMode == 0) {
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        } else {
          Encoder[0].writeRGBCode(0x000000);
        }
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      } else if (currentMode == &menu) {
        exitMenuFromTouchInput();
      } else {
        switchMode(&draw);

        extern int drawMode;
        if (drawMode == 0) {
          Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
        } else {
          Encoder[0].writeRGBCode(0x000000);
        }
        Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
      }
    }
  }



  lastTouchState[0] = touchState[0];
  lastTouchState[1] = touchState[1];
  lastTouchState[2] = touchState[2];
}

void enterSingleModeDirect() {
  if (currentMode != &draw) return;
  if (!canEnterSingleMode(GLOB.y, (int)GLOB.currentChannel)) return;
  switchMode(&singleMode);
  if (currentMode == &singleMode) {
    GLOB.singleMode = true;
  }
}

void exitDrawFromSingleDirect() {
  if (currentMode != &singleMode) return;
  GLOB.currentChannel = GLOB.y - 1;
  switchMode(&draw);
  if (currentMode == &draw) {
    GLOB.singleMode = false;
    extern int drawMode;
    if (drawMode == 0) {
      Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    } else {
      Encoder[0].writeRGBCode(0x000000);
    }
    Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
  }
}

static void applyPendingTouch1ModeToggle(unsigned long now) {
  if (!touch1ModeTogglePending) return;
  const bool released = (!touchState[0] && !touchState[1]);
  const bool timedOut = (now >= touch1ModeToggleDueMs);
  if (!released && !timedOut) return;
  touch1ModeTogglePending = false;
  if (touch1ModeToggleToSingle) {
    enterSingleModeDirect();
  } else {
    exitDrawFromSingleDirect();
  }
}

static void scheduleTouch1ModeToggle(bool toSingle, unsigned long now) {
  touch1ModeTogglePending = true;
  touch1ModeToggleToSingle = toSingle;
  touch1ModeToggleDueMs = now + TOUCH1_CHORD_GRACE_MS;
}

void checkPendingSampleNotes() {

  unsigned long now = millis();


  const size_t MAX_PENDING_NOTES = 64;


  while (pendingSampleNotes.size() > MAX_PENDING_NOTES) {
    pendingSampleNotes.pop_back();
  }


  const unsigned long STALE_THRESHOLD = 10000;
  for (size_t i = 0; i < pendingSampleNotes.size(); ) {
    PendingNoteEvent &ev = pendingSampleNotes[i];
    bool isStale = (ev.triggerTime < (now - STALE_THRESHOLD));
    bool isDue = (now >= ev.triggerTime);

    if (isStale) {
      pendingSampleNotes[i] = pendingSampleNotes.back();
      pendingSampleNotes.pop_back();
      continue;
    }

    if (isDue) {
      if (!isChildVoiceDisabled((int)ev.channel)
          && !isChannelSampleReloadBusy((unsigned int)ev.channel)) {
        if (shouldSkipVoiceTriggerForSyncPreview((int)ev.channel)) {
          triggerSetWavSyncPreview(ev.velocity);
        } else {
          triggerSamplerVoice(ev.channel, ev.pitch, ev.velocity, true);
        }
      }
      pendingSampleNotes[i] = pendingSampleNotes.back();
      pendingSampleNotes.pop_back();
      continue;
    }

    ++i;
  }
}

void showDoRecord() {

  enum RecordState { STATE_NORMAL,
                     STATE_RECORDING,
                     STATE_PLAYBACK };
  static RecordState state = STATE_NORMAL;
  static bool forcedLineMonitor = false;


  static bool thresholdActive = false;
  static bool lastPressed0 = false, lastPressed2 = false;
  static bool lastRecTouch4 = false, lastRecTouch5 = false;
  static unsigned long recordingStartTime = 0;
  static bool playbackActive = false;
  static unsigned long playbackStartTime = 0;
  static uint32_t lastModeNameHash = 0;
  static uint32_t lastRecRgb[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
  static uint32_t lastRecRgbMs = 0;
  static float peakHoldLevel = 0.0f;
  static unsigned long peakHoldDecayMs = 0;


  extern bool disableThresholdFlag;
  extern AudioAnalyzePeak peakRec;
  extern AudioPlaySdWav playSdWav1;
  extern bool previewIsPlaying;
  extern int peakRecIndex;
  extern const int maxRecPeaks;
  extern float peakRecValues[];
  extern elapsedMillis mRecsecs;

  unsigned long nowMs = millis();
  if (nowMs - lastRecRgbMs > 80) {
    for (int i = 0; i < 4; i++) lastRecRgb[i] = 0xFFFFFFFF;
  }
  lastRecRgbMs = nowMs;
  auto setRecEncoderRGB = [&](uint8_t idx, uint32_t rgb) {
    if (idx < 4 && lastRecRgb[idx] != rgb) {
      lastRecRgb[idx] = rgb;
      Encoder[idx].writeRGBCode(rgb);
    }
  };


  uint32_t currentModeNameHash = hashString(currentMode->name);
  if (currentModeNameHash != lastModeNameHash) {
    lastModeNameHash = currentModeNameHash;
    state = STATE_NORMAL;
    thresholdActive = false;
    playbackActive = false;
    forcedLineMonitor = false;
    peakHoldLevel = 0.0f;
    lastPressed0 = pressed[0];
    lastPressed2 = pressed[2];
    lastRecTouch4 = readDebouncedTouch(3);
    lastRecTouch5 = readDebouncedTouch(4);
    for (int i = 0; i < 4; i++) lastRecRgb[i] = 0xFFFFFFFF;
    extern bool sampleIsLoaded, firstcheck;
    extern CachedSample previewCache;
    sampleIsLoaded = false;
    firstcheck = true;
    previewCache.valid = false;
  }


  extern AudioMixer4 mixer_end;
  extern AudioAmplifier audioInputAmp;
  static bool directPassThroughActive = false;

  if (previewIsPlaying && GLOB.y == 1 && !getSpkrEnabled()) {
    audioInputAmp.gain(1.0f);
    mixer_end.gain(3, 1.0f);
    directPassThroughActive = true;
  } else if (directPassThroughActive) {
    audioInputAmp.gain(1.0f);
    mixer_end.gain(3, 0.0f);
    directPassThroughActive = false;
    forcedLineMonitor = false;
  }


  if (!directPassThroughActive) {
    mixer_end.gain(3, recInputMonitorGain());
    forcedLineMonitor = true;
  } else if (forcedLineMonitor) {
    mixer_end.gain(3, 0.0f);
    forcedLineMonitor = false;
  }


  if (disableThresholdFlag) {
    thresholdActive = false;
    disableThresholdFlag = false;
  }


  static uint16_t inputLevelSamples[8] = { 0 };
  static int sampleIndex = 0;
  static float currentInputLevel = 0.0f;

  if (peakRec.available()) {
    float rawPeak = peakRec.read();
    inputLevelSamples[sampleIndex] = (uint16_t)(rawPeak * 10000.0f);
    sampleIndex = (sampleIndex + 1) % 8;
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) sum += inputLevelSamples[i];
    currentInputLevel = sum / 80.0f;


    if (isRecording && mRecsecs > 22 && peakRecIndex < maxRecPeaks) {
      mRecsecs = 0;
      peakRecValues[peakRecIndex++] = rawPeak;
    }
  }

  if (currentInputLevel >= peakHoldLevel) {
    peakHoldLevel = currentInputLevel;
    peakHoldDecayMs = nowMs;
  } else if (nowMs - peakHoldDecayMs > 45) {
    peakHoldDecayMs = nowMs;
    peakHoldLevel = max(0.0f, peakHoldLevel - 2.5f);
  }

  int triggerThreshold = constrain((int)currentMode->pos[0], 0, 100);


  bool touch4Now = readDebouncedTouch(3);
  bool touch5Now = readDebouncedTouch(4);
  bool touch4Edge = (touch4Now && !lastRecTouch4);
  bool touch5Edge = (touch5Now && !lastRecTouch5);
  lastRecTouch4 = touch4Now;
  lastRecTouch5 = touch5Now;


  if (isRecording) {
    flushAudioQueueToRAM2();
    state = STATE_RECORDING;
  } else if (playbackActive) {
    state = STATE_PLAYBACK;
  } else {
    state = STATE_NORMAL;
  }





  auto presentStaticRecFrame = [&]() {
    FastLEDclear();
    if (thresholdActive) {
      setRecEncoderRGB(0, 0x00FF00);
      drawIndicator('L', 'G', 1, true, false);
    } else {
      setRecEncoderRGB(0, 0xFFFF00);
      drawIndicator('L', 'Y', 1, false, false);
    }
    setRecEncoderRGB(1, 0x000000);
    setRecEncoderRGB(2, 0xFF0000);
    drawIndicator('L', 'R', 3, true, false);
    setRecEncoderRGB(3, 0x0000FF);
    drawIndicator('L', 'X', 4, false, false);

    const int touch4X = 9;
    const int touch5X = ((int)maxX >= 24) ? 24 : 12;
    CRGB touch4Col = thresholdActive ? CRGB(0, 255, 60) : CRGB(180, 140, 0);
    light(touch4X, 1, touch4Col);
    light(touch4X, 2, touch4Col);
    light(touch5X, 1, CRGB(255, 0, 0));
    light(touch5X, 2, CRGB(255, 0, 0));

    int thresholdHeight = (int)roundf(mapf((float)triggerThreshold, 0.0f, 100.0f, 0.0f, (float)maxY));
    for (int y = 1; y <= thresholdHeight && y <= (int)maxY; y++) {
      CRGB tCol = thresholdActive
          ? (y == thresholdHeight ? CRGB(120, 255, 140) : CRGB(0, 180, 40))
          : (y == thresholdHeight ? CRGB(255, 255, 120) : CRGB(160, 130, 0));
      light(1, y, tCol);
    }

    drawText("REC", ((int)maxX >= 28) ? 8 : 2, 9, CRGB(255, 0, 0));
    lastUpdate = 0;
    extern void FastLEDshow();
    FastLEDshow();
  };


  switch (state) {
    case STATE_NORMAL:
      {

        if (touch4Edge || (pressed[0] && !lastPressed0)) {
          thresholdActive = !thresholdActive;
          if (thresholdActive && triggerThreshold == 0) {
            triggerThreshold = 25;
            currentMode->pos[0] = 25;
            Encoder[0].writeCounter((int32_t)25);
          }
        }
        lastPressed0 = pressed[0];


        bool manualTrigger = touch5Edge;
        bool audioTrigger = (thresholdActive && triggerThreshold > 0 && currentInputLevel > triggerThreshold);

        if (manualTrigger || audioTrigger) {
          presentStaticRecFrame();
          startRecordingRAM();
          recordingStartTime = nowMs;
          state = STATE_RECORDING;
          lastPressed2 = pressed[2];
          return;
        }


        if (pressed[2] && !lastPressed2) {
          char path[160];
          buildSamplePath(0, 0, path, sizeof(path));
          if (path[0]) {
            if (playSdWav1.isPlaying()) playSdWav1.stop();
            playSdWav1.play(path);
            previewIsPlaying = true;
            playbackActive = true;
            playbackStartTime = nowMs;
            state = STATE_PLAYBACK;
          }
        }
        lastPressed2 = pressed[2];
        break;
      }

    case STATE_RECORDING:
      {

        if (touch4Edge || (pressed[0] && !lastPressed0)) {
          thresholdActive = false;
        }
        lastPressed0 = pressed[0];


        if (touch5Edge) {
          stopRecordingRAM((int)SMP.wav[GLOB.currentChannel].oldID, (int)SMP.wav[GLOB.currentChannel].fileID);
          thresholdActive = false;
          state = STATE_NORMAL;
          lastPressed2 = pressed[2];
          break;
        }


        if (nowMs - recordingStartTime > 1000) {
          if (thresholdActive && triggerThreshold > 0 && currentInputLevel < (triggerThreshold * 0.5f)) {
            stopRecordingRAM((int)SMP.wav[GLOB.currentChannel].oldID, (int)SMP.wav[GLOB.currentChannel].fileID);
            thresholdActive = false;
            state = STATE_NORMAL;
          }
        }
        lastPressed2 = pressed[2];
        break;
      }

    case STATE_PLAYBACK:
      {

        if (touch5Edge) {
          playSdWav1.stop();
          previewIsPlaying = false;
          playbackActive = false;
          thresholdActive = false;
          presentStaticRecFrame();
          startRecordingRAM();
          recordingStartTime = nowMs;
          state = STATE_RECORDING;
          lastPressed2 = pressed[2];
          return;
        }


        if (touch4Edge) {
          playSdWav1.stop();
          previewIsPlaying = false;
          playbackActive = false;
          thresholdActive = !thresholdActive;
          state = STATE_NORMAL;
          break;
        }


        if (pressed[2] && !lastPressed2) {
          playSdWav1.stop();
          previewIsPlaying = false;
          playbackActive = false;
          state = STATE_NORMAL;
        }
        lastPressed2 = pressed[2];


        if (nowMs - playbackStartTime > 300) {
          if (!playSdWav1.isPlaying()) {
            previewIsPlaying = false;
            playbackActive = false;
            state = STATE_NORMAL;
          }
        }
        break;
      }
  }



  if (isRecording) return;


  FastLEDclear();


  if (thresholdActive) {
    setRecEncoderRGB(0, 0x00FF00);
    drawIndicator('L', 'G', 1, true, false);
  } else {
    setRecEncoderRGB(0, 0xFFFF00);
    drawIndicator('L', 'Y', 1, false, false);
  }

  setRecEncoderRGB(1, 0x000000);

  if (state == STATE_RECORDING) {
    setRecEncoderRGB(2, 0xFF0000);
    drawIndicator('L', 'R', 3, true, false);
  } else if (state == STATE_PLAYBACK) {
    setRecEncoderRGB(2, 0x00FF00);
    drawIndicator('L', 'G', 3, true, false);
  } else {
    setRecEncoderRGB(2, 0x00FF00);
    drawIndicator('L', 'G', 3, false, false);
  }

  setRecEncoderRGB(3, 0x0000FF);
  drawIndicator('L', 'X', 4, false, false);


  bool pulseFast = ((nowMs / 180) % 2) == 0;
  const int touch4X = 9;
  const int touch5X = ((int)maxX >= 24) ? 24 : 12;
  CRGB touch4Col = thresholdActive
      ? (pulseFast ? CRGB(0, 255, 60) : CRGB(0, 140, 30))
      : CRGB(180, 140, 0);
  CRGB touch5Col = (state == STATE_RECORDING)
      ? (pulseFast ? CRGB(255, 0, 0) : CRGB(100, 0, 0))
      : CRGB(255, 0, 0);
  light(touch4X, 1, touch4Col);
  light(touch4X, 2, touch4Col);
  light(touch5X, 1, touch5Col);
  light(touch5X, 2, touch5Col);


  int thresholdHeight = (int)roundf(mapf((float)triggerThreshold, 0.0f, 100.0f, 0.0f, (float)maxY));
  int inputHeight = (int)roundf(mapf(currentInputLevel, 0.0f, 100.0f, 0.0f, (float)maxY));
  int peakHoldY = (int)roundf(mapf(peakHoldLevel, 0.0f, 100.0f, 0.0f, (float)maxY));

  for (int y = 1; y <= thresholdHeight && y <= (int)maxY; y++) {
    CRGB tCol = thresholdActive
        ? (y == thresholdHeight ? CRGB(120, 255, 140) : CRGB(0, 180, 40))
        : (y == thresholdHeight ? CRGB(255, 255, 120) : CRGB(160, 130, 0));
    light(1, y, tCol);
  }

  for (int y = 1; y <= inputHeight && y <= (int)maxY; y++) {
    CRGB vuCol;
    if (y >= 14) {
      vuCol = CRGB(255, 0, 30);
    } else if (y >= 10) {
      vuCol = CRGB(255, 140, 0);
    } else {
      vuCol = CRGB(0, 220, 90);
    }
    light(2, y, vuCol);
  }
  if (peakHoldY >= 1 && peakHoldY <= (int)maxY) {
    light(2, peakHoldY, (peakHoldY >= 14) ? CRGB(255, 60, 60) : CRGB(200, 220, 255));
  }


  const bool wide = ((int)maxX >= 28);
  if (state == STATE_RECORDING) {
    int secs = (int)(recTime / 1000);
    int tenths = (int)((recTime % 1000) / 100);
    char buf[12];
    if (wide) {
      drawText("REC", 5, 9, CRGB(255, 0, 0));
      snprintf(buf, sizeof(buf), "%d.%d", secs, tenths);
      drawText(buf, 18, 9, UI_ORANGE);
    } else {
      snprintf(buf, sizeof(buf), "%d.%d", secs, tenths);
      drawText(buf, 4, 9, CRGB(255, 0, 0));
    }
  } else if (state == STATE_PLAYBACK) {
    unsigned long playbackTime = nowMs - playbackStartTime;
    int secs = (int)(playbackTime / 1000);
    int tenths = (int)((playbackTime % 1000) / 100);
    char buf[12];
    if (wide) {
      drawText("PLAY", 5, 9, CRGB(0, 255, 80));
      snprintf(buf, sizeof(buf), "%d.%d", secs, tenths);
      drawText(buf, 21, 9, UI_ORANGE);
    } else {
      snprintf(buf, sizeof(buf), "%d.%d", secs, tenths);
      drawText(buf, 4, 9, CRGB(0, 255, 80));
    }
  } else {
    if (thresholdActive) {
      CRGB armCol = pulseFast ? CRGB(0, 255, 60) : CRGB(0, 170, 40);
      drawText("ARM", 5, 9, armCol);
      if (wide) {
        char thrBuf[8];
        snprintf(thrBuf, sizeof(thrBuf), "%d", triggerThreshold);
        drawText(thrBuf, 18, 9, armCol);
      }
    } else {
      drawText("RDY", 5, 9, UI_ORANGE);
      if (wide && peakRecIndex > 0) {
        drawText("OK", 19, 9, CRGB(0, 220, 80));
      }
    }
  }
}

void checkSerialColors() {
  extern bool inEtcSubmenu;
  extern int getCurrentMenuMainSetting();
  if (!inEtcSubmenu || getCurrentMenuMainSetting() != 41) return;

  if (Serial.available() >= 4) {
    if (Serial.peek() == 'C') {

      if (Serial.available() >= 94) {
        if (Serial.read() == 'C' && Serial.read() == 'O' && Serial.read() == 'L' && Serial.read() == 'R') {
          uint8_t buf[90];
          int bytesRead = Serial.readBytes(buf, 90);

          if (bytesRead != 90) {
            return;
          }


          for (int i = 0; i < 15; i++) {
            col[i] = CRGB(buf[i * 3], buf[i * 3 + 1], buf[i * 3 + 2]);
          }


          for (int i = 0; i < 15; i++) {
            int baseIdx = 45 + i * 3;
            col_base[i] = CRGB(buf[baseIdx], buf[baseIdx + 1], buf[baseIdx + 2]);
          }


          colorsUpdatedViaSerial = true;




          extern void menuRequestFullRedraw();
          menuRequestFullRedraw();


        }
      }
    } else if (Serial.peek() == 'B') {

      if (Serial.available() >= 5) {
        if (Serial.read() == 'B' && Serial.read() == 'R' && Serial.read() == 'I' && Serial.read() == 'T') {
          uint8_t brightness = Serial.read();
          brightness = constrain(brightness, 3, 255);


          ledBrightness = brightness;


          saveSingleModeToEEPROM(26, (int8_t)ledBrightness);


          extern Mode *currentMode;
          extern Mode bpmMode;
          if (currentMode == &bpmMode) {


            unsigned int brightnessPos = (brightness >= 3) ? (brightness - 3) : 0;
            brightnessPos = constrain(brightnessPos, 0, 252);
            currentMode->pos[1] = brightnessPos;
            Encoder[1].writeCounter((int32_t)brightnessPos);
          }


          extern void menuRequestFullRedraw();
          menuRequestFullRedraw();
        }
      }
    } else if (Serial.peek() == 'S') {

      if (Serial.available() >= 94) {
        if (Serial.read() == 'S' && Serial.read() == 'A' && Serial.read() == 'V' && Serial.read() == 'E') {
          uint8_t buf[90];
          int bytesRead = Serial.readBytes(buf, 90);

          if (bytesRead != 90) {
            return;
          }


          if (SD.exists("scheme.txt")) {
            SD.remove("scheme.txt");
          }
          File schemeFile = SD.open("scheme.txt", FILE_WRITE);
          if (schemeFile) {

            for (int i = 0; i < 15; i++) {
              schemeFile.write(buf[i * 3]);
              schemeFile.write(buf[i * 3 + 1]);
              schemeFile.write(buf[i * 3 + 2]);
            }


            for (int i = 0; i < 15; i++) {
              int baseIdx = 45 + i * 3;
              schemeFile.write(buf[baseIdx]);
              schemeFile.write(buf[baseIdx + 1]);
              schemeFile.write(buf[baseIdx + 2]);
            }

            schemeFile.close();



            EEPROM.write(EEPROM_DATA_START + 22, 3);
            markSettingsBackupDirty();


            currentColorScheme = 3;


            for (int i = 0; i < 15; i++) {
              col[i] = CRGB(buf[i * 3], buf[i * 3 + 1], buf[i * 3 + 2]);
            }
            for (int i = 0; i < 15; i++) {
              int baseIdx = 45 + i * 3;
              col_base[i] = CRGB(buf[baseIdx], buf[baseIdx + 1], buf[baseIdx + 2]);
            }


            colorsUpdatedViaSerial = true;


            extern void menuRequestFullRedraw();
            menuRequestFullRedraw();

          } else {
          }
        }
      }
    } else {
      Serial.read();
    }
  }
}

void loop() {
  checkSerialColors();


  {
    extern bool inEtcSubmenu;
    extern int getCurrentMenuMainSetting();
    extern void sdSerialServerSetActive(bool on);
    extern void sdSerialServerPoll();
    extern void sdSerialServerPollNeedSdHint();
    extern bool sdSerialServerIsActive();
    const bool wantSd = inEtcSubmenu && (getCurrentMenuMainSetting() == 49);
    if (wantSd != sdSerialServerIsActive()) {
      sdSerialServerSetActive(wantSd);
      extern void menuRequestFullRedraw();
      menuRequestFullRedraw();
    }
    if (wantSd) sdSerialServerPoll();
    else sdSerialServerPollNeedSdHint();
  }

  updateExternalOneBlink();
  checkMidi();
  serviceSynthLfo();
  serviceMidiArp();

  extern void serviceSettingsBackup();
  serviceSettingsBackup();
  servicePendingSamplePersist();

  yield();

#if BATT_DEBUG_SERIAL
  {
    static elapsedMillis battDebugTimer;
    if (battDebugTimer >= 2000) {
      battDebugTimer = 0;
      int raw = analogRead(BATT_ADC_PIN);
      int corrected = raw - BATT_RAW_ZERO;
      if (corrected < 0) corrected = 0;
      float vMeasured = (float)corrected * (3.3f / (float)(BATT_RAW_FULL - BATT_RAW_ZERO));
      float vBat = vMeasured / BATT_DIVIDER_RATIO;
      float norm = (vBat - BATT_V_MIN) / (BATT_V_MAX - BATT_V_MIN);
      if (norm < 0.0f) norm = 0.0f;
      if (norm > 1.0f) norm = 1.0f;
      float pct = powf(norm, BATT_PCT_CURVE) * 100.0f;
      if (pct < 0.0f) pct = 0.0f;
      if (pct > 100.0f) pct = 100.0f;
      Serial.print("BATT raw=");
      Serial.print(raw);
      Serial.print(" Vmeas=");
      Serial.print(vMeasured, 3);
      Serial.print(" VBAT=");
      Serial.print(vBat, 3);
      Serial.print(" %=");
      Serial.println((int)(pct + 0.5f));
    }
  }
#endif



  if (isrPlayButtonTick) {

    isrPlayButtonTick = false;
    drawPlayButton();
  }





  if (pendingPauseUIUpdate) {
    pendingPauseUIUpdate = false;
    updateLastPage();
    if (currentMode == &draw || currentMode == &singleMode) {
      if (childLockEnabled) {
        enforceChildModeRestrictions();
      } else if (ctrlMode == 0 && SMP_PATTERN_MODE) {
        Encoder[1].writeMax((int32_t)encoderPageMax());
      } else if (ctrlMode == 1) {
        refreshCtrlEncoderConfig();
      }
    }
    Encoder[2].writeRGBCode(0x00FF00);
    if (!pendingPauseSkipSave) {
      pendingPauseAutoSave = true;
    }
    pendingPauseSkipSave = false;
  } else if (pendingPauseAutoSave) {
    if (isNowPlaying) {
      pendingPauseAutoSave = false;
    } else if (sdIoAudioSettled() || pauseSdWaitMs > 2500) {
      pendingPauseAutoSave = false;
      autoSave();
    }
  }

  if (isrDbgPatternFinished) {
    isrDbgPatternFinished = false;
  }

  if (isrDbgLoopCountChanged) {
    isrDbgLoopCountChanged = false;
  }


  if (pendingStopFastRecord) {
    pendingStopFastRecord = false;
    stopFastRecord();
  }


  if (pendingStartFastRecord && !fastRecordActive) {
    pendingStartFastRecord = false;
    recordingBorderArmed = true;
    extern void presentRecordingBorder();
    presentRecordingBorder();
    startFastRecord();
  }


  if (fastRecordActive) {
    flushAudioQueueToRAM();




    static elapsedMillis recTriggerPoll;
    if (recTriggerPoll >= 8) {
      recTriggerPoll = 0;
      if (currentMode == &touchyMode) {

        currentEncoderIndex = 0;
        Encoder[0].updateStatus();
        handle_button_state(&Encoder[0], 0);
        checkButtons();
      } else checkFastRec();
    }
    flushAudioQueueToRAM();
    yield();
    return;
  }





  if (isRecording && currentMode == &recordMode) {
    flushAudioQueueToRAM2();
    static elapsedMillis smpRecPoll;
    if (smpRecPoll >= 8) {
      smpRecPoll = 0;
      checkEncoders();
      checkButtons();
      if (currentMode == &recordMode) {
        showDoRecord();
      }
    }
    flushAudioQueueToRAM2();
    yield();
    return;
  }





  extern bool songModeActive;
  if (!fastRecordActive && !childLockEnabled && SMP_FLOW_MODE && !(voiceMode && !songModeActive) && patternMode != 3 &&
      isNowPlaying &&
      (currentMode == &draw || currentMode == &singleMode)) {
    static uint16_t lastAppliedFlowPage = 0;

    uint16_t bf = beatForUI;
    uint16_t timerPage = (bf > 0) ? (uint16_t)((bf - 1) / maxX + 1) : 1;
    if (timerPage != lastAppliedFlowPage) {


      GLOB.edit = timerPage;
      editpage = timerPage;



      int localX = mapXtoPageOffset((int)GLOB.x);
      localX = constrain(localX, 1, (int)maxX);
      unsigned int newGlobalX = ((timerPage - 1) * maxX) + (unsigned int)localX;
      if (newGlobalX < 1) newGlobalX = 1;
      if (newGlobalX > (maxlen - 1)) newGlobalX = (maxlen - 1);
      GLOB.x = newGlobalX;
      currentMode->pos[3] = (int)newGlobalX;
      Encoder[3].writeCounter((int32_t)newGlobalX);


      if (ctrlMode == 0) {
        currentMode->pos[1] = timerPage;
        Encoder[1].writeCounter((int32_t)timerPage);
      }
      lastFlowPage = timerPage;
      lastAppliedFlowPage = timerPage;
    }
  }

  bool pongActive = pong && currentMode == &draw;

  if (pongActive) {
    updatePongBall();
  } else {
    resetPongGame();
  }



  if (currentMode == &draw || currentMode == &singleMode) {
    unsigned long now = millis();


    unsigned long effectiveRefresh = (unsigned long)RefreshTime;
    if (MIDI_CLOCK_SEND && !isNowPlaying) {
      if (effectiveRefresh < 100UL) effectiveRefresh = 100UL;
    }
    if ((unsigned long)(now - lastDrawUpdate) >= effectiveRefresh) {
      lastDrawUpdate = now;
      drawBase();
      drawTriggers();
      if (isNowPlaying) drawTimer();


      extern void drawCountIn();
      if (countInActive) {
        drawCountIn();
      }
      extern void drawSensCountIn();
      if (sensCountActive) {
        drawSensCountIn();
      }



      if (sensCountActive) {
        extern void drawRecordingBorderDotted();
        drawRecordingBorderDotted();
      } else if (recordingBorderArmed || countInActive || countInPending) {
        extern void drawRecordingBorder();
        drawRecordingBorder();
      }


      if (fullMuteIsActive()) {
        drawFullMuteBorder();
        drawFullMuteIndicators();
      }


      extern void drawBatteryWarning();
      extern bool getBatteryWarningActive();
      if (getBatteryWarningActive()) {
        drawBatteryWarning();
      }

      if (pongActive) {
        drawPongBall();
      }


      if (filterDrawActive) {
        if (millis() <= filterDrawEndTime) {
          drawFilterCheck(filterDrawValue, (FilterType)0, filterDrawColor);
        } else {
          filterDrawActive = false;
        }
      }


      if (ctrlVolumeOverlayActive && (currentMode == &draw || currentMode == &singleMode)) {
        if (millis() <= ctrlVolumeOverlayUntil) {
          drawCtrlVolumeOverlay(ctrlVolumeOverlayValue);
        } else {
          ctrlVolumeOverlayActive = false;
        }
      }


      if (inputGainOverlayActive && currentMode == &draw && GLOB.y == 1 && ctrlMode == 1) {
        if (millis() <= inputGainOverlayUntil) {
          drawInputGainOverlay(inputGainOverlayValue, inputGainOverlayMax);
        } else {
          inputGainOverlayActive = false;
        }
      }


      if (channelNrOverlayActive && currentMode == &draw && showChannelNr) {
        if (millis() <= channelNrOverlayUntil && GLOB.y <= 9) {
          drawChannelNrOverlay(channelNrOverlayChannel, GLOB.currentChannel);
        } else {
          channelNrOverlayActive = false;
        }
      }


      if (pageNrOverlayActive && showChannelNr
          && (currentMode == &draw || currentMode == &singleMode)) {
        if (millis() <= pageNrOverlayUntil) {
          drawPageNrOverlay(pageNrOverlayPage);
        } else {
          pageNrOverlayActive = false;
        }
      }



      yield();

      FastLEDshow();
      checkMidi();
    }
  }


  extern bool getLedStripEnabled();
  if (!fastRecordActive && getLedStripEnabled()) {
    updateLedStrip();
  }
# 7280 "/Users/jank/git/toern/toern.ino"
  if (!fastRecordActive && previewIsPlaying) {
    static elapsedMillis peakCaptureTimer;
    if (peakCaptureTimer > 15) {
      peakCaptureTimer = 0;

      if (playSdWav1.isPlaying() && g_previewUseLivePeaks && peakIndex < maxPeaks) {
        extern AudioAnalyzePeak peak1;
        if (peak1.available()) {
          peakValues[peakIndex] = peak1.read();
          peakIndex++;
        }
      }
    }

    if (!playSdWav1.isPlaying()) {
      playSdWav1.stop();
      previewIsPlaying = false;



      if (previewTriggerMode == PREVIEW_MODE_PRESS) {
        extern bool isEncoder0PressedMode();
        extern void setEncoder0PressedMode(bool state);
        setEncoder0PressedMode(false);

      }
    }
  }

  checkEncoders();
  if (!fastRecordActive && deviceHasFaders) {
    updateNeoSliderVolume();
  }




  if (!fastRecordActive && currentMode != &velocity && currentMode != &filterMode && currentMode != &menu
      && currentMode != &set_Wav && currentMode != &recordMode && currentMode != &touchyMode
      && currentMode != &loadSaveTrack && currentMode != &loadSavePreset && currentMode != &set_SamplePack) {
    drawCursor();
  }
  checkButtons();
  checkTouchInputs();
  checkMidi();


  if (fastRecordActive) {
    flushAudioQueueToRAM();
    yield();
    return;
  }

  checkPendingSampleNotes();
  serviceSetWavSyncPreview();
  serviceSdPreviewRequests();



  yield();



  static bool drawModeInputMonitorActive = false;
  bool shouldMonitor = false;
  if (currentMode == &draw && currentMode->name != "RECORD_MODE") {
    if (inputMonitoringState == 1 && GLOB.y == 1) {

      shouldMonitor = true;
    } else if (inputMonitoringState == 2) {

      shouldMonitor = true;
    }
  }

  if (shouldMonitor) {
    extern AudioMixer4 mixer_end;
    extern AudioAmplifier audioInputAmp;
    extern int recMode;


    audioInputAmp.gain(1.0f);
    mixer_end.gain(3, recInputMonitorGain());
    drawModeInputMonitorActive = true;
  } else if (drawModeInputMonitorActive && currentMode->name != "RECORD_MODE") {

    extern AudioMixer4 mixer_end;
    mixer_end.gain(3, 0.0f);
    drawModeInputMonitorActive = false;
  }




  updateAllMixerGains();



  static unsigned long pressedStateTime[NUM_ENCODERS] = { 0, 0, 0, 0 };
  static bool lastPressedState[NUM_ENCODERS] = { false, false, false, false };
  unsigned long now = millis();
  for (int i = 0; i < NUM_ENCODERS; i++) {
    if (pressed[i] != lastPressedState[i]) {

      pressedStateTime[i] = now;
      lastPressedState[i] = pressed[i];
    } else if (pressed[i] && (now - pressedStateTime[i] > 2000)) {


      uint8_t encoderStatus = Encoder[i].readStatus();
      bool actualButtonPressed = (encoderStatus != 0xFF && (encoderStatus & 0x08) != 0);
      if (!actualButtonPressed) {

        pressed[i] = false;
        pressedStateTime[i] = 0;
      }
    }
  }


  static unsigned long preventPaintUnpaintSetTime = 0;
  static bool lastPreventState = false;
  if (preventPaintUnpaint != lastPreventState) {
    preventPaintUnpaintSetTime = now;
    lastPreventState = preventPaintUnpaint;
  } else if (preventPaintUnpaint && (now - preventPaintUnpaintSetTime > 5000)) {

    preventPaintUnpaint = false;
    preventPaintUnpaintSetTime = 0;
  }


  extern int drawMode;

  if (drawMode == 0) {


    if (!imageMode && note[cursorNoteStep()][GLOB.y].channel == 0
        && (currentMode == &draw || currentMode == &singleMode) && pressed[3] == true && !preventPaintUnpaint) {
      paintMode = false;
      freshPaint = true;
      unpaintMode = false;
      pressed[3] = false;
      paint();
      preventPaintUnpaint = false;
    }

    if ((currentMode == &draw || currentMode == &singleMode) && pressed[0] == true && !preventPaintUnpaint) {

      if (GLOB.y == 16) {
        pressed[0] = false;
      } else {
        paintMode = false;
        unpaintMode = false;
        pressed[0] = false;
        unpaint();
      }
    }
  } else if (!childLockEnabled) {




    static Mode *rModeSavedMode = nullptr;
    static bool encoder0PressedOnEmptyNote = false;
    extern bool isPressed[NUM_ENCODERS];


    bool encoder0Held = isPressed[0];

    bool encoder0LongHeld = (buttons[0] == 2);


    static bool lastEncoder0State = false;
    if (encoder0Held && !lastEncoder0State) {

      if (note[GLOB.x][GLOB.y].channel == 0) {
        encoder0PressedOnEmptyNote = true;
      } else {
        encoder0PressedOnEmptyNote = false;
      }
    }
    lastEncoder0State = encoder0Held;


    if (!encoder0Held) {
      encoder0PressedOnEmptyNote = false;
    }

    if ((currentMode == &draw || currentMode == &singleMode) && encoder0LongHeld && !imageMode
        && !(currentMode == &draw && !GLOB.singleMode && (touchState[3] || readDebouncedTouch(3)))) {


      if (currentMode != &velocity && !encoder0PressedOnEmptyNote) {
        rModeSavedMode = currentMode;
        if (!freshPaint && note[GLOB.x][GLOB.y].channel != 0) {
          unsigned int velo = round(mapf(note[GLOB.x][GLOB.y].velocity, 1, 127, 1, maxY));
          unsigned int chvol = SMP.channelVol[GLOB.currentChannel];
          GLOB.velocity = velo;


          unsigned int prob = note[GLOB.x][GLOB.y].probability;
          unsigned int probStep;
          if (prob == 0) probStep = 1;
          else if (prob == 25) probStep = 2;
          else if (prob == 50) probStep = 3;
          else if (prob == 75) probStep = 4;
          else probStep = 5;

          unsigned int condStep =
              noteConditionToStep(note[GLOB.x][GLOB.y].condition);

          GLOB.singleMode = (rModeSavedMode == &singleMode);
          suppressDrawRMuteUntilMs = millis() + 800;
          switchMode(&velocity);
          Encoder[0].writeCounter((int32_t)velo);
          Encoder[1].writeCounter((int32_t)probStep);
          Encoder[2].writeCounter((int32_t)condStep);
          Encoder[3].writeCounter((int32_t)chvol);
          currentMode->pos[1] = probStep;
          currentMode->pos[2] = condStep;
          currentMode->pos[3] = chvol;
        }
      }
    } else if (currentMode == &velocity && rModeSavedMode != nullptr && !encoder0Held) {


      switchMode(rModeSavedMode);
      rModeSavedMode = nullptr;
    }
  }



  if (currentMode == &set_Wav && pressed[1] == true && !isRecording) {
    pressed[1] = false;
    reversePreviewSample();
  }

  if (childLockEnabled && !touchyEnabled && (currentMode == &filterMode || currentMode == &velocity || currentMode == &subpatternMode)) {
    switchMode(GLOB.singleMode ? &singleMode : &draw);
  }


  if (currentMode == &touchyMode) {
    extern void showTouchyMode();
    showTouchyMode();
  } else if (currentMode->name == "FILTERMODE") {


    setNewFilters();

  } else if (currentMode->name == "BPM") {
    setBPM();
  } else if (currentMode->name == "VELOCITY") {
    setVelocity();
  } else if (currentMode->name == "LOADSAVE_TRACK") {
    showLoadSave();
  } else if (currentMode->name == "LOADSAVE_PRESET") {
    showPresetLoadSave();
  } else if (currentMode->name == "MENU") {
    extern bool inLookSubmenu;
    extern bool inRecsSubmenu;
    extern bool inMidiSubmenu;
    extern bool inVolSubmenu;
    extern bool inEtcSubmenu;
    extern void showLookMenu();
    extern void showRecsMenu();
    extern void showMidiMenu();
    extern void showVolMenu();
    extern void showEtcMenu();
    if (inLookSubmenu) {
      showLookMenu();
    } else if (inRecsSubmenu) {
      showRecsMenu();
    } else if (inMidiSubmenu) {
      showMidiMenu();
    } else if (inVolSubmenu) {
      showVolMenu();
    } else if (inEtcSubmenu) {
      showEtcMenu();
    } else {
      showMenu();
    }
  } else if (currentMode->name == "SET_SAMPLEPACK") {
    showSamplePack();
  } else if (currentMode->name == "SET_WAV") {
    if (isRecording) {
    }
    else { showWave(); }
  } else if (currentMode->name == "RECORD_MODE") {
    showDoRecord();
  } else if (currentMode->name == "NOTE_SHIFT") {
    shiftNotes();
    drawBase();
    drawTriggers();
    if (isNowPlaying) {

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


    if (noteOnTriggered[ch] && !persistentNoteOn[ch] &&
        !sequencedMonoSynthActive[ch] &&
        (millis() - startTime[ch] >= noteLen)) {
      if (!envelopes[ch]) continue;
      envelopes[ch]->noteOff();
      noteOnTriggered[ch] = false;
    }
  }

  autoOffActiveNotes();

  filterfreshsetted = false;

  FastLEDshow();

  yield();
}

FLASHMEM void triggerExternalOneBlink() {
  if (MIDI_CLOCK_SEND) return;
  if (isNowPlaying) return;

  Encoder[EXTERNAL_ONE_ENCODER_INDEX].writeRGBCode(0xFFFFFF);
  externalOneBlinkActive = true;

  unsigned long beatDurationMs = 120;
  if (SMP.bpm > 0.0f) {
    beatDurationMs = (unsigned long)(60000.0f / (SMP.bpm * 4.0f));
    if (beatDurationMs < 50) beatDurationMs = 50;
  }
  externalOneBlinkUntil = millis() + beatDurationMs;
}

FLASHMEM void updateExternalOneBlink() {
  if (!externalOneBlinkActive) return;
  if ((long)(millis() - externalOneBlinkUntil) >= 0) {
    externalOneBlinkActive = false;
    Encoder[EXTERNAL_ONE_ENCODER_INDEX].writeRGBCode(0x000000);
  }
}

float getNoteDuration(int channel) {




  float delayMs = 0.0f;
  if (!channelHasVoiceDelay(channel)) {
    delayMs = mapf(SMP.param_settings[channel][DELAY], 0, maxfilterResolution, 0, maxParamVal[DELAY]);
  }
  float attackMs = mapf(SMP.param_settings[channel][ATTACK], 0, maxfilterResolution, maxParamVal[ATTACK], 0);
  float holdMs = mapf(SMP.param_settings[channel][HOLD], 0, maxfilterResolution, 0, maxParamVal[HOLD]);
  float decayMs = mapf(SMP.param_settings[channel][DECAY], 0, maxfilterResolution, 0, maxParamVal[DECAY]);

  float gateMs = delayMs + attackMs + holdMs + decayMs;

  if (channel == 13 || channel == 14) {



    gateMs = delayMs + attackMs + 8.0f;
  }


  float minLen = attackMs + 5.0f;
  if (gateMs < minLen) gateMs = minLen;


  if (gateMs < 5.0f) gateMs = 5.0f;
  return gateMs;
}

FLASHMEM void shiftNotes() {
  unsigned int patternLength = lastPage * maxX;
  if (currentMode->pos[3] != GLOB.shiftX) {

    int shiftDirectionX = 0;
    if (currentMode->pos[3] > GLOB.shiftX) {
      shiftDirectionX = 1;
    } else {
      shiftDirectionX = -1;
    }
    GLOB.shiftX = 8;

    Encoder[3].writeCounter((int32_t)8);
    currentMode->pos[3] = 8;

    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        tmp[nx][ny].channel = 0;
        tmp[nx][ny].velocity = defaultVelocity;
        tmp[nx][ny].probability = 100;
        tmp[nx][ny].condition = 1;
        tmp[nx][ny].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    }


    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (note[nx][ny].channel == GLOB.currentChannel) {
          int newposX = nx + shiftDirectionX;


          if (newposX < 1) {
            newposX = patternLength;
          } else if (newposX > patternLength) {
            newposX = 1;
          }
          tmp[newposX][ny].channel = GLOB.currentChannel;
          tmp[newposX][ny].velocity = note[nx][ny].velocity;
          tmp[newposX][ny].probability = note[nx][ny].probability;
          tmp[newposX][ny].condition = note[nx][ny].condition;
          tmp[newposX][ny].midiPitch = note[nx][ny].midiPitch;
        }
      }
    }
    shifted = true;
  }

  if (currentMode->pos[0] != GLOB.shiftY) {

    int shiftDirectionY = 0;
    if (currentMode->pos[0] > GLOB.shiftY) {
      shiftDirectionY = -1;
    } else {
      shiftDirectionY = +1;
    }
    GLOB.shiftY = 8;

    Encoder[0].writeCounter((int32_t)8);
    currentMode->pos[0] = 8;

    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        tmp[nx][ny].channel = 0;
        tmp[nx][ny].velocity = defaultVelocity;
        tmp[nx][ny].probability = 100;
        tmp[nx][ny].condition = 1;
        tmp[nx][ny].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    }


    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= 15; ny++) {
        if (note[nx][ny].channel == GLOB.currentChannel) {
          int newposY = ny + shiftDirectionY;

          if (newposY < 1) {
            newposY = 15;
          } else if (newposY > 15) {
            newposY = 1;
          }
          tmp[nx][newposY].channel = GLOB.currentChannel;
          tmp[nx][newposY].velocity = note[nx][ny].velocity;
          tmp[nx][newposY].probability = note[nx][ny].probability;
          tmp[nx][newposY].condition = note[nx][ny].condition;
          tmp[nx][newposY].midiPitch =
              note[nx][ny].midiPitch <= 127
                ? (uint8_t)constrain((int)note[nx][ny].midiPitch + shiftDirectionY, 0, 127)
                : NOTE_MIDI_PITCH_NONE;
        }
      }
    }
    shifted = true;
  }


  if (currentMode->pos[1] != GLOB.shiftY1) {

    int shiftDirectionY1 = 0;
    if (currentMode->pos[1] > GLOB.shiftY1) {
      shiftDirectionY1 = -1;
    } else {
      shiftDirectionY1 = +1;
    }
    GLOB.shiftY1 = 8;

    Encoder[1].writeCounter((int32_t)8);
    currentMode->pos[1] = 8;


    unsigned int pageStartX = ((GLOB.edit - 1) * maxX) + 1;
    unsigned int pageEndX = GLOB.edit * maxX;


    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        tmp[nx][ny].channel = 0;
        tmp[nx][ny].velocity = defaultVelocity;
        tmp[nx][ny].probability = 100;
        tmp[nx][ny].condition = 1;
        tmp[nx][ny].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    }



    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      const bool onCurrentPage = (nx >= pageStartX && nx <= pageEndX);
      for (unsigned int ny = 1; ny <= 15; ny++) {
        if (note[nx][ny].channel == GLOB.currentChannel) {
          int newposY = ny;
          uint8_t newMidiPitch = note[nx][ny].midiPitch;
          if (onCurrentPage) {
            newposY = (int)ny + shiftDirectionY1;
            if (newposY < 1) {
              newposY = 15;
            } else if (newposY > 15) {
              newposY = 1;
            }
            if (newMidiPitch <= 127) {
              newMidiPitch = (uint8_t)constrain((int)newMidiPitch + shiftDirectionY1, 0, 127);
            }
          }
          tmp[nx][newposY].channel = GLOB.currentChannel;
          tmp[nx][newposY].velocity = note[nx][ny].velocity;
          tmp[nx][newposY].probability = note[nx][ny].probability;
          tmp[nx][newposY].condition = note[nx][ny].condition;
          tmp[nx][newposY].midiPitch = newMidiPitch;
        }
      }
    }
    shifted = true;
  }

  if (shifted) {


    for (unsigned int nx = 1; nx <= patternLength; nx++) {
      for (unsigned int ny = 1; ny <= maxY; ny++) {
        if (original[nx][ny].channel != 0 && original[nx][ny].channel != GLOB.currentChannel) {
          note[nx][ny] = original[nx][ny];
        } else {
          note[nx][ny].channel = 0;
          note[nx][ny].velocity = defaultVelocity;
          note[nx][ny].probability = 100;
          note[nx][ny].condition = 1;
          note[nx][ny].midiPitch = NOTE_MIDI_PITCH_NONE;
        }

        if (tmp[nx][ny].channel == GLOB.currentChannel) {
          note[nx][ny] = tmp[nx][ny];
        }
      }
    }
    shifted = false;
  }




  preventPaintUnpaint = false;
}
void tmpMuteAll(bool pressed) {
  if (childLockEnabled) return;

  if (pressed) {

    if (!tmpMuteActive) {

      for (unsigned i = 1; i <= maxY; i++) {
        prevMuteState[i] = getMuteState(i);
      }

      for (unsigned i = 1; i <= maxY; i++) {
        setMuteState(i, (i == GLOB.currentChannel) ? false : true);
      }
      tmpMuteActive = true;
    }
  } else {

    if (tmpMuteActive) {
      for (unsigned i = 1; i <= maxY; i++) {
        setMuteState(i, prevMuteState[i]);
      }
      tmpMuteActive = false;
    }
  }
}

static void silenceAllVoicesForFullMute() {
  for (int ch = 0; ch <= 8; ch++) {
    for (int note = 36; note <= 96; note++) {
      samplerNoteEvent(ch, note, 0, false, false);
    }
  }

  stopSynthChannel(13);
  stopSynthChannel(14);
  resetMidiPressedKeyCount11to14();
}

static void syncMuteArraysToSmp() {
  for (int ch = 0; ch < maxY; ch++) {
    SMP.globalMutes[ch] = globalMutes[ch];
    SMP.mute[ch] = globalMutes[ch] ? 1u : 0u;
    for (int page = 0; page < maxPages; page++) {
      SMP.pageMutes[page][ch] = pageMutes[page][ch];
    }
  }
}

FLASHMEM void fullMuteApplyView() {
  for (int ch = 0; ch < maxY; ch++) {
    bool muted = true;
    switch (fullMuteView) {
      case FullMuteView::AllMuted:
        muted = true;
        break;
      case FullMuteView::Ch12:

        muted = (ch != 1 && ch != 2);
        break;
      case FullMuteView::Custom: {
        uint16_t mask = drawRFullMuteCustomUnmuteMask;
        muted = (mask & (1u << ch)) == 0;
        break;
      }
      case FullMuteView::UnmuteAll:
        muted = false;
        break;
    }
    globalMutes[ch] = muted;
    for (int page = 0; page < maxPages; page++) {
      pageMutes[page][ch] = muted;
    }
  }
  syncMuteArraysToSmp();
  applyMutesAfterPMODSwitch();
  if (fullMuteView == FullMuteView::AllMuted) {
    silenceAllVoicesForFullMute();
  } else {

    for (int ch = 0; ch < maxY; ch++) {
      if (!globalMutes[ch]) continue;
      if (ch >= 1 && ch <= 8) {
        for (int note = 36; note <= 96; note++) {
          samplerNoteEvent(ch, note, 0, false, false);
        }
      } else if (ch == 13 || ch == 14) {
        stopSynthChannel(ch);
      }
    }
  }
}

FLASHMEM void fullMuteEnter() {
  if (childLockEnabled) return;
  if (fullMuteActive) return;

  memcpy(fullMuteSavedGlobal, globalMutes, sizeof(globalMutes));
  memcpy(fullMuteSavedPages, pageMutes, sizeof(pageMutes));
  fullMuteActive = true;
  fullMuteView = FullMuteView::AllMuted;
  fullMuteApplyView();
}

FLASHMEM void fullMuteExitRestore() {
  if (!fullMuteActive) return;

  fullMuteActive = false;
  fullMuteView = FullMuteView::AllMuted;
  memcpy(globalMutes, fullMuteSavedGlobal, sizeof(globalMutes));
  memcpy(pageMutes, fullMuteSavedPages, sizeof(pageMutes));
  syncMuteArraysToSmp();
  applyMutesAfterPMODSwitch();
}

FLASHMEM void fullMuteToggleView(FullMuteView target) {
  if (!fullMuteActive) return;
  if (fullMuteView == target) {
    fullMuteView = FullMuteView::AllMuted;
  } else {
    fullMuteView = target;
  }
  fullMuteApplyView();
}

FLASHMEM void toggleMute() {
  if (childLockEnabled) return;



  bool currentMuteState = getMuteStateForUI(GLOB.currentChannel);
  setMuteState(GLOB.currentChannel, !currentMuteState);
}

void deleteActiveCopy() {

  GLOB.activeCopy = false;
}

void play(bool fromStart) {
  if (touchyEnabled) return;
  pendingPauseAutoSave = false;



  ctrlVolumeOverlayActive = false;

  stopSynthChannel(13);
  stopSynthChannel(14);
  resetMidiPressedKeyCount11to14();
  sequencerSynthTick = 0;
  resetSequencedSynthLegato();

  if (fromStart) {
    updateLastPage();


    if (currentMode == &draw || currentMode == &singleMode) {
      if (ctrlMode == 0 && SMP_PATTERN_MODE) {
        Encoder[1].writeMax((int32_t)encoderPageMax());
      } else if (ctrlMode == 1) {
        refreshCtrlEncoderConfig();
      }
    }

    lastFlowPage = 0;
    deleteActiveCopy();


    extern bool songModeActive;
    extern int currentSongPosition;

    if (songModeActive) {

      patternMode = 2;
      SMP_PATTERN_MODE = true;


      currentSongPosition = -1;
      for (int i = 0; i < 64; i++) {
        if (SMP.songArrangement[i] > 0) {
          currentSongPosition = i;
          break;
        }
      }

      if (currentSongPosition >= 0 && SMP.songArrangement[currentSongPosition] > 0) {
        int pattern = SMP.songArrangement[currentSongPosition];
        GLOB.edit = pattern;
        GLOB.page = pattern;
        beat = (pattern - 1) * maxX + 1;
      } else {

        songModeActive = false;
        patternMode = 1;
        SMP_PATTERN_MODE = true;
        beat = (GLOB.edit - 1) * maxX + 1;
        GLOB.page = GLOB.edit;
      }
    } else if (SMP_PATTERN_MODE) {

      extern int patternMode;
      if (patternMode == 3) {

        GLOB.edit = constrain(
            (int)GLOB.edit, 1, effectivePageCount());
        GLOB.page = GLOB.edit;
        pendingPage = 0;
        beat = (GLOB.page - 1) * maxX + 1;
      } else {

        beat = (GLOB.edit - 1) * maxX + 1;
        GLOB.page = GLOB.edit;
      }
    } else {
      beat = 1;
      GLOB.page = 1;
    }

    enforceChildModeRestrictions();


    loopCount = 1;
    if (voiceMode) {
      extern bool songModeActive;
      updateVoiceLengths();
      for (int c = 0; c < 16; c++) voiceLoopCount[c] = 1;
      if (!songModeActive && !childLockEnabled) {
        beat = 1;
        GLOB.page = 1;
      }
    }

    Encoder[2].writeRGBCode(0xFFFF00);
    if (MIDI_CLOCK_SEND) {

      extern unsigned long effectiveTransportSendDelayMs();
      extern void armMasterTransportStartDelay();
      extern void resetMidiClockForTransportStart();
      resetMidiClockForTransportStart();
      if (effectiveTransportSendDelayMs() == 0) {
        isNowPlaying = true;
        playStartTime = millis();
        extern void pulseClockOnTransportStart();
        pulseClockOnTransportStart();
        if (SMP.bpm > 0.0f) {
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
        fillActiveMidiPitch = NOTE_MIDI_PITCH_NONE;
      } else {
        armMasterTransportStartDelay();
        isNowPlaying = false;
      }
    } else {

      pendingStartOnBar = true;
      isNowPlaying = false;
      extern bool getAnalogExtClock();
      extern void analogClockCatchIfHigh();
      if (getAnalogExtClock()) analogClockCatchIfHigh();
    }
  }



}

void pause(bool skipSave) {

  if (MIDI_CLOCK_SEND && MIDI_TRANSPORT_SEND) {
    MIDI.sendRealTime(midi::Stop);
  }
  MidiReleaseSequencerNotes();
  clearLiveRecordSkip();
  sequencerRecordStepStartedUs = 0;





  ctrlVolumeOverlayActive = false;
  isNowPlaying = false;
  pendingStartOnBar = false;
  lastFlowPage = 0;
  fillTimer.end();
  deleteActiveCopy();


  envelope0.noteOff();
  stopSynthChannel(13);
  stopSynthChannel(14);
  resetMidiPressedKeyCount11to14();
  sequencerSynthTick = 0;
  resetSequencedSynthLegato();


  beat = 1;
  beatForUI = beat;
  GLOB.page = 1;
  loopCount = 0;
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



  pendingPauseUIUpdate = true;
  pendingPauseSkipSave = skipSave;
  pauseSdWaitMs = 0;
  pauseSdSettleGen++;
}

bool sdIoAudioSettled() {
  if (isNowPlaying) return false;

  for (int ch = 0; ch < 15; ch++) {
    if (envelopes[ch] && envelopes[ch]->isActive()) return false;
  }
  for (int v = 0; v < 3; v++) {
    if (Senvelope1[v] && Senvelope1[v]->isActive()) return false;
    if (Senvelope2[v] && Senvelope2[v]->isActive()) return false;
    if (SenvelopeFilter1[v] && SenvelopeFilter1[v]->isActive()) return false;
  }
  if (playSdWav1.isPlaying()) return false;


  if (pendingPauseAutoSave) {
    for (int ch = 0; ch <= 8; ch++) {
      if (voices[ch] && voices[ch]->isPlaying()) {
        toern_audio::StateGuard guard;
        voices[ch]->stop();
      }
    }
  }

  static uint8_t quietGen = 255;
  static elapsedMillis quietMs;
  if (quietGen != pauseSdSettleGen) {
    quietGen = pauseSdSettleGen;
    quietMs = 0;
    return false;
  }

  if (quietMs < 120) return false;
  return true;
}

static int arpSigned(int ch) {
  return (int)SMP.synth_settings[ch][ARP_STEP] - ARP_CENTER;
}

bool synthMidiArpEnabled(int ch) {
  return (ch == 11 || ch == 13 || ch == 14) && arpSigned(ch) != 0;
}

static bool arpUsesEighths(int ch) {
  return arpSigned(ch) < 0;
}

static int arpPingPongIndex(uint32_t step, int spanSteps) {
  if (spanSteps < 2) spanSteps = 2;
  int upPeak = spanSteps - 1;
  int period = upPeak * 2;
  int phasePos = (int)(step % (uint32_t)period);
  return (phasePos > upPeak) ? (period - phasePos) : phasePos;
}

static int arpSpanSteps(int ch) {
  int spanSteps = (int)lroundf(mapf(SMP.synth_settings[ch][ARP_SPAN], 0,
                                    maxfilterResolution, 2.0f, 16.0f));
  if (spanSteps < 2) spanSteps = 2;
  return spanSteps;
}

static volatile uint16_t midiArpLastBeat[15] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                                 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                                 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
static volatile unsigned long midiArpLastMs[15] = { 0 };
static volatile int16_t midiArpLastCh11Note = INT16_MIN;
static volatile bool midiArpKick[15] = { false };

void midiArpResetVoice(int ch) {
  if (ch < 0 || ch >= 15) return;
  toern_audio::StateGuard guard;
  bool hadPhrase = (arpStepCounter[ch] != 0 || midiArpKick[ch] ||
                    midiArpLastBeat[ch] != 0xFFFF ||
                    (ch == 11 && midiArpLastCh11Note != INT16_MIN));
  arpStepCounter[ch] = 0;
  midiArpLastBeat[ch] = 0xFFFF;
  midiArpLastMs[ch] = 0;
  midiArpKick[ch] = false;
  if (!hadPhrase) return;
  if (ch == 11 && midiArpLastCh11Note != INT16_MIN) {
    stopSound(midiArpLastCh11Note, 0);
    midiArpLastCh11Note = INT16_MIN;
  } else if (ch == 13 || ch == 14) {
    stopSynthChannel(ch);
  }
}

static void midiArpFireChannel(int ch);

void midiArpRestart(int ch) {
  if (ch < 0 || ch >= 15) return;
  toern_audio::StateGuard guard;
  arpStepCounter[ch] = 0;
  midiArpLastBeat[ch] = 0xFFFF;
  midiArpLastMs[ch] = millis();
  midiArpKick[ch] = false;
  if (isNowPlaying) {
    const bool eighths = arpUsesEighths(ch);
    uint16_t b = quantizedMidiRecordBeat();
    midiArpLastBeat[ch] = eighths ? (uint16_t)((b + 1u) / 2u) : b;
  }
  midiArpFireChannel(ch);
}

void midiArpOnHeldNote(int ch) {
  if (ch < 0 || ch >= 15) return;
  if (pressedKeyCount[ch] == 1) midiArpRestart(ch);
}

static void midiArpFireChannel(int ch) {
  extern int collectHeldMidiNotes(int ch, uint8_t *pitches, uint8_t *vels, int maxN);
  extern uint8_t midiPitchToGridRow(int channel, uint8_t pitch);
  extern bool MIDI_WRITE_OVERWRITE;

  uint8_t pitches[16];
  uint8_t vels[16];
  int nHeld = collectHeldMidiNotes(ch, pitches, vels, 16);
  if (nHeld <= 0) return;


  int reflected = arpPingPongIndex(arpStepCounter[ch]++, arpSpanSteps(ch));
  int midiPitch = pitches[0];
  uint8_t vel = vels[0];
  if (nHeld >= 2) {
    int idx = reflected % nHeld;
    midiPitch = (int)pitches[idx] + 12 * (reflected / nHeld);
    vel = vels[idx];
  } else {
    int mag = abs(arpSigned(ch));
    float arpSemis = mapf((float)mag, 1.0f, (float)ARP_CENTER, 1.0f, 12.0f);
    midiPitch = (int)pitches[0] + (int)lroundf((float)reflected * arpSemis);
  }
  midiPitch = constrain(midiPitch, 0, 127);
  if (ch == 11) {
    extern bool MIDI_NOTE_CLAMP;
    extern int voiceSelect;
    const bool preservePitch = !MIDI_NOTE_CLAMP && voiceSelect != 2;
    int noteValue = 12 * (int)octave[0] + transpose;
    noteValue += preservePitch ? (midiPitch - 60)
                               : ((int)midiPitchToGridRow(11, (uint8_t)midiPitch) - 1);
    if (midiArpLastCh11Note != INT16_MIN) stopSound(midiArpLastCh11Note, 0);
    playSound(noteValue, 0, vel);
    midiArpLastCh11Note = (int16_t)noteValue;
  } else {
    playSynthMidi(ch, (uint8_t)midiPitch, vel, true);
  }

  if (isNowPlaying && GLOB.singleMode) {
    uint8_t row = midiPitchToGridRow(ch, (uint8_t)midiPitch);
    placeLiveGridNote((uint8_t)ch, row, vel, (uint8_t)midiPitch, false,
                      !MIDI_WRITE_OVERWRITE);
  }
}

static float monoSynthCurrFreq0[2] = { 0.0f, 0.0f };
static float monoSynthCurrFreq1[2] = { 0.0f, 0.0f };
static float monoSynthTargetFreq0[2] = { 0.0f, 0.0f };
static float monoSynthTargetFreq1[2] = { 0.0f, 0.0f };
static bool monoSynthGliding[2] = { false, false };
static unsigned long monoSynthGlideLastMs[2] = { 0, 0 };
static unsigned long monoSynthEnvStartMs[2] = { 0, 0 };
static uint8_t monoSynthLastVel[2] = { 100, 100 };
static bool monoSynthEnvActive[2] = { false, false };

static void computeMonoSynthFilterEnv(int ch, unsigned long nowMs, float baseHz,
                                      float baseRes, float &outHz,
                                      float &outRes, bool &outActive) {
  outHz = baseHz;
  outRes = baseRes;
  outActive = false;
  if (ch != 13 && ch != 14) return;
  int mIdx = ch - 13;
  uint8_t flt = (uint8_t)constrain((int)SMP.synth_settings[ch][FILTER], 0,
                                   (int)maxfilterResolution);
  if (flt == 0 || !monoSynthEnvActive[mIdx]) return;

  float dtMs = (float)(nowMs - monoSynthEnvStartMs[mIdx]);
  if (dtMs < 0.0f) dtMs = 0.0f;
  float decaySlider = (float)constrain((int)SMP.param_settings[ch][DECAY], 0,
                                       (int)maxfilterResolution);
  float fltDecayMs = (ch == 14)
      ? mapf(decaySlider, 0.0f, (float)maxfilterResolution, 110.0f, 480.0f)
      : mapf(decaySlider, 0.0f, (float)maxfilterResolution, 140.0f, 700.0f);

  float velNorm = (float)monoSynthLastVel[mIdx] / 127.0f;

  float accent = (ch == 14 && monoSynthLastVel[mIdx] >= 100)
      ? mapf((float)monoSynthLastVel[mIdx], 100.0f, 127.0f, 0.15f, 1.0f)
      : 0.0f;
  if (accent > 0.0f && fltDecayMs > 210.0f) {
    fltDecayMs = 210.0f;
  }

  float env = expf(-dtMs / fltDecayMs);
  if (env < 0.003f) {
    monoSynthEnvActive[mIdx] = false;
    return;
  }

  float envOctaves =
      mapf((float)flt, 0.0f, (float)maxfilterResolution, 0.0f, 2.2f) *
      (0.70f + 0.30f * velNorm + 0.25f * accent);
  outHz = constrain(baseHz * powf(2.0f, env * envOctaves), 20.0f, 9000.0f);
  outRes = constrain(baseRes + accent * env * 0.35f, 0.7f, 2.6f);
  outActive = true;
}

void serviceSynthLfo() {
  static float delayBpm = -1.0f;
  float bpmNow = (SMP.bpm > 0.0f) ? SMP.bpm : 100.0f;
  if (fabsf(bpmNow - delayBpm) > 0.05f) {
    delayBpm = bpmNow;
    extern void applyAllVoiceDelays();
    applyAllVoiceDelays();
  }
  unsigned long nowMs = millis();

  for (int ch = 13; ch <= 14; ch++) {
    int mIdx = ch - 13;
    if (!monoSynthGliding[mIdx] || !synths[ch][0] || !synths[ch][1]) continue;
    float dt = (float)(nowMs - monoSynthGlideLastMs[mIdx]);
    if (dt < 2.0f) continue;
    monoSynthGlideLastMs[mIdx] = nowMs;
    const float tauMs = 55.0f;
    float alpha = 1.0f - expf(-dt / tauMs);
    monoSynthCurrFreq0[mIdx] +=
        (monoSynthTargetFreq0[mIdx] - monoSynthCurrFreq0[mIdx]) * alpha;
    monoSynthCurrFreq1[mIdx] +=
        (monoSynthTargetFreq1[mIdx] - monoSynthCurrFreq1[mIdx]) * alpha;
    if (fabsf(monoSynthTargetFreq0[mIdx] - monoSynthCurrFreq0[mIdx]) < 0.15f) {
      monoSynthCurrFreq0[mIdx] = monoSynthTargetFreq0[mIdx];
      monoSynthCurrFreq1[mIdx] = monoSynthTargetFreq1[mIdx];
      monoSynthGliding[mIdx] = false;
    }
    toern_audio::StateGuard guard;
    synths[ch][0]->frequency(monoSynthCurrFreq0[mIdx]);
    synths[ch][1]->frequency(monoSynthCurrFreq1[mIdx]);
  }

  static bool lfoWasOn[15] = { false };
  extern uint16_t cutoff[2][3];
  extern float resonance[2][3];
  for (int ch = 11; ch <= 14; ch++) {
    if (ch == 12 || !filters[ch]) continue;
    float rateHz = 0.0f;
    if (SMP.synth_settings[ch][LFO_RATE] > 0) {
      rateHz = mapf(SMP.synth_settings[ch][LFO_RATE], 1, maxfilterResolution,
                    0.25f, 10.0f);
    }
    float depth = mapf(SMP.synth_settings[ch][LFO_DEPTH], 0, maxfilterResolution,
                       0.0f, 1.0f);
    bool on = (rateHz > 0.0f && depth > 0.0f);
    float lcut = SMP.filter_settings[ch][LOWCUT];
    float baseHz;
    if (lcut > 0.0f) {
      baseHz = mapf(constrain(lcut, 1.0f, (float)maxfilterResolution), 1.0f,
                    (float)maxfilterResolution, 40.0f, 2500.0f);
    } else {
      baseHz = mapf(SMP.filter_settings[ch][HCUT], 0, maxfilterResolution,
                    281.25f, 10000.0f);
    }
    float baseRes = mapf(SMP.filter_settings[ch][RES], 0, maxfilterResolution,
                         0.7f, 5.0f);

    float envHz = baseHz;
    float envRes = baseRes;
    bool envActive = false;
    if (ch == 13 || ch == 14) {
      computeMonoSynthFilterEnv(ch, nowMs, baseHz, baseRes, envHz, envRes,
                                envActive);
    }

    if (!on && !envActive) {
      if (lfoWasOn[ch]) {
        filters[ch]->frequency(baseHz);
        filters[ch]->resonance(baseRes);
        if (ch == 11) {
          for (int i = 0; i < 3; i++) {
            Sladder1[i]->frequency(cutoff[0][i]);
            Sladder1[i]->resonance(resonance[0][i]);
            Sladder2[i]->frequency(cutoff[0][i]);
            Sladder2[i]->resonance(resonance[0][i]);
          }
        }
        lfoWasOn[ch] = false;
      }
      continue;
    }
    if (!on && envActive) {
      filters[ch]->frequency(envHz);
      filters[ch]->resonance(envRes);
      lfoWasOn[ch] = true;
      continue;
    }
    float lfo = sinf((float)nowMs * rateHz * 0.0062831853f);
    float modHz = envHz * powf(2.0f, lfo * depth * LFO_CUTOFF_OCTAVES);
    filters[ch]->frequency(constrain(modHz, 10.0f, 12000.0f));
    float modRes = envRes + lfo * depth * LFO_RES_SWEEP;
    filters[ch]->resonance(constrain(modRes, 0.7f, 5.0f));
    if (ch == 11) {
      for (int i = 0; i < 3; i++) {
        float ladHz = (float)cutoff[0][i] * powf(2.0f, lfo * depth * LFO_CUTOFF_OCTAVES);
        float ladRes = resonance[0][i] + lfo * depth * LFO_RES_SWEEP;
        Sladder1[i]->frequency(constrain(ladHz, 10.0f, 12000.0f));
        Sladder1[i]->resonance(constrain(ladRes, 0.0f, 5.0f));
        Sladder2[i]->frequency(constrain(ladHz, 10.0f, 12000.0f));
        Sladder2[i]->resonance(constrain(ladRes, 0.0f, 5.0f));
      }
    }
    lfoWasOn[ch] = true;
  }
}

void serviceMidiArpOnPlayhead(uint16_t playedBeat) {
  for (int ch = 11; ch <= 14; ch++) {
    if (ch == 12) continue;
    if (!synthMidiArpEnabled(ch) || pressedKeyCount[ch] <= 0) {
      midiArpResetVoice(ch);
      continue;
    }
    const bool eighths = arpUsesEighths(ch);
    if (eighths && (playedBeat % 2u) == 0 && !midiArpKick[ch]) continue;
    uint16_t grid = eighths ? (uint16_t)((playedBeat + 1u) / 2u) : playedBeat;
    if (grid == midiArpLastBeat[ch] && !midiArpKick[ch]) continue;
    midiArpLastBeat[ch] = grid;
    midiArpKick[ch] = false;
    midiArpFireChannel(ch);
  }
}

void serviceMidiArp() {
  unsigned long now = millis();
  float bpm = (SMP.bpm > 0.0f) ? SMP.bpm : 100.0f;

  for (int ch = 11; ch <= 14; ch++) {
    if (ch == 12) continue;
    if (!synthMidiArpEnabled(ch) || pressedKeyCount[ch] <= 0) {
      midiArpResetVoice(ch);
      continue;
    }

    if (isNowPlaying) {
      if (!midiArpKick[ch]) continue;
      const bool eighths = arpUsesEighths(ch);
      uint16_t b = sequencerRecordBeat;
      midiArpLastBeat[ch] = eighths ? (uint16_t)((b + 1u) / 2u) : b;
      midiArpKick[ch] = false;
      midiArpFireChannel(ch);
      continue;
    }

    const bool eighths = arpUsesEighths(ch);
    unsigned long stepMs = (unsigned long)lroundf(60000.0f / (bpm * (eighths ? 2.0f : 4.0f)));
    if (stepMs < 20) stepMs = 20;
    if ((now - midiArpLastMs[ch]) < stepMs && !midiArpKick[ch]) continue;
    midiArpKick[ch] = false;
    midiArpLastMs[ch] = now;
    midiArpFireChannel(ch);
  }
}

static void playSynthInternal(int ch, int b, int vel, bool persistant,
                              bool legato, int midiPitchOverride) {
  if (isChildVoiceDisabled(ch)) return;
  if (ch < 0 || ch >= 15) return;
  if (!synths[ch][0] || !synths[ch][1]) return;



  if (ch >= 13 && ch <= 14 && !persistant && pressedKeyCount[ch] > 0) {
    return;
  }

  extern void sidechainOnVoice(uint8_t);
  sidechainOnVoice((uint8_t)ch);

  float frequency;
  if (midiPitchOverride >= 0 && midiPitchOverride <= 127) {


    frequency = 440.0f * powf(2.0f, ((float)midiPitchOverride - 81.0f) / 12.0f);
  } else {
    frequency = notesArray[constrain(b - ch + 47, 36, 62)];
  }
  float WaveFormVelocity = mapf(vel, 1, 127, 0.0, 1.0);


  float centSemis = mapf(SMP.synth_settings[ch][CENT], 0, maxfilterResolution, -24.0f, 24.0f);
  float cent_ratio = powf(2.0f, centSemis / 12.0f);


  float detune_amount = mapf(SMP.filter_settings[ch][DETUNE], 0, maxfilterResolution, -1.0f / 12.0f, 1.0f / 12.0f);
  float detune_ratio = powf(2.0f, detune_amount);


  int octave_shift = mapf(SMP.filter_settings[ch][OCTAVE], 0, maxfilterResolution, -3, +3);
  float octave_ratio = powf(2.0f, (float)octave_shift);

  float base = frequency * cent_ratio;
  float freq1 = base * octave_ratio * detune_ratio;
  float mappedSustain = 0.0f;
  if (!persistant && (ch == 13 || ch == 14)) {
    mappedSustain = mapf(SMP.param_settings[ch][SUSTAIN], 0, maxfilterResolution, 0, maxParamVal[SUSTAIN]);
  }

  unsigned long nowMs = millis();
  bool doSlide = false;
  if (ch == 13 || ch == 14) {
    int mIdx = ch - 13;
    uint8_t velClamped = (uint8_t)constrain(vel > 0 ? vel : defaultVelocity, 1, 127);
    monoSynthLastVel[mIdx] = velClamped;
    doSlide = legato && monoSynthCurrFreq0[mIdx] > 1.0f;
    if (doSlide) {
      monoSynthTargetFreq0[mIdx] = base;
      monoSynthTargetFreq1[mIdx] = freq1;
      monoSynthGliding[mIdx] = true;
      monoSynthGlideLastMs[mIdx] = nowMs;
    } else {
      monoSynthCurrFreq0[mIdx] = monoSynthTargetFreq0[mIdx] = base;
      monoSynthCurrFreq1[mIdx] = monoSynthTargetFreq1[mIdx] = freq1;
      monoSynthGliding[mIdx] = false;
      monoSynthEnvStartMs[mIdx] = nowMs;
      monoSynthEnvActive[mIdx] = (SMP.synth_settings[ch][FILTER] > 0);
    }
  }

  {
    toern_audio::StateGuard guard;


    if (!legato) stopSynthChannel(ch);

    if (!doSlide) {
      synths[ch][0]->frequency(base);
      synths[ch][1]->frequency(freq1);
    }
    synths[ch][0]->amplitude(WaveFormVelocity);
    synths[ch][1]->amplitude(ch == 14 ? (WaveFormVelocity * 0.80f) : WaveFormVelocity);
    if ((ch == 13 || ch == 14) && !legato && filters[ch] &&
        SMP.synth_settings[ch][FILTER] > 0) {
      float lcut = SMP.filter_settings[ch][LOWCUT];
      float baseHz = (lcut > 0.0f)
          ? mapf(constrain(lcut, 1.0f, (float)maxfilterResolution), 1.0f,
                 (float)maxfilterResolution, 40.0f, 2500.0f)
          : mapf(SMP.filter_settings[ch][HCUT], 0, maxfilterResolution,
                 281.25f, 10000.0f);
      float baseRes = mapf(SMP.filter_settings[ch][RES], 0, maxfilterResolution,
                           0.7f, 5.0f);
      float envHz = baseHz, envRes = baseRes;
      bool envAct = false;
      computeMonoSynthFilterEnv(ch, nowMs, baseHz, baseRes, envHz, envRes, envAct);
      if (envAct) {
        filters[ch]->frequency(envHz);
        filters[ch]->resonance(envRes);
      }
    }
    if (!envelopes[ch]) return;



    if (persistant && (ch == 13 || ch == 14)) {
      envelopes[ch]->sustain(ch == 14 ? max(0.35f, mappedSustain) : maxParamVal[SUSTAIN]);
    } else if (ch == 13 || ch == 14) {
      envelopes[ch]->sustain(mappedSustain);
    }

    if (!legato) envelopes[ch]->noteOn();

    if (persistant) {
      persistentNoteOn[ch] = true;
    } else if (pressedKeyCount[ch] == 0) {

      persistentNoteOn[ch] = false;
    }

    startTime[ch] = nowMs;
    noteOnTriggered[ch] = true;
  }
}

void playSynth(int ch, int b, int vel, bool persistant) {
  if (ch == 13 || ch == 14) sequencedMonoSynthActive[ch] = false;
  playSynthInternal(ch, b, vel, persistant, false, -1);
}

void playSynthMidi(int ch, uint8_t midiPitch, int vel, bool persistant) {
  if (ch == 13 || ch == 14) sequencedMonoSynthActive[ch] = false;
  bool midiLegato = persistant && (ch == 13 || ch == 14) &&
                    pressedKeyCount[ch] > 1 && persistentNoteOn[ch];
  playSynthInternal(ch, 0, vel, persistant, midiLegato, (int)midiPitch);
}

void playSequencedSynth(int ch, int b, int vel, uint32_t tick,
                        bool allowLegato, int midiPitchOverride) {
  if (ch != 13 && ch != 14) return;
  if (pressedKeyCount[ch] > 0) return;
  const int pitchKey = (midiPitchOverride >= 0)
                         ? (128 + midiPitchOverride) : b;
  bool legato = allowLegato && sequencedMonoSynthActive[ch] &&
                 sequencedMonoSynthLastTick[ch] != 0 &&
                 tick == sequencedMonoSynthLastTick[ch] + 1 &&
                 !persistentNoteOn[ch];
  if (legato && sequencedMonoSynthLastPitch[ch] == pitchKey) {


    startTime[ch] = millis();
    sequencedMonoSynthLastTick[ch] = tick;
    sequencedMonoSynthActive[ch] = true;
    return;
  }
  playSynthInternal(ch, b, vel, false, legato, midiPitchOverride);
  sequencedMonoSynthLastTick[ch] = tick;
  sequencedMonoSynthLastPitch[ch] = (int16_t)pitchKey;
  sequencedMonoSynthActive[ch] = true;
}

void finishSequencedMonoSynthTick(uint32_t tick, bool had13, bool had14) {
  for (int ch = 13; ch <= 14; ch++) {
    bool hadNote = ch == 13 ? had13 : had14;
    if (hadNote || !sequencedMonoSynthActive[ch]) continue;
    if (sequencedMonoSynthLastTick[ch] != 0 &&
        tick == sequencedMonoSynthLastTick[ch] + 1 &&
        pressedKeyCount[ch] == 0) {
      stopSynthChannel(ch);
      sequencedMonoSynthActive[ch] = false;
      sequencedMonoSynthLastTick[ch] = 0;
      sequencedMonoSynthLastPitch[ch] = 0;
    }
  }
}




static inline bool noteHasMidiPitch(const Note &cell) {
  return cell.midiPitch <= 127;
}

static inline int midiPitchForOutput(const Note &cell, int row) {
  return noteHasMidiPitch(cell) ? (int)cell.midiPitch : 48 + row - 1;
}


static inline int sampleVoiceOctaveSemis(int ch) {
  if (ch < 1 || ch > 8) return 0;
  return (int)lroundf(channelOctave[ch]);
}

static inline int samplePitchForNote(const Note &cell, int channel, int row) {
  if (noteHasMidiPitch(cell)) {
    return (12 * SampleRate[channel]) + (int)cell.midiPitch - 72;
  }
  return (12 * SampleRate[channel]) + row - (channel + 1);
}

static inline int ch11PitchForNote(const Note &cell, int row) {
  int pitch = 12 * (int)octave[0] + transpose;
  pitch += noteHasMidiPitch(cell) ? ((int)cell.midiPitch - 60) : (row - 1);
  return pitch;
}

FLASHMEM static void triggerActiveFillHit();

void playNote() {


  beatForUI = beat;



  if (!isNowPlaying) {
    return;
  }
  MidiReleaseSequencerNotes();




  static unsigned int lastEdit = GLOB.edit;

  if (childLockEnabled) {
    GLOB.edit = 1;
    GLOB.page = 1;
    pendingPage = 0;
    if (beat < 1 || beat > maxX) beat = 1;
  } else if (SMP_PATTERN_MODE) {
    extern bool songModeActive;
    extern int patternMode;


    if (GLOB.edit != lastEdit && !songModeActive && patternMode != 3) {
      unsigned int oldStart = (lastEdit - 1) * maxX + 1;
      unsigned int newStart = (GLOB.edit - 1) * maxX + 1;
      unsigned int offset = beat - oldStart;


      if (beat != newStart) {
        beat = newStart + offset;
      }
      lastEdit = GLOB.edit;
    }
  }
  beatForUI = beat;
  sequencerPreviousRecordBeat =
      sequencerRecordStepStartedUs == 0 ? (uint16_t)beat
                                        : sequencerRecordBeat;
  sequencerRecordBeat = (uint16_t)beat;
  uint32_t recordStepNowUs = micros();
  uint32_t measuredStepDurationUs =
      recordStepNowUs - sequencerRecordStepStartedUs;
  if (sequencerRecordStepStartedUs != 0 &&
      measuredStepDurationUs >= 1000 &&
      measuredStepDurationUs <= 5000000) {
    sequencerRecordStepDurationUs = measuredStepDurationUs;
  } else {
    sequencerRecordStepDurationUs =
        (uint32_t)max(1.0, playNoteInterval);
  }
  sequencerRecordStepStartedUs = recordStepNowUs;
  serviceMidiArpOnPlayhead((uint16_t)beat);


  if (countInPending && isNowPlaying) {
    unsigned int currentBeatInBar = ((beat - 1) % maxX) + 1;


    if (currentBeatInBar == 1) {
      countInPending = false;
      countInActive = true;
      countInBeat = 0;
      countInComplete = false;
    }
  }


  if (countInActive && isNowPlaying) {
    unsigned int currentBeatInBar = ((beat - 1) % maxX) + 1;




    unsigned int beatInterval = maxX / 4;
    unsigned int countInNumber = ((currentBeatInBar - 1) / beatInterval) + 1;


    if (countInNumber <= 4) {
      countInBeat = countInNumber;


      if (countInNumber == 4) {
        countInComplete = true;
      }
    } else {

      countInBeat = 0;
    }


    if (currentBeatInBar == 1 && countInComplete) {

      countInActive = false;
      countInBeat = 0;
      countInComplete = false;

      recordingStartBeat = beat;
      recordingBeatCount = 0;

      recordingBorderArmed = true;
      pendingStartFastRecord = true;
    }
  }




  if (fastRecordActive && recChannelClear == 3 && recordingStartBeat > 0 && isNowPlaying) {
    unsigned int currentBeatInBar = ((beat - 1) % maxX) + 1;


    recordingBeatCount++;



    extern int loopLength;
    unsigned int loopEndBeat;
    if (loopLength > 0) {
      loopEndBeat = loopLength * maxX;
    } else {
      extern unsigned int lastPage;
      loopEndBeat = lastPage * maxX;
    }
# 8886 "/Users/jank/git/toern/toern.ino"
    if (currentBeatInBar == 1 && recordingBeatCount >= loopEndBeat + 1) {
      pendingStopFastRecord = true;
      recordingStartBeat = 0;
      recordingBeatCount = 0;
    }
  }

  if (currentMode == &draw || currentMode == &singleMode || currentMode == &velocity) {
  }


  isrPlayButtonTick = true;

  sequencerSynthTick++;
  if (sequencerSynthTick == 0) sequencerSynthTick = 1;
  bool sequencedCh11HadNotes = false;
  bool sequencedCh13HadNotes = false;
  bool sequencedCh14HadNotes = false;




  extern bool songModeActive;
  extern void voiceFillColumn(unsigned int col, unsigned int globalPage, unsigned int srcOut[16]);
  extern unsigned int voicePickStep(const unsigned int src[16], unsigned int row);
  unsigned int voiceSrc[16];
  const bool useVoiceRead = voiceMode && !songModeActive && beat >= 1 && maxX >= 1;
  if (useVoiceRead) {
    voiceFillColumn(((beat - 1) % maxX) + 1, ((beat - 1) / maxX) + 1, voiceSrc);
  }

  for (unsigned int b = 1; b < maxY + 1; b++) {
    unsigned int srcBeat = useVoiceRead ? voicePickStep(voiceSrc, b) : beat;
    if (srcBeat > 0 && srcBeat <= maxlen) {
      int ch = note[srcBeat][b].channel;
      int vel = note[srcBeat][b].velocity;
      uint8_t prob = note[srcBeat][b].probability;
      uint8_t cond = note[srcBeat][b].condition;
      if (cond == 0) cond = 1;

      if (consumeLiveRecordSkip(srcBeat, b)) {
        continue;
      }

      if (ch > 0 && !isChildVoiceDisabled(ch) && !getMuteState(ch)) {



        if (cond > 1 && cond <= 20) {
          bool shouldPlay = false;
          uint16_t condLoop = loopCount;
          if (voiceMode && ch > 0 && ch < 16) {
            extern bool songModeActive;
            if (!songModeActive) condLoop = voiceLoopCount[ch];
          }
          if (condLoop < 1) condLoop = 1;
          if (cond <= 16) {



            shouldPlay = (condLoop % cond) == 0;
          } else {





            uint8_t x = (cond == 17) ? 2 : (cond == 18) ? 4
                                         : (cond == 19) ? 8
                                                        : 16;
            shouldPlay = (condLoop % x) == 1;
          }

          if (!shouldPlay) {
            continue;
          }
        }


        if (prob < 100) {
          int randValue = random(100);
          if (randValue >= prob) {
            continue;
          }
        }


        if (cond == NOTE_CONDITION_FILL) {


          if (fillHasTriggered || fillRunning) {
            if (fillTriggerStep >= 1 && fillTriggerStep <= maxlen &&
                fillTriggerRow >= 1 && fillTriggerRow <= maxY) {
              const Note &trig = note[fillTriggerStep][fillTriggerRow];
              if (trig.channel == 0 || trig.condition != NOTE_CONDITION_FILL) {
                fillHasTriggered = false;
                fillRunning = false;
                fillTriggerStep = 0;
                fillTriggerRow = 0;
              }
            }
          }
          if (!fillHasTriggered && !fillRunning) {
            fillRunning = true;



            fillStartSubTick = fillSubTick & ~3u;
            fillTriggerStep = srcBeat;
            fillTriggerRow = b;
            fillActiveChannel = ch;
            fillActiveVelocity = (vel == 0) ? defaultVelocity : vel;
            fillActiveRow = b;
            fillActiveMidiPitch = note[srcBeat][b].midiPitch;

            triggerActiveFillHit();
          }
          continue;
        }







        onNoteTriggered(ch);

        MidiSendNoteOn(midiPitchForOutput(note[srcBeat][b], b), ch,
                       scaledNoteVelocity(ch, vel));
        if (shouldSkipVoiceTriggerForSyncPreview(ch)) {

          triggerSetWavSyncPreview((uint8_t)((vel == 0) ? defaultVelocity : vel));
        } else if (ch < 9) {

          if (isChannelSampleReloadBusy((unsigned int)ch)) {
            continue;
          }
          int pitch = samplePitchForNote(note[srcBeat][b], ch, b);

          pitch += sampleVoiceOctaveSemis(ch);
          triggerSamplerVoice(ch, pitch, vel, true);
        } else if (ch == 11) {


          if (pressedKeyCount[11] == 0) {
            playSequencedSound(ch11PitchForNote(note[srcBeat][b], b),
                               0, vel, sequencerSynthTick,
                               cond == NOTE_CONDITION_GLIDE);
            sequencedCh11HadNotes = true;
          }

        } else if (ch >= 13 && ch < 15) {
          if (pressedKeyCount[ch] == 0) {
            const int midiPitch = noteHasMidiPitch(note[srcBeat][b])
                                    ? (int)note[srcBeat][b].midiPitch : -1;
            playSequencedSynth(ch, b, vel, sequencerSynthTick,
                               cond == NOTE_CONDITION_GLIDE, midiPitch);
            if (ch == 13) sequencedCh13HadNotes = true;
            else sequencedCh14HadNotes = true;
          }
        }


      }
    }
  }
  finishSequencedSoundTick(sequencerSynthTick, sequencedCh11HadNotes);
  finishSequencedMonoSynthTick(sequencerSynthTick, sequencedCh13HadNotes,
                               sequencedCh14HadNotes);


  onBeatTick(beat, sequencerPreviousRecordBeat);


  if (waitForFourBars && pulseCount >= totalPulsesToWait) {
    extern int patternMode;
    extern bool songModeActive;
    if (voiceMode && !songModeActive && !childLockEnabled) {
      beat = 1;
      GLOB.page = 1;
      for (int c = 0; c < 16; c++) voiceLoopCount[c] = 1;
    } else if (SMP_PATTERN_MODE) {
      if (patternMode == 3) {

        GLOB.edit = constrain(
            (int)GLOB.edit, 1, effectivePageCount());
        GLOB.page = GLOB.edit;
        pendingPage = 0;
        beat = (GLOB.page - 1) * maxX + 1;
      } else {

        beat = (GLOB.edit - 1) * maxX + 1;
        GLOB.page = GLOB.edit;
      }
    } else {
      beat = 1;
      GLOB.page = 1;
    }
    if (fastRecordActive) stopFastRecord();
    enforceChildModeRestrictions();
    isNowPlaying = true;
    waitForFourBars = false;
  }

  extern bool songModeActive;
  if (voiceMode && !songModeActive && !childLockEnabled) {
    unsigned int prevBeat = beat;
    unsigned int pages = voiceModePages < 1 ? 1 : voiceModePages;
    unsigned int cycleEnd = pages * maxX;
    if (cycleEnd < 1) cycleEnd = maxX;
    beat++;
    if (beat > cycleEnd || beat < 1) beat = 1;
    unsigned int newPage = (beat > 0) ? ((beat - 1) / maxX + 1) : 1;
    unsigned int prevPage = (prevBeat > 0) ? ((prevBeat - 1) / maxX + 1) : newPage;
    GLOB.page = newPage;
    if (newPage != prevPage) {
      for (int c = 1; c < 16; c++) {
        uint8_t len = voicePageLen[c];
        if (len < 1) continue;
        unsigned int prevV = ((prevPage - 1) % len) + 1;
        unsigned int newV = ((newPage - 1) % len) + 1;
        if (prevV == len && newV == 1) {
          uint16_t n = voiceLoopCount[c] + 1;
          if (n > 256) n = 1;
          voiceLoopCount[c] = n;
        }
      }
    }
  } else if (SMP_PATTERN_MODE) {
    extern int patternMode;



    unsigned int currentPlayingPage;
    if (patternMode == 3) {

      currentPlayingPage = GLOB.page;
      if (currentPlayingPage == 0) {

        currentPlayingPage = GLOB.edit;
        GLOB.page = GLOB.edit;
      }
    } else {

      currentPlayingPage = GLOB.edit;
    }


    unsigned int pageStart = (currentPlayingPage - 1) * maxX + 1;
    unsigned int pageEnd = pageStart + maxX - 1;


    if (patternMode == 3 && GLOB.page == 0) {
      GLOB.page = GLOB.edit;
    }


    if (songModeActive) {
      lastEdit = GLOB.edit;
    }



    unsigned int previousPage = GLOB.page;
    unsigned int previousBeat = beat;

    beat++;

    if (songModeActive) {

      if (beat > pageEnd) {

        isrDbgPatternFinishedBeat = beat;
        isrDbgPatternFinished = true;
        checkPages();
      } else {

        GLOB.page = GLOB.edit;
      }
    } else if (patternMode == 3) {


      if (beat > pageEnd) {
        unsigned int targetPage = (unsigned int)constrain(
            (int)GLOB.edit, 1, effectivePageCount());
        GLOB.page = targetPage;
        beat = (targetPage - 1) * maxX + 1;
        pendingPage = 0;
      } else if (beat < pageStart) {
        beat = pageStart;
      }
      lastEdit = GLOB.edit;
    } else {

      if (beat < pageStart || beat > pageEnd) {
        beat = pageStart;
      }
    }


    if (patternMode != 3) {
      GLOB.page = GLOB.edit;
    }


    unsigned int loopStartPage = GLOB.page;
    unsigned int loopStartBeat = (loopStartPage - 1) * maxX + 1;
    bool pageChanged = (previousPage != GLOB.page);
    bool wrappedToLoopStart = (beat == loopStartBeat) && ((previousBeat != loopStartBeat) || pageChanged);

    if (pageChanged) {
      loopCount = 1;
      isrDbgLoopCountValue = (uint16_t)loopCount;
      isrDbgLoopCountChanged = true;
    } else if (wrappedToLoopStart) {
      loopCount++;
      if (loopCount > 256) loopCount = 1;
      isrDbgLoopCountValue = (uint16_t)loopCount;
      isrDbgLoopCountChanged = true;
    }
  } else {

    unsigned int previousPage = GLOB.page;
    unsigned int previousBeat = beat;
    beat++;
    checkPages();

    bool wrappedToPatternStart = (GLOB.page == 1 && beat == 1) && ((previousPage != 1) || (previousBeat != 1));
    if (wrappedToPatternStart) {
      loopCount++;
      if (loopCount > 256) loopCount = 1;
      isrDbgLoopCountValue = (uint16_t)loopCount;
      isrDbgLoopCountChanged = true;
    }
  }
  for (int ch = 13; ch <= 14; ch++) {
    if (noteOnTriggered[ch] && !persistentNoteOn[ch] &&
        !sequencedMonoSynthActive[ch]) {
      float noteLen = getNoteDuration(ch);
      if ((millis() - startTime[ch] >= noteLen)) {
        if (!envelopes[ch]) continue;
        envelopes[ch]->noteOff();
        noteOnTriggered[ch] = false;
      }
    }
  }
}

void checkPages() {
  extern bool songModeActive;
  extern int currentSongPosition;

  if (childLockEnabled) {
    GLOB.edit = 1;
    GLOB.page = 1;
    pendingPage = 0;
    if (beat < 1 || beat > maxX) beat = 1;
    return;
  }


  if (songModeActive) {



    if (currentSongPosition < 0) {
      currentSongPosition = 0;
      for (int i = 0; i < 64; i++) {
        if (SMP.songArrangement[i] > 0) {
          currentSongPosition = i;
          break;
        }
      }
    }


    int startPos = currentSongPosition;
    int attempts = 0;
    do {
      currentSongPosition++;
      if (currentSongPosition >= 64) {
        currentSongPosition = 0;
      }
      attempts++;

      if (attempts >= 64 || currentSongPosition == startPos) {
        break;
      }
    } while (SMP.songArrangement[currentSongPosition] == 0);


    int pattern = SMP.songArrangement[currentSongPosition];
    if (pattern > 0 && pattern <= 16) {
      GLOB.edit = pattern;
      GLOB.page = pattern;
      beat = (pattern - 1) * maxX + 1;

    } else {

      songModeActive = false;
      patternMode = 1;
      SMP_PATTERN_MODE = true;
      beat = (GLOB.edit - 1) * maxX + 1;
      GLOB.page = GLOB.edit;
    }

    return;
  }

  if (SMP_PATTERN_MODE) {

    GLOB.page = GLOB.edit;
    return;
  }

  uint16_t newPage = (beat - 1) / maxX + 1;




  if (lastPage > 0 && newPage > lastPage) {
    beat = 1;

    newPage = 1;
  }
  GLOB.page = newPage;
}

unsigned int cursorNoteStep();

void unpaint() {
  paintMode = false;
  preventPaintUnpaint = false;



  unsigned int current_x = cursorNoteStep();
  unsigned int current_y = GLOB.y;

  if ((current_y > 0 && current_y <= maxY)) {



    if (current_y < 16) {
      if (!GLOB.singleMode) {
        if (simpleNotesView == 1 && !(voiceMode && isNowPlaying && !songModeActive)) {

          int voiceToUnpaint = current_y - 1;
          if (voiceToUnpaint >= 0 && voiceToUnpaint < maxY) {

            for (unsigned int y = 1; y <= maxY; y++) {
              if (note[current_x][y].channel == voiceToUnpaint) {
                note[current_x][y].channel = 0;
                note[current_x][y].velocity = defaultVelocity;
                note[current_x][y].probability = 100;
                note[current_x][y].condition = 1;
                note[current_x][y].midiPitch = NOTE_MIDI_PITCH_NONE;
              }
            }
          }
        } else {

          note[current_x][current_y].channel = 0;
          note[current_x][current_y].velocity = defaultVelocity;
          note[current_x][current_y].probability = 100;
          note[current_x][current_y].condition = 1;
          note[current_x][current_y].midiPitch = NOTE_MIDI_PITCH_NONE;
        }
      } else {
        if (note[current_x][current_y].channel == GLOB.currentChannel) {
          note[current_x][current_y].channel = 0;
          note[current_x][current_y].velocity = defaultVelocity;
          note[current_x][current_y].probability = 100;
          note[current_x][current_y].condition = 1;
          note[current_x][current_y].midiPitch = NOTE_MIDI_PITCH_NONE;
        }
      }
    } else if (current_y == 16) {
      clearPage();
    }
  }
  updateLastPage();


  if (currentMode == &draw || currentMode == &singleMode) {
    if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode)) {
      Encoder[1].writeMax((int32_t)encoderPageMax());
    } else if (ctrlMode == 1) {
      refreshCtrlEncoderConfig();
    }
  }


}


FLASHMEM static void triggerActiveFillHit() {
  const int ch = fillActiveChannel;
  if (ch <= 0) return;
  if (isChildVoiceDisabled(ch)) return;
  if (getMuteState(ch)) return;

  const int vel = (fillActiveVelocity == 0) ? defaultVelocity : fillActiveVelocity;
  const unsigned int row = fillActiveRow;
  Note fillCell = {};
  fillCell.channel = (uint8_t)ch;
  fillCell.velocity = (uint8_t)vel;
  fillCell.probability = 100;
  fillCell.condition = NOTE_CONDITION_FILL;
  fillCell.midiPitch = fillActiveMidiPitch;

  if (shouldSkipVoiceTriggerForSyncPreview(ch)) {
    triggerSetWavSyncPreview((uint8_t)vel);
    return;
  }

  if (ch < 9) {
    if (isChannelSampleReloadBusy((unsigned int)ch)) return;
    int pitch = samplePitchForNote(fillCell, ch, (int)row);
    pitch += sampleVoiceOctaveSemis(ch);
    triggerSamplerVoice(ch, pitch, vel, true);
  } else if (ch == 11) {
    playSound(ch11PitchForNote(fillCell, (int)row), 0, vel);
  } else if (ch >= 13 && ch < 15) {
    if (noteHasMidiPitch(fillCell)) {
      playSynthMidi(ch, fillCell.midiPitch, vel, false);
    } else {
      playSynth(ch, (int)row, vel, false);
    }
  }
}


void playFillNote() {
  if (!isNowPlaying) return;



  if (fillHasTriggered) {
    if (fillTriggerStep >= 1 && fillTriggerStep <= maxlen &&
        fillTriggerRow >= 1 && fillTriggerRow <= maxY) {
      const Note &trig = note[fillTriggerStep][fillTriggerRow];
      if (trig.channel == 0 || trig.condition != NOTE_CONDITION_FILL) {
        fillHasTriggered = false;
        fillTriggerStep = 0;
        fillTriggerRow = 0;
      } else {
        return;
      }
    } else {
      return;
    }
  }


  fillSubTick++;
  fillSubBeat = (unsigned int)(fillSubTick & 0x3U);


  if (!fillRunning) return;
  if (fillActiveChannel <= 0) return;


  if (fillTriggerStep >= 1 && fillTriggerStep <= maxlen &&
      fillTriggerRow >= 1 && fillTriggerRow <= maxY) {
    const Note &trig = note[fillTriggerStep][fillTriggerRow];
    if (trig.channel == 0 || trig.condition != NOTE_CONDITION_FILL) {
      fillRunning = false;
      fillHasTriggered = false;
      fillTriggerStep = 0;
      fillTriggerRow = 0;
      return;
    }
  }

  if (getMuteState(fillActiveChannel)) return;


  const uint32_t fillLengthBeats = (uint32_t)(2U * (uint32_t)maxX);




  const uint32_t elapsedSub = (fillSubTick >= fillStartSubTick) ? (fillSubTick - fillStartSubTick) : 0U;
  const uint32_t positionInFill = (elapsedSub / 4U) + 1U;

  if (positionInFill > fillLengthBeats) {


    if (fillTriggerStep >= 1 && fillTriggerStep <= maxlen &&
        fillTriggerRow >= 1 && fillTriggerRow <= maxY) {
      Note &trig = note[fillTriggerStep][fillTriggerRow];
      if (trig.channel != 0 && trig.condition == NOTE_CONDITION_FILL) {
        trig.condition = 1;
      }
    }
    fillRunning = false;
    fillHasTriggered = false;
    fillTriggerStep = 0;
    fillTriggerRow = 0;
    return;
  }


  if (elapsedSub < 4u && fillSubBeat == 0) return;


  const uint32_t sectionSize = (fillLengthBeats + 4U) / 5U;
  uint32_t section = (positionInFill - 1U) / sectionSize;
  if (section > 4U) section = 4U;

  bool shouldTrigger = false;
  switch (section) {
    case 0:
      shouldTrigger = (fillSubBeat == 0 && ((positionInFill - 1U) % 8U == 0U));
      break;
    case 1:
      shouldTrigger = (fillSubBeat == 0 && ((positionInFill - 1U) % 4U == 0U));
      break;
    case 2:
      shouldTrigger = (fillSubBeat == 0 && ((positionInFill - 1U) % 2U == 0U));
      break;
    case 3:
      shouldTrigger = (fillSubBeat == 0);
      break;
    case 4:
      shouldTrigger = (fillSubBeat == 0 || fillSubBeat == 2);
      break;
  }

  if (!shouldTrigger) return;
  triggerActiveFillHit();
}



static void triggerCurrentVoiceAudition(int pitchRow, int auditionVelocity) {
  const int ch = (int)GLOB.currentChannel;
  if (!isSingleModeChannelAllowed(ch) || isChildVoiceDisabled(ch)) return;

  const int row = (pitchRow >= 1) ? pitchRow : basePitchRowForChannel(ch);
  if (row < 1) return;

  const int velocity = auditionVelocity < 0 ? defaultVelocity : constrain(auditionVelocity, 1, 127);
  Note cell = {};
  cell.channel = (uint8_t)ch;
  cell.velocity = (uint8_t)velocity;
  cell.probability = 100;
  cell.condition = 1;
  cell.midiPitch = NOTE_MIDI_PITCH_NONE;

  onNoteTriggered(ch);

  if (ch >= 1 && ch <= 8) {
    if (isChannelSampleReloadBusy((unsigned int)ch)) return;
    int pitch = samplePitchForNote(cell, ch, row);
    pitch += sampleVoiceOctaveSemis(ch);
    triggerSamplerVoice(ch, pitch, velocity, true);
  } else if (ch == 11) {
    playSound(ch11PitchForNote(cell, row), 0, velocity);
  } else if (ch == 13 || ch == 14) {
    playSynth(ch, row, velocity, false);
  }
}

void triggerGridNote(unsigned int globalX, unsigned int y, bool allowMuted) {
  if (globalX < 1 || globalX > maxlen || y < 1 || y > maxY) return;

  Note &cell = note[globalX][y];
  int channel = cell.channel;
  if (channel == 0) return;
  if (isChildVoiceDisabled(channel)) return;



  if (!allowMuted && getMuteState(channel)) return;


  onNoteTriggered(channel);

  int velocity = cell.velocity > 0 ? cell.velocity : defaultVelocity;
  int pitch_from_row = y;

  if (shouldSkipVoiceTriggerForSyncPreview(channel)) {
    triggerSetWavSyncPreview((uint8_t)velocity);
    return;
  }

  if (channel > 0 && channel < 9) {
    if (isChannelSampleReloadBusy((unsigned int)channel)) return;
    int pitch = samplePitchForNote(cell, channel, pitch_from_row);
    pitch += sampleVoiceOctaveSemis(channel);
    triggerSamplerVoice(channel, pitch, velocity, true);
  } else if (channel == 11) {
    playSound(ch11PitchForNote(cell, pitch_from_row), 0, velocity);
  } else if (channel >= 13 && channel < 15) {
    if (noteHasMidiPitch(cell)) {
      playSynthMidi(channel, cell.midiPitch, velocity, false);
    } else {
      playSynth(channel, pitch_from_row, velocity, false);
    }
  }
}

void paint() {
  preventPaintUnpaint = false;

  unsigned int current_x = cursorNoteStep();
  unsigned int current_y = GLOB.y;

  bool channelBlocked = (GLOB.currentChannel == 9 || GLOB.currentChannel == 10 || GLOB.currentChannel == 12 || isChildVoiceDisabled((int)GLOB.currentChannel));

  if (!GLOB.singleMode) {
    if (isPaintableDrawRow(current_y)) {
      uint8_t existingCh = note[current_x][current_y].channel;
      if (imageMode) {
        int ch = imageModeResolvePaintChannel(existingCh);
        if (!isChildVoiceDisabled(ch)) {
          if (existingCh == 0) {
            note[current_x][current_y].probability = 100;
            note[current_x][current_y].condition = 1;
            note[current_x][current_y].midiPitch = NOTE_MIDI_PITCH_NONE;
          }
          note[current_x][current_y].channel = (uint8_t)ch;
          note[current_x][current_y].velocity = defaultVelocity;
          GLOB.currentChannel = ch;
        }
      } else if (!channelBlocked && existingCh == 0) {
        note[current_x][current_y].channel = GLOB.currentChannel;
        note[current_x][current_y].velocity = defaultVelocity;
        note[current_x][current_y].probability = 100;
        note[current_x][current_y].condition = 1;
        note[current_x][current_y].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    } else if (current_y == 16) {
      toggleCopyPaste();
    }
  } else {
    if ((current_y > 0 && current_y <= 15)) {
      uint8_t existingCh = note[current_x][current_y].channel;
      if (imageMode) {
        int ch = imageModeResolvePaintChannel(existingCh);
        if (!isChildVoiceDisabled(ch)) {
          if (existingCh == 0) {
            note[current_x][current_y].probability = 100;
            note[current_x][current_y].condition = 1;
            note[current_x][current_y].midiPitch = NOTE_MIDI_PITCH_NONE;
          }
          note[current_x][current_y].channel = (uint8_t)ch;
          note[current_x][current_y].velocity = defaultVelocity;
          GLOB.currentChannel = ch;
        }
      } else if (!channelBlocked && (existingCh == 0 || existingCh == GLOB.currentChannel)) {
        note[current_x][current_y].channel = GLOB.currentChannel;
        note[current_x][current_y].velocity = defaultVelocity;
        note[current_x][current_y].probability = 100;
        note[current_x][current_y].condition = 1;
        note[current_x][current_y].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    } else if (current_y == 16) {
      toggleCopyPaste();
    }
  }


  if (note[current_x][current_y].channel > maxY - 2) {
    note[current_x][current_y].channel = 0;
    note[current_x][current_y].midiPitch = NOTE_MIDI_PITCH_NONE;

    for (unsigned int vx = 1; vx < maxX + 1; vx++) {
      light(vx, note[current_x][current_y].channel + 1, col[note[current_x][current_y].channel] * 12);
    }
    FastLEDshow();
  }


  if (!isNowPlaying && !preventPaintUnpaint) {
    int painted_channel = note[current_x][current_y].channel;
    int painted_velocity = note[current_x][current_y].velocity;
    int pitch_from_row = current_y;
    Note &paintedCell = note[current_x][current_y];

    if (isChildVoiceDisabled(painted_channel)) {

    } else if (painted_channel > 0 && painted_channel < 9) {

      int pitch = samplePitchForNote(paintedCell, painted_channel, pitch_from_row);
      pitch += sampleVoiceOctaveSemis(painted_channel);
      triggerSamplerVoice(painted_channel, pitch, painted_velocity, true);
    } else if (painted_channel == 11) {


      int noteValue = ch11PitchForNote(paintedCell, pitch_from_row);
      if (!noteHasMidiPitch(paintedCell)) {
        noteValue = constrain(noteValue, 0, 107);
      }
      playSound(noteValue, 0, painted_velocity);
    } else if (painted_channel >= 13 && painted_channel < 15) {
      if (noteHasMidiPitch(paintedCell)) {
        playSynthMidi(painted_channel, paintedCell.midiPitch, painted_velocity, false);
      } else {
        playSynth(painted_channel, pitch_from_row, painted_velocity, false);
      }
    }
  }

  updateLastPage();


  if (currentMode == &draw || currentMode == &singleMode) {
    if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode)) {
      Encoder[1].writeMax((int32_t)encoderPageMax());
    } else if (ctrlMode == 1) {
      refreshCtrlEncoderConfig();
    }
  }


}

void toggleCopyPaste() {
  if (!GLOB.activeCopy) {


    unsigned int src = 0;
    for (unsigned int c = ((GLOB.edit - 1) * maxX) + 1; c < ((GLOB.edit - 1) * maxX) + (maxX + 1); c++) {
      src++;
      for (unsigned int y = 1; y < maxY + 1; y++) {
        if (GLOB.singleMode) {

          if (note[c][y].channel == GLOB.currentChannel) {
            tmp[src][y] = note[c][y];
          } else {

            tmp[src][y].channel = 0;
            tmp[src][y].velocity = defaultVelocity;
            tmp[src][y].probability = 100;
            tmp[src][y].condition = 1;
            tmp[src][y].midiPitch = NOTE_MIDI_PITCH_NONE;
          }
        } else {

          tmp[src][y] = note[c][y];
        }
      }
    }

    GLOB.copyChannel = GLOB.currentChannel;
  } else {


    unsigned int src = 0;
    for (unsigned int c = ((GLOB.edit - 1) * maxX) + 1; c < ((GLOB.edit - 1) * maxX) + (maxX + 1); c++) {
      src++;
      for (unsigned int y = 1; y < maxY + 1; y++) {
        if (GLOB.singleMode) {

          if (note[c][y].channel == GLOB.currentChannel) {
            note[c][y].channel = 0;
            note[c][y].velocity = defaultVelocity;
            note[c][y].probability = 100;
            note[c][y].condition = 1;
            note[c][y].midiPitch = NOTE_MIDI_PITCH_NONE;
          }

          if (tmp[src][y].channel == GLOB.copyChannel) {
            note[c][y].channel = GLOB.currentChannel;
            note[c][y].velocity = tmp[src][y].velocity;
            note[c][y].probability = tmp[src][y].probability;
            note[c][y].condition = tmp[src][y].condition;
            note[c][y].midiPitch = tmp[src][y].midiPitch;
          }

        } else {

          note[c][y] = tmp[src][y];
        }
      }
    }
  }
  updateLastPage();


  if (currentMode == &draw || currentMode == &singleMode) {
    if (ctrlMode == 0 && (SMP_PATTERN_MODE || GLOB.singleMode)) {
      Encoder[1].writeMax((int32_t)encoderPageMax());
    } else if (ctrlMode == 1) {
      refreshCtrlEncoderConfig();
    }
  }

  GLOB.activeCopy = !GLOB.activeCopy;
}

void clearNoteChannel(unsigned int c, unsigned int yStart, unsigned int yEnd, unsigned int channel_to_clear, bool singleModeActive) {



  for (unsigned int y = yStart; y <= yEnd; y++) {
    if (c > 0 && c <= maxlen && y > 0 && y <= maxY) {
      if (singleModeActive) {
        if (note[c][y].channel == channel_to_clear) {
          note[c][y].channel = 0;
          note[c][y].velocity = defaultVelocity;
          note[c][y].probability = 100;
          note[c][y].condition = 1;
          note[c][y].midiPitch = NOTE_MIDI_PITCH_NONE;
        }
      } else {
        note[c][y].channel = 0;
        note[c][y].velocity = defaultVelocity;
        note[c][y].probability = 100;
        note[c][y].condition = 1;
        note[c][y].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    }
  }
  if (channel_to_clear == 13 || channel_to_clear == 14) {
    stopSynthChannel(channel_to_clear);
    pressedKeyCount[channel_to_clear] = 0;
  }
}

FLASHMEM void updatePreviewVolume() {
  unsigned int level = constrain(previewVol, 0u, 50u);

  float gain = (float)level * 0.01f;





  mixer0.gain(0, gain * 1.25f);
  mixer0.gain(1, gain);
  ampPreview.gain(1.0f);
}

void updateBrightness() {



  if (currentMode != &bpmMode) return;



  uint8_t newBrightness = constrain(currentMode->pos[1] + 3, 3, 255);
  if (newBrightness != ledBrightness) {
    ledBrightness = newBrightness;

    saveSingleModeToEEPROM(26, (int8_t)ledBrightness);
  }

}

FLASHMEM void switchSubPattern() {
  FastLEDclear();


  if (!muteModeActive) {
    FastLEDshow();
    return;
  }

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

  if (muteModeArrowDirection != 0 && millis() <= muteModeArrowUntil) {
    const char *arrow = (muteModeArrowDirection > 0) ? ">>" : "<<";
    drawText(arrow, 7, 8, CRGB(255, 255, 255));
  } else if (soloRandomArrowDirection != 0 && millis() <= soloRandomArrowUntil) {

    const char *rnd = (soloRandomArrowDirection > 0) ? "?>" : "?<";
    drawText(rnd, 7, 8, CRGB(255, 0, 255));
  } else {
    if (muteModeArrowDirection != 0 && millis() > muteModeArrowUntil) {
      muteModeArrowDirection = 0;
      muteModeArrowUntil = 0;
      muteModeEncoderValue = 0;
      currentMode->pos[0] = 0;
      Encoder[0].writeCounter((int32_t)0);
    }
    if (soloRandomArrowDirection != 0 && millis() > soloRandomArrowUntil) {
      soloRandomArrowDirection = 0;
      soloRandomArrowUntil = 0;
    }
  }

  drawIndicator('L', 'W', 0);
  extern int drawMode;


  static uint8_t soloRingCh = 255;
  static int8_t soloRingDrawMode = -1;
  const uint8_t chNow = (uint8_t)GLOB.currentChannel;
  const bool ringsDirty = muteModeRingsDirty || (soloRingCh != chNow) || (soloRingDrawMode != (int8_t)drawMode);
  if (ringsDirty) {
    muteModeRingsDirty = false;
    soloRingCh = chNow;
    soloRingDrawMode = (int8_t)drawMode;
    if (drawMode == 0) {
      Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
    } else {
      Encoder[0].writeRGBCode(0x000000);
    }
    CRGB whiteColor = getIndicatorColor('W');
    Encoder[0].writeRGBCode(whiteColor.r << 16 | whiteColor.g << 8 | whiteColor.b);
    Encoder[1].writeRGBCode(0x000000);
    Encoder[2].writeRGBCode(0x00FF00);
    Encoder[3].writeRGBCode(0xFF00FF);
  }
  drawIndicator('L', 'G', 3, false, false);
  drawIndicator('L', 'P', 4, false, false);


  if (filterDrawActive) {
    if (millis() <= filterDrawEndTime) {
      drawFilterCheck(filterDrawValue, (FilterType)0, filterDrawColor);
    } else {
      filterDrawActive = false;
    }
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
  toern_audio::StateGuard guard;
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
  extern int clockMode;
  if (clockMode == 1) {
    SMP.bpm = currentMode->pos[3];
    if (SMP.bpm > 0) {
      playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
      playTimer.update((uint32_t)round(playNoteInterval));

      if (playNoteInterval >= 4) {
        fillTimer.update(playNoteInterval / 4);
      }



      extern void updateMidiClockOutput();
      updateMidiClockOutput();
    }
  }
  drawBPMScreen();
}

FLASHMEM void setBPM() {

  drawBPMScreen();


  static unsigned int lastPos1 = 0;
  static int lastPos2 = -1;
  static Mode *lastMode = nullptr;

  extern int clockMode;


  if (lastMode != currentMode) {
    lastPos1 = 0;
    lastPos2 = -1;
    lastMode = currentMode;
  }


  if (lastPos2 == -1) {

    currentMode->pos[2] = (clockMode == 1) ? 1 : 0;
    Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
    lastPos2 = currentMode->pos[2];
    lastPos1 = currentMode->pos[1];
  }


  if (currentMode->pos[1] != lastPos1) {
    updateBrightness();
    lastPos1 = currentMode->pos[1];
  }


  if (currentMode->pos[2] != lastPos2) {

    int newPos2 = constrain(currentMode->pos[2], 0, 1);
    if (newPos2 != currentMode->pos[2]) {
      currentMode->pos[2] = newPos2;
      Encoder[2].writeCounter((int32_t)newPos2);
    }


    clockMode = (newPos2 == 1) ? 1 : -1;


    extern void saveSingleModeToEEPROM(int index, int8_t value);
    saveSingleModeToEEPROM(1, clockMode);


    extern bool MIDI_CLOCK_SEND;
    extern bool MIDI_NOTE_SEND;
    extern int midiSendMode;
    if (midiSendMode == 0) {

      MIDI_CLOCK_SEND = (clockMode == 1);
      MIDI_NOTE_SEND = false;
    } else if (midiSendMode == 1) {

      MIDI_CLOCK_SEND = false;
      MIDI_NOTE_SEND = true;
    } else {

      MIDI_CLOCK_SEND = (clockMode == 1);
      MIDI_NOTE_SEND = true;
    }


    extern void resetMidiClockState();
    resetMidiClockState();


    drawBPMScreen();

    lastPos2 = newPos2;
  }

  if (currentMode->pos[3] != (unsigned int)SMP.bpm) {
    updateBPM();
  }
}

FLASHMEM void showLoadSave() {

  drawNoSD();


  unsigned int fileUi = currentMode->pos[3];
  if (fileUi > 1000u) {
    fileUi = 1000u;
    currentMode->pos[3] = fileUi;
    Encoder[3].writeCounter((int32_t)fileUi);
  }
  const bool isNewSlot = (fileUi == 0);
  const bool isAutosaveSlot = (fileUi == 1);
  if (!isNewSlot) {
    unsigned int fid = isAutosaveSlot ? 0u : (fileUi - 1u);
    if (SMP.file != fid) SMP.file = fid;
  }

  char OUTPUTf[50];
  if (isAutosaveSlot) {
    sprintf(OUTPUTf, "autosaved.txt");
  } else if (!isNewSlot) {
    sprintf(OUTPUTf, "%u.txt", SMP.file);
  } else {
    OUTPUTf[0] = 0;
  }



  static int lastUi = -2;
  static int8_t lastExists = -1;
  static int8_t lastLoadSettings = -1;
  static uint32_t lastFileRGB[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
  static uint32_t lastCallMs = 0;
  uint32_t nowMs = millis();
  if (nowMs - lastCallMs > 50) {
    lastUi = -2;
    lastExists = -1;
    lastLoadSettings = -1;
    for (int i = 0; i < 4; i++) lastFileRGB[i] = 0xFFFFFFFFu;
  }
  lastCallMs = nowMs;

  bool fileChanged = ((int)fileUi != lastUi);
  bool txtExists;
  if (isNewSlot) {
    txtExists = true;
    if (fileChanged) {
      lastUi = (int)fileUi;
      lastExists = 1;
    }
  } else if (fileChanged) {
    txtExists = SD.exists(OUTPUTf);
    lastUi = (int)fileUi;
    lastExists = txtExists ? 1 : 0;
  } else {
    txtExists = (lastExists == 1);
  }


  if (txtExists && !isNewSlot) {
    if (currentMode->pos[2] == 1) {
      SMP_LOAD_SETTINGS = true;
    } else {
      SMP_LOAD_SETTINGS = false;
    }
  } else {
    SMP_LOAD_SETTINGS = false;
    if (currentMode->pos[2] != 0) {
      currentMode->pos[2] = 0;
      Encoder[2].writeCounter((int32_t)0);
    }
  }

  const int8_t loadSettingsDrawn = SMP_LOAD_SETTINGS ? 1 : 0;
  if (!fileChanged && lastExists == (txtExists ? 1 : 0) && lastLoadSettings == loadSettingsDrawn) {
    return;
  }
  lastLoadSettings = loadSettingsDrawn;
  lastExists = txtExists ? 1 : 0;

  FastLEDclear();

  CRGB fileIconColor = isNewSlot ? UI_BLUE : (txtExists ? UI_GREEN : UI_DIM_RED);
  showIconsAt(ICON_FOLDER_BIG, fileIconColor, 2, 7);
  if (isNewSlot) {

    drawText("+", 4, 6, UI_WHITE);
  }



  auto setFileRGB = [&](int enc, uint32_t rgb) {
    if (enc < 0 || enc > 3) return;
    if (lastFileRGB[enc] == rgb) return;
    lastFileRGB[enc] = rgb;
    Encoder[enc].writeRGBCode(rgb);
  };
  auto matrixInd = [&](char size, char colorCode, int encoderNum) {
    drawIndicator(size, colorCode, encoderNum, false, false);
    CRGB c = getIndicatorColor(colorCode);
    if (colorCode == 'C') c = getCurrentChannelColor();
    CRGB maxB = normalizeToMaxBrightness(c);
    setFileRGB(encoderNum - 1, (uint32_t)maxB.r << 16 | (uint32_t)maxB.g << 8 | maxB.b);
  };

  matrixInd('L', 'X', 4);

  if (isNewSlot) {

    matrixInd('M', 'G', 1);
    setFileRGB(1, 0x000000);
    setFileRGB(2, 0x000000);
  } else if (isAutosaveSlot) {

    matrixInd('M', txtExists ? 'G' : 'E', 1);
    setFileRGB(1, 0x000000);
    if (SMP_LOAD_SETTINGS && txtExists) {
      matrixInd('C', 'W', 3);
    } else {
      setFileRGB(2, 0x000000);
    }
    drawText("A", 11, 11, txtExists ? UI_BRIGHT_GREEN : UI_BLUE);
  } else if (txtExists) {
    matrixInd('M', 'G', 1);
    matrixInd('M', 'D', 2);
    if (SMP_LOAD_SETTINGS) {
      matrixInd('C', 'W', 3);
    } else {
      setFileRGB(2, 0x000000);
    }
    drawNumber(SMP.file, UI_BRIGHT_GREEN, 11);
  } else {
    matrixInd('M', 'E', 1);
    matrixInd('M', 'R', 2);
    setFileRGB(2, 0x000000);
    drawNumber(SMP.file, UI_BLUE, 11);
  }
  FastLEDshow();
}

FLASHMEM void showPresetLoadSave() {
  drawNoSD();

  int ch = (int)GLOB.currentChannel;
  if (ch != 11 && ch != 13 && ch != 14) {
    switchMode(GLOB.singleMode ? &singleMode : &draw);
    return;
  }

  uint16_t slot = (uint16_t)constrain((int)currentMode->pos[3], 1, 99);
  if ((unsigned int)slot != currentMode->pos[3]) {
    currentMode->pos[3] = slot;
    Encoder[3].writeCounter((int32_t)slot);
  }
  setVoicePresetSlot(ch, slot);

  static int lastUiCh = -1;
  static int lastUiSlot = -2;
  static int8_t lastExists = -1;
  static uint32_t lastPsetRGB[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
  static uint32_t lastCallMs = 0;
  uint32_t nowMs = millis();
  if (nowMs - lastCallMs > 50) {
    lastUiCh = -1;
    lastUiSlot = -2;
    lastExists = -1;
    for (int i = 0; i < 4; i++) lastPsetRGB[i] = 0xFFFFFFFFu;
  }
  lastCallMs = nowMs;

  if (takeVoicePresetUiDirty()) lastUiSlot = -2;
  bool slotChanged = (ch != lastUiCh) || ((int)slot != lastUiSlot);
  bool exists;
  if (slotChanged) {
    exists = voicePresetExists(ch, slot);
    lastUiCh = ch;
    lastUiSlot = (int)slot;
    lastExists = exists ? 1 : 0;
  } else {
    exists = (lastExists == 1);
    return;
  }

  FastLEDclear();

  CRGB fileIconColor = exists ? UI_GREEN : UI_DIM_RED;
  showIconsAt(ICON_FOLDER_BIG, fileIconColor, 2, 7);

  auto setPsetRGB = [&](int enc, uint32_t rgb) {
    if (enc < 0 || enc > 3) return;
    if (lastPsetRGB[enc] == rgb) return;
    lastPsetRGB[enc] = rgb;
    Encoder[enc].writeRGBCode(rgb);
  };
  auto matrixInd = [&](char size, char colorCode, int encoderNum) {
    drawIndicator(size, colorCode, encoderNum, false, false);
    CRGB c = getIndicatorColor(colorCode);
    CRGB maxB = normalizeToMaxBrightness(c);
    setPsetRGB(encoderNum - 1, (uint32_t)maxB.r << 16 | (uint32_t)maxB.g << 8 | maxB.b);
  };

  matrixInd('L', 'X', 4);
  if (exists) {
    matrixInd('M', 'G', 1);
    matrixInd('M', 'D', 2);
    drawNumber(slot, UI_BRIGHT_GREEN, 11);
  } else {
    matrixInd('M', 'E', 1);
    matrixInd('M', 'R', 2);
    drawNumber(slot, UI_BLUE, 11);
  }
  setPsetRGB(2, 0x000000);
  FastLEDshow();
}

FLASHMEM void showSamplePack() {
  drawNoSD();

  if (currentMode->pos[3] != SMP.pack) {
    SMP.pack = currentMode->pos[3];
  }


  if (SMP.pack > 999) {
    SMP.pack = 0;
    currentMode->pos[3] = 0;
    Encoder[3].writeCounter((int32_t)0);
    EEPROM.put(0, SMP.pack);
    markSettingsBackupDirty();
  }



  static int lastPack = -2;
  static uint32_t lastPackRGB[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
  static uint32_t lastCallMs = 0;
  uint32_t nowMs = millis();
  if (nowMs - lastCallMs > 50) {
    lastPack = -2;
    for (int i = 0; i < 4; i++) lastPackRGB[i] = 0xFFFFFFFFu;
  }
  lastCallMs = nowMs;

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%u/%u.wav", SMP.pack, 1);
  bool packChanged = ((int)SMP.pack != lastPack);
  bool wavExists;
  if (packChanged) {
    wavExists = SD.exists(OUTPUTf);
    lastPack = (int)SMP.pack;
  } else {
    return;
  }

  FastLEDclear();



  CRGB packIconColor = wavExists ? UI_GREEN : UI_DIM_RED;
  showIconsAt(OLD_ICON_SAMPLEPACK, packIconColor, 2, 7);


  auto setPackRGB = [&](int enc, uint32_t rgb) {
    if (enc < 0 || enc > 3) return;
    if (lastPackRGB[enc] == rgb) return;
    lastPackRGB[enc] = rgb;
    Encoder[enc].writeRGBCode(rgb);
  };
  auto matrixInd = [&](char size, char colorCode, int encoderNum) {
    drawIndicator(size, colorCode, encoderNum, false, false);
    CRGB c = getIndicatorColor(colorCode);
    CRGB maxB = normalizeToMaxBrightness(c);
    setPackRGB(encoderNum - 1, (uint32_t)maxB.r << 16 | (uint32_t)maxB.g << 8 | maxB.b);
  };

  matrixInd('L', 'X', 4);

  if (wavExists) {
    matrixInd('M', 'G', 1);
    matrixInd('M', 'D', 2);
    drawNumber(SMP.pack, UI_BRIGHT_GREEN, 11);
  } else {
    matrixInd('M', 'E', 1);
    matrixInd('M', 'R', 2);
    drawNumber(SMP.pack, UI_BLUE, 11);
  }
  setPackRGB(2, 0x000000);


  FastLEDshow();
}

FLASHMEM void loadSamplePack(unsigned int pack_id, bool intro, bool preserveSp0Custom) {
  drawNoSD();


  if (pack_id > 999) {
    pack_id = 0;
  }

  samplePackID = pack_id;
  SMP.pack = pack_id;
  EEPROM.put(0, pack_id);
  markSettingsBackupDirty();



  unsigned int savedSeek = GLOB.seek;
  unsigned int savedSeekEnd = GLOB.seekEnd;
  GLOB.seek = 0;
  GLOB.seekEnd = 100;

  if (!preserveSp0Custom) {

    for (unsigned int z = 1; z < maxFiles; z++) {
      SMP.sp0Active[z] = false;
    }
    saveSp0StateToEEPROM();

    for (unsigned int z = 1; z < maxFiles; z++) {
      if (!intro) {

        showIcons(ICON_SAMPLE_BIG, UI_BG_DIM);
      } else {
        drawText("LOAD", 2, 11, col[(maxFiles + 1) - z]);
      }
      drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], UI_DIM_WHITE, intro);
      loadSample(pack_id, z);
    }
  } else {

    for (unsigned int z = 1; z < maxFiles; z++) {
      if (SMP.sp0Active[z]) {

        if (!intro) {

          showIcons(ICON_SAMPLE_BIG, UI_BG_DIM);
        } else {
          drawText("SP0", 2, 11, col[(maxFiles + 1) - z]);
        }
        drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], UI_DIM_WHITE, intro);
        loadSample(0, z);
      }
    }

    for (unsigned int z = 1; z < maxFiles; z++) {
      if (!SMP.sp0Active[z]) {

        if (!intro) {

          showIcons(ICON_SAMPLE_BIG, UI_BG_DIM);
        } else {
          drawText("LOAD", 2, 11, col[(maxFiles + 1) - z]);
        }
        drawLoadingBar(1, maxFiles, z, col_base[(maxFiles + 1) - z], UI_DIM_WHITE, intro);
        loadSample(pack_id, z);
      } else {
      }
    }
  }
  GLOB.seek = savedSeek;
  GLOB.seekEnd = savedSeekEnd;


  switchMode(&draw);
  applyDrawSingleYEncoderLimits();


  preventPaintUnpaint = false;
}

void updateVoiceLengths() {
  uint8_t len[16] = {0};
  uint16_t pageMask[16] = {0};
  uint16_t rowMask[16] = {0};
  int pages = effectivePageCount();
  if (pages < 1) pages = 1;
  if (pages > 16) pages = 16;
  for (int p = 1; p <= pages; p++) {
    unsigned int base = (unsigned int)(p - 1) * maxX;
    for (unsigned int ix = 1; ix <= maxX; ix++) {
      unsigned int x = base + ix;
      if (x >= maxlen) break;
      for (unsigned int iy = 1; iy <= maxY; iy++) {
        uint8_t ch = note[x][iy].channel;
        if (ch == 0 || ch > 15) continue;
        if ((uint8_t)p > len[ch]) len[ch] = (uint8_t)p;
        pageMask[ch] |= (uint16_t)(1u << (p - 1));
        if (iy >= 1 && iy <= 16) rowMask[ch] |= (uint16_t)(1u << (iy - 1));
      }
    }
  }
  uint8_t longest = 1;
  for (int c = 0; c < 16; c++) {

    if (len[c] == 0) {
      voicePageLen[c] = 0;
      voicePageMask[c] = 0;
      voiceRowMask[c] = 0;
    } else {
      voiceRowMask[c] = rowMask[c];
      voicePageMask[c] = pageMask[c];
      voicePageLen[c] = len[c];
      if (c >= 1 && len[c] > longest) longest = len[c];
    }
  }
  voiceModePages = longest;
}

unsigned int voicePageFor(int channel, unsigned int globalPage);


void voiceFillColumn(unsigned int col, unsigned int globalPage, unsigned int srcOut[16]) {
  for (int ch = 0; ch < 16; ch++) srcOut[ch] = 0;
  if (maxX < 1 || col < 1) return;
  for (int ch = 1; ch < 16; ch++) {
    if (voicePageLen[ch] < 1) continue;
    unsigned int page = voicePageFor(ch, globalPage);
    unsigned int src = (page - 1) * maxX + col;
    if (src < 1 || src >= maxlen) continue;
    srcOut[ch] = src;
  }
}


unsigned int voicePickStep(const unsigned int src[16], unsigned int row) {
  if (row < 1 || row > 16) return 0;
  const uint16_t bit = (uint16_t)(1u << (row - 1));
  for (int ch = 1; ch < 16; ch++) {
    unsigned int s = src[ch];
    if (!s) continue;
    if ((voiceRowMask[ch] & bit) == 0) continue;
    if (note[s][row].channel == (uint8_t)ch) return s;
  }
  return 0;
}


unsigned int voicePageFor(int channel, unsigned int globalPage) {
  uint8_t len = 1;
  int offset = 0;
  if (channel >= 0 && channel < 16) {
    if (voicePageLen[channel] > 0) len = voicePageLen[channel];
    offset = voicePageOffset[channel];
  }
  if (globalPage < 1) globalPage = 1;
  int idx = (int)(globalPage - 1) + offset;
  idx %= (int)len;
  if (idx < 0) idx += (int)len;
  return (unsigned int)idx + 1;
}


void voiceCuePage(int channel, int page) {
  extern bool isNowPlaying;
  extern unsigned int beatForUI;
  if (channel < 0 || channel > 15) return;
  uint8_t len = voicePageLen[channel];
  if (len < 1) len = 1;
  if (page < 1) page = 1;
  int wrapped = ((page - 1) % (int)len) + 1;
  unsigned int gPage = 1;
  if (isNowPlaying && beatForUI > 0 && maxX > 0) gPage = ((beatForUI - 1) / maxX) + 1;
  else if (GLOB.page > 0) gPage = GLOB.page;
  int off = (wrapped - 1) - (int)(gPage - 1);
  off %= (int)len;
  if (off < 0) off += (int)len;
  voicePageOffset[channel] = (int8_t)off;
}




unsigned int voiceOwnedStep(unsigned int row, unsigned int col, unsigned int globalPage) {
  unsigned int src[16];
  voiceFillColumn(col, globalPage, src);
  return voicePickStep(src, row);
}




unsigned int cursorNoteStep() {
  extern bool isNowPlaying;
  extern bool songModeActive;
  extern unsigned int beatForUI;
  if (!voiceMode || !isNowPlaying || songModeActive || childLockEnabled) return GLOB.x;
  if (maxX < 1 || GLOB.x < 1 || GLOB.y < 1) return GLOB.x;
  unsigned int col = ((GLOB.x - 1) % maxX) + 1;
  unsigned int gPage = (beatForUI > 0) ? ((beatForUI - 1) / maxX) + 1 : 1;
  unsigned int owned = voiceOwnedStep(GLOB.y, col, gPage);
  if (owned > 0) return owned;
  int ch = (int)GLOB.currentChannel;
  if (ch < 1 || ch > 15) return GLOB.x;
  unsigned int page = voicePageFor(ch, gPage);
  unsigned int src = (page - 1) * maxX + col;
  if (src < 1 || src >= maxlen) return GLOB.x;
  return src;
}

bool voicePageFilled(int channel, int page) {
  if (channel < 1 || channel > 15 || page < 1 || page > 16) return false;
  return (voicePageMask[channel] & (uint16_t)(1u << (page - 1))) != 0;
}

int voiceLastFilledPage(int channel) {
  int pages = effectivePageCount();
  if (pages > 16) pages = 16;
  int last = 0;
  for (int p = 1; p <= pages; p++) if (voicePageFilled(channel, p)) last = p;
  return last;
}



void voiceLimitPageEncoder(int channel) {
  int last = voiceLastFilledPage(channel);


  static int sentCh = -2;
  static int sentMin = -1;
  static int sentMax = -1;
  int minV = 1;
  int maxV = (last < 1) ? encoderPageMax() : last;
  if (channel == sentCh && minV == sentMin && maxV == sentMax) return;
  sentCh = channel;
  sentMin = minV;
  sentMax = maxV;
  Encoder[1].writeMin((int32_t)minV);
  Encoder[1].writeMax((int32_t)maxV);
}

void voiceApplyEditPage(int channel) {
  extern bool songModeActive;
  extern int ctrlMode;
  if (!voiceMode || songModeActive || ctrlMode != 0 || childLockEnabled) return;
  if (channel < 0) channel = 0;
  if (channel > 15) channel = 15;
  updateLastPage();
  int page = voiceEditPage[channel];
  if (page < 1) page = 1;
  int pmax = effectivePageCount();
  if (page > pmax) page = pmax;

  int last = voiceLastFilledPage(channel);
  if (last >= 1) {
    if (page > last) page = last;
    voiceEditPage[channel] = (uint8_t)page;
  }
  voiceLimitPageEncoder(channel);
  editpage = page;
  GLOB.edit = (unsigned int)page;
  if (currentMode) {
    currentMode->pos[1] = (unsigned int)page;
    int rel = ((int)GLOB.x - 1) % (int)maxX + 1;
    if (rel < 1) rel = 1;
    int xval = rel + (page - 1) * (int)maxX;
    xval = constrain(xval, 1, (int)MAX_STEPS);
    GLOB.x = (unsigned int)xval;
    currentMode->pos[3] = (unsigned int)xval;
  }
  Encoder[1].writeCounter((int32_t)page);
  Encoder[3].writeCounter((int32_t)GLOB.x);
}



void extendLastPageForLiveNote(uint8_t channel, uint8_t row, uint16_t targetBeat) {
  if (targetBeat < 1 || maxX < 1) return;
  unsigned int notePage = (targetBeat - 1) / maxX + 1;
  int effectiveMaxPages = effectivePageCount();
  if (effectiveMaxPages < 1) effectiveMaxPages = 1;
  if (notePage < 1 || notePage > (unsigned int)effectiveMaxPages) return;

  hasNotes[notePage] = true;

  extern int loopLength;
  if (voiceMode) {

    if (channel >= 1 && channel <= 15 && notePage <= 16) {
      if (row >= 1 && row <= 16) {
        voiceRowMask[channel] |= (uint16_t)(1u << (row - 1));
      }
      voicePageMask[channel] |= (uint16_t)(1u << (notePage - 1));
      if ((uint8_t)notePage > voicePageLen[channel]) {
        voicePageLen[channel] = (uint8_t)notePage;
      }
      if ((uint8_t)notePage > voiceModePages) {
        voiceModePages = (uint8_t)notePage;
      }
    }
    return;
  }

  if (loopLength > 0) return;


  if (notePage > lastPage) lastPage = notePage;
}

void updateLastPage() {
  updateVoiceLengths();
  int effectiveMaxPages = effectivePageCount();

  extern int loopLength;
  if (!voiceMode && loopLength > 0) {
    unsigned int newLast = (unsigned int)min(loopLength, effectiveMaxPages);
    if (newLast < 1) newLast = 1;
    for (unsigned int p = 1; p <= (unsigned int)effectiveMaxPages; p++) {
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
    for (unsigned int p = (unsigned int)effectiveMaxPages + 1; p <= maxPages; p++) {
      hasNotes[p] = false;
    }
    lastPage = newLast;
    return;
  }




  unsigned int newLast = 0;
  for (unsigned int p = 1; p <= (unsigned int)effectiveMaxPages; p++) {
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
    if (pageHasNotesThisPage) {
      newLast = p;
    }
  }
  for (unsigned int p = (unsigned int)effectiveMaxPages + 1; p <= maxPages; p++) {
    hasNotes[p] = false;
  }
  if (newLast == 0) newLast = 1;
  lastPage = newLast;
}

FLASHMEM void loadWav() {

  preventPaintUnpaint = true;
  freshPaint = false;

  sdIoBeginAudioSafe();
  const bool showProg = !isNowPlaying;
  if (showProg) {
    drawSampleLoadOverlay(0);
    FastLEDshow();
  }

  stopSdPreviewIfPlaying();



  if (!loadPreviewToChannel(GLOB.currentChannel, showProg)) {

    sdIoEndAudioSafe();
    preventPaintUnpaint = false;
    return;
  }



  copySampleToSamplepack0(GLOB.currentChannel, showProg);
  if (!isNowPlaying) {
    saveSp0StateToEEPROM();
    if (showProg) {
      drawSampleLoadOverlay(99);
      FastLEDshow();
    }
    flushSettingsBackupNow();
  }


  int savedEditPage = GLOB.edit;


  switchMode(&draw);
  GLOB.singleMode = false;


  GLOB.edit = savedEditPage;
  if (currentMode != &touchyMode) {
    currentMode->pos[1] = savedEditPage;
    Encoder[1].writeCounter((int32_t)savedEditPage);
  }

  sdIoEndAudioSafe();




}


bool findSliderDefPageSlot(int chan, SettingArray arr, int8_t idx, int &page, int &slot) {
  for (int p = 0; p < FILTER_PAGE_CAP; ++p) {
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


int getDefaultFastFilterValue(int channel, SettingArray arr, int8_t idx) {
  switch (arr) {
    case ARR_FILTER: return filterSetting(channel, idx);
    case ARR_SYNTH: return SMP.synth_settings[channel][idx];
    case ARR_PARAM: return SMP.param_settings[channel][idx];
    case ARR_STRETCH: return channel >= 1 && channel <= 8 ? sampleTimeStretch[channel] : 0;
    default: return 0;
  }
}
void setDefaultFastFilterValue(int channel, SettingArray arr, int8_t idx, int value) {
  switch (arr) {
    case ARR_FILTER: filterSetting(channel, idx) = value; break;
    case ARR_SYNTH: SMP.synth_settings[channel][idx] = value; break;
    case ARR_PARAM: SMP.param_settings[channel][idx] = value; break;
    case ARR_STRETCH: setSampleTimeStretch(channel, value); break;
    default: break;
  }
}


const uint8_t filterPageCount[NUM_CHANNELS] = { 0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 5, 0, 5, 5, 0 };




void setSliderDefForChannel(int channel) {

  if (channel >= 1 && channel <= 8) {
    static const SliderDefEntry sampleVoiceTemplate[3][4] = {
      { { ARR_FILTER, HCUT, "HCUT", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, LOWCUT, "LCUT", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, REVERB, "RVRB", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, BITCRUSHER, "BITC", 32, DISPLAY_NUMERIC, nullptr, 32 } },
      {
        { ARR_FILTER, RES, "RES", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, DETUNE, "DTNE", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, OCTAVE, "OCTV", 48, DISPLAY_NUMERIC, nullptr, 48 },
        { ARR_STRETCH, 0, "TIME", 22, DISPLAY_NUMERIC, nullptr, 23 },
      },
      { { ARR_PARAM, ATTACK, "ATTC", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_PARAM, DECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_PARAM, SUSTAIN, "SUST", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_PARAM, RELEASE, "RLSE", 32, DISPLAY_NUMERIC, nullptr, 32 } }
    };

    for (int p = 0; p < FILTER_PAGE_CAP; ++p) {
      for (int s = 0; s < 4; ++s) {
        sliderDef[channel][p][s] = SliderDefEntry{ ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0 };
      }
    }
    memcpy(sliderDef[channel], sampleVoiceTemplate, sizeof(sampleVoiceTemplate));
    sliderDef[channel][3][0] = { ARR_PARAM, DELAY, "DLAY", 32, DISPLAY_NUMERIC, nullptr, 32 };
    sliderDef[channel][3][1] = { ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0 };
    sliderDef[channel][3][2] = { ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0 };
    sliderDef[channel][3][3] = { ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0 };
    return;
  }

  static const SliderDefEntry lfoArpPage[4] = {
    { ARR_SYNTH, LFO_RATE, "LFO R", 32, DISPLAY_NUMERIC, nullptr, 32 },
    { ARR_SYNTH, LFO_DEPTH, "LFO D", 32, DISPLAY_NUMERIC, nullptr, 32 },
    { ARR_SYNTH, LFO_PHASE, "SPAN", 32, DISPLAY_NUMERIC, nullptr, 32 },
    { ARR_SYNTH, ARP_STEP, "ARP", 32, DISPLAY_NUMERIC, nullptr, 32 }
  };

  if (channel == 11) {
    memcpy(sliderDef[channel][4], lfoArpPage, sizeof(lfoArpPage));
    sliderDef[channel][1][3] = { ARR_PARAM, DELAY, "DLAY", 32, DISPLAY_NUMERIC, nullptr, 32 };
    return;
  }

  if (channel == 13 || channel == 14) {
    static const SliderDefEntry ch13_14Template[4][4] = {
      { { ARR_FILTER, HCUT, "HCUT", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, LOWCUT, "LCUT", 32, DISPLAY_NUMERIC, nullptr, 32 },

        { ARR_SYNTH, CENT, "CENT", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, BITCRUSHER, "BITC", 32, DISPLAY_NUMERIC, nullptr, 32 } },
      { { ARR_SYNTH, CUTOFF, "CUT", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_SYNTH, RESONANCE, "RES", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_SYNTH, FILTER, "FLT", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_PARAM, DELAY, "DLAY", 32, DISPLAY_NUMERIC, nullptr, 32 } },
      { { ARR_FILTER, DETUNE, "DTNE", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, OCTAVE, "OCTV", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_FILTER, FILTER_WAVEFORM, "WAVE", 16, DISPLAY_ENUM, waveformNames, 4 },
        { ARR_NONE, -1, "", 0, DISPLAY_NUMERIC, nullptr, 0 } },
      { { ARR_PARAM, ATTACK, "ATTC", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_PARAM, DECAY, "DCAY", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_PARAM, SUSTAIN, "SUST", 32, DISPLAY_NUMERIC, nullptr, 32 },
        { ARR_PARAM, RELEASE, "RLSE", 32, DISPLAY_NUMERIC, nullptr, 32 } }
    };
    memset(sliderDef[channel], 0, sizeof(sliderDef[channel]));
    memcpy(sliderDef[channel][0], ch13_14Template[0], sizeof(ch13_14Template[0]));
    memcpy(sliderDef[channel][1], ch13_14Template[1], sizeof(ch13_14Template[1]));
    memcpy(sliderDef[channel][2], ch13_14Template[2], sizeof(ch13_14Template[2]));
    memcpy(sliderDef[channel][3], ch13_14Template[3], sizeof(ch13_14Template[3]));
    memcpy(sliderDef[channel][4], lfoArpPage, sizeof(lfoArpPage));
    return;
  }
}


FLASHMEM void initPageMutes() {

  for (int ch = 0; ch < maxY; ch++) {
    globalMutes[ch] = SMP.globalMutes[ch];
    for (int page = 0; page < maxPages; page++) {
      pageMutes[page][ch] = SMP.pageMutes[page][ch];
    }
  }


  bool hasValidMuteData = false;
  for (int ch = 0; ch < maxY; ch++) {
    if (SMP.globalMutes[ch] != false || SMP.pageMutes[0][ch] != false) {
      hasValidMuteData = true;
      break;
    }
  }

  if (!hasValidMuteData) {

    for (int ch = 0; ch < maxY; ch++) {
      globalMutes[ch] = SMP.mute[ch];

      for (int page = 0; page < maxPages; page++) {
        pageMutes[page][ch] = SMP.mute[ch];
      }
    }
  }
}



bool getMuteState(int channel) {
  if (childLockEnabled) {
    return isChildVoiceMuted(channel);
  }

  if (SMP_PATTERN_MODE) {

    extern int patternMode;


    unsigned int mutePage = (patternMode == 3) ? GLOB.page : GLOB.edit;
    if (mutePage == 0) mutePage = GLOB.edit;
    return pageMutes[mutePage - 1][channel];
  } else {

    return globalMutes[channel];
  }
}


FLASHMEM bool getMuteStateForUI(int channel) {
  if (childLockEnabled) {
    return isChildVoiceMuted(channel);
  }

  if (SMP_PATTERN_MODE) {


    unsigned int mutePage = GLOB.edit;
    if (mutePage == 0) mutePage = 1;
    return pageMutes[mutePage - 1][channel];
  } else {

    return globalMutes[channel];
  }
}

static void stopMutedVoiceAudio(int ch) {


  if (ch >= 1 && ch <= 8) {
    for (int note = 36; note <= 96; note++) {
      samplerNoteEvent(ch, note, 0, false, false);
    }
    if (ch < 15 && envelopes[ch] != nullptr) {
      envelopes[ch]->noteOff();
    }
  } else if (ch == 11) {
    for (int noteValue = 0; noteValue <= 107; noteValue++) {
      stopSound(noteValue, 0);
      stopSound(noteValue, 1);
    }
    pressedKeyCount[11] = 0;
  } else if (ch == 13 || ch == 14) {
    stopSynthChannel(ch);
    pressedKeyCount[ch] = 0;
  }
}


FLASHMEM void setMuteState(int channel, bool muted) {
  if (channel < 0 || channel >= (int)maxY) return;

  if (childLockEnabled) {
    bool childMuted = isChildVoiceMuted(channel);
    globalMutes[channel] = childMuted;
    SMP.globalMutes[channel] = childMuted;
    SMP.mute[channel] = childMuted ? 1u : 0u;
    for (int page = 0; page < maxPages; page++) {
      pageMutes[page][channel] = childMuted;
      SMP.pageMutes[page][channel] = childMuted;
    }
    return;
  }

  if (SMP_PATTERN_MODE) {

    extern int patternMode;


    unsigned int mutePage = GLOB.edit;
    if (mutePage == 0) mutePage = 1;
    if (mutePage > maxPages) mutePage = maxPages;
    pageMutes[mutePage - 1][channel] = muted;


    if (muted) {
      bool shouldStop = true;
      if (patternMode == 3) {

        shouldStop = (GLOB.page > 0 && mutePage == GLOB.page);
      }
      if (shouldStop) {
        stopMutedVoiceAudio(channel);
      }
    }
  } else {

    globalMutes[channel] = muted;
    SMP.mute[channel] = muted;
    if (muted) {
      stopMutedVoiceAudio(channel);
    }
  }
}





FLASHMEM void unmuteAllChannels() {
  for (int ch = 0; ch < maxY; ch++) {
    bool childMuted = isChildVoiceMuted(ch);
    SMP.mute[ch] = childMuted ? 1u : 0u;
    if (childLockEnabled) {
      globalMutes[ch] = childMuted;
      SMP.globalMutes[ch] = childMuted;
      for (int page = 0; page < maxPages; page++) {
        pageMutes[page][ch] = childMuted;
        SMP.pageMutes[page][ch] = childMuted;
      }
    }
  }
}


FLASHMEM void applyMutesAfterPMODSwitch() {
  if (childLockEnabled) {
    unmuteAllChannels();
    return;
  }

  for (int ch = 0; ch < maxY; ch++) {
    bool muteState = getMuteState(ch);
    SMP.mute[ch] = muteState;
  }
}



FLASHMEM void copyPageMutes(int fromPage, int toPage) {
  if (!SMP_PATTERN_MODE || childLockEnabled) return;
  if (fromPage < 1 || toPage < 1) return;
  if (fromPage > (int)maxPages || toPage > (int)maxPages) return;
  if (fromPage == toPage) return;
  for (int ch = 0; ch < maxY; ch++) {
    bool muted = pageMutes[fromPage - 1][ch];
    pageMutes[toPage - 1][ch] = muted;
    SMP.pageMutes[toPage - 1][ch] = muted;
  }
}
# 1 "/Users/jank/git/toern/toern_fileoperations.ino"
#include "src/pattern_transaction.h"
static bool lastPatternSaveOk = false;
static const char *AUTOSAVE_PATH = "autosaved.txt";
static const uint8_t PITCH_EXTENSION_HEADER[8] = {
  'T', 'P', 'I', 'T', 1, 0x10, 0x00, 0
};
bool g_enterNewFileAfterCorruptAutoload = false;

FLASHMEM void clearPatternNotes() {
  for (unsigned int x = 0; x <= maxlen; x++) {
    for (unsigned int y = 0; y <= maxY; y++) {
      note[x][y].channel = 0;
      note[x][y].velocity = defaultVelocity;
      note[x][y].probability = 100;
      note[x][y].condition = 1;
      note[x][y].midiPitch = NOTE_MIDI_PITCH_NONE;
    }
  }
}

FLASHMEM void deleteAutosaveFile() {
  if (SD.exists(AUTOSAVE_PATH)) SD.remove(AUTOSAVE_PATH);
  SD.remove("autosaved.txt.tmp");
  SD.remove("autosaved.txt.bak");
}

static bool patternWriteAll(File &f, const uint8_t *data, size_t len) {
  extern void sdIoYield();
  while (len > 0) {
    size_t n = f.write(data, len);
    if (n == 0) return false;
    data += n;
    len -= n;
    sdIoYield();
  }
  return true;
}


FLASHMEM bool validPatternFile(const char *path, bool requirePitch) {
  File f = SD.open(path);
  if (!f) return false;
  const size_t notes = (size_t)(maxlen - 1) * maxY;
  const size_t base = notes * 4u + 2u + sizeof(SMP);
  const size_t total = base + sizeof(PITCH_EXTENSION_HEADER) + notes;
  bool ok = f.size() >= (requirePitch ? total : base);
  if (ok) {
    ok = f.seek(notes * 4u) && f.read() == 0xFF && f.read() == 0xFE;
  }
  if (ok && requirePitch) {
    uint8_t header[sizeof(PITCH_EXTENSION_HEADER)];
    ok = f.seek(base) && f.read(header, sizeof(header)) == (int)sizeof(header)
      && memcmp(header, PITCH_EXTENSION_HEADER, sizeof(header)) == 0;
  }

  if (ok) {
    ok = f.seek(0);
    uint8_t buf[512];
    size_t left = requirePitch ? total : base;
    while (ok && left) {
      const size_t count = min(left, sizeof(buf));
      ok = f.read(buf, count) == (int)count;
      left -= count;
      sdIoYield();
    }
  }
  f.close();
  return ok;
}

FLASHMEM void showPatternResult(bool success) {
  if (success) showSaveSuccessAnimation(false);
  else showSaveFailureAnimation(false);
}

FLASHMEM void savePattern(bool autosave) {
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();

  lastPatternSaveOk = false;

  if (!autosave && SMP.file == 0) {
    return;
  }

  drawNoSD();


  sdIoBeginAudioSafe();
  stopSdPreviewIfPlaying();


  if (!autosave) {
    FastLEDclear();
  }


  char OUTPUTf[50];
  if (autosave) {
    sprintf(OUTPUTf, "%s", AUTOSAVE_PATH);
  } else {
    sprintf(OUTPUTf, "%d.txt", (int)SMP.file);
  }

  char tmpPath[64], backupPath[64];
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", OUTPUTf);
  snprintf(backupPath, sizeof(backupPath), "%s.bak", OUTPUTf);
  bool empty = true;
  for (unsigned int x = 1; x < maxlen && empty; x++) {
    for (unsigned int y = 1; y <= maxY; y++) {
      if (note[x][y].channel != 0) { empty = false; break; }
    }
  }


  if (!autosave && empty) {
    lastPatternSaveOk = deletePatternTransaction(SD, OUTPUTf, tmpPath, backupPath);
    sdIoEndAudioSafe();
    showPatternResult(lastPatternSaveOk);
    switchMode(&draw);
    return;
  }
  recoverPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);

  if (!validPatternFile(OUTPUTf, false) && (validPatternFile(tmpPath, true) || validPatternFile(backupPath, false))) {
    sdIoEndAudioSafe();
    if (!autosave) { showPatternResult(false); switchMode(&draw); }
    return;
  }
  if (!autosave) { drawText("SAVE", 1, 8, UI_GREEN); FastLEDshow(); }
  File saveFile = SD.open(tmpPath, O_WRITE | O_CREAT | O_TRUNC);
  bool wroteOk = false;
  if (saveFile) {
    uint8_t buf[512];
    size_t bufLen = 0;
    unsigned rowCount = 0;
    wroteOk = true;

    for (unsigned int sdx = 1; sdx < maxlen && wroteOk; sdx++) {
      for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
        if (bufLen + 4 > sizeof(buf)) {
          if (!patternWriteAll(saveFile, buf, bufLen)) {
            wroteOk = false;
            break;
          }
          bufLen = 0;
        }
        buf[bufLen++] = note[sdx][sdy].channel;
        buf[bufLen++] = note[sdx][sdy].velocity;
        buf[bufLen++] = note[sdx][sdy].probability;
        buf[bufLen++] = note[sdx][sdy].condition;
        if ((++rowCount & 0x1Fu) == 0u) {
          sdIoYield();
        }
      }
    }
    if (wroteOk && bufLen > 0) {
      wroteOk = patternWriteAll(saveFile, buf, bufLen);
      bufLen = 0;
    }

    if (wroteOk) {
      const uint8_t marker[2] = { 0xFF, 0xFE };
      wroteOk = patternWriteAll(saveFile, marker, 2);
    }

    for (int ch = 0; ch < maxY; ch++) {
      SMP.globalMutes[ch] = globalMutes[ch];
      for (int page = 0; page < maxPages; page++) {
        SMP.pageMutes[page][ch] = pageMutes[page][ch];
      }
    }

    if (wroteOk) {
      const uint8_t *smpBytes = (const uint8_t *)&SMP;
      size_t smpLeft = sizeof(SMP);
      size_t smpOff = 0;
      const size_t smpChunk = 256;
      while (smpLeft > 0 && wroteOk) {
        size_t n = min(smpChunk, smpLeft);
        wroteOk = patternWriteAll(saveFile, smpBytes + smpOff, n);
        smpOff += n;
        smpLeft -= n;
      }
    }



    if (wroteOk) {
      wroteOk = patternWriteAll(saveFile, PITCH_EXTENSION_HEADER,
                                sizeof(PITCH_EXTENSION_HEADER));
    }
    if (wroteOk) {
      size_t pitchLen = 0;
      for (unsigned int sdx = 1; sdx < maxlen && wroteOk; sdx++) {
        for (unsigned int sdy = 1; sdy <= maxY; sdy++) {
          buf[pitchLen++] = note[sdx][sdy].midiPitch;
          if (pitchLen == sizeof(buf)) {
            wroteOk = patternWriteAll(saveFile, buf, pitchLen);
            pitchLen = 0;
            sdIoYield();
          }
        }
      }
      if (wroteOk && pitchLen > 0) {
        wroteOk = patternWriteAll(saveFile, buf, pitchLen);
      }
    }
    saveFile.close();
  }

  if (wroteOk) {
    wroteOk = commitPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);
  } else {

    SD.remove(tmpPath);
  }
  lastPatternSaveOk = wroteOk;
  if (!autosave) {
    if (wroteOk) {
      showPatternResult(true);
      switchMode(&draw);
    } else {
      showPatternResult(false);
      switchMode(&draw);
    }
  }


  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;

  sdIoYield();
  sdIoEndAudioSafe();
}

FLASHMEM void saveSamplePack(int pack) {
    extern void stopSdPreviewIfPlaying();
    extern size_t sdIoChunkSize();
    extern void sdIoYield();
    extern void sdIoBeginAudioSafe();
    extern void sdIoEndAudioSafe();

    sdIoBeginAudioSafe();
    stopSdPreviewIfPlaying();

    bool packSaved = true;
    char filename[64];
    char sourcePath[64];

    char dirPath[64];

    snprintf(dirPath, sizeof(dirPath), "%d", pack);
    if (!SD.exists(dirPath)) {
        packSaved = SD.mkdir(dirPath);
    }


    for (uint8_t ch = 1; ch <= 8 && packSaved; ch++) {

        drawLoadingBar(1, maxFiles, ch, col_base[(maxFiles + 1) - ch], UI_DIM_WHITE, false);

        showIcons(OLD_ICON_SAMPLEPACK, UI_BG_DIM);

        FastLEDshow();


        snprintf(sourcePath, sizeof(sourcePath), "0/%d.wav", ch);
        snprintf(filename, sizeof(filename), "%d/%d.wav", pack, ch);

        bool useSp0Override = SMP.sp0Active[ch] && SD.exists(sourcePath);

        if (useSp0Override) {


            if (SD.exists(filename)) {
                if (!SD.remove(filename)) { packSaved = false; break; }
            }


            File sourceFile = SD.open(sourcePath, FILE_READ);
            if (!sourceFile) {
                packSaved = false;
                break;
            }


            File destFile = SD.open(filename, FILE_WRITE);
            if (!destFile) {
                sourceFile.close();
                packSaved = false;
                break;
            }


            static EXTMEM uint8_t packCopyBuf[65536];
            const size_t chunk = sdIoChunkSize();
            const size_t sourceSize = sourceFile.size();
            size_t copied = 0;
            int bytesRead;
            while ((bytesRead = sourceFile.read(packCopyBuf, chunk)) > 0) {
                if (!patternWriteAll(destFile, packCopyBuf, (size_t)bytesRead)) { packSaved = false; break; }
                copied += (size_t)bytesRead;
                sdIoYield();
            }

            packSaved = packSaved && copied == sourceSize;
            sourceFile.close();
            destFile.close();
        } else if (loadedSampleLen[ch] > 0) {


            if (SD.exists(filename)) {
                if (!SD.remove(filename)) { packSaved = false; break; }
            }


            File outFile = SD.open(filename, FILE_WRITE);
            if (!outFile) {
                packSaved = false;
                break;
            }


            writeWavHeader(outFile, AUDIO_SAMPLE_RATE_EXACT, 16, 1);


            uint32_t sampleCount = loadedSampleLen[ch];


            uint32_t totalBytes = sampleCount * sizeof(int16_t);
            uint8_t* dataPtr = reinterpret_cast<uint8_t*>(sampled[ch]);
            const size_t CHUNK_SIZE = sdIoChunkSize();

            for (uint32_t offset = 0; offset < totalBytes; offset += CHUNK_SIZE) {
              size_t chunkSize = min(CHUNK_SIZE, (size_t)(totalBytes - offset));
              if (!patternWriteAll(outFile, dataPtr + offset, chunkSize)) { packSaved = false; break; }
              sdIoYield();
            }

            finalizeWavHeader(outFile, totalBytes);

            packSaved = packSaved && outFile.size() == totalBytes + 44u;
            outFile.close();


            if (packSaved) {
                File verifyFile = SD.open(filename, FILE_READ);
                uint8_t header[44] = {};
                packSaved = verifyFile && verifyFile.size() == totalBytes + 44u
                    && verifyFile.read(header, sizeof(header)) == (int)sizeof(header);
                uint32_t riffBytes = 0, pcmBytes = 0;
                memcpy(&riffBytes, header + 4, 4);
                memcpy(&pcmBytes, header + 40, 4);
                packSaved = packSaved && memcmp(header, "RIFF", 4) == 0
                    && memcmp(header + 8, "WAVE", 4) == 0
                    && riffBytes == totalBytes + 36u && pcmBytes == totalBytes;
                verifyFile.close();
            }
        } else {


            if (SD.exists(filename)) {
                if (!SD.remove(filename)) { packSaved = false; break; }
            }

            File emptyFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);
            if (emptyFile) {
                emptyFile.close();
            } else { packSaved = false; }
        }
        sdIoYield();
    }


    for (uint8_t ch = 1; packSaved && ch < maxFiles; ch++) {
        SMP.sp0Active[ch] = false;
    }
    if (packSaved) saveSp0StateToEEPROM();


    extern bool preventPaintUnpaint;
    preventPaintUnpaint = false;

   sdIoYield();
   sdIoEndAudioSafe();
   showPatternResult(packSaved);
   switchMode(&draw);
}

FLASHMEM void loadPattern(bool autoload) {
  extern bool isNowPlaying;
  extern void stopSdPreviewIfPlaying();
  extern void sdIoYield();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();

  drawNoSD();
  stopSdPreviewIfPlaying();


  if (!autoload) {
    sdIoBeginAudioSafe();
  }

  FastLEDclear();
  char OUTPUTf[50];

  if (autoload || SMP.file == 0) {
    sprintf(OUTPUTf, "%s", AUTOSAVE_PATH);
  } else {
    sprintf(OUTPUTf, "%d.txt", (int)SMP.file);
  }

  char tmpPath[64], backupPath[64];
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", OUTPUTf);
  snprintf(backupPath, sizeof(backupPath), "%s.bak", OUTPUTf);
  recoverPatternTransaction(SD, OUTPUTf, tmpPath, backupPath, validPatternFile);
  if (!autoload) { drawText("LOAD", 1, 8, UI_GREEN); FastLEDshow(); }
  bool loadedOk = false;
  const size_t expectedNotes = (size_t)(maxlen - 1) * (size_t)maxY;



  static EXTMEM uint8_t payload[(maxlen - 1) * maxY * 5u + 2u
                              + sizeof(SMP) + sizeof(PITCH_EXTENSION_HEADER)];
  const size_t baseBytes = expectedNotes * 4u + 2u + sizeof(SMP);
  File loadFile = SD.open(OUTPUTf);
  bool pitchPresent = false;
  if (loadFile) {
    size_t offset = 0;
    bool readOk = loadFile.size() >= baseBytes;
    while (readOk && offset < baseBytes) {
      const size_t count = min((size_t)512, baseBytes - offset);
      readOk = loadFile.read(payload + offset, count) == (int)count;
      offset += count;
      sdIoYield();
    }
    readOk = readOk && payload[expectedNotes * 4u] == 0xFF
                    && payload[expectedNotes * 4u + 1u] == 0xFE;
    if (readOk && loadFile.available() >= (int)(sizeof(PITCH_EXTENSION_HEADER) + expectedNotes)) {
      pitchPresent = loadFile.read(payload + baseBytes, sizeof(PITCH_EXTENSION_HEADER))
                       == (int)sizeof(PITCH_EXTENSION_HEADER)
        && memcmp(payload + baseBytes, PITCH_EXTENSION_HEADER, sizeof(PITCH_EXTENSION_HEADER)) == 0;
      size_t pitchOffset = 0;
      while (pitchPresent && pitchOffset < expectedNotes) {
        const size_t count = min((size_t)512, expectedNotes - pitchOffset);
        pitchPresent = loadFile.read(payload + baseBytes + sizeof(PITCH_EXTENSION_HEADER)
                                     + pitchOffset, count) == (int)count;
        pitchOffset += count;
        sdIoYield();
      }
    }
    loadFile.close();
    if (readOk) {
      clearPatternNotes();
      size_t index = 0;
      for (unsigned int x = 1; x < maxlen; x++) {
        for (unsigned int y = 1; y <= maxY; y++, index++) {
          const uint8_t *cell = payload + index * 4u;
          note[x][y].channel = cell[0] <= 16 ? cell[0] : 0;
          note[x][y].velocity = min(cell[1], (uint8_t)127);
          note[x][y].probability = min(cell[2], (uint8_t)100);
          note[x][y].condition = cell[3];
          const uint8_t pitch = pitchPresent
            ? payload[baseBytes + sizeof(PITCH_EXTENSION_HEADER) + index] : NOTE_MIDI_PITCH_NONE;
          note[x][y].midiPitch = (pitch <= 127 || pitch == NOTE_MIDI_PITCH_NONE)
            ? pitch : NOTE_MIDI_PITCH_NONE;
        }
      }
      memcpy(&SMP, payload + expectedNotes * 4u + 2u, sizeof(SMP));
      if (!isfinite(SMP.bpm) || SMP.bpm < 40.0f || SMP.bpm > 300.0f) SMP.bpm = 100.0f;
      if (SMP.file > 999) SMP.file = 1;
      if (SMP.pack > 999) SMP.pack = 1;
      for (int ch = 0; ch < maxY; ch++) {
        globalMutes[ch] = SMP.globalMutes[ch];
        for (int page = 0; page < maxPages; page++) pageMutes[page][ch] = SMP.pageMutes[page][ch];
      }
      unmuteAllChannels();
      applyMutesAfterPMODSwitch();
      loadedOk = true;
    }
    sdIoYield();
  }

  if (!loadedOk) {

    if (autoload) clearPatternNotes();
    const bool fileWasPresent = SD.exists(OUTPUTf);
    if (!autoload) sdIoEndAudioSafe();
    if (fileWasPresent) {
      extern void FastLEDclear();
      extern void FastLEDshow();
      extern void drawText(const char *text, int startX, int startY, CRGB color);
      showPatternResult(false);
      if (autoload) {
        g_enterNewFileAfterCorruptAutoload = true;
      } else {
        switchMode(&draw);
      }
    } else if (!autoload && SMP.file != 0) {
      extern void showNewFileScreen();
      showNewFileScreen();
    }
    extern bool preventPaintUnpaint;
    preventPaintUnpaint = false;
    return;
  }


  GLOB.singleMode = false;


  if (!isfinite(SMP.bpm) || SMP.bpm < 40.0f || SMP.bpm > 300.0f) {
    SMP.bpm = 100.0f;
  }

  bpmMode.pos[3] = SMP.bpm;
  playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
  playTimer.update((uint32_t)round(playNoteInterval));
  bpmMode.pos[2] = (clockMode == 1) ? 1 : 0;


  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;

  updateLastPage();


  if (!autoload && !isNowPlaying) {
    delay(500);
  }
  loadSMPSettings();

  if (!autoload) {
    if (!isNowPlaying) {
      delay(500);
    }
    switchMode(&draw);
    applyDrawSingleYEncoderLimits();
  }

  sdIoYield();
  if (!autoload) sdIoEndAudioSafe();
}

FLASHMEM void autoLoad() {


  loadPattern(true);


  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}

FLASHMEM void autoSave() {

  static unsigned long lastAutoSaveTime = 0;
  const unsigned long AUTO_SAVE_COOLDOWN_MS = 5000;

  unsigned long currentTime = millis();
  if (lastAutoSaveTime > 0 && (currentTime - lastAutoSaveTime) < AUTO_SAVE_COOLDOWN_MS) {
    return;
  }

  savePattern(true);
  if (lastPatternSaveOk) lastAutoSaveTime = currentTime;

  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}




static bool isVoicePresetChannel(int ch) {
  return ch == 11 || ch == 13 || ch == 14;
}

static uint16_t g_presetSlotByChannel[16] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static bool g_presetUiDirty = false;

void invalidateVoicePresetUi() { g_presetUiDirty = true; }
bool takeVoicePresetUiDirty() {
  if (!g_presetUiDirty) return false;
  g_presetUiDirty = false;
  return true;
}

uint16_t getVoicePresetSlot(int ch) {
  if (!isVoicePresetChannel(ch)) return 1;
  uint16_t s = g_presetSlotByChannel[ch];
  if (s < 1) s = 1;
  if (s > 99) s = 99;
  return s;
}

void setVoicePresetSlot(int ch, uint16_t slot) {
  if (!isVoicePresetChannel(ch)) return;
  if (slot < 1) slot = 1;
  if (slot > 99) slot = 99;
  g_presetSlotByChannel[ch] = slot;
}

static bool buildVoicePresetPath(int ch, uint16_t slot, char *out, size_t outSize) {
  if (!out || outSize < 24 || !isVoicePresetChannel(ch)) return false;
  if (slot < 1) slot = 1;
  if (slot > 99) slot = 99;
  snprintf(out, outSize, "presets/%d/%03u.pset", ch, (unsigned)slot);
  return true;
}

static bool ensureVoicePresetDirs(int ch) {
  if (!SD.exists("presets")) {
    if (!SD.mkdir("presets")) return false;
  }
  char dir[24];
  snprintf(dir, sizeof(dir), "presets/%d", ch);
  if (!SD.exists(dir)) {
    if (!SD.mkdir(dir)) return false;
  }
  return true;
}

static uint8_t readPresetSettingValue(int ch, SettingArray arr, int idx) {
  switch (arr) {
    case ARR_FILTER:
      return (uint8_t)constrain((int)lroundf(filterSetting(ch, idx)), 0, 255);
    case ARR_SYNTH:
      if (idx < 0 || idx >= 11) return 0;
      return (uint8_t)constrain((int)lroundf(SMP.synth_settings[ch][idx]), 0, 255);
    case ARR_PARAM:
      if (idx < 0 || idx >= PARAM_COUNT) return 0;
      return (uint8_t)constrain((int)lroundf(SMP.param_settings[ch][idx]), 0, 255);
    default:
      return 0;
  }
}

static void writePresetSettingValue(int ch, SettingArray arr, int idx, uint8_t value) {
  switch (arr) {
    case ARR_FILTER:
      filterSetting(ch, idx) = value;
      break;
    case ARR_SYNTH:
      if (idx >= 0 && idx < 11) SMP.synth_settings[ch][idx] = value;
      break;
    case ARR_PARAM:
      if (idx >= 0 && idx < PARAM_COUNT) SMP.param_settings[ch][idx] = value;
      break;
    default:
      break;
  }
}

static bool voicePresetFileValid(const char *path, bool ) {
  if (!path || !path[0] || !SD.exists(path)) return false;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  uint8_t hdr[4];
  bool ok = (f.read(hdr, 4) == 4) &&
            hdr[0] == 'T' && hdr[1] == 'P' && hdr[2] == 'S' && hdr[3] == 'T';
  f.close();
  return ok;
}

bool voicePresetExists(int ch, uint16_t slot) {
  char path[40];
  if (!buildVoicePresetPath(ch, slot, path, sizeof(path))) return false;
  return voicePresetFileValid(path, false);
}

FLASHMEM bool saveVoicePreset(int ch, uint16_t slot) {
  if (!isVoicePresetChannel(ch)) return false;
  if (!ensureVoicePresetDirs(ch)) return false;

  extern const uint8_t filterPageCount[NUM_CHANNELS];
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();
  extern void sdIoYield();

  char path[40];
  char tmpPath[48];
  char bakPath[48];
  if (!buildVoicePresetPath(ch, slot, path, sizeof(path))) return false;
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
  snprintf(bakPath, sizeof(bakPath), "%s.bak", path);

  struct Entry {
    uint8_t arr;
    uint8_t idx;
    uint8_t value;
  };
  Entry entries[48];
  uint8_t count = 0;

  auto pushEntry = [&](SettingArray arr, int idx) {
    if (count >= 48) return;
    if (arr == ARR_NONE || idx < 0) return;
    if (arr == ARR_PARAM && idx >= PARAM_COUNT) return;
    if (arr == ARR_SYNTH && idx >= 11) return;
    for (uint8_t i = 0; i < count; i++) {
      if (entries[i].arr == (uint8_t)arr && entries[i].idx == (uint8_t)idx) {
        entries[i].value = readPresetSettingValue(ch, arr, idx);
        return;
      }
    }
    entries[count].arr = (uint8_t)arr;
    entries[count].idx = (uint8_t)idx;
    entries[count].value = readPresetSettingValue(ch, arr, idx);
    count++;
  };

  int pages = filterPageCount[ch];
  if (pages <= 0) pages = 4;
  if (pages > FILTER_PAGE_CAP) pages = FILTER_PAGE_CAP;
  for (int p = 0; p < pages; p++) {
    for (int s = 0; s < 4; s++) {
      const SliderDefEntry &d = sliderDef[ch][p][s];
      pushEntry(d.arr, d.idx);
    }
  }
  if (ch == 11) pushEntry(ARR_SYNTH, FORM);

  sdIoBeginAudioSafe();
  if (SD.exists(tmpPath)) SD.remove(tmpPath);
  File f = SD.open(tmpPath, O_WRITE | O_CREAT | O_TRUNC);
  if (!f) {
    sdIoEndAudioSafe();
    return false;
  }

  const uint8_t magic[4] = { 'T', 'P', 'S', 'T' };
  const uint8_t version = 1;
  const uint8_t channelByte = (uint8_t)ch;
  bool ok = (f.write(magic, 4) == 4) &&
            (f.write(&version, 1) == 1) &&
            (f.write(&channelByte, 1) == 1) &&
            (f.write(&count, 1) == 1);
  for (uint8_t i = 0; ok && i < count; i++) {
    ok = (f.write(&entries[i].arr, 1) == 1) &&
         (f.write(&entries[i].idx, 1) == 1) &&
         (f.write(&entries[i].value, 1) == 1);
    if ((i & 7) == 0) sdIoYield();
  }
  f.close();
  if (!ok) {
    SD.remove(tmpPath);
    sdIoEndAudioSafe();
    return false;
  }

  ok = commitPatternTransaction(SD, path, tmpPath, bakPath, voicePresetFileValid);
  sdIoEndAudioSafe();
  if (ok) {
    setVoicePresetSlot(ch, slot);
    invalidateVoicePresetUi();
  }
  return ok;
}

FLASHMEM bool loadVoicePreset(int ch, uint16_t slot) {
  if (!isVoicePresetChannel(ch)) return false;

  char path[40];
  if (!buildVoicePresetPath(ch, slot, path, sizeof(path))) return false;
  if (!voicePresetFileValid(path, false)) return false;

  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();
  extern void sdIoYield();
  extern void applyMonoSynthCrf(int ch);
  extern void updateSynthVoice(int channel);
  extern void midiArpResetVoice(int ch);

  sdIoBeginAudioSafe();
  File f = SD.open(path, FILE_READ);
  if (!f) {
    sdIoEndAudioSafe();
    return false;
  }

  uint8_t magic[4];
  uint8_t version = 0;
  uint8_t fileCh = 0;
  uint8_t count = 0;
  bool ok = (f.read(magic, 4) == 4) &&
            magic[0] == 'T' && magic[1] == 'P' && magic[2] == 'S' && magic[3] == 'T' &&
            (f.read(&version, 1) == 1) && version == 1 &&
            (f.read(&fileCh, 1) == 1) &&
            (f.read(&count, 1) == 1) &&
            count <= 48;

  if (ok && fileCh != 11 && fileCh != 13 && fileCh != 14) ok = false;

  for (uint8_t i = 0; ok && i < count; i++) {
    uint8_t arr = 0, idx = 0, value = 0;
    ok = (f.read(&arr, 1) == 1) && (f.read(&idx, 1) == 1) && (f.read(&value, 1) == 1);
    if (!ok) break;
    writePresetSettingValue(ch, (SettingArray)arr, (int)idx, value);
    if ((i & 7) == 0) sdIoYield();
  }
  f.close();
  sdIoEndAudioSafe();
  if (!ok) return false;


  extern const uint8_t filterPageCount[NUM_CHANNELS];
  int pages = filterPageCount[ch];
  if (pages <= 0) pages = 4;
  if (pages > FILTER_PAGE_CAP) pages = FILTER_PAGE_CAP;
  for (int p = 0; p < pages; p++) {
    for (int s = 0; s < 4; s++) {
      const SliderDefEntry &d = sliderDef[ch][p][s];
      if (d.arr == ARR_NONE || d.idx < 0) continue;
      if (d.arr == ARR_FILTER) {
        setFilters((FilterType)d.idx, ch, false);
      } else if (d.arr == ARR_PARAM && d.idx < PARAM_COUNT) {
        setParams((ParameterType)d.idx, ch);
      }
    }
  }
  if (ch == 11) {
    updateSynthVoice(11);
  } else {
    applyMonoSynthCrf(ch);
  }
  midiArpResetVoice(ch);

  if (currentMode == &filterMode && (int)GLOB.currentChannel == ch) {
    extern uint8_t filterPage[NUM_CHANNELS];
    extern void initSliders(uint8_t page, uint8_t chan);
    initSliders(filterPage[ch], (uint8_t)ch);
  }

  setVoicePresetSlot(ch, slot);
  invalidateVoicePresetUi();
  return true;
}
# 1 "/Users/jank/git/toern/toern_filter.ino"

extern float detune[13];
extern float channelOctave[9];
extern uint8_t waveformsArray[2][5];


void handleWaveformChange(int index, unsigned int waveformType);


#define NUM_MIXER_CHANNELS 3

struct GainTransition {
  float currentGain[NUM_MIXER_CHANNELS];
  float targetGain[NUM_MIXER_CHANNELS];
  float step[NUM_MIXER_CHANNELS];
  bool active[NUM_MIXER_CHANNELS];
};

GainTransition filterMixerTransitions[NUM_FILTERS];



void updateMixerGains(int i) {
  if (i < 0 || i >= NUM_FILTERS || filtermixers[i] == nullptr) return;

  for (int ch = 0; ch < NUM_MIXER_CHANNELS; ++ch) {
    if (!filterMixerTransitions[i].active[ch]) continue;
    float& cur = filterMixerTransitions[i].currentGain[ch];
    float tgt = filterMixerTransitions[i].targetGain[ch];
    float stp = filterMixerTransitions[i].step[ch];
    if (fabs(cur - tgt) <= fabs(stp)) {
      cur = tgt;
      filterMixerTransitions[i].active[ch] = false;
    } else {
      cur += stp;
    }
    filtermixers[i]->gain(ch, cur);
  }
}




void updateAllMixerGains() {
  extern const int ALL_CHANNELS[];
  extern const int NUM_ALL_CHANNELS;
  for (int n = 0; n < NUM_ALL_CHANNELS; ++n) {
    updateMixerGains(ALL_CHANNELS[n]);
  }
}




void setMixerGainSmooth(int index, int channel, float target, int steps = 8) {
  if (filtermixers[index] == nullptr) return;
  float cur = filterMixerTransitions[index].currentGain[channel];
  filterMixerTransitions[index].targetGain[channel] = target;
  filterMixerTransitions[index].step[channel] = (target - cur) / steps;
  filterMixerTransitions[index].active[channel] = true;
}


void initFilterTransition(int index) {
  if (filtermixers[index] == nullptr) return;


  filterMixerTransitions[index].currentGain[0] = 1.0;
  filterMixerTransitions[index].currentGain[1] = 0.0;
  filterMixerTransitions[index].currentGain[2] = 0.0;

  filterMixerTransitions[index].targetGain[0] = 1.0;
  filterMixerTransitions[index].targetGain[1] = 0.0;
  filterMixerTransitions[index].targetGain[2] = 0.0;

  for (int ch = 0; ch < NUM_MIXER_CHANNELS; ++ch) {
    filterMixerTransitions[index].step[ch] = 0.0;
    filterMixerTransitions[index].active[ch] = false;
  }
}


void forceAllMixerGainsToTarget() {
    extern const int ALL_CHANNELS[];
    extern const int NUM_ALL_CHANNELS;
    for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
        int idx = ALL_CHANNELS[i];
        if (filtermixers[idx] == nullptr) continue;

        for (int ch = 0; ch < NUM_MIXER_CHANNELS; ++ch) {
            filterMixerTransitions[idx].currentGain[ch] = filterMixerTransitions[idx].targetGain[ch];
            filterMixerTransitions[idx].active[ch] = false;
            filtermixers[idx]->gain(ch, filterMixerTransitions[idx].currentGain[ch]);
        }
    }
}

void applyVoiceCuts(int index) {
  if (index < 0 || index >= 15 || filters[index] == nullptr) return;
  float hcut = SMP.filter_settings[index][HCUT];
  float lcut = SMP.filter_settings[index][LOWCUT];
  float hz;
  if (lcut > 0.0f) {

    float s = constrain(lcut, 1.0f, (float)maxfilterResolution);
    hz = mapf(s, 1.0f, (float)maxfilterResolution, 40.0f, 2500.0f);
    setMixerGainSmooth(index, 0, 0.0, 32);
    setMixerGainSmooth(index, 1, 0.0, 32);
    setMixerGainSmooth(index, 2, 1.0, 32);
  } else {
    hz = mapf(constrain(hcut, 0.0f, (float)maxfilterResolution), 0.0f, (float)maxfilterResolution, 281.25f, 10000.0f);
    setMixerGainSmooth(index, 0, 1.0, 32);
    setMixerGainSmooth(index, 1, 0.0, 32);
    setMixerGainSmooth(index, 2, 0.0, 32);
  }
  filters[index]->frequency(hz);
}



static float voiceDelayMsForSlider(int slider) {
  float bpm = (SMP.bpm > 0.0f) ? SMP.bpm : 100.0f;
  float beatMs = 60000.0f / bpm;
  float quantumMs = beatMs / 32.0f;
  if (quantumMs < 1.0f) quantumMs = 1.0f;
  float bufMs = (float)(VOICE_DELAY_SAMPLES - AUDIO_BLOCK_SAMPLES) *
                1000.0f / AUDIO_SAMPLE_RATE_EXACT;
  int n = constrain(slider, 1, (int)maxfilterResolution);
  int nMax = (int)floorf(bufMs / quantumMs);
  if (nMax < 1) nMax = 1;
  if (n > nMax) n = nMax;
  return (float)n * quantumMs;
}

void applyVoiceDelay(int ch) {
  const int idx = voiceDelayIndex(ch);
  if (idx < 0) return;
  const int slider = constrain((int)SMP.param_settings[ch][DELAY], 0, (int)maxfilterResolution);
  if (slider <= 0) {
    voiceDelay[idx].disable(0);
    voiceDelayFb[idx].gain(1, 0.0f);
    voiceDelayMix[idx].gain(0, 0.0f);
    voiceDelayMix[idx].gain(3, 1.0f);
    return;
  }
  voiceDelay[idx].delay(0, voiceDelayMsForSlider(slider));
  voiceDelayFb[idx].gain(1, mapf((float)slider, 1.0f, (float)maxfilterResolution, 0.52f, 0.68f));
  voiceDelayMix[idx].gain(0, 0.50f);
  voiceDelayMix[idx].gain(3, 1.0f);
}

void applyAllVoiceDelays() {
  const int chans[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14 };
  for (int c : chans) applyVoiceDelay(c);
}

void applyMonoSynthCrf(int ch) {
  if (ch != 13 && ch != 14) return;
  if (!filters[ch]) return;
  uint8_t cut = (uint8_t)constrain((int)SMP.synth_settings[ch][CUTOFF], 0, (int)maxfilterResolution);
  uint8_t reso = (uint8_t)constrain((int)SMP.synth_settings[ch][RESONANCE], 0, (int)maxfilterResolution);
  uint8_t flt = (uint8_t)constrain((int)SMP.synth_settings[ch][FILTER], 0, (int)maxfilterResolution);
  filterSetting(ch, HCUT) = cut;
  filterSetting(ch, RES) = reso;
  applyVoiceCuts(ch);
  filters[ch]->resonance(mapf(reso, 0, maxfilterResolution, 0.7f, 5.0f));
  filters[ch]->octaveControl(mapf(flt, 0, maxfilterResolution, 0.0f, 7.0f));
}


void setFilters(FilterType filterType, int index, bool initial) {

  float mappedValue = 0.0;
  if (filterType == HCUT) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 281.25, 10000.0);
  if (filterType == LOWCUT) mappedValue = mapf(SMP.filter_settings[index][filterType], 1, maxfilterResolution, 40.0, 2500.0);




  if (filterType == SPEED) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.125, 8.0);
  if (filterType == PITCH) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 3000.0);
  if (filterType == OFFSET) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 3000.0);

  if (filterType == RES) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.7, 5.0);
  if (filterType == OCTAVE) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 7.0);
  if (filterType == REVERB) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0.0, 0.79);

  if (filterType == BITCRUSHER) mappedValue = SMP.filter_settings[index][filterType];

  if (filterType == ACTIVE) mappedValue = mapf(SMP.filter_settings[index][filterType], 0, maxfilterResolution, 0, 1);



  switch (filterType) {

    case HCUT:
    case LOWCUT:
      {
        applyVoiceCuts(index);
        if ((index == 13 || index == 14) && filterType == HCUT) {
          SMP.synth_settings[index][CUTOFF] = (unsigned int)filterSetting(index, HCUT);
        }
        break;
      }

       case RES:
      {
        filters[index]->resonance(mappedValue);
        if (index == 13 || index == 14) {
          SMP.synth_settings[index][RESONANCE] = (unsigned int)filterSetting(index, RES);
        }
        break;
      }

        case OCTAVE:
      {


        if (index >= 1 && index <= 8) {
          int raw = constrain((int)lroundf(SMP.filter_settings[index][OCTAVE]), 0, 48);
          channelOctave[index] = (float)(raw - 24);
        }

        break;
      }

      case DETUNE:
      {
        if (index >= 1 && index <= 8) {

          detune[index] = mapf(SMP.filter_settings[index][DETUNE], 0, maxfilterResolution, -1.0f, 1.0f);
        } else if (index >= 9 && index <= 12) {

          detune[index] = mapf(SMP.filter_settings[index][DETUNE], 0, maxfilterResolution, -12.0f, 12.0f);
        }
        break;
      }

      case FILTER_WAVEFORM:
      {

        if (index == 11) {


          extern void updateSynthVoice(int channel);
          updateSynthVoice(11);
        } else if (index >= 13 && index <= 14) {

          uint8_t waveformType =
              constrain((int)mapf(
                  filterSetting(index, FILTER_WAVEFORM),
                  0, 16, 0, 3), 0, 3) + 1;

          handleWaveformChange(index, waveformType);
        }
        break;
      }

    case REVERB:
      {
        if (freeverbs[index] != nullptr && freeverbs[index] != 0) {


          float verbAmount = mapf(mappedValue, 0.0f, 0.79f, 0.0f, 1.0f);
          verbAmount = constrain(verbAmount, 0.0f, 1.0f);

          float wetGain = mapf(verbAmount, 0.0f, 1.0f, 0.0f, 0.54f);
          wetGain = constrain(wetGain, 0.0f, 0.54f);

          float dryGain = mapf(verbAmount, 0.0f, 1.0f, 1.0f, 0.88f);
          dryGain = constrain(dryGain, 0.88f, 1.0f);

          if (freeverbmixers[index] != 0) {
            freeverbmixers[index]->gain(0, wetGain);
            freeverbmixers[index]->gain(3, dryGain);
          }
          if (index == 11) {



            synthmixer11.gain(0, CH11_SYNTHMIX_GAIN * verbAmount);
            synthmixer11.gain(3, CH11_SYNTHMIX_GAIN);
          }

          if (wetGain > 0.01f) {
            float dampingValue = mapf(verbAmount, 0.0f, 1.0f, 0.01f, 0.42f);
            dampingValue = constrain(dampingValue, 0.01f, 0.42f);
            freeverbs[index]->damping(dampingValue);
            float roomSize = mapf(verbAmount, 0.0f, 1.0f, 0.0f, 1.0f);
            freeverbs[index]->roomsize(roomSize);
          } else {

            freeverbs[index]->roomsize(0.0f);
            freeverbs[index]->damping(0.25f);
            if (index == 11) {
              synthmixer11.gain(0, 0.0f);
            }
          }
        }
        break;
      }
    case BITCRUSHER:
      {

        int slider = (int)SMP.filter_settings[index][BITCRUSHER];
        slider = constrain(slider, 0, (int)maxfilterResolution);

        float channelvolume = mapf(SMP.channelVol[index], 0, maxY, 0.0f, 1.0f);

        if (slider <= 0) {
          bitcrushers[index]->bits(16);
          bitcrushers[index]->sampleRate((float)AUDIO_SAMPLE_RATE_EXACT);
          if (amps[index] != nullptr) amps[index]->gain(channelvolume);
          break;
        }

        int mv = (int)round(mapf((float)slider, 1.0f, maxfilterResolution, 1.0f, 16.0f));
        mv = constrain(mv, 1, 16);



        const float crushAmt = mapf((float)mv, 1.0f, 16.0f, 0.0f, 0.80f);
        int bitDepth = (int)round(mapf(crushAmt, 0.0f, 1.0f, 16.0f, 1.0f));
        int xsampleRate = (int)round(mapf(crushAmt, 0.0f, 1.0f,
                                          (float)AUDIO_SAMPLE_RATE_EXACT, 1000.0f));
        bitDepth = constrain(bitDepth, 4, 16);
        xsampleRate = constrain(xsampleRate, 9600, (int)AUDIO_SAMPLE_RATE_EXACT);

        bitcrushers[index]->bits(bitDepth);
        bitcrushers[index]->sampleRate((float)xsampleRate);

        float crushCompGain = mapf((float)mv, 1.0f, 16.0f, max(channelvolume, 0.1f), 0.6f);
        if (amps[index] != nullptr) amps[index]->gain(crushCompGain);
        break;
      }
    case EFX:
      {
        break;
      }
    default:
      break;
  }
}





void setFilterDefaults(int channel) {

  AudioMixer4* mixer = nullptr;
  switch (channel) {
    case 1: mixer = &filtermixer1; break;
    case 2: mixer = &filtermixer2; break;
    case 3: mixer = &filtermixer3; break;
    case 4: mixer = &filtermixer4; break;
    case 5: mixer = &filtermixer5; break;
    case 6: mixer = &filtermixer6; break;
    case 7: mixer = &filtermixer7; break;
    case 8: mixer = &filtermixer8; break;
    case 11: mixer = &filtermixer11; break;
    case 13: mixer = &filtermixer13; break;
    case 14: mixer = &filtermixer14; break;
    default: return;
  }
  if (mixer) {
    mixer->gain(0, 1);
    for (int i = 1; i < 4; ++i) {
      mixer->gain(i, 0);
    }
  }

  SMP.filter_settings[channel][HCUT] = (channel == 14) ? 14 : 32;
  SMP.filter_settings[channel][LOWCUT] = 0;
  SMP.filter_settings[channel][REVERB] = 0;
  SMP.filter_settings[channel][BITCRUSHER] = 0;
  SMP.filter_settings[channel][RES] = (channel == 14) ? 5 : 0;
  SMP.filter_settings[channel][DETUNE] = 16;

  SMP.filter_settings[channel][OCTAVE] = (channel >= 1 && channel <= 8) ? 24 : 16;
  filterSetting(channel, FILTER_WAVEFORM) = 8;

  SMP.param_settings[channel][DELAY] = 0;
  SMP.param_settings[channel][ATTACK] = 32;
  SMP.param_settings[channel][DECAY] = (channel == 14) ? 20 : 32;
  SMP.param_settings[channel][SUSTAIN] = (channel == 14) ? 20 : 32;
  SMP.param_settings[channel][RELEASE] = (channel == 11 || channel == 13 || channel == 14) ? 16 : 0;
  if (channel == 11) {
    extern void applySynthInstrumentPreset(int channel, int instrumentIdx);
    extern void updateSynthVoice(int channel);
    applySynthInstrumentPreset(11, 12);
    SMP.synth_settings[11][LFO_RATE] = 0;
    SMP.synth_settings[11][LFO_DEPTH] = 0;
    SMP.synth_settings[11][LFO_PHASE] = 0;
    SMP.synth_settings[11][ARP_STEP] = 16;
    updateSynthVoice(11);
  } else if (channel == 13 || channel == 14) {
    SMP.synth_settings[channel][CUTOFF] = (channel == 13) ? 32 : 14;
    SMP.synth_settings[channel][RESONANCE] = (channel == 14) ? 5 : 0;
    SMP.synth_settings[channel][FILTER] = (channel == 14) ? 8 : 0;
    SMP.synth_settings[channel][CENT] = (channel == 13) ? 0 : 8;
    SMP.synth_settings[channel][SEMI] = 16;
    SMP.synth_settings[channel][LFO_RATE] = 0;
    SMP.synth_settings[channel][LFO_DEPTH] = 0;
    SMP.synth_settings[channel][LFO_PHASE] = 0;
    SMP.synth_settings[channel][ARP_STEP] = 16;
  }
  if (channelHasVoiceDelay(channel)) {
    applyVoiceDelay(channel);
  }


  if (channel >= 1 && channel <= 12) {
    detune[channel] = 0.0;
  }


  if (channel >= 1 && channel <= 8) {
    channelOctave[channel] = 0.0;
  }

  SMP.filter_settings[channel][EFX] = 0;
  if (channel == 13 || channel == 14) {
    applyMonoSynthCrf(channel);
    setParams(ATTACK, channel);
    setParams(DECAY, channel);
    setParams(SUSTAIN, channel);
    setParams(RELEASE, channel);
  } else {
    filters[channel]->resonance(0.0);
    filters[channel]->frequency(10000);
  }

  if (freeverbs[channel] != nullptr) {
    freeverbs[channel]->roomsize(0);
    freeverbs[channel]->damping(0);
  }

  if (freeverbmixers[channel] != nullptr) {
    freeverbmixers[channel]->gain(0, 0);
    freeverbmixers[channel]->gain(3, 1);
  }
  bitcrushers[channel]->bits(16);
  bitcrushers[channel]->sampleRate((int)AUDIO_SAMPLE_RATE_EXACT);


  initFilterTransition(channel);
}
# 1 "/Users/jank/git/toern/toern_filterUI.ino"
extern const uint8_t filterPageCount[NUM_CHANNELS];
extern uint8_t filterPage[NUM_CHANNELS];


void applySynthInstrumentPreset(int channel, int instrumentIdx);

static bool channelHasFreeverb(uint8_t chan) {
  return !(chan == 3 || chan == 4);
}

DMAMEM unsigned long lastEncoderChange[4] = { 0, 0, 0, 0 };

int8_t lastChangedEncoder = -1;
DMAMEM unsigned long lastInteraction = millis();
const unsigned long FILTER_INTERACTION_TIMEOUT = 1000;

DMAMEM bool showSingleFilter[4] = { false, false, false, false };

static inline bool isWideFilterView2B() {
  extern int ledModules;
  extern bool ledModulesRotated;
  return (ledModules == 2 && ledModulesRotated && maxX > MATRIX_WIDTH);
}

static inline void getFilterSliderCols(uint8_t sliderIndex, uint8_t &x0, uint8_t &x1) {
  if (isWideFilterView2B()) {


    static const uint8_t wideCols[4][2] = {
      { 5, 6 }, { 12, 13 }, { 20, 21 }, { 27, 28 }
    };
    uint8_t idx = (sliderIndex < 4) ? sliderIndex : 3;
    x0 = wideCols[idx][0];
    x1 = wideCols[idx][1];
  } else {
    x0 = sliderCols[sliderIndex][0];
    x1 = sliderCols[sliderIndex][1];
  }

  if (x0 < 1) x0 = 1;
  if (x1 < x0) x1 = x0;
  if (x1 > maxX) x1 = maxX;
}

uint8_t scaleToDisplay(const SliderDefEntry& meta, uint8_t val) {
  return (meta.displayRange < meta.maxValue) ? constrain(mapf(val, 0, meta.maxValue, 0, meta.displayRange - 1), 0, meta.displayRange - 1) : val;
}


static inline bool isBipolarCenteredParam(const SliderDefEntry& meta) {
  if (meta.arr == ARR_FILTER && meta.idx == DETUNE && meta.maxValue == 32) return true;
  if (meta.arr == ARR_FILTER && meta.idx == OCTAVE && (meta.maxValue == 48 || meta.maxValue == 32)) return true;
  if (meta.arr == ARR_SYNTH && (meta.idx == CENT || meta.idx == SEMI) && meta.maxValue == 32) return true;
  if (meta.arr == ARR_SYNTH && meta.idx == ARP_STEP && meta.maxValue == 32) return true;
  return false;
}

static inline int bipolarCenter(const SliderDefEntry& meta) {
  if (meta.arr == ARR_FILTER && meta.idx == OCTAVE) return (int)meta.maxValue / 2;
  return 16;
}

#define MAX_FILTER_RESOLUTION 32.0
#define FILTER_BYPASS MAX_FILTER_RESOLUTION
#define FILTER_WET 0
#define FILTER_MID (MAX_FILTER_RESOLUTION / 2)
#define PARAM_COUNT 6


uint8_t readSetting(SettingArray arr, int8_t idx, uint8_t chan) {

  uint8_t raw = 0;
  switch (arr) {
    case ARR_FILTER: {
      const int hi = (idx == OCTAVE && chan >= 1 && chan <= 8) ? 48 : (int)MAX_FILTER_RESOLUTION;
      raw = constrain((int)lroundf(filterSetting(chan, idx)), 0, hi);
      break;
    }
    case ARR_SYNTH: raw = constrain(SMP.synth_settings[chan][idx], 0, MAX_FILTER_RESOLUTION); break;
    case ARR_PARAM: raw = constrain(SMP.param_settings[chan][idx], 0, MAX_FILTER_RESOLUTION); break;
    case ARR_STRETCH: raw = chan >= 1 && chan <= 8 ? sampleTimeStretch[chan] : 0; break;
    default: return 0;
  }
  return raw;
}


void drawSliderName(uint8_t x0, uint8_t x1, const char* name, CRGB filtercol) {
# 98 "/Users/jank/git/toern/toern_filterUI.ino"
  for (int x = 1; x <= (int)maxX; ++x) {
    light(x, 12, CRGB::Black);
    light(x, 13, CRGB::Black);
    light(x, 14, CRGB::Black);
    light(x, 15, CRGB::Black);
    light(x, 16, CRGB::Black);
  }

  int textPixelWidth = 0;
  for (int i = 0; name[i] != '\0'; ++i) {
    if (name[i] >= 32 && name[i] <= 126) {
      textPixelWidth += alphabet[name[i] - 32][0] + 1;
    }
  }


  int tx = ((int)maxX - textPixelWidth + 1) / 2;
  if (tx < 1) tx = 1;
  if (tx + textPixelWidth - 1 > (int)maxX) tx = (int)maxX - textPixelWidth + 1;
  if (tx < 1) tx = 1;
  drawText(name, tx, 12, filtercol);
}

void drawCornerValueCustom(uint8_t encoderIndex, uint8_t val, const SliderDefEntry& meta) {

  uint8_t chan = GLOB.currentChannel;
  uint8_t x;

  bool dualPanel = isWideFilterView2B();

  if (dualPanel) {


    uint8_t sx0, sx1;
    getFilterSliderCols(encoderIndex, sx0, sx1);
    int desiredX = (encoderIndex < 2) ? ((int)sx1 + 4) : ((int)sx0 - 10);
    x = (uint8_t)constrain(desiredX, 1, (int)maxX - 6);
  } else if (chan == 11 && encoderIndex == 0 && meta.arr == ARR_FILTER && meta.idx == FILTER_WAVEFORM) {
    x = 6;
  } else {
    x = (encoderIndex < 2) ? 10 : 2;
  }
  const uint8_t y = 5;
  const uint8_t w = 6, h = 7;


  for (uint8_t dx = 0; dx <= w; ++dx) {
    for (uint8_t dy = 0; dy < h; ++dy) {
      light(x + dx, y + dy - 1, CRGB::Black);
    }
  }

  char buf[6];
  uint8_t shownVal = scaleToDisplay(meta, val);
  const bool bipolar = isBipolarCenteredParam(meta);

  if (meta.arr == ARR_STRETCH) {
    if (val == 11) snprintf(buf, sizeof(buf), "OFF");
    else if (val < 11) snprintf(buf, sizeof(buf), "-%u%%", 10 - val);
    else snprintf(buf, sizeof(buf), "+%u%%", val - 12);
  } else if (meta.displayMode == DISPLAY_ENUM && shownVal < meta.displayRange && meta.enumNames && meta.enumNames[shownVal]) {
    snprintf(buf, sizeof(buf), "%s", meta.enumNames[shownVal]);
  } else if (bipolar) {

    if (meta.arr == ARR_FILTER && meta.idx == DETUNE) {
      int cents = (int)lroundf(mapf((float)val, 0.0f, 32.0f, -100.0f, 100.0f));
      snprintf(buf, sizeof(buf), "%d", cents);
    } else if (meta.arr == ARR_FILTER && meta.idx == OCTAVE) {
      snprintf(buf, sizeof(buf), "%d", (int)val - bipolarCenter(meta));
    } else if (meta.arr == ARR_SYNTH && meta.idx == ARP_STEP) {
      int off = (int)val - 16;
      if (off == 0) snprintf(buf, sizeof(buf), "OFF");
      else snprintf(buf, sizeof(buf), "%+d", off);
    } else {
      snprintf(buf, sizeof(buf), "%d", (int)val - 16);
    }
  } else {
    snprintf(buf, sizeof(buf), "%u", shownVal);
  }


  CRGB baseColor = filterColors[filterPage[chan]][encoderIndex];
  CRGB dimmed = baseColor;
  dimmed.nscale8(64);
  uint8_t blendVal = mapf(val, 0, meta.maxValue, 0, 255);

  const bool dangerHighFlip =
      (meta.arr == ARR_FILTER && (meta.idx == RES || meta.idx == BITCRUSHER))
      || (meta.arr == ARR_SYNTH && meta.idx == RESONANCE);
  CRGB textColor = dangerHighFlip ? blend(CRGB::Green, CRGB::Red, blendVal)
                                  : blend(CRGB::Red, CRGB::Green, blendVal);

  if (meta.arr == ARR_STRETCH || bipolar) {

    int width = -1;
    for (const char *p = buf; *p; ++p) width += alphabet[*p - 32][0] + 1;
    x = constrain((int)x, 1, max(1, (int)maxX - width + 1));
    for (int px = x; px < x + width && px <= maxX; ++px)
      for (uint8_t py = y - 1; py < y + h - 1; ++py) light(px, py, CRGB::Black);
    if (meta.arr == ARR_STRETCH) {
      textColor = val == 11 ? CRGB(100,100,100) : CRGB::Green;
    } else if (meta.arr == ARR_FILTER && meta.idx == OCTAVE) {

      const int semis = (int)val - bipolarCenter(meta);
      if (semis == 0 || semis == 12 || semis == -12 || semis == 24 || semis == -24)
        textColor = CRGB::White;
    } else if ((int)val == bipolarCenter(meta)) {
      textColor = CRGB::White;
    }
  }
  drawText(buf, x, y, textColor);
}

void drawVerticalSlider(uint8_t x0, uint8_t x1, uint8_t val, uint8_t maxVal, CRGB baseColor, bool lowHighSlider = false, uint8_t sliderIndex = 0, uint8_t page = 0, bool switchtype = false) {
  const uint8_t displayResolution = 10;
  uint8_t displayVal;
  if (switchtype) {
    displayVal = (val == 0 ? 1 : displayResolution);
  } else {
    displayVal = mapf(val, 0, maxVal, 1, displayResolution);
    displayVal = constrain(displayVal, 1, displayResolution);
  }


  uint8_t blendVal = mapf(val, 0, maxVal, 0, 255);
  CRGB color = blend(CRGB::Black, baseColor, blendVal);


  bool isDefaultFast = false;
  uint8_t chan = GLOB.currentChannel;
  if (chan < NUM_CHANNELS) {
    if (sliderDef[chan][page][sliderIndex].arr == defaultFastFilter[chan].arr &&
        sliderDef[chan][page][sliderIndex].idx == defaultFastFilter[chan].idx) {
      isDefaultFast = true;
    }
  }


  bool isDetuneOctaveAtMiddle = false;
  if (chan < NUM_CHANNELS) {
    if (sliderDef[chan][page][sliderIndex].arr == ARR_STRETCH && val == 11) {
      isDetuneOctaveAtMiddle = true;
    }
    const auto &metaMid = sliderDef[chan][page][sliderIndex];
    if (isBipolarCenteredParam(metaMid) && (int)val == bipolarCenter(metaMid)) {
      isDetuneOctaveAtMiddle = true;
    }
  }

  for (uint8_t y = 1; y <= displayResolution; ++y) {
    CRGB c;
    if (y == displayVal) {
      if (isDefaultFast) {
        c = CRGB(255, 255, 0);
      } else {
        c = CRGB::White;
      }
    } else if (switchtype) {

      c = (y < displayVal) ? CRGB::Green : CRGB::Red;
    } else if (lowHighSlider) {
      c = (y < displayVal) ? CRGB(148, 0, 211) : CRGB::Red;
    } else {

      if (isDetuneOctaveAtMiddle && y == 1) {
        c = CRGB::Green;
      } else {
        c = (y < displayVal) ? color : CRGB::Black;
      }
    }

    light(x0, y, c);
    light(x1, y, c);
  }
}

void initSliders(uint8_t page, uint8_t chan) {
  page = filterPage[chan];
  showFilterNames(chan);

  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[chan][page][i];
    if (d.arr == ARR_NONE && d.idx == -1) {
      currentMode->pos[i] = 0;
      Encoder[i].writeMin((int32_t)0);
      Encoder[i].writeMax((int32_t)0);
      Encoder[i].writeCounter((int32_t)0);
      continue;
    }
    const auto& meta = sliderDef[chan][page][i];
    uint8_t val = 0;

    if (d.arr != ARR_NONE) {
      if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) {
        continue;
      }

      val = readSetting(d.arr, d.idx, chan);


    }

    val = constrain(val, 0, meta.maxValue);
    currentMode->pos[i] = val;
    Encoder[i].writeMin((int32_t)0);
    Encoder[i].writeMax((int32_t)(meta.maxValue));
    Encoder[i].writeCounter((int32_t)val);
  }

  showFilterPages(chan);
}

void slider(uint8_t page) {
  unsigned long now = millis();
  bool activeInteraction = (now - lastInteraction < FILTER_INTERACTION_TIMEOUT);
  if (!activeInteraction && lastChangedEncoder != -1) {
    lastChangedEncoder = -1;
    for (uint8_t i = 0; i < 4; ++i) {
      showSingleFilter[i] = false;
    }
  }
  uint8_t chan = GLOB.currentChannel;
  for (uint8_t i = 0; i < 4; ++i) {
    const auto& def = sliderDef[chan][page][i];
    if (def.arr == ARR_NONE && def.idx == -1) continue;
    const auto& meta = sliderDef[chan][page][i];
    uint8_t val = constrain(currentMode->pos[i], 0, meta.maxValue);

    bool isLowHigh = false;
    bool switchtype = (def.arr == ARR_FILTER && def.idx == EFX) || (def.arr == ARR_FILTER && def.idx == ACTIVE);

    if (activeInteraction && i != lastChangedEncoder) {

      for (uint8_t y = 1; y <= 10; ++y) {
        uint8_t x0, x1;
        getFilterSliderCols(i, x0, x1);
        light(x0, y, CRGB::Black);
        light(x1, y, CRGB::Black);
      }
      continue;
    }



    CRGB sliderColor = filterColors[page][i];
    bool reverbDisabled = (def.arr == ARR_FILTER && def.idx == REVERB && !channelHasFreeverb(chan));
    if (reverbDisabled) {
      sliderColor = CRGB(4, 4, 4);
    }

    uint8_t x0, x1;
    getFilterSliderCols(i, x0, x1);

    drawVerticalSlider(
      x0,
      x1,
      val,
      meta.maxValue,
      sliderColor,
      isLowHigh,
      i,
      page,
      switchtype
    );
    showSingleFilter[i] = false;
    if (i == lastChangedEncoder && activeInteraction) {
      drawSliderName(x0,
                     x1,
                     def.name,
                     sliderColor);
      showSingleFilter[i] = true;
    }
  }


  bool allFalse = true;
  for (int i = 0; i < 4; ++i) {
    if (showSingleFilter[i]) {
      allFalse = false;
      break;
    }
  }

  if (allFalse) {
    showFilterNames(GLOB.currentChannel);
  }
}

void toggleDefaultFilterFromSlider(uint8_t page, uint8_t encoder) {
  auto& def = sliderDef[GLOB.currentChannel][page][encoder];
  uint8_t chan = GLOB.currentChannel;


  bool isCurrentlyActive = (defaultFastFilter[chan].arr == def.arr &&
                           defaultFastFilter[chan].idx == def.idx);

  if (isCurrentlyActive) {

    defaultFastFilter[chan].arr = ARR_NONE;
    defaultFastFilter[chan].idx = -1;
  } else {

    switch (def.arr) {
      case ARR_FILTER:
        defaultFastFilter[chan].arr = def.arr;
        defaultFastFilter[chan].idx = def.idx;
        break;

      case ARR_SYNTH:
        defaultFastFilter[chan].arr = def.arr;
        defaultFastFilter[chan].idx = def.idx;
        break;

      case ARR_PARAM:
        if (def.idx < PARAM_COUNT) {
          defaultFastFilter[chan].arr = def.arr;
          defaultFastFilter[chan].idx = def.idx;
        } else {
          return;
        }
        break;

      default:
        return;
    }
  }

  updateFilterEncoderColors();
}

void processAdjustments_new(uint8_t page) {
  uint8_t chan = GLOB.currentChannel;
  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[chan][page][i];
    if (d.arr == ARR_NONE && d.idx == -1) continue;
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) {
      continue;
    }

    const auto& meta = sliderDef[chan][page][i];
    uint8_t rawVal = constrain(currentMode->pos[i], 0, meta.maxValue);
    uint8_t val = scaleToDisplay(meta, rawVal);
    uint8_t prev = readSetting(d.arr, d.idx, chan);

    if (val == prev) continue;

    switch (d.arr) {
      case ARR_FILTER:
          filterSetting(chan, d.idx) = currentMode->pos[i];
          setFilters(d.idx, chan, false);
        break;

      case ARR_SYNTH:
        SMP.synth_settings[chan][d.idx] = currentMode->pos[i];
        if (chan == 11 && d.idx == INSTRUMENT) {
          applySynthInstrumentPreset(chan, (int)constrain(currentMode->pos[i], 0, 12));
          initSliders(filterPage[chan], chan);
        }
        if ((chan == 13 || chan == 14) &&
            (d.idx == CUTOFF || d.idx == RESONANCE || d.idx == FILTER)) {
          extern void applyMonoSynthCrf(int ch);
          applyMonoSynthCrf(chan);
        }
        break;

      case ARR_PARAM:
        SMP.param_settings[chan][d.idx] = currentMode->pos[i];
        setParams(d.idx, chan);
        break;

      case ARR_STRETCH:
        setSampleTimeStretch(chan, currentMode->pos[i]);
        break;

      default:
        break;
    }

  }

  if (GLOB.currentChannel == 11) updateSynthVoice(11);

}

extern bool readTouch2Pressed(void);


void setNewFilters() {
  static bool lastTouch = false;
  bool currTouch = readTouch2Pressed();
  uint8_t chan = GLOB.currentChannel;


  if (chan >= NUM_CHANNELS || filterPageCount[chan] == 0) {
    lastTouch = currTouch;
    return;
  }

  static uint8_t lastFilterPage[NUM_CHANNELS] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
  static uint8_t lastEncoderPos[4] = {255, 255, 255, 255};
  static uint8_t lastChannel = 255;
  static unsigned long lastRedrawTime = 0;

  const unsigned long REDRAW_THROTTLE_MS = 50;


  bool channelChanged = (chan != lastChannel);
  if (channelChanged) {
    lastChannel = chan;

    for (uint8_t i = 0; i < 4; ++i) {
      lastEncoderPos[i] = 255;
    }
  }

  bool pageChanged = (filterPage[chan] != lastFilterPage[chan]);
  if (currTouch && !lastTouch) {

    filterPage[chan] = (filterPage[chan] + 1) % filterPageCount[chan];
    filterDrawActive = false;
    filterDrawEndTime=0;
    initSliders(filterPage[chan], chan);


    updateFilterEncoderColors();
    pageChanged = true;
    lastFilterPage[chan] = filterPage[chan];
  }
  lastTouch = currTouch;

  bool anyChanged = false;
  bool encoderPosChanged = false;
  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[chan][filterPage[chan]][i];
    if (d.arr == ARR_NONE && d.idx == -1) continue;
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) continue;
    uint8_t val = constrain(currentMode->pos[i], 0, d.maxValue);
    uint8_t prev = readSetting(d.arr, d.idx, chan);


    if (currentMode->pos[i] != lastEncoderPos[i]) {
      encoderPosChanged = true;
      lastEncoderPos[i] = currentMode->pos[i];
    }

    if (val != prev) {
      showSingleFilter[i] = true;

      uint8_t x0, x1;
      getFilterSliderCols(i, x0, x1);
      drawSliderName(x0,
             x1,
                     d.name,
                     filterColors[filterPage[chan]][i]);

      anyChanged = true;
      lastEncoderChange[i] = millis();
      lastInteraction = millis();
      lastChangedEncoder = i;
    }
  }


  unsigned long now = millis();
  bool timeoutExpired = (lastInteraction > 0 && (now - lastInteraction) >= FILTER_INTERACTION_TIMEOUT);
  bool needsTimeoutCheck = (lastInteraction > 0 && !timeoutExpired);

  if (anyChanged) {
    processAdjustments_new(filterPage[chan]);
    showFilterPages(chan);
    lastRedrawTime = millis();
  } else if (pageChanged || encoderPosChanged || channelChanged || timeoutExpired) {


    if (timeoutExpired || (now - lastRedrawTime >= REDRAW_THROTTLE_MS)) {
      showFilterPages(chan);
      lastRedrawTime = now;
      if (pageChanged) {
        lastFilterPage[chan] = filterPage[chan];
      }
    }
  } else if (needsTimeoutCheck) {


    if (now - lastRedrawTime >= REDRAW_THROTTLE_MS) {
      showFilterPages(chan);
      lastRedrawTime = now;
    }
  }
}

void showFilterPages(uint8_t chan) {
  FastLEDclear();


  slider(filterPage[chan]);

  unsigned long now = millis();



  if (now - lastInteraction < FILTER_INTERACTION_TIMEOUT && lastChangedEncoder >= 0 && lastChangedEncoder < 4) {
    uint8_t val = constrain(currentMode->pos[lastChangedEncoder],
                            0, sliderDef[chan][filterPage[chan]][lastChangedEncoder].maxValue);
    drawCornerValueCustom(lastChangedEncoder, val, sliderDef[chan][filterPage[chan]][lastChangedEncoder]);
  }

  FastLEDshow();
}

void showFilterNames(uint8_t chan) {

  for (uint8_t x = 1; x <= maxX; ++x) {
        light(x, 12, CRGB::Black);
        light(x, 13, CRGB::Black);
        light(x, 14, CRGB::Black);
        light(x, 15, CRGB::Black);
        light(x, 16, CRGB::Black);
    }

    uint8_t page = filterPage[chan];
    for (uint8_t i = 0; i < 4; ++i) {
        const SliderDefEntry& def = sliderDef[chan][page][i];
        if (def.arr == ARR_NONE && def.idx == -1) continue;
        char abbrev[2] = { def.name[0], '\0' };
        CRGB color = filterColors[page][i];
        if (def.arr == ARR_FILTER && def.idx == REVERB && !channelHasFreeverb(chan)) {
          color = CRGB(4, 4, 4);
        }
        uint8_t ax, ax1;
        getFilterSliderCols(i, ax, ax1);
        if (ax > maxX) ax = maxX;
        drawText(abbrev, ax, 12, color);
    }
}

extern SliderDefEntry sliderDef[NUM_CHANNELS][5][4];
extern uint8_t filterPage[NUM_CHANNELS];

void updateFilterEncoderColors() {
  if (currentMode == &filterMode) {
    uint8_t chan = GLOB.currentChannel;
    uint8_t page = filterPage[chan];

    for (int i = 0; i < NUM_ENCODERS; i++) {
      CRGB color;


      bool isDefaultFast = false;
      if (chan < NUM_CHANNELS) {
        if (sliderDef[chan][page][i].arr == defaultFastFilter[chan].arr &&
            sliderDef[chan][page][i].idx == defaultFastFilter[chan].idx) {
          isDefaultFast = true;
        }
      }

      bool reverbDisabled = (sliderDef[chan][page][i].arr == ARR_FILTER &&
                             sliderDef[chan][page][i].idx == REVERB &&
                             !channelHasFreeverb(chan));

      if (isDefaultFast) {
        color = CRGB(255, 255, 0);
        if (reverbDisabled) {
          color = CRGB(16, 16, 0);
        }
      } else {
        color = filterColors[page][i];
        if (reverbDisabled) {
          color = CRGB(8, 8, 8);
        }
      }

      Encoder[i].writeRGBCode(CRGBToUint32(color));
    }
  }
}
# 1 "/Users/jank/git/toern/toern_fn.ino"

static int8_t fnDirection = 0;
static unsigned long fnDirectionUntil = 0;

static void updateFnState() {
  fnHeld = currentMode != &touchyMode && currentMode != &recordMode && readDebouncedTouch(4);
  if (fnHeld) noteUserActivity();
}



static uint8_t fnButtonEvent(int index, uint8_t physicalState) {
  if (currentMode == &touchyMode || currentMode == &recordMode) return physicalState;
  if (physicalState == 2 && fnHeld) fnButtonHeld[index] = true;
  const bool modified = fnButtonHeld[index] || fnHeld;
  uint8_t event = physicalState;
  if (modified) {
    if (physicalState == 1) event = 4;
    else if (physicalState == 2) event = 3;
    else if (physicalState == 9) event = 8;
  }
  if (physicalState == 1 || physicalState == 9) fnButtonHeld[index] = false;
  return event;
}

static bool dispatchFnTurn(const EncoderTurnEvent &event) {
  if (!event.fn || event.encoder != 0 || event.delta == 0) return false;
  const uint8_t channel = GLOB.currentChannel;
  if (fastRecordActive || isRecording || channel < 1 || channel > 8 || loadedSampleLen[channel] <= 1) return false;
  fnDirection = event.delta < 0 ? -1 : 1;
  applyChannelDirection(channel, fnDirection);
  fnDirectionUntil = millis() + 350;
  return true;
}

static bool handleFnEncoder(int index, int &rawValue, bool allowWrite) {
  static Mode *pendingRestore[NUM_ENCODERS] = {};
  if (pendingRestore[index] && pendingRestore[index] != currentMode) pendingRestore[index] = nullptr;
  if (currentMode == &touchyMode || currentMode == &recordMode || (!fnHeld && !pendingRestore[index])) return false;
  const int previous = (currentMode == &subpatternMode && muteModeActive && index == 0)
      ? 0 : (int)currentMode->pos[index];
  if (fnHeld) {
    const uint8_t status = Encoder[index].readStatus();
    const uint8_t rotationBits = i2cEncoderLibV2::RINC | i2cEncoderLibV2::RDEC |
        i2cEncoderLibV2::RMAX | i2cEncoderLibV2::RMIN;
    if (status != 0xFF && (status & rotationBits)) {
      int delta = rawValue - previous;
      if (delta == 0) {
        const bool right = status & (i2cEncoderLibV2::RINC | i2cEncoderLibV2::RMAX);
        const bool left = status & (i2cEncoderLibV2::RDEC | i2cEncoderLibV2::RMIN);
        if (right != left) delta = right ? 1 : -1;
      }
      const EncoderTurnEvent event = {index, delta, true};
      dispatchFnTurn(event);
    }
  }


  if (rawValue != previous) {
    if (allowWrite) {
      Encoder[index].writeCounter((int32_t)previous);
      pendingRestore[index] = nullptr;
    } else pendingRestore[index] = currentMode;
  } else pendingRestore[index] = nullptr;
  rawValue = currentMode->pos[index];
  return true;
}
# 1 "/Users/jank/git/toern/toern_helpers.ino"


void generateRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBassOrMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBassLine(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateMainMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBasicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
int createHarmonicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
int createMelodicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
int createRhythmicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony);
void generateSong();
void generateGenreTrack();
void setGenreBPM();
void applyBPMDirectly(int bpm);
void generateTechnoPattern(unsigned int start, unsigned int end, unsigned int page);
void generateHipHopPattern(unsigned int start, unsigned int end, unsigned int page);
void generateDnBPattern(unsigned int start, unsigned int end, unsigned int page);
void generateHousePattern(unsigned int start, unsigned int end, unsigned int page);
void generateAmbientPattern(unsigned int start, unsigned int end, unsigned int page);


static inline uint8_t clampVel(int v) {
  if (v < 1) return 1;
  if (v > 127) return 127;
  return (uint8_t)v;
}

static inline void placeGenNote(unsigned int c, int row, uint8_t ch, int vel, uint8_t prob = 100, uint8_t cond = 1) {

  note[c][row].channel = ch;
  note[c][row].velocity = clampVel(vel);
  note[c][row].probability = prob;
  note[c][row].condition = cond;
  note[c][row].midiPitch = NOTE_MIDI_PITCH_NONE;
}


static inline bool placeRandomNoteIfEmpty(unsigned int c, int row, uint8_t ch, int vel) {
  if (row < 1 || row > 16) return false;
  if (note[c][row].channel != 0) return false;
  placeGenNote(c, row, ch, vel, 100, 1);
  return true;
}


extern int genreType;
extern int genreLength;


extern Device SMP;
extern GlobalVars GLOB;
extern Mode* currentMode;
extern i2cEncoderLibV2 Encoder[];
extern Mode bpmMode;
extern IntervalTimer playTimer;
extern double playNoteInterval;
extern float detune[13];
extern float channelOctave[9];
extern int8_t channelDirection[maxFiles];
extern unsigned int recordingStartBeat;
extern unsigned int RefreshTime;




#define SAMPLE_BROWSER_PATH_MAX 128
#define BROWSE_MAX_TMP 256
#define SAMPLE_BROWSER_MAX_ENTRIES 384
#define SAMPLE_BROWSER_ENTRY_PARENT 0
#define SAMPLE_BROWSER_ENTRY_DIR 1
#define SAMPLE_BROWSER_ENTRY_FILE 2

#define TOERN_WAV_INDEX_FILE "samples/toern_wavs.txt"
#define TOERN_WAV_INDEX_MAX 2048
#define TOERN_WAV_INDEX_PATH 96
#define TOERN_WAV_MIN_BYTES 44
#define TOERN_WAV_SCAN_DIR_STACK 64
#define SOLO_RANDOM_PLAYLIST_SIZE 32

EXTMEM char sbTmpDirs[BROWSE_MAX_TMP][SAMPLE_BROWSER_NAME_MAX];
EXTMEM char sbTmpWavs[BROWSE_MAX_TMP][SAMPLE_BROWSER_NAME_MAX];
EXTMEM uint32_t sbTmpWavSizes[BROWSE_MAX_TMP];

bool manifestLoaded = false;
uint16_t manifestFolderCount = 0;

EXTMEM char g_wavIndex[TOERN_WAV_INDEX_MAX][TOERN_WAV_INDEX_PATH];
uint16_t g_wavIndexCount = 0;
bool g_wavIndexLoaded = false;
EXTMEM char g_wavScanDirStack[TOERN_WAV_SCAN_DIR_STACK][TOERN_WAV_INDEX_PATH];

uint16_t g_soloRandomPlaylist[SOLO_RANDOM_PLAYLIST_SIZE];
uint16_t g_soloRandomPlaylistCount = 0;
int16_t g_soloRandomPlaylistIndex = 0;
static char g_soloRandomLastRel[TOERN_WAV_INDEX_PATH] = {0};
static bool g_soloRandomListDirty = true;

EXTMEM char g_browseDir[maxFiles][SAMPLE_BROWSER_PATH_MAX];

EXTMEM char g_folderPickName[BROWSE_MAX_TMP][SAMPLE_BROWSER_NAME_MAX];
EXTMEM char g_wavPickName[BROWSE_MAX_TMP][SAMPLE_BROWSER_NAME_MAX];
EXTMEM uint8_t g_wavPickType[BROWSE_MAX_TMP];
EXTMEM uint32_t g_wavPickSize[BROWSE_MAX_TMP];
uint16_t g_folderPickCount = 0;
uint16_t g_wavPickCount = 0;
int g_wavFileCount = 0;
bool g_browseListDirty = true;

bool g_suppressNextWavPreviewAfterFolderNav = false;

unsigned long g_setWavIgnoreLoadUntilMs = 0;

unsigned int sampleBrowserHashName(const char* s) {
  unsigned int h = 5381;
  if (!s) return h;
  while (*s) h = ((h << 5) + h) + (unsigned char)(*s++);
  return h;
}

static bool hasWavExt(const char* name) {
  int len = strlen(name);
  if (len < 5) return false;
  const char* ext = name + len - 4;
  return (ext[0] == '.' || ext[0] == '.') && (tolower(ext[1]) == 'w') && (tolower(ext[2]) == 'a') && (tolower(ext[3]) == 'v');
}

static void copyBaseName64(const char* rawName, char* out, size_t outSz) {
  const char* base = rawName ? rawName : "";
  const char* slash = strrchr(base, '/');
  if (slash) base = slash + 1;
  strncpy(out, base, outSz - 1);
  out[outSz - 1] = 0;
}

static bool soloRandomIsJunkName(const char* base) {
  if (!base || !base[0] || base[0] == '.') return true;
  if (strcasecmp(base, "System Volume Information") == 0) return true;
  if (strcasecmp(base, "TRASH") == 0) return true;
  if (strcasecmp(base, "$RECYCLE.BIN") == 0) return true;
  if (strcasecmp(base, "FOUND.000") == 0) return true;
  return false;
}

static void soloRandomJoinRel(char* out, size_t outSz, const char* dir, const char* name) {
  if (dir && dir[0]) snprintf(out, outSz, "%s/%s", dir, name);
  else snprintf(out, outSz, "%s", name ? name : "");
}

static void soloRandomSetLastFromIndex() {
  g_soloRandomLastRel[0] = 0;
  if (g_soloRandomPlaylistCount == 0) return;
  if (g_soloRandomPlaylistIndex < 0) g_soloRandomPlaylistIndex = 0;
  if (g_soloRandomPlaylistIndex >= (int16_t)g_soloRandomPlaylistCount) {
    g_soloRandomPlaylistIndex = (int16_t)g_soloRandomPlaylistCount - 1;
  }
  uint16_t wi = g_soloRandomPlaylist[g_soloRandomPlaylistIndex];
  if (wi >= g_wavIndexCount) return;
  strncpy(g_soloRandomLastRel, g_wavIndex[wi], sizeof(g_soloRandomLastRel) - 1);
  g_soloRandomLastRel[sizeof(g_soloRandomLastRel) - 1] = 0;
}



bool soloRandomBuildPlayPath(const char* stored, char* out, size_t outSz) {
  if (!out || outSz == 0) return false;
  out[0] = 0;
  if (!stored || !stored[0]) return false;
  if (stored[0] == '*') {
    strncpy(out, stored + 1, outSz - 1);
  } else {
    strncpy(out, stored, outSz - 1);
  }
  out[outSz - 1] = 0;
  return out[0] != 0;
}

void soloRandomCommitPathToChannel(int ch, const char* fullSdPath) {
  if (ch < 1 || ch >= maxFiles || !fullSdPath || !fullSdPath[0]) return;
  if (strncmp(fullSdPath, "samples/", 8) == 0) {
    strncpy(SMP.samplePathRel[ch], fullSdPath + 8, 127);
  } else {
    snprintf(SMP.samplePathRel[ch], 128, "*%s", fullSdPath);
  }
  SMP.samplePathRel[ch][127] = 0;
}

bool soloRandomIsPlayableWav(const char* fullPath) {
  if (!fullPath || !fullPath[0]) return false;
  if (!SD.exists(fullPath)) return false;
  File f = SD.open(fullPath);
  if (!f) return false;
  if (f.isDirectory()) {
    f.close();
    return false;
  }
  uint32_t sz = f.size();
  f.close();
  return sz >= TOERN_WAV_MIN_BYTES;
}

static bool wavIndexPushPath(const char* path) {
  if (!path || !path[0]) return false;
  if (g_wavIndexCount >= TOERN_WAV_INDEX_MAX) return false;
  size_t n = strlen(path);
  if (n == 0 || n >= TOERN_WAV_INDEX_PATH) return false;
  strncpy(g_wavIndex[g_wavIndexCount], path, TOERN_WAV_INDEX_PATH - 1);
  g_wavIndex[g_wavIndexCount][TOERN_WAV_INDEX_PATH - 1] = 0;
  g_wavIndexCount++;
  return true;
}

FLASHMEM bool loadSampleManifest() {
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();
  extern void sdIoYield();

  g_wavIndexCount = 0;
  g_wavIndexLoaded = false;
  manifestLoaded = false;

  if (!SD.exists(TOERN_WAV_INDEX_FILE)) return false;

  sdIoBeginAudioSafe();
  File f = SD.open(TOERN_WAV_INDEX_FILE, FILE_READ);
  if (!f) {
    sdIoEndAudioSafe();
    return false;
  }

  char line[TOERN_WAV_INDEX_PATH];
  size_t n = 0;
  uint16_t lines = 0;
  while (f.available() && g_wavIndexCount < TOERN_WAV_INDEX_MAX) {
    int c = f.read();
    if (c < 0) break;
    if (c == '\r') continue;
    if (c == '\n') {
      line[n] = 0;
      if (n > 4) {
        const char* base = strrchr(line, '/');
        base = base ? base + 1 : line;
        if (hasWavExt(base)) wavIndexPushPath(line);
      }
      n = 0;
      if ((++lines & 31u) == 0u) sdIoYield();
      continue;
    }
    if (n + 1 < sizeof(line)) line[n++] = (char)c;
    else n = sizeof(line) - 1;
  }
  if (n > 4) {
    line[n] = 0;
    const char* base = strrchr(line, '/');
    base = base ? base + 1 : line;
    if (hasWavExt(base)) wavIndexPushPath(line);
  }
  f.close();
  sdIoEndAudioSafe();

  g_wavIndexLoaded = (g_wavIndexCount > 0);
  manifestLoaded = g_wavIndexLoaded;
  return g_wavIndexLoaded;
}


FLASHMEM bool scanAndWriteManifest() {
  extern void stopAllSetWavPreviewAudio();
  extern void sdIoBeginAudioSafe();
  extern void sdIoEndAudioSafe();
  extern void sdIoYield();

  stopAllSetWavPreviewAudio();
  sdIoBeginAudioSafe();

  g_wavIndexCount = 0;
  g_wavIndexLoaded = false;
  manifestLoaded = false;

  int stackTop = 1;
  g_wavScanDirStack[0][0] = 0;

  while (stackTop > 0 && g_wavIndexCount < TOERN_WAV_INDEX_MAX) {
    stackTop--;
    char rel[TOERN_WAV_INDEX_PATH];
    strncpy(rel, g_wavScanDirStack[stackTop], sizeof(rel) - 1);
    rel[sizeof(rel) - 1] = 0;

    char openPath[160];
    if (rel[0]) snprintf(openPath, sizeof(openPath), "%s", rel);
    else snprintf(openPath, sizeof(openPath), "/");

    File dir = SD.open(openPath);
    if (!dir || !dir.isDirectory()) {
      if (dir) dir.close();
      sdIoYield();
      continue;
    }

    File fe;
    uint16_t seen = 0;
    while ((fe = dir.openNextFile())) {
      char base[SAMPLE_BROWSER_NAME_MAX];
      copyBaseName64(fe.name(), base, sizeof(base));
      if (soloRandomIsJunkName(base)) {
        fe.close();
        continue;
      }

      if (strlen(rel) + 1 + strlen(base) >= TOERN_WAV_INDEX_PATH) {
        fe.close();
        continue;
      }

      char child[TOERN_WAV_INDEX_PATH];
      soloRandomJoinRel(child, sizeof(child), rel, base);

      if (fe.isDirectory()) {
        fe.close();
        if (stackTop < TOERN_WAV_SCAN_DIR_STACK) {
          strncpy(g_wavScanDirStack[stackTop], child, TOERN_WAV_INDEX_PATH - 1);
          g_wavScanDirStack[stackTop][TOERN_WAV_INDEX_PATH - 1] = 0;
          stackTop++;
        }
      } else if (hasWavExt(base)) {
        uint32_t sz = fe.size();
        fe.close();
        if (sz >= TOERN_WAV_MIN_BYTES) wavIndexPushPath(child);
      } else {
        fe.close();
      }

      if ((++seen & 7u) == 0u) sdIoYield();
    }
    dir.close();
    sdIoYield();
  }

  if (!SD.exists("samples")) SD.mkdir("samples");
  if (SD.exists(TOERN_WAV_INDEX_FILE)) SD.remove(TOERN_WAV_INDEX_FILE);

  File out = SD.open(TOERN_WAV_INDEX_FILE, FILE_WRITE);
  if (out) {
    for (uint16_t i = 0; i < g_wavIndexCount; i++) {
      out.println(g_wavIndex[i]);
      if ((i & 15u) == 0u) sdIoYield();
    }
    out.close();
  }

  sdIoEndAudioSafe();

  g_wavIndexLoaded = (g_wavIndexCount > 0);
  manifestLoaded = g_wavIndexLoaded;
  g_soloRandomListDirty = true;
  return g_wavIndexLoaded;
}

void soloRandomDropCurrentEntry() {
  if (g_soloRandomPlaylistCount == 0) return;
  int idx = (int)g_soloRandomPlaylistIndex;
  if (idx < 0) idx = 0;
  if (idx >= (int)g_soloRandomPlaylistCount) idx = (int)g_soloRandomPlaylistCount - 1;
  for (int i = idx; i < (int)g_soloRandomPlaylistCount - 1; i++) {
    g_soloRandomPlaylist[i] = g_soloRandomPlaylist[i + 1];
  }
  g_soloRandomPlaylistCount--;
  if (g_soloRandomPlaylistCount == 0) {
    g_soloRandomPlaylistIndex = 0;
    g_soloRandomLastRel[0] = 0;
    return;
  }
  if (g_soloRandomPlaylistIndex >= (int16_t)g_soloRandomPlaylistCount) {
    g_soloRandomPlaylistIndex = (int16_t)g_soloRandomPlaylistCount - 1;
  }
  soloRandomSetLastFromIndex();
}

static bool soloRandomAppendOneFromIndex() {
  if (g_soloRandomPlaylistCount >= SOLO_RANDOM_PLAYLIST_SIZE) return false;
  if (g_wavIndexCount == 0) return false;

  for (int attempt = 0; attempt < 12; attempt++) {
    uint16_t pick = (uint16_t)random((long)g_wavIndexCount);
    bool dup = false;
    for (uint16_t i = 0; i < g_soloRandomPlaylistCount; i++) {
      if (g_soloRandomPlaylist[i] == pick) {
        dup = true;
        break;
      }
    }
    if (dup && g_soloRandomPlaylistCount < g_wavIndexCount) continue;
    g_soloRandomPlaylist[g_soloRandomPlaylistCount++] = pick;
    return true;
  }
  return false;
}

FLASHMEM bool soloRandomRenewPlaylist() {
  g_soloRandomPlaylistCount = 0;
  g_soloRandomPlaylistIndex = 0;
  g_soloRandomLastRel[0] = 0;
  g_soloRandomListDirty = false;

  if (!g_wavIndexLoaded || g_wavIndexCount == 0) {
    if (!loadSampleManifest()) return false;
  }
  if (g_wavIndexCount == 0) return false;

  uint16_t want = SOLO_RANDOM_PLAYLIST_SIZE;
  if (want > g_wavIndexCount) want = g_wavIndexCount;
  for (uint16_t n = 0; n < want; n++) {
    if (!soloRandomAppendOneFromIndex()) break;
  }
  if (g_soloRandomPlaylistCount == 0) return false;
  g_soloRandomPlaylistIndex = 0;
  soloRandomSetLastFromIndex();
  return true;
}

bool soloRandomPickPath(char* outRel, size_t outRelSize) {
  if (g_soloRandomListDirty || g_soloRandomPlaylistCount == 0) {
    soloRandomRenewPlaylist();
  }
  if (g_soloRandomPlaylistCount == 0 || outRelSize == 0) {
    if (outRelSize > 0) outRel[0] = 0;
    return false;
  }
  soloRandomSetLastFromIndex();
  strncpy(outRel, g_soloRandomLastRel, outRelSize - 1);
  outRel[outRelSize - 1] = 0;
  return outRel[0] != 0;
}

bool soloRandomStepPlaylist(int delta) {
  if (g_soloRandomPlaylistCount == 0) {
    if (!soloRandomRenewPlaylist()) return false;
  }

  if (delta > 0) {
    int next = (int)g_soloRandomPlaylistIndex + 1;
    if (next < (int)g_soloRandomPlaylistCount) {
      g_soloRandomPlaylistIndex = (int16_t)next;
    } else if (g_soloRandomPlaylistCount < SOLO_RANDOM_PLAYLIST_SIZE &&
               g_soloRandomPlaylistCount < g_wavIndexCount) {
      if (soloRandomAppendOneFromIndex()) {
        g_soloRandomPlaylistIndex = (int16_t)(g_soloRandomPlaylistCount - 1);
      } else {
        g_soloRandomPlaylistIndex = 0;
      }
    } else {
      g_soloRandomPlaylistIndex = 0;
    }
  } else if (delta < 0) {
    int n = (int)g_soloRandomPlaylistCount;
    int idx = ((int)g_soloRandomPlaylistIndex - 1) % n;
    if (idx < 0) idx += n;
    g_soloRandomPlaylistIndex = (int16_t)idx;
  }

  soloRandomSetLastFromIndex();
  return g_soloRandomPlaylistCount > 0;
}

const char* soloRandomGetLastPreviewPath() {
  return g_soloRandomLastRel[0] ? g_soloRandomLastRel : nullptr;
}

static void sortNameBlock(char names[][SAMPLE_BROWSER_NAME_MAX], int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (strcasecmp(names[i], names[j]) > 0) {
        char tmp[SAMPLE_BROWSER_NAME_MAX];
        memcpy(tmp, names[i], sizeof(tmp));
        memcpy(names[i], names[j], sizeof(tmp));
        memcpy(names[j], tmp, sizeof(tmp));
      }
    }
  }
}

static void sortNameBlockWithSizes(char names[][SAMPLE_BROWSER_NAME_MAX], uint32_t sizes[], int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (strcasecmp(names[i], names[j]) > 0) {
        char tmp[SAMPLE_BROWSER_NAME_MAX];
        memcpy(tmp, names[i], sizeof(tmp));
        memcpy(names[i], names[j], sizeof(tmp));
        memcpy(names[j], tmp, sizeof(tmp));
        uint32_t stmp = sizes[i]; sizes[i] = sizes[j]; sizes[j] = stmp;
      }
    }
  }
}

static void browseDirGoParent(int ch) {
  char* p = g_browseDir[ch];
  char* slash = strrchr(p, '/');
  if (slash) *slash = 0;
  else p[0] = 0;
}

void sampleBrowserInvalidate() {
  g_browseListDirty = true;
  g_soloRandomListDirty = true;
}


int sampleBrowserBrowseIndexMax(int channel) {
  if (channel < 1 || channel >= maxFiles) return 0;
  if (g_browseListDirty) sampleBrowserRefreshList(channel);
  return max(1, (int)g_wavPickCount);
}


void sampleBrowserClampBrowseIndex(int channel) {
  if (channel < 1 || channel >= maxFiles) return;
  int vmax = sampleBrowserBrowseIndexMax(channel);
  int idx = constrain((int)currentMode->pos[3], 1, vmax);
  currentMode->pos[3] = (unsigned int)idx;
}


void sampleBrowserClampBrowseIndexAndHardware(int channel) {
  sampleBrowserClampBrowseIndex(channel);
  if (!encoderI2cWritesAllowed()) return;
  int vmax = sampleBrowserBrowseIndexMax(channel);
  int idx = (int)currentMode->pos[3];
  Encoder[3].updateStatus();
  int raw = (int)Encoder[3].readCounterInt();
  if (raw != idx) {
    Encoder[3].writeMin((int32_t)1);
    Encoder[3].writeMax((int32_t)vmax);
    Encoder[3].writeCounter((int32_t)idx);
  }
}

static bool sampleBrowserAppendFolderSegment(int channel, const char* name) {
  char* d = g_browseDir[channel];
  size_t len = strlen(d);
  size_t nl = strlen(name);
  if (len + 1 + nl >= (size_t)SAMPLE_BROWSER_PATH_MAX) return false;
  if (len) {
    char tmp[SAMPLE_BROWSER_PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s/%s", d, name);
    strncpy(d, tmp, SAMPLE_BROWSER_PATH_MAX - 1);
    d[SAMPLE_BROWSER_PATH_MAX - 1] = 0;
  } else {
    snprintf(d, SAMPLE_BROWSER_PATH_MAX, "%s", name);
  }
  return true;
}

static void sampleBrowserGetLeafFolderName(int channel, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  out[0] = 0;
  if (channel < 1 || channel >= maxFiles) return;
  const char* path = g_browseDir[channel];
  if (!path || !path[0]) return;
  const char* slash = strrchr(path, '/');
  const char* leaf = slash ? slash + 1 : path;
  strncpy(out, leaf, outSize - 1);
  out[outSize - 1] = 0;
}

uint8_t sampleBrowserEntryTypeAt(int idx) {
  if (idx < 0 || idx >= (int)g_wavPickCount) return SAMPLE_BROWSER_ENTRY_FILE;
  return g_wavPickType[idx];
}



static void sampleBrowserFinishFolderStep(int channel, unsigned int prevWavPos, const char* preferredFolderName) {
  sampleBrowserRefreshList(channel);
  int vmax = max(1, (int)g_wavPickCount);
  int browseIdx = 0;
  if (preferredFolderName && preferredFolderName[0]) {
    for (int i = 0; i < (int)g_wavPickCount; i++) {
      if (sampleBrowserEntryTypeAt(i) == SAMPLE_BROWSER_ENTRY_DIR &&
          strcasecmp(g_wavPickName[i], preferredFolderName) == 0) {
        browseIdx = i;
        break;
      }
    }
  } else if (g_wavPickCount >= 2 && sampleBrowserEntryTypeAt(0) != SAMPLE_BROWSER_ENTRY_FILE) {
    browseIdx = 1;
  }
  if (browseIdx < 0) browseIdx = 0;
  if (browseIdx >= vmax) browseIdx = vmax - 1;
  currentMode->pos[1] = 0;
  Encoder[2].writeMin((int32_t)0);
  Encoder[2].writeMax((int32_t)0);
  Encoder[2].writeCounter((int32_t)0);
  currentMode->pos[3] = (unsigned int)(browseIdx + 1);
  Encoder[3].writeMin((int32_t)1);
  Encoder[3].writeMax((int32_t)vmax);
  Encoder[3].writeCounter((int32_t)(browseIdx + 1));
  if (prevWavPos != (unsigned int)(browseIdx + 1)) g_suppressNextWavPreviewAfterFolderNav = true;
  g_setWavIgnoreLoadUntilMs = millis() + 220;
}

FLASHMEM void sampleBrowserRefreshList(int channel) {
  if (channel < 1 || channel >= maxFiles) return;
  int nd = 0, nw = 0;
  char fullPath[160];
  if (g_browseDir[channel][0]) {
    snprintf(fullPath, sizeof(fullPath), "samples/%s", g_browseDir[channel]);
  } else {
    snprintf(fullPath, sizeof(fullPath), "samples");
  }
  File dir = SD.open(fullPath);
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    g_browseDir[channel][0] = 0;
    snprintf(fullPath, sizeof(fullPath), "samples");
    dir = SD.open(fullPath);
    if (!dir || !dir.isDirectory()) {
      if (dir) dir.close();
      g_wavPickCount = 0;
      g_wavFileCount = 0;
      g_folderPickCount = 0;
      g_browseListDirty = false;
      return;
    }
  }
  File fe;
  while ((fe = dir.openNextFile())) {
    char base[SAMPLE_BROWSER_NAME_MAX];
    copyBaseName64(fe.name(), base, sizeof(base));
    if (!base[0] || base[0] == '.') {
      fe.close();
      continue;
    }
    if (fe.isDirectory()) {
      if (nd < BROWSE_MAX_TMP) {
        strncpy(sbTmpDirs[nd], base, SAMPLE_BROWSER_NAME_MAX - 1);
        sbTmpDirs[nd][SAMPLE_BROWSER_NAME_MAX - 1] = 0;
        nd++;
      }
    } else if (hasWavExt(base)) {
      if (nw < BROWSE_MAX_TMP) {
        strncpy(sbTmpWavs[nw], base, SAMPLE_BROWSER_NAME_MAX - 1);
        sbTmpWavs[nw][SAMPLE_BROWSER_NAME_MAX - 1] = 0;
        sbTmpWavSizes[nw] = (uint32_t)fe.size();
        nw++;
      }
    }
    fe.close();
  }
  dir.close();
  sortNameBlock(sbTmpDirs, nd);
  sortNameBlockWithSizes(sbTmpWavs, sbTmpWavSizes, nw);


  uint16_t fn = 0;
  if (g_browseDir[channel][0]) {
    strncpy(g_folderPickName[fn], "..", SAMPLE_BROWSER_NAME_MAX - 1);
    g_folderPickName[fn][SAMPLE_BROWSER_NAME_MAX - 1] = 0;
    fn++;
  }
  for (int i = 0; i < nd && fn < BROWSE_MAX_TMP; i++) {
    strncpy(g_folderPickName[fn], sbTmpDirs[i], SAMPLE_BROWSER_NAME_MAX - 1);
    g_folderPickName[fn][SAMPLE_BROWSER_NAME_MAX - 1] = 0;
    fn++;
  }
  g_folderPickCount = fn;


  uint16_t wn = 0;
  if (g_browseDir[channel][0] && wn < BROWSE_MAX_TMP) {
    strncpy(g_wavPickName[wn], "[../]", SAMPLE_BROWSER_NAME_MAX - 1);
    g_wavPickName[wn][SAMPLE_BROWSER_NAME_MAX - 1] = 0;
    g_wavPickType[wn] = SAMPLE_BROWSER_ENTRY_PARENT;
    g_wavPickSize[wn] = 0;
    wn++;
  }
  for (int i = 0; i < nd && wn < BROWSE_MAX_TMP; i++) {
    strncpy(g_wavPickName[wn], sbTmpDirs[i], SAMPLE_BROWSER_NAME_MAX - 1);
    g_wavPickName[wn][SAMPLE_BROWSER_NAME_MAX - 1] = 0;
    g_wavPickType[wn] = SAMPLE_BROWSER_ENTRY_DIR;
    g_wavPickSize[wn] = 0;
    wn++;
  }
  g_wavFileCount = nw;
  for (int i = 0; i < nw && wn < BROWSE_MAX_TMP; i++) {
    strncpy(g_wavPickName[wn], sbTmpWavs[i], SAMPLE_BROWSER_NAME_MAX - 1);
    g_wavPickName[wn][SAMPLE_BROWSER_NAME_MAX - 1] = 0;
    g_wavPickType[wn] = SAMPLE_BROWSER_ENTRY_FILE;
    g_wavPickSize[wn] = sbTmpWavSizes[i];
    wn++;
  }
  g_wavPickCount = wn;
  g_browseListDirty = false;
}

FLASHMEM void sampleBrowserSyncBrowseFromStoredPath(int ch) {
  if (ch < 1 || ch >= maxFiles) return;
  const char* sp = SMP.samplePathRel[ch];
  g_browseDir[ch][0] = 0;
  if (!sp || !sp[0] || sp[0] == '*') {

    sampleBrowserRefreshList(ch);
    currentMode->pos[1] = 0;
    Encoder[2].writeCounter((int32_t)0);
    Encoder[2].writeMax((int32_t)0);
    currentMode->pos[3] = (g_wavPickCount > 0) ? 1 : 0;
    Encoder[3].writeCounter((int32_t)max(1, (int)currentMode->pos[3]));
    Encoder[3].writeMax((int32_t)max(1, (int)g_wavPickCount));
    return;
  }
  char tmp[128];
  strncpy(tmp, sp, sizeof(tmp) - 1);
  tmp[127] = 0;
  char* slash = strrchr(tmp, '/');
  const char* leafName = tmp;
  if (slash) {
    *slash = 0;
    strncpy(g_browseDir[ch], tmp, SAMPLE_BROWSER_PATH_MAX - 1);
    g_browseDir[ch][SAMPLE_BROWSER_PATH_MAX - 1] = 0;
    leafName = slash + 1;
  } else {
    g_browseDir[ch][0] = 0;
    leafName = tmp;
  }
  sampleBrowserRefreshList(ch);
  for (uint16_t i = 0; i < g_wavPickCount; i++) {
    if (g_wavPickType[i] == SAMPLE_BROWSER_ENTRY_FILE && strcasecmp(g_wavPickName[i], leafName) == 0) {
      currentMode->pos[3] = (unsigned int)(i + 1);
      Encoder[3].writeCounter((int32_t)(i + 1));
      Encoder[3].writeMax((int32_t)max(1, (int)g_wavPickCount));
      currentMode->pos[1] = 0;
      Encoder[2].writeCounter((int32_t)0);
      Encoder[2].writeMax((int32_t)0);
      return;
    }
  }
  currentMode->pos[3] = (g_wavPickCount > 0) ? 1 : 0;
  Encoder[3].writeCounter((int32_t)max(1, (int)currentMode->pos[3]));
  Encoder[3].writeMax((int32_t)max(1, (int)g_wavPickCount));
  currentMode->pos[1] = 0;
  Encoder[2].writeCounter((int32_t)0);
  Encoder[2].writeMax((int32_t)0);
}

bool sampleBrowserIsLoadableSelection(int channel) {
  if (channel < 1 || channel >= maxFiles) return false;
  if (g_browseListDirty) sampleBrowserRefreshList(channel);
  int idx = (int)currentMode->pos[3] - 1;
  if (idx < 0 || idx >= (int)g_wavPickCount) return false;
  return g_wavPickType[idx] == SAMPLE_BROWSER_ENTRY_FILE;
}

FLASHMEM void sampleBrowserNavigatePress(int channel) {
  if (channel < 1 || channel >= maxFiles) return;
  sampleBrowserClampBrowseIndexAndHardware(channel);
  if (g_wavPickCount == 0) return;

  const unsigned int prevWavPos = currentMode->pos[3];
  char preferredFolderName[SAMPLE_BROWSER_NAME_MAX];
  preferredFolderName[0] = 0;
  const int idx = constrain((int)currentMode->pos[3] - 1, 0, max(0, (int)g_wavPickCount - 1));
  const char* name = g_wavPickName[idx];
  uint8_t type = g_wavPickType[idx];

  if (type == SAMPLE_BROWSER_ENTRY_PARENT) {
    sampleBrowserGetLeafFolderName(channel, preferredFolderName, sizeof(preferredFolderName));
    browseDirGoParent(channel);
  } else if (type == SAMPLE_BROWSER_ENTRY_DIR) {
    if (!sampleBrowserAppendFolderSegment(channel, name)) return;
  } else {
    return;
  }
  sampleBrowserFinishFolderStep(channel, prevWavPos, preferredFolderName);
}



FLASHMEM void sampleBrowserLastEncoderPress(int channel) {
  if (channel < 1 || channel >= maxFiles) return;
  sampleBrowserClampBrowseIndexAndHardware(channel);
  if (g_wavPickCount == 0) return;

  const int idx = constrain((int)currentMode->pos[3] - 1, 0, max(0, (int)g_wavPickCount - 1));
  const uint8_t type = g_wavPickType[idx];
  const bool onDirectory = (type == SAMPLE_BROWSER_ENTRY_PARENT || type == SAMPLE_BROWSER_ENTRY_DIR);

  if (onDirectory) {
    sampleBrowserNavigatePress(channel);
    return;
  }

  sampleBrowserGoUpPress(channel);
}


FLASHMEM void sampleBrowserGoUpPress(int channel) {
  if (channel < 1 || channel >= maxFiles) return;
  if (!g_browseDir[channel][0]) return;

  const unsigned int prevWavPos = currentMode->pos[3];
  char preferredFolderName[SAMPLE_BROWSER_NAME_MAX];
  preferredFolderName[0] = 0;
  sampleBrowserGetLeafFolderName(channel, preferredFolderName, sizeof(preferredFolderName));
  browseDirGoParent(channel);
  sampleBrowserFinishFolderStep(channel, prevWavPos, preferredFolderName);
}

FLASHMEM void sampleBrowserClearAll() {
  for (int i = 0; i < maxFiles; i++) {
    g_browseDir[i][0] = '\0';
    SMP.samplePathRel[i][0] = '\0';
  }
  g_folderPickCount = 0;
  g_wavPickCount = 0;
  g_wavFileCount = 0;
  g_browseListDirty = true;
  g_soloRandomListDirty = true;
  manifestLoaded = false;
  manifestFolderCount = 0;
}


FLASHMEM void generateNextNumericName(int folderIdx, char* outName, size_t outSize) {
  (void)folderIdx;
  char path[160];
  for (int n = 1; n < 1000; ++n) {
    snprintf(outName, outSize, "rec_%02d.wav", n);
    snprintf(path, sizeof(path), "samples/recs/%s", outName);
    if (!SD.exists(path)) return;
  }
  snprintf(outName, outSize, "rec_99.wav");
}



FLASHMEM void buildSamplePath(int folderIdx, int fileIdx, char* out, size_t outSize) {
  (void)folderIdx;
  (void)fileIdx;
  if (outSize > 0) out[0] = '\0';
  int ch = GLOB.currentChannel;
  if (ch < 1 || ch >= maxFiles) return;
  if (!SMP.samplePathRel[ch][0]) return;
  if (SMP.samplePathRel[ch][0] == '*') {
    snprintf(out, outSize, "%s", SMP.samplePathRel[ch] + 1);
  } else {
    snprintf(out, outSize, "samples/%s", SMP.samplePathRel[ch]);
  }
}



FLASHMEM void loadSMPSettings() {

  if (!SMP_LOAD_SETTINGS) {
    return;
  }


  for (int ch = 4; ch <= 8; ch++) {
    SMP.filter_settings[ch][EFX] = 0;
  }


  const int validChannels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numValidChannels = sizeof(validChannels) / sizeof(validChannels[0]);


  for (int i = 0; i < numValidChannels; i++) {
    int ch = validChannels[i];


    for (int f = 0; f < NUM_FILTERS; f++) {
      setFilters((FilterType)f, ch, true);
    }


    setParams(DELAY, ch);
    setParams(ATTACK, ch);
    setParams(HOLD, ch);
    setParams(DECAY, ch);
    setParams(SUSTAIN, ch);
    setParams(RELEASE, ch);


    if (ch == 11) {
      extern void applySynthInstrumentPreset(int channel, int instrumentIdx);
      extern void applySynthInstrumentFormDefault(int channel, int instrumentIdx);


      if (SMP.synth_settings[11][INSTRUMENT] > 12 ||
          (SMP.synth_settings[11][INSTRUMENT] == 0 &&
           SMP.param_settings[11][DECAY] == 32 &&
           SMP.param_settings[11][SUSTAIN] == 32)) {
        applySynthInstrumentPreset(11, 12);
      } else {



        applySynthInstrumentFormDefault(
            11, (int)SMP.synth_settings[11][INSTRUMENT]);
      }
      updateSynthVoice(11);
    } else if (ch == 13 || ch == 14) {
      extern void applyMonoSynthCrf(int ch);
      applyMonoSynthCrf(ch);
    }
  }



  forceAllMixerGainsToTarget();







}

void writeWavHeader(File &file, uint32_t sampleRate, uint8_t bitsPerSample, uint16_t numChannels) {
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint8_t blockAlign = numChannels * bitsPerSample / 8;


  uint8_t header[44] = {
    'R', 'I', 'F', 'F',
    0, 0, 0, 0,
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,
    1, 0,
    (uint8_t)(numChannels & 0xff), (uint8_t)(numChannels >> 8),
    (uint8_t)(sampleRate & 0xff), (uint8_t)((sampleRate >> 8) & 0xff),
    (uint8_t)((sampleRate >> 16) & 0xff), (uint8_t)((sampleRate >> 24) & 0xff),
    (uint8_t)(byteRate & 0xff), (uint8_t)((byteRate >> 8) & 0xff),
    (uint8_t)((byteRate >> 16) & 0xff), (uint8_t)((byteRate >> 24) & 0xff),
    blockAlign, 0,
    bitsPerSample, 0,
    'd', 'a', 't', 'a',
    0, 0, 0, 0
  };

  file.write(header, 44);
}

void finalizeWavHeader(File &file, uint32_t dataSize) {
  uint32_t riffSize = dataSize + 36;
  uint8_t riffBytes[4] = {
    (uint8_t)(riffSize & 0xff),
    (uint8_t)((riffSize >> 8) & 0xff),
    (uint8_t)((riffSize >> 16) & 0xff),
    (uint8_t)((riffSize >> 24) & 0xff)
  };
  uint8_t dataBytes[4] = {
    (uint8_t)(dataSize & 0xff),
    (uint8_t)((dataSize >> 8) & 0xff),
    (uint8_t)((dataSize >> 16) & 0xff),
    (uint8_t)((dataSize >> 24) & 0xff)
  };

  if (!file.seek(4)) return;
  file.write(riffBytes, sizeof(riffBytes));
  if (!file.seek(40)) return;
  file.write(dataBytes, sizeof(dataBytes));
  file.seek(44 + dataSize);
}



float recInputMonitorGain() {
  if (getSpkrEnabled()) return 0.0f;
  const float maxPlaybackGain = MIX_BUS_HEADROOM * MIX_END_SAMPLES_GAIN;
  if (recMode == 1) {
    return mapf((float)micGain, 0.0f, 63.0f, 0.0f, maxPlaybackGain);
  }
  return mapf((float)lineInLevel, 0.0f, 15.0f, 0.0f, maxPlaybackGain);
}

static int recRamDropRemaining = 0;

void startRecordingRAM() {
  if (isRecording) return;

  if (playSdWav1.isPlaying()) playSdWav1.stop();

  queue1.end();
  while (queue1.available()) {
    queue1.readBuffer();
    queue1.freeBuffer();
  }
  recTime = 0;
  recWriteIndex = 0;


  recRamDropRemaining = FAST_DROP_BLOCKS;


  extern int peakRecIndex;
  peakRecIndex = 0;

  queue1.begin();
  isRecording = true;
  applySpkrHardware();

  mixer_end.gain(3, recInputMonitorGain());
}

void flushAudioQueueToRAM2() {
  if (!isRecording) return;

  while (queue1.available() && recWriteIndex + 128 <= BUFFER_SAMPLES) {
    auto block = (int16_t *)queue1.readBuffer();
    if (recRamDropRemaining > 0) {
      recRamDropRemaining--;
      queue1.freeBuffer();
      continue;
    }
    memcpy(recBuffer + recWriteIndex, block, 128 * sizeof(int16_t));
    recWriteIndex += 128;
    queue1.freeBuffer();
  }

  if (recWriteIndex >= BUFFER_SAMPLES) {
    stopRecordingRAM(0, 0);
  }
}

void stopRecordingRAM(int fnr, int snr) {
  if (!isRecording) return;
  flushAudioQueueToRAM2();
  queue1.end();
  while (queue1.available()) {
    if (recRamDropRemaining > 0) {
      recRamDropRemaining--;
      queue1.readBuffer();
    } else if (recWriteIndex + 128 <= BUFFER_SAMPLES) {
      auto block = (int16_t *)queue1.readBuffer();
      memcpy(recBuffer + recWriteIndex, block, 128 * sizeof(int16_t));
      recWriteIndex += 128;
    } else {
      queue1.readBuffer();
    }
    queue1.freeBuffer();
  }
  isRecording = false;
  applySpkrHardware();


  mixer_end.gain(3, 0.0);

  if (recWriteIndex < 128) return;


  (void)fnr;
  (void)snr;
  char path[160];
  char tmpName[65];
  generateNextNumericName(0, tmpName, sizeof(tmpName));
  int ch = GLOB.currentChannel;
  if (ch < 1 || ch >= maxFiles) ch = 1;

  if (!SD.exists("samples")) SD.mkdir("samples");
  if (!SD.exists("samples/recs")) SD.mkdir("samples/recs");
  snprintf(path, sizeof(path), "samples/recs/%s", tmpName);
  if (SD.exists(path)) SD.remove(path);
  File f = SD.open(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!f) { return; }

  writeWavHeader(f, AUDIO_SAMPLE_RATE_EXACT, 16, 1);

  f.write((uint8_t *)recBuffer, recWriteIndex * sizeof(int16_t));
  finalizeWavHeader(f, recWriteIndex * sizeof(int16_t));

  f.close();

  sampleBrowserInvalidate();
  snprintf(SMP.samplePathRel[ch], 128, "recs/%s", tmpName);
  extern void invalidateShowWaveBrowserSync();
  invalidateShowWaveBrowserSync();


  extern bool sampleIsLoaded;
  extern bool firstcheck;
  extern CachedSample previewCache;
  extern void beginChannelSampleReload(unsigned int ch);
  extern void endChannelSampleReload(unsigned int ch);
  extern float rateFactor;

  sampleIsLoaded = false;
  firstcheck = true;
  previewCache.valid = false;


  beginChannelSampleReload((unsigned int)ch);
  int16_t *targetBuffer = (int16_t *)sampled[ch];
  int bufferSamples = (int)(sizeof(sampled[ch]) / 2);
  int samplesToCopy = (int)min((size_t)bufferSamples, recWriteIndex);
  if (samplesToCopy > 0) {
    memcpy(targetBuffer, recBuffer, (size_t)samplesToCopy * sizeof(int16_t));
    const int END_PAD_SAMPLES = 1024;
    int pad = min(END_PAD_SAMPLES, bufferSamples - samplesToCopy);
    if (pad > 0) {
      memset(targetBuffer + samplesToCopy, 0, (size_t)pad * sizeof(int16_t));
    }
    int playLen = samplesToCopy + pad;
    loadedSampleRate[ch] = 3;
    loadedSampleLen[ch] = playLen;
    samplerAddSample(ch, 36, targetBuffer, playLen, rateFactor);
    channelDirection[ch] = 1;
  }
  endChannelSampleReload((unsigned int)ch);
}


void startFastRecord() {
  if (fastRecordActive) return;


  if (!touchyEnabled && (recChannelClear == 1 || recChannelClear == 3)) {

    clearAllNotesOfChannel();
  }



   paintMode = false;
            freshPaint = true;
            unpaintMode = false;
            pressed[3] = false;


            extern bool preventPaintUnpaint;
            preventPaintUnpaint = false;


  if (!touchyEnabled && recChannelClear != 2) {
    note[beat][GLOB.currentChannel+1].channel = GLOB.currentChannel;
    note[beat][GLOB.currentChannel+1].velocity = defaultVelocity;
    note[beat][GLOB.currentChannel+1].probability = 100;
    note[beat][GLOB.currentChannel+1].condition = 1;
    note[beat][GLOB.currentChannel+1].midiPitch = NOTE_MIDI_PITCH_NONE;
  }


  queue1.end();
  while (queue1.available()) {
    queue1.readBuffer();
    queue1.freeBuffer();
  }


  int ch = GLOB.currentChannel;
  fastRecordChannel = (uint8_t)constrain(ch, 0, 255);


  extern AudioPlayArrayResmp *voices[];
  if (ch >= 0 && ch <= 8 && voices[ch] != nullptr) {
    voices[ch]->stop();
  }
  extern AudioEffectEnvelope *envelopes[];
  if (ch >= 0 && ch < 15 && envelopes[ch] != nullptr) {
    envelopes[ch]->noteOff();
  }


  if (ch >= 0 && ch <= 8) {
    for (int note = 36; note <= 96; note++) {
      samplerNoteEvent(ch, note, 0, false, false);
    }
  }

  fastRecWriteIndex[ch] = 0;


  if (recChannelClear == 3) {
    fastDropRemaining = 0;
  } else {
    fastDropRemaining = FAST_DROP_BLOCKS;
  }


  queue1.begin();
  fastRecordActive = true;
  applySpkrHardware();




  mixer_end.gain(3, recInputMonitorGain());


}


void flushAudioQueueToRAM() {
  if (fastRecordActive) {
    int ch = fastRecordChannel;
    auto &idx = fastRecWriteIndex[ch];


    int16_t *dest = reinterpret_cast<int16_t *>(sampled[ch]);


    while (queue1.available()) {
      int16_t *block = (int16_t *)queue1.readBuffer();

      if (fastDropRemaining > 0) {
        fastDropRemaining--;
        queue1.freeBuffer();
        continue;
      }

      if (idx >= FASTREC_MAX_SAMPLES) {
        queue1.freeBuffer();
        pendingStopFastRecord = true;
        break;
      }

      size_t room = FASTREC_MAX_SAMPLES - idx;
      size_t n = (room < (size_t)AUDIO_BLOCK_SAMPLES) ? room : (size_t)AUDIO_BLOCK_SAMPLES;
      memcpy(dest + idx, block, n * sizeof(int16_t));
      idx += n;
      queue1.freeBuffer();

      if (idx >= FASTREC_MAX_SAMPLES) {
        pendingStopFastRecord = true;
        break;
      }
    }
    return;
  }
}

void stopFastRecord() {
  if (!fastRecordActive) return;

  mixer_end.gain(3, 0.0);

  int ch = fastRecordChannel;
  auto &idx = fastRecWriteIndex[ch];
  int16_t *dest = reinterpret_cast<int16_t *>(sampled[ch]);



  int flushCount = 0;
  int lastQueueSize = -1;
  int stableCount = 0;


  while (fastRecordActive && queue1.available()) {
    int currentQueueSize = queue1.available();
    flushAudioQueueToRAM();
    flushCount++;


    if (currentQueueSize == lastQueueSize) {
      stableCount++;

      if (stableCount >= 3) break;
    } else {
      stableCount = 0;
      lastQueueSize = currentQueueSize;
    }


    if (flushCount >= 100) break;
  }


  fastRecordActive = false;
  applySpkrHardware();
  fastRecordChannel = 0;
  recordingBorderArmed = false;
  pendingStartFastRecord = false;
  fastRecWaitRelease = true;


  flushAudioQueueToRAM();

  queue1.end();



  flushCount = 0;
  lastQueueSize = -1;
  stableCount = 0;

  while (queue1.available() && idx < FASTREC_MAX_SAMPLES) {
    int currentQueueSize = queue1.available();
    int16_t *block = (int16_t *)queue1.readBuffer();
    size_t room = FASTREC_MAX_SAMPLES - idx;
    size_t n = (room < (size_t)AUDIO_BLOCK_SAMPLES) ? room : (size_t)AUDIO_BLOCK_SAMPLES;
    memcpy(dest + idx, block, n * sizeof(int16_t));
    idx += n;
    queue1.freeBuffer();
    flushCount++;


    if (currentQueueSize == lastQueueSize) {
      stableCount++;

      if (stableCount >= 3) break;
    } else {
      stableCount = 0;
      lastQueueSize = currentQueueSize;
    }


    if (flushCount >= 100) break;
  }


  unsigned int savedStartBeat = recordingStartBeat;


  recordingStartBeat = 0;


  extern unsigned int recordingBeatCount;
  recordingBeatCount = 0;

  loadedSampleLen[ch] = idx;
  loadedSampleRate[ch] = 3;



  if (ch >= 1 && ch < maxFiles && idx >= 128) {
    extern size_t sdIoChunkSize();
    extern void sdIoYield();
    extern void sdIoBeginAudioSafe();
    extern void sdIoEndAudioSafe();
    char path[160];
    char tmpName[65];
    generateNextNumericName(0, tmpName, sizeof(tmpName));
    if (!SD.exists("samples")) SD.mkdir("samples");
    if (!SD.exists("samples/recs")) SD.mkdir("samples/recs");
    snprintf(path, sizeof(path), "samples/recs/%s", tmpName);
    if (SD.exists(path)) SD.remove(path);
    sdIoBeginAudioSafe();
    File f = SD.open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (f) {
      writeWavHeader(f, AUDIO_SAMPLE_RATE_EXACT, 16, 1);
      const size_t CHUNK = sdIoChunkSize();
      uint8_t *dataPtr = reinterpret_cast<uint8_t *>(sampled[ch]);
      uint32_t dataBytes = (uint32_t)idx * sizeof(int16_t);
      for (uint32_t offset = 0; offset < dataBytes; offset += CHUNK) {
        size_t n = min(CHUNK, (size_t)(dataBytes - offset));
        f.write(dataPtr + offset, n);
        sdIoYield();
      }
      finalizeWavHeader(f, dataBytes);
      f.close();
      sampleBrowserInvalidate();
      snprintf(SMP.samplePathRel[ch], 128, "recs/%s", tmpName);
      SMP.samplePathRel[ch][127] = 0;
      GLOB.seek = 0;
      GLOB.seekEnd = 100;
      extern void invalidateShowWaveBrowserSync();
      invalidateShowWaveBrowserSync();
      extern CachedSample previewCache;
      previewCache.valid = false;
      extern bool sampleIsLoaded;
      extern bool firstcheck;
      sampleIsLoaded = false;
      firstcheck = true;
    }
    sdIoEndAudioSafe();
  }


  copySampleToSamplepack0(ch);
  saveSp0StateToEEPROM();


  samplerRemoveSamples(ch);
  samplerAddSample(ch,
    36,
    (int16_t*)sampled[ch],
    idx,
    rateFactor
  );
  channelDirection[ch] = 1;


  if (!touchyEnabled && recChannelClear == 3 && savedStartBeat > 0) {


    unsigned int nextBeat1 = ((savedStartBeat - 1) / maxX + 1) * maxX + 1;

    note[nextBeat1][ch+1].channel = ch;
    note[nextBeat1][ch+1].velocity = defaultVelocity;
    note[nextBeat1][ch+1].probability = 100;
    note[nextBeat1][ch+1].condition = 1;
    note[nextBeat1][ch+1].midiPitch = NOTE_MIDI_PITCH_NONE;
  }






}

FLASHMEM void clearAllNotesOfChannel() {
  uint8_t channel = GLOB.currentChannel;

  for (uint16_t step = 0; step < maxlen; step++) {
    for (uint8_t pitch = 1; pitch <= 16; pitch++) {
      if (note[step][pitch].channel == channel) {
        note[step][pitch].channel = 0;
        note[step][pitch].velocity = defaultVelocity;
        note[step][pitch].probability = 100;
        note[step][pitch].condition = 1;
        note[step][pitch].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    }
  }

  updateLastPage();


  if (!recordingBorderArmed && !fastRecordActive && !pendingStartFastRecord) {
    FastLEDshow();
  }
}

FLASHMEM void FastLEDclear() {


  for (unsigned int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

extern bool isNowPlaying;
extern uint32_t lastUserActivityMs;
extern int ledModules;

void light_single(unsigned int matrixId, unsigned int x, unsigned int y, CRGB color);
void clearLedStripForScreensaver();


static bool screensaverEncodersOff = false;



FLASHMEM static void ssLight(int x, int y, CRGB color) {
  if (x < 1 || y < 1) return;
  unsigned int matrixNum = (x - 1) / MATRIX_WIDTH;
  unsigned int localX = ((x - 1) % MATRIX_WIDTH) + 1;
  light_single(matrixNum, localX, y, color);
}

FLASHMEM static void drawScreensaverEye(int baseX, int baseY, bool closed,
                                        CRGB color, CRGB pColor,
                                        int pupilOffX, int pupilOffY) {
  if (closed) {
    for (int dx = 0; dx < 5; dx++) {
      ssLight(baseX + dx, baseY + 3, color);
    }
    ssLight(baseX + 1, baseY + 4, color);
    ssLight(baseX + 2, baseY + 4, color);
    ssLight(baseX + 3, baseY + 4, color);
    return;
  }

  ssLight(baseX + 1, baseY + 6, color);
  ssLight(baseX + 2, baseY + 6, color);
  ssLight(baseX + 3, baseY + 6, color);
  ssLight(baseX + 0, baseY + 5, color);
  ssLight(baseX + 4, baseY + 5, color);
  for (int dy = 1; dy <= 4; dy++) {
    ssLight(baseX + 0, baseY + dy, color);
    ssLight(baseX + 4, baseY + dy, color);
  }
  ssLight(baseX + 1, baseY + 0, color);
  ssLight(baseX + 2, baseY + 0, color);
  ssLight(baseX + 3, baseY + 0, color);

  int pupilX = baseX + 1 + pupilOffX;
  int pupilY = baseY + 2 + pupilOffY;
  if (pupilX < baseX + 1) pupilX = baseX + 1;
  if (pupilX > baseX + 2) pupilX = baseX + 2;
  if (pupilY < baseY + 1) pupilY = baseY + 1;
  if (pupilY > baseY + 3) pupilY = baseY + 3;

  ssLight(pupilX, pupilY, pColor);
  ssLight(pupilX + 1, pupilY, pColor);
  ssLight(pupilX, pupilY + 1, pColor);
  ssLight(pupilX + 1, pupilY + 1, pColor);
}

FLASHMEM static void drawScreensaverMatrix() {

  if (!screensaverEncodersOff) {
    for (int i = 0; i < 4; i++) {
      Encoder[i].writeRGBCode(0x000000);
    }
    screensaverEncodersOff = true;
  }

  FastLEDclear();
  clearLedStripForScreensaver();

  const uint32_t idleMs = millis() - lastUserActivityMs;
  const bool asleep = (idleMs >= 90000UL);



  const uint8_t outlineBrightness = asleep ? 7 : 15;
  const uint8_t pupilBrightness = asleep ? 7 : 80;
  const CRGB eyeColor = CRGB(outlineBrightness, outlineBrightness, outlineBrightness);
  const CRGB pupilColor = CRGB(pupilBrightness, pupilBrightness, pupilBrightness);


  const int displayWidth = (ledModules == 2) ? 32 : 16;
  const int eyeWidth = 5;
  const int gap = 2;
  const int totalWidth = eyeWidth * 2 + gap;
  const int leftEyeX = (displayWidth - totalWidth) / 2 + 1;
  const int rightEyeX = leftEyeX + eyeWidth + gap;
  int eyeY = 5;

  uint32_t now = millis();


  static int8_t dreamOffsetY = 0;
  static uint8_t dreamPhase = 0;
  static uint32_t lastDreamMs = 0;

  if (asleep) {

    if (now - lastDreamMs >= 3000) {
      lastDreamMs = now;
      dreamPhase = (dreamPhase + 1) % 4;
      switch (dreamPhase) {
        case 0: dreamOffsetY = 0; break;
        case 1: dreamOffsetY = 1; break;
        case 2: dreamOffsetY = 0; break;
        case 3: dreamOffsetY = -1; break;
      }
    }
    eyeY += dreamOffsetY;
  } else {
    dreamOffsetY = 0;
    dreamPhase = 0;
  }


  static bool isPeeking = false;
  static bool peekLeftEye = false;


  static uint32_t lastBlinkMs = 0;
  static uint32_t nextBlinkInterval = 3000;
  static bool isBlinking = false;

  if (!asleep) {
    if (!isBlinking && (now - lastBlinkMs >= nextBlinkInterval)) {
      isBlinking = true;
      lastBlinkMs = now;
    }
    if (isBlinking && (now - lastBlinkMs >= 150)) {
      isBlinking = false;
      nextBlinkInterval = 2500 + (now % 2500);
    }
  } else {
    isBlinking = false;
  }


  static int8_t targetPupilX = 0;
  static int8_t targetPupilY = 0;
  static int8_t currentPupilX = 0;
  static int8_t currentPupilY = 0;
  static uint32_t lastMoveMs = 0;
  static uint32_t nextMoveInterval = 1500;
  static uint32_t lastInterpMs = 0;

  if (!asleep) {
    if (now - lastMoveMs >= nextMoveInterval) {
      lastMoveMs = now;
      targetPupilX = -1 + (int8_t)(now % 3);
      targetPupilY = -1 + (int8_t)((now / 7) % 3);
      nextMoveInterval = 800 + (now % 1200);
    }
    if (now - lastInterpMs >= 100) {
      lastInterpMs = now;
      if (currentPupilX < targetPupilX) currentPupilX++;
      else if (currentPupilX > targetPupilX) currentPupilX--;
      if (currentPupilY < targetPupilY) currentPupilY++;
      else if (currentPupilY > targetPupilY) currentPupilY--;
    }
  } else {

    if (isPeeking) {
      currentPupilX = peekLeftEye ? 1 : -1;
      currentPupilY = 0;
    } else {
      currentPupilX = 0;
      currentPupilY = 0;
    }
  }


  bool leftClosed = isBlinking || (asleep && !(isPeeking && peekLeftEye));
  bool rightClosed = isBlinking || (asleep && !(isPeeking && !peekLeftEye));


  CRGB peekColor = CRGB(12, 12, 12);
  CRGB peekPupil = CRGB(40, 40, 40);

  CRGB leftColor = (isPeeking && peekLeftEye) ? peekColor : eyeColor;
  CRGB leftPupil = (isPeeking && peekLeftEye) ? peekPupil : pupilColor;
  CRGB rightColor = (isPeeking && !peekLeftEye) ? peekColor : eyeColor;
  CRGB rightPupil = (isPeeking && !peekLeftEye) ? peekPupil : pupilColor;

  drawScreensaverEye(leftEyeX, eyeY, leftClosed, leftColor, leftPupil, currentPupilX, currentPupilY);
  drawScreensaverEye(rightEyeX, eyeY, rightClosed, rightColor, rightPupil, currentPupilX, currentPupilY);
}

FLASHMEM void FastLEDshow() {

  if (isRecording || fastRecordActive) return;
  if (millis() - lastUpdate > RefreshTime) {
    lastUpdate = millis();
    extern bool sdSerialServerClientConnected();
    extern bool imageMode;
    extern bool fullMuteIsActive();
    extern bool eyesScreensaverEnabled();
    extern uint32_t eyesScreensaverIdleMs();
    extern Mode draw;
    extern Mode singleMode;
    extern Mode filterMode;


    const bool playingGridFocus = isNowPlaying && (
      currentMode == &draw ||
      currentMode == &singleMode ||
      currentMode == &filterMode ||
      imageMode ||
      fullMuteIsActive());



    const uint32_t eyesIdleMs = eyesScreensaverIdleMs();
    bool screensaverActive = currentMode != &touchyMode && !playingGridFocus
      && !recordingBorderArmed
      && !fastRecordActive
      && eyesScreensaverEnabled()
      && eyesIdleMs > 0
      && !sdSerialServerClientConnected()
      && (millis() - lastUserActivityMs >= eyesIdleMs);
    if (screensaverActive) {
      drawScreensaverMatrix();
    } else if (screensaverEncodersOff) {

      screensaverEncodersOff = false;
    }


    if (currentMode != &touchyMode && fnDirection != 0 && (int32_t)(fnDirectionUntil - millis()) > 0) {
      CRGB saved[NUM_LEDS];
      memcpy(saved, leds, sizeof(saved));
      drawText(fnDirection < 0 ? "<<" : ">>", 7, 8, UI_WHITE);
      FastLED.show();
      memcpy(leds, saved, sizeof(saved));
    } else {
      FastLED.show();
    }
  }
}

static void presentGridThenRecordingOverlay(bool dotted) {
  lastUserActivityMs = millis();
  if (currentMode == &draw || currentMode == &singleMode) {
    extern void drawBase();
    extern void drawTriggers();
    extern void drawTimer();
    drawBase();
    drawTriggers();
    if (isNowPlaying) drawTimer();
  }
  if (dotted) {
    extern void drawRecordingBorderDotted();
    drawRecordingBorderDotted();
  } else {
    extern void drawRecordingBorder();
    drawRecordingBorder();
  }
  lastUpdate = 0;
  FastLEDshow();
}



void presentRecordingBorder() {
  presentGridThenRecordingOverlay(false);
}

int getPage(int x) {

  return (x - 1) / maxX + 1;
}




static inline int randStep16(unsigned int x_rel) {
  return (int)(((x_rel - 1u) % 16u) + 1u);
}

FLASHMEM static CompanionRole companionRoleForChannel(uint8_t channel) {
  switch (channel) {
    case 1: return CompanionRole::Kick;
    case 2: return CompanionRole::Snare;
    case 3: return CompanionRole::ClosedHat;
    case 4: return CompanionRole::Clap;
    case 5: return CompanionRole::Tom;
    case 6:
    case 11: return CompanionRole::Bass;
    case 7:
    case 13:
    case 14: return CompanionRole::Keys;
    case 8: return CompanionRole::VocalsPad;
    default: return CompanionRole::Unknown;
  }
}

FLASHMEM static bool companionChannelSupported(uint8_t channel) {
  return companionRoleForChannel(channel) != CompanionRole::Unknown;
}

FLASHMEM static int companionEffectivePages() {
  int pages = (maxX > 0) ? (int)(MAX_STEPS / maxX) : 1;
  if (pages < 1) pages = 1;
  if (pages > (int)maxPages) pages = (int)maxPages;
  return pages;
}

FLASHMEM static int companionActivePages() {
  extern int loopLength;
  int pages = loopLength > 0 ? loopLength : (int)lastPage;
  if (pages < (int)GLOB.edit) pages = (int)GLOB.edit;
  int limit = companionEffectivePages();
  if (pages < 1) pages = 1;
  if (pages > limit) pages = limit;
  return pages;
}

FLASHMEM static void companionAddRoleWeight(CompanionContext &ctx, CompanionRole role,
                                   int phase, uint16_t weight) {
  switch (role) {
    case CompanionRole::Kick: ctx.kickWeight[phase] += weight; break;
    case CompanionRole::Snare: ctx.snareWeight[phase] += weight; break;
    case CompanionRole::Clap: ctx.clapWeight[phase] += weight; break;
    case CompanionRole::ClosedHat: ctx.hatWeight[phase] += weight; break;
    case CompanionRole::Tom: ctx.tomWeight[phase] += weight; break;
    case CompanionRole::Bass: ctx.bassWeight[phase] += weight; break;
    case CompanionRole::Keys:
    case CompanionRole::VocalsPad: ctx.harmonicWeight[phase] += weight; break;
    default: break;
  }
}

FLASHMEM static int companionPitchClassForRow(uint8_t channel, int row) {
  int noteValue = row - 1;
  if (channel >= 1 && channel <= 8) {
    noteValue = 12 * SampleRate[channel] + row - (channel + 1);
    noteValue += (int)lroundf(detune[channel]) + (int)lroundf(channelOctave[channel]);
  } else if (channel == 11) {
    noteValue = 12 * (int)octave[0] + transpose + row - 1;
  } else if (channel == 13 || channel == 14) {
    noteValue = row - channel + 47;
  }
  noteValue %= 12;
  return noteValue < 0 ? noteValue + 12 : noteValue;
}

FLASHMEM static int companionPitchClassForCell(const Note &cell, uint8_t channel, int row) {
  if (cell.midiPitch != NOTE_MIDI_PITCH_NONE && cell.midiPitch <= 127) {
    return (int)(cell.midiPitch % 12u);
  }
  return companionPitchClassForRow(channel, row);
}

FLASHMEM static uint8_t companionDefaultHomeRow(CompanionRole role) {
  switch (role) {
    case CompanionRole::Kick: return 3;
    case CompanionRole::Snare: return 5;
    case CompanionRole::ClosedHat: return 7;
    case CompanionRole::Clap: return 9;
    case CompanionRole::Tom: return 6;
    case CompanionRole::Bass: return 3;
    case CompanionRole::Keys: return 9;
    case CompanionRole::VocalsPad: return 10;
    default: return 5;
  }
}



FLASHMEM static CompanionContext analyzeCompanionContext(int firstPage, int lastSourcePage,
                                                int focusPage, uint8_t excludeChannel) {
  CompanionContext ctx;
  int limit = companionEffectivePages();
  firstPage = constrain(firstPage, 1, limit);
  lastSourcePage = constrain(lastSourcePage, firstPage, limit);
  focusPage = constrain(focusPage, 1, limit);

  uint16_t chRowHist[16][17] = {};
  uint32_t chVelSum[16] = {};
  uint32_t onBeatWeight = 0;
  uint32_t offBeatWeight = 0;

  for (int ch = 1; ch < 16; ch++) {
    ctx.channelHomeRow[ch] = companionDefaultHomeRow(companionRoleForChannel((uint8_t)ch));
    ctx.channelMeanVel[ch] = 96;
  }

  for (int page = firstPage; page <= lastSourcePage; page++) {
    int distance = abs(page - focusPage);
    uint16_t pageWeight = (distance == 0) ? 6 : ((distance == 1) ? 2 : 1);
    unsigned int start = (unsigned int)(page - 1) * maxX + 1;
    unsigned int end = min((unsigned int)MAX_STEPS + 1u, start + maxX);
    ctx.sourcePages++;

    for (unsigned int c = start; c < end; c++) {
      int phase = randStep16(c - start + 1) - 1;
      int bar = constrain(phase / 4, 0, 3);
      for (int row = 1; row <= 16; row++) {
        const Note &cell = note[c][row];
        uint8_t ch = cell.channel;
        if (ch == 0 || ch >= 16 || !companionChannelSupported(ch)) continue;



        chRowHist[ch][row] += pageWeight;
        chVelSum[ch] += cell.velocity;
        ctx.channelHitCount[ch]++;

        if (ch == excludeChannel) continue;

        uint16_t velocityWeight = (uint16_t)max(1, (int)cell.velocity / 20);
        uint16_t weight = pageWeight * velocityWeight;
        ctx.stepWeight[phase] += weight;
        ctx.stepVelocity[phase] += (uint32_t)cell.velocity * weight;
        CompanionRole role = companionRoleForChannel(ch);
        companionAddRoleWeight(ctx, role, phase, weight);
        ctx.totalWeight += weight;
        ctx.empty = false;

        if ((phase & 3) == 0) onBeatWeight += weight;
        else offBeatWeight += weight;

        if (distance <= 1) {
          if (role == CompanionRole::Kick) ctx.kickMask16 |= (uint16_t)(1u << phase);
          else if (role == CompanionRole::Snare || role == CompanionRole::Clap)
            ctx.snareMask16 |= (uint16_t)(1u << phase);
          else if (role == CompanionRole::Bass)
            ctx.bassMask16 |= (uint16_t)(1u << phase);
          else if (role == CompanionRole::Keys || role == CompanionRole::VocalsPad)
            ctx.harmonicMask16 |= (uint16_t)(1u << phase);
        }

        if (role == CompanionRole::Bass || role == CompanionRole::Keys ||
            role == CompanionRole::VocalsPad) {
          int pc = companionPitchClassForCell(cell, ch, row);
          uint16_t harmW = (role == CompanionRole::Bass) ? (weight * 2u) : weight;
          if ((phase & 3) == 0) harmW += pageWeight;
          ctx.rowWeight[row] += weight;
          ctx.pitchClassWeight[pc] += harmW;
          if (distance == 0) {
            ctx.barPitchClassWeight[bar][pc] += harmW * 2u;
            if (role == CompanionRole::Bass) {
              ctx.barBassPcWeight[bar][pc] += harmW * 3u;
            }
          } else {
            ctx.barPitchClassWeight[bar][pc] += pageWeight;
            if (role == CompanionRole::Bass) {
              ctx.barBassPcWeight[bar][pc] += pageWeight * 2u;
            }
          }
        }
      }
    }
  }

  for (int ch = 1; ch < 16; ch++) {
    uint16_t bestR = 0;
    for (int r = 1; r <= 16; r++) {
      if (chRowHist[ch][r] > bestR) {
        bestR = chRowHist[ch][r];
        ctx.channelHomeRow[ch] = (uint8_t)r;
      }
    }
    if (ctx.channelHitCount[ch] > 0) {
      ctx.channelMeanVel[ch] = (uint8_t)constrain(
          (int)(chVelSum[ch] / ctx.channelHitCount[ch]), 40, 124);
    }
  }

  if (onBeatWeight + offBeatWeight > 0) {
    ctx.syncopationScore = (uint8_t)constrain(
        (int)(offBeatWeight * 100UL / (onBeatWeight + offBeatWeight)), 0, 100);
  }

  uint16_t best = 0;
  for (int row = 1; row <= 16; row++) {
    if (ctx.rowWeight[row] > best) {
      best = ctx.rowWeight[row];
      ctx.rootRow = (uint8_t)row;
    }
  }


  static const int8_t majorScale[] = {0, 2, 4, 5, 7, 9, 11};
  static const int8_t minorScale[] = {0, 2, 3, 5, 7, 8, 10};
  static const int8_t degreeWeights[7] = {6, 2, 4, 3, 5, 3, 2};
  int bestScore = -32768;
  int bestRootPc = (ctx.rootRow - 1) % 12;
  bool bestMinor = false;
  for (int root = 0; root < 12; root++) {
    for (int mode = 0; mode < 2; mode++) {
      const int8_t *scale = mode ? minorScale : majorScale;
      int score = 0;
      for (int pc = 0; pc < 12; pc++) {
        int degree = -1;
        for (int d = 0; d < 7; d++) {
          if ((root + scale[d]) % 12 == pc) {
            degree = d;
            break;
          }
        }
        int multiplier = (degree >= 0) ? degreeWeights[degree] : -4;
        score += (int)ctx.pitchClassWeight[pc] * multiplier;
      }

      score += (int)ctx.barBassPcWeight[0][root] * 4;
      score += (int)ctx.barPitchClassWeight[0][root] * 2;
      if (score > bestScore) {
        bestScore = score;
        bestRootPc = root;
        bestMinor = mode != 0;
      }
    }
  }
  if (ctx.empty) bestRootPc = (ctx.rootRow - 1) % 12;
  ctx.rootRow = (uint8_t)(bestRootPc + 1);
  ctx.isMinor = bestMinor;
  return ctx;
}

FLASHMEM static void clearCompanionChannelPage(int page, uint8_t channel) {
  unsigned int start = (unsigned int)(page - 1) * maxX + 1;
  unsigned int end = min((unsigned int)MAX_STEPS + 1u, start + maxX);
  for (unsigned int c = start; c < end; c++) {
    for (int row = 1; row <= 16; row++) {
      if (note[c][row].channel != channel) continue;
      note[c][row].channel = 0;
      note[c][row].velocity = defaultVelocity;
      note[c][row].probability = 100;
      note[c][row].condition = 1;
      note[c][row].midiPitch = NOTE_MIDI_PITCH_NONE;
    }
  }
}

FLASHMEM static void clearCompanionPage(int page) {
  unsigned int start = (unsigned int)(page - 1) * maxX + 1;
  unsigned int end = min((unsigned int)MAX_STEPS + 1u, start + maxX);
  for (unsigned int c = start; c < end; c++) {
    for (int row = 1; row <= 16; row++) {
      note[c][row].channel = 0;
      note[c][row].velocity = defaultVelocity;
      note[c][row].probability = 100;
      note[c][row].condition = 1;
      note[c][row].midiPitch = NOTE_MIDI_PITCH_NONE;
    }
  }
}

FLASHMEM static int companionFreeRow(unsigned int step, int preferred);

FLASHMEM static int companionScaleRow(const CompanionContext &ctx, bool lowRegister, int motion) {
  static const int8_t majorIntervals[] = {0, 2, 4, 5, 7, 9, 11};
  static const int8_t minorIntervals[] = {0, 2, 3, 5, 7, 8, 10};
  const int8_t *scale = ctx.isMinor ? minorIntervals : majorIntervals;
  int root = ctx.empty ? random(1, 6) : (int)ctx.rootRow;
  int degree = motion % 7;
  if (degree < 0) degree += 7;
  int row = 1 + ((root - 1 + scale[degree]) % 12);
  if (lowRegister) {
    while (row > 8) row -= 7;
    if (row < 1) row = 1;
  } else {
    while (row < 5) row += 7;
    while (row > 16) row -= 12;
  }
  return constrain(row, 1, 16);
}

FLASHMEM static void companionFillScalePcs(const CompanionHarmony &harm, int8_t out[7]) {
  static const int8_t majorIntervals[] = {0, 2, 4, 5, 7, 9, 11};
  static const int8_t minorIntervals[] = {0, 2, 3, 5, 7, 8, 10};
  const int8_t *intervals = harm.isMinor ? minorIntervals : majorIntervals;
  for (int i = 0; i < 7; i++) {
    out[i] = (int8_t)((harm.rootPc + intervals[i]) % 12);
  }
}

FLASHMEM static CompanionHarmony companionMakeHarmony(const CompanionContext &ctx, int variation) {
  CompanionHarmony harm;
  uint32_t tonalWeight = 0;
  for (int pc = 0; pc < 12; pc++) tonalWeight += ctx.pitchClassWeight[pc];
  bool noTonalContext = tonalWeight == 0;
  harm.rootPc = noTonalContext ? (uint8_t)((variation * 5) % 12)
                               : (uint8_t)((ctx.rootRow - 1) % 12);
  harm.isMinor = noTonalContext ? (((variation / 2) & 1) == 0) : ctx.isMinor;


  static const uint8_t majorProg[][4] = {
      {0, 4, 5, 3},
      {0, 3, 4, 0},
      {0, 5, 3, 4},
      {5, 3, 0, 4},
  };
  static const uint8_t minorProg[][4] = {
      {0, 5, 2, 6},
      {0, 3, 5, 4},
      {0, 5, 3, 4},
      {0, 2, 5, 6},
  };
  const uint8_t (*table)[4] = harm.isMinor ? minorProg : majorProg;
  const uint8_t *prog = table[(variation / 3) & 3];

  int8_t scale[7];
  companionFillScalePcs(harm, scale);



  for (int bar = 0; bar < 4; bar++) {
    harm.degree[bar] = prog[bar];
    harm.chordRootPc[bar] = (uint8_t)scale[prog[bar] % 7];
    harm.barHasContext[bar] = false;

    uint32_t barTotal = 0;
    uint16_t bestBassW = 0;
    int bestBassPc = -1;
    for (int pc = 0; pc < 12; pc++) {
      barTotal += ctx.barPitchClassWeight[bar][pc];
      if (ctx.barBassPcWeight[bar][pc] > bestBassW) {
        bestBassW = ctx.barBassPcWeight[bar][pc];
        bestBassPc = pc;
      }
    }
    if (barTotal == 0) continue;
    harm.barHasContext[bar] = true;


    int bestDeg = prog[bar];
    int bestScore = -32768;
    for (int d = 0; d < 7; d++) {
      int rPc = scale[d];
      int tPc = scale[(d + 2) % 7];
      int fPc = scale[(d + 4) % 7];
      int sPc = scale[(d + 6) % 7];
      int sc = (int)ctx.barPitchClassWeight[bar][rPc] * 5 +
               (int)ctx.barPitchClassWeight[bar][tPc] * 3 +
               (int)ctx.barPitchClassWeight[bar][fPc] * 3 +
               (int)ctx.barPitchClassWeight[bar][sPc] * 1 +
               (int)ctx.barBassPcWeight[bar][rPc] * 6;
      if (d == (int)prog[bar]) sc += 2;
      if (sc > bestScore) {
        bestScore = sc;
        bestDeg = d;
      }
    }
    harm.degree[bar] = (uint8_t)bestDeg;
    harm.chordRootPc[bar] = (uint8_t)((bestBassPc >= 0) ? bestBassPc : scale[bestDeg]);
  }
  return harm;
}

FLASHMEM static int companionChordPcs(const CompanionHarmony &harm, int bar, bool withSeventh,
                             int8_t out[4]) {
  int8_t scale[7];
  companionFillScalePcs(harm, scale);
  int b = constrain(bar, 0, 3);
  int deg = harm.degree[b] % 7;
  out[0] = harm.barHasContext[b] ? (int8_t)harm.chordRootPc[b] : scale[deg];
  out[1] = scale[(deg + 2) % 7];
  out[2] = scale[(deg + 4) % 7];
  if (!withSeventh) return 3;
  out[3] = scale[(deg + 6) % 7];
  return 4;
}

FLASHMEM static int companionPcToChannelRow(int pc, uint8_t channel, int lo, int hi, int prefer) {
  pc = ((pc % 12) + 12) % 12;
  lo = constrain(lo, 1, 16);
  hi = constrain(hi, lo, 16);
  prefer = constrain(prefer, lo, hi);
  int best = lo;
  int bestDist = 99;
  for (int row = lo; row <= hi; row++) {
    if (companionPitchClassForRow(channel, row) != pc) continue;
    int dist = abs(row - prefer);
    if (dist < bestDist) {
      bestDist = dist;
      best = row;
    }
  }
  if (bestDist < 99) return best;

  return prefer;
}


FLASHMEM static int companionDegreeToPc(const CompanionHarmony &harm, int bar,
                               CompanionRole role, int motifStep, bool strongBeat) {
  int8_t chord[4];
  bool seventh = (role == CompanionRole::Keys);
  int nChord = companionChordPcs(harm, bar, seventh, chord);
  int8_t scale[7];
  companionFillScalePcs(harm, scale);
  int baseDeg = harm.degree[constrain(bar, 0, 3)] % 7;

  if (role == CompanionRole::Bass) {
    if (strongBeat || motifStep == 0) return chord[0];
    if (motifStep == 1) return chord[2];
    if (motifStep == 2) return chord[1];
    return scale[(baseDeg + motifStep + 7) % 7];
  }
  if (role == CompanionRole::VocalsPad) {
    int idx = ((motifStep % 3) + 3) % 3;
    return chord[idx];
  }
  if (strongBeat) {
    int idx = ((motifStep % nChord) + nChord) % nChord;
    return chord[idx];
  }
  int deg = (baseDeg + motifStep + 14) % 7;
  return scale[deg];
}



FLASHMEM static int companionChordLaneRow(const CompanionHarmony &harm, int bar,
                                 uint8_t channel, int variation, int prevRow) {
  int8_t chord[4];
  bool seventh = ((variation + bar) % 3) == 0;
  int n = companionChordPcs(harm, bar, seventh, chord);
  int inversion = (variation / 3 + bar) % 3;
  int idx = channel == 13 ? (inversion % n) : ((inversion + 1 + (seventh ? 1 : 0)) % n);
  if (channel == 14 && chord[idx] == chord[inversion % n]) {
    idx = (idx + 1) % n;
  }
  int prefer = (prevRow >= 1 && prevRow <= 16) ? prevRow : (channel == 13 ? 7 : 11);
  int lo = (channel == 13) ? 3 : 6;
  int hi = (channel == 13) ? 12 : 16;
  return companionPcToChannelRow(chord[idx], channel, lo, hi, prefer);
}

FLASHMEM static bool companionEuclideanHit(int phase, int pulses, int rotation) {
  pulses = constrain(pulses, 0, 16);
  int shifted = (phase - rotation) % 16;
  if (shifted < 0) shifted += 16;
  return pulses > 0 && ((shifted * pulses) % 16) < pulses;
}

FLASHMEM static uint32_t companionMixHash(uint32_t value) {
  value ^= value >> 16;
  value *= 0x7feb352dUL;
  value ^= value >> 15;
  value *= 0x846ca68bUL;
  return value ^ (value >> 16);
}

FLASHMEM static int8_t companionSoftShift(uint32_t value) {

  int bucket = value % 4;
  return bucket == 0 ? -1 : (bucket == 3 ? 1 : 0);
}

FLASHMEM static CompanionGroove companionMakeGroove(const CompanionContext &ctx,
                                           int page, int variation) {
  uint32_t hash = companionMixHash((uint32_t)variation * 131UL +
                                   (uint32_t)page * 977UL + ctx.totalWeight);
  for (int phase = 0; phase < 16; phase++) {
    hash = companionMixHash(hash + (uint32_t)ctx.stepWeight[phase] * (phase + 17));
  }

  CompanionGroove groove;
  groove.hatPulses = 3 + (hash & 3);
  groove.hatRotation = (hash >> 3) & 15;
  groove.clapPulses = 2 + ((hash >> 7) % 3);
  groove.clapRotation = (hash >> 11) & 15;
  groove.tomRotation = (hash >> 15) & 15;
  groove.snareShiftA = companionSoftShift(hash >> 19);
  groove.snareShiftB = companionSoftShift(hash >> 22);
  groove.secondHalfShift = companionSoftShift(hash >> 25);
  groove.secondHalfRotation = 3 + ((hash >> 28) % 7);
  return groove;
}

FLASHMEM static uint16_t companionMaxPhaseWeight(const uint16_t weights[16]) {
  uint16_t maximum = 0;
  for (int phase = 0; phase < 16; phase++) maximum = max(maximum, weights[phase]);
  return maximum;
}

FLASHMEM static int companionRelativeWeight(uint16_t value, uint16_t maximum) {
  if (maximum == 0) return 0;
  return constrain((int)((uint32_t)value * 100UL / maximum), 0, 100);
}



FLASHMEM static int companionPercussionRow(uint8_t channel, CompanionRole role, int phase,
                                  bool structural, int variation, const CompanionContext &ctx) {
  int home = (channel < 16 && ctx.channelHomeRow[channel] >= 1)
                 ? (int)ctx.channelHomeRow[channel]
                 : (int)companionDefaultHomeRow(role);
  if (structural) return constrain(home, 1, 16);

  if (role == CompanionRole::Kick || role == CompanionRole::Snare ||
      role == CompanionRole::Clap) {

    if (phase >= 14 && ((variation + phase) % 4 == 0)) {
      return constrain(home + ((variation & 1) ? 1 : -1), 1, 16);
    }
    return constrain(home, 1, 16);
  }

  if (role == CompanionRole::ClosedHat) {

    if ((phase & 3) == 2 && ((variation + phase) % 3 == 0)) {
      return constrain(home + 1, 1, 16);
    }
    return constrain(home, 1, 16);
  }

  if (role == CompanionRole::Tom) {

    static const int8_t tomContour[4] = {1, 0, 0, -1};
    int delta = (phase >= 12) ? tomContour[phase - 12] : ((phase & 4) ? 0 : 1);
    return constrain(home + delta, 1, 16);
  }

  return constrain(home, 1, 16);
}

FLASHMEM static int companionFreeRow(unsigned int step, int preferred) {
  preferred = constrain(preferred, 1, 16);
  if (note[step][preferred].channel == 0) return preferred;
  for (int distance = 1; distance <= 4; distance++) {
    int up = preferred + distance;
    int down = preferred - distance;
    if (up <= 16 && note[step][up].channel == 0) return up;
    if (down >= 1 && note[step][down].channel == 0) return down;
  }
  return 0;
}

FLASHMEM static int companionChannelStepLimit(uint8_t channel) {
  if (channel == 11) return 3;
  return 1;
}

FLASHMEM static bool companionPlaceLimited(unsigned int step, int preferredRow,
                                  uint8_t channel, int velocity,
                                  uint8_t probability = 100,
                                  uint8_t condition = 1) {
  int used = 0;
  for (int row = 1; row <= 16; row++) {
    if (note[step][row].channel == channel) used++;
  }
  if (used >= companionChannelStepLimit(channel)) return false;

  int freeRow = companionFreeRow(step, preferredRow);
  if (freeRow == 0) return false;
  placeGenNote(step, freeRow, channel, velocity, probability, condition);
  return true;
}

FLASHMEM static int companionVelocity(const CompanionContext &ctx, int phase, int base) {
  int velocity = base;
  if (ctx.stepWeight[phase] > 0) {
    int weighted = (int)(ctx.stepVelocity[phase] / ctx.stepWeight[phase]);
    if (weighted > 0 && weighted <= 127) velocity = (velocity * 2 + weighted) / 3;
  }
  bool quarter = (phase == 0 || phase == 4 || phase == 8 || phase == 12);
  bool eighth = (phase & 1) == 0;
  velocity += quarter ? random(6, 13) : (eighth ? random(-4, 5) : random(-14, -3));
  return constrain(velocity, 28, 126);
}

FLASHMEM static int companionTempoDensity() {
  int bpm = constrain((int)SMP.bpm, 40, 300);
  if (bpm >= 190) return 58;
  if (bpm >= 155) return 72;
  if (bpm <= 65) return 88;
  if (bpm <= 90) return 96;
  return 100;
}



FLASHMEM static void generateCompanionChannelPage(int page, uint8_t channel,
                                         const CompanionContext &ctx,
                                         const CompanionHarmony &harm,
                                         const CompanionGroove &groove,
                                         int variation) {
  CompanionRole role = companionRoleForChannel(channel);
  if (role == CompanionRole::Unknown) return;

  unsigned int start = (unsigned int)(page - 1) * maxX + 1;
  unsigned int end = min((unsigned int)MAX_STEPS + 1u, start + maxX);
  int tempoDensity = companionTempoDensity();
  uint32_t seedHash = companionMixHash((uint32_t)variation * 2654435761UL +
                                       (uint32_t)page * 40503UL +
                                       (uint32_t)channel * 977UL);



  bool ch11AsPolyComp = (channel == 11 && ctx.channelHitCount[6] >= 3);
  if (ch11AsPolyComp) {
    role = CompanionRole::Keys;
  }



  static const int8_t motifContours[8][8] = {
      {0, 0, 2, 1, 0, 4, 2, 1},
      {0, 2, 4, 2, 0, -1, 1, 2},
      {0, 4, 2, 0, 2, 1, 0, -1},
      {2, 1, 0, 2, 4, 2, 1, 0},
      {0, 1, 2, 4, 2, 1, 0, 2},
      {4, 2, 1, 0, 2, 4, 3, 2},
      {0, 0, 4, 4, 2, 2, 1, 0},
      {0, 2, 1, 3, 2, 0, -1, 0},
  };
  const int8_t *contour = motifContours[seedHash & 7];

  uint8_t callMask8 = 0;
  for (int s = 0; s < 8; s++) {
    bool k = (ctx.kickWeight[s] > 0) || (ctx.kickWeight[s + 8] > 0);
    bool sn = (ctx.snareWeight[s] > 0) || (ctx.clapWeight[s] > 0) ||
              (ctx.snareWeight[s + 8] > 0) || (ctx.clapWeight[s + 8] > 0);
    bool bs = (ctx.bassWeight[s] > 0) || (ctx.bassWeight[s + 8] > 0);
    bool hm = (ctx.harmonicWeight[s] > 0) || (ctx.harmonicWeight[s + 8] > 0);
    uint32_t stepH = companionMixHash(seedHash + (uint32_t)s * 313UL);

    if (role == CompanionRole::Bass) {
      bool onKick = k && !sn;
      bool pocketSync = !k && !sn && (s == 2 || s == 3 || s == 6 || s == 7) &&
                        ((stepH % 100) < (uint32_t)(38 + ctx.syncopationScore / 3));
      if (s == 0 || onKick || pocketSync || (ctx.empty && (s == 0 || s == 3 || s == 6))) {
        callMask8 |= (uint8_t)(1u << s);
      }
    } else if (role == CompanionRole::Keys) {

      bool chordBeat = (s == 0 || s == 4);
      bool offPocket = !sn && (!bs || s == 2 || s == 6) &&
                       (s == 2 || s == 3 || s == 6 || s == 7) &&
                       ((stepH % 100) < (uint32_t)((channel == 7) ? 45 : 32));
      if (chordBeat && ((stepH % 100) < 82)) callMask8 |= (uint8_t)(1u << s);
      else if (offPocket && !hm) callMask8 |= (uint8_t)(1u << s);
    }
  }
  if (role == CompanionRole::Bass && (callMask8 & 0x0Fu) == 0) callMask8 |= 0x01u;
  if (role == CompanionRole::Keys && callMask8 == 0) callMask8 |= 0x11u;

  int prevRow = (channel < 16 && ctx.channelHomeRow[channel] >= 1)
                    ? (int)ctx.channelHomeRow[channel]
                    : (int)companionDefaultHomeRow(role);

  for (unsigned int c = start; c < end; c++) {
    unsigned int pageOffset = c - start;
    int segment = pageOffset / 16;
    int phase = randStep16(pageOffset + 1) - 1;
    int bar = phase / 4;
    int halfStep = phase & 7;
    bool secondHalf = (phase >= 8);
    bool isTurnaround = (phase >= 12);
    bool quarter = (phase & 3) == 0;
    int segmentShift = segment > 0 ? groove.secondHalfShift : 0;
    int snareAnchorA = constrain(4 + groove.snareShiftA + segmentShift, 3, 5);
    int snareAnchorB = constrain(12 + groove.snareShiftB - segmentShift, 11, 13);
    bool backbeat = (phase == snareAnchorA || phase == snareAnchorB);
    bool structural = false;
    bool pitchedPerc = false;
    int chance = 0;
    int row = prevRow;
    int baseVelocity = (channel < 16 && ctx.channelHitCount[channel] > 0)
                           ? (int)ctx.channelMeanVel[channel]
                           : 94;

    switch (role) {
      case CompanionRole::Kick: {
        pitchedPerc = true;
        baseVelocity = 114;
        bool onSnare = (ctx.snareWeight[phase] > 0 || ctx.clapWeight[phase] > 0 || backbeat);
        if (phase == 0) {
          chance = 98;
          structural = true;
        } else if (phase == 8 && !onSnare) {
          chance = (SMP.bpm >= 150 && (variation & 1)) ? 35 : 88;
          structural = (chance >= 80);
        } else if (ctx.bassWeight[phase] > 0 && !onSnare) {
          chance = 78;
          structural = (phase == 4 || phase == 12);
        } else if (!onSnare && (phase == 6 || phase == 10 || phase == 14 || phase == 15)) {

          bool preSnareOrBar = (phase + 1 == snareAnchorB || phase >= 14);
          chance = preSnareOrBar ? 38 : 18;
        } else if (!onSnare && phase == 4 && (variation % 3 == 0)) {
          chance = 76;
          structural = true;
        }
        break;
      }

      case CompanionRole::Snare: {
        pitchedPerc = true;
        baseVelocity = 106;
        if (backbeat) {
          chance = ctx.clapWeight[phase] ? 72 : 94;
          structural = true;
        } else {
          int next = (phase + 1) & 15;
          bool pocketEmpty = (ctx.kickWeight[phase] == 0 && ctx.bassWeight[phase] == 0);
          bool ghostPocket = pocketEmpty && (phase == 7 || phase == 9 || phase == 11 || phase == 15);
          bool turnaroundFill = (phase >= 14 && (segment > 0 || (variation & 1)));
          if (turnaroundFill) {
            chance = 52;
            baseVelocity = 78;
          } else if (ghostPocket && ctx.kickWeight[next] == 0) {
            chance = 26;
            baseVelocity = 62;
          } else {
            chance = 0;
          }
        }
        break;
      }

      case CompanionRole::ClosedHat: {
        pitchedPerc = true;
        baseVelocity = 80;
        int pulses = groove.hatPulses + ((tempoDensity >= 90) ? 2 : 0);
        int rotation = (groove.hatRotation + (segment > 0 ? groove.secondHalfRotation : 0)) & 15;
        bool offbeat8th = ((phase & 3) == 2);
        bool even8th = ((phase & 1) == 0);
        bool euclid = companionEuclideanHit(phase, constrain(pulses, 4, 8), rotation);
        if (offbeat8th) {
          chance = 92;
          structural = true;
          baseVelocity = 88;
        } else if (even8th || euclid) {
          chance = (ctx.snareWeight[phase] > 0) ? 55 : 80;
          structural = true;
          baseVelocity = 74;
        } else {

          bool preSnare = (((phase + 1) & 15) == snareAnchorA || ((phase + 1) & 15) == snareAnchorB);
          chance = (preSnare || isTurnaround) ? 38 : 14;
          baseVelocity = 58;
        }
        break;
      }

      case CompanionRole::Clap: {
        pitchedPerc = true;
        baseVelocity = 96;
        int rotation = (groove.clapRotation + (segment > 0 ? groove.secondHalfRotation : 0)) & 15;
        bool euclid = companionEuclideanHit(phase, groove.clapPulses, rotation);
        if (backbeat) {
          chance = ctx.snareWeight[phase] ? 64 : 88;
          structural = true;
        } else if (euclid && ctx.kickWeight[phase] == 0 && (phase == 3 || phase == 6 || phase == 11 || phase == 14 || phase == 15)) {
          chance = 34;
          baseVelocity = 78;
        }
        break;
      }

      case CompanionRole::Tom: {
        pitchedPerc = true;
        baseVelocity = 88;
        bool pocketFree = (ctx.kickWeight[phase] == 0 && ctx.snareWeight[phase] == 0);
        if (isTurnaround && pocketFree && (phase == 13 || phase == 14 || phase == 15)) {
          chance = 58;
          structural = (phase == 14);
        } else if (pocketFree && (phase == 3 || phase == 6 || phase == 11) &&
                   companionEuclideanHit(phase, 3, groove.tomRotation)) {
          chance = 28;
        }
        break;
      }

      case CompanionRole::Bass: {
        baseVelocity = 106;
        bool motifHit = ((callMask8 >> halfStep) & 1u) != 0;
        if (secondHalf && isTurnaround) {

          motifHit = (phase == 12 && ctx.snareWeight[12] == 0) ||
                     (phase == 14) ||
                     (phase == 15 && (variation & 1));
        }
        if (ctx.kickWeight[phase] > 0 && ctx.snareWeight[phase] == 0) {
          chance = 92;
          structural = true;
        } else if (motifHit && ctx.snareWeight[phase] == 0) {
          chance = secondHalf ? 82 : 88;
          structural = (phase == 0 || phase == 8);
        } else {
          chance = 0;
        }

        if (chance > 0) {
          int degStep = contour[halfStep];
          if (phase == 0 || phase == 8 || (ctx.kickWeight[phase] > 0 && quarter)) {
            degStep = 0;
          } else if (phase == 14) {
            degStep = 4;
          } else if (phase == 15) {
            degStep = 6;
          } else if (secondHalf) {
            degStep = contour[(halfStep + 2) & 7];
          }
          int pc = companionDegreeToPc(harm, bar, CompanionRole::Bass, degStep,
                                       quarter || ctx.kickWeight[phase] > 0);
          int prefer = (phase == 14 || ( !quarter && (halfStep & 1))) ? 5 : 3;
          row = companionPcToChannelRow(pc, channel, 1, 8, prefer);
          prevRow = row;
        }
        break;
      }

      case CompanionRole::Keys: {
        baseVelocity = ch11AsPolyComp ? 84 : 86;
        bool motifHit = ((callMask8 >> halfStep) & 1u) != 0;
        if (secondHalf && phase >= 13) {

          motifHit = (phase == 14) || (phase == 15 && (channel == 7));
        }

        if (channel == 13 || channel == 14) {
          bool chordStab = (phase == 0 || phase == 6 || phase == 8 || phase == 12 ||
                            (phase == 3 && (variation & 1)) ||
                            (phase == 14 && ((variation + segment) & 1)));
          if (ctx.snareWeight[phase] > 0 && !quarter) chordStab = false;
          if (!chordStab) continue;
          row = companionChordLaneRow(harm, bar, channel, variation + segment * 3, prevRow);
          prevRow = row;
          companionPlaceLimited(c, row, channel,
                                companionVelocity(ctx, phase, baseVelocity));
          continue;
        }

        if (ch11AsPolyComp) {

          bool compHit = (phase == 0 || phase == 6 || phase == 8 || phase == 12 ||
                          (phase == 2 && ctx.bassWeight[2] == 0) ||
                          (phase == 14 && ctx.snareWeight[14] == 0));
          if (!compHit) continue;
          int8_t chord[4];
          int nChord = companionChordPcs(harm, bar, true, chord);
          int voices = (phase == 0 || phase == 8) ? 3 : 2;
          int vel = companionVelocity(ctx, phase, baseVelocity);
          for (int v = 0; v < voices; v++) {
            int pc = chord[(v + bar + (variation & 1)) % nChord];
            int targetPrefer = 6 + v * 3;
            int r = companionPcToChannelRow(pc, channel, 4, 15, targetPrefer);
            companionPlaceLimited(c, r, channel, max(36, vel - v * 6));
          }
          continue;
        }


        if (!motifHit) continue;
        int degStep = contour[halfStep];
        if (secondHalf) {

          degStep = isTurnaround ? ((phase == 14) ? 2 : 0) : (degStep + 2);
        }
        int pc = companionDegreeToPc(harm, bar, CompanionRole::Keys, degStep, quarter);
        int prefer = constrain(prevRow + ((degStep > 2) ? 1 : -1), 5, 14);
        row = companionPcToChannelRow(pc, channel, 3, 16, prefer);
        prevRow = row;
        companionPlaceLimited(c, row, channel,
                              companionVelocity(ctx, phase, baseVelocity));
        continue;
      }

      case CompanionRole::VocalsPad: {

        baseVelocity = 74;
        bool padHit = (phase == 0 || phase == 8 ||
                       (phase == 12 && harm.degree[3] != harm.degree[2]));
        if (!padHit) continue;
        int toneIdx = (bar + (variation & 1) + segment) % 3;
        int pc = companionDegreeToPc(harm, bar, CompanionRole::VocalsPad, toneIdx, true);
        row = companionPcToChannelRow(pc, channel, 4, 15, prevRow);
        prevRow = row;
        companionPlaceLimited(c, row, channel,
                              companionVelocity(ctx, phase, baseVelocity));
        continue;
      }

      default:
        break;
    }

    if (chance <= 0) continue;
    if (!structural) chance = chance * tempoDensity / 100;
    chance = constrain(chance, 0, 100);
    if ((int)random(100) >= chance) continue;

    if (pitchedPerc) {
      row = companionPercussionRow(channel, role, phase, structural, variation, ctx);
    }
    companionPlaceLimited(c, row, channel,
                          companionVelocity(ctx, phase, baseVelocity));
  }
}

FLASHMEM void drawRandoms() {
  uint8_t channel = (uint8_t)GLOB.currentChannel;
  if (!companionChannelSupported(channel)) return;

  int page = constrain((int)GLOB.edit, 1, companionEffectivePages());
  int activePages = companionActivePages();
  CompanionContext context = analyzeCompanionContext(1, activePages, page, channel);
  int seed = random(0, 256);
  CompanionHarmony harm = companionMakeHarmony(context, seed);
  CompanionGroove groove = companionMakeGroove(context, page, seed);

  clearCompanionChannelPage(page, channel);
  generateCompanionChannelPage(page, channel, context, harm, groove, seed);
  updateLastPage();
}




FLASHMEM void drawRandomsMultiPage() {
  extern bool isChildVoiceDisabled(int channel);
  int page = constrain((int)GLOB.edit, 1, companionEffectivePages());
  int activePages = companionActivePages();
  int seed = random(1, 256);





  bool pick[16] = {};
  pick[1] = (random(100) < 90);
  int backbeatPick = random(100);
  if (backbeatPick < 48) {
    pick[2] = true;
  } else if (backbeatPick < 78) {
    pick[4] = true;
  } else {
    pick[2] = true;
    pick[4] = true;
  }
  pick[3] = (random(100) < 82);
  pick[5] = (random(100) < 35);

  int bassPick = random(100);
  if (bassPick < 45) {
    pick[6] = true;
  } else if (bassPick < 82) {
    pick[11] = true;
  } else {
    pick[6] = true;
    pick[11] = true;
  }

  int harmPick = random(100);
  if (harmPick < 38) {
    pick[7] = true;
  } else if (harmPick < 76) {
    pick[13] = true;
    pick[14] = true;
  } else {
    pick[7] = true;
    pick[8] = true;
  }
  if (!pick[8] && random(100) < 30) {
    pick[8] = true;
  }


  clearCompanionPage(page);


  CompanionContext baseCtx = analyzeCompanionContext(1, activePages, page, 0);
  CompanionHarmony pageHarm = companionMakeHarmony(baseCtx, seed);
  CompanionGroove pageGroove = companionMakeGroove(baseCtx, page, seed);


  static const uint8_t generationOrder[] = {1, 2, 4, 3, 5, 6, 11, 13, 14, 7, 8};
  for (uint8_t ch : generationOrder) {
    if (!pick[ch] || isChildVoiceDisabled(ch)) continue;
    CompanionContext ctx = analyzeCompanionContext(1, max(activePages, page), page, ch);
    generateCompanionChannelPage(page, ch, ctx, pageHarm, pageGroove, seed);
  }

  updateLastPage();
}


FLASHMEM void generateRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {



  int patternType = (channel - 1) % 4;

  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel);

    bool shouldPlay = false;
    int velocity = defaultVelocity;

    switch(patternType) {
      case 0:
        shouldPlay = (beatPosition == 1 || beatPosition == 9);

        if(random(0, 100) < 20 && (beatPosition == 5 || beatPosition == 13)) {
          shouldPlay = true;
          velocity = 60;
        } else {
          velocity = 120;
        }
        break;
      case 1:
        shouldPlay = (beatPosition == 5 || beatPosition == 13);

        if(random(0, 100) < 30 && (beatPosition == 3 || beatPosition == 7 || beatPosition == 11 || beatPosition == 15)) {
          shouldPlay = true;
          velocity = 70;
        } else {
          velocity = 100;
        }
        break;
      case 2:
        shouldPlay = (beatPosition % 2 == 0);

        if(random(0, 100) < 15 && (beatPosition == 5 || beatPosition == 13)) {
          velocity = 90;
        } else {
          velocity = 80;
        }
        break;
      case 3:

        if(beatPosition == 1 || beatPosition == 9) {
          shouldPlay = true;
          velocity = 110;
        } else if(random(0, 100) < 40) {
          shouldPlay = true;
          velocity = 85;
        }
        break;
    }

    if(shouldPlay) {

      int noteRow = 1;
      if(harmony.hasRoot) {
        noteRow = harmony.rootNote;

        if(random(0, 100) < 20) {
          if(harmony.hasFifth) noteRow = 5;
          else if(harmony.hasThird) noteRow = (harmony.isMajor) ? 3 : 6;
        }
      } else if(harmony.hasFifth) {
        noteRow = 5;
      } else {
        noteRow = random(1, 9);
      }

      placeRandomNoteIfEmpty(c, noteRow, channel, velocity);
    }
  }
}


FLASHMEM void generateMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {

  int scaleNotes[8];
  int scaleSize = 0;

  if(harmony.isMajor) {
    int majorScale[] = {1, 3, 5, 6, 8, 10, 12, 13};
    for(int i = 0; i < 8; i++) {
      scaleNotes[i] = majorScale[i];
    }
    scaleSize = 8;
  } else if(harmony.isMinor) {
    int minorScale[] = {6, 8, 10, 11, 13, 15, 1, 3};
    for(int i = 0; i < 8; i++) {
      scaleNotes[i] = minorScale[i];
    }
    scaleSize = 8;
  } else {

    int pentatonic[] = {1, 3, 5, 8, 10};
    for(int i = 0; i < 5; i++) {
      scaleNotes[i] = pentatonic[i];
    }
    scaleSize = 5;
  }


  int voiceType = (channel - 5) % 4;
  int phraseLength = 4;
  int currentNote = 0;

  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel);


    if(beatPosition % phraseLength == 1) {

      currentNote = random(0, scaleSize);
    } else {

      int direction = random(0, 5);

      switch(voiceType) {
        case 0:
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;
          else if(direction == 3 && currentNote < scaleSize - 2) currentNote += 2;
          else if(direction == 4 && currentNote > 1) currentNote -= 2;
          break;
        case 1:
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;
          else if(direction == 3 && currentNote < scaleSize - 3) currentNote += 3;
          else if(direction == 4 && currentNote > 2) currentNote -= 3;
          break;
        case 2:
          if(direction == 1) currentNote = (currentNote + 2) % scaleSize;
          else if(direction == 2) currentNote = (currentNote + 3) % scaleSize;
          else if(direction == 3) currentNote = (currentNote + 4) % scaleSize;
          else if(direction == 4) currentNote = (currentNote - 1 + scaleSize) % scaleSize;
          break;
        case 3:
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;

          break;
        }
      }


    bool shouldPlay = false;
    int velocity = defaultVelocity;

    switch(voiceType) {
      case 0:
        if(beatPosition % 2 == 1) shouldPlay = true;
        else if(random(0, 100) < 30) shouldPlay = true;
        velocity += random(-15, 16);
        break;
      case 1:
        if(random(0, 100) < 60) shouldPlay = true;
        velocity += random(-10, 21);
        break;
      case 2:
        if(beatPosition % 4 == 1) shouldPlay = true;
        else if(random(0, 100) < 25) shouldPlay = true;
        velocity += random(-5, 26);
        break;
      case 3:
        if(beatPosition == 1 || beatPosition == 9) shouldPlay = true;
        else if(random(0, 100) < 20) shouldPlay = true;
        velocity += random(-5, 11);
        break;
    }

    if(shouldPlay && scaleSize > 0) {
      int noteRow = scaleNotes[currentNote];
      placeRandomNoteIfEmpty(c, noteRow, channel, velocity);
    }
  }
}


FLASHMEM void generateBassOrMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {

  bool generateBass = (random(0, 100) < 80);

  if(generateBass) {

    generateBassLine(start, end, channel, harmony);
  } else {

    generateMainMelody(start, end, channel, harmony);
  }
}


FLASHMEM void generateBassLine(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {

  int bassNotes[] = {1, 2, 3, 4, 5, 6, 7, 8};
  int bassSize = 8;

  int currentBassNote = 0;
  int phraseLength = 4;

  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel);


    if(beatPosition % phraseLength == 1) {
      currentBassNote = random(0, bassSize);
    } else {

      int direction = random(0, 6);
      if(direction == 1 && currentBassNote < bassSize - 1) currentBassNote++;
      else if(direction == 2 && currentBassNote > 0) currentBassNote--;
      else if(direction == 3 && currentBassNote < bassSize - 3) currentBassNote += 2;
      else if(direction == 4 && currentBassNote > 2) currentBassNote -= 2;
    }


    bool shouldPlay = false;
    if(beatPosition == 1 || beatPosition == 5 || beatPosition == 9 || beatPosition == 13) {
      shouldPlay = true;
    } else if(random(0, 100) < 25) {
      shouldPlay = true;
    }

    if(shouldPlay) {
      int noteRow = bassNotes[currentBassNote];
      placeRandomNoteIfEmpty(c, noteRow, channel, defaultVelocity + random(10, 31));
    }
  }
}


FLASHMEM void generateMainMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {

  int melodyNotes[] = {10, 12, 13, 15, 1, 3, 5, 6};
  int melodySize = 8;

  int currentNote = 0;
  int phraseLength = 8;

  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel);


    if(beatPosition % phraseLength == 1) {
      currentNote = random(0, melodySize);
    } else {

      int direction = random(0, 5);
      if(direction == 1 && currentNote < melodySize - 1) currentNote++;
      else if(direction == 2 && currentNote > 0) currentNote--;
      else if(direction == 3 && currentNote < melodySize - 2) currentNote += 2;
      else if(direction == 4 && currentNote > 1) currentNote -= 2;
    }


    bool shouldPlay = false;
    if(random(0, 100) < 70) shouldPlay = true;

    if(shouldPlay && melodySize > 0) {

      int noteCount;
      int chordChance = random(0, 100);
      if(chordChance < 80) {
        noteCount = 1;
      } else if(chordChance < 95) {
        noteCount = 2;
      } else {
        noteCount = 3;
      }

      int rootNote = melodyNotes[currentNote];

      if(noteCount == 1) {
        placeRandomNoteIfEmpty(c, rootNote, channel, defaultVelocity + random(-10, 31));
      } else if(noteCount == 2) {

        int rootPianoIndex = rootNote - 1;
        int intervalChoice = random(0, 4);
        int secondPianoIndex;

        switch(intervalChoice) {
          case 0: secondPianoIndex = rootPianoIndex + 3; break;
          case 1: secondPianoIndex = rootPianoIndex + 5; break;
          case 2: secondPianoIndex = rootPianoIndex + 7; break;
          case 3: secondPianoIndex = rootPianoIndex + 12; break;
        }


        if(secondPianoIndex > 15) secondPianoIndex -= 12;
        if(secondPianoIndex < 0) secondPianoIndex += 12;

        int secondNote = secondPianoIndex + 1;

        placeRandomNoteIfEmpty(c, rootNote, channel, defaultVelocity + random(-5, 21));
        placeRandomNoteIfEmpty(c, secondNote, channel, defaultVelocity + random(-10, 16));
      } else {

        int chordType;
        if(random(0, 100) < 90) {

          chordType = random(0, 2);
        } else {

          chordType = 2;
        }


        int rootPianoIndex = rootNote - 1;
        int secondPianoIndex, thirdPianoIndex;

        switch(chordType) {
          case 0:
            secondPianoIndex = rootPianoIndex + 4;
            thirdPianoIndex = rootPianoIndex + 7;
            break;
          case 1:
            secondPianoIndex = rootPianoIndex + 3;
            thirdPianoIndex = rootPianoIndex + 7;
            break;
          case 2:
            secondPianoIndex = rootPianoIndex + 5;
            thirdPianoIndex = rootPianoIndex + 7;
            break;
        }


        if(secondPianoIndex > 15) secondPianoIndex -= 12;
        if(thirdPianoIndex > 15) thirdPianoIndex -= 12;
        if(secondPianoIndex < 0) secondPianoIndex += 12;
        if(thirdPianoIndex < 0) thirdPianoIndex += 12;

        int secondNote = secondPianoIndex + 1;
        int thirdNote = thirdPianoIndex + 1;

        placeRandomNoteIfEmpty(c, rootNote, channel, defaultVelocity + random(-5, 21));
        placeRandomNoteIfEmpty(c, secondNote, channel, defaultVelocity + random(-10, 16));
        placeRandomNoteIfEmpty(c, thirdNote, channel, defaultVelocity + random(-10, 16));
      }
    }
  }
}






FLASHMEM int createHarmonicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {

  int scaleNotes[16];
  int scaleSize = 0;


  if (harmony.isMajor) {

    int majorScale[] = {1, 3, 5, 6, 8, 10, 12, 13};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = majorScale[i];
    }
    scaleSize = 8;
  } else if (harmony.isMinor) {

    int minorScale[] = {6, 8, 10, 11, 13, 15, 1, 3};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = minorScale[i];
    }
    scaleSize = 8;
  } else {

    int pentatonic[] = {1, 3, 5, 8, 10};
    for (int i = 0; i < 5; i++) {
      scaleNotes[i] = pentatonic[i];
    }
    scaleSize = 5;
  }


  int scaleDegree = -1;
  for (int i = 0; i < scaleSize; i++) {
    if (baseNote == scaleNotes[i]) {
      scaleDegree = i;
      break;
    }
  }


  if (scaleDegree == -1) {
    int minDistance = 16;
    for (int i = 0; i < scaleSize; i++) {
      int distance = abs(baseNote - scaleNotes[i]);
      if (distance < minDistance) {
        minDistance = distance;
        scaleDegree = i;
      }
    }
  }



  int progressionPatterns[][8] = {
    {0, 5, 3, 4, 0, 5, 3, 4},
    {0, 4, 5, 3, 0, 4, 5, 3},
    {5, 3, 0, 4, 5, 3, 0, 4},
    {0, 2, 3, 4, 0, 2, 3, 4},
    {0, 4, 2, 3, 0, 4, 2, 3},
    {0, 1, 3, 4, 0, 1, 3, 4},
    {0, 3, 4, 5, 0, 3, 4, 5},
    {0, 4, 1, 3, 0, 4, 1, 3}
  };

  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 8;
  int targetDegree = progressionPatterns[patternIndex][progressionStep];


  if (random(0, 100) < 10) {
    targetDegree += random(-1, 2);
  }


  int newScaleDegree = (scaleDegree + targetDegree) % scaleSize;
  if (newScaleDegree < 0) newScaleDegree += scaleSize;

  int resultNote = scaleNotes[newScaleDegree];


  if (random(0, 100) < 8) {
    if (random(0, 2) == 0) {
      resultNote += 7;
      if (resultNote > 16) resultNote -= 7;
    } else {
      resultNote -= 7;
      if (resultNote < 1) resultNote += 7;
    }
  }

  return resultNote;
}


FLASHMEM int createMelodicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {

  int scaleNotes[16];
  int scaleSize = 0;


  if (harmony.isMajor) {
    int majorScale[] = {1, 3, 5, 6, 8, 10, 12, 13};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = majorScale[i];
    }
    scaleSize = 8;
  } else if (harmony.isMinor) {
    int minorScale[] = {6, 8, 10, 11, 13, 15, 1, 3};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = minorScale[i];
    }
    scaleSize = 8;
  } else {
    int pentatonic[] = {1, 3, 5, 8, 10};
    for (int i = 0; i < 5; i++) {
      scaleNotes[i] = pentatonic[i];
    }
    scaleSize = 5;
  }


  int scaleDegree = -1;
  for (int i = 0; i < scaleSize; i++) {
    if (baseNote == scaleNotes[i]) {
      scaleDegree = i;
      break;
    }
  }


  if (scaleDegree == -1) {
    int minDistance = 16;
    for (int i = 0; i < scaleSize; i++) {
      int distance = abs(baseNote - scaleNotes[i]);
      if (distance < minDistance) {
        minDistance = distance;
        scaleDegree = i;
      }
    }
  }



  int melodicPatterns[][8] = {
    {0, 1, 2, 1, 0, -1, -2, -1},
    {0, 2, 4, 2, 0, -2, -4, -2},
    {0, 4, 2, 6, 4, 0, -2, 2},
    {0, 1, 3, 2, 4, 3, 5, 4},
    {0, 2, 1, 3, 2, 4, 3, 5},
    {0, 3, 1, 4, 2, 5, 3, 6},
    {0, -1, 1, -2, 2, -3, 3, -4},
    {0, 1, 0, 2, 1, 3, 2, 4}
  };

  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 8;
  int melodicOffset = melodicPatterns[patternIndex][progressionStep];


  if (random(0, 100) < 12) {
    melodicOffset += random(-1, 2);
  }


  int newScaleDegree = (scaleDegree + melodicOffset) % scaleSize;
  if (newScaleDegree < 0) newScaleDegree += scaleSize;

  int resultNote = scaleNotes[newScaleDegree];


  if (random(0, 100) < 6) {
    if (random(0, 2) == 0) {
      resultNote += 7;
      if (resultNote > 16) resultNote -= 7;
    } else {
      resultNote -= 7;
      if (resultNote < 1) resultNote += 7;
    }
  }


  if (random(0, 100) < 5) {
    if (random(0, 2) == 0) {
      resultNote += 1;
      if (resultNote > 16) resultNote -= 1;
    } else {
      resultNote -= 1;
      if (resultNote < 1) resultNote += 1;
    }
  }

  return resultNote;
}


FLASHMEM int createRhythmicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {




  int rhythmicVariations[][4] = {
    {0, 0, 0, 0},
    {0, 1, 0, -1},
    {0, 0, 1, 0},
    {0, 0, -1, 0},
    {0, 2, 0, -2},
    {0, 0, 2, 0},
    {0, 0, -2, 0},
    {0, 1, -1, 0}
  };

  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 4;
  int pitchOffset = rhythmicVariations[patternIndex][progressionStep];


  if (random(0, 100) < 5) {
    pitchOffset += random(-1, 2);
  }


  int resultNote = baseNote + pitchOffset;


  if (resultNote < 1) resultNote = 1;
  if (resultNote > 16) resultNote = 16;


  if (random(0, 100) < 2) {
    if (random(0, 2) == 0) {
      resultNote += 7;
      if (resultNote > 16) resultNote -= 7;
    } else {
      resultNote -= 7;
      if (resultNote < 1) resultNote += 7;
    }
  }

  return resultNote;
}


FLASHMEM void generateGenreTrack() {

  for (unsigned int page = 1; page <= maxPages; page++) {
    unsigned int start = (page - 1) * 16 + 1;
    unsigned int end = page * 16;
    for (unsigned int c = start; c <= end; c++) {
      for (int row = 1; row <= 16; row++) {
        note[c][row].channel = 0;
        note[c][row].velocity = 0;
        note[c][row].probability = 100;
        note[c][row].condition = 1;
        note[c][row].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    }
  }


  for (unsigned int page = 1; page <= genreLength; page++) {
    unsigned int start = (page - 1) * 16 + 1;
    unsigned int end = page * 16;

    switch (genreType) {
      case 0:
        break;

      case 1:
        generateTechnoPattern(start, end, page);
        break;

      case 2:
        generateHipHopPattern(start, end, page);
        break;

      case 3:
        generateDnBPattern(start, end, page);
        break;

      case 4:
        generateHousePattern(start, end, page);
        break;

      case 5:
        generateAmbientPattern(start, end, page);
        break;
    }
  }


  setGenreBPM();

  extern void resetAllChannelVolumesToDefault();
  resetAllChannelVolumesToDefault();


  switchMode(&draw);
}


FLASHMEM void setGenreBPM() {
  int targetBPM = 100;

  switch (genreType) {
    case 0:
      targetBPM = 100;
      break;
    case 1:
      targetBPM = 128 + random(-4, 5);
      break;
    case 2:
      targetBPM = 85 + random(-3, 4);
      break;
    case 3:
      targetBPM = 174 + random(-4, 5);
      break;
    case 4:
      targetBPM = 124 + random(-4, 5);
      break;
    case 5:
      targetBPM = 70 + random(-5, 6);
      break;
  }


  SMP.bpm = targetBPM;


  bpmMode.pos[3] = targetBPM;


  if (currentMode == &bpmMode) {
    Encoder[3].writeCounter((int32_t)targetBPM);
  }


  applyBPMDirectly(targetBPM);
}


void applyBPMDirectly(int bpm) {
  if (bpm > 0) {
    playNoteInterval = 60000000.0 / ((double)bpm * 4.0);
    playTimer.update((uint32_t)round(playNoteInterval));
  }
}


FLASHMEM void generateTechnoPattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMinor = true;
  const int baseNote = 8;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);


    bool kickPlay = false;
    if (pageOffset % 4 == 0) {

      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 4 == 1) {

      kickPlay = (beat % 2 == 1);
    } else if (pageOffset % 4 == 2) {

      kickPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 9 || beat == 12 || beat == 15);
    } else {

      kickPlay = (beat == 1 || beat == 2 || beat == 5 || beat == 6 || beat == 9 || beat == 10 || beat == 13 || beat == 14);
    }

    if (kickPlay) {
      int vel = 118 + ((beat == 1) ? 6 : 0) + random(-3, 4);
      placeGenNote(c, 3, 1, vel, 100, 1);
    }


    bool snarePlay = false;
    if (pageOffset % 3 == 0) {

      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {

      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 60 + random(-10, 11);
      } else {
        note[c][5].velocity = 80 + random(-10, 11);
      }
    } else {

      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 50 + random(-10, 11);
      } else {
        note[c][5].velocity = 75 + random(-10, 11);
      }
    }

    if (snarePlay) {
      int vel = 112 + random(-4, 5);
      uint8_t prob = 100;
      if (pageOffset % 3 == 1 && (beat == 3 || beat == 11)) {
        vel = 58 + random(-4, 5);
        prob = 45;
      } else if (pageOffset % 3 == 2 && (beat == 4 || beat == 6 || beat == 12 || beat == 14)) {
        vel = 62 + random(-5, 6);
        prob = 60;
      }
      placeGenNote(c, 5, 2, vel, prob, 1);
    }


    if (beat % 2 == 0) {
      int vel = 72 + ((beat % 4 == 0) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 7, 3, vel, 100, 1);
    }


    if (beat == 5 || beat == 13) {
      int vel = 92 + random(-4, 5);
      uint8_t prob = (pageOffset % 4 == 2) ? 55 : 85;
      placeGenNote(c, 8, 4, vel, prob, 1);
    }


    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      int vel = 62 + random(-5, 6);
      placeGenNote(c, 6, 5, vel, 40, 1);
    }


    bool bassPlay = false;
    if (pageOffset % 2 == 0) {

      bassPlay = (beat == 1 || beat == 9);
    } else {

      bassPlay = (beat == 1 || beat == 3 || beat == 5 || beat == 7 || beat == 9 || beat == 11 || beat == 13 || beat == 15);
    }

    if (bassPlay) {
      int row = createRhythmicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      if (row > 9) row -= 7;
      int vel = 102 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 100, 1);
    }


    if (random(0, 100) < 18) {
      int row = createMelodicProgression(chordRoot, pageOffset + (beat / 2), harmony);
      int vel = 84 + ((beat == 9) ? 8 : 0) + random(-4, 5);
      placeGenNote(c, row, 7, vel, 70, 1);
    }


    if (random(0, 100) < 10) {
      int row = chordRoot;
      int r = random(0, 3);
      if (r == 1) row += 2;
      if (r == 2) row += 4;
      if (row > 16) row -= 7;
      int vel = 96 + random(-4, 5);
      placeGenNote(c, row, 8, vel, 55, 2);
    }
  }
}


FLASHMEM void generateHipHopPattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMinor = true;
  const int baseNote = 6;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);


    bool kickPlay = false;
    if (pageOffset % 4 == 0) {

      kickPlay = (beat == 1 || beat == 7 || beat == 11);
    } else if (pageOffset % 4 == 1) {

      kickPlay = (beat == 1 || beat == 3 || beat == 7 || beat == 11 || beat == 15);
    } else if (pageOffset % 4 == 2) {

      kickPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 10 || beat == 13);
    } else {

      kickPlay = (beat == 1 || beat == 2 || beat == 7 || beat == 8 || beat == 11 || beat == 12);
    }

    if (kickPlay) {
      int vel = 118 + ((beat == 1) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 3, 1, vel, 100, 1);
    }


    bool snarePlay = false;
    if (pageOffset % 3 == 0) {

      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {

      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 70 + random(-10, 11);
      } else {
        note[c][5].velocity = 100 + random(-10, 11);
      }
    } else {

      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 60 + random(-10, 11);
      } else {
        note[c][5].velocity = 95 + random(-10, 11);
      }
    }

    if (snarePlay) {
      int vel = 112 + random(-4, 5);
      uint8_t prob = 100;
      if (pageOffset % 3 == 1 && (beat == 3 || beat == 11)) {
        vel = 60 + random(-5, 6);
        prob = 45;
      } else if (pageOffset % 3 == 2 && (beat == 4 || beat == 6 || beat == 12 || beat == 14)) {
        vel = 64 + random(-5, 6);
        prob = 60;
      }
      placeGenNote(c, 5, 2, vel, prob, 1);
    }


    if (beat % 2 == 0) {
      int vel = 68 + ((beat == 8 || beat == 16) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 7, 3, vel, 90, 1);
    }


    if (beat == 5 || beat == 13) {
      int vel = 92 + random(-4, 5);
      uint8_t prob = (pageOffset % 4 == 3) ? 55 : 80;
      placeGenNote(c, 8, 4, vel, prob, 1);
    }


    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      int vel = 58 + random(-5, 6);
      placeGenNote(c, 6, 5, vel, 30, 1);
    }


    bool bassPlay = false;
    if (pageOffset % 2 == 0) {

      bassPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else {

      bassPlay = (beat == 1 || beat == 3 || beat == 5 || beat == 7 || beat == 9 || beat == 11 || beat == 13 || beat == 15);
    }

    if (bassPlay) {
      int row = createRhythmicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      if (row > 9) row -= 7;
      int vel = 98 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 100, 1);
    }


    if (random(0, 100) < 14) {
      int row = createMelodicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      int vel = 88 + random(-4, 5);
      placeGenNote(c, row, 7, vel, 60, 2);
    }


    if (random(0, 100) < 8) {
      int row = chordRoot;
      if (random(0, 2) == 0) row += 3;
      if (row > 16) row -= 7;
      int vel = 90 + random(-4, 5);
      placeGenNote(c, row, 8, vel, 45, 4);
    }
  }
}


FLASHMEM void generateDnBPattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMinor = true;
  const int baseNote = 6;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);


    bool kickPlay = false;
    if (pageOffset % 4 == 0) {

      kickPlay = (beat == 1 || beat == 9);
    } else if (pageOffset % 4 == 1) {

      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 4 == 2) {

      kickPlay = (beat == 1 || beat == 7 || beat == 11);
    } else {

      kickPlay = (beat == 1 || beat == 3 || beat == 9 || beat == 11);
    }

    if (kickPlay) {
      int vel = 120 + ((beat == 1) ? 6 : 0) + random(-3, 4);
      placeGenNote(c, 3, 1, vel, 100, 1);
    }


    bool snarePlay = false;
    if (pageOffset % 3 == 0) {

      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {

      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 70 + random(-10, 11);
      } else {
        note[c][5].velocity = 110 + random(-10, 11);
      }
    } else {

      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 60 + random(-10, 11);
      } else {
        note[c][5].velocity = 100 + random(-10, 11);
      }
    }

    if (snarePlay) {
      int vel = 114 + random(-4, 5);
      uint8_t prob = 100;
      if (pageOffset % 3 == 1 && (beat == 3 || beat == 11)) {
        vel = 62 + random(-4, 5);
        prob = 45;
      } else if (pageOffset % 3 == 2 && (beat == 4 || beat == 6 || beat == 12 || beat == 14)) {
        vel = 66 + random(-5, 6);
        prob = 60;
      }
      placeGenNote(c, 5, 2, vel, prob, 1);
    }


    if (beat % 2 == 0) {
      int vel = 74 + ((beat % 4 == 0) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 7, 3, vel, 100, 1);
    }


    if (beat == 5 || beat == 13) {
      int vel = 94 + random(-4, 5);
      placeGenNote(c, 8, 4, vel, 80, 1);
    }


    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      int vel = 60 + random(-5, 6);
      placeGenNote(c, 6, 5, vel, 35, 1);
    }


    bool bassPlay = false;
    if (pageOffset % 2 == 0) {

      bassPlay = (beat % 2 == 1);
    } else {

      bassPlay = (beat == 1 || beat == 3 || beat == 6 || beat == 9 || beat == 11 || beat == 14);
    }

    if (bassPlay) {
      int row = createRhythmicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      if (row > 9) row -= 7;
      if (row < 1) row = 1;
      int vel = 104 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 100, 1);
    }


    if (random(0, 100) < 12) {
      int row = createMelodicProgression(chordRoot, pageOffset + (beat / 2), harmony);
      int vel = 88 + random(-4, 5);
      placeGenNote(c, row, 7, vel, 55, 2);
    }
  }
}


FLASHMEM void generateHousePattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMajor = true;
  const int baseNote = 8;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);


    bool kickPlay = false;
    if (pageOffset % 3 == 0) {

      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 3 == 1) {

      kickPlay = (beat % 2 == 1);
    } else {

      kickPlay = (beat == 1 || beat == 5 || beat == 7 || beat == 9 || beat == 13 || beat == 15);
    }

    if (kickPlay) {
      int vel = 114 + ((beat == 1) ? 6 : 0) + random(-3, 4);
      placeGenNote(c, 3, 1, vel, 100, 1);
    }


    bool snarePlay = false;
    if (pageOffset % 4 == 0) {

      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 4 == 1) {

      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 60 + random(-10, 11);
      } else {
        note[c][5].velocity = 85 + random(-10, 11);
      }
    } else if (pageOffset % 4 == 2) {

      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 50 + random(-10, 11);
      } else {
        note[c][5].velocity = 80 + random(-10, 11);
      }
    } else {

      snarePlay = (beat == 2 || beat == 5 || beat == 8 || beat == 13);
    }

    if (snarePlay) {
      int vel = 106 + random(-4, 5);
      uint8_t prob = 100;
      if (pageOffset % 4 == 1 && (beat == 3 || beat == 11)) {
        vel = 56 + random(-4, 5);
        prob = 45;
      } else if (pageOffset % 4 == 2 && (beat == 4 || beat == 6 || beat == 12 || beat == 14)) {
        vel = 62 + random(-5, 6);
        prob = 60;
      }
      placeGenNote(c, 5, 2, vel, prob, 1);
    }


    if (beat % 2 == 0) {
      int vel = 70 + ((beat % 4 == 0) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 7, 3, vel, 100, 1);
    }


    bool openHatPlay = false;
    if (pageOffset % 2 == 0) {

      openHatPlay = (beat == 5 || beat == 13);
    } else {

      openHatPlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
    }

    if (openHatPlay) {
      int vel = 92 + random(-4, 5);
      uint8_t prob = (pageOffset % 3 == 2) ? 65 : 85;
      placeGenNote(c, 8, 4, vel, prob, 1);
    }


    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      int vel = 58 + random(-5, 6);
      placeGenNote(c, 6, 5, vel, 30, 1);
    }


    bool bassPlay = false;
    if (pageOffset % 3 == 0) {

      bassPlay = (beat == 1 || beat == 9);
    } else if (pageOffset % 3 == 1) {

      bassPlay = (beat % 2 == 1);
    } else {

      bassPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 9 || beat == 12 || beat == 15);
    }

    if (bassPlay) {
      int row = createRhythmicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      if (row > 9) row -= 7;
      if (row < 1) row = 1;
      int vel = 100 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 100, 1);
    }


    if (random(0, 100) < 14) {
      int row = createMelodicProgression(chordRoot, pageOffset + (beat / 2), harmony);
      int vel = 86 + random(-4, 5);
      placeGenNote(c, row, 7, vel, 60, 1);
    }


    if (random(0, 100) < 8) {
      int row = chordRoot;
      int r = random(0, 3);
      if (r == 1) row += 2;
      if (r == 2) row += 4;
      if (row > 16) row -= 7;
      int vel = 92 + random(-4, 5);
      placeGenNote(c, row, 8, vel, 50, 2);
    }
  }
}


FLASHMEM void generateAmbientPattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMajor = (random(0, 100) < 55);
  harmony.isMinor = !harmony.isMajor;
  const int baseNote = harmony.isMajor ? 8 : 6;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);


    if (pageOffset % 3 == 0) {

      if (random(0, 100) < 15) {
        int channel = random(5, 9);
        int row = createMelodicProgression(chordRoot, pageOffset + (beat / 4), harmony);
        if (row < 7) row += 7;
        int vel = 42 + random(-3, 4);
        placeGenNote(c, row, (uint8_t)channel, vel, 65, 2);
      }
    } else if (pageOffset % 3 == 1) {

      if (random(0, 100) < 25) {
        int channel = random(5, 9);
        int row = createMelodicProgression(chordRoot, pageOffset + (beat / 2), harmony);
        if (row < 6) row += 7;
        int vel = 52 + random(-4, 5);
        placeGenNote(c, row, (uint8_t)channel, vel, 75, 1);
      }
    } else {

      if (random(0, 100) < 35) {
        int channel = random(5, 9);
        int row = createMelodicProgression(chordRoot, pageOffset + beat, harmony);
        if (row < 5) row += 7;
        int vel = 48 + random(-4, 5);
        placeGenNote(c, row, (uint8_t)channel, vel, 80, 1);
      }
    }


    if (pageOffset % 2 == 0) {

      if (random(0, 100) < 8) {
        int vel = 36 + random(-3, 4);
        placeGenNote(c, 6, 3, vel, 50, 4);
      }
    } else {

      if (random(0, 100) < 15) {
        int vel = 40 + random(-3, 4);
        placeGenNote(c, 6, 3, vel, 60, 2);
      }
    }


    if (random(0, 100) < 12) {
      int row = createRhythmicProgression(chordRoot, pageOffset, harmony);
      if (row > 9) row -= 7;
      if (row < 1) row = 1;
      int vel = 54 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 55, 4);
    }


    if (random(0, 100) < 18) {
      int row = createMelodicProgression(chordRoot, pageOffset, harmony);
      if (row < 6) row += 7;
      int vel = 46 + random(-3, 4);
      placeGenNote(c, row, 7, vel, 65, 2);
    }


    if (random(0, 100) < 5) {
      int vel = 44 + random(-3, 4);
      placeGenNote(c, 5, 2, vel, 35, 8);
    }
  }
}


FLASHMEM void generateBasicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {

  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int s = randStep16((unsigned int)x_rel);

    if(s == 1 || s == 5 || s == 9 || s == 13) {
      int noteRow = random(1, 9);
      placeRandomNoteIfEmpty(c, noteRow, channel, defaultVelocity);
    }
  }
}

FLASHMEM void clearPage() {
  extern bool voiceMode;
  extern bool songModeActive;
  extern bool isNowPlaying;
  extern unsigned int beatForUI;
  extern uint8_t voiceEditPage[16];
  extern unsigned int voicePageFor(int channel, unsigned int globalPage);


  if (voiceMode && !songModeActive && !GLOB.singleMode) {
    return;
  }

  unsigned int page = GLOB.edit < 1 ? 1 : GLOB.edit;
  if (voiceMode && !songModeActive && GLOB.singleMode) {
    int ch = (int)GLOB.currentChannel;
    if (isNowPlaying && beatForUI > 0 && maxX > 0) {
      unsigned int gPage = ((beatForUI - 1) / maxX) + 1;
      page = voicePageFor(ch, gPage);
    } else if (ch >= 0 && ch < 16 && voiceEditPage[ch] >= 1) {
      page = voiceEditPage[ch];
    }
  }
  if (page < 1) page = 1;

  unsigned int start = ((page - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  bool singleMode = GLOB.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();


  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}


FLASHMEM void resetAllAudioEffects() {

  extern const int ALL_CHANNELS[];
  extern const int NUM_ALL_CHANNELS;


  for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
    int idx = ALL_CHANNELS[i];
    setFilterDefaults(idx);
  }


  forceAllMixerGainsToTarget();


  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);

  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];


    setFilters(HCUT, ch, true);
    setFilters(LOWCUT, ch, true);
    setFilters(REVERB, ch, true);
    setFilters(BITCRUSHER, ch, true);
    setFilters(DETUNE, ch, true);
    setFilters(OCTAVE, ch, true);


    setParams(DELAY, ch);
    setParams(ATTACK, ch);
    setParams(DECAY, ch);
    setParams(SUSTAIN, ch);
    setParams(RELEASE, ch);


    if (ch == 11) {
      updateSynthVoice(11);
    }
  }


  forceAllMixerGainsToTarget();

}


FLASHMEM void startNew() {

  extern unsigned int samplePackID;
  extern double playNoteInterval;
  extern IntervalTimer playTimer;
  extern bool globalMutes[maxY];
  extern bool pageMutes[maxPages][maxY];
  extern bool songModeActive;
  extern bool preventPaintUnpaint;
  extern int patternMode;
  extern Mode draw;
  extern Mode bpmMode;
  extern Mode *currentMode;
  extern i2cEncoderLibV2 Encoder[NUM_ENCODERS];
  extern CRGB col[];
  extern bool isNowPlaying;
  extern CachedSample previewCache;


  sampleBrowserClearAll();

  previewCache.valid = false;
  previewCache.lengthBytes = 0;
  previewCache.plen = 0;


  if (isNowPlaying) {
    isNowPlaying = false;
  }


  FastLEDclear();
  drawText("NEW", 6, 8, CRGB(0, 255, 255));
  FastLEDshow();


  extern void clearPatternNotes();
  clearPatternNotes();


  SMP.bpm = 100.0;
  GLOB.vol = 100;
  GLOB.velocity = 10;
  GLOB.currentChannel = 1;
  GLOB.page = 1;
  GLOB.edit = 1;
  GLOB.singleMode = false;
  GLOB.x = 1;
  GLOB.y = 1;




  uint8_t preserveLedMode = EEPROM.read(EEPROM_DATA_START + 13);
  if (preserveLedMode < 1 || preserveLedMode > 4) preserveLedMode = 1;
  EEPROM.write(EEPROM_DATA_START + 0, 1);
  EEPROM.write(EEPROM_DATA_START + 1, 1);
  EEPROM.write(EEPROM_DATA_START + 2, 2);
  EEPROM.write(EEPROM_DATA_START + 3, (uint8_t)-1);
  EEPROM.write(EEPROM_DATA_START + 4, (uint8_t)-1);
  EEPROM.write(EEPROM_DATA_START + 5, 2);
  EEPROM.write(EEPROM_DATA_START + 58, 0xA5);
  EEPROM.write(EEPROM_DATA_START + 6, 0);
  EEPROM.write(EEPROM_DATA_START + 7, 20);
  EEPROM.write(EEPROM_DATA_START + 8, 1);
  EEPROM.write(EEPROM_DATA_START + 9, 10);
  EEPROM.write(EEPROM_DATA_START + 10, (6 << 2));
  EEPROM.write(EEPROM_DATA_START + 30, 12);
  EEPROM.write(EEPROM_DATA_START + 11, 1);
  EEPROM.write(EEPROM_DATA_START + 12, 0);
  EEPROM.write(EEPROM_DATA_START + 13, preserveLedMode);
  EEPROM.write(EEPROM_DATA_START + 14, 1);
  EEPROM.write(EEPROM_DATA_START + 15, 30);
  EEPROM.write(EEPROM_DATA_START + 16, 8);
  EEPROM.write(EEPROM_DATA_START + 17, 100);
  EEPROM.write(EEPROM_DATA_START + 18, 0);
  EEPROM.write(EEPROM_DATA_START + 19, 0);
  EEPROM.write(EEPROM_DATA_START + 20, 0);
  EEPROM.write(EEPROM_DATA_START + 21, 0);
  EEPROM.write(EEPROM_DATA_START + 22, 0);
  EEPROM.write(EEPROM_DATA_START + 23, 0);
  EEPROM.write(EEPROM_DATA_START + 24, 2);
  EEPROM.write(EEPROM_DATA_START + 25, 0);
  EEPROM.write(EEPROM_DATA_START + 26, 64);
  EEPROM.write(EEPROM_DATA_START + 27, 1);
  EEPROM.write(EEPROM_DATA_START + 55, 0xA5);
  EEPROM.write(EEPROM_DATA_START + 56, 1);
  EEPROM.write(EEPROM_DATA_START + 57, 0);
  EEPROM.write(EEPROM_DATA_START + 28, 1);
  EEPROM.write(EEPROM_DATA_START + 29, 0);
  EEPROM.write(EEPROM_DATA_START + 31, 0);
  EEPROM.write(EEPROM_DATA_START + 32, 0);
  EEPROM.write(EEPROM_DATA_START + 33, 5);
  EEPROM.put(EEPROM_DATA_START + 34, (uint16_t)0x0006);
  EEPROM.write(EEPROM_DATA_START + 36, 0);
  EEPROM.write(EEPROM_DATA_START + 37, 1);
  EEPROM.write(EEPROM_DATA_START + 38, 10);
  EEPROM.write(EEPROM_DATA_START + 39, 10);
  EEPROM.write(EEPROM_DATA_START + 40, 10);
  EEPROM.write(EEPROM_DATA_START + 41, 10);
  EEPROM.write(EEPROM_DATA_START + 42, 0);
  EEPROM.write(EEPROM_DATA_START + 43, 0);
  EEPROM.write(EEPROM_DATA_START + 44, 8);
  EEPROM.write(EEPROM_DATA_START + 45, 1);
  EEPROM.write(EEPROM_DATA_START + 46, 0);
  EEPROM.write(EEPROM_DATA_START + 47, 8);
  EEPROM.write(EEPROM_DATA_START + 48, 0);
  EEPROM.write(EEPROM_DATA_START + 49, 1);
  EEPROM.write(EEPROM_DATA_START + 51, 1);
  EEPROM.write(EEPROM_DATA_START + 52, 0);
  EEPROM.write(EEPROM_DATA_START + 53, 2);
  EEPROM.write(EEPROM_DATA_START + 54, 0);


  extern void loadMenuFromEEPROM();
  extern void applyAudioSettingsFromGlobals();
  loadMenuFromEEPROM();
  applyAudioSettingsFromGlobals();


  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);

  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];


    SMP.filter_settings[ch][HCUT] = (ch == 14) ? 14 : 32;
    SMP.filter_settings[ch][LOWCUT] = 0;
    SMP.filter_settings[ch][REVERB] = 0;
    SMP.filter_settings[ch][BITCRUSHER] = 0;
    SMP.filter_settings[ch][RES] = (ch == 14) ? 5 : 0;
    SMP.filter_settings[ch][DETUNE] = 16;
    SMP.filter_settings[ch][OCTAVE] = (ch >= 1 && ch <= 8) ? 24 : 16;
    SMP.filter_settings[ch][EFX] = 0;



    SMP.param_settings[ch][DELAY] = 0;
    SMP.param_settings[ch][ATTACK] = 32;
    SMP.param_settings[ch][DECAY] = (ch == 14) ? 20 : 32;
    SMP.param_settings[ch][SUSTAIN] = (ch == 14) ? 20 : 32;
    SMP.param_settings[ch][RELEASE] = (ch == 11 || ch == 13 || ch == 14) ? 16 : 0;

    if (ch >= 1 && ch <= 8) {
      setSampleTimeStretch(ch, 11);
    }


    if (ch == 11) {
      extern void applySynthInstrumentPreset(int channel, int instrumentIdx);
      applySynthInstrumentPreset(11, 12);
      SMP.synth_settings[11][LFO_RATE] = 0;
      SMP.synth_settings[11][LFO_DEPTH] = 0;
      SMP.synth_settings[11][LFO_PHASE] = 0;
      SMP.synth_settings[11][ARP_STEP] = 16;
    } else if (ch == 13 || ch == 14) {
      SMP.synth_settings[ch][CUTOFF] = (ch == 13) ? 32 : 14;
      SMP.synth_settings[ch][RESONANCE] = (ch == 14) ? 5 : 0;
      SMP.synth_settings[ch][FILTER] = (ch == 14) ? 8 : 0;
      SMP.synth_settings[ch][CENT] = (ch == 13) ? 0 : 8;
      SMP.synth_settings[ch][SEMI] = 16;
      SMP.synth_settings[ch][INSTRUMENT] = 0;
      SMP.synth_settings[ch][FORM] = 0;
      SMP.synth_settings[ch][LFO_RATE] = 0;
      SMP.synth_settings[ch][LFO_DEPTH] = 0;
      SMP.synth_settings[ch][LFO_PHASE] = 0;
      SMP.synth_settings[ch][ARP_STEP] = 16;
    }
  }


  for (int ch = 0; ch < maxY; ch++) {
    globalMutes[ch] = false;
    SMP.globalMutes[ch] = false;
    for (int page = 0; page < maxPages; page++) {
      pageMutes[page][ch] = false;
      SMP.pageMutes[page][ch] = false;
    }
    SMP.mute[ch] = 0;
  }
  unmuteAllChannels();


  for (int i = 0; i < 64; i++) {
    SMP.songArrangement[i] = 0;
  }


  for (int i = 1; i < maxFiles; i++) {
    SMP.sp0Active[i] = false;
  }
  saveSp0StateToEEPROM();


  SMP.pack = 1;
  samplePackID = 1;
  EEPROM.put(0, (unsigned int)1);
  markSettingsBackupDirty();

  loadSamplePack(1, false, false);


  bpmMode.pos[3] = SMP.bpm;
  playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
  playTimer.update((uint32_t)round(playNoteInterval));
  bpmMode.pos[2] = (clockMode == 1) ? 1 : 0;


  for (int i = 1; i < maxFiles; i++) {
    SMP.wav[i].oldID = 0;
    SMP.wav[i].fileID = i;
  }


  updateLastPage();


  songModeActive = false;
  SMP_PATTERN_MODE = false;
  patternMode = -1;


  preventPaintUnpaint = false;


  delay(300);
  switchMode(&draw);
  applyDrawSingleYEncoderLimits();



  Encoder[0].writeMax((int32_t)maxY);
  Encoder[0].writeMin((int32_t)1);
  Encoder[0].writeCounter((int32_t)GLOB.y);
  Encoder[1].writeCounter((int32_t)GLOB.edit);
  Encoder[2].writeCounter((int32_t)maxfilterResolution);
  Encoder[3].writeCounter((int32_t)GLOB.x);


  currentMode->pos[0] = GLOB.y;
  currentMode->pos[1] = GLOB.edit;
  currentMode->pos[2] = maxfilterResolution;
  currentMode->pos[3] = GLOB.x;


  Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
  Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));


  resetAllAudioEffects();

  extern void resetAllChannelVolumesToDefault();
  resetAllChannelVolumesToDefault();



  extern void setAllFilterPagesDefaultValues(int ch);
  extern uint8_t filterPage[NUM_CHANNELS];
  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    filterPage[ch] = 0;
    setAllFilterPagesDefaultValues(ch);
  }



  FastLEDclear();
  FastLEDshow();



  extern void deleteAutosaveFile();
  extern void savePattern(bool autosave);
  deleteAutosaveFile();
  savePattern(true);

}

int getFolderNumber(int value) {
  int folder = floor(value / 100);
  if (folder > FOLDER_MAX) folder = FOLDER_MAX;
  if (folder <= 0) folder = 0;
  return folder;
}







FLASHMEM static bool logoWaveTouches(unsigned int x, int wavePixelY, int logoStartX) {
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      const int logoX = (int)x + dx - logoStartX;
      const int logoY = wavePixelY + dy - 1;
      if (logoX >= 0 && logoX < (int)MATRIX_WIDTH &&
          logoY >= 0 && logoY < (int)maxY &&
          logo16_on_P(logo_rows, (uint8_t)logoX, (uint8_t)logoY)) {
        return true;
      }
    }
  }
  return false;
}

FLASHMEM static void drawLogoWavePixel(unsigned int x, float waveY, CRGB baseColor,
                                       int logoStartX, float logoFade,
                                       float sineFade, float fadeOut) {

  const int firstY = (int)floorf(waveY);
  const float fraction = waveY - firstY;
  for (int i = 0; i < 2; i++) {
    const int pixelY = firstY + i;
    if (pixelY < 1 || pixelY > (int)maxY) continue;
    const float coverage = i ? fraction : 1.0f - fraction;
    const float contourScale =
        logoWaveTouches(x, pixelY, logoStartX) ? (1.0f - 0.90f * logoFade) : 1.0f;
    const float level = sineFade * fadeOut * contourScale * coverage;
    if (level <= 0.0f) continue;
    CRGB color = baseColor;
    color.nscale8((uint8_t)constrain((int)roundf(255.0f * level), 0, 255));
    if (color.r | color.g | color.b) light(x, (unsigned int)pixelY, color);
  }
}



FLASHMEM static void drawLogoAnimationFrame(unsigned long elapsedMs, bool holdNoFadeOut) {
  extern void light(unsigned int x, unsigned int y, CRGB color);

  const unsigned long fadeOutStart = phase1Duration + phase2Duration;
  const float sineFade = constrain(
      (float)elapsedMs / (float)phase1Duration, 0.0f, 1.0f);
  const float logoFade = constrain(
      ((float)elapsedMs - (float)phase1Duration) /
          (float)phase2Duration,
      0.0f, 1.0f);
  const float fadeOut = holdNoFadeOut
      ? 1.0f
      : ((elapsedMs <= fadeOutStart)
             ? 1.0f
             : constrain(
                   1.0f - ((float)(elapsedMs - fadeOutStart) /
                               (float)phase3Duration),
                   0.0f, 1.0f));


  const float timeSec = (float)elapsedMs * 0.001f;
  const float hueProgress = fmodf(timeSec * 0.22f, 1.0f);
  const float amplitude =
      3.1f + 0.45f * sinf(timeSec * 1.4f);
  const float centerY = ((float)maxY + 1.0f) * 0.5f;
  const float whiteYOffset = -0.85f;
  const float blueYOffset = 0.85f;
  const float blueXPhaseOffset = 0.55f;
  const float widthDenominator = (maxX > 1) ? (float)(maxX - 1) : 1.0f;
  const int logoStartX =
      ((int)maxX - (int)MATRIX_WIDTH) / 2 + 1;


  const float whiteTravel = timeSec * 5.4f;
  const float blueTravel = timeSec * 2.6f;

  for (unsigned int x = 1; x <= maxX; x++) {
    const float xNorm = (float)(x - 1) / widthDenominator;
    const float xPhase = xNorm * 4.0f * (float)M_PI;


    drawLogoWavePixel(
        x,
        centerY + blueYOffset +
            amplitude * sinf(xPhase + blueXPhaseOffset - blueTravel),
        CRGB(3, 8, 28),
        logoStartX, logoFade, sineFade, fadeOut);
    drawLogoWavePixel(
        x,
        centerY + whiteYOffset +
            amplitude * sinf(xPhase - whiteTravel),
        CRGB(26, 26, 26),
        logoStartX, logoFade, sineFade, fadeOut);
  }

  const uint8_t logoBrightness = (uint8_t)constrain(
      (int)roundf(255.0f * logoFade * fadeOut), 0, 255);
  for (uint8_t y = 0; y < maxY; y++) {
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      if (logo16_on_P(logo_rows, x, y)) {
        CRGB color = getLogoPixelColor(x, y, hueProgress);
        color.nscale8(logoBrightness);
        if (logoBrightness > 0) {
          light((unsigned int)(logoStartX + x), y + 1, color);
        }
      }
    }
  }
}


FLASHMEM void runAnimation() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLEDshow();

  unsigned long startTime = millis();
  unsigned long lastFrameTime = millis();

  while (true) {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - startTime;

    if (currentTime - lastFrameTime < (unsigned long)RefreshTime) {
      yield();
      continue;
    }
    lastFrameTime = currentTime;

    if (elapsed > totalAnimationTime) {
      elapsed = totalAnimationTime;
    }

    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    drawLogoAnimationFrame(elapsed, false);
    FastLEDshow();

    if (elapsed >= totalAnimationTime) {
      break;
    }
  }

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLEDshow();
}



FLASHMEM void runInfoLogoAnimationLoop() {
  extern bool pressed[NUM_ENCODERS];
  extern bool isPressed[NUM_ENCODERS];
  extern uint8_t buttons[NUM_ENCODERS];
  extern ButtonState buttonState[NUM_ENCODERS];
  extern int currentEncoderIndex;
  extern void resetEtcInfoPageAnimation();



  buttons[3] = 0;
  pressed[3] = false;
  isPressed[3] = false;
  buttonState[3] = IDLE;

  unsigned long startTime = millis();
  unsigned long lastFrameTime = 0;
  bool sawRelease = true;

  for (;;) {
    currentEncoderIndex = 3;
    Encoder[3].updateStatus();
    const bool down = pressed[3] || isPressed[3];
    if (!down) {
      sawRelease = true;
    } else if (sawRelease) {


      break;
    }

    unsigned long currentTime = millis();
    if (currentTime - lastFrameTime < (unsigned long)RefreshTime) {
      yield();
      continue;
    }
    lastFrameTime = currentTime;

    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    drawLogoAnimationFrame(currentTime - startTime, true);
    FastLEDshow();
  }


  buttons[3] = 0;
  pressed[3] = false;
  isPressed[3] = false;
  buttonState[3] = IDLE;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLEDshow();
  resetEtcInfoPageAnimation();
  GLOB.singleMode = false;
  switchMode(&draw);
}


FLASHMEM static void companionStepPitch(const CompanionHarmony &harm, uint8_t channel,
                               int degDelta, int &ioRow, uint8_t &ioMidiPitch) {
  int8_t scale[7];
  companionFillScalePcs(harm, scale);

  int curPc = (ioMidiPitch != NOTE_MIDI_PITCH_NONE && ioMidiPitch <= 127)
                  ? (int)(ioMidiPitch % 12u)
                  : companionPitchClassForRow(channel, ioRow);

  int bestDeg = 0;
  int bestDist = 99;
  for (int d = 0; d < 7; d++) {
    int diff = abs(((scale[d] - curPc + 18) % 12) - 6);
    if (diff < bestDist) {
      bestDist = diff;
      bestDeg = d;
    }
  }

  int targetDeg = (bestDeg + degDelta) % 7;
  if (targetDeg < 0) targetDeg += 7;
  int octaveShift = (bestDeg + degDelta >= 7) ? 1 : ((bestDeg + degDelta < 0) ? -1 : 0);
  int targetPc = scale[targetDeg];

  if (ioMidiPitch != NOTE_MIDI_PITCH_NONE && ioMidiPitch <= 127) {
    int semitoneDelta = ((targetPc - curPc + 18) % 12) - 6 + octaveShift * 12;
    int nextMidi = constrain((int)ioMidiPitch + semitoneDelta, 24, 108);
    ioMidiPitch = (uint8_t)nextMidi;
  }

  int lo = (companionRoleForChannel(channel) == CompanionRole::Bass) ? 1 : 2;
  int hi = (companionRoleForChannel(channel) == CompanionRole::Bass) ? 9 : 16;
  int prefer = constrain(ioRow + degDelta, lo, hi);
  ioRow = companionPcToChannelRow(targetPc, channel, lo, hi, prefer);
}


FLASHMEM static bool companionPlaceCellLimited(unsigned int step, int preferredRow,
                                      uint8_t channel, int velocity,
                                      uint8_t probability, uint8_t condition,
                                      uint8_t midiPitch) {
  int used = 0;
  for (int row = 1; row <= 16; row++) {
    if (note[step][row].channel == channel) used++;
  }
  if (used >= companionChannelStepLimit(channel)) return false;

  int freeRow = companionFreeRow(step, preferredRow);
  if (freeRow == 0) return false;
  placeGenNote(step, freeRow, channel, velocity, probability, condition);


  note[step][freeRow].midiPitch = midiPitch;
  return true;
}






FLASHMEM static void evolveCompanionChannelFromBase(int srcPage, int dstPage, uint8_t channel,
                                           int arcRole, bool isFinalPage,
                                           const CompanionContext &ctx,
                                           const CompanionHarmony &harm,
                                           int variation) {
  CompanionRole role = companionRoleForChannel(channel);
  if (role == CompanionRole::Unknown) return;

  unsigned int srcStart = (unsigned int)(srcPage - 1) * maxX + 1;
  unsigned int dstStart = (unsigned int)(dstPage - 1) * maxX + 1;
  unsigned int steps = min((unsigned int)maxX, (unsigned int)(MAX_STEPS + 1u - dstStart));


  struct SrcEvent {
    uint8_t stepOffset;
    uint8_t row;
    uint8_t velocity;
    uint8_t probability;
    uint8_t condition;
    uint8_t midiPitch;
  };
  SrcEvent events[48];
  int eventCount = 0;
  uint32_t occupiedMask = 0;
  uint8_t homeRow = ctx.channelHomeRow[channel];
  uint8_t sampleMidiPitch = NOTE_MIDI_PITCH_NONE;

  for (unsigned int s = 0; s < steps; s++) {
    for (int r = 1; r <= 16; r++) {
      const Note &cell = note[srcStart + s][r];
      if (cell.channel != channel) continue;
      occupiedMask |= (1UL << (s & 31));
      homeRow = (uint8_t)r;
      if (cell.midiPitch != NOTE_MIDI_PITCH_NONE) sampleMidiPitch = cell.midiPitch;
      if (eventCount < 48) {
        events[eventCount++] = {(uint8_t)s, (uint8_t)r, cell.velocity,
                                cell.probability, cell.condition, cell.midiPitch};
      }
    }
  }

  if (eventCount == 0) return;

  uint32_t varHash = companionMixHash((uint32_t)variation * 2246822519UL +
                                      (uint32_t)dstPage * 3266489917UL +
                                      (uint32_t)channel * 668265263UL);


  for (int i = 0; i < eventCount; i++) {
    SrcEvent ev = events[i];
    int phase = randStep16(ev.stepOffset + 1) - 1;
    bool secondHalf = (phase >= 8);
    bool inTurnaround = (phase >= 12);
    bool quarter = (phase & 3) == 0;
    uint32_t h = companionMixHash(varHash + (uint32_t)i * 374761393UL);

    int dstOffset = (int)ev.stepOffset;
    int dstRow = (int)ev.row;
    int dstVel = (int)ev.velocity;
    uint8_t dstProb = ev.probability;
    uint8_t dstCond = ev.condition;
    uint8_t dstMidi = ev.midiPitch;

    if (role == CompanionRole::Kick || role == CompanionRole::Snare ||
        role == CompanionRole::ClosedHat || role == CompanionRole::Clap ||
        role == CompanionRole::Tom) {

      if (arcRole == 3 && inTurnaround && role == CompanionRole::ClosedHat && phase >= 14) {

        dstVel = max(32, dstVel - 18);
      } else if (arcRole == 1 && secondHalf && !quarter && phase != 12 && (h % 100) < 22) {

        int candidate = constrain(dstOffset + ((h & 1) ? 1 : -1), 0, (int)steps - 1);
        if ((occupiedMask & (1UL << (candidate & 31))) == 0 && (candidate & 3) != 0) {
          dstOffset = candidate;
        }
      }

      dstVel = constrain(dstVel + (int)(h % 9) - 4, 24, 127);
    } else if (role == CompanionRole::Bass) {

      if (arcRole == 1 && secondHalf && !quarter) {

        int delta = ((h & 3) == 0) ? 2 : (((h & 1) != 0) ? 4 : -1);
        companionStepPitch(harm, channel, delta, dstRow, dstMidi);
      } else if (arcRole == 2 && ((phase == 6 || phase == 10 || phase == 14) || (!quarter && (h % 100) < 45))) {

        companionStepPitch(harm, channel, 4, dstRow, dstMidi);
        dstVel = constrain(dstVel + 6, 36, 127);
      } else if ((arcRole == 3 || isFinalPage) && inTurnaround && phase > 12) {

        int walkDeg = (phase == 13) ? 2 : ((phase == 14) ? 4 : 6);
        companionStepPitch(harm, channel, walkDeg, dstRow, dstMidi);
      }
    } else if (role == CompanionRole::Keys || role == CompanionRole::VocalsPad) {

      if (arcRole == 0 && inTurnaround && !quarter && (h % 100) < 40) {
        companionStepPitch(harm, channel, (h & 1) ? 1 : -1, dstRow, dstMidi);
      } else if (arcRole == 1 && secondHalf) {

        int delta = inTurnaround ? ((phase >= 14) ? 0 : -1) : ((h & 1) ? 2 : -2);
        companionStepPitch(harm, channel, delta, dstRow, dstMidi);
      } else if (arcRole == 2) {

        int delta = quarter ? 2 : ((h & 1) ? 2 : 1);
        companionStepPitch(harm, channel, delta, dstRow, dstMidi);
        dstVel = constrain(dstVel + 5, 32, 124);
      } else if (arcRole == 3 || isFinalPage) {
        if (inTurnaround) {

          int delta = (phase >= 14) ? 0 : 4;
          companionStepPitch(harm, channel, delta, dstRow, dstMidi);
        }
      }
    }

    companionPlaceCellLimited(dstStart + (unsigned int)dstOffset, dstRow, channel,
                              dstVel, dstProb, dstCond, dstMidi);
  }


  for (unsigned int s = 0; s < steps; s++) {
    int phase = randStep16(s + 1) - 1;
    int bar = phase / 4;
    uint32_t stepH = companionMixHash(varHash + s * 198491317UL);
    bool alreadyHasHit = false;
    for (int r = 1; r <= 16; r++) {
      if (note[dstStart + s][r].channel == channel) {
        alreadyHasHit = true;
        break;
      }
    }
    if (alreadyHasHit) continue;

    int meanVel = ctx.channelMeanVel[channel];
    if (role == CompanionRole::Kick) {

      bool pickup = ((arcRole == 1 || arcRole == 3) && (phase == 14 || phase == 15) &&
                     ctx.snareWeight[phase] == 0 && (stepH % 100) < 42);
      if (pickup) {
        companionPlaceCellLimited(dstStart + s, homeRow, channel,
                                  constrain(meanVel - 16, 55, 108), 100, 1, sampleMidiPitch);
      }
    } else if (role == CompanionRole::Snare || role == CompanionRole::Clap) {

      bool turnaroundRoll = (arcRole == 3 || isFinalPage) &&
                            (phase == 13 || phase == 14 || phase == 15) &&
                            ((stepH % 100) < (uint32_t)(phase == 15 ? 78 : 52));
      bool ghostAdd = (arcRole == 1 || arcRole == 2) && (phase == 7 || phase == 11 || phase == 15) &&
                      ctx.kickWeight[phase] == 0 && ((stepH % 100) < 28);
      if (turnaroundRoll || ghostAdd) {
        int vel = turnaroundRoll
                      ? constrain(meanVel - 22 + (phase - 13) * 10, 48, 112)
                      : constrain(meanVel - 32, 38, 82);
        companionPlaceCellLimited(dstStart + s, homeRow, channel, vel, 100, 1, sampleMidiPitch);
      }
    } else if (role == CompanionRole::ClosedHat) {

      bool add16th = ((arcRole == 2 && (phase & 1) == 1 && (stepH % 100) < 35) ||
                      (arcRole == 3 && phase >= 12 && (stepH % 100) < 55));
      if (add16th) {
        companionPlaceCellLimited(dstStart + s, homeRow, channel,
                                  constrain(meanVel - 22, 36, 78), 100, 1, sampleMidiPitch);
      }
    } else if (role == CompanionRole::Tom) {

      if ((arcRole == 3 || isFinalPage) && phase >= 13 && ctx.snareWeight[phase] == 0 &&
          (stepH % 100) < 62) {
        int fillRow = constrain((int)homeRow + (14 - phase), 1, 16);
        companionPlaceCellLimited(dstStart + s, fillRow, channel,
                                  constrain(meanVel - 8 + (phase - 13) * 6, 52, 114),
                                  100, 1, sampleMidiPitch);
      }
    } else if (role == CompanionRole::Bass) {

      if ((arcRole >= 1) && (phase == 14 || phase == 15) && ctx.snareWeight[phase] == 0 &&
          (stepH % 100) < 48) {
        int deg = (phase == 14) ? 4 : 6;
        int pc = companionDegreeToPc(harm, bar, CompanionRole::Bass, deg, false);
        int row = companionPcToChannelRow(pc, channel, 1, 8, homeRow);
        uint8_t mp = sampleMidiPitch;
        if (mp != NOTE_MIDI_PITCH_NONE) {
          int rTmp = homeRow;
          companionStepPitch(harm, channel, (phase == 14) ? 4 : -1, rTmp, mp);
        }
        companionPlaceCellLimited(dstStart + s, row, channel,
                                  constrain(meanVel - 10, 58, 110), 100, 1, mp);
      }
    } else if (role == CompanionRole::Keys) {

      if ((arcRole == 1 || arcRole == 2) && (phase == 6 || phase == 14) &&
          eventCount <= 6 && (stepH % 100) < 45) {
        int deg = (phase == 14) ? 2 : 4;
        int pc = companionDegreeToPc(harm, bar, CompanionRole::Keys, deg, false);
        int row = companionPcToChannelRow(pc, channel, 3, 16, homeRow);
        uint8_t mp = sampleMidiPitch;
        if (mp != NOTE_MIDI_PITCH_NONE) {
          int rTmp = homeRow;
          companionStepPitch(harm, channel, 2, rTmp, mp);
        }
        companionPlaceCellLimited(dstStart + s, row, channel,
                                  constrain(meanVel - 12, 42, 102), 100, 1, mp);
      }
    }
  }
}





FLASHMEM void generateSong() {
  extern int aiTargetPage;
  extern int aiBaseStartPage;
  extern int aiBaseEndPage;

  int pageLimit = companionEffectivePages();
  int baseStart = constrain(aiBaseStartPage, 1, pageLimit);
  int baseEnd = constrain(aiBaseEndPage, baseStart, pageLimit);
  int outputStart = baseEnd + 1;
  int outputEnd = min(pageLimit, outputStart + max(1, aiTargetPage) - 1);
  if (outputStart > pageLimit) {
    extern void switchMode(Mode *newMode);
    extern Mode draw;
    switchMode(&draw);
    return;
  }

  static const uint8_t generationOrder[] = {1, 2, 3, 4, 5, 6, 11, 13, 14, 7, 8};
  bool channelsUsed[16] = {};
  bool channelOnPage[17][16] = {};
  bool baseEmpty = true;

  for (int page = baseStart; page <= baseEnd; page++) {
    unsigned int start = (unsigned int)(page - 1) * maxX + 1;
    unsigned int end = min((unsigned int)MAX_STEPS + 1u, start + maxX);
    for (unsigned int c = start; c < end; c++) {
      for (int row = 1; row <= 16; row++) {
        uint8_t ch = note[c][row].channel;
        if (ch < 16 && companionChannelSupported(ch)) {
          channelsUsed[ch] = true;
          channelOnPage[page][ch] = true;
          baseEmpty = false;
        }
      }
    }
  }

  preventPaintUnpaint = true;



  if (baseEmpty) {
    int starterSeed = random(1, 255);
    clearCompanionPage(baseStart);
    for (uint8_t ch : generationOrder) {
      channelsUsed[ch] = true;
      channelOnPage[baseStart][ch] = true;
      CompanionContext ctx = analyzeCompanionContext(baseStart, baseStart, baseStart, ch);
      CompanionHarmony harm = companionMakeHarmony(ctx, starterSeed);
      CompanionGroove groove = companionMakeGroove(ctx, baseStart, starterSeed);
      generateCompanionChannelPage(baseStart, ch, ctx, harm, groove, starterSeed);
    }
  }

  int baseLen = max(1, baseEnd - baseStart + 1);
  int sessionSeed = random(1, 255);
  extern bool SMP_PATTERN_MODE;
  extern void copyPageMutes(int fromPage, int toPage);

  for (int page = outputStart; page <= outputEnd; page++) {
    clearCompanionPage(page);
    int relIdx = page - outputStart;
    int srcPage = baseStart + (relIdx % baseLen);
    bool isFinalPage = (page == outputEnd);
    int arcRole = isFinalPage && (outputEnd > outputStart) ? 3 : (relIdx % 4);
    int pageSeed = sessionSeed + page * 37 + relIdx * 19;



    if (SMP_PATTERN_MODE) {
      copyPageMutes(srcPage, page);
    }


    CompanionContext pageContext = analyzeCompanionContext(baseStart, baseEnd, srcPage, 0);
    CompanionHarmony pageHarm = companionMakeHarmony(pageContext, pageSeed);
    CompanionGroove pageGroove = companionMakeGroove(pageContext, page, pageSeed);

    for (uint8_t ch : generationOrder) {
      if (!channelsUsed[ch]) continue;
      CompanionContext chCtx = analyzeCompanionContext(baseStart, page, srcPage, ch);

      if (channelOnPage[srcPage][ch]) {
        evolveCompanionChannelFromBase(srcPage, page, ch, arcRole, isFinalPage,
                                       chCtx, pageHarm, pageSeed);
      } else {


        int fallbackSrc = 0;
        for (int bp = baseStart; bp <= baseEnd; bp++) {
          if (channelOnPage[bp][ch]) {
            fallbackSrc = bp;
            break;
          }
        }
        if (fallbackSrc > 0 && (arcRole >= 1)) {
          evolveCompanionChannelFromBase(fallbackSrc, page, ch, arcRole, isFinalPage,
                                         chCtx, pageHarm, pageSeed);
        } else if (fallbackSrc == 0) {
          generateCompanionChannelPage(page, ch, chCtx, pageHarm, pageGroove, pageSeed);
        }
      }
    }
  }

  preventPaintUnpaint = false;
  updateLastPage();

  extern void switchMode(Mode *newMode);
  extern Mode draw;
  switchMode(&draw);
}
# 1 "/Users/jank/git/toern/toern_leds.ino"






#include <FastLED.h>


#define LED_STRIP_PIN 24
#define LED_STRIP_DEFAULT_LENGTH 256
#define LED_STRIP_MAX_LENGTH 256
#define MAX_RIPPLES 16
#define RIPPLE_DECAY_RATE 0.92f
#define RIPPLE_SPEED 1.5f
#define AUDIO_SENSITIVITY 0.3f
#define AUDIO_MIN_THRESHOLD 0.01f


DMAMEM CRGB stripLeds[LED_STRIP_MAX_LENGTH];


static int ledStripLength = LED_STRIP_DEFAULT_LENGTH;
static bool ledStripEnabled = true;


struct Ripple {
  bool active;
  float position;
  float intensity;
  uint8_t channels[4];
  uint8_t channelCount;
  float speed;
  unsigned long startTime;
};

static Ripple ripples[MAX_RIPPLES];
static float audioLevel = 0.0f;
static float smoothedAudioLevel = 0.0f;


extern bool isNowPlaying;
extern CRGB col[];


extern AudioAnalyzePeak peakOutput;



void initLedStrip() {

  if (ledStripLength > LED_STRIP_MAX_LENGTH) {
    ledStripLength = LED_STRIP_MAX_LENGTH;
  }


  for (int i = 0; i < MAX_RIPPLES; i++) {
    ripples[i].active = false;
    ripples[i].intensity = 0.0f;
    ripples[i].channelCount = 0;
  }


  for (int i = 0; i < ledStripLength; i++) {
    stripLeds[i] = CRGB::Black;
  }

  audioLevel = 0.0f;
  smoothedAudioLevel = 0.0f;
}


void setLedStripEnabled(bool enabled) {
  ledStripEnabled = enabled;
  if (!enabled) {

    for (int i = 0; i < ledStripLength; i++) {
      stripLeds[i] = CRGB::Black;
    }
  }
}


bool getLedStripEnabled() {
  return ledStripEnabled;
}



#define RIPPLE_MERGE_WINDOW_MS 50

void triggerLedStripRipple(uint8_t channel) {
  if (!ledStripEnabled || !isNowPlaying) return;
  if (channel == 0 || channel > 16) return;


  unsigned long now = millis();
  int mergeSlot = -1;
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (ripples[i].active &&
        (now - ripples[i].startTime) < RIPPLE_MERGE_WINDOW_MS &&
        ripples[i].position < 5.0f &&
        ripples[i].channelCount < 4) {

      bool alreadyIncluded = false;
      for (int j = 0; j < ripples[i].channelCount; j++) {
        if (ripples[i].channels[j] == channel) {
          alreadyIncluded = true;
          break;
        }
      }
      if (!alreadyIncluded) {
        mergeSlot = i;
        break;
      }
    }
  }

  if (mergeSlot >= 0) {

    ripples[mergeSlot].channels[ripples[mergeSlot].channelCount] = channel;
    ripples[mergeSlot].channelCount++;

    ripples[mergeSlot].intensity = min(1.0f, ripples[mergeSlot].intensity + 0.1f);
    return;
  }


  int slot = -1;
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (!ripples[i].active) {
      slot = i;
      break;
    }
  }


  if (slot == -1) {
    unsigned long oldestTime = ripples[0].startTime;
    slot = 0;
    for (int i = 1; i < MAX_RIPPLES; i++) {
      if (ripples[i].startTime < oldestTime) {
        oldestTime = ripples[i].startTime;
        slot = i;
      }
    }
  }


  ripples[slot].active = true;
  ripples[slot].position = 0.0f;
  ripples[slot].intensity = 1.0f;
  ripples[slot].channels[0] = channel;
  ripples[slot].channelCount = 1;
  ripples[slot].speed = RIPPLE_SPEED;
  ripples[slot].startTime = millis();
}



void updateLedStrip() {

  static bool lastLedStripEnabled = ledStripEnabled;
  static bool lastIsNowPlaying = isNowPlaying;


  bool stateChanged = (ledStripEnabled != lastLedStripEnabled) || (isNowPlaying != lastIsNowPlaying);
  if (stateChanged) {

    if (!ledStripEnabled || !isNowPlaying) {
      for (int i = 0; i < ledStripLength; i++) {
        stripLeds[i] = CRGB::Black;
      }
    }
    lastLedStripEnabled = ledStripEnabled;
    lastIsNowPlaying = isNowPlaying;
  }


  if (!ledStripEnabled || !isNowPlaying) {

    return;
  }


  static unsigned long lastUpdateTime = 0;
  const unsigned long UPDATE_INTERVAL_MS = 33;
  unsigned long now = millis();
  if (now - lastUpdateTime < UPDATE_INTERVAL_MS) {
    return;
  }
  lastUpdateTime = now;


  bool hasActiveRipples = false;
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (ripples[i].active) {
      hasActiveRipples = true;
      break;
    }
  }


  static unsigned long lastAudioRead = 0;
  if (millis() - lastAudioRead > 10) {
    lastAudioRead = millis();
    if (peakOutput.available()) {
      audioLevel = peakOutput.read();

      smoothedAudioLevel = smoothedAudioLevel * 0.7f + audioLevel * 0.3f;
    } else {

      smoothedAudioLevel *= 0.95f;
    }
  }


  if (!hasActiveRipples && smoothedAudioLevel < AUDIO_MIN_THRESHOLD) {

    for (int i = 0; i < ledStripLength; i++) {
      stripLeds[i] = CRGB::Black;
    }
    return;
  }


  for (int i = 0; i < ledStripLength; i++) {
    stripLeds[i] = CRGB::Black;
  }


  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (!ripples[i].active) continue;


    ripples[i].position += ripples[i].speed;


    ripples[i].intensity *= RIPPLE_DECAY_RATE;


    if (ripples[i].position >= ledStripLength || ripples[i].intensity < 0.01f) {
      ripples[i].active = false;
      ripples[i].channelCount = 0;
      continue;
    }


    CRGB baseColor = CRGB::Black;
    uint8_t activeChannelCount = 0;

    for (int c = 0; c < ripples[i].channelCount; c++) {
      uint8_t ch = ripples[i].channels[c];
      if (ch > 0 && ch <= 16) {
        CRGB channelColor = col[ch];


        if (channelColor.r == 0 && channelColor.g == 0 && channelColor.b == 0) {
          if (ch == 9 || ch == 10 || ch == 12) {

            channelColor = CRGB(30, 30, 30);
          } else {

            uint8_t hue = (ch * 16) % 255;
            channelColor = CHSV(hue, 255, 255);
          }
        }


        baseColor.r = min(255, baseColor.r + channelColor.r);
        baseColor.g = min(255, baseColor.g + channelColor.g);
        baseColor.b = min(255, baseColor.b + channelColor.b);
        activeChannelCount++;
      }
    }


    if (activeChannelCount == 0) {
      baseColor = CRGB::White;
    }
# 290 "/Users/jank/git/toern/toern_leds.ino"
    float audioBoost = 1.0f;
    if (smoothedAudioLevel > AUDIO_MIN_THRESHOLD) {
      audioBoost = 1.0f + (smoothedAudioLevel * AUDIO_SENSITIVITY);
      audioBoost = constrain(audioBoost, 1.0f, 2.0f);
    }


    float effectiveIntensity = ripples[i].intensity * audioBoost;
    effectiveIntensity = constrain(effectiveIntensity, 0.0f, 1.0f);


    float width = ripples[i].intensity * 8.0f;

    for (int j = 0; j < ledStripLength; j++) {
      float dist = abs((float)j - ripples[i].position);

      if (dist < width) {

        float brightness = 1.0f - (dist / width);
        brightness = brightness * brightness;
        brightness *= effectiveIntensity;


        if (j < ledStripLength) {
          CRGB pixelColor = baseColor;
          pixelColor.nscale8((uint8_t)(brightness * 255.0f));


          stripLeds[j] += pixelColor;
        }
      }
    }
  }


  if (smoothedAudioLevel > AUDIO_MIN_THRESHOLD) {
    float glowIntensity = smoothedAudioLevel * 0.1f;
    for (int i = 0; i < ledStripLength; i++) {

      CRGB glow = CRGB((uint8_t)(glowIntensity * 50.0f),
                       (uint8_t)(glowIntensity * 50.0f),
                       (uint8_t)(glowIntensity * 50.0f));
      stripLeds[i] += glow;
    }
  }



}



void onNoteTriggered(uint8_t channel) {
  if (ledStripEnabled && isNowPlaying && channel > 0 && channel <= 16) {
    triggerLedStripRipple(channel);
  }
}


FLASHMEM void clearLedStripForScreensaver() {
  for (int i = 0; i < ledStripLength; i++) {
    stripLeds[i] = CRGB::Black;
  }
}
# 1 "/Users/jank/git/toern/toern_menu.ino"

#define MENU_PAGES_COUNT 10
#define LOOK_PAGES_COUNT 4
#define RECS_PAGES_COUNT 6
#define MIDI_PAGES_COUNT 10
#define VOL_PAGES_COUNT 5
#define ETC_PAGES_COUNT 5
#define MODE_PAGES_COUNT 10
#define ADV_PAGES_COUNT 10


static uint8_t g_ppqnUiFocus = 0;
static uint8_t g_cinFocus = 0;

static uint8_t g_gainEnc3IsAll = 1;
static uint8_t g_gainUiFocus = 3;

static inline int mixGainEnc0FromStored(uint8_t g) {
  int v = (int)g;
  if (v < 0) v = 0;
  if (v > MIX_GAIN_MAX) v = MIX_GAIN_MAX;
  return MIX_GAIN_MAX - v;
}
static inline uint8_t mixGainStoredFromEnc0(int pos) {
  pos = constrain(pos, 0, MIX_GAIN_MAX);
  return (uint8_t)(MIX_GAIN_MAX - pos);
}


extern Mode *currentMode;
extern int ctrlMode;
void refreshCtrlEncoderConfig();
extern bool pong;
extern uint16_t pongUpdateInterval;
extern bool MIDI_TRANSPORT_SEND;
void updatePreviewVolume();
extern int previewTriggerMode;
extern const int PREVIEW_MODE_ON;
extern const int PREVIEW_MODE_PRESS;
extern const int PREVIEW_MODE_SYNC;
extern uint8_t currentColorScheme;
extern void applyColorScheme(uint8_t scheme);
extern uint16_t drawRFullMuteCustomUnmuteMask;

static inline CRGB dimIconColorFromText(CRGB textColor);
static inline CRGB menuTextColorFromCol(uint8_t colIndex);
static inline CRGB currentMenuParentTextColor();
static inline void drawLargeIndicatorCustom(CRGB color, int encoderNum);
static inline uint32_t hashEncoderPositions(const Mode *m);
static inline void clearTextArea(int startX, int startY, int width);
static inline CRGB menuValueColor(CRGB color) {
  uint8_t peak = max(color.r, max(color.g, color.b));
  if (!peak) return color;
  return CRGB((uint16_t)color.r * 120u / peak,
              (uint16_t)color.g * 120u / peak,
              (uint16_t)color.b * 120u / peak);
}

static inline void drawMenuText(const char* label, int x, int y, CRGB color) {
  drawText(label, x, y, y == 3 ? menuValueColor(color) : color);
}

static inline void drawMenuIndicator(char size, char code, int encoderNum,
                                     bool highlight = false, bool setEncoderLed = true) {
  drawIndicator(size, code, encoderNum, highlight, setEncoderLed, 120);
}

static inline void drawMenuValue(const char* label, int x, int y, CRGB color);


static inline int userChToInternalDrawR(int uch) {
  if (uch < 1) uch = 1;
  if (uch > 16) uch = 16;
  return (uch == 16) ? 0 : uch;
}
static inline int internalToUserChDrawR(int in) {
  if (in < 0) in = 0;
  if (in > 15) in = 15;
  return (in == 0) ? 16 : in;
}


struct MenuPage {
  const char* name;
  int mainSetting;
  bool hasAdditionalFeatures;
  const char* additionalFeatureName;
};

MenuPage menuPages[MENU_PAGES_COUNT] = {
  {"PATT", 1, false, nullptr},
  {"KIT", 2, false, nullptr},
  {"SMP", 3, false, nullptr},
  {"BPM", 5, false, nullptr},
  {"VOL", 26, false, nullptr},
  {"SETT", 19, false, nullptr},
  {"REC", 20, false, nullptr},
  {"MIDI", 21, false, nullptr},
  {"MODE", 61, false, nullptr},
  {"ETC", 34, false, nullptr}
};


MenuPage lookPages[LOOK_PAGES_COUNT] = {
  {"VIEW", 17, false, nullptr},
  {"CTRL", 25, false, nullptr},
  {"DRAW", 38, false, nullptr},
  {"MUTE", 47, false, nullptr}
};

static inline uint16_t pongSpeedToInterval(int speed) {
  speed = constrain(speed, 1, 99);
  return static_cast<uint16_t>(mapf(speed, 1, 99, 300, 20));
}

static inline int pongIntervalToSpeed(uint16_t interval) {
  interval = constrain(interval, 20, 300);
  return static_cast<int>(mapf(interval, 300, 20, 1, 99));
}


MenuPage recsPages[RECS_PAGES_COUNT] = {
  {"INPT", 4, true, "GAIN"},
  {"MIC", 30, false, nullptr},
  {"HPF", 64, false, nullptr},
  {"L-IN", 31, false, nullptr},
  {"TRIG", 11, false, nullptr},
  {"CLR", 12, false, nullptr}
};


MenuPage midiPages[MIDI_PAGES_COUNT] = {
  {"CH", 7, false, nullptr},
  {"TRAN", 8, false, nullptr},
  {"SEND", 13, false, nullptr},
  {"RCVE", 44, false, nullptr},
  {"FOLD", 51, false, nullptr},
  {"SYNC", 45, false, nullptr},
  {"PPQN", 50, false, nullptr},
  {"TRIG", 58, false, nullptr},
  {"CIN", 59, false, nullptr},
  {"OVWR", 60, false, nullptr},
};


MenuPage volPages[VOL_PAGES_COUNT] = {
  {"MAIN", 27, false, nullptr},
  {"GAIN", 53, false, nullptr},
  {"LOUT", 28, false, nullptr},
  {"PREV", 29, false, nullptr},
  {"SPKR", 43, false, nullptr}
};


MenuPage etcPages[ETC_PAGES_COUNT] = {
  {"INFO", 39, false, nullptr},
  {"SD", 49, false, nullptr},
  {"BATT", 42, false, nullptr},
  {"ADV", 62, false, nullptr},
  {"RSET", 16, true, "MODE"}
};
MenuPage modePages[MODE_PAGES_COUNT] = {
  {"FLOW", 10, false, nullptr},
  {"PMOD", 9, false, nullptr},
  {"LOOP", 18, false, nullptr},
  {"VMOD", 54, false, nullptr},
  {"CHLD", 48, false, nullptr},
  {"IMG", 56, false, nullptr},
  {"PONG", 24, false, nullptr},
  {"SONG", 22, false, nullptr},
  {"TCHY", 63, false, nullptr},
  {"AUTO", 15, true, "PAGES"}
};
MenuPage advancedPages[ADV_PAGES_COUNT] = {
  {"CRSR", 32, false, nullptr},
  {"LEDS", 23, false, nullptr},
  {"RAM", 52, false, nullptr},
  {"LGHT", 40, false, nullptr},
  {"COLR", 41, false, nullptr},
  {"EYES", 57, false, nullptr},
  {"FIRE", 55, false, nullptr},
  {"2-CH", 36, false, nullptr},
  {"USBP", 65, false, nullptr},
  {"HFC", 46, false, nullptr}
};



enum class SystemMenuGroup : uint8_t { Etc, Mode, Advanced };
static SystemMenuGroup currentSystemMenuGroup = SystemMenuGroup::Etc;
static MenuPage* activeSystemPages() {
  if (currentSystemMenuGroup == SystemMenuGroup::Mode) return modePages;
  if (currentSystemMenuGroup == SystemMenuGroup::Advanced) return advancedPages;
  return etcPages;
}
static int activeSystemPageCount() {
  if (currentSystemMenuGroup == SystemMenuGroup::Mode) return MODE_PAGES_COUNT;
  if (currentSystemMenuGroup == SystemMenuGroup::Advanced) return ADV_PAGES_COUNT;
  return ETC_PAGES_COUNT;
}

int systemMenuParentPage() {
  return currentSystemMenuGroup == SystemMenuGroup::Mode ? 8 : 9;
}


static bool infoPageFirstEnter = true;
static uint32_t infoPageEnterMs = 0;


static bool menuForceFullRedraw = false;
FLASHMEM void menuRequestFullRedraw() {
  menuForceFullRedraw = true;
}

FLASHMEM void togglePulseClockPolarityFromMenu() {
  extern void togglePulseClockPolarity();
  togglePulseClockPolarity();
  g_ppqnUiFocus = 0;
  menuRequestFullRedraw();
}

FLASHMEM void toggleMixGainEnc3Target() {
  g_gainEnc3IsAll = g_gainEnc3IsAll ? 0 : 1;
  g_gainUiFocus = g_gainEnc3IsAll ? 3 : 2;
  menuRequestFullRedraw();
}
static inline bool takeMenuForceFullRedraw() {
  if (!menuForceFullRedraw) return false;
  menuForceFullRedraw = false;
  return true;
}

static int textPixelWidth_3x5(const char *text) {
  if (!text) return 0;
  int w = 0;
  for (int i = 0; text[i] != '\0'; i++) {
    char c = text[i];
    if (c < 32 || c > 126) continue;
    w += alphabet[c - 32][0] + 1;
  }
  return w;
}

FLASHMEM void resetEtcInfoPageAnimation() {
  infoPageFirstEnter = true;
  infoPageEnterMs = 0;
}

#ifndef TOERN_AUDIO_MEMORY_BLOCKS
#define TOERN_AUDIO_MEMORY_BLOCKS 96
#endif

static int etcRamMetric = 0;
static int lastEtcRamEnc = -1;

static int computeReverbUsagePercent() {

  static const uint8_t kFreeverbChannels[] = { 1, 2, 5, 6, 7, 8, 11 };
  int sum = 0;
  int n = 0;
  for (uint8_t ch : kFreeverbChannels) {
    sum += (int)SMP.filter_settings[ch][REVERB];
    n++;
  }
  if (n <= 0) return 0;
  return constrain((sum * 100) / (n * (int)maxfilterResolution), 0, 100);
}

static int computeAudioUsedPercent() {
  int used = (int)AudioMemoryUsage();
  int pool = TOERN_AUDIO_MEMORY_BLOCKS;
  if (pool < 1) pool = 1;
  return constrain((used * 100) / pool, 0, 100);
}

static int computeAudioFreeBlocks() {
  int used = (int)AudioMemoryUsage();
  int freeBlocks = TOERN_AUDIO_MEMORY_BLOCKS - used;
  return freeBlocks < 0 ? 0 : freeBlocks;
}

FLASHMEM static void drawEtcRamPage() {
  const CRGB tc = currentMenuParentTextColor();
  drawText("RAM", 2, 10, tc);

  char line[8];
  CRGB valueColor = UI_GREEN;
  switch (etcRamMetric) {
    case 1: {
      int rev = computeReverbUsagePercent();
      snprintf(line, sizeof(line), "R%d", rev);
      valueColor = rev > 70 ? UI_YELLOW : UI_GREEN;
      break;
    }
    case 2: {
      int freeBlocks = computeAudioFreeBlocks();
      snprintf(line, sizeof(line), "F%d", freeBlocks);
      valueColor = freeBlocks < 12 ? UI_RED : (freeBlocks < 24 ? UI_YELLOW : UI_GREEN);
      break;
    }
    case 3: {
      int cpu = constrain((int)lroundf(AudioProcessorUsage()), 0, 99);
      snprintf(line, sizeof(line), "C%d", cpu);
      valueColor = cpu > 80 ? UI_RED : (cpu > 55 ? UI_YELLOW : UI_GREEN);
      break;
    }
    default: {
      int used = computeAudioUsedPercent();
      int peak = constrain(
          ((int)AudioMemoryUsageMax() * 100) / max(1, TOERN_AUDIO_MEMORY_BLOCKS),
          0, 100);

      if (peak > used + 5) {
        snprintf(line, sizeof(line), "A%d", peak);
      } else {
        snprintf(line, sizeof(line), "A%d", used);
      }
      valueColor = used > 85 ? UI_RED : (used > 65 ? UI_YELLOW : UI_GREEN);
      break;
    }
  }
  drawMenuValue(line, 2, 3, valueColor);
  drawMenuIndicator('L', 'G', 3, false, false);
}

FLASHMEM static void drawEtcInfoPage() {

  drawText("INFO", 2, 10, currentMenuParentTextColor());

  const uint32_t now = millis();
  if (infoPageFirstEnter) {
    infoPageFirstEnter = false;
    infoPageEnterMs = now;
  }


  const uint32_t holdMs = 2000;
  if (now - infoPageEnterMs < holdMs) {
    drawText(VERSION, 2, 3, CRGB(255, 255, 0));
    return;
  }


  static const char *kMsg =
    "   Thank you for using TOERN. Shout out to Matzesampler, Sabrina, Hairy and all others for supporting me. Jan";

  const uint32_t scrollStartMs = infoPageEnterMs + holdMs;
  const uint32_t elapsedMs = (now >= scrollStartMs) ? (now - scrollStartMs) : 0;
  const uint32_t msPerPixel = 60;
  const int offsetPx = (int)(elapsedMs / msPerPixel);

  const int versionBaseX = 2;
  const int versionX = versionBaseX - offsetPx;
  const int msgStartX = (int)maxX + 1;
  const int msgX = msgStartX - offsetPx;

  drawText(VERSION, versionX, 3, CRGB(255, 255, 0));
  drawText(kMsg, msgX, 3, UI_WHITE);


  const int msgW = textPixelWidth_3x5(kMsg);
  if (msgX < -msgW - 2) {
    infoPageEnterMs = now;
  }
}

int currentMenuPage = 0;
int currentLookPage = 0;
int currentRecsPage = 0;
int currentMidiPage = 0;
int currentVolPage = 0;
int currentEtcPage = 0;
bool inLookSubmenu = false;
bool inRecsSubmenu = false;
bool inMidiSubmenu = false;
bool inVolSubmenu = false;
bool inEtcSubmenu = false;
bool menuEnteredFromSingleMode = false;



static unsigned long menuExitMs = 0;
static const unsigned long MENU_REENTRY_GRACE_MS = 5000;

FLASHMEM void noteMenuExit() {
  menuExitMs = millis();
}




static int g_cachedEnc3Max = -999;
static int g_cachedEnc3Min = -999;

FLASHMEM void invalidateEnc3RangeCache() {
  g_cachedEnc3Max = -999;
  g_cachedEnc3Min = -999;
}

FLASHMEM static void ensureEnc3Range(int wantMin, int wantMax) {
  if (g_cachedEnc3Max != wantMax) {
    Encoder[3].writeMax((int32_t)wantMax);
    g_cachedEnc3Max = wantMax;
  }
  if (g_cachedEnc3Min != wantMin) {
    Encoder[3].writeMin((int32_t)wantMin);
    g_cachedEnc3Min = wantMin;
  }
}

FLASHMEM void prepareMenuEntry() {


  invalidateEnc3RangeCache();
  if (menuExitMs != 0 && (millis() - menuExitMs) >= MENU_REENTRY_GRACE_MS) {
    currentMenuPage = 0;
    inLookSubmenu = false;
    inRecsSubmenu = false;
    inMidiSubmenu = false;
    inVolSubmenu = false;
    inEtcSubmenu = false;
    currentLookPage = 0;
    currentRecsPage = 0;
    currentMidiPage = 0;
    currentVolPage = 0;
    currentEtcPage = 0;
    currentSystemMenuGroup = SystemMenuGroup::Etc;
  }
  extern Mode menu;
  menu.pos[3] = currentMenuPage;
}
int aiTargetPage = 6;
int aiBaseStartPage = 1;
int aiBaseEndPage = 1;


int genreType = 0;
int genreLength = 8;


bool newScreenFirstEnter = true;


int resetMenuOption = 0;


int drawMode = 0;




static const char *SETTINGS_BACKUP_PATH = "settings.txt";
static const char *SETTINGS_BACKUP_TMP_PATH = "settings.tmp";
static const char *SETTINGS_BACKUP_HEADER = "TOERN_SETTINGS_V1";
static const uint16_t SETTINGS_EEPROM_BLOCK_LEN = 59;
static const uint16_t EEPROM_SAMPLEPACK_ADDR = 0;
static const uint16_t EEPROM_SP0_STATE_ADDR = 200;
static const uint8_t EEPROM_SP0_STATE_COUNT = 8;
static bool settingsBackupDirty = false;
static uint32_t settingsBackupDirtyMs = 0;
static const uint32_t SETTINGS_BACKUP_DEBOUNCE_MS = 1500;



static uint8_t g_pendingMixGainMask = 0;
static bool g_pendingSp0Eeprom = false;

static uint8_t g_pendingEepromBytes[SETTINGS_EEPROM_BLOCK_LEN];
static uint8_t g_pendingEepromMask[(SETTINGS_EEPROM_BLOCK_LEN + 7) / 8];
static bool g_pendingEepromAny = false;

static inline void pendingEepromSet(int index, uint8_t value) {
  if (index < 0 || index >= (int)SETTINGS_EEPROM_BLOCK_LEN) return;
  g_pendingEepromBytes[index] = value;
  g_pendingEepromMask[index >> 3] |= (uint8_t)(1u << (index & 7));
  g_pendingEepromAny = true;
}

FLASHMEM static void flushPendingEepromWrites() {
  if (!g_pendingEepromAny) return;
  for (int i = 0; i < (int)SETTINGS_EEPROM_BLOCK_LEN; i++) {
    if (g_pendingEepromMask[i >> 3] & (uint8_t)(1u << (i & 7))) {
      EEPROM.write(EEPROM_DATA_START + i, g_pendingEepromBytes[i]);
    }
  }
  memset(g_pendingEepromMask, 0, sizeof(g_pendingEepromMask));
  g_pendingEepromAny = false;
}


EXTMEM static char g_settingsBackupReadLine[1024];
EXTMEM static uint8_t g_eepromRestoredBlock[SETTINGS_EEPROM_BLOCK_LEN];
EXTMEM static uint8_t g_eepromRestoredSp0[EEPROM_SP0_STATE_COUNT];

static inline uint32_t crc32_update(uint32_t crc, uint8_t data) {
  crc ^= data;
  for (int i = 0; i < 8; i++) {
    crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320UL : (crc >> 1);
  }
  return crc;
}

static uint32_t crc32_compute(const uint8_t *data, size_t len) {
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < len; i++) crc = crc32_update(crc, data[i]);
  return ~crc;
}

static bool readLine(File &f, char *buf, size_t bufSize) {
  if (!f) return false;
  size_t idx = 0;
  while (f.available()) {
    int c = f.read();
    if (c < 0) break;
    if (c == '\r') continue;
    if (c == '\n') break;
    if (idx + 1 < bufSize) buf[idx++] = (char)c;
  }
  buf[idx] = 0;

  if (idx == 0 && !f.available()) return false;
  return true;
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

static bool decodeHexBytes(const char *hex, uint8_t *out, size_t outLen) {
  for (size_t i = 0; i < outLen; i++) {
    int hi = hexNibble(hex[i * 2]);
    int lo = hexNibble(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) return false;
    out[i] = (uint8_t)((hi << 4) | lo);
  }
  return true;
}

static void encodeHexBytes(const uint8_t *in, size_t inLen, char *out, size_t outSize) {

  static const char *HEX_CHARS = "0123456789ABCDEF";
  size_t needed = inLen * 2 + 1;
  if (outSize < needed) {
    if (outSize > 0) out[0] = 0;
    return;
  }
  for (size_t i = 0; i < inLen; i++) {
    out[i * 2] = HEX_CHARS[(in[i] >> 4) & 0xF];
    out[i * 2 + 1] = HEX_CHARS[in[i] & 0xF];
  }
  out[inLen * 2] = 0;
}

void markSettingsBackupDirty() {
  settingsBackupDirty = true;
  settingsBackupDirtyMs = millis();
}

FLASHMEM static bool writeSettingsBackupToSD() {
  extern void stopSdPreviewIfPlaying();
  stopSdPreviewIfPlaying();


  unsigned int spid = 1;
  EEPROM.get(EEPROM_SAMPLEPACK_ADDR, spid);
  if (spid > 999) {
    spid = 0;
  }

  uint8_t block[SETTINGS_EEPROM_BLOCK_LEN];
  for (uint16_t i = 0; i < SETTINGS_EEPROM_BLOCK_LEN; i++) {
    block[i] = (uint8_t)EEPROM.read(EEPROM_DATA_START + i);
  }

  uint8_t sp0Block[EEPROM_SP0_STATE_COUNT];
  for (uint8_t i = 0; i < EEPROM_SP0_STATE_COUNT; i++) {
    sp0Block[i] = (uint8_t)EEPROM.read(EEPROM_SP0_STATE_ADDR + 1 + i);
  }

  uint8_t payload[sizeof(unsigned int) + SETTINGS_EEPROM_BLOCK_LEN + EEPROM_SP0_STATE_COUNT];
  memcpy(payload, &spid, sizeof(unsigned int));
  memcpy(payload + sizeof(unsigned int), block, SETTINGS_EEPROM_BLOCK_LEN);
  memcpy(payload + sizeof(unsigned int) + SETTINGS_EEPROM_BLOCK_LEN, sp0Block, EEPROM_SP0_STATE_COUNT);
  uint32_t crc = crc32_compute(payload, sizeof(payload));


  if (SD.exists(SETTINGS_BACKUP_TMP_PATH)) SD.remove(SETTINGS_BACKUP_TMP_PATH);
  File f = SD.open(SETTINGS_BACKUP_TMP_PATH, O_WRITE | O_CREAT | O_TRUNC);
  if (!f) return false;

  f.println(SETTINGS_BACKUP_HEADER);
  f.print("LEN=");
  f.println((unsigned int)sizeof(payload));
  f.print("CRC=");
  f.println(crc, HEX);
  f.print("DATA=");
  char hex[(sizeof(payload) * 2) + 1];
  encodeHexBytes(payload, sizeof(payload), hex, sizeof(hex));
  f.println(hex);
  f.close();

  if (SD.exists(SETTINGS_BACKUP_PATH)) SD.remove(SETTINGS_BACKUP_PATH);
  if (!SD.rename(SETTINGS_BACKUP_TMP_PATH, SETTINGS_BACKUP_PATH)) {

    return false;
  }
  return true;
}

FLASHMEM static bool readSettingsBackupFromSD(unsigned int &outSamplePackID, uint8_t *outBlock, size_t outBlockLen, uint8_t *outSp0Block, size_t outSp0BlockLen) {
  if (!SD.exists(SETTINGS_BACKUP_PATH)) return false;
  File f = SD.open(SETTINGS_BACKUP_PATH, FILE_READ);
  if (!f) return false;

  char *const line = g_settingsBackupReadLine;
  if (!readLine(f, line, sizeof(g_settingsBackupReadLine))) { f.close(); return false; }
  if (strcmp(line, SETTINGS_BACKUP_HEADER) != 0) { f.close(); return false; }

  if (!readLine(f, line, sizeof(g_settingsBackupReadLine))) { f.close(); return false; }
  if (strncmp(line, "LEN=", 4) != 0) { f.close(); return false; }
  unsigned int len = (unsigned int)atoi(line + 4);

  if (!readLine(f, line, sizeof(g_settingsBackupReadLine))) { f.close(); return false; }
  if (strncmp(line, "CRC=", 4) != 0) { f.close(); return false; }
  uint32_t expectedCrc = (uint32_t)strtoul(line + 4, nullptr, 16);

  if (!readLine(f, line, sizeof(g_settingsBackupReadLine))) { f.close(); return false; }
  if (strncmp(line, "DATA=", 5) != 0) { f.close(); return false; }
  const char *hex = line + 5;


  const size_t minLen = sizeof(unsigned int) + 1;
  const size_t maxLen = sizeof(unsigned int) + SETTINGS_EEPROM_BLOCK_LEN + EEPROM_SP0_STATE_COUNT;
  if (len < minLen || len > maxLen) { f.close(); return false; }
  if (strlen(hex) < (size_t)len * 2) { f.close(); return false; }

  uint8_t payload[sizeof(unsigned int) + SETTINGS_EEPROM_BLOCK_LEN + EEPROM_SP0_STATE_COUNT];
  if (!decodeHexBytes(hex, payload, (size_t)len)) { f.close(); return false; }
  uint32_t crc = crc32_compute(payload, (size_t)len);
  if (crc != expectedCrc) { f.close(); return false; }


  memcpy(&outSamplePackID, payload, sizeof(unsigned int));
  if (outBlockLen < SETTINGS_EEPROM_BLOCK_LEN) { f.close(); return false; }
  memset(outBlock, 0, SETTINGS_EEPROM_BLOCK_LEN);


  outBlock[37] = 1;

  if (outSp0BlockLen < EEPROM_SP0_STATE_COUNT) { f.close(); return false; }
  memset(outSp0Block, 0, EEPROM_SP0_STATE_COUNT);


  {
    size_t totalAfterSpid = (size_t)len - sizeof(unsigned int);
    size_t blkInFile = 0;
    size_t sp0Copy = 0;
    if (totalAfterSpid >= EEPROM_SP0_STATE_COUNT) {
      blkInFile = totalAfterSpid - EEPROM_SP0_STATE_COUNT;
      sp0Copy = EEPROM_SP0_STATE_COUNT;
    } else {
      blkInFile = totalAfterSpid;
    }
    size_t blkCopy = blkInFile;
    if (blkCopy > SETTINGS_EEPROM_BLOCK_LEN) blkCopy = SETTINGS_EEPROM_BLOCK_LEN;
    memcpy(outBlock, payload + sizeof(unsigned int), blkCopy);
    if (sp0Copy > 0) {
      memcpy(outSp0Block, payload + sizeof(unsigned int) + blkInFile, sp0Copy);
    }

    if (blkCopy <= 37) outBlock[37] = 1;

    if (blkCopy <= 55) outBlock[55] = 0xA5;

    if (blkCopy <= 56) outBlock[56] = 1;
    if (blkCopy <= 57) outBlock[57] = 0;

    {
      const bool gainAbsentOrBlank =
          (blkCopy <= 38) ||
          ((outBlock[38] | outBlock[39] | outBlock[40] | outBlock[41]) == 0);
      for (int i = 38; i <= 41; i++) {
        if (blkCopy <= (size_t)i || outBlock[i] > MIX_GAIN_MAX || gainAbsentOrBlank) {
          outBlock[i] = (uint8_t)MIX_GAIN_UNITY;
        }
      }
    }
  }
  f.close();
  return true;
}

FLASHMEM void serviceSettingsBackup() {

  extern bool isNowPlaying;
  extern bool sdIoAudioSettled();
  const bool audioQuiet = !isNowPlaying && sdIoAudioSettled();
  extern uint8_t mixGain14;
  extern uint8_t mixGain58;
  extern uint8_t mixGainSynth;
  extern uint8_t mixGainMaster;
  if (audioQuiet) {
    if (g_pendingMixGainMask) {
      if (g_pendingMixGainMask & 0x01) EEPROM.write(EEPROM_DATA_START + EEPROM_MIX_GAIN_14, mixGain14);
      if (g_pendingMixGainMask & 0x02) EEPROM.write(EEPROM_DATA_START + EEPROM_MIX_GAIN_58, mixGain58);
      if (g_pendingMixGainMask & 0x04) EEPROM.write(EEPROM_DATA_START + EEPROM_MIX_GAIN_SYN, mixGainSynth);
      if (g_pendingMixGainMask & 0x08) EEPROM.write(EEPROM_DATA_START + EEPROM_MIX_GAIN_ALL, mixGainMaster);
      g_pendingMixGainMask = 0;
      markSettingsBackupDirty();
    }
    if (g_pendingEepromAny) {
      flushPendingEepromWrites();
      markSettingsBackupDirty();
    }

    if (g_pendingSp0Eeprom) {
      g_pendingSp0Eeprom = false;
      saveSp0StateToEEPROM();
    }
  }

  if (!settingsBackupDirty) return;

  if (isNowPlaying || !audioQuiet) return;
  if ((uint32_t)(millis() - settingsBackupDirtyMs) < SETTINGS_BACKUP_DEBOUNCE_MS) return;

  if (writeSettingsBackupToSD()) {
    settingsBackupDirty = false;
  }
}




FLASHMEM void flushSettingsBackupNow() {
  if (!settingsBackupDirty) return;
  extern bool isNowPlaying;
  if (isNowPlaying) return;
  if (writeSettingsBackupToSD()) {
    settingsBackupDirty = false;
  }
}


FLASHMEM static uint8_t hfcEncodeStep(uint16_t cut) {
  return cut >= 256u ? 26u : (uint8_t)(cut / 10u);
}

FLASHMEM static uint16_t hfcDecodeStep(uint8_t step) {
  if (step > 26u) return 0u;
  return step == 26u ? 256u : (uint16_t)step * 10u;
}

FLASHMEM static uint16_t hfcNormalize256(uint16_t h) {
  if (h >= 256u) return 256u;
  return (uint16_t)((h / 10u) * 10u);
}

FLASHMEM static uint16_t hfcMigrate22000To256(uint16_t oldv) {
  if (oldv > 22000u) oldv = 22000u;
  oldv = (uint16_t)((oldv / 100u) * 100u);
  uint32_t m = (uint32_t)oldv * 256u + 11000u;
  uint16_t hv = (uint16_t)(m / 22000u);
  return hfcNormalize256(hv);
}

FLASHMEM static uint16_t hfcMigrate44200To256(uint16_t raw3) {
  if (raw3 > 44200u) raw3 = 44200u;
  raw3 = (uint16_t)((raw3 / 100u) * 100u);
  uint32_t m = (uint32_t)raw3 * 256u + 22100u;
  uint16_t hv = (uint16_t)(m / 44200u);
  return hfcNormalize256(hv);
}

FLASHMEM void loadMenuFromEEPROM() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {

    unsigned int restoredPack = 0;
    uint8_t *const restoredBlock = g_eepromRestoredBlock;
    uint8_t *const restoredSp0Block = g_eepromRestoredSp0;
    bool restored = readSettingsBackupFromSD(restoredPack, restoredBlock, SETTINGS_EEPROM_BLOCK_LEN, restoredSp0Block, EEPROM_SP0_STATE_COUNT);

    if (restored) {
      if (restoredPack > 999) {
        restoredPack = 0;
      }
      EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
      EEPROM.put(EEPROM_SAMPLEPACK_ADDR, restoredPack);
      for (uint16_t i = 0; i < SETTINGS_EEPROM_BLOCK_LEN; i++) {
        EEPROM.write(EEPROM_DATA_START + i, restoredBlock[i]);
      }
      for (uint8_t i = 0; i < EEPROM_SP0_STATE_COUNT; i++) {
        EEPROM.write(EEPROM_SP0_STATE_ADDR + 1 + i, restoredSp0Block[i]);
      }


      extern unsigned int samplePackID;
      samplePackID = restoredPack;
      SMP.pack = restoredPack;
    } else {

      EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
      EEPROM.put(EEPROM_SAMPLEPACK_ADDR, (unsigned int)1);
      EEPROM.write(EEPROM_DATA_START + 0, 1);
      EEPROM.write(EEPROM_DATA_START + 1, 1);
      EEPROM.write(EEPROM_DATA_START + 2, 2);
      EEPROM.write(EEPROM_DATA_START + 3, (uint8_t)-1);
      EEPROM.write(EEPROM_DATA_START + 4, -1);
      EEPROM.write(EEPROM_DATA_START + 5, 2);
      EEPROM.write(EEPROM_DATA_START + 58, 0xA5);
      EEPROM.write(EEPROM_DATA_START + 6, 0);
      EEPROM.write(EEPROM_DATA_START + 7, 20);
      EEPROM.write(EEPROM_DATA_START + 8, 1);
      EEPROM.write(EEPROM_DATA_START + 9, 10);

      EEPROM.write(EEPROM_DATA_START + 10, (uint8_t)(6 << 2));
      EEPROM.write(EEPROM_DATA_START + 30, 12);
      EEPROM.write(EEPROM_DATA_START + 11, 1);
      EEPROM.write(EEPROM_DATA_START + 12, 0);
      EEPROM.write(EEPROM_DATA_START + 13, 1);
      EEPROM.write(EEPROM_DATA_START + 14, 1);
      EEPROM.write(EEPROM_DATA_START + 15, 30);
      EEPROM.write(EEPROM_DATA_START + 16, 8);
      EEPROM.write(EEPROM_DATA_START + 17, 100);
      EEPROM.write(EEPROM_DATA_START + 18, 0);
      EEPROM.write(EEPROM_DATA_START + 19, 0);
      EEPROM.write(EEPROM_DATA_START + 20, 0);
      EEPROM.write(EEPROM_DATA_START + 21, 0);
      EEPROM.write(EEPROM_DATA_START + 22, 0);
      EEPROM.write(EEPROM_DATA_START + 23, 0);
      EEPROM.write(EEPROM_DATA_START + 24, 2);
      EEPROM.write(EEPROM_DATA_START + 25, 0);
      EEPROM.write(EEPROM_DATA_START + 26, 64);
      EEPROM.write(EEPROM_DATA_START + 27, 1);
      EEPROM.write(EEPROM_DATA_START + 55, 0xA5);
      EEPROM.write(EEPROM_DATA_START + 56, 1);
      EEPROM.write(EEPROM_DATA_START + 57, 0);
      EEPROM.write(EEPROM_DATA_START + 28, 1);

      EEPROM.write(EEPROM_DATA_START + 29, 0);
      EEPROM.write(EEPROM_DATA_START + 31, 0);
      EEPROM.write(EEPROM_DATA_START + 32, 0);
      EEPROM.write(EEPROM_DATA_START + 33, 5);
      EEPROM.put(EEPROM_DATA_START + 34, (uint16_t)0x0006);
      EEPROM.write(EEPROM_DATA_START + 36, 0);
      EEPROM.write(EEPROM_DATA_START + 37, 1);
      EEPROM.write(EEPROM_DATA_START + 38, 10);
      EEPROM.write(EEPROM_DATA_START + 39, 10);
      EEPROM.write(EEPROM_DATA_START + 40, 10);
      EEPROM.write(EEPROM_DATA_START + 41, 10);
      EEPROM.write(EEPROM_DATA_START + 42, 0);
      EEPROM.write(EEPROM_DATA_START + 43, 0);
      EEPROM.write(EEPROM_DATA_START + 44, 8);
      EEPROM.write(EEPROM_DATA_START + 45, 1);
      EEPROM.write(EEPROM_DATA_START + 46, 0);
      EEPROM.write(EEPROM_DATA_START + 47, 8);
      EEPROM.write(EEPROM_DATA_START + 48, 0);
      EEPROM.write(EEPROM_DATA_START + 49, 1);
      EEPROM.write(EEPROM_DATA_START + 51, 1);
      EEPROM.write(EEPROM_DATA_START + 52, 0);
      EEPROM.write(EEPROM_DATA_START + 53, 2);
      EEPROM.write(EEPROM_DATA_START + 54, 0);
      for (uint8_t i = 0; i < EEPROM_SP0_STATE_COUNT; i++) {
        EEPROM.write(EEPROM_SP0_STATE_ADDR + 1 + i, 0);
      }


      markSettingsBackupDirty();
    }
  }


  recMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 0);
  if (recMode != 1 && recMode != -1) {
    recMode = 1;
    saveSingleModeToEEPROM(0, recMode);
  }

  clockMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 1);
  if (clockMode != 0 && clockMode != 1) {
    clockMode = 1;
    saveSingleModeToEEPROM(1, clockMode);
  }

  transportMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 2);
  if (transportMode != -1 && transportMode != 1 && transportMode != 2) {
    transportMode = 2;
    saveSingleModeToEEPROM(2, transportMode);
  }

  patternMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 3);
  if (patternMode != -1 && patternMode != 1 && patternMode != 2 && patternMode != 3) {
    patternMode = -1;
    saveSingleModeToEEPROM(3, patternMode);
  }

  voiceSelect = (int8_t) EEPROM.read(EEPROM_DATA_START + 4);
  if (voiceSelect != -1 && voiceSelect != 1 && voiceSelect != 2) {
    voiceSelect = -1;
    saveSingleModeToEEPROM(4, voiceSelect);
  }

  fastRecMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 5);
  {
    const uint8_t FASTREC_SENS_DEFAULT_MIGRATE = 0xA5;
    uint8_t trigMig = EEPROM.read(EEPROM_DATA_START + 58);
    if (trigMig != FASTREC_SENS_DEFAULT_MIGRATE) {
      if (fastRecMode == 1) {
        fastRecMode = 2;
        saveSingleModeToEEPROM(5, 2);
      }
      EEPROM.write(EEPROM_DATA_START + 58, FASTREC_SENS_DEFAULT_MIGRATE);
    }
    if (fastRecMode > 4) {
      fastRecMode = 2;
      saveSingleModeToEEPROM(5, fastRecMode);
    }
  }

  recChannelClear = (int8_t) EEPROM.read(EEPROM_DATA_START + 6);
  if (recChannelClear < 0 || recChannelClear > 4) {
    recChannelClear = 0;
    saveSingleModeToEEPROM(6, recChannelClear);
  }

  previewVol = (int8_t) EEPROM.read(EEPROM_DATA_START + 7);
  if (previewVol > 50) {
    previewVol = 20;
    saveSingleModeToEEPROM(7, previewVol);
  }

  flowMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 8);
  if (flowMode != -1 && flowMode != 1) {
    flowMode = 1;
    saveSingleModeToEEPROM(8, flowMode);
  }

  {
    extern bool voiceMode;
    uint8_t voiceModeValue = EEPROM.read(EEPROM_DATA_START + 42);
    voiceMode = (voiceModeValue == 1);
    if (voiceModeValue > 1) EEPROM.write(EEPROM_DATA_START + 42, 0);
  }

  {
    extern uint8_t fireVoice;
    extern uint8_t fireLevel;
    extern uint8_t fireSize;
    extern uint8_t fireGravity;
    extern uint8_t fireColor;
    uint8_t fireValue = EEPROM.read(EEPROM_DATA_START + 43);
    const bool fireOk = (fireValue == 0) || (fireValue == 15) ||
                        (fireValue >= 1 && fireValue <= 8) ||
                        fireValue == 11 || fireValue == 13 || fireValue == 14;
    if (!fireOk) {
      fireValue = 0;
      saveSingleModeToEEPROM(43, 0);
    }
    fireVoice = fireValue;
    uint8_t levelValue = EEPROM.read(EEPROM_DATA_START + 44);
    if (levelValue < 1 || levelValue > 25) {
      levelValue = 8;
      saveSingleModeToEEPROM(44, 8);
    }
    fireLevel = levelValue;
    uint8_t sizeValue = EEPROM.read(EEPROM_DATA_START + 45);
    if (sizeValue < 1 || sizeValue > 4) {
      sizeValue = 1;
      saveSingleModeToEEPROM(45, 1);
    }
    fireSize = sizeValue;
    uint8_t gravValue = EEPROM.read(EEPROM_DATA_START + 46);
    if (gravValue > 8) {
      gravValue = 0;
      saveSingleModeToEEPROM(46, 0);
    }
    fireGravity = gravValue;
    uint8_t colorValue = EEPROM.read(EEPROM_DATA_START + 47);
    if (colorValue > 8) {
      colorValue = 8;
      saveSingleModeToEEPROM(47, 8);
    }
    fireColor = colorValue;
  }

  {
    extern bool imageMode;
    extern bool voiceMode;


    const int IMG_MIGRATE_ADDR = EEPROM_DATA_START + 50;
    if (EEPROM.read(IMG_MIGRATE_ADDR) != 0xA5) {
      EEPROM.write(EEPROM_DATA_START + 48, 0);
      EEPROM.write(IMG_MIGRATE_ADDR, 0xA5);
    }
    uint8_t imageModeValue = EEPROM.read(EEPROM_DATA_START + 48);
    imageMode = (imageModeValue == 1);
    if (imageModeValue > 1) {
      imageMode = false;
      EEPROM.write(EEPROM_DATA_START + 48, 0);
    }

    if (imageMode && voiceMode) {
      voiceMode = false;
      EEPROM.write(EEPROM_DATA_START + 42, 0);
    }
    if (imageMode) {
      extern void enforceImageModeConstraints();
      enforceImageModeConstraints();
    }
  }

  {
    extern uint8_t eyesMode;
    uint8_t eyesValue = EEPROM.read(EEPROM_DATA_START + 49);

    if (eyesValue > 3) {
      eyesValue = 1;
      EEPROM.write(EEPROM_DATA_START + 49, 1);
    }
    eyesMode = eyesValue;
  }

  micGain = (int8_t) EEPROM.read(EEPROM_DATA_START + 9);
  if (micGain < 1 || micGain > 63) {
    micGain = 10;
    saveSingleModeToEEPROM(9, micGain);
  }

  simpleNotesView = (int) EEPROM.read(EEPROM_DATA_START + 11);
  if (simpleNotesView < 1 || simpleNotesView > 2) {
    simpleNotesView = 1;
    saveSingleModeToEEPROM(11, simpleNotesView);
  }

  loopLength = (int) EEPROM.read(EEPROM_DATA_START + 12);
  if (loopLength < 0 || loopLength > 8) {
    loopLength = 0;
    saveSingleModeToEEPROM(12, loopLength);
  }
  uint8_t ledMode = EEPROM.read(EEPROM_DATA_START + 13);
  extern bool ledModulesRotated;
  if (ledMode < 1 || ledMode > 4) {
    ledMode = 1;
    saveSingleModeToEEPROM(13, 1);
  }
  ledModules = (ledMode == 1 || ledMode == 3) ? 1 : 2;
  ledModulesRotated = (ledMode >= 3);

  ctrlMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 14);
  if (ctrlMode != 0 && ctrlMode != 1) {
    ctrlMode = 1;
    saveSingleModeToEEPROM(14, ctrlMode);
  }


  extern unsigned int lineInLevel;
  lineInLevel = (unsigned int)EEPROM.read(EEPROM_DATA_START + 16);
  if (lineInLevel > 15) {
    lineInLevel = 8;
    saveSingleModeToEEPROM(16, lineInLevel);
  }


  if (EEPROM.read(EEPROM_DATA_START + 18) != 0) {
    saveSingleModeToEEPROM(18, 0);
  }


  extern bool showChannelNr;
  showChannelNr = (EEPROM.read(EEPROM_DATA_START + 19) != 0);


  drawMode = (int8_t)EEPROM.read(EEPROM_DATA_START + 21);
  if (drawMode < 0 || drawMode > 1) {
    drawMode = 0;
    saveSingleModeToEEPROM(21, drawMode);
  }




  extern int8_t stereoChannel;
  stereoChannel = (int8_t)EEPROM.read(EEPROM_DATA_START + 23);
  if (stereoChannel < 0 || stereoChannel > 8) {

    stereoChannel = 0;
    saveSingleModeToEEPROM(23, stereoChannel);
  }


  extern void applyStereoChannelRouting();
  applyStereoChannelRouting();



  extern int midiSendMode;
  midiSendMode = (int8_t)EEPROM.read(EEPROM_DATA_START + 24);
  if (midiSendMode < 0 || midiSendMode > 2) {

    midiSendMode = 2;
    saveSingleModeToEEPROM(24, midiSendMode);
  }


  extern bool getLedStripEnabled();
  extern void setLedStripEnabled(bool enabled);
  uint8_t ledStripValue = EEPROM.read(EEPROM_DATA_START + 25);
  bool ledStripEnabled = (ledStripValue != 0);
  if (ledStripValue > 1) {

    ledStripEnabled = false;
    saveSingleModeToEEPROM(25, 0);
  }
  setLedStripEnabled(ledStripEnabled);



  extern bool getSpkrEnabled();
  extern void setSpkrEnabled(bool enabled);
  const uint8_t SPKR_POLARITY_MIGRATE = 0xA5;
  uint8_t spkrValue = EEPROM.read(EEPROM_DATA_START + 27);
  uint8_t spkrMig = EEPROM.read(EEPROM_DATA_START + 55);
  bool spkrOn;
  if (spkrMig != SPKR_POLARITY_MIGRATE) {
    if (spkrValue > 1) spkrOn = true;
    else spkrOn = (spkrValue == 0);
    EEPROM.write(EEPROM_DATA_START + 55, SPKR_POLARITY_MIGRATE);
    saveSingleModeToEEPROM(27, spkrOn ? 1 : 0);
  } else {
    spkrOn = (spkrValue != 0);
    if (spkrValue > 1) {
      spkrOn = true;
      saveSingleModeToEEPROM(27, 1);
    }
  }
  setSpkrEnabled(spkrOn);



  extern void setAdcHpfEnabled(bool enabled);
  extern void setUsbSmpPreview(bool enabled);
  uint8_t hpfValue = EEPROM.read(EEPROM_DATA_START + 56);
  if (hpfValue > 1) {
    hpfValue = 1;
    saveSingleModeToEEPROM(56, 1);
  }
  setAdcHpfEnabled(hpfValue != 0);
  uint8_t usbPrevValue = EEPROM.read(EEPROM_DATA_START + 57);
  if (usbPrevValue > 1) {
    usbPrevValue = 0;
    saveSingleModeToEEPROM(57, 0);
  }
  setUsbSmpPreview(usbPrevValue != 0);


  extern bool getChildLockEnabled();
  extern void setChildLockEnabled(bool enabled);
  uint8_t childLockValue = EEPROM.read(EEPROM_DATA_START + 36);
  bool childLockEnabled = (childLockValue != 0);
  if (childLockValue > 1) {

    childLockEnabled = false;
    saveSingleModeToEEPROM(36, 0);
  }
  setChildLockEnabled(childLockEnabled);


  extern bool MIDI_NOTE_RECEIVE;
  uint8_t rcveValue = EEPROM.read(EEPROM_DATA_START + 28);
  if (rcveValue > 1) { rcveValue = 1; saveSingleModeToEEPROM(28, 1); }
  MIDI_NOTE_RECEIVE = (rcveValue != 0);



  extern bool MIDI_NOTE_CLAMP;
  uint8_t clampValue = EEPROM.read(EEPROM_DATA_START + 37);
  if (clampValue > 1) {
    clampValue = 1;
    saveSingleModeToEEPROM(37, 1);
  }
  MIDI_NOTE_CLAMP = (clampValue != 0);

  extern bool MIDI_WRITE_OVERWRITE;
  uint8_t wrteValue = EEPROM.read(EEPROM_DATA_START + 54);
  if (wrteValue > 1) {
    wrteValue = 0;
    saveSingleModeToEEPROM(54, 0);
  }
  MIDI_WRITE_OVERWRITE = (wrteValue != 0);



  {
    extern uint8_t mixGain14;
    extern uint8_t mixGain58;
    extern uint8_t mixGainSynth;
    extern uint8_t mixGainMaster;
    uint8_t g14 = EEPROM.read(EEPROM_DATA_START + EEPROM_MIX_GAIN_14);
    uint8_t g58 = EEPROM.read(EEPROM_DATA_START + EEPROM_MIX_GAIN_58);
    uint8_t gSyn = EEPROM.read(EEPROM_DATA_START + EEPROM_MIX_GAIN_SYN);
    uint8_t gAll = EEPROM.read(EEPROM_DATA_START + EEPROM_MIX_GAIN_ALL);
    bool migrated = false;
    const bool blank = ((g14 | g58 | gSyn | gAll) == 0);
    auto fixGain = [&](uint8_t g) -> uint8_t {
      if (blank || g > MIX_GAIN_MAX) return (uint8_t)MIX_GAIN_UNITY;
      return g;
    };
    uint8_t n14 = fixGain(g14);
    uint8_t n58 = fixGain(g58);
    uint8_t nSyn = fixGain(gSyn);
    uint8_t nAll = fixGain(gAll);
    if (n14 != g14) { EEPROM.write(EEPROM_DATA_START + EEPROM_MIX_GAIN_14, n14); migrated = true; }
    if (n58 != g58) { EEPROM.write(EEPROM_DATA_START + EEPROM_MIX_GAIN_58, n58); migrated = true; }
    if (nSyn != gSyn) { EEPROM.write(EEPROM_DATA_START + EEPROM_MIX_GAIN_SYN, nSyn); migrated = true; }
    if (nAll != gAll) { EEPROM.write(EEPROM_DATA_START + EEPROM_MIX_GAIN_ALL, nAll); migrated = true; }
    mixGain14 = n14;
    mixGain58 = n58;
    mixGainSynth = nSyn;
    mixGainMaster = nAll;
    if (migrated) markSettingsBackupDirty();
  }


  extern int8_t transportSendDelayMs;
  extern int8_t transportRcveDelayMs;
  uint8_t raw29 = EEPROM.read(EEPROM_DATA_START + 29);
  uint8_t raw31 = EEPROM.read(EEPROM_DATA_START + 31);

  if (raw31 == 0xFF) {
    uint8_t raw30 = EEPROM.read(EEPROM_DATA_START + 30);
    int8_t v29 = (raw29 <= 127) ? (int8_t)raw29 : 17;
    int8_t v30 = (raw30 <= 127) ? (int8_t)raw30 : 17;
    transportSendDelayMs = (int8_t)constrain((int)v29, -127, 127);
    transportRcveDelayMs = (int8_t)constrain((int)v30, -127, 127);
    EEPROM.write(EEPROM_DATA_START + 29, (uint8_t)transportSendDelayMs);
    EEPROM.write(EEPROM_DATA_START + 31, (uint8_t)transportRcveDelayMs);
    EEPROM.write(EEPROM_DATA_START + 30, 12);
  } else {
    transportSendDelayMs = (int8_t)constrain((int)(int8_t)raw29, -127, 127);
    transportRcveDelayMs = (int8_t)constrain((int)(int8_t)raw31, -127, 127);
  }


  {
    extern void loadPulseClockFromEEPROM();
    loadPulseClockFromEEPROM();
    extern void loadSidechainFromEEPROM();
    loadSidechainFromEEPROM();
    extern void loadAnalogExtClockFromEEPROM();
    loadAnalogExtClockFromEEPROM();
  }

  {
    extern uint16_t codecHfCut;
    uint8_t eqFmt = EEPROM.read(EEPROM_DATA_START + 33);
    uint16_t raw = 0;
    EEPROM.get(EEPROM_DATA_START + 32, raw);
    uint16_t cut = 0;

    if (eqFmt == 5) {
      cut = hfcDecodeStep(EEPROM.read(EEPROM_DATA_START + 32));
    } else if (eqFmt == 4) {


      cut = (raw & 0xFFu) ? hfcNormalize256(raw & 0xFFu) : 256u;
    } else if (eqFmt == 3) {
      cut = hfcMigrate44200To256(raw);
    } else if (eqFmt == 2) {
      uint16_t r = raw;
      if (r > 500u) r = 500u;
      r = (uint16_t)((r / 10u) * 10u);
      uint32_t m = (uint32_t)r * 256u + 250u;
      uint16_t h = (uint16_t)(m / 500u);
      cut = hfcNormalize256(h);
    } else if (eqFmt == 1) {
      uint16_t oldv = raw;
      if (oldv > 22000u) oldv = 22000u;
      oldv = (uint16_t)((oldv / 100u) * 100u);
      cut = hfcMigrate22000To256(oldv);
    } else {
      uint16_t oldv;
      if (raw >= 6000u && raw <= 20000u && (raw % 100u) == 0) {
        uint32_t delta = raw - 6000u;
        oldv = (uint16_t)(((14000u - delta) * 22000u + 7000u) / 14000u);
      } else if (raw > 0u && raw <= 22000u) {
        oldv = raw;
      } else {
        oldv = 0;
      }
      oldv = (uint16_t)((oldv / 100u) * 100u);
      if (oldv > 22000u) oldv = 22000u;
      cut = hfcMigrate22000To256(oldv);
    }
    if (eqFmt != 5) {
      EEPROM.write(EEPROM_DATA_START + 32, hfcEncodeStep(cut));
      EEPROM.write(EEPROM_DATA_START + 33, 5);
      markSettingsBackupDirty();
    }
    codecHfCut = cut;
  }

  {
    uint16_t fm = 0;
    EEPROM.get(EEPROM_DATA_START + 34, fm);
    drawRFullMuteCustomUnmuteMask = fm;
  }

  if (previewVol < 0 || previewVol > 50) {
    previewVol = 20;
    saveSingleModeToEEPROM(7, previewVol);
  }


  mixer_end.gain(3, 0.0);


  extern unsigned int maxX;
  maxX = MATRIX_WIDTH * ledModules;

  if (recChannelClear < 0 || recChannelClear > 4) {
    recChannelClear = 0;
    saveSingleModeToEEPROM(6, recChannelClear);
  }


  if (flowMode != -1 && flowMode != 1) {
    flowMode = 1;
    saveSingleModeToEEPROM(8, flowMode);
  }


  if (simpleNotesView < 1 || simpleNotesView > 2) {
    simpleNotesView = 1;
    saveSingleModeToEEPROM(11, simpleNotesView);
  }

  if (transportMode != -1 && transportMode != 1 && transportMode != 2) {
    transportMode = -1;
    saveSingleModeToEEPROM(2, transportMode);
  }


  if (loopLength < 0 || loopLength > 8) {
    loopLength = 0;
    saveSingleModeToEEPROM(12, loopLength);
  }


  if (voiceSelect != -1 && voiceSelect != 1 && voiceSelect != 2) {
    voiceSelect = 1;
    saveSingleModeToEEPROM(4, voiceSelect);
  }


  if (patternMode != -1 && patternMode != 1 && patternMode != 2 && patternMode != 3) {
    patternMode = -1;
    saveSingleModeToEEPROM(3, patternMode);
  }


  if (ledMode < 1 || ledMode > 4) {
    ledMode = 1;
    ledModules = 1;
    ledModulesRotated = false;
    saveSingleModeToEEPROM(13, 1);
  }

  if (ctrlMode != 0 && ctrlMode != 1) {
    ctrlMode = 1;
    saveSingleModeToEEPROM(14, ctrlMode);
  }


  SMP_PATTERN_MODE = (patternMode == 1 || patternMode == 2 || patternMode == 3);
  SMP_FLOW_MODE = (flowMode == 1);
  MIDI_VOICE_SELECT = (voiceSelect == 1 || voiceSelect == 2);
  MIDI_TRANSPORT_RECEIVE = (transportMode == 1);
  MIDI_TRANSPORT_SEND = (transportMode == 2);
  SMP_FAST_REC = fastRecMode;
  SMP_REC_CHANNEL_CLEAR = (recChannelClear == 1);


  extern bool songModeActive;
  songModeActive = (patternMode == 2);


  recInput = (recMode == 1) ? AUDIO_INPUT_MIC : AUDIO_INPUT_LINEIN;


  MIDI_CLOCK_SEND = (clockMode == 1);



  extern int midiSendMode;
  extern bool MIDI_NOTE_SEND;
  if (midiSendMode == 0) {

    MIDI_NOTE_SEND = false;
  } else if (midiSendMode == 1) {

    MIDI_CLOCK_SEND = false;
    MIDI_NOTE_SEND = true;
  } else {

    MIDI_NOTE_SEND = true;
  }
# 1428 "/Users/jank/git/toern/toern_menu.ino"
}


FLASHMEM void applyAudioSettingsFromGlobals() {

  extern struct GlobalVars GLOB;
  extern uint8_t lineOutLevelSetting;
  extern unsigned int micGain;
  extern unsigned int lineInLevel;



  float vol;
  if (GLOB.vol <= 10) {

    vol = GLOB.vol / 10.0f;
  } else {

    vol = GLOB.vol / 100.0f;
  }
  vol = constrain(vol, 0.0f, 1.0f);
  sgtl5000_1.volume(vol);


  sgtl5000_1.lineOutLevel(lineOutLevelSetting);


  extern unsigned int recInput;
  if (recInput == AUDIO_INPUT_MIC) {
    sgtl5000_1.micGain(micGain);
  } else {
    sgtl5000_1.micGain(0);
  }


  sgtl5000_1.lineInLevel(lineInLevel);

  extern void applySgtl5000AdcHighPass();
  applySgtl5000AdcHighPass();
  extern void applySgtl5000CodecOutputPath();
  applySgtl5000CodecOutputPath();


  updatePreviewVolume();

  extern void applyMixBusGains();
  applyMixBusGains();
}


FLASHMEM void saveSingleModeToEEPROM(int index, int8_t value) {
  extern bool isNowPlaying;
  if (index < 0 || index >= (int)SETTINGS_EEPROM_BLOCK_LEN) return;
  if (isNowPlaying) {
    pendingEepromSet(index, (uint8_t)value);
    markSettingsBackupDirty();
    return;
  }
  EEPROM.write(EEPROM_DATA_START + index, (uint8_t)value);
  markSettingsBackupDirty();
}


static inline void saveMixGainToEEPROM(int eepromIndex, uint8_t value, uint8_t pendingBit) {
  extern bool isNowPlaying;
  if (isNowPlaying) {
    g_pendingMixGainMask |= pendingBit;
    return;
  }
  saveSingleModeToEEPROM(eepromIndex, (int8_t)value);
}

static void saveTransportDelayToEEPROM(int slotBase, int8_t value) {
  saveSingleModeToEEPROM(slotBase, value);
}


FLASHMEM static void saveSettingsU16ToEEPROM(int index, uint16_t value) {
  extern bool isNowPlaying;
  if (index < 0 || index + 1 >= (int)SETTINGS_EEPROM_BLOCK_LEN) return;
  if (isNowPlaying) {
    pendingEepromSet(index, (uint8_t)(value & 0xFF));
    pendingEepromSet(index + 1, (uint8_t)((value >> 8) & 0xFF));
    markSettingsBackupDirty();
    return;
  }
  EEPROM.put(EEPROM_DATA_START + index, value);
  markSettingsBackupDirty();
}


FLASHMEM void saveSp0StateToEEPROM() {
  extern bool isNowPlaying;

  if (isNowPlaying) {
    g_pendingSp0Eeprom = true;
    markSettingsBackupDirty();
    return;
  }

  for (int i = 1; i < maxFiles; i++) {
    uint8_t value = SMP.sp0Active[i] ? 1 : 0;
    EEPROM.write(EEPROM_SP0_STATE_ADDR + i, value);
  }
  markSettingsBackupDirty();
}


FLASHMEM void loadSp0StateFromEEPROM() {
  for (int i = 1; i < maxFiles; i++) {
    uint8_t stored = EEPROM.read(EEPROM_SP0_STATE_ADDR + i);
    SMP.sp0Active[i] = (stored == 1);
  }
}

FLASHMEM void showMenu() {




  static int lastPagePosition = -1;
  static bool menuFirstEnter = true;


  if (currentMenuPage != currentMode->pos[3] && lastPagePosition != currentMenuPage) {
    lastPagePosition = -1;
    menuFirstEnter = true;
    currentMode->pos[3] = currentMenuPage;
  }

  if (menuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentMenuPage);
    lastPagePosition = currentMenuPage;
    menuFirstEnter = false;
  }

  ensureEnc3Range(0, (int)(MENU_PAGES_COUNT - 1));

  if (currentMode->pos[3] != lastPagePosition) {
    currentMenuPage = currentMode->pos[3];
    if (currentMenuPage >= MENU_PAGES_COUNT) currentMenuPage = MENU_PAGES_COUNT - 1;
    if (currentMenuPage < 0) currentMenuPage = 0;
    lastPagePosition = currentMenuPage;
  }


  const int pageIndex = currentMenuPage;
  MenuPage* currentPageInfo = &menuPages[pageIndex];
  const int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedPageIndex = -1;
  static int lastRenderedSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedPageIndex) || (mainSetting != lastRenderedSetting);

  if (fullRedraw) {
    FastLEDclear();

    drawMenuIndicator('L', 'G', 4);


    int startX = (int)maxX - MENU_PAGES_COUNT + 1;
    if (startX < 1) startX = 1;
    for (int i = 0; i < MENU_PAGES_COUNT; i++) {
      CRGB indicatorColor = (i == pageIndex) ? UI_RED : UI_BLUE;
      light(startX + i, maxY, indicatorColor);
    }


    if (mainSetting == 15) {

      CRGB greenColor = getIndicatorColor('G');
      CRGB yellowColor = getIndicatorColor('Y');
      CRGB whiteColor = getIndicatorColor('W');
      CRGB blueColor = getIndicatorColor('X');
      Encoder[0].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
      Encoder[1].writeRGBCode(yellowColor.r << 16 | yellowColor.g << 8 | yellowColor.b);
      Encoder[2].writeRGBCode(whiteColor.r << 16 | whiteColor.g << 8 | whiteColor.b);
      Encoder[3].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
    } else {

      CRGB menuNavColor = getIndicatorColor('G');
      Encoder[0].writeRGBCode(0x000000);
      Encoder[1].writeRGBCode(0x000000);
      Encoder[2].writeRGBCode(0x000000);
      Encoder[3].writeRGBCode(menuNavColor.r << 16 | menuNavColor.g << 8 | menuNavColor.b);
    }

    drawMainSettingStatus(mainSetting);
    if (currentPageInfo->hasAdditionalFeatures) {
      drawAdditionalFeatures(mainSetting);
    }

    lastRenderedPageIndex = pageIndex;
    lastRenderedSetting = mainSetting;
  }


  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (fullRedraw || didRedraw) {

    FastLEDshow();
  }
}

FLASHMEM void showLookMenu() {
  static uint32_t lastPosHash = 0;

  int pageIndex = currentLookPage;
  MenuPage* currentPageInfo = &lookPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedLookPage = -1;
  static int lastRenderedLookSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedLookPage) || (mainSetting != lastRenderedLookSetting);

  if (fullRedraw) {
    FastLEDclear();


  drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);






  const CRGB parent = currentMenuParentTextColor();
  const CRGB parentDim = dimIconColorFromText(parent);
  int startX = (int)maxX - LOOK_PAGES_COUNT + 1;
  if (startX < 1) startX = 1;
  for (int i = 0; i < LOOK_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
    light(startX + i, maxY, indicatorColor);
  }





  CRGB indicatorColor = currentMenuParentTextColor();
  const bool playValuePage = (mainSetting == 9 || mainSetting == 10 || mainSetting == 17 ||
      mainSetting == 18 || mainSetting == 23 || mainSetting == 24 || mainSetting == 25 ||
      mainSetting == 32 || mainSetting == 33 || mainSetting == 38 || mainSetting == 47 ||
      mainSetting == 54);
  Encoder[0].writeRGBCode(0x000000);
  Encoder[1].writeRGBCode(0x000000);
  Encoder[2].writeRGBCode(playValuePage ? (indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
  Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);


  drawMainSettingStatus(mainSetting);


  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }


  FastLEDshow();

  lastRenderedLookPage = pageIndex;
  lastRenderedLookSetting = mainSetting;
  }


  static int lastLookPagePosition = -1;
  static bool lookMenuFirstEnter = true;


  if (currentLookPage == 0 && currentMode->pos[3] != 0 && lastLookPagePosition != 0) {
    lastLookPagePosition = -1;
    lookMenuFirstEnter = true;
  }

  if (lookMenuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentLookPage);
    lastLookPagePosition = currentLookPage;
    lookMenuFirstEnter = false;
  }

  ensureEnc3Range(0, (int)(LOOK_PAGES_COUNT - 1));

  if (currentMode->pos[3] != lastLookPagePosition) {
    currentLookPage = currentMode->pos[3];
    if (currentLookPage >= LOOK_PAGES_COUNT) currentLookPage = LOOK_PAGES_COUNT - 1;
    if (currentLookPage < 0) currentLookPage = 0;
    lastLookPagePosition = currentLookPage;
  }


  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {

    FastLEDshow();
  }



  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    if (!didRedraw) menuRequestFullRedraw();
    lastPosHash = h;
  }
}

FLASHMEM void showRecsMenu() {
  static uint32_t lastPosHash = 0;

  int pageIndex = currentRecsPage;
  MenuPage* currentPageInfo = &recsPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedRecsPage = -1;
  static int lastRenderedRecsSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedRecsPage) || (mainSetting != lastRenderedRecsSetting);

  if (fullRedraw) {
    FastLEDclear();


  drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);





  if (currentPageInfo->mainSetting == 4) {
    drawText("INPT", 2, 10, menuTextColorFromCol(7));
  }



  const CRGB parent = currentMenuParentTextColor();
  const CRGB parentDim = dimIconColorFromText(parent);
  int startX = (int)maxX - RECS_PAGES_COUNT + 1;
  if (startX < 1) startX = 1;
  for (int i = 0; i < RECS_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
    light(startX + i, maxY, indicatorColor);
  }





  if (mainSetting == 11 || mainSetting == 4 || mainSetting == 12 || mainSetting == 64) {

    CRGB indicatorColor = currentMenuParentTextColor();
    Encoder[0].writeRGBCode(0x000000);
    Encoder[1].writeRGBCode(0x000000);
    Encoder[2].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  } else {

    CRGB indicatorColor = currentMenuParentTextColor();
    Encoder[0].writeRGBCode(0x000000);
    Encoder[1].writeRGBCode(0x000000);
    Encoder[2].writeRGBCode(0x000000);
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  }


  drawMainSettingStatus(mainSetting);


  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }


  FastLEDshow();

  lastRenderedRecsPage = pageIndex;
  lastRenderedRecsSetting = mainSetting;
  }


  static int lastRecsPagePosition = -1;
  static bool recsMenuFirstEnter = true;


  if (currentRecsPage == 0 && currentMode->pos[3] != 0 && lastRecsPagePosition != 0) {
    lastRecsPagePosition = -1;
    recsMenuFirstEnter = true;
  }

  if (recsMenuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentRecsPage);
    lastRecsPagePosition = currentRecsPage;
    recsMenuFirstEnter = false;
  }

  ensureEnc3Range(0, (int)(RECS_PAGES_COUNT - 1));

  if (currentMode->pos[3] != lastRecsPagePosition) {
    currentRecsPage = currentMode->pos[3];
    if (currentRecsPage >= RECS_PAGES_COUNT) currentRecsPage = RECS_PAGES_COUNT - 1;
    if (currentRecsPage < 0) currentRecsPage = 0;
    lastRecsPagePosition = currentRecsPage;
  }


  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {

    FastLEDshow();
  }



  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    if (!didRedraw) menuRequestFullRedraw();
    lastPosHash = h;
  }
}

FLASHMEM void showMidiMenu() {
  static uint32_t lastPosHash = 0;

  int pageIndex = currentMidiPage;
  MenuPage* currentPageInfo = &midiPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedMidiPage = -1;
  static int lastRenderedMidiSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedMidiPage) || (mainSetting != lastRenderedMidiSetting);

  if (fullRedraw) {
    FastLEDclear();


  drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);






  const CRGB parent = currentMenuParentTextColor();
  const CRGB parentDim = dimIconColorFromText(parent);
  int startX = (int)maxX - MIDI_PAGES_COUNT + 1;
  if (startX < 1) startX = 1;
  for (int i = 0; i < MIDI_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
    light(startX + i, maxY, indicatorColor);
  }






  CRGB indicatorColor = currentMenuParentTextColor();
  const bool midiValuePage = (mainSetting == 7 || mainSetting == 8 || mainSetting == 13 || mainSetting == 44 || mainSetting == 51 || mainSetting == 45 || mainSetting == 50 || mainSetting == 58 || mainSetting == 59 || mainSetting == 60);
  Encoder[0].writeRGBCode(0x000000);
  if (mainSetting == 50) {
    extern bool getPulseClockPolarityPositive();
    extern bool getPulseClockStopWithPlay();
    const bool polPos = getPulseClockPolarityPositive();
    const bool stopPlay = getPulseClockStopWithPlay();
    Encoder[0].writeRGBCode(0xFFFFFF);
    Encoder[1].writeRGBCode(stopPlay ? 0xFF0000 : 0x00FF00);
    Encoder[2].writeRGBCode(polPos ? 0x00FF00 : 0xFF0000);
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  } else if (mainSetting == 59) {
    Encoder[0].writeRGBCode(0xFFFFFF);
    Encoder[1].writeRGBCode(0x000000);
    Encoder[2].writeRGBCode(0xFFFFFF);
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  } else {
    Encoder[1].writeRGBCode(mainSetting == 45 ? (indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
    Encoder[2].writeRGBCode(midiValuePage ? (indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  }


  drawMainSettingStatus(mainSetting);


  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }


  FastLEDshow();

  lastRenderedMidiPage = pageIndex;
  lastRenderedMidiSetting = mainSetting;
  }


  static int lastMidiPagePosition = -1;
  static bool midiMenuFirstEnter = true;


  if (currentMidiPage == 0 && currentMode->pos[3] != 0 && lastMidiPagePosition != 0) {
    lastMidiPagePosition = -1;
    midiMenuFirstEnter = true;
  }

  if (midiMenuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentMidiPage);
    lastMidiPagePosition = currentMidiPage;
    midiMenuFirstEnter = false;
  }

  ensureEnc3Range(0, (int)(MIDI_PAGES_COUNT - 1));

  if (currentMode->pos[3] != lastMidiPagePosition) {
    currentMidiPage = currentMode->pos[3];
    if (currentMidiPage >= MIDI_PAGES_COUNT) currentMidiPage = MIDI_PAGES_COUNT - 1;
    if (currentMidiPage < 0) currentMidiPage = 0;
    lastMidiPagePosition = currentMidiPage;
  }


  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {

    FastLEDshow();
  }



  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    if (!didRedraw) menuRequestFullRedraw();
    lastPosHash = h;
  }
}

static bool previewModeLabelVisible = false;
static unsigned long previewModeLabelChangedAt = 0;
static constexpr unsigned long PREVIEW_MODE_LABEL_MS = 2500;

FLASHMEM void showVolMenu() {
  static uint32_t lastPosHash = 0;

  int pageIndex = currentVolPage;
  MenuPage* currentPageInfo = &volPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedVolPage = -1;
  static int lastRenderedVolSetting = -1;
  bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedVolPage) || (mainSetting != lastRenderedVolSetting);
  if (mainSetting == 29 && previewModeLabelVisible &&
      millis() - previewModeLabelChangedAt >= PREVIEW_MODE_LABEL_MS) {
    previewModeLabelVisible = false;
    fullRedraw = true;
  }
  if (mainSetting != 29 || pageIndex != lastRenderedVolPage || mainSetting != lastRenderedVolSetting) {
    previewModeLabelVisible = false;
  }

  if (fullRedraw) {
    FastLEDclear();


  drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);






  const CRGB parent = currentMenuParentTextColor();
  const CRGB parentDim = dimIconColorFromText(parent);
  int startX = (int)maxX - VOL_PAGES_COUNT + 1;
  if (startX < 1) startX = 1;
  for (int i = 0; i < VOL_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
    light(startX + i, maxY, indicatorColor);
  }





  CRGB indicatorColor = currentMenuParentTextColor();
  const bool volGainPage = (mainSetting == 53);
  const bool volValuePage = (mainSetting == 43);
  Encoder[0].writeRGBCode(volGainPage ? 0x00C8C8 : 0x000000);
  Encoder[1].writeRGBCode(volGainPage ? 0xC86400 : 0x000000);
  if (volGainPage) {
    Encoder[2].writeRGBCode(g_gainEnc3IsAll ? 0x00FF00 : 0xC828C8);
  } else {
    Encoder[2].writeRGBCode(volValuePage ? (indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
  }
  Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);


  drawMainSettingStatus(mainSetting);


  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }


  FastLEDshow();

  lastRenderedVolPage = pageIndex;
  lastRenderedVolSetting = mainSetting;
  }


  static int lastVolPagePosition = -1;
  static bool volMenuFirstEnter = true;


  if (currentVolPage == 0 && currentMode->pos[3] != 0 && lastVolPagePosition != 0) {
    lastVolPagePosition = -1;
    volMenuFirstEnter = true;
  }

  if (volMenuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentVolPage);
    lastVolPagePosition = currentVolPage;
    volMenuFirstEnter = false;
  }

  ensureEnc3Range(0, (int)(VOL_PAGES_COUNT - 1));

  if (currentMode->pos[3] != lastVolPagePosition) {
    currentVolPage = currentMode->pos[3];
    if (currentVolPage >= VOL_PAGES_COUNT) currentVolPage = VOL_PAGES_COUNT - 1;
    if (currentVolPage < 0) currentVolPage = 0;
    lastVolPagePosition = currentVolPage;
  }


  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {

    FastLEDshow();
  }



  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    if (!didRedraw) menuRequestFullRedraw();
    lastPosHash = h;
  }
}

FLASHMEM void showEtcMenu() {
  static uint32_t lastPosHash = 0;

  const int pageCount = activeSystemPageCount();
  int pageIndex = constrain(currentEtcPage, 0, pageCount - 1);
  MenuPage* currentPageInfo = &activeSystemPages()[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedEtcPage = -1;
  static int lastRenderedEtcSetting = -1;
  bool fullRedraw = (pageIndex != lastRenderedEtcPage) || (mainSetting != lastRenderedEtcSetting);
  if (takeMenuForceFullRedraw()) fullRedraw = true;
  if (mainSetting == 39) fullRedraw = true;

  if (mainSetting == 52) {
    static elapsedMillis ramMenuRedraw;
    if (ramMenuRedraw >= 200) {
      ramMenuRedraw = 0;
      fullRedraw = true;
    }
  }
  if (mainSetting == 49) {

    extern bool sdSerialServerIsActive();
    extern bool sdSerialServerClientConnected();
    static bool lastSdActive = false;
    static bool lastSdClient = false;
    bool nowActive = sdSerialServerIsActive();
    bool nowClient = sdSerialServerClientConnected();
    if (nowActive != lastSdActive || nowClient != lastSdClient) {
      lastSdActive = nowActive;
      lastSdClient = nowClient;
      fullRedraw = true;
    }
  }

  if (mainSetting == 42) {
    static elapsedMillis battMenuRedraw;
    if (battMenuRedraw >= 200) {
      battMenuRedraw = 0;
      fullRedraw = true;
    }
  }

  if (fullRedraw) {
    FastLEDclear();



    const CRGB parent = currentMenuParentTextColor();
    const CRGB parentDim = dimIconColorFromText(parent);
    int startX = (int)maxX - pageCount + 1;
    if (startX < 1) startX = 1;
    for (int i = 0; i < pageCount; i++) {
      CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
      light(startX + i, maxY, indicatorColor);
    }






    static uint32_t lastEtcRGB[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
    static int lastEtcRgbPage = -1;
    if (pageIndex != lastEtcRgbPage) {
      for (int i = 0; i < 4; i++) lastEtcRGB[i] = 0xFFFFFFFFu;
      lastEtcRgbPage = pageIndex;
    }
    auto setEtcRGB = [&](int enc, uint32_t rgb) {
      if (enc < 0 || enc > 3) return;
      if (lastEtcRGB[enc] == rgb) return;
      lastEtcRGB[enc] = rgb;
      Encoder[enc].writeRGBCode(rgb);
    };
    if (mainSetting == 15) {
      drawMenuIndicator('L', 'G', 1, false, false);

      setEtcRGB(0, 0x00FF00);
      CRGB y = getIndicatorColor('Y');
      CRGB w = getIndicatorColor('W');
      setEtcRGB(1, (uint32_t)y.r << 16 | (uint32_t)y.g << 8 | y.b);
      setEtcRGB(2, (uint32_t)w.r << 16 | (uint32_t)w.g << 8 | w.b);

      {
        CRGB c = currentMenuParentTextColor();
        drawLargeIndicatorCustom(c, 4);

        lastEtcRGB[3] = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
      }
    } else {

      CRGB indicatorColor = currentMenuParentTextColor();
      const bool etcValuePage =
          (mainSetting == 9 || mainSetting == 10 || mainSetting == 18 || mainSetting == 23 ||
           mainSetting == 24 || mainSetting == 32 || mainSetting == 36 || mainSetting == 46 || mainSetting == 54 || mainSetting == 63 || mainSetting == 16 || mainSetting == 40 || mainSetting == 41 || mainSetting == 48 ||
           mainSetting == 52 || mainSetting == 55 || mainSetting == 56 ||
           mainSetting == 57 || mainSetting == 65);

      {
        extern void getIndicatorXPositions(int encoderNum, int &x1, int &x2, int &x3);
        extern int ledModules;
        int x1, x2, x3;
        getIndicatorXPositions(4, x1, x2, x3);
        const bool isWide = (ledModules == 2 && maxX > 16);
        const int x4 = isWide ? min((int)maxX, x3 + 1) : x3;
        light(x1, 1, indicatorColor);
        light(x2, 1, indicatorColor);
        light(x3, 1, indicatorColor);
        if (isWide) light(x4, 1, indicatorColor);
      }
      setEtcRGB(0, 0x000000);
      setEtcRGB(1, 0x000000);
      setEtcRGB(2, etcValuePage ? ((uint32_t)indicatorColor.r << 16 | (uint32_t)indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
      setEtcRGB(3, (uint32_t)indicatorColor.r << 16 | (uint32_t)indicatorColor.g << 8 | indicatorColor.b);
    }

    drawMainSettingStatus(mainSetting);
    if (currentPageInfo->hasAdditionalFeatures) {
      drawAdditionalFeatures(mainSetting);
    }


    FastLEDshow();

    lastRenderedEtcPage = pageIndex;
    lastRenderedEtcSetting = mainSetting;
  }


  static int lastEtcPagePosition = -1;
  static bool etcMenuFirstEnter = true;


  if (currentEtcPage == 0 && currentMode->pos[3] != 0 && lastEtcPagePosition != 0) {
    lastEtcPagePosition = -1;
    etcMenuFirstEnter = true;
  }

  if (etcMenuFirstEnter) {
    Encoder[3].writeCounter((int32_t)currentEtcPage);
    lastEtcPagePosition = currentEtcPage;
    etcMenuFirstEnter = false;
  }

  ensureEnc3Range(0, (int)(pageCount - 1));
  if (currentMode->pos[3] != lastEtcPagePosition) {
    currentEtcPage = currentMode->pos[3];
    if (currentEtcPage >= pageCount) currentEtcPage = pageCount - 1;
    if (currentEtcPage < 0) currentEtcPage = 0;
    lastEtcPagePosition = currentEtcPage;
  }

  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {

    FastLEDshow();
  }



  if (mainSetting != 39) {
    uint32_t h = hashEncoderPositions(currentMode);
    if (h != lastPosHash) {
      if (!didRedraw) menuRequestFullRedraw();
      lastPosHash = h;
    }
  }
}

static inline CRGB dimIconColorFromText(CRGB textColor) {

  CRGB c = textColor;
  uint8_t maxv = max(c.r, max(c.g, c.b));
  if (maxv == 0) return c;
  const uint8_t target = 20;
  uint16_t scale = (uint16_t)target * 255u / maxv;
  if (scale > 255) scale = 255;
  c.nscale8_video((uint8_t)scale);
  return c;
}

static inline CRGB menuTextColorFromCol(uint8_t colIndex) {


  extern CRGB col[];
  CRGB c = col[colIndex];
  uint8_t maxv = max(c.r, max(c.g, c.b));
  if (maxv == 0) return c;
  const uint8_t target = 120;
  uint16_t scale = (uint16_t)target * 255u / maxv;
  if (scale > 255) scale = 255;
  c.nscale8_video((uint8_t)scale);
  return c;
}

static inline CRGB currentMenuParentTextColor() {


  if (inLookSubmenu) return menuTextColorFromCol(6);
  if (inRecsSubmenu) return menuTextColorFromCol(7);
  if (inMidiSubmenu) return menuTextColorFromCol(8);
  if (inVolSubmenu) return menuTextColorFromCol(5);
  if (inEtcSubmenu) return menuTextColorFromCol(currentSystemMenuGroup == SystemMenuGroup::Mode ? 13 : 14);
  return UI_WHITE;
}




static inline void drawLargeIndicatorCustom(CRGB color, int encoderNum) {
  extern void getIndicatorXPositions(int encoderNum, int &x1, int &x2, int &x3);
  extern int ledModules;
  extern unsigned int maxX;

  int x1, x2, x3;
  getIndicatorXPositions(encoderNum, x1, x2, x3);
  const bool isWide = (ledModules == 2 && maxX > 16);
  const int x4 = isWide ? min((int)maxX, x3 + 1) : x3;

  light(x1, 1, color);
  light(x2, 1, color);
  light(x3, 1, color);
  if (isWide) {
    light(x4, 1, color);
  }

  if (encoderNum >= 1 && encoderNum <= NUM_ENCODERS) {
    uint32_t rgbCode = (uint32_t(color.r) << 16) | (uint32_t(color.g) << 8) | (uint32_t)color.b;
    Encoder[encoderNum - 1].writeRGBCode(rgbCode);
  }
}

static inline uint32_t hashEncoderPositions(const Mode *m) {

  uint32_t h = 2166136261u;
  for (int i = 0; i < NUM_ENCODERS; i++) {
    h ^= (uint32_t)m->pos[i];
    h *= 16777619u;
  }
  return h;
}

FLASHMEM void drawMainSettingStatus(int setting) {
  switch (setting) {
    case 1:

      {
        const CRGB tc = menuTextColorFromCol(1);
        showIcons(ICON_FOLDER_BIG, dimIconColorFromText(tc));
        drawMenuText("PATT", 2, 3, tc);
      }
      break;

    case 2:

      {
        const CRGB tc = menuTextColorFromCol(2);
        showIcons(OLD_ICON_SAMPLEPACK, dimIconColorFromText(tc));
        drawMenuText("KIT", 2, 3, tc);
      }
      break;

    case 3:
      {
        const CRGB tc = menuTextColorFromCol(3);
        if (GLOB.currentChannel >= 1 && GLOB.currentChannel <= 8) {
          showIcons(ICON_SAMPLE_BIG, dimIconColorFromText(tc));
          drawMenuText("SMP", 2, 3, tc);
        } else if (GLOB.currentChannel == 11 || GLOB.currentChannel == 13 || GLOB.currentChannel == 14) {
          showIcons(ICON_FOLDER_BIG, dimIconColorFromText(tc));
          drawMenuText("PSET", 2, 3, tc);
        } else {
          showIcons(ICON_SAMPLE_BIG, dimIconColorFromText(tc));
          drawMenuText("(-)", 2, 3, tc);
        }
      }
      break;

    case 4:
      drawMenuText("INPT", 2, 10, currentMenuParentTextColor());
      drawRecMode();
      drawMenuIndicator('L', recMode == 1 ? 'W' : 'X', 3);
      break;

    case 5:

      {
        const CRGB tc = menuTextColorFromCol(4);
        showIcons(ICON_CLOCK, dimIconColorFromText(tc));
        drawMenuText("BPM", 2, 3, tc);
      }
      break;

    case 7:
      drawMenuText("CH", 2, 10, currentMenuParentTextColor());
      drawMidiVoiceSelect();
      drawMenuIndicator('L', voiceSelect == 1 ? 'X' : (voiceSelect == 2 ? 'M' : 'G'), 3);
      break;

    case 8:
      drawMenuText("TRAN", 2, 10, currentMenuParentTextColor());
      drawMidiTransport();
      drawMenuIndicator('L', transportMode == 0 ? 'R' : (transportMode == 1 ? 'G' : 'X'), 3);
      break;

    case 13:
      drawMenuText("SEND", 2, 10, currentMenuParentTextColor());
      drawMidiSend();
      {
        extern int midiSendMode;
        drawMenuIndicator('L', midiSendMode == 0 ? 'Y' : (midiSendMode == 1 ? 'G' : 'X'), 3);
      }
      break;

    case 44:
      {
        extern bool MIDI_NOTE_RECEIVE;
        drawMenuText("RCVE", 2, 10, currentMenuParentTextColor());
        drawMenuValue(MIDI_NOTE_RECEIVE ? "NOTE" : "OFF", 2, 3, MIDI_NOTE_RECEIVE ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', MIDI_NOTE_RECEIVE ? 'G' : 'R', 3);
      }
      break;

    case 60: {
        extern bool MIDI_WRITE_OVERWRITE;
        drawMenuText("OVWR", 2, 10, currentMenuParentTextColor());
        drawMenuValue(MIDI_WRITE_OVERWRITE ? "ON" : "OFF", 2, 3, MIDI_WRITE_OVERWRITE ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', MIDI_WRITE_OVERWRITE ? 'G' : 'R', 3);
      }
      break;

    case 51:
      {
        extern bool MIDI_NOTE_CLAMP;
        drawMenuText("FOLD", 2, 10, currentMenuParentTextColor());
        drawMenuValue(MIDI_NOTE_CLAMP ? "ON" : "OFF", 2, 3, MIDI_NOTE_CLAMP ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', MIDI_NOTE_CLAMP ? 'G' : 'R', 3);
      }
      break;

    case 45:
      {
        extern int8_t transportSendDelayMs;
        extern int8_t transportRcveDelayMs;
        const CRGB tc = currentMenuParentTextColor();
        int syncEdit = (currentMode->pos[1] == 0) ? 0 : 1;
        drawMenuText(syncEdit == 0 ? "SNC1" : "SNC2", 2, 10, tc);
        int val = (syncEdit == 0) ? (int)transportSendDelayMs : (int)transportRcveDelayMs;
        char buf[10];
        snprintf(buf, sizeof(buf), "%d", val);
        drawMenuValue(buf, 2, 3, syncEdit == 0 ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', 'Y', 1);
        drawMenuIndicator('L', 'W', 2);
      }
      break;

    case 50: {
      extern bool getPulseClockEnabled();
      extern bool getPulseClockPolarityPositive();
      extern bool getPulseClockStopWithPlay();
      extern uint8_t getPulseClockPpqn();
      extern uint8_t getPulseClockWidthMs();
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("PPQN", 2, 10, tc);
      bool on = getPulseClockEnabled();
      bool polPos = getPulseClockPolarityPositive();
      bool stopPlay = getPulseClockStopWithPlay();
      if (g_ppqnUiFocus == 2) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", (unsigned)getPulseClockWidthMs());
        drawMenuValue(buf, 2, 3, UI_WHITE);
      } else if (g_ppqnUiFocus == 1) {
        drawMenuValue(stopPlay ? "STOP" : "CONT", 2, 3, stopPlay ? UI_RED : UI_GREEN);
      } else if (!on) {
        drawMenuValue("OFF", 2, 3, UI_RED);
      } else {
        char buf[8];
        snprintf(buf, sizeof(buf), "%c%u", polPos ? '+' : '-', (unsigned)getPulseClockPpqn());
        drawMenuValue(buf, 2, 3, polPos ? UI_GREEN : UI_RED);
      }
      drawMenuIndicator('L', 'W', 1);
      drawMenuIndicator('L', stopPlay ? 'R' : 'G', 2);
      drawMenuIndicator('L', polPos ? 'G' : 'R', 3);
      break;
    }

    case 58: {
      extern uint8_t getSidechainVoice();
      extern CRGB col[];
      const uint8_t ch = getSidechainVoice();
      drawMenuText("TRIG", 2, 10, currentMenuParentTextColor());
      if (!ch) {
        drawMenuValue("OFF", 2, 3, UI_RED);
        drawMenuIndicator('L', 'R', 3);
      } else {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", (unsigned)ch);
        drawMenuValue(buf, 2, 3, col[ch]);
        drawMenuIndicator('L', 'G', 3);
      }
      break;
    }

    case 59: {
      extern uint8_t getAnalogInPpqn();
      extern uint8_t getAnalogInHoldMs();
      const CRGB tc = currentMenuParentTextColor();
      char buf[8];
      if (g_cinFocus == 1) {
        drawMenuText("WID", 2, 10, tc);
        snprintf(buf, sizeof(buf), "%u", (unsigned)getAnalogInHoldMs());
        drawMenuValue(buf, 2, 3, UI_WHITE);
      } else {
        drawMenuText("CIN", 2, 10, tc);
        snprintf(buf, sizeof(buf), "%u", (unsigned)getAnalogInPpqn());
        drawMenuValue(buf, 2, 3, UI_WHITE);
      }
      drawMenuIndicator('L', 'W', 1);
      drawMenuIndicator('L', 'W', 3);
      break;
    }

    case 9:
      drawMenuText("PMODE", 2, 10, currentMenuParentTextColor());
      drawPatternMode();
      drawMenuIndicator('L', patternMode == -1 ? 'R' : (patternMode == 1 ? 'G' : (patternMode == 2 ? 'Y' : 'N')), 3);
      break;

    case 10:
      drawMenuText("FLOW", 2, 10, currentMenuParentTextColor());
      drawFlowMode();
      drawMenuIndicator('L', flowMode == 1 ? 'G' : 'R', 3);
      break;

    case 11:
      drawMenuText("TRIG", 2, 10, currentMenuParentTextColor());
      drawFastRecMode();
      drawMenuIndicator('L', fastRecMode == 0 ? 'R' : (fastRecMode == 1 ? 'G' : (fastRecMode == 2 ? 'N' : (fastRecMode == 3 ? 'Y' : 'X'))), 3);
      break;

    case 12:
      drawMenuText("CLR", 2, 10, currentMenuParentTextColor());
      drawRecChannelClear();
      drawMenuIndicator('L', imageMode ? 'Y' : (recChannelClear == 0 ? 'R' : (recChannelClear == 1 ? 'G' : (recChannelClear == 2 ? 'Y' : (recChannelClear == 3 ? 'N' : 'M')))), 3);
      break;

    case 15:
      drawMenuText("AUTO", 2, 10, currentMenuParentTextColor());

      drawMenuIndicator('L', 'G', 1);
      drawMenuIndicator('L', 'Y', 2);
      drawMenuIndicator('L', 'W', 3);
      break;

    case 16:
      drawMenuText("RSET", 2, 10, currentMenuParentTextColor());





      drawMenuIndicator('L', 'G', 2);
      drawMenuIndicator('L', 'O', 3);

      break;

    case 39:
      drawEtcInfoPage();
      break;

    case 52:
      drawEtcRamPage();
      break;

    case 49:
      {
        extern bool sdSerialServerClientConnected();
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("SD", 2, 10, tc);
        bool connected = sdSerialServerClientConnected();

        drawMenuValue(connected ? "OK" : "WAIT", 2, 3, connected ? UI_GREEN : UI_YELLOW);
      }
      break;

    case 40:
      {
        extern bool getLedStripEnabled();
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("LGHT", 2, 10, tc);
        bool enabled = getLedStripEnabled();
        drawMenuValue(enabled ? "ON" : "OFF", 2, 3, enabled ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', enabled ? 'G' : 'R', 3);
      }
      break;

    case 41:
      {
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("COLR", 2, 10, tc);
        char schemeText[2];
        snprintf(schemeText, sizeof(schemeText), "%d", currentColorScheme);
        drawMenuValue(schemeText, 2, 3, UI_GREEN);
        drawMenuIndicator('L', 'G', 3);
      }
      break;

    case 42:
      {
        extern int getBatteryPercent();
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("BATT", 2, 10, tc);

        int pct = getBatteryPercent();
        pct = constrain(pct, 0, 100);

        char pctText[6];
        snprintf(pctText, sizeof(pctText), "%d%%", pct);


        CRGB stateColor = (pct > 20) ? UI_GREEN
                                     : (pct > BATT_WARN_PCT) ? CRGB(255, 165, 0)
                                                             : UI_RED;
        drawMenuText(pctText, 2, 3, stateColor);
      }
      break;

    case 48:
      {
        extern bool getChildLockEnabled();
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("CHLD", 2, 10, tc);
        bool enabled = getChildLockEnabled();
        drawMenuValue(enabled ? "ON" : "OFF", 2, 3, enabled ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', enabled ? 'G' : 'R', 3);
      }
      break;

    case 56: {
      extern bool imageMode;
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("IMG", 2, 10, tc);
      drawMenuValue(imageMode ? "ON" : "OFF", 2, 3, imageMode ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', imageMode ? 'G' : 'R', 3);
      break;
    }

    case 57: {
      extern uint8_t eyesMode;
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("EYES", 2, 10, tc);
      const char *label = "OFF";
      CRGB vc = UI_RED;
      if (eyesMode == 1) { label = "1MIN"; vc = UI_GREEN; }
      else if (eyesMode == 2) { label = "5MIN"; vc = UI_GREEN; }
      else if (eyesMode == 3) { label = "15MIN"; vc = UI_GREEN; }
      drawMenuValue(label, 2, 3, vc);
      drawMenuIndicator('L', eyesMode == 0 ? 'R' : 'G', 3);
      break;
    }

    case 55:
      {
        extern uint8_t fireVoice;
        extern uint8_t fireLevel;
        extern uint8_t fireSize;
        extern uint8_t fireGravity;
        extern uint8_t fireColor;
        extern uint8_t fireFocus;
        extern CRGB col[];
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("FIRE", 2, 10, tc);
        clearTextArea(1, 3, (int)maxX);
        char num[4];
        snprintf(num, sizeof(num), "%u", fireSize);
        drawMenuText(num, 1, 3, CRGB(255, 140, 20));
        snprintf(num, sizeof(num), "%u", fireLevel);
        drawMenuText(num, (int)maxX >= 24 ? 8 : 5, 3, CRGB(255, 180, 60));
        const bool wide = (int)maxX >= 24;
        int voiceX = wide ? 26 : 12;
        if (wide) {
          snprintf(num, sizeof(num), "%u", fireGravity);
          drawMenuText(num, 14, 3, fireFocus == 1 ? CRGB(220, 220, 220) : CRGB(70, 70, 70));
          snprintf(num, sizeof(num), "%u", fireColor);
          drawMenuText(num, 20, 3, fireFocus == 2 ? CRGB(255, 220, 120) : CRGB(80, 70, 30));
        }
        if (!wide && fireFocus != 0) {
          snprintf(num, sizeof(num), "%u", fireFocus == 1 ? fireGravity : fireColor);
          drawMenuText(num, 12, 3, fireFocus == 1 ? CRGB(220, 220, 220) : CRGB(255, 220, 120));
        } else if (!fireVoice) {
          drawMenuText(wide ? "OFF" : "-", voiceX, 3, UI_RED);
        } else if (fireVoice == 15) {
          drawMenuText(wide ? "ALL" : "A", voiceX, 3, CRGB(255, 220, 120));
        } else {
          snprintf(num, sizeof(num), "%u", fireVoice);
          int vx = voiceX;
          if (!wide && fireVoice >= 10) vx = 10;
          drawMenuText(num, vx, 3, col[fireVoice]);
        }
        int half = (int)maxX / 2;
        if (half < 1) half = 1;
        int gDots = (fireGravity * half) / 8;
        int cDots = (fireColor * ((int)maxX - half)) / 8;
        for (int x = 1; x <= (int)maxX; x++) {
          CRGB bar = CRGB(0, 0, 0);
          if (x <= half && x <= gDots) {
            bar = CRGB(180, 180, 180);
            if (fireFocus == 1) bar = CRGB(255, 255, 255);
          } else if (x > half && (x - half) <= cDots) {
            bar = CRGB(140, 90, 20);
            if (fireFocus == 2) bar = CRGB(255, 200, 60);
          }
          light(x, 8, bar);
        }
        drawMenuIndicator('L', 'O', 1);
        drawMenuIndicator('L', 'O', 2);
        char focusCode = fireVoice ? 'G' : 'R';
        if (fireFocus == 1) focusCode = 'W';
        else if (fireFocus == 2) focusCode = 'Y';
        drawMenuIndicator('L', focusCode, 3);
      }
      break;

    case 19:

      {
        const CRGB tc = menuTextColorFromCol(6);
        showIcons(ICON_SETTINGS_BIG, dimIconColorFromText(tc));
        drawMenuText("SETT", 2, 3, tc);
      }
      break;

    case 20:

      {
        const CRGB tc = menuTextColorFromCol(7);
        showIcons(ICON_RECORD, dimIconColorFromText(tc));
        drawMenuText("REC", 2, 3, tc);
      }
      break;

    case 21:

      {
        const CRGB tc = menuTextColorFromCol(8);
        showIcons(ICON_MIDISYNC, dimIconColorFromText(tc));
        drawMenuText("MIDI", 2, 3, tc);
      }
      break;

    case 22:
      {
        const CRGB tc = menuTextColorFromCol(13);
        drawMenuText("SONG", 2, 10, tc);
      }
      break;

    case 17:
      drawMenuText("VIEW", 2, 10, currentMenuParentTextColor());
      drawSimpleNotesView();
      drawMenuIndicator('L', simpleNotesView == 1 ? 'G' : 'X', 3);
      break;

    case 18:
      drawMenuText("LOOP", 2, 10, currentMenuParentTextColor());
      drawLoopLength();
      drawMenuIndicator('L', loopLength == 0 ? 'R' : 'N', 3);
      break;

    case 54: {
      extern bool voiceMode;
      extern bool imageMode;
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("VMOD", 2, 10, tc);

      const bool on = voiceMode && !imageMode;
      drawMenuValue(on ? "ON" : "OFF", 2, 3, on ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', on ? 'G' : 'R', 3);
      break;
    }

    case 23:
      drawMenuText("LEDS", 2, 10, currentMenuParentTextColor());
      drawLedModules();
      drawMenuIndicator('L', 'G', 3);
      break;

    case 24:
      drawMenuText("PONG", 2, 10, currentMenuParentTextColor());
      clearTextArea(2, 3, 8);
      clearTextArea(6, 3, 8);
      clearTextArea(10, 3, 8);
      if (pong) {
        drawMenuText("ON", 2, 3, UI_GREEN);
      } else {
        drawMenuValue("OFF", 2, 3, UI_RED);
      }
      if (pong) {
        char speedText[6];
        int speedValue = pongIntervalToSpeed(pongUpdateInterval);
        snprintf(speedText, sizeof(speedText), "%02d", speedValue);
        drawMenuText(speedText, 10, 3, CRGB(0, 150, 255));
        {
          CRGB indicatorColor = getIndicatorColor('B');
          Encoder[2].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
        }
        drawMenuIndicator('L', 'B', 3);
      } else {
        Encoder[2].writeRGBCode(0x000000);
      }
      break;

    case 63: {
      extern bool touchyEnabled;
      drawMenuText("TCHY", 2, 10, currentMenuParentTextColor());
      drawMenuValue(touchyEnabled ? "ON" : "OFF", 2, 3, touchyEnabled ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', touchyEnabled ? 'G' : 'R', 3);
      break;
    }
    case 61: {
      const CRGB tc = menuTextColorFromCol(13);
      showIcons(ICON_SONG, dimIconColorFromText(tc));
      drawMenuText("MODE", 2, 3, tc);
      break;
    }
    case 62: {
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("ADV", 2, 10, tc);
      break;
    }

    case 34:
      {
        const CRGB tc = menuTextColorFromCol(14);
        showIcons(ICON_PATTERN, dimIconColorFromText(tc));
        drawMenuText("ETC", 2, 3, tc);
      }
      break;

    case 38:
      drawMenuText("DRAW", 2, 10, currentMenuParentTextColor());
      drawMenuValue(drawMode == 0 ? "L+R" : "R", 2, 3, UI_GREEN);
      drawMenuIndicator('L', 'G', 3);
      break;

    case 47: {
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("MUTE", 2, 10, tc);
      int selUch = constrain((int)currentMode->pos[1], 1, 16);
      const int rowDots = 3;
      for (int uch = 1; uch <= 16; uch++) {
        int in = userChToInternalDrawR(uch);
        bool inMask = (drawRFullMuteCustomUnmuteMask & (1u << in)) != 0;
        CRGB dot = inMask ? UI_GREEN : UI_RED;
        if (uch == selUch) {
          dot.r = (uint8_t)min(255, (int)dot.r + 100);
          dot.g = (uint8_t)min(255, (int)dot.g + 60);
          dot.b = (uint8_t)min(255, (int)dot.b + 60);
        }
        light(uch, rowDots, dot);
      }
      drawMenuIndicator('L', 'Y', 2);
      drawMenuIndicator('L', 'W', 3);
      break;
    }

    case 25: {
      drawMenuText("CTRL", 2, 10, currentMenuParentTextColor());
      drawMenuValue(ctrlMode == 0 ? "PAGE" : "VOL", 2, 3, ctrlMode == 0 ? UI_GREEN : UI_ORANGE);

      drawMenuIndicator('L', ctrlMode == 0 ? 'G' : 'O', 3);
      break;
    }

    case 26:

      {
        const CRGB tc = menuTextColorFromCol(5);
        showIcons(ICON_VOLUME_BIG, dimIconColorFromText(tc));
        drawMenuText("VOL", 2, 3, tc);
      }
      break;

    case 27: {
      drawMenuText("MAIN", 2, 10, currentMenuParentTextColor());
      extern struct GlobalVars GLOB;


      int volDisplay = GLOB.vol;

      if (volDisplay <= 10) {
        volDisplay = volDisplay * 10;
      }
      char volText[8];
      snprintf(volText, sizeof(volText), "%d", volDisplay);
      drawMenuText(volText, 2, 3, UI_ORANGE);

      drawMenuIndicator('L', 'O', 3);
      break;
    }

    case 28: {
      drawMenuText("LOUT", 2, 10, currentMenuParentTextColor());
      extern uint8_t lineOutLevelSetting;
      char levelText[8];

      int displayVal = (lineOutLevelSetting - LINEOUT_MIN) + 1;
      snprintf(levelText, sizeof(levelText), "%d", displayVal);
      drawMenuText(levelText, 2, 3, UI_MAGENTA);


      CRGB pinkColor = UI_MAGENTA;
      Encoder[2].writeRGBCode(pinkColor.r << 16 | pinkColor.g << 8 | pinkColor.b);
      drawMenuIndicator('L', 'M', 3);
      break;
    }

    case 29: {
      const char* trigger = previewTriggerMode == PREVIEW_MODE_PRESS ? "PRSS" :
          (previewTriggerMode == PREVIEW_MODE_SYNC ? "SYNC" : "ON");
      const char code = previewTriggerMode == PREVIEW_MODE_PRESS ? 'R' :
          (previewTriggerMode == PREVIEW_MODE_SYNC ? 'Y' : 'G');
      const CRGB valueColor = previewTriggerMode == PREVIEW_MODE_PRESS ? UI_RED :
          (previewTriggerMode == PREVIEW_MODE_SYNC ? UI_YELLOW : UI_GREEN);
      drawMenuText("PREV", 2, 10, currentMenuParentTextColor());
      char volText[8];
      snprintf(volText, sizeof(volText), "%u", previewVol);
      drawMenuText(previewModeLabelVisible ? trigger : volText, 2, 3,
               previewModeLabelVisible ? valueColor : UI_ORANGE);
      drawMenuIndicator('L', code, 2);
      drawMenuIndicator('L', 'O', 3);
      Encoder[1].writeRGBCode(CRGBToUint32(valueColor));
      Encoder[2].writeRGBCode(CRGBToUint32(UI_ORANGE));
      break;
    }

    case 30: {
      drawMenuText("MIC", 2, 10, currentMenuParentTextColor());
      extern unsigned int micGain;
      char gainText[8];
      snprintf(gainText, sizeof(gainText), "%d", micGain);
      drawMenuText(gainText, 2, 3, UI_RED);


      CRGB redColor = UI_RED;
      Encoder[2].writeRGBCode(redColor.r << 16 | redColor.g << 8 | redColor.b);
      drawMenuIndicator('L', 'R', 3);
      break;
    }

    case 31: {
      drawMenuText("L-IN", 2, 10, currentMenuParentTextColor());
      extern unsigned int lineInLevel;
      char levelText[8];
      snprintf(levelText, sizeof(levelText), "%d", lineInLevel);
      drawMenuText(levelText, 2, 3, UI_BLUE);


      CRGB blueColor = UI_BLUE;
      Encoder[2].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
      drawMenuIndicator('L', 'X', 3);
      break;
    }

    case 36: {
      drawMenuText("2-CH", 2, 10, currentMenuParentTextColor());

      clearTextArea(6, 3, 8);
      extern int8_t stereoChannel;
      if (stereoChannel == 0) {
        drawMenuText("OFF", 2, 3, UI_RED);
      } else if (stereoChannel == 1) {
        drawMenuText("M+P", 2, 3, UI_GREEN);
      } else {
        drawMenuText("L+R", 2, 3, UI_GREEN);
      }
      drawMenuIndicator('L', stereoChannel == 0 ? 'R' : 'G', 3);
      break;
    }

    case 43: {
      extern bool getSpkrEnabled();
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("SPKR", 2, 10, tc);
      bool speakerOn = getSpkrEnabled();
      drawMenuValue(speakerOn ? "ON" : "OFF", 2, 3, speakerOn ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', speakerOn ? 'G' : 'R', 3);
      break;
    }

    case 64: {
      extern bool getAdcHpfEnabled();
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("HPF", 2, 10, tc);
      bool on = getAdcHpfEnabled();
      drawMenuValue(on ? "ON" : "OFF", 2, 3, on ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', on ? 'G' : 'R', 3);
      break;
    }

    case 65: {
      extern bool getUsbSmpPreview();
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("USBP", 2, 10, tc);
      bool on = getUsbSmpPreview();
      drawMenuValue(on ? "ON" : "OFF", 2, 3, on ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', on ? 'G' : 'R', 3);
      break;
    }

    case 46: {
      drawMenuText("HFC", 2, 10, currentMenuParentTextColor());
      extern uint16_t codecHfCut;
      char valText[8];
      unsigned disp = (unsigned)codecHfCut;
      if (disp > 256u) disp = 256u;
      if (disp != 256u) disp = (disp / 10u) * 10u;
      snprintf(valText, sizeof(valText), "%u", disp);
      clearTextArea(2, 3, 16);
      drawMenuText(valText, 2, 3, UI_RED);
      CRGB encCol = UI_RED;
      Encoder[2].writeRGBCode(encCol.r << 16 | encCol.g << 8 | encCol.b);
      drawMenuIndicator('L', 'R', 3);
      break;
    }

    case 53: {
      extern uint8_t mixGain14;
      extern uint8_t mixGain58;
      extern uint8_t mixGainSynth;
      extern uint8_t mixGainMaster;
      const char* title = "ALL";
      uint8_t shown = mixGainMaster;
      CRGB valCol = UI_GREEN;
      if (g_gainUiFocus == 0) { title = "1-4"; shown = mixGain14; valCol = UI_CYAN; }
      else if (g_gainUiFocus == 1) { title = "5-8"; shown = mixGain58; valCol = UI_ORANGE; }
      else if (g_gainUiFocus == 2) { title = "SYN"; shown = mixGainSynth; valCol = UI_MAGENTA; }

      clearTextArea(1, 10, (int)maxX);
      drawMenuText(title, 2, 10, currentMenuParentTextColor());
      char valText[8];
      snprintf(valText, sizeof(valText), "%u.%ux", (unsigned)(shown / 10u), (unsigned)(shown % 10u));
      drawMenuText(valText, 2, 3, valCol);
      const uint8_t bars[4] = { mixGain14, mixGain58, mixGainSynth, mixGainMaster };
      const CRGB barCol[4] = { UI_CYAN, UI_ORANGE, UI_MAGENTA, UI_GREEN };
      for (int i = 0; i < 4; i++) {
        int filled = ((int)bars[i] * 4 + MIX_GAIN_MAX / 2) / MIX_GAIN_MAX;
        if (filled > 4) filled = 4;
        int x0 = 1 + i * 4;
        CRGB on = barCol[i];
        if ((int)g_gainUiFocus != i) {
          on.nscale8(90);
        }
        for (int k = 0; k < 4; k++) {
          light(x0 + k, 8, (k < filled) ? on : CRGB(0, 0, 0));
        }
      }
      drawMenuIndicator('L', 'N', 1);
      drawMenuIndicator('L', 'O', 2);
      drawMenuIndicator('L', g_gainEnc3IsAll ? 'G' : 'M', 3);
      break;
    }

    case 32: {
      drawMenuText("CRSR", 2, 10, currentMenuParentTextColor());
      extern bool showChannelNr;
      const char* lbl = showChannelNr ? "CHNR" : "NORM";
      CRGB col = UI_GREEN;
      drawMenuValue(lbl, 2, 3, col);
      Encoder[2].writeRGBCode(UI_GREEN.r << 16 | UI_GREEN.g << 8 | UI_GREEN.b);
      drawMenuIndicator('L', 'G', 3);
      break;
    }

    case 33: {
      drawMenuText("PREV", 2, 10, currentMenuParentTextColor());
      const char* prevLbl = "ON";
      CRGB prevCol = UI_GREEN;
      if (previewTriggerMode == PREVIEW_MODE_PRESS) {
        prevLbl = "PRSS";
        prevCol = CRGB(0, 150, 255);
      } else if (previewTriggerMode == PREVIEW_MODE_SYNC) {
        prevLbl = "SYNC";
        prevCol = CRGB(255, 165, 0);
      }
      drawMenuValue(prevLbl, 2, 3, prevCol);
      CRGB greenColor = getIndicatorColor('G');
      Encoder[2].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
      drawMenuIndicator('L', 'B', 3);
      break;
    }
  }
}

FLASHMEM void drawAdditionalFeatures(int setting) {
  switch (setting) {
    case 4: {

      char gainText[8];
      sprintf(gainText, "%d", micGain);




      break;
    }


    case 15: {

      for (int x = 1; x <= 16; x++) {
        if (x >= aiBaseStartPage && x <= aiBaseEndPage) {
          light(x, 8, CRGB(255, 165, 0));
        } else if (x > aiBaseEndPage && x <= aiBaseEndPage + aiTargetPage) {
          light(x, 8, CRGB(0, 255, 0));
        } else {
          light(x, 8, CRGB(0, 0, 0));
        }
      }

      break;
    }

    case 16: {
      const char* modeText;
      if (resetMenuOption == 0) {
        modeText = "SD";
      } else if (resetMenuOption == 1) {
        modeText = "EFX";
      } else if (resetMenuOption == 2) {
        modeText = "FULL";
      } else if (resetMenuOption == 3) {
        modeText = "PATT";
      } else if (resetMenuOption == 4) {
        modeText = "KIT";
      } else {
        modeText = "ASAV";
      }
      drawMenuValue(modeText, 2, 3, UI_ORANGE);
      break;
    }

  }
}

FLASHMEM void drawGenreSelection() {

  const char* genres[] = {"BLNK", "TECH", "HIPH", "DNB", "HOUS", "AMBT"};
  CRGB genreColors[] = {
    CRGB(100, 100, 100),
    CRGB(255, 100, 0),
    CRGB(255, 0, 255),
    CRGB(0, 255, 0),
    CRGB(0, 100, 255),
    CRGB(255, 255, 0)
  };


  drawText(genres[genreType], 2, 3, genreColors[genreType]);
}

void cycleFireMenuFocus() {
  extern uint8_t fireFocus;
  fireFocus = (uint8_t)((fireFocus + 1) % 3);
  menuRequestFullRedraw();
}

FLASHMEM bool handleAdditionalFeatureControls(int setting) {
  static bool aiMenuFirstEnter = true;
  static bool menuFirstEnter = true;
  static bool pongMenuFirstEnter = true;
  static bool mainMenuFirstEnter = true;
  static bool lineMenuFirstEnter = true;
  static bool prevMenuFirstEnter = true;
  static bool micMenuFirstEnter = true;
  static bool lvlMenuFirstEnter = true;
  static bool stereoChMenuFirstEnter = true;
  static int lastStereoCh = -1;
  static int lastPongSpeed = -1;
  static int lastSetting = -1;
  bool didRedraw = false;


  if (setting != lastSetting) {
    aiMenuFirstEnter = true;
    menuFirstEnter = true;
    pongMenuFirstEnter = true;
    mainMenuFirstEnter = true;
    lineMenuFirstEnter = true;
    prevMenuFirstEnter = true;
    micMenuFirstEnter = true;
    lvlMenuFirstEnter = true;
    stereoChMenuFirstEnter = true;
    lastStereoCh = -1;
    infoPageFirstEnter = true;
    lastPongSpeed = -1;
    if (lastSetting == 50 || lastSetting == 53 || lastSetting == 55) {
      extern uint8_t fireFocus;
      if (lastSetting == 55) fireFocus = 0;
      Encoder[0].writeMin((int32_t)1);
      Encoder[0].writeMax((int32_t)1);
      Encoder[0].writeCounter((int32_t)1);
      currentMode->pos[0] = 1;
      Encoder[1].writeMin((int32_t)1);
      Encoder[1].writeMax((int32_t)1);
      Encoder[1].writeCounter((int32_t)1);
      currentMode->pos[1] = 1;
    }
    lastSetting = setting;
  }

  auto redrawMain = [&](int s) {


    clearTextArea(1, 3, (int)maxX);
    clearTextArea(1, 10, (int)maxX);
    drawMainSettingStatus(s);
    didRedraw = true;
  };
  auto redrawAdd = [&](int s) {
    drawAdditionalFeatures(s);
    didRedraw = true;
  };

  switch (setting) {
    case 4: {
      static int lastRecModeEnc = -1;
      int encVal = (recMode == 1) ? 0 : 1;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastRecModeEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastRecModeEnc) {
        recMode = (currentMode->pos[2] == 0) ? 1 : -1;
        encVal = (recMode == 1) ? 0 : 1;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastRecModeEnc = encVal;
        saveSingleModeToEEPROM(0, recMode);
        recInput = (recMode == 1) ? AUDIO_INPUT_MIC : AUDIO_INPUT_LINEIN;
        sgtl5000_1.inputSelect(recInput);
        if (recInput == AUDIO_INPUT_MIC) sgtl5000_1.micGain(micGain);
        else sgtl5000_1.micGain(0);
        extern void applySgtl5000AdcHighPass();
        applySgtl5000AdcHighPass();
        redrawMain(setting);
        redrawAdd(setting);
      }
      break;
    }

    case 12: {
      extern bool imageMode;
      static int lastRecChannelClear = -1;

      if (imageMode && recChannelClear != 2) {
        recChannelClear = 2;
        saveSingleModeToEEPROM(6, 2);
        SMP_REC_CHANNEL_CLEAR = false;
      }
      int shown = imageMode ? 2 : recChannelClear;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)shown);
        Encoder[2].writeMax((int32_t)4);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = shown;
        lastRecChannelClear = shown;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastRecChannelClear) {
        if (imageMode) {
          recChannelClear = 2;
          Encoder[2].writeCounter((int32_t)2);
          currentMode->pos[2] = 2;
          lastRecChannelClear = 2;
          saveSingleModeToEEPROM(6, 2);
          SMP_REC_CHANNEL_CLEAR = false;
        } else {
          recChannelClear = constrain((int)currentMode->pos[2], 0, 4);
          Encoder[2].writeCounter((int32_t)recChannelClear);
          currentMode->pos[2] = recChannelClear;
          lastRecChannelClear = recChannelClear;
          saveSingleModeToEEPROM(6, recChannelClear);
        }
        redrawMain(setting);
      }
      break;
    }

    case 7: {
      static int lastVoiceEnc = -1;
      auto vsToEnc = [](int vs) { return (vs == -1) ? 0 : vs; };
      auto encToVs = [](int e) { return (e == 0) ? -1 : e; };
      int encVal = vsToEnc(voiceSelect);
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)2);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastVoiceEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastVoiceEnc) {
        voiceSelect = encToVs(constrain((int)currentMode->pos[2], 0, 2));
        encVal = vsToEnc(voiceSelect);
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastVoiceEnc = encVal;
        saveSingleModeToEEPROM(4, voiceSelect);
        redrawMain(setting);
      }
      break;
    }

    case 8: {
      static int lastTransportEnc = -1;
      auto tmToEnc = [](int tm) { return (tm == -1) ? 0 : tm; };
      auto encToTm = [](int e) { return (e == 0) ? -1 : e; };
      int encVal = tmToEnc(transportMode);
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)2);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastTransportEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastTransportEnc) {
        transportMode = encToTm(constrain((int)currentMode->pos[2], 0, 2));
        encVal = tmToEnc(transportMode);
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastTransportEnc = encVal;
        saveSingleModeToEEPROM(2, transportMode);
        redrawMain(setting);
      }
      break;
    }

    case 13: {
      static int lastMidiSendEnc = -1;
      extern int midiSendMode;
      extern bool MIDI_CLOCK_SEND;
      extern bool MIDI_NOTE_SEND;
      extern int clockMode;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)midiSendMode);
        Encoder[2].writeMax((int32_t)2);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = midiSendMode;
        lastMidiSendEnc = midiSendMode;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastMidiSendEnc) {
        midiSendMode = constrain((int)currentMode->pos[2], 0, 2);
        if (midiSendMode == 0) {
          MIDI_CLOCK_SEND = (clockMode == 1);
          MIDI_NOTE_SEND = false;
        } else if (midiSendMode == 1) {
          MIDI_CLOCK_SEND = false;
          MIDI_NOTE_SEND = true;
        } else {
          MIDI_CLOCK_SEND = (clockMode == 1);
          MIDI_NOTE_SEND = true;
        }
        saveSingleModeToEEPROM(24, midiSendMode);
        Encoder[2].writeCounter((int32_t)midiSendMode);
        currentMode->pos[2] = midiSendMode;
        lastMidiSendEnc = midiSendMode;
        redrawMain(setting);
      }
      break;
    }

    case 44: {
      static int lastRcveEnc = -1;
      extern bool MIDI_NOTE_RECEIVE;
      int encVal = MIDI_NOTE_RECEIVE ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastRcveEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastRcveEnc) {
        MIDI_NOTE_RECEIVE = (currentMode->pos[2] == 1);
        encVal = MIDI_NOTE_RECEIVE ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastRcveEnc = encVal;
        saveSingleModeToEEPROM(28, (int8_t)encVal);
        redrawMain(setting);
      }
      break;
    }

    case 60: {
      static int lastWrteEnc = -1;
      extern bool MIDI_WRITE_OVERWRITE;
      int encVal = MIDI_WRITE_OVERWRITE ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastWrteEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastWrteEnc) {
        MIDI_WRITE_OVERWRITE = (currentMode->pos[2] == 1);
        encVal = MIDI_WRITE_OVERWRITE ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastWrteEnc = encVal;
        saveSingleModeToEEPROM(54, (int8_t)encVal);
        redrawMain(setting);
      }
      break;
    }

    case 51: {
      static int lastClampEnc = -1;
      extern bool MIDI_NOTE_CLAMP;
      int encVal = MIDI_NOTE_CLAMP ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastClampEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastClampEnc) {
        MIDI_NOTE_CLAMP = (currentMode->pos[2] == 1);
        encVal = MIDI_NOTE_CLAMP ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastClampEnc = encVal;
        saveSingleModeToEEPROM(37, (int8_t)encVal);
        redrawMain(setting);
      }
      break;
    }

    case 45: {
      extern int8_t transportSendDelayMs;
      extern int8_t transportRcveDelayMs;
      static int lastSyncEnc1 = -1;
      static int lastSyncEnc2 = -1;
      int syncEdit = (currentMode->pos[1] == 0) ? 0 : 1;
      int syncSigned = (syncEdit == 0) ? (int)transportSendDelayMs : (int)transportRcveDelayMs;
      int syncRaw = syncSigned + 127;
      syncRaw = constrain(syncRaw, 0, 254);
      if (menuFirstEnter) {
        Encoder[1].writeCounter((int32_t)syncEdit);
        Encoder[1].writeMax((int32_t)1);
        Encoder[1].writeMin((int32_t)0);
        Encoder[2].writeCounter((int32_t)syncRaw);
        Encoder[2].writeMax((int32_t)254);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[1] = syncEdit;
        currentMode->pos[2] = syncRaw;
        lastSyncEnc1 = syncEdit;
        lastSyncEnc2 = syncRaw;
        menuFirstEnter = false;
      }
      syncEdit = (int)currentMode->pos[1];
      syncRaw = constrain((int)currentMode->pos[2], 0, 254);
      syncSigned = syncRaw - 127;
      if (currentMode->pos[1] != lastSyncEnc1) {
        lastSyncEnc1 = syncEdit;
        syncSigned = (syncEdit == 0) ? (int)transportSendDelayMs : (int)transportRcveDelayMs;
        syncRaw = constrain(syncSigned + 127, 0, 254);
        Encoder[2].writeCounter((int32_t)syncRaw);
        currentMode->pos[2] = syncRaw;
        lastSyncEnc2 = syncRaw;
      }
      if ((int)currentMode->pos[2] != lastSyncEnc2) {
        lastSyncEnc2 = syncRaw;
        currentMode->pos[2] = syncRaw;
        if (syncEdit == 0) {
          transportSendDelayMs = (int8_t)constrain(syncSigned, -127, 127);
          saveTransportDelayToEEPROM(29, transportSendDelayMs);
        } else {
          transportRcveDelayMs = (int8_t)constrain(syncSigned, -127, 127);
          saveTransportDelayToEEPROM(31, transportRcveDelayMs);
        }
        redrawMain(setting);
      }
      break;
    }

    case 50: {
      extern bool getPulseClockStopWithPlay();
      extern uint8_t getPulseClockRateSel();
      extern uint8_t getPulseClockRateSelCount();
      extern uint8_t getPulseClockWidthMs();
      extern uint8_t getPulseClockWidthMsMin();
      extern uint8_t getPulseClockWidthMsMax();
      extern void setPulseClockStopWithPlay(bool);
      extern void setPulseClockRateSel(uint8_t);
      extern void setPulseClockWidthMs(uint8_t);
      static int lastPpqnWidth = -1;
      static int lastPpqnRun = -1;
      static int lastPpqnRate = -1;
      int widthVal = (int)getPulseClockWidthMs();
      int widthMin = (int)getPulseClockWidthMsMin();
      int widthMax = (int)getPulseClockWidthMsMax();
      int runVal = getPulseClockStopWithPlay() ? 0 : 1;
      int rateSel = (int)getPulseClockRateSel();
      uint8_t rateMax = getPulseClockRateSelCount() > 0 ? (uint8_t)(getPulseClockRateSelCount() - 1) : 0;
      if (menuFirstEnter) {
        Encoder[0].writeMin((int32_t)widthMin);
        Encoder[0].writeMax((int32_t)widthMax);
        Encoder[0].writeCounter((int32_t)widthVal);
        Encoder[1].writeMin((int32_t)0);
        Encoder[1].writeMax((int32_t)1);
        Encoder[1].writeCounter((int32_t)runVal);
        Encoder[2].writeMin((int32_t)0);
        Encoder[2].writeMax((int32_t)rateMax);
        Encoder[2].writeCounter((int32_t)rateSel);
        currentMode->pos[0] = (unsigned int)widthVal;
        currentMode->pos[1] = (unsigned int)runVal;
        currentMode->pos[2] = (unsigned int)rateSel;
        lastPpqnWidth = widthVal;
        lastPpqnRun = runVal;
        lastPpqnRate = rateSel;
        g_ppqnUiFocus = 0;
        menuFirstEnter = false;
      }
      if ((int)currentMode->pos[0] != lastPpqnWidth) {
        widthVal = constrain((int)currentMode->pos[0], widthMin, widthMax);
        setPulseClockWidthMs((uint8_t)widthVal);
        widthVal = (int)getPulseClockWidthMs();
        Encoder[0].writeCounter((int32_t)widthVal);
        currentMode->pos[0] = (unsigned int)widthVal;
        lastPpqnWidth = widthVal;
        g_ppqnUiFocus = 2;
        redrawMain(setting);
      }
      if ((int)currentMode->pos[1] != lastPpqnRun) {
        runVal = constrain((int)currentMode->pos[1], 0, 1);
        setPulseClockStopWithPlay(runVal == 0);
        runVal = getPulseClockStopWithPlay() ? 0 : 1;
        Encoder[1].writeCounter((int32_t)runVal);
        currentMode->pos[1] = (unsigned int)runVal;
        lastPpqnRun = runVal;
        g_ppqnUiFocus = 1;
        redrawMain(setting);
      }
      if ((int)currentMode->pos[2] != lastPpqnRate) {
        rateSel = constrain((int)currentMode->pos[2], 0, (int)rateMax);
        setPulseClockRateSel((uint8_t)rateSel);
        rateSel = (int)getPulseClockRateSel();
        Encoder[2].writeCounter((int32_t)rateSel);
        currentMode->pos[2] = (unsigned int)rateSel;
        lastPpqnRate = rateSel;
        g_ppqnUiFocus = 0;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 58: {
      static const uint8_t kSideChoices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14 };
      static const int kSideCount = 12;
      static int lastSideEnc = -1;
      extern uint8_t getSidechainVoice();
      extern void setSidechainVoice(uint8_t);
      int voiceIdx = 1;
      uint8_t cur = getSidechainVoice();
      for (int i = 0; i < kSideCount; i++) {
        if (kSideChoices[i] == cur) voiceIdx = i;
      }
      if (menuFirstEnter) {
        Encoder[2].writeMin((int32_t)0);
        Encoder[2].writeMax((int32_t)(kSideCount - 1));
        Encoder[2].writeCounter((int32_t)voiceIdx);
        currentMode->pos[2] = (unsigned int)voiceIdx;
        lastSideEnc = voiceIdx;
        menuFirstEnter = false;
      }
      if ((int)currentMode->pos[2] != lastSideEnc) {
        int raw = constrain((int)currentMode->pos[2], 0, kSideCount - 1);
        setSidechainVoice(kSideChoices[raw]);
        voiceIdx = raw;
        Encoder[2].writeCounter((int32_t)voiceIdx);
        currentMode->pos[2] = (unsigned int)voiceIdx;
        lastSideEnc = voiceIdx;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 59: {
      extern uint8_t getAnalogInHoldMs();
      extern void setAnalogInHoldMs(uint8_t);
      extern uint8_t getAnalogInPpqnIndex();
      extern uint8_t getAnalogInPpqnCount();
      extern void setAnalogInPpqnIndex(uint8_t);
      static const int kCinHoldMin = 1;
      static const int kCinHoldMax = 32;
      static int lastCinHold = -1;
      static int lastCinRate = -1;
      const int rateMax = (int)getAnalogInPpqnCount() - 1;
      if (menuFirstEnter) {
        Encoder[0].writeMin((int32_t)kCinHoldMin);
        Encoder[0].writeMax((int32_t)kCinHoldMax);
        Encoder[0].writeCounter((int32_t)getAnalogInHoldMs());
        Encoder[2].writeMin((int32_t)0);
        Encoder[2].writeMax((int32_t)rateMax);
        Encoder[2].writeCounter((int32_t)getAnalogInPpqnIndex());
        currentMode->pos[0] = (unsigned int)getAnalogInHoldMs();
        currentMode->pos[2] = (unsigned int)getAnalogInPpqnIndex();
        lastCinHold = (int)currentMode->pos[0];
        lastCinRate = (int)currentMode->pos[2];
        g_cinFocus = 0;
        menuFirstEnter = false;
      }
      if ((int)currentMode->pos[0] != lastCinHold) {
        setAnalogInHoldMs((uint8_t)constrain((int)currentMode->pos[0], kCinHoldMin, kCinHoldMax));
        lastCinHold = (int)getAnalogInHoldMs();
        Encoder[0].writeCounter((int32_t)lastCinHold);
        currentMode->pos[0] = (unsigned int)lastCinHold;
        g_cinFocus = 1;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      if ((int)currentMode->pos[2] != lastCinRate) {
        setAnalogInPpqnIndex((uint8_t)constrain((int)currentMode->pos[2], 0, rateMax));
        lastCinRate = (int)getAnalogInPpqnIndex();
        Encoder[2].writeCounter((int32_t)lastCinRate);
        currentMode->pos[2] = (unsigned int)lastCinRate;
        g_cinFocus = 0;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 52: {
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)etcRamMetric);
        Encoder[2].writeMax((int32_t)3);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = (unsigned int)etcRamMetric;
        lastEtcRamEnc = etcRamMetric;
        menuFirstEnter = false;
      }
      int encVal = constrain((int)currentMode->pos[2], 0, 3);
      if (encVal != lastEtcRamEnc) {
        etcRamMetric = encVal;
        lastEtcRamEnc = encVal;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = (unsigned int)encVal;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 40: {
      static int lastLghtEnc = -1;
      extern bool getLedStripEnabled();
      extern void setLedStripEnabled(bool);
      int encVal = getLedStripEnabled() ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastLghtEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastLghtEnc) {
        setLedStripEnabled(currentMode->pos[2] == 1);
        encVal = getLedStripEnabled() ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastLghtEnc = encVal;
        saveSingleModeToEEPROM(25, (int8_t)(encVal ? 1 : 0));
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 41: {
      static int lastColrEnc = -1;
      extern CRGB col[];
      extern CRGB col_base[];
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)currentColorScheme);
        Encoder[2].writeMax((int32_t)3);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = currentColorScheme;
        lastColrEnc = currentColorScheme;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastColrEnc) {
        uint8_t newScheme = constrain((int)currentMode->pos[2], 0, 3);
        if (newScheme == 3 && SD.exists("scheme.txt")) {
          File schemeFile = SD.open("scheme.txt", FILE_READ);
          if (schemeFile && schemeFile.size() >= 90) {
            uint8_t buf[90];
            int bytesRead = schemeFile.read(buf, 90);
            schemeFile.close();
            if (bytesRead == 90) {
              for (int i = 0; i < 15; i++)
                col[i] = CRGB(buf[i*3], buf[i*3+1], buf[i*3+2]);
              for (int i = 0; i < 15; i++) {
                int baseIdx = 45 + i * 3;
                col_base[i] = CRGB(buf[baseIdx], buf[baseIdx+1], buf[baseIdx+2]);
              }
              extern volatile bool colorsUpdatedViaSerial;
              colorsUpdatedViaSerial = true;
            } else { newScheme = 0; applyColorScheme(0); }
          } else { newScheme = 0; applyColorScheme(0); }
        } else {
          applyColorScheme(newScheme);
          extern volatile bool colorsUpdatedViaSerial;
          colorsUpdatedViaSerial = true;
        }
        currentColorScheme = newScheme;
        saveSingleModeToEEPROM(22, (int8_t)currentColorScheme);
        Encoder[2].writeCounter((int32_t)newScheme);
        currentMode->pos[2] = newScheme;
        lastColrEnc = newScheme;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 43: {
      static int lastSpkrEnc = -1;
      extern bool getSpkrEnabled();
      extern void setSpkrEnabled(bool);
      int encVal = getSpkrEnabled() ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastSpkrEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastSpkrEnc) {
        setSpkrEnabled(currentMode->pos[2] == 1);
        encVal = getSpkrEnabled() ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastSpkrEnc = encVal;
        saveSingleModeToEEPROM(27, (int8_t)(encVal ? 1 : 0));
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 64: {
      static int lastHpfEnc = -1;
      extern bool getAdcHpfEnabled();
      extern void setAdcHpfEnabled(bool);
      int encVal = getAdcHpfEnabled() ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastHpfEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastHpfEnc) {
        setAdcHpfEnabled(currentMode->pos[2] == 1);
        encVal = getAdcHpfEnabled() ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastHpfEnc = encVal;
        saveSingleModeToEEPROM(56, (int8_t)(encVal ? 1 : 0));
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 65: {
      static int lastUsbPrevEnc = -1;
      extern bool getUsbSmpPreview();
      extern void setUsbSmpPreview(bool);
      int encVal = getUsbSmpPreview() ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastUsbPrevEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastUsbPrevEnc) {
        setUsbSmpPreview(currentMode->pos[2] == 1);
        encVal = getUsbSmpPreview() ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastUsbPrevEnc = encVal;
        saveSingleModeToEEPROM(57, (int8_t)(encVal ? 1 : 0));
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 55: {
      static const uint8_t kFireChoices[] = {0, 15, 1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
      static int lastFireEnc = -1;
      static int lastFireLevel = -1;
      static int lastFireSize = -1;
      static int lastFireFocus = -1;
      static int lastFireFocusMax = -1;
      static bool fireLimitsSynced = false;
      extern uint8_t fireVoice;
      extern uint8_t fireLevel;
      extern uint8_t fireSize;
      extern uint8_t fireGravity;
      extern uint8_t fireColor;
      extern uint8_t fireFocus;
      int voiceIdx = 0;
      for (int i = 0; i < 13; i++) {
        if (kFireChoices[i] == fireVoice) voiceIdx = i;
      }
      int focusVal = voiceIdx;
      int focusMax = 12;
      if (fireFocus == 1) {
        focusVal = constrain((int)fireGravity, 0, 8);
        focusMax = 8;
      } else if (fireFocus == 2) {
        focusVal = constrain((int)fireColor, 0, 8);
        focusMax = 8;
      }
      if (menuFirstEnter || !fireLimitsSynced || focusMax != lastFireFocusMax) {
        Encoder[0].writeMin((int32_t)1);
        Encoder[0].writeMax((int32_t)4);
        Encoder[1].writeMin((int32_t)1);
        Encoder[1].writeMax((int32_t)25);
        Encoder[2].writeMin((int32_t)0);
        Encoder[2].writeMax((int32_t)focusMax);
        lastFireFocusMax = focusMax;
        fireLimitsSynced = true;
      }
      if (menuFirstEnter || (int)fireFocus != lastFireFocus) {
        int size = constrain((int)fireSize, 1, 4);
        int level = constrain((int)fireLevel, 1, 25);
        fireSize = (uint8_t)size;
        fireLevel = (uint8_t)level;
        Encoder[0].writeCounter((int32_t)size);
        currentMode->pos[0] = (unsigned int)size;
        lastFireSize = size;
        Encoder[1].writeCounter((int32_t)level);
        currentMode->pos[1] = (unsigned int)level;
        lastFireLevel = level;
        Encoder[2].writeCounter((int32_t)focusVal);
        currentMode->pos[2] = (unsigned int)focusVal;
        lastFireEnc = focusVal;
        lastFireFocus = fireFocus;
        menuFirstEnter = false;
      }
      if ((int)currentMode->pos[0] != lastFireSize) {
        int size = constrain((int)currentMode->pos[0], 1, 4);
        fireSize = (uint8_t)size;
        Encoder[0].writeCounter((int32_t)size);
        currentMode->pos[0] = (unsigned int)size;
        lastFireSize = size;
        saveSingleModeToEEPROM(45, (int8_t)fireSize);
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      if ((int)currentMode->pos[1] != lastFireLevel) {
        int level = constrain((int)currentMode->pos[1], 1, 25);
        fireLevel = (uint8_t)level;
        Encoder[1].writeCounter((int32_t)level);
        currentMode->pos[1] = (unsigned int)level;
        lastFireLevel = level;
        saveSingleModeToEEPROM(44, (int8_t)fireLevel);
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      if ((int)currentMode->pos[2] != lastFireEnc) {
        int raw = constrain((int)currentMode->pos[2], 0, focusMax);
        if (fireFocus == 1) {
          fireGravity = (uint8_t)raw;
          saveSingleModeToEEPROM(46, (int8_t)fireGravity);
        } else if (fireFocus == 2) {
          fireColor = (uint8_t)raw;
          saveSingleModeToEEPROM(47, (int8_t)fireColor);
        } else {
          fireVoice = kFireChoices[raw];
          saveSingleModeToEEPROM(43, (int8_t)fireVoice);
        }
        Encoder[2].writeCounter((int32_t)raw);
        currentMode->pos[2] = (unsigned int)raw;
        lastFireEnc = raw;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 48: {
      static int lastChldEnc = -1;
      extern bool getChildLockEnabled();
      extern void setChildLockEnabled(bool);
      int encVal = getChildLockEnabled() ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastChldEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastChldEnc) {
        setChildLockEnabled(currentMode->pos[2] == 1);
        encVal = getChildLockEnabled() ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastChldEnc = encVal;
        saveSingleModeToEEPROM(36, (int8_t)(encVal ? 1 : 0));
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 63: {
      extern bool touchyEnabled;
      extern void setTouchyEnabled(bool enabled);
      if (menuFirstEnter) {
        currentMode->pos[2] = touchyEnabled ? 1 : 0;
        Encoder[2].writeMin((int32_t)0);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
        menuFirstEnter = false;
      }
      bool enabled = currentMode->pos[2] != 0;
      if (enabled != touchyEnabled) { setTouchyEnabled(enabled); redrawMain(setting); }
      break;
    }
    case 56: {
      extern bool imageMode;
      extern void enforceImageModeConstraints();
      static int lastImgEnc = -1;
      int encVal = imageMode ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastImgEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastImgEnc) {
        imageMode = (currentMode->pos[2] == 1);
        encVal = imageMode ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastImgEnc = encVal;
        saveSingleModeToEEPROM(48, (int8_t)encVal);
        if (imageMode) enforceImageModeConstraints();
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 57: {
      extern uint8_t eyesMode;
      static int lastEyesEnc = -1;
      int encVal = constrain((int)eyesMode, 0, 3);
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)3);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastEyesEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastEyesEnc) {
        encVal = constrain((int)currentMode->pos[2], 0, 3);
        eyesMode = (uint8_t)encVal;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastEyesEnc = encVal;
        saveSingleModeToEEPROM(49, (int8_t)encVal);
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 46: {
      static int lastEqEnc = -1;
      extern uint16_t codecHfCut;
      extern void applySgtl5000CodecOutputPath();
      const int encMax = 26;
      if (menuFirstEnter) {
        int enc0;
        if (codecHfCut >= 256u) {
          enc0 = encMax;
        } else {
          enc0 = (int)(codecHfCut / 10u);
        }
        enc0 = constrain(enc0, 0, encMax);
        Encoder[2].writeCounter((int32_t)enc0);
        Encoder[2].writeMax((int32_t)encMax);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = enc0;
        lastEqEnc = enc0;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastEqEnc) {
        int encPos = constrain((int)currentMode->pos[2], 0, encMax);
        uint16_t newCut = (encPos >= encMax) ? 256u : (uint16_t)(encPos * 10);
        codecHfCut = newCut;
        saveSingleModeToEEPROM(32, hfcEncodeStep(codecHfCut));
        saveSingleModeToEEPROM(33, 5);
        applySgtl5000CodecOutputPath();
        Encoder[2].writeCounter((int32_t)encPos);
        currentMode->pos[2] = encPos;
        lastEqEnc = encPos;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 9: {
      static int lastPatternMode = -2;

      auto encToPm = [](int e) { return (e == 0) ? -1 : e; };
      auto pmToEnc = [](int p) { return (p == -1) ? 0 : p; };
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)pmToEnc(patternMode));
        Encoder[2].writeMax((int32_t)3);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = pmToEnc(patternMode);
        lastPatternMode = patternMode;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != pmToEnc(patternMode) || lastPatternMode != patternMode) {
        int enc = constrain((int)currentMode->pos[2], 0, 3);
        patternMode = encToPm(enc);
        Encoder[2].writeCounter((int32_t)enc);
        currentMode->pos[2] = enc;
        lastPatternMode = patternMode;
        saveSingleModeToEEPROM(3, patternMode);
        extern bool songModeActive;
        songModeActive = (patternMode == 2);
        if (patternMode != 3) { extern unsigned int pendingPage; pendingPage = 0; }
        unmuteAllChannels();
        applyMutesAfterPMODSwitch();
        if (currentMode == &draw || currentMode == &singleMode) {
          if (SMP_PATTERN_MODE) { updateLastPage(); Encoder[1].writeMax((int32_t)encoderPageMax()); }
          else { Encoder[1].writeMax((int32_t)maxPages); }
        }
        redrawMain(setting);
      }
      break;
    }

    case 10: {
      static int lastFlowEnc = -1;
      int encVal = (flowMode == 1) ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastFlowEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastFlowEnc) {
        flowMode = (currentMode->pos[2] == 1) ? 1 : -1;
        encVal = (flowMode == 1) ? 1 : 0;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastFlowEnc = encVal;
        lastFlowPage = 0;
        saveSingleModeToEEPROM(8, flowMode);
        redrawMain(setting);
      }
      break;
    }

    case 17: {
      static int lastViewEnc = -1;
      int encVal = simpleNotesView - 1;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastViewEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastViewEnc) {
        simpleNotesView = constrain((int)currentMode->pos[2], 0, 1) + 1;
        encVal = simpleNotesView - 1;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastViewEnc = encVal;
        saveSingleModeToEEPROM(11, simpleNotesView);
        redrawMain(setting);
      }
      break;
    }

    case 18: {
      static int lastLoopLength = -1;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)loopLength);
        Encoder[2].writeMax((int32_t)8);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = loopLength;
        lastLoopLength = loopLength;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastLoopLength) {
        loopLength = constrain((int)currentMode->pos[2], 0, 8);
        Encoder[2].writeCounter((int32_t)loopLength);
        currentMode->pos[2] = loopLength;
        lastLoopLength = loopLength;
        saveSingleModeToEEPROM(12, loopLength);
        redrawMain(setting);
      }
      break;
    }

    case 54: {
      extern bool voiceMode;
      extern bool imageMode;
      static int lastVoiceEnc = -1;
      int encVal = (voiceMode && !imageMode) ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastVoiceEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastVoiceEnc) {
        if (imageMode) {

          voiceMode = false;
          encVal = 0;
          Encoder[2].writeCounter((int32_t)0);
          currentMode->pos[2] = 0;
          lastVoiceEnc = 0;
          saveSingleModeToEEPROM(42, 0);
        } else {
          voiceMode = (currentMode->pos[2] == 1);
          encVal = voiceMode ? 1 : 0;
          Encoder[2].writeCounter((int32_t)encVal);
          currentMode->pos[2] = encVal;
          lastVoiceEnc = encVal;
          saveSingleModeToEEPROM(42, (int8_t)encVal);
          updateLastPage();
        }
        redrawMain(setting);
      }
      break;
    }

    case 23: {
      static int lastLedMode = -1;
      extern int ledModules;
      extern bool ledModulesRotated;
      uint8_t ledMode = ledModules + (ledModulesRotated ? 2 : 0);
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)(ledMode - 1));
        Encoder[2].writeMax((int32_t)3);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = ledMode - 1;
        lastLedMode = ledMode;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastLedMode - 1) {
        ledMode = constrain((int)currentMode->pos[2], 0, 3) + 1;
        ledModules = (ledMode == 1 || ledMode == 3) ? 1 : 2;
        ledModulesRotated = (ledMode >= 3);
        maxX = MATRIX_WIDTH * ledModules;
        extern void initLedStrip();
        initLedStrip();
        if (currentMode == &draw || currentMode == &singleMode) {
          int dynamicPages = (MAX_STEPS / maxX);
          Encoder[1].writeMax((int32_t)dynamicPages);
          Encoder[3].writeMax((int32_t)MAX_STEPS);
          if (currentMode->pos[1] > dynamicPages) {
            currentMode->pos[1] = dynamicPages;
            Encoder[1].writeCounter((int32_t)dynamicPages);
          }
          if (currentMode->pos[3] > MAX_STEPS) {
            currentMode->pos[3] = MAX_STEPS;
            Encoder[3].writeCounter((int32_t)MAX_STEPS);
          }
        }
        saveSingleModeToEEPROM(13, ledMode);
        Encoder[2].writeCounter((int32_t)(ledMode - 1));
        currentMode->pos[2] = ledMode - 1;
        lastLedMode = ledMode;
        FastLEDclear();
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 25: {
      static int lastCtrlMode = -1;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)ctrlMode);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = ctrlMode;
        lastCtrlMode = ctrlMode;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastCtrlMode) {
        ctrlMode = constrain((int)currentMode->pos[2], 0, 1);
        Encoder[2].writeCounter((int32_t)ctrlMode);
        currentMode->pos[2] = ctrlMode;
        lastCtrlMode = ctrlMode;
        saveSingleModeToEEPROM(14, ctrlMode);
        refreshCtrlEncoderConfig();
        redrawMain(setting);
      }
      break;
    }

    case 32: {
      static int lastCursorMode = -1;
      extern bool showChannelNr;
      int cursorMode = showChannelNr ? 1 : 0;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)cursorMode);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = cursorMode;
        lastCursorMode = cursorMode;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastCursorMode) {
        cursorMode = constrain((int)currentMode->pos[2], 0, 1);
        showChannelNr = (cursorMode == 1);
        saveSingleModeToEEPROM(18, 0);
        saveSingleModeToEEPROM(19, (int8_t)(showChannelNr ? 1 : 0));
        Encoder[2].writeCounter((int32_t)cursorMode);
        currentMode->pos[2] = cursorMode;
        lastCursorMode = cursorMode;
        redrawMain(setting);
      }
      break;
    }

    case 33: {
      static int lastPrevEnc = -1;
      int encVal = constrain(previewTriggerMode, 0, 2);
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)encVal);
        Encoder[2].writeMax((int32_t)2);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encVal;
        lastPrevEnc = encVal;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastPrevEnc) {
        previewTriggerMode = constrain((int)currentMode->pos[2], 0, 2);
        encVal = previewTriggerMode;
        Encoder[2].writeCounter((int32_t)encVal);
        currentMode->pos[2] = encVal;
        lastPrevEnc = encVal;
        saveSingleModeToEEPROM(20, (int8_t)previewTriggerMode);
        redrawMain(setting);
      }
      break;
    }

    case 38: {
      static int lastDrawMode = -1;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)drawMode);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = drawMode;
        lastDrawMode = drawMode;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastDrawMode) {
        drawMode = constrain((int)currentMode->pos[2], 0, 1);
        Encoder[2].writeCounter((int32_t)drawMode);
        currentMode->pos[2] = drawMode;
        lastDrawMode = drawMode;
        saveSingleModeToEEPROM(21, (int8_t)drawMode);
        redrawMain(setting);
      }
      break;
    }

    case 47: {
      static int lastMuteSelUch = -1;
      static int lastMuteOn = -1;
      if (menuFirstEnter) {
        int selUch = 1;
        for (int in = 0; in < 16; in++) {
          if (drawRFullMuteCustomUnmuteMask & (1u << in)) {
            selUch = internalToUserChDrawR(in);
            break;
          }
        }
        Encoder[1].writeCounter((int32_t)selUch);
        Encoder[1].writeMax((int32_t)16);
        Encoder[1].writeMin((int32_t)1);
        currentMode->pos[1] = selUch;
        int in = userChToInternalDrawR(selUch);
        int on = (drawRFullMuteCustomUnmuteMask & (1u << in)) ? 1 : 0;
        Encoder[2].writeCounter((int32_t)on);
        Encoder[2].writeMax((int32_t)1);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = on;
        lastMuteSelUch = selUch;
        lastMuteOn = on;
        menuFirstEnter = false;
      }
      int selUch = constrain((int)currentMode->pos[1], 1, 16);
      if (selUch != lastMuteSelUch) {
        lastMuteSelUch = selUch;
        currentMode->pos[1] = selUch;
        Encoder[1].writeCounter((int32_t)selUch);
        int in = userChToInternalDrawR(selUch);
        int on = (drawRFullMuteCustomUnmuteMask & (1u << in)) ? 1 : 0;
        Encoder[2].writeCounter((int32_t)on);
        currentMode->pos[2] = on;
        lastMuteOn = on;
        redrawMain(setting);
      }
      int inSel = userChToInternalDrawR(selUch);
      int on = constrain((int)currentMode->pos[2], 0, 1);
      if (on != lastMuteOn) {
        lastMuteOn = on;
        currentMode->pos[2] = on;
        Encoder[2].writeCounter((int32_t)on);
        if (on) {
          drawRFullMuteCustomUnmuteMask |= (uint16_t)(1u << inSel);
        } else {
          drawRFullMuteCustomUnmuteMask &= (uint16_t)~(1u << inSel);
        }
        saveSettingsU16ToEEPROM(34, drawRFullMuteCustomUnmuteMask);
        redrawMain(setting);
      }
      break;
    }

    case 11: {
      static int lastFastRecMode = -1;
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)fastRecMode);
        Encoder[2].writeMax((int32_t)4);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = fastRecMode;
        lastFastRecMode = fastRecMode;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastFastRecMode) {
        fastRecMode = constrain((int)currentMode->pos[2], 0, 4);
        Encoder[2].writeCounter((int32_t)fastRecMode);
        currentMode->pos[2] = fastRecMode;
        lastFastRecMode = fastRecMode;
        saveSingleModeToEEPROM(5, fastRecMode);
        redrawMain(setting);
      }
      break;
    }

    case 15: {
      static int lastAiTargetPage = -1;
      static int lastAiBaseStartPage = -1;
      static int lastAiBaseEndPage = -1;


      if (aiMenuFirstEnter) {


        Encoder[1].writeCounter((int32_t)aiBaseStartPage);
        Encoder[1].writeMax((int32_t)16);
        Encoder[1].writeMin((int32_t)1);

        Encoder[2].writeCounter((int32_t)aiBaseEndPage);
        Encoder[2].writeMax((int32_t)16);
        Encoder[2].writeMin((int32_t)1);

        Encoder[0].writeCounter((int32_t)aiTargetPage);
        Encoder[0].writeMax((int32_t)16);
        Encoder[0].writeMin((int32_t)1);

        aiMenuFirstEnter = false;
      }


      if (currentMode->pos[0] != lastAiTargetPage) {
        aiTargetPage = currentMode->pos[0];
        if (aiTargetPage > 16) aiTargetPage = 16;
        if (aiTargetPage < 1) aiTargetPage = 1;
        redrawAdd(setting);
        lastAiTargetPage = aiTargetPage;
      }


      if (currentMode->pos[1] != lastAiBaseStartPage) {
        aiBaseStartPage = currentMode->pos[1];
        if (aiBaseStartPage > 16) aiBaseStartPage = 16;
        if (aiBaseStartPage < 1) aiBaseStartPage = 1;


        if (aiBaseStartPage > aiBaseEndPage) {
          aiBaseEndPage = aiBaseStartPage;
          Encoder[2].writeCounter((int32_t)aiBaseEndPage);
        }

        redrawAdd(setting);
        lastAiBaseStartPage = aiBaseStartPage;
      }


      if (currentMode->pos[2] != lastAiBaseEndPage) {
        aiBaseEndPage = currentMode->pos[2];
        if (aiBaseEndPage > 16) aiBaseEndPage = 16;
        if (aiBaseEndPage < 1) aiBaseEndPage = 1;


        if (aiBaseEndPage < aiBaseStartPage) {
          aiBaseStartPage = aiBaseEndPage;
          Encoder[1].writeCounter((int32_t)aiBaseStartPage);
        }

        redrawAdd(setting);
        lastAiBaseEndPage = aiBaseEndPage;
      }
      break;
    }

    case 16: {
      static int lastResetOption = -1;

      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)resetMenuOption);
        Encoder[2].writeMax((int32_t)5);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = (unsigned int)resetMenuOption;
        lastResetOption = resetMenuOption;
        menuFirstEnter = false;
      }

      if ((int)currentMode->pos[2] != lastResetOption) {
        resetMenuOption = constrain((int)currentMode->pos[2], 0, 5);
        currentMode->pos[2] = (unsigned int)resetMenuOption;
        Encoder[2].writeCounter((int32_t)resetMenuOption);

        redrawAdd(setting);
        lastResetOption = resetMenuOption;
      }
      break;
    }

    case 42:
      break;

    case 24: {
      if (!pong) {
        static int lastPongEnc = -1;
        int encVal = 0;
        if (pongMenuFirstEnter) {
          Encoder[2].writeCounter((int32_t)0);
          Encoder[2].writeMax((int32_t)1);
          Encoder[2].writeMin((int32_t)0);
          currentMode->pos[2] = 0;
          lastPongEnc = 0;
          pongMenuFirstEnter = false;
        }
        if (currentMode->pos[2] != lastPongEnc) {
          if (currentMode->pos[2] == 1) {
            pong = true;
            pongMenuFirstEnter = true;
            lastPongSpeed = -1;
          }
          encVal = constrain((int)currentMode->pos[2], 0, 1);
          Encoder[2].writeCounter((int32_t)encVal);
          currentMode->pos[2] = encVal;
          lastPongEnc = encVal;
          redrawMain(setting);
        }
        break;
      }

      const int pongSpeedMin = 1;
      const int pongSpeedMax = 99;


      if (pongMenuFirstEnter) {
        int currentSpeed = pongIntervalToSpeed(pongUpdateInterval);
        Encoder[2].writeCounter(static_cast<int32_t>(currentSpeed));
        Encoder[2].writeMax(static_cast<int32_t>(pongSpeedMax));
        Encoder[2].writeMin(static_cast<int32_t>(0));
        currentMode->pos[2] = currentSpeed;
        lastPongSpeed = currentSpeed;
        pongMenuFirstEnter = false;
      }

      if (currentMode->pos[2] != lastPongSpeed) {
        int newVal = currentMode->pos[2];
        if (newVal <= 0) {
          pong = false;
          pongMenuFirstEnter = true;
          Encoder[2].writeCounter((int32_t)0);
          currentMode->pos[2] = 0;
          lastPongSpeed = -1;
          redrawMain(setting);
        } else {
          int newSpeed = constrain(newVal, pongSpeedMin, pongSpeedMax);
          if (newSpeed != lastPongSpeed) {
            uint16_t newInterval = pongSpeedToInterval(newSpeed);
            if (newInterval != pongUpdateInterval) {
              pongUpdateInterval = newInterval;
            }
            redrawMain(setting);
          }
          lastPongSpeed = newSpeed;
          Encoder[2].writeCounter(static_cast<int32_t>(newSpeed));
          currentMode->pos[2] = newSpeed;
        }
      }
      break;
    }

    case 27: {
      static int lastHpVol = -1;

      extern Mode *currentMode;
      extern struct GlobalVars GLOB;

      if (mainMenuFirstEnter) {


        int encoderVal;
        if (GLOB.vol <= 10) {

          encoderVal = GLOB.vol * 10;
        } else {

          encoderVal = GLOB.vol;
        }
        encoderVal = constrain(encoderVal, 0, 100);

        Encoder[2].writeCounter((int32_t)encoderVal);
        Encoder[2].writeMax((int32_t)100);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encoderVal;
        lastHpVol = encoderVal;
        mainMenuFirstEnter = false;


        float vol = encoderVal / 100.0f;
        sgtl5000_1.volume(vol);


        GLOB.vol = encoderVal;
      }

      if (currentMode->pos[2] != lastHpVol) {
        int encoderPos = constrain(currentMode->pos[2], 0, 100);


        float vol = encoderPos / 100.0f;


        GLOB.vol = encoderPos;


        sgtl5000_1.volume(vol);



        saveSingleModeToEEPROM(17, (int8_t)GLOB.vol);

        redrawMain(setting);
        lastHpVol = encoderPos;
      }
      break;
    }

    case 28: {
      static int lastLoVol = -1;

      extern uint8_t lineOutLevelSetting;

      if (lineMenuFirstEnter) {
        int displayVal = constrain((int)(lineOutLevelSetting - LINEOUT_MIN + 1), 1, (LINEOUT_MAX - LINEOUT_MIN + 1));
        Encoder[2].writeCounter((int32_t)displayVal);
        Encoder[2].writeMax((int32_t)(LINEOUT_MAX - LINEOUT_MIN + 1));
        Encoder[2].writeMin((int32_t)1);
        currentMode->pos[2] = displayVal;
        lastLoVol = displayVal;
        lineMenuFirstEnter = false;


        sgtl5000_1.lineOutLevel(32-lineOutLevelSetting);
      }

      if (currentMode->pos[2] != lastLoVol) {
        int encoderPos = constrain(currentMode->pos[2], 1, (LINEOUT_MAX - LINEOUT_MIN + 1));
        lineOutLevelSetting = (uint8_t)(LINEOUT_MIN + encoderPos - 1);


        sgtl5000_1.lineOutLevel(32-lineOutLevelSetting);


        saveSingleModeToEEPROM(15, (int8_t)lineOutLevelSetting);

        redrawMain(setting);
        lastLoVol = encoderPos;
      }
      break;
    }

    case 29: {
      static int lastPvVol = -1;
      static int lastPreviewTrigger = -1;

      extern unsigned int previewVol;

      if (prevMenuFirstEnter) {
        previewModeLabelVisible = false;
        const int trigger = constrain(previewTriggerMode, 0, 2);
        Encoder[1].writeMin((int32_t)0);
        Encoder[1].writeMax((int32_t)2);
        Encoder[1].writeCounter((int32_t)trigger);
        currentMode->pos[1] = trigger;
        lastPreviewTrigger = trigger;

        int encoderVal = constrain((int)previewVol, 0, 50);

        Encoder[2].writeCounter((int32_t)encoderVal);
        Encoder[2].writeMax((int32_t)50);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encoderVal;
        lastPvVol = encoderVal;
        prevMenuFirstEnter = false;


        updatePreviewVolume();
      }

      if ((int)currentMode->pos[1] != lastPreviewTrigger) {
        previewTriggerMode = constrain((int)currentMode->pos[1], 0, 2);
        lastPreviewTrigger = previewTriggerMode;
        currentMode->pos[1] = previewTriggerMode;
        saveSingleModeToEEPROM(20, (int8_t)previewTriggerMode);
        previewModeLabelVisible = true;
        previewModeLabelChangedAt = millis();
        redrawMain(setting);
      }
      if (currentMode->pos[2] != lastPvVol) {
        int encoderPos = constrain(currentMode->pos[2], 0, 50);
        previewModeLabelVisible = false;
        previewVol = encoderPos;


        updatePreviewVolume();


        saveSingleModeToEEPROM(7, previewVol);

        redrawMain(setting);
        lastPvVol = encoderPos;
      }
      break;
    }

    case 30: {
      static int lastMicGain = -1;

      extern unsigned int micGain;

      if (micMenuFirstEnter) {
        int encoderVal = constrain((int)micGain, 0, 63);
        Encoder[2].writeCounter((int32_t)encoderVal);
        Encoder[2].writeMax((int32_t)63);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encoderVal;
        lastMicGain = encoderVal;
        micMenuFirstEnter = false;


        extern unsigned int recInput;
        if (recInput == AUDIO_INPUT_MIC) {
          sgtl5000_1.micGain(micGain);
        } else {
          sgtl5000_1.micGain(0);
        }
      }

      if (currentMode->pos[2] != lastMicGain) {
        int encoderPos = constrain(currentMode->pos[2], 0, 63);
        micGain = (unsigned int)encoderPos;


        extern unsigned int recInput;
        if (recInput == AUDIO_INPUT_MIC) {
          sgtl5000_1.micGain(micGain);
        } else {
          sgtl5000_1.micGain(0);
        }


        saveSingleModeToEEPROM(9, micGain);

        redrawMain(setting);
        lastMicGain = encoderPos;
      }
      break;
    }

    case 31: {
      static int lastLineInLevel = -1;

      extern unsigned int lineInLevel;

      if (lvlMenuFirstEnter) {
        int encoderVal = constrain((int)lineInLevel, 0, 15);
        Encoder[2].writeCounter((int32_t)encoderVal);
        Encoder[2].writeMax((int32_t)15);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encoderVal;
        lastLineInLevel = encoderVal;
        lvlMenuFirstEnter = false;


        sgtl5000_1.lineInLevel(lineInLevel);
      }

      if (currentMode->pos[2] != lastLineInLevel) {
        int encoderPos = constrain(currentMode->pos[2], 0, 15);
        lineInLevel = (unsigned int)encoderPos;


        sgtl5000_1.lineInLevel(lineInLevel);


        saveSingleModeToEEPROM(16, lineInLevel);

        redrawMain(setting);
        lastLineInLevel = encoderPos;
      }
      break;
    }

    case 36: {
      extern int8_t stereoChannel;
      extern void applyStereoChannelRouting();




      Encoder[2].writeMax((int32_t)2);
      Encoder[2].writeMin((int32_t)0);

      if (stereoChMenuFirstEnter) {
        int encoderVal = constrain((int)stereoChannel, 0, 2);

        if (encoderVal > 2) encoderVal = 2;
        stereoChannel = (int8_t)encoderVal;

        saveSingleModeToEEPROM(23, stereoChannel);
        Encoder[2].writeCounter((int32_t)encoderVal);
        currentMode->pos[2] = encoderVal;
        lastStereoCh = encoderVal;
        stereoChMenuFirstEnter = false;

        applyStereoChannelRouting();
      }

      if (currentMode->pos[2] != lastStereoCh) {
        int encoderPos = constrain(currentMode->pos[2], 0, 2);
        stereoChannel = (int8_t)encoderPos;

        saveSingleModeToEEPROM(23, stereoChannel);
        applyStereoChannelRouting();

        redrawMain(setting);
        lastStereoCh = encoderPos;
      }
      break;
    }

    case 53: {
      extern uint8_t mixGain14;
      extern uint8_t mixGain58;
      extern uint8_t mixGainSynth;
      extern uint8_t mixGainMaster;
      extern void applyMixBusGains();
      static int lastG14 = -1;
      static int lastG58 = -1;
      static int lastGEnc3 = -1;
      static int lastEnc3IsAll = -1;
      if (menuFirstEnter) {
        g_gainEnc3IsAll = 1;
        g_gainUiFocus = 3;
        int enc0 = mixGainEnc0FromStored(mixGain14);
        Encoder[0].writeMin((int32_t)0);
        Encoder[0].writeMax((int32_t)MIX_GAIN_MAX);
        Encoder[0].writeCounter((int32_t)enc0);
        Encoder[1].writeMin((int32_t)0);
        Encoder[1].writeMax((int32_t)MIX_GAIN_MAX);
        Encoder[1].writeCounter((int32_t)mixGain58);
        Encoder[2].writeMin((int32_t)0);
        Encoder[2].writeMax((int32_t)MIX_GAIN_MAX);
        Encoder[2].writeCounter((int32_t)mixGainMaster);
        currentMode->pos[0] = (unsigned int)enc0;
        currentMode->pos[1] = mixGain58;
        currentMode->pos[2] = mixGainMaster;
        lastG14 = enc0;
        lastG58 = (int)mixGain58;
        lastGEnc3 = (int)mixGainMaster;
        lastEnc3IsAll = 1;
        menuFirstEnter = false;
      }
      if ((int)g_gainEnc3IsAll != lastEnc3IsAll) {
        uint8_t v = g_gainEnc3IsAll ? mixGainMaster : mixGainSynth;
        Encoder[2].writeMin((int32_t)0);
        Encoder[2].writeMax((int32_t)MIX_GAIN_MAX);
        Encoder[2].writeCounter((int32_t)v);
        currentMode->pos[2] = v;
        lastGEnc3 = (int)v;
        lastEnc3IsAll = (int)g_gainEnc3IsAll;
        redrawMain(setting);
      }
      if ((int)currentMode->pos[0] != lastG14) {
        int pos = constrain((int)currentMode->pos[0], 0, MIX_GAIN_MAX);
        mixGain14 = mixGainStoredFromEnc0(pos);
        Encoder[0].writeCounter((int32_t)pos);
        currentMode->pos[0] = (unsigned int)pos;
        lastG14 = pos;
        g_gainUiFocus = 0;
        saveMixGainToEEPROM(EEPROM_MIX_GAIN_14, mixGain14, 0x01);
        applyMixBusGains();
        redrawMain(setting);
      }
      if ((int)currentMode->pos[1] != lastG58) {
        mixGain58 = (uint8_t)constrain((int)currentMode->pos[1], 0, MIX_GAIN_MAX);
        Encoder[1].writeCounter((int32_t)mixGain58);
        currentMode->pos[1] = mixGain58;
        lastG58 = (int)mixGain58;
        g_gainUiFocus = 1;
        saveMixGainToEEPROM(EEPROM_MIX_GAIN_58, mixGain58, 0x02);
        applyMixBusGains();
        redrawMain(setting);
      }
      if ((int)currentMode->pos[2] != lastGEnc3) {
        uint8_t v = (uint8_t)constrain((int)currentMode->pos[2], 0, MIX_GAIN_MAX);
        if (g_gainEnc3IsAll) mixGainMaster = v;
        else mixGainSynth = v;
        Encoder[2].writeCounter((int32_t)v);
        currentMode->pos[2] = v;
        lastGEnc3 = (int)v;
        g_gainUiFocus = g_gainEnc3IsAll ? 3 : 2;
        saveMixGainToEEPROM(
          g_gainEnc3IsAll ? EEPROM_MIX_GAIN_ALL : EEPROM_MIX_GAIN_SYN,
          v,
          g_gainEnc3IsAll ? 0x08 : 0x04);
        applyMixBusGains();
        redrawMain(setting);
      }
      break;
    }

         default:

       aiMenuFirstEnter = true;
       menuFirstEnter = true;
       pongMenuFirstEnter = true;
       break;
  }
  return didRedraw;
}

FLASHMEM void switchMenu(int menuPosition){
   switch (menuPosition) {
      case 1:
        switchMode(&loadSaveTrack);
        break;

      case 2:
        switchMode(&set_SamplePack);
        currentMode->pos[3] = SMP.pack;
        Encoder[3].writeCounter((int32_t)SMP.pack);
        break;

      case 3:
        if (GLOB.currentChannel >= 1 && GLOB.currentChannel <= 8) {
          switchMode(&set_Wav);
        } else if (GLOB.currentChannel == 11 || GLOB.currentChannel == 13 || GLOB.currentChannel == 14) {
          extern Mode loadSavePreset;
          switchMode(&loadSavePreset);
        }
        break;

      case 4:

        recMode = recMode * (-1);
        saveSingleModeToEEPROM(0, recMode);

        recInput = (recMode == 1) ? AUDIO_INPUT_MIC : AUDIO_INPUT_LINEIN;
        sgtl5000_1.inputSelect(recInput);
        if (recInput == AUDIO_INPUT_MIC) {
          sgtl5000_1.micGain(micGain);
        } else {
          sgtl5000_1.micGain(0);
        }
        extern void applySgtl5000AdcHighPass();
        applySgtl5000AdcHighPass();

        drawMainSettingStatus(menuPosition);
        drawAdditionalFeatures(menuPosition);
        break;

      case 5:
        switchMode(&bpmMode);
        break;

      case 7:

        if (voiceSelect == -1) {
          voiceSelect = 1;
        } else if (voiceSelect == 1) {
          voiceSelect = 2;
        } else {
          voiceSelect = -1;
        }
        saveSingleModeToEEPROM(4, voiceSelect);

        drawMainSettingStatus(menuPosition);
        break;

      case 8:
        if (transportMode == -1) {
          transportMode = 1;
        } else if (transportMode == 1) {
          transportMode = 2;
        } else {
          transportMode = -1;
        }
        saveSingleModeToEEPROM(2, transportMode);
        drawMainSettingStatus(menuPosition);
        break;

      case 13:

        {
          extern int midiSendMode;
          extern bool MIDI_CLOCK_SEND;
          extern bool MIDI_NOTE_SEND;
          extern int clockMode;

          midiSendMode = (midiSendMode + 1) % 3;


          if (midiSendMode == 0) {

            MIDI_CLOCK_SEND = (clockMode == 1);
            MIDI_NOTE_SEND = false;
          } else if (midiSendMode == 1) {

            MIDI_CLOCK_SEND = false;
            MIDI_NOTE_SEND = true;
          } else {

            MIDI_CLOCK_SEND = (clockMode == 1);
            MIDI_NOTE_SEND = true;
          }

          saveSingleModeToEEPROM(24, midiSendMode);
          drawMainSettingStatus(menuPosition);
        }
        break;

      case 9:

        if (patternMode == -1) {
          patternMode = 1;
        } else if (patternMode == 1) {
          patternMode = 2;
        } else if (patternMode == 2) {
          patternMode = 3;
        } else {
          patternMode = -1;
        }
        saveSingleModeToEEPROM(3, patternMode);
        drawMainSettingStatus(menuPosition);


        extern bool songModeActive;
        songModeActive = (patternMode == 2);


        if (patternMode != 3) {
          extern unsigned int pendingPage;
          pendingPage = 0;
        }


        if (SMP_PATTERN_MODE) {

          unmuteAllChannels();
          applyMutesAfterPMODSwitch();
        } else {

          unmuteAllChannels();
          applyMutesAfterPMODSwitch();
        }


        if (currentMode == &draw || currentMode == &singleMode) {
          if (SMP_PATTERN_MODE) {


            updateLastPage();
            Encoder[1].writeMax((int32_t)encoderPageMax());
          } else {

            Encoder[1].writeMax((int32_t)maxPages);
          }
        }
        break;

      case 10:
        flowMode = flowMode * (-1);
        saveSingleModeToEEPROM(8, flowMode);
        drawMainSettingStatus(menuPosition);


        lastFlowPage = 0;
        break;

      case 11:
        fastRecMode = fastRecMode + 1;

        if (fastRecMode>4) fastRecMode=0;
        saveSingleModeToEEPROM(5, fastRecMode);
        drawMainSettingStatus(menuPosition);
        break;

         case 12: {
        extern bool imageMode;
        if (imageMode) {
          recChannelClear = 2;
        } else {
          recChannelClear = recChannelClear + 1;
          if (recChannelClear > 4) recChannelClear = 0;
        }
        saveSingleModeToEEPROM(6, recChannelClear);
        drawMainSettingStatus(menuPosition);
        break;
        }

        case 15:

        generateSong();
        break;

        case 16:
        if (resetMenuOption == 0) {

          FastLEDclear();
          drawText("SCAN", 2, 3, UI_GREEN);
          FastLEDshow();

          extern bool scanAndWriteManifest();
          extern void sampleBrowserInvalidate();
          scanAndWriteManifest();
          sampleBrowserInvalidate();
          showSaveSuccessAnimation();
        } else if (resetMenuOption == 1) {

          resetAllToDefaults();
        } else if (resetMenuOption == 2) {



          extern void startNew();
          startNew();

          FastLEDclear();
          drawText("SCAN", 2, 3, UI_GREEN);
          FastLEDshow();

          extern bool scanAndWriteManifest();
          extern void sampleBrowserInvalidate();
          scanAndWriteManifest();
          sampleBrowserInvalidate();
          showSaveSuccessAnimation();
        } else if (resetMenuOption == 3) {

          FastLEDclear();
          drawText("WIPE", 2, 10, UI_ORANGE);
          drawText("PATT", 2, 3, UI_ORANGE);
          FastLEDshow();

          char filename[24];
          int deletedCount = 0;
          for (int i = 1; i <= 999; i++) {
            sprintf(filename, "%d.txt", i);
            if (SD.exists(filename)) {
              SD.remove(filename);
              deletedCount++;
            }
            sprintf(filename, "%d.txt.tmp", i); SD.remove(filename);
            sprintf(filename, "%d.txt.bak", i); SD.remove(filename);
          }

          showSaveSuccessAnimation();
        } else if (resetMenuOption == 4) {

          FastLEDclear();
          drawText("WIPE", 2, 10, UI_ORANGE);
          drawText("KIT", 2, 3, UI_ORANGE);
          FastLEDshow();

          char dirname[16];
          char filepath[32];
          for (int pack = 0; pack <= 999; pack++) {
            if (pack == 1) continue;

            sprintf(dirname, "%d", pack);
            if (SD.exists(dirname)) {

              for (int sample = 1; sample <= 8; sample++) {
                sprintf(filepath, "%d/%d.wav", pack, sample);
                if (SD.exists(filepath)) {
                  SD.remove(filepath);
                }
              }

              SD.rmdir(dirname);
            }
          }


          extern void sampleBrowserInvalidate();
          sampleBrowserInvalidate();

          showSaveSuccessAnimation();
        } else if (resetMenuOption == 5) {

          FastLEDclear();
          drawText("WIPE", 2, 10, UI_ORANGE);
          drawText("ASAV", 2, 3, UI_ORANGE);
          FastLEDshow();

          extern void deleteAutosaveFile();
          extern void clearPatternNotes();
          deleteAutosaveFile();
          clearPatternNotes();

          showSaveSuccessAnimation();
        }
        break;

        case 19:

        inLookSubmenu = true;
        currentLookPage = 0;
        currentMode->pos[3] = 0;
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;

        case 38:

        drawMode = (drawMode == 0) ? 1 : 0;
        saveSingleModeToEEPROM(21, (int8_t)drawMode);
        drawMainSettingStatus(menuPosition);
        break;

        case 32:

        {
          extern bool showChannelNr;
          showChannelNr = !showChannelNr;
          saveSingleModeToEEPROM(18, 0);
          saveSingleModeToEEPROM(19, (int8_t)(showChannelNr ? 1 : 0));

          drawMainSettingStatus(menuPosition);
        }
        break;

        case 20:

        inRecsSubmenu = true;
        currentRecsPage = 0;
        currentMode->pos[3] = 0;
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;

        case 21:

        inMidiSubmenu = true;
        currentMidiPage = 0;
        currentMode->pos[3] = 0;
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;

        case 26:

        inVolSubmenu = true;
        currentVolPage = 0;
        currentMode->pos[3] = 0;
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;

        case 61:
        case 62:
        case 34:
        currentSystemMenuGroup = menuPosition == 61 ? SystemMenuGroup::Mode :
            (menuPosition == 62 ? SystemMenuGroup::Advanced : SystemMenuGroup::Etc);

        inEtcSubmenu = true;
        currentEtcPage = 0;
        currentMode->pos[3] = 0;
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;

        case 17:

        simpleNotesView = (simpleNotesView == 1) ? 2 : 1;
        saveSingleModeToEEPROM(11, simpleNotesView);
        drawMainSettingStatus(menuPosition);
        break;

        case 18:

        loopLength = loopLength + 1;
        if (loopLength > 8) loopLength = 0;
        saveSingleModeToEEPROM(12, loopLength);
        drawMainSettingStatus(menuPosition);
        break;

        case 54: {
        extern bool voiceMode;
        extern bool imageMode;
        if (imageMode) {

          voiceMode = false;
          saveSingleModeToEEPROM(42, 0);
        } else {
          voiceMode = !voiceMode;
          saveSingleModeToEEPROM(42, (int8_t)(voiceMode ? 1 : 0));
          updateLastPage();
        }
        drawMainSettingStatus(menuPosition);
        break;
        }

        case 22: {

        inEtcSubmenu = false;
        currentMenuPage = 8;
        extern Mode songMode;
        switchMode(&songMode);
        break;
        }

        case 23: {

        extern int ledModules;
        extern bool ledModulesRotated;
        extern unsigned int maxX;
        extern Mode draw;
        extern Mode singleMode;

        uint8_t ledMode = ledModules + (ledModulesRotated ? 2 : 0);
        switch (ledMode) {
          case 1: ledMode = 3; break;
          case 3: ledMode = 2; break;
          case 2: ledMode = 4; break;
          case 4: ledMode = 1; break;
          default: ledMode = 1; break;
        }

        ledModules = (ledMode == 1 || ledMode == 3) ? 1 : 2;
        ledModulesRotated = (ledMode >= 3);
        maxX = MATRIX_WIDTH * ledModules;


        extern void initLedStrip();
        initLedStrip();


        if (currentMode == &draw || currentMode == &singleMode) {
          int dynamicPages = (MAX_STEPS / maxX);
          Encoder[1].writeMax((int32_t)dynamicPages);
          Encoder[3].writeMax((int32_t)MAX_STEPS);


          if (currentMode->pos[1] > dynamicPages) {
            currentMode->pos[1] = dynamicPages;
            Encoder[1].writeCounter((int32_t)dynamicPages);
          }
          if (currentMode->pos[3] > MAX_STEPS) {
            currentMode->pos[3] = MAX_STEPS;
            Encoder[3].writeCounter((int32_t)MAX_STEPS);
          }
        }

        saveSingleModeToEEPROM(13, ledMode);
        drawMainSettingStatus(menuPosition);
        break;
        }

      case 24: {
        pong = !pong;
        drawMainSettingStatus(menuPosition);
        break;
      }

      case 25: {
        ctrlMode = ctrlMode ? 0 : 1;
        drawMainSettingStatus(menuPosition);
        refreshCtrlEncoderConfig();
        saveSingleModeToEEPROM(14, ctrlMode);
        break;
      }

      case 33: {

        if (previewTriggerMode == PREVIEW_MODE_ON) previewTriggerMode = PREVIEW_MODE_PRESS;
        else if (previewTriggerMode == PREVIEW_MODE_PRESS) previewTriggerMode = PREVIEW_MODE_SYNC;
        else previewTriggerMode = PREVIEW_MODE_ON;
        saveSingleModeToEEPROM(20, (int8_t)previewTriggerMode);
        drawMainSettingStatus(menuPosition);
        break;
      }

      case 40: {

        extern bool getLedStripEnabled();
        extern void setLedStripEnabled(bool enabled);
        bool currentState = getLedStripEnabled();
        bool newState = !currentState;
        setLedStripEnabled(newState);
        saveSingleModeToEEPROM(25, (int8_t)(newState ? 1 : 0));
        menuRequestFullRedraw();
        break;
      }

      case 56: {
        extern bool imageMode;
        extern void enforceImageModeConstraints();
        imageMode = !imageMode;
        saveSingleModeToEEPROM(48, (int8_t)(imageMode ? 1 : 0));
        if (imageMode) enforceImageModeConstraints();
        menuRequestFullRedraw();
        break;
      }

      case 57: {

        extern uint8_t eyesMode;
        eyesMode = (uint8_t)((eyesMode + 1) % 4);
        saveSingleModeToEEPROM(49, (int8_t)eyesMode);
        menuRequestFullRedraw();
        break;
      }

      case 52: {

        AudioMemoryUsageMaxReset();
        AudioProcessorUsageMaxReset();
        menuRequestFullRedraw();
        break;
      }

      case 53: {
        toggleMixGainEnc3Target();
        break;
      }

      case 43: {

        extern bool getSpkrEnabled();
        extern void setSpkrEnabled(bool enabled);
        bool currentState = getSpkrEnabled();
        bool newState = !currentState;
        setSpkrEnabled(newState);
        saveSingleModeToEEPROM(27, (int8_t)(newState ? 1 : 0));
        menuRequestFullRedraw();
        break;
      }

      case 64: {
        extern bool getAdcHpfEnabled();
        extern void setAdcHpfEnabled(bool);
        bool next = !getAdcHpfEnabled();
        setAdcHpfEnabled(next);
        saveSingleModeToEEPROM(56, (int8_t)(next ? 1 : 0));
        menuRequestFullRedraw();
        break;
      }

      case 65: {
        extern bool getUsbSmpPreview();
        extern void setUsbSmpPreview(bool);
        bool next = !getUsbSmpPreview();
        setUsbSmpPreview(next);
        saveSingleModeToEEPROM(57, (int8_t)(next ? 1 : 0));
        menuRequestFullRedraw();
        break;
      }

      case 41: {




        extern int currentEtcPage;
        int savedEtcPage = currentEtcPage;

        currentColorScheme = (currentColorScheme + 1) % 4;


        if (currentColorScheme == 3) {
          if (SD.exists("scheme.txt")) {
            File schemeFile = SD.open("scheme.txt", FILE_READ);
            if (schemeFile && schemeFile.size() >= 90) {
              uint8_t buf[90];
              int bytesRead = schemeFile.read(buf, 90);
              schemeFile.close();

              if (bytesRead == 90) {

                for (int i=0; i<15; i++) {
                  col[i] = CRGB(buf[i*3], buf[i*3+1], buf[i*3+2]);
                }


                for (int i=0; i<15; i++) {
                  int baseIdx = 45 + i*3;
                  col_base[i] = CRGB(buf[baseIdx], buf[baseIdx+1], buf[baseIdx+2]);
                }


                extern volatile bool colorsUpdatedViaSerial;
                colorsUpdatedViaSerial = true;

              } else {

                currentColorScheme = 0;
                applyColorScheme(0);
              }
            } else {

              currentColorScheme = 0;
              applyColorScheme(0);
            }
          } else {

            currentColorScheme = 0;
            applyColorScheme(0);
          }
        } else {

          applyColorScheme(currentColorScheme);


          extern volatile bool colorsUpdatedViaSerial;
          colorsUpdatedViaSerial = true;
        }


        saveSingleModeToEEPROM(22, (int8_t)currentColorScheme);


        currentEtcPage = savedEtcPage;
        currentMode->pos[3] = savedEtcPage;
        Encoder[3].writeCounter((int32_t)savedEtcPage);

        menuRequestFullRedraw();
        break;
      }

      case 42: {

        break;
      }
    }

}




FLASHMEM void resetNewModeState() {
  newScreenFirstEnter = true;
}




FLASHMEM void drawSimpleNotesView() {
  if (simpleNotesView != 1 && simpleNotesView != 2) simpleNotesView = 1;
  drawMenuValue(simpleNotesView == 1 ? "EASY" : "FULL", 2, 3, simpleNotesView == 1 ? CRGB(0, 255, 0) : CRGB(0, 0, 255));
}

FLASHMEM void drawLoopLength() {
  if (loopLength == 0) {
    drawMenuValue("OFF", 2, 3, UI_RED);
  } else {

    char buf[2];
    buf[0] = (char)('0' + constrain((int)loopLength, 1, 8));
    buf[1] = '\0';
    drawMenuValue(buf, 2, 3, UI_CYAN);
  }
}

FLASHMEM void drawLedModules() {
  extern int ledModules;
  extern bool ledModulesRotated;
  const char* lbl = (ledModules == 1) ? (ledModulesRotated ? "1B" : "1") : (ledModulesRotated ? "2B" : "2");
  drawMenuValue(lbl, 2, 3, CRGB(0, 255, 0));
}

FLASHMEM void showNewFileScreen() {

  switchMode(&newFileMode);
}


FLASHMEM void showNewFileMode() {
  FastLEDclear();


  CRGB newColor = CRGB(0, 255, 255);
  drawText("NEW", 6, 12, newColor);


  drawGenreSelection();


  if (genreType != 0) {
    int lengthLength = mapf(genreLength, 1, 16, 1, 16);
    for (int x = 1; x <= 16; x++) {
      if (x <= lengthLength) {
        light(x, 10, CRGB(255, 0, 255));
      } else {
        light(x, 10, CRGB(0, 0, 0));
      }
    }
  } else {

    for (int x = 1; x <= 16; x++) {
      light(x, 10, CRGB(0, 0, 0));
    }
  }




  drawMenuIndicator('M', 'G', 1, false, false);
  if (genreType != 0) {
    drawMenuIndicator('L', 'M', 2, false, false);
  }
  drawMenuIndicator('L', 'Y', 3, false, false);
  drawMenuIndicator('L', 'X', 4, false, false);

  CRGB pinkColor = getIndicatorColor('M');
  CRGB yellowColor = getIndicatorColor('Y');
  CRGB blueColor = getIndicatorColor('X');
  uint32_t pinkRgb = (uint32_t)pinkColor.r << 16 | (uint32_t)pinkColor.g << 8 | pinkColor.b;
  uint32_t yellowRgb = (uint32_t)yellowColor.r << 16 | (uint32_t)yellowColor.g << 8 | yellowColor.b;
  uint32_t blueRgb = (uint32_t)blueColor.r << 16 | (uint32_t)blueColor.g << 8 | blueColor.b;
  Encoder[0].writeRGBCode(0x00FF00);
  Encoder[1].writeRGBCode(genreType != 0 ? pinkRgb : 0x000000);
  Encoder[2].writeRGBCode(yellowRgb);
  Encoder[3].writeRGBCode(blueRgb);



  FastLEDshow();


  static int lastGenreType = -1;
  static int lastGenreLength = -1;


  if (newScreenFirstEnter) {

    Encoder[2].writeCounter((int32_t)genreType);
    Encoder[2].writeMax((int32_t)5);
    Encoder[2].writeMin((int32_t)0);
    currentMode->pos[2] = genreType;


    if (genreType != 0) {
      Encoder[1].writeCounter((int32_t)genreLength);
      Encoder[1].writeMax((int32_t)16);
      Encoder[1].writeMin((int32_t)1);
      currentMode->pos[1] = genreLength;
    } else {
      Encoder[1].writeCounter((int32_t)0);
      Encoder[1].writeMax((int32_t)0);
      Encoder[1].writeMin((int32_t)0);
      currentMode->pos[1] = 0;
    }


    Encoder[0].writeCounter((int32_t)0);
    Encoder[0].writeMax((int32_t)0);
    Encoder[0].writeMin((int32_t)0);
    Encoder[3].writeCounter((int32_t)0);
    Encoder[3].writeMax((int32_t)0);
    Encoder[3].writeMin((int32_t)0);
    currentMode->pos[0] = 0;
    currentMode->pos[3] = 0;

    lastGenreType = genreType;
    lastGenreLength = genreLength;
    newScreenFirstEnter = false;
  }


  int currentGenreType = currentMode->pos[2];
  if (currentGenreType != lastGenreType) {
    genreType = currentGenreType;
    if (genreType > 5) genreType = 5;
    if (genreType < 0) genreType = 0;


    if (genreType != currentGenreType) {
      Encoder[2].writeCounter((int32_t)genreType);
      currentMode->pos[2] = genreType;
    }

    drawGenreSelection();


    if (genreType != 0) {
      Encoder[1].writeCounter((int32_t)genreLength);
      Encoder[1].writeMax((int32_t)16);
      Encoder[1].writeMin((int32_t)1);
      currentMode->pos[1] = genreLength;
      Encoder[1].writeRGBCode(pinkRgb);
    } else {
      Encoder[1].writeCounter((int32_t)0);
      Encoder[1].writeMax((int32_t)0);
      Encoder[1].writeMin((int32_t)0);
      currentMode->pos[1] = 0;
      Encoder[1].writeRGBCode(0x000000);
    }

    lastGenreType = genreType;
  }


  if (genreType != 0) {
    int currentGenreLength = currentMode->pos[1];
    if (currentGenreLength != lastGenreLength) {
      genreLength = currentGenreLength;
      if (genreLength > 16) genreLength = 16;
      if (genreLength < 1) genreLength = 1;


      if (genreLength != currentGenreLength) {
        Encoder[1].writeCounter((int32_t)genreLength);
        currentMode->pos[1] = genreLength;
      }


      int lengthLength = mapf(genreLength, 1, 16, 1, 16);
      for (int x = 1; x <= 16; x++) {
        if (x <= lengthLength) {
          light(x, 10, CRGB(255, 0, 255));
        } else {
          light(x, 10, CRGB(0, 0, 0));
        }
      }
      FastLEDshow();

      lastGenreLength = genreLength;
    }
  }
}


FLASHMEM void enterEtcSdFromHost() {
  extern Mode menu;
  extern Mode *currentMode;
  extern void switchMode(Mode *newMode);

  if (currentMode != &menu) {
    switchMode(&menu);
  }
  if (currentMode != &menu) return;

  int sdPage = 0;
  for (int i = 0; i < ETC_PAGES_COUNT; i++) {
    if (etcPages[i].mainSetting == 49) {
      sdPage = i;
      break;
    }
  }
  int etcParent = 0;
  for (int i = 0; i < MENU_PAGES_COUNT; i++) {
    if (menuPages[i].mainSetting == 34) {
      etcParent = i;
      break;
    }
  }

  inLookSubmenu = false;
  inRecsSubmenu = false;
  inMidiSubmenu = false;
  inVolSubmenu = false;
  inEtcSubmenu = true;
  currentSystemMenuGroup = SystemMenuGroup::Etc;
  currentMenuPage = etcParent;
  currentEtcPage = sdPage;
  currentMode->pos[3] = sdPage;
  ensureEnc3Range(0, (int)(ETC_PAGES_COUNT - 1));
  Encoder[3].writeCounter((int32_t)sdPage);
  menuRequestFullRedraw();
}

FLASHMEM int getCurrentMenuMainSetting() {
  if (inLookSubmenu) {
    return lookPages[currentLookPage].mainSetting;
  }
  if (inRecsSubmenu) {
    return recsPages[currentRecsPage].mainSetting;
  }
  if (inMidiSubmenu) {
    return midiPages[currentMidiPage].mainSetting;
  }
  if (inVolSubmenu) {
    return volPages[currentVolPage].mainSetting;
  }
  if (inEtcSubmenu) {
    return activeSystemPages()[constrain(currentEtcPage, 0, activeSystemPageCount() - 1)].mainSetting;
  }
  return menuPages[currentMenuPage].mainSetting;
}



FLASHMEM void clearTextArea(int startX, int startY, int width) {
  for (int x = startX; x < startX + width && x <= (int)maxX; x++) {
    for (int y = startY; y < startY + 5 && y <= (int)maxY; y++) {
      light(x, y, CRGB(0, 0, 0));
    }
  }
}



static inline void drawMenuValue(const char* label, int x, int y, CRGB color) {
  clearTextArea(x, y, 16);
  drawText(label, x, y, menuValueColor(color));
}

FLASHMEM void drawRecChannelClear(){
  extern bool imageMode;
  int mode = imageMode ? 2 : recChannelClear;
  const char* lbl; CRGB col;
  switch (mode) {
    case 1: lbl = "ON"; col = UI_GREEN; SMP_REC_CHANNEL_CLEAR = true; break;
    case 0: lbl = "OFF"; col = UI_RED; SMP_REC_CHANNEL_CLEAR = false; break;
    case 2: lbl = "FIX"; col = UI_YELLOW; SMP_REC_CHANNEL_CLEAR = false; break;
    case 3: lbl = "ON1"; col = UI_CYAN; SMP_REC_CHANNEL_CLEAR = true; break;
    case 4: lbl = "CLIC";col = UI_MAGENTA;SMP_REC_CHANNEL_CLEAR = false; break;
    default: lbl = "OFF"; col = UI_RED; SMP_REC_CHANNEL_CLEAR = false; break;
  }
  drawMenuValue(lbl, 2, 3, col);
}

FLASHMEM void drawRecMode() {
  drawMenuValue(recMode == 1 ? "MIC" : "LINE", 2, 3, recMode == 1 ? UI_WHITE : UI_BLUE);
  recInput = (recMode == 1) ? AUDIO_INPUT_MIC : AUDIO_INPUT_LINEIN;
  sgtl5000_1.inputSelect(recInput);
  if (recInput == AUDIO_INPUT_MIC) sgtl5000_1.micGain(micGain);
  else sgtl5000_1.micGain(0);
  extern void applySgtl5000AdcHighPass();
  applySgtl5000AdcHighPass();
  FastLEDshow();
}

FLASHMEM void drawMidiVoiceSelect() {
  switch (voiceSelect) {
    case 1: drawMenuValue("MIDI", 2, 3, UI_BLUE); break;
    case 2: drawMenuValue("KEYS", 2, 3, UI_MAGENTA); break;
    default: drawMenuValue("SEL", 2, 3, UI_GREEN); break;
  }
  MIDI_VOICE_SELECT = (voiceSelect == 1 || voiceSelect == 2);
  FastLEDshow();
}

FLASHMEM void drawFastRecMode() {
  switch (fastRecMode) {
    case 4: drawMenuValue("+CON", 2, 3, UI_BLUE); break;
    case 3: drawMenuValue("-CON", 2, 3, UI_YELLOW); break;
    case 2: drawMenuValue("SENS", 2, 3, UI_CYAN); break;
    case 1: drawMenuValue("INST", 2, 3, UI_GREEN); break;
    default: drawMenuValue("OFF", 2, 3, UI_RED); break;
  }
  SMP_FAST_REC = fastRecMode;
  FastLEDshow();
}

FLASHMEM void drawPatternMode() {
  clearTextArea(10, 3, 8);
  switch (patternMode) {
    case 2: drawMenuValue("SONG", 2, 3, CRGB(255, 255, 0)); break;
    case 3: drawMenuValue("NEXT", 2, 3, CRGB(0, 255, 255)); break;
    case 1: drawMenuValue("ON", 2, 3, UI_GREEN); break;
    default: drawMenuValue("OFF", 2, 3, UI_RED); break;
  }
  SMP_PATTERN_MODE = (patternMode == 1 || patternMode == 2 || patternMode == 3);
  if (patternMode == 3) {
    extern bool isNowPlaying;
    unsigned int queuedPage =
        (isNowPlaying && GLOB.edit != GLOB.page) ? GLOB.edit : 0;
    if (queuedPage > 0) {
      char pageText[4];
      snprintf(pageText, sizeof(pageText), "%02d", (int)queuedPage);
      drawText(pageText, 10, 3, CRGB(0, 200, 200));
    }
  }
  FastLEDshow();
}

FLASHMEM void drawFlowMode() {
  if (flowMode != 1 && flowMode != -1) flowMode = -1;
  drawMenuValue(flowMode == 1 ? "ON" : "OFF", 2, 3, flowMode == 1 ? UI_GREEN : UI_RED);
  SMP_FLOW_MODE = (flowMode == 1);
  FastLEDshow();
}

FLASHMEM void drawMidiTransport() {
  if (transportMode != -1 && transportMode != 1 && transportMode != 2) transportMode = -1;
  switch (transportMode) {
    case 2: drawMenuValue("SEND", 2, 3, UI_BLUE); break;
    case 1: drawMenuValue("GET", 2, 3, UI_GREEN); break;
    default: drawMenuValue("OFF", 2, 3, UI_RED); break;
  }
  MIDI_TRANSPORT_RECEIVE = (transportMode == 1);
  MIDI_TRANSPORT_SEND = (transportMode == 2);
  FastLEDshow();
}

FLASHMEM void drawMidiSend() {
  extern int midiSendMode;
  extern bool MIDI_CLOCK_SEND;
  extern bool MIDI_NOTE_SEND;
  extern int clockMode;
  switch (midiSendMode) {
    case 0: drawMenuValue("CLCK", 2, 3, UI_YELLOW); MIDI_CLOCK_SEND = (clockMode == 1); MIDI_NOTE_SEND = false; break;
    case 1: drawMenuValue("NOTE", 2, 3, UI_GREEN); MIDI_CLOCK_SEND = false; MIDI_NOTE_SEND = true; break;
    default: drawMenuValue("BOTH", 2, 3, UI_BLUE); MIDI_CLOCK_SEND = (clockMode == 1); MIDI_NOTE_SEND = true; break;
  }
  FastLEDshow();
}

FLASHMEM void openTouchyMainMenu() {
  switchMode(&menu);
  if (currentMode != &menu) return;
  inLookSubmenu = inRecsSubmenu = inMidiSubmenu = inVolSubmenu = inEtcSubmenu = false;
  currentMenuPage = 8;
  menu.pos[3] = 8;
  Encoder[3].writeCounter((int32_t)8);
  invalidateEnc3RangeCache();
  menuRequestFullRedraw();
}
# 1 "/Users/jank/git/toern/toern_midi.ino"

extern float detune[13];
extern float channelOctave[9];
extern unsigned int maxX;
extern unsigned int pulseCount;
extern bool isNowPlaying;
extern bool MIDI_TRANSPORT_SEND;
extern bool SMP_PATTERN_MODE;
extern unsigned int beat;
extern int8_t transportSendDelayMs;
extern int8_t transportRcveDelayMs;


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


static unsigned long lastBPMMeasureTime = 0;
static uint32_t clocksSinceLastBPM = 0;
static float smoothedBPM = 0.0f;
static float bpmEstimate = 0.0f;
static float bpmEstimateError = 1.0f;
static const float BPM_PROCESS_NOISE = 0.8f;
static const float BPM_MEASUREMENT_NOISE = 0.5f;
static const uint32_t CLOCKS_PER_BPM_WINDOW = 24 * 4;
static const unsigned long NO_CLOCK_TIMEOUT_US = 2000000;
static unsigned long lastClockReceivedTime = 0;
static unsigned long lastMidiClockUs = 0;
static int lastStableBPM = 0;
static int stableBPMCount = 0;
static bool isBPMStable = false;
static uint8_t midiClockTicks = 0;
static uint16_t midiClockTickCounter = 0;
static bool initialBpmSyncDone = false;


bool getBPMStable() {
  return isBPMStable;
}



uint16_t getMidiClockTicks() {
  return midiClockTickCounter;
}
static uint8_t externalStepWithinPage = 0;
static unsigned long transportStartDelayUntil = 0;
static float midiClockSendBPM = 0.0f;
static unsigned long midiClockIntervalUs = 0;
static unsigned long midiNextClockMicros = 0;

static void markExternalOne() {
  midiClockTicks = 0;

  pulseCount = 0;
  if (maxX == 0) {
    externalStepWithinPage = 0;
    return;
  }
  externalStepWithinPage = 1;

  triggerExternalOneBlink();
}
static unsigned long lastClockSent = 0;

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

static const uint8_t PULSE_PPQN_TABLE[] = { 1, 2, 4, 8, 12, 16, 24, 32 };
static const uint8_t PULSE_PPQN_COUNT = sizeof(PULSE_PPQN_TABLE) / sizeof(PULSE_PPQN_TABLE[0]);
static const uint8_t PULSE_RATE_SEL_COUNT = PULSE_PPQN_COUNT + 1;

static const uint8_t ANALOG_IN_PPQN_TABLE[] = { 1, 2, 4, 8, 12, 16, 24, 32 };
static const uint8_t ANALOG_IN_PPQN_COUNT = sizeof(ANALOG_IN_PPQN_TABLE) / sizeof(ANALOG_IN_PPQN_TABLE[0]);

static bool pulseClockEnabled = false;
static bool pulseClockPolarityPositive = true;
static bool pulseClockStopWithPlay = false;
static uint8_t pulseClockWidthMs = PULSE_WIDTH_MS_DEFAULT;
static uint8_t pulseClockPpqnIndex = 6;
static uint16_t pulseClockPhase = 0;
static IntervalTimer pulseEndTimer;
static volatile bool pulseClockPinActive = false;
static IntervalTimer sidechainEndTimer;
static volatile bool sidechainPinActive = false;
static volatile uint32_t analogClockLastUs = 0;

bool analogClockPulseFresh() {
  if (!analogClockLastUs) return false;
  return ((micros() - analogClockLastUs) / 1000u) < 80u;
}
static volatile uint32_t analogClockFallUs = 0;
static volatile uint32_t analogClockPeriodUs = 0;
static volatile bool analogClockHigh = false;
static uint8_t analogInPpqnIndex = 2;
static volatile uint8_t analogSubstepsLeft = 0;
static uint8_t analogInHoldMs = ANALOG_IN_HOLD_MS_MIN;
static volatile uint32_t analogInHoldUs = ANALOG_IN_HOLD_MS_MIN * 1000u;
static volatile uint8_t analogStepPulse = 0;
static volatile uint8_t analogFillSubs = 0;

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

static uint8_t sidechainVoice = 1;
static bool analogExtClock = false;

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

  if (!analogExtClock || clockMode == 1) return;
  attachInterrupt(digitalPinToInterrupt(ANALOG_CLOCK_IN_PIN), analogClockIsr, CHANGE);
  NVIC_SET_PRIORITY(IRQ_GPIO6789, toern_audio::kTimerPriority);
}



void analogClockCatchIfHigh() {
  extern bool pendingStartOnBar;
  if (!analogExtClock || !pendingStartOnBar) return;
  noInterrupts();
  analogClockHigh = digitalReadFast(ANALOG_CLOCK_IN_PIN) != 0;


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


uint8_t getPulseClockRateSel() {
  if (!pulseClockEnabled) return 0;
  return (uint8_t)(pulseClockPpqnIndex + 1);
}

static void pulseClockSaveToEEPROM() {

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
  if (idx >= PULSE_PPQN_COUNT) idx = 6;
  pulseClockPpqnIndex = idx;
  pulseClockPolarityPositive = ((packed & 2) == 0);
  pulseClockStopWithPlay = ((packed & 0x20) != 0);
  uint8_t w = EEPROM.read(EEPROM_DATA_START + 30);
  if (w < PULSE_WIDTH_MS_MIN || w > PULSE_WIDTH_MS_MAX) w = PULSE_WIDTH_MS_DEFAULT;
  pulseClockWidthMs = w;
  bool on = (packed & 1) != 0;
  pulseClockEnabled = false;
  initAnalogClockHardware();
  if (on) setPulseClockEnabled(true);
}



void midiClockTick() {

  extern int clockMode;
  if (pulseClockEnabled && clockMode == 1) {
    pulseClockMidiTick();
  }


  if (MIDI_CLOCK_SEND) {



    Serial8.write(0xF8);

  }
}

static inline void configureMidiClockSend(float bpm, unsigned long nowMicros) {

  int roundedBPM = (int)round(bpm);
  if (roundedBPM < 1) roundedBPM = 1;
  if (roundedBPM > BPM_MAX) roundedBPM = BPM_MAX;




  double intervalUsDouble = 60000000.0 / ((double)roundedBPM * 24.0);
  midiClockIntervalUs = (unsigned long)round(intervalUsDouble);
  if (midiClockIntervalUs == 0) midiClockIntervalUs = 1;

  midiClockSendBPM = (float)roundedBPM;
  midiNextClockMicros = nowMicros;




  midiClockTimer.priority(toern_audio::kTimerPriority);
  midiClockTimer.begin(midiClockTick, midiClockIntervalUs);

}


void updateMidiClockOutput() {

  extern int clockMode;
  if (MIDI_CLOCK_SEND || (pulseClockEnabled && clockMode == 1)) {
    unsigned long now = micros();

    int roundedBPM = (int)round(SMP.bpm);
    if (roundedBPM < 1) roundedBPM = 1;
    if (roundedBPM > BPM_MAX) roundedBPM = BPM_MAX;
    configureMidiClockSend((float)roundedBPM, now);
  }
}


void handleMidiClock() {

  myClock(micros());
}

void checkMidi() {
  if (touchyEnabled) transportStartDelayUntil = 0;

  if (transportStartDelayUntil && micros() >= transportStartDelayUntil) {
    transportStartDelayUntil = 0;
    extern int patternMode;
    if (SMP_PATTERN_MODE && patternMode == 3) {


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






  static const uint8_t MAX_MIDI_MESSAGES_PER_LOOP = 16;
  uint8_t processedMessages = 0;
  while (processedMessages < MAX_MIDI_MESSAGES_PER_LOOP && MIDI.read()) {
    uint8_t miditype = MIDI.getType();

    if (miditype == midi::Clock) continue;
    if (miditype == midi::NoteOn) continue;

    if (miditype == midi::NoteOff) {
      uint8_t pitch = MIDI.getData1();
      uint8_t velocity = MIDI.getData2();
      logMidiRxNoteOff(MIDI.getChannel(), pitch, velocity);

      handleNoteOff(MIDI.getChannel(), pitch, velocity);
      continue;
    }


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







constexpr uint8_t clocksPerStep = 24 / 4;

void resetMidiClockState() {

  if (MIDI_CLOCK_SEND) {
    unsigned long now = micros();

    int roundedBPM = (int)round(SMP.bpm);
    if (roundedBPM < 1) roundedBPM = 1;



    int lastRoundedBPM = (int)round(midiClockSendBPM);
    if (midiClockIntervalUs == 0 || roundedBPM != lastRoundedBPM) {
      configureMidiClockSend((float)roundedBPM, now);
    }
    lastClockSent = now;

    if (SMP.bpm > 0.0f) {
        unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)SMP.bpm * 4.0));
        playTimer.begin(playNote, currentPlayNoteInterval);
    } else {
        playTimer.end();
    }
  } else {



    clocksSinceLastBPM = 0;
    lastBPMMeasureTime = 0;

    bpmEstimate = 0.0f;
    bpmEstimateError = 1.0f;


    lastStableBPM = 0;
    stableBPMCount = 0;
    isBPMStable = false;
    lastClockReceivedTime = 0;
    midiClockTickCounter = 0;
    initialBpmSyncDone = false;


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






void resetMidiClockForTransportStart() {

  if (!MIDI_CLOCK_SEND) return;

  unsigned long now = micros();
  int roundedBPM = (int)round(SMP.bpm);
  if (roundedBPM < 1) roundedBPM = 1;
  if (roundedBPM > BPM_MAX) roundedBPM = BPM_MAX;


  configureMidiClockSend((float)roundedBPM, now);
  lastClockSent = now;

  if (SMP.bpm > 0.0f) {
    unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)SMP.bpm * 4.0));
    playTimer.begin(playNote, currentPlayNoteInterval);
  } else {
    playTimer.end();
  }



  if (MIDI_TRANSPORT_SEND) {
    Serial8.write(0xFA);
    Serial8.write(0xF8);
  }
}




void myClock(unsigned long now_captured) {




  if (MIDI_CLOCK_SEND) {
    return;
  }
  if (analogExtClock) return;

  lastMidiClockUs = now_captured;


  pulseClockMidiTick();



  if (lastClockReceivedTime > 0 && (now_captured - lastClockReceivedTime) > NO_CLOCK_TIMEOUT_US) {

    bpmEstimate = 0.0f;
    bpmEstimateError = 1.0f;
    lastBPMMeasureTime = 0;
    clocksSinceLastBPM = 0;
    lastStableBPM = 0;
    stableBPMCount = 0;
    isBPMStable = false;
  }


  lastClockReceivedTime = now_captured;




  clocksSinceLastBPM++;

  if (lastBPMMeasureTime == 0) {

    lastBPMMeasureTime = now_captured;
    return;
  }



  pulseCount = (pulseCount + 1) % (24 * 4);

  midiClockTicks++;
  midiClockTickCounter++;
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

        triggerExternalOneBlink();

        if (pendingStartOnBar) {
          pendingStartOnBar = false;


          if (SMP_PATTERN_MODE) {
            beat = (GLOB.edit - 1) * maxX + 1;
            GLOB.page = GLOB.edit;
          } else {
            beat = 1;
            GLOB.page = 1;
          }
          playStartTime = millis();


          unsigned long delayUs = effectiveTransportRecvDelayMs() * 1000UL;
          transportStartDelayUntil = micros() + delayUs;
        }
      }
    }
  }



  if (clocksSinceLastBPM >= CLOCKS_PER_BPM_WINDOW) {

    unsigned long deltaTotal = now_captured - lastBPMMeasureTime;


    const unsigned long ABSOLUTE_MIN_DELTA = 7000;
    const unsigned long ABSOLUTE_MAX_DELTA = 75000;

    float intervalPerClock = (float)deltaTotal / (float)clocksSinceLastBPM;

    if (intervalPerClock >= ABSOLUTE_MIN_DELTA * 0.8f &&
        intervalPerClock <= ABSOLUTE_MAX_DELTA * 1.2f) {


      float rawBPM = 60000000.0f / (intervalPerClock * 24.0f);


      float clampedRawBPM = rawBPM;
      if (rawBPM < BPM_MIN) clampedRawBPM = BPM_MIN;
      if (rawBPM > BPM_MAX) clampedRawBPM = BPM_MAX;


      float filteredBPM;
      float kalmanGain;


      if (bpmEstimate == 0.0f) {
        filteredBPM = clampedRawBPM;
        bpmEstimateError = BPM_MEASUREMENT_NOISE;
        kalmanGain = 1.0f;
      } else {

        float predictedBPM = bpmEstimate;
        float predictedError = bpmEstimateError + BPM_PROCESS_NOISE;


        kalmanGain = predictedError / (predictedError + BPM_MEASUREMENT_NOISE);
        filteredBPM = predictedBPM + kalmanGain * (clampedRawBPM - predictedBPM);
        bpmEstimateError = (1.0f - kalmanGain) * predictedError;
      }


      bpmEstimate = filteredBPM;
      smoothedBPM = filteredBPM;


      int newBPM = round(filteredBPM);


      if (newBPM < BPM_MIN) newBPM = BPM_MIN;
      if (newBPM > BPM_MAX) newBPM = BPM_MAX;


      if (lastStableBPM == 0) {

        lastStableBPM = newBPM;
        stableBPMCount = 1;
        isBPMStable = false;
      } else if (newBPM == lastStableBPM) {

        stableBPMCount++;

        bool wasStable = isBPMStable;
        isBPMStable = (stableBPMCount > 2);


        if (isBPMStable && !wasStable) {
          extern IntervalTimer playTimer;
          extern void playNote();
          if (newBPM > 0) {
            unsigned long currentPlayNoteInterval = (unsigned long)lround(60000000.0 / ((double)newBPM * 4.0));



            if (!isNowPlaying && !pendingStartOnBar) {
              playTimer.begin(playNote, currentPlayNoteInterval);
            } else {
              playTimer.update(currentPlayNoteInterval);
            }
          }

          if (!initialBpmSyncDone) {
            initialBpmSyncDone = true;
          }


          extern Mode *currentMode;
          extern Mode bpmMode;
          if (currentMode == &bpmMode) {
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

        stableBPMCount = 1;
        lastStableBPM = newBPM;
        isBPMStable = false;
      }




      SMP.bpm = (float)newBPM;
    } else {
    }


    lastBPMMeasureTime = now_captured;
    clocksSinceLastBPM = 0;
  }


}

void MidiSendNoteOn(int pitch, int channel, int velocity) {

  extern bool MIDI_NOTE_SEND;
  if (!MIDI_NOTE_SEND) {
    return;
  }


  if (channel < 1) channel = 1;
  if (channel > 16) channel = 16;
  if (isChildVoiceDisabled(channel)) return;
  if (velocity < 0) velocity = 0;
  if (velocity > 127) velocity = 127;
  int note = constrain(pitch, 0, 127);


  MIDI.sendNoteOn(note, velocity, channel);



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




  unsigned long currentTime = millis();
  if (currentTime - playStartTime > 500) {
    pause(true);
  }
}


static int mapMidiToLogicalChannel(int midiChannel, uint8_t pitch) {
  extern int voiceSelect;
  extern bool MIDI_VOICE_SELECT;

  if (voiceSelect == 2) {

    int ch = ((int)pitch - 60) % 12 + 1;
    if (ch < 1) ch += 12;
    if (ch > 8) ch = 8;
    return ch;
  }
  if (!MIDI_VOICE_SELECT) {

    return (int)GLOB.currentChannel;
  }

  return midiChannel;
}



uint8_t midiPitchToGridRow(int channel, uint8_t pitch) {
  int anchorRow;
  if (channel == 11) {
    anchorRow = 12;
  } else if (channel == 13 || channel == 14) {
    anchorRow = channel - 11;
  } else {
    anchorRow = channel + 1;
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

      samplePitch = SampleRate[ch] * 12;
    } else if (preservePitch) {
      samplePitch = (SampleRate[ch] * 12) + (int)pitch - 72;
    } else {
      samplePitch = (SampleRate[ch] * 12) + (int)row - (ch + 1);
    }

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


  if (velocity == 0) {
    handleNoteOff((uint8_t)ch, pitch, velocity);
    return;
  }

  ch = mapMidiToLogicalChannel(ch, pitch);

  if (ch < 1 || ch > 16) return;
  if (isChildVoiceDisabled(ch)) return;


  if (pitch < 128 && !midiHeldNote[ch][pitch]) {
    midiHeldNote[ch][pitch] = true;
    midiHeldVelocity[ch][pitch] = velocity;
  }

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

    activeNotes[pitch] = true;
  } else {



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
  if (!MIDI_NOTE_RECEIVE) return;

  int channel = mapMidiToLogicalChannel((int)midiChannel, pitch);


  if (channel >= 11 && channel <= 14) {
    if (pitch < 128) {
      midiHeldNote[channel][pitch] = false;
      midiHeldVelocity[channel][pitch] = 0;
    }
    {
      int cnt = 0;
      for (int p = 0; p < 128; p++) {
        if (midiHeldNote[channel][p]) cnt++;
      }
      pressedKeyCount[channel] = (int16_t)cnt;
    }

    if (channel == 11) {



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
    return;
  }


}
void handleStart() {
  if (touchyEnabled) return;
  logMidiRxSimple("Start");

  if (!MIDI_TRANSPORT_RECEIVE) return;

  markExternalOne();




  if (SMP_PATTERN_MODE) {
    beat = (GLOB.edit - 1) * maxX + 1;
    GLOB.page = GLOB.edit;
  } else {
    beat = 1;
    GLOB.page = 1;
  }
  deleteActiveCopy();

  if (MIDI_CLOCK_SEND && MIDI_TRANSPORT_SEND) {


    MIDI.sendRealTime(midi::Start);
  }

  if (!MIDI_CLOCK_SEND && analogExtClock) {
    pendingStartOnBar = true;
    analogClockCatchIfHigh();
    return;
  }


  pendingStartOnBar = false;

  unsigned long delayUs = effectiveTransportRecvDelayMs() * 1000UL;
  transportStartDelayUntil = micros() + delayUs;
}

void armMasterTransportStartDelay() {
  transportStartDelayUntil = micros() + effectiveTransportSendDelayMs() * 1000UL;
}

void handleTimeCodeQuarterFrame(uint8_t data) {


}

void handleSongPosition(uint16_t beats) {



}
# 1 "/Users/jank/git/toern/toern_parameters.ino"

extern uint16_t attackAmp[2][3];
extern uint16_t decayAmp[2][3];
extern float sustainAmp[2][3];
extern uint16_t releaseAmp[2][3];
extern uint16_t releaseFilter[2][3];
extern void updateVals(int ch);


void setParams(ParameterType paramType, int index) {

  float mappedValue;
  switch (paramType) {
    case DELAY:
      if (channelHasVoiceDelay(index)) {
        extern void applyVoiceDelay(int ch);
        applyVoiceDelay(index);
      }
      break;
    case ATTACK:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, maxParamVal[ATTACK], 0);
      if (index == 11) {
        for (int v = 0; v < 3; v++) attackAmp[0][v] = (uint16_t)mappedValue;
        updateVals(0);
      } else {
        envelopes[index]->attack(mappedValue);
      }
      break;
    case DECAY:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[DECAY]);
      if (index == 11) {
        for (int v = 0; v < 3; v++) decayAmp[0][v] = (uint16_t)mappedValue;
        updateVals(0);
      } else {
        envelopes[index]->decay(mappedValue);
      }
      break;




    case SUSTAIN:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[SUSTAIN]);
      if (index == 11) {
        for (int v = 0; v < 3; v++) sustainAmp[0][v] = mappedValue;
        updateVals(0);
      } else {
        envelopes[index]->sustain(mappedValue);
      }
      break;
    case RELEASE:
      mappedValue = mapf(SMP.param_settings[index][paramType], 0, maxfilterResolution, 0, maxParamVal[RELEASE]);
      if (index == 11) {
        uint16_t relMs = (uint16_t)mappedValue;
        uint16_t fltRelMs = relMs < 180 ? 180 : relMs;
        for (int v = 0; v < 3; v++) {
          releaseAmp[0][v] = relMs;
          releaseFilter[0][v] = fltRelMs;
        }
        updateVals(0);
      } else {
        envelopes[index]->release(mappedValue);
      }
      break;
  }
}


void handleWaveformChange(int index, unsigned int waveformType) {

  if (synths[index][0] != nullptr) {
    switch (waveformType) {
      case 1:
        synths[index][0]->begin(WAVEFORM_SINE);
        break;
      case 2:
        synths[index][0]->begin(WAVEFORM_SQUARE);
        break;
      case 3:
        synths[index][0]->begin(WAVEFORM_SAWTOOTH);
        break;
      case 4:
        synths[index][0]->begin(WAVEFORM_TRIANGLE);
        break;
      default: return;
    }
  }
}

#define DEFAULT_CH11_INSTRUMENT 12

static const uint8_t kCh11Control1Default[13] = {
  8, 20, 12, 18, 5, 24, 20, 22, 26, 9,
  14, 20, 14,
};
static const uint8_t kCh11Control2Default[13] = {
  5, 4, 2, 10, 24, 1, 22, 14, 6, 10,
  12, 6, 10,
};
static const uint8_t kCh11Control3Default[13] = {
  12, 8, 2, 3, 20, 2, 2, 14, 4, 18,
  12, 10, 12,
};
static const uint8_t kCh11AttackDefault[13] = {
  32, 32, 32, 8, 26, 32, 28, 32, 32, 30,
  32, 32, 32,
};
static const uint8_t kCh11DecayDefault[13] = {
  6, 14, 1, 32, 32, 2, 0, 5, 0, 28,
  18, 28, 16,
};
static const uint8_t kCh11SustainDefault[13] = {
  0, 2, 28, 20, 26, 28, 31, 0, 31, 26,
  18, 20, 16,
};
static const uint8_t kCh11ReleaseDefault[13] = {
  2, 6, 2, 24, 20, 3, 10, 5, 1, 10,
  12, 16, 12,
};
static const uint8_t kCh11CentSliderDefault[13] = {
  5,
  14,
  14,
  14,
  14,
  18,
  18,
  14,
  18,
  14,
  5,
  14,
  5,
};
static const uint8_t kCh11FormDefault[13] = {
  0, 8, 4, 16, 12, 16, 12, 8, 4, 0,
  0, 12, 4,
};
static const uint8_t kCh11WaveDefault[13] = {
  8, 4, 4, 0, 12, 0, 12, 4, 4, 8,
  8, 12, 4,
};


static int defaultSettingValue(int ch, SettingArray arr, int8_t idx) {
  int ch11Inst = constrain((int)SMP.synth_settings[11][INSTRUMENT], 0, 12);
  switch (arr) {
    case ARR_FILTER:
      switch (idx) {
        case HCUT: return (ch == 14) ? 14 : 32;
        case LOWCUT: return 0;
        case REVERB: return 0;
        case BITCRUSHER: return 0;
        case DETUNE: return 16;
        case OCTAVE: return (ch >= 1 && ch <= 8) ? 24 : 16;
        case RES: return (ch == 14) ? 5 : 0;
        case EFX: return 0;
        case FILTER_WAVEFORM:
          return (ch == 11) ? kCh11WaveDefault[ch11Inst] : 8;
        default: return 0;
      }
    case ARR_PARAM:

      switch (idx) {
        case DELAY: return 0;
        case ATTACK:
          return (ch == 11) ? kCh11AttackDefault[ch11Inst] : 32;
        case DECAY:
          if (ch == 11) return kCh11DecayDefault[ch11Inst];
          return (ch == 14) ? 20 : 32;
        case SUSTAIN:
          if (ch == 11) return kCh11SustainDefault[ch11Inst];
          return (ch == 14) ? 20 : 32;
        case RELEASE:
          if (ch == 11) return kCh11ReleaseDefault[ch11Inst];
          return (ch == 13 || ch == 14) ? 16 : 0;
        default: return 0;
      }
    case ARR_SYNTH:
      switch (idx) {
        case CUTOFF:
          if (ch == 11) return kCh11Control1Default[ch11Inst];
          return (ch == 13) ? 32 : ((ch == 14) ? 14 : 16);
        case RESONANCE:
          if (ch == 11) return kCh11Control2Default[ch11Inst];
          return (ch == 14) ? 5 : 0;
        case FILTER:
          if (ch == 11) return kCh11Control3Default[ch11Inst];
          return (ch == 14) ? 8 : 0;
        case CENT:
          if (ch == 11) return kCh11CentSliderDefault[ch11Inst];
          if (ch == 13) return 0;
          if (ch == 14) return 8;
          return 16;
        case SEMI: return 16;
        case INSTRUMENT: return DEFAULT_CH11_INSTRUMENT;
        case FORM:
          return (ch == 11) ? kCh11FormDefault[ch11Inst] : 0;
        case LFO_RATE: return 0;
        case LFO_DEPTH: return 0;
        case LFO_PHASE: return 0;
        case ARP_STEP: return 16;
        default: return 0;
      }
    case ARR_STRETCH:
      return 11;
    default:
      return 0;
  }
}


static void applyDefaultAdsrSettings(int ch) {
  SMP.param_settings[ch][ATTACK] = defaultSettingValue(ch, ARR_PARAM, ATTACK);
  SMP.param_settings[ch][DECAY] = defaultSettingValue(ch, ARR_PARAM, DECAY);
  SMP.param_settings[ch][SUSTAIN] = defaultSettingValue(ch, ARR_PARAM, SUSTAIN);
  SMP.param_settings[ch][RELEASE] = defaultSettingValue(ch, ARR_PARAM, RELEASE);
  setParams(ATTACK, ch);
  setParams(DECAY, ch);
  setParams(SUSTAIN, ch);
  setParams(RELEASE, ch);
}


static void setFilterPageDefaultValues(int ch, uint8_t page) {
  if (ch < 0 || ch >= NUM_CHANNELS) return;
  extern SliderDefEntry sliderDef[NUM_CHANNELS][5][4];

  bool touchedSynth = false;
  bool touchedAdsr = false;

  for (uint8_t i = 0; i < 4; ++i) {
    const SliderDefEntry& d = sliderDef[ch][page][i];
    if (d.arr == ARR_NONE || d.idx < 0) continue;
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) continue;

    const int val = defaultSettingValue(ch, d.arr, d.idx);

    switch (d.arr) {
      case ARR_FILTER:
        filterSetting(ch, d.idx) = val;
        if (d.idx != EFX) {
          setFilters((FilterType)d.idx, ch, true);
        }
        break;
      case ARR_PARAM:
        SMP.param_settings[ch][d.idx] = val;
        setParams((ParameterType)d.idx, ch);
        if (d.idx == ATTACK || d.idx == DECAY || d.idx == SUSTAIN || d.idx == RELEASE) {
          touchedAdsr = true;
        }
        break;
      case ARR_SYNTH:
        SMP.synth_settings[ch][d.idx] = val;
        if (ch == 11 && d.idx == INSTRUMENT) {
          applySynthInstrumentPreset(ch, val);
        }
        if ((ch == 11 || ch == 13 || ch == 14) && d.idx == ARP_STEP) {
          extern void midiArpResetVoice(int ch);
          midiArpResetVoice(ch);
        }
        touchedSynth = true;
        break;
      case ARR_STRETCH:
        setSampleTimeStretch(ch, val);
        break;
      default:
        break;
    }
  }


  if (touchedAdsr) {
    applyDefaultAdsrSettings(ch);
  }

  if (touchedSynth) {
    if (ch == 11) updateSynthVoice(11);
    if (ch == 13 || ch == 14) {
      extern void applyMonoSynthCrf(int ch);
      applyMonoSynthCrf(ch);
    }
  }
}

void setCurrentFilterPageDefaultValues(int ch) {
  if (ch < 0 || ch >= NUM_CHANNELS) return;
  extern uint8_t filterPage[NUM_CHANNELS];
  const uint8_t page = filterPage[ch];
  setFilterPageDefaultValues(ch, page);
  initSliders(page, ch);
}




void setAllFilterPagesDefaultValues(int ch) {
  if (ch < 0 || ch >= NUM_CHANNELS) return;
  extern uint8_t filterPage[NUM_CHANNELS];
  extern const uint8_t filterPageCount[NUM_CHANNELS];

  int pages = filterPageCount[ch];
  if (pages <= 0) pages = 4;
  if (pages > 5) pages = 5;
  for (int p = 0; p < pages; ++p) {
    setFilterPageDefaultValues(ch, (uint8_t)p);
  }

  SMP.filter_settings[ch][EFX] = 0;


  SMP.param_settings[ch][DELAY] = 0;
  setParams(DELAY, ch);
  if (ch >= 1 && ch <= 8) {
    setSampleTimeStretch(ch, 11);
  }
  if (ch == 11) {
    SMP.synth_settings[11][INSTRUMENT] = DEFAULT_CH11_INSTRUMENT;
    applySynthInstrumentPreset(11, DEFAULT_CH11_INSTRUMENT);
  }
  if (ch == 11 || ch == 13 || ch == 14) {
    SMP.synth_settings[ch][LFO_RATE] = 0;
    SMP.synth_settings[ch][LFO_DEPTH] = 0;
    SMP.synth_settings[ch][LFO_PHASE] = 0;
    SMP.synth_settings[ch][ARP_STEP] = 16;
    extern void midiArpResetVoice(int ch);
    midiArpResetVoice(ch);
  }


  applyDefaultAdsrSettings(ch);

  if (ch == 11) updateSynthVoice(11);
  if (ch == 13 || ch == 14) {
    extern void applyMonoSynthCrf(int ch);
    applyMonoSynthCrf(ch);
  }
  if (currentMode == &filterMode && ch == (int)GLOB.currentChannel) {
    initSliders(filterPage[ch], ch);
  }
}

static uint8_t synthInstrumentFormDefault(int instrumentIdx) {
  return kCh11FormDefault[constrain(instrumentIdx, 0, 12)];
}

uint8_t synthInstrumentWaveDefault(int instrumentIdx) {
  return kCh11WaveDefault[constrain(instrumentIdx, 0, 12)];
}

void applySynthInstrumentFormDefault(int channel, int instrumentIdx) {
  if (channel != 11) return;
  SMP.synth_settings[channel][FORM] =
      synthInstrumentFormDefault(instrumentIdx);
}




void applySynthInstrumentPreset(int channel, int instrumentIdx) {
  if (channel != 11) return;
  instrumentIdx = constrain(instrumentIdx, 0, 12);

  SMP.synth_settings[channel][INSTRUMENT] = instrumentIdx;
  SMP.synth_settings[channel][CUTOFF] = kCh11Control1Default[instrumentIdx];
  SMP.synth_settings[channel][RESONANCE] = kCh11Control2Default[instrumentIdx];
  SMP.synth_settings[channel][FILTER] = kCh11Control3Default[instrumentIdx];
  SMP.synth_settings[channel][SEMI] = 16;
  SMP.synth_settings[channel][CENT] = kCh11CentSliderDefault[instrumentIdx];
  filterSetting(channel, FILTER_WAVEFORM) =
      synthInstrumentWaveDefault(instrumentIdx);
  applySynthInstrumentFormDefault(channel, instrumentIdx);
  SMP.param_settings[channel][ATTACK] = kCh11AttackDefault[instrumentIdx];
  SMP.param_settings[channel][DECAY] = kCh11DecayDefault[instrumentIdx];
  SMP.param_settings[channel][SUSTAIN] = kCh11SustainDefault[instrumentIdx];
  SMP.param_settings[channel][RELEASE] = kCh11ReleaseDefault[instrumentIdx];
}


void resetAllToDefaults() {

  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);

  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    setAllFilterPagesDefaultValues(ch);
  }


  extern float octave[2];
  octave[0] = -1.0f;
  octave[1] = 0.0f;


  extern void resetAllAudioEffects();
  resetAllAudioEffects();

  extern void resetAllChannelVolumesToDefault();
  resetAllChannelVolumesToDefault();

  showSaveSuccessAnimation();
}
# 1 "/Users/jank/git/toern/toern_sample.ino"

extern int8_t channelDirection[maxFiles];
extern uint16_t g_wavPickCount;
extern uint16_t g_folderPickCount;
extern int g_wavFileCount;
extern char g_wavPickName[][65];
extern uint8_t g_wavPickType[];
extern uint32_t g_wavPickSize[];
extern char g_folderPickName[][65];
extern char g_browseDir[][128];
extern bool g_browseListDirty;
extern void sampleBrowserRefreshList(int channel);
extern void sampleBrowserSyncBrowseFromStoredPath(int channel);
extern unsigned int sampleBrowserHashName(const char*);
extern uint8_t sampleBrowserEntryTypeAt(int idx);
FLASHMEM void previewSample(bool setMaxSampleLength);
FLASHMEM void previewSample(bool setMaxSampleLength, bool playAudio);
FLASHMEM void previewSampleNow();
void generateNextNumericName(int folderIdx, char* outName, size_t outSize);
extern int previewTriggerMode;
extern const int PREVIEW_MODE_ON;
extern const int PREVIEW_MODE_PRESS;
extern const int PREVIEW_MODE_SYNC;
extern bool g_suppressNextWavPreviewAfterFolderNav;
extern void sampleBrowserClampBrowseIndex(int channel);
extern void updatePreviewVolume();
extern bool encoderI2cWritesAllowed();
extern bool isNowPlaying;


static int lastEncoder3Value_forShowWave = -1;


static char g_queuedSdPreviewPath[160];
static bool g_queuedSdPreview = false;
static elapsedMillis g_sdPreviewQuietMs;
static bool g_previewPlayImmediate = false;
static const unsigned SD_PREVIEW_SETTLE_MS = 70;

bool g_previewUseLivePeaks = true;

static inline bool previewPlaysOnSelect() {
  return previewTriggerMode == PREVIEW_MODE_ON ||
         (previewTriggerMode == PREVIEW_MODE_SYNC && !isNowPlaying);
}



struct PeakScanState {
  File f;
  bool active = false;
  char path[160] = {0};
  uint32_t dataStart = 0;
  uint32_t dataSize = 0;
  uint32_t totalSamples = 0;
  uint32_t sampleIdx = 0;
  uint16_t outIndex = 0;
  uint32_t samplesInWindow = 0;
  int16_t windowPeak = 0;
};

static PeakScanState g_peakScan;
static bool g_encoder0PressedMode = false;


bool isEncoder0PressedMode() {
  return g_encoder0PressedMode;
}

void setEncoder0PressedMode(bool state) {
  g_encoder0PressedMode = state;
}

extern Mode set_Wav;
extern Mode *currentMode;

volatile bool g_pendingSyncPreviewPlay = false;

bool shouldSkipVoiceTriggerForSyncPreview(int ch) {
  if (previewTriggerMode != PREVIEW_MODE_SYNC) return false;
  if (currentMode != &set_Wav) return false;
  if (ch < 1 || ch > 8) return false;
  if (ch != (int)GLOB.currentChannel) return false;
  return sampleIsLoaded;
}

void triggerSetWavSyncPreview(uint8_t vel) {
  (void)vel;
  if (!sampleIsLoaded) return;
  g_pendingSyncPreviewPlay = true;
}

void serviceSetWavSyncPreview() {
  if (!g_pendingSyncPreviewPlay) return;
  g_pendingSyncPreviewPlay = false;
  if (currentMode != &set_Wav) return;
  if (previewTriggerMode != PREVIEW_MODE_SYNC) return;
  g_previewPlayImmediate = true;
  previewSample(false);
}

static bool buildCurrentBrowseSamplePath(char *out, size_t outSize) {
  if (!out || outSize == 0) return false;
  out[0] = '\0';
  int ch = constrain((int)GLOB.currentChannel, 1, 8);
  int idx = (int)currentMode->pos[3] - 1;
  if (idx < 0 || idx >= (int)g_wavPickCount) return false;
  if (sampleBrowserEntryTypeAt(idx) != 2) return false;
  if (g_browseDir[ch][0]) snprintf(out, outSize, "samples/%s/%s", g_browseDir[ch], g_wavPickName[idx]);
  else snprintf(out, outSize, "samples/%s", g_wavPickName[idx]);
  return true;
}




static bool commitCurrentBrowsePathToChannel(int ch) {
  ch = constrain(ch, 1, 8);
  int idx = (int)currentMode->pos[3] - 1;
  if (idx < 0 || idx >= (int)g_wavPickCount) return false;
  if (sampleBrowserEntryTypeAt(idx) != 2) return false;
  char rel[128];
  if (g_browseDir[ch][0]) snprintf(rel, sizeof(rel), "%s/%s", g_browseDir[ch], g_wavPickName[idx]);
  else snprintf(rel, sizeof(rel), "%s", g_wavPickName[idx]);
  strncpy(SMP.samplePathRel[ch], rel, 127);
  SMP.samplePathRel[ch][127] = 0;
  return true;
}




volatile bool channelSampleReloadBusy[maxFiles] = { false };

void stopSdPreviewIfPlaying() {
  extern AudioPlaySdWav playSdWav1;
  extern AudioEffectEnvelope envelope0;
  if (playSdWav1.isPlaying()) {
    playSdWav1.stop();
  }
  envelope0.noteOff();
}

static void stopSdPreviewAndWait() {
  const bool needSettle = g_peakScan.active || playSdWav1.isPlaying();
  stopPeakScan();
  stopSdPreviewIfPlaying();
  if (!needSettle) {
    previewIsPlaying = false;
    return;
  }
  elapsedMicros waited;
  while (playSdWav1.isPlaying() && waited < 10000) {
    yield();
  }

  delayMicroseconds(3000);
  previewIsPlaying = false;
}

static void startSdPreviewNow(const char *path) {
  if (!path || !path[0]) return;
  g_queuedSdPreview = false;
  stopSdPreviewAndWait();
  updatePreviewVolume();
  previewIsPlaying = true;
  sampleIsLoaded = true;
  playSdWav1.play(path);
}

static void requestSdPreview(const char *path) {
  if (!path || !path[0]) return;
  strncpy(g_queuedSdPreviewPath, path, sizeof(g_queuedSdPreviewPath) - 1);
  g_queuedSdPreviewPath[sizeof(g_queuedSdPreviewPath) - 1] = 0;
  g_queuedSdPreview = true;
  g_sdPreviewQuietMs = 0;
}

void serviceSdPreviewRequests() {
  if (!g_queuedSdPreview) return;
  if (g_sdPreviewQuietMs < SD_PREVIEW_SETTLE_MS) return;
  g_queuedSdPreview = false;
  startSdPreviewNow(g_queuedSdPreviewPath);
}



static volatile bool sdIoKeepAudioFed = false;

void sdIoBeginAudioSafe() { sdIoKeepAudioFed = true; }
void sdIoEndAudioSafe() { sdIoKeepAudioFed = false; }

size_t sdIoChunkSize() {
  extern bool isNowPlaying;

  return (isNowPlaying || sdIoKeepAudioFed) ? (size_t)8192 : (size_t)65536;
}

void sdIoYield() {
  extern bool isNowPlaying;
  yield();

  if (isNowPlaying || sdIoKeepAudioFed) {
    delayMicroseconds(80);
  }
}

bool isChannelSampleReloadBusy(unsigned int ch) {
  if (ch >= maxFiles) return false;
  return channelSampleReloadBusy[ch];
}

void beginChannelSampleReload(unsigned int ch) {
  if (ch >= maxFiles) return;
  toern_audio::StateGuard guard;
  channelSampleReloadBusy[ch] = true;

  if (voices[ch]) voices[ch]->stop();

  for (int note = 36; note <= 96; note++) {
    samplerNoteEvent(ch, note, 0, false, false);
  }
  if (ch < 15 && envelopes[ch] != nullptr) {
    envelopes[ch]->noteOff();
  }
  samplerRemoveSamples(ch);
}

void endChannelSampleReload(unsigned int ch) {
  if (ch >= maxFiles) return;
  channelSampleReloadBusy[ch] = false;
}

static bool findWavDataChunk(File &f, uint32_t &outStart, uint32_t &outSize) {
  outStart = 0;
  outSize = 0;
  uint8_t hdr[12];
  f.seek(0);
  if (f.read(hdr, sizeof(hdr)) != (int)sizeof(hdr)) {
    return false;
  }
  if (!(hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F' && hdr[8]=='W' && hdr[9]=='A' && hdr[10]=='V' && hdr[11]=='E')) {
    return false;
  }


  uint32_t riffSize = (uint32_t)hdr[4] | ((uint32_t)hdr[5] << 8) | ((uint32_t)hdr[6] << 16) | ((uint32_t)hdr[7] << 24);
  uint32_t fileSize = f.size();
  uint32_t expectedRiffSize = fileSize - 8;

  if (riffSize != expectedRiffSize) {
  }

  f.seek(12);
  uint32_t maxIterations = 100;
  uint32_t iterations = 0;
  while (f.available() && iterations < maxIterations) {
    iterations++;
    char id[4];
    if (f.read((uint8_t*)id, 4) != 4) {
      break;
    }
    uint8_t szb[4];
    if (f.read(szb, 4) != 4) {
      break;
    }
    uint32_t sz = (uint32_t)szb[0] | ((uint32_t)szb[1] << 8) | ((uint32_t)szb[2] << 16) | ((uint32_t)szb[3] << 24);


    uint32_t currentPos = f.position();
    if (sz > fileSize || (currentPos + sz) > fileSize) {
      break;
    }

    if (id[0]=='d' && id[1]=='a' && id[2]=='t' && id[3]=='a') {
      outStart = currentPos;
      outSize = sz;
      return (outSize >= 2);
    }


    uint32_t skip = sz + (sz & 1);
    uint32_t nextPos = currentPos + skip;
    if (nextPos > fileSize || !f.seek(nextPos)) {
      break;
    }
  }

  if (iterations >= maxIterations) {
  } else {
  }
  return false;
}

static void stopPeakScan() {
  if (g_peakScan.active) {
    if (g_peakScan.f) {
      g_peakScan.f.close();
    }
    g_peakScan.active = false;
    yield();
  }
}


void startPeakScan(const char *path, bool resetPeaks = true) {
  stopPeakScan();
  if (playSdWav1.isPlaying() || g_queuedSdPreview) return;
  strncpy(g_peakScan.path, path, sizeof(g_peakScan.path) - 1);
  g_peakScan.path[sizeof(g_peakScan.path) - 1] = 0;

  if (!SD.exists(g_peakScan.path)) return;
  g_peakScan.f = SD.open(g_peakScan.path, FILE_READ);
  if (!g_peakScan.f) return;

  uint32_t ds = 0, dz = 0;
  if (!findWavDataChunk(g_peakScan.f, ds, dz)) {

    ds = 44;
    dz = (uint32_t)max(0, (int)g_peakScan.f.size() - 44);
  }
  if (dz < 2) { g_peakScan.f.close(); return; }

  g_peakScan.dataStart = ds;
  g_peakScan.dataSize = dz;
  g_peakScan.totalSamples = dz / 2;

  if (resetPeaks) {
    g_peakScan.sampleIdx = 0;
    g_peakScan.outIndex = 0;
    peakIndex = 0;
    memset(peakValues, 0, sizeof(peakValues));
  } else {
    g_peakScan.outIndex = (uint16_t)peakIndex;
    g_peakScan.sampleIdx = (uint32_t)peakIndex * 640;
    if (g_peakScan.sampleIdx >= g_peakScan.totalSamples) {
       stopPeakScan();
       return;
    }
  }

  g_peakScan.samplesInWindow = 0;
  g_peakScan.windowPeak = 0;
  g_peakScan.active = true;

  g_peakScan.f.seek(g_peakScan.dataStart + (g_peakScan.sampleIdx * 2));
}

static void servicePeakScan(uint32_t maxBytesThisCall) {
  if (!g_peakScan.active) return;

  if (playSdWav1.isPlaying()) {
    stopPeakScan();
    return;
  }

  if (!g_peakScan.f.available()) { stopPeakScan(); return; }

  const uint32_t WINDOW_SAMPLES = 640;
  uint8_t buf[2048];
  uint32_t bytesLeft = maxBytesThisCall;
  uint32_t bytesProcessed = 0;
  const uint32_t YIELD_INTERVAL = 2048;

  while (bytesLeft > 0 && g_peakScan.active && g_peakScan.outIndex < (uint16_t)maxPeaks) {

    if (bytesProcessed >= YIELD_INTERVAL) {
      yield();
      bytesProcessed = 0;
    }

    uint32_t toRead = min((uint32_t)sizeof(buf), bytesLeft);
    int n = g_peakScan.f.read(buf, (int)toRead);
    if (n <= 0) { stopPeakScan(); break; }
    bytesLeft -= (uint32_t)n;
    bytesProcessed += (uint32_t)n;

    int samplesInChunk = n / 2;
    for (int i = 0; i < samplesInChunk; i++) {
      int lo = buf[i * 2];
      int hi = (int8_t)buf[i * 2 + 1];
      int16_t v = (int16_t)((hi << 8) | lo);
      int16_t a = (v < 0) ? (int16_t)(-v) : v;
      if (a > g_peakScan.windowPeak) g_peakScan.windowPeak = a;

      g_peakScan.sampleIdx++;
      g_peakScan.samplesInWindow++;

      if (g_peakScan.samplesInWindow >= WINDOW_SAMPLES) {

        if (g_peakScan.outIndex >= (uint16_t)maxPeaks) {
          stopPeakScan();
          break;
        }
        peakValues[g_peakScan.outIndex] = (float)g_peakScan.windowPeak / 32768.0f;
        g_peakScan.outIndex++;
        g_peakScan.samplesInWindow = 0;
        g_peakScan.windowPeak = 0;
        peakIndex = (int)g_peakScan.outIndex;
        if (g_peakScan.outIndex >= (uint16_t)maxPeaks) { stopPeakScan(); break; }
      }

      if (g_peakScan.sampleIdx >= g_peakScan.totalSamples) {
        if (g_peakScan.samplesInWindow > 0 && g_peakScan.outIndex < (uint16_t)maxPeaks) {
          peakValues[g_peakScan.outIndex] = (float)g_peakScan.windowPeak / 32768.0f;
          g_peakScan.outIndex++;
          peakIndex = (int)g_peakScan.outIndex;
        }
        stopPeakScan();
        break;
      }
    }
  }
}


void stopAllSetWavPreviewAudio() {
  g_queuedSdPreview = false;
  g_pendingSyncPreviewPlay = false;
  stopSdPreviewAndWait();
  envelope0.noteOff();
  samplerRemoveSamples(0);
  previewIsPlaying = false;
  sampleIsLoaded = false;
}

FLASHMEM void previewSample(bool setMaxSampleLength, bool playAudio) {
  g_previewPlayImmediate = false;

  stopSdPreviewAndWait();

  int ch = constrain((int)GLOB.currentChannel, 1, 8);
  if (g_browseListDirty) sampleBrowserRefreshList(ch);
  int idx = (int)currentMode->pos[3] - 1;
  if (idx < 0 || idx >= (int)g_wavPickCount) return;
  if (sampleBrowserEntryTypeAt(idx) != 2) return;

  char rel[128];
  rel[0] = '\0';
  if (g_browseDir[ch][0]) snprintf(rel, sizeof(rel), "%s/%s", g_browseDir[ch], g_wavPickName[idx]);
  else snprintf(rel, sizeof(rel), "%s", g_wavPickName[idx]);
  strncpy(SMP.samplePathRel[ch], rel, 127);
  SMP.samplePathRel[ch][127] = 0;

  char OUTPUTf[160];
  snprintf(OUTPUTf, sizeof(OUTPUTf), "samples/%s", rel);



  bool usingFullRange = !g_encoder0PressedMode && (GLOB.seek == 0) && (GLOB.seekEnd == 100 || GLOB.seekEnd == 0);
  if (usingFullRange) {
    if (!playAudio) {
      sampleIsLoaded = true;
      return;
    }
    if (GLOB.seekEnd == 0) {
      GLOB.seekEnd = 100;
      currentMode->pos[2] = GLOB.seekEnd;
      if (encoderI2cWritesAllowed()) Encoder[1].writeCounter((int32_t)GLOB.seekEnd);
    }

    previewCache.valid = false;
    previewCache.lengthBytes = 0;
    previewCache.plen = 0;

    static char lastPreviewRel[128] = {0};
    bool isNewSample = (strcasecmp(lastPreviewRel, rel) != 0);
    if (isNewSample) {
      strncpy(lastPreviewRel, rel, sizeof(lastPreviewRel) - 1);
      lastPreviewRel[sizeof(lastPreviewRel) - 1] = 0;
    }



    bool keepPeaks = (peakIndex > 0) &&
                     (previewTriggerMode == PREVIEW_MODE_PRESS ||
                      (previewTriggerMode == PREVIEW_MODE_SYNC && isNowPlaying));
    g_previewUseLivePeaks = !keepPeaks;
    if (!keepPeaks) {
      peakIndex = 0;
      memset(peakValues, 0, sizeof(peakValues));
    }

    startSdPreviewNow(OUTPUTf);
    return;
  }

  if (!previewCache.valid || strcasecmp(previewCache.pathRel, rel) != 0) {
    yield();
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      return;
    }

    int fileSize = previewFile.size();
    uint32_t dataStart = 44, dataSize = 0;
    if (!findWavDataChunk(previewFile, dataStart, dataSize)) {
      dataStart = 44;
      dataSize = (uint32_t)max(0, fileSize - 44);
    }




    PrevSampleRate = 3;



    GLOB.smplen = (int)(dataSize / (uint32_t)(PrevSampleRate * 2));

    bool loadPartial = (!setMaxSampleLength) && ((int)dataSize > (int)sizeof(sampled[0]));


    int plen = 0;
    int readStartBytes = (int)dataStart;
    int bytesToReadAll = (int)dataSize;
    if (loadPartial) {

      int startOffset = (GLOB.smplen * GLOB.seek) / 100;
      int endOffset = (GLOB.smplen * GLOB.seekEnd) / 100;
      endOffset = min(endOffset, GLOB.smplen);

      int startOffsetBytes = startOffset * PrevSampleRate * 2;
      int endOffsetBytes = endOffset * PrevSampleRate * 2;
      startOffsetBytes &= ~1;
      endOffsetBytes &= ~1;
      if (endOffsetBytes <= startOffsetBytes) {
        startOffsetBytes = 0;
        endOffsetBytes = min((int)dataSize, (int)sizeof(sampled[0]));
      }
      readStartBytes = (int)dataStart + startOffsetBytes;
      bytesToReadAll = min(endOffsetBytes - startOffsetBytes, (int)sizeof(sampled[0]));
    }

    previewFile.seek(readStartBytes);
    const size_t PREVIEW_IO_CHUNK = sdIoChunkSize();
    while (plen < bytesToReadAll) {
      size_t toRead = min(PREVIEW_IO_CHUNK, (size_t)(bytesToReadAll - plen));
      size_t bytesRead = previewFile.read(&sampled[0][plen], toRead);
      if (bytesRead == 0) break;
      plen += (int)bytesRead;
      sdIoYield();
    }
    previewFile.close();

    strncpy(previewCache.pathRel, rel, sizeof(previewCache.pathRel) - 1);
    previewCache.pathRel[sizeof(previewCache.pathRel) - 1] = 0;
    previewCache.lengthBytes = plen;
    previewCache.rate = PrevSampleRate;
    previewCache.valid = true;
    previewCache.plen = plen;
  } else {
    PrevSampleRate = previewCache.rate;

    GLOB.smplen = previewCache.lengthBytes / (PrevSampleRate * 2);
  }


  int startOffset = (GLOB.smplen * GLOB.seek) / 100;
  int endOffset = (GLOB.smplen * GLOB.seekEnd) / 100;
  endOffset = min(endOffset, GLOB.smplen);

  if (setMaxSampleLength) {
    endOffset = GLOB.smplen;
    GLOB.seekEnd = 100;
    currentMode->pos[2] = GLOB.seekEnd;
    if (encoderI2cWritesAllowed()) Encoder[1].writeCounter((int32_t)GLOB.seekEnd);
  }


  int startOffsetBytes = startOffset * PrevSampleRate * 2;
  int endOffsetBytes = endOffset * PrevSampleRate * 2;


  if (startOffsetBytes >= previewCache.lengthBytes) startOffsetBytes = 0;
  if (endOffsetBytes > previewCache.lengthBytes) endOffsetBytes = previewCache.lengthBytes;


  startOffsetBytes &= ~1;
  endOffsetBytes &= ~1;


  startOffsetBytes = constrain(startOffsetBytes, 0, previewCache.lengthBytes);
  endOffsetBytes = constrain(endOffsetBytes, 0, previewCache.lengthBytes);


  if (endOffsetBytes <= startOffsetBytes) {
    return;
  }


  int byteCount = (endOffsetBytes - startOffsetBytes) & ~1;


  int16_t* samplePtr = (int16_t*)&sampled[0][startOffsetBytes];
  size_t frameCount = byteCount / 2;


  samplerRemoveSamples(0);
  samplerAddSample(0,
    36,
    samplePtr,
    frameCount,
    rateFactor
  );
  sampleIsLoaded = true;

  if (playAudio) {
    samplerNoteEvent(0, 12 * PrevSampleRate, defaultVelocity, true, false);
  }
}

FLASHMEM void previewSample(bool setMaxSampleLength) {
  previewSample(setMaxSampleLength, true);
}

FLASHMEM void previewSampleNow() {
  g_previewPlayImmediate = true;
  previewSample(false);
}

static void loadEmptySampleToChannel(unsigned int sampleID) {
  if (sampleID >= maxFiles) {
    return;
  }

  beginChannelSampleReload(sampleID);


  SampleRate[sampleID] = 3;
  loadedSampleRate[sampleID] = 3;

  const int END_PAD_SAMPLES = 1024;
  int bufferSamples = (int)(sizeof(sampled[sampleID]) / 2);
  int samplerLen = min(END_PAD_SAMPLES, bufferSamples);
  if (samplerLen < 1) {
    samplerLen = 1;
  }
  memset(sampled[sampleID], 0, (size_t)samplerLen * 2);

  loadedSampleLen[sampleID] = 0;
  samplerAddSample(sampleID, 36, (int16_t *)sampled[sampleID], samplerLen, rateFactor);
  channelDirection[sampleID] = 1;
  endChannelSampleReload(sampleID);
}

FLASHMEM void loadSample(unsigned int packID, unsigned int sampleID) {
  if (sampleID >= maxFiles) return;
  drawNoSD();
  stopSdPreviewIfPlaying();

  char OUTPUTf[64];


  sprintf(OUTPUTf, "0/%d.wav", sampleID);
  bool loadingFromSp0 = (packID == 0 && SD.exists(OUTPUTf));


  if (packID != 0 && sampleID < maxFiles) {
    SMP.sp0Active[sampleID] = false;
  } else if (loadingFromSp0 && sampleID < maxFiles) {
    SMP.sp0Active[sampleID] = true;
  }

  if (loadingFromSp0) {

  } else if (packID == 0) {

    OUTPUTf[0] = '\0';
  } else {

    snprintf(OUTPUTf, sizeof(OUTPUTf), "%d/%d.wav", packID, sampleID);
  }

  if (!OUTPUTf[0] || !SD.exists(OUTPUTf)) {
    loadEmptySampleToChannel(sampleID);
    return;
  }





  File loadSample = SD.open(OUTPUTf);
  if (!loadSample) {
    loadEmptySampleToChannel(sampleID);
    sdIoYield();
    return;
  }

  {
    int fileSize = loadSample.size();


    SampleRate[sampleID] = 3;


    uint32_t dataStart = 44, dataSize = 0;
    if (!findWavDataChunk(loadSample, dataStart, dataSize)) {
      dataStart = 44;
      dataSize = (uint32_t)max(0, fileSize - 44);
    }

    if (fileSize <= 0 || dataSize == 0) {
      loadSample.close();
      loadEmptySampleToChannel(sampleID);
      sdIoYield();
      return;
    }


    GLOB.smplen = (int)(dataSize / (uint32_t)(SampleRate[sampleID] * 2));



    unsigned int startOffset = (GLOB.seek * GLOB.smplen) / 100;
    unsigned int startOffsetBytes = startOffset * SampleRate[sampleID] * 2;





    unsigned int seekEndPct = GLOB.seekEnd;
    if (seekEndPct == 0) seekEndPct = 100;
    if (seekEndPct > 0) seekEndPct = seekEndPct - 1;

    if (seekEndPct <= GLOB.seek) seekEndPct = min((unsigned int)100, (unsigned int)(GLOB.seek + 1));

    unsigned int endOffset = (seekEndPct * GLOB.smplen) / 100;
    if (endOffset > GLOB.smplen) endOffset = GLOB.smplen;

    unsigned int endOffsetBytes = endOffset * SampleRate[sampleID] * 2;
    endOffsetBytes = min(endOffsetBytes, (unsigned int)dataSize);


    beginChannelSampleReload(sampleID);

    loadSample.seek((uint32_t)dataStart + startOffsetBytes);
    unsigned int i = 0;


    const size_t CHUNK = sdIoChunkSize();
    size_t bytesToRead = min((size_t)(endOffsetBytes - startOffsetBytes), sizeof(sampled[sampleID]));
    while (i < bytesToRead) {
      size_t toRead = min(CHUNK, bytesToRead - (size_t)i);
      size_t bytesRead = loadSample.read(&sampled[sampleID][i], toRead);
      if (bytesRead == 0) break;
      i += (unsigned int)bytesRead;
      sdIoYield();
    }
    loadSample.close();


    i = i / 2;
    loadedSampleRate[sampleID] = SampleRate[sampleID];



    const int END_PAD_SAMPLES = 1024;
    int bufferSamples = (int)(sizeof(sampled[sampleID]) / 2);
    int samplesRead = (int)i;
    if (samplesRead < 0) samplesRead = 0;
    if (samplesRead > bufferSamples) samplesRead = bufferSamples;
    int pad = END_PAD_SAMPLES;
    if (pad > (bufferSamples - samplesRead)) pad = (bufferSamples - samplesRead);


    if (pad > 0 && (samplesRead + pad) <= bufferSamples) {
      memset(&sampled[sampleID][samplesRead * 2], 0, pad * 2);
    }

    int playLen = samplesRead + pad;

    loadedSampleLen[sampleID] = playLen;
    samplerAddSample(sampleID, 36, (int16_t *)sampled[sampleID], playLen, rateFactor);
    channelDirection[sampleID] = 1;
    endChannelSampleReload(sampleID);
  }
  sdIoYield();
}





static CRGB fileSizeToColor(uint32_t bytes) {
  const uint32_t KB = 1024;
  const uint32_t T_GREEN = 500 * KB;
  const uint32_t T_BLUE = 1200 * KB;
  const uint32_t T_RED = 1707 * KB;

  if (bytes <= T_GREEN) {
    return CRGB(0, 255, 0);
  } else if (bytes <= T_BLUE) {
    float t = (float)(bytes - T_GREEN) / (float)(T_BLUE - T_GREEN);
    return CRGB(0, (uint8_t)(255 * (1.0f - t)), (uint8_t)(255 * t));
  } else {
    float t = (float)(bytes - T_BLUE) / (float)(T_RED - T_BLUE);
    if (t > 1.0f) t = 1.0f;
    return CRGB((uint8_t)(255 * t), 0, (uint8_t)(255 * (1.0f - t)));
  }
}



static void drawSampleBrowserDepthVisual(int ch) {
  extern unsigned int maxX;
  extern void light(unsigned int x, unsigned int y, CRGB color);
  extern void drawChar(char c, int x, int y, CRGB color);
  ch = constrain(ch, 1, 8);
  const char* p = g_browseDir[ch];
  const CRGB rootCol = CRGB(0, 220, 0);
  const CRGB chevCol = CRGB(255, 220, 0);
  const CRGB initialCol = CRGB(200, 230, 255);
  const int textY = 6;

  for (unsigned int x = 1; x <= maxX; x++) {
    for (int y = 2; y <= 10; y++) {
      light(x, (unsigned int)y, CRGB::Black);
    }
  }

  int x = 1;
  drawChar('.', x, textY, rootCol);
  x += (int)alphabet['.' - 32][0] + 1;

  if (!p) return;

  while (*p) {
    while (*p == '/') p++;
    if (!*p) break;

    const char* seg = p;
    while (*p && *p != '/') p++;

    int gw = (int)alphabet['>' - 32][0] + 1;
    if (x + gw > (int)maxX + 1) break;
    drawChar('>', x, textY, chevCol);
    x += gw;

    char fc = *seg;
    if (fc < 32 || fc > 126) fc = '?';
    int cw = (int)alphabet[(uint8_t)fc - 32][0] + 1;
    if (x + cw > (int)maxX + 1) break;
    drawChar(fc, x, textY, initialCol);
    x += cw;
  }
}

static int g_lastChannelInShowWave = -1;
void invalidateShowWaveBrowserSync() {
  g_lastChannelInShowWave = -1;
}

void showWave() {

  auto resetSeekRange = []() {
    currentMode->pos[0] = 0;
    GLOB.seek = 0;
    currentMode->pos[2] = 100;
    GLOB.seekEnd = 100;
    if (encoderI2cWritesAllowed()) {
      Encoder[0].writeCounter((int32_t)0);
      Encoder[1].writeCounter((int32_t)100);
    }
  };

  auto refreshPeaksDisplay = [&]() {
    processPeaks();
  };


  extern bool readRecTouchHeld();
  extern Mode recordMode;
  if (readRecTouchHeld()) {
    switchMode(&recordMode);
    return;
  }

  yield();

  int ch = constrain((int)GLOB.currentChannel, 1, 8);
  if (g_lastChannelInShowWave != GLOB.currentChannel) {
    g_lastChannelInShowWave = GLOB.currentChannel;
    ch = constrain((int)GLOB.currentChannel, 1, 8);
    sampleBrowserRefreshList(ch);
    sampleBrowserSyncBrowseFromStoredPath(ch);
    SMP.wav[ch].fileID = (unsigned int)currentMode->pos[3];
    SMP.wav[ch].oldID = 0;
    lastEncoder3Value_forShowWave = -1;
  }

  int fileIdx = (int)SMP.wav[ch].fileID;
  if (fileIdx < 1) fileIdx = 1;
  char OUTPUTf[160];
  OUTPUTf[0] = 0;
  buildSamplePath(0, 0, OUTPUTf, sizeof(OUTPUTf));

  firstcheck = false;
  nofile = false;

  FastLEDclear();

  if (g_browseListDirty) sampleBrowserRefreshList(ch);


  int encoderSampleValue = (int)currentMode->pos[3];
  int maxList = max(1, (int)g_wavPickCount);
  if (encoderSampleValue < 1) encoderSampleValue = 1;
  if (encoderSampleValue > maxList) encoderSampleValue = maxList;
  if (encoderSampleValue != (int)currentMode->pos[3]) {
    currentMode->pos[3] = encoderSampleValue;
  }
  sampleBrowserClampBrowseIndex(ch);
  encoderSampleValue = (int)currentMode->pos[3];
  int selectedIdx = encoderSampleValue - 1;
  bool selectedIsFile = (selectedIdx >= 0 && selectedIdx < (int)g_wavPickCount &&
                         sampleBrowserEntryTypeAt(selectedIdx) == 2);


  drawIndicator('L', 'P', 1, false, false);
  if (selectedIsFile) {
    drawIndicator('L', 'Y', 2, false, false);
    drawIndicator('L', 'G', 3, false, false);
  }
  drawIndicator('L', 'W', 4, false, false);

  const bool allowEncWrite = encoderI2cWritesAllowed();
  static bool seekHwResetPending = false;
  static uint8_t seekHwResetTries = 0;



  currentMode->pos[1] = 0;



  static uint32_t lastShowWaveRGB[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
  static uint32_t lastShowWaveRgbMs = 0;
  static bool encRangeSynced = false;
  static int lastSyncedMaxList = -1;
  static int lastPeakScanKey = -1;
  uint32_t nowMs = millis();
  if (nowMs - lastShowWaveRgbMs > 50) {
    for (int i = 0; i < 4; i++) lastShowWaveRGB[i] = 0xFFFFFFFF;
    encRangeSynced = false;
    lastSyncedMaxList = -1;
    lastPeakScanKey = -1;
  }
  lastShowWaveRgbMs = nowMs;
  auto setShowWaveRGB = [&](uint8_t i, uint32_t rgb) {
    if (i < 4 && lastShowWaveRGB[i] != rgb) {
      lastShowWaveRGB[i] = rgb;
      if (allowEncWrite) Encoder[i].writeRGBCode(rgb);
    }
  };

  if (allowEncWrite && !encRangeSynced) {
    Encoder[2].writeMin((int32_t)0);
    Encoder[2].writeMax((int32_t)0);
    Encoder[2].writeCounter((int32_t)0);
    Encoder[1].writeMin((int32_t)0);
    Encoder[1].writeMax((int32_t)100);
    encRangeSynced = true;
  }

  setShowWaveRGB(0, 0xFF00FF);
  if (selectedIsFile) {
    setShowWaveRGB(1, 0xFFFF00);
    setShowWaveRGB(2, 0x00FF00);
  } else {
    setShowWaveRGB(1, 0x000000);
    setShowWaveRGB(2, 0x000000);
  }
  setShowWaveRGB(3, 0xFFFFFF);

  bool sampleChanged = selectedIsFile &&
                       ((encoderSampleValue != lastEncoder3Value_forShowWave) ||
                        (encoderSampleValue != SMP.wav[ch].fileID));


  if (sampleChanged) {
    resetSeekRange();
    seekHwResetPending = true;
    seekHwResetTries = 0;
  } else if (seekHwResetPending) {
    if (++seekHwResetTries > 3) seekHwResetPending = false;
  } else if (currentMode->pos[2] > 100) {
    currentMode->pos[2] = 100;
    if (allowEncWrite) Encoder[1].writeCounter((int32_t)currentMode->pos[2]);
  }


  if (!sampleChanged && !seekHwResetPending) {
    int newSeek = constrain(currentMode->pos[0], 0, GLOB.seekEnd - 1);
    if (newSeek != GLOB.seek) {
      GLOB.seek = newSeek;
      currentMode->pos[0] = newSeek;
      if (sampleIsLoaded && previewPlaysOnSelect() && !g_peakScan.active) {
        yield();
        previewSample(false, true);
      }
    }

    int newSeekEnd = constrain((int)currentMode->pos[2], GLOB.seek + 1, 100);
    if (newSeekEnd != (int)currentMode->pos[2] && allowEncWrite) {
      Encoder[1].writeCounter((int32_t)newSeekEnd);
      currentMode->pos[2] = (unsigned int)newSeekEnd;
    }
    if (newSeekEnd != GLOB.seekEnd) {
      GLOB.seekEnd = newSeekEnd;
      currentMode->pos[2] = newSeekEnd;
      if (sampleIsLoaded && previewPlaysOnSelect() && !g_peakScan.active) {
        yield();
        previewSample(false, true);
      }
    }
  }


  if (allowEncWrite && maxList != lastSyncedMaxList) {
    Encoder[3].writeMin((int32_t)1);
    Encoder[3].writeMax((int32_t)maxList);
    lastSyncedMaxList = maxList;
  }

  if ((int)lastEncoder3Value_forShowWave != encoderSampleValue) {
    lastEncoder3Value_forShowWave = encoderSampleValue;
    if (!selectedIsFile) {
      stopPeakScan();
      peakIndex = 0;
      stopSdPreviewAndWait();
      sampleIsLoaded = false;
      firstcheck = true;
      nofile = false;
    }
  }

  if (sampleChanged) {
    fileIdx = encoderSampleValue;
    SMP.wav[ch].oldID = 0;
    SMP.wav[ch].fileID = (unsigned int)fileIdx;

    yield();
    stopSdPreviewAndWait();
    sampleIsLoaded = false;
    firstcheck = true;
    nofile = false;

    previewCache.valid = false;


    if (g_suppressNextWavPreviewAfterFolderNav) {
      g_suppressNextWavPreviewAfterFolderNav = false;

      if (selectedIsFile) commitCurrentBrowsePathToChannel(ch);
    } else if (previewPlaysOnSelect()) {

      sampleIsLoaded = true;
      previewSampleNow();
    } else {


      commitCurrentBrowsePathToChannel(ch);
      char peakPath[160];
      if (buildCurrentBrowseSamplePath(peakPath, sizeof(peakPath))) {
        startPeakScan(peakPath);
      }
      sampleIsLoaded = true;
      lastPeakScanKey = (ch << 16) | encoderSampleValue;
    }
  }


  int peakScanKey = (ch << 16) | encoderSampleValue;
  if (selectedIsFile && peakIndex == 0 && !g_peakScan.active && !playSdWav1.isPlaying()
      && !previewIsPlaying && !g_queuedSdPreview && lastPeakScanKey != peakScanKey) {
    lastPeakScanKey = peakScanKey;
    char peakPath[160];
    if (buildCurrentBrowseSamplePath(peakPath, sizeof(peakPath))) {
      startPeakScan(peakPath);
      sampleIsLoaded = true;
    }
  }

  if (selectedIsFile && !playSdWav1.isPlaying() && !g_queuedSdPreview) {
    servicePeakScan(4096);
    yield();
  }


  if (GLOB.seekEnd > 100) {
    GLOB.seekEnd = 100;
    currentMode->pos[2] = GLOB.seekEnd;
    if (allowEncWrite) Encoder[1].writeCounter((int32_t)GLOB.seekEnd);
  }


  if (selectedIsFile) {
    refreshPeaksDisplay();
  } else {
    drawSampleBrowserDepthVisual(ch);
  }


  static int scrollSelectionOffset = 0;
  static unsigned long nextSelectionStepMs = 0;
  static int lastOverlayPos3 = -1;
  static bool overlayInit = false;
  static int selectionDir = 1;

  auto textPixelWidth = [](const char* text) {
    int w = 0;
    for (int i = 0; text[i] != '\0'; i++) {
      char c = text[i];
      if (c < 32 || c > 126) continue;
      w += alphabet[c - 32][0] + 1;
    }
    return w;
  };

  auto renderLine = [&](const char* text, int y, CRGB color, int& offset, int& dir, unsigned long& nextStepMs, bool isFilename = false) {
    int viewStart = 1;
    int viewEnd = 14;
    int viewWidth = viewEnd - viewStart + 1;
    int width = textPixelWidth(text);
    int fullStrLen = strlen(text);


    if (fullStrLen == 1) {
      drawText(text, 14, y, color);
      offset = 0;
      dir = 1;
      return;
    }


    int initialStartX = 1;
    if (isFilename && fullStrLen > 4) {
      initialStartX = 4;
    }


    if (fullStrLen <= 4 || width <= viewWidth) {
      drawText(text, initialStartX, y, color);
      offset = 0;
      dir = 1;
      return;
    }






    int offsetMax = max(0, initialStartX + width - 15);
    unsigned long now = millis();

    if (now >= nextStepMs) {
      if (dir > 0) {

        offset++;
        if (offset >= offsetMax) {
          offset = offsetMax;
          dir = -1;
          nextStepMs = now + 500;
        } else {
          nextStepMs = now + 50;
        }
      } else {

        offset--;
        if (offset <= 0) {
          offset = 0;
          dir = 1;
          nextStepMs = now + 500;
        } else {
          nextStepMs = now + 50;
        }
      }
    }

    int startX = initialStartX - offset;
    drawText(text, startX, y, color);
  };

  {
    int currentBrowsePos = (int)currentMode->pos[3];
    static char lastBrowseSnap[128] = {0};
    static int lastOverlayCh = -1;
    if (ch != lastOverlayCh) {
      lastOverlayCh = ch;
      overlayInit = false;
    }

    if (!overlayInit) {
      overlayInit = true;
      lastOverlayPos3 = currentBrowsePos;
      strncpy(lastBrowseSnap, g_browseDir[ch], sizeof(lastBrowseSnap) - 1);
      lastBrowseSnap[sizeof(lastBrowseSnap) - 1] = 0;
    }

    bool browseDirChanged = (strcmp(lastBrowseSnap, g_browseDir[ch]) != 0);
    bool pos3Changed = (currentBrowsePos != lastOverlayPos3);
    if (browseDirChanged || pos3Changed) {
      scrollSelectionOffset = 0;
      selectionDir = 1;
      nextSelectionStepMs = millis() + 300;
      lastOverlayPos3 = currentBrowsePos;
      strncpy(lastBrowseSnap, g_browseDir[ch], sizeof(lastBrowseSnap) - 1);
      lastBrowseSnap[sizeof(lastBrowseSnap) - 1] = 0;
    }

    char selectionLabel[128];
    selectionLabel[0] = '\0';
    int ei = (int)currentMode->pos[3] - 1;
    if (ei >= 0 && ei < (int)g_wavPickCount) {
      snprintf(selectionLabel, sizeof(selectionLabel), "%s", g_wavPickName[ei]);
    } else {
      snprintf(selectionLabel, sizeof(selectionLabel), "-");
    }
    bool selectionIsFileEntry = (ei >= 0 && ei < (int)g_wavPickCount && sampleBrowserEntryTypeAt(ei) == 2);
    if (selectionIsFileEntry) {
      size_t labLen = strlen(selectionLabel);
      if (labLen > 4) {
        char *ext = selectionLabel + (labLen - 4);
        if (ext[0] == '.' && (ext[1] == 'w' || ext[1] == 'W') &&
            (ext[2] == 'a' || ext[2] == 'A') && (ext[3] == 'v' || ext[3] == 'V')) {
          *ext = '\0';
        }
      }
    }
    CRGB selectionColor = selectionIsFileEntry
        ? fileSizeToColor(g_wavPickSize[ei])
        : CRGB(255, 255, 0);
    renderLine(selectionLabel, 12, selectionColor, scrollSelectionOffset, selectionDir, nextSelectionStepMs, selectionIsFileEntry);
  }

  yield();
}

static uint16_t g_pendingSp0CopyMask = 0;
static bool g_pendingSampleLoadAutosave = false;

static bool sampleLoadProgressAllowed() {
  return !isNowPlaying;
}

static void requestDeferredSp0Copy(unsigned int channel) {
  if (channel < 1 || channel > 8) return;
  g_pendingSp0CopyMask |= (uint16_t)(1u << channel);
}

static void sampleLoadUiThrottled(uint8_t percent, bool enable, uint8_t &lastDrawn) {
  if (!enable || !sampleLoadProgressAllowed()) return;
  extern void drawSampleLoadOverlay(uint8_t progressPercent);
  extern void FastLEDshow();
  if (percent > 100) percent = 100;
  if (lastDrawn == 255 || percent >= 100 || percent >= lastDrawn + 2) {
    lastDrawn = (percent >= 100) ? 100 : percent;
    drawSampleLoadOverlay(percent);
    FastLEDshow();
    yield();
  }
}

void requestDeferredSampleLoadAutosave() {
  g_pendingSampleLoadAutosave = true;
}

FLASHMEM void servicePendingSamplePersist() {
  if (isNowPlaying) return;
  extern bool sdIoAudioSettled();
  if (!sdIoAudioSettled()) return;
  if (g_pendingSp0CopyMask) {
    sdIoBeginAudioSafe();
    for (unsigned int ch = 1; ch <= 8; ch++) {
      if (g_pendingSp0CopyMask & (uint16_t)(1u << ch)) {
        g_pendingSp0CopyMask &= (uint16_t)~(1u << ch);
        copySampleToSamplepack0(ch, false);
        saveSp0StateToEEPROM();
      }
    }
    sdIoEndAudioSafe();
  }
  if (g_pendingSampleLoadAutosave) {
    g_pendingSampleLoadAutosave = false;
    extern void autoSave();
    autoSave();
  }
}


FLASHMEM void copySampleToSamplepack0(unsigned int channel, bool showLoadProgress) {
  if (channel < 1 || channel >= maxFiles) return;
  if (isNowPlaying) {

    requestDeferredSp0Copy(channel);
    SMP.sp0Active[channel] = true;
    return;
  }
  stopSdPreviewAndWait();


  if (!SD.exists("0")) {
    SD.mkdir("0");
  }

  char outputPath[50];
  sprintf(outputPath, "0/%d.wav", channel);


  if (SD.exists(outputPath)) {
    SD.remove(outputPath);
  }


  File outFile = SD.open(outputPath, FILE_WRITE);
  if (!outFile) {
    return;
  }


  uint32_t sampleCount = loadedSampleLen[channel];
  uint32_t dataSize = sampleCount * sizeof(int16_t);


  uint32_t sampleRate = (uint32_t)AUDIO_SAMPLE_RATE_EXACT;
  uint32_t byteRate = sampleRate * 1 * 16 / 8;
  uint8_t blockAlign = 1 * 16 / 8;


  uint8_t header[44] = {
    'R', 'I', 'F', 'F',
    (uint8_t)((dataSize + 36) & 0xff), (uint8_t)(((dataSize + 36) >> 8) & 0xff),
    (uint8_t)(((dataSize + 36) >> 16) & 0xff), (uint8_t)(((dataSize + 36) >> 24) & 0xff),
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,
    1, 0,
    1, 0,
    (uint8_t)(sampleRate & 0xff), (uint8_t)((sampleRate >> 8) & 0xff),
    (uint8_t)((sampleRate >> 16) & 0xff), (uint8_t)((sampleRate >> 24) & 0xff),
    (uint8_t)(byteRate & 0xff), (uint8_t)(((byteRate) >> 8) & 0xff),
    (uint8_t)((byteRate >> 16) & 0xff), (uint8_t)((byteRate >> 24) & 0xff),
    blockAlign, 0,
    16, 0,
    'd', 'a', 't', 'a',
    (uint8_t)(dataSize & 0xff), (uint8_t)((dataSize >> 8) & 0xff),
    (uint8_t)((dataSize >> 16) & 0xff), (uint8_t)((dataSize >> 24) & 0xff)
  };

  outFile.write(header, 44);


  uint8_t* dataPtr = reinterpret_cast<uint8_t*>(sampled[channel]);
  const size_t CHUNK_SIZE = sdIoChunkSize();

  uint8_t sp0ProgLast = 255;
  if (showLoadProgress && dataSize == 0) {
    sampleLoadUiThrottled(100, true, sp0ProgLast);
  }
  for (uint32_t offset = 0; offset < dataSize; offset += CHUNK_SIZE) {
    size_t chunkSize = min(CHUNK_SIZE, (size_t)(dataSize - offset));
    outFile.write(dataPtr + offset, chunkSize);
    sdIoYield();
    if (showLoadProgress && dataSize > 0) {
      uint32_t done = offset + (uint32_t)chunkSize;
      if (done > dataSize) done = dataSize;
      uint8_t pct = (uint8_t)(80u + (uint32_t)done * 20u / dataSize);
      if (pct > 100) pct = 100;
      sampleLoadUiThrottled(pct, true, sp0ProgLast);
    }
  }
  if (showLoadProgress) {
    sampleLoadUiThrottled(100, true, sp0ProgLast);
  }

  outFile.close();


  SMP.sp0Active[channel] = true;
}


void reversePreviewSample() {
  extern CachedSample previewCache;

  if (!previewCache.valid || previewCache.lengthBytes == 0) {
    return;
  }


  uint8_t* buffer = sampled[0];
  int numSamples = previewCache.lengthBytes / 2;


  int16_t* sampleBuffer = (int16_t*)buffer;
  for (int i = 0; i < numSamples / 2; i++) {
    int16_t temp = sampleBuffer[i];
    sampleBuffer[i] = sampleBuffer[numSamples - 1 - i];
    sampleBuffer[numSamples - 1 - i] = temp;
  }


  extern int peakIndex;
  extern float peakValues[];

  for (int i = 0; i < peakIndex / 2; i++) {
    float temp = peakValues[i];
    peakValues[i] = peakValues[peakIndex - 1 - i];
    peakValues[peakIndex - 1 - i] = temp;
  }


  int tempSeek = GLOB.seek;
  GLOB.seek = 100 - GLOB.seekEnd;
  GLOB.seekEnd = 100 - tempSeek;


  extern Mode* currentMode;
  currentMode->pos[0] = GLOB.seek;
  currentMode->pos[2] = GLOB.seekEnd;

  extern i2cEncoderLibV2 Encoder[];
  if (encoderI2cWritesAllowed()) {
    Encoder[0].writeCounter((int32_t)GLOB.seek);
    Encoder[1].writeCounter((int32_t)GLOB.seekEnd);
  }

  previewSample(false, previewPlaysOnSelect());
}


bool loadPreviewToChannel(unsigned int targetChannel, bool showLoadProgress) {
  if (targetChannel < 1 || targetChannel >= maxFiles) {
    return false;
  }

  stopSdPreviewAndWait();

  uint8_t loadProgLast = 255;
  const bool showProg = showLoadProgress && sampleLoadProgressAllowed();

  extern CachedSample previewCache;



  commitCurrentBrowsePathToChannel((int)GLOB.currentChannel);

  bool pathMismatch =
      (SMP.samplePathRel[GLOB.currentChannel][0] == 0) ||
      (strcasecmp(previewCache.pathRel, SMP.samplePathRel[GLOB.currentChannel]) != 0);
  bool needToLoadPreview = (!previewCache.valid || previewCache.lengthBytes == 0 || pathMismatch);


  if (needToLoadPreview) {
    char OUTPUTf[160];
    OUTPUTf[0] = '\0';
    buildSamplePath(0, 0, OUTPUTf, sizeof(OUTPUTf));
    if (!OUTPUTf[0] && !buildCurrentBrowseSamplePath(OUTPUTf, sizeof(OUTPUTf))) {
      return false;
    }

    sdIoYield();
    File previewFile = SD.open(OUTPUTf);
    if (!previewFile) {
      return false;
    }

    if (showProg) {
      drawSampleLoadOverlay(4);
      FastLEDshow();
    }

    int fileSize = previewFile.size();
    uint32_t dataStart = 44, dataSize = 0;
    if (!findWavDataChunk(previewFile, dataStart, dataSize)) {
      dataStart = 44;
      dataSize = (uint32_t)max(0, fileSize - 44);
    }



    int rate = 3;


    previewFile.seek(dataStart);
    int plen = 0;
    const size_t LPREVIEW_CHUNK = sdIoChunkSize();
    int chunkCount = 0;
    int maxToRead = (int)min((uint32_t)sizeof(sampled[0]), dataSize);
    while (plen < maxToRead) {
      size_t toRead = min(LPREVIEW_CHUNK, (size_t)(sizeof(sampled[0]) - (size_t)plen));
      size_t bytesRead = previewFile.read(&sampled[0][plen], toRead);
      if (bytesRead == 0) break;
      plen += (int)bytesRead;
      sdIoYield();
      chunkCount++;
      if (showProg && maxToRead > 0 && (chunkCount % 2) == 0) {
        uint8_t pct = (uint8_t)(4u + (uint32_t)plen * 62u / (uint32_t)maxToRead);
        sampleLoadUiThrottled(pct, true, loadProgLast);
      }
    }
    if (showProg && maxToRead > 0) {
      sampleLoadUiThrottled(66, true, loadProgLast);
    }
    previewFile.close();


    if (SMP.samplePathRel[GLOB.currentChannel][0]) {
      strncpy(previewCache.pathRel, SMP.samplePathRel[GLOB.currentChannel], sizeof(previewCache.pathRel) - 1);
      previewCache.pathRel[sizeof(previewCache.pathRel) - 1] = 0;
    }
    previewCache.lengthBytes = plen;
    previewCache.rate = rate;
    previewCache.valid = true;
    previewCache.plen = plen;
    if (plen < 2) {
      previewCache.valid = false;
      return false;
    }
  } else if (showProg) {
    drawSampleLoadOverlay(50);
    FastLEDshow();
  }

  if (previewCache.lengthBytes < 2) {
    return false;
  }


  int numSamples = previewCache.lengthBytes / 2;
  int startSample = (numSamples * GLOB.seek) / 100;

  int seekEndPct = (GLOB.seekEnd == 0) ? 100 : (int)GLOB.seekEnd;
  if (seekEndPct > 0) seekEndPct -= 1;
  if (seekEndPct <= (int)GLOB.seek) seekEndPct = min(100, (int)GLOB.seek + 1);

  int endSample = (numSamples * seekEndPct) / 100;

  if (endSample <= startSample) {
    endSample = numSamples;
  }

  int trimmedSamples = endSample - startSample;


  beginChannelSampleReload(targetChannel);

  int16_t* previewBuffer = (int16_t*)sampled[0];
  int16_t* targetBuffer = (int16_t*)sampled[targetChannel];
  int bufferSamples = (int)(sizeof(sampled[targetChannel]) / 2);
  int samplesRead = trimmedSamples;
  if (samplesRead < 0) samplesRead = 0;
  if (samplesRead > bufferSamples) samplesRead = bufferSamples;

  const int copyBurst = (isNowPlaying || sdIoKeepAudioFed) ? 256 : 4096;
  int copied = 0;
  while (copied < samplesRead) {
    int n = samplesRead - copied;
    if (n > copyBurst) n = copyBurst;
    memcpy(targetBuffer + copied, previewBuffer + startSample + copied, (size_t)n * sizeof(int16_t));
    copied += n;
    sdIoYield();
  }

  loadedSampleRate[targetChannel] = previewCache.rate;

  const int END_PAD_SAMPLES = 1024;
  int pad = END_PAD_SAMPLES;
  if (pad > (bufferSamples - samplesRead)) pad = (bufferSamples - samplesRead);

  if (pad > 0 && (samplesRead + pad) <= bufferSamples) {
    memset(&sampled[targetChannel][samplesRead * 2], 0, pad * 2);
  }

  int playLen = samplesRead + pad;

  loadedSampleLen[targetChannel] = playLen;
  samplerAddSample(targetChannel, 36, targetBuffer, playLen, rateFactor);
  channelDirection[targetChannel] = 1;
  endChannelSampleReload(targetChannel);

  if (showProg) {
    sampleLoadUiThrottled(78, true, loadProgLast);
  }

  SMP.wav[targetChannel].oldID = SMP.wav[GLOB.currentChannel].oldID;
  SMP.wav[targetChannel].fileID = SMP.wav[GLOB.currentChannel].fileID;
  strncpy(SMP.samplePathRel[targetChannel], SMP.samplePathRel[GLOB.currentChannel], 127);
  SMP.samplePathRel[targetChannel][127] = 0;
  return true;
}

void soloRandomPreviewCurrentVoice() {
  int ch = constrain((int)GLOB.currentChannel, 1, 8);

  char stored[128];
  if (!soloRandomPickPath(stored, sizeof(stored))) return;

  char OUTPUTf[160];
  if (!soloRandomBuildPlayPath(stored, OUTPUTf, sizeof(OUTPUTf))) return;
  if (!OUTPUTf[0]) return;



  soloRandomCommitPathToChannel(ch, OUTPUTf);

  stopPeakScan();
  GLOB.seek = 0;
  GLOB.seekEnd = 100;
  previewCache.valid = false;
  previewCache.lengthBytes = 0;
  previewCache.plen = 0;

  requestSdPreview(OUTPUTf);
}


void soloRandomStepAndPreview(int delta) {
  if (!soloRandomStepPlaylist(delta)) return;
  soloRandomPreviewCurrentVoice();
}

void soloRandomLoadLastPreview() {
  int ch = constrain((int)GLOB.currentChannel, 1, 8);
  const char* stored = soloRandomGetLastPreviewPath();
  if (!stored || !stored[0]) return;

  char OUTPUTf[160];
  if (!soloRandomBuildPlayPath(stored, OUTPUTf, sizeof(OUTPUTf))) return;
  if (!OUTPUTf[0]) return;


  g_queuedSdPreview = false;
  stopSdPreviewAndWait();
  previewIsPlaying = false;

  soloRandomCommitPathToChannel(ch, OUTPUTf);

  GLOB.seek = 0;
  GLOB.seekEnd = 100;
  previewCache.valid = false;
  previewCache.lengthBytes = 0;
  previewCache.plen = 0;

  preventPaintUnpaint = true;
  sdIoBeginAudioSafe();
  bool loaded = loadPreviewToChannel((unsigned int)ch, !isNowPlaying);
  if (loaded) {
    copySampleToSamplepack0((unsigned int)ch, !isNowPlaying);
    if (!isNowPlaying) {
      saveSp0StateToEEPROM();
      flushSettingsBackupNow();
    }
  }
  sdIoEndAudioSafe();
  preventPaintUnpaint = false;
}
# 1 "/Users/jank/git/toern/toern_sd_serial.ino"
# 29 "/Users/jank/git/toern/toern_sd_serial.ino"
#if defined(ARDUINO)

static const size_t SD_SER_CHUNK = 4096;
static const size_t SD_SER_PUT_BLOCK = 8192;
static const size_t SD_SER_GET_BLOCK = 512;
static const uint32_t SD_SER_IO_TIMEOUT_MS = 60000;

static bool sdSerActive = false;
static bool sdSerClientConnected = false;
static uint32_t sdSerLastClientMs = 0;
static const uint32_t SD_SER_CLIENT_TIMEOUT_MS = 15000;
static char sdSerLineBuf[192];
static size_t sdSerLineLen = 0;

static void sdSerDrainRx() {
  uint32_t start = millis();
  do {
    while (Serial.available()) (void)Serial.read();
    yield();
  } while ((millis() - start) < 30u);
  sdSerLineLen = 0;
}

static void sdSerAudioStopForSd() {



  extern bool isNowPlaying;
  extern void pause(bool skipSave);
  extern void allOff();
  extern void stopAllSetWavPreviewAudio();
  if (isNowPlaying) pause(true);
  allOff();
  stopAllSetWavPreviewAudio();
}

static void sdSerSdLock() {
  sdSerAudioStopForSd();
  AudioNoInterrupts();
}

static void sdSerSdUnlock() {
  AudioInterrupts();
}

static void sdSerTouchClient();

static uint32_t sdSerCrc32Update(uint32_t crc, const uint8_t *data, size_t len) {
  crc = ~crc;
  while (len--) {
    crc ^= *data++;
    for (int i = 0; i < 8; i++) {
      uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

static void sdSerReply(const char *msg) {
  Serial.println(msg);
}

static void sdSerReplyf(const char *fmt, ...) {
  char buf[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Serial.println(buf);
}

static void sdSerReplyFlush(const char *msg) {
  Serial.println(msg);
  Serial.flush();
}

static bool sdSerReadExact(uint8_t *dst, size_t len, uint32_t timeoutMs) {
  size_t got = 0;
  uint32_t start = millis();
  while (got < len) {
    int avail = Serial.available();
    if (avail > 0) {
      size_t take = (size_t)avail;
      if (take > len - got) take = len - got;
      int n = Serial.readBytes(dst + got, take);
      if (n <= 0) {
        if ((millis() - start) > timeoutMs) return false;
        yield();
        continue;
      }
      got += (size_t)n;
      start = millis();
    } else {
      if ((millis() - start) > timeoutMs) return false;
      yield();
    }
  }
  return true;
}


static bool sdSerNormalizePath(const char *in, char *out, size_t outLen) {
  if (!in || !out || outLen < 2) return false;

  while (*in == '/' || *in == '\\') in++;

  if (*in == '\0') {
    out[0] = '/';
    out[1] = '\0';
    return true;
  }

  size_t o = 0;
  while (*in && o + 1 < outLen) {
    if (*in == '/' || *in == '\\') {
      if (o == 0 || out[o - 1] != '/') out[o++] = '/';
      in++;
      continue;
    }

    char seg[64];
    size_t s = 0;
    while (*in && *in != '/' && *in != '\\' && s + 1 < sizeof(seg)) {
      seg[s++] = *in++;
    }
    seg[s] = '\0';
    if (s == 0) continue;
    if (strcmp(seg, ".") == 0) continue;
    if (strcmp(seg, "..") == 0) return false;

    if (o > 0 && out[o - 1] != '/') {
      if (o + 1 >= outLen) return false;
      out[o++] = '/';
    }
    if (o + s >= outLen) return false;
    memcpy(out + o, seg, s);
    o += s;
  }
  out[o] = '\0';
  if (o == 0) {
    out[0] = '/';
    out[1] = '\0';
  }
  return true;
}

static const char *sdSerFsPath(const char *normalized) {
  if (!normalized || normalized[0] == '\0') return "/";
  return normalized;
}

static void sdSerCmdPing() {
  sdSerReplyf("OK TOERN SD %s", VERSION);
}

static void sdSerCmdList(const char *pathArg) {
  char path[128];
  if (!sdSerNormalizePath(pathArg ? pathArg : "/", path, sizeof(path))) {
    sdSerReply("ERR BAD_PATH");
    return;
  }


  sdSerSdLock();
  File dir = SD.open(sdSerFsPath(path));
  if (!dir) {
    sdSerSdUnlock();
    sdSerReply("ERR NOT_FOUND");
    return;
  }
  if (!dir.isDirectory()) {
    dir.close();
    sdSerSdUnlock();
    sdSerReply("ERR NOT_DIR");
    return;
  }

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (entry.isDirectory()) {
      sdSerReplyf("D %s", entry.name());
    } else {
      uint32_t size = (uint32_t)entry.size();

      sdSerReplyf("F %lu %ld %s", (unsigned long)size, (long)-1, entry.name());
    }
    entry.close();
    sdSerLastClientMs = millis();
    yield();
  }
  dir.close();
  sdSerSdUnlock();
  sdSerReply("OK");
}

static void sdSerCmdMv(char *args) {
  char *fromTok = strtok(args, " \t");
  char *toTok = strtok(NULL, " \t");
  if (!fromTok || !toTok) {
    sdSerReply("ERR USAGE");
    return;
  }
  char from[128], to[128];
  if (!sdSerNormalizePath(fromTok, from, sizeof(from)) || strcmp(from, "/") == 0 ||
      !sdSerNormalizePath(toTok, to, sizeof(to)) || strcmp(to, "/") == 0) {
    sdSerReply("ERR BAD_PATH");
    return;
  }
  sdSerSdLock();
  if (!SD.exists(from)) {
    sdSerSdUnlock();
    sdSerReply("ERR NOT_FOUND");
    return;
  }
  if (SD.exists(to)) {
    sdSerSdUnlock();
    sdSerReply("ERR EXISTS");
    return;
  }
  bool ok = SD.rename(from, to);
  sdSerSdUnlock();
  if (!ok) {
    sdSerReply("ERR RENAME");
    return;
  }
  sdSerReply("OK");
}

static void sdSerCmdRm(const char *pathArg) {
  char path[128];
  if (!pathArg || !sdSerNormalizePath(pathArg, path, sizeof(path)) || strcmp(path, "/") == 0) {
    sdSerReply("ERR BAD_PATH");
    return;
  }
  sdSerSdLock();
  if (!SD.exists(path)) {
    sdSerSdUnlock();
    sdSerReply("ERR NOT_FOUND");
    return;
  }
  File f = SD.open(path);
  bool ok = false;
  if (f && f.isDirectory()) {
    f.close();
    ok = SD.rmdir(path);
    sdSerSdUnlock();
    if (!ok) {
      sdSerReply("ERR RMDIR");
      return;
    }
  } else {
    if (f) f.close();
    ok = SD.remove(path);
    sdSerSdUnlock();
    if (!ok) {
      sdSerReply("ERR REMOVE");
      return;
    }
  }
  sdSerReply("OK");
}

static void sdSerCmdMkdir(const char *pathArg) {
  char path[128];
  if (!pathArg || !sdSerNormalizePath(pathArg, path, sizeof(path)) || strcmp(path, "/") == 0) {
    sdSerReply("ERR BAD_PATH");
    return;
  }
  sdSerSdLock();
  if (SD.exists(path)) {
    sdSerSdUnlock();
    sdSerReply("ERR EXISTS");
    return;
  }
  bool ok = SD.mkdir(path);
  sdSerSdUnlock();
  if (!ok) {
    sdSerReply("ERR MKDIR");
    return;
  }
  sdSerReply("OK");
}

static void sdSerCmdPut(char *args) {
  char *pathTok = strtok(args, " \t");
  char *sizeTok = strtok(NULL, " \t");
  char *crcTok = strtok(NULL, " \t");
  if (!pathTok || !sizeTok || !crcTok) {
    sdSerReply("ERR USAGE");
    return;
  }

  char path[128];
  if (!sdSerNormalizePath(pathTok, path, sizeof(path)) || strcmp(path, "/") == 0) {
    sdSerReply("ERR BAD_PATH");
    return;
  }

  uint32_t size = (uint32_t)strtoul(sizeTok, NULL, 10);
  uint32_t expectCrc = (uint32_t)strtoul(crcTok, NULL, 16);
  if (size > (512ul * 1024ul * 1024ul)) {
    sdSerReply("ERR TOO_LARGE");
    return;
  }

  if (SD.exists(path)) {
    File existing = SD.open(path);
    if (existing && existing.isDirectory()) {
      existing.close();
      sdSerReply("ERR IS_DIR");
      return;
    }
    if (existing) existing.close();
    SD.remove(path);
  }

  {
    char parent[128];
    strncpy(parent, path, sizeof(parent) - 1);
    parent[sizeof(parent) - 1] = '\0';
    char *slash = strrchr(parent, '/');
    if (slash && slash != parent) {
      *slash = '\0';
      char build[128];
      build[0] = '\0';
      char *save = NULL;
      char *seg = strtok_r(parent, "/", &save);
      while (seg) {
        size_t bl = strlen(build);
        if (bl + 1 + strlen(seg) >= sizeof(build)) break;
        if (bl == 0) {
          strncpy(build, seg, sizeof(build) - 1);
        } else {
          snprintf(build + bl, sizeof(build) - bl, "/%s", seg);
        }
        if (!SD.exists(build)) {
          SD.mkdir(build);
        }
        seg = strtok_r(NULL, "/", &save);
      }
    }
  }

  File out = SD.open(path, O_WRITE | O_CREAT | O_TRUNC);
  if (!out) {
    sdSerReply("ERR OPEN");
    return;
  }

  sdSerReplyFlush("READY");

  static uint8_t chunk[SD_SER_CHUNK];
  uint32_t remaining = size;
  uint32_t crc = 0;
  bool ok = true;



  sdSerAudioStopForSd();
  while (remaining > 0) {


    size_t block = remaining > SD_SER_PUT_BLOCK ? SD_SER_PUT_BLOCK : (size_t)remaining;
    size_t got = 0;
    while (got < block) {
      size_t n = (block - got) > SD_SER_CHUNK ? SD_SER_CHUNK : (block - got);
      if (!sdSerReadExact(chunk, n, SD_SER_IO_TIMEOUT_MS)) {
        ok = false;
        break;
      }
      AudioNoInterrupts();
      size_t written = out.write(chunk, n);
      AudioInterrupts();
      if (written != n) {
        ok = false;
        break;
      }
      crc = sdSerCrc32Update(crc, chunk, n);
      got += n;
      remaining -= (uint32_t)n;
      sdSerLastClientMs = millis();
    }
    if (!ok) break;
    AudioNoInterrupts();
    out.flush();
    AudioInterrupts();
    sdSerReplyFlush("ACK");
  }
  AudioNoInterrupts();
  out.close();
  AudioInterrupts();

  if (!ok) {
    sdSerDrainRx();
    SD.remove(path);
    sdSerReplyFlush("ERR IO");
    return;
  }
  if (crc != expectCrc) {
    sdSerDrainRx();
    SD.remove(path);
    char err[64];
    snprintf(err, sizeof(err), "ERR CRC got=%08lX want=%08lX", (unsigned long)crc, (unsigned long)expectCrc);
    sdSerReplyFlush(err);
    return;
  }
  sdSerReplyFlush("OK");
}

static bool sdSerWriteAll(const uint8_t *data, size_t len) {
  size_t off = 0;
  uint32_t start = millis();
  while (off < len) {
    size_t w = Serial.write(data + off, len - off);
    if (w == 0) {
      if ((millis() - start) > SD_SER_IO_TIMEOUT_MS) return false;
      yield();
      continue;
    }
    off += w;
    start = millis();
    yield();
  }
#if defined(CORE_TEENSY)
  Serial.send_now();
#endif
  return true;
}

static bool sdSerWaitAck(uint32_t timeoutMs) {
  char buf[16];
  size_t len = 0;
  uint32_t start = millis();
  while ((millis() - start) <= timeoutMs) {
    while (Serial.available()) {
      int b = Serial.read();
      if (b < 0) break;
      char c = (char)b;
      if (c == '\n') {
        if (len > 0 && buf[len - 1] == '\r') len--;
        buf[len] = '\0';
        return strcasecmp(buf, "ACK") == 0;
      }
      if (c != '\r' && len + 1 < sizeof(buf)) buf[len++] = c;
      else if (c != '\r') len = 0;
    }
    yield();
  }
  return false;
}

static void sdSerCmdGet(const char *pathArg) {
  char path[128];
  if (!pathArg || !sdSerNormalizePath(pathArg, path, sizeof(path)) || strcmp(path, "/") == 0) {
    sdSerReply("ERR BAD_PATH");
    return;
  }
  if (!SD.exists(path)) {
    sdSerReply("ERR NOT_FOUND");
    return;
  }



  sdSerAudioStopForSd();

  File in = SD.open(path, FILE_READ);
  if (!in || in.isDirectory()) {
    if (in) in.close();
    sdSerReply("ERR OPEN");
    return;
  }

  uint32_t size = (uint32_t)in.size();

  sdSerReplyf("OK %lu", (unsigned long)size);
#if defined(CORE_TEENSY)
  Serial.send_now();
#endif

  if (!sdSerWaitAck(SD_SER_IO_TIMEOUT_MS)) {
    in.close();
    sdSerDrainRx();
    sdSerReply("ERR ACK");
    return;
  }

  static uint8_t chunk[SD_SER_GET_BLOCK];
  uint32_t crc = 0;
  uint32_t left = size;
  uint32_t sent = 0;
  while (left > 0) {
    size_t n = left > SD_SER_GET_BLOCK ? SD_SER_GET_BLOCK : (size_t)left;
    size_t got = in.read(chunk, n);
    if (got != n) {
      in.close();
      sdSerDrainRx();
      sdSerReply("ERR IO");
      return;
    }
    crc = sdSerCrc32Update(crc, chunk, got);
    if (!sdSerWriteAll(chunk, got)) {
      in.close();
      sdSerDrainRx();
      sdSerReply("ERR IO");
      return;
    }
    Serial.flush();
#if defined(CORE_TEENSY)
    Serial.send_now();
#endif
    if (!sdSerWaitAck(SD_SER_IO_TIMEOUT_MS)) {
      in.close();
      sdSerDrainRx();
      sdSerReply("ERR ACK");
      return;
    }
    left -= (uint32_t)got;
    sent += (uint32_t)got;
    sdSerLastClientMs = millis();

    if ((sent & 0x1fff) == 0) delayMicroseconds(200);
  }
  in.close();

  sdSerReplyf("CRC %08lX", (unsigned long)crc);
#if defined(CORE_TEENSY)
  Serial.send_now();
#endif
}


static void sdSerNoteHostPresent() {
  sdSerLastClientMs = millis();
  sdSerClientConnected = true;
}

static void sdSerExpireClient() {
  if (!sdSerClientConnected) return;
  if ((uint32_t)(millis() - sdSerLastClientMs) <= SD_SER_CLIENT_TIMEOUT_MS) return;
  sdSerClientConnected = false;
  if (sdSerActive) {
    extern void menuRequestFullRedraw();
    menuRequestFullRedraw();
  }
}

static void sdSerTouchClient() {
  const bool was = sdSerClientConnected;
  sdSerNoteHostPresent();
  if (was) return;

  sdSerAudioStopForSd();
  extern void menuRequestFullRedraw();
  menuRequestFullRedraw();
}

static char *sdSerSkipSpaces(char *s) {
  while (s && (*s == ' ' || *s == '\t')) s++;
  return s;
}


static const size_t SD_PAT_NOTES = 256u * 16u;
static const size_t SD_PAT_NOTE_BYTES = SD_PAT_NOTES * 4u;
static const size_t SD_PAT_BYTES = SD_PAT_NOTE_BYTES + 2u + 4u + 8u + SD_PAT_NOTES;
static const uint8_t SD_PAT_PITCH_HDR[8] = { 'T', 'P', 'I', 'T', 1, 0x10, 0x00, 0 };
static EXTMEM uint8_t sdPatBuf[SD_PAT_BYTES];

static void sdPatPack() {
  size_t i = 0;
  for (unsigned int x = 1; x <= 256; x++) {
    for (unsigned int y = 1; y <= 16; y++) {
      sdPatBuf[i++] = note[x][y].channel;
      sdPatBuf[i++] = note[x][y].velocity;
      sdPatBuf[i++] = note[x][y].probability;
      sdPatBuf[i++] = note[x][y].condition;
    }
  }
  sdPatBuf[i++] = 0xFF;
  sdPatBuf[i++] = 0xFE;
  float bpm = SMP.bpm;
  memcpy(sdPatBuf + i, &bpm, sizeof(bpm));
  i += sizeof(bpm);
  memcpy(sdPatBuf + i, SD_PAT_PITCH_HDR, sizeof(SD_PAT_PITCH_HDR));
  i += sizeof(SD_PAT_PITCH_HDR);
  for (unsigned int x = 1; x <= 256; x++) {
    for (unsigned int y = 1; y <= 16; y++) {
      sdPatBuf[i++] = note[x][y].midiPitch;
    }
  }
}

static bool sdPatApply() {
  if (sdPatBuf[SD_PAT_NOTE_BYTES] != 0xFF || sdPatBuf[SD_PAT_NOTE_BYTES + 1] != 0xFE) return false;
  const size_t pitchAt = SD_PAT_NOTE_BYTES + 2u + 4u;
  if (memcmp(sdPatBuf + pitchAt, SD_PAT_PITCH_HDR, sizeof(SD_PAT_PITCH_HDR)) != 0) return false;

  size_t i = 0;
  for (unsigned int x = 1; x <= 256; x++) {
    for (unsigned int y = 1; y <= 16; y++) {
      note[x][y].channel = sdPatBuf[i++];
      note[x][y].velocity = sdPatBuf[i++];
      note[x][y].probability = sdPatBuf[i++];
      note[x][y].condition = sdPatBuf[i++];
      note[x][y].midiPitch = NOTE_MIDI_PITCH_NONE;
    }
  }

  float bpm = 0;
  memcpy(&bpm, sdPatBuf + SD_PAT_NOTE_BYTES + 2, sizeof(bpm));
  if (bpm >= 40.0f && bpm <= 300.0f) {
    SMP.bpm = bpm;
    bpmMode.pos[3] = (unsigned int)bpm;
    playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
    playTimer.update((uint32_t)round(playNoteInterval));
  }

  size_t p = pitchAt + sizeof(SD_PAT_PITCH_HDR);
  for (unsigned int x = 1; x <= 256; x++) {
    for (unsigned int y = 1; y <= 16; y++) {
      uint8_t stored = sdPatBuf[p++];
      note[x][y].midiPitch =
          (stored <= 127 || stored == NOTE_MIDI_PITCH_NONE) ? stored : NOTE_MIDI_PITCH_NONE;
    }
  }
  return true;
}

static void sdSerCmdGetPat() {
  sdPatPack();
  sdSerAudioStopForSd();
  sdSerReplyf("OK %lu", (unsigned long)SD_PAT_BYTES);
#if defined(CORE_TEENSY)
  Serial.send_now();
#endif
  if (!sdSerWaitAck(SD_SER_IO_TIMEOUT_MS)) {
    sdSerDrainRx();
    sdSerReply("ERR ACK");
    return;
  }

  uint32_t crc = 0;
  uint32_t left = (uint32_t)SD_PAT_BYTES;
  uint32_t sent = 0;
  while (left > 0) {
    size_t n = left > SD_SER_GET_BLOCK ? SD_SER_GET_BLOCK : (size_t)left;
    crc = sdSerCrc32Update(crc, sdPatBuf + sent, n);
    if (!sdSerWriteAll(sdPatBuf + sent, n)) {
      sdSerDrainRx();
      sdSerReply("ERR IO");
      return;
    }
    Serial.flush();
#if defined(CORE_TEENSY)
    Serial.send_now();
#endif
    if (!sdSerWaitAck(SD_SER_IO_TIMEOUT_MS)) {
      sdSerDrainRx();
      sdSerReply("ERR ACK");
      return;
    }
    left -= (uint32_t)n;
    sent += (uint32_t)n;
    sdSerLastClientMs = millis();
    if ((sent & 0x1fff) == 0) delayMicroseconds(200);
  }
  sdSerReplyf("CRC %08lX", (unsigned long)crc);
#if defined(CORE_TEENSY)
  Serial.send_now();
#endif
  extern void showDrawAfterPatternTransfer();
  showDrawAfterPatternTransfer();
}

static void sdSerCmdPutPat(char *args) {
  char *sizeTok = strtok(args ? args : (char *)"", " \t");
  char *crcTok = strtok(NULL, " \t");
  if (!sizeTok || !crcTok) {
    sdSerReply("ERR USAGE");
    return;
  }
  uint32_t size = (uint32_t)strtoul(sizeTok, NULL, 10);
  uint32_t expectCrc = (uint32_t)strtoul(crcTok, NULL, 16);
  if (size != (uint32_t)SD_PAT_BYTES) {
    sdSerReply("ERR SIZE");
    return;
  }

  sdSerReplyFlush("READY");
  sdSerAudioStopForSd();

  uint32_t remaining = size;
  uint32_t crc = 0;
  uint32_t filled = 0;
  bool ok = true;
  while (remaining > 0) {
    size_t block = remaining > SD_SER_PUT_BLOCK ? SD_SER_PUT_BLOCK : (size_t)remaining;
    size_t got = 0;
    while (got < block) {
      size_t n = (block - got) > SD_SER_CHUNK ? SD_SER_CHUNK : (block - got);
      if (!sdSerReadExact(sdPatBuf + filled, n, SD_SER_IO_TIMEOUT_MS)) {
        ok = false;
        break;
      }
      crc = sdSerCrc32Update(crc, sdPatBuf + filled, n);
      got += n;
      filled += (uint32_t)n;
      remaining -= (uint32_t)n;
      sdSerLastClientMs = millis();
    }
    if (!ok) break;
    sdSerReplyFlush("ACK");
  }
  if (!ok) {
    sdSerDrainRx();
    sdSerReplyFlush("ERR IO");
    return;
  }
  if (crc != expectCrc) {
    sdSerDrainRx();
    char err[64];
    snprintf(err, sizeof(err), "ERR CRC got=%08lX want=%08lX", (unsigned long)crc, (unsigned long)expectCrc);
    sdSerReplyFlush(err);
    return;
  }
  if (!sdPatApply()) {
    sdSerReplyFlush("ERR PATTERN");
    return;
  }
  sdSerReplyFlush("OK");
  extern void showDrawAfterPatternTransfer();
  showDrawAfterPatternTransfer();
}

static void sdSerHandleLine(char *line) {
  line = sdSerSkipSpaces(line);
  if (*line == '\0') return;

  sdSerTouchClient();

  char cmd[16];
  size_t i = 0;
  while (line[i] && line[i] != ' ' && line[i] != '\t' && i + 1 < sizeof(cmd)) {
    cmd[i] = line[i];
    i++;
  }
  cmd[i] = '\0';
  char *args = sdSerSkipSpaces(line + i);

  if (strcasecmp(cmd, "PING") == 0) {
    sdSerCmdPing ();
  } else if (strcasecmp(cmd, "LIST") == 0) {
    sdSerCmdList(args && *args ? args : "/");
  } else if (strcasecmp(cmd, "RM") == 0) {
    sdSerCmdRm(args);
  } else if (strcasecmp(cmd, "MKDIR") == 0) {
    sdSerCmdMkdir(args);
  } else if (strcasecmp(cmd, "MV") == 0 || strcasecmp(cmd, "REN") == 0) {
    if (args && *args) sdSerCmdMv(args);
    else sdSerReply("ERR USAGE");
  } else if (strcasecmp(cmd, "PUT") == 0) {
    if (args && *args) sdSerCmdPut(args);
    else sdSerReply("ERR USAGE");
  } else if (strcasecmp(cmd, "GET") == 0) {
    sdSerCmdGet(args);
  } else if (strcasecmp(cmd, "GETPAT") == 0) {
    sdSerCmdGetPat();
  } else if (strcasecmp(cmd, "PUTPAT") == 0) {
    sdSerCmdPutPat(args);
  } else {
    sdSerReply("ERR UNKNOWN");
  }
}

bool sdSerialServerIsActive() {
  return sdSerActive;
}

bool sdSerialServerClientConnected() {


  if (!sdSerClientConnected) return false;
  return (uint32_t)(millis() - sdSerLastClientMs) <= SD_SER_CLIENT_TIMEOUT_MS;
}

void sdSerialServerSetActive(bool on) {
  if (on == sdSerActive) return;

  if (on) {


    Serial.setTimeout(50);
    sdSerDrainRx();
    sdSerExpireClient();
    sdSerActive = true;
    sdSerReplyf("OK TOERN SD %s", VERSION);
  } else {
    sdSerActive = false;
    sdSerDrainRx();
  }
}

void sdSerialServerPoll() {
  if (!sdSerActive) return;

  sdSerExpireClient();

  while (Serial.available()) {
    int b = Serial.read();
    if (b < 0) break;
    char c = (char)b;
    if (c == '\n') {
      sdSerLineBuf[sdSerLineLen] = '\0';
      if (sdSerLineLen > 0 && sdSerLineBuf[sdSerLineLen - 1] == '\r') {
        sdSerLineBuf[sdSerLineLen - 1] = '\0';
      }
      sdSerHandleLine(sdSerLineBuf);
      sdSerLineLen = 0;
      if (!sdSerActive) return;
    } else if (c != '\r') {
      if (sdSerLineLen + 1 < sizeof(sdSerLineBuf)) {
        sdSerLineBuf[sdSerLineLen++] = c;
      } else {
        sdSerLineLen = 0;
        sdSerReply("ERR LINE");
      }
    }
  }
}


static void sdSerHandleOffPage(char *line) {
  line = sdSerSkipSpaces(line);
  if (line[0] == '\0') return;

  char cmd[16];
  size_t i = 0;
  while (line[i] && line[i] != ' ' && line[i] != '\t' && i + 1 < sizeof(cmd)) {
    cmd[i] = line[i];
    i++;
  }
  cmd[i] = '\0';
  char *args = sdSerSkipSpaces(line + i);

  if (strcasecmp(cmd, "PING") == 0) {
    sdSerNoteHostPresent();
    sdSerReplyf("OK TOERN SD %s", VERSION);
    return;
  }



  if (strcasecmp(cmd, "LIST") == 0) {
    sdSerNoteHostPresent();
    sdSerCmdList(args && *args ? args : "/");
  } else if (strcasecmp(cmd, "RM") == 0) {
    sdSerNoteHostPresent();
    sdSerCmdRm(args);
  } else if (strcasecmp(cmd, "MKDIR") == 0) {
    sdSerNoteHostPresent();
    sdSerCmdMkdir(args);
  } else if (strcasecmp(cmd, "MV") == 0 || strcasecmp(cmd, "REN") == 0) {
    sdSerNoteHostPresent();
    if (args && *args) sdSerCmdMv(args);
    else sdSerReply("ERR USAGE");
  } else if (strcasecmp(cmd, "PUT") == 0) {
    sdSerNoteHostPresent();
    if (args && *args) sdSerCmdPut(args);
    else sdSerReply("ERR USAGE");
  } else if (strcasecmp(cmd, "GET") == 0) {
    sdSerNoteHostPresent();
    sdSerCmdGet(args);
  } else if (strcasecmp(cmd, "GETPAT") == 0) {
    sdSerNoteHostPresent();
    sdSerCmdGetPat();
  } else if (strcasecmp(cmd, "PUTPAT") == 0) {
    sdSerNoteHostPresent();
    sdSerCmdPutPat(args);
  } else {
    sdSerReply("ERR NEED_SD");
  }
}

void sdSerialServerPollNeedSdHint() {
  if (sdSerActive) return;
  sdSerExpireClient();



  extern bool inEtcSubmenu;
  extern int getCurrentMenuMainSetting();
  const bool onColr = inEtcSubmenu && (getCurrentMenuMainSetting() == 41);

  while (Serial.available()) {
    int peek = Serial.peek();
    if (peek < 0) break;
    if (onColr && (peek == 'C' || peek == 'B' || peek == 'S')) return;

    int b = Serial.read();
    if (b < 0) break;
    char c = (char)b;
    if (c == '\n') {
      sdSerLineBuf[sdSerLineLen] = '\0';
      if (sdSerLineLen > 0 && sdSerLineBuf[sdSerLineLen - 1] == '\r') {
        sdSerLineBuf[sdSerLineLen - 1] = '\0';
      }
      sdSerHandleOffPage(sdSerLineBuf);
      sdSerLineLen = 0;
    } else if (c != '\r') {
      if (sdSerLineLen + 1 < sizeof(sdSerLineBuf)) {
        sdSerLineBuf[sdSerLineLen++] = c;
      } else {
        sdSerLineLen = 0;
      }
    }
  }
}

#endif
# 1 "/Users/jank/git/toern/toern_synths.ino"
#define POLY_VOICES 3
#define INSTRUMENT_CHANNELS 2
#define MAXSLIDER 32.0





#define NOTE_EMPTY INT16_MIN

unsigned long voiceStartTime[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 0, 0, 0 },
  { 0, 0, 0 },
};

int16_t cents[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 0, 0, 0 },
  { 0, 0, 0 },
};
int16_t semitones[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 0, 0, 0 },
  { 0, 0, 0 },
};
int16_t pan[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 0, 0, 0 },
  { 0, 0, 0 },
};
uint8_t volume[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 100, 100, 100 },
  { 100, 100, 100 },
};

uint16_t attackAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 100, 100, 100 },
  { 100, 100, 100 },
};
uint16_t decayAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 100, 100, 100 },
  { 100, 100, 100 },
};
float sustainAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 1, 1, 1 },
  { 1, 1, 1 },
};
uint16_t releaseAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 100, 100, 100 },
  { 100, 100, 100 },
};

uint16_t attackFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 100, 100, 100 },
  { 100, 100, 100 },
};
uint16_t decayFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 100, 100, 100 },
  { 100, 100, 100 },
};
float sustainFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 1, 1, 1 },
  { 1, 1, 1 },
};
uint16_t releaseFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 100, 100, 100 },
  { 100, 100, 100 },
};

uint16_t cutoff[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 1000, 1000, 1000 },
  { 1000, 1000, 1000 },
};
float resonance[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 2.0f, 2.0f, 2.0f },
  { 2.0f, 2.0f, 2.0f },
};
float filterAmount[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 2.0f, 2.0f, 2.0f },
  { 2.0f, 2.0f, 2.0f },
};

uint8_t waveforms[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH },
  { WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH },
};
uint8_t waveformsArray[INSTRUMENT_CHANNELS][5] = {
  { WAVEFORM_SAWTOOTH, WAVEFORM_PULSE, WAVEFORM_SQUARE, WAVEFORM_TRIANGLE, WAVEFORM_SINE },
  { WAVEFORM_SAWTOOTH, WAVEFORM_PULSE, WAVEFORM_SQUARE, WAVEFORM_TRIANGLE, WAVEFORM_SINE },
};
float pulseWidth[INSTRUMENT_CHANNELS][POLY_VOICES] = {
  { 0.25f, 0.25f, 0.25f },
  { 0.25f, 0.25f, 0.25f },
};


int16_t noteArray[INSTRUMENT_CHANNELS][8] = {
  { NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY },
  { NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY, NOTE_EMPTY },
};
int16_t notePlaying[INSTRUMENT_CHANNELS] = { 0, 0 };

static uint32_t sequencedSoundCurrentTick = 0;
static uint32_t sequencedSoundLastTick = 0;
static bool sequencedSoundLegato = false;
static bool sequencedSoundTouched[POLY_VOICES] = {};

static float synthNoteFrequency(int noteIndex) {
  if (noteIndex >= 0 && noteIndex < 108) return notesArray[noteIndex];


  return NOTE_C0 * powf(2.0f, (float)noteIndex / 12.0f);
}

static void setSoundVoice(int note, int ch, int velocity, int voiceIdx,
                          bool triggerEnvelope) {
  int noteIdx0 = note + semitones[ch][0];
  int noteIdx1 = note + semitones[ch][1];
  int noteIdx2 = note + semitones[ch][2];

  float f0 = synthNoteFrequency(noteIdx0) * powf(2.0f, (cents[ch][0] + (float)random(-2, 3)) / 1200.0f);
  float f1 = synthNoteFrequency(noteIdx1) * powf(2.0f, (cents[ch][1] + (float)random(-2, 3)) / 1200.0f);
  float f2 = synthNoteFrequency(noteIdx2) * powf(2.0f, (cents[ch][2] + (float)random(-2, 3)) / 1200.0f);

  int velMidi = constrain(velocity > 0 ? velocity : defaultVelocity, 1, 127);
  float amp = mapf((float)velMidi, 1.0f, 127.0f, 0.0f, 1.0f);

  toern_audio::StateGuard guard;
  noteArray[ch][voiceIdx] = note;
  voiceStartTime[ch][voiceIdx] = millis();
  Swaveform1[voiceIdx]->frequency(f0);
  Swaveform2[voiceIdx]->frequency(f1);
  Swaveform3[voiceIdx]->frequency(f2);
  Swaveform1[voiceIdx]->amplitude(amp);
  Swaveform2[voiceIdx]->amplitude(amp);
  Swaveform3[voiceIdx]->amplitude(amp);
  if (triggerEnvelope) {
    if (ch == 0 && SMP.synth_settings[11][INSTRUMENT] == 10) {

      Sdc1[voiceIdx]->amplitude(0.60f + 0.40f * amp);
    } else {
      Sdc1[voiceIdx]->amplitude(1.0f);
    }
    Senvelope1[voiceIdx]->noteOn();
    Senvelope2[voiceIdx]->noteOn();
    SenvelopeFilter1[voiceIdx]->noteOn();
  }
}



void arp_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {

  octave[ch] = 5.0;
  cents[ch][0] = 7; cents[ch][1] = -7; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 12; semitones[ch][2] = 19;
  pan[ch][0] = -70; pan[ch][1] = 70; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 65; volume[ch][2] = 35;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 0; decayAmp[ch][1] = 0; decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 2; releaseAmp[ch][1] = 2; releaseAmp[ch][2] = 2;

  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 0; decayFilter[ch][1] = 0; decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 2; releaseFilter[ch][1] = 2; releaseFilter[ch][2] = 2;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float minResonance = 0.1, maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;


  waveforms[ch][0] = 1;
  waveforms[ch][1] = 1;
  waveforms[ch][2] = 0;






  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }


  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;


  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void lead_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 0; cents[ch][1] = -7; cents[ch][2] = 7;
  semitones[ch][0] = 0; semitones[ch][1] = 12; semitones[ch][2] = 0;
  pan[ch][0] = 0; pan[ch][1] = 35; pan[ch][2] = -35;
  volume[ch][0] = 100; volume[ch][1] = 45; volume[ch][2] = 20;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 250; decayAmp[ch][1] = 250; decayAmp[ch][2] = 250;
  sustainAmp[ch][0] = 0; sustainAmp[ch][1] = 0; sustainAmp[ch][2] = 0;
  releaseAmp[ch][0] = 250; releaseAmp[ch][1] = 250; releaseAmp[ch][2] = 250;

  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 250; decayFilter[ch][1] = 250; decayFilter[ch][2] = 250;
  sustainFilter[ch][0] = 0; sustainFilter[ch][1] = 0; sustainFilter[ch][2] = 0;
  releaseFilter[ch][0] = 250; releaseFilter[ch][1] = 250; releaseFilter[ch][2] = 250;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float minResonance = 0.1, maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 2;
  waveforms[ch][1] = 1;
  waveforms[ch][2] = 3;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void keys_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 2; cents[ch][1] = -2; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 2500; decayAmp[ch][1] = 2500; decayAmp[ch][2] = 2500;
  sustainAmp[ch][0] = 0; sustainAmp[ch][1] = 0; sustainAmp[ch][2] = 0;
  releaseAmp[ch][0] = 300; releaseAmp[ch][1] = 300; releaseAmp[ch][2] = 300;

  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 2500; decayFilter[ch][1] = 2500; decayFilter[ch][2] = 2500;
  sustainFilter[ch][0] = 0; sustainFilter[ch][1] = 0; sustainFilter[ch][2] = 0;
  releaseFilter[ch][0] = 300; releaseFilter[ch][1] = 300; releaseFilter[ch][2] = 300;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float minResonance = 0.1, maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 0;
  waveforms[ch][1] = 0;
  waveforms[ch][2] = 0;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void pad_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 15; cents[ch][1] = -15; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 0;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 60;

  attackAmp[ch][0] = 1500; attackAmp[ch][1] = 1500; attackAmp[ch][2] = 1500;
  sustainAmp[ch][0] = 0.6; sustainAmp[ch][1] = 0.6; sustainAmp[ch][2] = 0.6;
  releaseAmp[ch][0] = 1500; releaseAmp[ch][1] = 1500; releaseAmp[ch][2] = 1500;

  attackFilter[ch][0] = 1500; attackFilter[ch][1] = 1500; attackFilter[ch][2] = 1500;
  sustainFilter[ch][0] = 0.2; sustainFilter[ch][1] = 0.2; sustainFilter[ch][2] = 0.2;
  releaseFilter[ch][0] = 1500; releaseFilter[ch][1] = 1500; releaseFilter[ch][2] = 1500;


  float minDecay = 60.0, maxDecay = 3000.0;
  float mappedDecay = minDecay + (p1 / MAXSLIDER) * (maxDecay - minDecay);
  decayAmp[ch][0] = decayAmp[ch][1] = decayAmp[ch][2] = mappedDecay;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p2 / MAXSLIDER) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p3 / MAXSLIDER) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 4; waveforms[ch][1] = 4; waveforms[ch][2] = 4;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void organ_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 5.0;
  cents[ch][0] = 0; cents[ch][1] = 0; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 12; semitones[ch][2] = 19;
  pan[ch][0] = 0; pan[ch][1] = 0; pan[ch][2] = 0;
  volume[ch][0] = 85; volume[ch][1] = 70; volume[ch][2] = 55;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 100; decayAmp[ch][1] = 100; decayAmp[ch][2] = 100;
  sustainAmp[ch][0] = 0.1; sustainAmp[ch][1] = 0.1; sustainAmp[ch][2] = 0.1;
  releaseAmp[ch][0] = 100; releaseAmp[ch][1] = 100; releaseAmp[ch][2] = 100;

  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 100; decayFilter[ch][1] = 100; decayFilter[ch][2] = 100;
  sustainFilter[ch][0] = 0.6; sustainFilter[ch][1] = 0.6; sustainFilter[ch][2] = 0.6;
  releaseFilter[ch][0] = 100; releaseFilter[ch][1] = 100; releaseFilter[ch][2] = 100;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float minResonance = 0.1, maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 4; waveforms[ch][1] = 4; waveforms[ch][2] = 4;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void flute_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 5.0;
  cents[ch][0] = -1; cents[ch][1] = 1; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 45;

  decayAmp[ch][0] = 0; decayAmp[ch][1] = 0; decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 400; releaseAmp[ch][1] = 400; releaseAmp[ch][2] = 400;

  attackFilter[ch][0] = 500; attackFilter[ch][1] = 500; attackFilter[ch][2] = 500;
  decayFilter[ch][0] = 0; decayFilter[ch][1] = 0; decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 500; releaseFilter[ch][1] = 500; releaseFilter[ch][2] = 500;


  float minAttack = 100.0, maxAttack = 2000.0;
  float mappedAttack = minAttack + (p1 / MAXSLIDER) * (maxAttack - minAttack);
  attackAmp[ch][0] = attackAmp[ch][1] = attackAmp[ch][2] = mappedAttack;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p2 / MAXSLIDER) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p3 / MAXSLIDER) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 3; waveforms[ch][1] = 3; waveforms[ch][2] = 4;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void wow_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 12; cents[ch][1] = -12; cents[ch][2] = 4;
  semitones[ch][0] = 0; semitones[ch][1] = 7; semitones[ch][2] = 19;
  pan[ch][0] = -80; pan[ch][1] = 80; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;
  attackAmp[ch][0] = 400; attackAmp[ch][1] = 400; attackAmp[ch][2] = 400;
  decayAmp[ch][0] = 4000; decayAmp[ch][1] = 4000; decayAmp[ch][2] = 4000;
  sustainAmp[ch][0] = 0.8; sustainAmp[ch][1] = 0.8; sustainAmp[ch][2] = 0.8;


  float minRelease = 10.0, maxRelease = 1000.0;
  float mappedRelease = minRelease + (p3 / MAXSLIDER) * (maxRelease - minRelease);
  releaseAmp[ch][0] = releaseAmp[ch][1] = releaseAmp[ch][2] = mappedRelease;

  attackFilter[ch][0] = 800; attackFilter[ch][1] = 800; attackFilter[ch][2] = 800;
  decayFilter[ch][0] = 6000; decayFilter[ch][1] = 6000; decayFilter[ch][2] = 6000;
  sustainFilter[ch][0] = 0.2; sustainFilter[ch][1] = 0.2; sustainFilter[ch][2] = 0.2;
  releaseFilter[ch][0] = 10; releaseFilter[ch][1] = 10; releaseFilter[ch][2] = 10;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 3; waveforms[ch][1] = 3; waveforms[ch][2] = 0;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void bass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 2.0;
  cents[ch][0] = 5; cents[ch][1] = -5; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = -12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 400; decayAmp[ch][1] = 400; decayAmp[ch][2] = 400;
  sustainAmp[ch][0] = 0.0; sustainAmp[ch][1] = 0.0; sustainAmp[ch][2] = 0.0;
  releaseAmp[ch][0] = 10; releaseAmp[ch][1] = 10; releaseAmp[ch][2] = 10;

  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 250; decayFilter[ch][1] = 250; decayFilter[ch][2] = 250;
  sustainFilter[ch][0] = 0.0; sustainFilter[ch][1] = 0.0; sustainFilter[ch][2] = 0.0;
  releaseFilter[ch][0] = 10; releaseFilter[ch][1] = 10; releaseFilter[ch][2] = 10;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float minResonance = 0.1, maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;


  waveforms[ch][0] = 0; waveforms[ch][1] = 0; waveforms[ch][2] = 1;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void brass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = -7; cents[ch][1] = 7; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = -12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 55;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 2500; decayAmp[ch][1] = 2500; decayAmp[ch][2] = 2500;
  sustainAmp[ch][0] = 0.8; sustainAmp[ch][1] = 0.8; sustainAmp[ch][2] = 0.8;
  releaseAmp[ch][0] = 250; releaseAmp[ch][1] = 250; releaseAmp[ch][2] = 250;

  attackFilter[ch][0] = 100; attackFilter[ch][1] = 100; attackFilter[ch][2] = 100;
  decayFilter[ch][0] = 2500; decayFilter[ch][1] = 2500; decayFilter[ch][2] = 2500;
  sustainFilter[ch][0] = 0.8; sustainFilter[ch][1] = 0.8; sustainFilter[ch][2] = 0.8;
  releaseFilter[ch][0] = 250; releaseFilter[ch][1] = 250; releaseFilter[ch][2] = 250;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float minResonance = 0.1, maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 0; waveforms[ch][1] = 0; waveforms[ch][2] = 2;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void chiptune_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 0; cents[ch][1] = 0; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 10;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 0; decayAmp[ch][1] = 0; decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 100; releaseAmp[ch][1] = 100; releaseAmp[ch][2] = 100;

  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 0; decayFilter[ch][1] = 0; decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 100; releaseFilter[ch][1] = 100; releaseFilter[ch][2] = 100;


  float minCutoff = 400.0, maxCutoff = 6000.0;
  float minResonance = 0.1, maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 1; waveforms[ch][1] = 1; waveforms[ch][2] = 1;


  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void acid303_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 2.0;
  cents[ch][0] = 0; cents[ch][1] = 2; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = -12;
  pan[ch][0] = 0; pan[ch][1] = 0; pan[ch][2] = 0;

  volume[ch][0] = 100; volume[ch][1] = 75; volume[ch][2] = 60;
  pulseWidth[ch][0] = 0.50f; pulseWidth[ch][1] = 0.50f; pulseWidth[ch][2] = 0.50f;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 560; decayAmp[ch][1] = 560; decayAmp[ch][2] = 560;
  sustainAmp[ch][0] = 0.56; sustainAmp[ch][1] = 0.56; sustainAmp[ch][2] = 0.56;
  releaseAmp[ch][0] = 375; releaseAmp[ch][1] = 375; releaseAmp[ch][2] = 375;



  uint16_t fltDecay = (uint16_t)(210.0f + (p3 / MAXSLIDER) * 240.0f);
  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = fltDecay; decayFilter[ch][1] = fltDecay; decayFilter[ch][2] = fltDecay;
  sustainFilter[ch][0] = 0.25; sustainFilter[ch][1] = 0.25; sustainFilter[ch][2] = 0.25;
  releaseFilter[ch][0] = 375; releaseFilter[ch][1] = 375; releaseFilter[ch][2] = 375;



  float minCutoff = 320.0, maxCutoff = 4200.0;
  float minResonance = 0.12, maxResonance = 0.88;
  float minFilterAmount = 0.4, maxFilterAmount = 3.2;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;


  waveforms[ch][0] = 0; waveforms[ch][1] = 1; waveforms[ch][2] = 0;

  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
    cents[ch][i] += offsetCents;
    semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void dx7_epiano_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;

  cents[ch][0] = -6; cents[ch][1] = 6; cents[ch][2] = 2;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 19;
  pan[ch][0] = -50; pan[ch][1] = 50; pan[ch][2] = 0;

  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 85;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 875; decayAmp[ch][1] = 875; decayAmp[ch][2] = 875;
  sustainAmp[ch][0] = 0.625; sustainAmp[ch][1] = 0.625; sustainAmp[ch][2] = 0.625;
  releaseAmp[ch][0] = 500; releaseAmp[ch][1] = 500; releaseAmp[ch][2] = 500;


  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 350; decayFilter[ch][1] = 350; decayFilter[ch][2] = 350;
  sustainFilter[ch][0] = 0.45; sustainFilter[ch][1] = 0.45; sustainFilter[ch][2] = 0.45;
  releaseFilter[ch][0] = 500; releaseFilter[ch][1] = 500; releaseFilter[ch][2] = 500;

  float minCutoff = 1100.0, maxCutoff = 6800.0;
  float minResonance = 0.05, maxResonance = 0.72;
  float minFilterAmount = 0.2, maxFilterAmount = 2.2;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;


  waveforms[ch][0] = 3; waveforms[ch][1] = 4; waveforms[ch][2] = 3;

  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
    cents[ch][i] += offsetCents;
    semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



void dx7_bass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 2.0;

  cents[ch][0] = 0; cents[ch][1] = 3; cents[ch][2] = -2;
  semitones[ch][0] = 0; semitones[ch][1] = 12; semitones[ch][2] = 0;
  pan[ch][0] = 0; pan[ch][1] = 0; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 90; volume[ch][2] = 100;

  pulseWidth[ch][0] = 0.16f; pulseWidth[ch][1] = 0.25f; pulseWidth[ch][2] = 0.50f;

  attackAmp[ch][0] = 0; attackAmp[ch][1] = 0; attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 500; decayAmp[ch][1] = 500; decayAmp[ch][2] = 500;
  sustainAmp[ch][0] = 0.50; sustainAmp[ch][1] = 0.50; sustainAmp[ch][2] = 0.50;
  releaseAmp[ch][0] = 375; releaseAmp[ch][1] = 375; releaseAmp[ch][2] = 375;


  attackFilter[ch][0] = 0; attackFilter[ch][1] = 0; attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 180; decayFilter[ch][1] = 180; decayFilter[ch][2] = 180;
  sustainFilter[ch][0] = 0.30; sustainFilter[ch][1] = 0.30; sustainFilter[ch][2] = 0.30;
  releaseFilter[ch][0] = 375; releaseFilter[ch][1] = 375; releaseFilter[ch][2] = 375;

  float minCutoff = 480.0, maxCutoff = 5200.0;
  float minResonance = 0.10, maxResonance = 0.82;
  float minFilterAmount = 0.3, maxFilterAmount = 2.6;
  float mappedCutoff = minCutoff + (p1 / MAXSLIDER) * (maxCutoff - minCutoff);
  float mappedResonance = minResonance + (p2 / MAXSLIDER) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / MAXSLIDER) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;


  waveforms[ch][0] = 1; waveforms[ch][1] = 0; waveforms[ch][2] = 3;

  float offsetCents = -50.0 + (p4 / MAXSLIDER) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / MAXSLIDER) * 4.0);
  for (int i = 0; i < 3; i++) {
    cents[ch][i] += offsetCents;
    semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / MAXSLIDER) * 7.0;
  int newWaveform = (int)floor((p6 / MAXSLIDER) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}




static inline uint8_t synthWaveformBeginType(int ch, int oscIdx) {
  uint8_t col = waveforms[ch][oscIdx];
  if (col > 4) col = 4;
  return waveformsArray[ch][col];
}



void updateVals(int ch) {

  if (ch < 0 || ch >= INSTRUMENT_CHANNELS) {
    return;
  }

  static uint8_t begunType[POLY_VOICES][3] = {
    { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 }
  };
  for (int i = 0; i < POLY_VOICES; i++) {

    Senvelope1[i]->attack(attackAmp[ch][i]);
    Senvelope1[i]->decay(decayAmp[ch][i]);
    Senvelope1[i]->sustain(sustainAmp[ch][i]);
    Senvelope1[i]->release(releaseAmp[ch][i]);

    Senvelope2[i]->attack(attackAmp[ch][i]);
    Senvelope2[i]->decay(decayAmp[ch][i]);
    Senvelope2[i]->sustain(sustainAmp[ch][i]);
    Senvelope2[i]->release(releaseAmp[ch][i]);


    SenvelopeFilter1[i]->attack(attackFilter[ch][i]);
    SenvelopeFilter1[i]->decay(decayFilter[ch][i]);
    SenvelopeFilter1[i]->sustain(sustainFilter[ch][i]);
    SenvelopeFilter1[i]->release(releaseFilter[ch][i]);

    uint8_t wf0 = synthWaveformBeginType(ch, 0);
    uint8_t wf1 = synthWaveformBeginType(ch, 1);
    uint8_t wf2 = synthWaveformBeginType(ch, 2);

    if (begunType[i][0] != wf0) { Swaveform1[i]->begin(wf0); begunType[i][0] = wf0; }
    if (begunType[i][1] != wf1) { Swaveform2[i]->begin(wf1); begunType[i][1] = wf1; }
    if (begunType[i][2] != wf2) { Swaveform3[i]->begin(wf2); begunType[i][2] = wf2; }
    Swaveform1[i]->amplitude(1.0);
    Swaveform1[i]->pulseWidth(pulseWidth[ch][0]);
    Swaveform2[i]->amplitude(1.0);
    Swaveform2[i]->pulseWidth(pulseWidth[ch][1]);
    Swaveform3[i]->amplitude(1.0);
    Swaveform3[i]->pulseWidth(pulseWidth[ch][2]);


    Sladder1[i]->frequency(cutoff[ch][i]);
    Sladder1[i]->octaveControl(filterAmount[ch][i]);
    Sladder1[i]->resonance(resonance[ch][i]);

    Sladder2[i]->frequency(cutoff[ch][i]);
    Sladder2[i]->octaveControl(filterAmount[ch][i]);
    Sladder2[i]->resonance(resonance[ch][i]);





    float gainMultiplier = 0.33f;
    {
      int inst = constrain((int)SMP.synth_settings[11][INSTRUMENT], 0, 12);
      if (inst >= 10) gainMultiplier = 0.52f;
    }
    Smixer1[i]->gain(0, max(0, (-pan[ch][0] + 100.0) * 0.005) * (volume[ch][0] / 100.0) * gainMultiplier);
    Smixer1[i]->gain(1, max(0, (-pan[ch][1] + 100.0) * 0.005) * (volume[ch][1] / 100.0) * gainMultiplier);
    Smixer1[i]->gain(2, max(0, (-pan[ch][2] + 100.0) * 0.005) * (volume[ch][2] / 100.0) * gainMultiplier);

    Smixer2[i]->gain(0, max(0, (pan[ch][0] + 100.0) * 0.005) * (volume[ch][0] / 100.0) * gainMultiplier);
    Smixer2[i]->gain(1, max(0, (pan[ch][1] + 100.0) * 0.005) * (volume[ch][1] / 100.0) * gainMultiplier);
    Smixer2[i]->gain(2, max(0, (pan[ch][2] + 100.0) * 0.005) * (volume[ch][2] / 100.0) * gainMultiplier);
  }
}

static uint8_t synthWaveformControlColumn(uint8_t rawValue) {
  uint8_t choice = constrain(
      (int)mapf(rawValue, 0, 16, 0, 3), 0, 3);
  switch (choice) {
    case 0: return 4;
    case 1: return 2;
    case 2: return 0;
    default: return 3;
  }
}

static void applySynthWaveformControl(int instrument, int ch,
                                      uint8_t rawValue) {
  extern uint8_t synthInstrumentWaveDefault(int instrumentIdx);




  if (synthWaveformControlColumn(rawValue) ==
      synthWaveformControlColumn(synthInstrumentWaveDefault(instrument))) {
    return;
  }

  const bool uniformRecipe =
      waveforms[ch][0] == waveforms[ch][1] &&
      waveforms[ch][1] == waveforms[ch][2];
  const uint8_t selectedColumn = synthWaveformControlColumn(rawValue);
  waveforms[ch][0] = selectedColumn;
  if (uniformRecipe) {
    waveforms[ch][1] = selectedColumn;
    waveforms[ch][2] = selectedColumn;
  }
  updateVals(ch);
}

static void releasePolyVoice(int voiceIdx) {
  Senvelope1[voiceIdx]->noteOff();
  Senvelope2[voiceIdx]->noteOff();
  SenvelopeFilter1[voiceIdx]->noteOff();
}


void playSound(int note, int ch, int velocity) {

  if (ch < 0 || ch >= INSTRUMENT_CHANNELS) {
    return;
  }
  extern void sidechainOnVoice(uint8_t);
  sidechainOnVoice(11);

  toern_audio::StateGuard guard;
  int voiceIdx = -1;
  for (int voice = 0; voice < POLY_VOICES; voice++) {
    if (noteArray[ch][voice] == note) {
      voiceIdx = voice;
      break;
    }
  }
  if (voiceIdx < 0) {
    for (int voice = 0; voice < POLY_VOICES; voice++) {
      if (noteArray[ch][voice] == NOTE_EMPTY) {
        voiceIdx = voice;
        break;
      }
    }
  }
  if (voiceIdx < 0) {
    unsigned long oldest = 0xFFFFFFFFUL;
    for (int voice = 0; voice < POLY_VOICES; voice++) {
      if (voiceStartTime[ch][voice] <= oldest) {
        oldest = voiceStartTime[ch][voice];
        voiceIdx = voice;
      }
    }
  }
  if (voiceIdx < 0) voiceIdx = 0;
  if (noteArray[ch][voiceIdx] != NOTE_EMPTY) {
    releasePolyVoice(voiceIdx);
    noteArray[ch][voiceIdx] = NOTE_EMPTY;
  }
  notePlaying[ch] = (voiceIdx + 1) % POLY_VOICES;
  setSoundVoice(note, ch, velocity, voiceIdx, true);
}



void playSequencedSound(int note, int ch, int velocity, uint32_t tick,
                        bool allowLegato) {
  if (ch < 0 || ch >= INSTRUMENT_CHANNELS) return;

  extern void sidechainOnVoice(uint8_t);
  sidechainOnVoice(11);

  if (sequencedSoundCurrentTick != tick) {
    sequencedSoundCurrentTick = tick;
    sequencedSoundLegato =
        sequencedSoundLastTick != 0 && tick == sequencedSoundLastTick + 1;
    for (int voice = 0; voice < POLY_VOICES; voice++) {
      sequencedSoundTouched[voice] = false;
    }
  }

  if (allowLegato && sequencedSoundLegato) {

    for (int voice = 0; voice < POLY_VOICES; voice++) {
      if (!sequencedSoundTouched[voice] && noteArray[ch][voice] == note) {
        sequencedSoundTouched[voice] = true;
        voiceStartTime[ch][voice] = millis();
        return;
      }
    }

    for (int voice = 0; voice < POLY_VOICES; voice++) {
      if (!sequencedSoundTouched[voice] && noteArray[ch][voice] != NOTE_EMPTY) {
        sequencedSoundTouched[voice] = true;
        setSoundVoice(note, ch, velocity, voice, false);
        return;
      }
    }
  }



  int voiceIdx = -1;
  for (int voice = 0; voice < POLY_VOICES; voice++) {
    if (!sequencedSoundTouched[voice] && noteArray[ch][voice] == note) {
      Senvelope1[voice]->noteOff();
      Senvelope2[voice]->noteOff();
      SenvelopeFilter1[voice]->noteOff();
      noteArray[ch][voice] = NOTE_EMPTY;
      voiceIdx = voice;
      break;
    }
  }
  for (int voice = 0; voice < POLY_VOICES; voice++) {
    if (voiceIdx < 0 && noteArray[ch][voice] == NOTE_EMPTY) {
      voiceIdx = voice;
      break;
    }
  }
  if (voiceIdx < 0) {
    for (int voice = 0; voice < POLY_VOICES; voice++) {
      if (!sequencedSoundTouched[voice]) {
        voiceIdx = voice;
        break;
      }
    }
  }
  if (voiceIdx < 0) voiceIdx = notePlaying[ch] % POLY_VOICES;
  if (noteArray[ch][voiceIdx] != NOTE_EMPTY) {
    Senvelope1[voiceIdx]->noteOff();
    Senvelope2[voiceIdx]->noteOff();
    SenvelopeFilter1[voiceIdx]->noteOff();
  }
  notePlaying[ch] = (voiceIdx + 1) % POLY_VOICES;
  sequencedSoundTouched[voiceIdx] = true;
  setSoundVoice(note, ch, velocity, voiceIdx, true);
}

void finishSequencedSoundTick(uint32_t tick, bool hadNotes) {
  if (hadNotes && sequencedSoundCurrentTick == tick) {
    if (sequencedSoundLegato) {
      for (int voice = 0; voice < POLY_VOICES; voice++) {
        if (!sequencedSoundTouched[voice] && noteArray[0][voice] != NOTE_EMPTY) {
          Senvelope1[voice]->noteOff();
          Senvelope2[voice]->noteOff();
          SenvelopeFilter1[voice]->noteOff();
          noteArray[0][voice] = NOTE_EMPTY;
        }
      }
    }
    sequencedSoundLastTick = tick;
    return;
  }

  if (!hadNotes && sequencedSoundLastTick != 0 &&
      tick == sequencedSoundLastTick + 1) {
    if (pressedKeyCount[11] == 0) {
      for (int voice = 0; voice < POLY_VOICES; voice++) {
        if (noteArray[0][voice] != NOTE_EMPTY) {
          Senvelope1[voice]->noteOff();
          Senvelope2[voice]->noteOff();
          SenvelopeFilter1[voice]->noteOff();
          noteArray[0][voice] = NOTE_EMPTY;
        }
      }
    }


    sequencedSoundLastTick = 0;
  }
}

void resetSequencedSynthLegato() {
  sequencedSoundCurrentTick = 0;
  sequencedSoundLastTick = 0;
  sequencedSoundLegato = false;
  for (int voice = 0; voice < POLY_VOICES; voice++) {
    sequencedSoundTouched[voice] = false;
    if (noteArray[0][voice] != NOTE_EMPTY) {
      Senvelope1[voice]->noteOff();
      Senvelope2[voice]->noteOff();
      SenvelopeFilter1[voice]->noteOff();
      noteArray[0][voice] = NOTE_EMPTY;
    }
  }
  sequencedMonoSynthLastTick[13] = 0;
  sequencedMonoSynthLastTick[14] = 0;
  sequencedMonoSynthLastPitch[13] = 0;
  sequencedMonoSynthLastPitch[14] = 0;
  sequencedMonoSynthActive[13] = false;
  sequencedMonoSynthActive[14] = false;
}





static unsigned synthAutoOffMsForInstrument(int inst) {
  static const uint16_t kMs[13] = {
    450,
    520,
    260,
    1700,
    750,
    580,
    520,
    460,
    480,
    700,
    480,
    850,
    520,
  };
  inst = constrain(inst, 0, 12);
  return kMs[inst];
}


void autoOffActiveNotes() {

if (pressedKeyCount[11]>=1) return;
  unsigned long currentTime = millis();
  int inst = constrain((int)SMP.synth_settings[11][INSTRUMENT], 0, 12);
  unsigned long offMs = synthAutoOffMsForInstrument(inst);
  for (int ch = 0; ch < 2; ch++) {


    if (ch == 0 && sequencedSoundLastTick != 0) continue;
    for (int i = 0; i < POLY_VOICES; i++) {
      if (noteArray[ch][i] != NOTE_EMPTY) {
        if (currentTime - voiceStartTime[ch][i] >= offMs) {
          stopSound(noteArray[ch][i], ch);
        }
      }
    }
  }
}


void stopSound(int note, int ch) {
  toern_audio::StateGuard guard;
  for (int num = 0; num < POLY_VOICES; num++) {
    if (noteArray[ch][num] == note) {
      Senvelope1[num]->noteOff();
      Senvelope2[num]->noteOff();
      SenvelopeFilter1[num]->noteOff();
      noteArray[ch][num] = NOTE_EMPTY;
    }
  }
}




void updateSynthVoice(int channel){

       int instrumentValue = SMP.synth_settings[channel][INSTRUMENT];
        int cutoff = SMP.synth_settings[channel][CUTOFF];
        int resonance = SMP.synth_settings[channel][RESONANCE];
        int filter = SMP.synth_settings[channel][FILTER];


        int semi = SMP.synth_settings[channel][SEMI];
        int cent = SMP.synth_settings[channel][CENT];
        int form = SMP.synth_settings[channel][FORM];

        switchSynthVoice(instrumentValue,0, cutoff, resonance, filter, semi, cent, form);
        applySynthWaveformControl(
            instrumentValue, 0,
            (uint8_t)filterSetting(channel, FILTER_WAVEFORM));



        if (channel == 11) {
          setParams(ATTACK, 11);
          setParams(DECAY, 11);
          setParams(SUSTAIN, 11);
          setParams(RELEASE, 11);
        }
}



void switchSynthVoice(int typeValue, int ch, int p1,int p2, int p3, int p4, int p5, int p6) {
    if (ch >= 0 && ch < INSTRUMENT_CHANNELS) {
        pulseWidth[ch][0] = 0.25f;
        pulseWidth[ch][1] = 0.25f;
        pulseWidth[ch][2] = 0.25f;
    }
    switch(typeValue) {
        case 0: bass_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 1: keys_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 2: chiptune_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 3: pad_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 4: wow_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 5: organ_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 6: flute_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 7: lead_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 8: arp_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 9: brass_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 10: acid303_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 11: dx7_epiano_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 12: dx7_bass_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
    }
}
# 1 "/Users/jank/git/toern/toern_touch_input.ino"

bool readDebouncedTouch(int index) {
  static TouchDebouncer pads[5];
  if (index < 0 || index >= 5) return false;
  static const int pins[5] = {SWITCH_1, SWITCH_2, SWITCH_3, SWITCH_4, SWITCH_5};
  bool raw;
  if (exttouch && index == 0) raw = digitalReadFast(BUTTON_A) == LOW;
  else if (exttouch && index == 1) raw = digitalReadFast(BUTTON_B) == LOW;
  else raw = fastTouchRead(pins[index]) > touchThreshold;
  return pads[index].update(raw, (uint32_t)millis());
}
# 1 "/Users/jank/git/toern/toern_touchy.ino"


static const CRGB touchyPadColors[5] = {
  CRGB(255, 64, 64), CRGB(255, 170, 32), CRGB(64, 255, 128),
  CRGB(64, 128, 255), CRGB(220, 64, 255)
};
static uint8_t touchyLastVoice = 1;
static uint8_t touchyVelocity[6] = {100,100,100,100,100,100};
static unsigned int touchySavedVolume[6];
static bool touchyRecordHeld = false;
static bool touchyPrevious[5] = {};

void onTouchyModeEnter() {

  for (int pad=0;pad<5;pad++) touchyPrevious[pad] = readDebouncedTouch(pad);
}

void syncTouchyEncoders() {
  const int ch = touchyLastVoice;
  GLOB.currentChannel = ch;
  GLOB.y = ch + 1;
  const int values[4] = {(int)SMP.filter_settings[ch][OCTAVE], touchyVelocity[ch],
    (int)SMP.filter_settings[ch][REVERB], (int)SMP.filter_settings[ch][LOWCUT]};
  for (int i=0; i<4; i++) {
    touchyMode.pos[i] = values[i];
    Encoder[i].writeCounter((int32_t)values[i]);
  }
}

static bool touchyTintActive = false;
static int8_t touchyEntryHeldEncoder = -1;

bool consumeTouchyEntryHold(const uint8_t states[NUM_ENCODERS]) {
  if (touchyEntryHeldEncoder < 0) return false;
  if (states[touchyEntryHeldEncoder] == 0) { touchyEntryHeldEncoder = -1; return false; }
  return true;
}

FLASHMEM void setTouchyEnabled(bool enabled) {
  if (enabled == touchyEnabled) return;
  if (enabled && isNowPlaying) pause(false);
  if (enabled) { pendingStartOnBar = false; waitForFourBars = false; }
  for (int ch=1; ch<=5; ch++) {
    if (enabled) touchySavedVolume[ch] = SMP.channelVol[ch];
    SMP.channelVol[ch] = enabled ? 16 : touchySavedVolume[ch];
    applyVoiceAmpGainFromVol(ch, SMP.channelVol[ch]);
  }
  touchyEnabled = enabled;
}

static void selectTouchyVoice(uint8_t voice) {
  GLOB.currentChannel = voice;
  GLOB.y = voice + 1;
  if (currentMode == &touchyMode) syncTouchyEncoders();
}

FLASHMEM void checkTouchyTouches() {
  for (int i = 0; i < 5; i++) {
    const bool held = readDebouncedTouch(i);
    if (held && !touchyPrevious[i]) {
      touchyLastVoice = i + 1;
      touchyTintActive = true;
      selectTouchyVoice(touchyLastVoice);
      noteUserActivity();
      SMP.channelVol[touchyLastVoice] = 16;
      applyVoiceAmpGainFromVol(touchyLastVoice, 16);
      triggerCurrentVoiceAudition(basePitchRowForChannel(touchyLastVoice), touchyVelocity[touchyLastVoice]);
    }
    touchyPrevious[i] = held;
    if (i < 4) touchState[i] = lastTouchState[i] = held;
  }
}

FLASHMEM void handleTouchyButtons(const uint8_t states[NUM_ENCODERS]) {
  if (touchyRecordHeld) {
    if (states[0] == 9 || !isPressed[0]) {
      touchyRecordHeld = false;
      pendingStartFastRecord = false;
      if (fastRecordActive) stopFastRecord();
      fastRecWaitRelease = false;
    }
    return;
  }
  for (int i = 0; i < NUM_ENCODERS; i++) {
    if (states[i] == 1 || (states[i] == 2 && oldButtons[i] != 2)) {
      if (states[i] == 2 && (i == 2 || i == 3)) touchyEntryHeldEncoder = i;
      if (states[i] == 2 && i == 3) {
        extern void openTouchyMainMenu();
        setTouchyEnabled(false);
        openTouchyMainMenu();
      } else if (states[i] == 2 && i == 0) {
        selectTouchyVoice(touchyLastVoice);
        touchyRecordHeld = true;
        fastRecWaitRelease = false;
        recordingStartBeat = 0;
        recordingBeatCount = 0;
        recordingBorderArmed = true;
        pendingStartFastRecord = true;
      } else if (states[i] == 2 && i == 2) {
        selectTouchyVoice(touchyLastVoice);
        fxType = 0;
        switchMode(&filterMode);
      } else if (states[i] == 1 || i == 0) {

        GLOB.currentChannel = i + 1;
        GLOB.y = i + 2;
        switchMode(&set_Wav);
      }
      return;
    }
  }
}

FLASHMEM void showTouchyMode() {
  const int ch = touchyLastVoice;
  const FilterType types[4] = {OCTAVE, OCTAVE, REVERB, LOWCUT};
  for (int voice=1; voice<=5; voice++) {
    if (SMP.channelVol[voice] != 16) {
      SMP.channelVol[voice] = 16;
      applyVoiceAmpGainFromVol(voice, 16);
    }
  }
  for (int i=0; i<4; i++) {
    const int value = constrain((int)touchyMode.pos[i], (int)touchyMode.minValues[i], (int)touchyMode.maxValues[i]);
    if (i == 1) { touchyVelocity[ch] = value; continue; }
    if (i == 2 && (ch == 3 || ch == 4)) continue;
    if (SMP.filter_settings[ch][types[i]] != value) {
      SMP.filter_settings[ch][types[i]] = value;
      setFilters(types[i], ch, false);
    }
  }
  static elapsedMillis frame;
  if (frame < 33) return;
  frame = 0;
  FastLEDclear();
  const unsigned long now = millis();
  const int phase = (now / 250) % 4;
  const int x0 = (int)lroundf((int)maxX - 20 - ((int)maxX / 5) * (1.0f - cosf(now * PI / 3000.0f)) * .5f);
  const int top = (int)lroundf(14.0f + .65f * sinf(now * PI / 1800.0f));
  CRGB tint(170,62,105);
  if (touchyTintActive) {
    const CRGB pad = touchyPadColors[touchyLastVoice - 1];
    tint = CRGB(pad.r * 3 / 5, pad.g * 3 / 5, pad.b * 3 / 5);
  }

  static const char *sprite[11] = {
    "     BBBBBBBBBBBB    ",
    "    BCCCCCCCCCCCCB   ",
    "    BCEEFEEFDEEECB D ",
    "OO  BCEFEFEDGDFECBDGD",
    "OLO BCEEEEEDGGDDDDGGD",
    " OLOBCEEEEEDGGGGGGGGD",
    "  OLBCEEFEEDGKJGGKJGD",
    "  OOBCFEEEEDEJJGGJJED",
    "   DBCCCCCCCDGGGGGGD ",
    "    OLOBOLOBOLODOLO  ",
    "    OLL LLO OLL LLO  "
  };
  static const CRGB rainbow[6] = {
    CRGB(215,2,29), CRGB(255,144,36), CRGB(255,255,0),
    CRGB(65,255,76), CRGB(0,191,255), CRGB(212,38,255)
  };
  char animated[11][21];
  for (int y=0; y<11; y++) memcpy(animated[y], sprite[y], 21);

  static const char *tails[3][5] = {
    {"OO  ","OLO "," OLO","  OL","  OO"},
    {"    ","OO  ","OLLO"," OLL","  OO"},
    {"    ","    "," OOO","OLLL","OOOO"}
  };
  static const uint8_t tailPose[4] = {0,1,2,1};
  for (int y=0; y<5; y++) memcpy(animated[y+3], tails[tailPose[phase]][y], 4);
  static const char *toes[4] = {"OLL","OLO","LLO","OLO"};
  for (int leg=0; leg<4; leg++) {
    const int x=4+leg*4;
    memcpy(animated[9]+x, "OLO", 3);
    memcpy(animated[10]+x, toes[(phase+(leg%2)*2)%4], 3);
  }
  for (int x = 1; x <= (int)maxX; x++) {
    const int wave = (int)lroundf(.65f * sinf(x * .6f - now * PI / 600.0f));
    for (int y = 1; y <= 16; y++) {
      CRGB color = CRGB::Black;
      const int row = top - 4 + wave - y;
      if (x < x0 + 5 && row >= 0 && row < 6) color = rainbow[row];
      const int sx = x - x0, sy = top - y;
      if (sx >= 0 && sx < 21 && sy >= 0 && sy < 11) {
        switch (animated[sy][sx]) {
          case 'B': color = CRGB(72,30,5); break;
          case 'C': color = CRGB(170,86,22); break;
          case 'D': case 'J': color = CRGB::Black; break;
          case 'E': color = tint; break;
          case 'F': color = CRGB(255,4,94); break;
          case 'G': color = CRGB(48,48,56); break;
          case 'L': color = CRGB(112,112,124); break;
          case 'O': color = CRGB(28,28,36); break;
          case 'K': color = CRGB(200,195,170); break;
        }
      }
      light(x, y, CRGB(color.r / 2, color.g / 2, color.b / 2));
    }
  }
  FastLEDshow();
}
# 1 "/Users/jank/git/toern/toern_ui.ino"
extern const unsigned int maxlen;
extern void triggerGridNote(unsigned int globalX, unsigned int y, bool allowMuted = false);
extern CRGB col[];
extern unsigned int beatForUI;
extern uint8_t lineOutLevelSetting;
extern bool SMP_FLOW_MODE;

#define LINEOUT_LEVEL_MIN 13
#define LINEOUT_LEVEL_MAX 31

extern uint8_t ledBrightness;
extern bool ledModulesRotated;
#define LED_BRIGHTNESS_DEFAULT 64




void light_single(unsigned int matrixId, unsigned int x, unsigned int y, CRGB color) {
  if (matrixId >= LED_MODULES) return;
  if (x < 1 || x > MATRIX_WIDTH || y < 1 || y > maxY) return;

  unsigned int matrixOffset = matrixId * 256;
  unsigned int mappedX = x;
  unsigned int mappedY = y;



  if (ledModulesRotated) {
    mappedX = (MATRIX_WIDTH + 1) - y;
    mappedY = x;
  }

  unsigned int index;


  if (mappedY % 2 == 0) {

    index = (MATRIX_WIDTH - mappedX) + (MATRIX_WIDTH * (mappedY - 1));
  } else {

    index = (mappedX - 1) + (MATRIX_WIDTH * (mappedY - 1));
  }



  if (ledBrightness < 255) {
    color.nscale8(ledBrightness);
  }


  index += matrixOffset;
  if (index < NUM_LEDS) {
    leds[index] = color;
  }
}


void light(unsigned int x, unsigned int y, CRGB color) {
  if (y < 1 || y > maxY || x < 1 || x > maxX) return;


  unsigned int matrixNum = (x - 1) / MATRIX_WIDTH;
  unsigned int localX = ((x - 1) % MATRIX_WIDTH) + 1;


  light_single(matrixNum, localX, y, color);

}


CRGB getIndicatorColor(char colorCode) {
  switch (colorCode) {
    case 'G': return CRGB(0, 255, 0);
    case 'R': return CRGB(255, 0, 0);
    case 'X': return CRGB(0, 0, 255);
    case 'W': return CRGB(255, 255, 255);
    case 'O': return CRGB(255, 165, 0);
    case 'H': return CRGB(0, 191, 255);
    case 'B': return CRGB(0, 150, 255);
    case 'V': return CRGB(148, 0, 211);
    case 'P': return CRGB(50, 0, 50);
    case 'Y': return CRGB(255, 255, 0);
    case 'M': return CRGB(255, 0, 255);
    case 'C': return CRGB(0, 0, 0);
    case 'U': return CRGB(GLOB.vol * GLOB.vol, 20 - GLOB.vol, 0);
    case 'D': return CRGB(100, 0, 0);
    case 'E': return CRGB(0, 100, 0);
    case 'N': return CRGB(0, 255, 255);
    default: return CRGB(0, 0, 0);
  }
}


CRGB getCurrentChannelColor() {
  if (GLOB.currentChannel >= 0 && GLOB.currentChannel < 16) {
    return col_base[GLOB.currentChannel];
  }
  return CRGB(0, 0, 0);
}


CRGB applyHighlight(CRGB color, bool highlight) {
  if (highlight) {

    return CRGB(
      min(255, color.r * 1.5),
      min(255, color.g * 1.5),
      min(255, color.b * 1.5)
    );
  } else {

    return CRGB(
      color.r * 0.5,
      color.g * 0.5,
      color.b * 0.5
    );
  }
}


CRGB normalizeToMaxBrightness(CRGB color) {

  if (color.r == 0 && color.g == 0 && color.b == 0) {
    return color;
  }


  uint8_t maxChannel = max(max(color.r, color.g), color.b);


  if (maxChannel == 255) {
    return color;
  }



  uint16_t r = ((uint16_t)color.r * 255) / maxChannel;
  uint16_t g = ((uint16_t)color.g * 255) / maxChannel;
  uint16_t b = ((uint16_t)color.b * 255) / maxChannel;

  return CRGB((uint8_t)r, (uint8_t)g, (uint8_t)b);
}

void getIndicatorXPositions(int encoderNum, int &x1, int &x2, int &x3) {
  if (encoderNum < 1 || encoderNum > NUM_ENCODERS) {
    x1 = x2 = x3 = 1;
    return;
  }


  bool isWide = (ledModules == 2 && maxX > MATRIX_WIDTH);
  if (isWide) {


    static const int starts[4] = { 4, 11, 19, 26 };
    int slotStart = starts[encoderNum - 1];

    x1 = constrain(slotStart, 1, (int)maxX);
    x2 = constrain(slotStart + 1, 1, (int)maxX);
    x3 = constrain(slotStart + 2, 1, (int)maxX);
    return;
  }


  switch (encoderNum) {
    case 1: x1 = 1; x2 = 2; x3 = 3; break;
    case 2: x1 = 5; x2 = 6; x3 = 7; break;
    case 3: x1 = 9; x2 = 10; x3 = 11; break;
    case 4: x1 = 13; x2 = 14; x3 = 15; break;
    default: x1 = x2 = x3 = 1; break;
  }
}



void drawIndicator(char size, char colorCode, int encoderNum, bool highlight, bool setEncoderLed, uint8_t brightness) {
  CRGB color = getIndicatorColor(colorCode);


  if (colorCode == 'C') {
    color = getCurrentChannelColor();
  }


  if (highlight) {
    color = applyHighlight(color, true);
  }

  if (brightness != 255) {
    uint8_t peak = max(color.r, max(color.g, color.b));
    if (peak) color = CRGB((uint16_t)color.r * brightness / peak,
                           (uint16_t)color.g * brightness / peak,
                           (uint16_t)color.b * brightness / peak);

    if (colorCode == 'O') color = CRGB(brightness, brightness / 2, 0);
  }

  if (setEncoderLed && encoderNum >= 1 && encoderNum <= NUM_ENCODERS) {

    CRGB maxBrightnessColor = brightness == 255 ? normalizeToMaxBrightness(color) : color;
    uint32_t rgbCode = (uint32_t(maxBrightnessColor.r) << 16) | (uint32_t(maxBrightnessColor.g) << 8) | maxBrightnessColor.b;
    Encoder[encoderNum - 1].writeRGBCode(rgbCode);
  }

  int x1, x2, x3;
  getIndicatorXPositions(encoderNum, x1, x2, x3);
  bool isWide = (ledModules == 2 && maxX > MATRIX_WIDTH);
  int x4 = isWide ? min((int)maxX, x3 + 1) : x3;


  switch (size) {
    case 'S':
      light(x3, 1, color);
      break;
    case 'M':
      if (isWide) {
        light(x2, 1, color);
        light(x3, 1, color);
        light(x4, 1, color);
      } else {
        light(x2, 1, color);
        light(x3, 1, color);
      }
      break;
    case 'L':
      light(x1, 1, color);
      light(x2, 1, color);
      light(x3, 1, color);
      if (isWide) {
        light(x4, 1, color);
      }
      break;
    case 'C':

      light(x1, 1, color);
      light(x2, 1, color);
      light(x3, 1, color);
      if (isWide) {
        light(x4, 1, color);
      }

      int vx = isWide ? x3 : x2;
      light(vx, 1, color);
      light(vx, 2, color);
      light(vx, 3, color);
      break;
  }
}



void drawFilterCheck(int mappedValue, FilterType fx, CRGB color) {

  int activeLength = ::map(mappedValue, 0, maxfilterResolution, 0, maxX);


  for (int x = 1; x <= maxX; x++) {
    if (x <= activeLength) {

      float blend = float(x - 1) / max(1, activeLength - 1);
      CRGB grad = CRGB(
        255 * (1.0 - blend) + color.r * blend,
        255 * (1.0 - blend) + color.g * blend,
        255 * (1.0 - blend) + color.b * blend
      );
      light(x, maxY, grad);
    } else {

      light(x, maxY, CRGB(0, 0, 0));
    }

  }

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d", mappedValue);

  int textPixelWidth = 0;
  for (int i = 0; buffer[i] != '\0'; i++) {
    if (buffer[i] >= 32 && buffer[i] <= 126) {
      textPixelWidth += alphabet[buffer[i] - 32][0] + 1;
    }
  }

  int textStartX = (maxX + 1) - textPixelWidth;
  if (textStartX < 1) textStartX = 1;

  int boxStartX = max(1, textStartX - 1);
  int boxEndX = min((int)maxX, textStartX + textPixelWidth);


  for (int x = boxStartX; x <= boxEndX; x++) {
    for (int y = 7; y <= 13; y++) {
      light(x, y, CRGB::Black);
    }
  }


  drawText(buffer, textStartX, 8, color);
}

void drawPlayButton() {
  unsigned int timer = (beatForUI - 1) % maxX + 1;
  if (currentMode == &draw || currentMode == &singleMode) {
    if (isNowPlaying) {

      if (timer % 4 == 1) {
        Encoder[2].writeRGBCode(0xFF0000);
      } else {
        Encoder[2].writeRGBCode(0x550000);
      }
    }
  }
}

void drawNoSD() {
  playSdWav1.stop();
  if (drawNoSD_hasRun) return;


  bool noSDfound = false;

  while (!SD.begin(INT_SD)) {
    FastLEDclear();
    drawText("SD?", 3, 8, UI_RED);
    FastLEDshow();
    delay(1000);
    noSDfound = true;
    extern void noteUserActivity();
    noteUserActivity();
  }

  if (noSDfound && SD.begin(INT_SD)) {
    FastLEDclear();
    extern void sampleBrowserInvalidate();
    sampleBrowserInvalidate();
  }

  drawNoSD_hasRun = true;
}

void drawBase() {


  static bool muteCache[maxY];
  static bool muteCacheValid = false;
  static unsigned int lastFrame = 0;


  unsigned int currentFrame = (millis() / 33);
  if (currentFrame != lastFrame || !muteCacheValid) {
    extern bool getMuteStateForUI(int channel);
    for (unsigned int ch = 0; ch < maxY; ch++) {
      muteCache[ch] = getMuteStateForUI(ch);
    }
    muteCacheValid = true;
    lastFrame = currentFrame;
  }

  if (!GLOB.singleMode) {

    static CRGB cachedColors[maxY];
    static bool colorsCached = false;
    static bool lastDrawBaseColorMode = false;
    extern uint8_t currentColorScheme;
    static uint8_t lastColorScheme = 255;
    extern volatile bool colorsUpdatedViaSerial;


    extern bool imageMode;
    static bool lastImageMode = false;
    if (!colorsCached || drawBaseColorMode != lastDrawBaseColorMode || imageMode != lastImageMode
        || currentColorScheme != lastColorScheme || colorsUpdatedViaSerial) {
      for (unsigned int y = 0; y < maxY; y++) {
        cachedColors[y] = y < 15 ? col_base[y] : CRGB(0, 0, 0);
      }
      colorsCached = true;
      lastDrawBaseColorMode = drawBaseColorMode;
      lastImageMode = imageMode;
      lastColorScheme = currentColorScheme;
      colorsUpdatedViaSerial = false;
    }

    unsigned int colors = 0;
    for (unsigned int y = 1; y < maxY; y++) {
      const bool rowMuted = muteCache[y - 1];

      const bool useColor = drawBaseColorMode && !imageMode;
      const CRGB rowColor = cachedColors[colors];
      for (unsigned int x = 1; x < maxX + 1; x++) {
        if (rowMuted) {
          light(x, y, CRGB(0, 0, 0));
        } else {
          if (useColor) {
            light(x, y, rowColor);
          } else {
            light(x, y, CRGB(0, 0, 0));
          }
        }
      }
      colors++;
    }




  } else {

    unsigned int currentChannel = GLOB.currentChannel;
    bool isMuted = muteCache[currentChannel];


    static CRGB cachedSingleModeColor = CRGB(0, 0, 0);
    static unsigned int lastSingleModeChannel = 255;
    static CRGB cachedHighlightColor = CRGB(0, 0, 0);
    extern uint8_t currentColorScheme;
    static uint8_t lastSingleModeColorScheme = 255;
    extern volatile bool colorsUpdatedViaSerial;


    if (currentChannel != lastSingleModeChannel || currentColorScheme != lastSingleModeColorScheme || colorsUpdatedViaSerial) {
      cachedSingleModeColor = col_base[currentChannel];
      cachedHighlightColor = blend(cachedSingleModeColor, CRGB::White, 5);
      lastSingleModeChannel = currentChannel;
      lastSingleModeColorScheme = currentColorScheme;
      if (colorsUpdatedViaSerial) {
        colorsUpdatedViaSerial = false;
      }
    }

    for (unsigned int y = 1; y < maxY; y++) {

      extern bool imageMode;
      if (imageMode) {
        for (unsigned int x = 1; x < maxX + 1; x++) {
          light(x, y, CRGB(0, 0, 0));
        }
        continue;
      }


      CRGB color = (y == currentChannel + 1) ? cachedHighlightColor : cachedSingleModeColor;


      if (isMuted) {
        CHSV hsv = rgb2hsv_approximate(color);
        hsv.v = (hsv.v * 128) / 255;
        hsv2rgb_rainbow(hsv, color);
      }

      for (unsigned int x = 1; x < maxX + 1; x++) {


        if (y == currentChannel + 1 ) {
         color = blend(color, CRGB::White, 5);

         color.nscale8(190);
        }

        light(x, y, color);
      }
    }






  }




   extern int inputMonitoringState;
   extern bool imageMode;
   extern int imgBrushChannel;
   CRGB helperColor;
   if (imageMode) {
     int brush = imgBrushChannel;
     if (brush < 1) brush = 1;
     if (brush > 8) brush = 8;
     helperColor = col[brush];
     helperColor.nscale8(48);
   } else if (inputMonitoringState == 0) {
     helperColor = CRGB(10, 10, 10);
   } else if (inputMonitoringState == 1) {
     helperColor = CRGB(30, 30, 0);
   } else {
     helperColor = CRGB(0, 30, 0);
   }
   for (unsigned int x = 1; x <= maxX - 3; x += 4) {
    light(x, 1, helperColor);
  }

  drawStatus();

  drawPages();


  if (GLOB.singleMode && GLOB.currentChannel < maxY && muteCache[GLOB.currentChannel]) {
    extern int ledModules;
    const bool blinkOn = ((millis() / 300) & 0x1) == 0;
    const CRGB bar = blinkOn ? CRGB(255, 255, 0) : CRGB(0, 0, 0);
    const unsigned int x0 = (ledModules == 2) ? 16u : 8u;
    for (unsigned int x = x0; x <= maxX; x++) {
      light(x, 16, bar);
    }
  }
}

static bool pongBallInitialized = false;
static int pongBallX = 1;
static int pongBallY = 1;
static int pongVelocityX = 1;
static int pongVelocityY = 1;
static unsigned long pongLastUpdate = 0;
DMAMEM uint16_t pongUpdateInterval __attribute__((aligned(4))) = 70;


static const int8_t pongTailLength = 4;
DMAMEM int8_t pongTailX[pongTailLength] __attribute__((aligned(4)));
DMAMEM int8_t pongTailY[pongTailLength] __attribute__((aligned(4)));
DMAMEM int8_t pongTailIndex = 0;
DMAMEM uint8_t pongCollisionCount = 0;

FLASHMEM static void adjustPongAngle(int &vx, int &vy) {
  int signX = (vx >= 0) ? 1 : -1;
  int signY = (vy >= 0) ? 1 : -1;
  int absVx = abs(vx);
  int absVy = abs(vy);

  int currentIdx;
  if (absVx == 1 && absVy == 1) {
    currentIdx = 0;
  } else if (absVx == 2 && absVy == 1) {
    currentIdx = 1;
  } else {
    currentIdx = 2;
  }

  int newIdx = currentIdx;
  for (int attempts = 0; attempts < 4 && newIdx == currentIdx; attempts++) {
    newIdx = random(0, 3);
  }

  int newAbsVx = 1;
  int newAbsVy = 1;
  switch (newIdx) {
    case 0: newAbsVx = 1; newAbsVy = 1; break;
    case 1: newAbsVx = 2; newAbsVy = 1; break;
    case 2: newAbsVx = 1; newAbsVy = 2; break;
  }

  vx = signX * newAbsVx;
  vy = signY * newAbsVy;
}

 FLASHMEM static void registerPongCollision(int &vx, int &vy) {
  pongCollisionCount++;
  if (pongCollisionCount >= 10) {
    pongCollisionCount = 0;
    adjustPongAngle(vx, vy);
  }
}

FLASHMEM void resetPongGame() {
  pongBallInitialized = false;

  for (int i = 0; i < pongTailLength; i++) {
    pongTailX[i] = -1;
    pongTailY[i] = -1;
  }
  pongTailIndex = 0;
  pongCollisionCount = 0;
}

FLASHMEM static void initPongBall() {

  pongBallX = random(1, maxX + 1);
  pongBallY = random(1, maxY + 1);


  int angle = random(25, 66);
  bool flipX = (random(0, 2) == 0);
  bool flipY = (random(0, 2) == 0);



  if (angle < 40) {
    pongVelocityX = 2;
    pongVelocityY = 1;
  } else if (angle > 50) {
    pongVelocityX = 1;
    pongVelocityY = 2;
  } else {
    pongVelocityX = 1;
    pongVelocityY = 1;
  }

  if (flipX) pongVelocityX = -pongVelocityX;
  if (flipY) pongVelocityY = -pongVelocityY;

  pongLastUpdate = millis();
  pongBallInitialized = true;
}

static bool pongNoteAt(int px, int py) {
  if (px < 1 || px > (int)maxX || py < 1 || py > maxY) {
    return false;
  }

  unsigned int pageIndex = (GLOB.edit > 0) ? (GLOB.edit - 1) : 0;
  unsigned int globalX = (pageIndex * maxX) + px;
  if (globalX > maxlen) {
    return false;
  }

  int channel = note[globalX][py].channel;
  if (channel == 0) return false;
  if (isChildVoiceDisabled(channel)) return false;


  if (!getMuteState(channel)) return false;

  return true;
}

static void triggerPongCollision(int px, int py) {
  if (px < 1 || px > (int)maxX || py < 1 || py > maxY) {
    return;
  }

  unsigned int pageIndex = (GLOB.edit > 0) ? (GLOB.edit - 1) : 0;
  unsigned int globalX = (pageIndex * maxX) + px;
  if (globalX > maxlen) {
    return;
  }


  triggerGridNote(globalX, py, true);
}

FLASHMEM void updatePongBall() {
  if (!pongBallInitialized) {
    initPongBall();
  }

  unsigned long now = millis();
  if (now - pongLastUpdate < pongUpdateInterval) {
    return;
  }
  pongLastUpdate = now;

  int vx = pongVelocityX;
  int vy = pongVelocityY;

  int nextX = pongBallX + vx;
  int nextY = pongBallY + vy;

  if (nextX < 1 || nextX > (int)maxX) {
    vx = -vx;
    nextX = pongBallX + vx;
  }

  if (nextY < 1 || nextY > maxY) {
    vy = -vy;
    nextY = pongBallY + vy;
  }

  bool axisCollision = false;


  if (vx != 0) {
    int stepX = (vx > 0) ? 1 : -1;
    for (int step = 1; step <= abs(vx); step++) {
      int checkX = pongBallX + (step * stepX);
      if (checkX >= 1 && checkX <= (int)maxX && pongNoteAt(checkX, pongBallY)) {
        triggerPongCollision(checkX, pongBallY);
        vx = -vx;
        axisCollision = true;
        registerPongCollision(vx, vy);
        break;
      }
    }
  }


  if (vy != 0 && !axisCollision) {
    int stepY = (vy > 0) ? 1 : -1;
    for (int step = 1; step <= abs(vy); step++) {
      int checkY = pongBallY + (step * stepY);
      if (checkY >= 1 && checkY <= maxY && pongNoteAt(pongBallX, checkY)) {
        triggerPongCollision(pongBallX, checkY);
        vy = -vy;
        axisCollision = true;
        registerPongCollision(vx, vy);
        break;
      }
    }
  }


  if (!axisCollision) {
    int collisionX = pongBallX + vx;
    int collisionY = pongBallY + vy;
    if (collisionX >= 1 && collisionX <= (int)maxX && collisionY >= 1 && collisionY <= maxY && pongNoteAt(collisionX, collisionY)) {
      triggerPongCollision(collisionX, collisionY);
      vx = -vx;
      vy = -vy;
      registerPongCollision(vx, vy);
    }
  }


  pongTailX[pongTailIndex] = pongBallX;
  pongTailY[pongTailIndex] = pongBallY;
  pongTailIndex = (pongTailIndex + 1) % pongTailLength;

  pongBallX = constrain(pongBallX + vx, 1, (int)maxX);
  pongBallY = constrain(pongBallY + vy, 1, maxY);

  pongVelocityX = vx;
  pongVelocityY = vy;
}

FLASHMEM void drawPongBall() {

  for (int i = 0; i < pongTailLength; i++) {
    int tailIdx = (pongTailIndex + i) % pongTailLength;
    int tx = pongTailX[tailIdx];
    int ty = pongTailY[tailIdx];


    if (tx < 1 || ty < 1) continue;


    int brightness = (i + 1) * 255 / (pongTailLength + 1);


    uint8_t hue = (i * 255 / pongTailLength);
    CRGB color = CHSV(hue, 255, brightness);

    light(tx, ty, color);
  }


  light(pongBallX, pongBallY, CRGB(255, 255, 255));
}

FLASHMEM void showSaveSuccessAnimation(bool returnToDraw) {
  extern void FastLEDclear();
  extern void FastLEDshow();
  extern void sdIoYield();
  extern Mode draw;
  extern void switchMode(Mode *newMode);

  const int cx = (int)((maxX + 1) / 2);
  const int cy = (int)((maxY + 1) / 2);

  static const int8_t pts[][2] = {
    { -3, -1 }, { -2, -2 }, { -1, -3 }, { 0, -2 },
    { 1, -1 }, { 2, 0 }, { 3, 1 }, { 4, 2 }
  };
  const int npts = 8;
  const uint32_t t0 = millis();
  const uint32_t dur = 1000;

  while ((millis() - t0) < dur) {
    const uint32_t t = millis() - t0;
    FastLEDclear();

    int reveal = (int)((t * npts) / 420);
    if (reveal > npts) reveal = npts;

    uint8_t bri = 220;
    if (t < 120) {
      bri = (uint8_t)map((long)t, 0, 120, 40, 220);
    } else if (t > 780) {
      bri = (uint8_t)map((long)t, 780, (long)dur, 220, 20);
    }
    const CRGB col = CRGB(0, bri, 0);

    for (int i = 0; i < reveal; i++) {
      const int x = cx + pts[i][0];
      const int y = cy + pts[i][1];
      if (x >= 1 && x <= (int)maxX && y >= 1 && y <= (int)maxY) {
        light((unsigned int)x, (unsigned int)y, col);
      }
    }
    FastLEDshow();
    sdIoYield();
    delay(16);
  }

  if (returnToDraw) {
    switchMode(&draw);
  }
}

FLASHMEM void showSaveFailureAnimation(bool returnToDraw) {
  const int cx = (int)((maxX + 1) / 2);
  const int cy = (int)((maxY + 1) / 2);
  const uint32_t started = millis();
  const uint32_t duration = 1000;
  while (millis() - started < duration) {
    const uint32_t elapsed = millis() - started;
    const int radius = min(3, (int)(elapsed / 70));
    uint8_t brightness = elapsed > 780
      ? (uint8_t)map((long)elapsed, 780, (long)duration, 220, 20) : 220;
    FastLEDclear();
    for (int offset = -radius; offset <= radius; offset++) {
      light(cx + offset, cy + offset, CRGB(brightness, 0, 0));
      light(cx + offset, cy - offset, CRGB(brightness, 0, 0));
    }
    FastLEDshow();
    sdIoYield();
    delay(16);
  }
  if (returnToDraw) switchMode(&draw);
}

FLASHMEM void drawRecordingBorder() {

  CRGB redColor = CRGB(255, 0, 0);


  for (unsigned int x = 1; x <= maxX; x++) {
    light(x, 1, redColor);
    light(x, maxY, redColor);
  }


  for (unsigned int y = 2; y < maxY; y++) {
    light(1, y, redColor);
    light(maxX, y, redColor);
  }
}

FLASHMEM void drawRecordingBorderDotted() {
  CRGB redColor = CRGB(255, 0, 0);
  for (unsigned int x = 1; x <= maxX; x++) {
    if ((x & 1u) != 0) {
      light(x, 1, redColor);
      light(x, maxY, redColor);
    }
  }
  for (unsigned int y = 2; y < maxY; y++) {
    if ((y & 1u) != 0) {
      light(1, y, redColor);
      light(maxX, y, redColor);
    }
  }
}

FLASHMEM void drawFullMuteBorder() {

  CRGB borderColor = CRGB(0, 0, 80);

  for (unsigned int x = 1; x <= maxX; x++) {
    light(x, 1, borderColor);
    light(x, maxY, borderColor);
  }

  for (unsigned int y = 2; y < maxY; y++) {
    light(1, y, borderColor);
    light(maxX, y, borderColor);
  }
}


FLASHMEM static void drawFullMuteVerticalLabel(const char *text, int encoderNum, char colorCode) {
  if (!text || text[0] == '\0') return;
  int x1, x2, x3;
  getIndicatorXPositions(encoderNum, x1, x2, x3);
  CRGB color = getIndicatorColor(colorCode);



  char c0 = text[0];
  char c1 = text[1];
  bool two = (c1 != '\0');
  if (two) {
    drawChar(c0, x1, 9, color);
    drawChar(c1, x1, 3, color);
  } else {
    drawChar(c0, x1, 3, color);
  }
}

FLASHMEM void drawFullMuteIndicators() {

  drawIndicator('L', 'W', 1, false, false);
  drawIndicator('L', 'Y', 2, false, false);
  drawIndicator('L', 'G', 3, false, false);
  drawIndicator('L', 'N', 4, false, false);


  drawFullMuteVerticalLabel("-", 1, 'W');
  drawFullMuteVerticalLabel("21", 2, 'Y');
  drawFullMuteVerticalLabel("U", 3, 'G');
  drawFullMuteVerticalLabel("F", 4, 'N');

  static uint32_t lastFullMuteEncoderRGB[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
  auto setEncoderRGBIfChanged = [&lastFullMuteEncoderRGB](uint8_t i, uint32_t rgb) {
    if (i < 4 && lastFullMuteEncoderRGB[i] != rgb) {
      lastFullMuteEncoderRGB[i] = rgb;
      Encoder[i].writeRGBCode(rgb);
    }
  };

  CRGB w = getIndicatorColor('W');
  CRGB y = getIndicatorColor('Y');
  CRGB g = getIndicatorColor('G');
  CRGB n = getIndicatorColor('N');
  setEncoderRGBIfChanged(0, (uint32_t)w.r << 16 | (uint32_t)w.g << 8 | w.b);
  setEncoderRGBIfChanged(1, (uint32_t)y.r << 16 | (uint32_t)y.g << 8 | y.b);
  setEncoderRGBIfChanged(2, (uint32_t)g.r << 16 | (uint32_t)g.g << 8 | g.b);
  setEncoderRGBIfChanged(3, (uint32_t)n.r << 16 | (uint32_t)n.g << 8 | n.b);
}

FLASHMEM void drawBatteryWarning() {

  extern void FastLEDclear();
  FastLEDclear();


  CRGB redColor = CRGB(255, 0, 0);
  CRGB orangeColor = CRGB(255, 165, 0);


  for (unsigned int x = 1; x <= maxX; x++) {
    light(x, 1, redColor);
    light(x, maxY, redColor);
  }


  for (unsigned int y = 2; y < maxY; y++) {
    light(1, y, redColor);
    light(maxX, y, redColor);
  }


  drawText("LOW", 3, 10, redColor);


  drawText("BAT", 3, 3, orangeColor);


}

void drawStatus() {

  static uint32_t lastEncoderRGB[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
  static Mode* lastDrawStatusMode = nullptr;
  if (lastDrawStatusMode != currentMode) {
    lastDrawStatusMode = currentMode;
    for (int i = 0; i < 4; i++) lastEncoderRGB[i] = 0xFFFFFFFF;
  }
  auto setEncoderRGBIfChanged = [&lastEncoderRGB](uint8_t i, uint32_t rgb) {
    if (i < 4 && lastEncoderRGB[i] != rgb) {
      lastEncoderRGB[i] = rgb;
      Encoder[i].writeRGBCode(rgb);
    }
  };


  extern bool fullMuteIsActive();
  if (fullMuteIsActive() && (currentMode == &draw || currentMode == &singleMode)) {
    for (int i = 0; i < 4; i++) lastEncoderRGB[i] = 0xFFFFFFFF;
    return;
  }


  if (GLOB.activeCopy) {



    for (unsigned int s = 1; s <= maxX; s++) {
      light(s, 16, CRGB(0, 0, 0));
    }


    drawIndicator('M', 'X', 1, false, false);


    drawIndicator('M', 'G', 4, false, false);


    CRGB blueColor = getIndicatorColor('X');
    CRGB greenColor = getIndicatorColor('G');

    setEncoderRGBIfChanged(0, blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
    setEncoderRGBIfChanged(1, 0x000000);
    setEncoderRGBIfChanged(2, 0x000000);
    setEncoderRGBIfChanged(3, greenColor.r << 16 | greenColor.g << 8 | greenColor.b);

    return;
  }


  if (currentMode == &draw && GLOB.y == 16) {

    drawIndicator('M', 'R', 1, false, false);


    drawIndicator('M', 'Y', 4, false, false);


    CRGB redColor = getIndicatorColor('R');
    CRGB yellowColor = getIndicatorColor('Y');

    setEncoderRGBIfChanged(0, redColor.r << 16 | redColor.g << 8 | redColor.b);
    setEncoderRGBIfChanged(1, 0x000000);
    setEncoderRGBIfChanged(2, 0x000000);
    setEncoderRGBIfChanged(3, yellowColor.r << 16 | yellowColor.g << 8 | yellowColor.b);
  }


  if (GLOB.singleMode && GLOB.y == 16) {

    drawIndicator('M', 'R', 1, false, false);
    drawIndicator('M', 'W', 2, false, false);

    drawIndicator('M', 'Y', 4, false, false);


    CRGB redColor = getIndicatorColor('R');
    CRGB whiteColor = getIndicatorColor('W');
    CRGB yellowColor = getIndicatorColor('Y');

    setEncoderRGBIfChanged(0, redColor.r << 16 | redColor.g << 8 | redColor.b);
    setEncoderRGBIfChanged(1, whiteColor.r << 16 | whiteColor.g << 8 | whiteColor.b);
    setEncoderRGBIfChanged(2, 0x000000);
    setEncoderRGBIfChanged(3, yellowColor.r << 16 | yellowColor.g << 8 | yellowColor.b);
  } else if (GLOB.singleMode && GLOB.y != 16) {

    setEncoderRGBIfChanged(1, 0x000000);
  }

  if (currentMode == &noteShift) {

    static int lastMarqueeX = -1;
    static Mode* lastMarqueeMode = nullptr;
    if (lastMarqueeMode != currentMode) {
      lastMarqueeMode = currentMode;
      lastMarqueeX = -1;
    }
    int currX = (int)round(marqueePos);
    currX = (currX < 1) ? 1 : (currX > (int)maxX) ? (int)maxX : currX;
    if (lastMarqueeX < 0) {
      for (unsigned int x = 1; x <= maxX; x++) light(x, 1, CRGB(0, 0, 0));
    } else if (lastMarqueeX != currX) {
      light((unsigned int)lastMarqueeX, 1, CRGB(0, 0, 0));
    }
    light((unsigned int)currX, 1, CRGB(120, 120, 120));
    lastMarqueeX = currX;

    drawIndicator('L', 'G', 1, true, false);
    drawIndicator('L', 'X', 4, true, false);


    CRGB greenColor = applyHighlight(getIndicatorColor('G'), true);
    CRGB blueColor = applyHighlight(getIndicatorColor('X'), true);

    setEncoderRGBIfChanged(0, greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
    setEncoderRGBIfChanged(1, 0x000000);
    setEncoderRGBIfChanged(2, 0x000000);
    setEncoderRGBIfChanged(3, blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
  }


  extern bool songModeActive;
  extern Mode songMode;
  extern int ledModules;
  if (currentMode == &noteShift || songModeActive || currentMode == &songMode) {

    float speed = (currentMode == &noteShift) ? 1.0f : 0.2f;
    if (ledModules == 1) speed *= 0.5f;

    if (movingForward) {
      marqueePos = marqueePos + speed;
      if (marqueePos > maxX) {
        marqueePos = maxX;
        movingForward = false;
      }
    } else {
      marqueePos = marqueePos - speed;
      if (marqueePos < 1) {
        marqueePos = 1;
        movingForward = true;
      }
    }
  }
}

FLASHMEM void drawText(const char *text, int startX, int startY, CRGB color) {
  int xOffset = startX;

  for (int i = 0; text[i] != '\0'; i++) {
    drawChar(text[i], xOffset, startY, color);
    xOffset += alphabet[text[i] - 32][0] + 1;
  }
}

FLASHMEM void drawChar(char c, int x, int y, CRGB color) {
  if (c < 32 || c > 126) return;

  uint8_t index = c - 32;
  uint8_t width = alphabet[index][0];

  for (int col = 0; col < width; col++) {
    uint8_t columnData = alphabet[index][col + 1];


    uint8_t flippedColumn = 0;
    for (int row = 0; row < 5; row++) {
      if (columnData & (1 << row)) {
        flippedColumn |= (1 << (4 - row));
      }
    }

    for (int row = 0; row < 5; row++) {
      if (flippedColumn & (1 << row)) {
        light(x + col, y + row, color);
      }
    }
  }
}

FLASHMEM void drawNumber(float count, CRGB color, int topY) {
  char buffer[16];


  if (fabs(count - (int)count) < 0.01) {

    sprintf(buffer, "%d", (int)count);
  } else {

    dtostrf(count, 0, 2, buffer);
  }


  int textPixelWidth = 0;
  for (int i = 0; buffer[i] != '\0'; i++) {
    if (buffer[i] >= 32 && buffer[i] <= 126) {
      textPixelWidth += alphabet[buffer[i] - 32][0] + 1;
    }
  }

  int startX = (maxX + 1) - textPixelWidth;
  if (startX < 0) startX = 0;

  drawText(buffer, startX, topY, color);
  FastLEDshow();
}

FLASHMEM void drawVelocity() {
  unsigned int vy = currentMode->pos[0];
  unsigned int probStep = currentMode->pos[1];
  unsigned int condStep = currentMode->pos[2];
  unsigned int cv = currentMode->pos[3];


  auto sx = [](int x16) -> int {
    if ((int)maxX <= 16) return x16;
    return 1 + (x16 - 1) * ((int)maxX - 1) / 15;
  };

  FastLEDclear();


  int velX1 = sx(2), velX2 = sx(3);
  for (int x = velX1; x <= velX2; x++) {
    for (unsigned int y = 1; y < vy + 1; y++) {
      light(x, y, CRGB(y * y, 20 - y, 0));
    }
  }


  int pL = sx(5), pR = sx(8), pInnerL = sx(6), pInnerR = sx(7);
  CRGB darkWhite = CRGB(30, 30, 30);
  light(pL, 1, darkWhite);
  light(pInnerL, 1, darkWhite);
  light(pInnerR, 1, darkWhite);
  light(pR, 1, darkWhite);
  light(pL, 16, darkWhite);
  light(pInnerL, 16, darkWhite);
  light(pInnerR, 16, darkWhite);
  light(pR, 16, darkWhite);

  for (unsigned int y = 2; y <= 15; y++) {
    light(pL, y, darkWhite);
    light(pR, y, darkWhite);
  }

  for (int x = pInnerL; x <= pInnerR; x++) {
    if (probStep == 1) {
      light(x, 2, CRGB(100, 0, 0));
      light(x, 3, CRGB(100, 0, 0));
    }
    if (probStep == 2) {
      light(x, 5, CRGB(150, 50, 0));
      light(x, 6, CRGB(150, 50, 0));
    }
    if (probStep == 3) {
      light(x, 8, CRGB(150, 150, 0));
      light(x, 9, CRGB(150, 150, 0));
    }
    if (probStep == 4) {
      light(x, 11, CRGB(0, 150, 150));
      light(x, 12, CRGB(0, 150, 150));
    }
    if (probStep == 5) {
      light(x, 14, CRGB(0, 200, 0));
      light(x, 15, CRGB(0, 200, 0));
    }
  }


  const char* numeratorText;
  const char* denominatorText;
  CRGB textColor;
  if (condStep == 1) {
    numeratorText = "1"; denominatorText = "1"; textColor = CRGB(0, 200, 0);
  } else if (condStep == 2) {
    numeratorText = "1"; denominatorText = "2"; textColor = CRGB(0, 0, 255);
  } else if (condStep == 3) {
    numeratorText = "1"; denominatorText = "4"; textColor = CRGB(200, 0, 255);
  } else if (condStep == 4) {
    numeratorText = "1"; denominatorText = "8"; textColor = CRGB(255, 100, 0);
  } else if (condStep == 5) {
    numeratorText = "1"; denominatorText = "X"; textColor = CRGB(0, 200, 200);
  } else if (condStep == 6) {
    numeratorText = "2"; denominatorText = "1"; textColor = CRGB(0, 150, 255);
  } else if (condStep == 7) {
    numeratorText = "4"; denominatorText = "1"; textColor = CRGB(150, 0, 255);
  } else if (condStep == 8) {
    numeratorText = "8"; denominatorText = "1"; textColor = CRGB(255, 150, 0);
  } else if (condStep == 9) {
    numeratorText = "X"; denominatorText = "1"; textColor = CRGB(0, 255, 200);
  } else if (condStep == 10) {
    numeratorText = "F"; denominatorText = "F"; textColor = CRGB(255, 255, 0);
  } else if (condStep == 11) {
    numeratorText = "G"; denominatorText = "L"; textColor = CRGB(0, 255, 120);
  } else {
    numeratorText = "1"; denominatorText = "1"; textColor = CRGB(0, 200, 0);
  }


  int ix1, ix2, ix3;
  getIndicatorXPositions(3, ix1, ix2, ix3);
  drawText(numeratorText, ix1, 11, textColor);
  light(ix3 + 1, 10, darkWhite);
  light(ix3, 9, darkWhite);
  light(ix2, 8, darkWhite);
  light(ix1, 7, darkWhite);
  drawText(denominatorText, ix1 + 1, 2, textColor);


  int cvX1, cvX2, cvX3;
  getIndicatorXPositions(4, cvX1, cvX2, cvX3);
  for (int x = cvX2; x <= cvX3; x++) {
    for (unsigned int y = 1; y < cv + 1; y++) {
      light(x, y, CRGB(0, 20 - y, y * y));
    }
  }
}

FLASHMEM void drawCtrlVolumeOverlay(int volume) {
  int x1, x2, x3;
  getIndicatorXPositions(2, x1, x2, x3);
  const int startX = x2;
  const int endX = x3;
  volume = constrain(volume, 0, 16);

  if (volume == 0) {

    for (int x = startX; x <= endX; ++x) {
      light(x, 1, CRGB(60, 0, 0));
    }
    return;
  }

  for (int x = startX; x <= endX; ++x) {
    for (int y = 1; y <= volume; ++y) {
      uint8_t hue = mapf(y, 1, 16, 0, 96);
      uint8_t value = mapf(y, 1, 16, 180, 255);
      CRGB color = CHSV(hue, 255, value);
      light(x, y, color);
    }
  }
}

FLASHMEM void drawInputGainOverlay(int gain, int maxGain) {
  int x1, x2, x3;
  getIndicatorXPositions(2, x1, x2, x3);
  const int startX = x2;
  const int endX = x3;
  gain = constrain(gain, 0, maxGain);
  maxGain = max(maxGain, 1);

  if (gain == 0) {

    for (int x = startX; x <= endX; ++x) {
      light(x, 1, CRGB(0, 0, 60));
    }
    return;
  }


  int displayHeight = mapf(gain, 0, maxGain, 0, 16);
  displayHeight = constrain(displayHeight, 1, 16);

  for (int x = startX; x <= endX; ++x) {
    for (int y = 1; y <= displayHeight; ++y) {

      uint8_t hue = mapf(y, 1, 16, 160, 240);
      uint8_t value = mapf(y, 1, 16, 180, 255);
      CRGB color = CHSV(hue, 255, value);
      light(x, y, color);
    }
  }
}

FLASHMEM void drawChannelNrOverlay(int channelNum, int channelIdx) {


  if (channelIdx < 0 || channelIdx >= NUM_CHANNELS) return;

  CRGB channelColor = col[channelIdx];
  const int overlayY = 13;
  const int numberX = 2;
  const int borderY = 10;
  const int overlayEndX = 5;


  for (int x = 1; x <= overlayEndX && x <= (int)maxX; x++) {
    for (int y = 11; y <= 16 && y <= (int)maxY; y++) {
      light(x, y, CRGB(0, 0, 0));
    }
  }


  for (int x = 1; x <= overlayEndX && x <= (int)maxX; x++) {
    if (x >= 1 && x <= (int)maxX && borderY >= 1 && borderY <= (int)maxY) {
      light(x, borderY, channelColor);
    }
  }



  char numStr[4];
  sprintf(numStr, "%d", channelNum);



  int textY = overlayY - 1;

  drawText(numStr, numberX, textY, channelColor);
}

FLASHMEM void drawPageNrOverlay(int pageNum) {

  pageNum = constrain(pageNum, 1, 999);
  const CRGB pageColor = CRGB(220, 220, 220);
  const int textY = 11;

  char numStr[8];
  snprintf(numStr, sizeof(numStr), "%d", pageNum);

  int textPixelWidth = 0;
  for (int i = 0; numStr[i] != '\0'; i++) {
    if (numStr[i] >= 32 && numStr[i] <= 126) {
      textPixelWidth += alphabet[numStr[i] - 32][0] + 1;
    }
  }

  int textStartX = (int)maxX + 1 - textPixelWidth;
  if (textStartX < 1) textStartX = 1;
  int boxStartX = max(1, textStartX - 1);
  int boxEndX = min((int)maxX, textStartX + textPixelWidth);

  for (int x = boxStartX; x <= boxEndX; x++) {
    for (int y = 11; y <= 15 && y <= (int)maxY; y++) {
      light(x, y, CRGB(0, 0, 0));
    }
  }
  drawText(numStr, textStartX, textY, pageColor);
}



FLASHMEM void drawSampleLoadOverlay(uint8_t progressPercent) {
  if (progressPercent > 100) {
    progressPercent = 100;
  }

  FastLED.clear();

  const CRGB frameColor = CRGB(20, 20, 40);
  const CRGB textColor = CRGB(180, 220, 255);
  const CRGB trackColor = CRGB(10, 10, 24);


  for (int x = 1; x <= (int)maxX; ++x) {
    light(x, 3, frameColor);
  }

  light(1, 4, frameColor);
  light(maxX, 4, frameColor);

  int safeChannel = constrain(GLOB.currentChannel, 1, NUM_CHANNELS - 1);
  CRGB barColor = col[safeChannel];

  const int x0 = 2;
  const int x1 = (int)maxX - 1;
  const int barCols = (x1 >= x0) ? (x1 - x0 + 1) : 0;
  int filledCols = 0;
  if (barCols > 0) {
    filledCols = ((int)progressPercent * barCols + 99) / 100;
    if (progressPercent >= 100) {
      filledCols = barCols;
    }
  }
  for (int x = x0; x <= x1; ++x) {
    int idx = x - x0;
    CRGB c = (idx < filledCols) ? barColor : trackColor;
    light(x, 4, c);
    light(x, 5, c);
  }

  drawText("LOAD", 1, 10, textColor);
}

void drawPages() {

  CRGB ledColor;
  extern int loopLength;
  const unsigned int numModules = max(1u, maxX / MATRIX_WIDTH);
  const unsigned int effectiveMaxPages = maxPages / numModules;
  extern bool voiceMode;
  extern bool songModeActive;
  const bool voiceDots = voiceMode && !songModeActive;
  const unsigned int effectiveLoopLength = (!voiceDots && loopLength > 0) ? min((unsigned int)loopLength, effectiveMaxPages) : 0;


  for (unsigned int x = 1; x <= maxX; x++) {
    light(x, maxY, CRGB(0, 0, 0));
  }



  if (effectiveLoopLength > 0) {
    for (unsigned int x = 1; x <= maxX; x++) {
      light(x, maxY, CRGB(10, 0, 0));
    }
  }



  for (unsigned int p = 1; p <= maxPages && p <= MATRIX_WIDTH; p++) {
    if (p > effectiveMaxPages) {
      continue;
    }

    extern uint16_t voicePageMask[16];
    extern unsigned int voicePageFor(int channel, unsigned int globalPage);
    extern unsigned int beatForUI;
    int voiceCh = (int)GLOB.currentChannel;
    if (voiceCh < 0) voiceCh = 0;
    if (voiceCh > 15) voiceCh = 15;
    unsigned int voicePlayPage = 0;
    if (voiceDots && isNowPlaying && maxX > 0 && beatForUI > 0) {
      voicePlayPage = voicePageFor(voiceCh, ((beatForUI - 1) / maxX) + 1);
    }
    bool voiceHas = voiceDots && voiceCh > 0 && (voicePageMask[voiceCh] & (uint16_t)(1u << (p - 1)));


    bool outsideLoop = (effectiveLoopLength > 0 && p > effectiveLoopLength);

    if (voiceDots) {
      bool playingHere = voicePlayPage == p;
      if (playingHere && GLOB.edit == p) {
        ledColor = CRGB(255, 255, 50);
      } else if (playingHere) {
        ledColor = CRGB(0, 255, 0);
      } else if (GLOB.edit == p) {
        ledColor = CRGB(255, 255, 0);
      } else {
        ledColor = voiceHas ? CRGB(220, 220, 220) : CRGB(1, 0, 0);
      }
    } else if (outsideLoop) {

      ledColor = CRGB(10, 0, 0);
    } else {


      if (GLOB.page == p && GLOB.edit == p) {

        ledColor = CRGB(255, 255, 50);
      } else if (GLOB.page == p) {

        ledColor = CRGB(0, 255, 0);
      } else {
        if (GLOB.edit == p) {

          ledColor = CRGB(255, 255, 0);
        } else {
          ledColor = hasNotes[p] ? CRGB(0, 0, 35) : CRGB(1, 0, 0);
        }
      }
    }


    light(p, maxY, ledColor);
  }





  extern bool songModeActive;
  extern float marqueePos;
  if (songModeActive) {
    light(round(marqueePos), 1, CRGB(0, 80, 0));
  }
}




FLASHMEM void drawTriggers() {


  extern bool voiceMode;
  extern bool songModeActive;
  extern unsigned int voiceOwnedStep(unsigned int row, unsigned int col, unsigned int globalPage);
  extern void voiceFillColumn(unsigned int col, unsigned int globalPage, unsigned int srcOut[16]);
  extern unsigned int voicePickStep(const unsigned int src[16], unsigned int row);
  extern unsigned int beatForUI;
  const bool voiceGrid = voiceMode && isNowPlaying && !songModeActive;
  unsigned int voiceGlobalPage = 1;
  if (voiceGrid && beatForUI > 0 && maxX > 0) voiceGlobalPage = ((beatForUI - 1) / maxX) + 1;
  const bool isSingle = GLOB.singleMode;
  const bool isSimpleNotes = (simpleNotesView == 1 && !isSingle);



  bool muteCache[maxY + 1];
  extern bool getMuteStateForUI(int channel);
  for (unsigned int ch = 0; ch <= maxY; ++ch) {
    muteCache[ch] = getMuteStateForUI(ch);
  }


  const unsigned long now = millis();
  const uint8_t blinkPhase = (uint8_t)((now / 300) & 0x1);

  unsigned int voiceSrc[16];
  unsigned int voiceSrcCol = 0;

  for (unsigned int ix = 1; ix < maxX + 1; ix++) {
    if (voiceGrid && voiceSrcCol != ix) {
      voiceFillColumn(ix, voiceGlobalPage, voiceSrc);
      voiceSrcCol = ix;
    }
    for (unsigned int iy = 1; iy < maxY + 1; iy++) {
      unsigned int globalX;
      if (voiceGrid) {
        globalX = voicePickStep(voiceSrc, iy);
        if (globalX == 0) continue;
      } else {
        unsigned int rowPage = GLOB.edit > 0 ? GLOB.edit : 1;
        globalX = (rowPage - 1) * maxX + ix;
      }
      Note &cell = note[globalX][iy];
      int thisNote = cell.channel;
      if (thisNote <= 0) continue;



      if (isSimpleNotes && !voiceGrid) {
        int voiceY = thisNote + 1;
        if (voiceY >= 1 && voiceY <= (int)maxY) {
          light(ix, iy, CRGB(0, 0, 0));
          if (muteCache[thisNote]) {
            light(ix, voiceY, col_base[thisNote]);
          } else {
            light(ix, voiceY, col[thisNote]);
          }
        }
        continue;
      }

      const bool muted = muteCache[thisNote];
      if (!muted) {

        if (isSingle && thisNote == (int)GLOB.currentChannel) {
          uint8_t prob = cell.probability;
          uint8_t cond = cell.condition;
          if (cond == 0) cond = 1;

          CRGB noteColor = UI_BRIGHT_WHITE;


          if (cond == 2) {
            noteColor = (blinkPhase == 0) ? CRGB(0, 0, 255) : UI_BRIGHT_WHITE;
          } else if (cond == 4) {
            noteColor = (blinkPhase == 0) ? CRGB(200, 0, 255) : UI_BRIGHT_WHITE;
          } else if (cond == 8) {
            noteColor = (blinkPhase == 0) ? CRGB(255, 100, 0) : UI_BRIGHT_WHITE;
          } else if (cond == 16) {
            noteColor = (blinkPhase == 0) ? CRGB(0, 200, 200) : UI_BRIGHT_WHITE;
          } else if (cond == 17) {
            noteColor = (blinkPhase == 0) ? CRGB(0, 150, 255) : UI_BRIGHT_WHITE;
          } else if (cond == 18) {
            noteColor = (blinkPhase == 0) ? CRGB(150, 0, 255) : UI_BRIGHT_WHITE;
          } else if (cond == 19) {
            noteColor = (blinkPhase == 0) ? CRGB(255, 150, 0) : UI_BRIGHT_WHITE;
          } else if (cond == 20) {
            noteColor = (blinkPhase == 0) ? CRGB(0, 255, 200) : UI_BRIGHT_WHITE;
          } else if (cond == NOTE_CONDITION_GLIDE) {
            noteColor = (blinkPhase == 0) ? CRGB(0, 255, 120) : UI_BRIGHT_WHITE;
          } else if (prob < 100) {

            if (blinkPhase == 0) {
              uint8_t redIntensity = mapf(prob, 0, 100, 255, 80);
              noteColor = CRGB(redIntensity, 0, 0);
            } else {
              uint8_t brightness = mapf(prob, 0, 100, 30, 255);
              noteColor.nscale8(brightness);
            }
          }

          light(ix, iy, noteColor);
        } else {
          light(ix, iy, col[thisNote]);
        }

        if (thisNote != (int)GLOB.currentChannel && currentMode == &singleMode) {
          light(ix, iy, col_base[thisNote]);
        }
      } else {

        if (isSingle) {
          light(ix, iy, UI_DIM_WHITE);
        } else {
          light(ix, iy, col_base[thisNote]);
        }
      }
    }
  }
}






struct FireFly {
  int8_t x;
  int8_t y;
  int8_t drift;
  uint8_t life;
  uint8_t ch;
};
static const int FIRE_POOL = 25;
static FireFly fireFlies[FIRE_POOL];
static uint8_t fireSeenCol = 0;
static uint16_t fireSeenPage = 0;
static unsigned long fireTickAt = 0;
static bool firePoolActive = false;

static void fireClear() {
  if (!firePoolActive) return;
  for (int i = 0; i < FIRE_POOL; i++) fireFlies[i].life = 0;
  firePoolActive = false;
}

FLASHMEM static int fireLiving() {
  int n = 0;
  for (int i = 0; i < FIRE_POOL; i++) if (fireFlies[i].life) n++;
  return n;
}

FLASHMEM static int fireOldestSlot() {
  int slot = 0;
  uint8_t best = 255;
  for (int i = 0; i < FIRE_POOL; i++) {
    if (!fireFlies[i].life) return i;
    if (fireFlies[i].life < best) {
      best = fireFlies[i].life;
      slot = i;
    }
  }
  return slot;
}

FLASHMEM static void fireTrim(int cap) {
  for (;;) {
    int living = 0;
    int oldest = -1;
    uint8_t best = 255;
    for (int i = 0; i < FIRE_POOL; i++) {
      if (!fireFlies[i].life) continue;
      living++;
      if (fireFlies[i].life < best) {
        best = fireFlies[i].life;
        oldest = i;
      }
    }
    if (living <= cap || oldest < 0) return;
    fireFlies[oldest].life = 0;
  }
}

FLASHMEM static void fireSpawnOne(int x, int y, uint8_t ch, int n) {
  if (ch < 1 || ch > 14) return;
  int8_t drift = (n & 1) ? 1 : -1;

  int nx = x + 1;
  int ny = y - drift;
  if (nx > (int)maxX) nx = x;
  if (ny < 1) ny = 1;
  if (ny >= (int)maxY) ny = (int)maxY - 1;
  if (nx < 1) nx = 1;
  if (ny < 1 || ny >= (int)maxY) return;
  FireFly &f = fireFlies[fireOldestSlot()];
  f.x = (int8_t)nx;
  f.y = (int8_t)ny;
  f.drift = drift;
  f.life = 10;
  f.ch = ch;
  firePoolActive = true;
}

FLASHMEM static void fireSpawnColumn(const uint8_t *xs, const uint8_t *ys, const uint8_t *chs, int nHits) {
  extern uint8_t fireLevel;
  if (!nHits || !fireLevel) return;
  int cap = fireLevel;
  if (cap > FIRE_POOL) cap = FIRE_POOL;
  if (cap < 1) cap = 1;
  int room = cap - fireLiving();
  int born = room > 0 ? room : (cap + 3) / 4;
  if (born < 1) born = 1;
  if (born > cap) born = cap;
  for (int n = 0; n < born; n++) {
    int h = n % nHits;
    fireSpawnOne(xs[h], ys[h], chs[h], n);
  }
  fireTrim(cap);
}

FLASHMEM static void firePixel(int x, int y, int playX, const CRGB &c) {
  if (x < 1 || x > (int)maxX || y < 1 || y >= (int)maxY) return;
  if (x == playX) return;
  light(x, y, c);
}

FLASHMEM static void fireDraw(int playX) {
  extern uint8_t fireSize;
  extern uint8_t fireGravity;
  extern uint8_t fireColor;
  extern CRGB col[];
  if (!firePoolActive) return;
  unsigned long now = millis();
  const bool step = (unsigned long)(now - fireTickAt) >= 32;
  if (step) fireTickAt = now;
  const int gravPeriod = fireGravity ? (9 - (int)fireGravity) : 0;
  bool any = false;
  for (int i = 0; i < FIRE_POOL; i++) {
    FireFly &f = fireFlies[i];
    if (!f.life) continue;
    if (step) {
      f.life--;
      if ((f.life & 1) == 0) {
        f.x = (int8_t)(f.x + 1);
        if ((f.life & 2) == 0) f.y = (int8_t)(f.y - f.drift);
      }
      if (gravPeriod > 0 && (f.life % gravPeriod) == 0) f.x = (int8_t)(f.x - 1);
    }
    if (!f.life || f.y < 1 || f.y >= (int)maxY || f.x < 1 || f.x > (int)maxX || f.ch < 1 || f.ch > 14) {
      f.life = 0;
      continue;
    }
    any = true;
    if (f.x == playX) continue;
    CRGB voice = col[f.ch];
    CRGB base = voice;
    if (fireColor == 0) base = CRGB(230, 230, 230);
    else if (fireColor < 8) base = blend(CRGB(230, 230, 230), voice, (uint8_t)(fireColor * 32));
    uint8_t scale = (uint8_t)(f.life * 25);
    base.nscale8(scale);
    if (fireSize >= 2) {
      CRGB side = base;
      side.nscale8(110);
      firePixel(f.x, f.y - 1, playX, side);
      firePixel(f.x, f.y + 1, playX, side);
    }
    if (fireSize >= 3) {
      CRGB up = base;
      up.nscale8(80);
      firePixel(f.x + 1, f.y, playX, up);
    }
    if (fireSize >= 4) {
      CRGB corner = base;
      corner.nscale8(50);
      firePixel(f.x + 1, f.y - 1, playX, corner);
      firePixel(f.x + 1, f.y + 1, playX, corner);
    }
    if (f.life > 5 && f.x > 1) {
      CRGB trail = base;
      trail.nscale8(64);
      firePixel(f.x - 1, f.y, playX, trail);
    }
    firePixel(f.x, f.y, playX, base);
  }
  if (!any) firePoolActive = false;
}

void drawTimer() {

  unsigned int timer = ((beatForUI - 1) % maxX + 1);


  unsigned int beatForUIPage = (beatForUI - 1) / maxX + 1;


  extern bool SMP_FLOW_MODE;
  extern bool SMP_PATTERN_MODE;
  extern int patternMode;
  bool shouldShowTimer = false;

  extern bool voiceMode;
  extern bool songModeActive;
  extern void voiceFillColumn(unsigned int col, unsigned int globalPage, unsigned int srcOut[16]);
  extern unsigned int voicePickStep(const unsigned int src[16], unsigned int row);
  const bool voiceGrid = voiceMode && isNowPlaying && !songModeActive;
  if (voiceGrid) {

    shouldShowTimer = true;
  } else if (SMP_FLOW_MODE) {



    shouldShowTimer = (beatForUIPage == GLOB.edit);
  } else if (SMP_PATTERN_MODE) {


    shouldShowTimer =
        (patternMode != 3) || (beatForUIPage == GLOB.edit);
  } else {




    shouldShowTimer = (beatForUIPage == GLOB.edit);
  }


  if (shouldShowTimer) {
      if (timer < 1) timer = 1;
      extern uint8_t fireVoice;
      extern uint8_t fireLevel;
      const bool fireOn = fireVoice && fireLevel;
      if (!fireOn) fireClear();

      uint8_t fireHits = 0;
      uint8_t fireHitX[15];
      uint8_t fireHitY[15];
      uint8_t fireHitCh[15];
      bool freshCol = false;
      if (fireOn) {
        freshCol = (timer != fireSeenCol) || (beatForUIPage != fireSeenPage);
        if (freshCol) {
          fireSeenCol = (uint8_t)timer;
          fireSeenPage = (uint16_t)beatForUIPage;
        }
      }
      unsigned int voiceSrc[16];
      bool voiceSrcReady = false;
      for (unsigned int y = 1; y < maxY; y++) {
        unsigned int srcStep;
        if (voiceGrid) {
          if (!voiceSrcReady) {
            voiceFillColumn(timer, beatForUIPage, voiceSrc);
            voiceSrcReady = true;
          }
          srcStep = voicePickStep(voiceSrc, y);
          if (srcStep == 0) {
            light(timer, y, CRGB(10, 0, 0));
            continue;
          }
        } else {
          unsigned int notePage = GLOB.edit > 0 ? GLOB.edit : 1;
          srcStep = (notePage - 1) * maxX + timer;
        }
        int ch = note[srcStep][y].channel;
        light(timer, y, CRGB(10, 0, 0));

        if (ch> 0) {
          if (getMuteState(ch) == false) {
            const bool lit = !GLOB.singleMode || (GLOB.currentChannel == ch);
            if (lit) {
              light(timer, y, UI_BRIGHT_WHITE);
              if (fireOn && freshCol && fireHits < 15 &&
                  (fireVoice == 15 || (uint8_t)ch == fireVoice)) {
                fireHitX[fireHits] = (uint8_t)timer;
                fireHitY[fireHits] = (uint8_t)y;
                fireHitCh[fireHits] = (uint8_t)ch;
                fireHits++;
              }
            }
          } else {
            if( !GLOB.singleMode ) light(timer, y, CRGB(00, 00, 00));
          }
        }
      }
      if (fireOn) {
        if (freshCol && fireHits) fireSpawnColumn(fireHitX, fireHitY, fireHitCh, fireHits);
        fireDraw((int)timer);
      }
    } else {
      fireClear();
    }
}

FLASHMEM static int countLabelWidth(const char *text) {
  int w = 0;
  for (int i = 0; text[i] != '\0'; i++) {
    if (text[i] >= 32 && text[i] <= 126) w += alphabet[text[i] - 32][0] + 1;
  }
  if (w > 0) w -= 1;
  return w;
}

FLASHMEM static int countLabelX(const char *text) {
  int x = (((int)maxX - countLabelWidth(text)) / 2) + 1;
  if (x < 1) x = 1;
  return x;
}

FLASHMEM static void drawCountDigit(int n) {
  if (n < 1 || n > 4) return;
  char countText[4];
  snprintf(countText, sizeof(countText), "%d", n);
  const char *recLabel = "REC";
  const int countY = 3;
  const int recY = 9;
  const int recX = countLabelX(recLabel);
  const int countX = countLabelX(countText);
  int bgStartX = min(recX, countX);
  int recW = countLabelWidth(recLabel);
  int countW = countLabelWidth(countText);
  int bgEndX = max(recX + recW - 1, countX + countW - 1);
  bgStartX = max(1, bgStartX);
  bgEndX = min((int)maxX, bgEndX);
  const int bgStartY = min(recY, countY);
  const int bgEndY = max(recY, countY) + 4;
  for (int x = bgStartX; x <= bgEndX; x++) {
    for (int y = bgStartY; y <= bgEndY; y++) {
      if (x >= 1 && x <= (int)maxX && y >= 1 && y <= (int)maxY) {
        light(x, y, CRGB(0, 0, 0));
      }
    }
  }
  CRGB countColor;
  if (n == 1) countColor = CRGB(255, 255, 255);
  else if (n == 2) countColor = CRGB(0, 255, 0);
  else if (n == 3) countColor = CRGB(255, 255, 0);
  else countColor = CRGB(255, 0, 0);
  drawText(recLabel, recX, recY, CRGB(255, 0, 0));
  drawText(countText, countX, countY, countColor);
}

FLASHMEM void drawCountIn() {
  extern bool countInActive;
  extern int countInBeat;
  if (countInActive && countInBeat > 0 && countInBeat <= 4) {
    drawCountDigit(countInBeat);
  }
}

FLASHMEM void drawSensCountIn() {
  extern bool sensCountActive;
  extern int sensCountSec;
  if (sensCountActive && sensCountSec >= 1 && sensCountSec <= 3) {
    drawCountDigit(sensCountSec);
  }
}

int mapXtoPageOffset(int x) {
  return x - (GLOB.edit - 1) * maxX;
}




void drawCursor() {
  if (dir == 1)
    pulse += 8;
  if (dir == -1)
    pulse -= 8;
  if (pulse > 230) {
    dir = -1;
  }
  if (pulse < 1) {
    dir = 1;
  }

  int cursorX = mapXtoPageOffset(GLOB.x);
  int cursorY = GLOB.y;

  uint8_t hue = pulse;
  if (note[GLOB.x][GLOB.y].channel) {

    CRGB color = col[note[GLOB.x][GLOB.y].channel];
    light(cursorX, cursorY, color.nscale8_video(pulse));

  }else{
    light(cursorX, cursorY, CHSV(hue, 255, 255));
  }
}

FLASHMEM void drawBrightness() {



  unsigned int numLEDs = ((ledBrightness - 3) * 16) / (255 - 3) + 1;
  numLEDs = constrain(numLEDs, 1, 16);


  bool atDefault = (ledBrightness == LED_BRIGHTNESS_DEFAULT);

  for (unsigned int x = 1; x <= numLEDs && x <= 16; x++) {
    CRGB color;
    if (atDefault) {
      color = CRGB(255, 255, 0);
    } else {
      color = CRGB(16 * x, 16 * x, 16 * x);
    }
    light(x, 15, color);
    light(x, 16, color);
  }
}

static inline void drawIcon7x12_P(const uint8_t *data, int ox, int oy, CRGB color, int yOffset = 0) {
  for (uint8_t y = 0; y < ICON_H; y++) {
    for (uint8_t x = 0; x < ICON_W; x++) {
      if (icon7x12_on_P(data, x, y)) {


        light(ox + x, (int)maxY - (oy + y) + yOffset, color);
      }
    }
  }
}

FLASHMEM void showIconsAt(IconType ico, CRGB colors, int ox, int oy) {
  const uint8_t *data = nullptr;
  int yOffset = 0;

  switch (ico) {
    case ICON_DELETE: data = icon_delete; break;
    case OLD_ICON_SAMPLEPACK: data = OLD_icon_samplepack; break;
    case OLD_ICON_SAMPLE: data = OLD_icon_sample; break;
    case OLD_ICON_LOADSAVE: data = OLD_icon_loadsave; break;
    case OLD_ICON_LOADSAVE2: data = OLD_icon_loadsave2; break;
    case ICON_NEW: data = icon_new; break;
    case ICON_HOURGLASS: data = icon_hourglass; yOffset = 2; break;

    case HELPER_LOAD: data = helper_load; break;
    case HELPER_SEEK: data = helper_seek; break;
    case HELPER_SEEKSTART: data = helper_seekstart; break;
    case HELPER_FOLDER: data = helper_folder; break;
    case HELPER_SAVE: data = helper_save; break;
    case HELPER_EXIT: data = helper_exit; break;
    case HELPER_SELECT: data = helper_select; break;
    case HELPER_VOL: data = helper_vol; break;
    case HELPER_BRIGHT: data = helper_bright; break;
    case HELPER_BPM: data = helper_bpm; break;
    case HELPER_MINUS: data = helper_minus; break;

    case OLD_ICON_BPM: data = OLD_icon_bpm; break;
    case OLD_ICON_VOL: data = OLD_icon_vol; break;
    case ICON_SETTINGS: data = icon_settings; break;
    case OLD_ICON_REC: data = OLD_icon_rec; break;
    case OLD_ICON_REC2: data = OLD_icon_rec2; break;

    case ICON_PACK: data = icon_pack; break;
    case ICON_CLOCK: data = icon_clock; break;
    case ICON_PATTERN: data = icon_pattern; break;
    case ICON_FOLDER_BIG: data = icon_folder_big; break;
    case ICON_FILE_BIG: data = icon_file_big; break;
    case ICON_SYNC: data = icon_sync; break;
    case ICON_VOLUME_BIG: data = icon_volume_big; break;
    case ICON_SETTINGS_BIG: data = icon_settings_big; break;
    case ICON_VIEW: data = icon_view; break;
    case ICON_SAMPLE_BIG: data = icon_sample_big; break;
    case ICON_ENGINE: data = icon_engine; break;
    case ICON_MIDISYNC: data = icon_midisync; break;
    case ICON_SONG: data = icon_song; break;
    case ICON_RECORD: data = icon_record; break;
    default: break;
  }

  if (data != nullptr) {
    drawIcon7x12_P(data, ox, oy, colors, yOffset);
  }
}

FLASHMEM void showIcons(IconType ico, CRGB colors) {
  const uint8_t *data = nullptr;
  int ox = 0;
  int oy = 0;
  int yOffset = 0;


  switch (ico) {
    case ICON_DELETE: data = icon_delete; ox = 1; oy = 1; break;
    case OLD_ICON_SAMPLEPACK: data = OLD_icon_samplepack; ox = 2; oy = 1; break;
    case OLD_ICON_SAMPLE: data = OLD_icon_sample; ox = 1; oy = 1; break;
    case OLD_ICON_LOADSAVE: data = OLD_icon_loadsave; ox = 1; oy = 1; break;
    case OLD_ICON_LOADSAVE2: data = OLD_icon_loadsave2; ox = 1; oy = 6; break;

    case ICON_NEW: data = icon_new; ox = 9; oy = 13; break;
    case ICON_HOURGLASS: data = icon_hourglass; ox = 6; oy = 4; yOffset = 2; break;

    case HELPER_LOAD: data = helper_load; ox = 1; oy = 15; break;
    case HELPER_SEEK: data = helper_seek; ox = 10; oy = 14; break;
    case HELPER_SEEKSTART: data = helper_seekstart; ox = 2; oy = 14; break;
    case HELPER_FOLDER: data = helper_folder; ox = 6; oy = 13; break;
    case HELPER_SAVE: data = helper_save; ox = 5; oy = 15; break;
    case HELPER_EXIT: data = helper_exit; ox = 1; oy = 15; break;
    case HELPER_SELECT: data = helper_select; ox = 13; oy = 15; break;
    case HELPER_VOL: data = helper_vol; ox = 9; oy = 13; break;
    case HELPER_BRIGHT: data = helper_bright; ox = 5; oy = 13; break;
    case HELPER_BPM: data = helper_bpm; ox = 13; oy = 13; break;
    case HELPER_MINUS: data = helper_minus; ox = 9; oy = 14; break;

    case OLD_ICON_BPM: data = OLD_icon_bpm; ox = 1; oy = 1; break;
    case OLD_ICON_VOL: data = OLD_icon_vol; ox = 2; oy = 3; break;
    case ICON_SETTINGS: data = icon_settings; ox = 1; oy = 2; break;
    case OLD_ICON_REC: data = OLD_icon_rec; ox = 2; oy = 4; break;
    case OLD_ICON_REC2: data = OLD_icon_rec2; ox = 1; oy = 3; break;



    case ICON_PACK: data = icon_pack; ox = 2; oy = 1; break;
    case ICON_CLOCK: data = icon_clock; ox = 2; oy = 1; break;
    case ICON_PATTERN: data = icon_pattern; ox = 2; oy = 1; break;
    case ICON_FOLDER_BIG: data = icon_folder_big; ox = 2; oy = 1; break;
    case ICON_FILE_BIG: data = icon_file_big; ox = 2; oy = 1; break;
    case ICON_SYNC: data = icon_sync; ox = 1; oy = 1; break;
    case ICON_VOLUME_BIG: data = icon_volume_big; ox = 2; oy = 1; break;
    case ICON_SETTINGS_BIG: data = icon_settings_big; ox = 2; oy = 1; break;
    case ICON_VIEW: data = icon_view; ox = 2; oy = 1; break;
    case ICON_SAMPLE_BIG: data = icon_sample_big; ox = 2; oy = 1; break;
    case ICON_ENGINE: data = icon_engine; ox = 2; oy = 1; break;
    case ICON_MIDISYNC: data = icon_midisync; ox = 2; oy = 1; break;
    case ICON_SONG: data = icon_song; ox = 2; oy = 1; break;
    case ICON_RECORD: data = icon_record; ox = 2; oy = 1; break;
    default: break;
  }

  if (data != nullptr) {
    drawIcon7x12_P(data, ox, oy, colors, yOffset);
  }
}

void processPeaks() {
  static float interpolatedValues[32];
  static float lastSeek = -1;
  static float lastSeekEnd = -1;
  static int lastPeakIndex = -1;
  static bool hasCalculated = false;
  static CRGB cachedColors[32][6];
  static bool colorsCached = false;
  static float lastCachedValues[32];




  bool needsRecalc = (GLOB.seek != lastSeek) ||
                     (GLOB.seekEnd != lastSeekEnd) ||
                     (peakIndex != lastPeakIndex) ||
                     (peakIndex > 0 && !hasCalculated);

  if (needsRecalc) {
    lastSeek = GLOB.seek;
    lastSeekEnd = GLOB.seekEnd;
    lastPeakIndex = peakIndex;
  }


  if (peakIndex == 0) {
    hasCalculated = false;
    colorsCached = false;
  }

  if (peakIndex > 0 && needsRecalc) {
    hasCalculated = true;

    for (int i = 0; i < maxX; i++) {


      float normalizedPos = (float)i / (float)(maxX - 1);


      float seekPosition = GLOB.seek + normalizedPos * (GLOB.seekEnd - GLOB.seek);
      seekPosition = constrain(seekPosition, 0.0f, 100.0f);


      float peakPos = (seekPosition / 100.0f) * (peakIndex - 1);
      peakPos = constrain(peakPos, 0.0f, (float)(peakIndex - 1));

      int lowerIndex = floor(peakPos);
      int upperIndex = min(lowerIndex + 1, peakIndex - 1);


      if (peakIndex > 1 && lowerIndex != upperIndex) {
        float fraction = peakPos - lowerIndex;
        interpolatedValues[i] = peakValues[lowerIndex] * (1.0f - fraction) + peakValues[upperIndex] * fraction;
      } else {
        interpolatedValues[i] = peakValues[lowerIndex];
      }
    }
    colorsCached = false;
  } else if (needsRecalc) {

    for (int i = 0; i < maxX; i++) {
      interpolatedValues[i] = 0;
    }
    colorsCached = false;
  }


  static float cachedGainFactor = 1.0f;

  if (needsRecalc) {

    float maxPeak = 0.0f;
    for (int i = 0; i < maxX; i++) {
      if (interpolatedValues[i] > maxPeak) {
        maxPeak = interpolatedValues[i];
      }
    }


    if (maxPeak > 0.001f) {
      cachedGainFactor = 1.0f / maxPeak;
      cachedGainFactor = constrain(cachedGainFactor, 1.0f, 32.0f);
    } else {
      cachedGainFactor = 1.0f;
    }
  }

  float gainFactor = cachedGainFactor;


  for (int x = 1; x <= maxX; x++) {
    light(x, 3, CRGB(0, 0, 50));
  }


  int seekStartX = mapf(GLOB.seek, 0, 100, 1, maxX);
  int seekEndX = mapf(GLOB.seekEnd, 0, 100, 1, maxX);
  seekStartX = constrain(seekStartX, 1, maxX);
  seekEndX = constrain(seekEndX, 1, maxX);

  if (seekEndX <= seekStartX) {
    seekEndX = min(maxX, seekStartX + 1);
  }

  for (int x = 1; x <= seekStartX; x++) {
    light(x, 3, CRGB(0, 80, 0));
  }

  for (int x = seekEndX; x <= maxX; x++) {
    light(x, 3, CRGB(80, 0, 0));
  }


  bool needColorRecalc = !colorsCached;
  if (!needColorRecalc) {
    for (int i = 0; i < maxX; i++) {

      if (fabs(interpolatedValues[i] - lastCachedValues[i]) > 0.001f) {
        needColorRecalc = true;
        break;
      }
    }
  }


  if (peakIndex <= 0) {
    return;
  }


  for (int i = 0; i < maxX; i++) {
    int x = i + 1;


    float normalizedValue = interpolatedValues[i] * gainFactor;
    normalizedValue = constrain(normalizedValue, 0.0f, 1.0f);


    int yPeak = mapf(normalizedValue * 100, 0, 100, 5, 10);
    yPeak = constrain(yPeak, 5, 10);


    for (int y = 2; y < 5; y++) {
      if (y == 3) continue;
      light(x, y, CRGB::Black);
    }


    if (needColorRecalc) {
      for (int y = 5; y <= 10; y++) {


        float normalizedY = mapf(y, 5, 10, 0.0f, 1.0f);
        int hue = mapf(normalizedY, 0.0f, 1.0f, 0, 170);



        int sector = hue / 60;
        int remainder = hue % 60;
        int p = 0;
        int q = (255 * (60 - remainder)) / 60;
        int t = (255 * remainder) / 60;

        CRGB color;
        switch(sector) {
          case 0: color = CRGB(255, t, p); break;
          case 1: color = CRGB(q, 255, p); break;
          case 2: color = CRGB(p, 255, t); break;
          default: color = CRGB(p, q, 255); break;
        }


        color.r = (color.r * 40) / 100;
        color.g = (color.g * 40) / 100;
        color.b = (color.b * 40) / 100;

        cachedColors[i][y - 5] = color;
      }
    }


    for (int y = 5; y <= yPeak; y++) {
      light(x, y, cachedColors[i][y - 5]);
    }
  }


  if (needColorRecalc) {
    for (int i = 0; i < maxX; i++) {
      lastCachedValues[i] = interpolatedValues[i];
    }
    colorsCached = true;
  }
}

void drawLoadingBar(int minval, int maxval, int currentval, CRGB color, CRGB fontColor, bool intro) {
  int ypos = 4;
  int height = 2;
  int barwidth = mapf(currentval, minval, maxval, 1, maxX);
  FastLEDclear();

  for (int x = 1; x <= maxX; x++) {
    light(x, ypos - 1, fontColor);
  }

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

    drawNumber(currentval, fontColor, 11);
  } else {
    if (barwidth<4) drawText("LOAD", 2, 11, UI_BG_DARK);
    if (barwidth>=4 && barwidth<=8) drawText("KIT", 2, 11, UI_BG_DARK);
    char buf[6];
    snprintf(buf, sizeof(buf), "no.%d", SMP.pack );
    if (barwidth>8) drawText(buf, 2, 11, UI_BG_DARK);
    FastLEDshow();
  }
}





FLASHMEM void drawMidiArrow(bool direction, int x, int y, bool blink = false) {
  CRGB color = direction ? CRGB(0, 255, 0) : CRGB(255, 0, 0);
  if (!direction) {
    extern bool getAnalogExtClock();
    if (getAnalogExtClock()) color = CRGB(255, 165, 0);
  }


  if (blink) {
    color.r = (color.r * 30) / 100;
    color.g = (color.g * 30) / 100;
    color.b = (color.b * 30) / 100;
  }


  if (x < 1 || x > (int)maxX || y < 1 || y > maxY) return;

  if (direction) {


    if (x + 1 <= (int)maxX) light(x + 1, y, color);
    if (y - 1 >= 1) light(x, y - 1, color);
    if (y + 1 <= maxY) light(x, y + 1, color);
  } else {


    if (x - 1 >= 1) light(x - 1, y, color);
    if (y - 1 >= 1) light(x, y - 1, color);
    if (y + 1 <= maxY) light(x, y + 1, color);
  }
}


FLASHMEM void drawBPMWithReservedSpace(float bpm, CRGB color, int topY) {
  char buffer[16];



  int bpmInt = (int)round(bpm);
  sprintf(buffer, "%3d", bpmInt);


  drawText(buffer, 2, topY, color);
}

FLASHMEM void drawBPMScreen() {

  FastLEDclear();

  if (drawBaseColorMode) {
    drawBrightness();
  }






  drawIndicator('L', 'W', 2, false, false);
  drawIndicator('L', 'G', 3, false, false);
  drawIndicator('L', 'N', 4, false, false);


  extern Mode *currentMode;
  extern Mode bpmMode;
  extern int clockMode;
  bool isInt = false;
  if (currentMode == &bpmMode) {

    isInt = (currentMode->pos[2] == 1);
  } else {

    isInt = (clockMode == 1);
  }




  extern bool getBPMStable();
  extern bool getAnalogExtClock();
  const bool analogExt = !isInt && getAnalogExtClock();
  if (isInt) {
    drawBPMWithReservedSpace(SMP.bpm, UI_CYAN, 6);
  } else if (analogExt) {
    drawBPMWithReservedSpace(SMP.bpm, CRGB(255, 165, 0), 6);
  } else {
    bool isStable = getBPMStable();
    CRGB extColor = isStable ? CRGB(0, 255, 0) : CRGB(255, 0, 0);
    drawBPMWithReservedSpace(SMP.bpm, extColor, 6);
  }

  bool shouldBlink = false;
  if (!isInt) {
    if (analogExt) {
      extern bool analogClockPulseFresh();
      shouldBlink = !analogClockPulseFresh();
    } else {
      extern uint16_t getMidiClockTicks();
      uint16_t clockTicks = getMidiClockTicks();
      shouldBlink = ((clockTicks % 12) < 6);
    }
  }
  drawMidiArrow(isInt, maxX - 1, 8, shouldBlink);


  if (currentMode == &bpmMode) {

    char arrowColor = isInt ? 'G' : (analogExt ? 'O' : 'R');
    drawIndicator('L', arrowColor, 3, false, false);

    static uint32_t lastBpmRGB[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
    auto setBpmRGB = [&](int enc, uint32_t rgb) {
      if (enc < 0 || enc > 3) return;
      if (lastBpmRGB[enc] == rgb) return;
      lastBpmRGB[enc] = rgb;
      Encoder[enc].writeRGBCode(rgb);
    };
    CRGB w = getIndicatorColor('W');
    CRGB n = getIndicatorColor('N');
    CRGB arrow = getIndicatorColor(arrowColor);
    CRGB wM = normalizeToMaxBrightness(w);
    CRGB nM = normalizeToMaxBrightness(n);
    CRGB aM = normalizeToMaxBrightness(arrow);
    setBpmRGB(1, (uint32_t)wM.r << 16 | (uint32_t)wM.g << 8 | wM.b);
    setBpmRGB(2, (uint32_t)aM.r << 16 | (uint32_t)aM.g << 8 | aM.b);
    setBpmRGB(3, (uint32_t)nM.r << 16 | (uint32_t)nM.g << 8 | nM.b);
  }
}

void drawKnobColorDefault(){
  for (int i = 0; i < NUM_ENCODERS; i++) {
      Encoder[i].writeRGBCode(currentMode->knobcolor[i]);
  }
}




FLASHMEM void showSongMode() {
  extern Mode songMode;
  extern bool songModeActive;


  int songPosition = songMode.pos[3];
  int selectedPattern = songMode.pos[1];


  for (int x = 1; x <= maxX; x++) {
    for (int y = 1; y <= maxY; y++) {
      light(x, y, CRGB(0, 0, 0));
    }
  }


  extern float marqueePos;
  if (songModeActive) {
    light(round(marqueePos), 1, CRGB(0, 80, 0));
  }


  drawIndicator('L', 'M', 2, false, false);
  drawIndicator('L', 'N', 4, false, false);
  {
    static uint32_t lastSongRGB[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
    auto setSongRGB = [&](int enc, uint32_t rgb) {
      if (enc < 0 || enc > 3) return;
      if (lastSongRGB[enc] == rgb) return;
      lastSongRGB[enc] = rgb;
      Encoder[enc].writeRGBCode(rgb);
    };
    CRGB m = normalizeToMaxBrightness(getIndicatorColor('M'));
    CRGB n = normalizeToMaxBrightness(getIndicatorColor('N'));
    setSongRGB(1, (uint32_t)m.r << 16 | (uint32_t)m.g << 8 | m.b);
    setSongRGB(3, (uint32_t)n.r << 16 | (uint32_t)n.g << 8 | n.b);
  }


  char patText[8];
  snprintf(patText, sizeof(patText), "%02d", selectedPattern);
  int hue1 = (selectedPattern * 16) % 255;
  drawText(patText, 1, 11, CHSV(hue1, 255, 255));


  char posText[8];
  snprintf(posText, sizeof(posText), "%02d", songPosition);
  drawText(posText, 10, 11, CRGB(0, 255, 255));


  extern Device SMP;
  int storedPattern = SMP.songArrangement[songPosition - 1];
  if (storedPattern > 0) {
    char storedText[8];
    snprintf(storedText, sizeof(storedText), ">%02d", storedPattern);
    int hue2 = (storedPattern * 16) % 255;
    drawText(storedText, 1, 5, CHSV(hue2, 255, 255));
  }



  extern unsigned int maxX;
  int startPos = ((songPosition - 1) / maxX) * maxX;

  extern Device SMP;
  for (int i = 0; i < maxX && (startPos + i) < 64; i++) {
    int pos = startPos + i;
    int pattern = SMP.songArrangement[pos];

    int x = (i % maxX) + 1;
    int y = 3 - (i / maxX);

    CRGB color;
    if (pos == (songPosition - 1)) {

      color = CRGB(200, 200, 0);
    } else if (pattern > 0) {

      int hue = (pattern * 16) % 255;
      color = CHSV(hue, 255, 80);
    } else {

      color = CRGB(5, 5, 5);
    }

    light(x, y, color);
  }


  extern float marqueePos;
  if (songModeActive) {
    light(round(marqueePos), 1, CRGB(0, 80, 0));
  }


  extern int currentSongPosition;
  extern bool isNowPlaying;
  if (songModeActive && isNowPlaying && currentSongPosition >= 0) {

    if (currentSongPosition >= startPos && currentSongPosition < startPos + maxX) {
      int indicatorX = (currentSongPosition - startPos) + 1;
      light(indicatorX, 2, CRGB(255, 255, 255));
    }
  }
}