#pragma once
#include <Arduino.h>
#include <Hash.h>
#include <new>
#include "secure_memory.h"
#include "hardware_rng.h"

class TouchEntropy {
 public:
  ~TouchEntropy() { cancel(); }
  void begin() {
    cancel();
    hardwareRngEnable();
    rngEnabled_ = true;
    sha_.begin();
    samples_ = 0;
    started_ = millis();
  }
  void add(int16_t x, int16_t y, uint16_t pressure) {
    if (!rngEnabled_) return;
    struct __attribute__((packed)) Sample { int16_t x, y; uint16_t p; uint32_t us; uint32_t rng; } s;
    s = {x, y, pressure, micros(), esp_random()};
    sha_.write(reinterpret_cast<const uint8_t *>(&s), sizeof(s));
    secureZero(&s, sizeof(s));
    if (samples_ < 512) ++samples_;
  }
  uint8_t progress() const { return min<uint16_t>(100, samples_ * 100 / 160); }
  bool ready() const { return samples_ >= 160; }
  bool finish(uint8_t out[32]) {
    if (!rngEnabled_ || !ready()) return false;
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
    samples_ = 0;
    started_ = 0;
  }
 private:
  SHA256 sha_;
  uint16_t samples_ = 0;
  uint32_t started_ = 0;
  bool rngEnabled_ = false;
};
