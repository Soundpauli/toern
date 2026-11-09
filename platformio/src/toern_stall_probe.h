#pragma once
// Diagnostic: when loop() stops making progress, capture the program counter
// the CPU is stuck at. Hooks the SysTick vector (priority 32, so it still runs
// while PIT/audio interrupts or StateGuard regions are busy) and reads the
// stacked exception frame of whatever SysTick preempted.
#include <Arduino.h>

namespace toern_stall_probe {
struct Record { uint32_t ms, stuckMs, pc, lr, xpsr, r0, r1; };
static constexpr unsigned kRecords = 8;
static volatile Record records[kRecords];
static volatile uint32_t recordCount = 0;
static volatile uint32_t heartbeat = 0;  // bumped once per loop() pass
}  // namespace toern_stall_probe

extern "C" {
void (*toernStallProbePrev)() = nullptr;

__attribute__((used)) void toernStallProbeBody(uint32_t *frame) {
  using namespace toern_stall_probe;
  static uint32_t lastBeat = 0, stuckMs = 0;
  const uint32_t beat = heartbeat;
  if (beat != lastBeat) { lastBeat = beat; stuckMs = 0; return; }
  ++stuckMs;
  if (stuckMs != 300 && stuckMs != 900 && stuckMs != 2000) return;
  volatile Record &r = records[recordCount % kRecords];
  r.ms = millis(); r.stuckMs = stuckMs;
  r.r0 = frame[0]; r.r1 = frame[1]; r.lr = frame[5]; r.pc = frame[6]; r.xpsr = frame[7];
  ++recordCount;
}

__attribute__((naked, noinline)) void toernStallProbeSystick() {
  asm volatile(
      "mrs r0, msp\n"
      "push {r4, lr}\n"
      "bl toernStallProbeBody\n"
      "pop {r4, lr}\n"
      "movw r0, #:lower16:toernStallProbePrev\n"
      "movt r0, #:upper16:toernStallProbePrev\n"
      "ldr r0, [r0]\n"
      "bx r0\n");
}
}

namespace toern_stall_probe {
inline void install() {
  if (_VectorsRam[15] == toernStallProbeSystick) return;
  __disable_irq();
  toernStallProbePrev = _VectorsRam[15];
  _VectorsRam[15] = toernStallProbeSystick;
  __enable_irq();
}
}  // namespace toern_stall_probe
