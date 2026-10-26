// Menu page system - completely independent from maxPages
#define MENU_PAGES_COUNT 10
#define LOOK_PAGES_COUNT 4
#define RECS_PAGES_COUNT 6
#define MIDI_PAGES_COUNT 10
#define VOL_PAGES_COUNT 5
#define ETC_PAGES_COUNT 6
#define MODE_PAGES_COUNT 11
#define ADV_PAGES_COUNT 10

// PPQN page value readout: 0=rate/OFF, 1=STOP/CONT, 2=pulse width
static uint8_t g_ppqnUiFocus = 0;
static uint8_t g_cinFocus = 0;  // MIDI → PQ-I last turned: 0 rate/OFF (enc3), 2 hold-off (enc1)
// VOL→GAIN: encoder 3 edits ALL (1) or SYN (0). Click toggles.
static uint8_t g_gainEnc3IsAll = 1;
static uint8_t g_gainUiFocus = 3; // 0=1-4, 1=5-8, 2=SYN, 3=ALL
// Encoder 1 (1–4) is inverted: hardware 0 → 2.0, hardware 20 → 0.0
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

// External variables
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

// Match the start paths in play(), handleStart(), and analogDirectPulse().
// Negative offsets contribute to the opposite path, so test the selected value.
static bool syncDelayHasEffect(int syncEdit, int value, bool sendsClock,
                               bool receivesTransport, bool analogExt) {
  const bool sendPath = sendsClock;
  const bool receivePath = sendsClock ? receivesTransport : !analogExt;
  const bool targetsSend = (syncEdit == 0) != (value < 0);
  return targetsSend ? sendPath : receivePath;
}

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

// SETTINGS>MUTE & DRAW-R mask: user CH1..CH16 ↔ internal mute index 0..15 (same as draw: CH1=y2→internal 1; CH16=y1→internal 0).
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

// Page definitions - each page contains one main setting + additional features
struct MenuPage {
  const char* name;
  int mainSetting;  // The main setting for this page (menuPosition from old system)
  bool hasAdditionalFeatures;  // Whether this page has extra controls (like mic gain)
  const char* additionalFeatureName;  // Name of additional feature if any
};

MenuPage menuPages[MENU_PAGES_COUNT] = {
  {"PATT", 1, false, nullptr},           // Load/Save
  {"KIT", 2, false, nullptr},           // Sample Pack
  {"SMP", 3, false, nullptr},           // Wave (ch1–8) / PSET presets (ch11/13/14)
  {"BPM", 5, false, nullptr},           // Tempo / clock / brightness
  {"VOL", 26, false, nullptr},          // VOL submenu (MAIN, GAIN, LOUT, PREV, SPKR)
  {"SETT", 19, false, nullptr},         // SETT submenu (VIEW, CTRL, DRAW, MUTE)
  {"REC", 20, false, nullptr},         // RECS submenu (INPT, MIC, L-IN, TRIG, CLR)
  {"MIDI", 21, false, nullptr},         // MIDI submenu (CHN, TRANSP)
  {"MODE", 61, false, nullptr},         // Performance modes; inherits SONG icon/color
  {"ETC", 34, false, nullptr}           // ETC submenu (INFO, SD, BATT, ADV, RSET)
};

// LOOK/SETTINGS submenu pages (SETTINGS menu uses LOOK submenu)
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

// RECS submenu pages
MenuPage recsPages[RECS_PAGES_COUNT] = {
  {"INPT", 4, true, "GAIN"},            // Recording Mode (MIC/LINE)
  {"MIC", 30, false, nullptr},          // Microphone gain
  {"HPF", 64, false, nullptr},          // library high-pass on the record path
  {"L-IN", 31, false, nullptr},         // Line input level
  {"TRIG", 11, false, nullptr},         // Fast Rec Mode (On-The-fly Recording)
  {"CLR", 12, false, nullptr}           // Rec Channel Clear
};

// MIDI submenu pages
MenuPage midiPages[MIDI_PAGES_COUNT] = {
  {"CH", 7, false, nullptr},            // MIDI Voice Select (Channel)
  {"TRAN", 8, false, nullptr},          // MIDI Transport
  {"SEND", 13, false, nullptr},         // MIDI Send (CLCK, NOTE, BOTH)
  {"RCVE", 44, false, nullptr},         // MIDI Note Receive (OFF/NOTE)
  {"FOLD", 51, false, nullptr},         // Fold incoming pitch into the visible grid (ON/OFF)
  {"SYNC", 45, false, nullptr},         // Transport delay: SNC1/SNC2 −127..+127 ms (+ = this path, − = other path)
  {"PQ-I", 59, false, nullptr},         // CLK-IN: enc1 hold-off, enc3 OFF/rate + click polarity
  {"PQ-O", 50, false, nullptr},         // CLK-OUT: enc1 width, enc2 STOP/CONT, enc3 OFF/rate + click polarity
  {"TRIG", 58, false, nullptr},         // Sidechain out: which voice pulses J27
  {"OVWR", 60, false, nullptr},         // MIDI grid write: overwrite occupied cells
};

// VOL submenu pages
MenuPage volPages[VOL_PAGES_COUNT] = {
  {"MAIN", 27, false, nullptr},         // Headphone output volume
  {"GAIN", 53, false, nullptr},         // Mix bus / synth / master trim (0.0x–2.0x, 1.0x = unity)
  {"LOUT", 28, false, nullptr},         // Line output volume
  {"PREV", 29, false, nullptr},         // Preview volume
  {"SPKR", 43, false, nullptr}           // Speaker toggle (ON/OFF)
};

// ETC submenu pages
MenuPage etcPages[ETC_PAGES_COUNT] = {
  {"INFO", 39, false, nullptr},
  {"SD", 49, false, nullptr},
  {"BATT", 42, false, nullptr},
  {"ADV", 62, false, nullptr},
  {"RSET", 16, true, "MODE"},
  {"SWAP", 67, false, nullptr}
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
  {"AUTO", 15, true, "PAGES"},
  {"DEMO", 66, false, nullptr}
};
MenuPage advancedPages[ADV_PAGES_COUNT] = {
  {"CRSR", 32, false, nullptr},
  {"LEDS", 23, false, nullptr},
  {"RAM", 52, false, nullptr},
  {"LGHT", 40, false, nullptr},
  {"COLR", 41, false, nullptr},
  {"EYES", 57, false, nullptr},
  {"FIRE", 55, false, nullptr},
  {"2-CH", 36, false, nullptr},         // Stereo routing: OFF, M+P, or L+R
  {"USBP", 65, false, nullptr},         // SMP audition (voice 0) also on USB
  {"HFC", 46, false, nullptr}           // HF cut 0..256 (step 10, MAX=256) → GEQ treble band only
};

// These groups share the existing setting renderer/control handlers. IDs and
// EEPROM offsets stay unchanged; only their navigation hierarchy changes.
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
// Returning from MODE restores the MODE root; ADV restores ETC.
int systemMenuParentPage() {
  return currentSystemMenuGroup == SystemMenuGroup::Mode ? 8 : 9;
}

// SWAP uses playable voice IDs; the existing tmp copy/paste buffer is reused.
static const uint8_t swapVoices[] = {1,2,3,4,5,6,7,8,11,13,14};
static uint8_t swapFromChoice = 0, swapToChoice = 8;
static int8_t swapOctaves[2] = {0, 0};
static bool swapHeld[2] = {false, false};
static unsigned long swapLastAt = 0;
extern void swapChannelNotes(uint8_t from, uint8_t to, int8_t fromOctave, int8_t toOctave);

// --- INFO page animation state ---
static bool infoPageFirstEnter = true;
static uint32_t infoPageEnterMs = 0;

// Force a full redraw on next menu frame (used when entering/exiting menu/submenus).
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

FLASHMEM void toggleAnalogInPolarityFromMenu() {
  extern void toggleAnalogInPolarity();
  toggleAnalogInPolarity();
  g_cinFocus = 0;
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
    w += alphabet[c - 32][0] + 1;  // char width + 1 space
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

static int etcRamMetric = 0;  // 0=AUD%, 1=REV%, 2=FREE blocks, 3=CPU%
static int lastEtcRamEnc = -1;

static int computeReverbUsagePercent() {
  // Freeverb-equipped voices: average REVERB send vs slider max.
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
      // Prefer live usage; show peak when it is clearly higher.
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
  // Layout matches other menu pages: title at y=10, value at y=3.
  drawText("INFO", 2, 10, currentMenuParentTextColor());

  const uint32_t now = millis();
  if (infoPageFirstEnter) {
    infoPageFirstEnter = false;
    infoPageEnterMs = now;
  }

  // Phase 1: show version in yellow for 2 seconds
  const uint32_t holdMs = 2000;
  if (now - infoPageEnterMs < holdMs) {
    drawText(VERSION, 2, 3, CRGB(255, 255, 0));  // yellow version number below
    return;
  }

  // Phase 2: scroll version left while scrolling in the thank-you text from the right
  static const char *kMsg =
    "   Thank you for using TOERN. Shout out to Matzesampler, Sabrina, Hairy and all others for supporting me. Jan";

  const uint32_t scrollStartMs = infoPageEnterMs + holdMs;
  const uint32_t elapsedMs = (now >= scrollStartMs) ? (now - scrollStartMs) : 0;
  const uint32_t msPerPixel = 60;  // scroll speed (lower = faster)
  const int offsetPx = (int)(elapsedMs / msPerPixel);

  const int versionBaseX = 2;
  const int versionX = versionBaseX - offsetPx;
  const int msgStartX = (int)maxX + 1;
  const int msgX = msgStartX - offsetPx;

  drawText(VERSION, versionX, 3, CRGB(255, 255, 0));  // yellow, scrolling out
  drawText(kMsg, msgX, 3, UI_WHITE);                   // white, scrolling in

  // Loop once the message has fully exited left.
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
bool menuEnteredFromSingleMode = false;  // Restore draw vs single when exiting from submenu

// After leaving the menu (or a menu child screen), keep last main page for this long;
// reopen later → jump back to FILE (page 0).
static unsigned long menuExitMs = 0;
static const unsigned long MENU_REENTRY_GRACE_MS = 5000;

FLASHMEM void noteMenuExit() {
  menuExitMs = millis();
}

// Shared enc3 min/max cache across all menu/submenu screens. Per-screen statics
// skipped writeMax when wantMax matched a prior visit, while switchMode/draw had
// already raised HW max (menu.maxValues[3]=16) → dead steps past last page.
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
  // switchMode(&menu) writes Mode.maxValues[3] (16) before showMenu runs —
  // drop cache so the active page-count max is re-applied.
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
int aiTargetPage = 6; // Default target page for AI song generation
int aiBaseStartPage = 1; // Start of base page range for AI analysis
int aiBaseEndPage = 1;   // End of base page range for AI analysis

// Genre generation variables
int genreType = 0; // 0=BLNK, 1=TECH, 2=HIPH, 3=DNB, 4=HOUS, 5=AMBT
int genreLength = 8; // Default length for genre generation

// NEW mode state management
bool newScreenFirstEnter = true;

// Reset menu option: 0 = SD, 1 = EFX, 2 = FULL, 3 = FILE, 4 = PACK, 5 = ASAV, 6 = RAM (zero EEPROM)
int resetMenuOption = 0;

// DRAW mode: 0 = L+R (default), 1 = R (right-hand only)
int drawMode = 0;

// --- Settings backup (EEPROM <-> SD) ---
// We back up the menu/settings EEPROM block to a small text file on the SD card.
// This lets you restore settings if EEPROM is empty/corrupt after a brownout.
static const char *SETTINGS_BACKUP_PATH = "settings.txt";
static const char *SETTINGS_BACKUP_TMP_PATH = "settings.tmp";
static const char *SETTINGS_BACKUP_HEADER = "TOERN_SETTINGS_V1";
static const uint16_t SETTINGS_EEPROM_BLOCK_LEN = 60; // [59]=PQ-I OFF/polarity; [58]=TRIG SENS-default migrate; [57]=USB SMP preview; [56]=REC HPF; [55]=SPKR polarity marker; [54]=MIDI WRTE
static const uint16_t EEPROM_SAMPLEPACK_ADDR = 0;
static const uint16_t EEPROM_SP0_STATE_ADDR = 200;
static const uint8_t EEPROM_SP0_STATE_COUNT = 8;
static bool settingsBackupDirty = false;
static uint32_t settingsBackupDirtyMs = 0;
static const uint32_t SETTINGS_BACKUP_DEBOUNCE_MS = 1500;
// Bits 0..3 = 1-4 / 5-8 / SYN / ALL — EEPROM write deferred while sequencer plays
// (Teensy EEPROM is flash-backed; mid-playback writes stall FlexSPI and can bus-fault
// the audio eDMA → CrashReport / HalfKay 7-blink reboot after ~8s).
static uint8_t g_pendingMixGainMask = 0;
static bool g_pendingSp0Eeprom = false;
// All other settings-block bytes deferred the same way while isNowPlaying.
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

// Large buffers off DTCM: only used from loadMenuFromEEPROM / readSettingsBackupFromSD (single-threaded).
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
  // If we read nothing and hit EOF, signal failure
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
  // NOTE: Arduino defines HEX as a macro (base 16 for Print::print), so don't use HEX as an identifier.
  static const char *HEX_CHARS = "0123456789ABCDEF";
  size_t needed = inLen * 2 + 1;
  if (outSize < needed) {
    if (outSize > 0) out[0] = 0;
    return;
  }
  for (size_t i = 0; i < inLen; i++) {
    out[i * 2]     = HEX_CHARS[(in[i] >> 4) & 0xF];
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

  // payload = samplePackID (4 bytes, 0 = SP0 fallback) + settings block + SP0 override flags
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

  // Write atomically: settings.tmp -> rename to settings.txt
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
    // Fallback: if rename fails, keep tmp as evidence
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

  // Backward compatible: allow older backups with smaller EEPROM blocks.
  const size_t minLen = sizeof(unsigned int) + 1; // at least 1 byte of settings
  const size_t maxLen = sizeof(unsigned int) + SETTINGS_EEPROM_BLOCK_LEN + EEPROM_SP0_STATE_COUNT;
  if (len < minLen || len > maxLen) { f.close(); return false; }
  if (strlen(hex) < (size_t)len * 2) { f.close(); return false; }

  uint8_t payload[sizeof(unsigned int) + SETTINGS_EEPROM_BLOCK_LEN + EEPROM_SP0_STATE_COUNT];
  if (!decodeHexBytes(hex, payload, (size_t)len)) { f.close(); return false; }
  uint32_t crc = crc32_compute(payload, (size_t)len);
  if (crc != expectedCrc) { f.close(); return false; }

  // Unpack
  memcpy(&outSamplePackID, payload, sizeof(unsigned int));
  if (outBlockLen < SETTINGS_EEPROM_BLOCK_LEN) { f.close(); return false; }
  memset(outBlock, 0, SETTINGS_EEPROM_BLOCK_LEN);
  // Added after the V1 backup format shipped: an absent CLMP byte must preserve
  // the historical pitch-folding behavior (ON), not restore as OFF.
  outBlock[37] = 1;
  // GAIN defaults applied after memcpy (see below) so short/old backups don't mute.
  if (outSp0BlockLen < EEPROM_SP0_STATE_COUNT) { f.close(); return false; }
  memset(outSp0Block, 0, EEPROM_SP0_STATE_COUNT);
  // Payload layout: samplePackID (4) + settings block (variable up to SETTINGS_EEPROM_BLOCK_LEN) + sp0 (8).
  // Older backups used a shorter block; never read SP0 bytes into the settings block when block grows.
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
    // CLMP absent in short backups
    if (blkCopy <= 37) outBlock[37] = 1;
    // Short backups predate the SPKR polarity marker. Do not re-invert a saved byte 27.
    if (blkCopy <= 55) outBlock[55] = 0xA5;
    // REC>HPF absent → library high-pass ON. VOL>USBP absent → OFF.
    if (blkCopy <= 56) outBlock[56] = 1;
    if (blkCopy <= 57) outBlock[57] = 0;
    if (blkCopy <= 59) outBlock[59] = 0;  // PQ-I enabled, positive edge
    // VOL→GAIN (38–41): absent, virgin 0xFF, or zero-padded old backups → 1.0×
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
  // Persist anything deferred while the sequencer was running.
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
    // SP0 voice flags deferred while playing (same flash/bus-fault class as mix-gain).
    if (g_pendingSp0Eeprom) {
      g_pendingSp0Eeprom = false;
      saveSp0StateToEEPROM();
    }
  }

  if (!settingsBackupDirty) return;
  // Defer SD write while sequencer is playing or pause tails are still audible.
  if (isNowPlaying || !audioQuiet) return;
  if ((uint32_t)(millis() - settingsBackupDirtyMs) < SETTINGS_BACKUP_DEBOUNCE_MS) return;
  // Best effort: if SD write fails, keep dirty flag and try later.
  if (writeSettingsBackupToSD()) {
    settingsBackupDirty = false;
  }
}

// Flush settings + SP0 backup to SD immediately (no debounce). Use after load/save paths
// that should not leave a multi-second deferred write for loop() / DRAW mode.
// While playing, leave dirty so serviceSettingsBackup flushes after stop.
FLASHMEM void flushSettingsBackupNow() {
  if (!settingsBackupDirty) return;
  extern bool isNowPlaying;
  if (isNowPlaying) return;
  if (writeSettingsBackupToSD()) {
    settingsBackupDirty = false;
  }
}

// Store the 27 HFC positions in one byte; the adjacent byte is the format.
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
    // EEPROM "empty" - try SD backup first, otherwise initialize defaults
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

      // Keep runtime in sync for the rest of boot (loadSamplePack uses samplePackID)
      extern unsigned int samplePackID;
      samplePackID = restoredPack;
      SMP.pack = restoredPack;
    } else {
      // first run! write magic + defaults
      EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
      EEPROM.put(EEPROM_SAMPLEPACK_ADDR, (unsigned int)1);            // samplePackID default (1)
      EEPROM.write(EEPROM_DATA_START + 0,  1);   // recMode default (MIC)
      EEPROM.write(EEPROM_DATA_START + 1,  1);   // clockMode default (INT)
      EEPROM.write(EEPROM_DATA_START + 2,  2);   // transportMode default (SEND)
      EEPROM.write(EEPROM_DATA_START + 3,  (uint8_t)-1);   // patternMode default (OFF)
      EEPROM.write(EEPROM_DATA_START + 4, -1);   // voiceSelect default (OFF)
      EEPROM.write(EEPROM_DATA_START + 5,  2);   // fastRecMode default (SENS)
      EEPROM.write(EEPROM_DATA_START + 58, 0xA5); // TRIG default already SENS
      EEPROM.write(EEPROM_DATA_START + 6,  0);   // recChannelClear default (OFF)
      EEPROM.write(EEPROM_DATA_START + 7,  20);   // previewVol default (0-50 range, middle = 20)
      EEPROM.write(EEPROM_DATA_START + 8, 1);   // flowMode default (ON)
      EEPROM.write(EEPROM_DATA_START + 9, 10);   // micGain default (10)
      // slot 10: PPQN packed (bit0=on, bit1=polarity-, bits2-4=ppqn idx, bit5=STOP); default OFF / + / 24 / CONT
      EEPROM.write(EEPROM_DATA_START + 10, (uint8_t)(6 << 2));
      EEPROM.write(EEPROM_DATA_START + 30, 12);  // PPQN pulse width ms
      EEPROM.write(EEPROM_DATA_START + 11, 1);   // simpleNotesView default (1 = EASY)
      EEPROM.write(EEPROM_DATA_START + 12, 0);   // loopLength default (0 = OFF)
      EEPROM.write(EEPROM_DATA_START + 13, 1);   // ledMode default (1 = one panel, normal orientation)
      EEPROM.write(EEPROM_DATA_START + 14, 1);   // ctrlMode default (1 = VOL)
      EEPROM.write(EEPROM_DATA_START + 15, 30);  // lineOutLevelSetting default (30)
      EEPROM.write(EEPROM_DATA_START + 16, 8);   // lineInLevel default (8)
      EEPROM.write(EEPROM_DATA_START + 17, 100);  // GLOB.vol default (100, maps to 1.0 volume, 0-100 range)
      EEPROM.write(EEPROM_DATA_START + 18, 0);   // cursorType default (0 = NORM)
      EEPROM.write(EEPROM_DATA_START + 19, 0);   // showChannelNr default (0 = false, NORM mode)
      EEPROM.write(EEPROM_DATA_START + 20, 0);   // previewTriggerMode default (ON)
      EEPROM.write(EEPROM_DATA_START + 21, 0);   // drawMode default (0 = L+R)
      EEPROM.write(EEPROM_DATA_START + 22, 0);   // colorScheme default (0 = default scheme)
      EEPROM.write(EEPROM_DATA_START + 23, 0);   // stereoChannel default (OFF)
      EEPROM.write(EEPROM_DATA_START + 24, 2);   // midiSendMode default (2 = BOTH)
      EEPROM.write(EEPROM_DATA_START + 25, 0);   // ledStripEnabled default (0 = OFF)
      EEPROM.write(EEPROM_DATA_START + 26, 64);  // ledBrightness default (64, range 3-255)
      EEPROM.write(EEPROM_DATA_START + 27, 1);   // spkrEnabled default (1 = ON)
      EEPROM.write(EEPROM_DATA_START + 55, 0xA5); // SPKR polarity migrate done
      EEPROM.write(EEPROM_DATA_START + 56, 1);   // REC HPF default (library high-pass ON)
      EEPROM.write(EEPROM_DATA_START + 57, 0);   // VOL USB preview default (OFF)
      EEPROM.write(EEPROM_DATA_START + 28, 1);   // midiNoteReceive default (1 = NOTE/on)
      // Transport delays: slot 29 SEND, slot 31 RCVE (1 byte each, 0-127)
      EEPROM.write(EEPROM_DATA_START + 29, 0);   // transportSendDelayMs default (0)
      EEPROM.write(EEPROM_DATA_START + 31, 0);   // transportRcveDelayMs default (0)
      EEPROM.write(EEPROM_DATA_START + 32, 0);  // codecHfCut default (0 = off)
      EEPROM.write(EEPROM_DATA_START + 33, 5);
      EEPROM.put(EEPROM_DATA_START + 34, (uint16_t)0x0006);  // internal bits 1+2 = user CH1+CH2 (y=2,3)
      EEPROM.write(EEPROM_DATA_START + 36, 0);   // childLockEnabled default (OFF)
      EEPROM.write(EEPROM_DATA_START + 37, 1);   // MIDI pitch clamp default (ON)
      EEPROM.write(EEPROM_DATA_START + 38, 10);  // mixGain14 default (unity)
      EEPROM.write(EEPROM_DATA_START + 39, 10);  // mixGain58 default (unity)
      EEPROM.write(EEPROM_DATA_START + 40, 10);  // mixGainSynth default (unity)
      EEPROM.write(EEPROM_DATA_START + 41, 10);  // mixGainMaster default (unity)
      EEPROM.write(EEPROM_DATA_START + 42, 0);   // voiceMode default (OFF)
      EEPROM.write(EEPROM_DATA_START + 43, 0);   // fireVoice default (OFF)
      EEPROM.write(EEPROM_DATA_START + 44, 8);   // fireLevel default (8 particles)
      EEPROM.write(EEPROM_DATA_START + 45, 1);   // fireSize default (1 cell)
      EEPROM.write(EEPROM_DATA_START + 46, 0);   // fireGravity default (float up)
      EEPROM.write(EEPROM_DATA_START + 47, 8);   // fireColor default (full voice colour)
      EEPROM.write(EEPROM_DATA_START + 48, 0);   // imageMode default (OFF)
      EEPROM.write(EEPROM_DATA_START + 49, 1);   // eyesMode default (1 min)
      EEPROM.write(EEPROM_DATA_START + 51, 1);   // SIDE voice default (CH1)
      EEPROM.write(EEPROM_DATA_START + 52, 0);   // EXT clock source: 0=MIDI, 1=analog PPQN
      EEPROM.write(EEPROM_DATA_START + 59, 0);   // PQ-I enabled, positive edge
      EEPROM.write(EEPROM_DATA_START + 53, 2);   // PQ-I: 4 PPQN, 1 ms hold-off
      EEPROM.write(EEPROM_DATA_START + 54, 0);   // MIDI WRTE default (0 = OFF, keep existing notes)
      for (uint8_t i = 0; i < EEPROM_SP0_STATE_COUNT; i++) {
        EEPROM.write(EEPROM_SP0_STATE_ADDR + 1 + i, 0);
      }

      // Create/refresh SD backup for defaults (debounced)
      markSettingsBackupDirty();
    }
  }

  // now pull them in with validation for corrupted EEPROM values
  recMode       = (int8_t) EEPROM.read(EEPROM_DATA_START + 0);
  if (recMode != 1 && recMode != -1) {
    recMode = 1;  // Default to MIC
    saveSingleModeToEEPROM(0, recMode);
  }
  
  clockMode     = (int8_t) EEPROM.read(EEPROM_DATA_START + 1);
  // Older BPM encoder code saved EXT as -1; keep that selection on reboot.
  if (clockMode == -1) {
    clockMode = 0;
    saveSingleModeToEEPROM(1, clockMode);
  }
  if (clockMode != 0 && clockMode != 1) {
    clockMode = 1;  // Default to INT (internal clock)
    saveSingleModeToEEPROM(1, clockMode);
  }
  
  transportMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 2);
  if (transportMode != -1 && transportMode != 1 && transportMode != 2) {
    transportMode = 2;  // Default to SEND
    saveSingleModeToEEPROM(2, transportMode);
  }
  
  patternMode   = (int8_t) EEPROM.read(EEPROM_DATA_START + 3);
  if (patternMode != -1 && patternMode != 1 && patternMode != 2 && patternMode != 3) {
    patternMode = -1;  // Default to OFF
    saveSingleModeToEEPROM(3, patternMode);
  }
  
  voiceSelect   = (int8_t) EEPROM.read(EEPROM_DATA_START + 4);
  if (voiceSelect != -1 && voiceSelect != 1 && voiceSelect != 2) {
    voiceSelect = -1;  // Default to OFF
    saveSingleModeToEEPROM(4, voiceSelect);
  }
  
  fastRecMode   = (int8_t) EEPROM.read(EEPROM_DATA_START + 5);
  {
    const uint8_t FASTREC_SENS_DEFAULT_MIGRATE = 0xA5;
    uint8_t trigMig = EEPROM.read(EEPROM_DATA_START + 58);
    if (trigMig != FASTREC_SENS_DEFAULT_MIGRATE) {
      if (fastRecMode == 1) {
        fastRecMode = 2;  // old product default INST → SENS
        saveSingleModeToEEPROM(5, 2);
      }
      EEPROM.write(EEPROM_DATA_START + 58, FASTREC_SENS_DEFAULT_MIGRATE);
    }
    if (fastRecMode > 4) {
      fastRecMode = 2;  // Default SENS
      saveSingleModeToEEPROM(5, fastRecMode);
    }
  }
  
  recChannelClear   = (int8_t) EEPROM.read(EEPROM_DATA_START + 6);
  if (recChannelClear < 0 || recChannelClear > 4) {
    recChannelClear = 0;  // Default to OFF
    saveSingleModeToEEPROM(6, recChannelClear);
  }
  
  previewVol   = (int8_t) EEPROM.read(EEPROM_DATA_START + 7);
  if (previewVol > 50) {  // 0 is a valid silent preview volume.
    previewVol = 20;  // Default to mid
    saveSingleModeToEEPROM(7, previewVol);
  }
  
  flowMode     = (int8_t) EEPROM.read(EEPROM_DATA_START + 8);
  if (flowMode != -1 && flowMode != 1) {
    flowMode = 1;  // Default to ON
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
    // One-time recovery: early IMG bring-up could leave EEPROM+48 stuck ON, which
    // hides draw-mode base colors and looked like “IMG always on”. Clear once.
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
    // VMOD is not allowed with IMG — force VMOD off if both were stored on.
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
    // 0=OFF, 1=1min, 2=5min, 3=15min (legacy BAT/ON values 0–2 still map sanely)
    if (eyesValue > 3) {
      eyesValue = 1;  // default 1 min
      EEPROM.write(EEPROM_DATA_START + 49, 1);
    }
    eyesMode = eyesValue;
  }
  
  micGain     = (int8_t) EEPROM.read(EEPROM_DATA_START + 9);
  if (micGain < 1 || micGain > 63) {
    micGain = 10;  // Default to 10
    saveSingleModeToEEPROM(9, micGain);
  }
  
  simpleNotesView = (int) EEPROM.read(EEPROM_DATA_START + 11);
  if (simpleNotesView < 1 || simpleNotesView > 2) {
    simpleNotesView = 1;  // Default to EASY
    saveSingleModeToEEPROM(11, simpleNotesView);
  }
  
  loopLength = (int) EEPROM.read(EEPROM_DATA_START + 12);
  if (loopLength < 0 || loopLength > 8) {
    loopLength = 0;  // Default to OFF
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
    ctrlMode = 1;  // Default to VOL
    saveSingleModeToEEPROM(14, ctrlMode);
  }
  
  // Load lineInLevel from EEPROM (stored at EEPROM_DATA_START + 16)
  extern unsigned int lineInLevel;
  lineInLevel = (unsigned int)EEPROM.read(EEPROM_DATA_START + 16);
  if (lineInLevel > 15) {
    lineInLevel = 8;  // Default to 8 if invalid
    saveSingleModeToEEPROM(16, lineInLevel);
  }
  
  // Slot 18 used to store BIG cursor. That mode is gone; keep the byte at 0.
  if (EEPROM.read(EEPROM_DATA_START + 18) != 0) {
    saveSingleModeToEEPROM(18, 0);
  }
  
  // Load showChannelNr from EEPROM (stored at EEPROM_DATA_START + 19)
  extern bool showChannelNr;
  showChannelNr = (EEPROM.read(EEPROM_DATA_START + 19) != 0);
  
  // Load drawMode from EEPROM (stored at EEPROM_DATA_START + 21)
  drawMode = (int8_t)EEPROM.read(EEPROM_DATA_START + 21);
  if (drawMode < 0 || drawMode > 1) {
    drawMode = 0;  // Default to 0 (L+R) if invalid
    saveSingleModeToEEPROM(21, drawMode);
  }

  // Load stereoChannel from EEPROM (stored at EEPROM_DATA_START + 23)
  // 0=OFF, 1=M+P, 2=L+R
  // Convert old values for backward compatibility (conversion happens in menu first enter)
  extern int8_t stereoChannel;
  stereoChannel = (int8_t)EEPROM.read(EEPROM_DATA_START + 23);
  if (stereoChannel < 0 || stereoChannel > 8) {
    // Invalid value, default to OFF
    stereoChannel = 0;
    saveSingleModeToEEPROM(23, stereoChannel);
  }
  // Note: Values 1-8 will be converted in menu first enter (old 1->2, old 2-8->2)
  // Apply routing after loading from EEPROM
  extern void applyStereoChannelRouting();
  applyStereoChannelRouting();
  
  // Load midiSendMode from EEPROM (stored at EEPROM_DATA_START + 24)
  // 0=CLCK, 1=NOTE, 2=BOTH
  extern int midiSendMode;
  midiSendMode = (int8_t)EEPROM.read(EEPROM_DATA_START + 24);
  if (midiSendMode < 0 || midiSendMode > 2) {
    // Invalid value, default to BOTH
    midiSendMode = 2;
    saveSingleModeToEEPROM(24, midiSendMode);
  }
  
  // Load ledStripEnabled from EEPROM (stored at EEPROM_DATA_START + 25)
  extern bool getLedStripEnabled();
  extern void setLedStripEnabled(bool enabled);
  uint8_t ledStripValue = EEPROM.read(EEPROM_DATA_START + 25);
  bool ledStripEnabled = (ledStripValue != 0);
  if (ledStripValue > 1) {
    // Invalid value, default to OFF
    ledStripEnabled = false;
    saveSingleModeToEEPROM(25, 0);
  }
  setLedStripEnabled(ledStripEnabled);

  // Load spkrEnabled from EEPROM (stored at EEPROM_DATA_START + 27).
  // Offset 55 marks the PAM8403 polarity fix: before that, 1 meant UI/physical OFF.
  extern bool getSpkrEnabled();
  extern void setSpkrEnabled(bool enabled);
  const uint8_t SPKR_POLARITY_MIGRATE = 0xA5;
  uint8_t spkrValue = EEPROM.read(EEPROM_DATA_START + 27);
  uint8_t spkrMig = EEPROM.read(EEPROM_DATA_START + 55);
  bool spkrOn;
  if (spkrMig != SPKR_POLARITY_MIGRATE) {
    if (spkrValue > 1) spkrOn = true;
    else spkrOn = (spkrValue == 0);  // invert legacy sense once
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

  // REC>HPF (56): library high-pass on the capture path. Absent/invalid → ON.
  // VOL>USBP (57): SMP voice 0 also on USB. Absent/invalid → OFF.
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

  // Load childLockEnabled from EEPROM (stored at EEPROM_DATA_START + 36)
  extern bool getChildLockEnabled();
  extern void setChildLockEnabled(bool enabled);
  uint8_t childLockValue = EEPROM.read(EEPROM_DATA_START + 36);
  bool childLockEnabled = (childLockValue != 0);
  if (childLockValue > 1) {
    // Invalid value, default to OFF
    childLockEnabled = false;
    saveSingleModeToEEPROM(36, 0);
  }
  setChildLockEnabled(childLockEnabled);

  // Load midiNoteReceive from EEPROM (slot 28)
  extern bool MIDI_NOTE_RECEIVE;
  uint8_t rcveValue = EEPROM.read(EEPROM_DATA_START + 28);
  if (rcveValue > 1) { rcveValue = 1; saveSingleModeToEEPROM(28, 1); }
  MIDI_NOTE_RECEIVE = (rcveValue != 0);

  // Load MIDI pitch clamp (slot 37). Existing EEPROM is 0xFF here, which
  // migrates to ON so firmware behavior does not change after upgrading.
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

  // VOL→GAIN (slots 38–41). Missing EEPROM / virgin 0xFF / >20 → 1.0× (10).
  // All-zero (pre-GAIN firmware or zero-padded backup) also → 1.0× for every bus.
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

  // Load transport delay settings (slot 29 SNC1, slot 31 SNC2, int8_t -127..+127, EEPROM stores raw byte)
  extern int8_t transportSendDelayMs;
  extern int8_t transportRcveDelayMs;
  uint8_t raw29 = EEPROM.read(EEPROM_DATA_START + 29);
  uint8_t raw31 = EEPROM.read(EEPROM_DATA_START + 31);
  // Migration: old layout had slot 31=0xFF, 29=SEND, 30=RCVE (unsigned 0-127)
  if (raw31 == 0xFF) {
    uint8_t raw30 = EEPROM.read(EEPROM_DATA_START + 30);
    int8_t v29 = (raw29 <= 127) ? (int8_t)raw29 : 17;
    int8_t v30 = (raw30 <= 127) ? (int8_t)raw30 : 17;
    transportSendDelayMs = (int8_t)constrain((int)v29, -127, 127);
    transportRcveDelayMs = (int8_t)constrain((int)v30, -127, 127);
    EEPROM.write(EEPROM_DATA_START + 29, (uint8_t)transportSendDelayMs);
    EEPROM.write(EEPROM_DATA_START + 31, (uint8_t)transportRcveDelayMs);
    EEPROM.write(EEPROM_DATA_START + 30, 12);  // slot 30 reused as PPQN pulse width ms
  } else {
    transportSendDelayMs = (int8_t)constrain((int)(int8_t)raw29, -127, 127);
    transportRcveDelayMs = (int8_t)constrain((int)(int8_t)raw31, -127, 127);
  }

  // Load PPQN pulse settings after delay migration so slot 30 is pulse width, not leftover RCVE
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
      // Old format overlapped the high byte with its format marker.
      // Recover nonzero low-byte values; zero is ambiguous (old default MAX).
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
  
  // Safety: Ensure monitoring is OFF on startup to prevent feedback
  mixer_end.gain(3, 0.0);
  
  // Update maxX based on loaded ledModules value
  extern unsigned int maxX;
  maxX = MATRIX_WIDTH * ledModules;
  
  if (recChannelClear < 0 || recChannelClear > 4) {
    recChannelClear = 0;  // Default to OFF if invalid
    saveSingleModeToEEPROM(6, recChannelClear);
  }
  
  // Ensure flowMode is valid (-1 or 1)
  if (flowMode != -1 && flowMode != 1) {
    flowMode = 1;  // Default to ON if invalid value
    saveSingleModeToEEPROM(8, flowMode);
  }
  
  // Ensure simpleNotesView is valid (1-2)
  if (simpleNotesView < 1 || simpleNotesView > 2) {
    simpleNotesView = 1;  // Default to EASY if invalid value
    saveSingleModeToEEPROM(11, simpleNotesView);
  }
  
  if (transportMode != -1 && transportMode != 1 && transportMode != 2) {
    transportMode = -1;  // Default to OFF if invalid value
    saveSingleModeToEEPROM(2, transportMode);
  }

  // Ensure loopLength is valid (0-8)
  if (loopLength < 0 || loopLength > 8) {
    loopLength = 0;  // Default to OFF if invalid value
    saveSingleModeToEEPROM(12, loopLength);
  }
  
  // Ensure voiceSelect is valid (-1, 1, or 2)
  if (voiceSelect != -1 && voiceSelect != 1 && voiceSelect != 2) {
    voiceSelect = 1;  // Default to MIDI if invalid value
    saveSingleModeToEEPROM(4, voiceSelect);
  }
  
  // Ensure patternMode is valid (-1, 1, or 2)
  if (patternMode != -1 && patternMode != 1 && patternMode != 2 && patternMode != 3) {
    patternMode = -1;  // Default to OFF if invalid value
    saveSingleModeToEEPROM(3, patternMode);
  }
  
  // Ensure led mode is valid (1,2,3,4 => 1,2,1B,2B)
  if (ledMode < 1 || ledMode > 4) {
    ledMode = 1;     // Default to 1 if invalid value
    ledModules = 1;
    ledModulesRotated = false;
    saveSingleModeToEEPROM(13, 1);
  }

  if (ctrlMode != 0 && ctrlMode != 1) {
    ctrlMode = 1;
    saveSingleModeToEEPROM(14, ctrlMode);
  }

  // Set global flags
  SMP_PATTERN_MODE       = (patternMode   == 1 || patternMode == 2 || patternMode == 3);  // ON, SONG, or NEXT mode
  SMP_FLOW_MODE          = (flowMode      == 1);
  MIDI_VOICE_SELECT      = (voiceSelect   == 1 || voiceSelect == 2);  // MIDI or KEYS mode
  MIDI_TRANSPORT_RECEIVE = (transportMode == 1);
  MIDI_TRANSPORT_SEND = (transportMode == 2);
  SMP_FAST_REC           = fastRecMode;
  SMP_REC_CHANNEL_CLEAR  = (recChannelClear == 1);  // Only true for ON mode
  
  // Set song mode active flag
  extern bool songModeActive;
  songModeActive = (patternMode == 2);
  
  // Set audio input based on recMode
  recInput = (recMode == 1) ? AUDIO_INPUT_MIC : AUDIO_INPUT_LINEIN;
  
  // Set MIDI clock send based on clockMode
  MIDI_CLOCK_SEND = (clockMode == 1);
  
  // Set MIDI send flags based on midiSendMode (0=CLCK, 1=NOTE, 2=BOTH)
  // Note: MIDI_CLOCK_SEND is already set above, but we need to respect midiSendMode
  extern int midiSendMode;
  extern bool MIDI_NOTE_SEND;
  if (midiSendMode == 0) {
    // CLCK only: keep MIDI_CLOCK_SEND as set by clockMode, disable notes
    MIDI_NOTE_SEND = false;
  } else if (midiSendMode == 1) {
    // NOTE only: disable clock, enable notes
    MIDI_CLOCK_SEND = false;
    MIDI_NOTE_SEND = true;
  } else {
    // BOTH: keep MIDI_CLOCK_SEND as set by clockMode, enable notes
    MIDI_NOTE_SEND = true;
  }
  
  // Codec I2C is applied later by initSoundChip() / applyAudioSettingsFromGlobals().
  // Writing the SGTL5000 here runs before enable() on the boot path and wedges
  // the bus on the first start after a flash.

  // Note: These draw functions are for menu display, not startup initialization.
  // They should only be called when actually displaying the menu, not during setup().
  // The initialization logic (loading values from EEPROM) is complete above.
  // Drawing during startup causes brief text flashes (e.g., "MIC" before "KIT:").
  // drawRecMode();
  // drawMidiTransport();
  // drawPatternMode();
  // drawFlowMode();
  // drawMidiVoiceSelect();
  // drawFastRecMode();
  // drawRecChannelClear();
}

// Apply all audio-related global variables to hardware
FLASHMEM void applyAudioSettingsFromGlobals() {
  // GLOB, lineOutLevelSetting, previewVol, micGain, and lineInLevel are already in scope as globals
  extern struct GlobalVars GLOB;
  extern uint8_t lineOutLevelSetting;
  extern unsigned int micGain;
  extern unsigned int lineInLevel;
  
  // Apply main volume (headphone output)
  // Handle both legacy (0-10) and new (0-100) ranges
  float vol;
  if (GLOB.vol <= 10) {
    // Legacy value (0-10): convert to 0.0-1.0
    vol = GLOB.vol / 10.0f;
  } else {
    // New value (0-100): convert to 0.0-1.0
    vol = GLOB.vol / 100.0f;
  }
  vol = constrain(vol, 0.0f, 1.0f);
  sgtl5000_1.volume(vol);
  
  // Apply line output level
  sgtl5000_1.lineOutLevel(lineOutLevelSetting);
  
  // Apply mic gain only when MIC is selected; otherwise force 0 to keep MIC path off.
  extern unsigned int recInput;
  if (recInput == AUDIO_INPUT_MIC) {
    sgtl5000_1.micGain(micGain);
  } else {
    sgtl5000_1.micGain(0);
  }
  
  // Apply line input level
  sgtl5000_1.lineInLevel(lineInLevel);

  extern void applySgtl5000AdcHighPass();
  applySgtl5000AdcHighPass();
  extern void applySgtl5000CodecOutputPath();
  applySgtl5000CodecOutputPath();
  
  // Apply preview volume
  updatePreviewVolume();

  extern void applyMixBusGains();
  applyMixBusGains();
}

// call this after you change *any* one of the six modes in switchMenu():
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

// Live-apply mix gain; defer flash EEPROM while playing (avoids mid-playback stalls).
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

// uint16 settings (draw mute mask). HFC uses a separate step byte and format byte.
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

// Save samplepack 0 state to EEPROM (which voices are using sp0)
FLASHMEM void saveSp0StateToEEPROM() {
  extern bool isNowPlaying;
  // Mid-playback flash writes stall the same way mix-gain did — defer until stopped.
  if (isNowPlaying) {
    g_pendingSp0Eeprom = true;
    markSettingsBackupDirty();
    return;
  }
  // Use addresses 200-208 for sp0 state (8 voices = 8 bytes)
  for (int i = 1; i < maxFiles; i++) {
    uint8_t value = SMP.sp0Active[i] ? 1 : 0;
    EEPROM.write(EEPROM_SP0_STATE_ADDR + i, value);
  }
  markSettingsBackupDirty();
}

// Load samplepack 0 state from EEPROM
FLASHMEM void loadSp0StateFromEEPROM() {
  for (int i = 1; i < maxFiles; i++) {
    uint8_t stored = EEPROM.read(EEPROM_SP0_STATE_ADDR + i);
    SMP.sp0Active[i] = (stored == 1);
  }
}

FLASHMEM void showMenu() {
  // Only redraw the menu UI when the page/setting changes (or a value changed).
  // This avoids unnecessary clears/shows each loop.

  // Handle page navigation with encoder 4
  static int lastPagePosition = -1;
  static bool menuFirstEnter = true;
  
  // Reset tracking when returning from submenu (currentMenuPage doesn't match mode pos)
  if (currentMenuPage != currentMode->pos[3] && lastPagePosition != currentMenuPage) {
    lastPagePosition = -1;
    menuFirstEnter = true;
    currentMode->pos[3] = currentMenuPage;  // Sync mode position with current page
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

  // Current page/setting
  const int pageIndex = currentMenuPage;
  MenuPage* currentPageInfo = &menuPages[pageIndex];
  const int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedPageIndex = -1;
  static int lastRenderedSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedPageIndex) || (mainSetting != lastRenderedSetting);

  if (fullRedraw) {
    FastLEDclear();
    // New indicator system: menu: | | | L[G]
    drawMenuIndicator('L', 'G', 4);  // Encoder 4: menu navigation

    // Page indicator line at y=maxY
    int startX = (int)maxX - MENU_PAGES_COUNT + 1;
    if (startX < 1) startX = 1;
    for (int i = 0; i < MENU_PAGES_COUNT; i++) {
      CRGB indicatorColor = (i == pageIndex) ? UI_RED : UI_BLUE;
      light(startX + i, maxY, indicatorColor);
    }

    // Encoder RGB defaults (only when page changes)
    if (mainSetting == 15) {
      // AI page: multiple indicators - L[G], L[Y], L[W], L[X]
      CRGB greenColor = getIndicatorColor('G');
      CRGB yellowColor = getIndicatorColor('Y');
      CRGB whiteColor = getIndicatorColor('W');
      CRGB blueColor = getIndicatorColor('X');
      Encoder[0].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
      Encoder[1].writeRGBCode(yellowColor.r << 16 | yellowColor.g << 8 | yellowColor.b);
      Encoder[2].writeRGBCode(whiteColor.r << 16 | whiteColor.g << 8 | whiteColor.b);
      Encoder[3].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
    } else {
      // Keep encoder 4 aligned with the visible menu-navigation indicator color.
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

  // Handle encoder changes for pages with adjustable values.
  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (fullRedraw || didRedraw) {
    // FastLED.setBrightness(ledBrightness);
    FastLEDshow();
  }
}

FLASHMEM void showLookMenu() {
  static uint32_t lastPosHash = 0;
  // Redraw only when page/setting changes (or a value changed).
  int pageIndex = currentLookPage;
  MenuPage* currentPageInfo = &lookPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedLookPage = -1;
  static int lastRenderedLookSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedLookPage) || (mainSetting != lastRenderedLookSetting);

  if (fullRedraw) {
    FastLEDclear();

  // Submenu indicator (encoder 4) should always match submenu text color.
  drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);

  // Get current page info
  // (pageIndex/currentPageInfo already computed above)
  
  // Draw page indicator as a line at y=maxY (right-aligned)
  // Match the parent menu color (SETTINGS).
  const CRGB parent = currentMenuParentTextColor();
  const CRGB parentDim = dimIconColorFromText(parent);
  int startX = (int)maxX - LOOK_PAGES_COUNT + 1;
  if (startX < 1) startX = 1;
  for (int i = 0; i < LOOK_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
    light(startX + i, maxY, indicatorColor);
  }

  // Handle the main setting for this page
  // (mainSetting already computed above)
  
  // SETTINGS submenu: encoder 2 = value on toggle pages
  CRGB indicatorColor = currentMenuParentTextColor();
  const bool playValuePage = (mainSetting == 9 || mainSetting == 10 || mainSetting == 17 ||
      mainSetting == 18 || mainSetting == 23 || mainSetting == 24 || mainSetting == 25 ||
      mainSetting == 32 || mainSetting == 33 || mainSetting == 38 || mainSetting == 47 ||
      mainSetting == 54);
  Encoder[0].writeRGBCode(0x000000);
  Encoder[1].writeRGBCode(0x000000);
  Encoder[2].writeRGBCode(playValuePage ? (indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
  Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);

  // Draw the main setting status
  drawMainSettingStatus(mainSetting);
  
  // Draw additional features if this page has them
  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }

  // FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  lastRenderedLookPage = pageIndex;
  lastRenderedLookSetting = mainSetting;
  }

  // Handle page navigation with encoder 3
  static int lastLookPagePosition = -1;
  static bool lookMenuFirstEnter = true;
  
  // Reset tracking when entering submenu (currentLookPage is 0 and mode pos doesn't match)
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
  
  // Handle encoder changes for pages with adjustable values.
  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {
    // FastLED.setBrightness(ledBrightness);
    FastLEDshow();
  }

  // Mark render dirty on any encoder movement while in submenu.
  // Skip if controls already redrew this frame (avoids re-I2C RGB on every tick).
  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    if (!didRedraw) menuRequestFullRedraw();
    lastPosHash = h;
  }
}

FLASHMEM void showRecsMenu() {
  static uint32_t lastPosHash = 0;
  // Redraw only when page/setting changes (or a value changed).
  int pageIndex = currentRecsPage;
  MenuPage* currentPageInfo = &recsPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedRecsPage = -1;
  static int lastRenderedRecsSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedRecsPage) || (mainSetting != lastRenderedRecsSetting);

  if (fullRedraw) {
    FastLEDclear();

  // Submenu indicator (encoder 4) should always match submenu text color.
  drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);

  // Get current page info
  // (pageIndex/currentPageInfo already computed above)
  
  // Show header only on MIC/LINE toggle page
  if (currentPageInfo->mainSetting == 4) {
    drawText("INPT", 2, 10, menuTextColorFromCol(7));
  }
  
  // Draw page indicator as a line at y=maxY (right-aligned)
  // Match the parent menu color (RECS).
  const CRGB parent = currentMenuParentTextColor();
  const CRGB parentDim = dimIconColorFromText(parent);
  int startX = (int)maxX - RECS_PAGES_COUNT + 1;
  if (startX < 1) startX = 1;
  for (int i = 0; i < RECS_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
    light(startX + i, maxY, indicatorColor);
  }

  // Handle the main setting for this page
  // (mainSetting already computed above)
  
  // Set encoder colors based on main setting
  if (mainSetting == 11 || mainSetting == 4 || mainSetting == 12 || mainSetting == 64) {
    // TRIG, INPT, CLR: encoder 2 = value, encoder 3 = page nav
    CRGB indicatorColor = currentMenuParentTextColor();
    Encoder[0].writeRGBCode(0x000000);
    Encoder[1].writeRGBCode(0x000000);
    Encoder[2].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  } else {
    // Default: encoder 4 ring matches submenu text color
    CRGB indicatorColor = currentMenuParentTextColor();
    Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  }

  // Draw the main setting status
  drawMainSettingStatus(mainSetting);
  
  // Draw additional features if this page has them
  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }

  // FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  lastRenderedRecsPage = pageIndex;
  lastRenderedRecsSetting = mainSetting;
  }

  // Handle page navigation with encoder 3
  static int lastRecsPagePosition = -1;
  static bool recsMenuFirstEnter = true;
  
  // Reset tracking when entering submenu (currentRecsPage is 0 and mode pos doesn't match)
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
  
  // Handle encoder changes for pages with adjustable values.
  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {
    // FastLED.setBrightness(ledBrightness);
    FastLEDshow();
  }

  // Mark render dirty on any encoder movement while in submenu.
  // Skip if controls already redrew this frame (avoids re-I2C RGB on every tick).
  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    if (!didRedraw) menuRequestFullRedraw();
    lastPosHash = h;
  }
}

FLASHMEM void showMidiMenu() {
  static uint32_t lastPosHash = 0;
  // Redraw only when page/setting changes (or a value changed).
  int pageIndex = currentMidiPage;
  MenuPage* currentPageInfo = &midiPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedMidiPage = -1;
  static int lastRenderedMidiSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedMidiPage) || (mainSetting != lastRenderedMidiSetting);

  if (fullRedraw) {
    FastLEDclear();

  // Submenu page-nav indicator (encoder 4)
  drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);

  // Get current page info
  // (pageIndex/currentPageInfo already computed above)
  
  // Draw page indicator as a line at y=maxY (right-aligned)
  // Match the parent menu color (MIDI).
  const CRGB parent = currentMenuParentTextColor();
  const CRGB parentDim = dimIconColorFromText(parent);
  int startX = (int)maxX - MIDI_PAGES_COUNT + 1;
  if (startX < 1) startX = 1;
  for (int i = 0; i < MIDI_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
    light(startX + i, maxY, indicatorColor);
  }

  // Handle the main setting for this page
  // (mainSetting already computed above)
  
  // MIDI: encoder 2 = value on CH/TRAN/SEND/RCVE/CLMP/SIDE; enc1+2 = SYNC;
  // PPQN: enc1=width, enc2=STOP/CONT, enc3=OFF/rate (click=POL); enc4 = page nav
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
    extern bool getAnalogInPolarityPositive();
    Encoder[0].writeRGBCode(0xFFFFFF);
    Encoder[1].writeRGBCode(0x000000);
    Encoder[2].writeRGBCode(getAnalogInPolarityPositive() ? 0x00FF00 : 0xFF0000);
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  } else {
    Encoder[1].writeRGBCode(mainSetting == 45 ? (indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
    Encoder[2].writeRGBCode(midiValuePage ? (indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
    Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
  }

  // Draw the main setting status
  drawMainSettingStatus(mainSetting);
  
  // Draw additional features if this page has them
  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }

  // FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  lastRenderedMidiPage = pageIndex;
  lastRenderedMidiSetting = mainSetting;
  }

  // Handle page navigation with encoder 4
  static int lastMidiPagePosition = -1;
  static bool midiMenuFirstEnter = true;
  
  // Reset tracking when entering submenu (currentMidiPage is 0 and mode pos doesn't match)
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
  
  // Handle encoder changes for pages with adjustable values.
  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {
    // FastLED.setBrightness(ledBrightness);
    FastLEDshow();
  }

  // Mark render dirty on any encoder movement while in submenu.
  // Skip if controls already redrew this frame (avoids re-I2C RGB on every tick).
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
  // Redraw only when page/setting changes (or a value changed).
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

  // VOL submenu: page-nav indicator should always match VOL text color (e.g. "MAIN")
  drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);

  // Get current page info
  // (pageIndex/currentPageInfo already computed above)
  
  // Draw page indicator as a line at y=maxY (right-aligned)
  // Match the parent menu color (VOL).
  const CRGB parent = currentMenuParentTextColor();
  const CRGB parentDim = dimIconColorFromText(parent);
  int startX = (int)maxX - VOL_PAGES_COUNT + 1;
  if (startX < 1) startX = 1;
  for (int i = 0; i < VOL_PAGES_COUNT; i++) {
    CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
    light(startX + i, maxY, indicatorColor);
  }

  // Handle the main setting for this page
  // (mainSetting already computed above)
  
  // VOL: encoder 3 = value on SPKR; GAIN uses enc1–3
  CRGB indicatorColor = currentMenuParentTextColor();
  const bool volGainPage = (mainSetting == 53);
  const bool volValuePage = (mainSetting == 43);
  Encoder[0].writeRGBCode(volGainPage ? 0x00C8C8 : 0x000000); // 1-4 cyan
  Encoder[1].writeRGBCode(volGainPage ? 0xC86400 : 0x000000); // 5-8 orange
  if (volGainPage) {
    Encoder[2].writeRGBCode(g_gainEnc3IsAll ? 0x00FF00 : 0xC828C8); // ALL green / SYN magenta
  } else {
    Encoder[2].writeRGBCode(volValuePage ? (indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b) : 0x000000);
  }
  Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);

  // Draw the main setting status
  drawMainSettingStatus(mainSetting);
  
  // Draw additional features if this page has them
  if (currentPageInfo->hasAdditionalFeatures) {
    drawAdditionalFeatures(mainSetting);
  }

  // FastLED.setBrightness(ledBrightness);
  FastLEDshow();

  lastRenderedVolPage = pageIndex;
  lastRenderedVolSetting = mainSetting;
  }

  // Handle page navigation with encoder 3
  static int lastVolPagePosition = -1;
  static bool volMenuFirstEnter = true;
  
  // Reset tracking when entering submenu (currentVolPage is 0 and mode pos doesn't match)
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
  
  // Handle encoder changes for pages with adjustable values.
  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {
    // FastLED.setBrightness(ledBrightness);
    FastLEDshow();
  }

  // Mark render dirty on any encoder movement while in submenu.
  // Skip if controls already redrew this frame (GAIN used to double-redraw + re-I2C).
  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    if (!didRedraw) menuRequestFullRedraw();
    lastPosHash = h;
  }
}

FLASHMEM void showEtcMenu() {
  static uint32_t lastPosHash = 0;
  // Redraw only when page/setting changes, except INFO which animates continuously.
  const int pageCount = activeSystemPageCount();
  int pageIndex = constrain(currentEtcPage, 0, pageCount - 1);
  MenuPage* currentPageInfo = &activeSystemPages()[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedEtcPage = -1;
  static int lastRenderedEtcSetting = -1;
  bool fullRedraw = (pageIndex != lastRenderedEtcPage) || (mainSetting != lastRenderedEtcSetting);
  if (takeMenuForceFullRedraw()) fullRedraw = true;
  if (mainSetting == 39) fullRedraw = true;  // INFO animates
  // RAM (52): live audio/reverb/free dashboard
  if (mainSetting == 52) {
    static elapsedMillis ramMenuRedraw;
    if (ramMenuRedraw >= 200) {
      ramMenuRedraw = 0;
      fullRedraw = true;
    }
  }
  if (mainSetting == 49) {
    // SCON: refresh when the host link flips between WAIT and OK
    extern bool sdSerialServerClientConnected();
    static bool lastSdClient = false;
    bool nowClient = sdSerialServerClientConnected();
    if (nowClient != lastSdClient) {
      lastSdClient = nowClient;
      fullRedraw = true;
    }
  }
  // BATT (42): redraw every 200 ms for live updates
  if (mainSetting == 42) {
    static elapsedMillis battMenuRedraw;
    if (battMenuRedraw >= 200) {
      battMenuRedraw = 0;
      fullRedraw = true;
    }
  }

  if (fullRedraw) {
    FastLEDclear();

    // Draw page indicator as a line at y=maxY (right-aligned)
    // Match the parent menu color (ETC).
    const CRGB parent = currentMenuParentTextColor();
    const CRGB parentDim = dimIconColorFromText(parent);
    int startX = (int)maxX - pageCount + 1;
    if (startX < 1) startX = 1;
    for (int i = 0; i < pageCount; i++) {
      CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
      light(startX + i, maxY, indicatorColor);
    }

    // Indicators / encoder ring colors
    // For AUTO (mainSetting 15): encoder(0) is the green "generate" trigger; no blue exit indicator on encoder(3).
    // Otherwise keep ETC parent-colored encoder(3) ring (page nav).
    // Cache RGB: INFO/RAM/BATT force fullRedraw periodically — repeating identical
    // Duppa writes every frame stalls while playing.
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
      drawMenuIndicator('L', 'G', 1, false, false);  // Encoder(0): Large Green (generate)
      // Keep encoder(1)/(2) indicators from AI page itself (drawMainSettingStatus/drawAdditionalFeatures)
      setEtcRGB(0, 0x00FF00);
      CRGB y = getIndicatorColor('Y');
      CRGB w = getIndicatorColor('W');
      setEtcRGB(1, (uint32_t)y.r << 16 | (uint32_t)y.g << 8 | y.b);
      setEtcRGB(2, (uint32_t)w.r << 16 | (uint32_t)w.g << 8 | w.b);
      // Page-nav indicator (encoder 4) should always match ETC text color (e.g. "RSET")
      {
        CRGB c = currentMenuParentTextColor();
        drawLargeIndicatorCustom(c, 4);
        // drawLargeIndicatorCustom writes RGB — re-cache so later frames skip
        lastEtcRGB[3] = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
      }
    } else {
      // ETC submenu: encoder 3 = value on RSET(16), LGHT(40), COLR(41), CHLD(48), RAM(52), FIRE(55), IMG(56), EYES(57)
      CRGB indicatorColor = currentMenuParentTextColor();
      const bool etcValuePage =
          (mainSetting == 9 || mainSetting == 10 || mainSetting == 18 || mainSetting == 23 ||
           mainSetting == 24 || mainSetting == 32 || mainSetting == 36 || mainSetting == 46 || mainSetting == 54 || mainSetting == 63 || mainSetting == 16 || mainSetting == 40 || mainSetting == 41 || mainSetting == 48 ||
           mainSetting == 52 || mainSetting == 55 || mainSetting == 56 ||
           mainSetting == 57 || mainSetting == 65);
      // Matrix chrome without I2C; rings via cache
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

    // FastLED.setBrightness(ledBrightness);
    FastLEDshow();

    lastRenderedEtcPage = pageIndex;
    lastRenderedEtcSetting = mainSetting;
  }

  // Handle page navigation with encoder 3
  static int lastEtcPagePosition = -1;
  static bool etcMenuFirstEnter = true;
  
  // Reset tracking when entering submenu (currentEtcPage is 0 and mode pos doesn't match)
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
    // FastLED.setBrightness(ledBrightness);
    FastLEDshow();
  }

  // Mark render dirty on any encoder movement while in submenu (except INFO which animates).
  // Skip if controls already redrew this frame (avoids re-I2C RGB on every tick).
  if (mainSetting != 39) {
    uint32_t h = hashEncoderPositions(currentMode);
    if (h != lastPosHash) {
      if (!didRedraw) menuRequestFullRedraw();
      lastPosHash = h;
    }
  }
}

static inline CRGB dimIconColorFromText(CRGB textColor) {
  // Match the existing UI dim level (~20) regardless of the base text color.
  CRGB c = textColor;
  uint8_t maxv = max(c.r, max(c.g, c.b));
  if (maxv == 0) return c;
  const uint8_t target = 20; // matches UI_DIM_* constants in colors.h
  uint16_t scale = (uint16_t)target * 255u / maxv;
  if (scale > 255) scale = 255;
  c.nscale8_video((uint8_t)scale);
  return c;
}

static inline CRGB menuTextColorFromCol(uint8_t colIndex) {
  // Use palette colors from `col[]` (voice colors), but normalize brightness
  // to match the rest of the UI text (~120 peak channel).
  extern CRGB col[];
  CRGB c = col[colIndex];
  uint8_t maxv = max(c.r, max(c.g, c.b));
  if (maxv == 0) return c;
  const uint8_t target = 120; // similar to UI_* brightness
  uint16_t scale = (uint16_t)target * 255u / maxv;
  if (scale > 255) scale = 255;
  c.nscale8_video((uint8_t)scale);
  return c;
}

static inline CRGB currentMenuParentTextColor() {
  // Submenus should match their main-menu parent item color:
  // SETT=col[6], RECS=col[7], MIDI=col[8], VOL=col[5], ETC=col[14]
  if (inLookSubmenu) return menuTextColorFromCol(6);
  if (inRecsSubmenu) return menuTextColorFromCol(7);
  if (inMidiSubmenu) return menuTextColorFromCol(8);
  if (inVolSubmenu)  return menuTextColorFromCol(5);
  if (inEtcSubmenu)  return menuTextColorFromCol(currentSystemMenuGroup == SystemMenuGroup::Mode ? 13 : 14);
  return UI_WHITE;
}

// Draw a "large" top-row indicator and set encoder ring to an arbitrary color.
// This is used where we want the indicator color to match submenu text colors (palette-derived),
// not one of the fixed indicator color codes.
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
  // Simple FNV-1a over the 4 encoder positions to detect any knob changes.
  uint32_t h = 2166136261u;
  for (int i = 0; i < NUM_ENCODERS; i++) {
    h ^= (uint32_t)m->pos[i];
    h *= 16777619u;
  }
  return h;
}

FLASHMEM void drawMainSettingStatus(int setting) {
  switch (setting) {
    case 1: // DAT - Load/Save
      // FILE = folder
      {
        const CRGB tc = menuTextColorFromCol(1);
        showIcons(ICON_FOLDER_BIG, dimIconColorFromText(tc));
        drawMenuText("PATT", 2, 3, tc);
      }
      break;
      
    case 2: // KIT - Sample Pack
      // Pack = pack
      {
        const CRGB tc = menuTextColorFromCol(2);
        showIcons(OLD_ICON_SAMPLEPACK, dimIconColorFromText(tc));
        drawMenuText("KIT", 2, 3, tc);
      }
      break;
      
    case 3: // WAV / PSET — samples on ch1–8, filter presets on ch11/13/14
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
      
    case 4: // REC - Recording Mode (menu/mic) - encoder 3: MIC/LINE
      drawMenuText("INPT", 2, 10, currentMenuParentTextColor());
      drawRecMode();
      drawMenuIndicator('L', recMode == 1 ? 'W' : 'X', 3);  // White=MIC, Blue=LINE
      break;
      
    case 5: // BPM - tempo / clock / brightness
      // BPM = Clock
      {
        const CRGB tc = menuTextColorFromCol(4);
        showIcons(ICON_CLOCK, dimIconColorFromText(tc));
        drawMenuText("BPM", 2, 3, tc);
      }
      break;
      
    case 7: // CHN - MIDI Voice Select - encoder 3: YPOS/MIDI/KEYS
      drawMenuText("CH", 2, 10, currentMenuParentTextColor());
      drawMidiVoiceSelect();
      drawMenuIndicator('L', voiceSelect == 1 ? 'X' : (voiceSelect == 2 ? 'M' : 'G'), 3);
      break;
      
    case 8: // TRN - MIDI Transport - encoder 3: OFF/GET/SEND
      drawMenuText("TRAN", 2, 10, currentMenuParentTextColor());
      drawMidiTransport();
      drawMenuIndicator('L', transportMode == 0 ? 'R' : (transportMode == 1 ? 'G' : 'X'), 3);
      break;
      
    case 13: // SEND - MIDI Send (CLCK, NOTE, BOTH) - encoder 3
      drawMenuText("SEND", 2, 10, currentMenuParentTextColor());
      drawMidiSend();
      {
        extern int midiSendMode;
        drawMenuIndicator('L', midiSendMode == 0 ? 'Y' : (midiSendMode == 1 ? 'G' : 'X'), 3);
      }
      break;

    case 44: // RCVE - MIDI Note Receive (OFF/NOTE) - encoder 3
      {
        extern bool MIDI_NOTE_RECEIVE;
        drawMenuText("RCVE", 2, 10, currentMenuParentTextColor());
        drawMenuValue(MIDI_NOTE_RECEIVE ? "NOTE" : "OFF", 2, 3, MIDI_NOTE_RECEIVE ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', MIDI_NOTE_RECEIVE ? 'G' : 'R', 3);
      }
      break;

    case 60: { // WRTE - MIDI overwrite occupied grid cells
        extern bool MIDI_WRITE_OVERWRITE;
        drawMenuText("OVWR", 2, 10, currentMenuParentTextColor());
        drawMenuValue(MIDI_WRITE_OVERWRITE ? "ON" : "OFF", 2, 3, MIDI_WRITE_OVERWRITE ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', MIDI_WRITE_OVERWRITE ? 'G' : 'R', 3);
      }
      break;

    case 51: // CLMP - fold incoming pitch into the visible grid
      {
        extern bool MIDI_NOTE_CLAMP;
        drawMenuText("FOLD", 2, 10, currentMenuParentTextColor());
        drawMenuValue(MIDI_NOTE_CLAMP ? "ON" : "OFF", 2, 3, MIDI_NOTE_CLAMP ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', MIDI_NOTE_CLAMP ? 'G' : 'R', 3);
      }
      break;

    case 45: // SYNC - Transport delay SNC1/SNC2 −127..+127 ms
      {
        extern int8_t transportSendDelayMs;
        extern int8_t transportRcveDelayMs;
        const CRGB tc = currentMenuParentTextColor();
        int syncEdit = (currentMode->pos[1] == 0) ? 0 : 1;
        drawMenuText(syncEdit == 0 ? "SYC>" : "SYC<", 2, 10, tc);
        int val = (syncEdit == 0) ? (int)transportSendDelayMs : (int)transportRcveDelayMs;
        char buf[10];
        extern bool getAnalogExtClock();
        const bool active = syncDelayHasEffect(syncEdit, val, MIDI_CLOCK_SEND,
                                              MIDI_TRANSPORT_RECEIVE, getAnalogExtClock());
        if (active) snprintf(buf, sizeof(buf), "%d", val);
        else snprintf(buf, sizeof(buf), "--");
        drawMenuValue(buf, 2, 3, active ? (syncEdit == 0 ? UI_GREEN : UI_RED) : UI_RED);
        drawMenuIndicator('L', 'Y', 1);
        drawMenuIndicator('L', 'W', 2);
      }
      break;

    case 50: { // PPQN - enc1 width, enc2 STOP/CONT, enc3 OFF/rate (click POL)
      extern bool getPulseClockEnabled();
      extern bool getPulseClockPolarityPositive();
      extern bool getPulseClockStopWithPlay();
      extern uint8_t getPulseClockPpqn();
      extern uint8_t getPulseClockWidthMs();
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("PQ-O", 2, 10, tc);
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

    case 58: { // SIDE - which voice pulses analog sidechain out
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

    case 59: { // PQ-I - same rate/width presentation as PQ-O
      extern bool getAnalogInEnabled();
      extern bool getAnalogInPolarityPositive();
      extern uint8_t getAnalogInPpqn();
      extern uint8_t getAnalogInHoldMs();
      drawMenuText("PQ-I", 2, 10, currentMenuParentTextColor());
      const bool positive = getAnalogInPolarityPositive();
      char buf[8];
      if (g_cinFocus == 2) {
        snprintf(buf, sizeof(buf), "%u", (unsigned)getAnalogInHoldMs());
        drawMenuValue(buf, 2, 3, UI_WHITE);
      } else if (!getAnalogInEnabled()) {
        drawMenuValue("OFF", 2, 3, UI_RED);
      } else {
        snprintf(buf, sizeof(buf), "%c%u", positive ? '+' : '-', (unsigned)getAnalogInPpqn());
        drawMenuValue(buf, 2, 3, positive ? UI_GREEN : UI_RED);
      }
      drawMenuIndicator('L', 'W', 1);
      drawMenuIndicator('L', positive ? 'G' : 'R', 3);
      break;
    }

    case 9: // PMD - Pattern Mode - encoder 3
      drawMenuText("PMODE", 2, 10, currentMenuParentTextColor());
      drawPatternMode();
      drawMenuIndicator('L', patternMode == -1 ? 'R' : (patternMode == 1 ? 'G' : (patternMode == 2 ? 'Y' : 'N')), 3);
      break;
      
    case 10: // FLW - Flow Mode - encoder 3
      drawMenuText("FLOW", 2, 10, currentMenuParentTextColor());
      drawFlowMode();
      drawMenuIndicator('L', flowMode == 1 ? 'G' : 'R', 3);
      break;
      
    case 11: // OTR - Fast Rec Mode
      drawMenuText("TRIG", 2, 10, currentMenuParentTextColor());
      drawFastRecMode();
      drawMenuIndicator('L', fastRecMode == 0 ? 'R' : (fastRecMode == 1 ? 'G' : (fastRecMode == 2 ? 'N' : (fastRecMode == 3 ? 'Y' : 'X'))), 3);
      break;
      
    case 12: // CLR - Rec Channel Clear - encoder 3
      drawMenuText("CLR", 2, 10, currentMenuParentTextColor());
      drawRecChannelClear();
      drawMenuIndicator('L', imageMode ? 'Y' : (recChannelClear == 0 ? 'R' : (recChannelClear == 1 ? 'G' : (recChannelClear == 2 ? 'Y' : (recChannelClear == 3 ? 'N' : 'M')))), 3);
      break;
      
    case 15: // AI - Song Generation
      drawMenuText("AUTO", 2, 10, currentMenuParentTextColor());
      // AUTO generate: encoder(0) press triggers generation -> green indicator on encoder(0)
      drawMenuIndicator('L', 'G', 1);  // Encoder(0): Large Green (generate)
      drawMenuIndicator('L', 'Y', 2);  // Encoder 2: Large Yellow (base start)
      drawMenuIndicator('L', 'W', 3);  // Encoder 3: Large White (base end)
      break;
      
    case 16: // RST - Reset
      drawMenuText("RSET", 2, 10, currentMenuParentTextColor());
      /*if (resetMenuOption == 0) {
        drawMenuText("EFX", 2, 3, CRGB(100, 0, 0));  // Dark Red
      } else {
        drawMenuText("FULL", 2, 3, CRGB(100, 0, 0));  // Dark Red
      }*/
      drawMenuIndicator('L', 'G', 2);  // Encoder 2: press applies the reset
      drawMenuIndicator('L', 'O', 3);  // Encoder 3: turn to pick SD/EFX/FULL/FILE/PACK/ASAV
      
      break;

    case 39: // INFO - Version / credits scroll
      drawEtcInfoPage();
      break;

    case 52: // RAM - live audio memory / reverb / free blocks dashboard
      drawEtcRamPage();
      break;

    case 49: // SCON — serial-connection status. The server runs on every screen.
      {
        extern bool sdSerialServerClientConnected();
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("SCON", 2, 10, tc);
        bool connected = sdSerialServerClientConnected();
        // WAIT (yellow) until a host talks; OK (green) while that link is recent
        drawMenuValue(connected ? "OK" : "WAIT", 2, 3, connected ? UI_GREEN : UI_YELLOW);
      }
      break;
      
    case 40: // LGHT - LED Strip toggle (OFF/ON) - encoder 3
      {
        extern bool getLedStripEnabled();
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("LGHT", 2, 10, tc);
        bool enabled = getLedStripEnabled();
        drawMenuValue(enabled ? "ON" : "OFF", 2, 3, enabled ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', enabled ? 'G' : 'R', 3);
      }
      break;
      
    case 41: // COLR - Color scheme selection (0=default, 1, 2, 3=custom) - encoder 3
      {
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("COLR", 2, 10, tc);
        char schemeText[2];
        snprintf(schemeText, sizeof(schemeText), "%d", currentColorScheme);
        drawMenuValue(schemeText, 2, 3, UI_GREEN);
        drawMenuIndicator('L', 'G', 3);
      }
      break;

    case 42: // BATT - voltage-based estimate using calibrated divider ratio
      {
        extern int getBatteryPercent();
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("BATT", 2, 10, tc);
        
        int pct = getBatteryPercent();
        pct = constrain(pct, 0, 100);
        
        char pctText[6];
        snprintf(pctText, sizeof(pctText), "%d%%", pct);
        
        // Color scale: >20% Green, (BATT_WARN_PCT+1)–20% Orange, ≤BATT_WARN_PCT Red
        CRGB stateColor = (pct > 20) ? UI_GREEN
                                     : (pct > BATT_WARN_PCT) ? CRGB(255, 165, 0)
                                                             : UI_RED;
        drawMenuText(pctText, 2, 3, stateColor);
      }
      break;

    case 48: // CHLD - Child lock toggle (OFF/ON) - encoder 3
      {
        extern bool getChildLockEnabled();
        const CRGB tc = currentMenuParentTextColor();
        drawMenuText("CHLD", 2, 10, tc);
        bool enabled = getChildLockEnabled();
        drawMenuValue(enabled ? "ON" : "OFF", 2, 3, enabled ? UI_GREEN : UI_RED);
        drawMenuIndicator('L', enabled ? 'G' : 'R', 3);
      }
      break;

    case 56: { // IMG - Image mode (OFF/ON)
      extern bool imageMode;
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("IMG", 2, 10, tc);
      drawMenuValue(imageMode ? "ON" : "OFF", 2, 3, imageMode ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', imageMode ? 'G' : 'R', 3);
      break;
    }

    case 57: { // EYES - screensaver idle OFF / 1MIN / 5MIN / 15MIN
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

    case 55: // FIRE - enc1 size, enc2 count, enc3 voice (click: gravity, colour)
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

    case 19: // SETTINGS - Submenu
      // SETTINGS = settings icon
      {
        const CRGB tc = menuTextColorFromCol(6);
        showIcons(ICON_SETTINGS_BIG, dimIconColorFromText(tc));
        drawMenuText("SETT", 2, 3, tc);
      }
      break;
      
    case 20: // RECS - Submenu
      // REC = record (icon shown on main menu entry)
      {
        const CRGB tc = menuTextColorFromCol(7);
        showIcons(ICON_RECORD, dimIconColorFromText(tc));
        drawMenuText("REC", 2, 3, tc);
      }
      break;
      
    case 21: // MIDI - Submenu
      // MIDI = midi
      {
        const CRGB tc = menuTextColorFromCol(8);
        showIcons(ICON_MIDISYNC, dimIconColorFromText(tc));
        drawMenuText("MIDI", 2, 3, tc);
      }
      break;
      
    case 22: // MODE > SONG: plain entry label
      {
        const CRGB tc = menuTextColorFromCol(13);
        drawMenuText("SONG", 2, 10, tc);
      }
      break;
      
    case 17: // VIEW - Simple Notes View - encoder 3
      drawMenuText("VIEW", 2, 10, currentMenuParentTextColor());
      drawSimpleNotesView();
      drawMenuIndicator('L', simpleNotesView == 1 ? 'G' : 'X', 3);
      break;
      
    case 18: // LOOP - Loop Length - encoder 3
      drawMenuText("LOOP", 2, 10, currentMenuParentTextColor());
      drawLoopLength();
      drawMenuIndicator('L', loopLength == 0 ? 'R' : 'N', 3);
      break;

    case 54: { // VMOD - per-voice page loops (OFF/ON)
      extern bool voiceMode;
      extern bool imageMode;
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("VMOD", 2, 10, tc);
      // VMOD locked off while IMG is on.
      const bool on = voiceMode && !imageMode;
      drawMenuValue(on ? "ON" : "OFF", 2, 3, on ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', on ? 'G' : 'R', 3);
      break;
    }
      
    case 23: // LEDS - LED mode (1, 1B, 2, 2B) - encoder 3
      drawMenuText("LEDS", 2, 10, currentMenuParentTextColor());
      drawLedModules();
      drawMenuIndicator('L', 'G', 3);
      break;

    case 24: // PONG toggle
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
    case 67: {
      const uint8_t from = swapVoices[swapFromChoice], to = swapVoices[swapToChoice];
      drawMenuText(swapHeld[0] || swapHeld[1] ? "OCT" : swapLastAt ? "OK" : "SWAP", 2, 10, currentMenuParentTextColor());
      drawMenuValue("", 1, 3, UI_WHITE);
      char value[4];
      if (swapHeld[0]) snprintf(value, sizeof(value), "%+d", swapOctaves[0]);
      else snprintf(value, sizeof(value), "%u", from);
      drawMenuText(value, 1, 3, col[from]);
      if (swapHeld[1]) snprintf(value, sizeof(value), "%+d", swapOctaves[1]);
      else snprintf(value, sizeof(value), "%u", to);
      drawMenuText(value, maxX >= 24 ? 24 : 10, 3, col[to]);
      if (maxX >= 24) drawMenuText("<>", 12, 3, UI_WHITE);
      else { light(8, 4, UI_WHITE); light(9, 4, UI_WHITE); }
      drawLargeIndicatorCustom(col[from], 2);
      drawLargeIndicatorCustom(col[to], 3);
      drawMenuIndicator('L', from == to ? 'R' : 'G', 4);
      break;
    }
    case 66: {
      drawMenuText("DEMO", 2, 10, currentMenuParentTextColor());
      drawMenuValue("GO", 2, 3, UI_GREEN);
      break;
    }
    case 61: { // MODE inherits the former SONG main-menu identity.
      const CRGB tc = menuTextColorFromCol(13);
      showIcons(ICON_SONG, dimIconColorFromText(tc));
      drawMenuText("MODE", 2, 3, tc);
      break;
    }
    case 62: { // ETC > ADV
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("ADV", 2, 10, tc);
      break;
    }

    case 34: // ETC - Submenu
      {
        const CRGB tc = menuTextColorFromCol(14);
        showIcons(ICON_PATTERN, dimIconColorFromText(tc));
        drawMenuText("ETC", 2, 3, tc);
      }
      break;
      
    case 38: // DRAW - Toggle between L+R and R modes
      drawMenuText("DRAW", 2, 10, currentMenuParentTextColor());
      drawMenuValue(drawMode == 0 ? "L+R" : "R", 2, 3, UI_GREEN);
      drawMenuIndicator('L', 'G', 3);
      break;

    case 47: { // MUTE — columns 1..16 = user CH1..CH16 (see userChToInternalDrawR); green=in mask, red=out; enc1=CH, enc2=toggle
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

    case 25: { // CTRL - encoder behaviour
      drawMenuText("CTRL", 2, 10, currentMenuParentTextColor());
      drawMenuValue(ctrlMode == 0 ? "PAGE" : "VOL", 2, 3, ctrlMode == 0 ? UI_GREEN : UI_ORANGE);
      // Enc3 edits PAGE/VOL — green for PAGE, orange for VOL (ring + row-1 indicator).
      drawMenuIndicator('L', ctrlMode == 0 ? 'G' : 'O', 3);
      break;
    }
    
    case 26: // VOL - Submenu
      // VOL = volume
      {
        const CRGB tc = menuTextColorFromCol(5);
        showIcons(ICON_VOLUME_BIG, dimIconColorFromText(tc));
        drawMenuText("VOL", 2, 3, tc);
      }
      break;

    case 27: { // MAIN - Headphone output volume - encoder 3
      drawMenuText("MAIN", 2, 10, currentMenuParentTextColor());
      extern struct GlobalVars GLOB;
      
      // Display GLOB.vol directly as 0-100
      int volDisplay = GLOB.vol;
      // Handle legacy values (0-10) for backward compatibility
      if (volDisplay <= 10) {
        volDisplay = volDisplay * 10;
      }
      char volText[8];
      snprintf(volText, sizeof(volText), "%d", volDisplay);
      drawMenuText(volText, 2, 3, UI_ORANGE); // Match encoder 3 and its indicator.
      
      drawMenuIndicator('L', 'O', 3);
      break;
    }
    
    case 28: { // LOUT - Line output volume - encoder 3
      drawMenuText("LOUT", 2, 10, currentMenuParentTextColor());
      extern uint8_t lineOutLevelSetting;
      char levelText[8];
      // Display mapped range 1..19 (LINEOUT_MIN..LINEOUT_MAX) for user friendliness
      int displayVal = (lineOutLevelSetting - LINEOUT_MIN) + 1;  // 13->1, 31->19
      snprintf(levelText, sizeof(levelText), "%d", displayVal);
      drawMenuText(levelText, 2, 3, UI_MAGENTA); // Pink
      
      // Match the magenta value and indicator.
      CRGB pinkColor = UI_MAGENTA;
      Encoder[2].writeRGBCode(pinkColor.r << 16 | pinkColor.g << 8 | pinkColor.b);
      drawMenuIndicator('L', 'M', 3);
      break;
    }
    
    case 29: { // PREV: encoder 2 selects policy, encoder 3 sets volume.
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

    case 30: { // MIC - Microphone gain - encoder 3
      drawMenuText("MIC", 2, 10, currentMenuParentTextColor());
      extern unsigned int micGain;
      char gainText[8];
      snprintf(gainText, sizeof(gainText), "%d", micGain);
      drawMenuText(gainText, 2, 3, UI_RED); // Red
      
      // Red color for encoder
      CRGB redColor = UI_RED;
      Encoder[2].writeRGBCode(redColor.r << 16 | redColor.g << 8 | redColor.b);
      drawMenuIndicator('L', 'R', 3);
      break;
    }
    
    case 31: { // L-IN - Line input level - encoder 3
      drawMenuText("L-IN", 2, 10, currentMenuParentTextColor());
      extern unsigned int lineInLevel;
      char levelText[8];
      snprintf(levelText, sizeof(levelText), "%d", lineInLevel);
      drawMenuText(levelText, 2, 3, UI_BLUE); // Blue
      
      // Blue color for encoder
      CRGB blueColor = UI_BLUE;
      Encoder[2].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
      drawMenuIndicator('L', 'X', 3);
      break;
    }

    case 36: { // 2-CH - Stereo routing (OFF, M+P, or L+R) - encoder 3
      drawMenuText("2-CH", 2, 10, currentMenuParentTextColor());
      // Clear text area before drawing (3 chars max: "OFF", "M+P", "L+R")
      clearTextArea(6, 3, 8);
      extern int8_t stereoChannel;
      if (stereoChannel == 0) {
        drawMenuText("OFF", 2, 3, UI_RED);
      } else if (stereoChannel == 1) {
        drawMenuText("M+P", 2, 3, UI_GREEN);
      } else { // stereoChannel == 2
        drawMenuText("L+R", 2, 3, UI_GREEN);
      }
      drawMenuIndicator('L', stereoChannel == 0 ? 'R' : 'G', 3);
      break;
    }

    case 43: { // SPKR - Speaker toggle - encoder 3
      extern bool getSpkrEnabled();
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("SPKR", 2, 10, tc);
      bool speakerOn = getSpkrEnabled();
      drawMenuValue(speakerOn ? "ON" : "OFF", 2, 3, speakerOn ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', speakerOn ? 'G' : 'R', 3);
      break;
    }

    case 64: { // HPF - library high-pass on the record path
      extern bool getAdcHpfEnabled();
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("HPF", 2, 10, tc);
      bool on = getAdcHpfEnabled();
      drawMenuValue(on ? "ON" : "OFF", 2, 3, on ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', on ? 'G' : 'R', 3);
      break;
    }

    case 65: { // USBP - SMP voice 0 also on USB
      extern bool getUsbSmpPreview();
      const CRGB tc = currentMenuParentTextColor();
      drawMenuText("USBP", 2, 10, tc);
      bool on = getUsbSmpPreview();
      drawMenuValue(on ? "ON" : "OFF", 2, 3, on ? UI_GREEN : UI_RED);
      drawMenuIndicator('L', on ? 'G' : 'R', 3);
      break;
    }

    case 46: { // HFC — 0, 10..250, 256 (encoder 3)
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

    case 53: { // GAIN — enc1 1-4, enc2 5-8, enc3 SYN or ALL (click toggles)
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
      // Title length changes (ALL/1-4/SYN) — clear before draw (partial redraw path).
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
    
    case 32: { // CRSR - Cursor type - encoder 3
      drawMenuText("CRSR", 2, 10, currentMenuParentTextColor());
      extern bool showChannelNr;
      const char* lbl = showChannelNr ? "CHNR" : "NORM";
      CRGB col = UI_GREEN;
      drawMenuValue(lbl, 2, 3, col);
      Encoder[2].writeRGBCode(UI_GREEN.r << 16 | UI_GREEN.g << 8 | UI_GREEN.b);
      drawMenuIndicator('L', 'G', 3);
      break;
    }

    case 33: { // PREV - Preview trigger mode - encoder 3
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
    case 4: { // REC page - Mic Gain
      //drawText("GAIN:", 2, 12, UI_DIM_WHITE);
      char gainText[8];
      sprintf(gainText, "%d", micGain);
      //drawText(gainText, 8, 12, UI_WHITE);
      
      // Mic gain meter is now drawn vertically in drawRecMode() on x=16
      // No horizontal meter needed here since page indicator uses y=16
      break;
    }
      
    
    case 15: { // AI page - Base Page Range + Additional Pages Count (all on one line)
      // Draw everything on y=8: Orange base range + Green additional pages count
      for (int x = 1; x <= 16; x++) {
        if (x >= aiBaseStartPage && x <= aiBaseEndPage) {
          light(x, 8, CRGB(255, 165, 0)); // Orange for base pages
        } else if (x > aiBaseEndPage && x <= aiBaseEndPage + aiTargetPage) {
          light(x, 8, CRGB(0, 255, 0)); // Green for additional pages to generate
        } else {
          light(x, 8, CRGB(0, 0, 0));
        }
      }
      
      break;
    }

    case 16: { // RST page - show current mode (SD, EFX, FULL, PATT, KIT, ASAV, RAM)
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
      } else if (resetMenuOption == 5) {
        modeText = "ASAV";
      } else {
        modeText = "RAM";
      }
      drawMenuValue(modeText, 2, 3, UI_ORANGE);
      break;
    }
    
  }
}

FLASHMEM void drawGenreSelection() {
  // Draw genre options on y=8
  const char* genres[] = {"BLNK", "TECH", "HIPH", "DNB", "HOUS", "AMBT"};
  CRGB genreColors[] = {
    CRGB(100, 100, 100), // BLNK - gray
    CRGB(255, 100, 0),   // TECH - orange
    CRGB(255, 0, 255),   // HIPH - magenta
    CRGB(0, 255, 0),     // DNB - green
    CRGB(0, 100, 255),   // HOUS - blue
    CRGB(255, 255, 0)    // AMBT - yellow
  };
  
  // Draw current genre text
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
  
  // Reset first enter flags when switching to a different setting
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
    if (lastSetting == 67) {
      for (int enc = 1; enc <= 2; ++enc) {
        Encoder[enc].writeMin((int32_t)1); Encoder[enc].writeMax((int32_t)1);
        Encoder[enc].writeCounter((int32_t)1); currentMode->pos[enc] = 1;
      }
      swapHeld[0] = swapHeld[1] = false;
    }
    lastSetting = setting;
  }
  
  auto redrawMain = [&](int s) {
    // Partial redraws skip FastLEDclear — wipe value + title rows full-width
    // (GAIN title ALL/1-4/SYN; numeric width changes; LOOP OFF↔digit).
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
    case 67: {
      bool changed = false;
      for (int side = 0; side < 2; ++side) {
        const int enc = side + 1;
        const bool held = pressed[enc];
        uint8_t &choice = side == 0 ? swapFromChoice : swapToChoice;
        if (menuFirstEnter || held != swapHeld[side]) {
          swapHeld[side] = held;
          Encoder[enc].writeMin((int32_t)0);
          Encoder[enc].writeMax((int32_t)(held ? 16 : 10));
          const int value = held ? swapOctaves[side] + 8 : choice;
          Encoder[enc].writeCounter((int32_t)value); currentMode->pos[enc] = value;
          changed = true;
        }
        if (held) {
          const int8_t value = constrain((int)currentMode->pos[enc], 0, 16) - 8;
          if (swapOctaves[side] != value) { swapOctaves[side] = value; changed = true; }
        } else {
          const uint8_t value = constrain((int)currentMode->pos[enc], 0, 10);
          if (choice != value) { choice = value; changed = true; }
        }
      }
      menuFirstEnter = false;
      if (changed || (swapLastAt && millis() - swapLastAt >= 600)) {
        swapLastAt = 0; redrawMain(setting);
      }
      break;
    }
    case 4: { // INPT - MIC/LINE via encoder 2 rotation
      static int lastRecModeEnc = -1;
      int encVal = (recMode == 1) ? 0 : 1;  // 0=MIC, 1=LINE
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

    case 12: { // CLR - Rec Channel Clear (OFF/ON/FIX/ON1/CLIC) via encoder 2 rotation
      extern bool imageMode;
      static int lastRecChannelClear = -1;
      // IMG forces FIX (2); lock the control while IMG is on.
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

    case 7: { // CH - MIDI Voice Select (YPOS/MIDI/KEYS) via encoder 2 rotation
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

    case 8: { // TRAN - MIDI Transport (OFF/GET/SEND) via encoder 2 rotation
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

    case 13: { // SEND - MIDI Send (CLCK/NOTE/BOTH) via encoder 2 rotation
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

    case 44: { // RCVE - MIDI Note Receive (OFF/NOTE) via encoder 2 rotation
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

    case 60: { // WRTE - overwrite occupied cells (OFF/ON) via encoder 3
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

    case 51: { // CLMP - MIDI pitch clamp (OFF/ON) via encoder 2 rotation
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

    case 45: { // SYNC - SNC1/SNC2 −127..+127 ms (encoder raw 0..254 = value + 127; Mode.pos is unsigned)
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

    case 50: { // PPQN - enc1 width, enc2 STOP/CONT, enc3 OFF/rate
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
      int runVal = getPulseClockStopWithPlay() ? 0 : 1;  // 0=STOP, 1=CONT
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

    case 58: { // SIDE - encoder 3 selects voice (OFF, 1–8, 11, 13, 14)
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

    case 59: { // PQ-I - enc1 hold-off, enc3 OFF/rate (click polarity)
      extern uint8_t getAnalogInHoldMs();
      extern void setAnalogInHoldMs(uint8_t);
      extern uint8_t getAnalogInRateSel();
      extern uint8_t getAnalogInRateSelCount();
      extern void setAnalogInRateSel(uint8_t);
      static const int kCinHoldMin = 1;
      static const int kCinHoldMax = 32;
      static int lastCinHold = -1;
      static int lastCinRate = -1;
      const int rateMax = (int)getAnalogInRateSelCount() - 1;
      if (menuFirstEnter) {
        Encoder[0].writeMin((int32_t)kCinHoldMin);
        Encoder[0].writeMax((int32_t)kCinHoldMax);
        Encoder[0].writeCounter((int32_t)getAnalogInHoldMs());
        Encoder[2].writeMin((int32_t)0);
        Encoder[2].writeMax((int32_t)rateMax);
        Encoder[2].writeCounter((int32_t)getAnalogInRateSel());
        currentMode->pos[0] = (unsigned int)getAnalogInHoldMs();
        currentMode->pos[2] = (unsigned int)getAnalogInRateSel();
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
        g_cinFocus = 2;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      if ((int)currentMode->pos[2] != lastCinRate) {
        setAnalogInRateSel((uint8_t)constrain((int)currentMode->pos[2], 0, rateMax));
        lastCinRate = (int)getAnalogInRateSel();
        Encoder[2].writeCounter((int32_t)lastCinRate);
        currentMode->pos[2] = (unsigned int)lastCinRate;
        g_cinFocus = 0;
        menuRequestFullRedraw();
        redrawMain(setting);
      }
      break;
    }

    case 52: { // RAM - encoder 2 selects metric; click resets audio peak counters
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

    case 40: { // LGHT - LED Strip (OFF/ON) via encoder 2 rotation
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

    case 41: { // COLR - Color scheme (0,1,2,3) via encoder 2 rotation
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

    case 43: { // SPKR - Speaker (OFF/ON): encoder UP = ON, DOWN = OFF
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

    case 64: { // HPF: encoder UP = ON, DOWN = OFF
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

    case 65: { // USBP: encoder UP = ON, DOWN = OFF
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

    case 55: { // FIRE - enc1 size 1–4, enc2 count 1–25, enc3 voice / gravity / colour
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

    case 48: { // CHLD - Child lock (OFF/ON) via encoder 2 rotation
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
    case 56: { // IMG - Image mode (OFF/ON) via encoder 2
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

    case 57: { // EYES - OFF/1MIN/5MIN/15MIN via encoder 2
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

    case 46: { // HFC — enc 0..25 → 0..250 step 10; enc 26 → 256 (MAX)
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

    case 9: { // PMD - Pattern Mode (OFF/ON/SONG/NEXT) via encoder 2 rotation
      static int lastPatternMode = -2;
      // Map: encoder 0=-1, 1=1, 2=2, 3=3
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

    case 10: { // FLW - Flow Mode (OFF/ON) via encoder 2 rotation
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

    case 17: { // VIEW - Simple Notes View (EASY/FULL) via encoder 2 rotation
      static int lastViewEnc = -1;
      int encVal = simpleNotesView - 1;  // 0=EASY, 1=FULL
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

    case 18: { // LOOP - Loop Length (0-8) via encoder 2 rotation
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

    case 54: { // VMOD - per-voice page loops (OFF/ON) via encoder 2
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
          // VMOD not allowed while IMG is on — snap encoder back to OFF.
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

    case 23: { // LEDS - LED mode (1, 1B, 2, 2B) via encoder 2 rotation
      static int lastLedMode = -1;
      extern int ledModules;
      extern bool ledModulesRotated;
      uint8_t ledMode = ledModules + (ledModulesRotated ? 2 : 0);  // 1,2,3,4
      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)(ledMode - 1));  // 0-3
        Encoder[2].writeMax((int32_t)3);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = ledMode - 1;
        lastLedMode = ledMode;
        menuFirstEnter = false;
      }
      if (currentMode->pos[2] != lastLedMode - 1) {
        ledMode = constrain((int)currentMode->pos[2], 0, 3) + 1;  // 1-4
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

    case 25: { // CTRL - Encoder behaviour (PAGE/VOL) via encoder 2 rotation
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

    case 32: { // CRSR - Cursor type (NORM/CHNR) via encoder 2 rotation
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

    case 33: { // PREV - Preview trigger (ON/PRSS/SYNC) via encoder 2 rotation
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

    case 38: { // DRAW - L+R / R via encoder 2 rotation
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

    case 47: { // MUTE — enc1 = user CH1..CH16; enc2 toggles internal bit for that CH
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

    case 11: { // TRIG - Fast Rec Mode (OFF/INST/SENS/-CON/+CON) via encoder 2 rotation
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
      
    case 15: { // AI page - Base Start (enc1), Base End (enc2), Target Count (enc0)
      static int lastAiTargetPage = -1;
      static int lastAiBaseStartPage = -1;
      static int lastAiBaseEndPage = -1;
      
      // Set encoder counters only on first entry
      if (aiMenuFirstEnter) {
        // Encoder 0 is now the trigger button, not used for values
        
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
      
      // Handle target page count (encoder 0)
      if (currentMode->pos[0] != lastAiTargetPage) {
        aiTargetPage = currentMode->pos[0];
        if (aiTargetPage > 16) aiTargetPage = 16;
        if (aiTargetPage < 1) aiTargetPage = 1;
        redrawAdd(setting);
        lastAiTargetPage = aiTargetPage;
      }
      
      // Handle base start page (encoder 1)
      if (currentMode->pos[1] != lastAiBaseStartPage) {
        aiBaseStartPage = currentMode->pos[1];
        if (aiBaseStartPage > 16) aiBaseStartPage = 16;
        if (aiBaseStartPage < 1) aiBaseStartPage = 1;
        
        // Ensure base start <= base end
        if (aiBaseStartPage > aiBaseEndPage) {
          aiBaseEndPage = aiBaseStartPage;
          Encoder[2].writeCounter((int32_t)aiBaseEndPage);
        }
        
        redrawAdd(setting);
        lastAiBaseStartPage = aiBaseStartPage;
      }
      
      // Handle base end page (encoder 2)
      if (currentMode->pos[2] != lastAiBaseEndPage) {
        aiBaseEndPage = currentMode->pos[2];
        if (aiBaseEndPage > 16) aiBaseEndPage = 16;
        if (aiBaseEndPage < 1) aiBaseEndPage = 1;
        
        // Ensure base end >= base start
        if (aiBaseEndPage < aiBaseStartPage) {
          aiBaseStartPage = aiBaseEndPage;
          Encoder[1].writeCounter((int32_t)aiBaseStartPage);
        }
        
        redrawAdd(setting);
        lastAiBaseEndPage = aiBaseEndPage;
      }
      break;
    }
    
    case 16: { // RST page - Choose between SD, EFX, FULL, PATT, KIT, ASAV, RAM
      static int lastResetOption = -1;

      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)resetMenuOption);
        Encoder[2].writeMax((int32_t)6);  // 0=SD, 1=EFX, 2=FULL, 3=PATT, 4=KIT, 5=ASAV, 6=RAM
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = (unsigned int)resetMenuOption;
        lastResetOption = resetMenuOption;
        menuFirstEnter = false;
      }

      if ((int)currentMode->pos[2] != lastResetOption) {
        resetMenuOption = constrain((int)currentMode->pos[2], 0, 6);
        currentMode->pos[2] = (unsigned int)resetMenuOption;
        Encoder[2].writeCounter((int32_t)resetMenuOption);
        // Option label is in drawAdditionalFeatures (drawMenuValue clears the row).
        redrawAdd(setting);
        lastResetOption = resetMenuOption;
      }
      break;
    }

    case 42: // BATT - read-only, no encoder control
      break;
    
    case 24: { // PONG page - encoder 2: OFF/ON when !pong, speed when pong
      if (!pong) {
        static int lastPongEnc = -1;
        int encVal = 0;  // OFF
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
      // Encoder 2: 0=OFF, 1-99=speed (rotate down to 0 to turn off)

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
    
    case 27: { // MAIN - Headphone output volume
      static int lastHpVol = -1;
      
      extern Mode *currentMode;
      extern struct GlobalVars GLOB;
      
      if (mainMenuFirstEnter) {
        // Load GLOB.vol from EEPROM (0-10 legacy or 0-100 new)
        // Convert legacy 0-10 to 0-100 for backward compatibility
        int encoderVal;
        if (GLOB.vol <= 10) {
          // Legacy value (0-10): convert to 0-100
          encoderVal = GLOB.vol * 10;
        } else {
          // New value (0-100): use directly
          encoderVal = GLOB.vol;
        }
        encoderVal = constrain(encoderVal, 0, 100);
        
        Encoder[2].writeCounter((int32_t)encoderVal);
        Encoder[2].writeMax((int32_t)100);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encoderVal;
        lastHpVol = encoderVal;
        mainMenuFirstEnter = false;
        
        // Apply to hardware: map 0-100 to 0.0-1.0
        float vol = encoderVal / 100.0f;
        sgtl5000_1.volume(vol);
        
        // Update GLOB.vol to new range (0-100)
        GLOB.vol = encoderVal;
      }
      
      if (currentMode->pos[2] != lastHpVol) {
        int encoderPos = constrain(currentMode->pos[2], 0, 100);
        // Map encoder position (0-100) directly to volume (0.00-1.00)
        // Position 0 = 0.00, Position 100 = 1.00
        float vol = encoderPos / 100.0f;
        
        // Store in GLOB.vol as 0-100
        GLOB.vol = encoderPos;
        
        // Apply to hardware
        sgtl5000_1.volume(vol);
        
        // Save to EEPROM (slot 17 - separate from transportMode which uses slot 2)
        // Store as 0-100 (int8_t can hold -128 to 127, so 0-100 is fine)
        saveSingleModeToEEPROM(17, (int8_t)GLOB.vol);
        
        redrawMain(setting);
        lastHpVol = encoderPos;
      }
      break;
    }
    
    case 28: { // LINE - Line output volume
      static int lastLoVol = -1;
      
      extern uint8_t lineOutLevelSetting;
      
      if (lineMenuFirstEnter) {
        int displayVal = constrain((int)(lineOutLevelSetting - LINEOUT_MIN + 1), 1, (LINEOUT_MAX - LINEOUT_MIN + 1)); // 1..19
        Encoder[2].writeCounter((int32_t)displayVal);
        Encoder[2].writeMax((int32_t)(LINEOUT_MAX - LINEOUT_MIN + 1)); // 19
        Encoder[2].writeMin((int32_t)1);
        currentMode->pos[2] = displayVal;
        lastLoVol = displayVal;
        lineMenuFirstEnter = false;
        
        // Apply to hardware
        sgtl5000_1.lineOutLevel(32-lineOutLevelSetting);
      }
      
      if (currentMode->pos[2] != lastLoVol) {
        int encoderPos = constrain(currentMode->pos[2], 1, (LINEOUT_MAX - LINEOUT_MIN + 1)); // 1..19
        lineOutLevelSetting = (uint8_t)(LINEOUT_MIN + encoderPos - 1);
        
        // Apply to hardware
        sgtl5000_1.lineOutLevel(32-lineOutLevelSetting);
        
        // Save to EEPROM
        saveSingleModeToEEPROM(15, (int8_t)lineOutLevelSetting);
        
        redrawMain(setting);
        lastLoVol = encoderPos;
      }
      break;
    }
    
    case 29: { // PREV - Preview volume
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
        // Load previewVol from EEPROM (0-50), direct mapping to encoder range (0-50)
        int encoderVal = constrain((int)previewVol, 0, 50);
        
        Encoder[2].writeCounter((int32_t)encoderVal);
        Encoder[2].writeMax((int32_t)50);
        Encoder[2].writeMin((int32_t)0);
        currentMode->pos[2] = encoderVal;
        lastPvVol = encoderVal;
        prevMenuFirstEnter = false;
        
        // Apply to hardware
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
        previewVol = encoderPos;  // Store 0-50 (0.00-0.50)
        
        // Apply to hardware
        updatePreviewVolume();
        
        // Save to EEPROM
        saveSingleModeToEEPROM(7, previewVol);
        
        redrawMain(setting);
        lastPvVol = encoderPos;
      }
      break;
    }
    
    case 30: { // MIC - Microphone gain
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
        
        // Apply to hardware only if MIC is selected
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
        
        // Apply to hardware only if MIC is selected
        extern unsigned int recInput;
        if (recInput == AUDIO_INPUT_MIC) {
          sgtl5000_1.micGain(micGain);
        } else {
          sgtl5000_1.micGain(0);
        }
        
        // Save to EEPROM
        saveSingleModeToEEPROM(9, micGain);
        
        redrawMain(setting);
        lastMicGain = encoderPos;
      }
      break;
    }
    
    case 31: { // LVL - Line input level
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
        
        // Apply to hardware
        sgtl5000_1.lineInLevel(lineInLevel);
      }
      
      if (currentMode->pos[2] != lastLineInLevel) {
        int encoderPos = constrain(currentMode->pos[2], 0, 15);
        lineInLevel = (unsigned int)encoderPos;
        
        // Apply to hardware
        sgtl5000_1.lineInLevel(lineInLevel);
        
        // Save to EEPROM
        saveSingleModeToEEPROM(16, lineInLevel);
        
        redrawMain(setting);
        lastLineInLevel = encoderPos;
      }
      break;
    }

    case 36: { // 2-CH - Stereo routing (OFF, M+P, or L+R)
      extern int8_t stereoChannel;
      extern void applyStereoChannelRouting();
      
      // Always enforce correct encoder range for 2-CH.
      // This fixes the "can't change value" issue when coming from a page like PREV (0-50),
      // where encoder(2) would otherwise stay at a high count and effectively clamp to 2.
      Encoder[2].writeMax((int32_t)2);  // 0=OFF, 1=M+P, 2=L+R
      Encoder[2].writeMin((int32_t)0);

      if (stereoChMenuFirstEnter) {
        int encoderVal = constrain((int)stereoChannel, 0, 2);
        // Backward-compat: clamp any legacy/invalid values (>2) to L+R.
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
        stereoChannel = (int8_t)encoderPos;  // 0=OFF, 1=M+P, 2=L+R
        
        saveSingleModeToEEPROM(23, stereoChannel);
        applyStereoChannelRouting();
        
        redrawMain(setting);
        lastStereoCh = encoderPos;
      }
      break;
    }

    case 53: { // GAIN — enc1 1-4, enc2 5-8, enc3 SYN or ALL
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
       // Reset first enter flags when not on pages with additional features
       aiMenuFirstEnter = true;
       menuFirstEnter = true;
       pongMenuFirstEnter = true;
       break;
  }
  return didRedraw;
}

FLASHMEM void switchMenu(int menuPosition){
   switch (menuPosition) {
      case 67:
        if (swapFromChoice != swapToChoice) {
          swapChannelNotes(swapVoices[swapFromChoice], swapVoices[swapToChoice], swapOctaves[0], swapOctaves[1]);
          swapLastAt = millis();
          menuRequestFullRedraw();
        }
        break;
      case 66:
        startDemoAnimation();
        break;
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
        // Toggle input source: MIC <-> LINEIN
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
        // Cycle through: -1 (YPOS) -> 1 (MIDI) -> 2 (KEYS) -> -1 (YPOS) ...
        if (voiceSelect == -1) {
          voiceSelect = 1;  // YPOS -> MIDI
        } else if (voiceSelect == 1) {
          voiceSelect = 2;  // MIDI -> KEYS
        } else {
          voiceSelect = -1; // KEYS -> YPOS
        }
        saveSingleModeToEEPROM(4, voiceSelect);

        drawMainSettingStatus(menuPosition);
        break;

      case 8:
        if (transportMode == -1) {
          transportMode = 1; // OFF -> GET
        } else if (transportMode == 1) {
          transportMode = 2; // GET -> SEND
        } else {
          transportMode = -1; // SEND -> OFF
        }
        saveSingleModeToEEPROM(2, transportMode);
        drawMainSettingStatus(menuPosition);
        break;
        
      case 13:
        // Cycle through: CLCK (0) -> NOTE (1) -> BOTH (2) -> CLCK (0)
        {
          extern int midiSendMode;
          extern bool MIDI_CLOCK_SEND;
          extern bool MIDI_NOTE_SEND;
          extern int clockMode;
          
          midiSendMode = (midiSendMode + 1) % 3;  // Cycle 0->1->2->0
          
          // Update flags based on new mode
          if (midiSendMode == 0) {
            // CLCK only: clock enabled (if clockMode allows), notes disabled
            MIDI_CLOCK_SEND = (clockMode == 1);
            MIDI_NOTE_SEND = false;
          } else if (midiSendMode == 1) {
            // NOTE only: clock disabled, notes enabled
            MIDI_CLOCK_SEND = false;
            MIDI_NOTE_SEND = true;
          } else {
            // BOTH: clock enabled (if clockMode allows), notes enabled
            MIDI_CLOCK_SEND = (clockMode == 1);
            MIDI_NOTE_SEND = true;
          }
          
          saveSingleModeToEEPROM(24, midiSendMode);
          drawMainSettingStatus(menuPosition);
        }
        break;

      case 9:
        // Cycle through: -1 (OFF) -> 1 (ON) -> 2 (SONG) -> 3 (NEXT) -> -1 (OFF) ...
        if (patternMode == -1) {
          patternMode = 1;  // OFF -> ON
        } else if (patternMode == 1) {
          patternMode = 2;  // ON -> SONG
        } else if (patternMode == 2) {
          patternMode = 3;  // SONG -> NEXT
        } else {
          patternMode = -1; // NEXT -> OFF
        }
        saveSingleModeToEEPROM(3, patternMode);
        drawMainSettingStatus(menuPosition);
        
        // Update songModeActive flag
        extern bool songModeActive;
        songModeActive = (patternMode == 2);
        
        // Clear pending page when switching away from NEXT mode
        if (patternMode != 3) {
          extern unsigned int pendingPage;
          pendingPage = 0;
        }
        
        // Handle mute system when PMOD is toggled
        if (SMP_PATTERN_MODE) {
          // Switching TO PMOD mode - unmute all, then apply saved page mutes
          unmuteAllChannels();
          applyMutesAfterPMODSwitch();
        } else {
          // Switching FROM PMOD mode - unmute all, then apply global mutes
          unmuteAllChannels();
          applyMutesAfterPMODSwitch();
        }
        
        // Update encoder 1 limit when pattern mode is toggled
        if (currentMode == &draw || currentMode == &singleMode) {
          if (SMP_PATTERN_MODE) {
            // Pattern modes normally stop at lastPage; NEXT deliberately
            // exposes every runtime-valid page for live cueing.
            updateLastPage();
            Encoder[1].writeMax((int32_t)encoderPageMax());
          } else {
            // Pattern mode is OFF - allow up to maxPages
            Encoder[1].writeMax((int32_t)maxPages);
          }
        }
        break;

      case 10:
        flowMode = flowMode * (-1);
        saveSingleModeToEEPROM(8, flowMode);
        drawMainSettingStatus(menuPosition);
        
        // Reset lastFlowPage when FLOW mode is toggled
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
          recChannelClear = 2;  // FIX locked while IMG on
        } else {
          recChannelClear = recChannelClear + 1;
          if (recChannelClear > 4) recChannelClear = 0;  // Cycle: 0->1->2->3->4->0 (OFF->ON->FIX->ON1->CLIC->OFF)
        }
        saveSingleModeToEEPROM(6, recChannelClear);
        drawMainSettingStatus(menuPosition);
        break;
        }

        case 15:
        // AI Song Generation - generate song from current page to target page
        generateSong();
        break;

        case 16:
        if (resetMenuOption == 0) {
          // SD rescan: invalidate dynamic sample browser cache
          FastLEDclear();
          drawText("SCAN", 2, 3, UI_GREEN);
          FastLEDshow();

          extern bool scanAndWriteManifest();
          extern void sampleBrowserInvalidate();
          scanAndWriteManifest();
          sampleBrowserInvalidate();
          showSaveSuccessAnimation();
        } else if (resetMenuOption == 1) {
          // Reset effects/parameters to defaults
          resetAllToDefaults();
        } else if (resetMenuOption == 2) {
          // FULL reset: Complete reset (like startNew) AND rescan SD
          
          // Call startNew() for complete reset (clears notes, resets all settings, etc.)
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
          // FILE: Wipe all saved files (1-999.txt) but keep autosaved.txt
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
          // PACK: Remove all samplepack directories except 1/ (used by FULL reset)
          FastLEDclear();
          drawText("WIPE", 2, 10, UI_ORANGE);
          drawText("KIT", 2, 3, UI_ORANGE);
          FastLEDshow();
          
          char dirname[16];
          char filepath[32];
          for (int pack = 0; pack <= 999; pack++) {
            if (pack == 1) continue;  // Keep samplepack 1 (used by FULL reset)
            
            sprintf(dirname, "%d", pack);
            if (SD.exists(dirname)) {
              // Delete all .wav files in the directory first
              for (int sample = 1; sample <= 8; sample++) {
                sprintf(filepath, "%d/%d.wav", pack, sample);
                if (SD.exists(filepath)) {
                  SD.remove(filepath);
                }
              }
              // Try to remove the directory (only works if empty)
              SD.rmdir(dirname);
            }
          }
          
          // Invalidate sample browser cache after removing packs
          extern void sampleBrowserInvalidate();
          sampleBrowserInvalidate();

          showSaveSuccessAnimation();
        } else if (resetMenuOption == 5) {
          // ASAV: remove autosaved.txt (and leftover pattern.tmp)
          FastLEDclear();
          drawText("WIPE", 2, 10, UI_ORANGE);
          drawText("ASAV", 2, 3, UI_ORANGE);
          FastLEDshow();

          extern void deleteAutosaveFile();
          extern void clearPatternNotes();
          deleteAutosaveFile();
          clearPatternNotes();

          showSaveSuccessAnimation();
        } else if (resetMenuOption == 6) {
          // RAM: zero every EEPROM byte, same as touch1 held to 10s.
          // Drop the SD backup first so the next boot cannot copy the old block back.
          FastLEDclear();
          drawText("RAM", 3, 3, CRGB(255, 255, 0));
          FastLEDshow();

          AudioNoInterrupts();
          if (SD.exists(SETTINGS_BACKUP_PATH)) SD.remove(SETTINGS_BACKUP_PATH);
          if (SD.exists(SETTINGS_BACKUP_TMP_PATH)) SD.remove(SETTINGS_BACKUP_TMP_PATH);
          for (unsigned int i = 0; i < EEPROM.length(); i++) {
            EEPROM.write(i, 0);
          }

          FastLEDclear();
          drawText("RAM", 3, 3, CRGB(255, 255, 0));
          drawText("OK", 5, 10, CRGB(255, 255, 0));
          FastLEDshow();
          delay(1000);
          SCB_AIRCR = 0x05FA0004;
          while (true) { yield(); }
        }
        break;
        
        case 19:
        // Enter LOOK/SETTINGS submenu at first page
        inLookSubmenu = true;
        currentLookPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;
        
        case 38:
        // DRAW mode toggle: switch between L+R (0) and R (1)
        drawMode = (drawMode == 0) ? 1 : 0;
        saveSingleModeToEEPROM(21, (int8_t)drawMode);
        drawMainSettingStatus(menuPosition);
        break;
        
        case 32:
        // Toggle cursor: NORM <-> CHNR
        {
          extern bool showChannelNr;
          showChannelNr = !showChannelNr;
          saveSingleModeToEEPROM(18, 0);
          saveSingleModeToEEPROM(19, (int8_t)(showChannelNr ? 1 : 0));
          
          drawMainSettingStatus(menuPosition);
        }
        break;
        
        case 20:
        // Enter RECS submenu at first page
        inRecsSubmenu = true;
        currentRecsPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;
        
        case 21:
        // Enter MIDI submenu at first page
        inMidiSubmenu = true;
        currentMidiPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;
        
        case 26:
        // Enter VOL submenu at first page
        inVolSubmenu = true;
        currentVolPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;

        case 61:
        case 62:
        case 34:
        currentSystemMenuGroup = menuPosition == 61 ? SystemMenuGroup::Mode :
            (menuPosition == 62 ? SystemMenuGroup::Advanced : SystemMenuGroup::Etc);
        // Enter the selected group at its first page
        inEtcSubmenu = true;
        currentEtcPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        invalidateEnc3RangeCache();
        menuRequestFullRedraw();
        break;

        case 17:
        // Cycle through VIEW modes: 1=EASY, 2=FULL
        simpleNotesView = (simpleNotesView == 1) ? 2 : 1;
        saveSingleModeToEEPROM(11, simpleNotesView);
        drawMainSettingStatus(menuPosition);
        break;
        
        case 18:
        // Cycle through LOOP length: 0=OFF, 1-8
        loopLength = loopLength + 1;
        if (loopLength > 8) loopLength = 0;
        saveSingleModeToEEPROM(12, loopLength);
        drawMainSettingStatus(menuPosition);
        break;

        case 54: {
        extern bool voiceMode;
        extern bool imageMode;
        if (imageMode) {
          // VMOD not allowed while IMG is on.
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
        // Enter SONG mode from MODE, without leaving stale submenu state.
        inEtcSubmenu = false;
        currentMenuPage = 8;
        extern Mode songMode;
        switchMode(&songMode);
        break;
        }
        
        case 23: {
        // Cycle LED mode: 1 -> 1B -> 2 -> 2B -> 1
        extern int ledModules;
        extern bool ledModulesRotated;
        extern unsigned int maxX;
        extern Mode draw;
        extern Mode singleMode;

        uint8_t ledMode = ledModules + (ledModulesRotated ? 2 : 0);  // 1,2,3,4
        switch (ledMode) {
          case 1: ledMode = 3; break;  // 1  -> 1B
          case 3: ledMode = 2; break;  // 1B -> 2
          case 2: ledMode = 4; break;  // 2  -> 2B
          case 4: ledMode = 1; break;  // 2B -> 1
          default: ledMode = 1; break;
        }

        ledModules = (ledMode == 1 || ledMode == 3) ? 1 : 2;
        ledModulesRotated = (ledMode >= 3);
        maxX = MATRIX_WIDTH * ledModules;  // Update maxX runtime variable
        
        // Reinitialize LED strip with new module count
        extern void initLedStrip();
        initLedStrip();
        
        // Update encoder[1] max for draw/single mode based on new maxX
        if (currentMode == &draw || currentMode == &singleMode) {
          int dynamicPages = (MAX_STEPS / maxX);
          Encoder[1].writeMax((int32_t)dynamicPages);
          Encoder[3].writeMax((int32_t)MAX_STEPS);
          
          // Clamp current page to new max if needed
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
        // Toggle preview trigger mode: ON -> PRSS -> SYNC -> ON
        if (previewTriggerMode == PREVIEW_MODE_ON) previewTriggerMode = PREVIEW_MODE_PRESS;
        else if (previewTriggerMode == PREVIEW_MODE_PRESS) previewTriggerMode = PREVIEW_MODE_SYNC;
        else previewTriggerMode = PREVIEW_MODE_ON;
        saveSingleModeToEEPROM(20, (int8_t)previewTriggerMode);
        drawMainSettingStatus(menuPosition);
        break;
      }

      case 40: {
        // Toggle LED strip: OFF <-> ON
        extern bool getLedStripEnabled();
        extern void setLedStripEnabled(bool enabled);
        bool currentState = getLedStripEnabled();
        bool newState = !currentState;
        setLedStripEnabled(newState);
        saveSingleModeToEEPROM(25, (int8_t)(newState ? 1 : 0));
        menuRequestFullRedraw();  // Force ETC submenu to redraw with new state
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
        // Cycle EYES: OFF → 1MIN → 5MIN → 15MIN → OFF
        extern uint8_t eyesMode;
        eyesMode = (uint8_t)((eyesMode + 1) % 4);
        saveSingleModeToEEPROM(49, (int8_t)eyesMode);
        menuRequestFullRedraw();
        break;
      }

      case 52: {
        // RAM dashboard: encoder press resets audio peak counters
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
        // Toggle SPKR: OFF <-> ON
        extern bool getSpkrEnabled();
        extern void setSpkrEnabled(bool enabled);
        bool currentState = getSpkrEnabled();
        bool newState = !currentState;
        setSpkrEnabled(newState);
        saveSingleModeToEEPROM(27, (int8_t)(newState ? 1 : 0));
        menuRequestFullRedraw();  // Force VOL submenu to redraw with new state
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
        // COLR - Color scheme selection: toggle through 0->1->2->3->0
        // 0 = default, 1 = scheme 1, 2 = scheme 2, 3 = custom from scheme.txt
        
        // Preserve ETC page position (COLR is page 4) - don't overwrite with scheme number
        extern int currentEtcPage;
        int savedEtcPage = currentEtcPage;  // Save current ETC page (should be 4 for COLR)
        
        currentColorScheme = (currentColorScheme + 1) % 4;  // 0,1,2,3 -> wraps to 0
        
        // If scheme 3 is selected, load from scheme.txt if it exists
        if (currentColorScheme == 3) {
          if (SD.exists("scheme.txt")) {
            File schemeFile = SD.open("scheme.txt", FILE_READ);
            if (schemeFile && schemeFile.size() >= 90) {
              uint8_t buf[90];
              int bytesRead = schemeFile.read(buf, 90);
              schemeFile.close();
              
              if (bytesRead == 90) {
                // Load col[] array (Note ON colors) - bytes 0-44
                for (int i=0; i<15; i++) {
                  col[i] = CRGB(buf[i*3], buf[i*3+1], buf[i*3+2]);
                }
                
                // Load col_base[] array (Note OFF / Base colors) - bytes 45-89
                for (int i=0; i<15; i++) {
                  int baseIdx = 45 + i*3;
                  col_base[i] = CRGB(buf[baseIdx], buf[baseIdx+1], buf[baseIdx+2]);
                }
                
                // Set flag to invalidate color cache
                extern volatile bool colorsUpdatedViaSerial;
                colorsUpdatedViaSerial = true;
                
              } else {
                // File incomplete, fall back to scheme 0
                currentColorScheme = 0;
                applyColorScheme(0);
              }
            } else {
              // File doesn't exist or too small, fall back to scheme 0
              currentColorScheme = 0;
              applyColorScheme(0);
            }
          } else {
            // File doesn't exist, fall back to scheme 0
            currentColorScheme = 0;
            applyColorScheme(0);
          }
        } else {
          // Use normal scheme loading for 0, 1, 2
          applyColorScheme(currentColorScheme);
          
          // Set flag to invalidate color cache
          extern volatile bool colorsUpdatedViaSerial;
          colorsUpdatedViaSerial = true;
        }
        
        // Save scheme selection to EEPROM
        saveSingleModeToEEPROM(22, (int8_t)currentColorScheme);
        
        // Restore ETC page position to stay on COLR page (page 4)
        currentEtcPage = savedEtcPage;
        currentMode->pos[3] = savedEtcPage;  // Restore encoder position for ETC page navigation
        Encoder[3].writeCounter((int32_t)savedEtcPage);
        
        menuRequestFullRedraw();  // Force redraw to show new scheme number
        break;
      }

      case 42: {
        // BATT - read-only, encoder press does nothing
        break;
      }
    }
    //saveMenutoEEPROM();
}

// New functions for menu page navigation

// Reset NEW mode state when exiting
FLASHMEM void resetNewModeState() {
  newScreenFirstEnter = true;
}

// Show NEW screen when creating a new file via DAT

// Simple Notes View functions
FLASHMEM void drawSimpleNotesView() {
  if (simpleNotesView != 1 && simpleNotesView != 2) simpleNotesView = 1;
  drawMenuValue(simpleNotesView == 1 ? "EASY" : "FULL", 2, 3, simpleNotesView == 1 ? CRGB(0, 255, 0) : CRGB(0, 0, 255));
}

FLASHMEM void drawLoopLength() {
  if (loopLength == 0) {
    drawMenuValue("OFF", 2, 3, UI_RED);
  } else {
    // Avoid drawNumber (right-align + nested FastLEDshow).
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
  // Switch to new file mode
  switchMode(&newFileMode);
}

// Show NEW file mode screen
FLASHMEM void showNewFileMode() {
  FastLEDclear();
  
  // Draw page title
  CRGB newColor = CRGB(0, 255, 255);
  drawText("NEW", 6, 12, newColor);
  
  // Draw genre selection
  drawGenreSelection();
  
  // Draw length meter on y=8 in pink (only for non-BLNK genres)
  if (genreType != 0) { // Not BLNK
    int lengthLength = mapf(genreLength, 1, 16, 1, 16);
    for (int x = 1; x <= 16; x++) {
      if (x <= lengthLength) {
        light(x, 10, CRGB(255, 0, 255)); // Pink for length
      } else {
        light(x, 10, CRGB(0, 0, 0));
      }
    }
  } else {
    // Clear the length meter area for BLNK
    for (int x = 1; x <= 16; x++) {
      light(x, 10, CRGB(0, 0, 0));
    }
  }
  
  // NEW controls: enc0=load (green M), enc1=length (pink L), enc2=template (yellow L),
  // enc3=back to FILE (blue L — same as FILE menu enc4). Matrix-only draw so a later
  // LED write cannot reshape the L pixels; no extra helper dots on row 1.
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

  
  // FastLED.setBrightness(ledBrightness);
  FastLEDshow();
  
  // Initialize encoders for genre and length control
  static int lastGenreType = -1;
  static int lastGenreLength = -1;
  
  // Always initialize encoders when entering NEW mode
  if (newScreenFirstEnter) {
    // Template selection on 3rd encoder (encoder 2)
    Encoder[2].writeCounter((int32_t)genreType);
    Encoder[2].writeMax((int32_t)5);
    Encoder[2].writeMin((int32_t)0);
    currentMode->pos[2] = genreType;

    // Length on 2nd encoder (encoder 1) — only for non-BLNK
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

    // 1st encoder: press-only (load); 4th unused
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
  
  // Handle template type (encoder 2 - 3rd encoder)
  int currentGenreType = currentMode->pos[2];
  if (currentGenreType != lastGenreType) {
    genreType = currentGenreType;
    if (genreType > 5) genreType = 5;
    if (genreType < 0) genreType = 0;
    
    // Update encoder to match clamped value
    if (genreType != currentGenreType) {
      Encoder[2].writeCounter((int32_t)genreType);
      currentMode->pos[2] = genreType;
    }
    
    drawGenreSelection();
    
    // Enable/disable length on 2nd encoder based on template
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
  
  // Handle template length (encoder 1 - 2nd encoder) - only for non-BLNK
  if (genreType != 0) {
    int currentGenreLength = currentMode->pos[1];
    if (currentGenreLength != lastGenreLength) {
      genreLength = currentGenreLength;
      if (genreLength > 16) genreLength = 16;
      if (genreLength < 1) genreLength = 1;
      
      // Update encoder to match clamped value
      if (genreLength != currentGenreLength) {
        Encoder[1].writeCounter((int32_t)genreLength);
        currentMode->pos[1] = genreLength;
      }
      
      // Redraw length meter
      int lengthLength = mapf(genreLength, 1, 16, 1, 16);
      for (int x = 1; x <= 16; x++) {
        if (x <= lengthLength) {
          light(x, 10, CRGB(255, 0, 255)); // Pink for length
        } else {
          light(x, 10, CRGB(0, 0, 0));
        }
      }
      FastLEDshow();
      
      lastGenreLength = genreLength;
    }
  }
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

// Helper function to clear text area before drawing new text
// Clears a rectangle at (startX, startY) with width pixels wide and 5 pixels tall (character height)
FLASHMEM void clearTextArea(int startX, int startY, int width) {
  for (int x = startX; x < startX + width && x <= (int)maxX; x++) {
    for (int y = startY; y < startY + 5 && y <= (int)maxY; y++) {
      light(x, y, CRGB(0, 0, 0));
    }
  }
}

// Draw a menu value option at (x,y) - clears area and draws label with color.
// Use for ON/OFF, SENS, etc. style subitem toggles to reduce code duplication.
static inline void drawMenuValue(const char* label, int x, int y, CRGB color) {
  clearTextArea(x, y, 16);
  drawText(label, x, y, menuValueColor(color));
}

FLASHMEM void drawRecChannelClear(){
  extern bool imageMode;
  int mode = imageMode ? 2 : recChannelClear;  // IMG locks CLR to FIX
  const char* lbl; CRGB col;
  switch (mode) {
    case 1: lbl = "ON";   col = UI_GREEN;  SMP_REC_CHANNEL_CLEAR = true;  break;
    case 0: lbl = "OFF"; col = UI_RED;    SMP_REC_CHANNEL_CLEAR = false; break;
    case 2: lbl = "FIX"; col = UI_YELLOW; SMP_REC_CHANNEL_CLEAR = false; break;
    case 3: lbl = "ON1"; col = UI_CYAN;   SMP_REC_CHANNEL_CLEAR = true;  break;
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
// TOUCHY's long encoder-4 gesture opens the existing root menu at MODE.
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
