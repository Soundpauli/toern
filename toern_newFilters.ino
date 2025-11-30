extern uint8_t filterPageCount[NUM_CHANNELS];
extern uint8_t filterPage[NUM_CHANNELS];

static bool channelHasFreeverb(uint8_t chan) {
  return !(chan == 3 || chan == 4);
}

static bool channelIsDrumMode(uint8_t chan) {
  if (chan < 1 || chan > 3) return false;
  return SMP.filter_settings[chan][EFX] == 1;
}

static bool updateFilterPageAvailability(uint8_t chan) {
  if (chan >= 1 && chan <= 3) {
    uint8_t desiredCount = channelIsDrumMode(chan) ? 4 : 3;
    if (filterPageCount[chan] != desiredCount) {
      filterPageCount[chan] = desiredCount;
      if (desiredCount == 0) {
        filterPage[chan] = 0;
      } else if (filterPage[chan] >= desiredCount) {
        filterPage[chan] = desiredCount - 1;
      }
      return true;
    }
  }
  return false;
}
unsigned long lastEncoderChange[4] = { 0, 0, 0, 0 };

int8_t lastChangedEncoder = -1;
unsigned long lastInteraction = millis();  // global timestamp
const unsigned long FILTER_INTERACTION_TIMEOUT = 1000; // ms before reverting to initial view


bool showingAny = false;
bool showSingleFilter[4] = { false, false, false, false };


uint8_t scaleToDisplay(const SliderDefEntry& meta, uint8_t val) {
  return (meta.displayRange < meta.maxValue) ? constrain(mapf(val, 0, meta.maxValue, 0, meta.displayRange - 1), 0, meta.displayRange - 1) : val;
}

uint8_t scaleFromDisplay(const SliderDefEntry& meta, uint8_t val) {
  return (meta.displayRange < meta.maxValue) ? constrain(mapf(val, 0, meta.displayRange - 1, 0, meta.maxValue), 0, meta.maxValue) : val;
}


#define MAX_FILTER_RESOLUTION 32.0
#define FILTER_BYPASS MAX_FILTER_RESOLUTION
#define FILTER_WET 0
#define FILTER_MID (MAX_FILTER_RESOLUTION / 2)
#define PARAM_COUNT 6  // Number of valid params, must include RELEASE (index 5)

// Read raw 0..MAX_FILTER_RESOLUTION setting directly
uint8_t readSetting(SettingArray arr, int8_t idx, uint8_t chan) {
  //if (idx < 0) return 0;
  uint8_t raw = 0;
  switch (arr) {
    case ARR_FILTER: raw = constrain(SMP.filter_settings[chan][idx], 0, MAX_FILTER_RESOLUTION); break;
    case ARR_SYNTH: raw = constrain(SMP.synth_settings[chan][idx], 0, MAX_FILTER_RESOLUTION); break;
    case ARR_DRUM: raw = constrain(SMP.drum_settings[chan][idx], 0, MAX_FILTER_RESOLUTION); break;
    case ARR_PARAM: raw = constrain(SMP.param_settings[chan][idx], 0, MAX_FILTER_RESOLUTION); break;
    default: return 0;
  }
  return raw;
}


// 2) Helper to draw the name centered at y=10
void drawSliderName(uint8_t x0, uint8_t x1, const char* name, CRGB filtercol) {
  /*int len = strlen(name);
  const uint8_t charW = 3;
  int totalW = charW * len;
  int cx = (x0 + x1) / 2;
  int tx = cx - totalW/2;
  if (tx < 0)           tx = 0;
  if (tx + totalW > 16) tx = 16 - totalW;*/

  // CLEAR just that span on row 10
  for (int x = 1; x <= 16; ++x) {
    light(x, 12, CRGB::Black);
    light(x, 13, CRGB::Black);
    light(x, 14, CRGB::Black);
    light(x, 15, CRGB::Black);
    light(x, 16, CRGB::Black);
  }


  drawText(name, 2, 12, filtercol);
}



void drawCornerValueCustom(uint8_t encoderIndex, uint8_t val, const SliderDefEntry& meta) {
  // Special case: Channel 11 WAVE slider (encoder 0) should be at x=6
  uint8_t chan = GLOB.currentChannel;
  uint8_t x;
  if (chan == 11 && encoderIndex == 0 && meta.arr == ARR_FILTER && meta.idx == FILTER_WAVEFORM) {
    x = 6;
  } else {
    x = (encoderIndex < 2) ? 10 : 2;
  }
  const uint8_t y = 5;
  const uint8_t w = 6, h = 7;

  // Clear background
  for (uint8_t dx = 0; dx <= w; ++dx) {
    for (uint8_t dy = 0; dy < h; ++dy) {
      light(x + dx, y + dy - 1, CRGB::Black);
    }
  }

  char buf[4];
  uint8_t shownVal = scaleToDisplay(meta, val);

  if (meta.displayMode == DISPLAY_ENUM && shownVal < meta.displayRange && meta.enumNames && meta.enumNames[shownVal]) {
    snprintf(buf, sizeof(buf), "%s", meta.enumNames[shownVal]);
  } else {
    snprintf(buf, sizeof(buf), "%u", shownVal);
  }

  // ─── Color according to value ───
  CRGB baseColor = filterColors[filterPage[chan]][encoderIndex];
  CRGB dimmed = baseColor;
  dimmed.nscale8(64);
  uint8_t blendVal = mapf(val, 0, meta.maxValue, 0, 255);
  CRGB textColor = blend(CRGB::Red, CRGB::Green, blendVal);

  drawText(buf, x, y, textColor);
}



void drawVerticalSlider(uint8_t x0, uint8_t x1, uint8_t val, uint8_t maxVal, CRGB baseColor, bool lowHighSlider = false, uint8_t sliderIndex = 0, uint8_t page = 0, bool switchtype = false) {
  const uint8_t displayResolution = 10;
  uint8_t displayVal;
  if (switchtype) {
    displayVal = (val == 0 ? 1 : displayResolution);
  } else {
    displayVal = mapf(val, 0, maxVal, 1, displayResolution);
    displayVal = constrain(displayVal, 1, displayResolution);
  }

  // ─── Dim and blend base color by value (no fading) ───
  uint8_t blendVal = mapf(val, 0, maxVal, 0, 255);
  CRGB color = blend(CRGB::Black, baseColor, blendVal);

  // Determine if this slider is the defaultFastFilter for the current channel
  bool isDefaultFast = false;
  uint8_t chan = GLOB.currentChannel;
  if (chan < NUM_CHANNELS) {
    if (sliderDef[chan][page][sliderIndex].arr == defaultFastFilter[chan].arr &&
        sliderDef[chan][page][sliderIndex].idx == defaultFastFilter[chan].idx) {
      isDefaultFast = true;
    }
  }

  // Check if this is a PASS filter with value == 15
  bool isPassFilterAt15 = false;
  if (chan < NUM_CHANNELS) {
    if (sliderDef[chan][page][sliderIndex].arr == ARR_FILTER && 
        sliderDef[chan][page][sliderIndex].idx == PASS && 
        val == 15) {
      isPassFilterAt15 = true;
    }
  }

  // Check if this is DETUNE or OCTAVE at middle value (maxVal/2 for centered parameters)
  bool isDetuneOctaveAtMiddle = false;
  if (chan < NUM_CHANNELS) {
    if (sliderDef[chan][page][sliderIndex].arr == ARR_FILTER && 
        (sliderDef[chan][page][sliderIndex].idx == DETUNE || sliderDef[chan][page][sliderIndex].idx == OCTAVE) && 
        val == 16) { // Middle value for maxVal=32
      isDetuneOctaveAtMiddle = true;
    }
  }

  for (uint8_t y = 1; y <= displayResolution; ++y) {
    CRGB c;
    if (y == displayVal) {
      if (isDefaultFast) {
        c = CRGB(255, 255, 0); // Yellow
      } else {
        c = CRGB::White;
      }
    } else if (switchtype) {
      // For switchtype, val is 0 or 1, displayVal is 1 or displayResolution
      c = (y < displayVal) ? CRGB::Green : CRGB::Red;
    } else if (lowHighSlider) {
      // Special case: PASS filter at value 15 shows bright green
      if (isPassFilterAt15) {
        c = CRGB(0, 255, 0); // Bright Green
      } else {
        c = (y < displayVal) ? CRGB(148, 0, 211) : CRGB::Red; // Violet : Red
      }
    } else {
      // Special case: DETUNE and OCTAVE at middle value show green at top
      if (isDetuneOctaveAtMiddle && y == 1) {
        c = CRGB::Green; // Green at top when at middle value
      } else {
        c = (y < displayVal) ? color : CRGB::Black;
      }
    }

    light(x0, y, c);
    light(x1, y, c);
  }
}


void initSliders(uint8_t page, uint8_t chan) {
  updateFilterPageAvailability(chan);
  page = filterPage[chan];
  showFilterNames(chan);

  Serial.print("[Debug] Switched to slider page: ");
  Serial.println(page);
  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[chan][page][i];
    if (d.arr == ARR_NONE && d.idx == -1) continue; // Skip value change
    const auto& meta = sliderDef[chan][page][i];
    uint8_t val = 0;

    if (d.arr != ARR_NONE) {
      if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) {
        Serial.printf("[Warning] init skip invalid ARR_PARAM idx %d\n", d.idx);
        continue;
      }

      val = readSetting(d.arr, d.idx, chan);
      
      
    }

    val = constrain(val, 0, meta.maxValue);
    Serial.printf("[Debug] Encoder %u page %u channel %u -> value %u\n", i, page, chan, val);
    currentMode->pos[i] = val;
    Encoder[i].writeMax((int32_t)(meta.maxValue));
    Encoder[i].writeCounter((int32_t)val);
  }

  showFilterPages(chan);
}


void slider(uint8_t page) {
  unsigned long now = millis();
  bool activeInteraction = (now - lastInteraction < FILTER_INTERACTION_TIMEOUT);
  uint8_t chan = GLOB.currentChannel;
  for (uint8_t i = 0; i < 4; ++i) {
    const auto& def = sliderDef[chan][page][i];
    if (def.arr == ARR_NONE && def.idx == -1) continue; // Skip rendering
    const auto& meta = sliderDef[chan][page][i];
    uint8_t val = constrain(currentMode->pos[i], 0, meta.maxValue);
    // lowHighSlider is true if (old logic) OR if maxValue == 2
    bool isLowHigh = (def.arr == ARR_FILTER && def.idx == PASS); // or even others
    bool switchtype = (def.arr == ARR_FILTER && def.idx == EFX) || (def.arr == ARR_FILTER && def.idx == ACTIVE); // or even others
    // Skip drawing if interaction is active and this isn't the last touched encoder
    if (activeInteraction && i != lastChangedEncoder) {
      // Clear the slider area so it disappears
      for (uint8_t y = 1; y <= 10; ++y) {
        light(sliderCols[i][0], y, CRGB::Black);
        light(sliderCols[i][1], y, CRGB::Black);
      }
      continue;
    }

    //  Draw slider

    CRGB sliderColor = filterColors[page][i];
    bool reverbDisabled = (def.arr == ARR_FILTER && def.idx == REVERB && !channelHasFreeverb(chan));
    if (reverbDisabled) {
      sliderColor = CRGB(4, 4, 4);
    }

    drawVerticalSlider(
      sliderCols[i][0],
      sliderCols[i][1],
      val,
      meta.maxValue,
      sliderColor,
      isLowHigh,
      i, // sliderIndex
      page, // pass page
      switchtype // switchtype (default for now)
    );
    showSingleFilter[i] = false;
    if (i == lastChangedEncoder && activeInteraction) {
      drawSliderName(sliderCols[i][0],
                     sliderCols[i][1],
                     def.name,
                     sliderColor);
      showSingleFilter[i] = true;
    }
  }

  // Show fallback filter names if no single filter is active
  bool allFalse = true;
  for (int i = 0; i < 4; ++i) {
    if (showSingleFilter[i]) {
      allFalse = false;
      break;
    }
  }

  if (allFalse) {
    showFilterNames(GLOB.currentChannel);
  }
}

void printSliderDefTarget(uint8_t page, uint8_t encoder) {
  const auto& def = sliderDef[GLOB.currentChannel][page][encoder];
  const char* arrName = "UNKNOWN";
  const char* idxName = "??";

  // Array type string
  switch (def.arr) {
    case ARR_FILTER: arrName = "ARR_FILTER"; break;
    case ARR_SYNTH: arrName = "ARR_SYNTH"; break;
    case ARR_PARAM: arrName = "ARR_PARAM"; break;
    default: break;
  }

  // Use existing `filterNames` array for the label
  if (encoder < 4 && page < 4) {
    idxName = def.name;
  }

  Serial.printf("{%s, %s}\n", arrName, idxName);
}


void setDefaultFilterFromSlider(uint8_t page, uint8_t encoder) {
  auto& def = sliderDef[GLOB.currentChannel][page][encoder];
  uint8_t chan = GLOB.currentChannel;

  switch (def.arr) {
    case ARR_FILTER:
      defaultFastFilter[chan].arr = def.arr;
      defaultFastFilter[chan].idx = def.idx;
      //Serial.printf("[Default] FILTER set: ch=%u idx=%u\n", chan, def.idx);
      break;

    case ARR_SYNTH:
      defaultFastFilter[chan].arr = def.arr;
      defaultFastFilter[chan].idx = def.idx;
      //Serial.printf("[Default] SYNTH set: ch=%u idx=%u\n", chan, def.idx);
      break;

    case ARR_PARAM:
      if (def.idx < PARAM_COUNT) {
        defaultFastFilter[chan].arr = def.arr;
        defaultFastFilter[chan].idx = def.idx;
        //Serial.printf("[Default] PARAM set: ch=%u idx=%u\n", chan, def.idx);
      } else {
        Serial.printf("[Warning] PARAM idx out of range: %u\n", def.idx);
      }
      break;

    case ARR_DRUM:
      defaultFastFilter[chan].arr = def.arr;
      defaultFastFilter[chan].idx = def.idx;
      //Serial.printf("[Default] DRUM set: ch=%u idx=%u\n", chan, def.idx);
      break;

    default:
      Serial.printf("[Skip] Not assignable default: arr=%d\n", def.arr);
      break;
  }

  Serial.print("[Default] Set defaultFastFilter: ");
  printSliderDefTarget(page, encoder);
  
  // Update encoder colors to reflect the new default fast filter
  updateFilterEncoderColors();
}

void toggleDefaultFilterFromSlider(uint8_t page, uint8_t encoder) {
  auto& def = sliderDef[GLOB.currentChannel][page][encoder];
  uint8_t chan = GLOB.currentChannel;

  // Check if this encoder is already the active fast filter
  bool isCurrentlyActive = (defaultFastFilter[chan].arr == def.arr && 
                           defaultFastFilter[chan].idx == def.idx);

  if (isCurrentlyActive) {
    // Deselect the fast filter by setting it to ARR_NONE
    defaultFastFilter[chan].arr = ARR_NONE;
    defaultFastFilter[chan].idx = -1;
    Serial.print("[Default] Cleared defaultFastFilter");
  } else {
    // Set as new default fast filter
    switch (def.arr) {
      case ARR_FILTER:
        defaultFastFilter[chan].arr = def.arr;
        defaultFastFilter[chan].idx = def.idx;
        break;

      case ARR_SYNTH:
        defaultFastFilter[chan].arr = def.arr;
        defaultFastFilter[chan].idx = def.idx;
        break;

      case ARR_PARAM:
        if (def.idx < PARAM_COUNT) {
          defaultFastFilter[chan].arr = def.arr;
          defaultFastFilter[chan].idx = def.idx;
        } else {
          Serial.printf("[Warning] PARAM idx out of range: %u\n", def.idx);
          return; // Don't update colors if we can't set it
        }
        break;

      case ARR_DRUM:
        defaultFastFilter[chan].arr = def.arr;
        defaultFastFilter[chan].idx = def.idx;
        break;

      default:
        Serial.printf("[Skip] Not assignable default: arr=%d\n", def.arr);
        return; // Don't update colors if we can't set it
    }
    Serial.print("[Default] Set defaultFastFilter: ");
    printSliderDefTarget(page, encoder);
  }
  
  // Update encoder colors to reflect the new default fast filter state
  updateFilterEncoderColors();
}

void processAdjustments_new(uint8_t page) {
  Serial.print("[Debug] processAdjustments_new page: ");
  Serial.println(page);

  uint8_t chan = GLOB.currentChannel;
  bool pageCountChanged = false;

  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[chan][page][i];
    if (d.arr == ARR_NONE && d.idx == -1) continue; // Skip if ARR_NONE
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) {
      Serial.printf("[Warning] Skipping ARR_PARAM idx %d - out of bounds\n", d.idx);
      continue;
    }

    const auto& meta = sliderDef[chan][page][i];
    uint8_t rawVal = constrain(currentMode->pos[i], 0, meta.maxValue);
    uint8_t val = scaleToDisplay(meta, rawVal);
    uint8_t prev = readSetting(d.arr, d.idx, chan);
  
    if (val == prev) continue;

    // Print debug output
    const char* arrType = (d.arr == ARR_FILTER) ? "ARR_FILTER" : (d.arr == ARR_SYNTH) ? "ARR_SYNTH" : (d.arr == ARR_PARAM) ? "ARR_PARAM" : (d.arr == ARR_DRUM) ? "ARR_DRUM" : "ARR_NONE";
    Serial.printf("[Debug] %s:%s raw %u -> display %u\n", arrType, d.name, rawVal, val);

    float mappedVal = 0;

    switch (d.arr) {
      case ARR_FILTER:
          SMP.filter_settings[chan][d.idx] = currentMode->pos[i];
          setFilters(d.idx, chan, false);
          if (d.idx == EFX) {
            if (updateFilterPageAvailability(chan)) {
              pageCountChanged = true;
            }
          }
        break;

      case ARR_SYNTH:
        SMP.synth_settings[chan][d.idx] = currentMode->pos[i];
        
        break;

      case ARR_DRUM:
        SMP.drum_settings[chan][d.idx] =  currentMode->pos[i];
        setDrums(d.idx, chan);
        break;

      case ARR_PARAM:
        SMP.param_settings[chan][d.idx] = currentMode->pos[i];
        setParams(d.idx, chan);

        // Optional: process/update param visuals here
        break;

      default:
        break;
    }
    
  }
  
  if (pageCountChanged) {
    initSliders(filterPage[chan], chan);
    updateFilterEncoderColors();
  }


  if (GLOB.currentChannel == 11) updateSynthVoice(11);

}





// Handle touch-triggered page switching and conditional update propagation
void setNewFilters() {
  int touchValue = fastTouchRead(SWITCH_2);
  static bool lastTouch = false;
  bool currTouch = (touchValue > touchThreshold);
  uint8_t chan = GLOB.currentChannel;
  updateFilterPageAvailability(chan);
  
  static uint8_t lastFilterPage[NUM_CHANNELS] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
  static uint8_t lastEncoderPos[4] = {255, 255, 255, 255};
  static uint8_t lastChannel = 255;
  static unsigned long lastRedrawTime = 0;
  const unsigned long REDRAW_THROTTLE_MS = 50; // Throttle redraws to max once per 50ms
  
  // Reset encoder tracking when channel changes
  bool channelChanged = (chan != lastChannel);
  if (channelChanged) {
    lastChannel = chan;
    // Reset encoder position tracking to force redraw on channel change
    for (uint8_t i = 0; i < 4; ++i) {
      lastEncoderPos[i] = 255;
    }
  }
  
  bool pageChanged = (filterPage[chan] != lastFilterPage[chan]);
  if (currTouch && !lastTouch) {
    // Use filterPageCount[chan] for wrapping
    filterPage[chan] = (filterPage[chan] + 1) % filterPageCount[chan];
    filterDrawActive = false;
    filterDrawEndTime=0;
    initSliders(filterPage[chan], chan);
    
    // Update encoder colors for the new filter page and default fast filter
    updateFilterEncoderColors();
    pageChanged = true;
    lastFilterPage[chan] = filterPage[chan];
  }
  lastTouch = currTouch;

  bool anyChanged = false;
  bool encoderPosChanged = false;
  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[chan][filterPage[chan]][i];
    if (d.arr == ARR_NONE && d.idx == -1) continue; // Skip if ARR_NONE
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) continue;
    uint8_t val = constrain(currentMode->pos[i], 0, MAX_FILTER_RESOLUTION);
    uint8_t prev = readSetting(d.arr, d.idx, chan);
    
    // Check if encoder position changed (for redraw detection)
    if (currentMode->pos[i] != lastEncoderPos[i]) {
      encoderPosChanged = true;
      lastEncoderPos[i] = currentMode->pos[i];
    }
    
    if (val != prev) {
      showSingleFilter[i] = true;

      drawSliderName(sliderCols[i][0],
                     sliderCols[i][1],
                     d.name,
                     filterColors[filterPage[chan]][i]);


      anyChanged = true;
      lastEncoderChange[i] = millis();  // store activity timestamp
      lastInteraction = millis();
      lastChangedEncoder = i;
    }
  }

  if (anyChanged) {
    processAdjustments_new(filterPage[chan]);
    showFilterPages(chan);
    lastRedrawTime = millis();
  } else if (pageChanged || encoderPosChanged || channelChanged) {
    // Only redraw if page changed, encoder positions changed, or channel changed, and throttle to prevent excessive redraws
    unsigned long now = millis();
    if (now - lastRedrawTime >= REDRAW_THROTTLE_MS) {
      showFilterPages(chan);
      lastRedrawTime = now;
      if (pageChanged) {
        lastFilterPage[chan] = filterPage[chan];
      }
    }
  }
}

void showFilterPages(uint8_t chan) {
  FastLEDclear();
  

  slider(filterPage[chan]);


  unsigned long now = millis();


  // ─── overlay corner values ───────────────────
  //for (uint8_t i = 0; i < 4; ++i) {
  if (now - lastInteraction < FILTER_INTERACTION_TIMEOUT && lastChangedEncoder >= 0 && lastChangedEncoder < 4) {
    uint8_t val = constrain(currentMode->pos[lastChangedEncoder],
                            0, sliderDef[chan][filterPage[chan]][lastChangedEncoder].maxValue);
    drawCornerValueCustom(lastChangedEncoder, val, sliderDef[chan][filterPage[chan]][lastChangedEncoder]);
  }

  FastLEDshow();
}




void drawSliderValue(uint8_t x0, uint8_t x1, uint8_t val) {
  char buf[3];
  int len = snprintf(buf, sizeof(buf), "%u", val);  // 1 or 2 chars

  // estimate font width per character (e.g. 3px)
  const uint8_t charW = 3;
  const uint8_t totalW = charW * len;

  // center in between x0..x1
  int cx = (x0 + x1) / 2;
  int tx = cx - totalW / 2;
  // clamp so that text never goes past 0..16
  if (tx < 1) tx = 1;
  if (tx + totalW > 16) tx = 16 - totalW;

  // CLEAR just that span on row 6 (or whatever row you chose)
  for (int x = 1; x <= 16; ++x) {
    light(x, 6, CRGB::Black);
    light(x, 7, CRGB::Black);
    light(x, 8, CRGB::Black);
    light(x, 9, CRGB::Black);
    light(x, 10, CRGB::Black);
  }


  drawText(buf, tx, 6, CRGB::White);
}


void showFilterNames(uint8_t chan) {
    // Clear the entire headline area (y=12) before drawing to prevent brightness accumulation
    for (uint8_t x = 1; x <= 16; ++x) {
        light(x, 12, CRGB::Black);
        light(x, 13, CRGB::Black);
        light(x, 14, CRGB::Black);
        light(x, 15, CRGB::Black);
        light(x, 16, CRGB::Black);
    }
    
    uint8_t page = filterPage[chan];
    for (uint8_t i = 0; i < 4; ++i) {
        const SliderDefEntry& def = sliderDef[chan][page][i];
        if (def.arr == ARR_NONE && def.idx == -1) continue;
        char abbrev[2] = { def.name[0], '\0' };
        CRGB color = filterColors[page][i];
        if (def.arr == ARR_FILTER && def.idx == REVERB && !channelHasFreeverb(chan)) {
          color = CRGB(4, 4, 4);
        }
        drawText(abbrev, sliderCols[i][0], 12, color);
    }
}

extern SliderDefEntry sliderDef[NUM_CHANNELS][4][4];
extern uint8_t filterPage[NUM_CHANNELS];

void updateFilterEncoderColors() {
  if (currentMode == &filterMode) {
    uint8_t chan = GLOB.currentChannel;
    uint8_t page = filterPage[chan];
    
    for (int i = 0; i < NUM_ENCODERS; i++) {
      CRGB color;
      
      // Check if this encoder controls the default fast filter
      bool isDefaultFast = false;
      if (chan < NUM_CHANNELS) {
        if (sliderDef[chan][page][i].arr == defaultFastFilter[chan].arr &&
            sliderDef[chan][page][i].idx == defaultFastFilter[chan].idx) {
          isDefaultFast = true;
        }
      }
      
      bool reverbDisabled = (sliderDef[chan][page][i].arr == ARR_FILTER &&
                             sliderDef[chan][page][i].idx == REVERB &&
                             !channelHasFreeverb(chan));

      if (isDefaultFast) {
        color = CRGB(255, 255, 0); // Bright yellow for default fast filter
        if (reverbDisabled) {
          color = CRGB(16, 16, 0);
        }
      } else {
        color = filterColors[page][i]; // Normal filter page color
        if (reverbDisabled) {
          color = CRGB(8, 8, 8);
        }
      }
      
      Encoder[i].writeRGBCode(CRGBToUint32(color));
    }
  }
}

// Handle touch-triggered page switching and conditional update propagation
