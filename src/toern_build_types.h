#pragma once

struct MenuPage; // Arduino-generated menu helper prototypes

#define SAMPLE_BROWSER_NAME_MAX 65

struct HarmonicAnalysis {
  bool hasRoot = false;
  bool hasThird = false;
  bool hasFifth = false;
  bool hasSeventh = false;
  int rootNote = 0;
  bool isMajor = false;
  bool isMinor = false;
};

enum class CompanionRole : uint8_t {
  Kick,
  Snare,
  ClosedHat,
  Clap,
  Tom,
  Bass,
  Keys,
  VocalsPad,
  Unknown
};

struct CompanionContext {
  uint16_t stepWeight[16] = {};
  uint32_t stepVelocity[16] = {};
  uint16_t kickWeight[16] = {};
  uint16_t snareWeight[16] = {};
  uint16_t clapWeight[16] = {};
  uint16_t hatWeight[16] = {};
  uint16_t tomWeight[16] = {};
  uint16_t bassWeight[16] = {};
  uint16_t harmonicWeight[16] = {};
  uint16_t rowWeight[17] = {};
  uint16_t pitchClassWeight[12] = {};
  uint16_t barPitchClassWeight[4][12] = {};
  uint16_t barBassPcWeight[4][12] = {};
  uint8_t channelHomeRow[16] = {};
  uint8_t channelMeanVel[16] = {};
  uint16_t channelHitCount[16] = {};
  uint16_t kickMask16 = 0;
  uint16_t snareMask16 = 0;
  uint16_t bassMask16 = 0;
  uint16_t harmonicMask16 = 0;
  uint8_t syncopationScore = 0;  // 0..100 ratio of off-beat energy
  uint32_t totalWeight = 0;
  uint16_t sourcePages = 0;
  uint8_t rootRow = 1;
  bool isMinor = false;
  bool empty = true;
};

// Shared key / scale / bar-degree progression for companion generation.
struct CompanionHarmony {
  uint8_t rootPc = 0;       // 0..11 pitch class
  bool isMinor = true;
  uint8_t degree[4] = {0, 5, 2, 6};  // one chord degree per harmonic slot
  uint8_t chordRootPc[4] = {0, 0, 0, 0};  // actual or derived root pc per 4-step bar
  bool barHasContext[4] = {false, false, false, false};
};

struct CompanionGroove {
  uint8_t hatPulses;
  uint8_t hatRotation;
  uint8_t clapPulses;
  uint8_t clapRotation;
  uint8_t tomRotation;
  int8_t snareShiftA;
  int8_t snareShiftB;
  int8_t secondHalfShift;
  uint8_t secondHalfRotation;
};
