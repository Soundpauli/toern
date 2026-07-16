// SD card file server over USB Serial (Menu → ETC → SD).
// Active only while that menu page is open; polled from loop().
// Companion client: website/tools/sd-tool-standalone/toern_sd.py
//
// Line commands (UTF-8, terminated by '\n'):
//   PING
//   LIST [path]
//   RM <path>
//   MKDIR <path>
//   MV <from> <to>   (also REN)
//   PUT <path> <size> <crc32>
//     → READY
//     ← <8192-byte blocks>  (host waits for ACK after each block — USB flow control)
//     → ACK
//     → OK | ERR ...
//   GET <path>
//     → OK <size>
//     ← ACK                    (host ready)
//     → <512-byte data blocks>
//     ← ACK                    (after each block)
//     → CRC <crc32>
//
// LIST file lines: F <size> <duration_ms> <name>  (duration_ms = -1 if unknown/non-WAV)
// Binary transfers follow PUT/GET handshake; CRC32 is IEEE (zlib/binascii compatible).

#if defined(ARDUINO)

static const size_t SD_SER_CHUNK = 4096;
static const size_t SD_SER_PUT_BLOCK = 8192;  // host waits for ACK after each block (USB flow control)
static const size_t SD_SER_GET_BLOCK = 512;   // host ACKs each block (USB flow control)
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
  // Stop anything that might touch the SD card. Prefer this over holding
  // AudioNoInterrupts() across Serial I/O — on Teensy 4 that is __disable_irq()
  // and USB TX deadlocks once the CDC buffer fills.
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

// Reject empty, absolute-escape, and ".." path segments.
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

static bool sdSerEndsWithIgnoreCase(const char *name, const char *suffix) {
  if (!name || !suffix) return false;
  size_t n = strlen(name), s = strlen(suffix);
  if (n < s) return false;
  for (size_t i = 0; i < s; i++) {
    char a = name[n - s + i];
    char b = suffix[i];
    if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
    if (a != b) return false;
  }
  return true;
}

static uint32_t sdSerReadU32LE(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t sdSerReadU16LE(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

// Returns duration in milliseconds, or -1 if not a readable WAV.
static int32_t sdSerWavDurationMs(File &f) {
  uint8_t hdr[12];
  if (f.read(hdr, 12) != 12) return -1;
  if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) return -1;

  uint32_t sampleRate = 0;
  uint16_t blockAlign = 0;
  uint32_t byteRate = 0;
  uint32_t dataSize = 0;
  bool gotFmt = false;
  bool gotData = false;

  while (f.available() >= 8) {
    uint8_t chunkHdr[8];
    if (f.read(chunkHdr, 8) != 8) break;
    uint32_t sz = sdSerReadU32LE(chunkHdr + 4);
    if (memcmp(chunkHdr, "fmt ", 4) == 0) {
      uint8_t fmt[16];
      size_t need = sz < 16 ? (size_t)sz : 16;
      if (need < 14 || f.read(fmt, need) != (int)need) break;
      if (sz > need) {
        uint32_t skip = sz - (uint32_t)need;
        f.seek(f.position() + skip);
      }
      if (sz & 1u) f.read();  // pad byte
      sampleRate = sdSerReadU32LE(fmt + 4);
      byteRate = sdSerReadU32LE(fmt + 8);
      blockAlign = sdSerReadU16LE(fmt + 12);
      gotFmt = true;
    } else if (memcmp(chunkHdr, "data", 4) == 0) {
      dataSize = sz;
      gotData = true;
      break;
    } else {
      f.seek(f.position() + sz + (sz & 1u));
    }
    yield();
  }

  if (!gotFmt || !gotData) return -1;
  if (byteRate == 0 && sampleRate > 0 && blockAlign > 0) {
    byteRate = sampleRate * (uint32_t)blockAlign;
  }
  if (byteRate == 0) return -1;
  return (int32_t)((dataSize * 1000ull) / (uint64_t)byteRate);
}

static void sdSerCmdList(const char *pathArg) {
  char path[128];
  if (!sdSerNormalizePath(pathArg ? pathArg : "/", path, sizeof(path))) {
    sdSerReply("ERR BAD_PATH");
    return;
  }

  // Lock SD against audio ISR — concurrent SdFat access hard-faults and drops USB.
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
      // Skip WAV duration probing — opening every file made LIST hang on large cards.
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

  // Pause audio up front; only hold __disable_irq around SD writes — never
  // across Serial.read/ACK (USB needs IRQs).
  sdSerAudioStopForSd();
  while (remaining > 0) {
    // One flow-control block: host must wait for ACK before sending more.
    // USB CDC ignores baud rate and will otherwise overrun RX during SD writes.
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
      else if (c != '\r') len = 0;  // overflow — resync
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

  // Pause audio (may use SD). Do not use AudioNoInterrupts here — on Teensy 4 that is
  // __disable_irq() and breaks USB CDC for larger transfers.
  sdSerAudioStopForSd();

  File in = SD.open(path, FILE_READ);
  if (!in || in.isDirectory()) {
    if (in) in.close();
    sdSerReply("ERR OPEN");
    return;
  }

  uint32_t size = (uint32_t)in.size();
  // Single-pass: no CRC pre-scan (that was wedging USB on files ≳36KB).
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
    // Brief pause every 8KB keeps the USB stack healthy under browser load.
    if ((sent & 0x1fff) == 0) delayMicroseconds(200);
  }
  in.close();

  sdSerReplyf("CRC %08lX", (unsigned long)crc);
#if defined(CORE_TEENSY)
  Serial.send_now();
#endif
}

static void sdSerTouchClient() {
  sdSerLastClientMs = millis();
  if (!sdSerClientConnected) {
    sdSerClientConnected = true;
    extern void menuRequestFullRedraw();
    menuRequestFullRedraw();
  }
}

static char *sdSerSkipSpaces(char *s) {
  while (s && (*s == ' ' || *s == '\t')) s++;
  return s;
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
    sdSerCmdPing();
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
  } else {
    sdSerReply("ERR UNKNOWN");
  }
}

bool sdSerialServerIsActive() {
  return sdSerActive;
}

bool sdSerialServerClientConnected() {
  return sdSerActive && sdSerClientConnected;
}

void sdSerialServerSetActive(bool on) {
  if (on == sdSerActive) return;

  if (on) {
    extern bool isNowPlaying;
    extern void pause(bool skipSave);
    extern void allOff();
    extern void stopAllSetWavPreviewAudio();
    if (isNowPlaying) pause(true);
    allOff();
    stopAllSetWavPreviewAudio();

    Serial.setTimeout(50);
    sdSerDrainRx();
    sdSerClientConnected = false;
    sdSerLastClientMs = 0;
    sdSerActive = true;
    sdSerReplyf("OK TOERN SD %s", VERSION);
  } else {
    sdSerActive = false;
    sdSerClientConnected = false;
    sdSerLastClientMs = 0;
    sdSerDrainRx();
  }
}

void sdSerialServerPoll() {
  if (!sdSerActive) return;

  if (sdSerClientConnected && (millis() - sdSerLastClientMs > SD_SER_CLIENT_TIMEOUT_MS)) {
    sdSerClientConnected = false;
    extern void menuRequestFullRedraw();
    menuRequestFullRedraw();
  }

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
        sdSerLineLen = 0;  // overflow — reset
        sdSerReply("ERR LINE");
      }
    }
  }
}

// When SD page is not open, still answer PING so the host knows the device is alive.
void sdSerialServerPollNeedSdHint() {
  if (sdSerActive) return;
  // Don't steal binary color protocols (COLR/BRIT/SAVE) — those run on ETC→COLR.
  while (Serial.available()) {
    int peek = Serial.peek();
    if (peek < 0) break;
    if (peek == 'C' || peek == 'B' || peek == 'S') return;

    int b = Serial.read();
    if (b < 0) break;
    char c = (char)b;
    if (c == '\n') {
      sdSerLineBuf[sdSerLineLen] = '\0';
      if (sdSerLineLen > 0 && sdSerLineBuf[sdSerLineLen - 1] == '\r') {
        sdSerLineBuf[sdSerLineLen - 1] = '\0';
      }
      char *line = sdSerSkipSpaces(sdSerLineBuf);
      if (line[0] != '\0') {
        // Any text command while not on SD page.
        sdSerReply("ERR NEED_SD");
      }
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

#endif  // ARDUINO
