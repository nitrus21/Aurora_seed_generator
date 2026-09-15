#if defined(AURORA_BOARD_CYD)
#include "cyd_security.h"
#include "secure_memory.h"
#include "secure_lvgl_memory.h"
#include "board_config.h"
#include <esp_attr.h>
#include <esp_partition.h>
#include <esp_heap_caps.h>
#include <esp_private/system_internal.h>
#include <soc/cpu.h>
#include <soc/gpio_struct.h>
#include <freertos/FreeRTOS.h>

// The prebuilt Arduino SDK enables crash dumps. Link wrapping, removal of the
// dump partition AND a binary gate are needed: a -D flag alone cannot fix it.
extern "C" void IRAM_ATTR __wrap_esp_core_dump_init(void) {}
extern "C" void IRAM_ATTR __wrap_esp_core_dump_to_flash(void *) {}
extern "C" void IRAM_ATTR __wrap_esp_core_dump_to_uart(void *) {}
extern "C" void IRAM_ATTR __wrap_esp_panic_handler(void *) {
  // Arbitrary corruption/cache-off context: do not chase heap/UI pointers.
  // No register/stack UART dump, no flash write, no shutdown callbacks.
  GPIO.out_w1tc = 1U << AURORA_BACKLIGHT_PIN;
  esp_restart_noos();
}

extern "C" void auroraCydUiFailure(void) {
  // Only LVGL's normal task-context assertions use this owned-memory cleanup.
  portDISABLE_INTERRUPTS();
  const bool frozen = auroraUiTryFreezeAllocations();
  esp_cpu_stall(xPortGetCoreID() == 0 ? 1 : 0);
  GPIO.out_w1tc = 1U << AURORA_BACKLIGHT_PIN;
  auroraCydWipeApplication();
  if (frozen) auroraUiWipeFrozenAllocations();
  esp_restart_noos();
}

extern "C" bool auroraCydBootCleanup(void) {
  // The old 1.9.2 dump can survive an application-only upload. Validate the
  // complete partition identity before touching exactly its reserved region.
  const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
      static_cast<esp_partition_subtype_t>(0x40), "aurora_scrub");
  if (!part || part->address != 0x3F0000 || part->size != 0x10000 || part->encrypted)
    return false; // Old/mismatched partition table: never open a wallet.
  uint8_t check[256];
  bool dirty = false;
  for (size_t offset = 0; offset < part->size; offset += sizeof(check)) {
    if (esp_partition_read(part, offset, check, sizeof(check)) != ESP_OK) {
      secureZero(check, sizeof(check)); return false;
    }
    for (uint8_t b : check) dirty |= b != 0xff;
    secureZero(check, sizeof(check));
  }
  if (dirty && esp_partition_erase_range(part, 0, part->size) != ESP_OK) return false;
  for (size_t offset = 0; offset < part->size; offset += sizeof(check)) {
    if (esp_partition_read(part, offset, check, sizeof(check)) != ESP_OK) {
      secureZero(check, sizeof(check)); return false;
    }
    bool erased = true;
    for (uint8_t b : check) erased &= b == 0xff;
    secureZero(check, sizeof(check));
    if (!erased) return false;
  }
  // Scrub only successfully allocated free heap, never live SDK tasks/stacks.
  struct Block { Block *next; };
  Block *head = nullptr;
  bool ok = true;
  for (unsigned n = 0; n < 256; ++n) {
    const size_t size = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (size < sizeof(Block)) break;
    Block *b = static_cast<Block *>(heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
    if (!b) { ok = false; break; }
    secureZero(b, size);
    b->next = head; head = b;
    if (n == 255) ok = false;
  }
  while (head) { Block *next = head->next; secureZero(head, sizeof(*head)); heap_caps_free(head); head = next; }
  return ok;
}
#endif
