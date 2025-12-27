/* NeoTrellis 4-board setup with interrupts
   4 rows x 16 columns (4 boards side-by-side)
   I2C addresses: 0x2E (none), 0x2F (A0), 0x30 (A1), 0x32 (A2)
*/

// #include "Adafruit_NeoTrellis.h"

#define Y_DIM 4
#define X_DIM 16
#define INT_PIN 27
#define NUM_KEYS (X_DIM * Y_DIM)

// Create trellis array (1 row, 4 columns of boards)
Adafruit_NeoTrellis t_array[1][4] = {
  { Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x32) }
};

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, 1, 4);

// Track state of each key (on/off)
bool keyState[NUM_KEYS] = {false};

// Color wheel function
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// Key press callback - toggles on/off
TrellisCallback toggle(keyEvent evt) {
  if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    // Toggle state
    keyState[evt.bit.NUM] = !keyState[evt.bit.NUM];
    
    // Set color based on state
    if(keyState[evt.bit.NUM]) {
      trellis.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, NUM_KEYS, 0, 255)));
    } else {
      trellis.setPixelColor(evt.bit.NUM, 0);
    }
    trellis.show();
  }
  return 0;
}

void setup() {
  Serial.begin(9600);
  pinMode(INT_PIN, INPUT_PULLUP);
  
  if(!trellis.begin()) {
    Serial.println("Failed to begin trellis");
    while(1) delay(1);
  }
  
  // Setup all keys (only need RISING edge for toggle)
  for(int y = 0; y < Y_DIM; y++) {
    for(int x = 0; x < X_DIM; x++) {
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.registerCallback(x, y, toggle);
    }
  }
  
  // Startup animation
  for(int i = 0; i < NUM_KEYS; i++) {
    trellis.setPixelColor(i, Wheel(map(i, 0, NUM_KEYS, 0, 255)));
    trellis.show();
    delay(30);
  }
  delay(500);
  
  // Clear all
  for(int i = 0; i < NUM_KEYS; i++) {
    trellis.setPixelColor(i, 0);
  }
  trellis.show();
}

void loop() {
  if(!digitalRead(INT_PIN)) {
    trellis.read();
  }
  delay(2);
}
