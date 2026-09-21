#pragma once
#include "platform/runtime.h"
#include <Hash.h>
#include <new>
#include "secure_memory.h"
#include "hardware_rng.h"
#include "board_config.h"

class TouchEntropy {
 public:
#if defined(AURORA_BOARD_P4)
  static constexpr uint16_t REQUIRED_SAMPLES = 512;
  static constexpr uint32_t MIN_COLLECTION_MS = 10000;
  static constexpr uint32_t MIN_TOUCH_INTERVAL_US = 12000;
  static constexpr uint32_t MAX_ACTIVE_INTERVAL_US = 100000;
  static constexpr uint16_t MIN_TOUCH_TRAVEL = 4;
  static constexpr uint8_t REQUIRED_TOUCH_ZONES = 6;
#else
  static constexpr uint16_t REQUIRED_SAMPLES = 320;
#endif
  ~TouchEntropy() { cancel(); }
  void begin() {
    cancel();
    hardwareRngEnable();
    rngEnabled_ = true;
    // Separate, ephemeral key for the on-screen preview, never used by the wallet.
    esp_fill_random(previewKey_, sizeof(previewKey_));
    sha_.begin();
    samples_ = 0;
    started_ = millis();
  }
  void add(int16_t x, int16_t y, uint16_t pressure) {
#if defined(AURORA_BOARD_P4)
    // Reaching the minimum enables manual validation; it does not stop the
    // P4 collector. Extra interaction continues to feed the same SHA-256 pool.
    if (!rngEnabled_) return;
#else
    if (!rngEnabled_ || ready()) return;
#endif
    const uint32_t sampleTime = micros();

#if !defined(AURORA_BOARD_P4)
    // The LDR ADC and the internal SAR-ADC RNG source must not run together.
    // Complete the one-shot read before restoring the hardware entropy source.
    hardwareRngDisable();
    rngEnabled_ = false;
    analogReadResolution(12);
    analogSetPinAttenuation(AURORA_LIGHT_SENSOR_PIN, ADC_11db);
    const uint16_t light = analogRead(AURORA_LIGHT_SENSOR_PIN);
    hardwareRngEnable();
    rngEnabled_ = true;

    struct __attribute__((packed)) Sample {
      int16_t x, y;
      uint16_t p;
      uint32_t us, rng;
      uint16_t light;
    } s = {x, y, pressure, sampleTime, esp_random(), light};
#else
    // Capacitive contact strength is not a calibrated pressure sensor; no LDR.
    (void)pressure;
    const uint32_t sinceAccepted = touchSeen_ ? sampleTime - lastAcceptedUs_ : UINT32_MAX;
    const int32_t deltaX = static_cast<int32_t>(x) - lastAcceptedX_;
    const int32_t deltaY = static_cast<int32_t>(y) - lastAcceptedY_;
    const uint32_t travel = touchSeen_
        ? static_cast<uint32_t>((deltaX < 0 ? -deltaX : deltaX) +
                                (deltaY < 0 ? -deltaY : deltaY))
        : UINT32_MAX;
    const bool qualified = !touchSeen_ ||
        (sinceAccepted >= MIN_TOUCH_INTERVAL_US && travel >= MIN_TOUCH_TRAVEL);
    struct __attribute__((packed)) Sample {
      int16_t x, y;
      uint32_t us, rng, observation;
      uint8_t qualified;
    } s = {x, y, sampleTime, esp_random(), ++observations_,
           static_cast<uint8_t>(qualified)};
#endif
    sha_.write(reinterpret_cast<const uint8_t *>(&s), sizeof(s));

    // Display only a truncated HMAC of the sample, never raw RNG or pool bytes.
    uint8_t preview[32];
    sha256Hmac(previewKey_, sizeof(previewKey_),
               reinterpret_cast<const uint8_t *>(&s), sizeof(s), preview);
    memcpy(&previewToken_, preview, sizeof(previewToken_));
    secureZero(preview, sizeof(preview));
    secureZero(&s, sizeof(s));
#if defined(AURORA_BOARD_P4)
    if (qualified) {
      if (touchSeen_) {
        activeCollectionUs_ += sinceAccepted > MAX_ACTIVE_INTERVAL_US
            ? MAX_ACTIVE_INTERVAL_US : sinceAccepted;
      }
      touchSeen_ = true;
      lastAcceptedX_ = x; lastAcceptedY_ = y; lastAcceptedUs_ = sampleTime;
      // Entropy pad: x=24..455, y=152..339. Record a 4 x 3 coverage map.
      const unsigned column = x <= 24 ? 0 :
          static_cast<unsigned>(x - 24) * 4 / 432;
      const unsigned row = y <= 152 ? 0 :
          static_cast<unsigned>(y - 152) * 3 / 188;
      const unsigned zone = (row > 2 ? 2 : row) * 4 + (column > 3 ? 3 : column);
      touchZones_ |= static_cast<uint16_t>(1u << zone);
      if (samples_ < UINT16_MAX) ++samples_;
    }
#else
    ++samples_;
#endif
  }
  enum class Source : uint8_t { Microphone = 1, Camera = 2 };
  // Fresh, real auxiliary data only. This never advances the touch counter.
  bool addAuxiliary(Source source, uint32_t sequence, const uint8_t *data, size_t length) {
    const unsigned index = static_cast<unsigned>(source) - 1;
#if defined(AURORA_BOARD_P4)
    if (!rngEnabled_ || index >= 2 || !data || length == 0 || length > 4096) return false;
#else
    if (!rngEnabled_ || ready() || index >= 2 || !data || length == 0 || length > 4096) return false;
#endif
    if (auxSeen_[index] && static_cast<int32_t>(sequence - auxSequence_[index]) <= 0) return false;
    const uint8_t domain[] = {'A','U','R','O','R','A','-','A','U','X',1,static_cast<uint8_t>(source)};
    uint8_t metadata[12];
    for (unsigned i = 0; i < 4; ++i) {
      metadata[i] = static_cast<uint8_t>(sequence >> (8 * i));
      metadata[i + 4] = static_cast<uint8_t>(length >> (8 * i));
    }
    const uint32_t observedUs = micros();
    for (unsigned i = 0; i < 4; ++i)
      metadata[i + 8] = static_cast<uint8_t>(observedUs >> (8 * i));
    sha_.write(domain, sizeof(domain));
    sha_.write(metadata, sizeof(metadata));
    sha_.write(data, length);
    auxSeen_[index] = true;
    auxSequence_[index] = sequence;
    ++auxCount_[index];
    secureZero(metadata, sizeof(metadata));
    return true;
  }
  uint32_t auxiliaryCount(Source source) const {
    const unsigned index = static_cast<unsigned>(source) - 1;
    return index < 2 ? auxCount_[index] : 0;
  }
  // Collection progress, not an estimate of the number of unpredictable bits.
  uint8_t progress() const {
#if defined(AURORA_BOARD_P4)
    if (!rngEnabled_) return 0;
    const uint32_t sampleProgress = static_cast<uint32_t>(samples_) * 100 / REQUIRED_SAMPLES;
    const uint32_t timeProgress = static_cast<uint32_t>(
        (activeCollectionUs_ / 1000) * 100 / MIN_COLLECTION_MS);
    const uint32_t zoneProgress = static_cast<uint32_t>(zoneCount()) * 100 / REQUIRED_TOUCH_ZONES;
    uint32_t value = sampleProgress;
    if (timeProgress < value) value = timeProgress;
    if (zoneProgress < value) value = zoneProgress;
    if (value > 100) value = 100;
    return static_cast<uint8_t>(value);
#else
    return static_cast<uint8_t>(samples_ * 100 / REQUIRED_SAMPLES);
#endif
  }
  uint16_t sampleCount() const { return samples_; }
  uint32_t observationCount() const { return observations_; }
  uint32_t activeCollectionMs() const { return static_cast<uint32_t>(activeCollectionUs_ / 1000); }
  uint8_t coveredZoneCount() const { return zoneCount(); }
  uint32_t previewToken() const { return previewToken_; }
  bool ready() const {
#if defined(AURORA_BOARD_P4)
    return rngEnabled_ && samples_ >= REQUIRED_SAMPLES &&
        activeCollectionUs_ / 1000 >= MIN_COLLECTION_MS && zoneCount() >= REQUIRED_TOUCH_ZONES;
#else
    return rngEnabled_ && samples_ >= REQUIRED_SAMPLES;
#endif
  }
  bool finish(uint8_t out[32]) {
    if (!out || !ready()) return false;
#if defined(AURORA_BOARD_P4)
    uint32_t tail[9] = {millis() - started_, static_cast<uint32_t>(activeCollectionUs_ / 1000),
                        micros(), esp_random(), observations_, samples_, touchZones_,
                        auxCount_[0], auxCount_[1]};
#else
    uint32_t tail[3] = {millis() - started_, micros(), esp_random()};
#endif
    sha_.write(reinterpret_cast<uint8_t *>(tail), sizeof(tail));
    sha_.end(out);
    secureZero(tail, sizeof(tail));
    cancel();
    return true;
  }
  void cancel() {
    if (rngEnabled_) {
      hardwareRngDisable();
      rngEnabled_ = false;
    }
    sha_.~SHA256();
    secureZero(&sha_, sizeof(sha_));
    new (&sha_) SHA256();
    secureZero(previewKey_, sizeof(previewKey_));
    secureZero(&previewToken_, sizeof(previewToken_));
    samples_ = 0;
    started_ = 0;
    observations_ = 0; lastAcceptedUs_ = 0; activeCollectionUs_ = 0;
    lastAcceptedX_ = lastAcceptedY_ = 0; touchZones_ = 0; touchSeen_ = false;
    secureZero(auxSeen_, sizeof(auxSeen_));
    secureZero(auxSequence_, sizeof(auxSequence_));
    secureZero(auxCount_, sizeof(auxCount_));
  }
  // Terminal-failure/startup path: no peripheral operations or heap allocation
  // in the pinned uBitcoin SHA256 implementation (its context is inline).
  // The caller owns task quiescence; normal cancel() still disables the RNG.
  void wipeSecretsWithoutHardware() noexcept {
    sha_.~SHA256();
    secureZero(&sha_, sizeof(sha_));
    new (&sha_) SHA256();
    secureZero(previewKey_, sizeof(previewKey_));
    secureZero(&previewToken_, sizeof(previewToken_));
    samples_ = 0;
    started_ = 0;
    observations_ = 0; lastAcceptedUs_ = 0; activeCollectionUs_ = 0;
    lastAcceptedX_ = lastAcceptedY_ = 0; touchZones_ = 0; touchSeen_ = false;
    secureZero(auxSeen_, sizeof(auxSeen_));
    secureZero(auxSequence_, sizeof(auxSequence_));
    secureZero(auxCount_, sizeof(auxCount_));
  }
 private:
  uint8_t zoneCount() const {
    uint16_t value = touchZones_;
    uint8_t count = 0;
    while (value) { count += static_cast<uint8_t>(value & 1u); value >>= 1; }
    return count;
  }
  SHA256 sha_;
  uint8_t previewKey_[32]{};
  uint32_t previewToken_ = 0;
  uint16_t samples_ = 0;
  uint32_t started_ = 0;
  uint32_t observations_ = 0;
  uint32_t lastAcceptedUs_ = 0;
  uint64_t activeCollectionUs_ = 0;
  int16_t lastAcceptedX_ = 0, lastAcceptedY_ = 0;
  uint16_t touchZones_ = 0;
  bool touchSeen_ = false;
  bool rngEnabled_ = false;
  bool auxSeen_[2]{};
  uint32_t auxSequence_[2]{};
  uint32_t auxCount_[2]{};
};
