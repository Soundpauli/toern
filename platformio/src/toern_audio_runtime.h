#pragma once
#include <Arduino.h>

namespace toern_audio {
constexpr uint8_t kAudioPriority = 208;
// ALL IntervalTimers share IRQ_PIT on Teensy 4. A single priority-0 clock timer
// promotes the sequencer/fill callbacks too. Keep the entire PIT below audio.
constexpr uint8_t kTimerPriority = 224;
static_assert(kTimerPriority > kAudioPriority, "PIT must not preempt audio updates");

// Protect short sampler state transactions from both audio update and PIT
// callbacks, without blocking I2S DMA, UART or USB. Restore the prior IRQ state
// so nested guards and callers which already disabled an IRQ stay correct.
class StateGuard {
public:
    StateGuard() : audioEnabled_(NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0),
                   pitEnabled_(NVIC_IS_ENABLED(IRQ_PIT) != 0) {
        NVIC_DISABLE_IRQ(IRQ_PIT);
        NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
        __DSB();
        __ISB();
    }
    ~StateGuard() {
        __DSB();
        if (audioEnabled_) NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
        if (pitEnabled_) NVIC_ENABLE_IRQ(IRQ_PIT);
    }
    StateGuard(const StateGuard&) = delete;
    StateGuard& operator=(const StateGuard&) = delete;
private:
    bool audioEnabled_;
    bool pitEnabled_;
};

}
