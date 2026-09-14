#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Deterministic fixtures only. These stubs are never included in the firmware.
struct MockHardware {
  bool rngEnabled = false;
  unsigned adcStep = 0, lightReads = 0, rngReads = 0;
  uint16_t light = 1234;
  uint32_t time = 20000, rngSalt = 0;
  uint8_t previewSalt = 0;
  uint8_t *previewKey = nullptr;
};
inline MockHardware mock;
constexpr int ADC_11db = 3;
inline uint32_t millis() { return mock.time / 1000; }
inline uint32_t micros() { return mock.time; }
inline void analogReadResolution(int bits) {
  assert(!mock.rngEnabled && mock.adcStep == 0 && bits == 12);
  mock.adcStep = 1;
}
inline void analogSetPinAttenuation(int pin, int attenuation) {
  assert(!mock.rngEnabled && mock.adcStep == 1 && pin == 34 && attenuation == ADC_11db);
  mock.adcStep = 2;
}
inline uint16_t analogRead(int pin) {
  assert(!mock.rngEnabled && mock.adcStep == 2 && pin == 34);
  mock.adcStep = 0;
  ++mock.lightReads;
  return mock.light;
}
