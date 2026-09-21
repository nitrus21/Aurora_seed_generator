#pragma once

#include <stddef.h>
#include <stdint.h>

// Engineering health checks for a stream of conditioned 32-bit RNG words.
// These checks detect gross failures; they do not estimate min-entropy and are
// not a NIST SP 800-90B validation.
struct AuroraEntropyHealthSummary {
  uint32_t words = 0;
  uint64_t ones = 0;
  uint64_t transitions = 0;
  uint32_t bitOnes[32]{};
  uint32_t adjacentDuplicateWords = 0;
  uint32_t longestBitRun = 0;
};

class AuroraEntropyHealth {
 public:
  static constexpr uint32_t REQUIRED_WORDS = 500000;
  static constexpr uint32_t RATIO_MIN_PER_10000 = 4900;
  static constexpr uint32_t RATIO_MAX_PER_10000 = 5100;
  static constexpr uint32_t MAX_BIT_RUN = 64;

  void add(uint32_t word) {
    if (summary_.words && word == previousWord_) ++summary_.adjacentDuplicateWords;
    previousWord_ = word;

    for (unsigned bit = 0; bit < 32; ++bit) {
      const bool value = ((word >> bit) & 1U) != 0;
      summary_.ones += value ? 1U : 0U;
      summary_.bitOnes[bit] += value ? 1U : 0U;
      if (haveBit_) {
        if (value != previousBit_) {
          ++summary_.transitions;
          currentRun_ = 1;
        } else {
          ++currentRun_;
        }
      } else {
        haveBit_ = true;
        currentRun_ = 1;
      }
      previousBit_ = value;
      if (currentRun_ > summary_.longestBitRun) summary_.longestBitRun = currentRun_;
    }
    ++summary_.words;
  }

  const AuroraEntropyHealthSummary &summary() const { return summary_; }

  bool passed() const {
    if (summary_.words != REQUIRED_WORDS || summary_.adjacentDuplicateWords != 0 ||
        summary_.longestBitRun > MAX_BIT_RUN) return false;
    for (unsigned bit = 0; bit < 32; ++bit) {
      if (!ratioInRange(summary_.bitOnes[bit], summary_.words)) return false;
    }
    const uint64_t bits = static_cast<uint64_t>(summary_.words) * 32U;
    return ratioInRange(summary_.ones, bits) &&
           ratioInRange(summary_.transitions, bits - 1U);
  }

  static uint32_t ratioPer10000(uint64_t count, uint64_t total) {
    return total ? static_cast<uint32_t>((count * 10000U + total / 2U) / total) : 0;
  }

 private:
  static bool ratioInRange(uint64_t count, uint64_t total) {
    const uint32_t ratio = ratioPer10000(count, total);
    return ratio >= RATIO_MIN_PER_10000 && ratio <= RATIO_MAX_PER_10000;
  }

  AuroraEntropyHealthSummary summary_{};
  uint32_t previousWord_ = 0;
  uint32_t currentRun_ = 0;
  bool previousBit_ = false;
  bool haveBit_ = false;
};
