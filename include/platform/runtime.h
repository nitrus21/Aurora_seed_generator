#pragma once

#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

inline uint32_t millis() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }
inline uint32_t micros() { return static_cast<uint32_t>(esp_timer_get_time()); }
inline void delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms) ? pdMS_TO_TICKS(ms) : 1); }
template <typename T> inline T min(T a, T b) { return a < b ? a : b; }
template <typename T> inline T max(T a, T b) { return a > b ? a : b; }
struct AuroraSerial {
  template <typename... T> void printf(const char *format, T... args) { ::printf(format, args...); }
};
static AuroraSerial Serial;
#else
#include <Arduino.h>
#endif
