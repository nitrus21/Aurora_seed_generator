#pragma once
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void *auroraUiAlloc(size_t size);
void *auroraUiRealloc(void *pointer, size_t size);
void auroraUiFree(void *pointer);
#if defined(AURORA_BOARD_P4)
// Terminal cleanup only: after a successful freeze there is no resume/unlock.
bool auroraUiTryFreezeAllocations(void);
void auroraUiWipeFrozenAllocations(void);
#endif
#ifdef __cplusplus
}
#endif
