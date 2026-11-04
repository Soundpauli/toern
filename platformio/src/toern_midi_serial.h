#pragma once

#include <Arduino.h>

// Capture bytes immediately after the core UART ISR has emptied the hardware
// FIFO. MIDI callbacks still execute in loop(), using these receive timestamps.
// No parsing, notes, UI, or timer reconfiguration runs in the UART interrupt.
namespace toern_midi {
struct ReceivedByte { uint32_t us; uint8_t value; };
static DMAMEM ReceivedByte rxBytes[512];
static volatile uint16_t rxHead = 0, rxTail = 0;
static volatile uint32_t rxOverruns = 0;
static uint32_t parsedByteUs = 0;
static void (*coreSerial8Handler)() = nullptr;

static void receiveSerial8() {
  coreSerial8Handler();
  const uint32_t now = micros();
  while (Serial8.available()) {
    const uint8_t value = Serial8.read();
    const uint16_t next = (rxHead + 1u) & 511u;
    if (next == rxTail) { ++rxOverruns; continue; }
    rxBytes[rxHead] = { now, value };
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    rxHead = next;
  }
}

class TimestampedSerial {
 public:
  void begin(long baud) {
    Serial8.begin(baud);
    // Serial8.begin installs the core vector. Keep the core RX/TX handler and
    // layer timestamp capture around it; this also works in Arduino IDE builds.
    if (_VectorsRam[IRQ_LPUART5 + 16] != receiveSerial8)
      coreSerial8Handler = _VectorsRam[IRQ_LPUART5 + 16];
    attachInterruptVector(IRQ_LPUART5, receiveSerial8);
    NVIC_SET_PRIORITY(IRQ_LPUART5, 32);
  }
  unsigned available() const { return (rxHead - rxTail) & 511u; }
  int read() {
    if (rxHead == rxTail) return -1;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    const ReceivedByte byte = rxBytes[rxTail];
    parsedByteUs = byte.us;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    rxTail = (rxTail + 1u) & 511u;
    return byte.value;
  }
  size_t write(uint8_t value) { return Serial8.write(value); }
};
}  // namespace toern_midi
