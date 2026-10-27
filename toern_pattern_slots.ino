// A-E file subpatterns. FN+encoder 2 only moves the letter. The visible page
// is read immediately, before FN is released. Playback keeps the previous
// slot until its loop ends, the same wait as PMOD:NEXT, and never saves to SD
// while playing. When paused, switching slots saves the outgoing slot.
static uint8_t subpatternVoiceLen[16];
static uint16_t subpatternVoicePages[16], subpatternVoiceRows[16];
static bool subpatternHasNotes[maxPages + 1];
static unsigned subpatternLastPage = 1;
static uint8_t subpatternLongestVoice = 1;
static unsigned long subpatternErrorUntil = 0;
static uint16_t subpatternSlotPages[toern_pattern_slots::count];
static uint32_t subpatternSlotKnown = 0;
static bool subpatternDeleteLegacy = false;
static bool subpatternSwitchArmed = false;
static bool subpatternStagedNew = false;
static bool subpatternGridReady = false;
static uint8_t subpatternGridSlot = 255;
static uint8_t subpatternTurnGen = 0, subpatternLoadGen = 0;
static bool subpatternViewOn = false;
static bool subpatternViewDirty = false;
static uint8_t subpatternViewSlot = 0;
static unsigned subpatternViewPage = 1;
static unsigned subpatternSelectedPage = 0;
static DMAMEM Note subpatternView[33][maxY + 1];

FLASHMEM static void rememberGridPages(uint8_t slot, const Note (*grid)[maxY + 1]) {
  if (slot >= toern_pattern_slots::count) return;
  uint16_t mask = 0;
  const unsigned pages = (unsigned)effectivePageCount();
  const unsigned maxP = pages < 16 ? pages : 16;
  for (unsigned x = 1; x < maxlen; ++x) {
    const unsigned p = maxX > 0 ? ((x - 1) / maxX + 1) : 1u;
    if (p < 1 || p > maxP) continue;
    for (unsigned y = 1; y <= maxY; ++y) {
      if (grid[x][y].channel) { mask |= (uint16_t)(1u << (p - 1)); break; }
    }
  }
  subpatternSlotPages[slot] = mask;
  subpatternSlotKnown |= 1ul << slot;
  subpatternAvailable |= 1ul << slot;
}

FLASHMEM static void rememberSlotPages(uint8_t slot) {
  rememberGridPages(slot, note);
}

FLASHMEM static void subpatternCancelPreview() {
  subpatternViewOn = false;
  subpatternViewDirty = false;
  subpatternSwitchArmed = false;
  subpatternGridReady = false;
  subpatternStagedNew = false;
  subpatternGridSlot = 255;
  subpatternSelectedPage = 0;
}

FLASHMEM static void resetFileSubpattern() {
  subpatternReady = false;
  subpatternApplied = false;
  subpatternSelector = false;
  subpatternFile = 0;
  subpatternSlot = 0;
  subpatternLettered = false;
  subpatternSaveNew = false;
  subpatternOldSavePending = false;
  subpatternDeleteLegacy = false;
  subpatternSlotKnown = 0;
  subpatternTurnGen = subpatternLoadGen = 0;
  memset(subpatternSlotPages, 0, sizeof(subpatternSlotPages));
  subpatternCancelPreview();
}

FLASHMEM static void patternDestination(char *out, size_t size, unsigned file) {
  const bool active = file == subpatternFile;
  const uint8_t slot = active ? (subpatternSavingPrevious ? subpatternPreviousSlot : subpatternSlot) : 0;
  // The open pattern already knows if it is a lettered family. Another file
  // checks only its A name — never B-E, temps, or backups.
  bool lettered = active && subpatternLettered;
  if (!active) {
    char a[24];
    lettered = toern_pattern_slots::path(a, sizeof(a), file, 0, true) && SD.exists(a);
  }
  if (!toern_pattern_slots::path(out, size, file, slot, lettered)) out[0] = 0;
}

// Open only when this pattern actually has a lettered family (N-A.txt exists)
// or the user is reading slot A from legacy N.txt. An unpromoted pattern's B-E
// never open old leftover N-B..N-E files on the card.
FLASHMEM static File openSubpatternFile(unsigned file, uint8_t slot) {
  if (file < 1 || file > 999 || slot >= toern_pattern_slots::count) return File();
  char path[24];
  if (slot == 0) {
    if (subpatternLettered && toern_pattern_slots::path(path, sizeof(path), file, 0, true)) {
      File f = SD.open(path, FILE_READ);
      if (f) return f;
    }
    if (toern_pattern_slots::path(path, sizeof(path), file, 0, false))
      return SD.open(path, FILE_READ);
    return File();
  }
  if (!subpatternLettered) return File();
  if (!toern_pattern_slots::path(path, sizeof(path), file, slot, true)) return File();
  return SD.open(path, FILE_READ);
}

FLASHMEM static void subpatternClearView() {
  for (unsigned x = 1; x <= 32; ++x)
    for (unsigned y = 1; y <= maxY; ++y)
      subpatternView[x][y] = {0, defaultVelocity, 100, 1, NOTE_MIDI_PITCH_NONE};
}

FLASHMEM static uint16_t subpatternSlotFilledMask(uint8_t slot) {
  if (slot >= toern_pattern_slots::count) return 0;
  const unsigned pages = (unsigned)effectivePageCount();
  const unsigned maxP = pages < 16 ? pages : 16;
  if (slot == subpatternSlot) {
    uint16_t mask = 0;
    for (unsigned p = 1; p <= maxP; ++p)
      if (hasNotes[p]) mask |= (uint16_t)(1u << (p - 1));
    subpatternSlotPages[slot] = mask;
    subpatternSlotKnown |= 1ul << slot;
    subpatternAvailable |= 1ul << slot;
    return mask;
  }
  if (subpatternSlotKnown & (1ul << slot)) {
    uint16_t mask = subpatternSlotPages[slot];
    if (maxP < 16) mask &= (uint16_t)((1u << maxP) - 1u);
    return mask;
  }
  uint16_t mask = 0;
  sdIoBeginAudioSafe();
  File file = openSubpatternFile(subpatternFile, slot);
  if (file) {
    if (file.size() > 0) subpatternAvailable |= 1ul << slot;
    uint8_t buffer[512];
    const size_t cells = (size_t)(maxlen - 1) * maxY;
    size_t offset = 0;
    while (offset < cells) {
      const size_t count = min((size_t)128, cells - offset);
      if (file.read(buffer, count * 4) != (int)(count * 4)) break;
      for (size_t n = 0; n < count; ++n) {
        const uint8_t ch = buffer[n * 4];
        if (ch == 0 || ch > 16) continue;
        const unsigned x = 1 + (unsigned)((offset + n) / maxY);
        const unsigned page = maxX > 0 ? ((x - 1) / maxX + 1) : 1u;
        if (page >= 1 && page <= maxP) mask |= (uint16_t)(1u << (page - 1));
      }
      offset += count;
    }
    file.close();
  }
  sdIoEndAudioSafe();
  subpatternSlotPages[slot] = mask;
  subpatternSlotKnown |= 1ul << slot;
  return mask;
}

// One direct read of the page on screen. Missing file means an empty page.
// No exists/tmp/bak and no other letters.
FLASHMEM static void subpatternShowVisible(uint8_t slot) {
  if (slot >= toern_pattern_slots::count) return;
  const int pageCount = effectivePageCount();
  const unsigned maxP = pageCount > 0 ? (unsigned)pageCount : 1u;
  unsigned page = subpatternSelectedPage >= 1 ? subpatternSelectedPage : (GLOB.edit < 1 ? 1u : GLOB.edit);
  if (page > maxP) page = 1u;
  const uint16_t mask = subpatternSlotFilledMask(slot);
  if (subpatternSelectedPage == 0) {
    if (mask != 0 && !(mask & (uint16_t)(1u << (page - 1)))) {
      for (unsigned p = 1; p <= maxP && p <= 16; ++p) {
        if (mask & (uint16_t)(1u << (p - 1))) { page = p; break; }
      }
    } else if (mask == 0) {
      page = 1u;
    }
    subpatternSelectedPage = page;
  }
  subpatternViewPage = page;
  subpatternViewSlot = slot;
  subpatternViewOn = true;
  subpatternClearView();
  if (maxX == 0) return;
  const unsigned x0 = (page - 1) * maxX + 1;
  if (slot == subpatternSlot) {
    for (unsigned col = 1; col <= maxX && col <= 32; ++col) {
      const unsigned x = x0 + col - 1;
      if (x >= maxlen) break;
      for (unsigned y = 1; y <= maxY; ++y) subpatternView[col][y] = note[x][y];
    }
    return;
  }
  sdIoBeginAudioSafe();
  File file = openSubpatternFile(subpatternFile, slot);
  if (file) {
    if (file.size() > 0) {
      subpatternSlotKnown |= 1ul << slot;
      subpatternAvailable |= 1ul << slot;
    }
    const size_t cells = (size_t)maxX * maxY;
    const size_t index = (size_t)(x0 - 1) * maxY;
    if (x0 >= 1 && file.seek(index * 4)) {
      uint8_t buffer[256];
      size_t done = 0;
      while (done < cells) {
        const size_t count = min((size_t)64, cells - done);
        if (file.read(buffer, count * 4) != (int)(count * 4)) break;
        for (size_t n = 0; n < count; ++n) {
          const unsigned col = 1 + (done + n) / maxY;
          const unsigned y = 1 + (done + n) % maxY;
          if (col > 32 || y > maxY) continue;
          Note &cell = subpatternView[col][y];
          cell.channel = buffer[n * 4] <= 16 ? buffer[n * 4] : 0;
          cell.velocity = min(buffer[n * 4 + 1], (uint8_t)127);
          cell.probability = min(buffer[n * 4 + 2], (uint8_t)100);
          cell.condition = buffer[n * 4 + 3];
        }
        done += count;
      }
    }
    file.close();
  }
  sdIoEndAudioSafe();
}

FLASHMEM static bool subpatternTurn(int delta) {
  extern bool subpatternsEnabled;
  if (!subpatternsEnabled || fastRecordActive || isRecording || childLockEnabled || imageMode
      || subpatternReady || subpatternApplied || subpatternOldSavePending || subpatternSaveNew
      || subpatternSwitchArmed || SMP.file < 1 || SMP.file > 999) return true;
  if (!subpatternSelector) {
    subpatternErrorUntil = 0;
    if (subpatternFile != SMP.file) {
      subpatternFile = SMP.file;
      subpatternSlot = 0;
      char a[24];
      subpatternLettered = toern_pattern_slots::path(a, sizeof(a), subpatternFile, 0, true) && SD.exists(a);
      subpatternNotePagesLoaded();
    }
    subpatternSelected = subpatternSlot;
    subpatternSelectedPage = 0;
    rememberSlotPages(subpatternSlot);
    subpatternSelector = true;
  }
  const uint8_t before = subpatternSelected;
  subpatternSelected = toern_pattern_slots::select(subpatternSelected, delta < 0 ? -1 : (delta > 0 ? 1 : 0));
  if (before != subpatternSelected) {
    subpatternTurnGen++;
    subpatternGridReady = false;
    subpatternGridSlot = 255;
    subpatternSelectedPage = 0;
    subpatternViewDirty = true;
  } else if (!subpatternViewOn || subpatternViewSlot != subpatternSelected) {
    subpatternViewDirty = true;
  }
  return true;
}

FLASHMEM static bool subpatternPageTurn(int delta) {
  extern bool subpatternsEnabled;
  if (!subpatternsEnabled || !subpatternSelector || delta == 0) return true;
  const uint16_t mask = subpatternSlotFilledMask(subpatternSelected);
  if (mask == 0) {
    subpatternSelectedPage = 1;
    subpatternViewDirty = true;
    return true;
  }
  const int pageCount = effectivePageCount();
  const int maxP = (pageCount > 0 && pageCount <= 16) ? pageCount : 16;
  int cur = (int)(subpatternSelectedPage >= 1 ? subpatternSelectedPage
      : (subpatternViewOn ? subpatternViewPage : (GLOB.edit >= 1 ? GLOB.edit : 1)));
  if (cur < 1) cur = 1;
  if (cur > maxP) cur = maxP;
  const int step = delta < 0 ? -1 : 1;
  for (int p = cur + step; p >= 1 && p <= maxP; p += step) {
    if (mask & (uint16_t)(1u << (p - 1))) {
      subpatternSelectedPage = (unsigned)p;
      subpatternViewDirty = true;
      break;
    }
  }
  return true;
}

FLASHMEM static bool subpatternOverlayVisible() {
  extern bool subpatternsEnabled;
  if (!subpatternsEnabled) return false;
  return currentMode == &draw && ((fnHeld && subpatternSelector)
      || (int32_t)(subpatternErrorUntil - millis()) > 0);
}

FLASHMEM static void drawSubpatternOverlay() {
  if ((int32_t)(subpatternErrorUntil - millis()) > 0) {
    drawCenteredOverlayLabel("ERR", UI_RED);
    return;
  }
  unsigned page = subpatternSelectedPage >= 1 ? subpatternSelectedPage
      : (subpatternViewOn && subpatternViewSlot == subpatternSelected ? subpatternViewPage
      : (GLOB.edit >= 1 ? GLOB.edit : 1u));
  char label[6];
  snprintf(label, sizeof(label), "%c%u", (char)('A' + subpatternSelected), page);
  const bool known = (subpatternSlotKnown & (1ul << subpatternSelected)) != 0;
  const bool saved = (subpatternAvailable & (1ul << subpatternSelected)) != 0;
  drawCenteredOverlayLabel(label, (known && saved) ? UI_GREEN : UI_BLUE);
}

FLASHMEM static void subpatternFailure() {
  subpatternErrorUntil = millis() + 1200;
}

FLASHMEM static bool saveCurrentSubpattern() {
  const unsigned selectedFile = SMP.file;
  SMP.file = subpatternFile;
  subpatternQuietSave = true;
  savePattern(false);
  subpatternQuietSave = false;
  SMP.file = selectedFile;
  return lastPatternSaveOk;
}

FLASHMEM static bool prepareSubpattern(uint8_t slot) {
  for (unsigned x = 0; x <= maxlen; ++x)
    for (unsigned y = 0; y <= maxY; ++y)
      subpatternGrid[x][y] = {0, defaultVelocity, 100, 1, NOTE_MIDI_PITCH_NONE};
  if (slot >= toern_pattern_slots::count || subpatternFile < 1 || subpatternFile > 999)
    return false;
  File f = openSubpatternFile(subpatternFile, slot);
  subpatternStagedNew = !f;
  if (f) {
    uint8_t buffer[512];
    const size_t cells = (size_t)(maxlen - 1) * maxY;
    size_t offset = 0;
    while (offset < cells) {
      const size_t count = min((size_t)128, cells - offset);
      if (f.read(buffer, count * 4) != (int)(count * 4)) { f.close(); subpatternStagedNew = false; return false; }
      for (size_t n = 0; n < count; ++n) {
        Note &cell = subpatternGrid[1 + (offset + n) / maxY][1 + (offset + n) % maxY];
        cell.channel = buffer[n*4] <= 16 ? buffer[n*4] : 0;
        cell.velocity = min(buffer[n*4+1], (uint8_t)127);
        cell.probability = min(buffer[n*4+2], (uint8_t)100);
        cell.condition = buffer[n*4+3];
      }
      offset += count;
      sdIoYield();
    }
    const size_t base = cells * 4 + 2 + sizeof(SMP);
    if (!f.seek(base)) { f.close(); subpatternStagedNew = false; return false; }
    if (f.available() > 0) {
      uint8_t header[8];
      if (f.read(header, 8) != 8) { f.close(); subpatternStagedNew = false; return false; }
      if (memcmp(header, PITCH_EXTENSION_HEADER, 8) == 0) {
        offset = 0;
        while (offset < cells) {
          const size_t count = min(sizeof(buffer), cells - offset);
          if (f.read(buffer, count) != (int)count) { f.close(); subpatternStagedNew = false; return false; }
          for (size_t n = 0; n < count; ++n) {
            subpatternGrid[1 + (offset+n)/maxY][1 + (offset+n)%maxY].midiPitch =
                buffer[n] <= 127 ? buffer[n] : NOTE_MIDI_PITCH_NONE;
          }
          offset += count;
          sdIoYield();
        }
      }
    }
    f.close();
  }
  memset(subpatternVoiceLen, 0, sizeof(subpatternVoiceLen));
  memset(subpatternVoicePages, 0, sizeof(subpatternVoicePages));
  memset(subpatternVoiceRows, 0, sizeof(subpatternVoiceRows));
  memset(subpatternHasNotes, 0, sizeof(subpatternHasNotes));
  subpatternLastPage = 1; subpatternLongestVoice = 1;
  for (unsigned x = 1; x < maxlen; ++x) for (unsigned y = 1; y <= maxY; ++y) {
    const unsigned ch = subpatternGrid[x][y].channel;
    const unsigned page = (x - 1) / maxX + 1;
    if (!ch || page > (unsigned)effectivePageCount()) continue;
    subpatternHasNotes[page] = true;
    if (page > subpatternLastPage) subpatternLastPage = page;
    if (ch < 16) {
      subpatternVoiceLen[ch] = page;
      subpatternVoicePages[ch] |= 1u << (page - 1);
      subpatternVoiceRows[ch] |= 1u << (y - 1);
      if (page > subpatternLongestVoice) subpatternLongestVoice = page;
    }
  }
  extern int loopLength;
  if (!voiceMode && loopLength > 0) subpatternLastPage = min(loopLength, effectivePageCount());
  return true;
}

FLASHMEM static bool subpatternInstallStaged(uint8_t slot, bool now) {
  if (!(subpatternGridReady && subpatternGridSlot == slot)) {
    sdIoBeginAudioSafe();
    const bool prepared = prepareSubpattern(slot);
    sdIoEndAudioSafe();
    if (!prepared) {
      subpatternStagedNew = false;
      subpatternFailure();
      return false;
    }
    subpatternGridReady = true;
    subpatternGridSlot = slot;
  }
  if (!now) return true;
  {
    toern_audio::StateGuard guard;
    subpatternPendingSlot = slot;
    subpatternBoundary = true;
    __DMB();
    subpatternReady = true;
    if (!subpatternCommitAtStepStart()) {
      subpatternReady = false;
      subpatternStagedNew = false;
      subpatternFailure();
      return false;
    }
  }
  return true;
}

FLASHMEM static bool subpatternFnRelease() {
  if (!subpatternSelector) return false;
  subpatternSelector = false;
  if (currentMode != &draw) {
    subpatternCancelPreview();
    return false;
  }
  if (subpatternSelected == subpatternSlot) {
    if (subpatternSelectedPage >= 1 && subpatternSelectedPage <= maxPages
        && subpatternSelectedPage != GLOB.edit && hasNotes[subpatternSelectedPage]) {
      const unsigned p = subpatternSelectedPage;
      const unsigned col = (maxX > 0 && GLOB.x >= 1) ? (((GLOB.x - 1) % maxX) + 1) : 1u;
      GLOB.edit = p;
      editpage = (int)p;
      GLOB.x = (p - 1) * maxX + col;
      if (!isNowPlaying) {
        GLOB.page = p;
        beat = beatForUI = (p - 1) * maxX + 1;
      }
      subpatternCancelPreview();
      return true;
    }
    subpatternCancelPreview();
    return false;
  }
  if (fastRecordActive || isRecording || subpatternReady || subpatternApplied
      || subpatternOldSavePending || subpatternSaveNew) {
    subpatternCancelPreview();
    return false;
  }
  // When creating a lettered family from a legacy/blank pattern while paused,
  // wipe any stale N-B..N-E leftovers before reading or saving the target slot.
  if (!isNowPlaying && !subpatternLettered && subpatternFile >= 1 && subpatternFile <= 999) {
    sdIoBeginAudioSafe();
    for (uint8_t s = 1; s < toern_pattern_slots::count; ++s) {
      char sp[24], st[32], sb[32];
      if (!toern_pattern_slots::path(sp, sizeof(sp), subpatternFile, s, true)) continue;
      snprintf(st, sizeof(st), "%s.tmp", sp);
      snprintf(sb, sizeof(sb), "%s.bak", sp);
      deletePatternTransaction(SD, sp, st, sb);
    }
    sdIoEndAudioSafe();
  }
  // Playback keeps the previous slot until its boundary. Stopped playback
  // installs now, matching NEXT with the transport stopped.
  subpatternPendingSlot = subpatternSelected;
  subpatternSwitchArmed = true;
  subpatternViewOn = true;
  if (isNowPlaying) return false;
  if (!subpatternInstallStaged(subpatternSelected, true)) {
    subpatternCancelPreview();
    return false;
  }
  subpatternSwitchArmed = false;
  return true;
}

static bool subpatternCommitAtStepStart() {
  if (!subpatternReady || fastRecordActive || isRecording
      || (isNowPlaying && !subpatternBoundary)) return false;
  // Save the outgoing slot only when paused; while playing, never write to SD.
  const bool saveWhenPaused = !isNowPlaying;
  if (saveWhenPaused) {
    for (unsigned x = 0; x <= maxlen; ++x) {
      Note row[maxY + 1];
      memcpy(row, note[x], sizeof(row));
      memcpy(note[x], subpatternGrid[x], sizeof(row));
      memcpy(subpatternGrid[x], row, sizeof(row));
    }
    subpatternPreviousSlot = subpatternSlot;
    subpatternOldSavePending = true;
    rememberGridPages(subpatternPreviousSlot, subpatternGrid);
    if (subpatternSlot == 0 && !subpatternLettered) subpatternDeleteLegacy = true;
    subpatternLettered = true;
    subpatternSaveNew = subpatternStagedNew;
  } else {
    subpatternPreviousSlot = subpatternSlot;
    rememberGridPages(subpatternPreviousSlot, note);
    memcpy(note, subpatternGrid, sizeof(note));
    subpatternOldSavePending = false;
    subpatternSaveNew = false;
  }
  subpatternStagedNew = false;
  memcpy(voicePageLen, subpatternVoiceLen, sizeof(voicePageLen));
  memcpy(voicePageMask, subpatternVoicePages, sizeof(voicePageMask));
  memcpy(voiceRowMask, subpatternVoiceRows, sizeof(voiceRowMask));
  memcpy(hasNotes, subpatternHasNotes, sizeof(hasNotes));
  voiceModePages = subpatternLongestVoice;
  lastPage = subpatternLastPage;
  for (int ch = 0; ch < 16; ++ch) voiceLoopCount[ch] = 1;
  subpatternSlot = subpatternPendingSlot;
  SMP.file = subpatternFile;
  const unsigned currentPage = GLOB.edit >= 1 && GLOB.edit <= maxPages ? GLOB.edit : 1u;
  unsigned firstFilled = 1u;
  for (unsigned p = 1; p <= maxPages; ++p) {
    if (hasNotes[p]) { firstFilled = p; break; }
  }
  const unsigned requestedPage = (subpatternSelectedPage >= 1 && subpatternSelectedPage <= maxPages)
      ? subpatternSelectedPage : currentPage;
  const unsigned targetPage = hasNotes[requestedPage] ? requestedPage : firstFilled;
  const unsigned col = (maxX > 0 && GLOB.x >= 1) ? (((GLOB.x - 1) % maxX) + 1) : 1u;
  GLOB.page = GLOB.edit = targetPage;
  editpage = (int)targetPage;
  GLOB.x = (targetPage - 1) * maxX + col;
  beat = beatForUI = (targetPage - 1) * maxX + 1;
  loopCount = 1;
  pendingPage = 0;
  subpatternReady = false;
  subpatternBoundary = false;
  subpatternSwitchArmed = false;
  subpatternViewOn = false;
  subpatternGridReady = false;
  subpatternSelectedPage = 0;
  __DMB();
  subpatternApplied = true;
  rememberSlotPages(subpatternSlot);
  return true;
}

void subpatternNotePagesLoaded() {
  memset(subpatternSlotPages, 0, sizeof(subpatternSlotPages));
  subpatternSlotKnown = 0;
  subpatternAvailable = 0;
  rememberSlotPages(subpatternSlot);
  subpatternCancelPreview();
}

bool subpatternBrowsePageMask(uint16_t *mask) {
  if (!mask || subpatternSelected >= toern_pattern_slots::count) return false;
  if (!subpatternSelector && !subpatternViewOn) return false;
  const uint8_t slot = subpatternSelector ? subpatternSelected : subpatternViewSlot;
  if (slot >= toern_pattern_slots::count) return false;
  if (slot == subpatternSlot && !subpatternSwitchArmed && !subpatternReady) {
    uint16_t live = 0;
    const unsigned pages = maxPages < 16 ? maxPages : 16;
    for (unsigned p = 1; p <= pages; ++p)
      if (hasNotes[p]) live |= (uint16_t)(1u << (p - 1));
    *mask = live;
    return true;
  }
  if (subpatternSlotKnown & (1ul << slot)) {
    *mask = subpatternSlotPages[slot];
    return true;
  }
  if (subpatternViewOn && subpatternViewSlot == slot) {
    uint16_t bits = 0;
    bool any = false;
    for (unsigned x = 1; x <= maxX && x <= 32 && !any; ++x)
      for (unsigned y = 1; y <= maxY; ++y)
        if (subpatternView[x][y].channel) { any = true; break; }
    if (any && subpatternViewPage >= 1 && subpatternViewPage <= 16)
      bits = (uint16_t)(1u << (subpatternViewPage - 1));
    *mask = bits;
    return true;
  }
  return false;
}

Note *subpatternViewedNote(unsigned ix, unsigned iy) {
  if (!subpatternViewOn || ix < 1 || iy < 1 || ix > 32 || iy > maxY || ix > maxX) return nullptr;
  const unsigned editPage = GLOB.edit < 1 ? 1u : GLOB.edit;
  if (subpatternViewSlot == subpatternSlot && !subpatternSwitchArmed && !subpatternReady
      && subpatternViewPage == editPage) return nullptr;
  return &subpatternView[ix][iy];
}

bool subpatternShowingOtherSlot() {
  if (!isNowPlaying || !subpatternViewOn) return false;
  if (subpatternViewSlot == subpatternSlot && !subpatternSwitchArmed && !subpatternReady) return false;
  return true;
}

FLASHMEM static void stageSelectedSubpattern() {
  if (subpatternReady || subpatternApplied || subpatternOldSavePending || fastRecordActive || isRecording)
    return;
  const uint8_t viewSlot = subpatternSelector ? subpatternSelected
      : (subpatternSwitchArmed ? subpatternPendingSlot : subpatternViewSlot);
  const unsigned page = subpatternSelectedPage >= 1 ? subpatternSelectedPage : (GLOB.edit < 1 ? 1u : GLOB.edit);
  if ((subpatternSelector || subpatternSwitchArmed)
      && (subpatternViewDirty || !subpatternViewOn || subpatternViewSlot != viewSlot || page != subpatternViewPage)) {
    subpatternViewDirty = false;
    subpatternShowVisible(viewSlot);
    if (subpatternSelector) return;
  }
  if (!subpatternSelector && !subpatternSwitchArmed) return;
  const uint8_t target = subpatternSwitchArmed ? subpatternPendingSlot : subpatternSelected;
  if (target == subpatternSlot || target >= toern_pattern_slots::count) {
    if (subpatternSwitchArmed && target == subpatternSlot) subpatternCancelPreview();
    return;
  }
  if (!(subpatternGridReady && subpatternGridSlot == target)) {
    // The visible page is on screen. Read the rest on a later loop so
    // spinning A-E does not load every file.
    if (subpatternSelector && subpatternLoadGen != subpatternTurnGen) {
      subpatternLoadGen = subpatternTurnGen;
      return;
    }
    sdIoBeginAudioSafe();
    const bool prepared = prepareSubpattern(target);
    sdIoEndAudioSafe();
    if (!prepared) {
      subpatternCancelPreview();
      subpatternSelector = false;
      subpatternFailure();
      return;
    }
    subpatternGridReady = true;
    subpatternGridSlot = target;
  }
  if (!subpatternSwitchArmed) return;
  if (isNowPlaying) {
    toern_audio::StateGuard guard;
    subpatternPendingSlot = target;
    __DMB();
    subpatternReady = true;
    subpatternSwitchArmed = false;
    return;
  }
  if (!subpatternInstallStaged(target, true)) subpatternCancelPreview();
  else subpatternSwitchArmed = false;
}

FLASHMEM static void serviceSubpatterns() {
  if (subpatternSelector && currentMode != &draw) subpatternSelector = false;
  if (subpatternReady && !isNowPlaying) {
    toern_audio::StateGuard guard;
    subpatternCommitAtStepStart();
  }
  if (subpatternApplied) {
    subpatternApplied = false;
    // Keep the page if it also has notes, and keep the cursor column on that page.
    unsigned targetPage = GLOB.edit >= 1 && GLOB.edit <= maxPages ? GLOB.edit : 1u;
    int col = (maxX > 0 && GLOB.x >= 1) ? (int)(((GLOB.x - 1) % maxX) + 1) : 1;
    int targetX = (int)((targetPage - 1) * maxX) + col;
    GLOB.x = (unsigned)targetX;
    draw.pos[3] = (unsigned)targetX;
    if (ctrlMode == 0) draw.pos[1] = targetPage;
    if (currentMode == &draw || currentMode == &singleMode) {
      currentMode->pos[3] = (unsigned)targetX;
      Encoder[3].writeCounter((int32_t)targetX);
      if (ctrlMode == 0) {
        currentMode->pos[1] = targetPage;
        Encoder[1].writeMax((int32_t)encoderPageMax());
        Encoder[1].writeCounter((int32_t)targetPage);
      }
      applyDrawSingleYEncoderLimits();
    }
  }
  if (isNowPlaying) {
    subpatternOldSavePending = false;
    subpatternSaveNew = false;
    subpatternDeleteLegacy = false;
    stageSelectedSubpattern();
    return;
  }
  static unsigned long retryAt = 0;
  if ((subpatternOldSavePending || subpatternSaveNew) && (int32_t)(millis() - retryAt) < 0) return;
  if (subpatternOldSavePending) {
    subpatternSavingPrevious = true;
    const bool saved = saveCurrentSubpattern();
    subpatternSavingPrevious = false;
    if (!saved) {
      retryAt = millis() + 1500;
      subpatternFailure();
      return; // Keep the old grid and block another switch until saved.
    }
    subpatternOldSavePending = false;
    retryAt = 0;
    if (subpatternDeleteLegacy) {
      char legacy[24], legacyTmp[32], legacyBackup[32];
      toern_pattern_slots::path(legacy, sizeof(legacy), subpatternFile, 0, false);
      snprintf(legacyTmp, sizeof(legacyTmp), "%s.tmp", legacy);
      snprintf(legacyBackup, sizeof(legacyBackup), "%s.bak", legacy);
      sdIoBeginAudioSafe();
      if (deletePatternTransaction(SD, legacy, legacyTmp, legacyBackup))
        subpatternDeleteLegacy = false;
      sdIoEndAudioSafe();
    }
  }
  // Persist a newly created blank slot too when paused.
  if (subpatternSaveNew) {
    if (saveCurrentSubpattern()) {
      subpatternSaveNew = false;
      retryAt = 0;
    } else {
      retryAt = millis() + 1500;
      subpatternFailure();
    }
  }
  stageSelectedSubpattern();
}
