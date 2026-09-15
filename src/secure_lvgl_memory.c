#include "secure_lvgl_memory.h"
#include "secure_memory.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lvgl.h>

// Wipe every LVGL allocation before release, including previous textarea and
// label copies discarded on editing/reallocation. Never overwrite other tasks'
// heap or claim to sanitize the entire device's RAM/PSRAM/cache/framebuffers.
typedef union AuroraUiHeader {
#if defined(AURORA_BOARD_P4)
    struct {
        size_t size;
        union AuroraUiHeader *previous;
        union AuroraUiHeader *next;
    } owned;
#else
    size_t size;
#endif
    long double alignment;
    void *pointer_alignment;
    uint64_t integer_alignment;
} AuroraUiHeader;

#if defined(AURORA_BOARD_P4)
static AuroraUiHeader *allocations;
#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
static portMUX_TYPE allocation_lock = portMUX_INITIALIZER_UNLOCKED;
#define ALLOCATION_LOCK() portENTER_CRITICAL(&allocation_lock)
#define ALLOCATION_UNLOCK() portEXIT_CRITICAL(&allocation_lock)
#else
#define ALLOCATION_LOCK() ((void)0)
#define ALLOCATION_UNLOCK() ((void)0)
#endif
#define ALLOCATION_SIZE(header) ((header)->owned.size)

bool auroraUiTryFreezeAllocations(void) {
#if defined(ESP_PLATFORM)
    return portTRY_ENTER_CRITICAL(&allocation_lock, 0) == pdTRUE;
#else
    return true;
#endif
}

void auroraUiWipeFrozenAllocations(void) {
    // The caller has frozen allocation ownership and stopped other tasks. Keep
    // headers intact for traversal: only payloads can contain UI text/pixels.
    for (AuroraUiHeader *header = allocations; header; header = header->owned.next)
        secureZero(header + 1, header->owned.size);
}
#else
#define ALLOCATION_SIZE(header) ((header)->size)
#endif

void *auroraUiAlloc(size_t size) {
    if(size > SIZE_MAX - sizeof(AuroraUiHeader)) return NULL;
    AuroraUiHeader *header = (AuroraUiHeader *)malloc(sizeof(AuroraUiHeader) + size);
    if(!header) return NULL;
    ALLOCATION_SIZE(header) = size;
#if defined(AURORA_BOARD_P4)
    ALLOCATION_LOCK();
    header->owned.previous = NULL;
    header->owned.next = allocations;
    if (allocations) allocations->owned.previous = header;
    allocations = header;
    ALLOCATION_UNLOCK();
#endif
    return header + 1;
}

void auroraUiFree(void *pointer) {
    if(!pointer) return;
    AuroraUiHeader *header = (AuroraUiHeader *)pointer - 1;
    const size_t total = sizeof(*header) + ALLOCATION_SIZE(header);
#if defined(AURORA_BOARD_P4)
    // Keep the payload discoverable until it has actually been erased. Registry
    // metadata never contains secrets; remove it only after payload writeback.
    secureZero(pointer, ALLOCATION_SIZE(header));
    ALLOCATION_LOCK();
    if (header->owned.previous) header->owned.previous->owned.next = header->owned.next;
    else allocations = header->owned.next;
    if (header->owned.next) header->owned.next->owned.previous = header->owned.previous;
    ALLOCATION_UNLOCK();
    secureZero(header, sizeof(*header));
#else
    secureZero(header, total);
#endif
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
    memcpy(replacement, pointer, size < ALLOCATION_SIZE(header) ? size : ALLOCATION_SIZE(header));
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
