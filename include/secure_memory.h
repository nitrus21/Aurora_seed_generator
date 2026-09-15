#pragma once
#include <stddef.h>
#include <stdint.h>

#if defined(AURORA_BOARD_P4) && defined(ESP_PLATFORM)
#ifdef __cplusplus
extern "C" {
#endif
void auroraSecureZero(void *ptr, size_t len);
#ifdef __cplusplus
}
#endif
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

static inline void secureZero(void *ptr, size_t len) {
#if defined(AURORA_BOARD_P4) && defined(ESP_PLATFORM)
  auroraSecureZero(ptr, len);
#else
  volatile uint8_t *p = (volatile uint8_t *)ptr;
  while (len--) *p++ = 0;
#if defined(__GNUC__)
  __asm__ __volatile__("" ::: "memory");
#elif defined(_MSC_VER)
  _ReadWriteBarrier();
#endif
#endif
}
