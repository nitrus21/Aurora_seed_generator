#pragma once
#include <cstdint>

// Qualifies microphone activity without treating a DC offset or ambient hiss as
// a fresh event. This gate estimates activity only; audio bytes are hashed
// separately and never retained by this class.
class AudioEntropyGate {
 public:
  static constexpr uint32_t CALIBRATION_BLOCKS = 24;
  static constexpr uint32_t MIN_ACTIVITY = 1800;
  static constexpr uint32_t MAX_ACTIVITY = 16000;
  static constexpr uint32_t THRESHOLD_MULTIPLIER = 3;

  struct Result {
    bool qualified;
    uint8_t level;
    uint32_t threshold;
  };

  Result observe(uint32_t activity) {
    ++observedBlocks_;
    if (observedBlocks_ <= CALIBRATION_BLOCKS) {
      noiseFloor_ = observedBlocks_ == 1 ? activity :
          static_cast<uint32_t>((static_cast<uint64_t>(noiseFloor_) * 7 + activity) / 8);
    }
    const uint32_t adaptive = noiseFloor_ > UINT32_MAX / THRESHOLD_MULTIPLIER
        ? UINT32_MAX : noiseFloor_ * THRESHOLD_MULTIPLIER;
    uint32_t threshold = adaptive > MIN_ACTIVITY ? adaptive : MIN_ACTIVITY;
    if (threshold > MAX_ACTIVITY) threshold = MAX_ACTIVITY;
    const uint64_t denominator = static_cast<uint64_t>(threshold) * 2;
    const uint32_t meter = denominator
        ? static_cast<uint32_t>(static_cast<uint64_t>(activity) * 100 / denominator) : 0;
    const bool qualified = observedBlocks_ > CALIBRATION_BLOCKS && activity >= threshold;
    if (!qualified && observedBlocks_ > CALIBRATION_BLOCKS) {
      // Slow tracking prevents short loud events from desensitising the gate.
      noiseFloor_ = static_cast<uint32_t>((static_cast<uint64_t>(noiseFloor_) * 31 + activity) / 32);
    }
    return {qualified, static_cast<uint8_t>(meter > 100 ? 100 : meter), threshold};
  }

  uint32_t observedBlocks() const { return observedBlocks_; }
  uint32_t noiseFloor() const { return noiseFloor_; }

 private:
  uint32_t observedBlocks_ = 0;
  uint32_t noiseFloor_ = 0;
};
