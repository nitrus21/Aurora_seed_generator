#pragma once
#include "platform/runtime.h"
#include <Hash.h>
#include <new>
#include "secure_memory.h"
#include "hardware_rng.h"
#include "board_config.h"

class TouchEntropy {
 public:
  static constexpr uint16_t REQUIRED_SAMPLES = 320;
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
    if (!rngEnabled_ || ready()) return;
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
    struct __attribute__((packed)) Sample {
      int16_t x, y;
      uint32_t us, rng;
    } s = {x, y, sampleTime, esp_random()};
#endif
    sha_.write(reinterpret_cast<const uint8_t *>(&s), sizeof(s));

    // Display only a truncated HMAC of the sample, never raw RNG or pool bytes.
    uint8_t preview[32];
    sha256Hmac(previewKey_, sizeof(previewKey_),
               reinterpret_cast<const uint8_t *>(&s), sizeof(s), preview);
    memcpy(&previewToken_, preview, sizeof(previewToken_));
    secureZero(preview, sizeof(preview));
    secureZero(&s, sizeof(s));
    ++samples_;
  }
  enum class Source : uint8_t { Microphone = 1, Camera = 2 };
  // Fresh, real auxiliary data only. This never advances the 320-touch counter.
  bool addAuxiliary(Source source, uint32_t sequence, const uint8_t *data, size_t length) {
    const unsigned index = static_cast<unsigned>(source) - 1;
    if (!rngEnabled_ || ready() || index >= 2 || !data || length == 0 || length > 4096) return false;
    if (auxSeen_[index] && static_cast<int32_t>(sequence - auxSequence_[index]) <= 0) return false;
    const uint8_t domain[] = {'A','U','R','O','R','A','-','A','U','X',1,static_cast<uint8_t>(source)};
    uint8_t metadata[8];
    for (unsigned i = 0; i < 4; ++i) {
      metadata[i] = static_cast<uint8_t>(sequence >> (8 * i));
      metadata[i + 4] = static_cast<uint8_t>(length >> (8 * i));
    }
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
  uint8_t progress() const { return static_cast<uint8_t>(samples_ * 100 / REQUIRED_SAMPLES); }
  uint16_t sampleCount() const { return samples_; }
  uint32_t previewToken() const { return previewToken_; }
  bool ready() const { return rngEnabled_ && samples_ >= REQUIRED_SAMPLES; }
  bool finish(uint8_t out[32]) {
    if (!out || !ready()) return false;
    uint32_t tail[3] = {millis() - started_, micros(), esp_random()};
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
    secureZero(auxSeen_, sizeof(auxSeen_));
    secureZero(auxSequence_, sizeof(auxSequence_));
    secureZero(auxCount_, sizeof(auxCount_));
  }
 private:
  SHA256 sha_;
  uint8_t previewKey_[32]{};
  uint32_t previewToken_ = 0;
  uint16_t samples_ = 0;
  uint32_t started_ = 0;
  bool rngEnabled_ = false;
  bool auxSeen_[2]{};
  uint32_t auxSequence_[2]{};
  uint32_t auxCount_[2]{};
};
