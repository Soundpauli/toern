

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
extern Mode bpmMode;
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
    setParams(DELAY, ch);
    setParams(ATTACK, ch);
    setParams(HOLD, ch);
    setParams(DECAY, ch);
    setParams(SUSTAIN, ch);
    setParams(RELEASE, ch);

    // Load Synths - apply synth settings
    if (ch == 11) {
      extern void applySynthInstrumentPreset(int channel, int instrumentIdx);
      extern void applySynthInstrumentFormDefault(int channel, int instrumentIdx);
      // Migrate stale generic startup state (INST=0 with clobbered D=32/S=32/R=16 or CUT=16/RES=0/FLT=0)
      // to the new DXBS (12) default preset.
      if (SMP.synth_settings[11][INSTRUMENT] > 12 ||
          (SMP.synth_settings[11][INSTRUMENT] == 0 &&
           SMP.param_settings[11][DECAY] == 32 &&
           SMP.param_settings[11][SUSTAIN] == 32)) {
        applySynthInstrumentPreset(11, 12);
      } else {
        // FORM is an internal preset identity rather than an exposed user control.
        // Refresh it for older projects, but preserve their saved filter, pitch,
        // and ADSR controls.
        applySynthInstrumentFormDefault(
            11, (int)SMP.synth_settings[11][INSTRUMENT]);
      }
      updateSynthVoice(11);
    } else if (ch == 13 || ch == 14) {
      extern void applyMonoSynthCrf(int ch);
      applyMonoSynthCrf(ch);
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

// REC input monitor into mixer_end in3. Mute while VOL>SPKR is ON (electrical feedback).
// Speaker amp is also forced off while capturing (see applySpkrHardware).
float recInputMonitorGain() {
  if (getSpkrEnabled()) return 0.0f;  // spkrEnabled == speaker ON
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
  // Drain any stale blocks from queue1 before starting fresh capture
  queue1.end();
  while (queue1.available()) {
    queue1.readBuffer();
    queue1.freeBuffer();
  }
  recTime = 0;
  recWriteIndex = 0;
  // Drop initial ~25ms (5 blocks) while the one-shot WS2812Serial DMA transfer (~15.4ms)
  // finishes and the +5V/+3.3VA rails settle, matching startFastRecord().
  recRamDropRemaining = FAST_DROP_BLOCKS;
  
  // Reset peak recording index for fresh waveform display
  extern int peakRecIndex;
  peakRecIndex = 0;
  
  queue1.begin();  // start filling 128‑sample blocks
  isRecording = true;
  applySpkrHardware();

  mixer_end.gain(3, recInputMonitorGain());
}

void flushAudioQueueToRAM2() {
  if (!isRecording) return;
  // each AudioRecordQueue block is 128 samples
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
  // if we ever hit the end of the buffer, stop automatically and save:
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

  // Disable audio input monitoring
  mixer_end.gain(3, 0.0);

  if (recWriteIndex < 128) return;

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
  // write 44‑byte WAV header for 16‑bit/mono/44 100 Hz
  writeWavHeader(f, AUDIO_SAMPLE_RATE_EXACT, 16, 1);
  // write all your samples in one chunk
  f.write((uint8_t *)recBuffer, recWriteIndex * sizeof(int16_t));
  finalizeWavHeader(f, recWriteIndex * sizeof(int16_t));

  f.close();

  sampleBrowserInvalidate();
  snprintf(SMP.samplePathRel[ch], 128, "recs/%s", tmpName);
  extern void invalidateShowWaveBrowserSync();
  invalidateShowWaveBrowserSync();
  
  // Reload the sample metadata after recording and copy directly into the voice's RAM slot
  extern bool sampleIsLoaded;
  extern bool firstcheck;
  extern CachedSample previewCache;
  extern void beginChannelSampleReload(unsigned int ch);
  extern void endChannelSampleReload(unsigned int ch);
  extern float rateFactor;
  
  sampleIsLoaded = false;  // Force reload of new recording
  firstcheck = true;       // Reset check flag
  previewCache.valid = false;  // Invalidate cache so new recording is loaded

  // Also load the newly recorded buffer directly into sampled[ch] so the voice plays it immediately
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

// --------------------
void startFastRecord() {
  if (fastRecordActive) return;

  // Handle different recording modes based on recChannelClear
  if (!touchyEnabled && (recChannelClear == 1 || recChannelClear == 3)) {
    // ON / ON1: Clear all existing notes of channel, then add triggers/notes
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
  if (!touchyEnabled && recChannelClear != 2) {
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
  fastRecordChannel = (uint8_t)constrain(ch, 0, 255);
  
  // Immediately stop any playing sound on the recording channel
  extern AudioPlayArrayResmp *voices[];
  if (ch >= 0 && ch <= 8 && voices[ch] != nullptr) {
    voices[ch]->stop();  // stop reader before we overwrite sampled[ch]
  }
  extern AudioEffectEnvelope *envelopes[];
  if (ch >= 0 && ch < 15 && envelopes[ch] != nullptr) {
    envelopes[ch]->noteOff();
  }
  
  // Also stop all notes on the sampler for channels 0-8 (sample channels)
  if (ch >= 0 && ch <= 8) {
    for (int note = 36; note <= 96; note++) {
      samplerNoteEvent(ch, note, 0, false, false);
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
  applySpkrHardware();

  // Red border is armed + shown from main loop before this runs.
  // Do not FastLED.show() here — ON1 starts from the playNote ISR.
  
  mixer_end.gain(3, recInputMonitorGain());

  //Serial.printf("FAST RECORD ▶ ch%u (dropping %d blocks)\n", ch, FAST_DROP_BLOCKS);
}

// --------------------
void flushAudioQueueToRAM() {
  if (fastRecordActive) {
    int ch = fastRecordChannel;
    auto &idx = fastRecWriteIndex[ch];

    // Treat your byte buffer as an int16_t array for cleaner pointer math:
    int16_t *dest = reinterpret_cast<int16_t *>(sampled[ch]);

    // Pull every pending block
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
    return;  // skip your SD path
  }
}

void stopFastRecord() {
  if (!fastRecordActive) return;
  // CRITICAL: Disable audio input first to stop new data from entering queue
  mixer_end.gain(3, 0.0);
  
  int ch = fastRecordChannel;
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
  applySpkrHardware();
  fastRecordChannel = 0;
  recordingBorderArmed = false;
  pendingStartFastRecord = false;
  fastRecWaitRelease = true;
  
  // Flush one more time before ending the queue
  flushAudioQueueToRAM();
  
  queue1.end();
  
  // Final aggressive flush of any remaining data in the queue
  // Continue until queue is completely empty or buffer is full
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
  loadedSampleRate[ch] = 3;

  // Also publish under samples/recs/ so SET_WAV (SMP) can browse/trim/edit the take,
  // matching stopRecordingRAM(). SP0 copy below keeps the live pack slot in sync.
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
  
  // Auto-save recorded sample to samplepack 0 (deferred while sequencer is playing)
  copySampleToSamplepack0(ch);
  saveSp0StateToEEPROM();
  
  // Now load into sampler (do this after saves to minimize interruption)
  samplerRemoveSamples(ch);
  samplerAddSample(ch,
    36,                            // MIDI note #
    (int16_t*)sampled[ch],         // reinterpret bytes → int16_t
    idx,                           // sample-count
    rateFactor
  );
  channelDirection[ch] = 1;
  
  // For ON1 mode: Set note at x=1 after recording and copy are complete
  if (!touchyEnabled && recChannelClear == 3 && savedStartBeat > 0) {
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
 // samplerNoteEvent(ch, 36, defaultVelocity, true, false);
  //Serial.printf("◀ FASTREC ch%u, %u samples\n", ch, (unsigned)idx);

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
  // Don't FastLED.show() here — startFastRecord can run while the border is
  // already in the framebuffer, and a throttled show would wipe/skip it.
  if (!recordingBorderArmed && !fastRecordActive && !pendingStartFastRecord) {
    FastLEDshow();
  }
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
  // Asleep: closed lids dimmed further for less glare
  const uint8_t outlineBrightness = asleep ? 7 : 15;
  const uint8_t pupilBrightness = asleep ? 7 : 80;
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
  
  drawScreensaverEye(leftEyeX, eyeY, leftClosed, leftColor, leftPupil, currentPupilX, currentPupilY);
  drawScreensaverEye(rightEyeX, eyeY, rightClosed, rightColor, rightPupil, currentPupilX, currentPupilY);
}

FLASHMEM void FastLEDshow() {
  // Never push WS2812B frames while actively capturing audio in RAM (same as fastRecordActive).
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
    // While playing, keep the grid visible in draw / single / filter / IMG / fullMute.
    // Screensaver may still run during playback in other contexts (e.g. menu).
    const bool playingGridFocus = isNowPlaying && (
      currentMode == &draw ||
      currentMode == &singleMode ||
      currentMode == &filterMode ||
      imageMode ||
      fullMuteIsActive());
    // SD? wait loops already call noteUserActivity(), so idle timeout covers that case.
    // Do not gate on drawNoSD_hasRun — switchMode() clears it, which permanently
    // blocked the screensaver after any mode change.
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
      // Screensaver just ended - reset flag so encoders can be controlled again
      screensaverEncodersOff = false;
    }
    // Don't change global brightness - matrix is dimmed in software (light_single)
    // Strip stays at full brightness (set in setup() and not modified)
    if (currentMode != &touchyMode && fnDirection != 0 && (int32_t)(fnDirectionUntil - millis()) > 0) {
      CRGB saved[NUM_LEDS];
      memcpy(saved, leds, sizeof(saved));
      drawText(fnDirection < 0 ? "<<" : ">>", 7, 8, UI_WHITE);
      FastLED.show();
      memcpy(leds, saved, sizeof(saved));
    } else {
      FastLED.show();  // Shows both matrix (PIN 17) and strip (PIN 24)
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

// One-shot present: bypass FPS throttle so the red border is actually on the LEDs
// before capture starts (throttled FastLEDshow() was the missing-border case).
void presentRecordingBorder() {
  presentGridThenRecordingOverlay(false);
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

// Analyze other voices over a bounded page range. The destination page carries
// the most weight, adjacent pages less, and distant pages establish style/key.
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

        // Always track each channel's own home row & mean velocity across pages
        // (even if excluded from companion accompaniment weights) so drums/voices stay anchored.
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
          if ((phase & 3) == 0) harmW += pageWeight;  // downbeat harmonic anchor
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

  // Score tonic/mode candidates using Krumhansl-style tonal weights + bass root evidence.
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
      // Strong bonus when bar 0 or overall bass emphasizes this root
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

  int8_t scale[7];
  companionFillScalePcs(harm, scale);

  // Per-bar harmonic locking: if existing bass/harmonic voices sound in a 4-step bar,
  // lock that bar's chord degree & root to the actual notes on the page.
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

    // Score each diatonic triad (deg, deg+2, deg+4) against the bar's pitch classes
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
      if (d == (int)prog[bar]) sc += 2;  // gentle tie-breaker toward progression
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
  // A short row range may not contain every pitch class.
  return prefer;
}

// Pick pitch class from a scale-degree offset + harmonic role so motifs repeat coherently.
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
    if (motifStep == 1) return chord[2];  // fifth
    if (motifStep == 2) return chord[1];  // third
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

// Channels 13+14 form one two-note voice-led dyad: each lane gets a distinct chord tone
// and stays close to its previous row for smooth voice leading.
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
  // Keep half of anchors conventional; distribute the rest one step around it.
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

FLASHMEM static uint16_t companionMaxPhaseWeight(const uint16_t weights[16]) {
  uint16_t maximum = 0;
  for (int phase = 0; phase < 16; phase++) maximum = max(maximum, weights[phase]);
  return maximum;
}

FLASHMEM static int companionRelativeWeight(uint16_t value, uint16_t maximum) {
  if (maximum == 0) return 0;
  return constrain((int)((uint32_t)value * 100UL / maximum), 0, 100);
}

// Percussion pitch row: stay tightly anchored to the channel's established home row
// so drum samples keep their tuned character instead of jumping wildly across the grid.
FLASHMEM static int companionPercussionRow(uint8_t channel, CompanionRole role, int phase,
                                  bool structural, int variation, const CompanionContext &ctx) {
  int home = (channel < 16 && ctx.channelHomeRow[channel] >= 1)
                 ? (int)ctx.channelHomeRow[channel]
                 : (int)companionDefaultHomeRow(role);
  if (structural) return constrain(home, 1, 16);

  if (role == CompanionRole::Kick || role == CompanionRole::Snare ||
      role == CompanionRole::Clap) {
    // Pickups and ghost hits stay on home row (or at most ±1 on final turnaround step).
    if (phase >= 14 && ((variation + phase) % 4 == 0)) {
      return constrain(home + ((variation & 1) ? 1 : -1), 1, 16);
    }
    return constrain(home, 1, 16);
  }

  if (role == CompanionRole::ClosedHat) {
    // Subtle open/accented hat inflection (±1 row on off-beats)
    if ((phase & 3) == 2 && ((variation + phase) % 3 == 0)) {
      return constrain(home + 1, 1, 16);
    }
    return constrain(home, 1, 16);
  }

  if (role == CompanionRole::Tom) {
    // Toms can step across 3 adjacent pitch rows (high-mid-low turnaround)
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
  return 1;  // samples 1–8 and each 13/14 synth lane are monophonic
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

// Synthesize a coherent 16-step phrase with an 8-step seed motif (Call) and an 8-step Answer
// that locks tightly to the other voices' groove, bass, and per-bar harmony.
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

  // If Ch 6 already plays a dense bassline, Ch 11 acts as a polyphonic chord/arp companion
  // in mid register instead of clashing on a second low bassline.
  bool ch11AsPolyComp = (channel == 11 && ctx.channelHitCount[6] >= 3);
  if (ch11AsPolyComp) {
    role = CompanionRole::Keys;
  }

  // Build an 8-step rhythmic + melodic motif cell so the second half of the bar
  // repeats the rhythmic hook with a musical cadence/answer instead of random noise.
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
      // Interlock with bass/kick: favor chord downbeats and empty syncopated pockets
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
    int bar = phase / 4;  // four harmonic slots across one 16-step page
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
          // Pickup kicks leading into beat 3 or next bar
          bool preSnareOrBar = (phase + 1 == snareAnchorB || phase >= 14);
          chance = preSnareOrBar ? 38 : 18;
        } else if (!onSnare && phase == 4 && (variation % 3 == 0)) {
          chance = 76;  // 4-on-the-floor feel
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
          // 16th ghost ticks in empty gaps or before snare
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
          // Answer phrase in bar 4: walk up or syncopate into the loop turnaround
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
            degStep = 0;  // Lock root on primary kick downbeats
          } else if (phase == 14) {
            degStep = 4;  // Fifth on beat 4.5
          } else if (phase == 15) {
            degStep = 6;  // Leading scale tone into bar 0 root
          } else if (secondHalf) {
            degStep = contour[(halfStep + 2) & 7];  // Answered contour in second half
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
          // Melodic/chord answer turnaround at end of phrase
          motifHit = (phase == 14) || (phase == 15 && (channel == 7));
        }
        // Keep 13 and 14 locked together as a 2-voice synth section
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
          // Polyphonic chord comping on Ch 11 in pockets around the bass/snare
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

        // Channel 7 (or Ch 11 when not poly-comping): motif-driven melodic hook with Call & Answer
        if (!motifHit) continue;
        int degStep = contour[halfStep];
        if (secondHalf) {
          // Answer phrase: step the contour by a 3rd in bars 3–4 and resolve to chord root/3rd on last hit
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
        // Channel 8: sustained voice-led chord tones on harmonic boundaries
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

// Draw mode + touch4 held + long-press enc1 ("2000"): wipe the current edit page and
// generate a curated multi-voice subset of channels 1..14 (not all voices at once)
// using the same context-aware companion engine so each generated voice locks to the previous ones.
FLASHMEM void drawRandomsMultiPage() {
  extern bool isChildVoiceDisabled(int channel);
  int page = constrain((int)GLOB.edit, 1, companionEffectivePages());
  int activePages = companionActivePages();
  int seed = random(1, 256);

  // Curated subset of voices 1..14 (typically 5–7 voices, never all 11 at once):
  // - Drums: Kick (1) + Snare (2) or Clap (4) (or both occasionally) + Hat (3) + optional Tom (5)
  // - Bass: Ch 6 or Ch 11 (or Ch 6 bass + Ch 11 poly comping occasionally)
  // - Melodic/Synth: one or two of {Ch 7 lead, Ch 13+14 synth dyad, Ch 8 pad}
  bool pick[16] = {};
  pick[1] = (random(100) < 90);  // Kick almost always anchors
  int backbeatPick = random(100);
  if (backbeatPick < 48) {
    pick[2] = true;              // Snare
  } else if (backbeatPick < 78) {
    pick[4] = true;              // Clap
  } else {
    pick[2] = true;              // Snare + Clap layered
    pick[4] = true;
  }
  pick[3] = (random(100) < 82);  // Closed hat
  pick[5] = (random(100) < 35);  // Occasional tom/percussion

  int bassPick = random(100);
  if (bassPick < 45) {
    pick[6] = true;              // Sample bass
  } else if (bassPick < 82) {
    pick[11] = true;             // Poly synth bass
  } else {
    pick[6] = true;              // Sample bass + Ch 11 poly chord comp
    pick[11] = true;
  }

  int harmPick = random(100);
  if (harmPick < 38) {
    pick[7] = true;              // Sample keys/melody
  } else if (harmPick < 76) {
    pick[13] = true;             // Paired mono synths
    pick[14] = true;
  } else {
    pick[7] = true;
    pick[8] = true;              // Melody + pad
  }
  if (!pick[8] && random(100) < 30) {
    pick[8] = true;              // Occasional pad glue
  }

  // Clear all notes on the current edit page first
  clearCompanionPage(page);

  // Analyze other pages (if any) so key/root and home rows stay consistent with the rest of the track
  CompanionContext baseCtx = analyzeCompanionContext(1, activePages, page, 0);
  CompanionHarmony pageHarm = companionMakeHarmony(baseCtx, seed);
  CompanionGroove pageGroove = companionMakeGroove(baseCtx, page, seed);

  // Generate in musical role order so each voice reacts to the voices already placed on this page
  static const uint8_t generationOrder[] = {1, 2, 4, 3, 5, 6, 11, 13, 14, 7, 8};
  for (uint8_t ch : generationOrder) {
    if (!pick[ch] || isChildVoiceDisabled(ch)) continue;
    CompanionContext ctx = analyzeCompanionContext(1, max(activePages, page), page, ch);
    generateCompanionChannelPage(page, ch, ctx, pageHarm, pageGroove, seed);
  }

  updateLastPage();
}

// Generate rhythmic patterns for channels 1-4
FLASHMEM void generateRhythmicPattern(unsigned int start, unsigned int end, unsigned int channel, HarmonicAnalysis harmony) {
  // Define 4/4 time signature patterns with more sophisticated rhythms
  
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
  
  for(unsigned int c = start; c < end; c++) {
    int x_rel = c - start + 1;
    int beatPosition = randStep16((unsigned int)x_rel); // tile 16-step groove across maxX
    
    // Create melodic phrases with musical intervals
    if(beatPosition % phraseLength == 1) {
      // Start of phrase - choose a strong note
      currentNote = random(0, scaleSize);
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

// Context-aware melodic pattern generation that preserves base page melody

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
  
  // Set the BPM directly in SMP.bpm and update the BPM mode
  SMP.bpm = targetBPM;
  
  // Update the BPM mode position
  bpmMode.pos[3] = targetBPM;
  
  // If currently in BPM mode, update the encoder
  if (currentMode == &bpmMode) {
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
  extern bool voiceMode;
  extern bool songModeActive;
  extern bool isNowPlaying;
  extern unsigned int beatForUI;
  extern uint8_t voiceEditPage[16];
  extern unsigned int voicePageFor(int channel, unsigned int globalPage);

  // VMOD draw is a composite of per-voice pages — a global page wipe is meaningless.
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
  
  // Reset paint/unpaint prevention flag after clearPage operation
  extern bool preventPaintUnpaint;
  preventPaintUnpaint = false;
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
    setParams(DELAY, ch);
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
  extern Mode draw;
  extern Mode bpmMode;
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
  EEPROM.write(EEPROM_DATA_START + 3,  (uint8_t)-1);  // patternMode (OFF)
  EEPROM.write(EEPROM_DATA_START + 4, (uint8_t)-1);  // voiceSelect (OFF)
  EEPROM.write(EEPROM_DATA_START + 5,  2);    // fastRecMode (SENS)
  EEPROM.write(EEPROM_DATA_START + 58, 0xA5); // TRIG default already SENS
  EEPROM.write(EEPROM_DATA_START + 6,  0);    // recChannelClear (OFF)
  EEPROM.write(EEPROM_DATA_START + 7,  20);   // previewVol
  EEPROM.write(EEPROM_DATA_START + 8,  1);    // flowMode (ON)
  EEPROM.write(EEPROM_DATA_START + 9,  10);   // micGain
  EEPROM.write(EEPROM_DATA_START + 10, (6 << 2)); // PPQN pulse: OFF, +, 24, CONT
  EEPROM.write(EEPROM_DATA_START + 30, 12);   // PPQN pulse width ms
  EEPROM.write(EEPROM_DATA_START + 11, 1);    // simpleNotesView (EASY)
  EEPROM.write(EEPROM_DATA_START + 12, 0);    // loopLength (OFF)
  EEPROM.write(EEPROM_DATA_START + 13, preserveLedMode);  // ledMode: keep SETT>LEDS
  EEPROM.write(EEPROM_DATA_START + 14, 1);    // ctrlMode (VOL)
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
  EEPROM.write(EEPROM_DATA_START + 55, 0xA5); // SPKR polarity migrate done
  EEPROM.write(EEPROM_DATA_START + 56, 1);   // REC HPF default (library high-pass ON)
  EEPROM.write(EEPROM_DATA_START + 57, 0);   // VOL USB preview default (OFF)
  EEPROM.write(EEPROM_DATA_START + 28, 1);    // midiNoteReceive
  EEPROM.write(EEPROM_DATA_START + 29, 0);    // transportSendDelayMs
  EEPROM.write(EEPROM_DATA_START + 31, 0);    // transportRcveDelayMs
  EEPROM.write(EEPROM_DATA_START + 32, 0);  // codecHfCut
  EEPROM.write(EEPROM_DATA_START + 33, 5);    // HFC format
  EEPROM.put(EEPROM_DATA_START + 34, (uint16_t)0x0006);  // drawRFullMuteCustomUnmuteMask
  EEPROM.write(EEPROM_DATA_START + 36, 0);    // childLockEnabled (OFF)
  EEPROM.write(EEPROM_DATA_START + 37, 1);    // MIDI pitch clamp (ON)
  EEPROM.write(EEPROM_DATA_START + 38, 10);   // mixGain14 (unity)
  EEPROM.write(EEPROM_DATA_START + 39, 10);   // mixGain58 (unity)
  EEPROM.write(EEPROM_DATA_START + 40, 10);   // mixGainSynth (unity)
  EEPROM.write(EEPROM_DATA_START + 41, 10);   // mixGainMaster (unity)
  EEPROM.write(EEPROM_DATA_START + 42, 0);     // voiceMode (OFF)
  EEPROM.write(EEPROM_DATA_START + 43, 0);     // fireVoice (OFF)
  EEPROM.write(EEPROM_DATA_START + 44, 8);     // fireLevel (8 particles)
  EEPROM.write(EEPROM_DATA_START + 45, 1);     // fireSize (1 cell)
  EEPROM.write(EEPROM_DATA_START + 46, 0);     // fireGravity (float up)
  EEPROM.write(EEPROM_DATA_START + 47, 8);     // fireColor (full voice colour)
  EEPROM.write(EEPROM_DATA_START + 48, 0);     // imageMode (OFF)
  EEPROM.write(EEPROM_DATA_START + 49, 1);     // eyesMode (1 min)
  EEPROM.write(EEPROM_DATA_START + 51, 1);     // SIDE voice (CH1)
  EEPROM.write(EEPROM_DATA_START + 52, 0);     // EXT clock source MIDI (not analog)
  EEPROM.write(EEPROM_DATA_START + 53, 2);     // CIN: 4 PPQN, 1 ms hold-off
  EEPROM.write(EEPROM_DATA_START + 54, 0);     // MIDI WRTE (OFF)
  
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
    SMP.filter_settings[ch][HCUT] = (ch == 14) ? 14 : 32;
    SMP.filter_settings[ch][LOWCUT] = 0;
    SMP.filter_settings[ch][REVERB] = 0;
    SMP.filter_settings[ch][BITCRUSHER] = 0;
    SMP.filter_settings[ch][RES] = (ch == 14) ? 5 : 0;
    SMP.filter_settings[ch][DETUNE] = 16;
    SMP.filter_settings[ch][OCTAVE] = (ch >= 1 && ch <= 8) ? 24 : 16;
    SMP.filter_settings[ch][EFX] = 0;  // Sample mode
    
    // Reset parameter data (no hardware calls)
    // Default ADSR: delay=0, A=32, D=32, S=32, R=16 on synths 11/13 (ch14 303: D=10, S=0, R=6)
    SMP.param_settings[ch][DELAY] = 0;
    SMP.param_settings[ch][ATTACK] = 32;
    SMP.param_settings[ch][DECAY] = (ch == 14) ? 20 : 32;
    SMP.param_settings[ch][SUSTAIN] = (ch == 14) ? 20 : 32;
    SMP.param_settings[ch][RELEASE] = (ch == 11 || ch == 13 || ch == 14) ? 16 : 0;

    if (ch >= 1 && ch <= 8) {
      setSampleTimeStretch(ch, 11);
    }
    
    // Reset synth data (only for channels 11, 13-14)
    if (ch == 11) {
      extern void applySynthInstrumentPreset(int channel, int instrumentIdx);
      applySynthInstrumentPreset(11, 12);  // Default ch11 to DXBS (12)
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
  bpmMode.pos[3] = SMP.bpm;
  playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
  playTimer.update((uint32_t)round(playNoteInterval));
  bpmMode.pos[2] = (clockMode == 1) ? 1 : 0;
  
  // 9. RESET WAVE FILE IDS to defaults
  for (int i = 1; i < maxFiles; i++) {
    SMP.wav[i].oldID = 0;   // folder 0
    SMP.wav[i].fileID = i;  // file index = voice index (1..8)
  }
  
  // 10. UPDATE LAST PAGE (should be 1 since no notes)
  updateLastPage();
  
  // 11. RESET PATTERN MODE FLAGS (before switching mode)
  songModeActive = false;
  SMP_PATTERN_MODE = false;  // patternMode=-1 means OFF
  patternMode = -1;  // OFF (matches EEPROM default)
  
  // Reset paint/unpaint prevention flag
  preventPaintUnpaint = false;
  
  // 12. SWITCH TO DRAW MODE
  delay(300);  // Brief delay to show "NEW" message
  switchMode(&draw);
  applyDrawSingleYEncoderLimits();
  
  // 13. SYNC ENCODERS WITH CURRENT POSITION
  // Force encoder positions to match reset state
  Encoder[0].writeMax((int32_t)maxY);
  Encoder[0].writeMin((int32_t)1);
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

  // 14b. Ensure all filter pages (including DELAY, LFO, ARP, synth presets) are reset and applied.
  // Same path as FILTERMODE "0002" (all filter pages for each voice).
  extern void setAllFilterPagesDefaultValues(int ch);
  extern uint8_t filterPage[NUM_CHANNELS];
  for (int i = 0; i < numChannels; i++) {
    int ch = channels[i];
    filterPage[ch] = 0;
    setAllFilterPagesDefaultValues(ch);
  }
  
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

// Function to get a note from a given step and channel

/***************/
/**** INTRO ****/
/***************/

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
  // Split each curve between neighbouring LEDs instead of rounding its Y.
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

// Shared logo+sine frame. holdNoFadeOut keeps full brightness after fade-in
// (INFO loop); boot mode still fades everything out at the end.
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

  for (unsigned int x = 1; x <= maxX; x++) {
    const float xNorm = (float)(x - 1) / widthDenominator;
    const float xPhase = xNorm * 4.0f * (float)M_PI;

    // Darker dual sines; blue first so white stays readable on overlap.
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

// ----- Run the Animation Once (Called in setup) -----
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

// ETC → INFO (encoder 4 / 0001): fade in, then keep animating until the same
// button is pressed again. No fade-out while held open.
FLASHMEM void runInfoLogoAnimationLoop() {
  extern bool pressed[NUM_ENCODERS];
  extern bool isPressed[NUM_ENCODERS];
  extern uint8_t buttons[NUM_ENCODERS];
  extern ButtonState buttonState[NUM_ENCODERS];
  extern int currentEncoderIndex;
  extern void resetEtcInfoPageAnimation();

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
    currentEncoderIndex = 3;
    Encoder[3].updateStatus();
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

// Shift a preserved MIDI pitch or grid row along the active diatonic scale by `degDelta` scale degrees.
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

// Place a note while preserving midiPitch, probability, and condition.
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
  // If row had to move because of collision and midiPitch wasn't set, keep row pitch;
  // if midiPitch was set and row didn't move (or channel uses midiPitch), preserve it.
  note[step][freeRow].midiPitch = midiPitch;
  return true;
}

// Evolve a single voice on `dstPage` from `srcPage` according to its 4-bar phrase arc role:
// arcRole 0 = Groove A' (subtle micro-variation, 90% faithful)
// arcRole 1 = Response B (call in steps 1-8, melodic/syncopated answer in steps 9-16)
// arcRole 2 = Lift / Development (harmonic inversion, octave energy, hat propulsion)
// arcRole 3 = Turnaround / Fill (grounded steps 1-11, coordinated fill + walkup on steps 12-16)
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

  // Collect source events for this channel on srcPage
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

  // 1. Copy and mutate existing source events
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
      // Drums: never break primary downbeats/backbeats; vary turnaround & off-beats
      if (arcRole == 3 && inTurnaround && role == CompanionRole::ClosedHat && phase >= 14) {
        // Thin hats slightly during big snare/tom turnaround fill
        dstVel = max(32, dstVel - 18);
      } else if (arcRole == 1 && secondHalf && !quarter && phase != 12 && (h % 100) < 22) {
        // Response B: subtle 16th syncopation shift on off-beat percussion if pocket is free
        int candidate = constrain(dstOffset + ((h & 1) ? 1 : -1), 0, (int)steps - 1);
        if ((occupiedMask & (1UL << (candidate & 31))) == 0 && (candidate & 3) != 0) {
          dstOffset = candidate;
        }
      }
      // Gentle velocity humanization (±4) so repeats breathe
      dstVel = constrain(dstVel + (int)(h % 9) - 4, 24, 127);
    } else if (role == CompanionRole::Bass) {
      // Bass: keep beat 1 & main groove intact; evolve second half / turnaround
      if (arcRole == 1 && secondHalf && !quarter) {
        // Response B: answer phrase steps up/down to 5th or 3rd
        int delta = ((h & 3) == 0) ? 2 : (((h & 1) != 0) ? 4 : -1);
        companionStepPitch(harm, channel, delta, dstRow, dstMidi);
      } else if (arcRole == 2 && ((phase == 6 || phase == 10 || phase == 14) || (!quarter && (h % 100) < 45))) {
        // Lift: octave / fifth energy on off-beats
        companionStepPitch(harm, channel, 4, dstRow, dstMidi);
        dstVel = constrain(dstVel + 6, 36, 127);
      } else if ((arcRole == 3 || isFinalPage) && inTurnaround && phase > 12) {
        // Turnaround walkup toward the root
        int walkDeg = (phase == 13) ? 2 : ((phase == 14) ? 4 : 6);
        companionStepPitch(harm, channel, walkDeg, dstRow, dstMidi);
      }
    } else if (role == CompanionRole::Keys || role == CompanionRole::VocalsPad) {
      // Melodic / Synth / Chords: keep call (steps 1-8) recognizable, evolve answer & inversions
      if (arcRole == 0 && inTurnaround && !quarter && (h % 100) < 40) {
        companionStepPitch(harm, channel, (h & 1) ? 1 : -1, dstRow, dstMidi);
      } else if (arcRole == 1 && secondHalf) {
        // Call & Response: answer the motif in bars 3-4 with a diatonic 3rd/2nd contour shift
        int delta = inTurnaround ? ((phase >= 14) ? 0 : -1) : ((h & 1) ? 2 : -2);
        companionStepPitch(harm, channel, delta, dstRow, dstMidi);
      } else if (arcRole == 2) {
        // Development / Lift: shift chord inversion / upper extension (+2 scale degrees)
        int delta = quarter ? 2 : ((h & 1) ? 2 : 1);
        companionStepPitch(harm, channel, delta, dstRow, dstMidi);
        dstVel = constrain(dstVel + 5, 32, 124);
      } else if (arcRole == 3 || isFinalPage) {
        if (inTurnaround) {
          // Cadence resolution toward chord root/fifth at bar end
          int delta = (phase >= 14) ? 0 : 4;
          companionStepPitch(harm, channel, delta, dstRow, dstMidi);
        }
      }
    }

    companionPlaceCellLimited(dstStart + (unsigned int)dstOffset, dstRow, channel,
                              dstVel, dstProb, dstCond, dstMidi);
  }

  // 2. Add role-specific variations (ghost pickups, hat propulsion, turnaround fills, answer notes)
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
      // Add a pickup kick on step 15 (or 11 in Response/Turnaround) leading into downbeat
      bool pickup = ((arcRole == 1 || arcRole == 3) && (phase == 14 || phase == 15) &&
                     ctx.snareWeight[phase] == 0 && (stepH % 100) < 42);
      if (pickup) {
        companionPlaceCellLimited(dstStart + s, homeRow, channel,
                                  constrain(meanVel - 16, 55, 108), 100, 1, sampleMidiPitch);
      }
    } else if (role == CompanionRole::Snare || role == CompanionRole::Clap) {
      // Turnaround snare/clap fill on steps 14-16, or subtle ghost note in Response/Lift
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
      // Lift (arcRole 2) or Turnaround (arcRole 3): add 16th off-beat propulsion ticks
      bool add16th = ((arcRole == 2 && (phase & 1) == 1 && (stepH % 100) < 35) ||
                      (arcRole == 3 && phase >= 12 && (stepH % 100) < 55));
      if (add16th) {
        companionPlaceCellLimited(dstStart + s, homeRow, channel,
                                  constrain(meanVel - 22, 36, 78), 100, 1, sampleMidiPitch);
      }
    } else if (role == CompanionRole::Tom) {
      // Turnaround tom fill across steps 13-16
      if ((arcRole == 3 || isFinalPage) && phase >= 13 && ctx.snareWeight[phase] == 0 &&
          (stepH % 100) < 62) {
        int fillRow = constrain((int)homeRow + (14 - phase), 1, 16);
        companionPlaceCellLimited(dstStart + s, fillRow, channel,
                                  constrain(meanVel - 8 + (phase - 13) * 6, 52, 114),
                                  100, 1, sampleMidiPitch);
      }
    } else if (role == CompanionRole::Bass) {
      // Add an octave/fifth pickup on step 14 or 15 in Response/Turnaround bars
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
      // Add a passing/answer ornament on step 6 or 14 when sparse
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

// MODE > AUTO: true continuation and variation engine.
// Clones and evolves the user's base pages (baseStart..baseEnd) across a 4-page musical arc
// (Groove A' -> Response B -> Lift/Development -> Turnaround/Fill) so every voice keeps its
// tuned rows, MIDI pitches, and core pocket while developing naturally.
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

  // If the base range is completely empty, synthesize a cohesive starter groove on baseStart
  // first so baseStart..outputEnd form one continuous track.
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

    // PMOD: carry the source page's mute map onto the generated page so muted
    // voices stay silent when the continuation plays.
    if (SMP_PATTERN_MODE) {
      copyPageMutes(srcPage, page);
    }

    // Analyze source page context & harmony so variations lock to the exact key and bar roots
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
        // Voice exists in the base range, just not on srcPage: find any base page that has it,
        // or synthesize a companion part during Lift/Turnaround pages.
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
