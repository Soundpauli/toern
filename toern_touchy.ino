// Five-pad instrument. Sample playback, loading, filters and voice gain all use
// the existing paths; only this screen and its input mapping are new.
static const CRGB touchyPadColors[5] = {
  CRGB(255, 64, 64), CRGB(255, 170, 32), CRGB(64, 255, 128),
  CRGB(64, 128, 255), CRGB(220, 64, 255)
};
static uint8_t touchyLastVoice = 1;
static uint8_t touchyVelocity[6] = {100,100,100,100,100,100};
static unsigned int touchySavedVolume[6];
static bool touchyRecordHeld = false;

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
  return true; // The hold that opened a screen must not activate that screen.
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
  static bool latched[5] = {};
  static bool releasing[5] = {};
  static unsigned long releaseSince[5] = {};
  const int raw[5] = {
    exttouch ? (readTouch1Pressed() ? touchThreshold + 1 : 0) : fastTouchRead(SWITCH_1),
    exttouch ? (readTouch2Pressed() ? touchThreshold + 1 : 0) : fastTouchRead(SWITCH_2),
    fastTouchRead(SWITCH_3), fastTouchRead(SWITCH_4), fastTouchRead(SWITCH_5)
  };
  const unsigned long now = millis();
  const int releaseThreshold = touchThreshold * 7 / 10;
  for (int i = 0; i < 5; i++) {
    if (!latched[i] && raw[i] > touchThreshold) {
      latched[i] = true;
      releasing[i] = false;
      touchyLastVoice = i + 1;
      touchyTintActive = true;
      selectTouchyVoice(touchyLastVoice);
      noteUserActivity();
      SMP.channelVol[touchyLastVoice] = 16;
      applyVoiceAmpGainFromVol(touchyLastVoice, 16);
      triggerCurrentVoiceAudition(basePitchRowForChannel(touchyLastVoice), touchyVelocity[touchyLastVoice]);
    } else if (latched[i]) {
      if (raw[i] < releaseThreshold) {
        if (!releasing[i]) { releasing[i] = true; releaseSince[i] = now; }
        if (now - releaseSince[i] >= 60) { latched[i] = false; releasing[i] = false; }
      } else {
        releasing[i] = false; // A noisy dip while held never rearms the pad.
      }
    }
    if (i < 4) touchState[i] = lastTouchState[i] = latched[i];
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
        // Browser selection must not change the last pad-selected voice.
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
  const float x0 = (int)maxX - 20 - ((int)maxX / 5) * (1.0f - cosf(now * PI / 3000.0f)) * .5f;
  const float top = 14.0f + .65f * sinf(now * PI / 1800.0f);
  CRGB tint(170,62,105);
  if (touchyTintActive) {
    const CRGB pad = touchyPadColors[touchyLastVoice - 1];
    tint = CRGB(pad.r * 3 / 5, pad.g * 3 / 5, pad.b * 3 / 5);
  }
  // Reference sprite, mirrored to face right: 21 wide by 11 high.
  static const char *sprite[11] = {
    "     BBBBBBBBBBBB    ",
    "    BCCCCCCCCCCCCB   ",
    "    BCEEFEEFDEEECB D ",
    "    BCEFEFEDGDFECBDGD",
    " DD BCEEEEEDGGDDDDGGD",
    "DGGDBCEEEEEDGGGGGGGGD",
    " DD BCEEFEEDGKJGGKJGD",
    "    BCFEEEEDEJJGGJJED",
    "   DBCCCCCCCDGGGGGGD ",
    "  DGGBBBBBBBDDDDDDD  ",
    "   DD DD     D  DD   "
  };
  static const CRGB rainbow[6] = {
    CRGB(215,2,29), CRGB(255,144,36), CRGB(255,255,0),
    CRGB(65,255,76), CRGB(0,191,255), CRGB(212,38,255)
  };
  char animated[11][21];
  memset(animated, ' ', sizeof(animated));
  for (int y = 0; y < 11; y++) for (int x = 0; x < 21; x++) {
    if (sprite[y][x] == ' ') continue;
    const int px = x + (y == 10 ? phase % 2 : 0);
    const int py = y - (x < 4 && y < 9 ? phase % 2 : 0);
    if (px < 21 && py >= 0) animated[py][px] = sprite[y][x];
  }
  for (int x = 1; x <= (int)maxX; x++) {
    const float wave = .65f * sinf(x * .6f - now * PI / 600.0f);
    for (int y = 1; y <= 16; y++) {
      float background[3] = {};
      if (x < x0 + 5) {
        const float ry = top - 4 + wave - y;
        const int row = floorf(ry);
        const float f = ry - row;
        for (int i = 0; i < 2; i++) if (row + i >= 0 && row + i < 6) {
          const float weight = i ? f : 1 - f;
          for (int c = 0; c < 3; c++) background[c] += rainbow[row + i][c] * weight;
        }
      }
      const float sx = x - x0, sy = top - y;
      const int ix = floorf(sx), iy = floorf(sy);
      const float fx = sx - ix, fy = sy - iy;
      float foreground[3] = {}, alpha = 0;
      for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) {
        if (ix + dx < 0 || ix + dx >= 21 || iy + dy < 0 || iy + dy >= 11) continue;
        const char pixel = animated[iy + dy][ix + dx];
        if (pixel == ' ') continue;
        const float weight = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy);
        CRGB ink;
        switch (pixel) {
          case 'B': ink = CRGB(72,30,5); break;
          case 'C': ink = CRGB(170,86,22); break;
          case 'D': case 'J': ink = CRGB::Black; break;
          case 'E': ink = tint; break;
          case 'F': ink = CRGB(255,4,94); break;
          case 'G': ink = CRGB(48,48,56); break;
          case 'K': ink = CRGB(200,195,170); break;
          default: continue;
        }
        alpha += weight;
        for (int c = 0; c < 3; c++) foreground[c] += ink[c] * weight;
      }
      CRGB color;
      for (int c = 0; c < 3; c++) color[c] = (uint8_t)lroundf(background[c] * (1 - alpha) + foreground[c]);
      light(x, y, color);
    }
  }
  FastLEDshow();
}
