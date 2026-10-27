// SD card file server over USB Serial.
// Polled from loop() on every screen once a host program has opened the port.
// Menu → ETC → SD only displays SCON / WAIT / OK. It does not enable the server.
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
//
// LIST file lines: F <size> <duration_ms> <name>  (duration_ms = -1 if unknown/non-WAV)
// Binary transfers follow PUT/GET handshake; CRC32 is IEEE (zlib/binascii compatible).

#if defined(ARDUINO)

// Command/transfer work runs on demand from the main loop; keep it out of ITCM.
// The per-loop polling and connection bookkeeping remain in fast RAM.

// Buffer the whole host flow-control block before SD I/O. The host may still
// be transmitting its second half when only 4096 bytes have arrived.
static const size_t SD_SER_CHUNK = 8192;
static const size_t SD_SER_PUT_BLOCK = 8192;  // host waits for ACK after each block (USB flow control)
static const size_t SD_SER_GET_BLOCK = 512;   // host ACKs each block (USB flow control)
static const uint32_t SD_SER_IO_TIMEOUT_MS = 60000;

static bool sdSerActive = false;
static bool sdSerClientConnected = false;
static uint32_t sdSerLastClientMs = 0;
static const uint32_t SD_SER_CLIENT_TIMEOUT_MS = 15000;
static char sdSerLineBuf[192];
static size_t sdSerLineLen = 0;
static bool sdSerPutTrace = false;

FLASHMEM static void sdSerDrainRx() {
  uint32_t start = millis();
  do {
    while (Serial.available()) (void)Serial.read();
    yield();
  } while ((millis() - start) < 30u);
  sdSerLineLen = 0;
}

FLASHMEM static void sdSerAudioStopForSd() {
  // Stop playback and previews before any host-side SD mutations. USB
  // interrupts remain enabled; AudioNoInterrupts masks only the audio IRQ.
  extern bool isNowPlaying;
  extern void pause(bool skipSave);
  extern void allOff();
  extern void stopAllSetWavPreviewAudio();
  if (isNowPlaying) pause(true);
  allOff();
  stopAllSetWavPreviewAudio();
}

FLASHMEM static void sdSerSdLock() {
  sdSerAudioStopForSd();
  AudioNoInterrupts();
}

FLASHMEM static void sdSerSdUnlock() {
  AudioInterrupts();
}

FLASHMEM static void sdSerTouchClient();

FLASHMEM static uint32_t sdSerCrc32Update(uint32_t crc, const uint8_t *data, size_t len) {
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

FLASHMEM static void sdSerReply(const char *msg) {
  Serial.println(msg);
}

FLASHMEM static void sdSerReplyf(const char *fmt, ...) {
  char buf[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Serial.println(buf);
}

FLASHMEM static void sdSerReplyFlush(const char *msg) {
  Serial.println(msg);
  Serial.flush();
}

// Optional diagnostics for a host explicitly issuing PUTTRACE. Normal PUT
// framing stays READY / ACK / OK, with no extra traffic.
FLASHMEM static void sdSerPutCheckpoint(const char *stage, uint32_t offset) {
  if (!sdSerPutTrace) return;
  sdSerReplyf("TRACE %s %lu", stage, (unsigned long)offset);
  Serial.flush();
}

FLASHMEM static void sdSerCmdInfo() {
  sdSerReplyf("OK SDINFO cpu=%lu tempCenti=%d error=%u data=%lu",
              (unsigned long)F_CPU_ACTUAL, (int)(tempmonGetTemp() * 100.0f),
              (unsigned)SD.sdfs.card()->errorCode(),
              (unsigned long)SD.sdfs.card()->errorData());
}

FLASHMEM static bool sdSerReadExact(uint8_t *dst, size_t len, uint32_t timeoutMs) {
  size_t got = 0;
  uint32_t start = millis();
  uint32_t traced = start;
  while (got < len) {
    if (sdSerPutTrace && millis() - traced >= 1000) {
      sdSerReplyf("TRACE WAIT got=%lu len=%lu avail=%d", (unsigned long)got, (unsigned long)len, Serial.available());
      Serial.flush();
      traced = millis();
    }
    int avail = Serial.available();
    if (avail > 0) {
      size_t take = (size_t)avail;
      if (take > len - got) take = len - got;
      if (sdSerPutTrace) sdSerReplyf("TRACE RX %lu %lu", (unsigned long)got, (unsigned long)take);
      // The Teensy core's bulk reader re-enables USB IRQ when it requeues
      // each consumed packet, then keeps updating the receive ring without
      // exclusion. Its queue scheduler also enables global IRQs, defeating
      // an outer interrupt guard. Consume one byte per core call instead:
      // each call finishes its accounting before requeueing a packet.
      int n = 0;
      while ((size_t)n < take) {
        const int b = Serial.read();
        if (b < 0) break;
        dst[got + (size_t)n++] = (uint8_t)b;
      }
      if (sdSerPutTrace) sdSerReplyf("TRACE RXDONE %d", n);
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
FLASHMEM static bool sdSerNormalizePath(const char *in, char *out, size_t outLen) {
  if (!in || !out || outLen < 2) return false;

  while (*in == '/' || *in == '\\') in++;

  if (*in == '\0') {
    out[0] = '/';
    out[1] = '\0';
    return true;
  }

  size_t o = 0;
  while (*in) {
    while (*in == '/' || *in == '\\') ++in;
    const char *seg = in;
    while (*in && *in != '/' && *in != '\\') {
      if ((uint8_t)*in < 32 || (uint8_t)*in == 127) return false;
      ++in;
    }
    const size_t s = in - seg;
    if (!s || (s == 1 && seg[0] == '.')) continue;
    if (s == 2 && seg[0] == '.' && seg[1] == '.') return false;
    if (o > 0) {
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

FLASHMEM static const char *sdSerFsPath(const char *normalized) {
  if (!normalized || normalized[0] == '\0') return "/";
  return normalized;
}

FLASHMEM static void sdSerCmdPing() {
  sdSerReplyf("OK TOERN SD %s PUT_SPACES PUT_TRACE", VERSION);
}

FLASHMEM static void sdSerCmdList(const char *pathArg) {
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

FLASHMEM static void sdSerCmdMv(char *args) {
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

FLASHMEM static void sdSerCmdRm(const char *pathArg) {
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

FLASHMEM static void sdSerCmdMkdir(const char *pathArg) {
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

FLASHMEM static void sdSerCmdPut(char *args) {
  char *pathTok = nullptr;
  uint32_t size = 0, expectCrc = 0;
  if (!toern_sd::parsePutArgs(args, pathTok, size, expectCrc)) {
    sdSerReply("ERR USAGE");
    return;
  }

  char path[128];
  if (!sdSerNormalizePath(pathTok, path, sizeof(path)) || strcmp(path, "/") == 0) {
    sdSerReply("ERR BAD_PATH");
    return;
  }

  if (size > (512ul * 1024ul * 1024ul)) {
    sdSerReply("ERR TOO_LARGE");
    return;
  }

  sdSerAudioStopForSd();
  sdSerPutCheckpoint("OPEN", 0);
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

  // Full USB flow-control block; only used during host-to-SD uploads.
  DMAMEM static uint8_t chunk[SD_SER_CHUNK] __attribute__((aligned(32)));
  uint32_t remaining = size;
  uint32_t crc = 0;
  bool ok = true;

  // Audio is stopped before opening/truncating the file. Keep USB interrupts
  // active during the serial handshake and while waiting for the next block.
  while (remaining > 0) {
    // One flow-control block: host must wait for ACK before sending more.
    // USB CDC ignores baud rate and will otherwise overrun RX during SD writes.
    size_t block = remaining > SD_SER_PUT_BLOCK ? SD_SER_PUT_BLOCK : (size_t)remaining;
    size_t got = 0;
    while (got < block) {
      size_t n = (block - got) > SD_SER_CHUNK ? SD_SER_CHUNK : (block - got);
      sdSerPutCheckpoint("READ", size - remaining);
      if (!sdSerReadExact(chunk, n, SD_SER_IO_TIMEOUT_MS)) {
        ok = false;
        break;
      }
      sdSerPutCheckpoint("WRITE", size - remaining);
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
    sdSerPutCheckpoint("FLUSH", size - remaining);
    AudioNoInterrupts();
    out.flush();
    AudioInterrupts();
    sdSerPutCheckpoint("ACK", size - remaining);
    sdSerReplyFlush("ACK");
  }
  sdSerPutCheckpoint("CLOSE", size - remaining);
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

FLASHMEM static bool sdSerWriteAll(const uint8_t *data, size_t len) {
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

FLASHMEM static bool sdSerWaitAck(uint32_t timeoutMs) {
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

FLASHMEM static void sdSerCmdGet(const char *pathArg) {
  char path[128];
  if (!pathArg || !sdSerNormalizePath(pathArg, path, sizeof(path)) || strcmp(path, "/") == 0) {
    sdSerReply("ERR BAD_PATH");
    return;
  }
  if (!SD.exists(path)) {
    sdSerReply("ERR NOT_FOUND");
    return;
  }

  // Stop SD audio readers before opening the host download stream.
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

FLASHMEM static void sdSerTouchClient() {
  const bool was = sdSerClientConnected;
  sdSerNoteHostPresent();
  if (was) return;
  // First contact on the SD page stops play so the card is safe to touch.
  sdSerAudioStopForSd();
  extern void menuRequestFullRedraw();
  menuRequestFullRedraw();
}

FLASHMEM static char *sdSerSkipSpaces(char *s) {
  while (s && (*s == ' ' || *s == '\t')) s++;
  return s;
}

// Current-pattern RAM image: notes (x then y) + FF FE + float BPM + TPIT pitches.
static const size_t SD_PAT_NOTES = 256u * 16u;
static const size_t SD_PAT_NOTE_BYTES = SD_PAT_NOTES * 4u;
static const size_t SD_PAT_BYTES = SD_PAT_NOTE_BYTES + 2u + 4u + 8u + SD_PAT_NOTES;
static const uint8_t SD_PAT_PITCH_HDR[8] = { 'T', 'P', 'I', 'T', 1, 0x10, 0x00, 0 };
static EXTMEM uint8_t sdPatBuf[SD_PAT_BYTES];

FLASHMEM static void sdPatPack() {
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

FLASHMEM static bool sdPatApply() {
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
    syncBpmEncoderPositions();
    playNoteInterval = 60000000.0 / ((double)SMP.bpm * 4.0);
    playTimer.update(playNoteInterval);
    fillTimer.update(playNoteInterval / 4.0);
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

FLASHMEM static void sdSerCmdGetPat() {
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

FLASHMEM static void sdSerCmdPutPat(char *args) {
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

FLASHMEM static void sdSerHandleLine(char *line) {
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
  } else if (strcasecmp(cmd, "SDINFO") == 0) {
    sdSerCmdInfo();
  } else if (strcasecmp(cmd, "LIST") == 0) {
    sdSerCmdList(args && *args ? args : "/");
  } else if (strcasecmp(cmd, "RM") == 0) {
    sdSerCmdRm(args);
  } else if (strcasecmp(cmd, "MKDIR") == 0) {
    sdSerCmdMkdir(args);
  } else if (strcasecmp(cmd, "MV") == 0 || strcasecmp(cmd, "REN") == 0) {
    if (args && *args) sdSerCmdMv(args);
    else sdSerReply("ERR USAGE");
  } else if (strcasecmp(cmd, "PUT") == 0 || strcasecmp(cmd, "PUTTRACE") == 0) {
    sdSerPutTrace = strcasecmp(cmd, "PUTTRACE") == 0;
    if (args && *args) sdSerCmdPut(args);
    else sdSerReply("ERR USAGE");
    sdSerPutTrace = false;
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
    sdSerReplyf("OK TOERN SD %s PUT_SPACES PUT_TRACE", VERSION);
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
FLASHMEM static void sdSerHandleOffPage(char *line) {
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
    sdSerReplyf("OK TOERN SD %s PUT_SPACES PUT_TRACE", VERSION);
    return;
  }

  if (strcasecmp(cmd, "SDINFO") == 0) {
    sdSerNoteHostPresent();
    sdSerCmdInfo();
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
  } else if (strcasecmp(cmd, "PUT") == 0 || strcasecmp(cmd, "PUTTRACE") == 0) {
    sdSerNoteHostPresent();
    sdSerPutTrace = strcasecmp(cmd, "PUTTRACE") == 0;
    if (args && *args) sdSerCmdPut(args);
    else sdSerReply("ERR USAGE");
    sdSerPutTrace = false;
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
    sdSerReply("ERR UNKNOWN");
  }
}

void sdSerialServerPollNeedSdHint() {
  // Cable only: no program has the port open. Do not read or wait.
  if (!Serial) return;
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
