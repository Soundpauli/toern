#include <stdint.h>
const CRGB col_Folder[] = {
    CRGB(139, 0, 0),     // Dark Red
    CRGB(255, 69, 0),    // Burnt Orange
    CRGB(255, 255, 0),   // Gold
    CRGB(0, 139, 0),     // Green
    CRGB(0, 140, 130),   // Türkis
    CRGB(0, 0, 255),     // Blau
    CRGB(140, 0, 120),   // Lila
    CRGB(220, 100, 100), // rosa
    CRGB(0, 255, 70),  // s1s
    CRGB(255, 50, 50)  // s2#
};


const CRGB col[] = {
    CRGB(0, 0, 0),     
    CRGB(139, 0, 0),     // Dark Red
    CRGB(255, 69, 0),    // Burnt Orange
    CRGB(255, 255, 0),   // Gold
    CRGB(0, 139, 0),     // Green
    CRGB(0, 140, 130),   // Türkis
    CRGB(0, 0, 255),     // Blau
    CRGB(140, 0, 120),   // Lila
    CRGB(220, 100, 100), // rosa
    CRGB(0, 0, 0), 
    CRGB(0, 0, 0), 
    CRGB(0, 0, 0), 
    CRGB(0, 0, 0), 
    CRGB(0, 255, 70),  // s1s
    CRGB(255, 50, 50)  // s2
  };


// Normal: 70% brightness
const CRGB col_base[] = {
    CRGB(0, 0, 0),
    CRGB(9, 0, 0),     // Dark Red
    CRGB(18, 4, 0),    // Orange
    CRGB(10, 10, 0),   // Gold
    CRGB(0, 4, 0),     // Green
    CRGB(0, 5, 5),     // Türkis
    CRGB(0, 0, 4),     // Blau 
    CRGB(15, 1, 4),    // Lila
    CRGB(22, 12, 16),  // Rosa
    CRGB(0, 0, 0), 
    CRGB(0, 0, 0), 
    CRGB(0, 0, 0), 
    CRGB(0, 0, 0),
    CRGB(0, 10, 4),   //21
    CRGB(18, 4, 4)   // s2
  };





const uint32_t colors[] = {
  0xFF0000, // 1 rot (red)
  0xfa6902, // 7 orange (orange)
  0xFFFF00, // 3 gelb (yellow)
  0x83ff4a, // 6 hellgrün (light green)
  0x00AA00, // 4 grün (green)
  0x06d1c7, // 8 türkis (turquoise)
  0x0000AA, // 2 blau (blue)
  0x800080, // 5 lila (purple)
  0xD3D3D3, // 13 hellgrau (light gray)
  0x222222, // 14 dunkelgrau (dark gray)
  0x000000, // 0 schwarz (black)
  0x000000, // 9 schwarz (black)
  0x000000, // 10 schwarz (black)
  0x000000, // 11 schwarz (black)
  0x000000  // 12 schwarz (black)
};

uint32_t CRGBToUint32(CRGB color) {
    return ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | (uint32_t)color.b;
}

void writeColors() {
    for (size_t i = 0; i < 16; i++) {
        uint8_t r = (colors[i] >> 16) & 0xFF; // Extract red
        uint8_t g = (colors[i] >> 8) & 0xFF;  // Extract green
        uint8_t b = colors[i] & 0xFF;         // Extract blue
     //   col[i] = CRGB(r, g, b);              // Assign to `cols` array
    }
}


CRGB uint32ToCRGBWithControls(uint32_t rgb, uint8_t saturation, uint8_t brightness) {
    uint8_t r = (rgb >> 16) & 0xFF; // Extract red
    uint8_t g = (rgb >> 8) & 0xFF;  // Extract green
    uint8_t b = rgb & 0xFF;         // Extract blue

    // Create CRGB object from RGB values
    CRGB color = CRGB(r, g, b);

    // Convert RGB to HSV
    CHSV hsv = rgb2hsv_approximate(color);

    // Apply new saturation and brightness
    hsv.s = saturation;   // Set saturation (0-255)
    hsv.v = brightness;   // Set brightness (0-255)

    // Convert back to CRGB
    CRGB adjustedColor;
    hsv2rgb_rainbow(hsv, adjustedColor);

    return adjustedColor;

}