#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define __attribute__(...)
/* Native tests validate C control flow and writes, not RISC-V fences/caches. */
#define __asm__
#define __volatile__(...)
#define CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE 0
#define ESP_OK 0
#define ESP_CACHE_MSYNC_FLAG_DIR_C2M 1
#define ESP_CACHE_MSYNC_FLAG_TYPE_DATA 2
#define ESP_CACHE_MSYNC_FLAG_UNALIGNED 4
#define ESP_CACHE_MSYNC_FLAG_INVALIDATE 8
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_INTERNAL 2
#define MALLOC_CAP_8BIT 4
#define CACHE_MAP_L1_DCACHE 16
#define CACHE_MAP_L2_CACHE 32
#define LVGL_VERSION_MAJOR 0
typedef unsigned portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define pdTRUE 1
#define portENTER_CRITICAL(lock) testEnterCritical(lock)
#define portEXIT_CRITICAL(lock) testExitCritical(lock)
#define portTRY_ENTER_CRITICAL(lock, timeout) testTryCritical(lock, timeout)
void testEnterCritical(portMUX_TYPE *lock);
void testExitCritical(portMUX_TYPE *lock);
int testTryCritical(portMUX_TYPE *lock, unsigned timeout);
int esp_cache_msync(void *pointer, size_t size, int flags);
bool esp_ptr_in_dram(const void *pointer);
bool esp_ptr_external_ram(const void *pointer);
void rv_utils_intr_global_disable(void);
unsigned esp_cpu_get_core_id(void);
void esp_cpu_stall(unsigned core);
int Cache_WriteBack_All(uint32_t map);
void esp_restart_noos(void);
size_t heap_caps_get_largest_free_block(uint32_t caps);
void *heap_caps_malloc(size_t size, uint32_t caps);
void heap_caps_free(void *pointer);
