#include <Arduino.h>
#include <Wire.h>

// GNU ld wrapping keeps upstream Wire/Audio/BusIO sources untouched. The primary
// bus must be slow BEFORE begin() returns: BusIO probes immediately afterward.
// Only primary Wire is changed; Wire1/Wire2 retain their own configuration.
static bool primaryWireReady = false;

extern void toernApplyI2cTimingAfterBegin();
extern "C" void __real__ZN7TwoWire5beginEv(TwoWire *bus);
extern "C" void __wrap__ZN7TwoWire5beginEv(TwoWire *bus) {
  __real__ZN7TwoWire5beginEv(bus);
  if (bus == &Wire) {
    toernApplyI2cTimingAfterBegin();
    primaryWireReady = true;
  }
}

// Startup kit/mode restoration already accesses encoders, before the codec's
// internal Wire.begin(). Do not touch an uninitialized peripheral. Encoder
// configuration is written again by initEncoders() after codec initialization.
extern "C" uint8_t __real__ZN7TwoWire15endTransmissionEh(TwoWire *bus, uint8_t stop);
extern "C" uint8_t __wrap__ZN7TwoWire15endTransmissionEh(TwoWire *bus, uint8_t stop) {
  if (bus == &Wire && !primaryWireReady) return 4;  // Wire transaction error
  return __real__ZN7TwoWire15endTransmissionEh(bus, stop);
}

extern "C" uint8_t __real__ZN7TwoWire11requestFromEhhh(
    TwoWire *bus, uint8_t address, uint8_t length, uint8_t stop);
extern "C" uint8_t __wrap__ZN7TwoWire11requestFromEhhh(
    TwoWire *bus, uint8_t address, uint8_t length, uint8_t stop) {
  if (bus == &Wire && !primaryWireReady) return 0;  // no received bytes
  return __real__ZN7TwoWire11requestFromEhhh(bus, address, length, stop);
}
