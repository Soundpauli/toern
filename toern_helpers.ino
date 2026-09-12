
// Global buffer for AI generation in external RAM (prevents stack overflow)
EXTMEM BasePagePattern g_channelPatterns[16];

void generateRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBassOrMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBassLine(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateMainMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateBasicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony);
void generateContextAwareRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony, BasePagePattern* basePattern, int pageOffset);
void generateContextAwareMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony, BasePagePattern* basePattern, int pageOffset);
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

// ---- NEW/genre generation helpers (no RAM1 globals; no large locals) ----
static inline uint8_t clampVel(int v) {
  if (v < 1) return 1;
  if (v > 127) return 127;
  return (uint8_t)v;
}

static inline void placeGenNote(unsigned int c, int row, uint8_t ch, int vel, uint8_t prob = 100, uint8_t cond = 1) {
  // Always write probability/condition explicitly so generated notes don't inherit stale values.
  note[c][row].channel = ch;
  note[c][row].velocity = clampVel(vel);
  note[c][row].probability = prob;
  note[c][row].condition = cond;
  note[c][row].midiPitch = NOTE_MIDI_PITCH_NONE;
}

// drawRandoms / single-mode y=16: empty-slot only; always 100% probability + condition 1 (1/1).
static inline bool placeRandomNoteIfEmpty(unsigned int c, int row, uint8_t ch, int vel) {
  if (row < 1 || row > 16) return false;
  if (note[c][row].channel != 0) return false;
  placeGenNote(c, row, ch, vel, 100, 1);  // 100% / always
  return true;
}

// External variables from menu
extern int genreType;
extern int genreLength;

// External variables from main system
extern Device SMP;
extern GlobalVars GLOB;
extern Mode* currentMode;
extern i2cEncoderLibV2 Encoder[];
extern Mode volume_bpm;
extern IntervalTimer playTimer;
extern double playNoteInterval;
extern float detune[13]; // Global detune array for channels 1-12
extern float channelOctave[9]; // Global octave array for channels 1-8
extern int8_t channelDirection[maxFiles];
extern unsigned int recordingStartBeat;  // Beat where recording started
extern unsigned int RefreshTime;  // Display refresh timing (30 FPS = 33ms per frame)

// ── Dynamic sample browser (SD): SET_WAV lists built on demand ──
// Mute-mode random (?>) uses samples/toern_wavs.txt from ETC→RSET→SD scan.
// Encoder ticks never walk directories (that races AudioPlaySdWav / eDMA).
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
// Combined browser list on last encoder: [../] (when not at root), dirs first, then files.
EXTMEM char g_folderPickName[BROWSE_MAX_TMP][SAMPLE_BROWSER_NAME_MAX];
EXTMEM char g_wavPickName[BROWSE_MAX_TMP][SAMPLE_BROWSER_NAME_MAX];
EXTMEM uint8_t g_wavPickType[BROWSE_MAX_TMP];
EXTMEM uint32_t g_wavPickSize[BROWSE_MAX_TMP];
uint16_t g_folderPickCount = 0;
uint16_t g_wavPickCount = 0;
int g_wavFileCount = 0;  // number of file entries inside the combined list
bool g_browseListDirty = true;
// Set when folder navigation resets pos[3] so showWave() does not start a new preview (feels like "load").
bool g_suppressNextWavPreviewAfterFolderNav = false;
// After folder enter/up, ignore encoder-2 sample load briefly (I2C / mechanical coupling ghosts).
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

// Playlist entries are SD-root paths. samplePathRel stays samples/-relative when possible;
// paths outside samples/ are stored with a leading '*' so buildSamplePath can open them.
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

// ETC → RSET → SD: walk whole card once, write samples/toern_wavs.txt, keep RAM index.
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
  g_wavScanDirStack[0][0] = 0;  // empty = SD root

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

FLASHMEM void soloRandomRefreshWavList() {
  soloRandomRenewPlaylist();
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

uint16_t soloRandomPlaylistCount() {
  return g_soloRandomPlaylistCount;
}

int16_t soloRandomPlaylistIndex() {
  return g_soloRandomPlaylistIndex;
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

// Highest valid index on the last encoder browse row (1-based on hardware, count on UI).
int sampleBrowserBrowseIndexMax(int channel) {
  if (channel < 1 || channel >= maxFiles) return 0;
  if (g_browseListDirty) sampleBrowserRefreshList(channel);
  return max(1, (int)g_wavPickCount);
}

// RAM-only clamp. checkEncoders() already owns Encoder[3] reads.
void sampleBrowserClampBrowseIndex(int channel) {
  if (channel < 1 || channel >= maxFiles) return;
  int vmax = sampleBrowserBrowseIndexMax(channel);
  int idx = constrain((int)currentMode->pos[3], 1, vmax);
  currentMode->pos[3] = (unsigned int)idx;
}

// Navigate / mode-entry: clamp, then snap hardware only if it is out of range.
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

bool sampleBrowserSelectionIsDirectory(int channel) {
  if (channel < 1 || channel >= maxFiles) return false;
  if (g_browseListDirty) sampleBrowserRefreshList(channel);
  int idx = (int)currentMode->pos[3] - 1;
  uint8_t type = sampleBrowserEntryTypeAt(idx);
  return type == SAMPLE_BROWSER_ENTRY_PARENT || type == SAMPLE_BROWSER_ENTRY_DIR;
}

bool sampleBrowserSelectionIsFile(int channel) {
  if (channel < 1 || channel >= maxFiles) return false;
  if (g_browseListDirty) sampleBrowserRefreshList(channel);
  int idx = (int)currentMode->pos[3] - 1;
  return sampleBrowserEntryTypeAt(idx) == SAMPLE_BROWSER_ENTRY_FILE;
}

// After ".." or entering a subfolder: rebuild lists, reset browse picker.
// If coming from a child folder, keep that folder selected in the parent list.
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

  /* ---- Legacy folder row retained for compatibility with old overlay code. ---- */
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

  /* ---- Last encoder row: [../] (if needed), then dirs, then files ---- */
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
    // Empty or SD-root absolute (outside samples/) — browse from samples root.
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

// SET_WAV: last encoder press — directory row ([../] or folder): navigate (up or into folder).
// File row: go up one level when not at samples root; at root, press does nothing.
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

// SET_WAV: always go up one directory level (no-op at samples root).
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

// Generate next numeric filename: rec_01.wav ... rec_999.wav (always in samples/recs)
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

// Playback / recording path from stored relative path (samples/...)
// Leading '*' means SD-root absolute (used by mute-mode random outside samples/).
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

FLASHMEM void EEPROMgetLastFiles() {
  // Dynamic browser: nothing to preload from SD index files
}





// Wrapper: load all saved SMP settings (parameters, filters, synths)
// for every channel.
FLASHMEM void loadSMPSettings() {
  // Don't load settings if SMP_LOAD_SETTINGS is false
  if (!SMP_LOAD_SETTINGS) {
    return;
  }
  
  // CRITICAL FIX: Ensure channels 4-8 are always in sample mode (EFX=0)
  for (int ch = 4; ch <= 8; ch++) {
    SMP.filter_settings[ch][EFX] = 0;
  }
  
  // Define the valid channels: 1,2,3,4,5,6,7,8,11,13,14
  const int validChannels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numValidChannels = sizeof(validChannels) / sizeof(validChannels[0]);
  

  for (int i = 0; i < numValidChannels; i++) {
    int ch = validChannels[i];
    
    // Load Filters - apply all filter settings for this channel
    for (int f = 0; f < NUM_FILTERS; f++) {
      setFilters((FilterType)f, ch, true);
    }
    
    // Load Parameters - apply saved envelope settings to the live audio objects.
    // DELAY is not a hardware envelope setter here; ATTACK..RELEASE are.
    setParams(ATTACK, ch);
    setParams(HOLD, ch);
    setParams(DECAY, ch);
    setParams(SUSTAIN, ch);
    setParams(RELEASE, ch);

    // Load Synths - apply synth settings (only for channel 11)
    if (ch == 11) {
      // FORM is an internal preset identity rather than an exposed user control.
      // Refresh it for older projects, but preserve their saved filter, pitch,
      // and ADSR controls.
      extern void applySynthInstrumentFormDefault(int channel, int instrumentIdx);
      applySynthInstrumentFormDefault(
          11, (int)SMP.synth_settings[11][INSTRUMENT]);
      updateSynthVoice(11);
    }
  }

  // Apply any pending filter mixer targets immediately after loading so synth channels
  // do not require an extra trigger/play cycle before their routing is correct.
  forceAllMixerGainsToTarget();
  
  // Legacy drum path removed; no initialization needed.
  
  
  // Optionally update other settings such as channel volumes
  // or call updateFiltersAndParameters() if needed.
  //updateFiltersAndParameters();
}



void writeWavHeader(File &file, uint32_t sampleRate, uint8_t bitsPerSample, uint16_t numChannels) {
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint8_t blockAlign = numChannels * bitsPerSample / 8;

  // WAV header (44 bytes)
  uint8_t header[44] = {
    'R', 'I', 'F', 'F',
    0, 0, 0, 0,  // <- file size - 8 (filled in later)
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  // PCM chunk size
    1, 0,         // Audio format (1 = PCM)
    (uint8_t)(numChannels & 0xff), (uint8_t)(numChannels >> 8),
    (uint8_t)(sampleRate & 0xff), (uint8_t)((sampleRate >> 8) & 0xff),
    (uint8_t)((sampleRate >> 16) & 0xff), (uint8_t)((sampleRate >> 24) & 0xff),
    (uint8_t)(byteRate & 0xff), (uint8_t)((byteRate >> 8) & 0xff),
    (uint8_t)((byteRate >> 16) & 0xff), (uint8_t)((byteRate >> 24) & 0xff),
    blockAlign, 0,
    bitsPerSample, 0,
    'd', 'a', 't', 'a',
    0, 0, 0, 0  // <- data chunk size (filled in later)
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


void startRecordingRAM() {
  if (isRecording) return;

  if (playSdWav1.isPlaying()) playSdWav1.stop();
  Encoder[0].writeRGBCode(0x000000);
  Encoder[1].writeRGBCode(0xFF0000);
  Encoder[2].writeRGBCode(0x000000);
  Encoder[3].writeRGBCode(0x000000);
  recTime = 0;
  recWriteIndex = 0;
  
  // Reset peak recording index for fresh waveform display
  extern int peakRecIndex;
  peakRecIndex = 0;
  
  queue1.begin();  // start filling 128‑sample blocks
  // REC = record
  showIcons(ICON_RECORD, UI_DIM_RED);
  FastLEDshow();
  isRecording = true;
  
  // Enable audio input monitoring using VOL menu input level settings
  extern int recMode;
  extern unsigned int lineInLevel;  // From VOL menu (0-15)
  extern unsigned int micGain;      // From VOL menu (0-63)
  extern AudioMixer4 mixer_end;
  extern bool getSpkrEnabled();
  
  float monitorGain = 0.0f;
  const float maxPlaybackGain = MIX_BUS_HEADROOM * MIX_END_SAMPLES_GAIN;  // Match typical single-hit playback level
  if (recMode == 1) {
    // Mic input: if SPKR is on, disable monitor path to avoid feedback while recording.
    if (getSpkrEnabled()) {
      monitorGain = 0.0f;
    } else {
      monitorGain = mapf(micGain, 0, 63, 0.0f, maxPlaybackGain);
    }
  } else {
    // Line input: map lineInLevel (0-15) to mixer gain (0.0-maxPlaybackGain)
    monitorGain = mapf(lineInLevel, 0, 15, 0.0f, maxPlaybackGain);
  }
  mixer_end.gain(3, monitorGain);
}

void flushAudioQueueToRAM2() {
  if (!isRecording) return;
  // each AudioRecordQueue block is 128 samples
  while (queue1.available() && recWriteIndex + 128 <= BUFFER_SAMPLES) {
    auto block = (int16_t *)queue1.readBuffer();
    memcpy(recBuffer + recWriteIndex, block, 128 * sizeof(int16_t));
    recWriteIndex += 128;
    queue1.freeBuffer();
  }
  // if we ever hit the end of the buffer, stop automatically:
  if (recWriteIndex >= BUFFER_SAMPLES) {
    //Serial.println("⚠️ Buffer full");
    queue1.end();
    isRecording = false;
  }
}

void stopRecordingRAM(int fnr, int snr) {
  if (!isRecording) return;
  flushAudioQueueToRAM();
  queue1.end();
  isRecording = false;
  
  // Disable audio input monitoring
  mixer_end.gain(3, 0.0);

  // open WAV on SD (next free rec_XX.wav in the dedicated samples/recs folder)
  (void)fnr;
  (void)snr;
  char path[160];
  char tmpName[65];
  generateNextNumericName(0, tmpName, sizeof(tmpName));
  int ch = GLOB.currentChannel;
  if (ch < 1 || ch >= maxFiles) ch = 1;
  // Recorded files always go into samples/recs (created on demand).
  if (!SD.exists("samples")) SD.mkdir("samples");
  if (!SD.exists("samples/recs")) SD.mkdir("samples/recs");
  snprintf(path, sizeof(path), "samples/recs/%s", tmpName);
  if (SD.exists(path)) SD.remove(path);
  File f = SD.open(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!f) { return; }
  // write 44‑byte WAV header for 16‑bit/mono/22 050 Hz
  writeWavHeader(f, AUDIO_SAMPLE_RATE_EXACT, 16, 1);
  // write all your samples in one chunk
  f.write((uint8_t *)recBuffer, recWriteIndex * sizeof(int16_t));
  finalizeWavHeader(f, recWriteIndex * sizeof(int16_t));

  f.close();
  //Serial.print("💾 Saved ");
  //Serial.println(path);

  sampleBrowserInvalidate();
  snprintf(SMP.samplePathRel[ch], 128, "recs/%s", tmpName);
  
  // Reload the sample metadata after recording
  extern bool sampleIsLoaded;
  extern bool firstcheck;
  extern CachedSample previewCache;
  
  sampleIsLoaded = false;  // Force reload of new recording
  firstcheck = true;       // Reset check flag
  previewCache.valid = false;  // Invalidate cache so new recording is loaded
  
  // Don't auto-play - user will press encoder[2] to play if desired
  // playSdWav1.play(path);
}



// --------------------
void startFastRecord() {
  if (fastRecordActive) return;

  // Handle different recording modes based on recChannelClear
  if (recChannelClear == 1) {
    // ON mode: Clear all existing notes of channel, then add triggers/notes
    clearAllNotesOfChannel();
  }
  // OFF mode (recChannelClear == 0): Add triggers/notes as soon as recording starts
  // FIX mode (recChannelClear == 2): Don't manipulate any notes - just record

   paintMode = false;
            freshPaint = true;
            unpaintMode = false;
            pressed[3] = false;
            
            // Reset paint/unpaint prevention flag when starting fast record
            extern bool preventPaintUnpaint;
            preventPaintUnpaint = false;
            
  // Only add triggers/notes if not in FIX mode
  if (recChannelClear != 2) {
    note[beat][GLOB.currentChannel+1].channel = GLOB.currentChannel;
    note[beat][GLOB.currentChannel+1].velocity = defaultVelocity;
    note[beat][GLOB.currentChannel+1].probability = 100;
    note[beat][GLOB.currentChannel+1].condition = 1;
    note[beat][GLOB.currentChannel+1].midiPitch = NOTE_MIDI_PITCH_NONE;
  }
            
  // 1) Stop & clear any queued audio so old data never sneaks in
  queue1.end();
  while (queue1.available()) {
    queue1.readBuffer();
    queue1.freeBuffer();
  }

  // 2) Reset our write index and drop counter
  int ch = GLOB.currentChannel;
  
  // Immediately stop any playing sound on the recording channel
  extern AudioEffectEnvelope *envelopes[];
  if (ch >= 0 && ch < 15 && envelopes[ch] != nullptr) {
    envelopes[ch]->noteOff();
  }
  
  // Also stop all notes on the sampler for channels 0-8 (sample channels)
  // Stop notes in the typical MIDI range (36-96 covers most sample pitches)
  if (ch >= 0 && ch <= 8) {
    extern arraysampler _samplers[];
    // Stop notes in a reasonable range (MIDI note 36-96, covering most sample pitches)
    for (int note = 36; note <= 96; note++) {
      _samplers[ch].noteEvent(note, 0, false, false);
    }
  }
  
  fastRecWriteIndex[ch] = 0;
  // For ON1 mode, don't drop initial audio - we want a perfect loop from beat 1
  // For other modes, drop first ~25ms to avoid noise/click at start (reduced from 200ms)
  if (recChannelClear == 3) {
    fastDropRemaining = 0;  // ON1 mode: no drop, capture from beat 1
  } else {
    fastDropRemaining = FAST_DROP_BLOCKS;  // Other modes: drop first ~25ms
  }

  // 3) Restart recording queue
  queue1.begin();
  fastRecordActive = true;
  
  // Enable audio input monitoring using VOL menu input level settings
  extern int recMode;
  extern unsigned int lineInLevel;  // From VOL menu (0-15)
  extern unsigned int micGain;      // From VOL menu (0-63)
  extern AudioMixer4 mixer_end;
  extern bool getSpkrEnabled();
  
  float monitorGain = 0.0;
  const float maxPlaybackGain = MIX_BUS_HEADROOM * MIX_END_SAMPLES_GAIN;  // Match typical single-hit playback level
  if (recMode == 1) {
    // Mic input: if SPKR is on, disable monitor path to avoid feedback while recording.
    if (getSpkrEnabled()) {
      monitorGain = 0.0f;
    } else {
      monitorGain = mapf(micGain, 0, 63, 0.0, maxPlaybackGain);
    }
  } else {
    // Line input: map lineInLevel (0-15) to mixer gain (0.0-maxPlaybackGain) to match loudest playback
    monitorGain = mapf(lineInLevel, 0, 15, 0.0, maxPlaybackGain);
  }
  mixer_end.gain(3, monitorGain);

  //Serial.printf("FAST RECORD ▶ ch%u (dropping %d blocks)\n", ch, FAST_DROP_BLOCKS);
}

// --------------------
void flushAudioQueueToRAM() {
  if (fastRecordActive) {
    int ch = GLOB.currentChannel;
    auto &idx = fastRecWriteIndex[ch];

    // Treat your byte buffer as an int16_t array for cleaner pointer math:
    int16_t *dest = reinterpret_cast<int16_t *>(sampled[ch]);

    // Pull every pending block
    while (queue1.available()) {
      int16_t *block = (int16_t *)queue1.readBuffer();

      if (fastDropRemaining > 0) {
        // still skipping the first 200ms
        fastDropRemaining--;
      } else if (idx + AUDIO_BLOCK_SAMPLES <= BUFFER_SAMPLES) {
        // copy 128 samples (256 bytes) into our buffer
        memcpy(dest + idx, block, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
        idx += AUDIO_BLOCK_SAMPLES;
      }

      queue1.freeBuffer();

      // auto-stop if full
      if (idx >= BUFFER_SAMPLES) {
        stopFastRecord();
        break;
      }
    }
    return;  // skip your SD path
  }
}

void stopFastRecord() {
  // CRITICAL: Disable audio input first to stop new data from entering queue
  mixer_end.gain(3, 0.0);
  
  int ch = GLOB.currentChannel;
  auto &idx = fastRecWriteIndex[ch];
  int16_t *dest = reinterpret_cast<int16_t *>(sampled[ch]);
  
  // Flush all remaining audio data while fastRecordActive is still true
  // Use multiple passes to ensure we capture all data for the final beat
  int flushCount = 0;
  int lastQueueSize = -1;
  int stableCount = 0;
  
  // Continue flushing until queue is empty or no progress is made
  while (fastRecordActive && queue1.available()) {
    int currentQueueSize = queue1.available();
    flushAudioQueueToRAM();
    flushCount++;
    
    // Check if queue size changed (progress made)
    if (currentQueueSize == lastQueueSize) {
      stableCount++;
      // If queue size hasn't changed for 3 passes, likely done
      if (stableCount >= 3) break;
    } else {
      stableCount = 0;
      lastQueueSize = currentQueueSize;
    }
    
    // Safety limit to prevent infinite loops
    if (flushCount >= 100) break;
  }
  
  // Now safe to stop recording
  fastRecordActive = false;
  
  // Flush one more time before ending the queue
  flushAudioQueueToRAM();
  
  queue1.end();
  
  // Final aggressive flush of any remaining data in the queue
  // Continue until queue is completely empty or buffer is full
  flushCount = 0;
  lastQueueSize = -1;
  stableCount = 0;
  
  while (queue1.available() && idx + AUDIO_BLOCK_SAMPLES <= BUFFER_SAMPLES) {
    int currentQueueSize = queue1.available();
    int16_t *block = (int16_t *)queue1.readBuffer();
    memcpy(dest + idx, block, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
    idx += AUDIO_BLOCK_SAMPLES;
    queue1.freeBuffer();
    flushCount++;
    
    // Check if queue size changed (progress made)
    if (currentQueueSize == lastQueueSize) {
      stableCount++;
      // If queue size hasn't changed for 3 passes, likely done
      if (stableCount >= 3) break;
    } else {
      stableCount = 0;
      lastQueueSize = currentQueueSize;
    }
    
    // Safety limit to prevent infinite loops
    if (flushCount >= 100) break;
  }
  
  // Save recording start beat before resetting (needed for ON1 mode note placement)
  unsigned int savedStartBeat = recordingStartBeat;
  
  // Reset recording start beat tracking
  recordingStartBeat = 0;
  
  // Reset absolute beat counter
  extern unsigned int recordingBeatCount;
  recordingBeatCount = 0;
  
  loadedSampleLen[ch] = idx;
  
  // Defer sampler loading and heavy operations to avoid blocking audio
  // These will happen after the critical stop sequence
  
  // Auto-save recorded sample to samplepack 0
  
  // Do heavy save operations (SD write, EEPROM) - these might cause brief hang
  // but we've already stopped recording, so audio playback should continue
  copySampleToSamplepack0(ch);
  saveSp0StateToEEPROM();
  
  // Now load into sampler (do this after saves to minimize interruption)
  _samplers[ch].removeAllSamples();
  _samplers[ch].addSample(
    36,                            // MIDI note #
    (int16_t*)sampled[ch],         // reinterpret bytes → int16_t
    idx,                           // sample-count
    rateFactor
  );
  channelDirection[ch] = 1;
  
  // For ON1 mode: Set note at x=1 after recording and copy are complete
  if (recChannelClear == 3 && savedStartBeat > 0) {
    // Find the next x=1 position (beat 1 in the next bar)
    // Since recording stopped at maxX, the next x=1 is at the start of the next bar
    unsigned int nextBeat1 = ((savedStartBeat - 1) / maxX + 1) * maxX + 1;
    // Set note at x=1
    note[nextBeat1][ch+1].channel = ch;
    note[nextBeat1][ch+1].velocity = defaultVelocity;
    note[nextBeat1][ch+1].probability = 100;
    note[nextBeat1][ch+1].condition = 1;
    note[nextBeat1][ch+1].midiPitch = NOTE_MIDI_PITCH_NONE;
  }
  
  // give back your knob color + preview
 // Encoder[0].writeRGBCode(CRGBToUint32(col[ch]));
 // _samplers[ch].noteEvent(36, defaultVelocity, true, false);
  //Serial.printf("◀ FASTREC ch%u, %u samples\n", ch, (unsigned)idx);

}

FLASHMEM void EEPROMsetLastFile() {
  sampleBrowserInvalidate();
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

  updateLastPage();  // Optional: If your UI tracks last updated page
  FastLEDshow();     // Optional: Refresh LED grid if used
}

FLASHMEM void FastLEDclear() {
  // Clear ALL LEDs in the buffer to prevent flickering from uninitialized LEDs
  // Even if only 1 module is active, we must clear the entire buffer
  for (unsigned int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

extern bool isNowPlaying;
extern uint32_t lastUserActivityMs;
extern int ledModules;

void light_single(unsigned int matrixId, unsigned int x, unsigned int y, CRGB color);
void clearLedStripForScreensaver();

// Flag to track if encoder LEDs were turned off by screensaver
static bool screensaverEncodersOff = false;

// Comical eyes screensaver: eyes look around and blink, after 30s more they close and dim
// Helper to draw pixel for screensaver (handles matrix addressing)
static void ssLight(int x, int y, CRGB color) {
  if (x < 1 || y < 1) return;
  unsigned int matrixNum = (x - 1) / MATRIX_WIDTH;
  unsigned int localX = ((x - 1) % MATRIX_WIDTH) + 1;
  light_single(matrixNum, localX, y, color);
}

FLASHMEM static void drawScreensaverMatrix() {
  // Turn off encoder LEDs when screensaver is active (only write once on entry)
  if (!screensaverEncodersOff) {
    for (int i = 0; i < 4; i++) {
      Encoder[i].writeRGBCode(0x000000);
    }
    screensaverEncodersOff = true;
  }
  
  FastLEDclear();
  clearLedStripForScreensaver();

  const uint32_t idleMs = millis() - lastUserActivityMs;
  const bool asleep = (idleMs >= 90000UL);  // After 90s total (60s to start + 30s more)
  
  // Eye outline dimmer than pupils - pupils are bright white
  const uint8_t outlineBrightness = asleep ? 18 : 15;
  const uint8_t pupilBrightness = asleep ? 18 : 80;
  const CRGB eyeColor = CRGB(outlineBrightness, outlineBrightness, outlineBrightness);
  const CRGB pupilColor = CRGB(pupilBrightness, pupilBrightness, pupilBrightness);
  
  // Eye positioning - centered on display
  const int displayWidth = (ledModules == 2) ? 32 : 16;
  const int eyeWidth = 5;
  const int gap = 2;
  const int totalWidth = eyeWidth * 2 + gap;  // 12 pixels
  const int leftEyeX = (displayWidth - totalWidth) / 2 + 1;
  const int rightEyeX = leftEyeX + eyeWidth + gap;
  int eyeY = 5;  // Base Y position (bottom of eyes)
  
  uint32_t now = millis();
  
  // Dream movement: slow up/down drift when asleep (0 → +1 → 0 → -1 → 0)
  static int8_t dreamOffsetY = 0;
  static uint8_t dreamPhase = 0;  // 0=center, 1=up, 2=center, 3=down
  static uint32_t lastDreamMs = 0;
  
  if (asleep) {
    // Cycle through positions every 3 seconds
    if (now - lastDreamMs >= 3000) {
      lastDreamMs = now;
      dreamPhase = (dreamPhase + 1) % 4;
      switch (dreamPhase) {
        case 0: dreamOffsetY = 0; break;   // center
        case 1: dreamOffsetY = 1; break;   // up
        case 2: dreamOffsetY = 0; break;   // center
        case 3: dreamOffsetY = -1; break;  // down
      }
    }
    eyeY += dreamOffsetY;
  } else {
    dreamOffsetY = 0;
    dreamPhase = 0;
  }
  
  // Peek disabled - eyes stay closed while asleep
  static bool isPeeking = false;
  static bool peekLeftEye = false;
  
  // Blink state: blink every ~3-5 seconds for ~150ms (when awake)
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
  
  // Pupil movement: smooth random looking around (when awake)
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
    // When peeking, look to the side
    if (isPeeking) {
      currentPupilX = peekLeftEye ? 1 : -1;
      currentPupilY = 0;
    } else {
      currentPupilX = 0;
      currentPupilY = 0;
    }
  }
  
  // Draw a single eye
  auto drawEye = [&](int baseX, int baseY, bool closed, CRGB color, CRGB pColor) {
    if (closed) {
      // Closed eye: horizontal line with eyelid curve
      for (int dx = 0; dx < 5; dx++) {
        ssLight(baseX + dx, baseY + 3, color);
      }
      ssLight(baseX + 1, baseY + 4, color);
      ssLight(baseX + 2, baseY + 4, color);
      ssLight(baseX + 3, baseY + 4, color);
      return;
    }
    
    // Open eye outline
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
    
    // Pupil
    int pupilX = baseX + 1 + currentPupilX;
    int pupilY = baseY + 2 + currentPupilY;
    if (pupilX < baseX + 1) pupilX = baseX + 1;
    if (pupilX > baseX + 2) pupilX = baseX + 2;
    if (pupilY < baseY + 1) pupilY = baseY + 1;
    if (pupilY > baseY + 3) pupilY = baseY + 3;
    
    ssLight(pupilX, pupilY, pColor);
    ssLight(pupilX + 1, pupilY, pColor);
    ssLight(pupilX, pupilY + 1, pColor);
    ssLight(pupilX + 1, pupilY + 1, pColor);
  };
  
  // Determine eye states
  bool leftClosed = isBlinking || (asleep && !(isPeeking && peekLeftEye));
  bool rightClosed = isBlinking || (asleep && !(isPeeking && !peekLeftEye));
  
  // Peeking eye: dim outline, brighter pupil
  CRGB peekColor = CRGB(12, 12, 12);
  CRGB peekPupil = CRGB(40, 40, 40);
  
  CRGB leftColor = (isPeeking && peekLeftEye) ? peekColor : eyeColor;
  CRGB leftPupil = (isPeeking && peekLeftEye) ? peekPupil : pupilColor;
  CRGB rightColor = (isPeeking && !peekLeftEye) ? peekColor : eyeColor;
  CRGB rightPupil = (isPeeking && !peekLeftEye) ? peekPupil : pupilColor;
  
  drawEye(leftEyeX, eyeY, leftClosed, leftColor, leftPupil);
  drawEye(rightEyeX, eyeY, rightClosed, rightColor, rightPupil);
}

FLASHMEM void FastLEDshow() {
  if (millis() - lastUpdate > RefreshTime) {
    lastUpdate = millis();
    extern bool sdSerialServerClientConnected();
    extern bool drawNoSD_hasRun;
    // Never start screensaver while stuck on SD? (card missing / not ready yet)
    bool screensaverActive = drawNoSD_hasRun
      && !isNowPlaying
      && !sdSerialServerClientConnected()
      && (millis() - lastUserActivityMs >= 60000UL);
    if (screensaverActive) {
      drawScreensaverMatrix();
    } else if (screensaverEncodersOff) {
      // Screensaver just ended - reset flag so encoders can be controlled again
      screensaverEncodersOff = false;
    }
    // Don't change global brightness - matrix is dimmed in software (light_single)
    // Strip stays at full brightness (set in setup() and not modified)
    FastLED.show();  // Shows both matrix (PIN 17) and strip (PIN 24)
  }
}



int getPage(int x) {
  //updateLastPage();
  return (x - 1) / maxX + 1;  // Calculate page number
}

// Harmonic analysis structure for musical random generation (moved to top of file)

// 16-step pattern tiles across the page (maxX may be 16 or 32).
static inline int randStep16(unsigned int x_rel) {
  return (int)(((x_rel - 1u) % 16u) + 1u);
}

static CompanionRole companionRoleForChannel(uint8_t channel) {
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

static bool companionChannelSupported(uint8_t channel) {
  return companionRoleForChannel(channel) != CompanionRole::Unknown;
}

static int companionEffectivePages() {
  int pages = (maxX > 0) ? (int)(MAX_STEPS / maxX) : 1;
  if (pages < 1) pages = 1;
  if (pages > (int)maxPages) pages = (int)maxPages;
  return pages;
}

static int companionActivePages() {
  extern int loopLength;
  int pages = loopLength > 0 ? loopLength : (int)lastPage;
  if (pages < (int)GLOB.edit) pages = (int)GLOB.edit;
  int limit = companionEffectivePages();
  if (pages < 1) pages = 1;
  if (pages > limit) pages = limit;
  return pages;
}

static void companionAddRoleWeight(CompanionContext &ctx, CompanionRole role,
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

static int companionPitchClassForRow(uint8_t channel, int row) {
  int noteValue = row - 1;
  if (channel >= 1 && channel <= 8) {
    noteValue = 12 * SampleRate[channel] + row - (channel + 1);
    noteValue += (int)detune[channel] + (int)(channelOctave[channel] * 12);
  } else if (channel == 11) {
    noteValue = 12 * (int)octave[0] + transpose + row - 1;
  } else if (channel == 13 || channel == 14) {
    noteValue = row - channel + 47;
  }
  noteValue %= 12;
  return noteValue < 0 ? noteValue + 12 : noteValue;
}

// Analyze other voices over a bounded page range. The destination page carries
// the most weight, adjacent pages less, and distant pages establish style/key.
static CompanionContext analyzeCompanionContext(int firstPage, int lastSourcePage,
                                                int focusPage, uint8_t excludeChannel) {
  CompanionContext ctx;
  int limit = companionEffectivePages();
  firstPage = constrain(firstPage, 1, limit);
  lastSourcePage = constrain(lastSourcePage, firstPage, limit);
  focusPage = constrain(focusPage, 1, limit);

  for (int page = firstPage; page <= lastSourcePage; page++) {
    int distance = abs(page - focusPage);
    uint16_t pageWeight = (distance == 0) ? 4 : ((distance == 1) ? 2 : 1);
    unsigned int start = (unsigned int)(page - 1) * maxX + 1;
    unsigned int end = min((unsigned int)MAX_STEPS + 1u, start + maxX);
    ctx.sourcePages++;

    for (unsigned int c = start; c < end; c++) {
      int phase = randStep16(c - start + 1) - 1;
      for (int row = 1; row <= 16; row++) {
        uint8_t ch = note[c][row].channel;
        if (ch == 0 || ch == excludeChannel || !companionChannelSupported(ch)) continue;

        uint16_t velocityWeight = (uint16_t)max(1, (int)note[c][row].velocity / 24);
        uint16_t weight = pageWeight * velocityWeight;
        ctx.stepWeight[phase] += weight;
        ctx.stepVelocity[phase] += (uint32_t)note[c][row].velocity * weight;
        companionAddRoleWeight(ctx, companionRoleForChannel(ch), phase, weight);
        ctx.totalWeight += weight;
        ctx.empty = false;

        CompanionRole role = companionRoleForChannel(ch);
        if (role == CompanionRole::Bass || role == CompanionRole::Keys ||
            role == CompanionRole::VocalsPad) {
          ctx.rowWeight[row] += weight;
          ctx.pitchClassWeight[companionPitchClassForRow(ch, row)] += weight;
        }
      }
    }
  }

  uint16_t best = 0;
  for (int row = 1; row <= 16; row++) {
    if (ctx.rowWeight[row] > best) {
      best = ctx.rowWeight[row];
      ctx.rootRow = (uint8_t)row;
    }
  }

  // Score all tonic/mode candidates rather than assuming the busiest note is
  // the root. Chord tones carry more evidence; out-of-scale notes are penalized.
  static const int8_t majorScale[] = {0, 2, 4, 5, 7, 9, 11};
  static const int8_t minorScale[] = {0, 2, 3, 5, 7, 8, 10};
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
        int multiplier = -2;
        if (degree == 0) multiplier = 5;
        else if (degree == 2 || degree == 4) multiplier = 3;
        else if (degree >= 0) multiplier = 1;
        score += (int)ctx.pitchClassWeight[pc] * multiplier;
      }
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

static void clearCompanionChannelPage(int page, uint8_t channel) {
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

static void clearCompanionPage(int page) {
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

static int companionFreeRow(unsigned int step, int preferred);

static int companionScaleRow(const CompanionContext &ctx, bool lowRegister, int motion) {
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

static CompanionHarmony companionMakeHarmony(const CompanionContext &ctx, int variation) {
  CompanionHarmony harm;
  uint32_t tonalWeight = 0;
  for (int pc = 0; pc < 12; pc++) tonalWeight += ctx.pitchClassWeight[pc];
  bool noTonalContext = tonalWeight == 0;
  harm.rootPc = noTonalContext ? (uint8_t)((variation * 5) % 12)
                               : (uint8_t)((ctx.rootRow - 1) % 12);
  harm.isMinor = noTonalContext ? (((variation / 2) & 1) == 0) : ctx.isMinor;

  // Common diatonic loops (scale degrees 0..6). Minor defaults to i–VI–III–VII.
  static const uint8_t majorProg[][4] = {
      {0, 4, 5, 3},  // I V vi IV
      {0, 3, 4, 0},  // I IV V I
      {0, 5, 3, 4},  // I vi IV V
      {5, 3, 0, 4},  // vi IV I V
  };
  static const uint8_t minorProg[][4] = {
      {0, 5, 2, 6},  // i VI III VII
      {0, 3, 5, 4},  // i iv VI V
      {0, 5, 3, 4},  // i VI iv V
      {0, 2, 5, 6},  // i III VI VII
  };
  const uint8_t (*table)[4] = harm.isMinor ? minorProg : majorProg;
  const uint8_t *prog = table[(variation / 3) & 3];
  for (int i = 0; i < 4; i++) harm.degree[i] = prog[i];
  return harm;
}

static void companionFillScalePcs(const CompanionHarmony &harm, int8_t out[7]) {
  static const int8_t majorIntervals[] = {0, 2, 4, 5, 7, 9, 11};
  static const int8_t minorIntervals[] = {0, 2, 3, 5, 7, 8, 10};
  const int8_t *intervals = harm.isMinor ? minorIntervals : majorIntervals;
  for (int i = 0; i < 7; i++) {
    out[i] = (int8_t)((harm.rootPc + intervals[i]) % 12);
  }
}

static int companionChordPcs(const CompanionHarmony &harm, int bar, bool withSeventh,
                             int8_t out[4]) {
  int8_t scale[7];
  companionFillScalePcs(harm, scale);
  int deg = harm.degree[constrain(bar, 0, 3)] % 7;
  out[0] = scale[deg];
  out[1] = scale[(deg + 2) % 7];
  out[2] = scale[(deg + 4) % 7];
  if (!withSeventh) return 3;
  out[3] = scale[(deg + 6) % 7];
  return 4;
}

static int companionPcToChannelRow(int pc, uint8_t channel, int lo, int hi, int prefer) {
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
  // A short row range may not contain every pitch class.
  return prefer;
}

// 60% chord tone / 30% other scale tone / 10% chromatic passing tone.
// Bass prefers root + fifth; pads stay on chord tones.
static int companionPickPitchClass(const CompanionHarmony &harm, int bar,
                                   CompanionRole role, int prevPc) {
  int8_t chord[4];
  bool seventh = (role == CompanionRole::Keys && random(100) < 40);
  int nChord = companionChordPcs(harm, bar, seventh, chord);
  int8_t scale[7];
  companionFillScalePcs(harm, scale);

  auto pickChord = [&](bool rootFifthBias) -> int {
    if (rootFifthBias) {
      int r = random(100);
      if (r < 55) return chord[0];
      if (r < 85) return chord[2];  // fifth
      return chord[1];              // third
    }
    return chord[random(0, nChord)];
  };

  auto pickScaleOther = [&]() -> int {
    for (int attempt = 0; attempt < 8; attempt++) {
      int pc = scale[random(0, 7)];
      bool inChord = false;
      for (int i = 0; i < nChord; i++) {
        if (pc == chord[i]) {
          inChord = true;
          break;
        }
      }
      if (!inChord) return pc;
    }
    return scale[random(0, 7)];
  };

  auto pickChromatic = [&]() -> int {
    int base = (prevPc >= 0) ? prevPc : chord[0];
    int delta = (random(100) < 50) ? 1 : -1;
    return (base + delta + 12) % 12;
  };

  if (role == CompanionRole::Bass) {
    int r = random(100);
    if (r < 55) return chord[0];
    if (r < 85) return chord[2];
    if (r < 95) return pickScaleOther();
    return pickChromatic();
  }

  if (role == CompanionRole::VocalsPad) {
    int r = random(100);
    if (r < 88) return pickChord(true);
    if (r < 97) return pickScaleOther();
    return pickChromatic();
  }

  // Keys / lead: 60% chord / 30% other scale / 10% chromatic.
  int r = random(100);
  if (r < 60) return pickChord(false);
  if (r < 90) return pickScaleOther();
  return pickChromatic();
}

static int companionToneRow(const CompanionHarmony &harm, int bar, CompanionRole role,
                            uint8_t channel, int prevPc, int lo, int hi, int prefer) {
  int pc = companionPickPitchClass(harm, bar, role, prevPc);
  return companionPcToChannelRow(pc, channel, lo, hi, prefer);
}

// Channels 13+14 form one two-note chord: each lane gets one distinct chord tone.
static int companionChordLaneRow(const CompanionHarmony &harm, int bar,
                                 uint8_t channel, int variation) {
  int8_t chord[4];
  bool seventh = ((variation + bar) % 4) == 0;
  int n = companionChordPcs(harm, bar, seventh, chord);
  int inversion = (variation / 3 + bar) % 3;
  int idx = channel == 13 ? inversion : (inversion + (seventh ? 3 : 2)) % n;
  int prefer = channel == 13 ? 7 : 11;
  return companionPcToChannelRow(chord[idx], channel, 1, 16, prefer);
}

static bool companionEuclideanHit(int phase, int pulses, int rotation) {
  pulses = constrain(pulses, 0, 16);
  int shifted = (phase - rotation) % 16;
  if (shifted < 0) shifted += 16;
  return pulses > 0 && ((shifted * pulses) % 16) < pulses;
}

static uint32_t companionMixHash(uint32_t value) {
  value ^= value >> 16;
  value *= 0x7feb352dUL;
  value ^= value >> 15;
  value *= 0x846ca68bUL;
  return value ^ (value >> 16);
}

static int8_t companionSoftShift(uint32_t value) {
  // Keep half of anchors conventional; distribute the rest one step around it.
  int bucket = value % 4;
  return bucket == 0 ? -1 : (bucket == 3 ? 1 : 0);
}

static CompanionGroove companionMakeGroove(const CompanionContext &ctx,
                                           int page, int variation) {
  uint32_t hash = companionMixHash((uint32_t)variation * 131UL +
                                   (uint32_t)page * 977UL + ctx.totalWeight);
  for (int phase = 0; phase < 16; phase++) {
    hash = companionMixHash(hash + (uint32_t)ctx.stepWeight[phase] * (phase + 17));
  }

  CompanionGroove groove;
  groove.hatPulses = 3 + (hash & 3);  // E(3..6,16)
  groove.hatRotation = (hash >> 3) & 15;
  groove.clapPulses = 2 + ((hash >> 7) % 3);  // E(2..4,16)
  groove.clapRotation = (hash >> 11) & 15;
  groove.tomRotation = (hash >> 15) & 15;
  groove.snareShiftA = companionSoftShift(hash >> 19);
  groove.snareShiftB = companionSoftShift(hash >> 22);
  groove.secondHalfShift = companionSoftShift(hash >> 25);
  groove.secondHalfRotation = 3 + ((hash >> 28) % 7);
  return groove;
}

static uint16_t companionMaxPhaseWeight(const uint16_t weights[16]) {
  uint16_t maximum = 0;
  for (int phase = 0; phase < 16; phase++) maximum = max(maximum, weights[phase]);
  return maximum;
}

static int companionRelativeWeight(uint16_t value, uint16_t maximum) {
  if (maximum == 0) return 0;
  return constrain((int)((uint32_t)value * 100UL / maximum), 0, 100);
}

// Sample-rate pitch lanes for drums/percussion. Homes stay role-typical, but every
// hit can move so generated pages are not stuck on one monotone row.
static int companionPercussionRow(CompanionRole role, int phase, bool structural,
                                  int variation, int &motif, const CompanionContext &ctx) {
  int home = 5;
  int lo = 1;
  int hi = 16;
  int wander = 2;
  switch (role) {
    case CompanionRole::Kick:
      home = 3;
      lo = 1;
      hi = 7;
      wander = structural ? 1 : 2;
      break;
    case CompanionRole::Snare:
      home = 5;
      lo = 2;
      hi = 11;
      wander = structural ? 2 : 3;
      break;
    case CompanionRole::ClosedHat:
      home = 7;
      lo = 3;
      hi = 14;
      wander = 4;
      break;
    case CompanionRole::Clap:
      home = 9;
      lo = 4;
      hi = 14;
      wander = structural ? 2 : 3;
      break;
    case CompanionRole::Tom:
      home = 6;
      lo = 2;
      hi = 12;
      wander = 4;
      break;
    default:
      break;
  }

  // Seed a short motif from the page variation so repeats still feel related.
  if ((phase == 0 && (variation & 1)) || random(100) < 18) {
    motif += random(1, 3);
  }

  int row = home;
  if (role == CompanionRole::ClosedHat || role == CompanionRole::Clap) {
    // Hats/claps share the melodic centre when context exists; otherwise wander.
    row = companionScaleRow(ctx, false, motif + (phase / 2) + (variation % 5));
    if (abs(row - home) > wander + 2) {
      row = home + ((row > home) ? wander : -wander) + random(-1, 2);
    }
  } else if (role == CompanionRole::Tom) {
    row = (phase >= 12) ? (home + ((phase + variation + motif) % 5) - 1)
                        : (home + ((motif + phase / 4) % 3) - 1);
  } else {
    // Kick/snare: accents near home, ghosts and pickups roam more.
    int offset = structural ? random(-1, 2) : random(-wander, wander + 1);
    if (!structural && (phase == 3 || phase == 6 || phase == 11 || phase == 14)) {
      offset += (variation & 1) ? 1 : -1;
    }
    row = home + offset + ((motif % 3) - 1);
  }

  // Occasional wider jump so a page is not visually/audibly flat.
  if (random(100) < (structural ? 8 : 22)) {
    row += random(-wander, wander + 1);
  }
  return constrain(row, lo, hi);
}

static int companionFreeRow(unsigned int step, int preferred) {
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

static int companionChannelStepLimit(uint8_t channel) {
  if (channel == 11) return 3;
  return 1;  // samples 1–8 and each 13/14 synth lane are monophonic
}

static bool companionPlaceLimited(unsigned int step, int preferredRow,
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

static int companionVelocity(const CompanionContext &ctx, int phase, int base) {
  int velocity = base;
  if (ctx.stepWeight[phase] > 0) {
    int weighted = (int)(ctx.stepVelocity[phase] / ctx.stepWeight[phase]);
    if (weighted > 0 && weighted <= 127) velocity = (velocity + weighted) / 2;
  }
  bool quarter = (phase == 0 || phase == 4 || phase == 8 || phase == 12);
  velocity += quarter ? random(5, 14) : random(-14, 8);
  return constrain(velocity, 24, 127);
}

static int companionTempoDensity() {
  int bpm = constrain((int)SMP.bpm, 40, 300);
  if (bpm >= 190) return 58;
  if (bpm >= 155) return 72;
  if (bpm <= 65) return 88;
  if (bpm <= 90) return 96;
  return 100;
}

static void generateCompanionChannelPage(int page, uint8_t channel,
                                         const CompanionContext &ctx,
                                         const CompanionHarmony &harm,
                                         const CompanionGroove &groove,
                                         int variation) {
  CompanionRole role = companionRoleForChannel(channel);
  if (role == CompanionRole::Unknown) return;

  unsigned int start = (unsigned int)(page - 1) * maxX + 1;
  unsigned int end = min((unsigned int)MAX_STEPS + 1u, start + maxX);
  int tempoDensity = companionTempoDensity();
  int percMotif = random(0, 5) + (variation % 7);
  int prevPc = -1;
  uint16_t maxStepWeight = companionMaxPhaseWeight(ctx.stepWeight);
  uint16_t maxHatWeight = companionMaxPhaseWeight(ctx.hatWeight);
  uint16_t maxClapWeight = companionMaxPhaseWeight(ctx.clapWeight);
  uint16_t maxBassWeight = companionMaxPhaseWeight(ctx.bassWeight);

  for (unsigned int c = start; c < end; c++) {
    unsigned int pageOffset = c - start;
    int segment = pageOffset / 16;
    int phase = randStep16(pageOffset + 1) - 1;
    int bar = phase / 4;  // four harmonic slots across one 16-step page
    bool quarter = (phase == 0 || phase == 4 || phase == 8 || phase == 12);
    int segmentShift = segment > 0 ? groove.secondHalfShift : 0;
    int snareAnchorA = constrain(4 + groove.snareShiftA + segmentShift, 3, 5);
    int snareAnchorB = constrain(12 + groove.snareShiftB - segmentShift, 11, 13);
    bool backbeat = (phase == snareAnchorA || phase == snareAnchorB);
    bool chordChange = (phase % 4 == 0);
    bool structural = false;
    bool pitchedPerc = false;
    int chance = 0;
    int row = 1;
    int baseVelocity = 92;

    switch (role) {
      case CompanionRole::Kick:
        chance = (phase == 0) ? 96 : ((phase == 8) ? 82 : 0);
        structural = (phase == 0 || phase == 8);
        pitchedPerc = true;
        if (ctx.bassWeight[phase]) chance = max(chance, 62);
        if ((phase == 6 || phase == 10 || phase == 15) && random(100) < 35) chance = 24;
        baseVelocity = 112;
        break;

      case CompanionRole::Snare:
        if (backbeat) {
          chance = 82;
          if (ctx.kickWeight[phase]) chance += 8;
          // A clap may layer the backbeat, but should not force a duplicate snare.
          if (ctx.clapWeight[phase]) chance -= 22;
          if (!ctx.kickWeight[phase] &&
              companionRelativeWeight(ctx.stepWeight[phase], maxStepWeight) > 70) {
            chance -= 12;
          }
          structural = true;
        } else {
          int prev = (phase + 15) % 16;
          int next = (phase + 1) % 16;
          bool nearBackbeat = (next == snareAnchorA || next == snareAnchorB ||
                               prev == snareAnchorA || prev == snareAnchorB);
          bool ghostCandidate =
              nearBackbeat || companionEuclideanHit(phase, 3, groove.tomRotation);
          chance = ghostCandidate ? 10 : 1;
          if (ctx.kickWeight[phase] || ctx.kickWeight[next]) chance += 8;
          int density = companionRelativeWeight(ctx.stepWeight[phase], maxStepWeight);
          chance += density < 25 ? 7 : (density > 70 ? -5 : 0);
        }
        pitchedPerc = true;
        baseVelocity = 102;
        break;

      case CompanionRole::ClosedHat:
        // Euclidean candidates fill ensemble gaps instead of cloning one mask.
        {
          int pulses = groove.hatPulses - (tempoDensity < 70 ? 1 : 0);
          int rotation = (groove.hatRotation +
                          (segment > 0 ? groove.secondHalfRotation : 0)) % 16;
          bool candidate = companionEuclideanHit(phase, max(3, pulses), rotation);
          int density = companionRelativeWeight(ctx.stepWeight[phase], maxStepWeight);
          chance = candidate ? (84 - density / 3) : 2;
          if (ctx.kickWeight[phase] || ctx.snareWeight[phase]) chance -= 8;
          if (ctx.snareWeight[(phase + 15) % 16]) chance += 8;
          int existingHat = companionRelativeWeight(ctx.hatWeight[phase], maxHatWeight);
          chance = chance * (100 - existingHat / 2) / 100;
          structural = candidate && chance >= 65;
        }
        pitchedPerc = true;
        if (phase == 15) chance = max(chance, 10);
        baseVelocity = 78;
        break;

      case CompanionRole::Clap:
        {
          int rotation = (groove.clapRotation +
                          (segment > 0 ? groove.secondHalfRotation : 0)) % 16;
          bool candidate =
              companionEuclideanHit(phase, groove.clapPulses, rotation);
          if (backbeat) {
            // Sometimes reinforce a snare; more often carry an empty backbeat.
            chance = ctx.snareWeight[phase] ? 43 : 69;
            structural = true;
          } else if (candidate) {
            int density = companionRelativeWeight(ctx.stepWeight[phase], maxStepWeight);
            chance = density < 50 ? 28 : 16;
          } else {
            chance = (phase == 15) ? 8 : 2;
          }
          int existingClap =
              companionRelativeWeight(ctx.clapWeight[phase], maxClapWeight);
          chance = chance * (100 - existingClap / 2) / 100;
        }
        pitchedPerc = true;
        baseVelocity = 94;
        break;

      case CompanionRole::Tom:
        {
          int rotation = (groove.tomRotation +
                          (segment > 0 ? groove.secondHalfRotation : 0)) % 16;
          chance = companionEuclideanHit(phase, 3, rotation) ? 18 : 2;
        }
        if (phase >= 13) chance = max(chance, 30);
        pitchedPerc = true;
        if (ctx.snareWeight[phase] || ctx.tomWeight[phase]) chance /= 3;
        baseVelocity = 88;
        break;

      case CompanionRole::Bass: {
        // Root/fifth-led line follows kick accents and leaves room for harmony.
        chance = ctx.kickWeight[phase] ? 76 : (quarter ? 38 : 5);
        structural = ctx.kickWeight[phase] || (phase == 0 || phase == 8);
        if (ctx.empty) chance = quarter ? 62 : ((phase == 6 || phase == 14) ? 12 : 2);
        int existingBass =
            companionRelativeWeight(ctx.bassWeight[phase], maxBassWeight);
        if (existingBass > 0) chance = chance * max(20, 70 - existingBass / 2) / 100;
        if (ctx.harmonicWeight[phase] && !ctx.kickWeight[phase]) chance = chance * 65 / 100;
        if (!quarter && ctx.snareWeight[(phase + 1) % 16]) chance = max(chance, 17);
        if (!ctx.kickWeight[phase] &&
            companionRelativeWeight(ctx.stepWeight[phase], maxStepWeight) > 75) {
          chance = chance * 70 / 100;
        }
        baseVelocity = 104;
        row = companionToneRow(harm, bar, role, channel, prevPc, 1, 8, 3);
        prevPc = companionPitchClassForRow(channel, row);
        break;
      }

      case CompanionRole::Keys: {
        // Sample key lane 7 stays mono. Synth lanes 13+14 form a dyad together,
        // one chord tone per lane; neither lane receives stacked notes.
        baseVelocity = 82;
        if (chordChange) {
          chance = (phase == 0 || phase == 8) ? 88 : 62;
          structural = true;
        } else if (phase == 2 || phase == 6 || phase == 10 || phase == 14) {
          chance = 10;
        } else {
          chance = 2;
        }
        if (channel == 7 && ctx.harmonicWeight[phase]) {
          chance = chance * (structural ? 65 : 45) / 100;
        }
        if (!structural) chance = chance * tempoDensity / 100;
        chance = constrain(chance, 0, 100);
        // The deterministic gate keeps channels 13 and 14 rhythmically paired.
        int gate = (variation * 29 + phase * 17 + segment * 37) % 100;
        if (gate < chance) {
          if (channel == 13 || channel == 14) {
            row = companionChordLaneRow(harm, bar, channel,
                                        variation + segment * 5);
          } else {
            row = companionToneRow(harm, bar, role, channel, prevPc, 1, 16, 9);
          }
          prevPc = companionPitchClassForRow(channel, row);
        } else {
          continue;
        }
        break;
      }

      case CompanionRole::VocalsPad: {
        // Channel 8 is mono: sparse sustained chord tones, never stacked chords.
        baseVelocity = 72;
        if (phase == 0 || phase == 8) {
          chance = 78;
          structural = true;
        } else if (phase == 4 || phase == 12) {
          chance = 18;
        } else {
          chance = 0;
        }
        if (!structural) chance = chance * tempoDensity / 100;
        chance = constrain(chance, 0, 100);
        if ((int)random(100) < chance) {
          row = companionToneRow(harm, bar, role, channel, prevPc, 1, 16, 10);
          prevPc = companionPitchClassForRow(channel, row);
          companionPlaceLimited(c, row, channel,
                                companionVelocity(ctx, phase, baseVelocity));
        }
        continue;
      }

      default:
        break;
    }

    // Tempo thins embellishments, not the 4/4 anchors that make the result usable.
    if (role != CompanionRole::Keys && role != CompanionRole::VocalsPad) {
      if (!structural) chance = chance * tempoDensity / 100;
      chance = constrain(chance, 0, 100);
      if ((int)random(100) >= chance) continue;
    }
    if (pitchedPerc) {
      row = companionPercussionRow(role, phase, structural, variation, percMotif, ctx);
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

// Generate rhythmic patterns for channels 1-4
FLASHMEM void generateRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Define 4/4 time signature patterns with more sophisticated rhythms
  const int strongBeats[] = {1, 5, 9, 13};  // Beat 1, 2, 3, 4
  const int weakBeats[] = {3, 7, 11, 15};   // Off-beats
  
  // Different rhythm patterns for different channels
  int patternType = (channel - 1) % 4;
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel); // tile 16-step groove across maxX
    
    bool shouldPlay = false;
    int velocity = defaultVelocity;
    
    switch(patternType) {
      case 0: // Channel 1: Kick-like pattern
        shouldPlay = (beatPosition == 1 || beatPosition == 9); // Beat 1 and 3
        // Add some ghost notes for groove
        if(random(0, 100) < 20 && (beatPosition == 5 || beatPosition == 13)) {
          shouldPlay = true;
          velocity = 60; // Ghost note
        } else {
          velocity = 120; // Strong kick
        }
        break;
      case 1: // Channel 2: Snare-like pattern  
        shouldPlay = (beatPosition == 5 || beatPosition == 13); // Beat 2 and 4
        // Add some ghost snares
        if(random(0, 100) < 30 && (beatPosition == 3 || beatPosition == 7 || beatPosition == 11 || beatPosition == 15)) {
          shouldPlay = true;
          velocity = 70; // Ghost snare
        } else {
          velocity = 100; // Strong snare
        }
        break;
      case 2: // Channel 3: Hi-hat pattern
        shouldPlay = (beatPosition % 2 == 0); // Every other beat
        // Add some open hi-hats
        if(random(0, 100) < 15 && (beatPosition == 5 || beatPosition == 13)) {
          velocity = 90; // Open hi-hat
        } else {
          velocity = 80; // Closed hi-hat
        }
        break;
      case 3: // Channel 4: Percussion pattern
        // More musical percussion with accent patterns
        if(beatPosition == 1 || beatPosition == 9) {
          shouldPlay = true;
          velocity = 110; // Accent
        } else if(random(0, 100) < 40) {
          shouldPlay = true;
          velocity = 85; // Regular hit
        }
        break;
    }
    
    if(shouldPlay) {
      // Choose a rhythmically appropriate note with harmonic awareness
      int noteRow = 1; // Default to root
      if(harmony.hasRoot) {
        noteRow = harmony.rootNote;
        // Add some harmonic variation
        if(random(0, 100) < 20) {
          if(harmony.hasFifth) noteRow = 5;
          else if(harmony.hasThird) noteRow = (harmony.isMajor) ? 3 : 6;
        }
      } else if(harmony.hasFifth) {
        noteRow = 5;
      } else {
        noteRow = random(1, 9); // Random if no harmony detected
      }
      
      placeRandomNoteIfEmpty(c, noteRow, channel, velocity);
    }
  }
}

// Generate melodic patterns for channels 5-8
FLASHMEM void generateMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Define scale notes based on harmonic analysis
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
    // Default pentatonic scale
    int pentatonic[] = {1, 3, 5, 8, 10};
    for(int i = 0; i < 5; i++) {
      scaleNotes[i] = pentatonic[i];
    }
    scaleSize = 5;
  }
  
  // Different melodic styles for different voice channels
  int voiceType = (channel - 5) % 4;
  int phraseLength = 4; // 4-beat phrases
  int currentNote = 0;
  int lastDirection = 0; // Track melodic direction for smoother lines
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel); // tile 16-step groove across maxX
    
    // Create melodic phrases with musical intervals
    if(beatPosition % phraseLength == 1) {
      // Start of phrase - choose a strong note
      currentNote = random(0, scaleSize);
      lastDirection = 0;
    } else {
      // Continue phrase with musical motion
      int direction = random(0, 5); // More options for musical variety
      
      switch(voiceType) {
        case 0: // Voice 1: Smooth step-wise motion
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;
          else if(direction == 3 && currentNote < scaleSize - 2) currentNote += 2;
          else if(direction == 4 && currentNote > 1) currentNote -= 2;
          break;
        case 1: // Voice 2: More leaps and jumps
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;
          else if(direction == 3 && currentNote < scaleSize - 3) currentNote += 3;
          else if(direction == 4 && currentNote > 2) currentNote -= 3;
          break;
        case 2: // Voice 3: Arpeggio-like patterns
          if(direction == 1) currentNote = (currentNote + 2) % scaleSize;
          else if(direction == 2) currentNote = (currentNote + 3) % scaleSize;
          else if(direction == 3) currentNote = (currentNote + 4) % scaleSize;
          else if(direction == 4) currentNote = (currentNote - 1 + scaleSize) % scaleSize;
          break;
        case 3: // Voice 4: Sustained notes with occasional movement
          if(direction == 1 && currentNote < scaleSize - 1) currentNote++;
          else if(direction == 2 && currentNote > 0) currentNote--;
          // Otherwise stay on same note (sustained)
          break;
        }
      }
    
    // Add rhythmic variation based on voice type
    bool shouldPlay = false;
    int velocity = defaultVelocity;
    
    switch(voiceType) {
      case 0: // Voice 1: Regular rhythm
        if(beatPosition % 2 == 1) shouldPlay = true;
        else if(random(0, 100) < 30) shouldPlay = true;
        velocity += random(-15, 16);
        break;
      case 1: // Voice 2: Syncopated rhythm
        if(random(0, 100) < 60) shouldPlay = true;
        velocity += random(-10, 21);
        break;
      case 2: // Voice 3: Arpeggio rhythm
        if(beatPosition % 4 == 1) shouldPlay = true;
        else if(random(0, 100) < 25) shouldPlay = true;
        velocity += random(-5, 26);
        break;
      case 3: // Voice 4: Sustained notes
        if(beatPosition == 1 || beatPosition == 9) shouldPlay = true;
        else if(random(0, 100) < 20) shouldPlay = true;
        velocity += random(-5, 11); // Less velocity variation
        break;
    }
    
    if(shouldPlay && scaleSize > 0) {
      int noteRow = scaleNotes[currentNote];
      placeRandomNoteIfEmpty(c, noteRow, channel, velocity);
    }
  }
}

// Generate bass or main melody for channel 11
FLASHMEM void generateBassOrMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Channel 11 should primarily be bass lines (80% bass, 20% melody)
  bool generateBass = (random(0, 100) < 80);
  
  if(generateBass) {
    // Generate bass line
    generateBassLine(start, end, channel, harmony);
  } else {
    // Generate main melody (with reduced chords)
    generateMainMelody(start, end, channel, harmony);
  }
}

// Generate bass line
FLASHMEM void generateBassLine(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Classic bass notes: root, third, fifth, octave (low register)
  int bassNotes[] = {1, 2, 3, 4, 5, 6, 7, 8}; // Low register (C3 to C4)
  int bassSize = 8;
  
  int currentBassNote = 0;
  int phraseLength = 4; // Shorter phrases for bass
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel); // tile 16-step groove across maxX
    
    // Create bass phrases
    if(beatPosition % phraseLength == 1) {
      currentBassNote = random(0, bassSize);
    } else {
      // Bass motion - mostly stepwise with occasional leaps
      int direction = random(0, 6);
      if(direction == 1 && currentBassNote < bassSize - 1) currentBassNote++;
      else if(direction == 2 && currentBassNote > 0) currentBassNote--;
      else if(direction == 3 && currentBassNote < bassSize - 3) currentBassNote += 2; // Leap up
      else if(direction == 4 && currentBassNote > 2) currentBassNote -= 2; // Leap down
    }
    
    // Bass plays on strong beats with some off-beat variation
    bool shouldPlay = false;
    if(beatPosition == 1 || beatPosition == 5 || beatPosition == 9 || beatPosition == 13) {
      shouldPlay = true; // Strong beats
    } else if(random(0, 100) < 25) {
      shouldPlay = true; // Some off-beats
    }
    
    if(shouldPlay) {
      int noteRow = bassNotes[currentBassNote];
      placeRandomNoteIfEmpty(c, noteRow, channel, defaultVelocity + random(10, 31));
    }
  }
}

// Generate main melody
FLASHMEM void generateMainMelody(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Melody uses higher register and more complex patterns
  int melodyNotes[] = {10, 12, 13, 15, 1, 3, 5, 6};
  int melodySize = 8;
  
  int currentNote = 0;
  int phraseLength = 8; // Longer phrases for melody
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel); // tile 16-step groove across maxX
    
    // Create melodic phrases
    if(beatPosition % phraseLength == 1) {
      currentNote = random(0, melodySize);
    } else {
      // Melodic motion with larger intervals
      int direction = random(0, 5); // More variation
      if(direction == 1 && currentNote < melodySize - 1) currentNote++;
      else if(direction == 2 && currentNote > 0) currentNote--;
      else if(direction == 3 && currentNote < melodySize - 2) currentNote += 2;
      else if(direction == 4 && currentNote > 1) currentNote -= 2;
    }
    
    // Melody plays more frequently
    bool shouldPlay = false;
    if(random(0, 100) < 70) shouldPlay = true;
    
    if(shouldPlay && melodySize > 0) {
      // Decide how many notes to play (mostly single notes, occasional chords)
      int noteCount;
      int chordChance = random(0, 100);
      if(chordChance < 80) {
        noteCount = 1; // 80% single notes
      } else if(chordChance < 95) {
        noteCount = 2; // 15% two-note chords
      } else {
        noteCount = 3; // 5% three-note chords
      }
      
      int rootNote = melodyNotes[currentNote];
      
      if(noteCount == 1) {
        placeRandomNoteIfEmpty(c, rootNote, channel, defaultVelocity + random(-10, 31));
      } else if(noteCount == 2) {
        // Two-note chord - convert rootNote (1-16) to piano index (0-15), add intervals, convert back
        int rootPianoIndex = rootNote - 1; // Convert 1-16 to 0-15
        int intervalChoice = random(0, 4); // 0-3 for different intervals
        int secondPianoIndex;
        
        switch(intervalChoice) {
          case 0: secondPianoIndex = rootPianoIndex + 3; break;  // Minor third (C-Eb)
          case 1: secondPianoIndex = rootPianoIndex + 5; break;  // Perfect fourth (C-F)
          case 2: secondPianoIndex = rootPianoIndex + 7; break;  // Perfect fifth (C-G)
          case 3: secondPianoIndex = rootPianoIndex + 12; break; // Perfect octave (C-C)
        }
        
        // Ensure piano index stays within valid range (0-15)
        if(secondPianoIndex > 15) secondPianoIndex -= 12;  // Octave down
        if(secondPianoIndex < 0) secondPianoIndex += 12;   // Octave up
        
        int secondNote = secondPianoIndex + 1; // Convert back to 1-16
        
        placeRandomNoteIfEmpty(c, rootNote, channel, defaultVelocity + random(-5, 21));
        placeRandomNoteIfEmpty(c, secondNote, channel, defaultVelocity + random(-10, 16));
      } else { // noteCount == 3
        // Three-note chord - 50% major, 50% minor (with occasional sus4)
        int chordType;
        if(random(0, 100) < 90) {
          // 90% chance for major or minor (45% each)
          chordType = random(0, 2); // 0 = major, 1 = minor
        } else {
          // 10% chance for sus4
          chordType = 2;
        }
        
        // Convert rootNote (1-16) to piano index (0-15)
        int rootPianoIndex = rootNote - 1;
        int secondPianoIndex, thirdPianoIndex;
        
        switch(chordType) {
          case 0: // Major triad (C-E-G)
            secondPianoIndex = rootPianoIndex + 4;  // Major third (C to E)
            thirdPianoIndex = rootPianoIndex + 7;   // Perfect fifth (C to G)
            break;
          case 1: // Minor triad (C-Eb-G)
            secondPianoIndex = rootPianoIndex + 3;  // Minor third (C to Eb)
            thirdPianoIndex = rootPianoIndex + 7;    // Perfect fifth (C to G)
            break;
          case 2: // Sus4 chord (C-F-G)
            secondPianoIndex = rootPianoIndex + 5;  // Perfect fourth (C to F)
            thirdPianoIndex = rootPianoIndex + 7;    // Perfect fifth (C to G)
            break;
        }
        
        // Ensure piano indices stay within valid range (0-15)
        if(secondPianoIndex > 15) secondPianoIndex -= 12;  // Octave down
        if(thirdPianoIndex > 15) thirdPianoIndex -= 12;    // Octave down
        if(secondPianoIndex < 0) secondPianoIndex += 12;   // Octave up
        if(thirdPianoIndex < 0) thirdPianoIndex += 12;      // Octave up
        
        int secondNote = secondPianoIndex + 1; // Convert back to 1-16
        int thirdNote = thirdPianoIndex + 1;   // Convert back to 1-16
        
        placeRandomNoteIfEmpty(c, rootNote, channel, defaultVelocity + random(-5, 21));
        placeRandomNoteIfEmpty(c, secondNote, channel, defaultVelocity + random(-10, 16));
        placeRandomNoteIfEmpty(c, thirdNote, channel, defaultVelocity + random(-10, 16));
      }
    }
  }
}

// Context-aware rhythmic pattern generation that preserves base page rhythm
FLASHMEM void generateContextAwareRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony, BasePagePattern* basePattern, int pageOffset) {
  // Preserve the original rhythm structure with subtle variations
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = ((x_rel - 1) % maxX) + 1;
    
    // Use the base pattern rhythm as foundation
    bool shouldPlay = basePattern->rhythmPattern[beatPosition - 1] == 1;
    
    // Add subtle variations based on page progression
    if (shouldPlay) {
      // Occasionally skip a note for variation (8% chance - subtle)
      if (random(0, 100) < 8) {
        shouldPlay = false;
      }
    } else {
      // Occasionally add a note for variation (4% chance - subtle)
      if (random(0, 100) < 4) {
        shouldPlay = true;
      }
    }
    
    if (shouldPlay) {
      // Use the base pattern's note row with harmonic progression
      int baseNoteRow = basePattern->noteRows[beatPosition - 1];
      int noteRow = baseNoteRow;
      
        // Apply harmonic progression based on page offset - create real musical progressions
        if (pageOffset > 0) {
          // For rhythm channels (1-4), use conservative pitch variations
          if (channel >= 1 && channel <= 4) {
            noteRow = createRhythmicProgression(baseNoteRow, pageOffset, harmony);
          } else {
            // For melodic channels, use full harmonic progressions
            noteRow = createHarmonicProgression(baseNoteRow, pageOffset, harmony);
          }
        }
      
      // Use base pattern velocity with slight variation
      int baseVelocity = basePattern->velocities[beatPosition - 1];
      int velocity = baseVelocity + random(-10, 11);
      if (velocity < 40) velocity = 40;
      if (velocity > 127) velocity = 127;
      
      placeRandomNoteIfEmpty(c, noteRow, channel, velocity);
    }
  }
}

// Context-aware melodic pattern generation that preserves base page melody
FLASHMEM void generateContextAwareMelodicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony, BasePagePattern* basePattern, int pageOffset) {
  // Preserve the original melodic structure with harmonic progressions
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = ((x_rel - 1) % maxX) + 1;
    
    // Use the base pattern rhythm as foundation
    bool shouldPlay = basePattern->rhythmPattern[beatPosition - 1] == 1;
    
    // Add subtle variations based on page progression
    if (shouldPlay) {
      // Occasionally skip a note for variation (6% chance - subtle)
      if (random(0, 100) < 6) {
        shouldPlay = false;
      }
    } else {
      // Occasionally add a note for variation (3% chance - subtle)
      if (random(0, 100) < 3) {
        shouldPlay = true;
      }
    }
    
    if (shouldPlay) {
      // Use the base pattern's note row with harmonic progression
      int baseNoteRow = basePattern->noteRows[beatPosition - 1];
      int noteRow = baseNoteRow;
      
      // Apply harmonic progression based on page offset - create real musical progressions
      if (pageOffset > 0) {
        // For rhythm channels (1-4), use conservative pitch variations
        if (channel >= 1 && channel <= 4) {
          noteRow = createRhythmicProgression(baseNoteRow, pageOffset, harmony);
        } else {
          // For melodic channels, use full melodic progressions
          noteRow = createMelodicProgression(baseNoteRow, pageOffset, harmony);
        }
      }
      
      // Use base pattern velocity with slight variation
      int baseVelocity = basePattern->velocities[beatPosition - 1];
      int velocity = baseVelocity + random(-8, 9);
      if (velocity < 40) velocity = 40;
      if (velocity > 127) velocity = 127;
      
      placeRandomNoteIfEmpty(c, noteRow, channel, velocity);
    }
  }
}

// Create dynamic harmonic progressions with more variation
FLASHMEM int createHarmonicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {
  // Analyze the base note's harmonic function within the detected scale
  int scaleNotes[16];
  int scaleSize = 0;
  
  // Build scale based on harmony analysis
  if (harmony.isMajor) {
    // Major scale: C D E F G A B C (1 3 5 6 8 10 12 13)
    int majorScale[] = {1, 3, 5, 6, 8, 10, 12, 13};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = majorScale[i];
    }
    scaleSize = 8;
  } else if (harmony.isMinor) {
    // Minor scale: A B C D E F G A (6 8 10 11 13 15 1 3)
    int minorScale[] = {6, 8, 10, 11, 13, 15, 1, 3};
    for (int i = 0; i < 8; i++) {
      scaleNotes[i] = minorScale[i];
    }
    scaleSize = 8;
  } else {
    // Pentatonic scale as fallback: C D E G A (1 3 5 8 10)
    int pentatonic[] = {1, 3, 5, 8, 10};
    for (int i = 0; i < 5; i++) {
      scaleNotes[i] = pentatonic[i];
    }
    scaleSize = 5;
  }
  
  // Find the base note's position in the scale
  int scaleDegree = -1;
  for (int i = 0; i < scaleSize; i++) {
    if (baseNote == scaleNotes[i]) {
      scaleDegree = i;
      break;
    }
  }
  
  // If base note not in scale, find closest
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
  
  // Create subtle harmonic progressions that preserve character
  // Use gentle, character-preserving progressions
  int progressionPatterns[][8] = {
    {0, 5, 3, 4, 0, 5, 3, 4},  // I-vi-IV-V (classic, gentle)
    {0, 4, 5, 3, 0, 4, 5, 3},  // I-V-vi-IV (gentle pop progression)
    {5, 3, 0, 4, 5, 3, 0, 4},  // vi-IV-I-V (minor start, gentle)
    {0, 2, 3, 4, 0, 2, 3, 4},  // I-iii-IV-V (major with gentle third)
    {0, 4, 2, 3, 0, 4, 2, 3},  // I-V-iii-IV (gentle major progression)
    {0, 1, 3, 4, 0, 1, 3, 4},  // I-ii-IV-V (gentle with minor second)
    {0, 3, 4, 5, 0, 3, 4, 5},  // I-IV-V-vi (gentle major progression)
    {0, 4, 1, 3, 0, 4, 1, 3}   // I-V-ii-IV (gentle with minor second)
  };
  
  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 8;
  int targetDegree = progressionPatterns[patternIndex][progressionStep];
  
  // Add subtle randomness for gentle variation
  if (random(0, 100) < 10) { // 10% chance for subtle variation
    targetDegree += random(-1, 2); // Very gentle offset
  }
  
  // Apply the progression
  int newScaleDegree = (scaleDegree + targetDegree) % scaleSize;
  if (newScaleDegree < 0) newScaleDegree += scaleSize;
  
  int resultNote = scaleNotes[newScaleDegree];
  
  // Add gentle octave variations (much less frequent)
  if (random(0, 100) < 8) { // 8% chance for octave change
    if (random(0, 2) == 0) {
      resultNote += 7; // Up an octave
      if (resultNote > 16) resultNote -= 7; // Keep in range
    } else {
      resultNote -= 7; // Down an octave
      if (resultNote < 1) resultNote += 7; // Keep in range
    }
  }
  
  return resultNote;
}

// Create dynamic melodic progressions with more variation and excitement
FLASHMEM int createMelodicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {
  // Analyze the base note's harmonic function
  int scaleNotes[16];
  int scaleSize = 0;
  
  // Build scale based on harmony analysis
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
  
  // Find the base note's position in the scale
  int scaleDegree = -1;
  for (int i = 0; i < scaleSize; i++) {
    if (baseNote == scaleNotes[i]) {
      scaleDegree = i;
      break;
    }
  }
  
  // If base note not in scale, find closest
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
  
  // Create gentle melodic progressions that preserve character
  // Use subtle, character-preserving melodic patterns
  int melodicPatterns[][8] = {
    {0, 1, 2, 1, 0, -1, -2, -1},  // Gentle stepwise motion
    {0, 2, 4, 2, 0, -2, -4, -2},  // Gentle thirds
    {0, 4, 2, 6, 4, 0, -2, 2},    // Gentle arpeggio-like
    {0, 1, 3, 2, 4, 3, 5, 4},    // Gentle chromatic with scale notes
    {0, 2, 1, 3, 2, 4, 3, 5},    // Gentle alternating motion
    {0, 3, 1, 4, 2, 5, 3, 6},    // Gentle mixed intervals
    {0, -1, 1, -2, 2, -3, 3, -4}, // Gentle oscillating
    {0, 1, 0, 2, 1, 3, 2, 4}     // Gentle gradual ascent
  };
  
  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 8;
  int melodicOffset = melodicPatterns[patternIndex][progressionStep];
  
  // Add subtle randomness for gentle variation
  if (random(0, 100) < 12) { // 12% chance for subtle variation
    melodicOffset += random(-1, 2); // Very gentle offset
  }
  
  // Apply the melodic progression
  int newScaleDegree = (scaleDegree + melodicOffset) % scaleSize;
  if (newScaleDegree < 0) newScaleDegree += scaleSize;
  
  int resultNote = scaleNotes[newScaleDegree];
  
  // Add gentle octave variations (much less frequent)
  if (random(0, 100) < 6) { // 6% chance for octave change
    if (random(0, 2) == 0) {
      resultNote += 7; // Up an octave
      if (resultNote > 16) resultNote -= 7; // Keep in range
    } else {
      resultNote -= 7; // Down an octave
      if (resultNote < 1) resultNote += 7; // Keep in range
    }
  }
  
  // Add very subtle chromatic passing tones
  if (random(0, 100) < 5) { // 5% chance for chromatic variation
    if (random(0, 2) == 0) {
      resultNote += 1; // Up a semitone
      if (resultNote > 16) resultNote -= 1;
    } else {
      resultNote -= 1; // Down a semitone
      if (resultNote < 1) resultNote += 1;
    }
  }
  
  return resultNote;
}

// Create very conservative rhythmic progressions that stay close to original pitch
FLASHMEM int createRhythmicProgression(int baseNote, int pageOffset, HarmonicAnalysis harmony) {
  // For rhythm channels, we want to stay very close to the original pitch
  // Only make minimal changes to maintain the rhythmic character
  
  // Very conservative pitch variations - mostly stay on the same note
  int rhythmicVariations[][4] = {
    {0, 0, 0, 0},    // Stay exactly the same
    {0, 1, 0, -1},   // Very gentle up/down motion
    {0, 0, 1, 0},    // Occasional step up
    {0, 0, -1, 0},   // Occasional step down
    {0, 2, 0, -2},   // Very gentle third motion
    {0, 0, 2, 0},    // Occasional third up
    {0, 0, -2, 0},   // Occasional third down
    {0, 1, -1, 0}    // Gentle up then down
  };
  
  int patternIndex = pageOffset % 8;
  int progressionStep = (pageOffset / 8) % 4;
  int pitchOffset = rhythmicVariations[patternIndex][progressionStep];
  
  // Add very minimal randomness (much less than melodic channels)
  if (random(0, 100) < 5) { // Only 5% chance for variation
    pitchOffset += random(-1, 2); // Very gentle offset
  }
  
  // Apply the minimal progression
  int resultNote = baseNote + pitchOffset;
  
  // Keep within bounds
  if (resultNote < 1) resultNote = 1;
  if (resultNote > 16) resultNote = 16;
  
  // Very rarely add octave variation (much less than melodic channels)
  if (random(0, 100) < 2) { // Only 2% chance for octave change
    if (random(0, 2) == 0) {
      resultNote += 7; // Up an octave
      if (resultNote > 16) resultNote -= 7; // Keep in range
    } else {
      resultNote -= 7; // Down an octave
      if (resultNote < 1) resultNote += 7; // Keep in range
    }
  }
  
  return resultNote;
}

// Generate genre-based track from page 1 to selected length
FLASHMEM void generateGenreTrack() {
  // Clear ALL pages first (complete reset)
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
  
  // Generate genre-specific patterns ONLY from page 1 to genreLength (exact count)
  for (unsigned int page = 1; page <= genreLength; page++) {
    unsigned int start = (page - 1) * 16 + 1;
    unsigned int end = page * 16;
    
    switch (genreType) {
      case 0: // BLNK - Already cleared above
        break;
        
      case 1: // TECH - Techno patterns
        generateTechnoPattern(start, end, page);
        break;
        
      case 2: // HIPH - Hip-hop patterns
        generateHipHopPattern(start, end, page);
        break;
        
      case 3: // DNB patterns
        generateDnBPattern(start, end, page);
        break;
        
      case 4: // HOUS - House patterns
        generateHousePattern(start, end, page);
        break;
        
      case 5: // AMBT - Ambient patterns
        generateAmbientPattern(start, end, page);
        break;
    }
  }
  
  // Set genre-appropriate BPM
  setGenreBPM();

  extern void resetAllChannelVolumesToDefault();
  resetAllChannelVolumesToDefault();
  
  // Auto close menu after generation
  switchMode(&draw);
}

// Set genre-appropriate BPM
FLASHMEM void setGenreBPM() {
  int targetBPM = 100; // Default BPM
  
  switch (genreType) {
    case 0: // BLNK
      targetBPM = 100; // Default
      break;
    case 1: // TECH - Techno
      targetBPM = 128 + random(-4, 5); // 124-132 BPM
      break;
    case 2: // HIPH - Hip-hop
      targetBPM = 85 + random(-3, 4); // 82-88 BPM
      break;
    case 3: // DNB pattern
      targetBPM = 174 + random(-4, 5); // 170-178 BPM
      break;
    case 4: // HOUS - House
      targetBPM = 124 + random(-4, 5); // 120-128 BPM
      break;
    case 5: // AMBT - Ambient
      targetBPM = 70 + random(-5, 6); // 65-75 BPM
      break;
  }
  
  // Set the BPM directly in SMP.bpm and update the volume_bpm mode
  SMP.bpm = targetBPM;
  
  // Update the volume_bpm mode position
  volume_bpm.pos[3] = targetBPM;
  
  // If currently in volume_bpm mode, update the encoder
  if (currentMode == &volume_bpm) {
    Encoder[3].writeCounter((int32_t)targetBPM);
  }
  
  // Apply the BPM immediately by updating the playback timer
  applyBPMDirectly(targetBPM);
}

// Apply BPM directly without relying on MIDI clock condition
void applyBPMDirectly(int bpm) {
  if (bpm > 0) { // Avoid division by zero
    playNoteInterval = 60000000.0 / ((double)bpm * 4.0);
    playTimer.update((uint32_t)round(playNoteInterval));
  }
}

// Techno pattern generation - Enhanced and dynamic
FLASHMEM void generateTechnoPattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMinor = true;
  const int baseNote = 8;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);
    
    // Dynamic kick patterns - varies by page
    bool kickPlay = false;
    if (pageOffset % 4 == 0) {
      // Standard four-on-floor
      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 4 == 1) {
      // Techno variation: 1, 3, 5, 7, 9, 11, 13, 15
      kickPlay = (beat % 2 == 1);
    } else if (pageOffset % 4 == 2) {
      // Break pattern: 1, 4, 7, 9, 12, 15
      kickPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 9 || beat == 12 || beat == 15);
    } else {
      // Complex: 1, 2, 5, 6, 9, 10, 13, 14
      kickPlay = (beat == 1 || beat == 2 || beat == 5 || beat == 6 || beat == 9 || beat == 10 || beat == 13 || beat == 14);
    }
    
    if (kickPlay) {
      int vel = 118 + ((beat == 1) ? 6 : 0) + random(-3, 4);
      placeGenNote(c, 3, 1, vel, 100, 1);
    }
    
    // Dynamic snare patterns
    bool snarePlay = false;
    if (pageOffset % 3 == 0) {
      // Standard backbeat
      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {
      // Ghost snares
      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 60 + random(-10, 11); // Ghost
      } else {
        note[c][5].velocity = 80 + random(-10, 11); // Main
      }
    } else {
      // Roll pattern
      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 50 + random(-10, 11); // Roll
      } else {
        note[c][5].velocity = 75 + random(-10, 11); // Main
      }
    }
    
    if (snarePlay) {
      int vel = 112 + random(-4, 5);
      uint8_t prob = 100;
      if (pageOffset % 3 == 1 && (beat == 3 || beat == 11)) {
        vel = 58 + random(-4, 5); // ghost
        prob = 45;
      } else if (pageOffset % 3 == 2 && (beat == 4 || beat == 6 || beat == 12 || beat == 14)) {
        vel = 62 + random(-5, 6); // roll
        prob = 60;
      }
      placeGenNote(c, 5, 2, vel, prob, 1);
    }
    
    // Complex hi-hat patterns
    if (beat % 2 == 0) {
      int vel = 72 + ((beat % 4 == 0) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 7, 3, vel, 100, 1);
    }
    
    // Open hi-hats
    if (beat == 5 || beat == 13) {
      int vel = 92 + random(-4, 5);
      uint8_t prob = (pageOffset % 4 == 2) ? 55 : 85;
      placeGenNote(c, 8, 4, vel, prob, 1);
    }
    
    // Additional percussion
    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      int vel = 62 + random(-5, 6);
      placeGenNote(c, 6, 5, vel, 40, 1);
    }
    
    // Dynamic bass lines
    bool bassPlay = false;
    if (pageOffset % 2 == 0) {
      // Standard techno bass
      bassPlay = (beat == 1 || beat == 9);
    } else {
      // Complex bass pattern
      bassPlay = (beat == 1 || beat == 3 || beat == 5 || beat == 7 || beat == 9 || beat == 11 || beat == 13 || beat == 15);
    }
    
    if (bassPlay) {
      int row = createRhythmicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      if (row > 9) row -= 7;
      int vel = 102 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 100, 1);
    }
    
    // Melodic elements
    if (random(0, 100) < 18) {
      int row = createMelodicProgression(chordRoot, pageOffset + (beat / 2), harmony);
      int vel = 84 + ((beat == 9) ? 8 : 0) + random(-4, 5);
      placeGenNote(c, row, 7, vel, 70, 1);
    }
    
    // Additional stabs
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

// Hip-hop pattern generation - Enhanced and dynamic
FLASHMEM void generateHipHopPattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMinor = true;
  const int baseNote = 6;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);
    
    // Dynamic kick patterns - varies by page
    bool kickPlay = false;
    if (pageOffset % 4 == 0) {
      // Classic hip-hop: 1, 7, 11
      kickPlay = (beat == 1 || beat == 7 || beat == 11);
    } else if (pageOffset % 4 == 1) {
      // Trap variation: 1, 3, 7, 11, 15
      kickPlay = (beat == 1 || beat == 3 || beat == 7 || beat == 11 || beat == 15);
    } else if (pageOffset % 4 == 2) {
      // Complex: 1, 4, 7, 10, 13
      kickPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 10 || beat == 13);
    } else {
      // Break pattern: 1, 2, 7, 8, 11, 12
      kickPlay = (beat == 1 || beat == 2 || beat == 7 || beat == 8 || beat == 11 || beat == 12);
    }
    
    if (kickPlay) {
      int vel = 118 + ((beat == 1) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 3, 1, vel, 100, 1);
    }
    
    // Dynamic snare patterns
    bool snarePlay = false;
    if (pageOffset % 3 == 0) {
      // Standard backbeat
      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {
      // Ghost snares
      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 70 + random(-10, 11); // Ghost
      } else {
        note[c][5].velocity = 100 + random(-10, 11); // Main
      }
    } else {
      // Roll pattern
      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 60 + random(-10, 11); // Roll
      } else {
        note[c][5].velocity = 95 + random(-10, 11); // Main
      }
    }
    
    if (snarePlay) {
      int vel = 112 + random(-4, 5);
      uint8_t prob = 100;
      if (pageOffset % 3 == 1 && (beat == 3 || beat == 11)) {
        vel = 60 + random(-5, 6); // ghost
        prob = 45;
      } else if (pageOffset % 3 == 2 && (beat == 4 || beat == 6 || beat == 12 || beat == 14)) {
        vel = 64 + random(-5, 6); // roll
        prob = 60;
      }
      placeGenNote(c, 5, 2, vel, prob, 1);
    }
    
    // Complex hi-hat patterns
    if (beat % 2 == 0) {
      int vel = 68 + ((beat == 8 || beat == 16) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 7, 3, vel, 90, 1);
    }
    
    // Open hi-hats
    if (beat == 5 || beat == 13) {
      int vel = 92 + random(-4, 5);
      uint8_t prob = (pageOffset % 4 == 3) ? 55 : 80;
      placeGenNote(c, 8, 4, vel, prob, 1);
    }
    
    // Additional percussion
    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      int vel = 58 + random(-5, 6);
      placeGenNote(c, 6, 5, vel, 30, 1);
    }
    
    // Dynamic bass lines
    bool bassPlay = false;
    if (pageOffset % 2 == 0) {
      // Standard hip-hop bass
      bassPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else {
      // Complex bass pattern
      bassPlay = (beat == 1 || beat == 3 || beat == 5 || beat == 7 || beat == 9 || beat == 11 || beat == 13 || beat == 15);
    }
    
    if (bassPlay) {
      int row = createRhythmicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      if (row > 9) row -= 7;
      int vel = 98 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 100, 1);
    }
    
    // Melodic elements
    if (random(0, 100) < 14) {
      int row = createMelodicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      int vel = 88 + random(-4, 5);
      placeGenNote(c, row, 7, vel, 60, 2);
    }
    
    // Additional stabs
    if (random(0, 100) < 8) {
      int row = chordRoot;
      if (random(0, 2) == 0) row += 3;
      if (row > 16) row -= 7;
      int vel = 90 + random(-4, 5);
      placeGenNote(c, row, 8, vel, 45, 4);
    }
  }
}

// DNB pattern generation - More dynamic and complex
FLASHMEM void generateDnBPattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMinor = true;
  const int baseNote = 6;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1; // For variation across pages
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);
    
    // Dynamic kick pattern - varies by page
    bool kickPlay = false;
    if (pageOffset % 4 == 0) {
      // Standard DnB: 1, 9
      kickPlay = (beat == 1 || beat == 9);
    } else if (pageOffset % 4 == 1) {
      // Variation: 1, 5, 9, 13 (four-on-floor)
      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 4 == 2) {
      // Break pattern: 1, 7, 11
      kickPlay = (beat == 1 || beat == 7 || beat == 11);
    } else {
      // Complex: 1, 3, 9, 11
      kickPlay = (beat == 1 || beat == 3 || beat == 9 || beat == 11);
    }
    
    if (kickPlay) {
      int vel = 120 + ((beat == 1) ? 6 : 0) + random(-3, 4);
      placeGenNote(c, 3, 1, vel, 100, 1);
    }
    
    // Dynamic snare pattern
    bool snarePlay = false;
    if (pageOffset % 3 == 0) {
      // Standard backbeat: 5, 13
      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 3 == 1) {
      // Ghost snares: 3, 5, 11, 13
      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 70 + random(-10, 11); // Ghost snare
      } else {
        note[c][5].velocity = 110 + random(-10, 11); // Main snare
      }
    } else {
      // Roll pattern: 4, 5, 6, 12, 13, 14
      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 60 + random(-10, 11); // Roll notes
      } else {
        note[c][5].velocity = 100 + random(-10, 11); // Main snares
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
    
    // Complex hi-hat patterns - multiple layers
    if (beat % 2 == 0) {
      int vel = 74 + ((beat % 4 == 0) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 7, 3, vel, 100, 1);
    }
    
    // Open hi-hats on snare beats
    if (beat == 5 || beat == 13) {
      int vel = 94 + random(-4, 5);
      placeGenNote(c, 8, 4, vel, 80, 1);
    }
    
    // Additional percussion layer
    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      int vel = 60 + random(-5, 6);
      placeGenNote(c, 6, 5, vel, 35, 1);
    }
    
    // Dynamic bass line - varies by page
    bool bassPlay = false;
    if (pageOffset % 2 == 0) {
      // Standard DnB: every odd beat
      bassPlay = (beat % 2 == 1);
    } else {
      // Complex: syncopated pattern
      bassPlay = (beat == 1 || beat == 3 || beat == 6 || beat == 9 || beat == 11 || beat == 14);
    }
    
    if (bassPlay) {
      int row = createRhythmicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      if (row > 9) row -= 7;
      if (row < 1) row = 1;
      int vel = 104 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 100, 1);
    }
    
    // Additional melodic elements
    if (random(0, 100) < 12) {
      int row = createMelodicProgression(chordRoot, pageOffset + (beat / 2), harmony);
      int vel = 88 + random(-4, 5);
      placeGenNote(c, row, 7, vel, 55, 2);
    }
  }
}

// House pattern generation - More dynamic and groovy
FLASHMEM void generateHousePattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMajor = true;
  const int baseNote = 8;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1; // For variation across pages
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);
    
    // Dynamic kick pattern - varies by page
    bool kickPlay = false;
    if (pageOffset % 3 == 0) {
      // Standard house: four-on-the-floor
      kickPlay = (beat == 1 || beat == 5 || beat == 9 || beat == 13);
    } else if (pageOffset % 3 == 1) {
      // Groove variation: 1, 3, 5, 7, 9, 11, 13, 15
      kickPlay = (beat % 2 == 1);
    } else {
      // Break pattern: 1, 5, 7, 9, 13, 15
      kickPlay = (beat == 1 || beat == 5 || beat == 7 || beat == 9 || beat == 13 || beat == 15);
    }
    
    if (kickPlay) {
      int vel = 114 + ((beat == 1) ? 6 : 0) + random(-3, 4);
      placeGenNote(c, 3, 1, vel, 100, 1);
    }
    
    // Dynamic snare pattern
    bool snarePlay = false;
    if (pageOffset % 4 == 0) {
      // Standard backbeat: 5, 13
      snarePlay = (beat == 5 || beat == 13);
    } else if (pageOffset % 4 == 1) {
      // Ghost snares: 3, 5, 11, 13
      snarePlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
      if (beat == 3 || beat == 11) {
        note[c][5].velocity = 60 + random(-10, 11); // Ghost snare
      } else {
        note[c][5].velocity = 85 + random(-10, 11); // Main snare
      }
    } else if (pageOffset % 4 == 2) {
      // Roll pattern: 4, 5, 6, 12, 13, 14
      snarePlay = (beat == 4 || beat == 5 || beat == 6 || beat == 12 || beat == 13 || beat == 14);
      if (beat == 4 || beat == 6 || beat == 12 || beat == 14) {
        note[c][5].velocity = 50 + random(-10, 11); // Roll notes
      } else {
        note[c][5].velocity = 80 + random(-10, 11); // Main snares
      }
    } else {
      // Complex: 2, 5, 8, 13
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
    
    // Dynamic hi-hat patterns
    if (beat % 2 == 0) {
      int vel = 70 + ((beat % 4 == 0) ? 6 : 0) + random(-4, 5);
      placeGenNote(c, 7, 3, vel, 100, 1);
    }
    
    // Open hi-hats - varies by page
    bool openHatPlay = false;
    if (pageOffset % 2 == 0) {
      // Standard: on snare beats
      openHatPlay = (beat == 5 || beat == 13);
    } else {
      // Variation: 3, 5, 11, 13
      openHatPlay = (beat == 3 || beat == 5 || beat == 11 || beat == 13);
    }
    
    if (openHatPlay) {
      int vel = 92 + random(-4, 5);
      uint8_t prob = (pageOffset % 3 == 2) ? 65 : 85;
      placeGenNote(c, 8, 4, vel, prob, 1);
    }
    
    // Additional percussion layer
    if (beat == 2 || beat == 6 || beat == 10 || beat == 14) {
      int vel = 58 + random(-5, 6);
      placeGenNote(c, 6, 5, vel, 30, 1);
    }
    
    // Dynamic bass line - varies by page
    bool bassPlay = false;
    if (pageOffset % 3 == 0) {
      // Standard house: 1, 9
      bassPlay = (beat == 1 || beat == 9);
    } else if (pageOffset % 3 == 1) {
      // Groove: 1, 3, 5, 7, 9, 11, 13, 15
      bassPlay = (beat % 2 == 1);
    } else {
      // Complex: 1, 4, 7, 9, 12, 15
      bassPlay = (beat == 1 || beat == 4 || beat == 7 || beat == 9 || beat == 12 || beat == 15);
    }
    
    if (bassPlay) {
      int row = createRhythmicProgression(chordRoot, pageOffset + (beat / 4), harmony);
      if (row > 9) row -= 7;
      if (row < 1) row = 1;
      int vel = 100 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 100, 1);
    }
    
    // Melodic elements - house style
    if (random(0, 100) < 14) {
      int row = createMelodicProgression(chordRoot, pageOffset + (beat / 2), harmony);
      int vel = 86 + random(-4, 5);
      placeGenNote(c, row, 7, vel, 60, 1);
    }
    
    // Additional stabs
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

// Ambient pattern generation - Enhanced and atmospheric
FLASHMEM void generateAmbientPattern(unsigned int start, unsigned int end, unsigned int page) {
  HarmonicAnalysis harmony;
  harmony.isMajor = (random(0, 100) < 55);
  harmony.isMinor = !harmony.isMajor;
  const int baseNote = harmony.isMajor ? 8 : 6;
  for (unsigned int c = start; c < end; c++) {
    int beat = ((c - start) % maxX) + 1;
    int pageOffset = page - 1;
    int chordRoot = createHarmonicProgression(baseNote, pageOffset, harmony);
    
    // Dynamic atmospheric patterns - varies by page
    if (pageOffset % 3 == 0) {
      // Sparse, ethereal
      if (random(0, 100) < 15) { // 15% chance
        int channel = random(5, 9); // Use melodic channels
        int row = createMelodicProgression(chordRoot, pageOffset + (beat / 4), harmony);
        if (row < 7) row += 7; // keep higher register
        int vel = 42 + random(-3, 4);
        placeGenNote(c, row, (uint8_t)channel, vel, 65, 2);
      }
    } else if (pageOffset % 3 == 1) {
      // More active, but still ambient
      if (random(0, 100) < 25) { // 25% chance
        int channel = random(5, 9); // Use melodic channels
        int row = createMelodicProgression(chordRoot, pageOffset + (beat / 2), harmony);
        if (row < 6) row += 7;
        int vel = 52 + random(-4, 5);
        placeGenNote(c, row, (uint8_t)channel, vel, 75, 1);
      }
    } else {
      // Dense but soft
      if (random(0, 100) < 35) { // 35% chance
        int channel = random(5, 9); // Use melodic channels
        int row = createMelodicProgression(chordRoot, pageOffset + beat, harmony);
        if (row < 5) row += 7;
        int vel = 48 + random(-4, 5);
        placeGenNote(c, row, (uint8_t)channel, vel, 80, 1);
      }
    }
    
    // Soft percussion - varies by page
    if (pageOffset % 2 == 0) {
      // Very sparse percussion
      if (random(0, 100) < 8) { // 8% chance
        int vel = 36 + random(-3, 4);
        placeGenNote(c, 6, 3, vel, 50, 4);
      }
    } else {
      // More active percussion
      if (random(0, 100) < 15) { // 15% chance
        int vel = 40 + random(-3, 4);
        placeGenNote(c, 6, 3, vel, 60, 2);
      }
    }
    
    // Occasional soft bass
    if (random(0, 100) < 12) { // 12% chance
      int row = createRhythmicProgression(chordRoot, pageOffset, harmony);
      if (row > 9) row -= 7;
      if (row < 1) row = 1;
      int vel = 54 + random(-4, 5);
      placeGenNote(c, row, 6, vel, 55, 4);
    }
    
    // Atmospheric pads
    if (random(0, 100) < 18) { // 18% chance
      int row = createMelodicProgression(chordRoot, pageOffset, harmony);
      if (row < 6) row += 7;
      int vel = 46 + random(-3, 4);
      placeGenNote(c, row, 7, vel, 65, 2);
    }
    
    // Occasional soft snare
    if (random(0, 100) < 5) { // 5% chance
      int vel = 44 + random(-3, 4);
      placeGenNote(c, 5, 2, vel, 35, 8);
    }
  }
}

// Fallback basic pattern for other channels
FLASHMEM void generateBasicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Tile 16-step strong beats across page (fixes maxX=32 only-first-half bug)
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
  //GLOB.edit = 1;

  unsigned int start = ((GLOB.edit - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  bool singleMode = GLOB.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();
  
  // Reset paint/unpaint prevention flag after clearPage operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
}

FLASHMEM void clearPageX(int thatpage) {
  //GLOB.edit = 1;

  unsigned int start = ((thatpage - 1) * maxX) + 1;
  unsigned int end = start + maxX;
  unsigned int channel = GLOB.currentChannel;
  bool singleMode = GLOB.singleMode;

  for (unsigned int c = start; c < end; c++) {
    clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
  }
  updateLastPage();
}


// Reset all audio effects/filters to clean defaults
FLASHMEM void resetAllAudioEffects() {
  
  extern const int ALL_CHANNELS[];
  extern const int NUM_ALL_CHANNELS;
  
  // Step 1: Reset all filter mixer gains and audio objects to clean state
  for (int i = 0; i < NUM_ALL_CHANNELS; ++i) {
    int idx = ALL_CHANNELS[i];
    setFilterDefaults(idx);  // Resets mixers, filters, reverbs, bitcrushers to default
  }
  
  // Step 2: Force all mixer gains to target (ensures smooth transitions are off)
  forceAllMixerGainsToTarget();
  
  // Step 3: Apply reset data from SMP to audio hardware
  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);
  
  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    
    // Apply filters to hardware (setFilters reads from SMP.filter_settings and applies)
    setFilters(HCUT, ch, true);
    setFilters(LOWCUT, ch, true);
    setFilters(REVERB, ch, true);
    setFilters(BITCRUSHER, ch, true);
    setFilters(DETUNE, ch, true);
    setFilters(OCTAVE, ch, true);
    
    // Apply parameters to hardware (setParams reads from SMP.param_settings and applies)
    setParams(ATTACK, ch);
    setParams(DECAY, ch);
    setParams(SUSTAIN, ch);
    setParams(RELEASE, ch);
    
    // Update synth voice (only for channel 11)
    if (ch == 11) {
      updateSynthVoice(11);
    }
  }

  // Commit post-reset filter routing immediately.
  forceAllMixerGainsToTarget();
  
}


// Complete reset for NEW start - clears everything and loads fresh defaults
FLASHMEM void startNew() {
  // Declare all external variables at the beginning
  extern unsigned int samplePackID;
  extern double playNoteInterval;
  extern IntervalTimer playTimer;
  extern bool globalMutes[maxY];
  extern bool pageMutes[maxPages][maxY];
  extern bool songModeActive;
  extern bool preventPaintUnpaint;
  extern int patternMode;
  extern uint8_t filterPage[NUM_CHANNELS];
  extern const int ALL_CHANNELS[];
  extern const int NUM_ALL_CHANNELS;
  extern Mode draw;
  extern Mode volume_bpm;
  extern Mode *currentMode;
  extern i2cEncoderLibV2 Encoder[NUM_ENCODERS];
  extern CRGB col[];
  extern bool isNowPlaying;
  extern CachedSample previewCache;
  
  // FULL reset: clear dynamic sample browser state
  sampleBrowserClearAll();
  // Clear any preview/cache state as well
  previewCache.valid = false;
  previewCache.lengthBytes = 0;
  previewCache.plen = 0;
  
  // 0. STOP PLAYBACK FIRST (if playing)
  if (isNowPlaying) {
    isNowPlaying = false;
  }
  
  // Visual feedback - clear display and show message
  FastLEDclear();
  drawText("NEW", 6, 8, CRGB(0, 255, 255));
  FastLEDshow();
  
  // 1. CLEAR ALL NOTES (all pages, all channels, all velocities, all probabilities)
  extern void clearPatternNotes();
  clearPatternNotes();
  
  // 2. RESET GLOBAL VARIABLES FIRST (before using GLOB.currentChannel)
  SMP.bpm = 100.0;
  GLOB.vol = 100;  // 0-100 range, default to max
  GLOB.velocity = 10;
  GLOB.currentChannel = 1;  // Set this FIRST before using it
  GLOB.page = 1;
  GLOB.edit = 1;
  GLOB.singleMode = false;
  GLOB.x = 1;
  GLOB.y = 1;
  
  // 2b. RESET ALL EEPROM SETTINGS TO DEFAULTS
  // Write defaults to EEPROM - globals will be reloaded on next boot
  // SETT>LEDS (panel count / rotation) stays — full reset should not undress the hardware.
  uint8_t preserveLedMode = EEPROM.read(EEPROM_DATA_START + 13);
  if (preserveLedMode < 1 || preserveLedMode > 4) preserveLedMode = 1;
  EEPROM.write(EEPROM_DATA_START + 0,  1);    // recMode (MIC)
  EEPROM.write(EEPROM_DATA_START + 1,  1);    // clockMode (INT)
  EEPROM.write(EEPROM_DATA_START + 2,  2);    // transportMode (SEND)
  EEPROM.write(EEPROM_DATA_START + 3,  1);    // patternMode (ON)
  EEPROM.write(EEPROM_DATA_START + 4, (uint8_t)-1);  // voiceSelect (OFF)
  EEPROM.write(EEPROM_DATA_START + 5,  1);    // fastRecMode
  EEPROM.write(EEPROM_DATA_START + 6,  1);    // recChannelClear
  EEPROM.write(EEPROM_DATA_START + 7,  20);   // previewVol
  EEPROM.write(EEPROM_DATA_START + 8, (uint8_t)-1);  // flowMode (OFF)
  EEPROM.write(EEPROM_DATA_START + 9,  10);   // micGain
  EEPROM.write(EEPROM_DATA_START + 10, (6 << 2)); // PPQN pulse: OFF, +, 24, CONT
  EEPROM.write(EEPROM_DATA_START + 30, 12);   // PPQN pulse width ms
  EEPROM.write(EEPROM_DATA_START + 11, 1);    // simpleNotesView (EASY)
  EEPROM.write(EEPROM_DATA_START + 12, 0);    // loopLength (OFF)
  EEPROM.write(EEPROM_DATA_START + 13, preserveLedMode);  // ledMode: keep SETT>LEDS
  EEPROM.write(EEPROM_DATA_START + 14, 0);    // ctrlMode (PAGE)
  EEPROM.write(EEPROM_DATA_START + 15, 30);   // lineOutLevelSetting
  EEPROM.write(EEPROM_DATA_START + 16, 8);    // lineInLevel
  EEPROM.write(EEPROM_DATA_START + 17, 100);  // GLOB.vol
  EEPROM.write(EEPROM_DATA_START + 18, 0);    // cursorType
  EEPROM.write(EEPROM_DATA_START + 19, 0);    // showChannelNr
  EEPROM.write(EEPROM_DATA_START + 20, 0);    // previewTriggerMode
  EEPROM.write(EEPROM_DATA_START + 21, 0);    // drawMode
  EEPROM.write(EEPROM_DATA_START + 22, 0);    // colorScheme
  EEPROM.write(EEPROM_DATA_START + 23, 0);    // stereoChannel
  EEPROM.write(EEPROM_DATA_START + 24, 2);    // midiSendMode (BOTH)
  EEPROM.write(EEPROM_DATA_START + 25, 0);    // ledStripEnabled (OFF)
  EEPROM.write(EEPROM_DATA_START + 26, 64);   // ledBrightness
  EEPROM.write(EEPROM_DATA_START + 27, 1);    // spkrEnabled (ON)
  EEPROM.write(EEPROM_DATA_START + 28, 1);    // midiNoteReceive
  EEPROM.write(EEPROM_DATA_START + 29, 0);    // transportSendDelayMs
  EEPROM.write(EEPROM_DATA_START + 31, 0);    // transportRcveDelayMs
  EEPROM.put(EEPROM_DATA_START + 32, (uint16_t)256);  // codecHfCut
  EEPROM.write(EEPROM_DATA_START + 33, 4);    // HFC format
  EEPROM.put(EEPROM_DATA_START + 34, (uint16_t)0x0006);  // drawRFullMuteCustomUnmuteMask
  EEPROM.write(EEPROM_DATA_START + 36, 0);    // childLockEnabled (OFF)
  EEPROM.write(EEPROM_DATA_START + 37, 1);    // MIDI pitch clamp (ON)
  
  // Reload settings from EEPROM and apply to hardware
  extern void loadMenuFromEEPROM();
  extern void applyAudioSettingsFromGlobals();
  loadMenuFromEEPROM();
  applyAudioSettingsFromGlobals();
  
  // 3. RESET ALL FILTER/PARAMETER DATA (just data, no audio hardware yet)
  const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14};
  const int numChannels = sizeof(channels) / sizeof(channels[0]);
  
  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    
    // Reset filter data (no hardware calls)
    SMP.filter_settings[ch][HCUT] = 32;
    SMP.filter_settings[ch][LOWCUT] = 0;
    SMP.filter_settings[ch][REVERB] = 0;
    SMP.filter_settings[ch][BITCRUSHER] = 0;
    SMP.filter_settings[ch][DETUNE] = 16;
    SMP.filter_settings[ch][OCTAVE] = 16;
    SMP.filter_settings[ch][EFX] = 0;  // Sample mode
    
    // Reset parameter data (no hardware calls)
    // Channel-specific ADSR defaults:
    // - ch13/14 (synths): A=32, D=9, S=20, R=9
    // - all others: keep existing defaults
    if (ch == 13 || ch == 14) {
      SMP.param_settings[ch][ATTACK] = 32;
      SMP.param_settings[ch][DECAY] = 9;
      SMP.param_settings[ch][SUSTAIN] = 20;
      SMP.param_settings[ch][RELEASE] = 9;
    } else {
      SMP.param_settings[ch][ATTACK] = 32;
      SMP.param_settings[ch][DECAY] = 0;
      SMP.param_settings[ch][SUSTAIN] = 10;
      SMP.param_settings[ch][RELEASE] = 5;
    }
    
    // Reset synth data (only for channels 11, 13-14)
    if (ch == 11 || ch == 13 || ch == 14) {
      SMP.synth_settings[ch][CUTOFF] = 0;
      SMP.synth_settings[ch][RESONANCE] = 0;
      SMP.synth_settings[ch][FILTER] = 0;
      SMP.synth_settings[ch][CENT] = 16;
      SMP.synth_settings[ch][SEMI] = 0;
      SMP.synth_settings[ch][INSTRUMENT] = 0;
      SMP.synth_settings[ch][FORM] = 0;
      SMP.synth_settings[ch][LFO_RATE] = 0;
      SMP.synth_settings[ch][LFO_DEPTH] = 0;
      SMP.synth_settings[ch][LFO_PHASE] = 0;
      SMP.synth_settings[ch][ARP_STEP] = 0;
    }
  }
  
  // 4. CLEAR ALL MUTE STATES
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
  
  // 5. CLEAR ALL SONG ARRANGEMENT
  for (int i = 0; i < 64; i++) {
    SMP.songArrangement[i] = 0;
  }
  
  // 6. CLEAR ALL SAMPLEPACK 0 (sp0) STATES - Remove all custom samples
  for (int i = 1; i < maxFiles; i++) {
    SMP.sp0Active[i] = false;
  }
  saveSp0StateToEEPROM();
  
  // 7. LOAD SAMPLEPACK 1 (fresh default samples)
  SMP.pack = 1;
  samplePackID = 1;
  EEPROM.put(0, (unsigned int)1);  // Save to EEPROM
  markSettingsBackupDirty();
  // Reset should overwrite everything (no SP0 preservation)
  loadSamplePack(1, false, false);
  
  // 8. UPDATE BPM AND TIMER
  Mode *bpm_vol = &volume_bpm;
  bpm_vol->pos[3] = SMP.bpm;
  playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
  playTimer.update((uint32_t)round(playNoteInterval));
  bpm_vol->pos[2] = GLOB.vol;
  
  // 9. RESET WAVE FILE IDS to defaults
  for (int i = 1; i < maxFiles; i++) {
    SMP.wav[i].oldID = 0;   // folder 0
    SMP.wav[i].fileID = i;  // file index = voice index (1..8)
  }
  
  // 10. UPDATE LAST PAGE (should be 1 since no notes)
  updateLastPage();
  
  // 11. RESET PATTERN MODE FLAGS (before switching mode)
  songModeActive = false;
  SMP_PATTERN_MODE = true;   // patternMode=1 means ON
  patternMode = 1;  // ON (matches EEPROM default)
  
  // Reset paint/unpaint prevention flag
  preventPaintUnpaint = false;
  
  // 12. SWITCH TO DRAW MODE
  delay(300);  // Brief delay to show "NEW" message
  switchMode(&draw);
  
  // 13. SYNC ENCODERS WITH CURRENT POSITION
  // Force encoder positions to match reset state
  Encoder[0].writeCounter((int32_t)GLOB.y);       // Y position (channel)
  Encoder[1].writeCounter((int32_t)GLOB.edit);    // Page
  Encoder[2].writeCounter((int32_t)maxfilterResolution);  // Filter/velocity
  Encoder[3].writeCounter((int32_t)GLOB.x);       // X position
  
  // Update currentMode positions to match
  currentMode->pos[0] = GLOB.y;
  currentMode->pos[1] = GLOB.edit;
  currentMode->pos[2] = maxfilterResolution;
  currentMode->pos[3] = GLOB.x;
  
  // Set encoder colors to match channel 1
  Encoder[0].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
  Encoder[3].writeRGBCode(CRGBToUint32(col[GLOB.currentChannel]));
  
  // 14. RESET ALL AUDIO EFFECTS/FILTERS TO CLEAN DEFAULTS
  resetAllAudioEffects();

  extern void resetAllChannelVolumesToDefault();
  resetAllChannelVolumesToDefault();

  // 14b. Ensure synth channels (13/14) become immediately audible after FULL reset.
  // This mirrors FILTERMODE "0002" (long-press) behavior, so you don't have to manually reset once.
  setEnvelopeDefaultValues(13);
  setFiltersDefaultValues(13);
  setSynthDefaultValues(13);
  setEnvelopeDefaultValues(14);
  setFiltersDefaultValues(14);
  setSynthDefaultValues(14);
  
  // 15. FORCE DISPLAY REFRESH
  // Clear any stale display data and force a complete redraw
  FastLEDclear();
  FastLEDshow();
  
  // 16. AUTOSAVE EMPTY STATE
  // Drop any corrupt/stale autosave first, then write a known-empty file.
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

int getFileNumber(int value) {
  // return the file number in the folder, so 113 = 13, 423 = 23
  int folder = getFolderNumber(value);
  int wavfile = value % 100;
  if (wavfile <= 0) wavfile = 0;
  return wavfile + folder * 100;
}

CRGB getCol(unsigned int g) {
  return col[g] * 10;
}


void setNote(uint16_t step,  uint8_t pitch, uint8_t channel, uint8_t velocity) {
  if (step < maxlen && channel < NUM_CHANNELS) {
    note[step][pitch].channel = channel;
    note[step][pitch].velocity = velocity;
    note[step][pitch].probability = 100;
    note[step][pitch].condition = 1;
    note[step][pitch].midiPitch = NOTE_MIDI_PITCH_NONE;
  }
}

// Function to get a note from a given step and channel
Note getNote(uint16_t step, uint8_t channel) {
  if (step < maxlen && channel < NUM_CHANNELS) {
    return note[step][channel];
  }
  // Return a default Note if indices are out-of-range.
  return {0, 0, 100, 1, NOTE_MIDI_PITCH_NONE};
}


/***************/
/**** INTRO ****/
/***************/





// ----- Determine Color of Each LED Based on Time -----
CRGB getPixelColor(uint8_t x, uint8_t y, unsigned long elapsed) {
  if (elapsed < phase1Duration) {
    // PHASE 1: Rainbow Logo (only phase - shown for 2 seconds)
    if (logo16_on_P(logo_rows, x, y)) {
      float timeFactor = (float)elapsed / phase1Duration;  // 0..1
      return getLogoPixelColor(x, y, timeFactor);
    } else {
      return CRGB::Black;
    }
  } else {
    // After animation, stay black
    return CRGB::Black;
  }
}








// Shared logo+sine frame. holdNoFadeOut keeps full brightness after fade-in
// (INFO loop); boot mode still fades everything out at the end.
static void drawLogoAnimationFrame(unsigned long elapsedMs, bool holdNoFadeOut) {
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

  // Continuous motion time so INFO can keep scrolling after fade-in.
  const float timeSec = (float)elapsedMs * 0.001f;
  const float hueProgress = fmodf(timeSec * 0.22f, 1.0f);
  const float amplitude =
      3.1f + 0.45f * sinf(timeSec * 1.4f);
  const float centerY = ((float)maxY + 1.0f) * 0.5f;
  const float whiteYOffset = -0.85f;  // matrix Y grows downward → negative = up
  const float blueYOffset = 0.85f;
  const float blueXPhaseOffset = 0.55f;
  const float widthDenominator = (maxX > 1) ? (float)(maxX - 1) : 1.0f;
  const int logoStartX =
      ((int)maxX - (int)MATRIX_WIDTH) / 2 + 1;

  // White travels faster so the two waves keep changing their overlap.
  const float whiteTravel = timeSec * 5.4f;
  const float blueTravel = timeSec * 2.6f;

  auto waveTouchesLogo = [&](unsigned int x, int wavePixelY) -> bool {
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
  };

  auto drawWavePixel = [&](unsigned int x, float waveY, CRGB baseColor) {
    const int wavePixelY = (int)roundf(waveY);
    if (wavePixelY < 1 || wavePixelY > (int)maxY) return;

    float contourScale =
        waveTouchesLogo(x, wavePixelY) ? (1.0f - 0.90f * logoFade) : 1.0f;
    float level = sineFade * fadeOut * contourScale;
    if (level <= 0.0f) return;

    CRGB color = baseColor;
    color.nscale8((uint8_t)constrain((int)roundf(255.0f * level), 0, 255));
    if (color.r | color.g | color.b) {
      light(x, (unsigned int)wavePixelY, color);
    }
  };

  for (unsigned int x = 1; x <= maxX; x++) {
    const float xNorm = (float)(x - 1) / widthDenominator;
    const float xPhase = xNorm * 4.0f * (float)M_PI;

    // Darker dual sines; blue first so white stays readable on overlap.
    drawWavePixel(
        x,
        centerY + blueYOffset +
            amplitude * sinf(xPhase + blueXPhaseOffset - blueTravel),
        CRGB(3, 8, 28));
    drawWavePixel(
        x,
        centerY + whiteYOffset +
            amplitude * sinf(xPhase - whiteTravel),
        CRGB(26, 26, 26));
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

// ----- Run the Animation Once (Called in setup) -----
void runAnimation() {
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

// ETC → INFO (encoder 4 / 0001): fade in, then keep animating until the same
// button is pressed again. No fade-out while held open.
void runInfoLogoAnimationLoop() {
  extern bool pressed[NUM_ENCODERS];
  extern bool isPressed[NUM_ENCODERS];
  extern uint8_t buttons[NUM_ENCODERS];
  extern ButtonState buttonState[NUM_ENCODERS];
  extern int currentEncoderIndex;
  extern void resetEtcInfoPageAnimation();

  auto pollEncoder4 = []() {
    currentEncoderIndex = 3;
    Encoder[3].updateStatus();
  };

  // INFO starts from the short-release event (0001), so consume that event
  // without waiting on another I2C release callback inside this blocking loop.
  buttons[3] = 0;
  pressed[3] = false;
  isPressed[3] = false;
  buttonState[3] = IDLE;

  unsigned long startTime = millis();
  unsigned long lastFrameTime = 0;
  bool sawRelease = true;

  for (;;) {
    pollEncoder4();
    const bool down = pressed[3] || isPressed[3];
    if (!down) {
      sawRelease = true;
    } else if (sawRelease) {
      // Fresh press: exit immediately. Waiting here for the release callback
      // can deadlock the main thread while audio/serial interrupts keep running.
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

  // Clear stale button edge state so checkMode doesn't re-fire or leave menu blank.
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

// Enhanced base page pattern analysis structure (moved to top of file)

// Retained only as reference while the companion engine replaces the old,
// separate AUTO implementation.
#if 0
FLASHMEM void generateSongLegacy() {
  extern int aiTargetPage; // Access the target page from menu
  extern int aiBaseStartPage; // Access the base start page from menu
  extern int aiBaseEndPage;   // Access the base end page from menu
  
  int currentPage = GLOB.page;
  int targetPage = aiTargetPage;
  
  // Validate target page - ensure it's higher than current page
  if (targetPage <= currentPage) {
    targetPage = currentPage + 1;
  }
  
  // Don't exceed maxPages
  if (targetPage > maxPages) {
    targetPage = maxPages;
  }
  
  // Validate base page range
  if (aiBaseStartPage > aiBaseEndPage) {
    return;
  }
  
  // Check if base pages have content and analyze patterns
  bool basePagesEmpty = true;
  bool channelsUsed[16] = {false};
  // STACK OVERFLOW FIX: Use global EXTMEM buffer to prevent stack overflow
  BasePagePattern* channelPatterns = g_channelPatterns; // Use global EXTMEM buffer
  
  // Initialize pattern analysis
  for (int ch = 0; ch < 16; ch++) {
    channelPatterns[ch].stepCount = 0;
    channelPatterns[ch].density = 0.0;
    channelPatterns[ch].mostCommonRow = 1;
    channelPatterns[ch].isRhythmic = false;
    channelPatterns[ch].isMelodic = false;
    for (int step = 0; step < 16; step++) {
      channelPatterns[ch].hasNotes[step] = false;
      channelPatterns[ch].noteRows[step] = 0;
      channelPatterns[ch].velocities[step] = defaultVelocity;
      channelPatterns[ch].rhythmPattern[step] = 0;
    }
  }
  
  // Analyze all base pages content with detailed pattern extraction
  for (int basePage = aiBaseStartPage; basePage <= aiBaseEndPage; basePage++) {
    unsigned int pageStartStep = (basePage - 1) * maxX + 1;
    unsigned int pageEndStep = basePage * maxX;
    
    for (unsigned int c = pageStartStep; c <= pageEndStep; c++) {
      for (unsigned int r = 1; r <= maxY; r++) {
        if (note[c][r].channel != 0) {
          basePagesEmpty = false;
          int ch = note[c][r].channel;
          channelsUsed[ch] = true;
          
          // Calculate relative step position within the 16-step pattern
          int relativeStep = ((c - pageStartStep) % 16);
          
          // Record pattern information
          channelPatterns[ch].hasNotes[relativeStep] = true;
          channelPatterns[ch].noteRows[relativeStep] = r;
          channelPatterns[ch].velocities[relativeStep] = note[c][r].velocity;
          channelPatterns[ch].rhythmPattern[relativeStep] = 1;
          channelPatterns[ch].stepCount++;
        }
      }
    }
  }
  
  // Analyze patterns for each used channel
  for (int ch = 1; ch <= 15; ch++) {
    if (channelsUsed[ch]) {
      BasePagePattern* pattern = &channelPatterns[ch];
      
      // Calculate density
      pattern->density = (float)pattern->stepCount / 16.0;
      
      // Find most common note row
      int rowCounts[17] = {0}; // 1-16 for note rows
      for (int step = 0; step < 16; step++) {
        if (pattern->hasNotes[step]) {
          rowCounts[pattern->noteRows[step]]++;
        }
      }
      int maxCount = 0;
      for (int row = 1; row <= 16; row++) {
        if (rowCounts[row] > maxCount) {
          maxCount = rowCounts[row];
          pattern->mostCommonRow = row;
        }
      }
      
      // Determine if rhythmic or melodic based on pattern characteristics
      if (ch >= 1 && ch <= 4) {
        pattern->isRhythmic = true;
        pattern->isMelodic = false;
      } else if (ch >= 5 && ch <= 8) {
        pattern->isRhythmic = false;
        pattern->isMelodic = true;
      } else if (ch == 11) {
        // Channel 11 can be both - determine based on density and note variation
        bool hasNoteVariation = false;
        for (int i = 1; i < 16; i++) {
          if (pattern->hasNotes[i] && pattern->noteRows[i] != pattern->mostCommonRow) {
            hasNoteVariation = true;
            break;
          }
        }
        pattern->isMelodic = hasNoteVariation || pattern->density > 0.3;
        pattern->isRhythmic = !pattern->isMelodic;
      } else {
        // Other channels - analyze based on density and variation
        pattern->isMelodic = pattern->density > 0.2;
        pattern->isRhythmic = !pattern->isMelodic;
      }
      
    }
  }
  
  // Debug: Show what musical elements are present in base pages
  int rhythmChannels = 0, melodyChannels = 0, otherChannels = 0;
  for (int ch = 1; ch <= 15; ch++) {
    if (channelsUsed[ch]) {
      if (channelPatterns[ch].isRhythmic) rhythmChannels++;
      else if (channelPatterns[ch].isMelodic) melodyChannels++;
      else otherChannels++;
    }
  }
  
  // Generate pages starting from after the base page range
  int startPage = aiBaseEndPage + 1;
  int endPage = startPage + aiTargetPage - 1;
  
  // Safety check: ensure we don't exceed maxPages
  if (endPage > maxPages) {
    endPage = maxPages;
  }
  
  // Debug: Show what pages will be generated
  
  for (int page = startPage; page <= endPage; page++) {
    
    // Save current page context
    int originalPage = GLOB.page;
    
    // Set the page context for generation
    GLOB.page = page;
    
    // Always clear the page since we're generating new content
    // Calculate the step range for this specific page
    unsigned int pageStartStep = (page - 1) * maxX + 1;
    unsigned int pageEndStep = page * maxX;
    
    for (unsigned int c = pageStartStep; c <= pageEndStep; c++) {
      for (unsigned int r = 1; r <= maxY; r++) {
        note[c][r].channel = 0;
        note[c][r].velocity = defaultVelocity;
        note[c][r].probability = 100;
        note[c][r].condition = 1;
        note[c][r].midiPitch = NOTE_MIDI_PITCH_NONE;
      }
    }
    
    // Generate pattern for this page
    HarmonicAnalysis harmony;
    
    // Analyze existing harmony from base page for intelligent progression
    harmony.hasRoot = false;
    harmony.hasThird = false;
    harmony.hasFifth = false;
    harmony.hasSeventh = false;
    harmony.rootNote = 1;
    harmony.isMajor = false;
    harmony.isMinor = false;
    
    // Analyze base pages harmony if they exist
    if (!basePagesEmpty) {
      // Find the most common root note across all base pages
      int noteCounts[16] = {0};
      for (int basePage = aiBaseStartPage; basePage <= aiBaseEndPage; basePage++) {
        unsigned int pageStartStep = (basePage - 1) * maxX + 1;
        unsigned int pageEndStep = basePage * maxX;
        
        for (unsigned int c = pageStartStep; c <= pageEndStep; c++) {
          for (unsigned int r = 1; r <= maxY; r++) {
            if (note[c][r].channel != 0) {
              noteCounts[r]++;
            }
          }
        }
      }
      
      // Find the most prominent note as root
      int maxCount = 0;
      for (int i = 1; i <= 16; i++) {
        if (noteCounts[i] > maxCount) {
          maxCount = noteCounts[i];
          harmony.rootNote = i;
        }
      }
    }
    
    // Create harmonic progression based on page number relative to base pages
    if (page > aiBaseEndPage) {
      int pageOffset = page - aiBaseEndPage;
      
      // Common chord progressions: I, ii, iii, IV, V, vi, vii°
      int progression[] = {1, 2, 3, 4, 5, 6, 7, 1}; // I-ii-iii-IV-V-vi-vii°-I
      int progressionIndex = (pageOffset - 1) % 8;
      int chordDegree = progression[progressionIndex];
      
      // Convert chord degree to root note (simplified)
      harmony.rootNote = ((harmony.rootNote + chordDegree - 1) % 8) + 1;
      
      // Determine major/minor based on chord degree
      if (chordDegree == 1 || chordDegree == 4 || chordDegree == 5) {
        harmony.isMajor = true;
        harmony.isMinor = false;
      } else if (chordDegree == 2 || chordDegree == 3 || chordDegree == 6) {
        harmony.isMajor = false;
        harmony.isMinor = true;
      } else {
        // Diminished or other - use alternating pattern
        harmony.isMajor = (page % 2 == 0);
        harmony.isMinor = !harmony.isMajor;
      }
      
    }
    
    // Generate patterns for used channels - only for this specific page
    // Use analyzed base page patterns with intelligent variations
    for (int ch = 1; ch <= 15; ch++) {
      if (channelsUsed[ch]) {
        BasePagePattern* basePattern = &channelPatterns[ch];
        
        // Add dynamic variation based on page position relative to base pages
        int pageOffset = page - aiBaseEndPage;
        float intensityMultiplier = 0.8 + (pageOffset * 0.1); // Gradually increase intensity
        if (intensityMultiplier > 1.2) intensityMultiplier = 1.2;
        
        // Generate based on analyzed pattern type
        if (basePattern->isRhythmic) {
          generateContextAwareRhythmicPattern(pageStartStep, pageEndStep, ch, harmony, basePattern, pageOffset);
        } else if (basePattern->isMelodic) {
          generateContextAwareMelodicPattern(pageStartStep, pageEndStep, ch, harmony, basePattern, pageOffset);
        } else {
          // Fallback for other channels
          generateBasicPattern(pageStartStep, pageEndStep, ch, harmony);
        }
      }
    }
    
    // Restore original page context
    GLOB.page = originalPage;
  }
  
  // Auto-close menu and return to main interface
  extern void switchMode(Mode *newMode);
  extern Mode draw;
  switchMode(&draw);
}
#endif

// ETC > AUTO: extend the selected base range with the same role-aware engine
// used by single-mode 2000. Empty bases create a coordinated starter ensemble.
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

  bool channelsUsed[16] = {};
  bool baseEmpty = true;
  for (int page = baseStart; page <= baseEnd; page++) {
    unsigned int start = (unsigned int)(page - 1) * maxX + 1;
    unsigned int end = min((unsigned int)MAX_STEPS + 1u, start + maxX);
    for (unsigned int c = start; c < end; c++) {
      for (int row = 1; row <= 16; row++) {
        uint8_t ch = note[c][row].channel;
        if (ch < 16 && companionChannelSupported(ch)) {
          channelsUsed[ch] = true;
          baseEmpty = false;
        }
      }
    }
  }

  // Role order matters: harmony/rhythm companions can react to earlier voices.
  static const uint8_t generationOrder[] = {1, 2, 3, 4, 5, 6, 11, 13, 14, 7, 8};
  if (baseEmpty) {
    for (uint8_t ch : generationOrder) channelsUsed[ch] = true;
  }

  preventPaintUnpaint = true;
  for (int page = outputStart; page <= outputEnd; page++) {
    clearCompanionPage(page);
    // One page-level harmony seed so bass/keys/pads share the same progression.
    int pageSeed = page * 31 + baseEnd * 7 + random(0, 256);
    CompanionContext pageContext =
        analyzeCompanionContext(baseStart, page, page, 0);
    CompanionHarmony pageHarm = companionMakeHarmony(pageContext, pageSeed);
    CompanionGroove pageGroove =
        companionMakeGroove(pageContext, page, pageSeed);

    for (uint8_t ch : generationOrder) {
      if (!channelsUsed[ch]) continue;
      // Include generated voices on this page as companions for later roles.
      CompanionContext context =
          analyzeCompanionContext(baseStart, page, page, ch);
      generateCompanionChannelPage(page, ch, context, pageHarm,
                                   pageGroove, pageSeed);
    }
  }
  preventPaintUnpaint = false;
  updateLastPage();

  extern void switchMode(Mode *newMode);
  extern Mode draw;
  switchMode(&draw);
}
