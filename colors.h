
static const CRGB filterColors[5][4] = {
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
  },

  // ─── Page 4: WHITE for all sliders
  {
    CRGB(255,255,255),
    CRGB(255,255,255),
    CRGB(255,255,255),
    CRGB(255,255,255)
  }
};


const CRGB col_Folder[] = {
  CRGB(139, 0, 0),      // Dark Red
  CRGB(255, 69, 0),     // Burnt Orange
  CRGB(255, 255, 0),    // Gold
  CRGB(0, 139, 0),      // Green
  CRGB(0, 140, 130),    // Türkis
  CRGB(0, 0, 255),      // Blau
  CRGB(140, 0, 120),    // Lila
  CRGB(220, 100, 100),  // rosa
  CRGB(0, 255, 70),     // s1s
  CRGB(255, 50, 50)     // s2#
};



// Color scheme 0: Default (original)
static const CRGB col_0[] = {
  CRGB(0, 0, 0),
  CRGB(255, 0, 0),      // Dark Red
  CRGB(255, 69, 0),     // Burnt Orange
  CRGB(255, 255, 0),    // Gold
  CRGB(0, 200, 0),      // Green
  CRGB(0, 255, 255),    // Türkis
  CRGB(0, 0, 255),      // Blau
  CRGB(255, 0, 255),    // Lila
  CRGB(220, 100, 100),  // rosa
  CRGB(0, 0, 0),
  CRGB(0, 0, 0),  //s4
  CRGB(120, 120, 120),      //s3
  CRGB(0, 0, 0),
  CRGB(0, 255, 70),  // s2
  CRGB(255, 50, 50)  // s1
};

// Color scheme 1: Strong hue gradient (voices 1-8), fixed blacks/white (9-12), warm accents (13-14)
static const CRGB col_1[] = {
  CRGB(0, 0, 0),
  CRGB(0, 0, 180),     // Deep Blue
  CRGB(0, 60, 220),    // Blue
  CRGB(0, 180, 255),   // Cyan
  CRGB(0, 200, 120),   // Teal-Green
  CRGB(0, 220, 0),     // Green
  CRGB(120, 255, 0),   // Yellow-Green
  CRGB(255, 220, 0),   // Yellow
  CRGB(255, 140, 0),   // Orange
  CRGB(0, 0, 0),       // ch9 black
  CRGB(0, 0, 0),       // ch10 black
  CRGB(255, 255, 255), // ch11 white
  CRGB(0, 0, 0),       // ch12 black
  CRGB(255, 40, 40),   // Red (ch13)
  CRGB(255, 0, 180)    // Magenta (ch14)
};

// Color scheme 2: High contrast Red/Violet theme
static const CRGB col_2[] = {
  CRGB(0, 0, 0),
  CRGB(0, 35, 209),  // ch1
  CRGB(120, 0, 255),  // ch2
  CRGB(180, 0, 220),  // ch3
  CRGB(255, 102, 191),  // ch4
  CRGB(255, 0, 80),  // ch5
  CRGB(255, 54, 54),  // ch6
  CRGB(255, 106, 0),  // ch7
  CRGB(255, 217, 0),  // ch8
  CRGB(0, 0, 0),  // ch9
  CRGB(0, 0, 0),  // ch10
  CRGB(120, 120, 120),  // ch11 white
  CRGB(0, 0, 0),  // ch12
  CRGB(184, 0, 73),  // ch13
  CRGB(135, 0, 0),  // ch14
};

// Runtime color arrays (copied from selected scheme)
CRGB col[15];

// Normal: 70% brightness - Color scheme 0: Default (original)
static const CRGB col_base_0[] = {
  CRGB(0, 0, 0),
  CRGB(5, 0, 0),     // Dark Red
  CRGB(18, 4, 0),    // Orange
  CRGB(10, 10, 0),   // Gold
  CRGB(0, 4, 0),     // Green
  CRGB(0, 5, 5),     // Türkis
  CRGB(0, 0, 4),     // Blau
  CRGB(15, 1, 4),    // Lila
  CRGB(22, 12, 16),  // Rosa
  CRGB(0, 0, 0),
  CRGB(0, 0, 0), //s4
  CRGB(6, 6, 6),//s3
  CRGB(0, 0, 0),
  CRGB(0, 10, 4),  //s2
  CRGB(18, 4, 4)   // s1
};

// Normal: 70% brightness - Color scheme 1: Blue-ish
static const CRGB col_base_1[] = {
  CRGB(0, 0, 0),
  CRGB(0, 0, 4),   // Deep Blue
  CRGB(0, 2, 6),   // Blue
  CRGB(0, 6, 7),   // Cyan
  CRGB(0, 6, 3),   // Teal-Green
  CRGB(0, 6, 0),   // Green
  CRGB(3, 6, 0),   // Yellow-Green
  CRGB(6, 5, 0),   // Yellow
  CRGB(6, 3, 0),   // Orange
  CRGB(0, 0, 0),   // ch9 black
  CRGB(0, 0, 0),   // ch10 black
  CRGB(6, 6, 6),   // ch11 white (dim)
  CRGB(0, 0, 0),   // ch12 black
  CRGB(6, 1, 1),   // Red
  CRGB(6, 0, 4)    // Magenta
};

// Normal: 70% brightness - Color scheme 2: High contrast Red/Violet
static const CRGB col_base_2[] = {
  CRGB(0, 0, 0),
  CRGB(2, 0, 6),   // Purple-Blue
  CRGB(3, 0, 6),   // Violet
  CRGB(4, 0, 5),   // Magenta-Violet
  CRGB(6, 0, 4),   // Magenta-Red
  CRGB(6, 0, 2),   // Pink-Red
  CRGB(6, 0, 0),   // Red
  CRGB(6, 1, 0),   // Red-Orange
  CRGB(6, 2, 0),   // Deep Orange
  CRGB(0, 0, 0),   // ch9 black
  CRGB(0, 0, 0),   // ch10 black
  CRGB(6, 6, 6),   // ch11 white (dim)
  CRGB(0, 0, 0),   // ch12 black
  CRGB(7, 6, 5),   // Warm White
  CRGB(4, 5, 7)    // Cool White
};

// Runtime base color arrays (copied from selected scheme)
CRGB col_base[15];

// Global variable to track current color scheme (0=default, 1=blue-ish, 2=whitish)
extern uint8_t currentColorScheme;

// Flag to invalidate color cache when colors are updated via serial
extern volatile bool colorsUpdatedViaSerial;

// Function to copy selected color scheme to runtime arrays
inline void applyColorScheme(uint8_t scheme) {
  const CRGB* src_col;
  const CRGB* src_col_base;
  
  switch (scheme) {
    case 1:
      src_col = col_1;
      src_col_base = col_base_1;
      break;
    case 2:
      src_col = col_2;
      src_col_base = col_base_2;
      break;
    default:
      src_col = col_0;
      src_col_base = col_base_0;
      break;
  }
  
  // Copy arrays
  for (int i = 0; i < 15; i++) {
    col[i] = src_col[i];
    col_base[i] = src_col_base[i];
  }
}


uint32_t CRGBToUint32(CRGB color) {
  return ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | (uint32_t)color.b;
}



// ----- Phase 1: Compute Logo Pixel Color -----
// Use FastLED's CHSV for an x/y/time-based rainbow.
CRGB getLogoPixelColor(uint8_t gridX, uint8_t gridY, float timeFactor) {
  // Hue depends on (x + y) plus a time offset
  int rawHue = ((gridX + gridY) * 20 + (int)(timeFactor * 360)) % 360;
  uint8_t hue = (uint8_t)(rawHue * 255 / 360);
  return CHSV(hue, 255, 255);
}


CRGB uint32ToCRGBWithControls(uint32_t rgb, uint8_t saturation, uint8_t brightness) {
  uint8_t r = (rgb >> 16) & 0xFF;  // Extract red
  uint8_t g = (rgb >> 8) & 0xFF;   // Extract green
  uint8_t b = rgb & 0xFF;          // Extract blue

  // Create CRGB object from RGB values
  CRGB color = CRGB(r, g, b);

  // Convert RGB to HSV
  CHSV hsv = rgb2hsv_approximate(color);

  // Apply new saturation and brightness
  hsv.s = saturation;  // Set saturation (0-255)
  hsv.v = brightness;  // Set brightness (0-255)

  // Convert back to CRGB
  CRGB adjustedColor;
  hsv2rgb_rainbow(hsv, adjustedColor);
  return adjustedColor;
}

CRGB rainbowColor(float hue) {
  uint8_t r = (sin(hue + 0.0f) * 127.5f + 127.5f);                // Red channel
  uint8_t g = (sin(hue + 2.0f * M_PI / 3.0f) * 127.5f + 127.5f);  // Green channel
  uint8_t b = (sin(hue + 4.0f * M_PI / 3.0f) * 127.5f + 127.5f);  // Blue channel
  return CRGB(r, g, b);
}

// ----- Standardized UI Colors with Uniform Brightness -----
// Target brightness: ~120-150 for good visibility and consistency

// Primary UI colors (brightness ~120-150)
const CRGB UI_WHITE = CRGB(120, 120, 120);        // Standard white text/icons
const CRGB UI_RED = CRGB(120, 0, 0);              // Standard red (ON, errors, etc.)
const CRGB UI_GREEN = CRGB(0, 120, 0);            // Standard green (ON, success, etc.)
const CRGB UI_BLUE = CRGB(0, 0, 120);             // Standard blue (info, etc.)
const CRGB UI_YELLOW = CRGB(120, 120, 0);         // Standard yellow (warnings, etc.)
const CRGB UI_CYAN = CRGB(0, 120, 120);           // Standard cyan (info, etc.)
const CRGB UI_MAGENTA = CRGB(120, 0, 120);        // Standard magenta (special states, etc.)
const CRGB UI_ORANGE = CRGB(120, 60, 0);          // Standard orange (warnings, etc.)
const CRGB UI_BLACK = CRGB(0, 0, 0);              // Standard BLACK (invisible)

// Secondary UI colors (brightness ~80-100 for less prominent elements)
const CRGB UI_DIM_WHITE = CRGB(20, 20, 20);       // Dim white for secondary text
const CRGB UI_DIM_RED = CRGB(20, 0, 0);           // Dim red
const CRGB UI_DIM_GREEN = CRGB(0, 20, 0);         // Dim green
const CRGB UI_DIM_BLUE = CRGB(0, 0, 20);          // Dim blue
const CRGB UI_DIM_YELLOW = CRGB(20, 20, 0);       // Dim yellow
const CRGB UI_DIM_ORANGE = CRGB(40, 20, 0);       // Dim orange
const CRGB UI_DIM_MAGENTA = CRGB(20, 0, 20);      // Dim magenta

// Accent colors (brightness ~150-180 for highlights and active states)
const CRGB UI_BRIGHT_WHITE = CRGB(150, 150, 150); // Bright white for highlights
const CRGB UI_BRIGHT_RED = CRGB(150, 0, 0);       // Bright red for active states
const CRGB UI_BRIGHT_GREEN = CRGB(0, 150, 0);     // Bright green for active states
const CRGB UI_BRIGHT_BLUE = CRGB(0, 0, 150);      // Bright blue for active states

// Background/dim colors (brightness ~20-40 for subtle elements)
const CRGB UI_BG_DARK = CRGB(20, 20, 20);         // Dark background
const CRGB UI_BG_DIM = CRGB(40, 40, 40);          // Dim background
