#pragma once
#include <cstdlib>
#define MALLOC_CAP_SPIRAM 1
inline void *heap_caps_calloc(size_t count, size_t size, unsigned) { return calloc(count, size); }
