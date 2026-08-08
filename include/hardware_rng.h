#pragma once

#include <stddef.h>
#include <stdint.h>
#include <esp_system.h>
#include <bootloader_random.h>

// On the original ESP32, esp_random() is only a true hardware RNG while an
// entropy source is active. AURORA keeps Wi-Fi/Bluetooth disabled, therefore
// the internal SAR-ADC entropy source must be enabled explicitly.
inline void hardwareRngEnable() {
  bootloader_random_enable();
}

inline void hardwareRngDisable() {
  bootloader_random_disable();
}

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
