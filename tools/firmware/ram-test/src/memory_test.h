#pragma once
#include <stdint.h>

namespace toern_ramtest {
constexpr uint32_t targetBytes = 16u * 1024u * 1024u;
constexpr uint32_t totalStages = 26; // March C- (6) + ten whole-memory write/read patterns (20)
enum class State { Idle, Running, Pass, Fail, Missing, Aborted };
struct Failure { uint32_t offset = 0, expected = 0, actual = 0; };

// Backend supplies uncached read/write access. No test buffers occupy PSRAM.
// Each stage visits EVERY 32-bit word, including both chips and the last word.
template<class Memory> class Test {
 public:
  Memory &memory;
  State state = State::Idle;
  Failure failure;
  uint32_t stage = 0, cursor = 0, run = 0;
  explicit Test(Memory &m) : memory(m) {}
  void start(uint32_t detectedBytes) {
    ++run; stage = cursor = 0; failure = Failure{};
    state = detectedBytes == targetBytes ? State::Running : State::Missing;
  }
  void abort() { if (state == State::Running) state = State::Aborted; }
  uint32_t progress() const {
    return (uint64_t(stage) * words + cursor) * 100u / (uint64_t(totalStages) * words);
  }
  // Small aligned blocks keep USB polling responsive even during lengthy runs.
  void tick() {
    if (state != State::Running) return;
    constexpr uint32_t block = 1024;
    const uint32_t count = words - cursor < block ? words - cursor : block;
    const bool reverse = stage == 3 || stage == 4;
    const uint32_t first = reverse ? words - cursor - count : cursor;
    memory.prepare(first, count);
    for (uint32_t n = 0; n < count; ++n) {
      const uint32_t i = reverse ? first + count - n - 1 : first + n;
      uint32_t expected = 0, write = 0;
      bool reads = true, writes = true;
      if (stage == 0) reads = false;
      else if (stage < 5) { expected = (stage == 2 || stage == 4) ? UINT32_MAX : 0; write = ~expected; }
      else if (stage == 5) writes = false;
      else { expected = pattern((stage - 6) / 2, i); write = expected; reads = (stage & 1u) != 0; writes = !reads; }
      if (reads) {
        const uint32_t actual = memory.read(i);
        if (actual != expected) {
          failure = {i * 4u, expected, actual}; state = State::Fail;
          memory.finish(first, count); return;
        }
      }
      if (writes) memory.write(i, write);
    }
    memory.finish(first, count);
    cursor += count;
    if (cursor == words) { cursor = 0; if (++stage == totalStages) state = State::Pass; }
  }
 private:
  static constexpr uint32_t words = targetBytes / 4u;
  static uint32_t pattern(uint32_t p, uint32_t i) {
    if (p == 0) return 0;
    if (p == 1) return UINT32_MAX;
    if (p == 2) return 0x55555555u;
    if (p == 3) return 0xAAAAAAAAu;
    if (p == 4) return i ^ 0x5A698421u;
    if (p == 5) return ~(i ^ 0x5A698421u);
    // Deterministic address-dependent patterns; distinct values across chips.
    uint32_t x = i ^ (0x9E3779B9u * (p - 5));
    x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu;
    return x ^ (x >> 16);
  }
};
}
