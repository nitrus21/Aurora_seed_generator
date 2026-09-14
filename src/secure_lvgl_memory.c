#include "secure_lvgl_memory.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lvgl.h>

// Wipe every LVGL allocation before release, including previous textarea and
// label copies discarded on editing/reallocation. Never overwrite other tasks'
// heap or claim to sanitize the entire device's RAM/PSRAM/cache/framebuffers.
typedef union {
    size_t size;
    long double alignment;
    void *pointer_alignment;
    uint64_t integer_alignment;
} AuroraUiHeader;

void *auroraUiAlloc(size_t size) {
    if(size > SIZE_MAX - sizeof(AuroraUiHeader)) return NULL;
    AuroraUiHeader *header = (AuroraUiHeader *)malloc(sizeof(AuroraUiHeader) + size);
    if(!header) return NULL;
    header->size = size;
    return header + 1;
}

void auroraUiFree(void *pointer) {
    if(!pointer) return;
    AuroraUiHeader *header = (AuroraUiHeader *)pointer - 1;
    const size_t total = sizeof(*header) + header->size;
    volatile uint8_t *bytes = (volatile uint8_t *)header;
    for(size_t i = 0; i < total; ++i) bytes[i] = 0;
#ifdef AURORA_MEMORY_TEST
    extern void auroraUiWipeAudit(const void *, size_t);
    auroraUiWipeAudit(header, total); // Inspect BEFORE free, never freed memory.
#endif
    free(header);
}

void *auroraUiRealloc(void *pointer, size_t size) {
    if(!size) { auroraUiFree(pointer); return NULL; }
    if(!pointer) return auroraUiAlloc(size);
    AuroraUiHeader *header = (AuroraUiHeader *)pointer - 1;
    void *replacement = auroraUiAlloc(size);
    if(!replacement) return NULL; // Original allocation remains owned/intact.
    memcpy(replacement, pointer, size < header->size ? size : header->size);
    auroraUiFree(pointer);
    return replacement;
}

#if LVGL_VERSION_MAJOR >= 9 && LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM
void lv_mem_init(void) {}
void lv_mem_deinit(void) {}
lv_mem_pool_t lv_mem_add_pool(void *memory, size_t bytes) { (void)memory; (void)bytes; return NULL; }
void lv_mem_remove_pool(lv_mem_pool_t pool) { (void)pool; }
void *lv_malloc_core(size_t size) { return auroraUiAlloc(size); }
void *lv_realloc_core(void *pointer, size_t size) { return auroraUiRealloc(pointer, size); }
void lv_free_core(void *pointer) { auroraUiFree(pointer); }
void lv_mem_monitor_core(lv_mem_monitor_t *monitor) { memset(monitor, 0, sizeof(*monitor)); }
lv_result_t lv_mem_test_core(void) { return LV_RESULT_OK; }
#endif
