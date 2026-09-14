#pragma once
#include <cstring>
inline size_t strlcat(char *out, const char *value, size_t size) {
  const size_t used = strnlen(out, size), length = strlen(value);
  if (used < size) {
    const size_t copy = length < size - used - 1 ? length : size - used - 1;
    memcpy(out + used, value, copy); out[used + copy] = 0;
  }
  return used + length;
}
#include "../../native/stubs/Arduino.h"
#include <cstdio>
#include <cstdlib>
#include <new>
#if defined(_MSC_VER)
#define __attribute__(...)
#define __asm__
#define __volatile__(...)
#define strtok_r strtok_s
#endif
inline size_t strlcpy(char *out, const char *in, size_t size) {
  if (size) { strncpy(out, in, size - 1); out[size - 1] = 0; }
  return strlen(in);
}
inline void delay(uint32_t ms) { mock.time += ms * 1000; }
struct UiSerial { template<typename... T> void printf(const char *, T...) {} };
inline UiSerial Serial;
