#pragma once
#include "Arduino.h"
inline uint32_t esp_random() {
  assert(mock.rngEnabled && mock.adcStep == 0);
  return (0x12345678u + 0x01020304u * ++mock.rngReads) ^ mock.rngSalt;
}
inline void esp_fill_random(void *buffer, size_t length) {
  assert(mock.rngEnabled && length == 32);
  auto bytes = static_cast<uint8_t *>(buffer);
  mock.previewKey = bytes;
  for (size_t i = 0; i < length; ++i) bytes[i] = static_cast<uint8_t>(i + 1 + mock.previewSalt);
}
