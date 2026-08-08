#pragma once
#include <stddef.h>
#include <stdint.h>

inline void secureZero(void *ptr, size_t len) {
  volatile uint8_t *p = static_cast<volatile uint8_t *>(ptr);
  while (len--) *p++ = 0;
  __asm__ __volatile__("" ::: "memory");
}

