#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *auroraUiAlloc(size_t size);
void *auroraUiRealloc(void *pointer, size_t size);
void auroraUiFree(void *pointer);
#ifdef __cplusplus
}
#endif
