// FN is an input modifier. It never changes an encoder's limits or step size.
static int8_t fnDirection = 0;
static unsigned long fnDirectionUntil = 0;
// Release is latched through the encoder read so FN+encoder 2 cannot fall
// through into the page (CTRL page) or volume (CTRL vol) handler.
static bool fnReleaseLatch = false;
static bool fnReleaseWork = false;

static void updateFnState() {
  const bool wasHeld = fnHeld;
  const bool fillChord = pollTouchFillChord();
  fnHeld = !fillChord && currentMode != &touchyMode && currentMode != &recordMode && readDebouncedTouch(4);
  if (fnHeld && !wasHeld && currentMode == &filterMode && GLOB.currentChannel >= 1
      && GLOB.currentChannel <= 8 && filterPage[GLOB.currentChannel] == 2) {
    setSampleGate(GLOB.currentChannel, !sampleGateEnabled(GLOB.currentChannel));
    sampleGateOverlayUntil = millis() + 800;
    lastChangedEncoder = -1;
    showFilterPages(GLOB.currentChannel);
  }
  if (fillChord) subpatternSelector = false;
  if (wasHeld && !fnHeld && currentMode == &draw) {
    fnReleaseLatch = true;
    fnReleaseWork = true;
  } else if (currentMode == &touchyMode || currentMode == &recordMode) {
    fnReleaseLatch = false;
    fnReleaseWork = false;
  }
  if (fnHeld) noteUserActivity();
}

static bool fnEncoder2Held() {
  return fnHeld || fnReleaseLatch || subpatternSelector;
}

static bool finishFnSubpatternRelease(bool allowWrite) {
  bool switched = false;
  if (fnReleaseLatch && allowWrite && currentMode) {
    Encoder[1].writeCounter((int32_t)currentMode->pos[1]);
    Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
  }
  if (fnReleaseWork) {
    switched = subpatternFnRelease();
    fnReleaseWork = false;
  }
  if (fnReleaseLatch && allowWrite && currentMode) {
    Encoder[1].writeCounter((int32_t)currentMode->pos[1]);
    Encoder[2].writeCounter((int32_t)currentMode->pos[2]);
  }
  if (allowWrite) fnReleaseLatch = false;
  return switched;
}

// Keep the physical button FSM unchanged. Modified events have their own codes:
// 1=short, 2=long, 9=release; FN: 4=short, 3=long, 8=release.
static uint8_t fnButtonEvent(int index, uint8_t physicalState) {
  if (currentMode == &touchyMode || currentMode == &recordMode) return physicalState;
  if (physicalState == 2 && fnHeld) fnButtonHeld[index] = true;
  const bool modified = fnButtonHeld[index] || fnHeld;
  uint8_t event = physicalState;
  if (modified) {
    if (physicalState == 1) event = 4;
    else if (physicalState == 2) event = 3;
    else if (physicalState == 9) event = 8;
  }
  if (physicalState == 1 || physicalState == 9) fnButtonHeld[index] = false;
  return event;
}

static bool dispatchFnTurn(const EncoderTurnEvent &event) {
  if (!event.fn || event.delta == 0) return false;
  if (event.encoder == 1 && currentMode == &draw) return subpatternTurn(event.delta);
  if (event.encoder == 2 && currentMode == &draw && subpatternSelector) return subpatternPageTurn(event.delta);
  if (event.encoder != 0) return false;
  const uint8_t channel = GLOB.currentChannel;
  if (fastRecordActive || isRecording || channel < 1 || channel > 8 || loadedSampleLen[channel] <= 1) return false;
  fnDirection = event.delta < 0 ? -1 : 1;
  applyChannelDirection(channel, fnDirection);
  fnDirectionUntil = millis() + 350;
  return true;
}

static bool handleFnEncoder(int index, int &rawValue, bool allowWrite) {
  static Mode *pendingRestore[NUM_ENCODERS] = {};
  if (pendingRestore[index] && pendingRestore[index] != currentMode) pendingRestore[index] = nullptr;
  const bool consumeFn = fnHeld || fnReleaseLatch;
  if (currentMode == &touchyMode || currentMode == &recordMode || (!consumeFn && !pendingRestore[index])) return false;
  const int previous = (currentMode == &subpatternMode && muteModeActive && index == 0)
      ? 0 : (int)currentMode->pos[index];
  if (fnHeld) {
    const uint8_t status = Encoder[index].readStatus();
    const uint8_t rotationBits = i2cEncoderLibV2::RINC | i2cEncoderLibV2::RDEC |
        i2cEncoderLibV2::RMAX | i2cEncoderLibV2::RMIN;
    int delta = rawValue - previous;
    if (status != 0xFF && (status & rotationBits)) {
      if (delta == 0) {
        const bool right = status & (i2cEncoderLibV2::RINC | i2cEncoderLibV2::RMAX);
        const bool left = status & (i2cEncoderLibV2::RDEC | i2cEncoderLibV2::RMIN);
        if (right != left) delta = right ? 1 : -1;
      }
      const EncoderTurnEvent event = {index, delta, true};
      dispatchFnTurn(event);
    } else if ((index == 1 || (index == 2 && subpatternSelector)) && delta != 0) {
      // Do not rewind a real counter change without also moving the subpattern / page.
      const EncoderTurnEvent event = {index, delta, true};
      dispatchFnTurn(event);
    }
  }
  // Consume ALL FN input, including unmapped actions and invalid sample voices.
  // Rewind only the counter; retain the mode's limits, step and logical value.
  if (rawValue != previous) {
    if (allowWrite) {
      Encoder[index].writeCounter((int32_t)previous);
      pendingRestore[index] = nullptr;
    } else pendingRestore[index] = currentMode;
  } else pendingRestore[index] = nullptr;
  rawValue = currentMode->pos[index];
  return true;
}
