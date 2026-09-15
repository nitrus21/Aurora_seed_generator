// Real LVGL, public dummy text only. No wallet, microSD or hardware access.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <lvgl.h>

static bool failNextReallocation;
static bool assertionReached;
static const char *expectedOwnedAllocation;
static bool expectedAllocationWiped;
static size_t inspectedWipes;

extern "C" void auroraTestAssertionReached(void) { assertionReached = true; }
extern "C" void *auroraTestTextAreaRealloc(void *pointer, size_t size) {
    if(failNextReallocation) {
        failNextReallocation = false;
        return nullptr;
    }
    return lv_realloc(pointer, size);
}

extern "C" void auroraUiWipeAudit(const void *pointer, size_t size) {
    const auto *bytes = static_cast<const uint8_t *>(pointer);
    for(size_t i = 0; i < size; ++i) assert(bytes[i] == 0);
    ++inspectedWipes;
    const uintptr_t begin = reinterpret_cast<uintptr_t>(pointer);
    const uintptr_t expected = reinterpret_cast<uintptr_t>(expectedOwnedAllocation);
    if(expectedOwnedAllocation && expected >= begin && expected < begin + size)
        expectedAllocationWiped = true; // Inspection occurs BEFORE free.
}

static void flush(lv_display_t *display, const lv_area_t *, uint8_t *) {
    lv_display_flush_ready(display);
}

static void exercise(unsigned operation) {
    static const char fixture[] = "public-fixture-password";
    lv_obj_t *area = lv_textarea_create(lv_screen_active());
    lv_textarea_set_password_mode(area, true);
    lv_textarea_set_max_length(area, 0);
    lv_textarea_set_text(area, fixture);
    lv_textarea_set_cursor_pos(area, LV_TEXTAREA_CURSOR_LAST);
    expectedOwnedAllocation = lv_textarea_get_text(area);
    expectedAllocationWiped = false;
    assert(!strcmp(expectedOwnedAllocation, fixture));
    assertionReached = false;
    failNextReallocation = true;
    if(operation == 0) lv_textarea_add_char(area, 'x');
    else if(operation == 1) lv_textarea_add_text(area, "xy");
    else lv_textarea_delete_char(area);
    assert(assertionReached && !failNextReallocation);
    // The failed realloc never freed the original; this is NOT a freed read.
    assert(lv_textarea_get_text(area) == expectedOwnedAllocation);
    const char *expected = operation == 2 ? "public-fixture-passwor" : fixture;
    assert(!strcmp(expectedOwnedAllocation, expected));
    lv_obj_delete(area);
    assert(expectedAllocationWiped);
    expectedOwnedAllocation = nullptr;
}

int main() {
    lv_init();
    lv_display_t *display = lv_display_create(480, 800);
    static uint16_t drawBuffer[480 * 20];
    lv_display_set_buffers(display, drawBuffer, nullptr, sizeof(drawBuffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    for(unsigned operation = 0; operation < 3; ++operation) exercise(operation);
    lv_display_delete(display);
    printf("PASS: all three textarea realloc OOM paths retain ownership and wipe the allocation on teardown.\n");
    printf("PASS: %zu allocator wipes checked before free.\n", inspectedWipes);
    puts("LIMIT: native deterministic fault injection; ESP emergency reset is validated separately.");
    return 0;
}
