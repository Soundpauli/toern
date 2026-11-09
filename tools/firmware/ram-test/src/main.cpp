#include <Arduino.h>
#include "memory_test.h"
extern "C" uint8_t external_psram_size;

// Only the external 16 MB is destructive-tested. No EEPROM, SD, EXTMEM globals
// or external-heap allocations are used. Code, stack and USB live internally.
struct Psram {
  volatile uint32_t *base = reinterpret_cast<volatile uint32_t *>(0x70000000u);
  void prepare(uint32_t first, uint32_t count) {
    arm_dcache_delete((void *)(base + first), count * 4u);
  }
  void finish(uint32_t first, uint32_t count) {
    // Write dirty cache lines to the physical chips, then invalidate. The next
    // read must reach PSRAM rather than pass by reading back the CPU's cache.
    arm_dcache_flush_delete((void *)(base + first), count * 4u);
  }
  uint32_t read(uint32_t i) { return base[i]; }
  void write(uint32_t i, uint32_t value) { base[i] = value; }
} memory;
static toern_ramtest::Test<Psram> test(memory);
static char line[48];
static uint32_t lineLength = 0, lastReport = 0, started = 0, elapsed = 0;
static bool overflow = false;

static const char *stateName() {
  using toern_ramtest::State;
  switch (test.state) {
    case State::Running: return "running";
    case State::Pass: return "pass";
    case State::Fail: return "fail";
    case State::Missing: return "missing";
    case State::Aborted: return "aborted";
    default: return "idle";
  }
}
static void report(bool reply = false) {
  if (!Serial) return;
  Serial.printf("RAMTEST {\"ok\":true,\"reply\":%s,\"protocol\":1,\"device\":\"toern-psram-test\",\"state\":\"%s\",\"run\":%lu,\"detectedBytes\":%lu,\"targetBytes\":%lu,\"testedBytes\":%lu,\"stage\":%lu,\"stages\":%lu,\"progress\":%lu,\"elapsedMs\":%lu,\"offset\":%lu,\"expected\":%lu,\"actual\":%lu,\"cpuMHz\":816}\n",
    reply ? "true" : "false", stateName(), (unsigned long)test.run, (unsigned long)external_psram_size * 1048576u,
    (unsigned long)toern_ramtest::targetBytes,
    test.state == toern_ramtest::State::Pass ? (unsigned long)toern_ramtest::targetBytes : 0ul,
    (unsigned long)test.stage, (unsigned long)toern_ramtest::totalStages,
    (unsigned long)test.progress(), (unsigned long)elapsed,
    (unsigned long)test.failure.offset, (unsigned long)test.failure.expected, (unsigned long)test.failure.actual);
  lastReport = millis();
}
static void command() {
  if (overflow) return;
  line[lineLength] = 0;
  if (!strcmp(line, "RAMTEST STATUS")) report(true);
  else if (!strcmp(line, "RAMTEST START")) {
    if (test.state != toern_ramtest::State::Running) {
      test.start((uint32_t)external_psram_size * 1048576u);
      elapsed = 0; started = millis();
    }
    report(true);
  } else if (!strcmp(line, "RAMTEST ABORT")) { test.abort(); report(true); }
}
void setup() { pinMode(LED_BUILTIN, OUTPUT); Serial.begin(115200); }
void loop() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n') { command(); lineLength = 0; overflow = false; }
    else if (c != '\r') { if (lineLength < sizeof(line) - 1) line[lineLength++] = c; else overflow = true; }
  }
  if (test.state == toern_ramtest::State::Running) {
    test.tick(); elapsed = millis() - started;
    if (test.state != toern_ramtest::State::Running || millis() - lastReport >= 250u) report();
  }
  digitalWriteFast(LED_BUILTIN, test.state == toern_ramtest::State::Pass ? HIGH :
    test.state == toern_ramtest::State::Running && ((millis() / 250u) & 1u));
  yield();
}
