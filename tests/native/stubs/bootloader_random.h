#pragma once
#include "Arduino.h"
inline void bootloader_random_enable() {
  assert(!mock.rngEnabled && mock.adcStep == 0);
  mock.rngEnabled = true;
}
inline void bootloader_random_disable() {
  assert(mock.rngEnabled && mock.adcStep == 0);
  mock.rngEnabled = false;
}
