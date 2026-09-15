#include "security_memory.h"
#include "secure_lvgl_memory.h"
#include "esp_cache.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_private/system_internal.h"
#include "esp32p4/rom/cache.h"
#include "riscv/rv_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

static AuroraEmergencyWipe emergency_wipe;
static volatile bool terminal_cleanup;

void auroraSecuritySetEmergencyWipe(AuroraEmergencyWipe callback) {
    emergency_wipe = callback; // Registered once, before starting the UI task.
}

void auroraSecureZero(void *ptr, size_t len) {
    if (!len) return;
    if (!ptr) auroraSecurityPanic();
    volatile uint8_t *bytes = (volatile uint8_t *)ptr;
    for (size_t i = 0; i < len; ++i) bytes[i] = 0;
    __asm__ __volatile__("" ::: "memory");
    // Terminal cleanup has stopped the other CPU. Do not acquire an SDK lock
    // which it might have owned: the panic handler flushes ROM caches directly.
    if (terminal_cleanup) return;
    // RTC/LP RAM is not cached. All application secret allocations use DRAM or
    // PSRAM; never send an uncached address to the cache API.
    if (esp_ptr_in_dram(ptr) || esp_ptr_external_ram(ptr)) {
        // Writeback ONLY, never invalidate adjacent live bytes. CPU-owned
        // ordinary allocations/stack can share a line. DMA buffers must have
        // relinquished ownership before this function is used on them.
        // Bound each SDK critical section for large PSRAM buffers (AEZEED/boot).
        for (size_t offset = 0; offset < len;) {
            size_t chunk = len - offset;
            if (chunk > 65536) chunk = 65536;
            if (esp_cache_msync((uint8_t *)ptr + offset, chunk,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA |
                    ESP_CACHE_MSYNC_FLAG_UNALIGNED) != ESP_OK)
                auroraSecurityPanic();
            offset += chunk;
        }
    }
}

void auroraSecurityPanic(void) {
    // This is the LVGL assertion/fatal-cleanup path, not a general recovery
    // from corrupted arbitrary RAM. No GUI calls, allocation, serial output or
    // normal shutdown hooks are safe here. A busy allocation registry is not
    // traversed; boot cleanup remains the fallback for unowned blocks.
    rv_utils_intr_global_disable();
    const bool frozen = auroraUiTryFreezeAllocations();
#if !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
    esp_cpu_stall(esp_cpu_get_core_id() == 0 ? 1 : 0);
#endif
    terminal_cleanup = true;
    if (emergency_wipe) emergency_wipe();
    if (frozen) auroraUiWipeFrozenAllocations();
    __asm__ __volatile__("fence rw,rw" ::: "memory");
    Cache_WriteBack_All(CACHE_MAP_L1_DCACHE);
    Cache_WriteBack_All(CACHE_MAP_L2_CACHE);
    esp_restart_noos();
}

typedef struct ScrubBlock {
    struct ScrubBlock *next;
    size_t size;
} ScrubBlock;

static bool scrubOwnedHeap(uint32_t caps, size_t *total) {
    ScrubBlock *blocks = NULL;
    bool ok = true;
    // Holding each allocation until the end prevents repeatedly wiping the
    // same free block. Only allocated payloads are touched, never TLSF metadata
    // or another task's live stack. Tiny unusable heap tails remain excluded.
    for (unsigned count = 0; count < 256; ++count) {
        size_t size = heap_caps_get_largest_free_block(caps);
        if (size < sizeof(ScrubBlock)) break;
        ScrubBlock *block = heap_caps_malloc(size, caps);
        if (!block) { ok = false; break; }
        auroraSecureZero(block, size);
        block->next = blocks;
        block->size = size;
        blocks = block;
        *total += size;
        if (count == 255) ok = false; // Fail closed if fragmentation exceeds bound.
    }
    while (blocks) {
        ScrubBlock *next = blocks->next;
        // Includes the temporary chain metadata before returning ownership.
        auroraSecureZero(blocks, sizeof(*blocks));
        heap_caps_free(blocks);
        blocks = next;
    }
    return ok;
}

bool auroraStartupMemoryScrub(size_t *internalBytes, size_t *externalBytes) {
    if (!internalBytes || !externalBytes) return false;
    *internalBytes = *externalBytes = 0;
    if (!scrubOwnedHeap(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, externalBytes)) return false;
    if (!scrubOwnedHeap(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, internalBytes)) return false;
    return *internalBytes != 0 && *externalBytes != 0;
}
