// FN is an input modifier. It never changes an encoder's limits or step size.
static int8_t fnDirection = 0;
static unsigned long fnDirectionUntil = 0;

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
  if (fnHeld) noteUserActivity();
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
  if (!event.fn || event.encoder != 0 || event.delta == 0) return false;
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
  if (currentMode == &touchyMode || currentMode == &recordMode || (!fnHeld && !pendingRestore[index])) return false;
  const int previous = (currentMode == &subpatternMode && muteModeActive && index == 0)
      ? 0 : (int)currentMode->pos[index];
  if (fnHeld) {
    const uint8_t status = Encoder[index].readStatus();
    const uint8_t rotationBits = i2cEncoderLibV2::RINC | i2cEncoderLibV2::RDEC |
        i2cEncoderLibV2::RMAX | i2cEncoderLibV2::RMIN;
    if (status != 0xFF && (status & rotationBits)) {
      int delta = rawValue - previous;
      if (delta == 0) {
        const bool right = status & (i2cEncoderLibV2::RINC | i2cEncoderLibV2::RMAX);
        const bool left = status & (i2cEncoderLibV2::RDEC | i2cEncoderLibV2::RMIN);
        if (right != left) delta = right ? 1 : -1;
      }
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
