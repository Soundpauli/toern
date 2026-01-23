// Menu page system - completely independent from maxPages
#define MENU_PAGES_COUNT 10
#define LOOK_PAGES_COUNT 10
#define RECS_PAGES_COUNT 5
#define MIDI_PAGES_COUNT 3
#define VOL_PAGES_COUNT 4
#define ETC_PAGES_COUNT 4

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

// Page definitions - each page contains one main setting + additional features
struct MenuPage {
  const char* name;
  int mainSetting;  // The main setting for this page (menuPosition from old system)
  bool hasAdditionalFeatures;  // Whether this page has extra controls (like mic gain)
  const char* additionalFeatureName;  // Name of additional feature if any
};

MenuPage menuPages[MENU_PAGES_COUNT] = {
  {"DAT", 1, false, nullptr},           // Load/Save
  {"KIT", 2, false, nullptr},           // Sample Pack
  {"WAV", 3, false, nullptr},           // Wave Selection
  {"BPM", 5, false, nullptr},           // BPM/Volume
  {"VOL", 26, false, nullptr},          // VOL submenu (MAIN, LOUT, PREV, 2-CH)
  {"PLAY", 19, false, nullptr},         // PLAY submenu (FLW, PREV, VIEW, PMD, LOOP)
  {"RECS", 20, false, nullptr},         // RECS submenu (INPT, MIC, L-IN, TRIG, CLR)
  {"MIDI", 21, false, nullptr},         // MIDI submenu (CHN, TRANSP)
  {"SONG", 22, false, nullptr},         // Song Mode - arrange patterns into songs
  {"ETC", 34, false, nullptr}           // ETC submenu (AUTO, RST)
};

// LOOK/PLAY submenu pages (PLAY menu uses LOOK submenu)
MenuPage lookPages[LOOK_PAGES_COUNT] = {
  {"FLW", 10, false, nullptr},          // Flow Mode
  {"PREV", 33, false, nullptr},         // Preview trigger mode (ON/PRSS)
  {"VIEW", 17, false, nullptr},         // Simple Notes View
  {"PMD", 9, false, nullptr},           // Pattern Mode
  {"LOOP", 18, false, nullptr},         // Loop Length
  {"CTRL", 25, false, nullptr},         // Encoder control mode
  {"LEDS", 23, false, nullptr},         // LED Modules Count (1 or 2)
  {"PONG", 24, false, nullptr},         // Pong Toggle
  {"CRSR", 32, false, nullptr},         // Cursor Type (NORM/CHNR/BIG)
  {"DRAW", 38, false, nullptr}          // DRAW mode toggle (L+R / R)
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
  {"L-IN", 31, false, nullptr},         // Line input level
  {"TRIG", 11, false, nullptr},         // Fast Rec Mode (On-The-fly Recording)
  {"CLR", 12, false, nullptr}           // Rec Channel Clear
};

// MIDI submenu pages
MenuPage midiPages[MIDI_PAGES_COUNT] = {
  {"CH", 7, false, nullptr},            // MIDI Voice Select (Channel)
  {"TRAN", 8, false, nullptr},         // MIDI Transport
  {"SEND", 13, false, nullptr}         // MIDI Send (CLCK, NOTE, BOTH)
};

// VOL submenu pages
MenuPage volPages[VOL_PAGES_COUNT] = {
  {"MAIN", 27, false, nullptr},         // Headphone output volume
  {"LOUT", 28, false, nullptr},         // Line output volume
  {"PREV", 29, false, nullptr},         // Preview volume
  {"2-CH", 36, false, nullptr}          // Stereo routing: OFF, M+P, or L+R
};

// ETC submenu pages
MenuPage etcPages[ETC_PAGES_COUNT] = {
  {"INFO", 39, false, nullptr},          // Info / version / credits
  {"AUTO", 15, true, "PAGES"},          // AI Song Generation + Page Count
  {"RST", 16, true, "MODE"},             // Reset Effects / SD Rescan (EFX or SD)
  {"LGHT", 40, false, nullptr}           // LED Strip toggle (OFF/ON)
};

// --- INFO page animation state ---
static bool infoPageFirstEnter = true;
static uint32_t infoPageEnterMs = 0;

// Force a full redraw on next menu frame (used when entering/exiting menu/submenus).
static bool menuForceFullRedraw = false;
void menuRequestFullRedraw() {
  menuForceFullRedraw = true;
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
int aiTargetPage = 6; // Default target page for AI song generation
int aiBaseStartPage = 1; // Start of base page range for AI analysis
int aiBaseEndPage = 1;   // End of base page range for AI analysis

// Genre generation variables
int genreType = 0; // 0=BLNK, 1=TECH, 2=HIPH, 3=DNB, 4=HOUS, 5=AMBT
int genreLength = 8; // Default length for genre generation

// NEW mode state management
bool newScreenFirstEnter = true;

// Reset menu option: 0 = EFX reset, 1 = SD rescan, 2 = FULL reset
int resetMenuOption = 0;

// DRAW mode: 0 = L+R (default), 1 = R (right-hand only)
int drawMode = 0;

// --- Settings backup (EEPROM <-> SD) ---
// We back up the menu/settings EEPROM block to a small text file on the SD card.
// This lets you restore settings if EEPROM is empty/corrupt after a brownout.
static const char *SETTINGS_BACKUP_PATH = "settings.txt";
static const char *SETTINGS_BACKUP_TMP_PATH = "settings.tmp";
static const char *SETTINGS_BACKUP_HEADER = "TOERN_SETTINGS_V1";
static const uint16_t SETTINGS_EEPROM_BLOCK_LEN = 26; // EEPROM_DATA_START + [0..25]
static bool settingsBackupDirty = false;
static uint32_t settingsBackupDirtyMs = 0;
static const uint32_t SETTINGS_BACKUP_DEBOUNCE_MS = 1500;

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

static bool writeSettingsBackupToSD() {
  // payload = samplePackID (4 bytes) + menu/settings bytes (EEPROM_DATA_START..+20)
  unsigned int spid = 1;
  EEPROM.get(0, spid);

  uint8_t block[SETTINGS_EEPROM_BLOCK_LEN];
  for (uint16_t i = 0; i < SETTINGS_EEPROM_BLOCK_LEN; i++) {
    block[i] = (uint8_t)EEPROM.read(EEPROM_DATA_START + i);
  }

  uint8_t payload[sizeof(unsigned int) + SETTINGS_EEPROM_BLOCK_LEN];
  memcpy(payload, &spid, sizeof(unsigned int));
  memcpy(payload + sizeof(unsigned int), block, SETTINGS_EEPROM_BLOCK_LEN);
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

static bool readSettingsBackupFromSD(unsigned int &outSamplePackID, uint8_t *outBlock, size_t outBlockLen) {
  if (!SD.exists(SETTINGS_BACKUP_PATH)) return false;
  File f = SD.open(SETTINGS_BACKUP_PATH, FILE_READ);
  if (!f) return false;

  char line[1024];
  if (!readLine(f, line, sizeof(line))) { f.close(); return false; }
  if (strcmp(line, SETTINGS_BACKUP_HEADER) != 0) { f.close(); return false; }

  if (!readLine(f, line, sizeof(line))) { f.close(); return false; }
  if (strncmp(line, "LEN=", 4) != 0) { f.close(); return false; }
  unsigned int len = (unsigned int)atoi(line + 4);

  if (!readLine(f, line, sizeof(line))) { f.close(); return false; }
  if (strncmp(line, "CRC=", 4) != 0) { f.close(); return false; }
  uint32_t expectedCrc = (uint32_t)strtoul(line + 4, nullptr, 16);

  if (!readLine(f, line, sizeof(line))) { f.close(); return false; }
  if (strncmp(line, "DATA=", 5) != 0) { f.close(); return false; }
  const char *hex = line + 5;

  // Backward compatible: allow older backups with smaller EEPROM blocks.
  const size_t minLen = sizeof(unsigned int) + 1; // at least 1 byte of settings
  const size_t maxLen = sizeof(unsigned int) + SETTINGS_EEPROM_BLOCK_LEN;
  if (len < minLen || len > maxLen) { f.close(); return false; }
  if (strlen(hex) < (size_t)len * 2) { f.close(); return false; }

  uint8_t payload[len];
  if (!decodeHexBytes(hex, payload, (size_t)len)) { f.close(); return false; }
  uint32_t crc = crc32_compute(payload, (size_t)len);
  if (crc != expectedCrc) { f.close(); return false; }

  // Unpack
  memcpy(&outSamplePackID, payload, sizeof(unsigned int));
  if (outBlockLen < SETTINGS_EEPROM_BLOCK_LEN) { f.close(); return false; }
  memset(outBlock, 0, SETTINGS_EEPROM_BLOCK_LEN);
  size_t bytesInFile = (size_t)len - sizeof(unsigned int);
  if (bytesInFile > SETTINGS_EEPROM_BLOCK_LEN) bytesInFile = SETTINGS_EEPROM_BLOCK_LEN;
  memcpy(outBlock, payload + sizeof(unsigned int), bytesInFile);
  f.close();
  return true;
}

void serviceSettingsBackup() {
  if (!settingsBackupDirty) return;
  if ((uint32_t)(millis() - settingsBackupDirtyMs) < SETTINGS_BACKUP_DEBOUNCE_MS) return;
  // Best effort: if SD write fails, keep dirty flag and try later.
  if (writeSettingsBackupToSD()) {
    settingsBackupDirty = false;
  }
}

void loadMenuFromEEPROM() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
    // EEPROM "empty" - try SD backup first, otherwise initialize defaults
    unsigned int restoredPack = 1;
    uint8_t restoredBlock[SETTINGS_EEPROM_BLOCK_LEN];
    bool restored = readSettingsBackupFromSD(restoredPack, restoredBlock, sizeof(restoredBlock));

    if (restored) {
      Serial.println("EEPROM empty - restoring settings from settings.txt");
      EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
      EEPROM.put(0, restoredPack);
      for (uint16_t i = 0; i < SETTINGS_EEPROM_BLOCK_LEN; i++) {
        EEPROM.write(EEPROM_DATA_START + i, restoredBlock[i]);
      }

      // Keep runtime in sync for the rest of boot (loadSamplePack uses samplePackID)
      extern unsigned int samplePackID;
      samplePackID = restoredPack;
      SMP.pack = restoredPack;
    } else {
      // first run! write magic + defaults
      Serial.println("First run detected - initializing EEPROM with defaults");
      EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
      EEPROM.put(0, (unsigned int)1);            // samplePackID default (1)
      // Default to LINEIN so the SGTL5000 MIC/capture path is off by default.
      EEPROM.write(EEPROM_DATA_START + 0, -1);   // recMode default
      EEPROM.write(EEPROM_DATA_START + 1,  1);   // clockMode default
      EEPROM.write(EEPROM_DATA_START + 2,  1);   // transportMode default
      EEPROM.write(EEPROM_DATA_START + 3,  1);   // patternMode default
      EEPROM.write(EEPROM_DATA_START + 4,  1);   // voiceSelect default
      EEPROM.write(EEPROM_DATA_START + 5,  1);   // fastRecMode default
      EEPROM.write(EEPROM_DATA_START + 6,  1);   // recChannelClear default
      EEPROM.write(EEPROM_DATA_START + 7,  20);   // previewVol default (0-50 range, middle = 20)
      EEPROM.write(EEPROM_DATA_START + 8, -1);   // flowMode default (OFF)
      EEPROM.write(EEPROM_DATA_START + 9, 10);   // micGain default (10)
      EEPROM.write(EEPROM_DATA_START + 11, 1);   // simpleNotesView default (1 = EASY)
      EEPROM.write(EEPROM_DATA_START + 12, 0);   // loopLength default (0 = OFF)
      EEPROM.write(EEPROM_DATA_START + 13, 1);   // ledModules default (1)
      EEPROM.write(EEPROM_DATA_START + 14, 0);   // ctrlMode default (0 = PAGE)
      EEPROM.write(EEPROM_DATA_START + 15, 30);  // lineOutLevelSetting default (30)
      EEPROM.write(EEPROM_DATA_START + 16, 8);   // lineInLevel default (8)
      EEPROM.write(EEPROM_DATA_START + 17, 100);  // GLOB.vol default (100, maps to 1.0 volume, 0-100 range)
      EEPROM.write(EEPROM_DATA_START + 18, 0);   // cursorType default (0 = NORM)
      EEPROM.write(EEPROM_DATA_START + 19, 0);   // showChannelNr default (0 = false, NORM mode)
      EEPROM.write(EEPROM_DATA_START + 20, 0);   // previewTriggerMode default (ON)
      EEPROM.write(EEPROM_DATA_START + 21, 0);   // drawMode default (0 = L+R)
      EEPROM.write(EEPROM_DATA_START + 23, 0);   // stereoChannel default (OFF)
      EEPROM.write(EEPROM_DATA_START + 24, 2);   // midiSendMode default (2 = BOTH)
      EEPROM.write(EEPROM_DATA_START + 25, 1);   // ledStripEnabled default (1 = ON)

      Serial.println("EEPROM initialized with defaults.");

      // Create/refresh SD backup for defaults (debounced)
      markSettingsBackupDirty();
    }
  }

  // now pull them in
  recMode       = (int8_t) EEPROM.read(EEPROM_DATA_START + 0);
  clockMode     = (int8_t) EEPROM.read(EEPROM_DATA_START + 1);
  transportMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 2);
  patternMode   = (int8_t) EEPROM.read(EEPROM_DATA_START + 3);
  voiceSelect   = (int8_t) EEPROM.read(EEPROM_DATA_START + 4);
  fastRecMode   = (int8_t) EEPROM.read(EEPROM_DATA_START + 5);
  recChannelClear   = (int8_t) EEPROM.read(EEPROM_DATA_START + 6);
  previewVol   = (int8_t) EEPROM.read(EEPROM_DATA_START + 7);
  flowMode     = (int8_t) EEPROM.read(EEPROM_DATA_START + 8);
  micGain     = (int8_t) EEPROM.read(EEPROM_DATA_START + 9);
  simpleNotesView = (int) EEPROM.read(EEPROM_DATA_START + 11);
  loopLength = (int) EEPROM.read(EEPROM_DATA_START + 12);
  ledModules = (int) EEPROM.read(EEPROM_DATA_START + 13);
  ctrlMode = (int8_t) EEPROM.read(EEPROM_DATA_START + 14);
  
  // Load lineInLevel from EEPROM (stored at EEPROM_DATA_START + 16)
  extern unsigned int lineInLevel;
  lineInLevel = (unsigned int)EEPROM.read(EEPROM_DATA_START + 16);
  if (lineInLevel > 15) {
    lineInLevel = 8;  // Default to 8 if invalid
    EEPROM.write(EEPROM_DATA_START + 16, lineInLevel);
  }
  
  // Load cursorType from EEPROM (stored at EEPROM_DATA_START + 18)
  extern int cursorType;
  cursorType = (int8_t)EEPROM.read(EEPROM_DATA_START + 18);
  if (cursorType < 0 || cursorType > 1) {
    cursorType = 0;  // Default to 0 (NORM) if invalid
    EEPROM.write(EEPROM_DATA_START + 18, cursorType);
  }
  
  // Load showChannelNr from EEPROM (stored at EEPROM_DATA_START + 19)
  extern bool showChannelNr;
  showChannelNr = (EEPROM.read(EEPROM_DATA_START + 19) != 0);
  
  // Load drawMode from EEPROM (stored at EEPROM_DATA_START + 21)
  drawMode = (int8_t)EEPROM.read(EEPROM_DATA_START + 21);
  if (drawMode < 0 || drawMode > 1) {
    drawMode = 0;  // Default to 0 (L+R) if invalid
    EEPROM.write(EEPROM_DATA_START + 21, drawMode);
  }

  // Load stereoChannel from EEPROM (stored at EEPROM_DATA_START + 23)
  // 0=OFF, 1=M+P, 2=L+R
  // Convert old values for backward compatibility (conversion happens in menu first enter)
  extern int8_t stereoChannel;
  stereoChannel = (int8_t)EEPROM.read(EEPROM_DATA_START + 23);
  if (stereoChannel < 0 || stereoChannel > 8) {
    // Invalid value, default to OFF
    stereoChannel = 0;
    EEPROM.write(EEPROM_DATA_START + 23, stereoChannel);
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
    EEPROM.write(EEPROM_DATA_START + 24, midiSendMode);
  }
  
  // Load ledStripEnabled from EEPROM (stored at EEPROM_DATA_START + 25)
  extern bool getLedStripEnabled();
  extern void setLedStripEnabled(bool enabled);
  uint8_t ledStripValue = EEPROM.read(EEPROM_DATA_START + 25);
  bool ledStripEnabled = (ledStripValue != 0);
  if (ledStripValue > 1) {
    // Invalid value, default to ON
    ledStripEnabled = true;
    EEPROM.write(EEPROM_DATA_START + 25, 1);
  }
  setLedStripEnabled(ledStripEnabled);
  
  if (previewVol < 0 || previewVol > 50) previewVol = 20;
  
  // Safety: Ensure monitoring is OFF on startup to prevent feedback
  mixer_end.gain(3, 0.0);
  
  // Update maxX based on loaded ledModules value
  extern unsigned int maxX;
  maxX = MATRIX_WIDTH * ledModules;
  
  if (recChannelClear < 0 || recChannelClear > 3) recChannelClear = 1;  // Default to ON if invalid
  
  // Ensure flowMode is valid (-1 or 1)
  if (flowMode != -1 && flowMode != 1) {
    flowMode = -1;  // Default to OFF if invalid value
  }
  
  // Ensure simpleNotesView is valid (1-2)
  if (simpleNotesView < 1 || simpleNotesView > 2) {
    simpleNotesView = 1;  // Default to EASY if invalid value
  }
  
  if (transportMode != -1 && transportMode != 1 && transportMode != 2) {
    transportMode = -1;  // Default to OFF if invalid value
  }

  // Ensure loopLength is valid (0-8)
  if (loopLength < 0 || loopLength > 8) {
    loopLength = 0;  // Default to OFF if invalid value
  }
  
  // Ensure voiceSelect is valid (-1, 1, or 2)
  if (voiceSelect != -1 && voiceSelect != 1 && voiceSelect != 2) {
    voiceSelect = 1;  // Default to MIDI if invalid value
  }
  
  // Ensure patternMode is valid (-1, 1, or 2)
  if (patternMode != -1 && patternMode != 1 && patternMode != 2 && patternMode != 3) {
    patternMode = -1;  // Default to OFF if invalid value
  }
  
  // Ensure ledModules is valid (1 or 2)
  if (ledModules < 1 || ledModules > 2) {
  if (ctrlMode != 0 && ctrlMode != 1) {
    ctrlMode = 0;
  }

    Serial.print("Invalid ledModules value: ");
    Serial.print(ledModules);
    Serial.println(" - defaulting to 1");
    ledModules = 1;  // Default to 1 if invalid value
    EEPROM.write(EEPROM_DATA_START + 13, 1);  // Save corrected value
  }

  Serial.println("Loaded Menu values from EEPROM:");
  Serial.print("  ledModules="); Serial.print(ledModules); Serial.print(" (maxX="); Serial.print(maxX); Serial.println(")");
  Serial.print("  samplePackID="); Serial.println(samplePackID);
  Serial.print("  recMode="); Serial.println(recMode);
  Serial.print("  clockMode="); Serial.println(clockMode);
  Serial.print("  patternMode="); Serial.println(patternMode);
  Serial.print("  loopLength="); Serial.println(loopLength);

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
  
  // Apply mic gain only when MIC is selected; otherwise force 0 to keep MIC path off.
  if (recInput == AUDIO_INPUT_MIC) {
    sgtl5000_1.micGain(micGain);
  } else {
    sgtl5000_1.micGain(0);
  }

  // Note: These draw functions are for menu display, not startup initialization.
  // They should only be called when actually displaying the menu, not during setup().
  // The initialization logic (loading values from EEPROM) is complete above.
  // Drawing during startup causes brief text flashes (e.g., "MIC" before "KIT:").
  // drawRecMode();
  // drawClockMode();
  // drawMidiTransport();
  // drawPatternMode();
  // drawFlowMode();
  // drawMidiVoiceSelect();
  // drawFastRecMode();
  // drawRecChannelClear();
  
  // Apply input selection from recMode.
  extern unsigned int recInput;
  sgtl5000_1.inputSelect(recInput);
}

// Apply all audio-related global variables to hardware
void applyAudioSettingsFromGlobals() {
  // GLOB, lineOutLevelSetting, previewVol, micGain, and lineInLevel are already in scope as globals
  extern struct GlobalVars GLOB;
  extern uint8_t lineOutLevelSetting;
  extern unsigned int previewVol;
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
  
  // Apply preview volume
  updatePreviewVolume();
}

// call this after you change *any* one of the six modes in switchMenu():
void saveSingleModeToEEPROM(int index, int8_t value) {
  EEPROM.write(EEPROM_DATA_START + index, (uint8_t)value);
  markSettingsBackupDirty();
}

// Save samplepack 0 state to EEPROM (which voices are using sp0)
void saveSp0StateToEEPROM() {
  Serial.println("=== Saving SP0 State to EEPROM ===");
  // Use addresses 200-208 for sp0 state (8 voices = 8 bytes)
  for (int i = 1; i < maxFiles; i++) {
    uint8_t value = SMP.sp0Active[i] ? 1 : 0;
    EEPROM.write(200 + i, value);
    Serial.print("Voice ");
    Serial.print(i);
    Serial.print(": sp0Active = ");
    Serial.print(SMP.sp0Active[i] ? "TRUE" : "FALSE");
    Serial.print(", saving ");
    Serial.println(value);
  }
  Serial.println("=== SP0 State Saved ===");
}

// Load samplepack 0 state from EEPROM
void loadSp0StateFromEEPROM() {
  Serial.println("=== Loading SP0 State from EEPROM ===");
  for (int i = 1; i < maxFiles; i++) {
    uint8_t stored = EEPROM.read(200 + i);
    SMP.sp0Active[i] = (stored == 1);
    Serial.print("Voice ");
    Serial.print(i);
    Serial.print(": EEPROM value = ");
    Serial.print(stored);
    Serial.print(", sp0Active = ");
    Serial.println(SMP.sp0Active[i] ? "TRUE" : "FALSE");
  }
  Serial.println("=== SP0 State Loaded ===");
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
  
  // Always set encoder limits to ensure they match current MENU_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(MENU_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
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
    drawIndicator('L', 'G', 4);  // Encoder 4: menu navigation

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
      Encoder[0].writeRGBCode(0x000000);
      Encoder[1].writeRGBCode(0x000000);
      Encoder[2].writeRGBCode(0x000000);
      Encoder[3].writeRGBCode(0x000000);
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
  // Match the parent menu color (PLAY).
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
  
  // Default: encoder 4 ring matches submenu text color
  CRGB indicatorColor = currentMenuParentTextColor();
  Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
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
  
  // Always set encoder limits to ensure they match current LOOK_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(LOOK_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
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
  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    menuRequestFullRedraw();
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
  if (mainSetting == 4 && recMode == 1) {
    // REC page in MIC mode: no encoder indicators (encoder(2) removed)
    CRGB orangeColor = currentMenuParentTextColor();
    Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
    Encoder[2].writeRGBCode(0x000000); // Black (no indicator - removed)
    Encoder[3].writeRGBCode(orangeColor.r << 16 | orangeColor.g << 8 | orangeColor.b);
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
  
  // Always set encoder limits to ensure they match current RECS_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(RECS_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
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
  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    menuRequestFullRedraw();
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

  // Submenu indicator (encoder 4) should always match submenu text color.
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
  
  // Default: encoder 4 ring matches submenu text color
  CRGB indicatorColor = currentMenuParentTextColor();
  Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[2].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[3].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);

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

  // Handle page navigation with encoder 3
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
  
  // Always set encoder limits to ensure they match current MIDI_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(MIDI_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
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
  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    menuRequestFullRedraw();
    lastPosHash = h;
  }
}

FLASHMEM void showVolMenu() {
  static uint32_t lastPosHash = 0;
  // Redraw only when page/setting changes (or a value changed).
  int pageIndex = currentVolPage;
  MenuPage* currentPageInfo = &volPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedVolPage = -1;
  static int lastRenderedVolSetting = -1;
  const bool fullRedraw = takeMenuForceFullRedraw() || (pageIndex != lastRenderedVolPage) || (mainSetting != lastRenderedVolSetting);

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
  
  // Set encoder colors based on page (will be set in drawMainSettingStatus for each page)
  // Default: only show page-nav ring on encoder 4 (matches parent text color)
  CRGB indicatorColor = currentMenuParentTextColor();
  Encoder[0].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[1].writeRGBCode(0x000000); // Black (no indicator)
  Encoder[2].writeRGBCode(0x000000); // Black (no indicator) - will be set per page
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
  
  // Always set encoder limits to ensure they match current VOL_PAGES_COUNT
  Encoder[3].writeMax((int32_t)(VOL_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  
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
  uint32_t h = hashEncoderPositions(currentMode);
  if (h != lastPosHash) {
    menuRequestFullRedraw();
    lastPosHash = h;
  }
}

FLASHMEM void showEtcMenu() {
  static uint32_t lastPosHash = 0;
  // Redraw only when page/setting changes, except INFO which animates continuously.
  int pageIndex = currentEtcPage;
  MenuPage* currentPageInfo = &etcPages[pageIndex];
  int mainSetting = currentPageInfo->mainSetting;

  static int lastRenderedEtcPage = -1;
  static int lastRenderedEtcSetting = -1;
  bool fullRedraw = (pageIndex != lastRenderedEtcPage) || (mainSetting != lastRenderedEtcSetting);
  if (takeMenuForceFullRedraw()) fullRedraw = true;
  if (mainSetting == 39) fullRedraw = true;  // INFO animates

  if (fullRedraw) {
    FastLEDclear();

    // Draw page indicator as a line at y=maxY (right-aligned)
    // Match the parent menu color (ETC).
    const CRGB parent = currentMenuParentTextColor();
    const CRGB parentDim = dimIconColorFromText(parent);
    int startX = (int)maxX - ETC_PAGES_COUNT + 1;
    if (startX < 1) startX = 1;
    for (int i = 0; i < ETC_PAGES_COUNT; i++) {
      CRGB indicatorColor = (i == pageIndex) ? parent : parentDim;
      light(startX + i, maxY, indicatorColor);
    }

    // Indicators / encoder ring colors
    // For AUTO (mainSetting 15): encoder(0) is the green "generate" trigger; no blue exit indicator on encoder(3).
    // Otherwise keep ETC parent-colored encoder(3) ring (page nav).
    if (mainSetting == 15) {
      drawIndicator('L', 'G', 1);  // Encoder(0): Large Green (generate)
      // Keep encoder(1)/(2) indicators from AI page itself (drawMainSettingStatus/drawAdditionalFeatures)
      Encoder[0].writeRGBCode(0x00FF00);
      Encoder[1].writeRGBCode(getIndicatorColor('Y').r << 16 | getIndicatorColor('Y').g << 8 | getIndicatorColor('Y').b);
      Encoder[2].writeRGBCode(getIndicatorColor('W').r << 16 | getIndicatorColor('W').g << 8 | getIndicatorColor('W').b);
      // Page-nav indicator (encoder 4) should always match ETC text color (e.g. "RSET")
      drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);
    } else {
      // ETC submenu: page-nav indicator should always match ETC text color (e.g. "RSET")
      drawLargeIndicatorCustom(currentMenuParentTextColor(), 4);
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
  
  Encoder[3].writeMax((int32_t)(ETC_PAGES_COUNT - 1));
  Encoder[3].writeMin((int32_t)0);
  if (currentMode->pos[3] != lastEtcPagePosition) {
    currentEtcPage = currentMode->pos[3];
    if (currentEtcPage >= ETC_PAGES_COUNT) currentEtcPage = ETC_PAGES_COUNT - 1;
    if (currentEtcPage < 0) currentEtcPage = 0;
    lastEtcPagePosition = currentEtcPage;
  }

  const bool didRedraw = handleAdditionalFeatureControls(mainSetting);
  if (!fullRedraw && didRedraw) {
    // FastLED.setBrightness(ledBrightness);
    FastLEDshow();
  }

  // Mark render dirty on any encoder movement while in submenu (except INFO which animates).
  if (mainSetting != 39) {
    uint32_t h = hashEncoderPositions(currentMode);
    if (h != lastPosHash) {
      menuRequestFullRedraw();
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
  extern const CRGB col[];
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
  // PLAY=col[6], RECS=col[7], MIDI=col[8], VOL=col[5], ETC=col[14]
  if (inLookSubmenu) return menuTextColorFromCol(6);
  if (inRecsSubmenu) return menuTextColorFromCol(7);
  if (inMidiSubmenu) return menuTextColorFromCol(8);
  if (inVolSubmenu)  return menuTextColorFromCol(5);
  if (inEtcSubmenu)  return menuTextColorFromCol(14);
  return UI_WHITE;
}

// Draw a "large" top-row indicator and set encoder ring to an arbitrary color.
// This is used where we want the indicator color to match submenu text colors (palette-derived),
// not one of the fixed indicator color codes.
static inline void drawLargeIndicatorCustom(CRGB color, int encoderNum) {
  int x1, x2, x3;
  switch (encoderNum) {
    case 1: x1 = 1;  x2 = 2;  x3 = 3;  break;
    case 2: x1 = 5;  x2 = 6;  x3 = 7;  break;
    case 3: x1 = 9;  x2 = 10; x3 = 11; break;
    case 4: x1 = 13; x2 = 14; x3 = 15; break;
    default: return;
  }
  light(x1, 1, color);
  light(x2, 1, color);
  light(x3, 1, color);

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
        drawText("FILE", 2, 3, tc);
      }
      break;
      
    case 2: // KIT - Sample Pack
      // Pack = pack
      {
        const CRGB tc = menuTextColorFromCol(2);
        showIcons(OLD_ICON_SAMPLEPACK, dimIconColorFromText(tc));
        drawText("PACK", 2, 3, tc);
      }
      break;
      
    case 3: // WAV - Wave Selection
      // wave = sample
      {
        const CRGB tc = menuTextColorFromCol(3);
        showIcons(ICON_SAMPLE_BIG, dimIconColorFromText(tc));
      if (GLOB.currentChannel > 0 && GLOB.currentChannel < 9) {
        drawText("WAVE", 2, 3, tc);
      } else {
        drawText("(-)", 2, 3, tc);
      }
      }
      break;
      
    case 4: // REC - Recording Mode (menu/mic)
      // Icon should show in main menu entry (RECS), not inside the REC submenu page.
      // No red indicator for encoder(2) - removed
      drawRecMode();
      break;
      
    case 5: // BPM - BPM/Volume
      // BPM = Clock
      {
        const CRGB tc = menuTextColorFromCol(4);
        showIcons(ICON_CLOCK, dimIconColorFromText(tc));
        drawText("BPM", 2, 3, tc);
      }
      break;
      
    case 7: // CHN - MIDI Voice Select
      drawText("CH", 2, 10, currentMenuParentTextColor());
      drawMidiVoiceSelect();
      break;
      
    case 8: // TRN - MIDI Transport
      drawText("TRAN", 2, 10, currentMenuParentTextColor());
      drawMidiTransport();
      break;
      
    case 13: // SEND - MIDI Send (CLCK, NOTE, BOTH)
      drawText("SEND", 2, 10, currentMenuParentTextColor());
      drawMidiSend();
      break;
      
    case 9: // PMD - Pattern Mode
      drawText("PMODE", 2, 10, currentMenuParentTextColor());
      drawPatternMode();
      break;
      
    case 10: // FLW - Flow Mode
      drawText("FLOW", 2, 10, currentMenuParentTextColor());
      drawFlowMode();
      break;
      
    case 11: // OTR - Fast Rec Mode
      drawText("TRIG", 2, 10, currentMenuParentTextColor());
      drawFastRecMode();
      break;
      
    case 12: // CLR - Rec Channel Clear
      drawText("CLR", 2, 10, currentMenuParentTextColor());
      drawRecChannelClear();
      break;
      
    case 15: // AI - Song Generation
      drawText("AUTO", 2, 10, currentMenuParentTextColor());
      // AUTO generate: encoder(0) press triggers generation -> green indicator on encoder(0)
      drawIndicator('L', 'G', 1);  // Encoder(0): Large Green (generate)
      drawIndicator('L', 'Y', 2);  // Encoder 2: Large Yellow (base start)
      drawIndicator('L', 'W', 3);  // Encoder 3: Large White (base end)
      break;
      
    case 16: // RST - Reset
      drawText("RSET", 2, 10, currentMenuParentTextColor());
      /*if (resetMenuOption == 0) {
        drawText("EFX", 2, 3, CRGB(100, 0, 0));  // Dark Red
      } else {
        drawText("FULL", 2, 3, CRGB(100, 0, 0));  // Dark Red
      }*/
      drawIndicator('L', 'O', 3);  // Encoder 3: Large Orange indicator
      
      break;

    case 39: // INFO - Version / credits scroll
      drawEtcInfoPage();
      break;
      
    case 40: // LGHT - LED Strip toggle (OFF/ON)
      {
        extern bool getLedStripEnabled();
        const CRGB tc = currentMenuParentTextColor();
        drawText("LGHT", 2, 10, tc);
        bool enabled = getLedStripEnabled();
        drawText(enabled ? "ON" : "OFF", 2, 3, enabled ? CRGB(0, 255, 0) : CRGB(255, 0, 0));
      }
      break;
      
    case 19: // PLAY - Submenu
      // PLAY = settings icon
      {
        const CRGB tc = menuTextColorFromCol(6);
        showIcons(ICON_SETTINGS_BIG, dimIconColorFromText(tc));
        drawText("PLAY", 2, 3, tc);
      }
      break;
      
    case 20: // RECS - Submenu
      // REC = record (icon shown on main menu entry)
      {
        const CRGB tc = menuTextColorFromCol(7);
        showIcons(ICON_RECORD, dimIconColorFromText(tc));
        drawText("REC", 2, 3, tc);
      }
      break;
      
    case 21: // MIDI - Submenu
      // MIDI = midi
      {
        const CRGB tc = menuTextColorFromCol(8);
        showIcons(ICON_MIDISYNC, dimIconColorFromText(tc));
        drawText("MIDI", 2, 3, tc);
      }
      break;
      
    case 22: // SONG - Song Mode
      // SONG = song
      {
        const CRGB tc = menuTextColorFromCol(13);
        showIcons(ICON_SONG, dimIconColorFromText(tc));
        drawText("SONG", 2, 3, tc);
      }
      break;
      
    case 17: // VIEW - Simple Notes View
      drawText("VIEW", 2, 10, currentMenuParentTextColor());
      drawSimpleNotesView();
      break;
      
    case 18: // LOOP - Loop Length
      drawText("LOOP", 2, 10, currentMenuParentTextColor());
      drawLoopLength();
      break;
      
    case 23: // LEDS - LED Modules Count
      drawText("LEDS", 2, 10, currentMenuParentTextColor());
      drawLedModules();
      break;

    case 24: // PONG toggle
      drawText("PONG", 2, 10, currentMenuParentTextColor());
      // Clear text areas before drawing
      clearTextArea(2, 3, 8);  // Clear speed text area
      clearTextArea(6, 3, 8);  // Clear "OFF" area
      clearTextArea(10, 3, 8); // Clear "ON" area
      if (pong) {
        drawText("ON", 10, 3, UI_GREEN);
      } else {
        drawText("OFF", 6, 3, UI_RED);
      }
      if (pong) {
        char speedText[6];
        int speedValue = pongIntervalToSpeed(pongUpdateInterval);
        snprintf(speedText, sizeof(speedText), "%02d", speedValue);
        drawText(speedText, 2, 3, CRGB(0, 150, 255));
        {
          CRGB indicatorColor = getIndicatorColor('B');
          Encoder[2].writeRGBCode(indicatorColor.r << 16 | indicatorColor.g << 8 | indicatorColor.b);
        }
        drawIndicator('L', 'B', 3);
      } else {
        Encoder[2].writeRGBCode(0x000000);
      }
      break;

    case 34: // ETC - Submenu
      {
        const CRGB tc = menuTextColorFromCol(14);
        showIcons(ICON_PATTERN, dimIconColorFromText(tc));
        drawText("ETC", 2, 3, tc);
      }
      break;
      
    case 38: // DRAW - Toggle between L+R and R modes
      drawText("DRAW", 2, 10, currentMenuParentTextColor());
      // Clear text area before drawing (3 chars max: "L+R", 1 char: "R")
      clearTextArea(2, 3, 16);
      if (drawMode == 0) {
        drawText("L+R", 2, 3, UI_GREEN);
      } else {
        drawText("R", 2, 3, UI_GREEN);
      }
      break;

    case 25: { // CTRL - encoder behaviour
      drawText("CTRL", 2, 10, currentMenuParentTextColor());
      // Clear text area before drawing (4 chars max: "PAGE", 3 chars: "VOL")
      clearTextArea(2, 3, 16);
      if (ctrlMode == 0) {
        drawText("PAGE", 2, 3, UI_GREEN);
      } else {
        drawText("VOL", 2, 3, UI_ORANGE);
      }
      break;
    }
    
    case 26: // VOL - Submenu
      // VOL = volume
      {
        const CRGB tc = menuTextColorFromCol(5);
        showIcons(ICON_VOLUME_BIG, dimIconColorFromText(tc));
        drawText("VOL", 2, 3, tc);
      }
      break;
      
    case 27: { // MAIN - Headphone output volume
      drawText("MAIN", 2, 10, currentMenuParentTextColor());
      extern Mode *currentMode;
      extern struct GlobalVars GLOB;
      
      // Display GLOB.vol directly as 0-100
      int volDisplay = GLOB.vol;
      // Handle legacy values (0-10) for backward compatibility
      if (volDisplay <= 10) {
        volDisplay = volDisplay * 10;
      }
      char volText[8];
      snprintf(volText, sizeof(volText), "%d", volDisplay);
      drawText(volText, 2, 3, CRGB(55, 10, 0)); // Orange-red gradient
      
      // Dynamic gradient color based on volume level (use float for color calculation)
      float vol = GLOB.vol / 10.0f;
      int r = (int)(vol * 255.0f);
      int g = (int)((1.0f - vol) * 100.0f);
      int b = 0;
      CRGB volColor = CRGB(r, g, b);
      Encoder[2].writeRGBCode(volColor.r << 16 | volColor.g << 8 | volColor.b);
      break;
    }
    
    case 28: { // LOUT - Line output volume
      drawText("LOUT", 2, 10, currentMenuParentTextColor());
      extern uint8_t lineOutLevelSetting;
      char levelText[8];
      // Display mapped range 1..19 (LINEOUT_MIN..LINEOUT_MAX) for user friendliness
      int displayVal = (lineOutLevelSetting - LINEOUT_MIN) + 1;  // 13->1, 31->19
      snprintf(levelText, sizeof(levelText), "%d", displayVal);
      drawText(levelText, 2, 3, CRGB(255, 192, 203)); // Pink
      
      // Pink color for encoder
      CRGB pinkColor = CRGB(255, 192, 203);
      Encoder[2].writeRGBCode(pinkColor.r << 16 | pinkColor.g << 8 | pinkColor.b);
      break;
    }
    
    case 29: { // PREV - Preview volume
      drawText("PREV", 2, 10, currentMenuParentTextColor());
      extern unsigned int previewVol;
      // Display as 0-50 instead of 0.00-0.50
      char volText[8];
      snprintf(volText, sizeof(volText), "%d", previewVol);
      drawText(volText, 2, 3, CRGB(0, 160, 80)); // Green
      
      // Green color for encoder
      CRGB greenColor = CRGB(0, 160, 80);
      Encoder[2].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
      break;
    }
    
    case 30: { // MIC - Microphone gain
      drawText("MIC", 2, 10, currentMenuParentTextColor());
      extern unsigned int micGain;
      char gainText[8];
      snprintf(gainText, sizeof(gainText), "%d", micGain);
      drawText(gainText, 2, 3, CRGB(55, 0, 0)); // Red
      
      // Red color for encoder
      CRGB redColor = CRGB(55, 0, 0);
      Encoder[2].writeRGBCode(redColor.r << 16 | redColor.g << 8 | redColor.b);
      break;
    }
    
    case 31: { // L-IN - Line input level
      drawText("L-IN", 2, 10, currentMenuParentTextColor());
      extern unsigned int lineInLevel;
      char levelText[8];
      snprintf(levelText, sizeof(levelText), "%d", lineInLevel);
      drawText(levelText, 2, 3, CRGB(0, 0, 55)); // Blue
      
      // Blue color for encoder
      CRGB blueColor = CRGB(0, 0, 55);
      Encoder[2].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
      break;
    }

    case 36: { // 2-CH - Stereo routing (OFF, M+P, or L+R)
      drawText("2-CH", 2, 10, currentMenuParentTextColor());
      // Clear text area before drawing (3 chars max: "OFF", "M+P", "L+R")
      clearTextArea(6, 3, 8);
      extern int8_t stereoChannel;
      if (stereoChannel == 0) {
        drawText("OFF", 6, 3, UI_RED);
      } else if (stereoChannel == 1) {
        drawText("M+P", 6, 3, UI_GREEN);
      } else { // stereoChannel == 2
        drawText("L+R", 6, 3, UI_GREEN);
      }
      CRGB blueColor = getIndicatorColor('X');
      Encoder[2].writeRGBCode(blueColor.r << 16 | blueColor.g << 8 | blueColor.b);
      break;
    }
    
    case 32: { // CRSR - Cursor Type
      drawText("CRSR", 2, 10, currentMenuParentTextColor());
      // Clear text area before drawing (4 chars max: "NORM", "CHNR", 3 chars: "BIG")
      clearTextArea(2, 3, 16);
      extern bool showChannelNr;
      extern int cursorType;
      
      // Determine current mode: 0=NORM, 1=CHNR, 2=BIG
      int cursorMode = 0;
      if (showChannelNr && cursorType == 0) {
        cursorMode = 1; // CHNR
      } else if (!showChannelNr && cursorType == 1) {
        cursorMode = 2; // BIG
      } else {
        cursorMode = 0; // NORM
      }
      
      if (cursorMode == 0) {
        drawText("NORM", 2, 3, CRGB(150, 100, 0)); // Dark green
      } else if (cursorMode == 1) {
        drawText("CHNR", 2, 3, CRGB(150, 200, 0)); // Medium green
      } else {
        drawText("BIG", 2, 3, CRGB(150, 255, 0)); // Bright green
      }
      
      // Green color for encoder
      CRGB greenColor = CRGB(0, 255, 0);
      Encoder[2].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
      break;
    }

    case 33: { // PREV - Preview trigger mode
      drawText("PREV", 2, 10, currentMenuParentTextColor());
      // Clear text area before drawing (4 chars max: "PRSS", 2 chars: "ON")
      clearTextArea(2, 3, 16);
      if (previewTriggerMode == PREVIEW_MODE_PRESS) {
        drawText("PRSS", 2, 3, CRGB(0, 150, 255)); // Blue for press-only
      } else {
        drawText("ON", 2, 3, UI_GREEN);
      }
      // Encoder color: green by default
      CRGB greenColor = getIndicatorColor('G');
      Encoder[2].writeRGBCode(greenColor.r << 16 | greenColor.g << 8 | greenColor.b);
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
      
      // Draw "PAGE" text in pink at the bottom
      drawText("PAGE", 2, 3, CRGB(0, 0, 10)); // Pink color
      break;
    }

    case 16: { // RST page - show current mode (EFX, SD, or FULL)
      const char* modeText;
      if (resetMenuOption == 0) {
        modeText = "EFX";
      } else if (resetMenuOption == 1) {
        modeText = "SD";
      } else {
        modeText = "FULL";
      }
      for (int x = 1; x <= 16; x++) {
        light(x, 8, CRGB::Black);
      }
      drawText(modeText, 2, 3, UI_ORANGE);
      FastLEDshow();
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

FLASHMEM bool handleAdditionalFeatureControls(int setting) {
  static bool recMenuFirstEnter = true;
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
    recMenuFirstEnter = true;
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
    lastSetting = setting;
  }
  
  auto redrawMain = [&](int s) {
    drawMainSettingStatus(s);
    didRedraw = true;
  };
  auto redrawAdd = [&](int s) {
    drawAdditionalFeatures(s);
    didRedraw = true;
  };

  switch (setting) {
    case 4: // REC page - Mic Gain control removed (encoder(2) disabled)
      // Encoder(2) functionality removed - no mic gain control in REC menu
      break;
      
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
    
    case 16: { // RST page - Choose between EFX, SD, and FULL reset
      static int lastResetOption = -1;

      if (menuFirstEnter) {
        Encoder[2].writeCounter((int32_t)resetMenuOption);
        Encoder[2].writeMax((int32_t)2);  // 0=EFX, 1=SD, 2=FULL
        Encoder[2].writeMin((int32_t)0);
        menuFirstEnter = false;
      }

      if (currentMode->pos[2] != lastResetOption) {
        resetMenuOption = constrain(currentMode->pos[2], 0, 2);
        Encoder[2].writeCounter((int32_t)resetMenuOption);
        redrawMain(setting);
        lastResetOption = resetMenuOption;
      }
      break;
    }

    case 40: // LGHT - LED Strip toggle (handled by button press in switchMenu)
      // Toggle is handled by encoder(4) press via switchMenu() - no encoder control needed
      break;
    
    case 24: { // PONG page - speed control on encoder 2
      if (!pong) {
        pongMenuFirstEnter = true;
        lastPongSpeed = -1;
        Encoder[2].writeRGBCode(0x000000);
        break;
      }

      const int pongSpeedMin = 1;
      const int pongSpeedMax = 99;

      if (pongMenuFirstEnter) {
        int currentSpeed = pongIntervalToSpeed(pongUpdateInterval);
        Encoder[2].writeCounter(static_cast<int32_t>(currentSpeed));
        Encoder[2].writeMax(static_cast<int32_t>(pongSpeedMax));
        Encoder[2].writeMin(static_cast<int32_t>(pongSpeedMin));
        currentMode->pos[2] = currentSpeed;
        lastPongSpeed = currentSpeed;
        pongMenuFirstEnter = false;
      }

      if (currentMode->pos[2] != lastPongSpeed) {
        int newSpeed = currentMode->pos[2];
        newSpeed = constrain(newSpeed, pongSpeedMin, pongSpeedMax);
        if (newSpeed != lastPongSpeed) {
          uint16_t newInterval = pongSpeedToInterval(newSpeed);
          if (newInterval != pongUpdateInterval) {
            pongUpdateInterval = newInterval;
          }
          redrawMain(setting);
        }
        lastPongSpeed = newSpeed;
        // Keep encoder within bounds in case constrain clipped it
        Encoder[2].writeCounter(static_cast<int32_t>(newSpeed));
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
      
      extern unsigned int previewVol;
      
      if (prevMenuFirstEnter) {
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
      
      if (currentMode->pos[2] != lastPvVol) {
        int encoderPos = constrain(currentMode->pos[2], 0, 50);
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

         default:
       // Reset first enter flags when not on pages with additional features
       recMenuFirstEnter = true;
       aiMenuFirstEnter = true;
       menuFirstEnter = true;
       pongMenuFirstEnter = true;
       break;
  }
  return didRedraw;
}

void switchMenu(int menuPosition){
   switch (menuPosition) {
      case 1:
        switchMode(&loadSaveTrack);
        break;

      case 2:
        switchMode(&set_SamplePack);
        break;

      case 3:
        if (GLOB.currentChannel < 1 || GLOB.currentChannel > 8) return;
        switchMode(&set_Wav);
        // Manifest selection: folder in oldID, file in fileID
        currentMode->pos[1] = (int)SMP.wav[GLOB.currentChannel].oldID;
        currentMode->pos[3] = (int)SMP.wav[GLOB.currentChannel].fileID;
        Encoder[1].writeCounter((int32_t)currentMode->pos[1]);
        Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
        //set encoder to currently Loaded Sample!!
        //Encoder[3].writeCounter((int32_t)((SMP.wav[GLOB.currentChannel][0] * 4) - 1));
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
        
        drawMainSettingStatus(menuPosition);
        drawAdditionalFeatures(menuPosition);
        break;

      case 5:
        switchMode(&volume_bpm);
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
        Serial.print("PMOD changed - songModeActive: ");
        Serial.println(songModeActive ? "TRUE" : "FALSE");
        
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
            // Pattern mode is ON - limit to lastPage
            updateLastPage();
            Encoder[1].writeMax((int32_t)lastPage);
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
                            
        if (fastRecMode>3) fastRecMode=0;
        saveSingleModeToEEPROM(5, fastRecMode);
        drawMainSettingStatus(menuPosition);
        break;

         case 12:
        recChannelClear = recChannelClear + 1;
        if (recChannelClear > 3) recChannelClear = 0;  // Cycle: 0->1->2->3->0 (OFF->ON->FIX->ON1->OFF)
        saveSingleModeToEEPROM(6, recChannelClear);
        drawMainSettingStatus(menuPosition);
        break;

        case 15:
        // AI Song Generation - generate song from current page to target page
        generateSong();
        break;

        case 16:
        Serial.print("=== RSET menu: resetMenuOption=");
        Serial.println(resetMenuOption);
        if (resetMenuOption == 0) {
          // Reset effects/parameters to defaults
          Serial.println("=== RSET: Executing EFX reset ===");
          resetAllToDefaults();
        } else if (resetMenuOption == 1) {
          // SD rescan: Rescan samples folder and update map.txt
          Serial.println("=== SD READ: Rescanning samples folder ===");
          FastLEDclear();
          drawText("SCAN", 2, 3, UI_GREEN);
          FastLEDshow();
          
          // Rescan and write manifest
          extern bool scanAndWriteManifest();
          bool success = scanAndWriteManifest();
          
          if (success) {
            extern bool loadSampleManifest();
            bool loadSuccess = loadSampleManifest();
            FastLEDclear();  // Clear before showing result
            if (loadSuccess) {
              drawText("DONE", 2, 3, UI_GREEN);
              Serial.println("=== SD READ: Scan complete ===");
            } else {
              drawText("FAIL", 2, 3, UI_RED);
              Serial.println("=== SD READ: Scan OK but load failed ===");
            }
          } else {
            FastLEDclear();  // Clear before showing result
            drawText("FAIL", 2, 3, UI_RED);
            Serial.println("=== SD READ: Scan failed ===");
          }
          FastLEDshow();
          delay(1000);
          
          // Close menu and return to draw mode
          extern Mode draw;
          extern void switchMode(Mode*);
          switchMode(&draw);
        } else { // resetMenuOption == 2 (FULL)
          // FULL reset: Complete reset (like startNew) AND rescan SD
          Serial.println("=== RSET: Executing FULL reset ===");
          
          // Call startNew() for complete reset (clears notes, resets all settings, etc.)
          extern void startNew();
          startNew();
          
          // Then rescan SD
          Serial.println("=== SD READ: Rescanning samples folder ===");
          FastLEDclear();
          drawText("SCAN", 2, 3, UI_GREEN);
          FastLEDshow();
          
          // Rescan and write manifest
          extern bool scanAndWriteManifest();
          bool success = scanAndWriteManifest();
          
          if (success) {
            extern bool loadSampleManifest();
            bool loadSuccess = loadSampleManifest();
            FastLEDclear();  // Clear before showing result
            if (loadSuccess) {
              drawText("DONE", 2, 3, UI_GREEN);
              Serial.println("=== SD READ: Scan complete ===");
            } else {
              drawText("FAIL", 2, 3, UI_RED);
              Serial.println("=== SD READ: Scan OK but load failed ===");
            }
          } else {
            FastLEDclear();  // Clear before showing result
            drawText("FAIL", 2, 3, UI_RED);
            Serial.println("=== SD READ: Scan failed ===");
          }
          FastLEDshow();
          delay(1000);
          
          // startNew() already switches to draw mode, so we don't need to do it again
        }
        break;
        
        case 19:
        // Enter LOOK/PLAY submenu at first page
        inLookSubmenu = true;
        currentLookPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        menuRequestFullRedraw();
        break;
        
        case 38:
        // DRAW mode toggle: switch between L+R (0) and R (1)
        drawMode = (drawMode == 0) ? 1 : 0;
        saveSingleModeToEEPROM(21, (int8_t)drawMode);
        drawMainSettingStatus(menuPosition);
        break;
        
        case 32:
        // Cycle through cursor modes: NORM (0) -> CHNR (1) -> BIG (2) -> NORM (0)
        {
          extern bool showChannelNr;
          extern int cursorType;
          
          // Determine current cursor mode
          int cursorMode = 0;
          if (showChannelNr && cursorType == 0) {
            cursorMode = 1; // CHNR
          } else if (!showChannelNr && cursorType == 1) {
            cursorMode = 2; // BIG
          } else {
            cursorMode = 0; // NORM
          }
          
          // Cycle to next mode
          cursorMode = (cursorMode + 1) % 3;
          
          // Apply new mode
          if (cursorMode == 0) {
            // NORM: showChannelNr=false, cursorType=0
            showChannelNr = false;
            cursorType = 0;
          } else if (cursorMode == 1) {
            // CHNR: showChannelNr=true, cursorType=0
            showChannelNr = true;
            cursorType = 0;
          } else {
            // BIG: showChannelNr=false, cursorType=1
            showChannelNr = false;
            cursorType = 1;
          }
          
          // Save to EEPROM
          saveSingleModeToEEPROM(18, cursorType);
          EEPROM.write(EEPROM_DATA_START + 19, showChannelNr ? 1 : 0);
          markSettingsBackupDirty();
          
          drawMainSettingStatus(menuPosition);
        }
        break;
        
        case 20:
        // Enter RECS submenu at first page
        inRecsSubmenu = true;
        currentRecsPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        menuRequestFullRedraw();
        break;
        
        case 21:
        // Enter MIDI submenu at first page
        inMidiSubmenu = true;
        currentMidiPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        menuRequestFullRedraw();
        break;
        
        case 26:
        // Enter VOL submenu at first page
        inVolSubmenu = true;
        currentVolPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
        menuRequestFullRedraw();
        break;

        case 34:
        // Enter ETC submenu at first page
        inEtcSubmenu = true;
        currentEtcPage = 0;
        currentMode->pos[3] = 0;  // Set mode position to match
        Encoder[3].writeCounter((int32_t)0);
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
        
        case 22: {
        // Enter SONG mode
        extern Mode songMode;
        switchMode(&songMode);
        break;
        }
        
        case 23: {
        // Toggle LED modules: 1 or 2
        extern int ledModules;
        extern unsigned int maxX;
        extern Mode draw;
        extern Mode singleMode;
        ledModules = (ledModules == 1) ? 2 : 1;
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
        
        saveSingleModeToEEPROM(13, ledModules);
        drawMainSettingStatus(menuPosition);
        Serial.print("LED Modules set to: ");
        Serial.print(ledModules);
        Serial.print(", maxX now: ");
        Serial.print(maxX);
        Serial.print(", max pages now: ");
        Serial.println(MAX_STEPS / maxX);
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
        // Toggle preview trigger mode: ON -> PRSS -> ON
        previewTriggerMode = (previewTriggerMode == PREVIEW_MODE_ON) ? PREVIEW_MODE_PRESS : PREVIEW_MODE_ON;
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
    }
    //saveMenutoEEPROM();
}

// New functions for menu page navigation
void nextMenuPage() {
  currentMenuPage = (currentMenuPage + 1) % MENU_PAGES_COUNT;
}

void previousMenuPage() {
  currentMenuPage = (currentMenuPage - 1 + MENU_PAGES_COUNT) % MENU_PAGES_COUNT;
}

void goToMenuPage(int page) {
  if (page >= 0 && page < MENU_PAGES_COUNT) {
    currentMenuPage = page;
  }
}

// Function to reset menu state when leaving menu mode
void resetMenuState() {
  // This function can be called when exiting menu mode to reset any state
  // For now, we'll let the static variables handle the reset automatically
}

// Reset NEW mode state when exiting
void resetNewModeState() {
  newScreenFirstEnter = true;
}

// Show NEW screen when creating a new file via DAT

// Simple Notes View functions
void drawSimpleNotesView() {
  // Clear text area before drawing (4 chars max: "EASY", "FULL")
  clearTextArea(2, 3, 16);
  // Show current state: 1=EASY, 2=FULL
  switch (simpleNotesView) {
    case 1:
      drawText("EASY", 2, 3, CRGB(0, 255, 0));
      break;
    case 2:
      drawText("FULL", 2, 3, CRGB(0, 0, 255));
      break;
    default:
      // Default to EASY if invalid value
      simpleNotesView = 1;
      drawText("EASY", 2, 3, CRGB(0, 255, 0));
      break;
  }
}

void drawLoopLength() {
  // Clear text area before drawing (3 chars max: "OFF", or number)
  clearTextArea(2, 3, 16);
  // Show current state: 0=OFF, 1-8=forced length
  if (loopLength == 0) {
    drawText("OFF", 2, 3, CRGB(100, 100, 100));
  } else {
    drawNumber(loopLength, CRGB(0, 200, 255), 3);
  }
}

void drawLedModules() {
  // Show current LED modules count: 1 or 2
  extern int ledModules;
  drawNumber(ledModules, CRGB(0, 255, 0), 3);
}

void showNewFileScreen() {
  // Switch to new file mode
  switchMode(&newFileMode);
}

// Show NEW file mode screen
void showNewFileMode() {
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
  
  // New indicator system: new: | | M[V] | L[N]
  // Encoder 0: no indicator (no functionality)
  // Encoder 1: empty (no indicator)
  
  // Only show L[V] if not BLNK (genreType != 0)
  if (genreType != 0) {
    drawIndicator('L', 'V', 3);  // Encoder 3: Large Violet (only for non-BLNK)
  }
  // Encoder 3: empty if BLNK
  
  // Encoder(3) press starts generation -> match indicator to NEW text color (cyan)
  drawIndicator('L', 'N', 4);

  // Set encoder colors to match indicators
  // Encoder 0: Black (no indicator)
  Encoder[0].writeRGBCode(0x000000);
  // Encoder 1: Black (no indicator)
  Encoder[1].writeRGBCode(0x000000);
  
  // Encoder 3: Large Violet (L[V]) only if genreType != 0 (not BLNK)
  if (genreType != 0) {
    CRGB violetColor = getIndicatorColor('V'); // Violet
    Encoder[2].writeRGBCode(violetColor.r << 16 | violetColor.g << 8 | violetColor.b);
  } else {
    Encoder[2].writeRGBCode(0x000000); // Black when BLNK
  }
  
  // Encoder 4: Large Cyan (L[N]) to match NEW title
  CRGB cyanColor = newColor;
  Encoder[3].writeRGBCode(cyanColor.r << 16 | cyanColor.g << 8 | cyanColor.b);

  
  // FastLED.setBrightness(ledBrightness);
  FastLEDshow();
  
  // Initialize encoders for genre and length control
  static int lastGenreType = -1;
  static int lastGenreLength = -1;
  
  // Always initialize encoders when entering NEW mode
  if (newScreenFirstEnter) {
    // Genre selection by last encoder (encoder 3)
    Encoder[3].writeCounter((int32_t)genreType);
    Encoder[3].writeMax((int32_t)5);
    Encoder[3].writeMin((int32_t)0);
    
    if (genreType != 0) { // Only enable length control for non-BLNK genres
      // Length selection by 3rd encoder (encoder 2)
      Encoder[2].writeCounter((int32_t)genreLength);
      Encoder[2].writeMax((int32_t)16);
      Encoder[2].writeMin((int32_t)1);
    } else {
      // Disable encoder 2 for BLNK
      Encoder[2].writeCounter((int32_t)0);
      Encoder[2].writeMax((int32_t)0);
      Encoder[2].writeMin((int32_t)0);
    }
    
    // Update currentMode positions to match encoders
    currentMode->pos[3] = genreType;
    currentMode->pos[2] = genreType != 0 ? genreLength : 0;
    
    lastGenreType = genreType;
    lastGenreLength = genreLength;
    newScreenFirstEnter = false;
  }
  
  // Handle genre type (encoder 3 - last encoder)
  int currentGenreType = currentMode->pos[3];
  if (currentGenreType != lastGenreType) {
    genreType = currentGenreType;
    if (genreType > 5) genreType = 5;
    if (genreType < 0) genreType = 0;
    
    // Update encoder to match clamped value
    if (genreType != currentGenreType) {
      Encoder[3].writeCounter((int32_t)genreType);
      currentMode->pos[3] = genreType;
    }
    
    drawGenreSelection();
    
    // Update pink helper based on genre type
    if (genreType != 0) { // Not BLNK - show pink helper dot
      light(10, 1, CRGB(255, 0, 255)); // Pink helper dot
    } else { // BLNK - clear pink helper
      light(10, 1, CRGB(0, 0, 0)); // Clear pink helper
    }
    FastLEDshow(); // Update display to show pink helper change
    
    // Update encoder 2 based on genre type
    if (genreType != 0) { // Enable length control for non-BLNK genres
      Encoder[2].writeCounter((int32_t)genreLength);
      Encoder[2].writeMax((int32_t)16);
      Encoder[2].writeMin((int32_t)1);
      currentMode->pos[2] = genreLength;
    } else {
      // Disable encoder 2 for BLNK
      Encoder[2].writeCounter((int32_t)0);
      Encoder[2].writeMax((int32_t)0);
      Encoder[2].writeMin((int32_t)0);
      currentMode->pos[2] = 0;
    }
    
    lastGenreType = genreType;
  }
  
  // Handle genre length (encoder 2 - 3rd encoder) - only for non-BLNK genres
  if (genreType != 0) {
    int currentGenreLength = currentMode->pos[2];
    if (currentGenreLength != lastGenreLength) {
      genreLength = currentGenreLength;
      if (genreLength > 16) genreLength = 16;
      if (genreLength < 1) genreLength = 1;
      
      // Update encoder to match clamped value
      if (genreLength != currentGenreLength) {
        Encoder[2].writeCounter((int32_t)genreLength);
        currentMode->pos[2] = genreLength;
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

// Helper function to get main setting for current menu page
int getCurrentMenuMainSetting() {
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
    return etcPages[currentEtcPage].mainSetting;
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

FLASHMEM void drawRecChannelClear(){
  // Clear text area before drawing (4 chars max: "ON1" or "OFF")
  clearTextArea(2, 3, 16);
  if (recChannelClear == 1) {
    drawText("ON", 2, 3, UI_GREEN);
    SMP_REC_CHANNEL_CLEAR = true;  // Clear mode
  } else if (recChannelClear == 0) {
    drawText("OFF",2, 3, UI_RED);
    SMP_REC_CHANNEL_CLEAR = false; // Add triggers mode
  } else if (recChannelClear == 2) {
    drawText("FIX", 2, 3, UI_YELLOW);
    SMP_REC_CHANNEL_CLEAR = false; // FIX mode - no manipulation
  } else if (recChannelClear == 3) {
    drawText("ON1", 2, 3, UI_CYAN);
    SMP_REC_CHANNEL_CLEAR = true;  // ON1 mode - count-in then record on beat 1
  }
}

FLASHMEM void drawRecMode() {
  // Clear text area before drawing (4 chars max: "LINE")
  clearTextArea(2, 3, 16);
  if (recMode == 1) {
    drawText("MIC", 2, 3, UI_WHITE);
    recInput = AUDIO_INPUT_MIC;
    sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
    sgtl5000_1.micGain(micGain);
  } else {
    drawText("LINE", 2, 3, UI_BLUE);
    recInput = AUDIO_INPUT_LINEIN;
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000_1.micGain(0);
  }


  FastLEDshow();
}

FLASHMEM void drawClockMode() {
  // Clear text area before drawing (3 chars max: "INT" or "EXT")
  clearTextArea(2, 3, 16);
  extern int midiSendMode;
  extern bool MIDI_NOTE_SEND;
  if (clockMode == 1) {
    drawText("INT", 2, 3, UI_GREEN);
    // Update MIDI_CLOCK_SEND based on midiSendMode
    if (midiSendMode == 0) {
      // CLCK only: clock enabled
      MIDI_CLOCK_SEND = true;
      MIDI_NOTE_SEND = false;
    } else if (midiSendMode == 1) {
      // NOTE only: clock disabled
      MIDI_CLOCK_SEND = false;
      MIDI_NOTE_SEND = true;
    } else {
      // BOTH: clock enabled
      MIDI_CLOCK_SEND = true;
      MIDI_NOTE_SEND = true;
    }
  }else{
    drawText("EXT", 2, 3, UI_YELLOW);
    // Update MIDI_CLOCK_SEND based on midiSendMode
    if (midiSendMode == 0) {
      // CLCK only: clock disabled (EXT mode)
      MIDI_CLOCK_SEND = false;
      MIDI_NOTE_SEND = false;
    } else if (midiSendMode == 1) {
      // NOTE only: clock disabled
      MIDI_CLOCK_SEND = false;
      MIDI_NOTE_SEND = true;
    } else {
      // BOTH: clock disabled (EXT mode)
      MIDI_CLOCK_SEND = false;
      MIDI_NOTE_SEND = true;
    }
  }

  FastLEDshow();
}


FLASHMEM void drawMidiVoiceSelect() {
  // Clear text area before drawing (4 chars max: "MIDI", "KEYS", "YPOS")
  clearTextArea(2, 3, 16);
  if (voiceSelect == 1) {
    drawText("MIDI", 2, 3, UI_BLUE);
    MIDI_VOICE_SELECT = true;
  } else if (voiceSelect == 2) {
    drawText("KEYS", 2, 3, UI_MAGENTA);
    MIDI_VOICE_SELECT = true;  // KEYS mode also uses MIDI channel info
  } else {
    drawText("YPOS", 2, 3, UI_GREEN);
     MIDI_VOICE_SELECT = false;
  }

  FastLEDshow();
}



FLASHMEM void drawPreviewVol() {
  int barLength = mapf(previewVol, 0, 50, 1, maxX);
  for (int x = 1; x <= (int)maxX; x++) {
    CRGB color = (x <= barLength) ? CRGB(0, 160, 80) : CRGB(0, 0, 0);
    light(x, 6, color);
    light(x, 7, color);
  }
  char label[8];
  //snprintf(label, sizeof(label), "PV %d", previewVol);  // Display 0-5
  //drawText(label, 2, 3, UI_CYAN);
  FastLEDshow();
}

FLASHMEM void drawFastRecMode() {
  // Clear text area before drawing (4 chars max: "+CON", "-CON", "SENS", "OFF")
  clearTextArea(2, 3, 16);
  if (fastRecMode == 3) {
    drawText("+CON", 2, 3, UI_DIM_BLUE);
    SMP_FAST_REC = 3;
  }


  if (fastRecMode == 2) {
    drawText("-CON", 2, 3, UI_DIM_YELLOW);
    SMP_FAST_REC = 2;
   
  }


  if (fastRecMode == 1) {
    drawText("SENS", 2, 3, UI_GREEN);
    SMP_FAST_REC = 1;
   
  }

  if (fastRecMode == 0) {
    drawText("OFF", 2, 3, UI_RED);
    SMP_FAST_REC = 0;
  }
  FastLEDshow();
}


FLASHMEM void drawPatternMode() {
  // Clear text area before drawing (4 chars max: "SONG", "NEXT", "OFF")
  clearTextArea(2, 3, 16);
  // Also clear area for pending page number if it exists
  clearTextArea(10, 3, 8);
  if (patternMode == 2) {
    drawText("SONG", 2, 3, CRGB(255, 255, 0)); // Yellow for SONG mode
    SMP_PATTERN_MODE = true;
  } else if (patternMode == 3) {
    drawText("NEXT", 2, 3, CRGB(0, 255, 255)); // Cyan for NEXT mode
    SMP_PATTERN_MODE = true;
    // Show pending page if set
    extern unsigned int pendingPage;
    if (pendingPage > 0) {
      char pageText[4];
      snprintf(pageText, sizeof(pageText), "%02d", (int)pendingPage);
      drawText(pageText, 10, 3, CRGB(0, 200, 200)); // Dimmer cyan for page number
    }
  } else if (patternMode == 1) {
    drawText("ON", 2, 3, UI_GREEN);
    SMP_PATTERN_MODE = true;
  } else {
    drawText("OFF", 2, 3, UI_RED);
    SMP_PATTERN_MODE = false;
  }

  FastLEDshow();
}

FLASHMEM void drawFlowMode() {
  // Clear text area before drawing (3 chars max: "OFF", 2 chars: "ON")
  clearTextArea(2, 3, 16);
  if (flowMode == 1) {
    drawText("ON", 2, 3, UI_GREEN);  // Use same coordinates as drawRecChannelClear
    SMP_FLOW_MODE = true;
  } else if (flowMode == -1) {
    drawText("OFF", 2, 3, UI_RED);  // Use same coordinates as drawRecChannelClear
    SMP_FLOW_MODE = false;
  } else {
    // Fallback for any unexpected values
    drawText("OFF", 2, 3, UI_RED);  // Use same coordinates as drawRecChannelClear
    SMP_FLOW_MODE = false;
    flowMode = -1;  // Reset to valid value
  }

  FastLEDshow();
}


FLASHMEM void drawMidiTransport() {
  // Clear text area before drawing (4 chars max: "SEND", 3 chars: "GET", "OFF")
  clearTextArea(2, 3, 16);
  if (transportMode == 2) {
    drawText("SEND", 2, 3, UI_BLUE);
    MIDI_TRANSPORT_RECEIVE = false;
    MIDI_TRANSPORT_SEND = true;
  } else if (transportMode == 1) {
    drawText("GET", 2, 3, UI_GREEN);
    MIDI_TRANSPORT_RECEIVE = true;
    MIDI_TRANSPORT_SEND = false;
  } else {
    drawText("OFF", 2, 3, UI_RED);
    MIDI_TRANSPORT_RECEIVE = false;
    MIDI_TRANSPORT_SEND = false;
    transportMode = -1;
  }

  FastLEDshow();
}

FLASHMEM void drawMidiSend() {
  // Clear text area before drawing (4 chars max: "CLCK", "NOTE", "BOTH")
  clearTextArea(2, 3, 16);
  extern int midiSendMode;
  extern bool MIDI_CLOCK_SEND;
  extern bool MIDI_NOTE_SEND;
  extern int clockMode;
  
  if (midiSendMode == 0) {
    // CLCK only: clock enabled (if clockMode allows), notes disabled
    drawText("CLCK", 2, 3, UI_YELLOW);
    MIDI_CLOCK_SEND = (clockMode == 1);  // Respect clockMode setting
    MIDI_NOTE_SEND = false;
  } else if (midiSendMode == 1) {
    // NOTE only: clock disabled, notes enabled
    drawText("NOTE", 2, 3, UI_GREEN);
    MIDI_CLOCK_SEND = false;
    MIDI_NOTE_SEND = true;
  } else {
    // BOTH: clock enabled (if clockMode allows), notes enabled
    drawText("BOTH", 2, 3, UI_BLUE);
    MIDI_CLOCK_SEND = (clockMode == 1);  // Respect clockMode setting
    MIDI_NOTE_SEND = true;
  }

  FastLEDshow();
}