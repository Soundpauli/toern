// SD card file server over USB Serial (Menu → ETC → SD).
// Active only while that menu page is open; polled from loop().
// Companion client: tools/sd-tool-standalone/toern_sd.py
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
//   GETPAT / PUTPAT <size> <crc32>
//     Same framing as GET / PUT, but the bytes are the current pattern in RAM
//     (notes, BPM, MIDI pitches). Nothing is written to the SD card.
//   IMPORTSAVE <slot>
//     Configure source-faithful sampler defaults, load samplepack <slot>, save the
//     current RAM pattern through the normal TŒRN save path, then load it back
//     through the normal TŒRN pattern loader. This persists the full SMP struct.
//
// LIST file lines: F <size> <duration_ms> <name>  (duration_ms = -1 if unknown/non-WAV)
// Binary transfers follow PUT/GET handshake; CRC32 is IEEE (zlib/binascii compatible).

#if defined(ARDUINO)

// Reliability over raw speed: smaller synchronous SdFat writes avoid short
// writes/ERR IO on some cards while USB CDC is active. The external PUT ACK
// framing stays 8192 bytes, so existing host tools remain compatible.
static const size_t SD_SER_CHUNK = 512;
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

// Host is talking (PING / pattern transfer). Does not stop audio.
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
  // First contact on the SD page stops play so the card is safe to touch.
  sdSerAudioStopForSd();
  extern void menuRequestFullRedraw();
  menuRequestFullRedraw();
}

static char *sdSerSkipSpaces(char *s) {
  while (s && (*s == ' ' || *s == '\t')) s++;
  return s;
}

// Current-pattern RAM image: notes (x then y) + FF FE + float BPM + TPIT pitches.
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
    volume_bpm.pos[3] = (unsigned int)bpm;
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

static bool sdSerParsePatternSlot(const char *args, long &slot) {
  if (!args || !*args) return false;
  char *end = nullptr;
  slot = strtol(args, &end, 10);
  while (end && (*end == ' ' || *end == '\t')) end++;
  return slot >= 1 && slot <= 999 && (!end || *end == '\0');
}

// TIME is a runtime array, not part of Device/SMP in the current firmware.
// We persist it backward-compatibly in the otherwise unused legacy SPEED slot:
// stored = TIME + 1, so legacy zero means "not stored" and loads as OFF (11).
static inline void sdSerStoreTimeStretchInSmp(int ch, uint8_t value) {
  SMP.filter_settings[ch][SPEED] = (float)(constrain((int)value, 0, 21) + 1);
}

static bool sdSerImportedSettingsOk(unsigned int slot) {
  if (SMP.file != slot || SMP.pack != slot) return false;
  for (int ch = 1; ch <= 8; ch++) {
    if ((int)lroundf(SMP.channelVol[ch]) != 16) return false;
    if ((int)lroundf(SMP.filter_settings[ch][HCUT]) != 32) return false;
    if ((int)lroundf(SMP.filter_settings[ch][LOWCUT]) != 0) return false;
    if ((int)lroundf(SMP.filter_settings[ch][REVERB]) != 0) return false;
    if ((int)lroundf(SMP.filter_settings[ch][BITCRUSHER]) != 0) return false;
    if ((int)lroundf(SMP.filter_settings[ch][DETUNE]) != 16) return false;
    if ((int)lroundf(SMP.filter_settings[ch][OCTAVE]) != 24) return false;
    if ((int)lroundf(SMP.filter_settings[ch][RES]) != 0) return false;
    if ((int)lroundf(SMP.filter_settings[ch][EFX]) != 0) return false;
    if ((int)lroundf(SMP.filter_settings[ch][SPEED]) != 12) return false; // TIME OFF = 11, encoded +1
    if ((int)lroundf(SMP.param_settings[ch][ATTACK]) != 32) return false;
    if ((int)lroundf(SMP.param_settings[ch][DECAY]) != 32) return false;
    if ((int)lroundf(SMP.param_settings[ch][SUSTAIN]) != 32) return false;
    if ((int)lroundf(SMP.param_settings[ch][RELEASE]) != 0) return false;
  }
  return true;
}

static void sdSerCmdImportSave(const char *args) {
  long slotLong = 0;
  if (!sdSerParsePatternSlot(args, slotLong)) {
    sdSerReply("ERR SLOT");
    return;
  }
  const unsigned int slot = (unsigned int)slotLong;

  sdSerAudioStopForSd();

  // Force ordinary samplepack paths (<slot>/1.wav ... <slot>/8.wav), not custom
  // browser selections, then load the pack before applying playback parameters.
  for (int ch = 1; ch <= 8; ch++) {
    SMP.samplePathRel[ch][0] = '\0';
    SMP.sp0Active[ch] = false;
  }
  SMP.pack = slot;
  loadSamplePack(slot, false, false);

  // Use TŒRN's own current defaults. This is deliberately neutral because the
  // extracted WAV already contains the source track's timbre/filtering.
  // Current defaults include HCUT=32, LOWCUT=0, REV=0, BITC=0, DTNE=16,
  // sample OCTV=24, RES=0, EFX=0 and ADSR A/D/S/R=32/32/32/0.
  for (int ch = 1; ch <= 8; ch++) {
    setAllFilterPagesDefaultValues(ch);
    SMP.channelVol[ch] = 16;
    setSampleTimeStretch(ch, 11); // TIME OFF
    sdSerStoreTimeStretchInSmp(ch, 11);

    // Imported patterns should not mysteriously load muted.
    SMP.globalMutes[ch] = false;
    globalMutes[ch] = false;
    SMP.mute[ch] = 0;
    for (int page = 0; page < maxPages; page++) {
      SMP.pageMutes[page][ch] = false;
      pageMutes[page][ch] = false;
    }
  }
  reapplyAllSampleChannelGains();

  // Save through the exact normal pattern writer. That writes:
  // notes + FF FE + complete raw SMP struct + TPIT pitch extension.
  SMP.file = slot;
  savePattern(false);

  char path[24];
  snprintf(path, sizeof(path), "%u.txt", slot);
  if (!SD.exists(path)) {
    sdSerReplyFlush("ERR SAVE_NOT_FOUND");
    return;
  }

  // Critical verification: reload the file through the exact normal loader.
  // loadPattern() restores SMP and then calls loadSMPSettings(), so this tests the
  // same path the user will use later from the device UI.
  SMP.file = slot;
  loadPattern(false);
  if (!sdSerImportedSettingsOk(slot)) {
    sdSerReplyFlush("ERR VERIFY_SETTINGS");
    return;
  }

  // Verify every sampler channel actually referenced by the imported pattern has
  // a non-empty sample loaded after the canonical reload.
  bool usedSampleChannel[9] = { false, false, false, false, false, false, false, false, false };
  for (unsigned int x = 1; x <= 256; x++) {
    for (unsigned int y = 1; y <= 16; y++) {
      const int ch = (int)note[x][y].channel;
      if (ch >= 1 && ch <= 8) usedSampleChannel[ch] = true;
    }
  }
  for (int ch = 1; ch <= 8; ch++) {
    if (usedSampleChannel[ch] && loadedSampleLen[ch] == 0) {
      char err[40];
      snprintf(err, sizeof(err), "ERR SAMPLE_CH%d", ch);
      sdSerReplyFlush(err);
      return;
    }
  }

  File verify = SD.open(path, FILE_READ);
  uint32_t size = verify ? (uint32_t)verify.size() : 0;
  if (verify) verify.close();
  if (size == 0) {
    sdSerReplyFlush("ERR VERIFY_FILE");
    return;
  }
  char ok[64];
  snprintf(ok, sizeof(ok), "OK SAVED %u.txt %lu", slot, (unsigned long)size);
  sdSerReplyFlush(ok);
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
    sdSerCmdPing  ();
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
  } else if (strcasecmp(cmd, "IMPORTSAVE") == 0) {
    sdSerCmdImportSave(args);
  } else {
    sdSerReply("ERR UNKNOWN");
  }
}

bool sdSerialServerIsActive() {
  return sdSerActive;
}

bool sdSerialServerClientConnected() {
  // True while a host keepalive is recent, including pattern transfer from Draw.
  // The SD page is not required.
  if (!sdSerClientConnected) return false;
  return (uint32_t)(millis() - sdSerLastClientMs) <= SD_SER_CLIENT_TIMEOUT_MS;
}

void sdSerialServerSetActive(bool on) {
  if (on == sdSerActive) return;

  if (on) {
    // Stay in WAIT without stopping play; audio stops when host connects (OK).
    // Keep a live off-page session so the screensaver does not start on the way in.
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
        sdSerLineLen = 0;  // overflow — reset
        sdSerReply("ERR LINE");
      }
    }
  }
}

// Pattern + file exchange works from any screen. PING does not open the SD page.
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

  // Same commands as the SD page, without forcing the OK screen.
  // Do not call sdSerTouchClient — that pauses audio for the SD menu OK state.
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
  } else if (strcasecmp(cmd, "IMPORTSAVE") == 0) {
    sdSerNoteHostPresent();
    sdSerCmdImportSave(args);
  } else {
    sdSerReply("ERR NEED_SD");
  }
}

void sdSerialServerPollNeedSdHint() {
  if (sdSerActive) return;
  sdSerExpireClient();

  // Only defer to the color protocol while Menu → ETC → COLR is open.
  // Elsewhere a leftover 'C'/'B'/'S' must not block PING/GETPAT/LIST forever.
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

#endif  // ARDUINO
