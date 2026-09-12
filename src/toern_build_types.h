#pragma once

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

struct BasePagePattern {
  bool hasNotes[16];
  int noteRows[16];
  int velocities[16];
  int stepCount;
  float density;
  int mostCommonRow;
  int rhythmPattern[16];
  bool isRhythmic;
  bool isMelodic;
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
