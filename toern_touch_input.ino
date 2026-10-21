// Shared state survives screen changes; a held pad never becomes a fresh touch.
bool readDebouncedTouch(int index) {
  static TouchDebouncer pads[5];
  if (index < 0 || index >= 5) return false;
  static const int pins[5] = {SWITCH_1, SWITCH_2, SWITCH_3, SWITCH_4, SWITCH_5};
  bool raw;
  // Remote panel: grounded buttons on J12/J14. Cap pads false-trigger under EMI.
  if (exttouch) {
    if (index == 0) raw = digitalReadFast(BUTTON_A) == LOW;       // SINGLE
    else if (index == 1) raw = digitalReadFast(BUTTON_B) == LOW;  // MENU
    else if (index == 2) raw = digitalReadFast(BUTTON_C) == LOW;  // REC / touch3
    else if (index == 3) raw = digitalReadFast(BUTTON_D) == LOW;  // touch4
    else raw = false;  // touch5 / FN — no external button
  } else {
    raw = fastTouchRead(pins[index]) > touchThreshold;
  }
  return pads[index].update(raw, (uint32_t)millis());
}
