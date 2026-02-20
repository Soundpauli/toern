// LED Strip Visualization Module
// Ripple effects based on triggered notes/channels with audio reactivity
//
// Uses PIN 24 as a separate data line for the LED strip (independent from matrix on PIN 17).
// The strip can be connected externally via the onboard connector.

#include <FastLED.h>

// Configuration
#define LED_STRIP_PIN 24  // Separate pin for LED strip
#define LED_STRIP_DEFAULT_LENGTH 256
#define LED_STRIP_MAX_LENGTH 256
#define MAX_RIPPLES 16
#define RIPPLE_DECAY_RATE 0.92f
#define RIPPLE_SPEED 1.5f  // Fixed speed - LEDs per frame
#define AUDIO_SENSITIVITY 0.3f
#define AUDIO_MIN_THRESHOLD 0.01f

// Separate LED array for the strip (on PIN 24)
DMAMEM CRGB stripLeds[LED_STRIP_MAX_LENGTH];

// LED strip state
static int ledStripLength = LED_STRIP_DEFAULT_LENGTH;
static bool ledStripEnabled = true;

// Ripple structure
struct Ripple {
  bool active;
  float position;      // Position along strip (0.0 to stripLength)
  float intensity;    // Current intensity (0.0 to 1.0)
  uint8_t channels[4]; // Up to 4 channels that triggered this ripple (for color blending)
  uint8_t channelCount; // Number of active channels in this ripple
  float speed;        // Speed of ripple expansion
  unsigned long startTime;
};

static Ripple ripples[MAX_RIPPLES];
static float audioLevel = 0.0f;
static float smoothedAudioLevel = 0.0f;

// Forward declarations
extern bool isNowPlaying;
extern CRGB col[];

// Audio peak analyzer for output level (will be added to audioinit.h)
extern AudioAnalyzePeak peakOutput;

// Initialize LED strip visualization
// Call this AFTER FastLED.addLeds for the strip has been set up in setup()
void initLedStrip() {
  // Constrain strip length to available space
  if (ledStripLength > LED_STRIP_MAX_LENGTH) {
    ledStripLength = LED_STRIP_MAX_LENGTH;
  }
  
  // Clear all ripples
  for (int i = 0; i < MAX_RIPPLES; i++) {
    ripples[i].active = false;
    ripples[i].intensity = 0.0f;
    ripples[i].channelCount = 0;
  }
  
  // Clear strip LEDs
  for (int i = 0; i < ledStripLength; i++) {
    stripLeds[i] = CRGB::Black;
  }
  
  audioLevel = 0.0f;
  smoothedAudioLevel = 0.0f;
}

// Set LED strip length (called from menu or settings)
void setLedStripLength(int length) {
  ledStripLength = constrain(length, 1, LED_STRIP_MAX_LENGTH);
}

// Get LED strip length
int getLedStripLength() {
  return ledStripLength;
}

// Enable/disable LED strip visualization
void setLedStripEnabled(bool enabled) {
  ledStripEnabled = enabled;
  if (!enabled) {
    // Clear strip when disabled
    for (int i = 0; i < ledStripLength; i++) {
      stripLeds[i] = CRGB::Black;
    }
  }
}

// Get LED strip enabled state
bool getLedStripEnabled() {
  return ledStripEnabled;
}

// Trigger a ripple from a specific channel
// If a ripple was just created (within RIPPLE_MERGE_WINDOW_MS), add this channel to it instead
#define RIPPLE_MERGE_WINDOW_MS 50  // Merge channels triggered within 50ms

void triggerLedStripRipple(uint8_t channel) {
  if (!ledStripEnabled || !isNowPlaying) return;
  if (channel == 0 || channel > 16) return;  // Invalid channel
  
  // Check if there's a recent ripple we can add this channel to (for simultaneous triggers)
  unsigned long now = millis();
  int mergeSlot = -1;
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (ripples[i].active && 
        (now - ripples[i].startTime) < RIPPLE_MERGE_WINDOW_MS &&
        ripples[i].position < 5.0f &&  // Only merge if ripple is still near the start
        ripples[i].channelCount < 4) {  // Max 4 channels per ripple
      // Check if this channel is already in the ripple
      bool alreadyIncluded = false;
      for (int j = 0; j < ripples[i].channelCount; j++) {
        if (ripples[i].channels[j] == channel) {
          alreadyIncluded = true;
          break;
        }
      }
      if (!alreadyIncluded) {
        mergeSlot = i;
        break;
      }
    }
  }
  
  if (mergeSlot >= 0) {
    // Add channel to existing ripple
    ripples[mergeSlot].channels[ripples[mergeSlot].channelCount] = channel;
    ripples[mergeSlot].channelCount++;
    // Boost intensity slightly when multiple channels merge
    ripples[mergeSlot].intensity = min(1.0f, ripples[mergeSlot].intensity + 0.1f);
    return;
  }
  
  // Find an available ripple slot for a new ripple
  int slot = -1;
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (!ripples[i].active) {
      slot = i;
      break;
    }
  }
  
  // If all slots are full, reuse the oldest one
  if (slot == -1) {
    unsigned long oldestTime = ripples[0].startTime;
    slot = 0;
    for (int i = 1; i < MAX_RIPPLES; i++) {
      if (ripples[i].startTime < oldestTime) {
        oldestTime = ripples[i].startTime;
        slot = i;
      }
    }
  }
  
  // Initialize new ripple with fixed speed
  ripples[slot].active = true;
  ripples[slot].position = 0.0f;  // Start at beginning of strip
  ripples[slot].intensity = 1.0f;
  ripples[slot].channels[0] = channel;
  ripples[slot].channelCount = 1;
  ripples[slot].speed = RIPPLE_SPEED;  // Fixed speed for smooth travel
  ripples[slot].startTime = millis();
}

// Update LED strip visualization (call from main loop)
// Optimizations #3 and #8: Throttle updates when idle, rate-limit to fixed FPS
void updateLedStrip() {
  // Optimization #3: Track previous state and only clear on state change
  static bool lastLedStripEnabled = ledStripEnabled;
  static bool lastIsNowPlaying = isNowPlaying;
  static bool stripClearedOnStateChange = false;
  
  // Check for state changes
  bool stateChanged = (ledStripEnabled != lastLedStripEnabled) || (isNowPlaying != lastIsNowPlaying);
  if (stateChanged) {
    // Clear strip only when state changes (enabled→disabled, playing→paused)
    if (!ledStripEnabled || !isNowPlaying) {
      for (int i = 0; i < ledStripLength; i++) {
        stripLeds[i] = CRGB::Black;
      }
      stripClearedOnStateChange = true;
    }
    lastLedStripEnabled = ledStripEnabled;
    lastIsNowPlaying = isNowPlaying;
  }
  
  // Optimization #3: Skip entirely when disabled and not playing (no need to update)
  if (!ledStripEnabled || !isNowPlaying) {
    // Strip already cleared on state change, just return
    return;
  }
  
  // Optimization #8: Rate-limit to fixed FPS (30 Hz = 33ms, matching RefreshTime)
  static unsigned long lastUpdateTime = 0;
  const unsigned long UPDATE_INTERVAL_MS = 33;  // 30 FPS
  unsigned long now = millis();
  if (now - lastUpdateTime < UPDATE_INTERVAL_MS) {
    return;  // Skip update if not enough time has passed
  }
  lastUpdateTime = now;
  
  // Optimization #8: Early return if no active ripples (saves CPU)
  bool hasActiveRipples = false;
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (ripples[i].active) {
      hasActiveRipples = true;
      break;
    }
  }
  
  // Read audio level (throttled to ~10ms, different from visual update rate)
  static unsigned long lastAudioRead = 0;
  if (millis() - lastAudioRead > 10) {  // Read every ~10ms
    lastAudioRead = millis();
    if (peakOutput.available()) {
      audioLevel = peakOutput.read();
      // Smooth the audio level
      smoothedAudioLevel = smoothedAudioLevel * 0.7f + audioLevel * 0.3f;
    } else {
      // Decay if no audio available
      smoothedAudioLevel *= 0.95f;
    }
  }
  
  // If no active ripples and no significant audio, skip rendering (but still update audio)
  if (!hasActiveRipples && smoothedAudioLevel < AUDIO_MIN_THRESHOLD) {
    // Clear strip if there's nothing to show
    for (int i = 0; i < ledStripLength; i++) {
      stripLeds[i] = CRGB::Black;
    }
    return;
  }
  
  // Clear strip first (only when we're actually rendering)
  for (int i = 0; i < ledStripLength; i++) {
    stripLeds[i] = CRGB::Black;
  }
  
  // Update and render ripples
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (!ripples[i].active) continue;
    
    // Update ripple position with fixed speed
    ripples[i].position += ripples[i].speed;
    
    // Decay intensity
    ripples[i].intensity *= RIPPLE_DECAY_RATE;
    
    // Check if ripple has passed the end or faded out
    if (ripples[i].position >= ledStripLength || ripples[i].intensity < 0.01f) {
      ripples[i].active = false;
      ripples[i].channelCount = 0; // Clear channels
      continue;
    }
    
    // Get blended color from all channels in this ripple
    CRGB baseColor = CRGB::Black;
    uint8_t activeChannelCount = 0;
    
    for (int c = 0; c < ripples[i].channelCount; c++) {
      uint8_t ch = ripples[i].channels[c];
      if (ch > 0 && ch <= 16) {
        CRGB channelColor = col[ch];
        
        // Handle channels that are intentionally black (9, 10, 12 are unused)
        if (channelColor.r == 0 && channelColor.g == 0 && channelColor.b == 0) {
          if (ch == 9 || ch == 10 || ch == 12) {
            // Unused channels: use dim white so ripple is visible
            channelColor = CRGB(30, 30, 30);
          } else {
            // Unexpected black channel: use channel-based hue for visibility
            uint8_t hue = (ch * 16) % 255;
            channelColor = CHSV(hue, 255, 255);
          }
        }
        
        // Blend colors together (additive blending for multiple channels)
        baseColor.r = min(255, baseColor.r + channelColor.r);
        baseColor.g = min(255, baseColor.g + channelColor.g);
        baseColor.b = min(255, baseColor.b + channelColor.b);
        activeChannelCount++;
      }
    }
    
    // If no valid channels, use white fallback
    if (activeChannelCount == 0) {
      baseColor = CRGB::White;
    } 
    // Removed normalization to maximize brightness (additive mixing is already clamped to 255 above)
    /* else if (activeChannelCount > 1) {
      // Normalize when multiple channels to prevent over-brightness
      baseColor.r = baseColor.r / activeChannelCount;
      baseColor.g = baseColor.g / activeChannelCount;
      baseColor.b = baseColor.b / activeChannelCount;
    } */
    
    // Apply audio reactivity to intensity
    float audioBoost = 1.0f;
    if (smoothedAudioLevel > AUDIO_MIN_THRESHOLD) {
      audioBoost = 1.0f + (smoothedAudioLevel * AUDIO_SENSITIVITY);
      audioBoost = constrain(audioBoost, 1.0f, 2.0f);
    }
    
    // Calculate effective intensity with audio boost
    float effectiveIntensity = ripples[i].intensity * audioBoost;
    effectiveIntensity = constrain(effectiveIntensity, 0.0f, 1.0f);
    
    // Render ripple as expanding wave
    int centerPos = (int)ripples[i].position;
    float width = ripples[i].intensity * 8.0f;  // Ripple width
    
    for (int j = 0; j < ledStripLength; j++) {
      float dist = abs((float)j - ripples[i].position);
      
      if (dist < width) {
        // Calculate brightness based on distance from center
        float brightness = 1.0f - (dist / width);
        brightness = brightness * brightness;  // Square for smoother falloff
        brightness *= effectiveIntensity;
        
        // Apply to LED
        if (j < ledStripLength) {
          CRGB pixelColor = baseColor;
          pixelColor.nscale8((uint8_t)(brightness * 255.0f));
          
          // Blend with existing color (for multiple ripples)
          stripLeds[j] += pixelColor;
        }
      }
    }
  }
  
  // Apply overall audio level as background glow
  if (smoothedAudioLevel > AUDIO_MIN_THRESHOLD) {
    float glowIntensity = smoothedAudioLevel * 0.1f;  // Subtle background glow
    for (int i = 0; i < ledStripLength; i++) {
      // Add subtle white glow based on audio
      CRGB glow = CRGB((uint8_t)(glowIntensity * 50.0f), 
                       (uint8_t)(glowIntensity * 50.0f), 
                       (uint8_t)(glowIntensity * 50.0f));
      stripLeds[i] += glow;
    }
  }
  
  // Strip LEDs will be shown by FastLEDshow() in main loop
  // FastLED.show() automatically shows all controllers
}

// Hook into note triggers (call from playNote or triggerGridNote)
// Note: This is called even when channels are muted, so ripples will show regardless of mute state
void onNoteTriggered(uint8_t channel) {
  if (ledStripEnabled && isNowPlaying && channel > 0 && channel <= 16) {
    triggerLedStripRipple(channel);
  }
}
