unsigned long lastEncoderChange[4] = { 0, 0, 0, 0 };

int8_t lastChangedEncoder = -1;
unsigned long lastInteraction = millis();  // global timestamp


bool showingAny = false;
bool showSingleFilter[4] = { false, false, false, false };

// 1) Define your per‐page, per‐slot filter names:
static const char* filterNames[4][4] = {
  { "PASS", "FREQ", "REVB", "BITC" },  // page 0
  { "CUTF", "RESZ", "FREQ", "OCTV" },  // page 1
  { "WAVE", "INST", "TUNE", "LEN" },   // page 2
  { "ATTC", "DCAY", "SUST", "RLSE" }   // page 3
};



static const CRGB filterColors[4][4] = {
  // ─── Page 0: RED to ORANGE (Hue 0–30)
  {
    CHSV(0, 255, 255),   // Red
    CHSV(10, 255, 255),  // Red-Orange
    CHSV(20, 255, 255),  // Orange
    CHSV(30, 255, 255)   // Orange-Yellow
  },

  // ─── Page 1: BLUE to CYAN (Hue 160–200)
  {
    CHSV(160, 255, 255),  // Blue
    CHSV(170, 255, 255),  // Blue-Cyan
    CHSV(185, 255, 255),  // Cyan
    CHSV(200, 255, 255)   // Aqua
  },

  // ─── Page 2: GREEN to LIME (Hue 90–130)
  {
    CHSV(90, 255, 255),   // Lime
    CHSV(100, 255, 255),  // Yellow-Green
    CHSV(115, 255, 255),  // Green
    CHSV(130, 255, 255)   // Blue-Green
  },

  {
    CHSV(220, 255, 255),  // Indigo Violet
    CHSV(235, 255, 255),  // Purple
    CHSV(250, 255, 255),  // Orchid
    CHSV(255, 255, 150)   // Fuchsia
  }
};

uint8_t scaleToDisplay(const SliderMeta& meta, uint8_t val) {
  return (meta.displayRange < meta.maxValue) ? constrain(mapf(val, 0, meta.maxValue, 0, meta.displayRange - 1), 0, meta.displayRange - 1) : val;
}

uint8_t scaleFromDisplay(const SliderMeta& meta, uint8_t val) {
  return (meta.displayRange < meta.maxValue) ? constrain(mapf(val, 0, meta.displayRange - 1, 0, meta.maxValue), 0, meta.maxValue) : val;
}


#define MAX_FILTER_RESOLUTION 32
#define FILTER_BYPASS MAX_FILTER_RESOLUTION
#define FILTER_WET 0
#define FILTER_MID (MAX_FILTER_RESOLUTION / 2)
#define PARAM_COUNT 6  // Number of valid params, must include RELEASE (index 5)

// Read raw 0..MAX_FILTER_RESOLUTION setting directly
uint8_t readSetting(SettingArray arr, int8_t idx, uint8_t chan) {
  if (idx < 0) return 0;
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


// Blended LOW+HIGH readback for page 0 slider 0
uint8_t readLowHighBlend(uint8_t chan) {
  int lp = SMP.filter_settings[chan][LOWPASS];
  int hp = SMP.filter_settings[chan][HIGHPASS];
  int val = 0;

  if (hp == FILTER_BYPASS) {
    val = mapf(lp, FILTER_WET, FILTER_BYPASS, 0, FILTER_MID);
  } else if (lp == FILTER_BYPASS) {
    val = mapf(hp, FILTER_BYPASS, FILTER_WET, FILTER_MID, MAX_FILTER_RESOLUTION);
  } else {
    //Serial.printf("[Debug] readLowHighBlend: ambiguous state LP=%d HP=%d\n", lp, hp);
    val = FILTER_MID;
  }

  return constrain(val, 0, MAX_FILTER_RESOLUTION);
}
void drawCornerValueCustom(uint8_t encoderIndex, uint8_t val, const SliderMeta& meta) {
  const uint8_t x = (encoderIndex < 2) ? 10 : 2;
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
  CRGB baseColor = filterColors[filterPage][encoderIndex];
  CRGB dimmed = baseColor;
  dimmed.nscale8(64);
  uint8_t blendVal = mapf(val, 0, meta.maxValue, 0, 255);
  CRGB textColor = blend(CRGB::Black, dimmed, blendVal);

  drawText(buf, x, y, textColor);
}


void drawFireBackground(uint16_t timeBase = millis()) {
  for (uint8_t x = 0; x < 16; ++x) {
    for (uint8_t y = 0; y < 16; ++y) {
      // Use a unique offset for each coordinate
      uint8_t noise = inoise8(x * 12, y * 24, timeBase / 4);  // smooth flow

      // Map noise to fire hue (0–30) and scale brightness low
      uint8_t brightness = scale8(noise, 24);   // dim! (0–255 scaled to 0–24)
      uint8_t fireHue = mapf(y, 0, 15, 0, 25);  // hue gradient from red to yellow

      // Optional: fade bottom brighter
      if (y > 12) brightness = scale8(brightness, 150);  // boost base

      CRGB fireColor = CHSV(fireHue, 255, brightness);
      light(x + 1, y + 1, fireColor);  // your coords are 1-based
    }
  }
}
void drawVerticalSlider(uint8_t x0, uint8_t x1, uint8_t val, uint8_t maxVal, CRGB baseColor, bool lowHighSlider = false, uint8_t sliderIndex = 0) {
  const uint8_t displayResolution = 10;
  uint8_t displayVal = mapf(val, 0, maxVal, 1, displayResolution);
  displayVal = constrain(displayVal, 1, displayResolution);

  // ─── Animated Pulse Modulation ───
  // Offset each slider with a phase shift
  uint8_t pulsePhase = millis() / 8 + sliderIndex * 64;
  uint8_t pulse = sin8(pulsePhase);                      // 0–255 sine wave
  uint8_t brightnessMod = mapf(pulse, 0, 255, 32, 128);  // You can tune min/max for subtlety

  // ─── Dim and blend base color by value ───
  CRGB dimmed = baseColor;
  dimmed.nscale8(brightnessMod);  // animated brightness dim
  uint8_t blendVal = mapf(val, 0, maxVal, 0, 255);
  CRGB color = blend(CRGB::Black, dimmed, blendVal);

  for (uint8_t y = 1; y <= displayResolution; ++y) {
    CRGB c;
    if (y == displayVal) {
      c = CRGB::White;
    } else if (lowHighSlider) {
      c = (y < displayVal) ? CRGB::Red : CRGB::Yellow;
    } else {
      c = (y < displayVal) ? color : CRGB::Black;
    }

    light(x0, y, c);
    light(x1, y, c);
  }
}


void initSliders(uint8_t page) {
  showFilterNames();

  Serial.print("[Debug] Switched to slider page: ");
  Serial.println(page);
  uint8_t chan = SMP.currentChannel;

  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[page][i];
    const auto& meta = sliderMeta[page][i];
    uint8_t val = 0;

    if (d.arr != ARR_NONE) {
      if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) {
        Serial.printf("[Warning] init skip invalid ARR_PARAM idx %d\n", d.idx);
        continue;
      }
      if (page == 0 && i == 0) {
        val = readLowHighBlend(chan);
      } else {
        val = readSetting(d.arr, d.idx, chan);
      }
    }

    val = constrain(val, 0, meta.maxValue);
    Serial.printf("[Debug] Encoder %u page %u channel %u -> value %u\n", i, page, chan, val);
    currentMode->pos[i] = val;
    Encoder[i].writeMax((int32_t)(meta.maxValue));
    //Encoder[i].writeMax((int32_t)meta.maxValue); // Prevent encoder going out of bounds
    Encoder[i].writeCounter((int32_t)val);
  }

  showFilterPages();
}


void slider(uint8_t page) {
  unsigned long now = millis();
  bool activeInteraction = (now - lastInteraction < 600);

  for (uint8_t i = 0; i < 4; ++i) {
    const auto& meta = sliderMeta[page][i];
    uint8_t val = constrain(currentMode->pos[i], 0, meta.maxValue);
    bool isLowHigh = (page == 0 && i == 0);

    // ❌ Skip drawing if interaction is active and this isn't the last touched encoder
    if (activeInteraction && i != lastChangedEncoder) {
      // Clear the slider area so it disappears
      for (uint8_t y = 1; y <= 10; ++y) {
        light(sliderCols[i][0], y, CRGB::Black);
        light(sliderCols[i][1], y, CRGB::Black);
      }
      continue;
    }

    // ✅ Draw slider
    //drawVerticalSlider( sliderCols[i][0], sliderCols[i][1], val, meta.maxValue, filterColors[page][i], isLowHigh);
    drawVerticalSlider(
      sliderCols[i][0],
      sliderCols[i][1],
      val,
      meta.maxValue,
      filterColors[page][i],
      isLowHigh,
      i  // <- for animation phase offset
    );
    showSingleFilter[i] = false;
    if (i == lastChangedEncoder && activeInteraction) {
      drawSliderName(sliderCols[i][0],
                     sliderCols[i][1],
                     filterNames[page][i],
                     filterColors[page][i]);
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
    showFilterNames();
  }
}

void printSliderDefTarget(uint8_t page, uint8_t encoder) {
  const auto& def = sliderDef[page][encoder];
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
    idxName = filterNames[page][encoder];
  }

  Serial.printf("{%s, %s}\n", arrName, idxName);
}


void setDefaultFilterFromSlider(uint8_t page, uint8_t encoder) {
  auto& def = sliderDef[page][encoder];
  uint8_t chan = SMP.currentChannel;

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

    default:
      Serial.printf("[Skip] Not assignable default: arr=%d\n", def.arr);
      break;
  }

  Serial.print("[Default] Set defaultFastFilter: ");
  printSliderDefTarget(page, encoder);
}

void processAdjustments_new(uint8_t page) {
  Serial.print("[Debug] processAdjustments_new page: ");
  Serial.println(page);

  uint8_t chan = SMP.currentChannel;

  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[page][i];
    if (d.arr == ARR_NONE) continue;
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) {
      Serial.printf("[Warning] Skipping ARR_PARAM idx %d - out of bounds\n", d.idx);
      continue;
    }

    const auto& meta = sliderMeta[page][i];
    uint8_t rawVal = constrain(currentMode->pos[i], 0, meta.maxValue);
    uint8_t val = scaleToDisplay(meta, rawVal);

    uint8_t prev = (page == 0 && i == 0)
                     ? readLowHighBlend(chan)
                     : readSetting(d.arr, d.idx, chan);

    if (val == prev) continue;

    Serial.printf("[Debug] Applying adj page %u slot %u arr %d idx %d raw %u -> display %u\n",
                  page, i, d.arr, d.idx, rawVal, val);

    float mappedVal = 0;

    switch (d.arr) {
      case ARR_FILTER:
        if (page == 0 && i == 0) {
          int lp = 0, hp = 0;
          if (val <= FILTER_MID) {
            lp = mapf(val, 0, FILTER_MID, FILTER_WET, FILTER_BYPASS);
            hp = FILTER_BYPASS;
          } else {
            lp = FILTER_BYPASS;
            hp = mapf(val, FILTER_MID, MAX_FILTER_RESOLUTION, FILTER_BYPASS, FILTER_WET);
          }
          SMP.filter_settings[chan][LOWPASS] = constrain(lp, 0, MAX_FILTER_RESOLUTION);
          SMP.filter_settings[chan][HIGHPASS] = constrain(hp, 0, MAX_FILTER_RESOLUTION);
          Serial.printf("[Debug] LP=%d HP=%d\n", lp, hp);
        } else {
          SMP.filter_settings[chan][d.idx] = val;
          mappedVal = processFilterAdjustment(d.idx, chan, i);
          updateFilterValue(d.idx, chan, mappedVal);
        }
        break;

      case ARR_SYNTH:
        SMP.synth_settings[chan][d.idx] = val;
        mappedVal = processSynthAdjustment(d.idx, chan, i);
        updateSynthValue(d.idx, chan, mappedVal);
        break;

      case ARR_DRUM:
        SMP.drum_settings[chan][d.idx] = val;
        mappedVal = processDrumAdjustment(d.idx, chan, i);
        updateDrumValue(d.idx, chan, mappedVal);
        break;

      case ARR_PARAM:
        if (SMP.param_settings[chan][d.idx] == val) continue;
        SMP.param_settings[chan][d.idx] = val;
        // Optional: process/update param visuals here
        break;

      default:
        break;
    }
  }
}

// Apply adjustments back to SMP arrays based on sliders and current page (only on change)
void processAdjustments_new2(uint8_t page) {
  Serial.print("[Debug] processAdjustments_new page: ");
  Serial.println(page);
  uint8_t chan = SMP.currentChannel;
  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[page][i];
    if (d.arr == ARR_NONE) continue;

    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) {
      Serial.printf("[Warning] Skipping ARR_PARAM idx %d - out of bounds\n", d.idx);
      continue;
    }

    //    uint8_t val = constrain(currentMode->pos[i], 0, MAX_FILTER_RESOLUTION);

    const auto& meta = sliderMeta[page][i];
    uint8_t val = constrain(currentMode->pos[i], 0, meta.maxValue);

    uint8_t prev = (page == 0 && i == 0) ? readLowHighBlend(chan) : readSetting(d.arr, d.idx, chan);
    if (val == prev) continue;

    Serial.printf("[Debug] Applying adj page %u slot %u arr %d idx %d raw %u\n", page, i, d.arr, d.idx, val);
    float mappedVal = 0;

    switch (d.arr) {
      case ARR_FILTER:
        if (page == 0 && i == 0) {
          int lp = 0, hp = 0;
          if (val <= FILTER_MID) {
            lp = mapf(val, 0, FILTER_MID, FILTER_WET, FILTER_BYPASS);
            hp = FILTER_BYPASS;
          } else {
            lp = FILTER_BYPASS;
            hp = mapf(val, FILTER_MID, MAX_FILTER_RESOLUTION, FILTER_BYPASS, FILTER_WET);
          }
          SMP.filter_settings[chan][LOWPASS] = constrain(lp, 0, MAX_FILTER_RESOLUTION);
          SMP.filter_settings[chan][HIGHPASS] = constrain(hp, 0, MAX_FILTER_RESOLUTION);
          Serial.printf("[Debug] LP=%d HP=%d\n", lp, hp);
        } else {
          SMP.filter_settings[chan][d.idx] = val;
          mappedVal = processFilterAdjustment(d.idx, chan, i);
          updateFilterValue(d.idx, chan, mappedVal);
        }
        break;
      case ARR_SYNTH:
        SMP.synth_settings[chan][d.idx] = val;
        mappedVal = processSynthAdjustment(d.idx, chan, i);
        updateSynthValue(d.idx, chan, mappedVal);
        break;
      case ARR_DRUM:
        SMP.drum_settings[chan][d.idx] = val;
        mappedVal = processDrumAdjustment(d.idx, chan, i);
        updateDrumValue(d.idx, chan, mappedVal);
        break;
      case ARR_PARAM:
        if (SMP.param_settings[chan][d.idx] == val) continue;
        SMP.param_settings[chan][d.idx] = val;
        //mappedVal = processParameterAdjustment(d.idx, chan);
        //updateParameterValue(d.idx, chan, mappedVal);
        break;
      default:
        break;
    }
  }
}

// Handle touch-triggered page switching and conditional update propagation
void setNewFilters() {
  int touchValue = fastTouchRead(SWITCH_2);
  static bool lastTouch = false;
  bool currTouch = (touchValue > touchThreshold);
  if (currTouch && !lastTouch) {
    filterPage = (filterPage + 1) % 4;
    initSliders(filterPage);
  }
  lastTouch = currTouch;

  bool anyChanged = false;
  uint8_t chan = SMP.currentChannel;
  for (uint8_t i = 0; i < 4; ++i) {
    auto& d = sliderDef[filterPage][i];
    if (d.arr == ARR_NONE) continue;
    if (d.arr == ARR_PARAM && d.idx >= PARAM_COUNT) continue;
    uint8_t val = constrain(currentMode->pos[i], 0, MAX_FILTER_RESOLUTION);
    uint8_t prev = (filterPage == 0 && i == 0) ? readLowHighBlend(chan) : readSetting(d.arr, d.idx, chan);
    if (val != prev) {
      showSingleFilter[i] = true;

      drawSliderName(sliderCols[i][0],
                     sliderCols[i][1],
                     filterNames[filterPage][i],
                     filterColors[filterPage][i]);


      anyChanged = true;
      lastEncoderChange[i] = millis();  // store activity timestamp
      lastInteraction = millis();
      lastChangedEncoder = i;
    }
  }

  if (anyChanged) {
    processAdjustments_new(filterPage);
    showFilterPages();
  }

  showFilterPages();
}

void showFilterPages() {
  FastLED.clear();
  //drawFireBackground();  // 🔥 Background first

  slider(filterPage);


  unsigned long now = millis();


  // ─── overlay corner values ───────────────────
  //for (uint8_t i = 0; i < 4; ++i) {
  if (now - lastInteraction < 600) {
    uint8_t val = constrain(currentMode->pos[lastChangedEncoder],
                            0, sliderMeta[filterPage][lastChangedEncoder].maxValue);
    drawCornerValueCustom(lastChangedEncoder, val, sliderMeta[filterPage][lastChangedEncoder]);
  }

  FastLED.show();
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


void showFilterNames() {
  for (uint8_t i = 0; i < 4; ++i) {
    // pick the first char of the name as your abbreviation
    char abbrev[2] = { filterNames[filterPage][i][0], '\0' };
    drawText(abbrev,
             sliderCols[i][0],
             12,
             filterColors[filterPage][i]);
  }
}
