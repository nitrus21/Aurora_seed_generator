#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
uint32_t esp_random(void);
void esp_fill_random(void *, size_t);
void bootloader_random_enable(void);
void bootloader_random_disable(void);
#ifdef __cplusplus
}
#endif
