// Shared state survives screen changes; a held pad never becomes a fresh touch.
bool readDebouncedTouch(int index) {
  static TouchDebouncer pads[5];
  if (index < 0 || index >= 5) return false;
  static const int pins[5] = {SWITCH_1, SWITCH_2, SWITCH_3, SWITCH_4, SWITCH_5};
  bool raw;
  if (exttouch && index == 0) raw = digitalReadFast(BUTTON_A) == LOW;
  else if (exttouch && index == 1) raw = digitalReadFast(BUTTON_B) == LOW;
  else raw = fastTouchRead(pins[index]) > touchThreshold;
  return pads[index].update(raw, (uint32_t)millis());
}
