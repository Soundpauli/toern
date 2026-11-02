#include <Arduino.h>
#include <Wire.h>
#include "src/toern_i2c_health.h"
static ToernI2cHealth primaryHealth;
void toernI2cRuntimeBegin(){primaryHealth.runtime=true;}
bool toernI2cReady(){return primaryHealth.ready(millis());}
uint32_t toernI2cErrors(){return primaryHealth.errors;}
uint32_t toernI2cMaxUs(){return primaryHealth.maxUs;}
uint32_t toernI2cSkipped(){return primaryHealth.skipped;}

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
  if(bus!=&Wire)return __real__ZN7TwoWire15endTransmissionEh(bus,stop);
  if(!primaryHealth.ready(millis())){++primaryHealth.skipped;return 4;}
  const uint32_t started=micros();
  const uint8_t result=__real__ZN7TwoWire15endTransmissionEh(bus,stop);
  primaryHealth.record(millis(),micros()-started,result);return result;
}

extern "C" uint8_t __real__ZN7TwoWire11requestFromEhhh(
    TwoWire *bus, uint8_t address, uint8_t length, uint8_t stop);
extern "C" uint8_t __wrap__ZN7TwoWire11requestFromEhhh(
    TwoWire *bus, uint8_t address, uint8_t length, uint8_t stop) {
  if (bus == &Wire && !primaryWireReady) return 0;  // no received bytes
  if(bus!=&Wire)return __real__ZN7TwoWire11requestFromEhhh(bus,address,length,stop);
  if(!primaryHealth.ready(millis())){++primaryHealth.skipped;return 0;}
  const uint32_t started=micros();
  const uint8_t result=__real__ZN7TwoWire11requestFromEhhh(bus,address,length,stop);
  primaryHealth.record(millis(),micros()-started,result==length?0:4);return result;
}
