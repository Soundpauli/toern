#if TOERN_REMOTE_CONTROL
// REMOTE v1: ASCII requests, prefixed JSON replies. All work stays in loop().
static ToernRemoteInput remoteInput;
static bool remoteTurnFault = false;
static uint32_t remoteInputRecoveries = 0;
static DMAMEM CRGB remoteDisplayFrame[NUM_LEDS];
static bool remoteDisplayValid = false;
static bool remoteDisplayRotated = false;

void remoteCaptureDisplay() {
  if (!remoteInput.live) return;
  memcpy(remoteDisplayFrame, leds, sizeof(remoteDisplayFrame));
  remoteDisplayRotated = ledModulesRotated;
  remoteDisplayValid = true;
}

static void remoteButtonTransition(int index, bool before) {
  const bool after = remoteInput.held(index);
  if (before == after) return;
  if (after) {
    pressed[index] = !fnHeld || currentMode == &touchyMode;
    encoder_button_pushed(&Encoder[index], index);
  } else {
    encoder_button_released(&Encoder[index], index);
    pressed[index] = false;
  }
}

void remotePhysicalButton(int index, bool down) {
  const bool before = remoteInput.held(index);
  const bool previousPhysical = remoteInput.physical[index];
  remoteInput.physical[index] = down;
  // Hardware callbacks are edges, not repeated level samples. A new push must
  // recover a missed physical release just as the original callback did; only
  // an actual remote hold should suppress retriggering the combined button.
  if (down && previousPhysical && !remoteInput.remote[index]) {
    pressed[index] = !fnHeld || currentMode == &touchyMode;
    encoder_button_pushed(&Encoder[index], index);
  } else remoteButtonTransition(index, before);
}

bool remoteTouchHeld(int index) {
  return index >= 0 && index < 5 && remoteInput.touches[index];
}

void remoteReleaseInputs() {
  for (int i = 0; i < 4; ++i) {
    const bool before = remoteInput.held(i);
    remoteInput.remote[i] = false;
    remoteInput.turns[i] = 0;
    remoteButtonTransition(i, before);
  }
  for (int i = 0; i < 5; ++i) remoteInput.touches[i] = false;
}

void remoteEndSession() {
  remoteReleaseInputs();
  remoteInput.live = false;
  remoteDisplayValid = false;
}

void remoteService() {
  if (!remoteInput.live) return;
  if (!Serial || remoteInput.expired(millis())) { remoteEndSession(); return; }
  // The existing button FSM can recover/reset after a lost I2C release or a
  // stuck hold. Retire its stale physical shadow too; otherwise managed MCP
  // gestures stay blocked and the next push is incorrectly deduplicated.
  for (int i = 0; i < 4; ++i) {
    if (remoteInput.physical[i] && !remoteInput.remote[i] && !isPressed[i] && buttonState[i] == IDLE) {
      remoteInput.physical[i] = false;
      ++remoteInputRecoveries;
    }
  }
}

void remoteApplyEncoderTurn(int index, int &raw, bool allowWrite, bool fnConsumed) {
  if (!allowWrite || !remoteInput.turns[index]) return;
  const int delta = remoteInput.turns[index];
  remoteInput.turns[index] = 0;
  if (fnConsumed) {
    if (fnHeld) {
      const EncoderTurnEvent event = {index, delta, true};
      dispatchFnTurn(event);
    }
  } else {
    const int32_t lo = Encoder[index].readMin();
    const int32_t hi = Encoder[index].readMax();
    const int32_t step = Encoder[index].readStep();
    if (lo > hi || step < 1 || step > 10000) { remoteTurnFault = true; return; }
    const int64_t value = (int64_t)raw + (int64_t)delta * step;
    raw = (int)max((int64_t)lo, min((int64_t)hi, value));
    Encoder[index].writeCounter((int32_t)raw);
  }
  noteUserActivity();
}

FLASHMEM static void remoteReplyStart(unsigned long id) {
  Serial.printf("REMOTE {\"id\":%lu,", id);
}

FLASHMEM static void remoteReplyError(unsigned long id, const char *error) {
  remoteReplyStart(id);
  Serial.printf("\"ok\":false,\"error\":\"%s\"}\n", error);
}

FLASHMEM static void remoteReplyState(unsigned long id) {
  remoteReplyStart(id);
  // Smallest-code Teensy printf omits floats; emit BPM with integer formatting.
  const unsigned long bpmMilli = (unsigned long)lroundf(SMP.bpm * 1000.0f);
  Serial.printf("\"ok\":true,\"protocol\":1,\"firmware\":\"%s\","
      "\"mode\":\"%s\",\"menu_setting\":%d,\"cursor\":{\"x\":%u,\"y\":%u,\"page\":%u},"
      "\"voice\":%u,\"playing\":%s,\"bpm\":%lu.%03lu,\"kit\":%u,\"browsed_kit\":%u,"
      "\"fn\":%s,\"pending_turns\":%s,\"turn_error\":%s,\"positions\":[%u,%u,%u,%u],\"buttons\":[",
      VERSION, currentMode->name.c_str(), currentMode == &menu ? getCurrentMenuMainSetting() : -1,
      GLOB.x, GLOB.y, GLOB.page, GLOB.currentChannel, isNowPlaying ? "true" : "false",
      bpmMilli / 1000u, bpmMilli % 1000u, samplePackID, SMP.pack, fnHeld ? "true" : "false",
      remoteInput.pending() ? "true" : "false", remoteTurnFault ? "\"ENCODER_IO\"" : "null", currentMode->pos[0], currentMode->pos[1],
      currentMode->pos[2], currentMode->pos[3]);
  for (int i = 0; i < 4; ++i) Serial.printf("%s%s", i ? "," : "", remoteInput.held(i) ? "true" : "false");
  Serial.print("],\"remote_touches\":[");
  for (int i = 0; i < 5; ++i) Serial.printf("%s%s", i ? "," : "", remoteInput.touches[i] ? "true" : "false");
  Serial.print("],");
  remotePrintUiState();
  Serial.println("}");
}

FLASHMEM static void remoteReplyDisplay(unsigned long id) {
  if (!remoteDisplayValid) { remoteReplyError(id, "NO_FRAME"); return; }
  remoteReplyStart(id);
  // Logical coordinates: x left to right, rows top to bottom (device y descends).
  // Read the rendered LED buffer, including software dimming, without redrawing.
  Serial.printf("\"ok\":true,\"width\":%u,\"height\":%u,\"rgb\":\"", maxX, maxY);
  char row[LED_MODULES * 16 * 6 + 1];
  static const char hex[] = "0123456789abcdef";
  for (unsigned rowY = maxY; rowY >= 1; --rowY) {
    unsigned out = 0;
    for (unsigned x = 1; x <= maxX; ++x) {
      const unsigned panel = (x - 1) / MATRIX_WIDTH;
      unsigned mx = (x - 1) % MATRIX_WIDTH + 1, my = rowY;
      if (remoteDisplayRotated) { mx = MATRIX_WIDTH + 1 - rowY; my = (x - 1) % MATRIX_WIDTH + 1; }
      const unsigned idx = panel * 256 + MATRIX_WIDTH * (my - 1)
          + (my % 2 ? mx - 1 : MATRIX_WIDTH - mx);
      const CRGB c = remoteDisplayFrame[idx];
      for (uint8_t v : {c.r, c.g, c.b}) { row[out++] = hex[v >> 4]; row[out++] = hex[v & 15]; }
    }
    row[out] = 0;
    Serial.print(row);
  }
  Serial.println("\"}");
}

FLASHMEM void remoteHandleCommand(char *args) {
  // Strict token counts and bounded integers: no partial/overflowing commands.
  char *tokens[6] = {};
  int count = 0;
  char *save = nullptr;
  for (char *p = strtok_r(args, " \t", &save); p; p = strtok_r(nullptr, " \t", &save)) {
    if (count == 6) { remoteReplyError(0, "USAGE"); return; }
    tokens[count++] = p;
  }
  long values[3] = {};
  if (count < 2) { remoteReplyError(0, "USAGE"); return; }
  int slots[] = {0, 2, 3};
  for (int i = 0; i < 3; ++i) {
    if (slots[i] >= count) continue;
    char *end;
    errno = 0;
    values[i] = strtol(tokens[slots[i]], &end, 10);
    if (errno || !tokens[slots[i]][0] || *end) { remoteReplyError(0, "NUMBER"); return; }
  }
  if (values[0] < 1 || values[0] > 2147483647L) { remoteReplyError(0, "ID"); return; }
  const unsigned long id = (unsigned long)values[0];
  const char *op = tokens[1];
  // HELLO is the only operation that starts a session. An open USB serial
  // connection (for SD transfer, MIDI tools, etc.) never activates MCP work.
  remoteService();
  if (count == 2 && !strcmp(op, "HELLO")) {
    if (!remoteInput.live) {
      for (int i = 0; i < 4; ++i) remoteInput.physical[i] = isPressed[i];
      remoteDisplayValid = false;
      remoteTurnFault = false;
    }
    remoteInput.renew(millis());
    remoteReplyState(id);
    return;
  }
  if (!remoteInput.live) { remoteReplyError(id, "NO_SESSION"); return; }
  if (count == 2 && !strcmp(op, "BYE")) {
    remoteEndSession();
    remoteReplyStart(id);
    Serial.println("\"ok\":true}");
    return;
  }
  if (count == 2 && !strcmp(op, "PERF")) {
    remoteInput.renew(millis());
    remoteReplyPerformance(id);
    return;
  }
  if (count == 2 && !strcmp(op, "MENUS")) {
    remoteInput.renew(millis());
    remoteReplyMenus(id);
    return;
  }
  if ((count == 2 || count == 3) && !strcmp(op, "FILTERS")) {
    const long voice = count == 3 ? values[1] : (long)GLOB.currentChannel;
    if (voice < 1 || voice > 15) { remoteReplyError(id, "VOICE"); return; }
    remoteInput.renew(millis());
    remoteReplyFilters(id, (int)voice);
    return;
  }
  if (count == 2 && (!strcmp(op, "STATE") || !strcmp(op, "KEEPALIVE"))) {
    remoteInput.renew(millis());
    if (!strcmp(op, "KEEPALIVE")) {
      remoteReplyStart(id);
      Serial.println("\"ok\":true}");
    } else remoteReplyState(id);
  } else if (count == 2 && !strcmp(op, "DISPLAY")) {
    remoteInput.renew(millis());
    remoteReplyDisplay(id);
  } else if (count == 2 && !strcmp(op, "RELEASE")) {
    remoteReleaseInputs();
    remoteInput.renew(millis());
    remoteReplyState(id);
  } else if (count == 4 && !strcmp(op, "TURN")) {
    if (values[1] < 1 || values[1] > 4 || !values[2] || values[2] < -64 || values[2] > 64) {
      remoteReplyError(id, "RANGE"); return;
    }
    if (fastRecordActive || isRecording) { remoteReplyError(id, "RECORDING"); return; }
    if (!remoteInput.enqueue((int)values[1] - 1, (int)values[2])) { remoteReplyError(id, "BUSY"); return; }
    remoteTurnFault = false;
    remoteInput.renew(millis());
    noteUserActivity();
    remoteReplyState(id);
  } else if (count == 4 && (!strcmp(op, "BUTTON") || !strcmp(op, "TOUCH"))) {
    const bool button = !strcmp(op, "BUTTON");
    if (values[1] < 1 || values[1] > (button ? 4 : 5) || values[2] < 0 || values[2] > 1) {
      remoteReplyError(id, "RANGE"); return;
    }
    const int index = (int)values[1] - 1;
    remoteInput.renew(millis());
    if (button) {
      const bool before = remoteInput.held(index);
      remoteInput.remote[index] = values[2];
      remoteButtonTransition(index, before);
    } else remoteInput.touches[index] = values[2];
    noteUserActivity();
    remoteReplyState(id);
  } else remoteReplyError(id, "USAGE");
}

#else
// Keep a small explicit handshake error so a host can identify a disabled build.
// The SD serial dispatcher and its commands remain available.
FLASHMEM void remoteHandleCommand(char *args) {
  const unsigned long id = strtoul(args, nullptr, 10);
  Serial.printf("REMOTE {\"id\":%lu,\"ok\":false,\"error\":\"DISABLED\"}\n", id);
}
#endif
