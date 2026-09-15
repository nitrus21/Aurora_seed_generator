/* Real P4 production C included with SDK/hardware mocks. Public fixtures only.
 * No device/SD access; no claims about physical writeback or power-loss timing. */
#include "test_sdk.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *testUiMalloc(size_t size);
static void testUiFree(void *pointer);
#include "../../targets/waveshare_p4/main/security_memory.c"
#define malloc testUiMalloc
#define free testUiFree
#include "../../src/secure_lvgl_memory.c"
#undef malloc
#undef free

typedef struct {
    long double alignment;
    unsigned char bytes[512];
    uint32_t caps;
    size_t size;
    bool used, sdkOwned;
    unsigned takes, returns;
} Slot;
static Slot slots[270];
static size_t slotCount;
static unsigned heapCalls, failHeapCall, mallocCalls, wipes;
static bool failUiMalloc, dramPointer, externalPointer, cacheFailure;
static bool interruptsDisabled, cpuStalled, freezeAvailable;
static unsigned currentCore, msyncCalls, romCalls;
static unsigned char *expectedSyncNext;
static size_t expectedSyncRemaining;
static uint32_t romMaps[2];
static unsigned char fixedSecret[31];
static bool callbackRan;
static jmp_buf restartTarget;

static void allBytes(const void *pointer, size_t size, unsigned char value) {
    const unsigned char *bytes = pointer;
    for(size_t i = 0; i < size; ++i) assert(bytes[i] == value);
}

static void resetMock(void) {
    assert(allocations == NULL);
    memset(slots, 0, sizeof(slots));
    slotCount = heapCalls = failHeapCall = mallocCalls = wipes = 0;
    failUiMalloc = cacheFailure = interruptsDisabled = cpuStalled = callbackRan = false;
    dramPointer = freezeAvailable = true;
    externalPointer = false;
    currentCore = msyncCalls = romCalls = 0;
    expectedSyncNext = NULL; expectedSyncRemaining = 0;
    allocation_lock = 0;
    terminal_cleanup = false;
    emergency_wipe = NULL;
}

static void slot(size_t size, uint32_t caps, bool live) {
    assert(slotCount < sizeof(slots) / sizeof(slots[0]) && size <= 512);
    Slot *s = &slots[slotCount++];
    memset(s->bytes, 0xA5, sizeof(s->bytes));
    s->size = size; s->caps = caps | MALLOC_CAP_8BIT;
    s->used = s->sdkOwned = live;
}

size_t heap_caps_get_largest_free_block(uint32_t caps) {
    size_t largest = 0;
    for(size_t i = 0; i < slotCount; ++i)
        if(!slots[i].used && slots[i].caps == caps && slots[i].size > largest)
            largest = slots[i].size;
    return largest;
}

void *heap_caps_malloc(size_t size, uint32_t caps) {
    ++heapCalls;
    if(heapCalls == failHeapCall) return NULL;
    for(size_t i = 0; i < slotCount; ++i) {
        Slot *s = &slots[i];
        if(!s->used && s->caps == caps && s->size == size) {
            assert(s->takes == 0); // Detect re-scrubbing already-freed blocks.
            s->used = true; ++s->takes; return s->bytes;
        }
    }
    assert(false && "Unexpected heap allocation request");
    return NULL;
}

void heap_caps_free(void *pointer) {
    for(size_t i = 0; i < slotCount; ++i) {
        Slot *s = &slots[i];
        if(s->bytes == pointer) {
            assert(s->used && !s->sdkOwned && s->takes == 1 && s->returns == 0);
            allBytes(s->bytes, s->size, 0); // Before releasing ownership.
            allBytes(s->bytes + s->size, sizeof(s->bytes) - s->size, 0xA5);
            s->used = false; ++s->returns; return;
        }
    }
    assert(false && "Free of unowned buffer");
}

static void ownedSlotsReleased(void) {
    for(size_t i = 0; i < slotCount; ++i) {
        Slot *s = &slots[i];
        if(s->sdkOwned) { assert(s->used && !s->takes); allBytes(s->bytes, 512, 0xA5); }
        else { assert(!s->used); assert(s->takes == s->returns); }
    }
}

void testEnterCritical(portMUX_TYPE *lock) { ++*lock; }
void testExitCritical(portMUX_TYPE *lock) { assert(*lock); --*lock; }
int testTryCritical(portMUX_TYPE *lock, unsigned timeout) {
    assert(interruptsDisabled && timeout == 0);
    if(!freezeAvailable) return 0;
    ++*lock; return pdTRUE;
}
bool esp_ptr_in_dram(const void *pointer) { (void)pointer; return dramPointer; }
bool esp_ptr_external_ram(const void *pointer) { (void)pointer; return externalPointer; }
int esp_cache_msync(void *pointer, size_t size, int flags) {
    assert(!terminal_cleanup && !cpuStalled);
    assert(flags == (ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA |
        ESP_CACHE_MSYNC_FLAG_UNALIGNED));
    assert(size > 0 && size <= 65536);
    if(expectedSyncNext) {
        const size_t expectedSize = expectedSyncRemaining > 65536 ? 65536 : expectedSyncRemaining;
        assert(pointer == expectedSyncNext && size == expectedSize);
        expectedSyncNext += size; expectedSyncRemaining -= size;
    }
    allBytes(pointer, size, 0); ++msyncCalls;
    return cacheFailure ? -1 : ESP_OK;
}
void rv_utils_intr_global_disable(void) { interruptsDisabled = true; }
unsigned esp_cpu_get_core_id(void) { return currentCore; }
void esp_cpu_stall(unsigned core) { assert(interruptsDisabled && core == (currentCore ? 0 : 1)); cpuStalled = true; }
int Cache_WriteBack_All(uint32_t map) {
    assert(terminal_cleanup && cpuStalled && interruptsDisabled && romCalls < 2);
    romMaps[romCalls++] = map; return 0;
}
void esp_restart_noos(void) {
    assert(romCalls == 2 && romMaps[0] == CACHE_MAP_L1_DCACHE && romMaps[1] == CACHE_MAP_L2_CACHE);
    longjmp(restartTarget, 1);
}
static void *testUiMalloc(size_t size) { ++mallocCalls; return failUiMalloc ? NULL : malloc(size); }
static void testUiFree(void *pointer) { free(pointer); }
void auroraUiWipeAudit(const void *pointer, size_t size) { allBytes(pointer, size, 0); ++wipes; }
static void emergencyCallback(void) {
    assert(cpuStalled && interruptsDisabled && terminal_cleanup);
    auroraSecureZero(fixedSecret, sizeof(fixedSecret));
    callbackRan = true;
}

static void testStartup(void) {
    resetMock();
    slot(64, MALLOC_CAP_SPIRAM, false); slot(512, MALLOC_CAP_SPIRAM, false);
    slot(80, MALLOC_CAP_INTERNAL, false); slot(32, MALLOC_CAP_INTERNAL, false);
    slot(256, MALLOC_CAP_INTERNAL, true); // Live SDK memory must remain untouched.
    slot(1, MALLOC_CAP_SPIRAM, false); // Tiny unusable tails are explicitly excluded.
    size_t internal = 999, external = 999;
    assert(auroraStartupMemoryScrub(&internal, &external));
    assert(internal == 112 && external == 576 && heapCalls == 4 && msyncCalls == 8);
    ownedSlotsReleased();
    allBytes(slots[5].bytes, 512, 0xA5);
    assert(!auroraStartupMemoryScrub(NULL, &external));
    assert(!auroraStartupMemoryScrub(&internal, NULL));
    puts("PASS: boot visits each allocatable payload once, releases wiped blocks, excludes live SDK memory and tiny tails.");
}

static void testStartupFailures(void) {
    for(unsigned failure = 1; failure <= 4; ++failure) {
        resetMock();
        slot(128, MALLOC_CAP_SPIRAM, false); slot(64, MALLOC_CAP_SPIRAM, false);
        slot(128, MALLOC_CAP_INTERNAL, false); slot(64, MALLOC_CAP_INTERNAL, false);
        failHeapCall = failure;
        size_t internal, external;
        assert(!auroraStartupMemoryScrub(&internal, &external));
        assert(heapCalls == failure);
        ownedSlotsReleased();
    }
    resetMock();
    slot(32, MALLOC_CAP_INTERNAL, false);
    size_t internal, external;
    assert(!auroraStartupMemoryScrub(&internal, &external)); // No PSRAM is fail-closed.
    ownedSlotsReleased();
    resetMock();
    for(unsigned i = 0; i < 257; ++i) slot(32, MALLOC_CAP_SPIRAM, false);
    assert(!auroraStartupMemoryScrub(&internal, &external));
    assert(heapCalls == 256 && external == 256 * 32 && !internal);
    ownedSlotsReleased();
    puts("PASS: allocation failures and fragmentation bound fail closed and release all successfully owned blocks.");
}

static void testZero(void) {
    resetMock();
    unsigned char guarded[19]; memset(guarded, 0xA5, sizeof(guarded));
    auroraSecureZero(guarded + 3, 11);
    allBytes(guarded, 3, 0xA5); allBytes(guarded + 3, 11, 0); allBytes(guarded + 14, 5, 0xA5);
    assert(msyncCalls == 1);
    auroraSecureZero(NULL, 0); assert(msyncCalls == 1);
    dramPointer = false;
    auroraSecureZero(guarded, 3); assert(msyncCalls == 1); // Uncached memory does not call cache SDK.
    externalPointer = true;
    auroraSecureZero(guarded + 14, 5); assert(msyncCalls == 2);
    static unsigned char large[131089];
    memset(large, 0xA5, sizeof(large));
    expectedSyncNext = large + 3; expectedSyncRemaining = 131077;
    auroraSecureZero(large + 3, 131077);
    assert(msyncCalls == 5 && expectedSyncRemaining == 0);
    allBytes(large, 3, 0xA5); allBytes(large + 3, 131077, 0);
    allBytes(large + 131080, 9, 0xA5);
    expectedSyncNext = NULL;
    puts("PASS: zeroing touches only requested bytes, C2M never invalidates, uncached and zero-length paths avoid cache calls.");
    puts("PASS: large unaligned buffer requests contiguous cache writebacks, each at most 64 KiB, through the final partial chunk.");
}

static void testPanic(bool freeze) {
    resetMock();
    currentCore = freeze ? 0 : 1;
    char *first = auroraUiAlloc(21), *middle = auroraUiAlloc(11), *last = auroraUiAlloc(29);
    assert(first && middle && last);
    memset(first, 0x42, 21); memset(middle, 0x43, 11); memset(last, 0x44, 29);
    auroraUiFree(middle); // Check removal in the middle of the registry.
    failUiMalloc = true;
    assert(!auroraUiRealloc(first, 40)); allBytes(first, 21, 0x42);
    failUiMalloc = false;
    memset(fixedSecret, 0x45, sizeof(fixedSecret));
    auroraSecuritySetEmergencyWipe(emergencyCallback);
    freezeAvailable = freeze;
    const unsigned priorMallocs = mallocCalls, priorSyncs = msyncCalls;
    if(setjmp(restartTarget) == 0) auroraSecurityPanic();
    assert(callbackRan && mallocCalls == priorMallocs && msyncCalls == priorSyncs);
    allBytes(fixedSecret, sizeof(fixedSecret), 0);
    allBytes(first, 21, freeze ? 0 : 0x42); allBytes(last, 29, freeze ? 0 : 0x44);
    // Test-only reset of terminal state; production never resumes this process.
    terminal_cleanup = false; cpuStalled = false; allocation_lock = 0;
    auroraUiFree(first); auroraUiFree(last);
    assert(allocations == NULL && wipes == 3);
}

static void testFatalZero(void) {
    for(unsigned failure = 0; failure < 2; ++failure) {
        resetMock();
        static unsigned char value[4];
        memset(value, 0x42, sizeof(value));
        cacheFailure = failure == 1;
        if(setjmp(restartTarget) == 0) {
            if(failure) auroraSecureZero(value, sizeof(value));
            else auroraSecureZero(NULL, 4);
            assert(false && "Fatal zero error must not return");
        }
        if(failure) allBytes(value, sizeof(value), 0);
        assert(terminal_cleanup && romCalls == 2);
    }
    resetMock();
    puts("PASS: null nonempty input and cache-sync failure enter terminal reset; both CPU choices and registry-busy fallback tested.");
}

int main(void) {
    testStartup(); testStartupFailures(); testZero();
    testPanic(true); testPanic(false); testFatalZero();
    puts("PASS: emergency callback and frozen registry wipe, no GUI/allocation/SDK-cache-lock calls after CPU stall.");
    puts("LIMIT: SDK stubs cannot establish physical cache writeback, DMA quiescence, watchdog timing or power-cut erasure.");
    return 0;
}
