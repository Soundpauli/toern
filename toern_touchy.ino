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
static bool touchyPrevious[5] = {};

void onTouchyModeEnter() {
  // A held FN/exit pad must be released before it can play a TOUCHY voice.
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
  const int x0 = (int)lroundf((int)maxX - 20 - ((int)maxX / 5) * (1.0f - cosf(now * PI / 3000.0f)) * .5f);
  const int top = (int)lroundf(14.0f + .65f * sinf(now * PI / 1800.0f));
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
  // Tail root stays attached; only the bend changes through up/level/down/level.
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
