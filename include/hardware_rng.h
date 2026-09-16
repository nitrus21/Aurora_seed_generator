#pragma once

#include <stddef.h>
#include <stdint.h>
#include <esp_system.h>
#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)
#include <esp_random.h>
#endif
#include <bootloader_random.h>

// On the original ESP32, esp_random() is only a true hardware RNG while an
// entropy source is active. AURORA keeps Wi-Fi/Bluetooth disabled, therefore
// the internal SAR-ADC entropy source must be enabled explicitly.
// Balanced ownership: a short cryptographic read must not disable the source
// still owned by the entropy collector. Task context only; not an ISR API.
void hardwareRngEnable();
void hardwareRngDisable();

inline void hardwareRandomFill(void *buffer, size_t length) {
  hardwareRngEnable();
  esp_fill_random(buffer, length);
  hardwareRngDisable();
}

inline uint32_t hardwareRandom32() {
  uint32_t value = 0;
  hardwareRandomFill(&value, sizeof(value));
  return value;
}
