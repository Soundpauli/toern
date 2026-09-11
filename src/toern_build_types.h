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
