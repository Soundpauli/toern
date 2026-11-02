#if TOERN_REMOTE_CONTROL
// Semantic readback uses the same menu/slider definitions as the panel.
// It is read-only: no encoder polling, screen changes, or parameter writes.
FLASHMEM void remotePrintUiState() {
  const unsigned ch = GLOB.currentChannel;
  const unsigned step = cursorNoteStep();
  Serial.printf("\"single_mode\":%s,\"demo_active\":%s,\"filter_page\":%u,\"filter_page_count\":%u,",
      GLOB.singleMode ? "true" : "false", demoAnimationActive ? "true" : "false", ch < NUM_CHANNELS ? filterPage[ch] + 1u : 0u,
      ch < NUM_CHANNELS ? filterPageCount[ch] : 0u);
  if (step >= 1 && step < maxlen && GLOB.y >= 1 && GLOB.y <= maxY) {
    const Note &cell = note[step][GLOB.y];
    Serial.printf("\"cell\":{\"step\":%u,\"row\":%u,\"voice\":%u,\"velocity\":%u,\"probability\":%u,\"condition\":%u}",
        step, GLOB.y, cell.channel, cell.velocity, cell.probability, cell.condition);
  } else Serial.print("\"cell\":null");
}

FLASHMEM void remoteReplyFilters(unsigned long id, int ch) {
  if (ch < 1 || ch >= NUM_CHANNELS || filterPageCount[ch] == 0) {
    remoteReplyError(id, "VOICE"); return;
  }
  remoteReplyStart(id);
  Serial.printf("\"ok\":true,\"voice\":%d,\"active_page\":%u,\"pages\":[", ch, filterPage[ch] + 1u);
  for (unsigned p = 0; p < filterPageCount[ch] && p < FILTER_PAGE_CAP; ++p) {
    Serial.printf("%s{\"page\":%u,\"sliders\":[", p ? "," : "", p + 1);
    bool comma = false;
    for (unsigned slot = 0; slot < 4; ++slot) {
      const auto &d = sliderDef[ch][p][slot];
      if (d.arr == ARR_NONE || d.idx < 0) continue;
      Serial.printf("%s{\"encoder\":%u,\"name\":\"%s\",\"value\":%u,\"max\":%u,\"default\":%d,\"array\":%u,\"index\":%d}",
          comma ? "," : "", slot + 1, d.name, readSetting(d.arr, d.idx, ch),
          d.maxValue, defaultSettingValue(ch, d.arr, d.idx), (unsigned)d.arr, (int)d.idx);
      comma = true;
    }
    Serial.print("]}");
  }
  Serial.println("]}");
}

FLASHMEM void remoteReplyMenus(unsigned long id) {
  MenuPage *pages = menuPages;
  int count = MENU_PAGES_COUNT, index = currentMenuPage;
  const char *group = "ROOT";
  if (inLookSubmenu) { pages = lookPages; count = LOOK_PAGES_COUNT; index = currentLookPage; group = "SETT"; }
  else if (inRecsSubmenu) { pages = recsPages; count = RECS_PAGES_COUNT; index = currentRecsPage; group = "REC"; }
  else if (inMidiSubmenu) { pages = midiPages; count = MIDI_PAGES_COUNT; index = currentMidiPage; group = "MIDI"; }
  else if (inVolSubmenu) { pages = volPages; count = VOL_PAGES_COUNT; index = currentVolPage; group = "VOL"; }
  else if (inEtcSubmenu) {
    pages = activeSystemPages(); count = activeSystemPageCount(); index = currentEtcPage;
    group = currentSystemMenuGroup == SystemMenuGroup::Mode ? "MODE" :
        currentSystemMenuGroup == SystemMenuGroup::Advanced ? "ADV" : "ETC";
  }
  index = constrain(index, 0, count - 1);
  remoteReplyStart(id);
  Serial.printf("\"ok\":true,\"active\":%s,\"group\":\"%s\",\"index\":%d,\"name\":\"%s\",\"setting\":%d,\"items\":[",
      currentMode == &menu ? "true" : "false", group, index, pages[index].name, pages[index].mainSetting);
  for (int i = 0; i < count; ++i) Serial.printf("%s{\"index\":%d,\"name\":\"%s\",\"setting\":%d}",
      i ? "," : "", i, pages[i].name, pages[i].mainSetting);
  Serial.println("]}");
}

#endif
